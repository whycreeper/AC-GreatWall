### 512-bit Configuration

The 512-bit configuration currently supports **Pylon-512 only**.

This code path is provided mainly as a reference implementation and for experimental verification. It is not fully optimized and has not been evaluated as extensively as the 128-, 192-, and 256-bit settings. At the current stage, the 512-bit configuration temporarily uses **SHAKE256** as the hash function.  

For practical evaluation, the 128-, 192-, and 256-bit configurations are recommended.

```
make -j32 all-sec512_gwccs_64_0_pprf_avx2
```