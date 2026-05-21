#ifndef OWF_PROOF_H
#define OWF_PROOF_H



	#define OWF_KEY_SCHEDULE_CONSTRAINTS 0
	#define OWF_BLOCKS 1
	#define OWF_ROUNDS 2

#if SECURITY_PARAM == 128
	typedef block137 owf_block;
	#define OWF_KEY_WITNESS_BITS 144
	#define OWF_BLOCK_SIZE 18
	#define OWF_CONSTRAINTS_PER_ROUND 2
	
#elif SECURITY_PARAM == 192
	typedef block193 owf_block;
	#define OWF_KEY_WITNESS_BITS 200
	#define OWF_BLOCK_SIZE 25
	#define OWF_CONSTRAINTS_PER_ROUND 2

#elif SECURITY_PARAM == 256
	typedef block257 owf_block;
	#define OWF_KEY_WITNESS_BITS 264
	#define OWF_BLOCK_SIZE 33
	#define OWF_CONSTRAINTS_PER_ROUND 2

#endif

	// inline owf_block owf_block_xor(owf_block x, owf_block y) { return block_secpar_xor(x, y); }
	// inline owf_block owf_block_set_low32(uint32_t x) { return block_secpar_set_low32(x); }
	// inline bool owf_block_any_zeros(owf_block x) { return block_secpar_any_zeros(x); }


#define OWF_NUM_CONSTRAINTS (OWF_BLOCKS * OWF_CONSTRAINTS_PER_ROUND * OWF_ROUNDS + OWF_KEY_SCHEDULE_CONSTRAINTS)
#define WITNESS_BITS (8 * OWF_BLOCKS * OWF_BLOCK_SIZE * (OWF_ROUNDS - 1) + OWF_KEY_WITNESS_BITS)

#include "aes.h"
#include "rain.h"
#include "greatwall.h"
#include "mq.h"
#include "quicksilver.h"

struct public_key;
typedef struct public_key public_key;

void owf_constraints_prover(quicksilver_state* state, const public_key* pk);
void owf_constraints_verifier(quicksilver_state* state, const public_key* pk);

#endif
