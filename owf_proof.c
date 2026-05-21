#include "config.h"

#include "aes.h"
#include "rain.h"
#include "greatwall.h"
#include "mq.h"
#include "faest_details.h"
#include "owf_proof.h"
#include "quicksilver.h"
#include <stdio.h>
#include <string.h>

static void debug_print_bytes(const char* tag, const void* data, size_t len)
{
    const unsigned char* p = (const unsigned char*)data;
    printf("%s = ", tag);
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

static void debug_print_poly_secpar(const char* tag, poly_secpar_vec x)
{
    debug_print_bytes(tag, &x, sizeof(x));
}

static void debug_print_vec_gfsecpar(const char* tag, quicksilver_vec_gfsecpar x, bool verifier)
{
    char buf[128];

    if (!verifier) {
        snprintf(buf, sizeof(buf), "%s.value", tag);
        debug_print_poly_secpar(buf, x.value);
    }

    snprintf(buf, sizeof(buf), "%s.mac", tag);
    debug_print_poly_secpar(buf, x.mac);
}

static int debug_poly_secpar_eq(poly_secpar_vec a, poly_secpar_vec b)
{
    return memcmp(&a, &b, sizeof(a)) == 0;
}


#define NUM_COLS (OWF_BLOCK_SIZE / 4)
#define N_WD (SECURITY_PARAM / 32)
// NEW
    #define S_ENC OWF_ROUNDS

static inline void gf137_shift_reduce(
    qs_gf137_bits* out,
    const qs_gf137_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[273];

    for (size_t i = 0; i < 273; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 137; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }

    // poly: x^137 + x^21 + 1

    for (int pos = 272; pos >= 137; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2(state);

        // X^pos → X^{pos-137+21} + X^{pos-137}
        tmp[pos - 137 + 21] =
            quicksilver_add_gf2(state, tmp[pos - 137 + 21], t);

        tmp[pos - 137] =
            quicksilver_add_gf2(state, tmp[pos - 137], t);
    }

    for (size_t i = 0; i < 137; ++i) {
        out->bit[i] = tmp[i];
    }
}

static inline void gf193_shift_reduce(
    qs_gf193_bits* out,
    const qs_gf193_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[385];

    for (size_t i = 0; i < 385; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 193; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }

    // poly: x^193 + x^15 + 1
    for (int pos = 384; pos >= 193; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2();

        // X^pos -> X^{pos-193+15} + X^{pos-193}
        tmp[pos - 193 + 15] =
            quicksilver_add_gf2(state, tmp[pos - 193 + 15], t);

        tmp[pos - 193] =
            quicksilver_add_gf2(state, tmp[pos - 193], t);
    }

    for (size_t i = 0; i < 193; ++i) {
        out->bit[i] = tmp[i];
    }
}

static inline void gf257_shift_reduce(
    qs_gf257_bits* out,
    const qs_gf257_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[513];

    for (size_t i = 0; i < 513; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 257; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }

    // poly: x^257 + x^12 + 1
    for (int pos = 512; pos >= 257; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2();

        // X^pos -> X^{pos-257+a} + X^{pos-257}
        tmp[pos - 257 + 12] =
            quicksilver_add_gf2(
                state,
                tmp[pos - 257 + 12],
                t
            );

        tmp[pos - 257] =
            quicksilver_add_gf2(
                state,
                tmp[pos - 257],
                t
            );
    }

    for (size_t i = 0; i < 257; ++i) {
        out->bit[i] = tmp[i];
    }
}

static void qs_gf137_mul_constraint(
    quicksilver_state* state,
    const qs_gf137_bits* x,
    const qs_gf137_bits* y,
    const qs_gf137_bits* z)
{
    quicksilver_vec_deg2 acc_lo = quicksilver_zero_deg2();
    quicksilver_vec_deg2 acc_hi = quicksilver_zero_deg2();

    for (size_t i = 0; i < 137; ++i) {

        quicksilver_vec_gf2 xi = x->bit[i];
        quicksilver_vec_gfsecpar Xi = quicksilver_combine_1_bit(state, xi);

        qs_gf137_bits Li_bits;
        gf137_shift_reduce(&Li_bits, y, i, state);

        quicksilver_vec_gfsecpar Li_lo = quicksilver_combine_secpar_bits(state, Li_bits.bit);
        quicksilver_vec_gfsecpar Li_hi = quicksilver_combine_secpar_bits_not_full(state, Li_bits.bit + 128);

        acc_lo = quicksilver_add_deg2(
            state,
            acc_lo,
            quicksilver_mul(state, Xi, Li_lo)
        );

        acc_hi = quicksilver_add_deg2(
            state,
            acc_hi,
            quicksilver_mul(state, Xi, Li_hi)
        );
    }

    quicksilver_vec_gfsecpar z_lo = quicksilver_combine_secpar_bits(state, z->bit);
    quicksilver_vec_gfsecpar z_hi = quicksilver_combine_secpar_bits_not_full(state, z->bit + 128);

    acc_lo = quicksilver_add_deg2(
        state,
        acc_lo,
        quicksilver_mul(state, z_lo, quicksilver_one_gfsecpar(state))
    );

    acc_hi = quicksilver_add_deg2(
        state,
        acc_hi,
        quicksilver_mul(state, z_hi, quicksilver_one_gfsecpar(state))
    );

    quicksilver_constraint(state, acc_lo);
    quicksilver_constraint(state, acc_hi);
}

static void qs_gf193_mul_constraint(
    quicksilver_state* state,
    const qs_gf193_bits* x,
    const qs_gf193_bits* y,
    const qs_gf193_bits* z)
{
    quicksilver_vec_deg2 acc_lo = quicksilver_zero_deg2();
    quicksilver_vec_deg2 acc_hi = quicksilver_zero_deg2();

    for (size_t i = 0; i < 193; ++i) {
        quicksilver_vec_gf2 xi = x->bit[i];
        quicksilver_vec_gfsecpar Xi =
            quicksilver_combine_1_bit(state, xi);

        qs_gf193_bits Li_bits;
        gf193_shift_reduce(&Li_bits, y, i, state);

        quicksilver_vec_gfsecpar Li_lo =
            quicksilver_combine_secpar_bits(state, Li_bits.bit); // 0..191

        quicksilver_vec_gfsecpar Li_hi =
            quicksilver_combine_1_bit(state, Li_bits.bit[192]);  // bit 192

        acc_lo = quicksilver_add_deg2(
            state,
            acc_lo,
            quicksilver_mul(state, Xi, Li_lo)
        );

        acc_hi = quicksilver_add_deg2(
            state,
            acc_hi,
            quicksilver_mul(state, Xi, Li_hi)
        );
    }

    quicksilver_vec_gfsecpar z_lo =
        quicksilver_combine_secpar_bits(state, z->bit); // 0..191

    quicksilver_vec_gfsecpar z_hi =
        quicksilver_combine_1_bit(state, z->bit[192]);  // bit 192

    acc_lo = quicksilver_add_deg2(
        state,
        acc_lo,
        quicksilver_mul(state, z_lo, quicksilver_one_gfsecpar(state))
    );

    acc_hi = quicksilver_add_deg2(
        state,
        acc_hi,
        quicksilver_mul(state, z_hi, quicksilver_one_gfsecpar(state))
    );

    quicksilver_constraint(state, acc_lo);
    quicksilver_constraint(state, acc_hi);
}

static void qs_gf257_mul_constraint(
    quicksilver_state* state,
    const qs_gf257_bits* x,
    const qs_gf257_bits* y,
    const qs_gf257_bits* z)
{
    quicksilver_vec_deg2 acc_lo = quicksilver_zero_deg2();
    quicksilver_vec_deg2 acc_hi = quicksilver_zero_deg2();

    for (size_t i = 0; i < 257; ++i) {
        quicksilver_vec_gf2 xi = x->bit[i];

        quicksilver_vec_gfsecpar Xi =
            quicksilver_combine_1_bit(state, xi);

        qs_gf257_bits Li_bits;
        gf257_shift_reduce(&Li_bits, y, i, state);

        quicksilver_vec_gfsecpar Li_lo =
            quicksilver_combine_secpar_bits(state, Li_bits.bit);

        quicksilver_vec_gfsecpar Li_hi =
            quicksilver_combine_1_bit(state, Li_bits.bit[256]);

        acc_lo = quicksilver_add_deg2(
            state,
            acc_lo,
            quicksilver_mul(state, Xi, Li_lo)
        );

        acc_hi = quicksilver_add_deg2(
            state,
            acc_hi,
            quicksilver_mul(state, Xi, Li_hi)
        );
    }

    quicksilver_vec_gfsecpar z_lo =
        quicksilver_combine_secpar_bits(state, z->bit);

    quicksilver_vec_gfsecpar z_hi =
        quicksilver_combine_1_bit(state, z->bit[256]);

    acc_lo = quicksilver_add_deg2(
        state,
        acc_lo,
        quicksilver_mul(state, z_lo, quicksilver_one_gfsecpar(state))
    );

    acc_hi = quicksilver_add_deg2(
        state,
        acc_hi,
        quicksilver_mul(state, z_hi, quicksilver_one_gfsecpar(state))
    );

    quicksilver_constraint(state, acc_lo);
    quicksilver_constraint(state, acc_hi);
}

static ALWAYS_INLINE void load_rc137_bits(
    quicksilver_state* state,
    qs_gf137_bits* out,
    const uint64_t rc_words[3])
{
    for (size_t i = 0; i < 137; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE void load_rc193_bits(
    quicksilver_state* state,
    qs_gf193_bits* out,
    const uint64_t rc_words[4])
{
    for (size_t i = 0; i < 193; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE void load_rc257_bits(
    quicksilver_state* state,
    qs_gf257_bits* out,
    const uint64_t rc_words[5])
{
    for (size_t i = 0; i < 257; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE uint8_t mat137_get_bit(
    const uint64_t matrix[137][3],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE uint8_t mat193_get_bit(
    const uint64_t matrix[193][4],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE uint8_t mat257_get_bit(
    const uint64_t matrix[257][5],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE void enc_constraints(quicksilver_state* state, owf_block out) {
    
#if SECURITY_PARAM == 128

        qs_gf137_bits inv_inputs[S_ENC];
        qs_gf137_bits inv_outputs[S_ENC];
        qs_gf137_bits pow_outputs[S_ENC];
    
        
        qs_gf137_bits sk_bits;
        for (size_t bit_j = 0; bit_j < 137; ++bit_j){
            sk_bits.bit[bit_j] = quicksilver_get_witness_vec(state, bit_j);
            inv_outputs[0].bit[bit_j] = quicksilver_get_witness_vec(state, bit_j + 144);
        }

        qs_gf137_bits rc0_bits, rc1_bits, rc2_bits;

        load_rc137_bits(state, &rc0_bits, greatwall_rc_137[0]);
        load_rc137_bits(state, &rc1_bits, greatwall_rc_137[1]);
        load_rc137_bits(state, &rc2_bits, greatwall_rc_137[2]);

        qs_gf137_bits matrix0_output, matrix1_output, matrix2_output;
        for (size_t matrow = 0; matrow < 137; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_0, matrow, matcol);
                if (bit) {
                    matrix0_output.bit[matrow] = quicksilver_add_gf2(state, matrix0_output.bit[matrow], sk_bits.bit[matcol]);
                }
            }
        }

        for (size_t i = 0; i < 137; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_pow_mat_137_70, matrow, matcol);
                if (bit) {
                    pow_outputs[0].bit[matrow] = quicksilver_add_gf2(state, pow_outputs[0].bit[matrow], inv_outputs[0].bit[matcol]);
                }
            }
        }
        
        qs_gf137_bits tmp;

        for (size_t i = 0; i < 137; ++i) {
            tmp.bit[i] = quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_1, matrow, matcol);
                if (bit) {
                    matrix1_output.bit[matrow] = quicksilver_add_gf2(state, matrix1_output.bit[matrow], tmp.bit[matcol]);
                }
            }
        }

        for (size_t i = 0; i < 137; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    rc1_bits.bit[i]
                );
        }

        qs_gf137_bits out_bits;
        for (size_t i = 0; i < 137; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2();
        }

        for (size_t i = 0; i < 137; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc2_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_2_inv, matrow, matcol);
                if (bit) {
                    inv_outputs[1].bit[matrow] = quicksilver_add_gf2(state, inv_outputs[1].bit[matrow], matrix2_output.bit[matcol]);
                }
            }
        }
        
        for (size_t matrow = 0; matrow < 137; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_pow_mat_137_75, matrow, matcol);
                if (bit) {
                    pow_outputs[1].bit[matrow] = quicksilver_add_gf2(state, pow_outputs[1].bit[matrow], inv_outputs[1].bit[matcol]);
                }
            }
        }
        for(int i = 0; i < S_ENC; i++)qs_gf137_mul_constraint(state, &inv_inputs[i], &inv_outputs[i], &pow_outputs[i]);

