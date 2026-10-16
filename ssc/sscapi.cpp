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


#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>

#include "lib_util.h"
#include "core.h"
#include "sscapi.h"


#pragma warning (disable : 4706 )

SSCEXPORT int ssc_version()
{
	return 308;
}

SSCEXPORT const char *ssc_build_info()
{
	static const char *_bi = __PLATFORM__ " " __ARCH__ " " __COMPILER__ " " __DATE__ " " __TIME__;
	return _bi;
}

/* to add new computation modules,
	specify an extern module entry,
	and add it to 'module_table'
*/

// NOTE: EnergyPlus: only the pvwattsv5_1ts compute module is built (used by PVWatts.cc), the other modules are not referenced so they are not linked
extern module_entry_info
/* extern declarations of modules for linking */
	cm_entry_pvwattsv5_1ts
	;

/* official module table */
static module_entry_info *module_table[] = {
	&cm_entry_pvwattsv5_1ts,
0 };

extern var_info vtab_oandm[];
extern var_info vtab_hybrid_tech_inputs[];
extern var_info vtab_oandm_hybrid[];


SSCEXPORT ssc_module_t ssc_module_create( const char *name )
{
	std::string lname = util::lower_case( name );

	int i=0;
	while ( module_table[i] != 0
		 && module_table[i]->f_create != 0 )
	{
		if ( lname == util::lower_case( module_table[i]->name ) )
			return (*(module_table[i]->f_create))();
		i++;
	}

	return 0;
}

SSCEXPORT void ssc_module_free( ssc_module_t p_mod )
{
	compute_module *cm = static_cast<compute_module*>(p_mod);
	if (cm) delete cm; // calls destructors for compute_module and tcskernel if a ssc-tcs technology
}

/*************************** var object manipulation ***************************/


SSCEXPORT ssc_var_t ssc_var_create(){
    return static_cast<ssc_var_t >( new var_data );
}

SSCEXPORT void ssc_var_free( ssc_var_t p_var )
{
    auto vd = static_cast<var_data*>(p_var);
    delete vd;
}

SSCEXPORT void ssc_var_clear( ssc_var_t p_var )
{
    auto vd = static_cast<var_data*>(p_var);
    if (vd) vd->clear();
}

SSCEXPORT int ssc_var_query(ssc_var_t p_var){
    auto vt = static_cast<var_data*>(p_var);
    if(!vt) return -1;
    return vt->type;
}

SSCEXPORT void ssc_var_size(ssc_var_t p_var, int* nrows, int* ncols){
    auto vt = static_cast<var_data*>(p_var);
    if(!vt) return;
    switch(vt->type){
        default:
        case SSC_INVALID:
            if (nrows) *nrows = 0;
            if (ncols) *ncols = 0;
            return;
        case SSC_ARRAY:
            if (nrows) *nrows = (int)vt->num.length();
            if (ncols) *ncols = 1;
            return;
        case SSC_TABLE:
            if (nrows) *nrows = (int)vt->table.size();
            if (ncols) *ncols = 1;
            return;
        case SSC_NUMBER:
        case SSC_STRING:
            if (nrows) *nrows = 1;
            if (ncols) *ncols = 1;
            return;
        case SSC_MATRIX:
            if (nrows) *nrows = (int)vt->num.nrows();
            if (ncols) *ncols = (int)vt->num.ncols();
            return;
        case SSC_DATARR:
            if (nrows) *nrows = (int)vt->vec.size();
            if (ncols) *ncols = 1;
            return;
        case SSC_DATMAT:
            if (nrows) *nrows = (int)vt->mat.size();
            if (ncols) *ncols = (int)vt->mat[0].size();
            return;
    }
}

SSCEXPORT void ssc_var_set_string( ssc_var_t p_var, const char *value )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->clear();
    vt->type = SSC_STRING;
    vt->str = value;
}

SSCEXPORT void ssc_var_set_number( ssc_var_t p_var, ssc_number_t value )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->clear();
    vt->type = SSC_NUMBER;
    vt->num = value;
}

