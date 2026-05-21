import collections
import sys
from Cryptodome.Hash import SHAKE256

if len(sys.argv) != 3:
    print("Usage: <script> <n> <num_rounds>")
    exit()

n = int(sys.argv[1])
num_rounds = int(sys.argv[2])

if n not in [64, 128, 137, 192, 193, 256, 257, 384, 512]:
    print("Only n in {64, 128, 137, 192, 256, 384, 512} supported.")
    exit()

out_file = open(f"output_n{n}_r{num_rounds}.txt", "w")
sys.stdout = out_file

F = None
if n == 64:
    F.<z> = GF(2^n)
elif n == 128:
    F.<z> = GF(2^n, modulus=x^128 + x^7 + x^2 + x + 1)
elif n == 137:
    F.<z> = GF(2^n, modulus=x^137 + x^21 + 1)
elif n == 192:
    F.<z> = GF(2^n, modulus=x^192 + x^7 + x^2 + x + 1)
elif n == 193:
    F.<z> = GF(2^n, modulus=x^193 + x^15 + 1)
elif n == 256:
    F.<z> = GF(2^n, modulus=x^256 + x^10 + x^5 + x^2 + 1)
elif n == 257:
    F.<z> = GF(2^n, modulus=x^257 + x^12 + 1)
elif n == 384:
    F.<z> = GF(2^n, modulus=x^384 + x^16 + x^15 + x^6 + 1)
elif n == 512:
    F.<z> = GF(2^n, modulus=x^512 + x^8 + x^5 + x^2 + 1)


def gen_random_numbers(state_size):
    shake = SHAKE256.new()
    shake.update(f"GreatWall-{state_size}".encode("utf-8"))

    mask = (1 << int(state_size)) - 1
    n_bytes = int((state_size + 7) // 8)

    while True:
        b = shake.read(n_bytes)
        val = int.from_bytes(b, byteorder="big") & mask
        yield val


rand_gen = gen_random_numbers(n)


def rotate_list_right(l, r):
    d = collections.deque(l)
    d.rotate(r)
    return list(d)


def build_lin_perm_poly(n):
    poly_coefficients = [F.fetch_int(next(rand_gen)) for _ in range(n)]

    matrix_dickson = matrix(
        F,
        [
            rotate_list_right(
                [poly_coefficients[j]^(2^i) for j in range(n)],
                i
            )
            for i in range(n)
        ]
    )

    while F.fetch_int(0) in poly_coefficients or not matrix_dickson.is_invertible():
        poly_coefficients = [F.fetch_int(next(rand_gen)) for _ in range(n)]
        matrix_dickson = matrix(
            F,
            [
                rotate_list_right(
                    [poly_coefficients[j]^(2^i) for j in range(n)],
                    i
                )
                for i in range(n)
            ]
        )

    return poly_coefficients


def build_inverse_poly(poly_coefficients):
    matrix_dickson = matrix(
        F,
        [
            rotate_list_right(
                [poly_coefficients[j]^(2^i) for j in range(n)],
                i
            )
            for i in range(n)
        ]
    )

    determinant = matrix_dickson.determinant()
    adjugate = matrix_dickson.adjugate()

    return [(1 / determinant) * adjugate[0, i] for i in range(n)]


def build_mat_from_poly(poly_coefficients):
    M = matrix(GF(2), n, n)

    basis = [F.fetch_int(2)^i for i in range(n)]
    dual_basis = F.dual_basis()

    poly_evaluations = [
        sum(poly_coefficients[j] * (basis[i]^(2^j)) for j in range(n))
        for i in range(n)
    ]

    for i in range(n):
        for j in range(n):
            M[i, j] = (dual_basis[i] * poly_evaluations[j]).trace()

    return M


def print_u64_words_from_int(x, n):
    words = (n + 63) // 64
    vals = []

    for w in range(words):
        val = (x >> (64 * w)) & ((1 << 64) - 1)

        if w == words - 1:
            valid_bits = n - 64 * w
            val &= (1 << valid_bits) - 1

        vals.append(val)

    print("    {" + ", ".join(f"0x{v:016x}" for v in vals) + "},")


def print_mat_as_c_array(name, M):
    words = (n + 63) // 64

    print(f"const uint64_t {name}[{n}][{words}] = {{")

    for i in range(n):
        vals = []

        for w in range(words):
            val = 0

            for j in range(64):
                col = 64 * w + j
                if col < n:
                    bit = M[i, col]
                    val |= int(bit) << j

            vals.append(val)

        print("    {" + ", ".join(f"0x{v:016x}" for v in vals) + "},")

    print("};")


def build_linear_layer():
    poly_coefficients = build_lin_perm_poly(n)
    poly_inverse_coefficients = build_inverse_poly(poly_coefficients)

    while F.fetch_int(0) in poly_inverse_coefficients:
        poly_coefficients = build_lin_perm_poly(n)
        poly_inverse_coefficients = build_inverse_poly(poly_coefficients)

    M = build_mat_from_poly(poly_coefficients)
    M_inv = build_mat_from_poly(poly_inverse_coefficients)

    return M, M_inv


print(f"--- n = {n} ---")
print("Irreducible polynomial:", F.modulus())
print("----------")

# 3 round constants
print("--- ROUND CONSTANTS ---")
print(f"const uint64_t greatwall_rc_{n}[3][{(n + 63) // 64}] = {{")

round_constants = []
for i in range(3):
    rc = next(rand_gen) & ((1 << n) - 1)
    round_constants.append(F.fetch_int(rc))
    print_u64_words_from_int(rc, n)

print("};")

# 3 matrices
print("--- LINEAR LAYERS ---")

for i in range(3):

    M, M_inv = build_linear_layer()
    print_mat_as_c_array(f"greatwall_mat_{n}_{i}", M.transpose())
    
    if i == 2:
        print_mat_as_c_array(f"greatwall_mat_{n}_{i}_inv", M_inv.transpose())

out_file.close()