# DouBaoCoin experimental offline tools

These tools are under development and are not audited for production. Do not use
real funds. The miner defaults to regtest; mainnet requires an explicitly supplied approved
genesis hash and separate owner authorization. It does not start a node or create
a wallet. The current node refuses to start mainnet. It requires Python 3 on macOS arm64 and
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
chain. Only synthetic test inputs may be used until the owner separately approves
formal genesis parameters and the generation procedure. See `--help`.
