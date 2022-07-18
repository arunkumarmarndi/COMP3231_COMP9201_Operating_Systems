#ifndef PRODUCERCONSUMER_H
#define PRODUCERCONSUMER_H

/*  This file contains constants, types, and prototypes for the
 *  producer consumer problem. It is included by the test framework and the
 *  file you modify so as to share the definitions between both.

 *  YOU SHOULD NOT RELY ON CHANGES TO THIS FILE

 *  We will replace it in testing, so any changes will be lost.
 */


#define BUFFER_ITEMS 10   /* The number of items that can be stored in
                            the bounded buffer */

/* 
 * The producer_send() function should block if more than this number
 * of items is sent to the buffer, but won't block while there is
 * space in the buffer.
 */



/* 
 * This is a type definition of the data_item that will be passed
 * between produced and consumers
 */

typedef struct data_item {
        int data1;
        int data2;
} data_item_t;


extern int run_producerconsumer(int, char**);



/* These are the prototypes for the functions that you need to synchronise in
   producerconsumer.c */

data_item_t * consumer_receive(void);   /* receive a data item,
                                         * blocking if no item is
                                         * available in the shared
                                         * buffer
                                         */

void producer_send(data_item_t *);      /* send a data item to the shared
                                         * buffer, block if full.
                                         */

void producerconsumer_startup(void);    /* initialise buffer and
                                         * synchronising code 
                                         */

void producerconsumer_shutdown(void);   /*
                                         * clean up the system at the
                                         * end 
                                         */
#endif 
