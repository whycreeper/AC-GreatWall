#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "quicksilver.h"
#include "vole_check.h"
#include "vole_params.h"
#include "vector_com.h"
#include "vole_commit.h"

// #if defined(OWF_AES_CTR)
// #define FAEST_IV_BYTES (OWF_BLOCKS * OWF_BLOCK_SIZE)
// #elif defined(OWF_RIJNDAEL_EVEN_MANSOUR)
// #define FAEST_IV_BYTES (SECURITY_PARAM / 8)
// #elif defined(OWF_RAIN_3) || defined(OWF_RAIN_4)
// #define FAEST_IV_BYTES (SECURITY_PARAM / 8)
// #elif defined(OWF_GREATWALL)
#define FAEST_IV_BYTES 0
// #elif defined(OWF_MQ_2_1)
// #define FAEST_IV_BYTES (SECURITY_PARAM / 8)
// #elif defined(OWF_MQ_2_8)
// #define FAEST_IV_BYTES (SECURITY_PARAM / 8)
// #endif

#if SECURITY_PARAM == 128
#define GREATWALL_SECRET_KEY_BITS 137
#elif SECURITY_PARAM == 192
#define GREATWALL_SECRET_KEY_BITS 193
#elif SECURITY_PARAM == 256
#define GREATWALL_SECRET_KEY_BITS 257
// #elif defined(OWF_GREATWALL) && SECURITY_PARAM == 384
// #define GREATWALL_SECRET_KEY_BITS 389
// #elif defined(OWF_GREATWALL) && SECURITY_PARAM == 512
// #define GREATWALL_SECRET_KEY_BITS 521
#endif

// #if defined(OWF_MQ_2_1) || defined(OWF_MQ_2_8)
// #define FAEST_SECRET_KEY_BYTES ((MQ_M*MQ_GF_BITS)/8 + FAEST_IV_BYTES)
// #elif defined(OWF_GREATWALL)
#define GREATWALL_SECRET_KEY_BYTES (((GREATWALL_SECRET_KEY_BITS + 7) / 8) + FAEST_IV_BYTES)
// #else
// #define FAEST_SECRET_KEY_BYTES ((SECURITY_PARAM / 8) + FAEST_IV_BYTES)
// #endif


#define GREATWALL_PUBLIC_KEY_BYTES GREATWALL_SECRET_KEY_BYTES

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0 && ZERO_BITS_IN_CHALLENGE_3 == 0
	#define COUNTER_BYTES 0
#else
	#define COUNTER_BYTES 4
#endif

#define FAEST_SIGNATURE_BYTES ( \
	VOLE_COMMIT_SIZE + \
	VOLE_CHECK_PROOF_BYTES + \
	WITNESS_BITS / 8 + \
	QUICKSILVER_PROOF_BYTES + \
	VECTOR_COM_OPEN_SIZE + \
	SECURITY_PARAM / 8 + \
	16 + COUNTER_BYTES)

// Find the public key corresponding to a given secret key. Returns true if sk_packed is a valid
// secret key, and false otherwise. For key generation, this function is intended to be called
// repeatedly on random values of sk_packed until a valid key is found. pk_packed must be
// FAEST_PUBLIC_KEY_BYTES long, while sk_packed must be FAEST_SECRET_KEY_BYTES long.
bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed);

// Generate a signature (of length FAEST_SIGNATURE_BYTES) for a message msg (of length msg_len)
// using a secret key sk_packed (of length FAEST_SECRET_KEY_BYTES) and randomness random_seed (of
// length random_seed_len). random_seed can be set to null for deterministic signatures. The
// randomness is mixed with pseudorandom bits generated from secret key and the message, to protect
// against bad randomness. Returns true if sk_packed is a valid secret key, and false otherwise.
bool faest_sign(
	uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
	const uint8_t* random_seed, size_t random_seed_len);

// Verify a signature (of length FAEST_SIGNATURE_BYTES) for a message msg (of length msg_len)
// using a public key pk_packed (of length FAEST_PUBLIC_KEY_BYTES). Returns true for a valid
// signature and false otherwise.
bool faest_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed);
