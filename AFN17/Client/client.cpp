#include "client.h"

const std::pair<std::string, int> Server0_IPAndPort = {"127.0.0.1", 4008};
const std::pair<std::string, int> Server1_IPAndPort = {"127.0.0.1", 4009};

Client::Client(size_t N, size_t element_size): 
    N(N), element_size(element_size),
    num_cell_per_element(static_cast<size_t>(element_size / sizeof(cell_type))),
    lg_N(static_cast<size_t>(log2(N))), Z_value(lg_N*lg_N*lg_N), Z_aux(lg_N*lg_N), Num_slice(lg_N), Slice_size(lg_N*lg_N),
    L(std::ceil(static_cast<double>(lg_N)/log2(Num_slice))), Leaf_num(static_cast<size_t>(pow(Num_slice, L))), Z_pos(4),
    access_counter(0), evict_counter(0),
    setup_bandwidth(0), access_bandwidth(0),
    server0_socket(io_context), server1_socket(io_context)
{
    A_pos.resize(4, MAX_SIZE_T);
    Pos_Stash.resize(lg_N - 2);
    connect_to_server(server0_socket, Server0_IPAndPort.first, Server0_IPAndPort.second);
    connect_to_server(server1_socket, Server1_IPAndPort.first, Server1_IPAndPort.second);
    set_socket_timeout(server0_socket, 6000);  // 6000 seconds (10 minutes)
    set_socket_timeout(server1_socket, 6000);
}

