/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "producerconsumer.h"

/* The bounded buffer uses an extra free slot to distinguish between
   empty and full. See
   http://www.mathcs.emory.edu/~cheung/Courses/171/Syllabus/8-List/array-queue2.html
   for details if needed. 
*/
//solved according to the question
#define BUFFER_SIZE (BUFFER_ITEMS + 1)

/* Declare any variables you need here to keep track of and
   synchronise the bounded buffer. The declaration of a buffer is
   shown below. It is an array of pointers to data_items.
*/
struct lock *mutex; //lock for mutual exclusion
struct cv *consumer_wait,*producer_wait;


data_item_t * item_buffer[BUFFER_SIZE];

volatile int write_head, read_tail;



/* The following functions test if the buffer is full or empty. They
   are obviously not synchronised in any way */

static bool is_full() {
        return (write_head + 1) % BUFFER_SIZE == read_tail;
}

static bool is_empty() {
        return write_head == read_tail;
}

/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. It should not busy wait! Any concurrency issues involving
   shared state should also be handled. 
*/

data_item_t * consumer_receive(void)
{
        data_item_t * item;

        lock_acquire(mutex);
        while(is_empty()) {
                /* busy wait */
                cv_wait(consumer_wait,mutex);

        }
        item = item_buffer[read_tail];
        read_tail = (read_tail + 1) % BUFFER_SIZE;
        cv_signal(producer_wait,mutex);
        lock_release(mutex);

        return item;
}

/* producer_send() is called by a producer to store data in the
   bounded buffer.  It should block on a sync primitive if no space is
   available in the buffer. It should not busy wait! Any concurrency
   issues involving shared state should also be handled.
*/

void producer_send(data_item_t *item)
{
        lock_acquire(mutex);
        while(is_full()) {
                /* busy wait */
                cv_wait(producer_wait,mutex);
        }
        item_buffer[write_head] = item;
        write_head = (write_head + 1) % BUFFER_SIZE;
        cv_signal(consumer_wait,mutex);
        lock_release(mutex);
}

/* Perform any initialisation (e.g. of global data or synchronisation
   variables) you need here. Note: You can panic if any allocationa
   fails during setup
*/

void producerconsumer_startup(void)
{
        write_head = read_tail = 0;
        mutex = lock_create("mutex");
        consumer_wait = cv_create("consumer_wait");
        producer_wait = cv_create("producer_wait");

        
}

/* Perform your clean-up here */
void producerconsumer_shutdown(void)
{
        lock_destroy(mutex);
        cv_destroy(consumer_wait);
        cv_destroy(producer_wait);
}

