# ASCENT

This repository implement ASCENT framework to accelerate NTT in different degrees and modulus on spatial architecture.

## Setup

CPU: i9-14900K / i3-4160K

OS: Ubuntu 2024.04 TLS

Tools: Vitis 2025.1

Hardware: XCVC1902-VSVA2179-2MP-ES

## C-profile

  We provide C file as a benchmark running NTT with different degrees on CPUs. To execute, open terminal and type

    ./ntt_profile 10000

  This testbench implements 256-, 512-, and 1024-degree NTT as they satisfy most of the PQC applications built on LWE problems. If you want to try more test case, feel free to change the twiddle factors, degrees, and  modular. 

## AIE/HLE Examples

  We also provide 512- and 1024-degree implementations of NTT on both AI engines and FPGA, for reproducible research.  
