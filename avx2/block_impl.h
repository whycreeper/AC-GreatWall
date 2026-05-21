#ifndef BLOCK_IMPL_AVX2_H
#define BLOCK_IMPL_AVX2_H

#include <immintrin.h>
#include <wmmintrin.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

typedef __m128i block128;
typedef __m256i block256;

typedef struct
{
	block128 data[3];
} block384;
typedef struct
{
	block256 data[2];
} block512;

// Unfortunately, there's no alternative version of these that works on integers.
#define shuffle_2x4xepi32(x, y, i) \
	_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), i))
#define permute_8xepi32(x, i) \
	_mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(x), i))
#define shuffle_2x4xepi64(x, y, i) \
	_mm256_castpd_si256(_mm256_shuffle_pd(_mm256_castsi256_pd(x), _mm256_castsi256_pd(y), i))


// ##################################################################################################################
// ################################### TAKEN FROM THE RAINIER IMPLEMENTATION ########################################
// ##################################################################################################################


inline __m256i mm256_compute_mask_2(const uint64_t idx, const size_t bit) {
  const uint64_t m1 = -((idx >> bit) & 1);
  const uint64_t m2 = -((idx >> (bit + 1)) & 1);
  return _mm256_set_epi64x(m2, m2, m1, m1);
}
inline __m256i mm256_compute_mask(const uint64_t idx, const size_t bit) {
  return _mm256_set1_epi64x(-((idx >> bit) & 1));
}

// ################### 128 ####################
inline __m128i* block128_as_m128i(block128* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook128(__m128i out[2], const __m128i a, const __m128i b) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a, b, 0x00);
  out[1] = _mm_clmulepi64_si128(a, b, 0x11);
  tmp[0] = _mm_clmulepi64_si128(a, b, 0x01);
  tmp[1] = _mm_clmulepi64_si128(a, b, 0x10);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);
  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);
}
inline void reduce_clmul128(__m128i out[1], const __m128i in[2]) {
  __m128i p = _mm_set_epi64x(0x0, 0x87);
  __m128i t0, t1, t2;
  t0 = _mm_clmulepi64_si128(in[1], p, 0x01); // in[1]_high * p
  t1 = _mm_slli_si128(t0, 8);    // low 64bit of result, shifted to high
  t2 = _mm_srli_si128(t0, 8);    // high 64bit of result, shifted to high
  t2 = _mm_xor_si128(t2, in[1]); // update in[1]_low with high64 of result
  t0 = _mm_clmulepi64_si128(t2, p, 0x00); // updated in[1]_low * p
  out[0] = _mm_xor_si128(t0, in[0]);      // add in[1]_low * p to result
  out[0] = _mm_xor_si128(out[0], t1); // also add the low part of in[1]_high * p
}
inline void sqr128(__m128i out[2], const __m128i a) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a, mask);
  tmp[1] = _mm_srli_epi64(a, 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}
inline void gf128sqr(__m128i *out, const __m128i in) {
  __m128i tmp[2];
  sqr128(tmp, in);
  reduce_clmul128(out, tmp);
}
inline void gf128mul(__m128i *out, const __m128i in1, const __m128i in2) {
  __m128i tmp[2];
  clmul_schoolbook128(tmp, in1, in2);
  reduce_clmul128(out, tmp);
}
inline void block128_inverse(block128* block) {
  const size_t u[11] = {1, 2, 3, 6, 12, 24, 48, 51, 63, 126, 127};
  // q = u[i] - u[i - 1] should give us the corresponding values
  // (1, 1, 3, 6, 12, 24, 3, 12, 63, 1), which will have corresponding indexes
  const size_t q_index[10] = {0, 0, 2, 3, 4, 5, 2, 4, 8, 0};
  __m128i b[11];
  // b[0] = *block128_as_m128i(block);
  memcpy(b, block, 16);
  for (size_t i = 1; i < 11; ++i) {
    __m128i b_p = b[i - 1];
    __m128i b_q = b[q_index[i - 1]];
    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf128sqr(&b_p, b_p);
    }
    gf128mul(&b[i], b_p, b_q);
  }
  gf128sqr(block, b[10]);
}
inline void block128_mersenne_inverse(block128* block) {
  // inverse Mersenne S-box with e = 3
// (2 ^ 3 - 1) ^ (-1) mod (2 ^ 128 - 1)
// = 0x49249249249249249249249249249249
// =   100 100 100 100 ... 100 1
// 42 times '100' and '1'

  __m128i in[4];//in, in_100, in_4_100, in_21_100
  // b[0] = *block128_as_m128i(block);
  memcpy(in, block, 16);

  __m128i tmp, tmp2;
  gf128sqr(&tmp, in[0]);
  gf128sqr(&in[1], tmp);

  gf128sqr(&tmp, in[1]);
  for(size_t i = 1 ; i < 3; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[1]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 6; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&in[2], tmp, tmp2);


  gf128sqr(&tmp, in[2]);
  for(size_t i = 1 ; i < 12; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[2]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 24; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, tmp2);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 12; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[2]);
  gf128sqr(&tmp, tmp2);
  for(size_t i = 1 ; i < 3; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&in[3], tmp, in[1]);

  
  gf128sqr(&tmp, in[3]);
  for(size_t i = 1 ; i < 63; i++){
    gf128sqr(&tmp, tmp);
  }
  gf128mul(&tmp2, tmp, in[3]);

  gf128sqr(&tmp2, tmp2);

  gf128mul(block, tmp2, in[0]);

}
inline void block128_multiply_with_GF2_matrix(block128* block, const uint64_t* matrix) {
  block128 tmp;
  for (size_t j = 0; j < 2; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*2) + (i*2)];
      uint64_t bit = _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1])) & 1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block128_multiply_with_transposed_GF2_matrix(block128* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 2; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 8, idx >>= 8, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask_2(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask_2(idx, 2)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask_2(idx, 4)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask_2(idx, 6)));
    }
  }
  cval[0] = _mm256_xor_si256(cval[0], cval[1]);
  *block = _mm_xor_si128(_mm256_extracti128_si256(cval[0], 0),
                                     _mm256_extracti128_si256(cval[0], 1));
}
// ################### 137 ####################
inline __m128i* block137_as_m128i(block137* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook137(__m128i out[3], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);
  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);
}
static inline void reduce_clmul137(__m128i out[2], const __m128i in[3]) {
  // modulus = x^137 + x^21 + 1
  uint64_t t[6] = {0};

  // 384 bits total
  memcpy(&t[0], &in[0], 16);  // bits   0..127
  memcpy(&t[2], &in[1], 16);  // bits 128..255
  memcpy(&t[4], &in[2], 16);  // bits 256..383

  for (int bit = 383; bit >= 137; --bit) {
    int word = bit >> 6;
    int off  = bit & 63;
    uint64_t mask = 1ULL << off;
    if (t[word] & mask) {
      // clear x^bit
      t[word] ^= mask;
      // add x^(bit - 137)
      {
        int b = bit - 137;
        t[b >> 6] ^= (1ULL << (b & 63));
      }
      // add x^(bit - 116) because x^137 = x^21 + 1
      {
        int b = bit - 116;
        t[b >> 6] ^= (1ULL << (b & 63));
      }
    }
  }
  // keep only 137 bits = 128 + 9
  t[2] &= 0x1FFULL;   // low 9 bits
  t[3] = 0;
  t[4] = 0;
  t[5] = 0;
  memcpy(&out[0], &t[0], 16);
  // memcpy(&out[1], &t[2], 16);
  out[1] = _mm_set_epi64x(0, t[2] & 0x1FFULL);
}

