
#ifndef __VECTOR_H_
#define __VECTOR_H_
#include <stddef.h>

struct vector;

typedef struct vector * Vector;

/**
 * @brief allocates and returns a new Vector data structure.
 * @param ctor pointer to user constructor function.
 * @param dtor pointer to user destructor function.
 * @return Vector the vector.
 */
Vector new_vector(void * (*ctor)(const void *copy),void (*dtor)(void *item));


/**
 * @brief inserts a new item to the vector.
 * @param v reference to the vector
 * @param copy_item a reference to the item you wish to push in the vector.
 * @return void* returns pointer to inserted element, or returns NULL pointer if allocation was not sucessfull.
 */
void * vector_insert(Vector v,const void * copy_item);


/**
 * @brief returns a pointer to the first pointer of the vector item list.
 * @param v reference to the vector
 * @return void* const* 
 */
void *const *vector_begin(const Vector v);

/**
 * @brief returns a pointer to the last pointer of the vector item list.
 * @param v reference to the vector
 * @return void* const* 
 */
void *const *vector_end(const Vector v);

/**
 * @brief returns how many items are actually inside the vector
 * @param v reference to the vector
 * @return size_t 
 */
size_t vector_get_item_count(const Vector v);

/**
 * @brief completely deallocates the vector, by calling the dtor function on each item + free'ing the memory of the vector
 * @param v reference to the vector
 */
void vector_destroy(Vector * v);


/**
 * @brief macro for iterating a vector.
 */
#define VECTOR_FOR_EACH(begin,end,v) for(begin   = vector_begin(v),end= vector_end(v);begin <= end;begin++)

/**
 * @brief macro for reverse iterating a vector.
 */
#define VECTOR_FOR_EACH_R(begin,end,v) for(begin   = vector_begin(v),end= vector_end(v);begin <= end;end--)

#endif
