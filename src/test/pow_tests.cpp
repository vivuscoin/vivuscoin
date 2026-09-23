// Copyright (c) 2015-2018 The Bitcoin Core developers
// Copyright (c) 2021 The Vivuscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <pow.h>
#include <random.h>
#include <util/system.h>
#include <test/test_vivuscoin.h>

#include <boost/test/unit_test.hpp>

#include <utility>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00d86aU);
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffffU);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c0168fdU);
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fdU);
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}


/* ------------------------------------------------------------------------
 * v1.1 difficulty rules: LWMA-1 + emergency easing (task 06).
 *
 * The vectors below were produced by 06-build/gen_pow_vectors.py, which
 * drives the task 05 reference simulator (05-sim/sim.py) as an oracle: each
 * block was appended with the nBits the simulator requires for its
 * timestamp.  GetNextWorkRequired must reproduce every nBits exactly from
 * the preceding headers, including the legacy blocks before activation and
 * the window that straddles the activation height.
 * ---------------------------------------------------------------------- */

struct LwmaVector {
    int nFirstHeight;
    int nFirstCheckHeight;
    std::vector<std::pair<int64_t, unsigned int>> blocks; // (nTime, nBits)
};

// Chain 1 (mainnet params, T=240): frozen tip 24,092, 5-year gap, 49 legacy blocks, LWMA from 24,142 with all emergency boundaries
// first height 24023, 302 blocks; check GetNextWorkRequired from height 24093
static const LwmaVector VEC_MAINNET_TRANSITION = {24023, 24093, {
    {1632314640, 0x1c01ee3b}, // h=24023 steps=0
    {1632314880, 0x1c01ee3b}, // h=24024 steps=0
    {1632315120, 0x1c01ee3b}, // h=24025 steps=0
    {1632315360, 0x1c01ee3b}, // h=24026 steps=0
    {1632315600, 0x1c01ee3b}, // h=24027 steps=0
    {1632315840, 0x1c01ee3b}, // h=24028 steps=0
    {1632316080, 0x1c01ee3b}, // h=24029 steps=0
    {1632316320, 0x1c01ee3b}, // h=24030 steps=0
    {1632316560, 0x1c01ee3b}, // h=24031 steps=0
    {1632316800, 0x1c01ee3b}, // h=24032 steps=0
    {1632317040, 0x1c01ee3b}, // h=24033 steps=0
    {1632317280, 0x1c01ee3b}, // h=24034 steps=0
    {1632317520, 0x1c01ee3b}, // h=24035 steps=0
    {1632317760, 0x1c01ee3b}, // h=24036 steps=0
    {1632318000, 0x1c01ee3b}, // h=24037 steps=0
    {1632318240, 0x1c01ee3b}, // h=24038 steps=0
    {1632318480, 0x1c01ee3b}, // h=24039 steps=0
    {1632318720, 0x1c01ee3b}, // h=24040 steps=0
    {1632318960, 0x1c01ee3b}, // h=24041 steps=0
    {1632319200, 0x1c01ee3b}, // h=24042 steps=0
    {1632319440, 0x1c01ee3b}, // h=24043 steps=0
    {1632319680, 0x1c01ee3b}, // h=24044 steps=0
    {1632319920, 0x1c01ee3b}, // h=24045 steps=0
    {1632320160, 0x1c01ee3b}, // h=24046 steps=0
    {1632320400, 0x1c01ee3b}, // h=24047 steps=0
    {1632320640, 0x1c01ee3b}, // h=24048 steps=0
    {1632320880, 0x1c01ee3b}, // h=24049 steps=0
    {1632321120, 0x1c01ee3b}, // h=24050 steps=0
    {1632321360, 0x1c01ee3b}, // h=24051 steps=0
    {1632321600, 0x1c01ee3b}, // h=24052 steps=0
    {1632321840, 0x1c01ee3b}, // h=24053 steps=0
    {1632322080, 0x1c01ee3b}, // h=24054 steps=0
    {1632322320, 0x1c01ee3b}, // h=24055 steps=0
    {1632322560, 0x1c01ee3b}, // h=24056 steps=0
    {1632322800, 0x1c01ee3b}, // h=24057 steps=0
    {1632323040, 0x1c01ee3b}, // h=24058 steps=0
    {1632323280, 0x1c01ee3b}, // h=24059 steps=0
    {1632323520, 0x1c01ee3b}, // h=24060 steps=0
    {1632323760, 0x1c01ee3b}, // h=24061 steps=0
    {1632324000, 0x1c01ee3b}, // h=24062 steps=0
    {1632324240, 0x1c01ee3b}, // h=24063 steps=0
    {1632324480, 0x1c01ee3b}, // h=24064 steps=0
    {1632324720, 0x1c01ee3b}, // h=24065 steps=0
    {1632324960, 0x1c01ee3b}, // h=24066 steps=0
    {1632325200, 0x1c01ee3b}, // h=24067 steps=0
    {1632325440, 0x1c01ee3b}, // h=24068 steps=0
    {1632325680, 0x1c01ee3b}, // h=24069 steps=0
    {1632325920, 0x1c01ee3b}, // h=24070 steps=0
    {1632326160, 0x1c01ee3b}, // h=24071 steps=0
    {1632326400, 0x1c01ee3b}, // h=24072 steps=0
    {1632326640, 0x1c01ee3b}, // h=24073 steps=0
    {1632326880, 0x1c01ee3b}, // h=24074 steps=0
    {1632327120, 0x1c01ee3b}, // h=24075 steps=0
    {1632327360, 0x1c01ee3b}, // h=24076 steps=0
    {1632327600, 0x1c01ee3b}, // h=24077 steps=0
    {1632327840, 0x1c01ee3b}, // h=24078 steps=0
    {1632328080, 0x1c01ee3b}, // h=24079 steps=0
    {1632328320, 0x1c01ee3b}, // h=24080 steps=0
    {1632328560, 0x1c01ee3b}, // h=24081 steps=0
    {1632328800, 0x1c01ee3b}, // h=24082 steps=0
    {1632329040, 0x1c01ee3b}, // h=24083 steps=0
    {1632329280, 0x1c01ee3b}, // h=24084 steps=0
    {1632329520, 0x1c01ee3b}, // h=24085 steps=0
    {1632329760, 0x1c01ee3b}, // h=24086 steps=0
    {1632330000, 0x1c01ee3b}, // h=24087 steps=0
    {1632330240, 0x1c01ee3b}, // h=24088 steps=0
    {1632330480, 0x1c01ee3b}, // h=24089 steps=0
    {1632330720, 0x1c01ee3b}, // h=24090 steps=0
    {1632330960, 0x1c01ee3b}, // h=24091 steps=0
    {1632331200, 0x1c01ee3b}, // h=24092 steps=0
    {1760500000, 0x1c01ee3b}, // h=24093 steps=0
    {1760500240, 0x1c01ee3b}, // h=24094 steps=0
    {1760500480, 0x1c01ee3b}, // h=24095 steps=0
    {1760500720, 0x1c01ee3b}, // h=24096 steps=0
    {1760500960, 0x1c01ee3b}, // h=24097 steps=0
    {1760501200, 0x1c01ee3b}, // h=24098 steps=0
    {1760501440, 0x1c01ee3b}, // h=24099 steps=0
    {1760501680, 0x1c01ee3b}, // h=24100 steps=0
    {1760501920, 0x1c01ee3b}, // h=24101 steps=0
    {1760502160, 0x1c01ee3b}, // h=24102 steps=0
    {1760502400, 0x1c01ee3b}, // h=24103 steps=0
    {1760502640, 0x1c01ee3b}, // h=24104 steps=0
    {1760502880, 0x1c01ee3b}, // h=24105 steps=0
    {1760503120, 0x1c01ee3b}, // h=24106 steps=0
    {1760503360, 0x1c01ee3b}, // h=24107 steps=0
    {1760503600, 0x1c01ee3b}, // h=24108 steps=0
    {1760503840, 0x1c01ee3b}, // h=24109 steps=0
    {1760504080, 0x1c01ee3b}, // h=24110 steps=0
    {1760504320, 0x1c01ee3b}, // h=24111 steps=0
    {1760504560, 0x1c01ee3b}, // h=24112 steps=0
    {1760504800, 0x1c01ee3b}, // h=24113 steps=0
    {1760505040, 0x1c01ee3b}, // h=24114 steps=0
    {1760505280, 0x1c01ee3b}, // h=24115 steps=0
    {1760505520, 0x1c01ee3b}, // h=24116 steps=0
    {1760505760, 0x1c01ee3b}, // h=24117 steps=0
    {1760506000, 0x1c01ee3b}, // h=24118 steps=0
    {1760506240, 0x1c01ee3b}, // h=24119 steps=0
    {1760506480, 0x1c01ee3b}, // h=24120 steps=0
    {1760506720, 0x1c01ee3b}, // h=24121 steps=0
    {1760506960, 0x1c01ee3b}, // h=24122 steps=0
    {1760507200, 0x1c01ee3b}, // h=24123 steps=0
    {1760507440, 0x1c01ee3b}, // h=24124 steps=0
    {1760507680, 0x1c01ee3b}, // h=24125 steps=0
    {1760507920, 0x1c01ee3b}, // h=24126 steps=0
    {1760508160, 0x1c01ee3b}, // h=24127 steps=0
    {1760508400, 0x1c01ee3b}, // h=24128 steps=0
    {1760508640, 0x1c01ee3b}, // h=24129 steps=0
    {1760508880, 0x1c01ee3b}, // h=24130 steps=0
    {1760509120, 0x1c01ee3b}, // h=24131 steps=0
    {1760509360, 0x1c01ee3b}, // h=24132 steps=0
    {1760509600, 0x1c01ee3b}, // h=24133 steps=0
    {1760509840, 0x1c01ee3b}, // h=24134 steps=0
    {1760510080, 0x1c01ee3b}, // h=24135 steps=0
    {1760510320, 0x1c01ee3b}, // h=24136 steps=0
    {1760510560, 0x1c01ee3b}, // h=24137 steps=0
    {1760510800, 0x1c01ee3b}, // h=24138 steps=0
    {1760511040, 0x1c01ee3b}, // h=24139 steps=0
    {1760511280, 0x1c01ee3b}, // h=24140 steps=0
    {1760511520, 0x1c01ee3b}, // h=24141 steps=0
    {1760511898, 0x1c01fe6f}, // h=24142 steps=0
    {1760512312, 0x1c0206af}, // h=24143 steps=0
    {1760512471, 0x1c02115e}, // h=24144 steps=0
    {1760512544, 0x1c020ace}, // h=24145 steps=0
    {1760512545, 0x1c01fe60}, // h=24146 steps=0
    {1760512806, 0x1c01ed06}, // h=24147 steps=0
    {1760512958, 0x1c01ed45}, // h=24148 steps=0
    {1760513300, 0x1c01e61b}, // h=24149 steps=0
    {1760513412, 0x1c01ebce}, // h=24150 steps=0
    {1760513765, 0x1c01e1e4}, // h=24151 steps=0
    {1760513841, 0x1c01e84b}, // h=24152 steps=0
    {1760514230, 0x1c01dbe9}, // h=24153 steps=0
    {1760514544, 0x1c01e4b5}, // h=24154 steps=0
    {1760514672, 0x1c01e9c0}, // h=24155 steps=0
    {1760514857, 0x1c01e236}, // h=24156 steps=0
    {1760515132, 0x1c01de87}, // h=24157 steps=0
    {1760515183, 0x1c01e0f0}, // h=24158 steps=0
    {1760515377, 0x1c01d436}, // h=24159 steps=0
    {1760515769, 0x1c01d12a}, // h=24160 steps=0
    {1760515843, 0x1c01db7b}, // h=24161 steps=0
    {1760516233, 0x1c01d057}, // h=24162 steps=0
    {1760516511, 0x1c01da81}, // h=24163 steps=0
    {1760516957, 0x1c01dd1c}, // h=24164 steps=0
    {1760517055, 0x1c01eb02}, // h=24165 steps=0
    {1760517078, 0x1c01e181}, // h=24166 steps=0
    {1760517465, 0x1c01d2f7}, // h=24167 steps=0
    {1760517857, 0x1c01dce6}, // h=24168 steps=0
    {1760517998, 0x1c01e725}, // h=24169 steps=0
    {1760518022, 0x1c01e08c}, // h=24170 steps=0
    {1760518075, 0x1c01d21c}, // h=24171 steps=0
    {1760518317, 0x1c01c5a7}, // h=24172 steps=0
    {1760518400, 0x1c01c5e1}, // h=24173 steps=0
    {1760519126, 0x1c01bb7c}, // h=24174 steps=0
    {1760519339, 0x1c01dc04}, // h=24175 steps=0
    {1760519393, 0x1c01da45}, // h=24176 steps=0
    {1760519649, 0x1c01cdef}, // h=24177 steps=0
    {1760519756, 0x1c01cf13}, // h=24178 steps=0
    {1760520404, 0x1c01c64f}, // h=24179 steps=0
    {1760520981, 0x1c01e17a}, // h=24180 steps=0
    {1760521154, 0x1c01f7e1}, // h=24181 steps=0
    {1760522593, 0x1c01f37d}, // h=24182 steps=0
    {1760522833, 0x1c02431e}, // h=24183 steps=0
    {1760523073, 0x1c024370}, // h=24184 steps=0
    {1760524513, 0x1c048782}, // h=24185 steps=1
    {1760524753, 0x1c02a16e}, // h=24186 steps=0
    {1760524993, 0x1c02a2ab}, // h=24187 steps=0
    {1760526672, 0x1c0547ce}, // h=24188 steps=1
    {1760526912, 0x1c030960}, // h=24189 steps=0
    {1760527152, 0x1c030c17}, // h=24190 steps=0
    {1760528832, 0x1c0c3b2c}, // h=24191 steps=2
    {1760529072, 0x1c03a907}, // h=24192 steps=0
    {1760529312, 0x1c03aeb2}, // h=24193 steps=0
    {1760529552, 0x1c03b45a}, // h=24194 steps=0
    {1760531712, 0x1c3b9ff0}, // h=24195 steps=4
    {1760531952, 0x1c05c4e0}, // h=24196 steps=0
    {1760532192, 0x1c05d5ce}, // h=24197 steps=0
    {1760532432, 0x1c05e6c7}, // h=24198 steps=0
    {1760532672, 0x1c05f7c6}, // h=24199 steps=0
    {1760535792, 0x1d00ffff}, // h=24200 steps=8
    {1760536032, 0x1c087b10}, // h=24201 steps=0
    {1760536272, 0x1c089af2}, // h=24202 steps=0
    {1760536512, 0x1c08ba62}, // h=24203 steps=0
    {1760536752, 0x1c08d9fc}, // h=24204 steps=0
    {1760536992, 0x1c08f9cb}, // h=24205 steps=0
    {1760548032, 0x1d00ffff}, // h=24206 steps=24
    {1760548033, 0x1c09cbbe}, // h=24207 steps=0
    {1760548034, 0x1c09c5f0}, // h=24208 steps=0
    {1760548035, 0x1c09bd41}, // h=24209 steps=0
    {1760548036, 0x1c09b18e}, // h=24210 steps=0
    {1760548037, 0x1c09a30b}, // h=24211 steps=0
    {1760548038, 0x1c09916e}, // h=24212 steps=0
    {1760548039, 0x1c097d0d}, // h=24213 steps=0
    {1760548040, 0x1c09657e}, // h=24214 steps=0
    {1760548041, 0x1c094b4c}, // h=24215 steps=0
    {1760548042, 0x1c092e62}, // h=24216 steps=0
    {1760548282, 0x1c090e6f}, // h=24217 steps=0
    {1760548522, 0x1c0921fb}, // h=24218 steps=0
    {1760548762, 0x1c09352e}, // h=24219 steps=0
    {1760549002, 0x1c09477b}, // h=24220 steps=0
    {1760549242, 0x1c095929}, // h=24221 steps=0
    {1760549482, 0x1c096aad}, // h=24222 steps=0
    {1760549722, 0x1c097b37}, // h=24223 steps=0
    {1760549962, 0x1c098b91}, // h=24224 steps=0
    {1760550202, 0x1c099b6f}, // h=24225 steps=0
    {1760550442, 0x1c09ab4d}, // h=24226 steps=0
    {1760550682, 0x1c09ba28}, // h=24227 steps=0
    {1760550922, 0x1c09c7b3}, // h=24228 steps=0
    {1760551162, 0x1c09d500}, // h=24229 steps=0
    {1760551402, 0x1c09e219}, // h=24230 steps=0
    {1760551642, 0x1c09ee2c}, // h=24231 steps=0
    {1760551882, 0x1c09f8c0}, // h=24232 steps=0
    {1760552122, 0x1c0a01d8}, // h=24233 steps=0
    {1760552362, 0x1c0a0a0d}, // h=24234 steps=0
    {1760552602, 0x1c0a10c0}, // h=24235 steps=0
    {1760552842, 0x1c0a185d}, // h=24236 steps=0
    {1760552642, 0x1c0a1eff}, // h=24237 steps=0
    {1760552882, 0x1c09d7a6}, // h=24238 steps=0
    {1760553122, 0x1c09995e}, // h=24239 steps=0
    {1760553362, 0x1c0999bf}, // h=24240 steps=0
    {1760553602, 0x1c099afe}, // h=24241 steps=0
    {1760553842, 0x1c099ce8}, // h=24242 steps=0
    {1760554082, 0x1c099db9}, // h=24243 steps=0
    {1760554322, 0x1c09a35c}, // h=24244 steps=0
    {1760554562, 0x1c09a874}, // h=24245 steps=0
    {1760554802, 0x1c09a3c9}, // h=24246 steps=0
    {1760555042, 0x1c09ad68}, // h=24247 steps=0
    {1760555282, 0x1c09b6ba}, // h=24248 steps=0
    {1760555522, 0x1c09b53d}, // h=24249 steps=0
    {1760555762, 0x1c09c3ac}, // h=24250 steps=0
    {1760556002, 0x1c09d208}, // h=24251 steps=0
    {1760556242, 0x1c09bc5e}, // h=24252 steps=0
    {1760555043, 0x1c09cf5f}, // h=24253 steps=0
    {1760558162, 0x1c4c4178}, // h=24254 steps=3
    {1760558402, 0x1c0c8dbc}, // h=24255 steps=0
    {1760558642, 0x1c0bb089}, // h=24256 steps=0
    {1760558882, 0x1c0bc753}, // h=24257 steps=0
    {1760559122, 0x1c0bde28}, // h=24258 steps=0
    {1760559362, 0x1c0bf508}, // h=24259 steps=0
    {1760559602, 0x1c0c0bf1}, // h=24260 steps=0
    {1760559842, 0x1c0b22ed}, // h=24261 steps=0
    {1760560082, 0x1c0b32dd}, // h=24262 steps=0
    {1760560322, 0x1c0b4290}, // h=24263 steps=0
    {1760560562, 0x1c0b5206}, // h=24264 steps=0
    {1760560802, 0x1c0b613d}, // h=24265 steps=0
    {1760561042, 0x1c0b7032}, // h=24266 steps=0
    {1760561282, 0x1c0ba76d}, // h=24267 steps=0
    {1760561522, 0x1c0bbbe5}, // h=24268 steps=0
    {1760561762, 0x1c0bcf55}, // h=24269 steps=0
    {1760562002, 0x1c0be1c1}, // h=24270 steps=0
    {1760562242, 0x1c0bf331}, // h=24271 steps=0
    {1760562482, 0x1c0c03aa}, // h=24272 steps=0
    {1760562722, 0x1c0c1332}, // h=24273 steps=0
    {1760562962, 0x1c0c21cf}, // h=24274 steps=0
    {1760563202, 0x1c0c2f85}, // h=24275 steps=0
    {1760563442, 0x1c0c3c58}, // h=24276 steps=0
    {1760563682, 0x1c0c484b}, // h=24277 steps=0
    {1760563922, 0x1c0c5363}, // h=24278 steps=0
    {1760564162, 0x1c0c5e4c}, // h=24279 steps=0
    {1760564402, 0x1c0c6907}, // h=24280 steps=0
    {1760564642, 0x1c0c7398}, // h=24281 steps=0
    {1760564882, 0x1c0c7e01}, // h=24282 steps=0
    {1760565122, 0x1c0c8842}, // h=24283 steps=0
    {1760565362, 0x1c0c925f}, // h=24284 steps=0
    {1760565602, 0x1c0c9c57}, // h=24285 steps=0
    {1760565842, 0x1c0ca62e}, // h=24286 steps=0
    {1760566082, 0x1c0cafe1}, // h=24287 steps=0
    {1760566322, 0x1c0cb976}, // h=24288 steps=0
    {1760566562, 0x1c0cc2f2}, // h=24289 steps=0
    {1760566802, 0x1c0ccc54}, // h=24290 steps=0
    {1760567042, 0x1c0cd59f}, // h=24291 steps=0
    {1760567282, 0x1c0cded6}, // h=24292 steps=0
    {1760567522, 0x1c0ce7ff}, // h=24293 steps=0
    {1760567762, 0x1c0cf120}, // h=24294 steps=0
    {1760568002, 0x1c0cfa3f}, // h=24295 steps=0
    {1760568242, 0x1c0d0360}, // h=24296 steps=0
    {1760568482, 0x1c0d0c81}, // h=24297 steps=0
    {1760568722, 0x1c0d1720}, // h=24298 steps=0
    {1760568962, 0x1c0d1e69}, // h=24299 steps=0
    {1760569202, 0x1c0d26d5}, // h=24300 steps=0
    {1760569442, 0x1c0d2f53}, // h=24301 steps=0
    {1760569682, 0x1c0d37e1}, // h=24302 steps=0
    {1760569922, 0x1c0d407a}, // h=24303 steps=0
    {1760570162, 0x1c0d4925}, // h=24304 steps=0
    {1760570402, 0x1c0d51cb}, // h=24305 steps=0
    {1760570642, 0x1c0d5a6f}, // h=24306 steps=0
    {1760570882, 0x1c0d633b}, // h=24307 steps=0
    {1760571122, 0x1c0d6bf3}, // h=24308 steps=0
    {1760571362, 0x1c0d7495}, // h=24309 steps=0
    {1760571602, 0x1c0d7d52}, // h=24310 steps=0
    {1760571842, 0x1c0d85e5}, // h=24311 steps=0
    {1760572082, 0x1c0d8e4c}, // h=24312 steps=0
    {1760572322, 0x1c0d9723}, // h=24313 steps=0
    {1760572562, 0x1c0d9fbc}, // h=24314 steps=0
    {1760572802, 0x1c0c8b01}, // h=24315 steps=0
    {1760573042, 0x1c0c8af5}, // h=24316 steps=0
    {1760573282, 0x1c0c8e99}, // h=24317 steps=0
    {1760573522, 0x1c0c91eb}, // h=24318 steps=0
    {1760573762, 0x1c0c94ea}, // h=24319 steps=0
    {1760574002, 0x1c0c9794}, // h=24320 steps=0
    {1760574242, 0x1c0c99e8}, // h=24321 steps=0
    {1760574482, 0x1c0ca028}, // h=24322 steps=0
    {1760574722, 0x1c0ca63f}, // h=24323 steps=0
    {1760574962, 0x1c0cac2c}, // h=24324 steps=0
}};

