#ifndef BLOCK_H
#define BLOCK_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"

typedef struct
{
	uint64_t data[3];
} block137;

typedef struct
{
	uint64_t data[3];
} block192;

typedef struct
{
	uint64_t data[4];
} block193;

typedef struct
{
	uint64_t data[5];
} block257;

typedef struct
{
	uint64_t data[9];
} block521;

static inline void block137_canonicalize(block137* a) {
    a->data[2] &= 0x1FFULL;
}

static inline void block137_load_to_block(block137* out, const uint8_t* in)
{
	assert((in[17] & 0xFE) == 0);
    memset(out, 0, sizeof(*out));
    memcpy(out, in, 18);
    out->data[2] &= 0x1FFULL;
}

static inline void block137_load_words(block137* out, const uint64_t in[3])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2] & 0x1FFULL;

    assert((in[2] & ~0x1FFULL) == 0);
}

static inline bool block137_is_canonical(const block137* in)
{
    return (in->data[2] & ~0x1FFULL) == 0;
}

static inline void block137_store_from_block(uint8_t* out, const block137* in)
{
	assert(block137_is_canonical(in));
	memcpy(out, in, 18);
}
inline block137 block137_xor(block137 x, block137 y)
{
	// Plain c version for now at least. Hopefully it will be autovectorized.
	block137 out;
	out.data[0] = x.data[0] ^ y.data[0];
	out.data[1] = x.data[1] ^ y.data[1];
	out.data[2] = x.data[2] ^ y.data[2];
	return out;
}


static inline void block193_canonicalize(block193* a) {
    a->data[3] &= 0x1ULL;
}

static inline bool block193_is_canonical(const block193* in)
{
    return (in->data[3] & ~0x1ULL) == 0;
}

static inline void block193_load_to_block(block193* out, const uint8_t* in)
{
    assert((in[24] & 0xFE) == 0);
    memset(out, 0, sizeof(*out));
    memcpy(out, in, 25);
    out->data[3] &= 0x1ULL;
}

static inline void block193_load_words(block193* out, const uint64_t in[4])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2];
    out->data[3] = in[3] & 0x1ULL;

    assert((in[3] & ~0x1ULL) == 0);
}

static inline void block193_store_from_block(uint8_t* out, const block193* in)
{
    assert(block193_is_canonical(in));
    memcpy(out, in, 25);
}

static inline block193 block193_xor(block193 x, block193 y)
{
    block193 out;
    out.data[0] = x.data[0] ^ y.data[0];
    out.data[1] = x.data[1] ^ y.data[1];
    out.data[2] = x.data[2] ^ y.data[2];
    out.data[3] = (x.data[3] ^ y.data[3]) & 0x1ULL;
    return out;
}

static inline void block257_canonicalize(block257* a) {
    a->data[4] &= 0x1ULL;
}

static inline bool block257_is_canonical(const block257* in)
{
    return (in->data[4] & ~0x1ULL) == 0;
}

static inline void block257_load_to_block(block257* out, const uint8_t* in)
{
    // 257 bit = 32 bytes + 1 bit
    assert((in[32] & 0xFE) == 0);

    memset(out, 0, sizeof(*out));
    memcpy(out, in, 33);

    out->data[4] &= 0x1ULL;
}

static inline void block257_load_words(block257* out, const uint64_t in[5])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2];
    out->data[3] = in[3];
    out->data[4] = in[4] & 0x1ULL;

    assert((in[4] & ~0x1ULL) == 0);
}

static inline void block257_store_from_block(uint8_t* out, const block257* in)
{
    assert(block257_is_canonical(in));

    memcpy(out, in, 33);

    out[32] &= 0x01;
}

static inline block257 block257_xor(block257 x, block257 y)
{
    block257 out;

    out.data[0] = x.data[0] ^ y.data[0];
    out.data[1] = x.data[1] ^ y.data[1];
    out.data[2] = x.data[2] ^ y.data[2];
    out.data[3] = x.data[3] ^ y.data[3];
    out.data[4] = (x.data[4] ^ y.data[4]) & 0x1ULL;

    return out;
}

static inline void block521_canonicalize(block521* a) {
    a->data[8] &= 0x1FFULL;
}

static inline bool block521_is_canonical(const block521* in)
{
    return (in->data[8] & ~0x1FFULL) == 0;
}

static inline void block521_load_to_block(block521* out, const uint8_t* in)
{
    // 521 bit = 65 bytes + 1 bit
    assert((in[65] & 0xFE) == 0);

    memset(out, 0, sizeof(*out));
    memcpy(out, in, 66);

    out->data[8] &= 0x1FFULL;
}

