@echo off
setlocal
rem Manual launch only, after the announced public start. No automatic mining.
"%~dp0bin\doubaocoin-qt.exe" -listen=0 -server=1 -rpcbind=127.0.0.1 -rpcallowip=127.0.0.1 -rpcport=49322 -prune=550 -addnode=64.44.157.133:49321 -addnode=103.91.219.156:49321
