@echo off
setlocal
rem Use native Bitcoin peer discovery. No automatic mining.
"%~dp0bin\doubaocoin-qt.exe" -server=1 -rpcbind=127.0.0.1 -rpcallowip=127.0.0.1 -rpcport=49322 -prune=550