SSCEXPORT void ssc_var_set_array( ssc_var_t p_var, ssc_number_t *pvalues, int length )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->clear();
    vt->type = SSC_ARRAY;
    vt->num.assign(pvalues, length);
}

SSCEXPORT void ssc_var_set_matrix( ssc_var_t p_var, ssc_number_t *pvalues, int nrows, int ncols )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->clear();
    vt->type = SSC_MATRIX;
    vt->num.assign(pvalues, nrows, ncols);
}

SSCEXPORT void ssc_var_set_table( ssc_var_t p_var, ssc_data_t table )
{
    auto vt = static_cast<var_data*>(p_var);
    auto value = static_cast<var_table*>(table);
    if (!vt || !value) return;
    vt->clear();
    vt->type = SSC_TABLE;
    vt->table = *value;
}

SSCEXPORT void ssc_var_set_data_array(ssc_var_t p_var, ssc_var_t p_var_entry, int r ){
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->type = SSC_DATARR;
    auto& vec = vt->vec;
    if (r >= (int)vec.size())
        vec.resize(r + 1);
    vec[r] = *static_cast<var_data*>(p_var_entry);
}

SSCEXPORT void ssc_var_set_data_matrix(ssc_var_t p_var, ssc_var_t p_var_entry, int r, int c ){
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return;
    vt->type = SSC_DATMAT;
    auto& mat = vt->mat;
    if (r >= (int)mat.size())
        mat.resize(r + 1);
    for (auto& i : mat)
        if (c >= (int)i.size())
            i.resize(c + 1);
    mat[r][c] = *static_cast<var_data*>(p_var_entry);
}

SSCEXPORT const char *ssc_var_get_string( ssc_var_t p_var )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt || vt->type != SSC_STRING) return 0;
    return vt->str.c_str();
}

SSCEXPORT ssc_number_t ssc_var_get_number( ssc_var_t p_var )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt || vt->type != SSC_NUMBER) return 0;
    return vt->num[0];
}

SSCEXPORT ssc_number_t *ssc_var_get_array(ssc_var_t p_var,  int *length )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt || vt->type != SSC_ARRAY) return 0;
    if (length) *length = (int) vt->num.length();
    return vt->num.data();
}

SSCEXPORT ssc_number_t *ssc_var_get_matrix( ssc_var_t p_var, int *nrows, int *ncols )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt || vt->type != SSC_MATRIX) return 0;
    if (nrows) *nrows = (int) vt->num.nrows();
    if (ncols) *ncols = (int) vt->num.ncols();
    return vt->num.data();
}

SSCEXPORT ssc_data_t ssc_var_get_table( ssc_var_t p_var )
{
    auto vt = static_cast<var_data*>(p_var);
    if (!vt || vt->type != SSC_TABLE) return 0;
    return static_cast<ssc_data_t>( &(vt->table) );
}

SSCEXPORT ssc_var_t ssc_var_get_var_array(ssc_var_t p_var, int r) {
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return 0;
    if (r < (int)vt->vec.size())
        return &vt->vec[r];
    else
        return nullptr;
}

SSCEXPORT ssc_var_t ssc_var_get_var_matrix(ssc_var_t p_var, int r, int c) {
    auto vt = static_cast<var_data*>(p_var);
    if (!vt) return 0;
    if (r < (int)vt->mat.size() && c < (int)vt->mat[r].size())
        return &vt->mat[r][c];
    else
        return nullptr;
}


/*************************** data object manipulation ***************************/

SSCEXPORT ssc_data_t ssc_data_create()
{
	return static_cast<ssc_data_t>( new var_table );
}

SSCEXPORT void ssc_data_free( ssc_data_t p_data )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (vt) delete vt;
}

SSCEXPORT void ssc_data_clear( ssc_data_t p_data )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (vt) vt->clear();
}

SSCEXPORT void ssc_data_unassign( ssc_data_t p_data, const char *name )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return;
	vt->unassign( name );
}


SSCEXPORT int ssc_data_rename( ssc_data_t p_data, const char *oldname, const char *newname )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;

	return vt->rename( oldname, newname ) ? 1 : 0;
}

