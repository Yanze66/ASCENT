#include "ntt_q0.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static bool read_512(const std::string& path, std::vector<int32_t>& v)
{
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "Cannot open: " << path << "\n";
        return false;
    }

    v.clear();
    v.reserve(NTT_N);

    long long x;
    while (fin >> x && (int)v.size() < NTT_N)
        v.push_back((int32_t)x);

    if ((int)v.size() != NTT_N) {
        std::cerr << path << " contains only " << v.size()
                  << " values; expected " << NTT_N << "\n";
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    std::string input_file =
            "/home/yzwu/Desktop/fast_fft/hls_component/stage0_input_q0.txt";
    std::string golden_file =
    "/home/yzwu/Desktop/fast_fft/hls_component/p_q0_stage8_golden.txt";
    std::vector<int32_t> input;
    std::vector<int32_t> golden;

    if (!read_512(input_file, input))
        return 1;

    if (!read_512(golden_file, golden))
        return 1;

    hls::stream<ap_int<32>> in_stream;
    hls::stream<ap_int<32>> out_stream;

    for (int i = 0; i < NTT_N; ++i)
        in_stream.write((ap_int<32>)input[i]);

    ntt_q0_9stage(in_stream, out_stream);

    int errors = 0;

    std::ofstream fout("hls_ntt_q0_output.txt");

    for (int i = 0; i < NTT_N; ++i) {
        if (out_stream.empty()) {
            std::cerr << "ERROR: output stream ended at index "
                      << i << "\n";
            return 2;
        }

        ap_int<32> y_ap = out_stream.read();
        int32_t y = (int32_t)y_ap;

        fout << y << "\n";

        if (y != golden[i]) {
            if (errors < 16) {
                std::cerr
                    << "Mismatch @" << i
                    << ": HLS=" << y
                    << ", golden=" << golden[i]
                    << "\n";
            }
            ++errors;
        }
    }

    if (!out_stream.empty()) {
        std::cerr << "WARNING: output stream contains extra data\n";
    }

    if (errors == 0) {
        std::cout << "PASS: 512/512 outputs match Stage8 golden.\n";
        return 0;
    }

    std::cout << "FAIL: " << errors << "/" << NTT_N
              << " coefficients mismatch.\n";
    return 3;
}