static inline void sqr137(__m128i out[3], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}
static inline void gf137sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[3];
  sqr137(tmp, in);
  reduce_clmul137(out, tmp);
}
static inline void gf137mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[3];
  clmul_schoolbook137(tmp, in1, in2);
  reduce_clmul137(out, tmp);
}
static inline void block137_sqr(block137* out, const block137* in)
{
    gf137sqr((__m128i*)out->data, (const __m128i*)in->data);
    out->data[2] &= 0x1FFULL;
}

static inline void block137_mul(block137* out, const block137* a, const block137* b)
{
    gf137mul((__m128i*)out->data, (const __m128i*)a->data, (const __m128i*)b->data);
    out->data[2] &= 0x1FFULL;
}
static inline block137 block137_set_one(void)
{
    block137 r = {{0, 0, 0}};
    r.data[0] = 1;
    return r;
}

static inline void block137_mask(block137* x)
{
    x->data[2] &= 0x1FFULL;
}

static void block137_pow_bin(block137* block, const char* exp_bits)
{
    block137 base = *block;
    block137 result = block137_set_one();
    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        // result = result^2
        block137 tmp;

        block137_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block137_mul(&result, &result, &base);
        }
    }

    block137_mask(&result);
    *block = result;
}
//todo: reduce numbers of mul
static inline void block137_mersenne_inverse1(block137* block) {
  //e = 70
    static const char EXP_INV_1[] =
        "11011011011011011011011011011011011011011011011011011011011011011011101101101101101101101101101101101101101101101101101101101101101101101";

    block137_pow_bin(block, EXP_INV_1);
}

static inline void block137_mersenne_inverse2(block137* block) {
  //e = 75
    static const char EXP_INV_1[] =
        "11011011011101101101101110110110110111011011011011101101101101110110110111011011011011101101101101110110110110111011011011011101101101101";

    block137_pow_bin(block, EXP_INV_1);
}
static inline void block137_multiply_with_GF2_matrix(block137* block, const uint64_t matrix[137][3]) {
    block137 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2] & 0x1FFULL;

    for (size_t row = 0; row < 137; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2])) & 1ULL;

        tmp.data[row >> 6] ^= bit << (row & 63);
    }

    tmp.data[2] &= 0x1FFULL;
    *block = tmp;
}

// ################### 193 ####################

inline __m128i* block193_as_m128i(block193* block) {
  return (__m128i*)block;
}

static inline void block193_mask(block193* x)
{
    x->data[3] &= 0x1ULL;
}

static inline block193 block193_set_one(void)
{
    block193 r = {{0, 0, 0, 0}};
    r.data[0] = 1;
    return r;
}

static inline void clmul_schoolbook193(__m128i out[4],
                                       const __m128i a[2],
                                       const __m128i b[2])
{
    uint64_t aw[4], bw[4];
    uint64_t t[8] = {0};

    memcpy(aw, a, 32);
    memcpy(bw, b, 32);

    aw[3] &= 0x1ULL;
    bw[3] &= 0x1ULL;

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            __m128i aa = _mm_set_epi64x(0, aw[i]);
            __m128i bb = _mm_set_epi64x(0, bw[j]);
            __m128i p = _mm_clmulepi64_si128(aa, bb, 0x00);

            uint64_t pp[2];
            _mm_storeu_si128((__m128i*)pp, p);

            t[i + j]     ^= pp[0];
            t[i + j + 1] ^= pp[1];
        }
    }

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(t[5], t[4]);
    out[3] = _mm_set_epi64x(t[7], t[6]);
}

/*
 * modulus = x^193 + x^15 + 1
 */