void Client::connect_to_server(boost::asio::ip::tcp::socket& socket, const std::string& server_ip, int server_port) {
    try {
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(server_ip, std::to_string(server_port));
        boost::asio::connect(socket, endpoints);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

std::pair<size_t, size_t> Client::get_evict_pos(size_t vir_addr) {
    assert(lg_N > 2);
    std::vector<size_t> tmp_index_list(lg_N - 1); // [1, 4] -> [1, 8] -> ...
    tmp_index_list[lg_N - 2] = std::ceil(static_cast<double>(vir_addr)); //  / 2
    size_t old_pos, new_pos;
    for (size_t i = 0; i < lg_N - 2; i++) {
        tmp_index_list[lg_N - 3 - i] = std::ceil(static_cast<double>(tmp_index_list[lg_N - 2 - i]) / 2);
    }
    old_pos = A_pos[tmp_index_list[0] - 1]; // next-accessed pos
    new_pos = get_random_position(2); // next-new pos
    A_pos[tmp_index_list[0] - 1] = new_pos;
    for (size_t i = 0; i < lg_N - 2; i++) {
        boost::asio::write(server0_socket, boost::asio::buffer(&old_pos, sizeof(size_t))); // Send the leafPos of the first tree
        
        access_bandwidth += sizeof(size_t);

        std::vector<PosStruct> accPos_List;
        size_t tmp_reserved_pos = old_pos;
        /**
         * Access the pos
         */
        for (size_t j = 0; j < Pos_Stash[i].size(); j++) {
            PosStruct tmp_Poss = Pos_Stash[i][j];
            if (tmp_Poss.x == tmp_index_list[i]) {
                tmp_Poss.x_pos = new_pos;
                size_t case_identifier = ((tmp_index_list[i + 1] & 1) << 1) | (i == lg_N - 3);
                switch (case_identifier) {
                    case 3: 
                        old_pos = tmp_Poss.x_mul2ms1_pos;
                        new_pos = get_random_position(Leaf_num);
                        tmp_Poss.x_mul2ms1_pos = new_pos;
                        break;
                    case 2: 
                        old_pos = tmp_Poss.x_mul2ms1_pos;
                        new_pos = get_random_position(1 << (i + 2));
                        tmp_Poss.x_mul2ms1_pos = new_pos;
                        break;
                    case 1: 
                        old_pos = tmp_Poss.x_mul2_pos;
                        new_pos = get_random_position(Leaf_num);
                        tmp_Poss.x_mul2_pos = new_pos;
                        break;
                    case 0:
                        old_pos = tmp_Poss.x_mul2_pos;
                        new_pos = get_random_position(1 << (i + 2));
                        tmp_Poss.x_mul2_pos = new_pos;
                        break;
                } 
            }
            accPos_List.push_back(tmp_Poss);
        }
        for (size_t k = 0; k < (i + 2) * Z_pos; k++) {
            PosStruct tmp_Poss;
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x, sizeof(size_t)));
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_pos, sizeof(size_t))); 
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_mul2_pos, sizeof(size_t))); 
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_mul2ms1_pos, sizeof(size_t)));

            access_bandwidth += sizeof(size_t) << 2;

            if (tmp_Poss.x != MAX_SIZE_T) {
                if (tmp_Poss.x == tmp_index_list[i]) {
                    tmp_Poss.x_pos = new_pos;
                    size_t case_identifier = ((tmp_index_list[i + 1] & 1) << 1) | (i == lg_N - 3);
                    switch (case_identifier) {
                        case 3: 
                            old_pos = tmp_Poss.x_mul2ms1_pos;
                            new_pos = get_random_position(Leaf_num);
                            tmp_Poss.x_mul2ms1_pos = new_pos;
                            break;
                        case 2: 
                            old_pos = tmp_Poss.x_mul2ms1_pos;
                            new_pos = get_random_position(1 << (i + 2));
                            tmp_Poss.x_mul2ms1_pos = new_pos;
                            break;
                        case 1: 
                            old_pos = tmp_Poss.x_mul2_pos;
                            new_pos = get_random_position(Leaf_num);
                            tmp_Poss.x_mul2_pos = new_pos;
                            break;
                        case 0:
                            old_pos = tmp_Poss.x_mul2_pos;
                            new_pos = get_random_position(1 << (i + 2));
                            tmp_Poss.x_mul2_pos = new_pos;
                            break;
                    } 
                }
                accPos_List.push_back(tmp_Poss);
            }
        }

        /**
         * Rewrite the pos
         */
        std::vector<PosStruct> tmpStashP_List;
        std::vector<std::vector<PosStruct>> rewritePos_List(i + 2, std::vector<PosStruct>(Z_pos));
        for (size_t j = 0; j < accPos_List.size(); j++) {
            size_t rewrite_levInd = findSharedLevel(tmp_reserved_pos, accPos_List[j].x_pos, i + 2);
            size_t k;
            for (k = 0; k < Z_pos; k++) {
                if (rewritePos_List[rewrite_levInd][k].x == MAX_SIZE_T) {
                    rewritePos_List[rewrite_levInd][k] = accPos_List[j];
                    break;
                }
            }
            if (k == Z_pos) {
                tmpStashP_List.push_back(accPos_List[j]);
            }
        }

        /**
         * Update the Pos_Stash and rewrite the posList
         */
        Pos_Stash[i] = tmpStashP_List;
        for (size_t j = 0; j < i + 2; j++) {
            for (size_t k = 0; k < Z_pos; k++) {
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x, sizeof(size_t)));
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_pos, sizeof(size_t))); 
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_mul2_pos, sizeof(size_t))); 
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_mul2ms1_pos, sizeof(size_t)));
                
                access_bandwidth += sizeof(size_t) << 2;

            }
        }
    }

    std::pair<size_t, size_t> res_old_new_pos = {old_pos, new_pos};
    return res_old_new_pos;
}