// Chain 2 (mainnet params): window full of difficulty 5,500,000 blocks, then stalls of 18, 24 and 24 (capped) steps, then recovery
// first height 931, 198 blocks; check GetNextWorkRequired from height 1001
static const LwmaVector VEC_PUMPED_STALL = {931, 1001, {
    {1760483440, 0x1a030ce4}, // h=931 steps=0
    {1760483680, 0x1a030ce4}, // h=932 steps=0
    {1760483920, 0x1a030ce4}, // h=933 steps=0
    {1760484160, 0x1a030ce4}, // h=934 steps=0
    {1760484400, 0x1a030ce4}, // h=935 steps=0
    {1760484640, 0x1a030ce4}, // h=936 steps=0
    {1760484880, 0x1a030ce4}, // h=937 steps=0
    {1760485120, 0x1a030ce4}, // h=938 steps=0
    {1760485360, 0x1a030ce4}, // h=939 steps=0
    {1760485600, 0x1a030ce4}, // h=940 steps=0
    {1760485840, 0x1a030ce4}, // h=941 steps=0
    {1760486080, 0x1a030ce4}, // h=942 steps=0
    {1760486320, 0x1a030ce4}, // h=943 steps=0
    {1760486560, 0x1a030ce4}, // h=944 steps=0
    {1760486800, 0x1a030ce4}, // h=945 steps=0
    {1760487040, 0x1a030ce4}, // h=946 steps=0
    {1760487280, 0x1a030ce4}, // h=947 steps=0
    {1760487520, 0x1a030ce4}, // h=948 steps=0
    {1760487760, 0x1a030ce4}, // h=949 steps=0
    {1760488000, 0x1a030ce4}, // h=950 steps=0
    {1760488240, 0x1a030ce4}, // h=951 steps=0
    {1760488480, 0x1a030ce4}, // h=952 steps=0
    {1760488720, 0x1a030ce4}, // h=953 steps=0
    {1760488960, 0x1a030ce4}, // h=954 steps=0
    {1760489200, 0x1a030ce4}, // h=955 steps=0
    {1760489440, 0x1a030ce4}, // h=956 steps=0
    {1760489680, 0x1a030ce4}, // h=957 steps=0
    {1760489920, 0x1a030ce4}, // h=958 steps=0
    {1760490160, 0x1a030ce4}, // h=959 steps=0
    {1760490400, 0x1a030ce4}, // h=960 steps=0
    {1760490640, 0x1a030ce4}, // h=961 steps=0
    {1760490880, 0x1a030ce4}, // h=962 steps=0
    {1760491120, 0x1a030ce4}, // h=963 steps=0
    {1760491360, 0x1a030ce4}, // h=964 steps=0
    {1760491600, 0x1a030ce4}, // h=965 steps=0
    {1760491840, 0x1a030ce4}, // h=966 steps=0
    {1760492080, 0x1a030ce4}, // h=967 steps=0
    {1760492320, 0x1a030ce4}, // h=968 steps=0
    {1760492560, 0x1a030ce4}, // h=969 steps=0
    {1760492800, 0x1a030ce4}, // h=970 steps=0
    {1760493040, 0x1a030ce4}, // h=971 steps=0
    {1760493280, 0x1a030ce4}, // h=972 steps=0
    {1760493520, 0x1a030ce4}, // h=973 steps=0
    {1760493760, 0x1a030ce4}, // h=974 steps=0
    {1760494000, 0x1a030ce4}, // h=975 steps=0
    {1760494240, 0x1a030ce4}, // h=976 steps=0
    {1760494480, 0x1a030ce4}, // h=977 steps=0
    {1760494720, 0x1a030ce4}, // h=978 steps=0
    {1760494960, 0x1a030ce4}, // h=979 steps=0
    {1760495200, 0x1a030ce4}, // h=980 steps=0
    {1760495440, 0x1a030ce4}, // h=981 steps=0
    {1760495680, 0x1a030ce4}, // h=982 steps=0
    {1760495920, 0x1a030ce4}, // h=983 steps=0
    {1760496160, 0x1a030ce4}, // h=984 steps=0
    {1760496400, 0x1a030ce4}, // h=985 steps=0
    {1760496640, 0x1a030ce4}, // h=986 steps=0
    {1760496880, 0x1a030ce4}, // h=987 steps=0
    {1760497120, 0x1a030ce4}, // h=988 steps=0
    {1760497360, 0x1a030ce4}, // h=989 steps=0
    {1760497600, 0x1a030ce4}, // h=990 steps=0
    {1760497840, 0x1a030ce4}, // h=991 steps=0
    {1760498080, 0x1a030ce4}, // h=992 steps=0
    {1760498320, 0x1a030ce4}, // h=993 steps=0
    {1760498560, 0x1a030ce4}, // h=994 steps=0
    {1760498800, 0x1a030ce4}, // h=995 steps=0
    {1760499040, 0x1a030ce4}, // h=996 steps=0
    {1760499280, 0x1a030ce4}, // h=997 steps=0
    {1760499520, 0x1a030ce4}, // h=998 steps=0
    {1760499760, 0x1a030ce4}, // h=999 steps=0
    {1760500000, 0x1a030ce4}, // h=1000 steps=0
    {1760505520, 0x1c0c338c}, // h=1001 steps=18
    {1760505760, 0x1a074740}, // h=1002 steps=0
    {1760506000, 0x1a0757d3}, // h=1003 steps=0
    {1760506240, 0x1a07689e}, // h=1004 steps=0
    {1760506480, 0x1a0779a3}, // h=1005 steps=0
    {1760506720, 0x1a078ae1}, // h=1006 steps=0
    {1760513680, 0x1d00ffff}, // h=1007 steps=24
    {1760522320, 0x1d00ffff}, // h=1008 steps=24
    {1760522560, 0x1a0cb654}, // h=1009 steps=0
    {1760522800, 0x1a0ce071}, // h=1010 steps=0
    {1760523040, 0x1a0d0ae6}, // h=1011 steps=0
    {1760523280, 0x1a0d35b1}, // h=1012 steps=0
    {1760523520, 0x1a0d60cf}, // h=1013 steps=0
    {1760523760, 0x1a0d8c3e}, // h=1014 steps=0
    {1760524000, 0x1a0db7fc}, // h=1015 steps=0
    {1760524240, 0x1a0de405}, // h=1016 steps=0
    {1760524480, 0x1a0e1057}, // h=1017 steps=0
    {1760524720, 0x1a0e3cef}, // h=1018 steps=0
    {1760524960, 0x1a0e69ca}, // h=1019 steps=0
    {1760525200, 0x1a0e96e5}, // h=1020 steps=0
    {1760525440, 0x1a0ec43c}, // h=1021 steps=0
    {1760525680, 0x1a0ef1cc}, // h=1022 steps=0
    {1760525920, 0x1a0f1f92}, // h=1023 steps=0
    {1760526160, 0x1a0f4d8a}, // h=1024 steps=0
    {1760526400, 0x1a0f7bb0}, // h=1025 steps=0
    {1760526640, 0x1a0faa01}, // h=1026 steps=0
    {1760526880, 0x1a0fd879}, // h=1027 steps=0
    {1760527120, 0x1a100713}, // h=1028 steps=0
    {1760527360, 0x1a1035cc}, // h=1029 steps=0
    {1760527600, 0x1a1064a0}, // h=1030 steps=0
    {1760527840, 0x1a109389}, // h=1031 steps=0
    {1760528080, 0x1a10c284}, // h=1032 steps=0
    {1760528320, 0x1a10f18c}, // h=1033 steps=0
    {1760528560, 0x1a11209c}, // h=1034 steps=0
    {1760528800, 0x1a114fb0}, // h=1035 steps=0
    {1760529040, 0x1a117ec3}, // h=1036 steps=0
    {1760529280, 0x1a11adcf}, // h=1037 steps=0
    {1760529520, 0x1a11dcd0}, // h=1038 steps=0
    {1760529760, 0x1a120bc1}, // h=1039 steps=0
    {1760530000, 0x1a123a9c}, // h=1040 steps=0
    {1760530240, 0x1a12695c}, // h=1041 steps=0
    {1760530480, 0x1a1297fb}, // h=1042 steps=0
    {1760530720, 0x1a12c674}, // h=1043 steps=0
    {1760530960, 0x1a12f4c2}, // h=1044 steps=0
    {1760531200, 0x1a1322de}, // h=1045 steps=0
    {1760531440, 0x1a1350c3}, // h=1046 steps=0
    {1760531680, 0x1a137e6a}, // h=1047 steps=0
    {1760531920, 0x1a13abcf}, // h=1048 steps=0
    {1760532160, 0x1a13d8ea}, // h=1049 steps=0
    {1760532400, 0x1a1405b5}, // h=1050 steps=0
    {1760532640, 0x1a14322c}, // h=1051 steps=0
    {1760532880, 0x1a145e46}, // h=1052 steps=0
    {1760533120, 0x1a1489fe}, // h=1053 steps=0
    {1760533360, 0x1a14b54d}, // h=1054 steps=0
    {1760533600, 0x1a14e02d}, // h=1055 steps=0
    {1760533840, 0x1a150a97}, // h=1056 steps=0
    {1760534080, 0x1a153485}, // h=1057 steps=0
    {1760534320, 0x1a155df0}, // h=1058 steps=0
    {1760534560, 0x1a1586d1}, // h=1059 steps=0
    {1760534800, 0x1a15af22}, // h=1060 steps=0
    {1760535040, 0x1a15d6db}, // h=1061 steps=0
    {1760535280, 0x1a12ace9}, // h=1062 steps=0
    {1760535520, 0x1a12c5c5}, // h=1063 steps=0
    {1760535760, 0x1a12de3d}, // h=1064 steps=0
    {1760536000, 0x1a12f64e}, // h=1065 steps=0
    {1760536240, 0x1a130df5}, // h=1066 steps=0
    {1760536480, 0x1a13252f}, // h=1067 steps=0
    {1760536720, 0x1a124ab8}, // h=1068 steps=0
    {1760536960, 0x1a117af1}, // h=1069 steps=0
    {1760537200, 0x1a118f49}, // h=1070 steps=0
    {1760537440, 0x1a11a344}, // h=1071 steps=0
    {1760537680, 0x1a11b6e0}, // h=1072 steps=0
    {1760537920, 0x1a11ca18}, // h=1073 steps=0
    {1760538160, 0x1a11dcea}, // h=1074 steps=0
    {1760538400, 0x1a11ef53}, // h=1075 steps=0
    {1760538640, 0x1a120151}, // h=1076 steps=0
    {1760538880, 0x1a1212df}, // h=1077 steps=0
    {1760539120, 0x1a1223fa}, // h=1078 steps=0
    {1760539360, 0x1a1234a1}, // h=1079 steps=0
    {1760539600, 0x1a1244cf}, // h=1080 steps=0
    {1760539840, 0x1a125482}, // h=1081 steps=0
    {1760540080, 0x1a1263b6}, // h=1082 steps=0
    {1760540320, 0x1a127269}, // h=1083 steps=0
    {1760540560, 0x1a128098}, // h=1084 steps=0
    {1760540800, 0x1a128e3e}, // h=1085 steps=0
    {1760541040, 0x1a129b5a}, // h=1086 steps=0
    {1760541280, 0x1a12a7e8}, // h=1087 steps=0
    {1760541520, 0x1a12b3e6}, // h=1088 steps=0
    {1760541760, 0x1a12bf50}, // h=1089 steps=0
    {1760542000, 0x1a12ca23}, // h=1090 steps=0
    {1760542240, 0x1a12d45d}, // h=1091 steps=0
    {1760542480, 0x1a12ddfa}, // h=1092 steps=0
    {1760542720, 0x1a12e6f8}, // h=1093 steps=0
    {1760542960, 0x1a12ef53}, // h=1094 steps=0
    {1760543200, 0x1a12f709}, // h=1095 steps=0
    {1760543440, 0x1a12fe18}, // h=1096 steps=0
    {1760543680, 0x1a13047b}, // h=1097 steps=0
    {1760543920, 0x1a130a31}, // h=1098 steps=0
    {1760544160, 0x1a130f37}, // h=1099 steps=0
    {1760544400, 0x1a13138a}, // h=1100 steps=0
    {1760544640, 0x1a131728}, // h=1101 steps=0
    {1760544880, 0x1a131a0d}, // h=1102 steps=0
    {1760545120, 0x1a131c38}, // h=1103 steps=0
    {1760545360, 0x1a131da6}, // h=1104 steps=0
    {1760545600, 0x1a131e55}, // h=1105 steps=0
    {1760545840, 0x1a131e41}, // h=1106 steps=0
    {1760546080, 0x1a131d6a}, // h=1107 steps=0
    {1760546320, 0x1a131bcc}, // h=1108 steps=0
    {1760546560, 0x1a131965}, // h=1109 steps=0
    {1760546800, 0x1a131634}, // h=1110 steps=0
    {1760547040, 0x1a131236}, // h=1111 steps=0
    {1760547280, 0x1a130d6a}, // h=1112 steps=0
    {1760547520, 0x1a1307cd}, // h=1113 steps=0
    {1760547760, 0x1a13015d}, // h=1114 steps=0
    {1760548000, 0x1a12fa19}, // h=1115 steps=0
    {1760548240, 0x1a12f1ff}, // h=1116 steps=0
    {1760548480, 0x1a12e90d}, // h=1117 steps=0
    {1760548720, 0x1a12df43}, // h=1118 steps=0
    {1760548960, 0x1a12d49e}, // h=1119 steps=0
    {1760549200, 0x1a12c91d}, // h=1120 steps=0
    {1760549440, 0x1a12bcbf}, // h=1121 steps=0
    {1760549680, 0x1a12af83}, // h=1122 steps=0
    {1760549920, 0x1a12af8e}, // h=1123 steps=0
    {1760550160, 0x1a12af2f}, // h=1124 steps=0
    {1760550400, 0x1a12ae66}, // h=1125 steps=0
    {1760550640, 0x1a12ad34}, // h=1126 steps=0
    {1760550880, 0x1a12ab97}, // h=1127 steps=0
    {1760551120, 0x1a12a990}, // h=1128 steps=0
}};

