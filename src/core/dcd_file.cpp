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

#include "dcd_file.h"
#include "molfile_plugin.h"

using namespace std;
using namespace pteros;
using namespace Eigen;

extern molfile_plugin_t dcd_plugin;

DCD_file::DCD_file(string fname, char open_mode): VMD_molfile_plugin_wrapper(fname,open_mode){
    plugin = &dcd_plugin;
    accepted_format = DCD_FILE;
    open(fname,open_mode);
}
