
#ifndef _COMMON_H_
#define _COMMON_H_
#include "vector.h"
#include "trie.h"
struct symbol {
    enum {
        sym_extern,
        sym_entry,
        sym_code,
        sym_data,
        sym_entry_code,
        sym_entry_data
    }sym_type;
    unsigned int address;
    unsigned int declared_line;
    char sym_name[31 +1];
};
extern const char * symbol_type_strings[6];
/**
@brief-this struct describes an object representation of compiled asm language
@param-code_section - an array of 12 binary codes, representing the code region of the program
@param-data_section - an array of 12 binary codes, representing the data region of the program
@param-symbol_table - an array of symbols which is defined above ^, this is the 'symbol table'
@param-extern_calls - an array of mapping which extern symbol is being called at what address
@param-symbol_table_lookup - an trie ( prefix tree) for searching efficiently symbols in the symbol table
@param-entries_count - amount of symbols that are entries in the symbol table.( entry code or entry data)
*/
struct object_file {
    Vector code_section;
    Vector data_section;
    Vector symbol_table;
    Vector extern_calls;
    Trie   symbol_table_lookup;
    int entries_count;
};


struct extern_call {
    char  extern_name[31 + 1];
    Vector call_addresses; 
};


struct object_file mmn14_assembler_create_new_obj();

/**
@brief - this function releases all allocated memory
@param obj - pointer to 'struct object_file' type
 */
void mmn14_assembler_destroy_obj(struct object_file * obj);

/**
 * @brief -this function is manageextern calls during assembly process
 * @param -extern_calls - represents the vector that holds info about extern calls
 * @param -extern_name - represents the name of the external symbol for which an external call address is being added
 * @param -call_addr - represents the address where the external symbol is being called from 
 */
void mmn14_assembler_add_extern(Vector extern_calls, const char * extern_name, const unsigned int call_addr);

#endif
