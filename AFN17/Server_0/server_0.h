#include <x86intrin.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <../boost/asio.hpp>
#include "../../DPF-CPP/AES.h"
#include "../../DPF-CPP/dpf.h"
#include "../../TW-PIR/utils.h"
#include "../../TW-PIR/alignment_allocator.h"
#include "../../Hash-Function/utils.h"

struct PosStruct {
    size_t x = MAX_SIZE_T;
    size_t x_pos = MAX_SIZE_T;
    size_t x_mul2_pos = MAX_SIZE_T;
    size_t x_mul2ms1_pos = MAX_SIZE_T;    
};

class Server_0 {
public:
    // Given the pararmeters
    size_t N;
    size_t element_size;
    size_t num_cell_per_element;
    size_t lg_N;
    // Tree information
    size_t Z_value;
    size_t Z_aux;
    size_t Num_slice;
    size_t Slice_size;
    size_t L;
    size_t Leaf_num;
    size_t Z_pos;
    size_t access_counter;
    size_t evict_counter;
    
    Server_0(boost::asio::io_context& io_context, short port, size_t N, size_t element_size);
    void Setup();
    void Access();
    // void Rebuild(size_t level);
    // void RebuildMaxLevel();

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    std::array<char, 1024> recv_buffer_;
    
    // Storage
    // Level + Bucket + Pos
    std::vector<std::vector<std::vector<std::pair<element_type, size_t>>>> Value_Tree; 
    std::vector<std::vector<std::pair<element_type, size_t>>> Value_Aux;
    // Which Tree + Level + Bucket + Pos, dataNum: [4, 8, 16, ...]
    std::vector<std::vector<std::vector<std::vector<PosStruct>>>> Pos_Tree;

    void evict_pos();
    void evict_value(size_t leafID);

     // std::vector<std::vector<std::vector<std::tuple<size_t, size_t, size_t, size_t>>>>& Path_List,

};