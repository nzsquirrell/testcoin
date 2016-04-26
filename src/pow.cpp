// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, int algo)
{
    const arith_uint256 nProofOfWorkLimit = UintToArith256(params.powLimit);

    // Genesis block
    if (pindexLast == NULL)
    {
        LogPrintf("pindexLast is null. returning nProofOfWorkLimit\n");
        return nProofOfWorkLimit.GetCompact();
    }

    // find previous block with same algo
    const CBlockIndex* pindexPrev = GetLastBlockIndexForAlgo(pindexLast, algo);
    
    // Genesis block
    if (pindexPrev == NULL)
    {
        LogPrintf("pindexPrev is null. returning nProofOfWorkLimit\n");
        return nProofOfWorkLimit.GetCompact();
    }
    
    const CBlockIndex* pindexFirst = pindexPrev;

    // Go back by what we want to be params.nAveragingInterval blocks
    for (int i = 0; pindexFirst && i < params.nAveragingInterval - 1; i++)
    {
        pindexFirst = pindexFirst->pprev;
        pindexFirst = GetLastBlockIndexForAlgo(pindexFirst, algo);
        if (pindexFirst == NULL)
        {
            LogPrintf("pindexFirst is null. returning nProofOfWorkLimit\n");
            return nProofOfWorkLimit.GetCompact();
        }
    }
    
    return CalculateNextWorkRequiredV1(pindexPrev, pindexFirst, params, algo);
}

unsigned int CalculateNextWorkRequiredV1(const CBlockIndex* pindexPrev, const CBlockIndex* pindexFirst, const Consensus::Params& params, int algo)
{
    const arith_uint256 nProofOfWorkLimit = UintToArith256(params.powLimit);    
    
    int64_t nTargetSpacingPerAlgo = params.nPowTargetSpacing * NUM_ALGOS; // 60 * 5 = 300s per algo
    int64_t nAveragingTargetTimespan = params.nAveragingInterval * nTargetSpacingPerAlgo; // 10 * 300 = 3000s, 50 minutes
    int64_t nMaxAdjustDown = 4; // 4% adjustment down
    int64_t nMaxAdjustUp = 4; // 4% adjustment up
    
    int64_t nMinActualTimespan = nAveragingTargetTimespan * (100 - nMaxAdjustUp) / 100;
    int64_t nMaxActualTimespan = nAveragingTargetTimespan * (100 + nMaxAdjustDown) / 100;
    
    int64_t nActualTimespan = pindexPrev->GetMedianTimePast() - pindexFirst->GetMedianTimePast();

    if(fDebug)
    {
        LogPrintf("  nActualTimespan = %d before bounds   %d   %d\n", nActualTimespan, pindexPrev->GetMedianTimePast(), pindexFirst->GetMedianTimePast());
    }
    
    if (nActualTimespan < nMinActualTimespan)
        nActualTimespan = nMinActualTimespan;
    if (nActualTimespan > nMaxActualTimespan)
        nActualTimespan = nMaxActualTimespan;
    
    if(fDebug)
    {
        LogPrintf("  nActualTimespan = %d after bounds   %d   %d\n", nActualTimespan, nMinActualTimespan, nMaxActualTimespan);
    }
    
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexPrev->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= nAveragingTargetTimespan;
    if (bnNew > nProofOfWorkLimit)
        bnNew = nProofOfWorkLimit;
    
    /// debug print
    if(fDebug)
    {
        LogPrintf("CalculateNextWorkRequiredV1(Algo=%d): RETARGET\n", algo);
        LogPrintf("CalculateNextWorkRequiredV1(Algo=%d): nTargetTimespan = %d    nActualTimespan = %d\n", algo, nAveragingTargetTimespan, nActualTimespan);
        LogPrintf("CalculateNextWorkRequiredV1(Algo=%d): Before: %08x  %s\n", algo, pindexPrev->nBits, bnOld.ToString());
        LogPrintf("CalculateNextWorkRequiredV1(Algo=%d): After:  %08x  %s\n", algo, bnNew.GetCompact(), bnNew.ToString());
    }

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
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("CheckProofOfWork(): hash doesn't match nBits");

    return true;
}