static inline void reduce_clmul193(__m128i out[2], const __m128i in[4])
{
    uint64_t t[8] = {0};

    memcpy(&t[0], &in[0], 16);  // bits   0..127
    memcpy(&t[2], &in[1], 16);  // bits 128..255
    memcpy(&t[4], &in[2], 16);  // bits 256..383
    memcpy(&t[6], &in[3], 16);  // bits 384..511

    /*
     * x^193 = x^15 + 1
     *
     * For bit >= 193:
     * x^bit = x^(bit - 193) * x^193
     *       = x^(bit - 193) * (x^15 + 1)
     *       = x^(bit - 178) + x^(bit - 193)
     */
    for (int bit = 511; bit >= 193; --bit) {
        int word = bit >> 6;
        int off  = bit & 63;
        uint64_t mask = 1ULL << off;

        if (t[word] & mask) {
            // clear x^bit
            t[word] ^= mask;

            // add x^(bit - 193)
            {
                int b = bit - 193;
                t[b >> 6] ^= (1ULL << (b & 63));
            }

            // add x^(bit - 178), because 193 - 15 = 178
            {
                int b = bit - 178;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
        }
    }

    // keep only 193 bits = 64 + 64 + 64 + 1
    t[3] &= 0x1ULL;
    t[4] = 0;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3] & 0x1ULL, t[2]);
}

static inline void sqr193(__m128i out[4], const __m128i a[2])
{
    __m128i tmp[2];

    __m128i sqrT = _mm_set_epi64x(
        0x5554515045444140,
        0x1514111005040100
    );

    __m128i mask = _mm_set_epi64x(
        0x0F0F0F0F0F0F0F0F,
        0x0F0F0F0F0F0F0F0F
    );

    // square a[0], covering data[0], data[1]
    tmp[0] = _mm_and_si128(a[0], mask);
    tmp[1] = _mm_srli_epi64(a[0], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    // square a[1], covering data[2], data[3]
    tmp[0] = _mm_and_si128(a[1], mask);
    tmp[1] = _mm_srli_epi64(a[1], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}

static inline void gf193sqr(__m128i* out, const __m128i* in)
{
    __m128i tmp[4];
    sqr193(tmp, in);
    reduce_clmul193(out, tmp);
}

static inline void gf193mul(__m128i* out, const __m128i* in1, const __m128i* in2)
{
    __m128i tmp[4];
    clmul_schoolbook193(tmp, in1, in2);
    reduce_clmul193(out, tmp);
}

static inline void block193_sqr(block193* out, const block193* in)
{
    gf193sqr((__m128i*)out->data, (const __m128i*)in->data);
    out->data[3] &= 0x1ULL;
}

static inline void block193_mul(block193* out, const block193* a, const block193* b)
{
    gf193mul((__m128i*)out->data,
             (const __m128i*)a->data,
             (const __m128i*)b->data);
    out->data[3] &= 0x1ULL;
}

static void block193_pow_bin(block193* block, const char* exp_bits)
{
    block193 base = *block;
    block193 result = block193_set_one();

    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        block193 tmp;

        block193_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block193_mul(&result, &result, &base);
        }
    }

    block193_mask(&result);
    *block = result;
}

static inline void block193_mersenne_inverse1(block193* block)
{
    static const char EXP_INV_1[] =
        "0100100101001001001010010010100100100101001001010010010010100100101001001001010010010100100100101001001001010010010100100100101001001010010010010100100101001001001010010010100100100101001001001";

    block193_pow_bin(block, EXP_INV_1);
}

static inline void block193_mersenne_inverse2(block193* block)
{
    static const char EXP_INV_2[] =
        "0101010101010101010100101010101010101010100101010101010101010100101010101010101010100101010101010101010101001010101010101010101001010101010101010101001010101010101010101001010101010101010101001";

    block193_pow_bin(block, EXP_INV_2);
}

static inline void block193_multiply_with_GF2_matrix(
    block193* block,
    const uint64_t matrix[193][4]
) {
    block193 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;
    tmp.data[3] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2];
    const uint64_t x3 = block->data[3] & 0x1ULL;

    for (size_t row = 0; row < 193; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2]) ^
             _mm_popcnt_u64(x3 & (A[3] & 0x1ULL))) & 1ULL;

        tmp.data[row >> 6] |= bit << (row & 63);
    }

    tmp.data[3] &= 0x1ULL;
    *block = tmp;
}
// ################### 257 ####################

inline __m128i* block257_as_m128i(block257* block) {
  return (__m128i*)block;
}

static inline void block257_mask(block257* x)
{
    x->data[4] &= 0x1ULL;
}

static inline block257 block257_set_one(void)
{
    block257 r = {{0, 0, 0, 0, 0}};
    r.data[0] = 1;
    return r;
}


static inline void clmul_schoolbook257(__m128i out[5],
                                       const __m128i a[3],
                                       const __m128i b[3])
{
    uint64_t aw[6], bw[6];
    uint64_t t[10] = {0};

    memcpy(aw, a, 48);
    memcpy(bw, b, 48);

    aw[4] &= 0x1ULL;
    bw[4] &= 0x1ULL;
    aw[5] = 0;
    bw[5] = 0;

    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 5; j++) {
            __m128i aa = _mm_set_epi64x(0, aw[i]);
            __m128i bb = _mm_set_epi64x(0, bw[j]);
            __m128i p = _mm_clmulepi64_si128(aa, bb, 0x00);

            uint64_t pp[2];
            _mm_storeu_si128((__m128i*)pp, p);

            t[i + j]     ^= pp[0];
            t[i + j + 1] ^= pp[1];
        }
    }

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(t[5], t[4]);
    out[3] = _mm_set_epi64x(t[7], t[6]);
    out[4] = _mm_set_epi64x(t[9], t[8]);
}

/*
 * modulus = x^257 + x^12 + 1
 */
