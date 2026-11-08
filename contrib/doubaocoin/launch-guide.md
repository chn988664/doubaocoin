# DouBaoCoin 发布与连接说明

本说明对应 32.1.1 发布程序包。运行钱包或节点不会自动开始挖矿；正式网络启动公告由项目所有者发布。

## 正式网络

DouBaoCoin（豆包币），代码 DBCN，精度 8 位。初始补贴 50 DBCN，每 210000 区块减半，沿用 Bitcoin 原版补贴行为；理论补贴总和 20999999.97690000 DBCN，创世补贴不可花费，可进入可花费 UTXO 的理论最大新增补贴 20999949.97690000 DBCN。无预挖。

SHA-256d，目标间隔 600 秒，每 2016 区块调整，目标时间 1209600 秒，单次调整限制为 1/4–4 倍。无最低难度特殊规则，BIP94 关闭，Coinbase 成熟期 100。BIP34/65/66、CSV、SegWit 从高度 1 启用，保留现代 P2SH、Witness、Taproot 验证，删除 Bitcoin 历史例外。MAX_MONEY 为 21000000 × COIN。

初始 nBits 为 `0x1d00ffff`，展开目标为 `00000000ffff0000000000000000000000000000000000000000000000000000`；powLimit 为 `00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff`。首个后继区块继承该难度。600 秒是调整目标，不是单台 CPU 的实际承诺；按 T007-R1 单线程约 1055902 H/s，平均约 68 分钟一个区块，随机等待可能显著更长。参数不随 Mac 算力修改。

链名 `main`，网络 Magic `fbc7b2d4`，P2P 49321，RPC 49322。新安装直接启动即可使用 Bitcoin Core 原生节点发现，无需手工指定节点。DNS 种子：

```text
seed1.dbcn.wudoubao.us
seed2.dbcn.wudoubao.us
```

DNS 不可用时使用内置备用种子；之后通过原生地址交换和地址管理器选择对端，种子并非指定的永久连接中心。服务器之间直接连接，不以 Mac 为中转。RPC 仅绑定本机回环，不向公网开放。旧配置中的 `connect=` 会覆盖自动选择，`dnsseed=0`、`fixedseeds=0` 等显式设置也会被保留；升级时应自行核对这些早期私网设置，发布包不会覆盖用户配置。部署细节见 [原生节点发现](bootstrap.md)。

地址 P2PKH `0x1c`、P2SH `0x3f`、WIF `0x9f`、Bech32/Bech32m `dbcn`、扩展公钥 `0x02db0001`、扩展私钥 `0x02db0002`。两种旧式地址与部分其他币种存在版本冲突，所有者已接受；此风险尚未消除。默认使用 `dbcn1` Bech32 地址，付款 URI 为 `doubaocoin:`，不接受 `bitcoin:` URI。

## 创世独立复验

正式创世时间 `1794119357`（2026-11-08 06:29:17 UTC），版本 1，Nonce `3880018009`，Merkle root `334f8d85d7a7c0b6df26cccde033bc1be9a9ee6f90ac736f2f591039191ded85`，Hash `0000000007340821f32d4424dda9cb6b4f165b09fc10a83bcc1d6431fa1f95e0`。

完整序列化见 [mainnet-genesis.json](mainnet-genesis.json)。该区块已由所有者确认。创世文字为 `DouBaoCoin Genesis 2026`，交易版本 1，scriptSig `0017446f7542616f436f696e2047656e657369732032303236`，输出 50 DBCN，脚本 `6a`（OP_RETURN）；创世交易不进入 UTXO。

在源码根目录，将完整区块交给 C++ 验证器，不启动节点：

```sh
python3 -c 'import json; print(json.load(open("contrib/doubaocoin/mainnet-genesis.json"))["block"])' | \
  /path/to/bin/doubaocoin-verify-genesis --main
```

也可用独立 Python 标准库复算区块头和交易哈希：

```sh
python3 - <<'PY'
import hashlib, json
d = json.load(open('contrib/doubaocoin/mainnet-genesis.json'))
def h(x): return hashlib.sha256(hashlib.sha256(x).digest()).digest()
assert h(bytes.fromhex(d['header']))[::-1].hex() == d['block_hash']
assert h(bytes.fromhex(d['transaction']))[::-1].hex() == d['merkle_root']
assert int(d['block_hash'], 16) <= int(d['target'], 16)
assert d['block'] == d['header'] + '01' + d['transaction']
print('Genesis header, transaction, target and serialization verified')
PY
```

## 钱包与备份

默认主网收款路径 `m/84'/1145193294'/0'/0/序号`，找零 `m/84'/1145193294'/0'/1/序号`。编号由项目所有者批准，**尚未在 SLIP-0044 登记**。随机种子、描述符及数据目录与 Bitcoin 独立；不要复用 Bitcoin 钱包。钱包通过完整 wallet.dat 或私有描述符恢复，本程序没有助记词恢复功能。恢复必须保留原描述符路径。

详见 [钱包备份与恢复](wallet-backup.md)。私钥、私有描述符、钱包文件、密码和 RPC Cookie 都不能提交到源码仓库。使用矿工前，钱包所有者必须亲自备份并确认公开收款地址。

