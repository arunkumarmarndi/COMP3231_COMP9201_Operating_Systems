#include "opt-synchprobs.h"
#include "counter.h"
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * ********************************************************************
 * INSERT ANY GLOBAL VARIABLES THAT YOU MAY REQUIRE HERE
 * ********************************************************************
 */


/*
 * counter_initialise() allocates a synchronised counter and initialises
 * it with the value passed in.
 * 
 * Multiple counters an be initialised concurrently. You can assume
 * kmalloc() is thread safe (i.e. it is itself synchronised).
 */

struct sync_counter * counter_initialise(int val)
{
        struct sync_counter *sc_ptr;



        sc_ptr = (struct sync_counter *) kmalloc(sizeof(struct sync_counter));

        

        if (sc_ptr == NULL) {
                /* Allocation failed */
                return NULL;
        }


        sc_ptr->l = lock_create("Lock_1"); //creating the lock;

        if (sc_ptr->l == NULL){
                /*Sanity check*/
                /*Allocation failure*/
                return NULL;
        }

        sc_ptr->counter = val;
        
        
        /*
         * ********************************************************************
         * INSERT ANY INITIALISATION CODE YOU MAY REQUIRE HERE
         * ********************************************************************
         */
        
        return sc_ptr;
}

/*
 * counter_read_and_destroy() takes a pointer to a specific
 * synchronised counter and de-allocates any resources associated with
 * it and returns the final value of the counter.
 *
 * This function can be called concurrently. You can assume kfree() is thread safe.
 */
//modified according to the question

int counter_read_and_destroy(struct sync_counter *sc_ptr)
{
        int count;

        count = sc_ptr->counter;
        
        /*
         * **********************************************************************
         * INSERT ANY CLEANUP CODE YOU MAY REQUIRE HERE
         * **********************************************************************
         * 
         */
        lock_destroy(sc_ptr->l); //deallocate the memory, destroy the lock that was initialized
        kfree(sc_ptr); //deallocate the pointer
        return count;
}

/*
 * Increment the specific synchronised counter by 1. This function can
 * be called concurrently by multiple threads.
 */

void counter_increment(struct sync_counter *sc_ptr)
{
        lock_acquire(sc_ptr->l);
        sc_ptr->counter = sc_ptr->counter + 1;
        lock_release(sc_ptr->l);
}

/*
 * Decrement the specific synchronised counter by 1.This function can
 * be called concurrently by multiple threads. 
 */

void counter_decrement(struct sync_counter *sc_ptr)
{
        lock_acquire(sc_ptr->l);
        sc_ptr->counter = sc_ptr->counter - 1;
        lock_release(sc_ptr->l);
}