// Chain 3 (regtest powLimit 0x207fffff, T=600): 6T spacing at powLimit (256-bit overflow guard), fast blocks, stalls
// first height 230, 177 blocks; check GetNextWorkRequired from height 300
static const LwmaVector VEC_REGTEST = {230, 300, {
    {1760251600, 0x207fffff}, // h=230 steps=0
    {1760255200, 0x207fffff}, // h=231 steps=0
    {1760258800, 0x207fffff}, // h=232 steps=0
    {1760262400, 0x207fffff}, // h=233 steps=0
    {1760266000, 0x207fffff}, // h=234 steps=0
    {1760269600, 0x207fffff}, // h=235 steps=0
    {1760273200, 0x207fffff}, // h=236 steps=0
    {1760276800, 0x207fffff}, // h=237 steps=0
    {1760280400, 0x207fffff}, // h=238 steps=0
    {1760284000, 0x207fffff}, // h=239 steps=0
    {1760287600, 0x207fffff}, // h=240 steps=0
    {1760291200, 0x207fffff}, // h=241 steps=0
    {1760294800, 0x207fffff}, // h=242 steps=0
    {1760298400, 0x207fffff}, // h=243 steps=0
    {1760302000, 0x207fffff}, // h=244 steps=0
    {1760305600, 0x207fffff}, // h=245 steps=0
    {1760309200, 0x207fffff}, // h=246 steps=0
    {1760312800, 0x207fffff}, // h=247 steps=0
    {1760316400, 0x207fffff}, // h=248 steps=0
    {1760320000, 0x207fffff}, // h=249 steps=0
    {1760323600, 0x207fffff}, // h=250 steps=0
    {1760327200, 0x207fffff}, // h=251 steps=0
    {1760330800, 0x207fffff}, // h=252 steps=0
    {1760334400, 0x207fffff}, // h=253 steps=0
    {1760338000, 0x207fffff}, // h=254 steps=0
    {1760341600, 0x207fffff}, // h=255 steps=0
    {1760345200, 0x207fffff}, // h=256 steps=0
    {1760348800, 0x207fffff}, // h=257 steps=0
    {1760352400, 0x207fffff}, // h=258 steps=0
    {1760356000, 0x207fffff}, // h=259 steps=0
    {1760359600, 0x207fffff}, // h=260 steps=0
    {1760363200, 0x207fffff}, // h=261 steps=0
    {1760366800, 0x207fffff}, // h=262 steps=0
    {1760370400, 0x207fffff}, // h=263 steps=0
    {1760374000, 0x207fffff}, // h=264 steps=0
    {1760377600, 0x207fffff}, // h=265 steps=0
    {1760381200, 0x207fffff}, // h=266 steps=0
    {1760384800, 0x207fffff}, // h=267 steps=0
    {1760388400, 0x207fffff}, // h=268 steps=0
    {1760392000, 0x207fffff}, // h=269 steps=0
    {1760395600, 0x207fffff}, // h=270 steps=0
    {1760399200, 0x207fffff}, // h=271 steps=0
    {1760402800, 0x207fffff}, // h=272 steps=0
    {1760406400, 0x207fffff}, // h=273 steps=0
    {1760410000, 0x207fffff}, // h=274 steps=0
    {1760413600, 0x207fffff}, // h=275 steps=0
    {1760417200, 0x207fffff}, // h=276 steps=0
    {1760420800, 0x207fffff}, // h=277 steps=0
    {1760424400, 0x207fffff}, // h=278 steps=0
    {1760428000, 0x207fffff}, // h=279 steps=0
    {1760431600, 0x207fffff}, // h=280 steps=0
    {1760435200, 0x207fffff}, // h=281 steps=0
    {1760438800, 0x207fffff}, // h=282 steps=0
    {1760442400, 0x207fffff}, // h=283 steps=0
    {1760446000, 0x207fffff}, // h=284 steps=0
    {1760449600, 0x207fffff}, // h=285 steps=0
    {1760453200, 0x207fffff}, // h=286 steps=0
    {1760456800, 0x207fffff}, // h=287 steps=0
    {1760460400, 0x207fffff}, // h=288 steps=0
    {1760464000, 0x207fffff}, // h=289 steps=0
    {1760467600, 0x207fffff}, // h=290 steps=0
    {1760471200, 0x207fffff}, // h=291 steps=0
    {1760474800, 0x207fffff}, // h=292 steps=0
    {1760478400, 0x207fffff}, // h=293 steps=0
    {1760482000, 0x207fffff}, // h=294 steps=0
    {1760485600, 0x207fffff}, // h=295 steps=0
    {1760489200, 0x207fffff}, // h=296 steps=0
    {1760492800, 0x207fffff}, // h=297 steps=0
    {1760496400, 0x207fffff}, // h=298 steps=0
    {1760500000, 0x207fffff}, // h=299 steps=0
    {1760503600, 0x207fffff}, // h=300 steps=1
    {1760507200, 0x207fffff}, // h=301 steps=1
    {1760510800, 0x207fffff}, // h=302 steps=1
    {1760514400, 0x207fffff}, // h=303 steps=1
    {1760518000, 0x207fffff}, // h=304 steps=1
    {1760518001, 0x207fffff}, // h=305 steps=0
    {1760518002, 0x207fffff}, // h=306 steps=0
    {1760518003, 0x207fffff}, // h=307 steps=0
    {1760518004, 0x207fffff}, // h=308 steps=0
    {1760518005, 0x207fffff}, // h=309 steps=0
    {1760518006, 0x207fffff}, // h=310 steps=0
    {1760518007, 0x207fffff}, // h=311 steps=0
    {1760518008, 0x207fffff}, // h=312 steps=0
    {1760518009, 0x207fffff}, // h=313 steps=0
    {1760518010, 0x207fffff}, // h=314 steps=0
    {1760518011, 0x207fffff}, // h=315 steps=0
    {1760518012, 0x207fffff}, // h=316 steps=0
    {1760518013, 0x207fffff}, // h=317 steps=0
    {1760518014, 0x207fffff}, // h=318 steps=0
    {1760518015, 0x207fffff}, // h=319 steps=0
    {1760518016, 0x207fffff}, // h=320 steps=0
    {1760518017, 0x207fffff}, // h=321 steps=0
    {1760518018, 0x207fffff}, // h=322 steps=0
    {1760518019, 0x207fffff}, // h=323 steps=0
    {1760518020, 0x207fffff}, // h=324 steps=0
    {1760518021, 0x207fffff}, // h=325 steps=0
    {1760518022, 0x207fffff}, // h=326 steps=0
    {1760518023, 0x207fffff}, // h=327 steps=0
    {1760518024, 0x207fffff}, // h=328 steps=0
    {1760518025, 0x207fffff}, // h=329 steps=0
    {1760518026, 0x207fffff}, // h=330 steps=0
    {1760518027, 0x207fffff}, // h=331 steps=0
    {1760518028, 0x207fffff}, // h=332 steps=0
    {1760518029, 0x207fffff}, // h=333 steps=0
    {1760518030, 0x207fffff}, // h=334 steps=0
    {1760518031, 0x207fffff}, // h=335 steps=0
    {1760518032, 0x207fffff}, // h=336 steps=0
    {1760518033, 0x207fffff}, // h=337 steps=0
    {1760518034, 0x207fffff}, // h=338 steps=0
    {1760518035, 0x207fffff}, // h=339 steps=0
    {1760518036, 0x207fffff}, // h=340 steps=0
    {1760518037, 0x207e1479}, // h=341 steps=0
    {1760518038, 0x2073fb4d}, // h=342 steps=0
    {1760518039, 0x206a2af7}, // h=343 steps=0
    {1760518040, 0x2060ada2}, // h=344 steps=0
    {1760518041, 0x20578c65}, // h=345 steps=0
    {1760518042, 0x204ecf43}, // h=346 steps=0
    {1760518043, 0x20467d22}, // h=347 steps=0
    {1760518044, 0x203e9bcf}, // h=348 steps=0
    {1760518045, 0x20373001}, // h=349 steps=0
    {1760518046, 0x20303d5d}, // h=350 steps=0
    {1760518047, 0x2029c684}, // h=351 steps=0
    {1760518048, 0x2023cd1b}, // h=352 steps=0
    {1760518049, 0x201e51d9}, // h=353 steps=0
    {1760518050, 0x20195498}, // h=354 steps=0
    {1760518051, 0x2014d464}, // h=355 steps=0
    {1760518052, 0x2010cf8a}, // h=356 steps=0
    {1760518053, 0x200d43ad}, // h=357 steps=0
    {1760518054, 0x200a2dd8}, // h=358 steps=0
    {1760518055, 0x20078a90}, // h=359 steps=0
    {1760518056, 0x200555e3}, // h=360 steps=0
    {1760518057, 0x20038b7e}, // h=361 steps=0
    {1760518058, 0x200226bb}, // h=362 steps=0
    {1760518059, 0x200122b4}, // h=363 steps=0
    {1760518060, 0x1f7a4ff2}, // h=364 steps=0
    {1760518061, 0x1f285271}, // h=365 steps=0
    {1760518062, 0x1f2769b6}, // h=366 steps=0
    {1760518063, 0x1f2680f9}, // h=367 steps=0
    {1760518064, 0x1f25983b}, // h=368 steps=0
    {1760518065, 0x1f24af7b}, // h=369 steps=0
    {1760518066, 0x1f23c6ba}, // h=370 steps=0
    {1760518067, 0x1f22ddf6}, // h=371 steps=0
    {1760518068, 0x1f21f531}, // h=372 steps=0
    {1760518069, 0x1f210c6b}, // h=373 steps=0
    {1760518070, 0x1f2023a3}, // h=374 steps=0
    {1760524670, 0x2007ceb6}, // h=375 steps=6
    {1760588270, 0x207fffff}, // h=376 steps=24
    {1760588870, 0x201b0d3f}, // h=377 steps=0
    {1760589470, 0x201c2807}, // h=378 steps=0
    {1760590070, 0x201d252e}, // h=379 steps=0
    {1760590670, 0x201e0586}, // h=380 steps=0
    {1760591270, 0x201ec9d5}, // h=381 steps=0
    {1760591870, 0x201f72d3}, // h=382 steps=0
    {1760592470, 0x2020012b}, // h=383 steps=0
    {1760593070, 0x2020757d}, // h=384 steps=0
    {1760593670, 0x2020d05c}, // h=385 steps=0
    {1760594270, 0x20211253}, // h=386 steps=0
    {1760594870, 0x20213be2}, // h=387 steps=0
    {1760595470, 0x20214d82}, // h=388 steps=0
    {1760596070, 0x202147a2}, // h=389 steps=0
    {1760596670, 0x20212aab}, // h=390 steps=0
    {1760597270, 0x2020f6ff}, // h=391 steps=0
    {1760597870, 0x2020acfb}, // h=392 steps=0
    {1760598470, 0x20204cf4}, // h=393 steps=0
    {1760599070, 0x201fd73c}, // h=394 steps=0
    {1760599670, 0x201f4c22}, // h=395 steps=0
    {1760600270, 0x201eabee}, // h=396 steps=0
    {1760600870, 0x201df6e8}, // h=397 steps=0
    {1760601470, 0x201d2d53}, // h=398 steps=0
    {1760602070, 0x201c4f72}, // h=399 steps=0
    {1760602670, 0x201b5d86}, // h=400 steps=0
    {1760603270, 0x201a57cd}, // h=401 steps=0
    {1760603870, 0x201945c1}, // h=402 steps=0
    {1760604470, 0x20184749}, // h=403 steps=0
    {1760605070, 0x20175da9}, // h=404 steps=0
    {1760605670, 0x201689dc}, // h=405 steps=0
    {1760606270, 0x2015cc91}, // h=406 steps=0
}};