static inline void reduce_clmul257(__m128i out[3], const __m128i in[5])
{
    uint64_t t[10] = {0};

    memcpy(&t[0], &in[0], 16);  // bits   0..127
    memcpy(&t[2], &in[1], 16);  // bits 128..255
    memcpy(&t[4], &in[2], 16);  // bits 256..383
    memcpy(&t[6], &in[3], 16);  // bits 384..511
    memcpy(&t[8], &in[4], 16);  // bits 512..639

    /*
     * x^257 = x^12 + 1
     *
     * For bit >= 257:
     * x^bit = x^(bit - 257) * x^257
     *       = x^(bit - 257) * (x^12 + 1)
     *       = x^(bit - 245) + x^(bit - 257)
     */
    for (int bit = 639; bit >= 257; --bit) {
        int word = bit >> 6;
        int off  = bit & 63;
        uint64_t mask = 1ULL << off;

        if (t[word] & mask) {
            t[word] ^= mask;

            // add x^(bit - 257)
            {
                int b = bit - 257;
                t[b >> 6] ^= (1ULL << (b & 63));
            }

            // add x^(bit - 245), because 257 - 12 = 245
            {
                int b = bit - 245;
                t[b >> 6] ^= (1ULL << (b & 63));
            }
        }
    }

    // keep only 257 bits = 64 + 64 + 64 + 64 + 1
    t[4] &= 0x1ULL;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;
    t[8] = 0;
    t[9] = 0;

    out[0] = _mm_set_epi64x(t[1], t[0]);
    out[1] = _mm_set_epi64x(t[3], t[2]);
    out[2] = _mm_set_epi64x(0, t[4] & 0x1ULL);
}

