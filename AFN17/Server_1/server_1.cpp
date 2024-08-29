#include "server_1.h"

Server_1::Server_1(boost::asio::io_context& io_context, short port, size_t N, size_t element_size): 
    N(N), element_size(element_size),
    num_cell_per_element(static_cast<size_t>(element_size / sizeof(cell_type))),
    lg_N(static_cast<size_t>(log2(N))), Z_value(lg_N*lg_N*lg_N), Z_aux(lg_N*lg_N), Num_slice(lg_N), Slice_size(lg_N*lg_N),
    L(std::ceil(static_cast<double>(lg_N)/log2(Num_slice))), Leaf_num(static_cast<size_t>(pow(Num_slice, L))), Z_pos(4),
    access_counter(0), evict_counter(0),
    acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    socket_(io_context)
{
    // Level + Bucket + Pos
    Value_Tree.resize(L + 1);
    for (size_t i = 0; i < L + 1; i++) {
        Value_Tree[i].resize(static_cast<size_t>(pow(Num_slice, i)));
        for (size_t j = 0; j < Value_Tree[i].size(); j++) {
            Value_Tree[i][j].resize(Z_value);
            for (size_t k = 0; k < Z_value; k++) {
                Value_Tree[i][j][k].first.resize(num_cell_per_element);
                Value_Tree[i][j][k].second = MAX_SIZE_T;
            }
        }
    }

    // Leaf_num aux
    Value_Aux.resize(Leaf_num);
    for (size_t i = 0; i < Leaf_num; i++) {
        Value_Aux[i].resize(Z_aux);
        for (size_t k = 0; k < Z_aux; k++) {
            Value_Aux[i][k].first.resize(num_cell_per_element);
            Value_Aux[i][k].second = MAX_SIZE_T;
        }
    }

    acceptor_.accept(socket_);
    set_socket_timeout(socket_, 6000);
}


void Server_1::evict_value(size_t evict_counter) {
    element_type ele(num_cell_per_element);
    size_t tmp_counter = evict_counter % Leaf_num;
    std::vector<uint8_t> aaaa;
    // Which bucket, which slice
    std::vector<std::pair<size_t, size_t>> reverseOrder = toReverseLexicographicalOrder(tmp_counter, L, Num_slice);
    for (size_t i = 0; i < L; i++) {
        // Send pos to Client
        element_array_type tmp_ele_array(num_cell_per_element * Z_value);
        for (size_t j = 0; j < Z_value; j++) {
            ele = Value_Tree[i][reverseOrder[i].first][j].first;
            write_data(tmp_ele_array, j, element_size, ele);
            Value_Tree[i][reverseOrder[i].first][j].first = get_zero_element(element_size);
            Value_Tree[i][reverseOrder[i].first][j].second = MAX_SIZE_T;
        }
        for (size_t j = 0; j < Num_slice; j++) {
            for (size_t k = 0; k < Slice_size; k++) {
                size_t dpf_key_size = getDPF_keySize(Z_value);
                std::vector<uint8_t> dpf_key(dpf_key_size);
                boost::asio::read(socket_, boost::asio::buffer(dpf_key.data(), dpf_key_size));
                size_t log_tab_length = std::ceil(log2(Z_value));
                if(log_tab_length > 10) {aaaa = DPF::EvalFull8(dpf_key, log_tab_length);}    
                else {aaaa = DPF::EvalFull(dpf_key, log_tab_length);}
                ele = pir_read(tmp_ele_array, aaaa, Z_value, element_size);
                boost::asio::write(socket_, boost::asio::buffer(ele.data(), element_size));

                boost::asio::read(socket_, boost::asio::buffer(ele.data(), element_size));
                Value_Tree[i + 1][reverseOrder[i].first * Num_slice + j][reverseOrder[i + 1].second * Slice_size + k].first = ele;
            }
        }
    }

    // Final level
    element_array_type tmp_ele_array(num_cell_per_element * (Z_value + Z_aux));
    for (size_t j = 0; j < Z_value; j++) {
        ele = Value_Tree[L][reverseOrder[L].first][j].first;
        write_data(tmp_ele_array, j, element_size, ele);
        Value_Tree[L][reverseOrder[L].first][j].first = get_zero_element(element_size);
        Value_Tree[L][reverseOrder[L].first][j].second = MAX_SIZE_T;
    }
    for (size_t j = 0; j < Z_aux; j++) {
        ele = Value_Aux[reverseOrder[L].first][j].first;
        write_data(tmp_ele_array, j + Z_value, element_size, ele);
        Value_Aux[reverseOrder[L].first][j].first = get_zero_element(element_size);
        Value_Aux[reverseOrder[L].first][j].second = MAX_SIZE_T;
    }
    for (size_t k = 0; k < Z_aux; k++) {
        size_t dpf_key_size = getDPF_keySize(Z_value + Z_aux);
        std::vector<uint8_t> dpf_key(dpf_key_size);
        boost::asio::read(socket_, boost::asio::buffer(dpf_key.data(), dpf_key_size));
        size_t log_tab_length = std::ceil(log2(Z_value + Z_aux));
        if(log_tab_length > 10) {aaaa = DPF::EvalFull8(dpf_key, log_tab_length);}    
        else {aaaa = DPF::EvalFull(dpf_key, log_tab_length);}
        ele = pir_read(tmp_ele_array, aaaa, Z_value + Z_aux, element_size);
        boost::asio::write(socket_, boost::asio::buffer(ele.data(), element_size));

        boost::asio::read(socket_, boost::asio::buffer(ele.data(), element_size));
        Value_Aux[reverseOrder[L].first][k].first = ele;
    }
}

