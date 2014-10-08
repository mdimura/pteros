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

#ifndef TNG_FILE_H
#define TNG_FILE_H

#include <string>
//#include "pteros/core/mol_file.h"
#include "vmd_molfile_plugin_wrapper.h"
#include "tng_io.h"

namespace pteros {

/// Use VMD plugin for TNG
class TNG_file: public VMD_molfile_plugin_wrapper {
public:
    TNG_file(std::string fname, char openmode);    

    virtual Mol_file_content get_content_type() const {
        Mol_file_content c;
        c.structure = true;
        c.trajectory = true;
        return c;
    }

};

}
#endif /* MOL_FILE_H */