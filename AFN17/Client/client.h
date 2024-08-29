#include <x86intrin.h>
#include <cmath>
#include <../boost/asio.hpp>
#include "../../DPF-CPP/AES.h"
#include "../../DPF-CPP/dpf.h"
#include "../../TW-PIR/utils.h"
#include "../../TW-PIR/alignment_allocator.h"
#include "../../Hash-Function/utils.h"

typedef __m128i key_type;

struct PosStruct {
    size_t x = MAX_SIZE_T;
    size_t x_pos = MAX_SIZE_T;
    size_t x_mul2_pos = MAX_SIZE_T;
    size_t x_mul2ms1_pos = MAX_SIZE_T;    
};

class Client {
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

    std::vector<size_t> A_pos;
    std::vector<std::vector<PosStruct>> Pos_Stash;

    Client(size_t N, size_t element_size);
    void Setup();
    element_type Access(char op, addr_type req_addr, uint8_t* write_value);
    // void Rebuild(size_t level);
    // void RebuildMaxLevel();

    // Cost
    size_t setup_bandwidth;
    size_t access_bandwidth;

private:
    // Server connections
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket server0_socket;
    boost::asio::ip::tcp::socket server1_socket;
    
    
    void connect_to_server(boost::asio::ip::tcp::socket& socket, const std::string& server_ip, int server_port);
    // Return <oldLeaf, newLeaf>
    std::pair<size_t, size_t> get_evict_pos(size_t vir_addr);
    size_t evict_pos(size_t vir_addr);
    void evict_value(size_t evict_counter);
};