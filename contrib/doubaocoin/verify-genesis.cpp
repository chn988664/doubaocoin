// Independent offline verification using Bitcoin Core's C++ block and PoW code.
// This program never starts a node, searches for a nonce, or writes a wallet.
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <pow.h>
#include <primitives/block.h>
#include <script/script.h>
#include <streams.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    try {
        uint32_t bits;
        const bool main_mode = argc == 2 && std::string{argv[1]} == "--main";
        if (main_mode) {
            bits = 0x1d00ffff;
        } else {
            require(argc == 3 && std::string{argv[1]} == "--test-bits",
                    "use --main or --test-bits HEX; block hex is read from stdin");
            const std::string bits_arg{argv[2]};
            size_t consumed{0};
            const auto parsed_bits = std::stoul(bits_arg, &consumed, 16);
            require(consumed == bits_arg.size() && parsed_bits <= 0xffffffffUL, "invalid compact target");
            bits = static_cast<uint32_t>(parsed_bits);
        }
        std::string hex;
        std::getline(std::cin, hex);
        require(IsHex(hex), "invalid block hex");
        const auto raw = ParseHex(hex);
        DataStream input{std::span<const uint8_t>{raw}};
        CBlock block;
        input >> TX_WITH_WITNESS(block);
        require(input.empty(), "trailing block data");
        DataStream encoded;
        encoded << TX_WITH_WITNESS(block);
        require(encoded.size() == raw.size() && std::memcmp(encoded.data(), raw.data(), raw.size()) == 0,
                "block serialization mismatch");
        require(block.nVersion == 1 && block.hashPrevBlock.IsNull() && block.nBits == bits,
                "invalid genesis header");
        require(block.vtx.size() == 1 && block.vtx[0] && block.vtx[0]->IsCoinBase(), "invalid coinbase count");
        const auto& tx = *block.vtx[0];
        const auto expected_sig = ParseHex("0017446f7542616f436f696e2047656e657369732032303236");
        require(tx.version == 1 && tx.nLockTime == 0 && !tx.HasWitness() &&
                tx.vin[0].nSequence == 0xffffffff &&
                std::equal(tx.vin[0].scriptSig.begin(), tx.vin[0].scriptSig.end(), expected_sig.begin(), expected_sig.end()),
                "invalid C07 coinbase input");
        require(tx.vout.size() == 1 && tx.vout[0].nValue == 50 * COIN &&
                tx.vout[0].scriptPubKey == (CScript{} << OP_RETURN) &&
                tx.vout[0].scriptPubKey.IsUnspendable(), "invalid C07 output");
        const uint256 expected_root{"334f8d85d7a7c0b6df26cccde033bc1be9a9ee6f90ac736f2f591039191ded85"};
        require(block.hashMerkleRoot == expected_root && BlockMerkleRoot(block) == expected_root, "invalid Merkle root");
        Consensus::Params consensus{};
        // Only explicit test mode uses an easier limit for synthetic blocks.
        consensus.powLimit = main_mode
            ? uint256{"00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"}
            : uint256{"7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        require(CheckProofOfWorkImpl(block.GetHash(), block.nBits, consensus), "invalid proof of work");
        std::cout << "C++ independent block, C07 and PoW verification PASS\n"
                  << "time=" << block.nTime << " nonce=" << block.nNonce
                  << " hash=" << block.GetHash().ToString() << " merkle=" << block.hashMerkleRoot.ToString() << '\n';
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "verification failed: " << exc.what() << '\n';
        return 1;
    }
}
