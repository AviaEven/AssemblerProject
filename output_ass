
#include "output_unit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#define EXT_EXTENSION ".ext"
#define ENT_EXTENSION ".ent"
#define OB_EXTENSION ".ob"
#define BASE64 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
/**
@brief - this function creates an output file 
@param- 'ent_file_name' - char pointer , it represents the name of the file
	' symbol_table - from type Vector, this parameter represents  the symbol table
*/
static void create_output_ent_file(const char * ent_file_name, Vector symbol_table) {
    FILE * ent_file;
    void *const * begin;/*used to iterate in the symbol table*/
    void *const * end;/*used to iterate in the symbol table*/
    ent_file = fopen(ent_file_name,"w");
    if(ent_file_name) {
        VECTOR_FOR_EACH(begin,end, symbol_table) {
            if(*begin) {
                if (((struct symbol*)(*begin))->sym_type >= sym_entry_code) {
                    fprintf(ent_file,"%s\t%u\n",((struct symbol*)(*begin))->sym_name,((struct symbol*)(*begin))->address);
                }
            }
    }
    fclose(ent_file);
    }
}
/**
@brief - this function gets info about external calls and their addresses and writes info to the output file
@param - 'ext_file_name' - char pointer which represents the name of the create file
	 'extern_calls' - this parameter is an array for external call data 
*/
static void create_output_ext_file(const char * ext_file_name, Vector extern_calls) {
    FILE * ext_file;
/*these variables, are uses for iterations in the data structure*/
    void *const * bgn_ext;
    void *const * end_ext;
    void *const * address_begin;
    void *const * address_end;
    ext_file = fopen(ext_file_name,"w");
    if(ext_file) {
        VECTOR_FOR_EACH(bgn_ext,end_ext, extern_calls) {
            if(*bgn_ext) {
                VECTOR_FOR_EACH(address_begin,address_end, ((const struct extern_call * )(*bgn_ext))->call_addresses) {
                    if(*address_begin) {
                        fprintf(ext_file,"%s\t%u\n",((const struct extern_call * )(*bgn_ext))->extern_name,*((unsigned int*)(*address_begin)));
                    }
                }
            }
        }
    }
    fclose(ext_file);
}
/**
@brief - this function encodes the data using base 64 encoding and writes the encoded characters to the obj_file
@param - 'obj_file' - FILE pointer,to the output file where the data will be
	 'memory_section' - from type 'Vector', which represents  the generic array containing memory
*/
static void create_output_memory_section(FILE * obj_file,const Vector memory_section) {
    void *const * begin;
    void *const * end;
    const char * const  b64_chars = BASE64;
    unsigned int msb;
    unsigned int lsb;
    VECTOR_FOR_EACH(begin,end, memory_section) {
        if(*begin) { 
            msb = (*(unsigned int *)(*begin) >> 6) & 0x3F;/*getting the most significant 6 bits and by shifting right, we keep the 6 lower bits*/
            lsb = (*(unsigned int *)(*begin) & 0x3F);/*getting the least significant 6 bits*/
            fprintf(obj_file,"%c%c",b64_chars[msb],b64_chars[lsb]);
            fprintf(obj_file,"\n");
        }
    }
}


void mmn14_output(char * base_name, const struct object_file *obj,const char *out_dir) {
    char *ext_file_name;
    char *ent_file_name;
    char *ob_file_name;
    FILE * ob_file;
    size_t str_len;
    base_name = out_dir != NULL ?  basename(base_name) : base_name;/*allocate memory and copy the result*/
    base_name = strcat(strcpy(malloc(strlen(out_dir) + strlen(base_name) + 1),out_dir),base_name);
    str_len = strlen(base_name);

    if(obj->entries_count >=1) {
        ent_file_name = strcat(strcpy(malloc(str_len + strlen(ENT_EXTENSION) +1 ),base_name),ENT_EXTENSION);/*create entry file*/
        create_output_ent_file(ent_file_name,obj->symbol_table);
        free(ent_file_name);
    }

    if(vector_get_item_count(obj->extern_calls) >= 1) {
        ext_file_name = strcat(strcpy(malloc(str_len + strlen(EXT_EXTENSION) +1 ),base_name),EXT_EXTENSION);/*create external file*/
        create_output_ext_file(ext_file_name,obj->extern_calls);
        free(ext_file_name);
    }

    ob_file_name = strcat(strcpy(malloc(str_len + strlen(OB_EXTENSION) +1) ,base_name),OB_EXTENSION);/*create obj file*/
    ob_file = fopen(ob_file_name,"w");
    if(ob_file) {/*write item count and memory sections to the object file*/
        fprintf(ob_file,"%lu %lu\n",(unsigned long)vector_get_item_count(obj->code_section),(unsigned long)vector_get_item_count(obj->data_section));
        create_output_memory_section(ob_file,obj->code_section);
        create_output_memory_section(ob_file,obj->data_section);
        fclose(ob_file);
    }
    
    free(ob_file_name);
    if(out_dir)
        free(base_name);
}

