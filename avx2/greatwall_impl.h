// #ifndef GREATWALL_IMPL_H
// #define GREATWALL_IMPL_H

// #include <assert.h>
// #include <string.h>
// #include <inttypes.h>
// #include <immintrin.h>
// #include <wmmintrin.h>

// #include "transpose.h"
// #include "util.h"
// #include <stdio.h>

// inline void print_block_hex(const char* tag, const void* data, size_t len)
// {
//     // const unsigned char* p = (const unsigned char*)data;
//     // printf("%s = ", tag);
//     // for (size_t i = 0; i < len; ++i)
//     //     printf("%02x", p[i]);
//     // printf("\n");
// }

// inline void greatwall_round_function(uint64_t* block, const uint64_t* key, const uint64_t* rc, const uint64_t* mat, uint64_t* after_sbox) {
// #if SECURITY_PARAM == 128
//     block137 tmp;
//     memcpy(&tmp, block, 16);
//     print_block_hex("before SBOX tmp", &tmp, 16);
//     block137_mersenne_inverse1(&tmp);
//     print_block_hex("after SBOX tmp", &tmp, 16);
//     memcpy(after_sbox, &tmp, 16);
//     block137_multiply_with_GF2_matrix(&tmp, mat);
//     print_block_hex("after mat tmp", &tmp, 16);
//     tmp = block137_xor(tmp, *(block137*)key);
//     print_block_hex("after add key tmp", &tmp, 16);
//     tmp = block137_xor(tmp, *(block137*)rc);
//     print_block_hex("after add rc tmp", &tmp, 16);
//     memcpy(block, &tmp, 16);
// #elif SECURITY_PARAM == 192
//     block192 tmp;
//     memcpy(&tmp, block, 24);
//     block192_mersenne_inverse(&tmp);
//     memcpy(after_sbox, &tmp, 24);
//     block192_multiply_with_GF2_matrix(&tmp, mat);
//     tmp = block192_xor(tmp, *(block192*)key);
//     tmp = block192_xor(tmp, *(block192*)rc);
//     memcpy(block, &tmp, 24);
// #elif SECURITY_PARAM == 256
//     block256 tmp;
//     memcpy(&tmp, block, 32);
//     block256_mersenne_inverse(&tmp);
//     memcpy(after_sbox, &tmp, 32);
//     block256_multiply_with_GF2_matrix(&tmp, mat);
//     tmp = block256_xor(tmp, *(block256*)key);
//     tmp = block256_xor(tmp, *(block256*)rc);
//     memcpy(block, &tmp, 32);
// #endif
// }

// inline void greatwall_last_round_function(uint64_t* block, const uint64_t* key, const uint64_t* rc) {
// #if SECURITY_PARAM == 128
//     block137 tmp;
//     memcpy(&tmp, block, 16);
//     block137_mersenne_inverse(&tmp);
//     tmp = block137_xor(tmp, *(block137*)key);
//     tmp = block137_xor(tmp, *(block137*)rc);
//     memcpy(block, &tmp, 16);
// #elif SECURITY_PARAM == 192
//     block192 tmp;
//     memcpy(&tmp, block, 24);
//     block192_mersenne_inverse(&tmp);
//     tmp = block192_xor(tmp, *(block192*)key);
//     tmp = block192_xor(tmp, *(block192*)rc);
//     memcpy(block, &tmp, 24);
// #elif SECURITY_PARAM == 256
//     block256 tmp;
//     memcpy(&tmp, block, 32);
//     block256_mersenne_inverse(&tmp);
//     tmp = block256_xor(tmp, *(block256*)key);
//     tmp = block256_xor(tmp, *(block256*)rc);
//     memcpy(block, &tmp, 32);
// #endif
// }

// #endif