static std::vector<CBlockIndex> BuildChain(const LwmaVector& vec)
{
    std::vector<CBlockIndex> chain(vec.blocks.size());
    for (size_t i = 0; i < chain.size(); ++i) {
        chain[i].pprev = i ? &chain[i - 1] : nullptr;
        chain[i].nHeight = vec.nFirstHeight + static_cast<int>(i);
        chain[i].nTime = static_cast<uint32_t>(vec.blocks[i].first);
        chain[i].nBits = vec.blocks[i].second;
    }
    return chain;
}

static void CheckLwmaVector(const LwmaVector& vec, const Consensus::Params& params)
{
    std::vector<CBlockIndex> chain = BuildChain(vec);
    int nChecked = 0;
    for (size_t i = 1; i < chain.size(); ++i) {
        if (chain[i].nHeight < vec.nFirstCheckHeight) continue;
        CBlockHeader header;
        header.nTime = chain[i].nTime;
        header.nBits = chain[i].nBits;
        const unsigned int nRequired = GetNextWorkRequired(&chain[i - 1], &header, params);
        BOOST_CHECK_MESSAGE(nRequired == chain[i].nBits,
                            strprintf("height %d time %d: required %08x, vector %08x", chain[i].nHeight, chain[i].nTime, nRequired, chain[i].nBits));
        ++nChecked;
    }
    BOOST_CHECK(nChecked > 0);
}