static inline void sqr257(__m128i out[5], const __m128i a[3])
{
    __m128i tmp[2];

    __m128i sqrT = _mm_set_epi64x(
        0x5554515045444140,
        0x1514111005040100
    );

    __m128i mask = _mm_set_epi64x(
        0x0F0F0F0F0F0F0F0F,
        0x0F0F0F0F0F0F0F0F
    );

    // square a[0], covering data[0], data[1]
    tmp[0] = _mm_and_si128(a[0], mask);
    tmp[1] = _mm_srli_epi64(a[0], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    // square a[1], covering data[2], data[3]
    tmp[0] = _mm_and_si128(a[1], mask);
    tmp[1] = _mm_srli_epi64(a[1], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
    out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

    // square a[2], covering data[4], padding
    tmp[0] = _mm_and_si128(a[2], mask);
    tmp[1] = _mm_srli_epi64(a[2], 4);
    tmp[1] = _mm_and_si128(tmp[1], mask);

    tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
    tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);

    out[4] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}

static inline void gf257sqr(__m128i* out, const __m128i* in)
{
    __m128i tmp[5];
    sqr257(tmp, in);
    reduce_clmul257(out, tmp);
}

static inline void gf257mul(__m128i* out, const __m128i* in1, const __m128i* in2)
{
    __m128i tmp[5];
    clmul_schoolbook257(tmp, in1, in2);
    reduce_clmul257(out, tmp);
}

static inline void block257_sqr(block257* out, const block257* in)
{
    __m128i in128[3] = {
        _mm_set_epi64x(in->data[1], in->data[0]),
        _mm_set_epi64x(in->data[3], in->data[2]),
        _mm_set_epi64x(0, in->data[4] & 0x1ULL)
    };

    __m128i out128[3];
    gf257sqr(out128, in128);

    uint64_t words[6];
    _mm_storeu_si128((__m128i*)&words[0], out128[0]);
    _mm_storeu_si128((__m128i*)&words[2], out128[1]);
    _mm_storeu_si128((__m128i*)&words[4], out128[2]);

    out->data[0] = words[0];
    out->data[1] = words[1];
    out->data[2] = words[2];
    out->data[3] = words[3];
    out->data[4] = words[4] & 0x1ULL;
}

static inline void block257_mul(block257* out, const block257* a, const block257* b)
{
    __m128i a128[3] = {
        _mm_set_epi64x(a->data[1], a->data[0]),
        _mm_set_epi64x(a->data[3], a->data[2]),
        _mm_set_epi64x(0, a->data[4] & 0x1ULL)
    };

    __m128i b128[3] = {
        _mm_set_epi64x(b->data[1], b->data[0]),
        _mm_set_epi64x(b->data[3], b->data[2]),
        _mm_set_epi64x(0, b->data[4] & 0x1ULL)
    };

    __m128i out128[3];
    gf257mul(out128, a128, b128);

    uint64_t words[6];
    _mm_storeu_si128((__m128i*)&words[0], out128[0]);
    _mm_storeu_si128((__m128i*)&words[2], out128[1]);
    _mm_storeu_si128((__m128i*)&words[4], out128[2]);

    out->data[0] = words[0];
    out->data[1] = words[1];
    out->data[2] = words[2];
    out->data[3] = words[3];
    out->data[4] = words[4] & 0x1ULL;
}

static void block257_pow_bin(block257* block, const char* exp_bits)
{
    block257 base = *block;
    block257 result = block257_set_one();

    for (const char* p = exp_bits; *p; ++p) {
        if (*p != '0' && *p != '1')
            continue;

        block257 tmp;

        block257_sqr(&tmp, &result);
        result = tmp;

        if (*p == '1') {
            block257_mul(&result, &result, &base);
        }
    }

    block257_mask(&result);
    *block = result;
}

static inline void block257_mersenne_inverse1(block257* block)
{
    static const char EXP_INV_1[] =
        "01010101010101010101010101001010101010101010101010101010010101010101010101010101010100101010101010101010101010101001010101010101010101010101001010101010101010101010101010010101010101010101010101010100101010101010101010101010101001010101010101010101010101001";

    block257_pow_bin(block, EXP_INV_1);
}

static inline void block257_mersenne_inverse2(block257* block)
{
    static const char EXP_INV_2[] =
        "01010100101010100101010100101010100101010100101010100101010100101010100101010100101010100101010100101010100101010100101010100101010010101010010101010010101010010101010010101010010101010010101010010101010010101010010101010010101010010101010010101010010101001";

    block257_pow_bin(block, EXP_INV_2);
}

static inline void block257_multiply_with_GF2_matrix(
    block257* block,
    const uint64_t matrix[257][5]
) {
    block257 tmp;
    tmp.data[0] = 0;
    tmp.data[1] = 0;
    tmp.data[2] = 0;
    tmp.data[3] = 0;
    tmp.data[4] = 0;

    const uint64_t x0 = block->data[0];
    const uint64_t x1 = block->data[1];
    const uint64_t x2 = block->data[2];
    const uint64_t x3 = block->data[3];
    const uint64_t x4 = block->data[4] & 0x1ULL;

    for (size_t row = 0; row < 257; ++row) {
        const uint64_t* A = matrix[row];

        uint64_t bit =
            (_mm_popcnt_u64(x0 & A[0]) ^
             _mm_popcnt_u64(x1 & A[1]) ^
             _mm_popcnt_u64(x2 & A[2]) ^
             _mm_popcnt_u64(x3 & A[3]) ^
             _mm_popcnt_u64(x4 & (A[4] & 0x1ULL))) & 1ULL;

        tmp.data[row >> 6] |= bit << (row & 63);
    }

    tmp.data[4] &= 0x1ULL;
    *block = tmp;
}

// ################### 192 ####################
inline __m128i* block192_as_m128i(block192* block) {
  return (__m128i*)block;
}
inline void clmul_schoolbook192(__m128i out[3], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[3];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);
  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);
}
inline void reduce_clmul192(__m128i out[2], const __m128i in[3]) {
  // modulus = x^192 + x^7 + x^2 + x + 1
  __m128i p = _mm_set_epi64x(0x0, 0x87);
  __m128i t0, t1, t2, t3;
  t0 = _mm_clmulepi64_si128(in[2], p, 0x01); // in[2]_high * p
  t3 = _mm_xor_si128(in[1], t0);             // update in[1]_low and in[1]_high

  t0 = _mm_clmulepi64_si128(in[2], p, 0x00); // in[2]_low * p
  t1 = _mm_slli_si128(t0, 8); // low 64bit of result, shifted to high
  t2 = _mm_srli_si128(t0, 8); // high 64bit of result, shifted to high
  t3 = _mm_xor_si128(t3, t2); // update in[1]_low

  t0 = _mm_clmulepi64_si128(t3, p, 0x01); // in[1]_high * p
  out[0] = _mm_xor_si128(t0, in[0]);
  out[0] = _mm_xor_si128(out[0], t1);
  out[1] = _mm_and_si128(t3, _mm_set_epi64x(0x0, 0xFFFFFFFFFFFFFFFF));
}
inline void sqr192(__m128i out[3], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
}
inline void gf192sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[3];
  sqr192(tmp, in);
  reduce_clmul192(out, tmp);
}
inline void gf192mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[3];
  clmul_schoolbook192(tmp, in1, in2);
  reduce_clmul192(out, tmp);
}
inline void block192_inverse(block192* block) {
  const size_t u[12] = {1, 2, 3, 5, 10, 20, 23, 46, 92, 95, 190, 191};
  // q = u[i] - u[i - 1] should give us the corresponding values
  // (1, 1, 2, 5, 10, 3, 23, 46, 3, 95, 1), which will have corresponding
  // indexes
  const size_t q_index[11] = {0, 0, 1, 3, 4, 2, 6, 7, 2, 9, 0};
  __m128i b[12][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 8);

  for (size_t i = 1; i < 12; ++i) {

    __m128i b_p[2] = {b[i - 1][0], b[i - 1][1]};
    __m128i b_q[2] = {b[q_index[i - 1]][0], b[q_index[i - 1]][1]};

    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf192sqr(b_p, b_p);
    }
    gf192mul(b[i], b_p, b_q);
  }
  __m128i* tmp;
  tmp = (__m128i*)malloc(32); // offset problem..
  gf192sqr(tmp, b[11]);
  memcpy(block, tmp, 24);
  free(tmp);
}
inline void block192_mersenne_inverse(block192* block) {
  // inverse Mersenne S-box with e = 5
// (2 ^ 5 - 1) ^ (-1) mod (2 ^ 192 - 1)
// = 0x5294a5294a5294a5294a5294a5294a5294a5294a5294a529
// =   01010 01010 01010 ... 01010 01
// 38 times '01010' and '01'

  __m128i b[5][2];//in[2], in_01010[2], in_2_01010[2], in_4_01010[2], in_38_01010[2]

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 8);

  __m128i tmp[2], tmp2[2];

  gf192sqr(tmp,b[0]);
  gf192sqr(tmp, tmp);
  gf192mul(tmp, tmp, b[0]);
  gf192sqr(b[1], tmp);

  gf192sqr(tmp, b[1]);
  for(size_t i = 1 ; i < 5; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[2], tmp, b[1]);

  gf192sqr(tmp, b[2]);
  for(size_t i = 1 ; i < 10; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[3], tmp, b[2]);

  gf192sqr(tmp, b[3]);
  for(size_t i = 1 ; i < 20; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, b[3]);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 40; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, tmp2);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 80; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, tmp2);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 20; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(tmp2, tmp, b[3]);
  gf192sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 10; i++){
    gf192sqr(tmp, tmp);
  }
  gf192mul(b[4], tmp, b[2]);

  gf192sqr(tmp, b[4]);
  gf192sqr(tmp, tmp);

  __m128i* tmp3;
  tmp3 = (__m128i*)malloc(32); // offset problem..
  gf192mul(tmp3, tmp, b[0]);
  memcpy(block, tmp3, 24);
  free(tmp3);
}
inline void block192_multiply_with_GF2_matrix(block192* block, const uint64_t* matrix) {
  block192 tmp;
  for (size_t j = 0; j < 3; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*4) + (i*4)];
      uint64_t bit =
          _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1]) ^
                         (((uint64_t*)block)[2] & A[2])) &  1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block192_multiply_with_transposed_GF2_matrix(block192* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 3; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 4, idx >>= 4, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask(idx, 1)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask(idx, 2)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask(idx, 3)));
    }
  }
  block256 tmp = _mm256_xor_si256(cval[0], cval[1]);
  memcpy(block, &tmp, 24);
}

