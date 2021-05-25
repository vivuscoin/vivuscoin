Shared Libraries
================

## vivuscoinconsensus

The purpose of this library is to make the verification functionality that is critical to Vivuscoin's consensus available to other applications, e.g. to language bindings.

### API

The interface is defined in the C header `vivuscoinconsensus.h` located in `src/script/vivuscoinconsensus.h`.

#### Version

`vivuscoinconsensus_version` returns an `unsigned int` with the API version *(currently `1`)*.

#### Script Validation

`vivuscoinconsensus_verify_script` returns an `int` with the status of the verification. It will be `1` if the input script correctly spends the previous output `scriptPubKey`.

##### Parameters
- `const unsigned char *scriptPubKey` - The previous output script that encumbers spending.
- `unsigned int scriptPubKeyLen` - The number of bytes for the `scriptPubKey`.
- `const unsigned char *txTo` - The transaction with the input that is spending the previous output.
- `unsigned int txToLen` - The number of bytes for the `txTo`.
- `unsigned int nIn` - The index of the input in `txTo` that spends the `scriptPubKey`.
- `unsigned int flags` - The script validation flags *(see below)*.
- `vivuscoinconsensus_error* err` - Will have the error/success code for the operation *(see below)*.

##### Script Flags
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_NONE`
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH` - Evaluate P2SH ([BIP16](https://github.com/vivuscoin/bips/blob/master/bip-0016.mediawiki)) subscripts
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG` - Enforce strict DER ([BIP66](https://github.com/vivuscoin/bips/blob/master/bip-0066.mediawiki)) compliance
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY` - Enforce NULLDUMMY ([BIP147](https://github.com/vivuscoin/bips/blob/master/bip-0147.mediawiki))
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY` - Enable CHECKLOCKTIMEVERIFY ([BIP65](https://github.com/vivuscoin/bips/blob/master/bip-0065.mediawiki))
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY` - Enable CHECKSEQUENCEVERIFY ([BIP112](https://github.com/vivuscoin/bips/blob/master/bip-0112.mediawiki))
- `vivuscoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS` - Enable WITNESS ([BIP141](https://github.com/vivuscoin/bips/blob/master/bip-0141.mediawiki))

##### Errors
- `vivuscoinconsensus_ERR_OK` - No errors with input parameters *(see the return value of `vivuscoinconsensus_verify_script` for the verification status)*
- `vivuscoinconsensus_ERR_TX_INDEX` - An invalid index for `txTo`
- `vivuscoinconsensus_ERR_TX_SIZE_MISMATCH` - `txToLen` did not match with the size of `txTo`
- `vivuscoinconsensus_ERR_DESERIALIZE` - An error deserializing `txTo`
- `vivuscoinconsensus_ERR_AMOUNT_REQUIRED` - Input amount is required if WITNESS is used

### Example Implementations
- [NVivuscoin](https://github.com/NicolasDorier/NVivuscoin/blob/master/NVivuscoin/Script.cs#L814) (.NET Bindings)
- [node-libvivuscoinconsensus](https://github.com/bitpay/node-libvivuscoinconsensus) (Node.js Bindings)
- [java-libvivuscoinconsensus](https://github.com/dexX7/java-libvivuscoinconsensus) (Java Bindings)
- [vivuscoinconsensus-php](https://github.com/Bit-Wasp/vivuscoinconsensus-php) (PHP Bindings)
