#ifndef GLOBAL_CPP
#define GLOBAL_CPP

#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>

using Node = sdsl::int_vector_size_type;
using CST = sdsl::cst_sada<sdsl::csa_wt<>,
    sdsl::lcp_support_sada<>,
    sdsl::bp_support_sada<>,
    sdsl::rank_support_v5<10,2>,
    sdsl::select_support_mcl<10,2>>;

#endif