static inline void block521_load_words(block521* out, const uint64_t in[9])
{
    out->data[0] = in[0];
    out->data[1] = in[1];
    out->data[2] = in[2];
    out->data[3] = in[3];
    out->data[4] = in[4];
    out->data[5] = in[5];
    out->data[6] = in[6];
    out->data[7] = in[7];
    out->data[8] = in[8] & 0x1FFULL;

    assert((in[8] & ~0x1FFULL) == 0);
}

static inline void block521_store_from_block(uint8_t* out, const block521* in)
{
    assert(block521_is_canonical(in));

    memcpy(out, in, 66);

    out[65] &= 0x01;
}

static inline block521 block521_xor(block521 x, block521 y)
{
    block521 out;

    out.data[0] = x.data[0] ^ y.data[0];
    out.data[1] = x.data[1] ^ y.data[1];
    out.data[2] = x.data[2] ^ y.data[2];
    out.data[3] = x.data[3] ^ y.data[3];
    out.data[4] = x.data[4] ^ y.data[4];
    out.data[5] = x.data[5] ^ y.data[5];
    out.data[6] = x.data[6] ^ y.data[6];
    out.data[7] = x.data[7] ^ y.data[7];
    out.data[8] = (x.data[8] ^ y.data[8]) & 0x1FFULL;

    return out;
}





inline block192 block192_xor(block192 x, block192 y)
{
	// Plain c version for now at least. Hopefully it will be autovectorized.
	block192 out;
	out.data[0] = x.data[0] ^ y.data[0];
	out.data[1] = x.data[1] ^ y.data[1];
	out.data[2] = x.data[2] ^ y.data[2];
	return out;
}

inline block192 block192_and(block192 x, block192 y)
{
	block192 out;
	out.data[0] = x.data[0] & y.data[0];
	out.data[1] = x.data[1] & y.data[1];
	out.data[2] = x.data[2] & y.data[2];
	return out;
}

inline block192 block192_set_all_8(uint8_t x)
{
	uint64_t x64 = x;
	x64 *= UINT64_MAX / 0xff;

	block192 out = {{x64, x64, x64}};
	return out;
}

inline block192 block192_set_low64(uint64_t x)
{
	block192 out = {{x, 0, 0}};
	return out;
}

inline block192 block192_set_low32(uint32_t x)
{
	return block192_set_low64(x);
}

inline block192 block192_set_zero()
{
	return block192_set_low64(0);
}

#include "block_impl.h"

// Interface defined by block_impl.h

// typedef /**/ block128;
// typedef /**/ block256;
// typedef /**/ block384;
// typedef /**/ block512;
//
// // Block representing a chunck of a column for the small field VOLE. Used when reducing the PRG
// // outputs down to a VOLE correlation. THis will be at least as big as vole_cipher_block.
// typedef /**/ vole_block;
// #define VOLE_BLOCK_SHIFT /**/

// Number of block128s in a vole_block.
#define VOLE_BLOCK (1 << VOLE_BLOCK_SHIFT)

static_assert(sizeof(block128) == 16, "Padding in block128.");
static_assert(sizeof(block192) == 24, "Padding in block192.");
static_assert(sizeof(block256) == 32, "Padding in block256.");
static_assert(sizeof(block384) == 48, "Padding in block384.");
static_assert(sizeof(block512) == 64, "Padding in block512.");

inline block128 block128_xor(block128 x, block128 y);
inline block256 block256_xor(block256 x, block256 y);
inline block384 block384_xor(block384 x, block384 y);
inline block512 block512_xor(block512 x, block512 y);
inline vole_block vole_block_xor(vole_block x, vole_block y);

inline block128 block128_and(block128 x, block128 y);
inline block256 block256_and(block256 x, block256 y);
inline block384 block384_and(block384 x, block384 y);
inline block512 block512_and(block512 x, block512 y);
inline vole_block vole_block_and(vole_block x, vole_block y);

inline block128 block128_set_zero();
inline block256 block256_set_zero();
inline block384 block384_set_zero();
inline block512 block512_set_zero();

inline block128 block128_set_all_8(uint8_t x);
inline block256 block256_set_all_8(uint8_t x);
inline block384 block384_set_all_8(uint8_t x);
inline block512 block512_set_all_8(uint8_t x);
inline vole_block vole_block_set_all_8(uint8_t x);

inline block128 block128_set_low32(uint32_t x);
inline block256 block256_set_low32(uint32_t x);
inline block384 block384_set_low32(uint32_t x);
inline block512 block512_set_low32(uint32_t x);
inline vole_block vole_block_set_low32(uint32_t x);