// Evict
size_t Client::evict_pos(size_t vir_addr) {
    assert(lg_N > 2);
    std::vector<size_t> tmp_index_list(lg_N - 1); // [1, 4] -> [1, 8] -> ...
    tmp_index_list[lg_N - 2] = std::ceil(static_cast<double>(vir_addr)); //  / 2
    size_t old_pos, new_pos;
    for (size_t i = 0; i < lg_N - 2; i++) {
        tmp_index_list[lg_N - 3 - i] = std::ceil(static_cast<double>(tmp_index_list[lg_N - 2 - i]) / 2);
    }
    old_pos = A_pos[tmp_index_list[0] - 1]; // next-accessed pos
    if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(2);}
    new_pos = get_random_position(2); // next-new pos
    A_pos[tmp_index_list[0] - 1] = new_pos;
    for (size_t i = 0; i < lg_N - 2; i++) {
        boost::asio::write(server0_socket, boost::asio::buffer(&old_pos, sizeof(size_t))); // Send the leafPos of the first tree
        setup_bandwidth += sizeof(size_t);

        std::vector<PosStruct> accPos_List;
        size_t tmp_reserved_pos = old_pos;
        bool found_flag = false;
        /**
         * Access the pos
         */
        for (size_t j = 0; j < Pos_Stash[i].size(); j++) {
            PosStruct tmp_Poss = Pos_Stash[i][j];
            if (tmp_Poss.x == tmp_index_list[i]) {
                found_flag = true;
                tmp_Poss.x_pos = new_pos;
                size_t case_identifier = ((tmp_index_list[i + 1] & 1) << 1) | (i == lg_N - 3);
                switch (case_identifier) {
                    case 3: 
                        old_pos = tmp_Poss.x_mul2ms1_pos;
                        if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(Leaf_num);}
                        new_pos = get_random_position(Leaf_num);
                        tmp_Poss.x_mul2ms1_pos = new_pos;
                        break;
                    case 2: 
                        old_pos = tmp_Poss.x_mul2ms1_pos;
                        if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(1 << (i + 2));}
                        new_pos = get_random_position(1 << (i + 2));
                        tmp_Poss.x_mul2ms1_pos = new_pos;
                        break;
                    case 1: 
                        old_pos = tmp_Poss.x_mul2_pos;
                        if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(Leaf_num);}
                        new_pos = get_random_position(Leaf_num);
                        tmp_Poss.x_mul2_pos = new_pos;
                        break;
                    case 0:
                        old_pos = tmp_Poss.x_mul2_pos;
                        if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(1 << (i + 2));}
                        new_pos = get_random_position(1 << (i + 2));
                        tmp_Poss.x_mul2_pos = new_pos;
                        break;
                } 
            }
            accPos_List.push_back(tmp_Poss);
        }
        for (size_t k = 0; k < (i + 2) * Z_pos; k++) {
            PosStruct tmp_Poss;
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x, sizeof(size_t)));
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_pos, sizeof(size_t))); 
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_mul2_pos, sizeof(size_t))); 
            boost::asio::read(server0_socket, boost::asio::buffer(&tmp_Poss.x_mul2ms1_pos, sizeof(size_t)));
            setup_bandwidth += sizeof(size_t) << 2;

            if (tmp_Poss.x != MAX_SIZE_T) {
                if (tmp_Poss.x == tmp_index_list[i]) {
                    found_flag = true;
                    tmp_Poss.x_pos = new_pos;
                    size_t case_identifier = ((tmp_index_list[i + 1] & 1) << 1) | (i == lg_N - 3);
                    switch (case_identifier) {
                        case 3: 
                            old_pos = tmp_Poss.x_mul2ms1_pos;
                            if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(Leaf_num);}
                            new_pos = get_random_position(Leaf_num);
                            tmp_Poss.x_mul2ms1_pos = new_pos;
                            break;
                        case 2: 
                            old_pos = tmp_Poss.x_mul2ms1_pos;
                            if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(1 << (i + 2));}
                            new_pos = get_random_position(1 << (i + 2));
                            tmp_Poss.x_mul2ms1_pos = new_pos;
                            break;
                        case 1: 
                            old_pos = tmp_Poss.x_mul2_pos;
                            if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(Leaf_num);}
                            new_pos = get_random_position(Leaf_num);
                            tmp_Poss.x_mul2_pos = new_pos;
                            break;
                        case 0:
                            old_pos = tmp_Poss.x_mul2_pos;
                            if (old_pos == MAX_SIZE_T) {old_pos = get_random_position(1 << (i + 2));}
                            new_pos = get_random_position(1 << (i + 2));
                            tmp_Poss.x_mul2_pos = new_pos;
                            break;
                    } 
                }
                accPos_List.push_back(tmp_Poss);
            }
        }

        /**
         * Write into the pos tree
         */
        if (!found_flag) {
            PosStruct tmp_Poss;
            tmp_Poss.x = tmp_index_list[i];
            tmp_Poss.x_pos = new_pos;
            size_t case_identifier = ((tmp_index_list[i + 1] & 1) << 1) | (i == lg_N - 3);
            switch (case_identifier) {
                case 3: 
                    old_pos = get_random_position(Leaf_num);
                    new_pos = get_random_position(Leaf_num);
                    tmp_Poss.x_mul2ms1_pos = new_pos;
                    break;
                case 2: 
                    old_pos = get_random_position(1 << (i + 2));
                    new_pos = get_random_position(1 << (i + 2));
                    tmp_Poss.x_mul2ms1_pos = new_pos;
                    break;
                case 1: 
                    old_pos = get_random_position(Leaf_num);
                    new_pos = get_random_position(Leaf_num);
                    tmp_Poss.x_mul2_pos = new_pos;
                    break;
                case 0:
                    old_pos = get_random_position(1 << (i + 2));
                    new_pos = get_random_position(1 << (i + 2));
                    tmp_Poss.x_mul2_pos = new_pos;
                    break;
            }
            accPos_List.push_back(tmp_Poss);
        }

        /**
         * Rewrite the pos
         */
        std::vector<PosStruct> tmpStashP_List;
        std::vector<std::vector<PosStruct>> rewritePos_List(i + 2, std::vector<PosStruct>(Z_pos));
        for (size_t j = 0; j < accPos_List.size(); j++) {
            size_t rewrite_levInd = findSharedLevel(tmp_reserved_pos, accPos_List[j].x_pos, i + 2);
            size_t k;
            for (k = 0; k < Z_pos; k++) {
                if (rewritePos_List[rewrite_levInd][k].x == MAX_SIZE_T) {
                    rewritePos_List[rewrite_levInd][k] = accPos_List[j];
                    break;
                }
            }
            if (k == Z_pos) {
                tmpStashP_List.push_back(accPos_List[j]);
            }
        }

        /**
         * Update the Pos_Stash and rewrite the posList
         */
        Pos_Stash[i] = tmpStashP_List;
        for (size_t j = 0; j < i + 2; j++) {
            for (size_t k = 0; k < Z_pos; k++) {
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x, sizeof(size_t)));
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_pos, sizeof(size_t))); 
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_mul2_pos, sizeof(size_t))); 
                boost::asio::write(server0_socket, boost::asio::buffer(&rewritePos_List[j][k].x_mul2ms1_pos, sizeof(size_t)));
                setup_bandwidth += sizeof(size_t) << 2;
            }
        }
    }
    return new_pos;   
}

