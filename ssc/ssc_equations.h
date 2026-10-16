/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __ssc_eqn_h
#define __ssc_eqn_h

#include "sscapi.h"

/**
 *  Returns true if completed successfully. For failures, query the "error" string that has been assigned to the `data`.
 *  For non-fatal issues that result in successful completion, a `warning` string will be provided.
 */
typedef bool (*ssc_equation_ptr)(ssc_data_t data);

/**
 * name:
 *      If the equation only affects variables from a single SSC Group, then you can name it <Group>_<fx>
 *          It'll show up in PySAM as <CMOD>.<Group>.<fx>
 *      Otherwise, you can name it <fx> for it to show up as <CMOD>.<fx>
 * cmod:
 *      Which compute module the equation should be associated with. At the moment, if you want to associate it with
 *      multiple compute modules, a new entry for each is required.
 * auto_eval:
 *      True means the equation is run in core.cpp before exec()
 * PySAM_export:
 *      True to export to PySAM (only)
 */

struct ssc_equation_entry{
    const char* name;
    ssc_equation_ptr func;
    const char* cmod;
    const char* doc;
    bool auto_eval;
    bool PySAM_export;
};

/**
 * Table of ssc_equations for SDK access
 *
 * #TODO: separate this logically if it gets big enough
 */

static ssc_equation_entry ssc_equation_table [] = {
        // NOTE: EnergyPlus: no compute module that has equations is built (only pvwattsv5_1ts), so the table is empty and no cmod_*_eqns.h is needed
        {nullptr, nullptr, nullptr, nullptr, false, false}
};


#endif