BOOST_AUTO_TEST_CASE(lwma_vectors_mainnet_transition)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    params.nLwmaActivationHeight = 24092 + 50; // the vector was generated for this height
    BOOST_CHECK_EQUAL(params.nPowTargetSpacing, 240);
    BOOST_CHECK_EQUAL(params.nLwmaWindow, 60);
    CheckLwmaVector(VEC_MAINNET_TRANSITION, params);
}

BOOST_AUTO_TEST_CASE(lwma_vectors_pumped_stall)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    params.nLwmaActivationHeight = 1001;
    CheckLwmaVector(VEC_PUMPED_STALL, params);
}

BOOST_AUTO_TEST_CASE(lwma_vectors_regtest_powlimit)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus();
    params.nLwmaActivationHeight = 300;
    BOOST_CHECK_EQUAL(params.nPowTargetSpacing, 600);
    CheckLwmaVector(VEC_REGTEST, params);
}

/* Blocks below the activation height must be governed by the legacy rule whatever their timing. */
BOOST_AUTO_TEST_CASE(lwma_activation_boundary)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    params.nLwmaActivationHeight = 24092 + 50;
    std::vector<CBlockIndex> chain = BuildChain(VEC_MAINNET_TRANSITION);
    const int nAct = params.nLwmaActivationHeight;
    BOOST_CHECK(!IsLwmaActive(nAct - 1, params));
    BOOST_CHECK(IsLwmaActive(nAct, params));

    // Legacy: a five-year gap changes nothing at a non-retarget height.
    CBlockHeader header;
    const CBlockIndex* pindexPrev = &chain[nAct - 2 - VEC_MAINNET_TRANSITION.nFirstHeight]; // parent of block nAct-1
    BOOST_CHECK_EQUAL(pindexPrev->nHeight + 1, nAct - 1);
    header.nTime = pindexPrev->nTime + 5 * 365 * 24 * 3600;
    BOOST_CHECK_EQUAL(GetNextWorkRequired(pindexPrev, &header, params), pindexPrev->nBits);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(pindexPrev, header.nTime, params), 0);

    // First v1.1 block: the same gap is an emergency of 24 steps, capped at powLimit.
    pindexPrev = &chain[nAct - 1 - VEC_MAINNET_TRANSITION.nFirstHeight];
    BOOST_CHECK_EQUAL(pindexPrev->nHeight + 1, nAct);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(pindexPrev, header.nTime, params), 24);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(pindexPrev, &header, params), 0x1d00ffffU);
    // ... and with a normal timestamp it is the LWMA base target, below powLimit.
    header.nTime = pindexPrev->nTime + 240;
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(pindexPrev, header.nTime, params), 0);
    BOOST_CHECK_EQUAL(GetNextWorkRequired(pindexPrev, &header, params), LwmaNextWorkRequired(pindexPrev, params));
    BOOST_CHECK(GetNextWorkRequired(pindexPrev, &header, params) != 0x1d00ffffU);
}

