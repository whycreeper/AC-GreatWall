import sys

if len(sys.argv) != 2:
    print("Usage: sage gen_pow_mat_137.sage <e>")
    exit()

n = 257
e = int(sys.argv[1])

F.<z> = GF(2^n, modulus=x^257 + x^12 + 1)

WORDS = (n + 63) // 64


def field_to_bits(a):
    v = a.polynomial()
    bits = [0] * n

    for i in range(n):
        bits[i] = int(v[i]) if i <= v.degree() else 0

    return bits


def print_mat_as_c(name, M):
    print(f"const uint64_t {name}[{n}][{WORDS}] = {{")

    for row in range(n):
        vals = []

        for w in range(WORDS):
            val = 0
            for b in range(64):
                col = 64 * w + b
                if col < n:
                    val |= int(M[row, col]) << b

            if w == WORDS - 1:
                val &= (1 << (n - 64 * w)) - 1

            vals.append(val)

        print("    {" + ", ".join(f"0x{v:016x}" for v in vals) + "},")

    print("};")


# ensure bits(x^(2^e)) = M * bits(x)
M = matrix(GF(2), n, n)

for col in range(n):
    basis_elem = z^col
    image = basis_elem^(2^e)
    image_bits = field_to_bits(image)

    for row in range(n):
        M[row, col] = image_bits[row]


print_mat_as_c(f"greatwall_pow_mat_{n}_{e}", M)