Windows 10（1809 或更新）/11 Intel/AMD x86-64 包中的 `打开豆包币钱包.cmd` 启动图形钱包，数据目录默认 `%APPDATA%\DouBaoCoin`。系统范围依据 [Qt 6.8 官方平台说明](https://doc.qt.io/qt-6.8/windows.html)，不代表本轮进行了原生 Windows 实机验收。首次创建钱包后，通过菜单备份钱包；重要备份保存在独立且受保护的介质上。启动脚本仅启用本机 RPC，采用 550 MiB 修剪目标；P2P 连接与监听沿用原生默认行为，不修改 Bitcoin 的目录。直接运行 `bin\doubaocoin-qt.exe` 也使用相同的内置种子。无需为了运行钱包关闭系统安全保护。

## Mac 本地挖矿

先由本地完整验证节点同步，再由矿工访问本机 RPC Cookie。远程 VPS 不保存本地钱包或私钥。正常公网连接无需 SSH 隧道或手工指定中心：

```sh
/path/to/bin/doubaocoind -server=1 -rpcbind=127.0.0.1 \
  -rpcallowip=127.0.0.1 -rpcport=49322 -prune=550
```

本地节点自动发现并选择 P2P 对端。RPC `rpcbind=127.0.0.1`、`rpcallowip=127.0.0.1`、`rpcport=49322`。节点数据、配置、钱包和 Cookie 仅供本机用户访问。

创世之后若超过 24 小时没有新区块，默认 IBD 年龄门槛会阻止 getblocktemplate。经本轮合理启动配置指示，准备仅在 Mac 挖矿节点临时采用 `maxtipage=604800`；它不改变共识、难度或验证。此准备范围截止 2026-11-15 06:29:17 UTC，超期须重新审批。首个区块被网络接受后，停止矿工、删除该设置并重启本地节点；五台服务器始终保留默认 86400。其他参与者使用同类启动设置时也须知悉该就绪判断的放宽，不可关闭 PoW 或伪造链尖。

```sh
# 仅在正式公开挖矿时刻之后执行；替换为自己已备份钱包的公开地址：
python3 /path/to/doubaocoin-miner \
  --cookie "$HOME/Library/Application Support/DouBaoCoin/.cookie" \
  --port 49322 --address dbcn1_PUBLIC_RECEIVING_ADDRESS \
  --mainnet-genesis 0000000007340821f32d4424dda9cb6b4f165b09fc10a83bcc1d6431fa1f95e0 \
  --blocks 1
```

矿工需要 Python 3，单线程执行实际 SHA-256d。Ctrl-C 可停止，不设置开机挖矿。遇到临时 RPC 故障会重试；节点重启后重新核对创世和地址，错误网络身份会退出。休眠和网络断开会中断有效挖矿；恢复后检查节点对端和链尖，再手动继续。首块提交成功后，核对五台服务器的高度、Hash、难度、Coinbase 收款脚本和 50 DBCN 奖励，不能只看矿工输出。

## 构建与发布限制

macOS ARM64 与 Linux x86-64 构建说明沿用仓库 CMake 文档。Windows GUI 使用固定源码中的 `depends`，Qt 6.8.4、MinGW-w64 GCC 13 或更高，见 [Windows 构建说明](../../doc/build-windows.md)。构建先校验依赖源文件哈希，再通过生成的 toolchain.cmake 编译 GUI、节点和工具。发布包应同时提供准确源码提交、构建选项、SHA-256、依赖许可证及对应源码获取材料。

Windows x86-64 在 Ubuntu 24.04 上的本轮配置如下（需先安装仓库文档列出的 MinGW、CMake、Ninja 和 depends 构建依赖）：

```sh
make -C depends HOST=x86_64-w64-mingw32 NO_ZMQ=1 NO_USDT=1 NO_IPC=1 -j2
cmake -S . -B build-win -G Ninja \
  --toolchain "$PWD/depends/x86_64-w64-mingw32/toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS='-Wl,--no-insert-timestamp' \
  -DBUILD_GUI=ON -DBUILD_TESTS=ON -DBUILD_GUI_TESTS=ON \
  -DENABLE_IPC=OFF -DWITH_ZMQ=OFF -DBUILD_WALLET_TOOL=ON -DBUILD_TX=ON -DBUILD_UTIL=ON
cmake --build build-win --parallel 2 --target bitcoin-qt bitcoind bitcoin-cli \
  bitcoin-wallet bitcoin-tx bitcoin-util bitcoin doubaocoin-verify-genesis test_bitcoin-qt
```

内部 CMake 目标保留上游名字；输出程序使用 `doubaocoin` 名称。公开发布时必须使用指定的豆包币源码归档或仓库提交，不能改用上游 Bitcoin 源码。

普通构建成功不等于可复现构建通过；兼容层运行不等于原生 Windows 实机验收。程序包未获得商业代码签名或平台公证，不宣称已获得第三方安全认证。Qt 静态链接版本应附许可证、相应源码和可重新构建/链接的应用源码与说明。

Qt 许可及重新链接要求见 [Qt 官方开源许可说明](https://www.qt.io/development/open-source-lgpl-obligations)。发布时应同时提供应用源码和对应依赖源码包，不限制用户修改及重新构建 Qt。

第三方独立审计未进行，长期稳定性、外部矿工接入和外部 SHA-256 ASIC 攻击风险尚未完成公开链安全验收。低算力不会自动获得高安全性。发布主体目前为个人、面向所有地区；未提供适用司法辖区或律师审查，不能宣称已满足全球合规要求。公开源码、程序和启动时间必须先向参与者提供，再按所有者的最终授权启动和挖矿。
