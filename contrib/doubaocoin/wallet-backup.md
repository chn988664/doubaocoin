# DouBaoCoin 描述符钱包备份与恢复

本文件中的命令均使用隔离的 `regtest`。只有在正式主网启动获单独授权、节点和钱包准备就绪后，才能将命令中的 `-regtest` 换成 `-chain=main` 并使用独立的正式数据目录。不要把钱包文件、私有描述符、口令、RPC cookie 或助记词发送给任何人或提交到 Git。

Bitcoin Core 描述符钱包使用独立生成的随机密钥和描述符；本程序不提供将助记词作为钱包恢复输入的功能。以下示例通过本机 RPC cookie 连接，节点 RPC 应保持仅绑定回环地址。

经项目所有者批准，正式主网新建钱包的默认收款类型为 Bech32（`dbcn1`），路径为 `m/84'/1145193294'/0'/0/序号`；找零使用 `m/84'/1145193294'/0'/1/序号`。钱包独立生成随机种子，不复用 Bitcoin 钱包。编号 `1145193294` 尚未在 SLIP-0044 登记；如果以后正式分配不同编号，原钱包的描述符及备份仍须保留，不能靠更改编号恢复原地址。其他地址类型对应 44、49 或 86 号用途，路径均保存在各自的描述符中。`regtest` 使用独立测试链参数，不代表上述正式主网路径。

## 准备受保护的备份位置

在加密存储卷上新建专用目录，确保运行节点的用户能够写入。示例变量请替换为本机实际路径和钱包名；路径本身不应包含秘密。

```sh
DBCN_CLI=/path/to/doubaocoin-cli
DBCN_DATADIR=/path/to/isolated-test-data
DBCN_BACKUP_DIR=/path/on/encrypted-volume/doubaocoin-backup
DBCN_WALLET=source
mkdir -m 700 "$DBCN_BACKUP_DIR"
umask 077
```

备份文件可能包含能够支配全部资金的私钥。钱包文件本身是否加密取决于钱包状态；目录权限并不替代加密存储。不要在终端回显私有描述符，也不要通过聊天发送备份。

## 备份及恢复钱包文件

节点运行且已加载源钱包时，使用 `backupwallet` 获得一致的钱包文件副本：

```sh
"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" "-rpcwallet=$DBCN_WALLET" \
  backupwallet "$DBCN_BACKUP_DIR/wallet-backup.dat"
```

在同一链的节点上以一个尚未使用的钱包名恢复：

```sh
"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" \
  restorewallet recovered "$DBCN_BACKUP_DIR/wallet-backup.dat"
```

恢复后等待钱包同步和必要的重扫，再核对地址与余额；钱包文件备份还包含描述符导入不能完整恢复的标签等钱包元数据。测试备份应使用隔离环境和无价值资金。

## 私有描述符导出与导入

私有描述符备份是敏感明文，即使源钱包已加密也是如此。请只写入上面的加密目录；对于已加密且锁定的钱包，先通过安全终端解锁，再在导出后重新锁定。只读钱包不能导出不存在的私钥。

```sh
# 如钱包已加密且锁定，在终端安全输入口令，不要把口令写入命令行：
"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" "-rpcwallet=$DBCN_WALLET" \
  -stdinwalletpassphrase walletpassphrase 120

# 防止覆盖旧备份；输出重定向到受保护文件，不显示私有描述符。
set -o noclobber
"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" "-rpcwallet=$DBCN_WALLET" \
  listdescriptors true > "$DBCN_BACKUP_DIR/private-descriptors.json"
set +o noclobber
```

若钱包已加密，导出完成后运行 `"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" "-rpcwallet=$DBCN_WALLET" walletlock`。导出失败时不要使用空文件作为备份。

在隔离节点创建一个新空白钱包，然后以标准输入传递描述符，避免把私钥写入进程参数或终端：

```sh
"$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" createwallet descriptor_recovered false true
set -o pipefail
set -o noclobber
python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); keys=("desc","timestamp","active","internal","range","next_index"); print(json.dumps([{k:v for k,v in x.items() if k in keys} for x in d["descriptors"]]))' \
  "$DBCN_BACKUP_DIR/private-descriptors.json" | \
  "$DBCN_CLI" -regtest "-datadir=$DBCN_DATADIR" -rpcwallet=descriptor_recovered \
  -stdin importdescriptors > "$DBCN_BACKUP_DIR/import-result.json" 2> "$DBCN_BACKUP_DIR/import-errors.log"
set +o noclobber
python3 -c 'import json,sys; r=json.load(open(sys.argv[1])); print("导入成功",sum(x.get("success",False) for x in r),"/",len(r)); sys.exit(not all(x.get("success",False) for x in r))' \
  "$DBCN_BACKUP_DIR/import-result.json"
```

保留备份中的 `timestamp`、`range` 和 `next_index`；不要用 `"now"` 代替旧钱包的时间而跳过历史扫描。若节点已修剪所需历史区块，必须使用保留完整历史的节点或恢复相应区块后重扫。旧备份可能不包含备份后新增的描述符、地址范围或标签；在安全环境中定期更新备份并实际演练恢复。

## 导入单个私钥

描述符钱包使用 `importdescriptors` 导入私钥。下面的隔离测试示例适用于**压缩公钥对应的原生 SegWit 地址**；其他地址类型需要匹配原地址的描述符类型。私钥通过隐藏输入读取，仅通过本机标准输入传给 RPC，不写入命令参数或文件。请先备份目标钱包；导入可能触发从创世开始的重扫，需要完整区块历史。

```sh
python3 - "$DBCN_CLI" "$DBCN_DATADIR" "$DBCN_WALLET" <<'PY'
import getpass, json, subprocess, sys
cli, datadir, wallet = sys.argv[1:]
base = [cli, '-regtest', '-datadir=' + datadir, '-rpcwallet=' + wallet, '-stdin']
key = getpass.getpass('WIF 私钥（输入时不显示）：')
descriptor = 'wpkh(' + key + ')'
info = subprocess.run(base + ['getdescriptorinfo'], input=descriptor + '\n',
                      text=True, capture_output=True, check=False)
if info.returncode:
    sys.exit('描述符无效或节点不可用；未导入。')
parsed = json.loads(info.stdout)
if not parsed.get('hasprivatekeys'):
    sys.exit('描述符不含私钥；未导入。')
request = [{'desc': descriptor + '#' + parsed['checksum'], 'timestamp': 0}]
result = subprocess.run(base + ['importdescriptors'], input=json.dumps(request) + '\n',
                        text=True, capture_output=True, check=False)
if result.returncode:
    sys.exit('导入或重扫失败；请检查节点和钱包状态。')
response = json.loads(result.stdout)
if len(response) != 1 or not response[0].get('success'):
    sys.exit('钱包未确认导入；请检查节点和钱包状态。')
print('私钥描述符导入成功；请在重扫完成后核对地址及余额。')
PY
```

私钥在 Python 和节点进程内存中会短暂存在；导入后也会保存在钱包中。不要把它粘贴到聊天、日志或不可信的终端会话。正式主网获准运行后，须使用该网络自己的 WIF 和独立数据目录，并将示例中的 `-regtest` 改为 `-chain=main`。
