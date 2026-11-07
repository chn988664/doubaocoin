// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/doubaocoin_key_io_valid.json.h>
#include <test/data/key_io_invalid.json.h>
#include <test/data/key_io_valid.json.h>

#include <key.h>
#include <key_io.h>
#include <script/script.h>
#include <test/util/json.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>

BOOST_FIXTURE_TEST_SUITE(key_io_tests, BasicTestingSetup)

// Encoding-only fixtures retain the upstream vectors without constructing a
// Bitcoin network or exposing its parameters through a runtime factory.
class BitcoinEncodingParams : public CChainParams
{
public:
    explicit BitcoinEncodingParams(ChainType chain) : CChainParams(*CChainParams::RegTest())
    {
        const bool main = chain == ChainType::MAIN;
        base58Prefixes[PUBKEY_ADDRESS] = {static_cast<unsigned char>(main ? 0 : 111)};
        base58Prefixes[SCRIPT_ADDRESS] = {static_cast<unsigned char>(main ? 5 : 196)};
        base58Prefixes[SECRET_KEY] = {static_cast<unsigned char>(main ? 128 : 239)};
        bech32_hrp = main ? "bc" : (chain == ChainType::REGTEST ? "bcrt" : "tb");
    }
};

// Goal: check that parsed keys match test payload
static void CheckValidParse(const UniValue& tests, bool bitcoin_vectors)
{
    CKey privkey;
    CTxDestination destination;

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 3) { // Allow for extra stuff (useful for comments)
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();
        const std::vector<std::byte> exp_payload{ParseHex<std::byte>(test[1].get_str())};
        const UniValue &metadata = test[2].get_obj();
        bool isPrivkey = metadata.find_value("isPrivkey").get_bool();
        const auto chain = ChainTypeFromString(metadata.find_value("chain").get_str()).value();
        const BitcoinEncodingParams bitcoin_params{chain};
        const auto doubao_params = chain == ChainType::REGTEST ? CChainParams::RegTest() : CChainParams::TestNet4();
        const CChainParams& params = bitcoin_vectors ? static_cast<const CChainParams&>(bitcoin_params) : *doubao_params;
        const auto other_doubao_params = chain == ChainType::REGTEST ? CChainParams::TestNet4() : CChainParams::RegTest();
        for (const CChainParams* other : {other_doubao_params.get(), bitcoin_vectors ? doubao_params.get() : static_cast<const CChainParams*>(&bitcoin_params)}) {
            if (isPrivkey) {
                BOOST_CHECK_MESSAGE(!DecodeSecret(exp_base58string, *other).IsValid(), "cross-network private key accepted: " + strTest);
            } else {
                BOOST_CHECK_MESSAGE(!IsValidDestination(DecodeDestination(exp_base58string, *other)), "cross-network address accepted: " + strTest);
            }
        }
        bool try_case_flip = metadata.find_value("tryCaseFlip").isNull() ? false : metadata.find_value("tryCaseFlip").get_bool();
        if (isPrivkey) {
            bool isCompressed = metadata.find_value("isCompressed").get_bool();
            // Must be valid private key
            privkey = DecodeSecret(exp_base58string, params);
            BOOST_CHECK_MESSAGE(privkey.IsValid(), "!IsValid:" + strTest);
            BOOST_CHECK_MESSAGE(privkey.IsCompressed() == isCompressed, "compressed mismatch:" + strTest);
            BOOST_CHECK_MESSAGE(std::ranges::equal(privkey, exp_payload), "key mismatch:" + strTest);

            // Private key must be invalid public key
            destination = DecodeDestination(exp_base58string, params);
            BOOST_CHECK_MESSAGE(!IsValidDestination(destination), "IsValid privkey as pubkey:" + strTest);
        } else {
            // Must be valid public key
            destination = DecodeDestination(exp_base58string, params);
            CScript script = GetScriptForDestination(destination);
            BOOST_CHECK_MESSAGE(IsValidDestination(destination), "!IsValid:" + strTest);
            BOOST_CHECK_EQUAL(HexStr(script), HexStr(exp_payload));

            // Try flipped case version
            for (char& c : exp_base58string) {
                if (c >= 'a' && c <= 'z') {
                    c = (c - 'a') + 'A';
                } else if (c >= 'A' && c <= 'Z') {
                    c = (c - 'A') + 'a';
                }
            }
            destination = DecodeDestination(exp_base58string, params);
            BOOST_CHECK_MESSAGE(IsValidDestination(destination) == try_case_flip, "!IsValid case flipped:" + strTest);
            if (IsValidDestination(destination)) {
                script = GetScriptForDestination(destination);
                BOOST_CHECK_EQUAL(HexStr(script), HexStr(exp_payload));
            }

            // Public key must be invalid private key
            privkey = DecodeSecret(exp_base58string, params);
            BOOST_CHECK_MESSAGE(!privkey.IsValid(), "IsValid pubkey as privkey:" + strTest);
        }
    }
}

