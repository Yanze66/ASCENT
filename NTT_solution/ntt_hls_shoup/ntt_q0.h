#ifndef NTT_Q0_H
#define NTT_Q0_H

#include <ap_int.h>
#include <hls_stream.h>

static const int NTT_N = 512;
static const ap_uint<32> Q0 = 167772161u;

void ntt_q0_9stage(
    hls::stream<ap_int<32>>& in,
    hls::stream<ap_int<32>>& out);

#endif