/* Emergency rule arithmetic: reference = max of the last 11 timestamps, steps = 1 + floor((gap - 6T)/T), cap 24. */
BOOST_AUTO_TEST_CASE(emergency_easing_steps)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    params.nLwmaActivationHeight = 0;
    const int64_t T = params.nPowTargetSpacing;
    std::vector<CBlockIndex> chain(20);
    for (size_t i = 0; i < chain.size(); ++i) {
        chain[i].pprev = i ? &chain[i - 1] : nullptr;
        chain[i].nHeight = static_cast<int>(i);
        chain[i].nTime = 1000000 + static_cast<uint32_t>(i) * 100;
        chain[i].nBits = 0x1c00ffff;
    }
    // Backdate the tip below an older block: the reference is the maximum, not the tip.
    chain[19].nTime = chain[19].nTime - 500;
    const CBlockIndex* tip = &chain[19];
    const int64_t nRef = chain[18].nTime;
    BOOST_CHECK_EQUAL(EmergencyReferenceTime(tip, params), nRef);
    // Only the last 11 blocks count.
    chain[8].nTime = 9000000;
    BOOST_CHECK_EQUAL(EmergencyReferenceTime(tip, params), nRef);
    chain[9].nTime = 9000000;
    BOOST_CHECK_EQUAL(EmergencyReferenceTime(tip, params), 9000000);
    chain[9].nTime = 1000900;

    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef - 100000, params), 0);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 6 * T - 1, params), 0);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 6 * T, params), 1);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 7 * T - 1, params), 1);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 7 * T, params), 2);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 29 * T, params), 24);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 30 * T, params), 24);
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 5 * 365 * 86400, params), 24);
    // Below activation there is never an emergency.
    params.nLwmaActivationHeight = 100;
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(tip, nRef + 30 * T, params), 0);
}

