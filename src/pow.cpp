// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2021 The Vivuscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <algorithm>
#include <vector>

bool IsLwmaActive(int nHeight, const Consensus::Params& params)
{
    return nHeight >= params.nLwmaActivationHeight;
}

int64_t EmergencyReferenceTime(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    assert(pindexPrev != nullptr);
    int64_t nRef = pindexPrev->GetBlockTime();
    const CBlockIndex* pindex = pindexPrev;
    for (int i = 1; i < params.nEmergencyRefSpan && pindex->pprev != nullptr; ++i) {
        pindex = pindex->pprev;
        nRef = std::max(nRef, pindex->GetBlockTime());
    }
    return nRef;
}

int EmergencyEasingSteps(const CBlockIndex* pindexPrev, int64_t nCandidateTime, const Consensus::Params& params)
{
    assert(pindexPrev != nullptr);
    if (!IsLwmaActive(pindexPrev->nHeight + 1, params)) {
        return 0;
    }
    const int64_t T = params.nPowTargetSpacing;
    const int64_t nTrigger = params.nEmergencyTriggerMult * T;
    const int64_t nGap = nCandidateTime - EmergencyReferenceTime(pindexPrev, params);
    if (nGap < nTrigger) {
        return 0;
    }
    const int64_t nSteps = 1 + (nGap - nTrigger) / T;
    return static_cast<int>(std::min<int64_t>(nSteps, params.nEmergencyMaxSteps));
}

unsigned int ApplyEmergencyEasing(unsigned int nBaseBits, int steps, const Consensus::Params& params)
{
    if (steps <= 0) {
        return nBaseBits;
    }
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBaseBits);
    // base * 2^steps > powLimit  <=>  base > floor(powLimit / 2^steps); test it
    // this way round so the shift can never overflow 256 bits.
    if (bnTarget > (bnPowLimit >> steps)) {
        return bnPowLimit.GetCompact();
    }
    bnTarget <<= steps;
    return bnTarget.GetCompact();
}

arith_uint256 LwmaWindowCredit(const CBlockIndex* pindex, const Consensus::Params& params)
{
    assert(pindex != nullptr);
    arith_uint256 bnTarget;
    bnTarget.SetCompact(pindex->nBits);
    const int steps = pindex->pprev ? EmergencyEasingSteps(pindex->pprev, pindex->GetBlockTime(), params) : 0;
    const int shift = std::max(0, steps - params.nEmergencyCreditCapSteps);
    if (shift > 0) {
        bnTarget >>= shift;
    }
    if (bnTarget == 0) {
        bnTarget = 1;
    }
    return bnTarget;
}

unsigned int LwmaNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    const int64_t T = params.nPowTargetSpacing;
    const int N = params.nLwmaWindow;
    assert(N > 0 && T > 0);
    // k = N(N+1)T/2 normalises the weighted solvetime sum to T when every solvetime is T.
    const int64_t k = static_cast<int64_t>(N) * (N + 1) * T / 2;
    const int64_t nMaxSolvetime = params.nLwmaMaxSolvetimeMult * T;

    // The window is the last N blocks; the block before it supplies the first
    // previous timestamp.  Both must exist (mirrors the reference simulator).
    if (pindexLast->nHeight < N) {
        return bnPowLimit.GetCompact();
    }
    std::vector<const CBlockIndex*> vWindow(N);
    const CBlockIndex* pindex = pindexLast;
    for (int i = N - 1; i >= 0; --i) {
        vWindow[i] = pindex;
        pindex = pindex->pprev;
    }
    assert(pindex != nullptr && pindex->nHeight == pindexLast->nHeight - N);

    int64_t nPrevTime = pindex->GetBlockTime();
    int64_t nWeightedSolvetimes = 0;
    arith_uint256 bnSumTargets = 0;
    for (int j = 1; j <= N; ++j) {
        const CBlockIndex* pblock = vWindow[j - 1];
        // Monotone +1 s minimum: a timestamp at or before its predecessor counts as 1 s later.
        const int64_t nThisTime = pblock->GetBlockTime() > nPrevTime ? pblock->GetBlockTime() : nPrevTime + 1;
        const int64_t nSolvetime = std::min<int64_t>(nMaxSolvetime, nThisTime - nPrevTime);
        nPrevTime = nThisTime;
        nWeightedSolvetimes += nSolvetime * j;
        bnSumTargets += LwmaWindowCredit(pblock, params) / arith_uint256(N) / arith_uint256(k);
    }
    assert(nWeightedSolvetimes > 0);

    // next = sumTargets * weightedSolvetimes, capped at powLimit.  Compare first
    // so the multiplication cannot overflow 256 bits on chains with a large powLimit.
    const arith_uint256 bnWeight(static_cast<uint64_t>(nWeightedSolvetimes));
    if (bnSumTargets > bnPowLimit / bnWeight) {
        return bnPowLimit.GetCompact();
    }
    arith_uint256 bnNext = bnSumTargets * bnWeight;
    if (bnNext > bnPowLimit) {
        bnNext = bnPowLimit;
    }
    return bnNext.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // v1.1 rules from the activation height: LWMA-1 base target, then the
    // emergency easing for the candidate's own timestamp.  Everything below
    // this point is the legacy rule and is left untouched so that blocks
    // below the activation height validate exactly as before.
    if (IsLwmaActive(pindexLast->nHeight + 1, params)) {
        const unsigned int nBaseBits = LwmaNextWorkRequired(pindexLast, params);
        const int steps = EmergencyEasingSteps(pindexLast, pblock->GetBlockTime(), params);
        return ApplyEmergencyEasing(nBaseBits, steps, params);
    }

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
