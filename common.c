
#include "common.h"
#include <string.h>
#include <stdlib.h>

const char * symbol_type_strings[6] = {
    "external symbol",
    "entry symbol",
    "code symbol",
    "data symbol",
     "entry code symbol",
    "entry data symbol"
};
static void * create_extern_call(const void * ptr) {
    return memcpy(malloc(sizeof(struct extern_call)),ptr,sizeof(struct extern_call));
}
/**
@brief-this function is to release allocated for objects of type 'extern_calls' 
@param- 'ptr' - pointer to an already created object in order to delete its allocated memory
*/
static void destroy_extern_call(void * ptr) {
    struct extern_call * external_call = ptr;
    vector_destroy(&external_call->call_addresses);
    free(external_call);
}
/**
@brief- this function creates a copy of memory word
@param - 'copy' pointer to the created copied word
@return - returns a pointer to the copied memory word.
*/
void * create_memory_word(const void * copy) {
    return memcpy(malloc(sizeof(unsigned int)),copy,sizeof(unsigned int));
}
/**
@brief - this function is to free memory which was allocated for the memory word pointed by 'obj'
@param - 'obj' - used to point to the allocated memory
*/
void destroy_memory_word(void * obj) {
    free(obj);
}
/**
@brief - this function creates objects of ty[e 'struct symbol' by  allocating memory
@param - 'ptr' - pointer to an already created object in order to create new one
@return - returns a pointer to the new allocated memory of the new object of 'struct symbol'
*/
static void * create_symbol(const void * ptr) {
    return memcpy(malloc(sizeof(struct symbol)),ptr,sizeof(struct symbol));
}

/**
@brief - this function is to release allocated for objects of type 'struct symbol' 
@param - 'ptr' - pointer to an already created object in order to delete its allocated memory
*/
static void destroy_symbol(void * ptr) {
    free(ptr);
}

void mmn14_assembler_add_extern(Vector extern_calls, const char * extern_name, const unsigned int call_addr) {
    void * const * begin_ex;
    void * const * end_ex;
    struct extern_call new_extern = {0};
    VECTOR_FOR_EACH(begin_ex, end_ex, extern_calls) {
        if(*begin_ex) {/*check if the current input matches the the extern call*/
            if(strcmp(extern_name,((const struct extern_call*)(*begin_ex))->extern_name) == 0) {
                vector_insert(((const struct extern_call*)(*begin_ex))->call_addresses,&call_addr);/*Add the call_addr to the existing extern_call entry*/
                return;
            }
        }
    }
    strcpy(new_extern.extern_name,extern_name);
    new_extern.call_addresses = new_vector(create_memory_word, destroy_memory_word);
    vector_insert(new_extern.call_addresses,&call_addr);
    vector_insert(extern_calls,&new_extern);
}


struct object_file mmn14_assembler_create_new_obj() {
    struct object_file ret = {0};
    ret.code_section = new_vector(create_memory_word,destroy_memory_word);
    ret.data_section = new_vector(create_memory_word,destroy_memory_word);
    ret.symbol_table = new_vector(create_symbol, destroy_symbol);
    ret.extern_calls = new_vector(create_extern_call,destroy_extern_call);
    ret.symbol_table_lookup = trie();
    return ret;
}


void mmn14_assembler_destroy_obj(struct object_file * obj) {
    vector_destroy(&obj->code_section);
    vector_destroy(&obj->data_section);
    vector_destroy(&obj->symbol_table);
    vector_destroy(&obj->extern_calls);
    trie_destroy(&obj->symbol_table_lookup);


}