void Server_1::Setup() {
    element_type ele(num_cell_per_element);
    /**
     * Setup: Insert random N elements to the server
     */
    for (size_t i = 0 ;i < N; i++) {
        access_counter++;
        boost::asio::read(socket_, boost::asio::buffer(ele.data(), element_size));
        Value_Tree[0][0][access_counter % (Z_value >> 1)].first = ele;

        if (access_counter % (Z_value >> 1) == 0) {
            evict_value(evict_counter);
            evict_counter++;
        }
    }
}


void Server_1::Access() {
    element_type ele(num_cell_per_element);
    size_t pos, tmp_addr;
    std::vector<uint8_t> aaaa;
    boost::asio::read(socket_, boost::asio::buffer(&pos, sizeof(size_t)));
    std::vector<size_t> path = getPathFromRootToLeaf(pos, L, Num_slice);
    element_array_type tmp_ele_array(num_cell_per_element * (Z_value * (L + 1) + Z_aux));
    for (size_t i = 0; i <= L; i++) {
        for (size_t j = 0; j < Z_value; j++) {
            ele = Value_Tree[i][path[i]][j].first;
            write_data(tmp_ele_array, i * Z_value + j, element_size, ele);
        }
    }
    for (size_t j = 0; j < Z_aux; j++) {
        ele = Value_Aux[path[L]][j].first;
        write_data(tmp_ele_array, (L + 1) * Z_value + j, element_size, ele);
    }

    size_t dpf_key_size = getDPF_keySize(Z_value * (L + 1) + Z_aux);
    std::vector<uint8_t> dpf_key(dpf_key_size);
    boost::asio::read(socket_, boost::asio::buffer(dpf_key.data(), dpf_key_size));
    size_t log_tab_length = std::ceil(log2(Z_value * (L + 1) + Z_aux));
    if(log_tab_length > 10) {aaaa = DPF::EvalFull8(dpf_key, log_tab_length);}    
    else {aaaa = DPF::EvalFull(dpf_key, log_tab_length);}
    ele = pir_read(tmp_ele_array, aaaa, Z_value * (L + 1) + Z_aux, element_size);
    boost::asio::write(socket_, boost::asio::buffer(ele.data(), element_size));

    for (size_t i = 0; i <= L; i++) {
        for (size_t j = 0; j < Z_value; j++) {
            boost::asio::read(socket_, boost::asio::buffer(&tmp_addr, sizeof(size_t)));
            changeFirstSizeT(Value_Tree[i][path[i]][j].first, tmp_addr);
        }
    }
    for (size_t j = 0; j < Z_aux; j++) {
        boost::asio::read(socket_, boost::asio::buffer(&tmp_addr, sizeof(size_t)));
        changeFirstSizeT(Value_Aux[path[L]][j].first, tmp_addr);
    }

    access_counter++;
    boost::asio::read(socket_, boost::asio::buffer(ele.data(), element_size));
    Value_Tree[0][0][access_counter % (Z_value >> 1)].first = ele;
    
    if (access_counter % (Z_value >> 1) == 0) {
        evict_value(evict_counter);
        evict_counter++;
    }
}