// ################### 256 ####################
inline __m128i* block256_as_m128i(block256* block) {
  return (__m128i*)(block);
}
inline void clmul_schoolbook256(__m128i out[4], const __m128i a[2], const __m128i b[2]) {
  __m128i tmp[4];
  out[0] = _mm_clmulepi64_si128(a[0], b[0], 0x00);

  out[1] = _mm_clmulepi64_si128(a[0], b[0], 0x11);
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[0], b[1], 0x00));
  out[1] = _mm_xor_si128(out[1], _mm_clmulepi64_si128(a[1], b[0], 0x00));

  out[2] = _mm_clmulepi64_si128(a[1], b[1], 0x00);
  out[2] = _mm_xor_si128(out[2], _mm_clmulepi64_si128(a[0], b[1], 0x11));
  out[2] = _mm_xor_si128(out[2], _mm_clmulepi64_si128(a[1], b[0], 0x11));

  out[3] = _mm_clmulepi64_si128(a[1], b[1], 0x11);

  tmp[0] = _mm_clmulepi64_si128(a[0], b[0], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[0], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[0] = _mm_xor_si128(out[0], tmp[1]);
  out[1] = _mm_xor_si128(out[1], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[0], 0x10);
  tmp[1] = _mm_clmulepi64_si128(a[0], b[1], 0x01);
  tmp[2] = _mm_clmulepi64_si128(a[0], b[1], 0x10);
  tmp[3] = _mm_clmulepi64_si128(a[1], b[0], 0x01);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[2] = _mm_xor_si128(tmp[2], tmp[3]);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[2]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[1] = _mm_xor_si128(out[1], tmp[1]);
  out[2] = _mm_xor_si128(out[2], tmp[2]);

  tmp[0] = _mm_clmulepi64_si128(a[1], b[1], 0x01);
  tmp[1] = _mm_clmulepi64_si128(a[1], b[1], 0x10);

  tmp[0] = _mm_xor_si128(tmp[0], tmp[1]);
  tmp[1] = _mm_slli_si128(tmp[0], 8);
  tmp[2] = _mm_srli_si128(tmp[0], 8);

  out[2] = _mm_xor_si128(out[2], tmp[1]);
  out[3] = _mm_xor_si128(out[3], tmp[2]);
}
inline void reduce_clmul256(__m128i out[2], const __m128i in[4]) {
  // modulus = x^256 + x^10 + x^5 + x^2 + 1
  __m128i p = _mm_set_epi64x(0x0, 0x425);
  __m128i t0, t1, t2, t3;
  t0 = _mm_clmulepi64_si128(in[3], p, 0x01); // in[3]_high * p
  t1 = _mm_slli_si128(t0, 8);        // low 64bit of result, shifted to high
  t2 = _mm_srli_si128(t0, 8);        // high 64bit of result, shifted to low
  t3 = _mm_xor_si128(in[2], t2);     // update in[2]_low
  out[1] = _mm_xor_si128(in[1], t1); // update in[1]_hi

  t0 = _mm_clmulepi64_si128(in[3], p, 0x00); // in[3]_low * p
  out[1] = _mm_xor_si128(out[1], t0);        // update in[1]_hi and in[1]_lo

  t0 = _mm_clmulepi64_si128(in[2], p, 0x01); // in[2]_high * p
  t1 = _mm_slli_si128(t0, 8);         // low 64bit of result, shifted to high
  t2 = _mm_srli_si128(t0, 8);         // high 64bit of result, shifted to low
  out[1] = _mm_xor_si128(out[1], t2); // update in[1]_low
  out[0] = _mm_xor_si128(t1, in[0]);
  t0 = _mm_clmulepi64_si128(t3, p, 0x00); // in[2]_low * p
  out[0] = _mm_xor_si128(t0, out[0]);
}
inline void sqr256(__m128i out[4], const __m128i a[2]) {
  __m128i tmp[2];
  __m128i sqrT = _mm_set_epi64x(0x5554515045444140, 0x1514111005040100);
  __m128i mask = _mm_set_epi64x(0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F);
  tmp[0] = _mm_and_si128(a[0], mask);
  tmp[1] = _mm_srli_epi64(a[0], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[0] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[1] = _mm_unpackhi_epi8(tmp[0], tmp[1]);

  tmp[0] = _mm_and_si128(a[1], mask);
  tmp[1] = _mm_srli_epi64(a[1], 4);
  tmp[1] = _mm_and_si128(tmp[1], mask);
  tmp[0] = _mm_shuffle_epi8(sqrT, tmp[0]);
  tmp[1] = _mm_shuffle_epi8(sqrT, tmp[1]);
  out[2] = _mm_unpacklo_epi8(tmp[0], tmp[1]);
  out[3] = _mm_unpackhi_epi8(tmp[0], tmp[1]);
}
inline void gf256sqr(__m128i *out, const __m128i *in) {
  __m128i tmp[4];
  sqr256(tmp, in);
  reduce_clmul256(out, tmp);
}
inline void gf256mul(__m128i *out, const __m128i *in1, const __m128i *in2) {
  __m128i tmp[4];
  clmul_schoolbook256(tmp, in1, in2);
  reduce_clmul256(out, tmp);
}
inline void block256_inverse(block256* block) {
  const size_t u[11] = {1, 2, 3, 6, 12, 15, 30, 60, 120, 240, 255};
  // q = u[i] - u[i - 1] should give us the corresponding values
  // (1, 1, 3, 6, 3, 15, 30, 60, 120, 15), which will have corresponding
  // indexes
  const size_t q_index[10] = {0, 0, 2, 3, 2, 5, 6, 7, 8, 5};
  __m128i b[11][2];

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 16);

  for (size_t i = 1; i < 11; ++i) {

    __m128i b_p[2] = {b[i - 1][0], b[i - 1][1]};
    __m128i b_q[2] = {b[q_index[i - 1]][0], b[q_index[i - 1]][1]};

    for (size_t m = u[q_index[i - 1]]; m; --m) {
      gf256sqr(b_p, b_p);
    }

    gf256mul(b[i], b_p, b_q);
  }
  gf256sqr((__m128i*)block, b[10]);
}
inline void block256_mersenne_inverse(block256* block) {
// inverse Mersenne S-box with e = 3
// (2 ^ 3 - 1) ^ (-1) mod (2 ^ 256 - 1)
// = 0xdb6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6db6d
// =   110 110 110 110 ... 110 1
// 85 times '110' and '1' 85=5*17

  __m128i b[4][2];//in[2], in_110[2], in_5_110[2], in_17_5_110[2]

  memcpy(&b[0][0], &((uint64_t*)block)[0], 16);
  memcpy(&b[0][1], &((uint64_t*)block)[2], 16);
  
  __m128i tmp[2], tmp2[2];

  gf256sqr(tmp, b[0]);
  gf256mul(tmp, tmp, b[0]);
  gf256sqr(b[1], tmp);

  gf256sqr(tmp, b[1]);
  for(size_t i = 1 ; i < 3; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, b[1]);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 6; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 3; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(b[2], tmp, b[1]);

  gf256sqr(tmp, b[2]);
  for(size_t i = 1 ; i < 15; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, b[2]);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 30; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 60; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 120; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(tmp2, tmp, tmp2);
  gf256sqr(tmp, tmp2);
  for(size_t i = 1 ; i < 15; i++){
    gf256sqr(tmp, tmp);
  }
  gf256mul(b[3], tmp, b[2]);
  
  gf256sqr(tmp, b[3]);
  
  gf256mul((__m128i*)block, tmp, b[0]);
  
}
inline void block256_multiply_with_GF2_matrix(block256* block, const uint64_t* matrix) {
  block256 tmp;
  for (size_t j = 0; j < 4; j++) {
    uint64_t t = 0;
    for (size_t i = 0; i < 64; i++) {
      const uint64_t *A = &matrix[(j*64*4) + (i*4)];
      uint64_t bit =
          _mm_popcnt_u64((((uint64_t*)block)[0] & A[0]) ^ (((uint64_t*)block)[1] & A[1]) ^
                         (((uint64_t*)block)[2] & A[2]) ^ (((uint64_t*)block)[3] & A[3])) &   1;
      t ^= (bit << i);
    }
    ((uint64_t*)&tmp)[j] = t;
  }
  *block = tmp;
}
inline void block256_multiply_with_transposed_GF2_matrix(block256* block, const uint64_t* matrix) {
  const uint64_t *vptr = (uint64_t*)block;
  const __m256i *Ablock = (const __m256i*)matrix;

  __m256i cval[2] = {_mm256_setzero_si256(), _mm256_setzero_si256()};
  for (unsigned int w = 4; w; --w, ++vptr) {
    uint64_t idx = *vptr;
    for (unsigned int i = sizeof(uint64_t) * 8; i;
         i -= 4, idx >>= 4, Ablock += 4) {
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[0], mm256_compute_mask(idx, 0)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[1], mm256_compute_mask(idx, 1)));
      cval[0] = _mm256_xor_si256(
          cval[0], _mm256_and_si256(Ablock[2], mm256_compute_mask(idx, 2)));
      cval[1] = _mm256_xor_si256(
          cval[1], _mm256_and_si256(Ablock[3], mm256_compute_mask(idx, 3)));
    }
  }
  block256 tmp = _mm256_xor_si256(cval[0], cval[1]);
  memcpy(block, &tmp, 24);
}


