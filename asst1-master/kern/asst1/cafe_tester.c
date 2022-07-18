/*
 * Testing file for the producer / consumer simulation.
 *
 * This starts up a number of producer and consumer threads and has them
 * communicate via the API defined in producerconsumer.h, which is
 * implemented by you in producerconsumer.c.
 *
 * NOTE: DO NOT RELY ON ANY CHANGES YOU MAKE TO THIS FILE, BECAUSE
 * IT WILL BE OVERWRITTEN DURING TESTING.
 */
#include "opt-synchprobs.h"
#include <types.h>  /* required by lib.h */
#include <lib.h>    /* for kprintf */
#include <synch.h>  /* for P(), V(), sem_* */
#include <thread.h> /* for thread_fork() */
#include <test.h>

#include "cafe.h"

#define NUM_COFFEES 2

/* Semaphores which the simulator uses to determine when all
 * barista threads and all customer threads have finished.
 */
static struct semaphore *customers_finished;
static struct semaphore *baristas_finished;


static void
barista_thread(void *unused_ptr, unsigned long barista_num)
{
        int i;
        unsigned int ticket;
        unsigned long cust;
        (void)unused_ptr; /* Avoid compiler warnings */
        
        kprintf("Barista thread %ld started\n", barista_num);

        for (i = 0; i < (NUM_COFFEES * NUM_CUSTOMERS / NUM_BARISTAS); i++) {

                /* Serve the next ticket */
                ticket = next_ticket_to_serve();

                kprintf("Barista %ld serving ticket %d\n", barista_num, ticket);
                
                cust = announce_serving_ticket(barista_num, ticket);

                kprintf("Barista %ld accepted customer %ld with ticket %d\n",
                        barista_num, cust, ticket);
                

                /* After announcing the next ticket, receiving the
                   order, making coffee, and handing it to the
                   customer is not simulated. */
        }
        
        V(baristas_finished);
}

static void
customer_thread(void *unused_ptr, unsigned long customer_num)
{
        int i;
        int ticket;
        unsigned long barista;
        (void)unused_ptr;

        kprintf("Customer thread %ld started\n", customer_num);

        for (i = 0; i < NUM_COFFEES; i++) {

                /* get a ticket from the ticket dispenser */
                ticket = get_ticket();
                kprintf("Customer %ld got ticket %d\n", customer_num, ticket);

                /* Now wait for the ticket to woken up by a specific barista */
                barista = wait_to_order(customer_num, ticket);

                /* We don't simulate the customer subsequently
                   ordering a coffee from the identified barista,
                   receiving it, and drinking it. */
                
                kprintf("Customer %ld ordered from barista %ld\n", customer_num, barista); 
        }
        
        kprintf("Customer %ld leaving cafe\n", customer_num);
        leave_cafe(customer_num);

        /* Signal that we're done. */
        V(customers_finished);
}

/* Create a bunch of threads to drink coffee. */
static void
start_customer_threads()
{
        int i;
        int result;

        for(i = 0; i < NUM_CUSTOMERS; i++) {
                result = thread_fork("customer thread", NULL,
                                     customer_thread, NULL, i);
                if(result) {
                        panic("start_customer_threads: couldn't fork (%s)\n",
                              strerror(result));
                }
        }
}

/* Create a bunch of baristas to make coffee. */
static void
start_barista_threads()
{
        int i;
        int result;

        for(i = 0; i < NUM_BARISTAS; i++) {
                result = thread_fork("barista thread", NULL,
                                     barista_thread, NULL, i);
                if(result) {
                        panic("start_barista_threads: couldn't fork (%s)\n",
                              strerror(result));
                }
        }
}

/* Wait for all barista threads to exit.  Baristas signal a semaphore
on exit, so waiting for them to finish means waiting on that semaphore
NUM_BARISTAS times. */

static void
wait_for_barista_threads()
{
        int i;
        kprintf("Waiting for barista threads to exit...\n");
        for(i = 0; i < NUM_BARISTAS; i++) {
                P(baristas_finished);
        }
        kprintf("All barista threads have exited.\n");
}

/* Wait for all the customer threads to exit.
 */
static void
wait_customer_threads()
{
        int i;

        /* Now wait for all customers to signal completion. */
        for(i = 0; i < NUM_CUSTOMERS; i++) {
                P(customers_finished);
        }
}

/* The main function for the simulation. */
int
run_cafe(int nargs, char **args)
{
        (void) nargs; /* Avoid "unused variable" warning */
        (void) args;

        kprintf("run_cafe: starting up\n");

        /* Initialise synch primitives used in this testing framework */
        customers_finished = sem_create("customer_finished", 0);
        if(!customers_finished) {
                panic("run_cafe: couldn't create semaphore\n");
        }

        baristas_finished = sem_create("barista_finished", 0);
        if(!baristas_finished ) {
                panic("run_cafe: couldn't create semaphore\n");
        }

        /* Now run the student code required to initialise their synch
           primitives etc */
        cafe_startup();

        /* Run the simulation */
        start_customer_threads();
        start_barista_threads();

        /* Wait for all producers and consumers to finish */

        wait_for_barista_threads();
        wait_customer_threads();

        /* Run any student code required to clean up after the simulation */
        cafe_shutdown();

        /* Done! */
        sem_destroy(baristas_finished);
        sem_destroy(customers_finished);
        return 0;
}