SSCEXPORT int ssc_data_query( ssc_data_t p_data, const char *name )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return SSC_INVALID;
	var_data *dat = vt->lookup(name);
	if (!dat) return SSC_INVALID;
	else return dat->type;
}

SSCEXPORT const char *ssc_data_first( ssc_data_t p_data ) // returns the name of the first data item, 0 if empty
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	return vt->first();
}

SSCEXPORT const char *ssc_data_next( ssc_data_t p_data ) // returns the next name in the data set object, 0, if none left.
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	return vt->next();
}

SSCEXPORT ssc_var_t ssc_data_lookup(ssc_data_t p_data, const char *name)
{
    var_table *vt = static_cast<var_table*>(p_data);
    if (!vt) return nullptr;
    return vt->lookup(name);
}

SSCEXPORT ssc_var_t ssc_data_lookup_case(ssc_data_t p_data, const char *name)
{
    var_table *vt = static_cast<var_table*>(p_data);
    if (!vt) return nullptr;
    return vt->lookup_match_case(name);
}

SSCEXPORT void ssc_data_set_var(ssc_data_t p_data, const char* name, ssc_var_t p_var)
{
    auto vt = static_cast<var_table*>(p_data);
    if (!vt) return;
    auto vd = static_cast<var_data*>(p_var);
    if (!p_var) return;
    vt->assign(name, *vd);
}

SSCEXPORT void ssc_data_set_var_match_case(ssc_data_t p_data, const char* name, ssc_var_t p_var)
{
    auto vt = static_cast<var_table*>(p_data);
    if (!vt) return;
    auto vd = static_cast<var_data*>(p_var);
    if (!p_var) return;
    vt->assign_match_case(name, *vd);
}

SSCEXPORT void ssc_data_set_string( ssc_data_t p_data, const char *name, const char *value )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return;
	vt->assign( name, var_data( std::string(value) ) );
}

SSCEXPORT void ssc_data_set_number( ssc_data_t p_data, const char *name, ssc_number_t value )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return;
	vt->assign( name, var_data( value ) );
}

SSCEXPORT void ssc_data_set_array( ssc_data_t p_data, const char *name, ssc_number_t *pvalues, int length )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return;
	vt->assign( name, var_data( pvalues, length ) );
}

SSCEXPORT void ssc_data_set_matrix( ssc_data_t p_data, const char *name, ssc_number_t *pvalues, int nrows, int ncols )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return;
	vt->assign( name, var_data(pvalues, nrows, ncols) );
}

SSCEXPORT void ssc_data_set_table( ssc_data_t p_data, const char *name, ssc_data_t table )
{
	var_table *vt = static_cast<var_table*>(p_data);
	var_table *value = static_cast<var_table*>(table);
	if (!vt || !value) return;
	var_data *dat = vt->assign( name, var_data() );
	dat->type = SSC_TABLE;
	dat->table = *value;  // invokes operator= for deep copy
}

SSCEXPORT void ssc_data_set_data_array(ssc_data_t p_data, const char *name, ssc_var_t *data_array, int nrows ){
    auto vt = static_cast<var_table*>(p_data);
    if (!vt) return;
    std::vector<var_data> vec;
    for (int i = 0; i < nrows; i++){
        auto tab = static_cast<var_data*>(data_array[i]);
        vec.emplace_back(*tab);
    }
    vt->assign( name, var_data(vec));
}

SSCEXPORT void ssc_data_set_data_matrix(ssc_data_t p_data, const char *name, ssc_var_t *data_matrix, int nrows, int ncols ){
    auto  *vt = static_cast<var_table*>(p_data);
    if (!vt) return;
    std::vector<std::vector<var_data>> mat;
    for (int i = 0; i < nrows; i++){
        std::vector<var_data> row;
        for (int j = 0; j < ncols; j++){
            auto tab = static_cast<var_data*>(data_matrix[i * nrows + j]);
            row.emplace_back(*tab);
        }
        mat.emplace_back(row);
    }
    vt->assign( name, var_data(mat));
}