#elif SECURITY_PARAM == 192

        qs_gf193_bits inv_inputs[S_ENC];
        qs_gf193_bits inv_outputs[S_ENC];
        qs_gf193_bits pow_outputs[S_ENC];

        qs_gf193_bits sk_bits;

        for (size_t bit_j = 0; bit_j < 193; ++bit_j) {
            sk_bits.bit[bit_j] = quicksilver_get_witness_vec(state, bit_j);

            inv_outputs[0].bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j + 200);
        }

        qs_gf193_bits rc0_bits, rc1_bits, rc2_bits;

        load_rc193_bits(state, &rc0_bits, greatwall_rc_193[0]);
        load_rc193_bits(state, &rc1_bits, greatwall_rc_193[1]);
        load_rc193_bits(state, &rc2_bits, greatwall_rc_193[2]);

        qs_gf193_bits matrix0_output, matrix1_output, matrix2_output;

        /* matrix0_output = M0 * sk */
        for (size_t matrow = 0; matrow < 193; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 193; matcol++) {
                uint8_t bit = mat193_get_bit(greatwall_mat_193_0, matrow, matcol);

                if (bit) {
                    matrix0_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix0_output.bit[matrow],
                            sk_bits.bit[matcol]
                        );
                }
            }
        }

        /* inv_inputs[0] = M0 * sk + rc0 */
        for (size_t i = 0; i < 193; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }

        /* pow_outputs[0] = PowMat_1 * inv_outputs[0] */
        for (size_t matrow = 0; matrow < 193; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 193; matcol++) {
                uint8_t bit =
                    mat193_get_bit(greatwall_pow_mat_193_87, matrow, matcol);

                if (bit) {
                    pow_outputs[0].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[0].bit[matrow],
                            inv_outputs[0].bit[matcol]
                        );
                }
            }
        }

        /* tmp = inv_outputs[0] + sk */
        qs_gf193_bits tmp;

        for (size_t i = 0; i < 193; ++i) {
            tmp.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }

        /* matrix1_output = M1 * tmp */
        for (size_t matrow = 0; matrow < 193; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 193; matcol++) {
                uint8_t bit = mat193_get_bit(greatwall_mat_193_1, matrow, matcol);

                if (bit) {
                    matrix1_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix1_output.bit[matrow],
                            tmp.bit[matcol]
                        );
                }
            }
        }

        /* inv_inputs[1] = M1 * tmp + rc1 */
        for (size_t i = 0; i < 193; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    rc1_bits.bit[i]
                );
        }

        /* public output bits */
        qs_gf193_bits out_bits;

        for (size_t i = 0; i < 193; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2();
        }

        /*
        * matrix2_output = inv_inputs[1] + rc2 + sk + out
        */
        for (size_t i = 0; i < 193; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc2_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }

        /* inv_outputs[1] = M2^{-1} * matrix2_output */
        for (size_t matrow = 0; matrow < 193; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 193; matcol++) {
                uint8_t bit =
                    mat193_get_bit(greatwall_mat_193_2_inv, matrow, matcol);

                if (bit) {
                    inv_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            inv_outputs[1].bit[matrow],
                            matrix2_output.bit[matcol]
                        );
                }
            }
        }

        /* pow_outputs[1] = PowMat_2 * inv_outputs[1] */
        for (size_t matrow = 0; matrow < 193; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 193; matcol++) {
                uint8_t bit =
                    mat193_get_bit(greatwall_pow_mat_193_107, matrow, matcol);

                if (bit) {
                    pow_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[1].bit[matrow],
                            inv_outputs[1].bit[matcol]
                        );
                }
            }
        }
        for(int i = 0; i < S_ENC; i++)qs_gf193_mul_constraint(state, &inv_inputs[i], &inv_outputs[i], &pow_outputs[i]);