/* Target doubling per step, capped at powLimit, with exact compact rounding. */
BOOST_AUTO_TEST_CASE(apply_emergency_easing)
{
    const Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 0, params), 0x1c00ffffU);
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 1, params), 0x1c01fffeU);
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 7, params), 0x1c7fff80U);
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 8, params), 0x1d00ffffU); // exactly powLimit
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 9, params), 0x1d00ffffU); // capped
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1c00ffff, 24, params), 0x1d00ffffU);
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1d00ffff, 1, params), 0x1d00ffffU);
    // base 0x1a1234ab ~ 2^204 stays below powLimit (~2^224) for up to 19 doublings;
    // the result carries the usual compact rounding of the shifted value.
    for (int steps = 1; steps <= 19; ++steps) {
        arith_uint256 shifted;
        shifted.SetCompact(0x1a1234ab);
        shifted <<= steps;
        arith_uint256 expected;
        expected.SetCompact(shifted.GetCompact());
        arith_uint256 got;
        got.SetCompact(ApplyEmergencyEasing(0x1a1234ab, steps, params));
        BOOST_CHECK(got == expected);
        BOOST_CHECK(got < UintToArith256(params.powLimit));
    }
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x1a1234ab, 20, params), 0x1d00ffffU);
    // Regtest powLimit is close to 2^255: the shift must be guarded, not overflow to zero.
    const Consensus::Params regtest = CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus();
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x207fffff, 24, regtest), 0x207fffffU);
    BOOST_CHECK_EQUAL(ApplyEmergencyEasing(0x2000ffff, 24, regtest), 0x207fffffU);
}

