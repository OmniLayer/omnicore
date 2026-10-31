Debug log categories
====================

Omni Core writes its own log, separate from Bitcoin Core's `debug.log`. Categories are selected with `-omnidebug` and implemented by the `msc_debug_*` flags in `src/omnicore/log.cpp`.

```bash
omnicored -omnidebug=pending
omnicored -omnidebug=parser -omnidebug=metadex1 -omnidebug=ui
omnicored -omnidebug=all
omnicored -omnidebug=none
omnicored -omnidebug=none -omnidebug=parser -omnidebug=sto
```

The same keys can be repeated in `bitcoin.conf`:

```
omnidebug=none
omnidebug=parser
omnidebug=sto
```

## How selection works

`InitDebugLogLevels()` walks each `-omnidebug` value in order:

- If `-omnidebug` is not set, the compiled defaults below are left as they are.
- A category name enables that category. Every other category keeps its current value, including the compiled defaults that are already enabled.
- `"all"` enables every category.
- `"none"` disables every category.
- A name that is not a category and is not `"all"` or `"none"` is ignored.
- A later value overrides an earlier one. `-omnidebug=parser -omnidebug=none` ends with every category disabled. `-omnidebug=none -omnidebug=parser` enables only `parser`.

The flag in code is `msc_debug_<category>`. For example, `-omnidebug=pending` sets `msc_debug_pending`.

Lines are appended to `omnicore.log` in the data directory, or to the path given by `-omnilogfile`. If `-printtoconsole` is set, Omni lines go to standard output and are not also written to the Omni log file. Each line is prefixed with a timestamp.

## Categories

"Enabled" below means the compiled default is on. That is what you get when `-omnidebug` is omitted. Enabling one extra category does not turn these off; pass `"none"` first to start from a clean set.

Categories marked unused are accepted by `-omnidebug` but no current log statement reads the flag, so changing them has no effect.

| Category | Default | What it logs |
|----------|---------|--------------|
| `parser_data` | disabled | Output inventory and reference-address selection while parsing, including Class A collisions, Class B multisig data, and the detected Class C payload |
| `parser_readonly` | disabled | During RPC / read-only parsing, the per-transaction header and the Class A "may be a DEx payment" hint. Block parsing prints that header whether or not this category is enabled |
| `parser_dex` | enabled | Each non-Exodus output considered as a possible DEx bitcoin payment. Together with non-RPC parsing, or with `parser_readonly`, also the Class A hint that a transaction may be a DEx payment |
| `parser` | disabled | Deobfuscated Class B packet bytes while parsing |
| `verbose` | disabled | The identified sender and fee, and the single assembled payload |
| `verbose2` | disabled | Unused |
| `verbose3` | disabled | Unused |
| `vin` | disabled | Input `scriptSig`s examined while identifying the sender |
| `script` | disabled | `scriptPubKey` bytes and multisig destinations while collecting Class B packets |
| `dex` | enabled | DEx offer, accept, and payment-book updates |
| `send` | enabled | Unused |
| `tokens` | disabled | Coin selection: outputs skipped because they are below the economic threshold, and outpoints selected from the sender |
| `spec` | disabled | Class B payloads whose sequence numbers are not sequential |
| `exo` | disabled | Dev OMNI vesting, the Exodus balance after a block, block hashes during the initial scan, and sender identification when an input pays the Exodus address. Also allows `tally` lines for the Exodus address, which `tally` otherwise suppresses |
| `tally` | enabled | Balance before and after each tally update, except updates of the Exodus address unless `exo` is also enabled |
| `sp` | enabled | Crowdsale token credits, and extra detail when a maxed or expired crowdsale is removed |
| `sto` | enabled | Per-holder amounts while calculating a send-to-owners distribution |
| `txdb` | disabled | Transaction-list database writes, the stored database version, and list statistics |
| `tradedb` | enabled | Success or failure when recording a new MetaDEx trade or a trade match |
| `persistence` | disabled | LevelDB open, wipe, and clear; loading and pruning state files; close messages for the Omni databases |
| `ui` | disabled | Unused |
| `pending` | enabled | Pending-transaction inserts, and the amount found when a pending transaction is checked |
| `metadex1` | disabled | MetaDEx matching and insertion: prices compared, amounts traded, and orders inserted or erased |
| `metadex2` | disabled | The price comparison inside a match, and the order book before and after cancel-at-price and cancel-everything |
| `metadex3` | disabled | The order book before and after a trade, and each order visited while cancelling at a price or for a pair. Described in code as printing the order book before and after each trade |
| `packets` | enabled | Decoded payload fields while interpreting a transaction. Not used for RPC / read-only interpretation; use `packets_readonly` for that |
| `packets_readonly` | disabled | The same decoded payload fields when interpreting a transaction in RPC / read-only mode |
| `consensus_hash` | disabled | Each string added while building the consensus hash, and the resulting hash |
| `consensus_hash_every_block` | disabled | Computes and logs the consensus hash for every parsed block. This is much more work than the normal checkpoint path |
| `alerts` | enabled | Unused. Alert payload fields are logged by `packets` / `packets_readonly` |
| `consensus_hash_every_transaction` | disabled | The consensus hash after each successfully interpreted Omni transaction |
| `fees` | enabled | Fee-cache updates, fee distribution, fee-history writes, and MetaDEx matches that skip the fee because one property is OMNI |
| `nftdb` | disabled | Non-fungible token database moves, data writes, lookups, and the per-block sanity check |

## Notes

- `"all"` is extremely verbose. `consensus_hash_every_block` and `consensus_hash_every_transaction` recompute the consensus hash continuously.
- `send`, `verbose2`, `verbose3`, `ui`, and `alerts` are still valid `-omnidebug` names because `InitDebugLogLevels()` lists them. Nothing in the current tree consults those flags.
- An older category list in the 0.0.10 release notes includes `walletcache` and does not match the flags above. This document follows `src/omnicore/log.cpp`.