void Client::evict_value(size_t evict_counter) {
    element_type ele(num_cell_per_element);
    element_type tmp_ele_shared0(num_cell_per_element);
    element_type tmp_ele_shared1(num_cell_per_element);
    size_t tmp_counter = evict_counter % Leaf_num;
    size_t tmp_pos;
    std::vector<uint8_t> aaaa;
    // Which bucket, which slice
    std::vector<std::pair<size_t, size_t>> reverseOrder = toReverseLexicographicalOrder(tmp_counter, L, Num_slice);

    for (size_t i = 0; i < L; i++) {
        // Send pos to Client
        std::vector<size_t> pos_list(Z_value);
        for (size_t j = 0; j < Z_value; j++) {
            boost::asio::read(server0_socket, boost::asio::buffer(&pos_list[j], sizeof(size_t)));

            if (access_counter <= N) {setup_bandwidth += sizeof(size_t);}
            else {access_bandwidth += sizeof(size_t);}
        }
        for (size_t j = 0; j < Num_slice; j++) {
            size_t real_ele_num = 0;
            for (size_t k = 0; k < Z_value; k++) {
                if (pos_list[k] != MAX_SIZE_T && getNodeIdAtLevel(pos_list[k], i + 1, L, Num_slice) == reverseOrder[i].first * Num_slice + j) {
                    auto keys_posL = DPF::Gen(k, std::ceil(log2(Z_value)));
                    boost::asio::write(server0_socket, boost::asio::buffer(keys_posL.first.data(), keys_posL.first.size()));
                    boost::asio::write(server1_socket, boost::asio::buffer(keys_posL.second.data(), keys_posL.second.size()));
                    boost::asio::read(server0_socket, boost::asio::buffer(tmp_ele_shared0.data(), element_size));
                    boost::asio::read(server1_socket, boost::asio::buffer(tmp_ele_shared1.data(), element_size));
                    ele = convert_share_to_element(tmp_ele_shared0, tmp_ele_shared1);
                    boost::asio::write(server0_socket, boost::asio::buffer(ele.data(), element_size));
                    boost::asio::write(server0_socket, boost::asio::buffer(&pos_list[k], sizeof(size_t)));
                    boost::asio::write(server1_socket, boost::asio::buffer(ele.data(), element_size));
                    real_ele_num++;

                    if (access_counter <= N) {setup_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
                    else {access_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
                }
            }
            assert(real_ele_num <= Slice_size);
            while (real_ele_num < Slice_size) {
                auto keys_posL = DPF::Gen(0, std::ceil(log2(Z_value)));
                boost::asio::write(server0_socket, boost::asio::buffer((keys_posL.first.data()), keys_posL.first.size()));
                boost::asio::write(server1_socket, boost::asio::buffer((keys_posL.second.data()), keys_posL.second.size()));
                boost::asio::read(server0_socket, boost::asio::buffer(tmp_ele_shared0.data(), element_size));
                boost::asio::read(server1_socket, boost::asio::buffer(tmp_ele_shared1.data(), element_size));
                ele = get_zero_element(element_size);
                tmp_pos = MAX_SIZE_T;
                boost::asio::write(server0_socket, boost::asio::buffer(ele.data(), element_size));
                boost::asio::write(server0_socket, boost::asio::buffer(&tmp_pos, sizeof(size_t)));
                boost::asio::write(server1_socket, boost::asio::buffer(ele.data(), element_size));
                real_ele_num++;

                if (access_counter <= N) {setup_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
                else {access_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
            }
        }
    }

    // Final level
    std::vector<size_t> pos_list(Z_value + Z_aux);
    for (size_t j = 0; j < Z_value + Z_aux; j++) {
        boost::asio::read(server0_socket, boost::asio::buffer(&pos_list[j], sizeof(size_t)));
        
        if (access_counter <= N) {setup_bandwidth += sizeof(size_t);}
        else {access_bandwidth += sizeof(size_t);}
    }
    size_t real_ele_num = 0;

    for (size_t k = 0; k < Z_value + Z_aux; k++) {
        if (pos_list[k] != MAX_SIZE_T) {
            auto keys_posL = DPF::Gen(k, std::ceil(log2(Z_value + Z_aux)));
            boost::asio::write(server0_socket, boost::asio::buffer((keys_posL.first.data()), keys_posL.first.size()));
            boost::asio::write(server1_socket, boost::asio::buffer((keys_posL.second.data()), keys_posL.second.size()));
            boost::asio::read(server0_socket, boost::asio::buffer(tmp_ele_shared0.data(), element_size));
            boost::asio::read(server1_socket, boost::asio::buffer(tmp_ele_shared1.data(), element_size));
            ele = convert_share_to_element(tmp_ele_shared0, tmp_ele_shared1);
            boost::asio::write(server0_socket, boost::asio::buffer(ele.data(), element_size));
            boost::asio::write(server0_socket, boost::asio::buffer(&pos_list[k], sizeof(size_t)));
            boost::asio::write(server1_socket, boost::asio::buffer(ele.data(), element_size));
            real_ele_num++;
            

            if (access_counter <= N) {setup_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
            else {access_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
        }
    }
    assert(real_ele_num <= Z_aux);
    while (real_ele_num < Z_aux) {
        auto keys_posL = DPF::Gen(0, std::ceil(log2(Z_value + Z_aux)));
        boost::asio::write(server0_socket, boost::asio::buffer((keys_posL.first.data()), keys_posL.first.size()));
        boost::asio::write(server1_socket, boost::asio::buffer((keys_posL.second.data()), keys_posL.second.size()));
        boost::asio::read(server0_socket, boost::asio::buffer(tmp_ele_shared0.data(), element_size));
        boost::asio::read(server1_socket, boost::asio::buffer(tmp_ele_shared1.data(), element_size));
        ele = get_zero_element(element_size);
        tmp_pos = MAX_SIZE_T;
        boost::asio::write(server0_socket, boost::asio::buffer(ele.data(), element_size));
        boost::asio::write(server0_socket, boost::asio::buffer(&tmp_pos, sizeof(size_t)));
        boost::asio::write(server1_socket, boost::asio::buffer(ele.data(), element_size));
        real_ele_num++;
        

        if (access_counter <= N) {setup_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
        else {access_bandwidth += (keys_posL.first.size() << 1) + (element_size << 2) + sizeof(size_t);}
    }
}

void Client::Setup() {
    element_type ele(num_cell_per_element);
    size_t pos;
    /**
     * Setup: Insert random N elements to the server
     */
    for (size_t i = 0 ;i < N; i++) {
        access_counter++;
        ele = get_random_element(i + 1, element_size);
        pos = evict_pos(i + 1);
        boost::asio::write(server0_socket, boost::asio::buffer(ele.data(), element_size));
        boost::asio::write(server0_socket, boost::asio::buffer(&pos, sizeof(size_t)));
        boost::asio::write(server1_socket, boost::asio::buffer(ele.data(), element_size));

        setup_bandwidth += (element_size << 1) + sizeof(size_t);

        if (access_counter % (Z_value >> 1) == 0) {
            evict_value(evict_counter);
            evict_counter++;
        }
    }
}

element_type Client::Access(char op, addr_type req_addr, uint8_t* write_value) {
    // accessed element
    element_type accessed_element(num_cell_per_element);
    element_type written_element(num_cell_per_element);
    element_type tmp_ele_shared0(num_cell_per_element);
    element_type tmp_ele_shared1(num_cell_per_element);
    std::pair<size_t, size_t> old_new_pos = get_evict_pos(req_addr);
    boost::asio::write(server0_socket, boost::asio::buffer(&old_new_pos.first, sizeof(size_t)));
    boost::asio::write(server1_socket, boost::asio::buffer(&old_new_pos.first, sizeof(size_t)));

    access_bandwidth += sizeof(size_t) << 1;
    // old & new
    std::vector<size_t> addr_list(Z_value * (L + 1) + Z_aux);
    size_t req_loc = MAX_SIZE_T;
    for (size_t i = 0; i < Z_value * (L + 1) + Z_aux; i++) {
        boost::asio::read(server0_socket, boost::asio::buffer(&addr_list[i], sizeof(size_t)));

        access_bandwidth += sizeof(size_t);
        if (addr_list[i] == req_addr) {
            req_loc = i;
            addr_list[i] = 0;
        }
    }
    assert(req_loc != MAX_SIZE_T);
    auto keys_posL = DPF::Gen(req_loc, std::ceil(log2(Z_value * (L + 1) + Z_aux)));
    boost::asio::write(server0_socket, boost::asio::buffer((keys_posL.first.data()), keys_posL.first.size()));
    boost::asio::write(server1_socket, boost::asio::buffer((keys_posL.second.data()), keys_posL.second.size()));
    boost::asio::read(server0_socket, boost::asio::buffer(tmp_ele_shared0.data(), element_size));
    boost::asio::read(server1_socket, boost::asio::buffer(tmp_ele_shared1.data(), element_size));

    access_bandwidth += (keys_posL.first.size() << 1) + (element_size << 1);

    accessed_element = convert_share_to_element(tmp_ele_shared0, tmp_ele_shared1);
    written_element = accessed_element;
    for (size_t i = 0; i < Z_value * (L + 1) + Z_aux; i++) {
        boost::asio::write(server0_socket, boost::asio::buffer(&addr_list[i], sizeof(size_t)));
        boost::asio::write(server1_socket, boost::asio::buffer(&addr_list[i], sizeof(size_t)));
        access_bandwidth += sizeof(size_t) << 1;
    }

    if (op == 'w') {
        written_element = convert_addrVal_to_element(std::make_pair(req_addr, write_value), element_size);
    }

    boost::asio::write(server0_socket, boost::asio::buffer(written_element.data(), element_size));
    boost::asio::write(server0_socket, boost::asio::buffer(&old_new_pos.second, sizeof(size_t)));
    boost::asio::write(server1_socket, boost::asio::buffer(written_element.data(), element_size));

    access_bandwidth += sizeof(size_t) + (element_size << 1);

    access_counter++;
    if (access_counter % (Z_value >> 1) == 0) {
        evict_value(evict_counter);
        evict_counter++;
    }

    return accessed_element;
}