// Outside header guard to handle mutual inclusion.
#include "config.h"
#include "vole_params.h"


#ifndef GREATWALL_H
#define GREATWALL_H

#include "block.h"

#if defined(OWF_GREATWALL)

#if SECURITY_PARAM == 128
extern const uint64_t greatwall_rc_137 [3][3];
extern const uint64_t greatwall_mat_137_0 [137][3];
extern const uint64_t greatwall_mat_137_1 [137][3];
extern const uint64_t greatwall_mat_137_2 [137][3];
extern const uint64_t greatwall_mat_137_2_inv [137][3];
extern const uint64_t greatwall_pow_mat_137_70 [137][3];
extern const uint64_t greatwall_pow_mat_137_75 [137][3];
#elif SECURITY_PARAM == 192
extern const uint64_t greatwall_rc_193 [3][4];
extern const uint64_t greatwall_mat_193_0 [193][4];
extern const uint64_t greatwall_mat_193_1 [193][4];
extern const uint64_t greatwall_mat_193_2 [193][4];
extern const uint64_t greatwall_mat_193_2_inv [193][4];
extern const uint64_t greatwall_pow_mat_193_87 [193][4];
extern const uint64_t greatwall_pow_mat_193_107 [193][4];
#elif SECURITY_PARAM == 256
extern const uint64_t greatwall_rc_257 [3][5];
extern const uint64_t greatwall_mat_257_0 [257][5];
extern const uint64_t greatwall_mat_257_1 [257][5];
extern const uint64_t greatwall_mat_257_2 [257][5];
extern const uint64_t greatwall_mat_257_2_inv [257][5];
extern const uint64_t greatwall_pow_mat_257_114 [257][5];
extern const uint64_t greatwall_pow_mat_257_124 [257][5];
#elif SECURITY_PARAM == 512
extern const uint64_t greatwall_rc_521 [3][9];
extern const uint64_t greatwall_mat_521_0 [521][9];
extern const uint64_t greatwall_mat_521_1 [521][9];
extern const uint64_t greatwall_mat_521_2 [521][9];
extern const uint64_t greatwall_mat_521_2_inv [521][9];
extern const uint64_t greatwall_pow_mat_521_248 [521][9];
extern const uint64_t greatwall_pow_mat_521_273 [521][9];
#endif

#endif

#include "greatwall_impl.h"

void greatwall_encrypt_block(uint64_t* block, const uint64_t* key);

#endif
