## GreatWall

The codebase is designed for implementing and evaluating Pylon (OWF used in GreatWall) within VOLEitH-based signature constructions.

This implementation supports **only Pylon** and the security parameters are 128, 192, and 256 bits.

This project is a research-oriented modification of the FAEST one-tree framework: 

https://github.com/faest-sign/faest-one-tree

## Main Modifications

The major changes include:

- Added Pylon OWF integration
- Modified the underlying finite field configuration to match Pylon-specific parameter settings
- Adapted finite field arithmetic and internal representations
- Updated witness generation and constraint handling

## Compilation

Building requires GNU make.
Requires gcc 13.1.0 or higher
Do not forget to pull the submodules

To build just a single setting, run `make -j<number_of_threads> <setting_name>`.
To build tests as well, run `make -j<number_of_threads> all-<setting_name>`.
The output will be in `Additional_Implementations/<setting_name>`.

## Settings

Each setting is given a name: `sec<security_parameter>_<primitives>_<𝜏>_<w>_<seeds>_<platform_setting>`.

Recommended configurations:
- make -j32 all-sec128_gwccs_11_7_100_avx2
- make -j32 all-sec192_gwccs_16_8_183_avx2
- make -j32 all-sec256_gwccs_22_6_246_avx2
- make -j32 all-sec128_gwccs_16_8_108_avx2
- make -j32 all-sec192_gwccs_24_8_184_avx2
- make -j32 all-sec256_gwccs_32_7_248_avx2

After compilation, benchmarking can be performed using:

```bash
./bench.sh
```

### 512-bit Configuration

The 512-bit configuration currently supports **Pylon-512 only** and is on branch-512.

The 512-bit configuration is mainly a reference implementation and for experimental verification. It is not fully optimized and has not been evaluated as extensively as the 128-, 192-, and 256-bit settings. At the current stage, the 512-bit configuration temporarily uses **SHAKE256** as the hash function.  

For practical evaluation, the 128-, 192-, and 256-bit configurations are recommended.