SSCEXPORT const char *ssc_data_get_string( ssc_data_t p_data, const char *name )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	var_data *dat = vt->lookup(name);
	if (!dat || dat->type != SSC_STRING) return 0;
	return dat->str.c_str();
}

SSCEXPORT ssc_bool_t ssc_data_get_number( ssc_data_t p_data, const char *name, ssc_number_t *value )
{
	if (!value) return 0;
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	var_data *dat = vt->lookup(name);
	if (!dat || dat->type != SSC_NUMBER) return 0;
	*value = dat->num;
	return 1;
}

SSCEXPORT ssc_number_t *ssc_data_get_array(ssc_data_t p_data,  const char *name, int *length )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	var_data *dat = vt->lookup(name);
	if (!dat || dat->type != SSC_ARRAY) return 0;
	if (length) *length = (int) dat->num.length();
	return dat->num.data();
}

SSCEXPORT ssc_number_t *ssc_data_get_matrix( ssc_data_t p_data, const char *name, int *nrows, int *ncols )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	var_data *dat = vt->lookup(name);
	if (!dat || dat->type != SSC_MATRIX) return 0;
	if (nrows) *nrows = (int) dat->num.nrows();
	if (ncols) *ncols = (int) dat->num.ncols();
	return dat->num.data();
}

SSCEXPORT ssc_data_t ssc_data_get_table( ssc_data_t p_data, const char *name )
{
	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt) return 0;
	var_data *dat = vt->lookup(name);
	if (!dat || dat->type != SSC_TABLE) return 0;
	return static_cast<ssc_data_t>( &(dat->table) );
}

SSCEXPORT ssc_var_t ssc_data_get_data_array(ssc_data_t p_data, const char *name, int *nrows) {
    auto vt = static_cast<var_table*>(p_data);
    if (!vt) return 0;
    var_data *dat = vt->lookup(name);
    if (!dat || dat->type != SSC_DATARR) return 0;
    if (nrows)
        *nrows = (int) dat->vec.size();
    else
        return nullptr;
    return dat;
}

SSCEXPORT ssc_var_t ssc_data_get_data_matrix(ssc_data_t p_data, const char *name, int *nrows, int *ncols ){
    auto vt = static_cast<var_table*>(p_data);
    if (!vt) return 0;
    var_data *dat = vt->lookup(name);
    if (!dat || dat->type != SSC_DATMAT) return 0;
    if (nrows) *nrows = (int) dat->mat.size();
    if (ncols){
        if (!dat->mat.empty())
            *ncols = (int) dat->mat[0].size();
        else
            *ncols = 0;
    }
    return dat;
}

SSCEXPORT ssc_bool_t ssc_data_deep_copy(ssc_data_t source, ssc_data_t dest) {
    auto source_vt = static_cast<var_table*>(source);
    if (!source_vt) return 0;
    auto dest_vt = static_cast<var_table*>(dest);
    if (!dest_vt) return 0;
	*dest_vt = *source_vt;  // invokes operator= for deep copy
    return 1;
}

// NOTE: EnergyPlus: json_to_ssc_data / json_file_to_ssc_data / ssc_data_to_json (and their helpers) removed. Not used by E+, they were the only reason rapidjson was needed


SSCEXPORT ssc_entry_t ssc_module_entry( int index )
{
	int max=0;
	while( module_table[max++] != 0 );

	if (index >= 0 && index < max) return static_cast<ssc_entry_t>(module_table[index]);
	else return 0;
}

SSCEXPORT const char *ssc_entry_name( ssc_entry_t p_entry )
{
	module_entry_info *p = static_cast<module_entry_info*>(p_entry);
	return p ? p->name : 0;
}

SSCEXPORT const char *ssc_entry_description( ssc_entry_t p_entry )
{
	module_entry_info *p = static_cast<module_entry_info*>(p_entry);
	return p ? p->description : 0;
}

SSCEXPORT int ssc_entry_version( ssc_entry_t p_entry )
{
	module_entry_info *p = static_cast<module_entry_info*>(p_entry);
	return p ? p->version : 0;
}