inline block128 block128_set_low64(uint64_t x);
inline block256 block256_set_low64(uint64_t x);
inline block384 block384_set_low64(uint64_t x);
inline block512 block512_set_low64(uint64_t x);
inline vole_block vole_block_set_low64(uint64_t x);

inline block256 block256_set_128(block128 x0, block128 x1);
inline block256 block256_set_low128(block128 x);

inline bool block128_any_zeros(block128 x);
inline bool block192_any_zeros(block192 x);
inline bool block256_any_zeros(block256 x);
inline bool block512_any_zeros(block512 x);

inline block128 block128_byte_reverse(block128 x);

inline block256 block256_from_2_block128(block128 x, block128 y);

#if SECURITY_PARAM == 128
#define BLOCK_SECPAR_LEN_SHIFT 0
#define BLOCK_2SECPAR_LEN 2
typedef block128 block_secpar;
typedef block256 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block128_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block128_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block128_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block128_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block128_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block128_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block256_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block256_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block256_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block256_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block256_set_zero(); }
#elif SECURITY_PARAM == 192
#define BLOCK_2SECPAR_LEN 3
typedef block192 block_secpar;
typedef block384 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block192_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block192_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block192_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block192_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block192_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block192_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block192_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block384_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block384_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block384_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block384_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block384_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block384_set_zero(); }
#elif SECURITY_PARAM == 256
#define BLOCK_SECPAR_LEN_SHIFT 1
#define BLOCK_2SECPAR_LEN 4
typedef block256 block_secpar;
typedef block512 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block256_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block256_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block256_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block256_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block256_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block256_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block256_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y) { return block512_xor(x, y); }
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y) { return block512_and(x, y); }
inline block_2secpar block_2secpar_set_all_8(uint8_t x) { return block512_set_all_8(x); }
inline block_2secpar block_2secpar_set_low32(uint32_t x) { return block512_set_low32(x); }
inline block_2secpar block_2secpar_set_low64(uint64_t x) { return block512_set_low64(x); }
inline block_2secpar block_2secpar_set_zero() { return block512_set_zero(); }
#elif SECURITY_PARAM == 512
#define BLOCK_SECPAR_LEN_SHIFT 2
#define BLOCK_2SECPAR_LEN 8
typedef block512 block_secpar;
typedef struct
{
	block512 data[2];
} block1024;
typedef block1024 block_2secpar;
inline block_secpar block_secpar_xor(block_secpar x, block_secpar y) { return block512_xor(x, y); }
inline block_secpar block_secpar_and(block_secpar x, block_secpar y) { return block512_and(x, y); }
inline block_secpar block_secpar_set_all_8(uint8_t x) { return block512_set_all_8(x); }
inline block_secpar block_secpar_set_low32(uint32_t x) { return block512_set_low32(x); }
inline block_secpar block_secpar_set_low64(uint64_t x) { return block512_set_low64(x); }
inline block_secpar block_secpar_set_zero() { return block512_set_zero(); }
inline bool block_secpar_any_zeros(block_secpar x) { return block512_any_zeros(x); }
inline block_2secpar block_2secpar_xor(block_2secpar x, block_2secpar y)
{
	x.data[0] = block512_xor(x.data[0], y.data[0]);
	x.data[1] = block512_xor(x.data[1], y.data[1]);
	return x;
}
inline block_2secpar block_2secpar_and(block_2secpar x, block_2secpar y)
{
	x.data[0] = block512_and(x.data[0], y.data[0]);
	x.data[1] = block512_and(x.data[1], y.data[1]);
	return x;
}
inline block_2secpar block_2secpar_set_all_8(uint8_t x)
{
	block_2secpar out = {{block512_set_all_8(x), block512_set_all_8(x)}};
	return out;
}
inline block_2secpar block_2secpar_set_low32(uint32_t x)
{
	block_2secpar out = {{block512_set_low32(x), block512_set_zero()}};
	return out;
}
inline block_2secpar block_2secpar_set_low64(uint64_t x)
{
	block_2secpar out = {{block512_set_low64(x), block512_set_zero()}};
	return out;
}
inline block_2secpar block_2secpar_set_zero()
{
	block_2secpar out = {{block512_set_zero(), block512_set_zero()}};
	return out;
}
#endif

// Number of block128s in a block_secpar, assuming that this is a whole number.
#define BLOCK_SECPAR_LEN (1 << BLOCK_SECPAR_LEN_SHIFT)

#endif
