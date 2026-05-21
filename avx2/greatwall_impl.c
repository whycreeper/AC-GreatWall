#include "greatwall.h"

#include <assert.h>

// void greatwall_encrypt_block(uint64_t* block, const uint64_t* key) {
//     #if defined(OWF_GREATWALL)
//     #if SECURITY_PARAM == 128
//     block128 sbox_out;
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_128[0], (uint64_t*)&greatwall_mat_128[0*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_128[1], (uint64_t*)&greatwall_mat_128[1*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_last_round_function(block, key, (uint64_t*)&greatwall_rc_128[2]);
//     #elif SECURITY_PARAM == 192
//     block192 sbox_out;
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_192[0], (uint64_t*)&greatwall_mat_192[0*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_192[1], (uint64_t*)&greatwall_mat_192[1*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_last_round_function(block, key, (uint64_t*)&greatwall_rc_192[2]);
//     #elif SECURITY_PARAM == 256
//     block256 sbox_out;
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_256[0], (uint64_t*)&greatwall_mat_256[0*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_round_function(block, key, (uint64_t*)&greatwall_rc_256[1], (uint64_t*)&greatwall_mat_256[1*GREATWALL_SECRET_KEY_BITS], (uint64_t*)&sbox_out);
//     greatwall_last_round_function(block, key, (uint64_t*)&greatwall_rc_256[2]);
//     #endif
//     #endif
// }