SSCEXPORT ssc_info_t ssc_module_var_info( ssc_module_t p_mod, int index )
{
	compute_module *cm = static_cast<compute_module*>(p_mod);
	if (!cm) return 0;
	return static_cast<ssc_info_t>( cm->info( index ) );
}

SSCEXPORT int ssc_info_var_type( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->var_type : SSC_INVALID;
}

SSCEXPORT int ssc_info_data_type( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->data_type : SSC_INVALID;
}

SSCEXPORT const char *ssc_info_name( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->name : 0;
}

SSCEXPORT const char *ssc_info_label( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->label : 0;
}

SSCEXPORT const char *ssc_info_units( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->units : 0;
}

SSCEXPORT const char *ssc_info_meta( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->meta : 0;
}

SSCEXPORT const char *ssc_info_required( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi? vi->required_if : 0;
}

SSCEXPORT const char *ssc_info_group( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->group : 0;
}

SSCEXPORT const char *ssc_info_constraints( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->constraints : 0;
}

SSCEXPORT const char *ssc_info_uihint( ssc_info_t p_inf )
{
	var_info *vi = static_cast<var_info*>(p_inf);
	return vi ? vi->ui_hint : 0;
}

/*
class default_sync_proc : public util::sync_piped_process
{
private:
	ssc_handler_t m_handler;
public:
	default_sync_proc( ssc_handler_t ph ) : m_handler(ph) {  }

	virtual void on_stdout(const std::string &line_text)
	{
		ssc_module_extproc_output( m_handler, line_text.c_str() );
	}
};
*/

static ssc_bool_t default_internal_handler_no_print( ssc_module_t /*p_mod*/, ssc_handler_t /*p_handler*/,
	int /*action_type*/, float /*f0*/, float /*f1*/,
	const char * /*s0*/, const char * /*s1*/,
	void * /*p_data*/ )
{
	// ignore all warnings and errors
	// don't print progress updates
	return 1;
}

static ssc_bool_t default_internal_handler( ssc_module_t /*p_mod*/, ssc_handler_t /*p_handler*/,
	int action_type, float f0, float f1,
	const char *s0, const char * /*s1*/,
	void * /*p_data*/ )
{
	if (action_type == SSC_LOG)
	{
		// print log message to console
		std::cout << "Log ";
		switch( (int)f0 )
		{
		case SSC_NOTICE: std::cout << "Notice: " << s0 << " time " << f1 << std::endl; break;
		case SSC_WARNING: std::cout << "Warning: " << s0 << " time " << f1 << std::endl; break;
		case SSC_ERROR: std::cout << "Error: " << s0 << " time " << f1 << std::endl; break;
		default: std::cout << "Log notice uninterpretable: " << f0 << " time " << f1 << std::endl; break;
		}
		return 1;
	}
	else if (action_type == SSC_UPDATE)
	{
		// print status update to console
		printf( "%5.2f %% %s @ %g\n", f0, s0, f1 );
		return 1; // return 0 to abort simulation as needed.
	}
	else
		return 0;
}

SSCEXPORT ssc_bool_t ssc_module_exec_simple( const char *name, ssc_data_t p_data )
{
	ssc_module_t p_mod = ssc_module_create( name );
	if ( !p_mod ) return 0;

	ssc_bool_t result = ssc_module_exec( p_mod, p_data );

	ssc_module_free( p_mod );
	return result;
}

SSCEXPORT const char *ssc_module_exec_simple_nothread( const char *name, ssc_data_t p_data )
{
static char p_internal_buf[256];

	ssc_module_t p_mod = ssc_module_create( name );
	if (!p_mod) return 0;

	ssc_bool_t result = ssc_module_exec( p_mod, p_data );

	// copy over first error if there was one to internal buffer
	if (!result)
	{
		strcpy(p_internal_buf, "general error detected");

		const char *text;
		int type;
		int i=0;
		while( (text = ssc_module_log( p_mod, i, &type, 0 )) )
		{
			if (type == SSC_ERROR)
			{
				strncpy( p_internal_buf, text, 255 );
				break;
			}
			i++;
		}
	}

	ssc_module_free( p_mod );
	return result ? 0 : p_internal_buf;
}

