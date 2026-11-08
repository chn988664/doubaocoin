# DouBaoCoin experimental offline tools

These tools are under development and are not audited for production. Do not use
real funds. The miner defaults to regtest; mainnet requires an explicitly supplied approved
genesis hash and separate owner authorization. It does not start a node or create
a wallet. The mainnet genesis is finalized, but launching mainnet requires separate
owner authorization. The miner requires Python 3 on macOS arm64 and
a locally running DouBaoCoin regtest node with a test payout address. Example:

```sh
./doubaocoin-miner --cookie /path/to/test-data/regtest/.cookie \
  --port 49326 --address <regtest-address> --blocks 1
```

RPC connects only to `127.0.0.1`; credentials are read from the cookie. `--blocks
0` runs until SIGINT/SIGTERM. Work is refreshed periodically; see `--help`.
Do not expose RPC, store real keys in test wallets, or use this tool to mine
mainnet before explicit owner authorization. Independent review and additional
operational tests remain necessary.

`doubaocoin-genesis` is an offline candidate calculation tool requiring explicit
message, time, target bits and nonce range. It neither approves nor configures a
chain. The finalized genesis was generated under a separately approved procedure;
other searches require their own authorization. `--output` saves a
successful result without replacing an existing file; progress is written to
standard error. The caller must record approved timestamp and nonce ranges and
must not silently expand them if a range has no solution. See `--help`.

`doubaocoin-verify-genesis --main` reads one serialized block hex line from
standard input and independently checks C07, C02 proof of work, Merkle root
and serialization using the node's C++ implementation. `--test-bits HEX` is
only for synthetic test blocks; it does not validate a mainnet candidate.

The [wallet backup and recovery guide](wallet-backup.md) uses isolated regtest
examples and explains handling of sensitive private descriptors.