// ##################################################################################################################
// ################################### STOPS NOW ####################################################################
// ##################################################################################################################


inline block128 block128_xor(block128 x, block128 y) { return _mm_xor_si128(x, y); }
inline block256 block256_xor(block256 x, block256 y) { return _mm256_xor_si256(x, y); }
inline block128 block128_and(block128 x, block128 y) { return _mm_and_si128(x, y); }
inline block256 block256_and(block256 x, block256 y) { return _mm256_and_si256(x, y); }
inline block128 block128_set_zero() { return _mm_setzero_si128(); }
inline block256 block256_set_zero() { return _mm256_setzero_si256(); }
inline block128 block128_set_all_8(uint8_t x) { return _mm_set1_epi8(x); }
inline block256 block256_set_all_8(uint8_t x) { return _mm256_set1_epi8(x); }
inline block128 block128_set_low32(uint32_t x) { return _mm_setr_epi32(x, 0, 0, 0); }
inline block256 block256_set_low32(uint32_t x) { return _mm256_setr_epi32(x, 0, 0, 0, 0, 0, 0, 0); }
inline block128 block128_set_low64(uint64_t x) { return _mm_set_epi64x(0, x); }
inline block256 block256_set_low64(uint64_t x) { return _mm256_setr_epi64x(x, 0, 0, 0); }
inline block256 block256_set_128(block128 x0, block128 x1) { return _mm256_setr_m128i(x0, x1); }

inline block256 block256_set_low128(block128 x)
{
	return _mm256_inserti128_si256(_mm256_setzero_si256(), x, 0);
}