#elif SECURITY_PARAM == 256

        qs_gf257_bits inv_inputs[S_ENC];
        qs_gf257_bits inv_outputs[S_ENC];
        qs_gf257_bits pow_outputs[S_ENC];

        qs_gf257_bits sk_bits;

        for (size_t bit_j = 0; bit_j < 257; ++bit_j) {
            sk_bits.bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j);

            inv_outputs[0].bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j + 264);
        }

        qs_gf257_bits rc0_bits, rc1_bits, rc2_bits;

        load_rc257_bits(state, &rc0_bits, greatwall_rc_257[0]);
        load_rc257_bits(state, &rc1_bits, greatwall_rc_257[1]);
        load_rc257_bits(state, &rc2_bits, greatwall_rc_257[2]);

        qs_gf257_bits matrix0_output, matrix1_output, matrix2_output;

        /* matrix0_output = M0 * sk */
        for (size_t matrow = 0; matrow < 257; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 257; matcol++) {
                uint8_t bit =
                    mat257_get_bit(greatwall_mat_257_0, matrow, matcol);

                if (bit) {
                    matrix0_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix0_output.bit[matrow],
                            sk_bits.bit[matcol]
                        );
                }
            }
        }

        /* inv_inputs[0] = M0 * sk + rc0 */
        for (size_t i = 0; i < 257; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }

        /* pow_outputs[0] = PowMat_1 * inv_outputs[0] */
        for (size_t matrow = 0; matrow < 257; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 257; matcol++) {
                uint8_t bit =
                    mat257_get_bit(greatwall_pow_mat_257_114,
                                   matrow,
                                   matcol);

                if (bit) {
                    pow_outputs[0].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[0].bit[matrow],
                            inv_outputs[0].bit[matcol]
                        );
                }
            }
        }

        /* tmp = inv_outputs[0] + sk */
        qs_gf257_bits tmp;

        for (size_t i = 0; i < 257; ++i) {
            tmp.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }

        /* matrix1_output = M1 * tmp */
        for (size_t matrow = 0; matrow < 257; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 257; matcol++) {
                uint8_t bit =
                    mat257_get_bit(greatwall_mat_257_1, matrow, matcol);

                if (bit) {
                    matrix1_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix1_output.bit[matrow],
                            tmp.bit[matcol]
                        );
                }
            }
        }

        /* inv_inputs[1] = M1 * tmp + rc1 */
        for (size_t i = 0; i < 257; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    rc1_bits.bit[i]
                );
        }

        /* public output bits */
        qs_gf257_bits out_bits;

        for (size_t i = 0; i < 257; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2(state);
        }

        /*
         * matrix2_output = inv_inputs[1] + rc2 + sk + out
         */
        for (size_t i = 0; i < 257; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc2_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }

        /* inv_outputs[1] = M2^{-1} * matrix2_output */
        for (size_t matrow = 0; matrow < 257; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 257; matcol++) {
                uint8_t bit =
                    mat257_get_bit(greatwall_mat_257_2_inv,
                                   matrow,
                                   matcol);

                if (bit) {
                    inv_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            inv_outputs[1].bit[matrow],
                            matrix2_output.bit[matcol]
                        );
                }
            }
        }

        /* pow_outputs[1] = PowMat_2 * inv_outputs[1] */
        for (size_t matrow = 0; matrow < 257; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 257; matcol++) {
                uint8_t bit =
                    mat257_get_bit(greatwall_pow_mat_257_124,
                                   matrow,
                                   matcol);

                if (bit) {
                    pow_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[1].bit[matrow],
                            inv_outputs[1].bit[matcol]
                        );
                }
            }
        }

        for (int i = 0; i < S_ENC; i++) {
            qs_gf257_mul_constraint(
                state,
                &inv_inputs[i],
                &inv_outputs[i],
                &pow_outputs[i]
            );
        }

