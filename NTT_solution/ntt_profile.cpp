#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>

static constexpr uint32_t Q = 8380417u;

static inline uint32_t add_mod(uint32_t a, uint32_t b) {
    uint32_t s = a + b;
    if (s >= Q) s -= Q;
    return s;
}

static inline uint32_t sub_mod(uint32_t a, uint32_t b) {
    return (a >= b) ? (a - b) : (a + Q - b);
}

static inline uint32_t mul_mod(uint32_t a, uint32_t b) {
    return static_cast<uint32_t>((static_cast<uint64_t>(a) * b) % Q);
}

static uint32_t pow_mod(uint32_t a, uint64_t e) {
    uint64_t r = 1, x = a;
    while (e) {
        if (e & 1) r = (r * x) % Q;
        x = (x * x) % Q;
        e >>= 1;
    }
    return static_cast<uint32_t>(r);
}

// q-1 = 8380416 = 2^13 * 3 * 11 * 31
static uint32_t primitive_generator() {
    static constexpr uint32_t factors[] = {2, 3, 11, 31};
    for (uint32_t g = 2; g < Q; ++g) {
        bool ok = true;
        for (uint32_t p : factors) {
            if (pow_mod(g, (Q - 1) / p) == 1) {
                ok = false;
                break;
            }
        }
        if (ok) return g;
    }
    std::abort();
}

template <size_t N>
static inline void bit_reverse_permute(std::array<uint32_t, N>& a) {
    for (size_t i = 1, j = 0; i < N; ++i) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
}

template <size_t N>
static void ntt_forward(std::array<uint32_t, N>& a, uint32_t root_n) {
    static_assert((N & (N - 1)) == 0, "N must be power of two");
    bit_reverse_permute(a);

    for (size_t len = 2; len <= N; len <<= 1) {
        uint32_t wlen = pow_mod(root_n, N / len);
        for (size_t i = 0; i < N; i += len) {
            uint32_t w = 1;
            size_t half = len >> 1;
            for (size_t j = 0; j < half; ++j) {
                uint32_t u = a[i + j];
                uint32_t v = mul_mod(a[i + j + half], w);
                a[i + j] = add_mod(u, v);
                a[i + j + half] = sub_mod(u, v);
                w = mul_mod(w, wlen);
            }
        }
    }
}

template <size_t N>
static void ntt_inverse(std::array<uint32_t, N>& a, uint32_t root_n) {
    uint32_t root_inv = pow_mod(root_n, Q - 2);
    ntt_forward<N>(a, root_inv);
    uint32_t n_inv = pow_mod(static_cast<uint32_t>(N), Q - 2);
    for (auto& x : a) x = mul_mod(x, n_inv);
}

static void ntt_256(std::array<uint32_t, 256>& a, uint32_t root) {
    ntt_forward<256>(a, root);
}
static void ntt_512(std::array<uint32_t, 512>& a, uint32_t root) {
    ntt_forward<512>(a, root);
}
static void ntt_1024(std::array<uint32_t, 1024>& a, uint32_t root) {
    ntt_forward<1024>(a, root);
}

static volatile uint64_t g_sink = 0;

template <size_t N>
static void verify_roundtrip(uint32_t root_n) {
    std::array<uint32_t, N> a{}, original{};
    std::mt19937 rng(12345 + static_cast<unsigned>(N));
    std::uniform_int_distribution<uint32_t> dist(0, Q - 1);
    for (size_t i = 0; i < N; ++i) a[i] = original[i] = dist(rng);
    ntt_forward<N>(a, root_n);
    ntt_inverse<N>(a, root_n);
    if (a != original) {
        std::cerr << "Round-trip verification FAILED for N=" << N << "\n";
        std::exit(1);
    }
}

template <size_t N>
static void benchmark(uint32_t root_n, size_t iterations) {
    std::array<uint32_t, N> input{}, work{};
    std::mt19937 rng(20261012 + static_cast<unsigned>(N));
    std::uniform_int_distribution<uint32_t> dist(0, Q - 1);
    for (auto& x : input) x = dist(rng);

    for (int k = 0; k < 100; ++k) {
        work = input;
        ntt_forward<N>(work, root_n);
        g_sink ^= work[k % N];
    }

    using clock = std::chrono::steady_clock;
    double total_ns = 0.0;
    double min_ns = 1e100;

    for (size_t it = 0; it < iterations; ++it) {
        work = input; // not timed
        auto t0 = clock::now();
        ntt_forward<N>(work, root_n);
        auto t1 = clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
        total_ns += ns;
        if (ns < min_ns) min_ns = ns;
        g_sink ^= work[it % N];
    }

    double avg_ns = total_ns / iterations;
    std::cout << "N=" << N
              << "  avg=" << avg_ns / 1000.0 << " us"
              << "  min=" << min_ns / 1000.0 << " us"
              << "  NTT/s=" << 1e9 / avg_ns
              << "  coeff/s=" << (1e9 / avg_ns) * N
              << "\n";
}

int main(int argc, char** argv) {
    size_t iterations = 10000;
    if (argc > 1) iterations = std::strtoull(argv[1], nullptr, 10);

    uint32_t g = primitive_generator();
    uint32_t root256 = pow_mod(g, (Q - 1) / 256);
    uint32_t root512 = pow_mod(g, (Q - 1) / 512);
    uint32_t root1024 = pow_mod(g, (Q - 1) / 1024);

    std::cout << "q=" << Q << "\n";
    std::cout << "primitive generator g=" << g << "\n";
    std::cout << "root256=" << root256
              << " root512=" << root512
              << " root1024=" << root1024 << "\n";

    if (pow_mod(root256,256) != 1 || pow_mod(root256,128) == 1 ||
        pow_mod(root512,512) != 1 || pow_mod(root512,256) == 1 ||
        pow_mod(root1024,1024) != 1 || pow_mod(root1024,512) == 1) {
        std::cerr << "Root-order check FAILED\n";
        return 1;
    }

    verify_roundtrip<256>(root256);
    verify_roundtrip<512>(root512);
    verify_roundtrip<1024>(root1024);
    std::cout << "Round-trip verification: PASS\n";

    benchmark<256>(root256, iterations);
    benchmark<512>(root512, iterations);
    benchmark<1024>(root1024, iterations);

    std::cout << "checksum=" << g_sink << "\n";
    return 0;
}