arith_uint256 GetBlockProofBase(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

int GetAlgoWorkFactor(int algo)
{
/*     if (!TestNet() && (nHeight < nBlockAlgoWorkWeightStart))
    {
        return 1;
    }
    if (TestNet() && (nHeight < 100))
    {
        return 1;
    }
 */
    switch (algo)
    {
        case ALGO_SHA256D:
            return 1; 
        // work factor = absolute work ratio * optimisation factor
        case ALGO_SCRYPT:
            return 1024 * 4;
        case ALGO_GROESTL:
            return 64 * 8;
        case ALGO_SKEIN:
            return 4 * 6;
        case ALGO_QUBIT:
            return 128 * 8;
        default:
            return 1;
    }
}

arith_uint256 GetPrevWorkForAlgo(const CBlockIndex& block, int algo)
{
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (pindex->GetAlgo() == algo)
        {
            return GetBlockProofBase(*pindex);
        }
        pindex = pindex->pprev;
    }
    return UintToArith256(Params().GetConsensus().powLimit);
}

arith_uint256 GetPrevWorkForAlgoWithDecay(const CBlockIndex& block, int algo)
{
    int nDistance = 0;
    arith_uint256 nWork;
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (nDistance > 32)
        {
            return UintToArith256(Params().GetConsensus().powLimit);
        }
        if (pindex->GetAlgo() == algo)
        {
            arith_uint256 nWork = GetBlockProofBase(*pindex);
            nWork *= (32 - nDistance);
            nWork /= 32;
            if (nWork < UintToArith256(Params().GetConsensus().powLimit))
                nWork = UintToArith256(Params().GetConsensus().powLimit);
            return nWork;
        }
        pindex = pindex->pprev;
        nDistance++;
    }
    return UintToArith256(Params().GetConsensus().powLimit);
}

arith_uint256 GetPrevWorkForAlgoWithDecay2(const CBlockIndex& block, int algo)
{
    int nDistance = 0;
    arith_uint256 nWork;
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (nDistance > 32)
        {
            return arith_uint256(0);
        }
        if (pindex->GetAlgo() == algo)
        {
            arith_uint256 nWork = GetBlockProofBase(*pindex);
            nWork *= (32 - nDistance);
            nWork /= 32;
            return nWork;
        }
        pindex = pindex->pprev;
        nDistance++;
    }
    return arith_uint256(0);
}
    
arith_uint256 GetPrevWorkForAlgoWithDecay3(const CBlockIndex& block, int algo)
{
    int nDistance = 0;
    arith_uint256 nWork;
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (nDistance > 100)
        {
            return arith_uint256(0);
        }
        if (pindex->GetAlgo() == algo)
        {
            arith_uint256 nWork = GetBlockProofBase(*pindex);
            nWork *= (100 - nDistance);
            nWork /= 100;
            return nWork;
        }
        pindex = pindex->pprev;
        nDistance++;
    }
    return arith_uint256(0);
}

arith_uint256 GetGeometricMeanPrevWork(const CBlockIndex& block)
{
    arith_uint256 bnRes;
    arith_uint256 nBlockWork = GetBlockProofBase(block);
    int nAlgo = block.GetAlgo();
    
    for (int algo = 0; algo < NUM_ALGOS; algo++)
    {
        if (algo != nAlgo)
        {
            arith_uint256 nBlockWorkAlt = GetPrevWorkForAlgoWithDecay3(block, algo);
            if (nBlockWorkAlt != 0)
                nBlockWork *= nBlockWorkAlt;
        }
    }
    bnRes = nBlockWork;
    // Compute the geometric mean
    bnRes = nthRoot(bnRes, NUM_ALGOS);
    // Scale to roughly match the old work calculation
    bnRes <<= 8;
    return bnRes;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    int algo = block.GetAlgo();
    arith_uint256 bnTarget;
    bnTarget = GetBlockProofBase(block) * GetAlgoWorkFactor(algo);
    return bnTarget;
}
   
bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    if(fDebug)
    {
        LogPrintf("DEBUG: AUX-proof-of-work submitted  \n");
    }
    /* Except for legacy blocks with full version 1, ensure that
       the chain ID is correct.  Legacy blocks are not allowed since
       the merge-mining start, which is checked in AcceptBlockHeader
       where the height is known.  */
    if (!block.nVersion.IsLegacy() && params.fStrictChainId && block.nVersion.GetChainId() != params.nAuxpowChainId)
        return error("%s : block does not have our chain ID"
                     " (got %d, expected %d, full nVersion %d)",
                     __func__,
                     block.nVersion.GetChainId(),
                     params.nAuxpowChainId,
                     block.nVersion.GetFullVersion());

    /* If there is no auxpow, just check the block hash.  */
    if (!block.auxpow) {
        if (block.nVersion.IsAuxpow())
            return error("%s : no auxpow on block with auxpow version",
                         __func__);

        if (!CheckProofOfWork(block.GetPoWHash(), block.nBits, params))
            return error("%s : non-AUX proof of work failed", __func__);

        return true;
    }

    /* We have auxpow.  Check it.  */

    if (!block.nVersion.IsAuxpow())
        return error("%s : auxpow on block with non-auxpow version", __func__);

    if (!block.auxpow->check(block.GetHash(), block.nVersion.GetChainId(), params))
        return error("%s : AUX POW is not valid", __func__);
    
/*    if(fDebug)
    {
        bool fNegative;
        bool fOverflow;
        arith_uint256 bnTarget;
        bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
        
        LogPrintf("DEBUG: proof-of-work submitted  \n  parent-PoWhash: %s\n  ntarget: %s\n",
        block.auxpow->getParentBlockPoWHash().ToString().c_str(),
        bnTarget.ToString().c_str());
    }*/
    
    if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits, params))
        return error("%s : AUX proof of work failed", __func__);

    return true;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, int algo)
{
    while (pindex && pindex->pprev && (pindex->GetAlgo() != algo))
        pindex = pindex->pprev;
    return pindex;
}

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, int algo)
{
    for (;;)
    {
        if (!pindex)
            return NULL;
        if (pindex->GetAlgo() == algo)
            return pindex;
        pindex = pindex->pprev;
    }
}

int BN_cmp(arith_uint256 a, arith_uint256 b)
{
    if(a < b)
        return -1;
    if(a > b)
        return 1;
    return 0;
}

arith_uint256 BN_set_negative(arith_uint256 value, int sign)
{
    if(value<0 && sign)
    {
        return value;
    }
    else
    {
        return -1 * value;
    }
}

arith_uint256 nthRoot(const arith_uint256& value, int n)
{
    assert(n > 1);
    if (value==0)
        return 0;
    assert(value>0);

    // starting approximation
    int nRootBits = (value.bits() + n - 1) / n;
    int nStartingBits = std::min(8, nRootBits);
    arith_uint256 bnUpper = value;
    bnUpper >>= (nRootBits - nStartingBits)*n;
    arith_uint256 bnCur = 0;
    for (int i = nStartingBits - 1; i >= 0; i--)
    {
        arith_uint256 bnNext = bnCur;
        bnNext += 1 << i;
        arith_uint256 bnPower(1);
        for (int j = 0; j < n; j++)
            bnPower *= bnNext;
        if (BN_cmp(bnPower, bnUpper) <= 0)
            bnCur = bnNext;
    }
    if (nRootBits == nStartingBits)
        return bnCur;
    bnCur <<= nRootBits - nStartingBits;

    // iterate: cur = cur + (*this / cur^^(n-1) - cur)/n
    arith_uint256 bnDelta;
    arith_uint256 bnRoot(n);
    int nTerminate = 0;
    // this should always converge in fewer steps, but limit just in case
    for (int it = 0; it < 20; it++)
    {
        arith_uint256 bnDenominator = 1;
        for (int i = 0; i < n - 1; i++)
            bnDenominator *= bnCur;
        bnDelta = value / bnDenominator - bnCur;
        if (bnDelta.EqualTo(0))
            return bnCur;
        if (bnDelta<0)
        {
            if (nTerminate == 1)
                return bnCur - 1;
            bnDelta = BN_set_negative(bnDelta, 0);
            if (BN_cmp(bnDelta, bnRoot) <= 0)
            {
                bnCur -= 1;
                nTerminate = -1;
                continue;
            }
            bnDelta = BN_set_negative(bnDelta, 1);
        }
        else
        {
            if (nTerminate == -1)
                return bnCur;
            if (BN_cmp(bnDelta, bnRoot) <= 0)
            {
                bnCur += 1;
                nTerminate = 1;
                continue;
            }
        }
        bnCur += bnDelta / n;
        nTerminate = 0;
    }
    return bnCur;
}
