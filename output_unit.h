
#ifndef __OUTPUT_UNIT_H_
#define __OUTPUT_UNIT_H_
#include "../common/mmn14_common.h"

/**
 * @brief this is the output unit.
 * @param base_name the file name and directive
 * @param obj the object file
 * @param out_dir out dir name 
 */
void mmn14_output(char * base_name, const struct object_file *obj,const char *out_dir);




#endif
