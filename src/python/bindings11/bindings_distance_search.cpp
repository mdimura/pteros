/*
 *
 *                This source code is part of
 *                    ******************
 *                    ***   Pteros   ***
 *                    ******************
 *                 molecular modeling library
 *
 * Copyright (c) 2009-2013, Semen Yesylevskyy
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Artistic License:
 *
 * Please note, that Artistic License is slightly more restrictive
 * then GPL license in terms of distributing the modified versions
 * of this software (they should be approved first).
 * Read http://www.opensource.org/licenses/artistic-license-2.0.php
 * for details. Such license fits scientific software better then
 * GPL because it prevents the distribution of bugged derivatives.
 *
*/

#include "pteros/core/distance_search.h"
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

namespace py = pybind11;
using namespace pteros;
using namespace std;
using namespace Eigen;
using namespace pybind11::literals;



void make_bindings_Distance_search(py::module& m){

    m.def("search_contacts",[](float d,
          const Selection& sel,
          bool absolute_index = false,
          bool periodic = false,
          bool do_dist_vec = false
          )
    {
        std::vector<float>* dist_vec_ptr = do_dist_vec ? new std::vector<float> : nullptr;
        std::vector<Vector2i>* pairs_ptr = new std::vector<Vector2i>;
        search_contacts(d,sel,*pairs_ptr,absolute_index,periodic,dist_vec_ptr);
        Map<Matrix<int,Dynamic,Dynamic,Eigen::RowMajor>> m((int*)pairs_ptr->data(),pairs_ptr->size(),2);

        if(do_dist_vec){
            Map<Matrix<float,Dynamic,Dynamic,Eigen::RowMajor>> v((float*)dist_vec_ptr->data(),dist_vec_ptr->size(),1);
            return py::make_tuple(m,v);
        } else {
            return py::make_tuple(m,py::none());
        }
    }, py::return_value_policy::take_ownership, "d"_a, "sel"_a, "abs_ind"_a=false, "periodic"_a=false,"do_distances"_a=false);


    m.def("search_contacts",[](float d,
          const Selection& sel1,
          const Selection& sel2,
          bool absolute_index = false,
          bool periodic = false,
          bool do_dist_vec = false
          )
    {
        std::vector<float>* dist_vec_ptr = do_dist_vec ? new std::vector<float> : nullptr;
        std::vector<Vector2i>* pairs_ptr = new std::vector<Vector2i>;
        search_contacts(d,sel1,sel2,*pairs_ptr,absolute_index,periodic,dist_vec_ptr);
        Map<Matrix<int,Dynamic,Dynamic,Eigen::RowMajor>> m((int*)pairs_ptr->data(),pairs_ptr->size(),2);

        if(do_dist_vec){
            Map<Matrix<float,Dynamic,Dynamic,Eigen::RowMajor>> v((float*)dist_vec_ptr->data(),dist_vec_ptr->size(),1);
            return py::make_tuple(m,v);
        } else {
            return py::make_tuple(m,py::none());
        }
    }, py::return_value_policy::take_ownership, "d"_a, "sel1"_a, "sel2"_a, "abs_ind"_a=false, "periodic"_a=false,"do_distances"_a=false);


    m.def("search_within",[](float d,
          const Selection& src,
          const Selection& target,
          bool include_self = true,
          bool periodic = false
          )
    {
        std::vector<int>* res_ptr = new std::vector<int>;
        search_within(d,src,target,*res_ptr,include_self,periodic);
        Map<Matrix<int,Dynamic,Dynamic,Eigen::RowMajor>> v((int*)res_ptr->data(),res_ptr->size(),1);
        return v;
    }, "d"_a, "src"_a, "target"_a, "include_self"_a=true, "periodic"_a=false);


    py::class_<Distance_search_within>(m, "Distance_search_within")
            .def(py::init<float,const Selection&,bool,bool>(), "d"_a,"src"_a,"abs_ind"_a=false,"periodic"_a=false)
            .def("setup",&Distance_search_within::setup, "d"_a,"src"_a,"abs_ind"_a=false,"periodic"_a=false)

            .def("search_within",[](Distance_search_within* obj, Vector3f_const_ref coord)
                {
                   std::vector<int>* res_ptr = new std::vector<int>;
                   obj->search_within(coord,*res_ptr);
                   Map<Matrix<int,Dynamic,Dynamic,Eigen::RowMajor>> v((int*)res_ptr->data(),res_ptr->size(),1);
                   return v;
                })

            .def("search_within",[](Distance_search_within* obj, const Selection& target, bool include_self)
                {
                    std::vector<int>* res_ptr = new std::vector<int>;
                    obj->search_within(target,*res_ptr,include_self);
                    Map<Matrix<int,Dynamic,Dynamic,Eigen::RowMajor>> v((int*)res_ptr->data(),res_ptr->size(),1);
                    return v;
                },"target"_a, "include_self"_a=true)
    ;
}
