// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "arith_uint256.h"
#include "tinyformat.h"
#include "chainparams.h"
#include <stdio.h>
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;

#include "chainparamsseeds.h"

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        /** 
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf8;
        pchMessageStart[1] = 0xbc;
        pchMessageStart[2] = 0xb3;
        pchMessageStart[3] = 0xd8;
        vAlertPubKey = ParseHex("04fc9702847840aaf195de8442ebecedf5b095cdbb9bc716bda9110971b28a49e0ead8564ff0db22209e0374782c093bb899692d524e9d6a6956e7c5ecbcd68284");
        nDefaultPort = 58333;
        nMinerThreads = 0;
        nPruneAfterHeight = 100000;

        /**
         * Build the genesis block. Note that the output of its generation
         * transaction cannot be spent since it did not originally exist in the
         * database.
         *
         * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
         *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
         *   vMerkleTree: 4a5e1e
         */
        const char* pszTimestamp = "Blah blah blah Blah blah blah blah blah Blah blah blah blah blah Blah blah blah";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock.SetNull();
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1455597574;
        genesis.nBits    = 0x1e0fffff;
        genesis.nNonce   = 1434119;

        consensus.hashGenesisBlock = genesis.GetHash();
        
        // If genesis block hash does not match, then generate new genesis hash.
        
        uint256 tmphashGenesisBlock = uint256S("0xb519bb2dd76860028f90b06ec7035467f9a48dea48d105a1d9f339bc778b17c3");
        if (true && genesis.GetHash() != tmphashGenesisBlock)
        {
            printf("Searching for genesis block...\n");
            bool fNegative;
            bool fOverflow;
            arith_uint256 bnTarget;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            
            uint256 thash = genesis.GetPoWHash();
            
            while (UintToArith256(thash) > bnTarget)
            {
                thash = genesis.GetPoWHash();
                if (UintToArith256(thash) <= bnTarget)
                    break;
                if ((genesis.nNonce & 0xFFFFF) == 0)
                {
                    printf("nonce %08X: PoWhash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
            }
            printf("genesis.nTime = %u \n", genesis.nTime);
            printf("genesis.nNonce = %u \n", genesis.nNonce);
            printf("genesis.GetHash = %s\n", genesis.GetHash().ToString().c_str());
            printf("genesis.GetPoWHash = %s\n", genesis.GetPoWHash().ToString().c_str());
            printf("genesis.hashMerkleRoot = %s\n", genesis.BuildMerkleTree().ToString().c_str());
        }
        
        assert(consensus.hashGenesisBlock == uint256S("0xb519bb2dd76860028f90b06ec7035467f9a48dea48d105a1d9f339bc778b17c3"));
        assert(genesis.hashMerkleRoot == uint256S("0xa1c37dfaac8ac852263a658ab7024bd52954a748c9b149b0aec5c3193c1c34ab"));

        vSeeds.push_back(CDNSSeedData("testcoin.local", "seed.testcoin.local"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,5);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (Checkpoints::CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x046818587e9b8b51d7e8aaf61e2d59d2ce180b473ab68f27290cd14170905370")),
            1455597574, // * UNIX timestamp of last checkpoint block
            1,   // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            10.0     // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.fPowAllowMinDifficultyBlocks = true;
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbc;
        pchMessageStart[2] = 0xb3;
        pchMessageStart[3] = 0xd8;
        vAlertPubKey = ParseHex("04302390343f91cc401d56d68b123028bf52e5fca1939df127f63c6467cdf9c8e2c14b61104cf817d0b780da337893ecc4aaff1309e536162dabbdb45200ca2b0a");
        nDefaultPort = 68333;
        nMinerThreads = 0;
        nPruneAfterHeight = 1000;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1455597594;
        genesis.nNonce = 856768;
        consensus.hashGenesisBlock = genesis.GetHash();
        
        // If genesis block hash does not match, then generate new genesis hash.
        
        uint256 tmphashGenesisBlock = uint256S("0xca8e01ba2dc3200766d4dc33283e941d0a902eee7ec364f70e477923a213e115");
        if (true && genesis.GetHash() != tmphashGenesisBlock)
        {
            printf("Searching for Testnet genesis block...\n");
            bool fNegative;
            bool fOverflow;
            arith_uint256 bnTarget;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            
            uint256 thash = genesis.GetPoWHash();
            
            while (UintToArith256(thash) > bnTarget)
            {
                thash = genesis.GetPoWHash();
                if (UintToArith256(thash) <= bnTarget)
                    break;
                if ((genesis.nNonce & 0xFFFFF) == 0)
                {
                    printf("nonce %08X: PoWhash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
            }
            printf("genesis.nTime = %u \n", genesis.nTime);
            printf("genesis.nNonce = %u \n", genesis.nNonce);
            printf("genesis.GetHash = %s\n", genesis.GetHash().ToString().c_str());
            printf("genesis.GetPoWHash = %s\n", genesis.GetPoWHash().ToString().c_str());
            printf("genesis.hashMerkleRoot = %s\n", genesis.BuildMerkleTree().ToString().c_str());
        }
        
        assert(consensus.hashGenesisBlock == uint256S("0xca8e01ba2dc3200766d4dc33283e941d0a902eee7ec364f70e477923a213e115"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("testcoin.local", "test.seed.testcoin.local"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,127);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,125);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,8);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (Checkpoints::CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x1231796efe9e3f686b41c1efb0e4c17f644087ae5903a97546fcfdc5f61e5af9")),
            1455597594,
            1,
            10
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xbc;
        pchMessageStart[2] = 0xb3;
        pchMessageStart[3] = 0xd8;
        nMinerThreads = 1;
        genesis.nTime = 1455597514;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 0;
        consensus.hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        
        // If genesis block hash does not match, then generate new genesis hash.
        
        uint256 tmphashGenesisBlock = uint256S("0xd64af1e5d810601d1513a45d75a47c73d031b5d97805143c14f648bb5e92d5f1");
        if (true && genesis.GetHash() != tmphashGenesisBlock)
        {
            printf("Searching for Regtest genesis block...\n");
            bool fNegative;
            bool fOverflow;
            arith_uint256 bnTarget;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            
            uint256 thash = genesis.GetPoWHash();
            
            while (UintToArith256(thash) > bnTarget)
            {
                thash = genesis.GetPoWHash();
                if (UintToArith256(thash) <= bnTarget)
                    break;
                if ((genesis.nNonce & 0xFFFFF) == 0)
                {
                    printf("nonce %08X: PoWhash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
            }
            printf("genesis.nTime = %u \n", genesis.nTime);
            printf("genesis.nNonce = %u \n", genesis.nNonce);
            printf("genesis.GetHash = %s\n", genesis.GetHash().ToString().c_str());
            printf("genesis.GetPoWHash = %s\n", genesis.GetPoWHash().ToString().c_str());
            printf("genesis.hashMerkleRoot = %s\n", genesis.BuildMerkleTree().ToString().c_str());
        }
        
        assert(consensus.hashGenesisBlock == uint256S("0xd64af1e5d810601d1513a45d75a47c73d031b5d97805143c14f648bb5e92d5f1"));
        nPruneAfterHeight = 1000;

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (Checkpoints::CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0xfe4cba81d8acc9ca7722beb2f26a45d3fd0b2b37feea0abb6df80065af5a05c0")),
            0,
            0,
            0
        };
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
