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

#include "pteros/python/compiled_plugin.h"
#include <fstream>

using namespace std;
using namespace pteros;
using namespace Eigen;

class covar_matr: public pteros::Compiled_plugin_base {
public:
    covar_matr(pteros::Trajectory_processor* pr, pteros::Options_tree* opt): Compiled_plugin_base(pr,opt) {}

protected:

    void pre_process(){
        do_align = options->get_value<bool>("align",true);
        sel.modify(system, options->get_value<string>("selection") );

        system.frame_dup(0);

        N = sel.size();

        matr.resize(3*N,3*N); matr.fill(0.0);
        mean.resize(3,N); mean.fill(0.0);
    }

    bool process_frame(const Frame_info &info){
        // Align if requested
        if(do_align) sel.fit(0,1);

        int i,j;
        // Do accumulation
        for(i=0;i<N;++i){
            mean.col(i) += sel.XYZ(i);
            for(j=i;j<N;++j){
                matr.block<3,3>(3*i,3*j) += sel.XYZ(i) * sel.XYZ(j).transpose();
            }
        }

        return true;
    }

    void post_process(const Frame_info &info){
        string fname;
        ofstream f;

        float T = float(info.valid_frame+1);

        // Get average
        mean /= T;
        matr.triangularView<Eigen::Upper>() /= T;

        int i,j;
        // Now compute covariance
        for(i=0;i<N;++i){
            for(j=i;j<N;++j){
                matr.block<3,3>(3*i,3*j) -= mean.col(i)*mean.col(j).transpose();
            }
        }

        // Make simmetric
        matr.triangularView<Eigen::StrictlyLower>() = matr.triangularView<Eigen::StrictlyUpper>().transpose();

        bool write_whole = options->get_value<bool>("write_whole",false);
        if(write_whole){
            // Output whole matrix
            fname = label+"-whole.dat";
            f.open(fname.c_str());
            f << "# Covariance matrix (3N*3N)" << endl;
            f << matr << endl;
            f.close();
        }

        // Output matrix of diagonal elements only (cov(x)+cov(y)+cov(z))
        Eigen::MatrixXd m(N,N);
        for(i=0;i<N;++i){
            for(j=0;j<N;++j){
                m(i,j) = matr.block<3,3>(3*i,3*j).trace();
            }
        }

        fname = label+".dat";
        f.open(fname.c_str());
        f << "# Covariance matrix (N*N)" << endl;
        f << m << endl;
        f.close();
    }

private:
    bool do_align;
    int N;
    MatrixXf matr;
    MatrixXf mean;
    Selection sel;
};

CREATE_COMPILED_PLUGIN(covar_matr)