/* Window credit: an eased block is credited with target >> max(0, steps - 6), recomputed from its ancestors. */
BOOST_AUTO_TEST_CASE(lwma_window_credit)
{
    Consensus::Params params = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    params.nLwmaActivationHeight = 0;
    const int64_t T = params.nPowTargetSpacing;
    std::vector<CBlockIndex> chain(13);
    for (size_t i = 0; i < chain.size(); ++i) {
        chain[i].pprev = i ? &chain[i - 1] : nullptr;
        chain[i].nHeight = static_cast<int>(i);
        chain[i].nTime = 1000000 + static_cast<uint32_t>(i) * T;
        chain[i].nBits = 0x1c00ffff;
    }
    arith_uint256 target;
    target.SetCompact(0x1c00ffff);
    BOOST_CHECK(LwmaWindowCredit(&chain[0], params) == target);   // genesis: no ancestors, no easing
    BOOST_CHECK(LwmaWindowCredit(&chain[12], params) == target);  // normal block
    chain[12].nTime = chain[11].nTime + 6 * T + 5 * T;             // 6 steps: within the cap, full credit
    BOOST_CHECK_EQUAL(EmergencyEasingSteps(&chain[11], chain[12].nTime, params), 6);
    BOOST_CHECK(LwmaWindowCredit(&chain[12], params) == target);
    chain[12].nTime = chain[11].nTime + 6 * T + 7 * T;             // 8 steps: credit = target >> 2
    BOOST_CHECK(LwmaWindowCredit(&chain[12], params) == (target >> 2));
    chain[12].nTime = chain[11].nTime + 6 * T + 40 * T;            // 24 steps: credit = target >> 18
    BOOST_CHECK(LwmaWindowCredit(&chain[12], params) == (target >> 18));
    // Below activation nothing is ever eased, so the credit is the plain target.
    params.nLwmaActivationHeight = 100;
    BOOST_CHECK(LwmaWindowCredit(&chain[12], params) == target);
}

BOOST_AUTO_TEST_SUITE_END()
