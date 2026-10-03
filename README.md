# Vivuscoin Core

Vivuscoin (VVC) is an open-source SHA-256d proof-of-work cryptocurrency: 240-second blocks and a 50 VVC block
reward that halves every 210,000 blocks. Vivuscoin Core is the wallet and full-node software, derived from
Bitcoin Core and released under the MIT licence.

Website: https://vivuscoin.com · Explorer: https://vivuscoin.com/explorer/ · News: https://vivuscoin.com/news/

## Downloads

Current release: **Vivuscoin Core 1.1.0** — https://github.com/vivuscoin/vivuscoin/releases/tag/v1.1.0

| Platform | File |
|---|---|
| Windows 64-bit | `vivuscoin-1.1.0-win64-setup.exe` (installer) or `vivuscoin-1.1.0-win64.zip` |
| macOS (Apple silicon) | `Vivuscoin-Core-1.1.0-macos-arm64.dmg` (GUI, macOS 26) or `vivuscoin-1.1.0-macos-arm64-headless.tar.gz` (daemon + CLI, macOS 14+) |
| Linux x86-64 | `vivuscoin-1.1.0-x86_64-linux-gnu.tar.gz` (daemon, CLI and `vivuscoin-qt`; glibc 2.29+) |

Compare each file with `SHA256SUMS` on the release page before running it. The macOS and Windows builds are
unsigned: right-click › Open on macOS, confirm the SmartScreen prompt on Windows.

**Upgrade to 1.1 before block 24,147.** Install over the existing wallet; the data directory and wallet file
are reused and no reindex is needed.

## Quick start

1. Install and start Vivuscoin Core. It syncs from the seed nodes; if it shows no connections, add
   `addnode=seed1.vivuscoin.com` and `addnode=seed2.vivuscoin.com` to `vivuscoin.conf` and restart.
2. The wallet is synced when its block height matches https://vivuscoin.com/explorer/.
3. Receive tab › create an address (it starts with `A`). Legacy `7…`/`8…` and bech32 `vvc1…` addresses also work.
4. Get VVC by mining (below) or from the airdrop: https://vivuscoin.com/airdrop/ (50 VVC per address).

Headless: `vivuscoind -daemon`, then `vivuscoin-cli getblockchaininfo`. Guides: https://vivuscoin.com/get-started/
and https://vivuscoin.com/developers/.

## Mining

Solo stratum endpoint for any SHA-256d miner: `stratum+tcp://pool.vivuscoin.com:3333`, username = your VVC
address, password `x`. A block you find pays its full 50 VVC reward to that address; the endpoint holds nothing.

```
cpuminer -a sha256d -o stratum+tcp://pool.vivuscoin.com:3333 -u <YOUR_VVC_ADDRESS> -p x
```

Live pool hashrate and workers: https://vivuscoin.com/pool/ (JSON: https://vivuscoin.com/pool/stats.json).
Guide: https://vivuscoin.com/mining/. You can also mine against your own node with `getblocktemplate`.

## Network

| Parameter | Value |
|---|---|
| Proof of work | SHA-256d |
| Block target / reward | 240 s / 50 VVC, halving every 210,000 blocks |
| Difficulty | adjusts every block (LWMA-1) |
| P2P port | 8168 |
| Seed nodes | `seed1.vivuscoin.com`, `seed2.vivuscoin.com` |
| Genesis | `000000009ba1cc4ecf00c8d24b704f3e88f8c03a70fba8f3e6c350007b5a2cd5` |
| User agent | `/Parvus:1.1.0/` |

Explorer API: `https://vivuscoin.com/explorer/api/v1/` (for example `totalcoins`).

## Build from source

Release tag: `v1.1.0`. Instructions: [doc/build-unix.md](doc/build-unix.md), [doc/build-osx.md](doc/build-osx.md),
[doc/build-windows.md](doc/build-windows.md). Release binaries for Linux and Windows are built with the
[`depends`](depends/README.md) system.

```
./autogen.sh
./configure
make
make check                      # unit tests
test/functional/test_runner.py  # functional tests
```

## Contributing

Bug reports and questions: https://github.com/vivuscoin/vivuscoin/issues · support@vivuscoin.com.
See [CONTRIBUTING.md](CONTRIBUTING.md) and [doc/developer-notes.md](doc/developer-notes.md). Consensus changes
get the most scrutiny and the slowest merges, on purpose.

Community: [BitcoinTalk thread](https://bitcointalk.org/index.php?topic=5595753.0)

## License

Vivuscoin Core is released under the terms of the MIT licence. See [COPYING](COPYING).