inline block384 block384_xor(block384 x, block384 y)
{
	block384 out;
	out.data[0] = block128_xor(x.data[0], y.data[0]);
	out.data[1] = block128_xor(x.data[1], y.data[1]);
	out.data[2] = block128_xor(x.data[2], y.data[2]);
	return out;
}
inline block512 block512_xor(block512 x, block512 y)
{
	block512 out;
	out.data[0] = block256_xor(x.data[0], y.data[0]);
	out.data[1] = block256_xor(x.data[1], y.data[1]);
	return out;
}

inline block384 block384_and(block384 x, block384 y)
{
	block384 out;
	out.data[0] = block128_and(x.data[0], y.data[0]);
	out.data[1] = block128_and(x.data[1], y.data[1]);
	out.data[2] = block128_and(x.data[2], y.data[2]);
	return out;
}
inline block512 block512_and(block512 x, block512 y)
{
	block512 out;
	out.data[0] = block256_and(x.data[0], y.data[0]);
	out.data[1] = block256_and(x.data[1], y.data[1]);
	return out;
}

inline block384 block384_set_zero()
{
	block384 out;
	out.data[0] = block128_set_zero();
	out.data[1] = block128_set_zero();
	out.data[2] = block128_set_zero();
	return out;
}
inline block512 block512_set_zero()
{
	block512 out;
	out.data[0] = block256_set_zero();
	out.data[1] = block256_set_zero();
	return out;
}

inline block384 block384_set_all_8(uint8_t x)
{
	block384 out;
	out.data[0] = block128_set_all_8(x);
	out.data[1] = block128_set_all_8(x);
	out.data[2] = block128_set_all_8(x);
	return out;
}
inline block512 block512_set_all_8(uint8_t x)
{
	block512 out;
	out.data[0] = block256_set_all_8(x);
	out.data[1] = block256_set_all_8(x);
	return out;
}

inline block384 block384_set_low32(uint32_t x)
{
	block384 out;
	out.data[0] = block128_set_low32(x);
	return out;
}
inline block512 block512_set_low32(uint32_t x)
{
	block512 out;
	out.data[0] = block256_set_low32(x);
	return out;
}
inline block384 block384_set_low64(uint64_t x)
{
	block384 out;
	out.data[0] = block128_set_low64(x);
	return out;
}
inline block512 block512_set_low64(uint64_t x)
{
	block512 out;
	out.data[0] = block256_set_low64(x);
	return out;
}

inline bool block128_any_zeros(block128 x)
{
	return _mm_movemask_epi8(_mm_cmpeq_epi8(x, _mm_setzero_si128()));
}

inline bool block256_any_zeros(block256 x)
{
	return _mm256_movemask_epi8(_mm256_cmpeq_epi8(x, _mm256_setzero_si256()));
}

inline bool block192_any_zeros(block192 x)
{
	block256 b = block256_set_zero();
	memcpy(&b, &x, sizeof(x));
	return _mm256_movemask_epi8(_mm256_cmpeq_epi8(b, _mm256_setzero_si256())) & 0x00ffffff;
}

inline block128 block128_byte_reverse(block128 x)
{
	block128 shuffle = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
	return _mm_shuffle_epi8(x, shuffle);
}

inline block256 block256_from_2_block128(block128 x, block128 y)
{
	return _mm256_setr_m128i(x, y);
}

#define VOLE_BLOCK_SHIFT 0
typedef block128 vole_block;
inline vole_block vole_block_set_zero() { return block128_set_zero(); }
inline vole_block vole_block_xor(vole_block x, vole_block y) { return block128_xor(x, y); }
inline vole_block vole_block_and(vole_block x, vole_block y) { return block128_and(x, y); }
inline vole_block vole_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline vole_block vole_block_set_low32(uint32_t x) { return block128_set_low32(x); }
inline vole_block vole_block_set_low64(uint64_t x) { return block128_set_low64(x); }

#ifndef HAVE_VCLMUL
#define POLY_VEC_LEN_SHIFT 0
typedef block128 clmul_block;
inline clmul_block clmul_block_xor(clmul_block x, clmul_block y) { return block128_xor(x, y); }
inline clmul_block clmul_block_and(clmul_block x, clmul_block y) { return block128_and(x, y); }
inline clmul_block clmul_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline clmul_block clmul_block_set_zero() { return block128_set_zero(); }

inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x00);
}
inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x10);
}
inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x01);
}
inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y)
{
	return _mm_clmulepi64_si128(x, y, 0x11);
}

inline clmul_block clmul_block_shift_left_64(clmul_block x)
{
	return _mm_slli_si128(x, 8);
}
inline clmul_block clmul_block_shift_right_64(clmul_block x)
{
	return _mm_srli_si128(x, 8);
}
inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y) // output = y high, x low.
{
	return _mm_alignr_epi8(x, y, 8);
}
inline clmul_block clmul_block_broadcast_low64(clmul_block x)
{
	return _mm_broadcastq_epi64(x);
}

#else
#define POLY_VEC_LEN_SHIFT 1
typedef block256 clmul_block;
inline clmul_block clmul_block_xor(clmul_block x, clmul_block y) { return block256_xor(x, y); }
inline clmul_block clmul_block_and(clmul_block x, clmul_block y) { return block256_and(x, y); }
inline clmul_block clmul_block_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline clmul_block clmul_block_set_zero() { return block256_set_zero(); }

inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x00);
}
inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x10);
}
inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x01);
}
inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y)
{
	return _mm256_clmulepi64_epi128(x, y, 0x11);
}

inline clmul_block clmul_block_shift_left_64(clmul_block x)
{
	return _mm256_slli_si256(x, 8);
}
inline clmul_block clmul_block_shift_right_64(clmul_block x)
{
	return _mm256_srli_si256(x, 8);
}
inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y) // output = y high, x low.
{
	return _mm256_alignr_epi8(x, y, 8);
}
inline clmul_block clmul_block_broadcast_low64(clmul_block x)
{
	return _mm256_shuffle_epi32(x, 0x44);
}
#endif

#endif