BOOST_AUTO_TEST_CASE(key_io_valid_parse)
{
    CheckValidParse(read_json(json_tests::key_io_valid), true);
    CheckValidParse(read_json(json_tests::doubaocoin_key_io_valid), false);
}

// Goal: check that generated keys match test vectors
static void CheckValidGen(const UniValue& tests, bool bitcoin_vectors)
{
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 3) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();
        std::vector<unsigned char> exp_payload = ParseHex(test[1].get_str());
        const UniValue &metadata = test[2].get_obj();
        bool isPrivkey = metadata.find_value("isPrivkey").get_bool();
        const auto chain = ChainTypeFromString(metadata.find_value("chain").get_str()).value();
        const BitcoinEncodingParams bitcoin_params{chain};
        const auto doubao_params = chain == ChainType::REGTEST ? CChainParams::RegTest() : CChainParams::TestNet4();
        const CChainParams& params = bitcoin_vectors ? static_cast<const CChainParams&>(bitcoin_params) : *doubao_params;
        if (isPrivkey) {
            bool isCompressed = metadata.find_value("isCompressed").get_bool();
            CKey key;
            key.Set(exp_payload.begin(), exp_payload.end(), isCompressed);
            assert(key.IsValid());
            BOOST_CHECK_MESSAGE(EncodeSecret(key, params) == exp_base58string, "result mismatch: " + strTest);
        } else {
            CTxDestination dest;
            CScript exp_script(exp_payload.begin(), exp_payload.end());
            BOOST_CHECK(ExtractDestination(exp_script, dest));
            std::string address = EncodeDestination(dest, params);

            BOOST_CHECK_EQUAL(address, exp_base58string);
        }
    }

}

BOOST_AUTO_TEST_CASE(key_io_valid_gen)
{
    CheckValidGen(read_json(json_tests::key_io_valid), true);
    CheckValidGen(read_json(json_tests::doubaocoin_key_io_valid), false);
}

// Goal: check that base58 parsing code is robust against a variety of corrupted data
BOOST_AUTO_TEST_CASE(key_io_invalid)
{
    UniValue tests = read_json(json_tests::key_io_invalid); // Negative testcases
    CKey privkey;
    CTxDestination destination;

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 1) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();

        // must be invalid as public and as private key
        for (const auto& chain : {ChainType::MAIN, ChainType::TESTNET, ChainType::SIGNET, ChainType::TESTNET4, ChainType::REGTEST}) {
            SelectParams(chain);
            destination = DecodeDestination(exp_base58string);
            BOOST_CHECK_MESSAGE(!IsValidDestination(destination), "IsValid pubkey in mainnet:" + strTest);
            privkey = DecodeSecret(exp_base58string);
            BOOST_CHECK_MESSAGE(!privkey.IsValid(), "IsValid privkey in mainnet:" + strTest);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
