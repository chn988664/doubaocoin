# Native Bitcoin Core peer discovery for DouBaoCoin

Mainnet uses the existing Bitcoin Core DNS seeding, BIP155 fixed seeds,
address relay, addrman persistence, network-group selection and automatic
outbound connections. No P2P, validation, consensus or wallet algorithm is replaced.

DNS seeds: `seed1.dbcn.wudoubao.us` and `seed2.dbcn.wudoubao.us`.
Fixed-seed input: `contrib/doubaocoin/seeds/nodes_main.txt`; the serialized
array in `src/doubaocoinseeds.h` is produced by the unmodified Bitcoin Core
`contrib/seeds/generate-seeds.py` `process_nodes` function.

A pristine installation can start the node or Windows GUI directly. No old
`peers.dat`, manual `addnode`, copied example configuration or launcher-specific
bootstrap arguments are required. DNS failures fall back to fixed seeds through
upstream logic. Seed addresses are initial candidates, not exclusive peers.

An existing `connect=` overrides automatic peer selection. Existing explicit
`dnsseed=0`, `fixedseeds=0`, `discover=0` or `listen=0` settings are respected;
remove them deliberately if migrating an early private topology to public P2P.
The release must not overwrite a user's configuration or enable RPC externally.
Pruning is optional. Public archival nodes should retain full block history.

## Seed DNS service

Use **unmodified sipa/bitcoin-seeder**, fixed revision
`92d2b464702ac952cde59a29301dfcd9c1f81eb5`:
https://github.com/sipa/bitcoin-seeder/tree/92d2b464702ac952cde59a29301dfcd9c1f81eb5

Network identity is supplied by its existing CLI parameters:

```sh
dnsseed -h seed1.dbcn.wudoubao.us -n ns1.dbcn.wudoubao.us -m hostmaster.wudoubao.us \
  -a LOCAL_BIND_IP -p 53 -t 8 -d 1 -b 49321 \
  --magic fbc7b2d4 --minheight 1 \
  --knownblock 0000000007340821f32d4424dda9cb6b4f165b09fc10a83bcc1d6431fa1f95e0 \
  -s 64.44.157.133 -s 103.91.219.156
```

The second service uses `seed2` / `ns2`. Run as a dedicated non-root user, with
only bind-service capability and an isolated writable crawler data directory.
Use an address assigned to the server for `LOCAL_BIND_IP`; behind cloud NAT,
use `0.0.0.0` and point the nameserver's A record to the external address.
The authoritative seed answers and service-bit filtering come from upstream;
there is no application-specific DNS response code or static fake crawler database.
Root DNS zone delegation uses DNS-only A records for `ns1.dbcn`/`ns2.dbcn`, and
NS records for `seed1.dbcn`/`seed2.dbcn`. No Bitcoin seed domains or addresses are used.

## Public node admission

Keep Bitcoin Core's existing inbound/transaction-relay admission logic. Size
`maxconnections` to the server budget: at 24, reserving 11 outbound slots and
50% transaction-relay inbound means only floor((24-11)*0.5)=6 normal inbound peers.
At 64 the same policy permits 26. A listening public node also needs a reachable
P2P port and address advertisement; RPC remains loopback-only. Do not force all
nodes to a two-center `connect=` topology, and do not disable address discovery.

## Release acceptance

Use new, isolated mainnet data directories and the actual packaged binaries.
Check automatic cold startup, DNS-only discovery, fixed-seed fallback with DNS
disabled, persisted peers across restart, valid address gossip, and interruption
of an individual bootstrap endpoint. Native Windows GUI and CLI/daemon entrypoints
must share the same bootstrap data. Never replace this with only regtest checks,
old configured-node checks, or port-connect probes. The genesis hash, proof of work,
reward/maturity rules and existing wallet data must remain unchanged.