static int sg_defaultPrint = 1;

SSCEXPORT void ssc_module_exec_set_print( int print )
{
	sg_defaultPrint = print;
}

SSCEXPORT ssc_bool_t ssc_module_exec( ssc_module_t p_mod, ssc_data_t p_data )
{
	return ssc_module_exec_with_handler( p_mod, p_data, sg_defaultPrint ? default_internal_handler : default_internal_handler_no_print, 0 );
}

class default_exec_handler : public handler_interface
{
private:
	ssc_bool_t (*m_hfunc)( ssc_module_t, ssc_handler_t, int, float, float, const char *, const char *, void * );
	void *m_hdata;

public:
	default_exec_handler(
		compute_module *cm,
		ssc_bool_t (*f)( ssc_module_t, ssc_handler_t, int, float, float, const char *, const char *, void * ),
		void *d )
		: handler_interface(cm)
	{
		m_hfunc = f;
		m_hdata = d;
	}


	virtual void on_log( const std::string &text, int type, float time )
	{
		if (!m_hfunc) return;
		(*m_hfunc)( static_cast<ssc_module_t>( module() ),
					static_cast<ssc_handler_t>( static_cast<handler_interface*>(this) ),
					SSC_LOG, (float)type, time, text.c_str(), 0, m_hdata );
	}

	virtual bool on_update( const std::string &text, float percent, float time )
	{
		if (!m_hfunc) return true;

		return (*m_hfunc)( static_cast<ssc_module_t>( module() ),
					static_cast<ssc_handler_t>( static_cast<handler_interface*>(this) ),
					SSC_UPDATE, percent, time, text.c_str(), 0, m_hdata ) ? 1 : 0;
	}
};

SSCEXPORT ssc_bool_t ssc_module_exec_with_handler(
	ssc_module_t p_mod,
	ssc_data_t p_data,
	ssc_bool_t (*pf_handler)( ssc_module_t, ssc_handler_t, int, float, float, const char*, const char *, void * ),
	void *pf_user_data )
{
	compute_module *cm = static_cast<compute_module*>(p_mod);
	if (!cm) return 0;

	var_table *vt = static_cast<var_table*>(p_data);
	if (!vt)
	{
		cm->log("invalid data object provided", SSC_ERROR);
		return 0;
	}

	default_exec_handler h( cm, pf_handler, pf_user_data );
	return cm->compute( &h, vt ) ? 1 : 0;
}


SSCEXPORT void ssc_module_extproc_output( ssc_handler_t p_handler, const char *output_line )
{
	handler_interface *hi = static_cast<handler_interface*>( p_handler );
	if (hi)	hi->on_stdout( output_line );
}

SSCEXPORT ssc_bool_t ssc_module_add_var_info(ssc_module_t p_mod, ssc_info_t v)
{
    compute_module* cm = static_cast<compute_module*>(p_mod);
    if (!p_mod) return 0;

    var_info* vi = static_cast<var_info*>(v);
    if (!vi) return 0;
    cm->add_var_info(vi);

    return 1;
}

