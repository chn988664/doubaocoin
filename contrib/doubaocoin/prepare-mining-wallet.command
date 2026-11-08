#!/bin/zsh
# Run by the wallet owner. This script never starts a node or a miner.
set -euo pipefail
umask 077

DBCN_PACKAGE_DIR=${0:A:h}
DBCN_WALLET_TOOL="$DBCN_PACKAGE_DIR/bin/doubaocoin-wallet"
DBCN_DATA="$HOME/Library/Application Support/DouBaoCoin"
DBCN_WALLET_FILE="$DBCN_DATA/wallets/mining/wallet.dat"

if [[ ! -x "$DBCN_WALLET_TOOL" ]]; then
    print -u2 '请将此文件放在程序包顶层，与 bin 文件夹并列。'
    exit 1
fi

print '本操作只在本机创建或打开 mining 钱包、取得公开收款地址并复制备份。'
print '钱包文件含私钥，尚未加密。请在下一窗口选择已挂载的加密磁盘或加密磁盘映像。'
print '不要选择源码目录、Git 目录或云同步目录；不要把钱包文件发给任何人。'
DBCN_BACKUP_PARENT=$(/usr/bin/osascript -e 'POSIX path of (choose folder with prompt "选择已挂载的加密存储位置，用于豆包币钱包备份")')
DBCN_BACKUP_DIR="${DBCN_BACKUP_PARENT%/}/DouBaoCoin-mining-backup-$(date -u +%Y%m%dT%H%M%SZ)"
mkdir -m 700 "$DBCN_BACKUP_DIR"
mkdir -p "$DBCN_DATA/wallets"
chmod 700 "$DBCN_DATA"
chmod 700 "$DBCN_DATA/wallets"

if [[ ! -e "$DBCN_WALLET_FILE" ]]; then
    "$DBCN_WALLET_TOOL" "-datadir=$DBCN_DATA" -wallet=mining create
else
    "$DBCN_WALLET_TOOL" "-datadir=$DBCN_DATA" -wallet=mining info
fi
DBCN_ADDRESS=$("$DBCN_WALLET_TOOL" "-datadir=$DBCN_DATA" -wallet=mining getnewaddress)
if [[ "$DBCN_ADDRESS" != dbcn1q* || "$DBCN_ADDRESS" == *$'\n'* ]]; then
    print -u2 '未获得预期的主网公开收款地址，已停止。'
    exit 1
fi

# Each wallet-tool process closes the database before exiting. Copying only
# after it exits avoids taking an inconsistent copy of a running SQLite file.
cp "$DBCN_WALLET_FILE" "$DBCN_BACKUP_DIR/wallet.dat"
chmod 600 "$DBCN_BACKUP_DIR/wallet.dat"
cmp -s "$DBCN_WALLET_FILE" "$DBCN_BACKUP_DIR/wallet.dat"
print -r -- "$DBCN_ADDRESS" > "$DBCN_BACKUP_DIR/receiving-address.txt"
print -r -- "$DBCN_ADDRESS" > "$DBCN_DATA/mining-payout-address.txt"
cat > "$DBCN_BACKUP_DIR/恢复说明.txt" <<'EOF'
此 wallet.dat 包含私钥，必须保存在加密存储中，切勿发送给他人。
这不是助记词钱包。保留完整 wallet.dat，恢复时使用同一 DouBaoCoin 网络。
节点停止后，可将副本放在独立数据目录的 wallets/mining/wallet.dat，
或在已获准运行的本地节点上使用 restorewallet 恢复到尚不存在的钱包名。
恢复后核对 receiving-address.txt 中的公开地址归属，等待必要的历史重扫。
不要覆盖现有钱包，不要导入 Bitcoin 的数据目录，不要同时运行两个挖矿钱包副本。
备份副本可能不包含后来新导入的描述符和标签，请定期更新备份。
EOF
chmod 600 "$DBCN_BACKUP_DIR/receiving-address.txt" "$DBCN_BACKUP_DIR/恢复说明.txt" "$DBCN_DATA/mining-payout-address.txt"

print ''
print "备份副本已保存：$DBCN_BACKUP_DIR"
print '请确认该位置可以重新打开，并妥善保存。仅将下面的公开地址和“备份已确认”回复到任务会话。'
print -r -- "$DBCN_ADDRESS"
print '本操作没有启动主网或开始挖矿。'
read -r '?按回车关闭窗口。'
