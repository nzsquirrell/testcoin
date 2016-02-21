// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
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


/*unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("params.nPowTargetTimespan = %d    nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}*/

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