SSCEXPORT ssc_bool_t ssc_module_hybridize(ssc_module_t p_mod)
{
    compute_module* cmod = static_cast<compute_module*>(p_mod);
    if (!p_mod)
        return 0;

    if (vtab_oandm_hybrid[0].var_type == SSC_INVALID){
        memcpy(&vtab_oandm_hybrid, &vtab_oandm, 35 * sizeof(var_info));

        for (size_t i=0; i < 35; i++){
            if (vtab_oandm_hybrid[i].var_type != 0)
                vtab_oandm_hybrid[i].group = "HybridCosts";
        }
    }

    // copy only the subset for the technology
    std::string cmod_name = cmod->get_name();
    var_info* vtab_oandm_hybrid_tech[35];
    for (size_t i=0; i<35; i++)
        vtab_oandm_hybrid_tech[i] = nullptr;

    size_t copy_counter = 0;
    for (size_t i=0; i<35; i++){
        if (vtab_oandm_hybrid[i].var_type == SSC_INVALID)
            break;
        std::string name = std::string(vtab_oandm_hybrid[i].name);
        std::string meta = std::string(vtab_oandm_hybrid[i].meta);
        if (!meta.size()){
            // if no meta description on variable, apply it to all technologies
            if (!cmod->has_info(name)) {
                vtab_oandm_hybrid_tech[copy_counter] = &vtab_oandm_hybrid[i];
                copy_counter++;
            }
            continue;
        }

        size_t pos = 0;
        std::vector<std::string> tokens;
        while ((pos = meta.find(',')) != std::string::npos){
            std::string token = meta.substr(0, pos);
            tokens.push_back(token);
            meta.erase(0, pos + 1);
        }
        if (meta.size())
            tokens.push_back(meta);

        if (tokens[0][0] == '!'){
            // apply blocklist
            bool restricted = false;
            for (std::string token:tokens){
                if (cmod_name == token.substr(1)){
                    restricted = true;
                    break;
                }
            }
            if (!restricted) {
                if (!cmod->has_info(name)){
                    vtab_oandm_hybrid_tech[copy_counter] = &vtab_oandm_hybrid[i];
                    copy_counter++;
                }
                continue;
            }
        }
        else {
            // apply allowlist
            for (std::string token:tokens){
                if (cmod_name == token){
                    if (!cmod->has_info(name)){
                        vtab_oandm_hybrid_tech[copy_counter] = &vtab_oandm_hybrid[i];
                        copy_counter++;
                    }
                    break;
                }
            }
        }
    }    

    cmod->add_var_info(vtab_oandm_hybrid_tech);
    cmod->add_var_info(vtab_hybrid_tech_inputs);
    return 1;
}

SSCEXPORT const char *ssc_module_log( ssc_module_t p_mod, int index, int *item_type, float *time )
{
	compute_module *cm = static_cast<compute_module*>(p_mod);
	if (!p_mod) return 0;

	compute_module::log_item *l = cm->log(index);
	if (!l) return 0;

	if (item_type) *item_type = l->type;
	if (time) *time = l->time;

	return l->text.c_str();
}

SSCEXPORT void __ssc_segfault()
{
	std::string *pstr = 0;
	std::string mystr = *pstr;
}

//static std::string* s_python_path
static std::unique_ptr<std::string> s_python_path;

SSCEXPORT int set_python_path(const char* abs_path) {
    if (util::dir_exists(abs_path)){
//        delete s_python_path;
        s_python_path = std::unique_ptr<std::string>( new std::string(abs_path));
        return 1;
    }
    else
        return 0;
}

SSCEXPORT const char *get_python_path() {
    if (s_python_path)
        return s_python_path->c_str();
    else
        return nullptr;
}

SSCEXPORT int ssc_stateful_module_setup(ssc_module_t p_mod, ssc_data_t p_data) {
    auto cm = static_cast<compute_module*>(p_mod);
    if (!cm)
        return 0;

    auto vt = static_cast<var_table*>(p_data);
    if (!vt)
        cm->log("p_data invalid.");

    std::string lname = cm->get_name();
    int i = 0;
    while ( module_table[i] != nullptr && module_table[i]->f_create != nullptr ) {
        if ( lname == util::lower_case( module_table[i]->name ) ) {
            if (module_table[i]->f_setup_stateful) {
                try {
                    return (*(module_table[i]->f_setup_stateful))(cm, vt);
                }
                catch (general_error& e) {
                    cm->log(e.err_text, SSC_ERROR, e.time);
                    return 0;
                }
                catch (std::exception& e) {
                    cm->log("setup fail(" + cm->get_name() + "): " + e.what(), SSC_ERROR, -1);
                    return 0;
                }
            }
            else {
                cm->log("This module is not stateful. `setup` does not need to be called.");
                return 0;
            }
        }
        i++;
    }
    return 0;
}