#endif

    // if (!state->verifier){

    //     debug_print_bytes("public out", &out, sizeof(out));

    //     debug_print_bytes("secret sk", state->witness, 16);

    //     for (size_t sbox_j = 0; sbox_j < S_ENC; ++sbox_j) {
    //         char name[64];

    //         snprintf(name, sizeof(name), "inv_inputs[%zu]", sbox_j);
    //         debug_print_vec_gfsecpar(name, inv_inputs[sbox_j], state->verifier);

    //         snprintf(name, sizeof(name), "inv_outputs[%zu]", sbox_j);
    //         debug_print_vec_gfsecpar(name, inv_outputs[sbox_j], state->verifier);

    //         snprintf(name, sizeof(name), "pow_outputs[%zu]", sbox_j);
    //         debug_print_vec_gfsecpar(name, pow_outputs[sbox_j], state->verifier);

    //         poly_secpar_vec lhs =
    //             poly_2secpar_reduce_secpar(
    //                 poly_secpar_mul(inv_inputs[sbox_j].value, inv_outputs[sbox_j].value)
    //             );

    //         printf("---- check sbox_j = %zu ----\n", sbox_j);
    //         debug_print_poly_secpar("lhs = input*output", lhs);
    //         debug_print_poly_secpar("rhs = pow", pow_outputs[sbox_j].value);

    //         if (!debug_poly_secpar_eq(lhs, pow_outputs[sbox_j].value)) {
    //             printf("MISMATCH at sbox_j = %zu\n", sbox_j);
    //         } else {
    //             printf("OK at sbox_j = %zu\n", sbox_j);
    //         }
    //     }
    // }
    
}

static ALWAYS_INLINE void owf_constraints(quicksilver_state* state, const public_key* pk)
{
    enc_constraints(state, pk->owf_output);
}

void owf_constraints_prover(quicksilver_state* state, const public_key* pk)
{
	assert(!state->verifier);
	state->verifier = false; // Let the compiler know that it is constant.
	owf_constraints(state, pk);
}

void owf_constraints_verifier(quicksilver_state* state, const public_key* pk)
{
	assert(state->verifier);
	state->verifier = true; // Let the compiler know that it is constant.
	owf_constraints(state, pk);
}


// extern inline owf_block owf_block_xor(owf_block x, owf_block y);
// extern inline owf_block owf_block_set_low32(uint32_t x);
// extern inline bool owf_block_any_zeros(owf_block x);
