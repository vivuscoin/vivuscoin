// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2021 The Vivuscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VIVUSCOIN_POW_H
#define VIVUSCOIN_POW_H

#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

class arith_uint256;

/**
 * Required nBits for the block after pindexLast.  Below the v1.1 activation
 * height this is the legacy every-5040-blocks retarget; from the activation
 * height on it is the LWMA-1 target with the emergency easing rule applied,
 * which makes the result depend on the candidate block's own timestamp.
 */
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);

/** Whether the v1.1 difficulty rules (LWMA-1 + emergency easing) govern a block at nHeight. */
bool IsLwmaActive(int nHeight, const Consensus::Params&);

/**
 * LWMA-1 base target (nBits) for the block after pindexLast, before any
 * emergency easing.  Zawy's reference form: linearly weighted mean of the last
 * N solvetimes (monotone +1 s minimum, 6T maximum) times the mean of the
 * window blocks' credited targets.  Returns powLimit while fewer than N+1
 * blocks exist.
 */
unsigned int LwmaNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params&);

/** Emergency rule reference time: the maximum timestamp of pindexPrev and its (nEmergencyRefSpan-1) ancestors. */
int64_t EmergencyReferenceTime(const CBlockIndex* pindexPrev, const Consensus::Params&);

/**
 * Number of emergency target doublings a block with timestamp nCandidateTime
 * on top of pindexPrev gets: 0 below the activation height or while the gap
 * to the reference time is < 6T, otherwise 1 + floor((gap - 6T) / T), capped
 * at nEmergencyMaxSteps.  Depends only on ancestors and the block's own time.
 */
int EmergencyEasingSteps(const CBlockIndex* pindexPrev, int64_t nCandidateTime, const Consensus::Params&);

/** Multiply the compact target nBaseBits by 2^steps, capped at powLimit, returned in compact form. */
unsigned int ApplyEmergencyEasing(unsigned int nBaseBits, int steps, const Consensus::Params&);

/**
 * Target a block contributes to the LWMA window: its own target, reduced by
 * 2^(steps - nEmergencyCreditCapSteps) if it was eased by more than the cap
 * (steps are recomputed from its ancestors and timestamp; never zero).
 */
arith_uint256 LwmaWindowCredit(const CBlockIndex* pindex, const Consensus::Params&);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);

#endif // VIVUSCOIN_POW_H
