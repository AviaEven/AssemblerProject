
#include "vector.h"
#include <stdlib.h>

#define VECTOR_BEGIN_SIZE 1

struct vector {
    void ** items;
    size_t  ptr_count;
    size_t  item_count;
    void *  (*ctor)(const void *copy);/*ctor - constructor*/
    void    (*dtor)(void *item);/*dtor - deconstructor*/
};

Vector new_vector(void * (*ctor)(const void *copy),void (*dtor)(void *item)) {
    Vector new = calloc(1,sizeof(struct vector));/*allocates memory for every new vector*/
    if(new == NULL)
        return NULL;
    new->ptr_count = VECTOR_BEGIN_SIZE;/*sets the capacity of the vector*/
    new->items = calloc(VECTOR_BEGIN_SIZE,sizeof(void*));/*allocates memory for the array inside the vector */
    if(new->items == NULL) {
        free(new);
        return NULL;
    }
    new->ctor = ctor;
    new->dtor = dtor;
    return new;
}

void * vector_insert(Vector v,const void * copy_item) {
    size_t it;
    void ** temp;/*This variable is used to temporarily store a new pointer to the resized items array when the vector needs to be resized. */
    if(v->item_count == v->ptr_count) {/*checks if vector is full if it is, the vector needs to be resized*/
        v->ptr_count *=2;
        temp = realloc(v->items,v->ptr_count * sizeof(void *));
        if(temp == NULL) {
            v->ptr_count /= 2;/*if temp=null it means that there isn't enough memory available to resize the vector*/
            return NULL;
        }
        v->items = temp;
        v->items[v->item_count] = v->ctor(copy_item);/*creates a copy of the data provided by 'copy item'*/
        if(v->items[v->item_count] ==NULL) {
            return NULL;
        }
        v->item_count++;
        
        for(it = v->item_count; it < v->ptr_count; it++) {
            v->items[it] = NULL;
        }
    }else {/*if the vector isn't full, it searches the NULL slot in the 'items' array*/
        for(it = 0;it<v->ptr_count;it++) {
            if(v->items[it] == NULL) {
                v->items[it] = v->ctor(copy_item);
                if(v->items[it] != NULL) {
                    v->item_count++;
                    break;    
                }
                return NULL;
            }
        }
    }
    return v->items[v->item_count-1];
}

void *const *vector_begin(const Vector v) {
    return v->items;
}

void *const *vector_end(const Vector v) {
    return &v->items[v->ptr_count - 1];
}

size_t vector_get_item_count(const Vector v) {
    return v->item_count; 
}

void vector_destroy(Vector * v) {
    size_t it;
    if(*v != NULL) {
        if((*v)->dtor != NULL) {
            for(it = 0;it < (*v)->ptr_count; it++) {
                if((*v)->items[it] != NULL)
                    (*v)->dtor((*v)->items[it]);
            }
        }
        free((*v)->items);
        free(*v);
        *v = NULL;
    }
}
