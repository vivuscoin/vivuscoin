#!/usr/bin/env python3
# Copyright (c) 2026 The Vivuscoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the v1.1 consensus changes on regtest.

 - activation: legacy (no-retarget) rule below -lwmaactivationheight, LWMA-1
   at and above it, checked block by block against a Python port of the
   task 05 reference simulator (05-sim/sim.py) using regtest parameters;
 - stall recovery: emergency easing steps from the candidate timestamp,
   powLimit cap, and the reduced window credit of eased blocks;
 - nBits depends on the block's own time (strict equality, bad-diffbits);
 - future time limit of 720 s (time-too-new);
 - rolling finality: reorgs deeper than -maxreorgdepth are refused outside
   IBD; recovery via invalidateblock or -maxreorgdepth=-1.
"""
import os
import time

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import ToHex
from test_framework.test_framework import VivuscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    connect_nodes_bi,
    disconnect_nodes,
    set_node_times,
    sync_blocks,
    wait_until,
)

# Regtest parameters (chainparams.cpp).
T = 600                       # nPowTargetSpacing
N = 60                        # nLwmaWindow
MAX_SOLVE = 6 * T             # nLwmaMaxSolvetimeMult * T
REF_SPAN = 11                 # nEmergencyRefSpan
TRIGGER = 6 * T               # nEmergencyTriggerMult * T
MAX_STEPS = 24                # nEmergencyMaxSteps
CREDIT_CAP = 6                # nEmergencyCreditCapSteps
POWLIMIT_COMPACT = 0x207fffff
ACTIVATION = 200
MAX_REORG_DEPTH = 10
FTL = 720                     # MAX_FUTURE_BLOCK_TIME


# ---- reference implementation, ported line for line from 05-sim/sim.py ----
def set_compact(c):
    size = c >> 24
    word = c & 0x007fffff
    if size <= 3:
        return word >> (8 * (3 - size))
    return word << (8 * (size - 3))


def get_compact(x):
    if x == 0:
        return 0
    size = (x.bit_length() + 7) // 8
    if size <= 3:
        comp = x << (8 * (3 - size))
    else:
        comp = x >> (8 * (size - 3))
    if comp & 0x00800000:
        comp >>= 8
        size += 1
    return comp | (size << 24)


POWLIMIT = set_compact(POWLIMIT_COMPACT)


def round_target(t):
    return set_compact(get_compact(min(t, POWLIMIT)))


class Reference:
    """Header chain (time, nBits) and the v1.1 rules as sim.py models them."""

    def __init__(self):
        self.times = []
        self.bits = []

    def append(self, t, nbits):
        self.times.append(t)
        self.bits.append(nbits)

    def height(self):
        return len(self.times) - 1

    def ref_time(self, idx):
        lo = max(0, idx - REF_SPAN + 1)
        return max(self.times[lo:idx + 1])

    def steps(self, idx, t_cand):
        """Easing steps of a candidate with timestamp t_cand on top of block idx."""
        if idx + 1 < ACTIVATION:
            return 0
        gap = t_cand - self.ref_time(idx)
        if gap < TRIGGER:
            return 0
        return min(MAX_STEPS, 1 + (gap - TRIGGER) // T)

    def credit(self, idx):
        target = set_compact(self.bits[idx])
        s = self.steps(idx - 1, self.times[idx]) if idx > 0 else 0
        shift = max(0, s - CREDIT_CAP)
        return max(1, target >> shift)

    def lwma(self, idx):
        k = N * (N + 1) * T // 2
        if idx < N:
            return POWLIMIT
        prev_ts = self.times[idx - N]
        sum_w = 0
        avg = 0
        j = 0
        for i in range(idx - N + 1, idx + 1):
            this_ts = self.times[i] if self.times[i] > prev_ts else prev_ts + 1
            solve = min(MAX_SOLVE, this_ts - prev_ts)
            prev_ts = this_ts
            j += 1
            sum_w += solve * j
            avg += self.credit(i) // N // k
        nxt = avg * sum_w
        if nxt > POWLIMIT:
            nxt = POWLIMIT
        return round_target(nxt)

    def legacy(self, idx, t_cand):
        """pow.cpp legacy path on regtest: fPowAllowMinDifficultyBlocks with the
        regtest genesis at 0x1d00ffff, so only blocks more than 2T after their
        parent get powLimit; anything faster walks back to a real-difficulty
        block (the genesis) and is unmineable here.  fPowNoRetargeting makes
        the 2016-block retarget a no-op."""
        if t_cand > self.times[idx] + 2 * T:
            return POWLIMIT_COMPACT
        i = idx
        while i > 0 and i % 2016 != 0 and self.bits[i] == POWLIMIT_COMPACT:
            i -= 1
        return self.bits[i]

    def required(self, idx, t_cand):
        """nBits required for a candidate with timestamp t_cand on top of block idx."""
        if idx + 1 < ACTIVATION:
            return self.legacy(idx, t_cand)
        base = self.lwma(idx)
        s = self.steps(idx, t_cand)
        tgt = base * (2 ** s)
        if tgt > POWLIMIT:
            tgt = POWLIMIT
        return get_compact(round_target(tgt))


class LwmaV11Test(VivuscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.base_args = ['-lwmaactivationheight=%d' % ACTIVATION, '-maxreorgdepth=%d' % MAX_REORG_DEPTH]
        # node2 stays disconnected and, with -maxtipage=1, permanently in IBD until the finality test needs it
        self.extra_args = [self.base_args, self.base_args, self.base_args + ['-maxtipage=1']]

    def setup_network(self):
        self.setup_nodes()
        connect_nodes_bi(self.nodes, 0, 1)
        self.sync_all([self.nodes[0:2]])

    # ---- helpers -----------------------------------------------------------
    def record(self, node, blockhash):
        hdr = node.getblockheader(blockhash)
        assert_equal(hdr['height'], self.ref.height() + 1)
        nbits = int(hdr['bits'], 16)
        expected = self.ref.required(self.ref.height(), hdr['time'])
        assert_equal('%08x' % nbits, '%08x' % expected)
        self.ref.append(hdr['time'], nbits)
        return hdr

    def mine(self, node, count, spacing):
        """Mine count blocks on node, advancing the mock clock by spacing before each one."""
        hashes = []
        for _ in range(count):
            self.t += spacing
            set_node_times(self.nodes, self.t)
            h = node.generatetoaddress(1, self.addr)[0]
            self.record(node, h)
            hashes.append(h)
        return hashes

    def make_block(self, node, ntime, nbits=None):
        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        block = create_block(int(tip, 16), create_coinbase(height), ntime)
        block.nBits = nbits if nbits is not None else self.ref.required(self.ref.height(), ntime)
        block.solve()
        return block

    def target(self, nbits):
        return set_compact(nbits)

    # ---- test -----------------------------------------------------------------
    def run_test(self):
        node0, node1, node2 = self.nodes
        self.addr = node0.get_deterministic_priv_key().address
        self.ref = Reference()
        genesis = node0.getblockheader(node0.getblockhash(0))
        self.ref.append(genesis['time'], int(genesis['bits'], 16))
        self.t = int(time.time()) - 3000 * T
        set_node_times(self.nodes, self.t)

        self.log.info("Legacy rule below activation (regtest min-difficulty rule, no retarget)")
        # Legacy regtest blocks are only mineable more than 2T after their parent (see Reference.legacy).
        self.mine(node0, ACTIVATION - 1, 2 * T + 1)
        assert_equal(node0.getblockcount(), ACTIVATION - 1)
        for h in range(1, ACTIVATION):
            assert_equal(self.ref.bits[h], POWLIMIT_COMPACT)
        # Under the legacy rule a block only T after its parent would need the genesis
        # difficulty (0x1d00ffff); under LWMA the window of slow blocks gives powLimit.
        assert_equal(self.ref.legacy(ACTIVATION - 1, self.t + T), 0x1d00ffff)
        assert_equal(self.ref.required(ACTIVATION - 1, self.t + T), POWLIMIT_COMPACT)
        # Activation must be at a height where the emergency rule cannot see any old gap.
        assert_equal(self.ref.steps(ACTIVATION - 1, self.t + T), 0)

        self.log.info("Activation at %d: LWMA governs nBits; fast blocks raise difficulty" % ACTIVATION)
        self.mine(node0, 1, T)
        assert_equal(self.ref.bits[ACTIVATION], POWLIMIT_COMPACT)
        # 25 blocks with equal timestamps (+1 s clamp) outweigh the slow legacy blocks still in the window
        self.mine(node0, 25, 0)
        bits_fast = self.ref.bits[-1]
        assert_greater_than(POWLIMIT, self.target(bits_fast))
        self.log.info("  nBits after 25 fast blocks: %08x (target / powLimit = %.3f)" % (bits_fast, self.target(bits_fast) / POWLIMIT))
        # a window of 2T-spaced blocks pushes the target back to the powLimit cap
        self.mine(node0, 60, 2 * T)
        assert_equal(self.ref.bits[-1], POWLIMIT_COMPACT)

        self.log.info("Fast blocks raise difficulty ~%dx per window (the +1 s clamp), then a stall is eased" % T)
        self.mine(node0, 55, 0)
        bits_pumped = self.ref.bits[-1]
        assert_greater_than(POWLIMIT // 20, self.target(bits_pumped))
        self.log.info("  pumped nBits: %08x (difficulty x%.1f)" % (bits_pumped, POWLIMIT / self.target(bits_pumped)))
        tip_idx = self.ref.height()
        # gap of 6T + 2T + 10 s from the max-of-11 reference -> 3 steps -> target x8
        t_stall = self.ref.ref_time(tip_idx) + TRIGGER + 2 * T + 10
        assert_equal(self.ref.steps(tip_idx, t_stall), 3)
        self.t = t_stall - T
        self.mine(node0, 1, T)
        bits_eased = self.ref.bits[-1]
        assert_equal(self.target(bits_eased), round_target(self.ref.lwma(tip_idx) * 8))
        self.log.info("  eased nBits after a %d s stall: %08x (3 steps)" % (TRIGGER + 2 * T + 10, bits_eased))
        # a 6T + 30T gap -> 31 steps, capped at 24 -> powLimit
        tip_idx = self.ref.height()
        t_stall = self.ref.ref_time(tip_idx) + TRIGGER + 30 * T
        assert_equal(self.ref.steps(tip_idx, t_stall), MAX_STEPS)
        self.t = t_stall - T
        self.mine(node0, 1, T)
        assert_equal(self.ref.bits[-1], POWLIMIT_COMPACT)
        # the 24-step block is credited at powLimit >> 18 in the window; recovery blocks must match
        assert_equal(self.ref.credit(self.ref.height()), POWLIMIT >> 18)
        self.mine(node0, 30, T)

        self.log.info("nBits is a function of the block's own timestamp (strict equality)")
        self.mine(node0, 55, 0)                      # pump again so 3 and 4 easing steps stay below powLimit
        tip_idx = self.ref.height()
        t_a = self.ref.ref_time(tip_idx) + TRIGGER + 2 * T + 10          # 3 steps
        t_b = t_a + T                                                     # 4 steps
        assert_equal(self.ref.steps(tip_idx, t_a), 3)
        assert_equal(self.ref.steps(tip_idx, t_b), 4)
        bits_a = self.ref.required(tip_idx, t_a)
        bits_b = self.ref.required(tip_idx, t_b)
        assert bits_a != bits_b
        self.t = t_b
        set_node_times(self.nodes, self.t)
        # stale template: time rolled across a step boundary without recomputing nBits -> rejected
        bad = self.make_block(node0, t_b, nbits=bits_a)
        assert_equal(node0.submitblock(ToHex(bad)), 'bad-diffbits')
        # nBits too easy for a non-stall time is rejected as well
        bad = self.make_block(node0, t_a - 3 * T, nbits=bits_a)
        assert_equal(node0.submitblock(ToHex(bad)), 'bad-diffbits')
        # a block stamped t_a with the nBits for t_a is valid even when received at t_b
        good = self.make_block(node0, t_a, nbits=bits_a)
        assert_equal(node0.submitblock(ToHex(good)), None)
        assert_equal(node0.getbestblockhash(), good.hash)
        self.record(node0, good.hash)
        # getblocktemplate follows the clock: the template's bits match the reference for curtime
        tmpl = node0.getblocktemplate()
        assert_equal(int(tmpl['bits'], 16), self.ref.required(self.ref.height(), tmpl['curtime']))

        self.log.info("Future time limit: %d s" % FTL)
        self.mine(node0, 5, T)
        now = self.t
        set_node_times(self.nodes, now)
        too_new = self.make_block(node0, now + FTL + 1)
        assert_equal(node0.submitblock(ToHex(too_new)), 'time-too-new')
        assert_equal(node0.getblockcount(), self.ref.height())
        ok = self.make_block(node0, now + FTL)
        assert_equal(node0.submitblock(ToHex(ok)), None)
        self.record(node0, ok.hash)
        self.t = now + FTL
        set_node_times(self.nodes, self.t)
        self.mine(node0, 5, T)
        sync_blocks([node0, node1])
        assert_equal(node1.getbestblockhash(), node0.getbestblockhash())

        # Bring the target back to the powLimit cap so every branch block below carries
        # the same work and the longer branch is the heavier one.
        # (just under the 6T clamp: the target grows ~6x per window until it hits the cap)
        self.mine(node0, 150, 6 * T - 1)
        sync_blocks([node0, node1])
        assert_equal(self.ref.bits[-1], POWLIMIT_COMPACT)

        self.log.info("Rolling finality: a reorg of depth <= %d is accepted" % MAX_REORG_DEPTH)
        self.split()
        self.mine(node0, MAX_REORG_DEPTH, 2 * T)
        self.reload_ref(node1)
        self.mine(node1, MAX_REORG_DEPTH + 2, 2 * T)
        assert_greater_than(int(node1.getblockchaininfo()['chainwork'], 16), int(node0.getblockchaininfo()['chainwork'], 16))
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks([node0, node1])
        assert_equal(node0.getbestblockhash(), node1.getbestblockhash())
        assert_equal(node0.getblockcount(), self.fork_height + MAX_REORG_DEPTH + 2)

        self.log.info("Rolling finality: a reorg of depth %d is refused" % (MAX_REORG_DEPTH + 1))
        self.split()
        branch_a = self.mine(node0, MAX_REORG_DEPTH + 1, 2 * T)
        tip_a = node0.getbestblockhash()
        self.reload_ref(node1)
        self.mine(node1, MAX_REORG_DEPTH + 3, 2 * T)
        tip_b = node1.getbestblockhash()
        assert_greater_than(int(node1.getblockchaininfo()['chainwork'], 16), int(node0.getblockchaininfo()['chainwork'], 16))
        with node0.assert_debug_log(['refusing to reorganize %d blocks' % (MAX_REORG_DEPTH + 1)]):
            connect_nodes_bi(self.nodes, 0, 1)
            self.wait_for_tip_known(node0, tip_b)
            self.wait_for_tip_known(node1, tip_a)
            time.sleep(2)
        assert_equal(node0.getbestblockhash(), tip_a)
        assert_equal(node1.getbestblockhash(), tip_b)
        assert 'refusing to reorganize' in node0.getblockchaininfo()['warnings']
        # node0 keeps refusing as the competing chain grows
        self.mine(node1, 3, 2 * T)
        tip_b = node1.getbestblockhash()
        self.wait_for_tip_known(node0, tip_b)
        time.sleep(2)
        assert_equal(node0.getbestblockhash(), tip_a)

        self.log.info("The limit does not apply in initial block download: node2 (kept in IBD by -maxtipage=1) follows the heavier chain")
        connect_nodes_bi(self.nodes, 0, 2)
        wait_until(lambda: node2.getbestblockhash() == tip_a, timeout=60)
        assert_equal(node2.getblockchaininfo()['initialblockdownload'], True)
        connect_nodes_bi(self.nodes, 1, 2)
        self.mine(node1, 1, 2 * T)
        tip_b = node1.getbestblockhash()
        wait_until(lambda: node2.getbestblockhash() == tip_b, timeout=60)
        assert_equal(node0.getbestblockhash(), tip_a)
        disconnect_nodes(node0, 2)
        disconnect_nodes(node2, 0)
        disconnect_nodes(node1, 2)
        disconnect_nodes(node2, 1)

        self.log.info("Manual recovery 1: invalidateblock on the first block of our branch")
        node0.invalidateblock(branch_a[0])
        wait_until(lambda: node0.getbestblockhash() == tip_b, timeout=30)
        node0.reconsiderblock(branch_a[0])
        assert_equal(node0.getbestblockhash(), tip_b)   # the heavier chain stays active

        self.log.info("Manual recovery 2: restart with -maxreorgdepth=-1")
        self.split()
        self.mine(node0, MAX_REORG_DEPTH + 1, 2 * T)
        tip_a = node0.getbestblockhash()
        self.reload_ref(node1)
        self.mine(node1, MAX_REORG_DEPTH + 3, 2 * T)
        tip_b = node1.getbestblockhash()
        connect_nodes_bi(self.nodes, 0, 1)
        self.wait_for_tip_known(node0, tip_b)
        time.sleep(2)
        assert_equal(node0.getbestblockhash(), tip_a)
        self.restart_node(0, extra_args=['-lwmaactivationheight=%d' % ACTIVATION, '-maxreorgdepth=-1'])
        node0.setmocktime(self.t)
        with open(os.path.join(node0.datadir, 'regtest', 'debug.log'), encoding='utf-8') as f:
            assert 'rolling finality is DISABLED' in f.read()
        wait_until(lambda: node0.getbestblockhash() == tip_b, timeout=60)
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks([node0, node1])
        self.restart_node(0, extra_args=self.base_args)
        node0.setmocktime(self.t)
        assert_equal(node0.getbestblockhash(), tip_b)

    # ---- reorg helpers ---------------------------------------------------------
    def split(self):
        """Disconnect node0 and node1 and remember the fork point."""
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[1], 0)
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        self.fork_height = self.nodes[0].getblockcount()
        self.ref_fork = (list(self.ref.times), list(self.ref.bits))

    def reload_ref(self, node):
        """Point the reference at node's active chain, re-checking every header past the fork."""
        self.ref.times, self.ref.bits = list(self.ref_fork[0]), list(self.ref_fork[1])
        assert_equal(self.ref.height(), self.fork_height)
        for h in range(self.fork_height + 1, node.getblockcount() + 1):
            self.record(node, node.getblockhash(h))

    def wait_for_tip_known(self, node, blockhash):
        wait_until(lambda: blockhash in [t['hash'] for t in node.getchaintips()], timeout=60)


if __name__ == '__main__':
    LwmaV11Test().main()
