## Introduction
This is a project for the paper "Efficient Two-server ORAM for Resource-Constrained Clients". This project also implements the state-of-the-art [LO13](https://eprint.iacr.org/2011/384.pdf), [AFN17](https://eprint.iacr.org/2016/849.pdf), and [KM19](https://arxiv.org/pdf/1802.05145.pdf). Note that we also compare the results with Duoram(https://eprint.iacr.org/2022/1747.pdf), please see https://git-crysp.uwaterloo.ca/avadapal/duoram for the detailed implentations. 

## Prerequisites
1. A system with an x86 processor that supports AVX/AVX2 instructions. 
2. Ubuntu 20.04 operating system.
3. Gcc version $\geq$ 9.4.

## Quick Start
  - Clone this project

        git clone https://github.com/GfKbYu/Cforam.git
        cd Cforam
    
  - Build and test our Cforam
  
        ./build_and_run.sh Ours test_one 10 1024 32 32 0
        
  - The above command means that testing the Cforam with ``<log_database_size>=10, <access_times>=1024, <element_size>=32, <tag_size>=32, <0Or2>=0``. The output is stored in the file ``Ours/Result/ours_results.csv`` as follows:
  
        log_database_size,element_size,setup_time (s),setup_bandwidth (B),amortized_access_time (s),amortized_access_bandwidth (B)
        10,32,0.0335716,196608,0.016307,4645.89

## Running the experiments

### Figure 5
  - Build and run

        cd TW-PIR/build
        cmake ..
        make
        ./test_pir_read
        ./test_pir_write
        
  - The test results is showed in the folder ``TW-PIR/Result/``

### Figures 6,7
  - Before running the program, you can use linux ``tc`` to control and simulate different network situations

        sudo tc qdisc add dev lo root netem delay 50us rate 80gbit

  - Build and test for different schemes, database sizes, and block sizes

        ./build_and_run.sh <scheme> test_mul <log_database_sizes length> <log_database_sizes values> <element_sizes length> <element_sizes values> <0Or2>
        
  - For example, testing our Cforam for different database sizes under ``element_size=32``

        ./build_and_run.sh Ours test_mul 4 6 8 10 12 1 32 0

        
### Figure 8
  - Running Cforam and Cforam for 8-byte blocks

        ./build_and_run.sh Ours test_mul 6 15 16 17 18 19 20 1 32 2
        ./build_and_run.sh OursOpt test_mul 6 15 16 17 18 19 20 1 32 2
