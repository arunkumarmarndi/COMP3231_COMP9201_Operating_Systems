/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "cafe.h"

/* Some variables shared between threads */

static unsigned int ticket_counter; /* the next customer's ticket */
static unsigned int next_serving;   /* the barista's next ticket to serve */
static unsigned current_customers;  /* customers remaining in cafe */

//solved the cafe problems. added more function for better synchronisation

// these were added by me to solve the problems on concurrency
struct lock *ticket_lock,*serve_lock,*leave_lock,*cur_state,*cur_state_lock; //locks declaration
struct cv *barista_wait[NUM_BARISTAS];  //arrays of conditional variable for barista
struct cv *cust_wait[NUM_CUSTOMERS];    //arrays of conditional variable for customer

/*
 * get_ticket: generates an ticket number for a customers next order.
 */

static unsigned long n_announcement;
static unsigned long n_waiting;

//node declaration
typedef struct node{
        unsigned int val;
        struct node *next;
}node_t;

//list declaration
typedef struct list{
        node_t *head;
        node_t *tail;
}list_t;

static list_t *new_list(){
        list_t *new = kmalloc(sizeof(list_t));
        new->head = new->tail;
        return new;
}

static node_t *new_node(unsigned int val){
        node_t *new = kmalloc(sizeof(node_t));
        new->val = val;
        new->next = NULL;
        return new;
}

static list_t *append(list_t *list,unsigned int val){
        KASSERT(list);
        if(!list->head){
                list->head = new_node(val);
                list->tail =list->head;
        }
        else{
                list->tail->next = new_node(val);
                list->tail = list->tail->next;
        }
        return list;
}

static int has_element(list_t *l,unsigned int val){
        KASSERT(l);
        for(node_t *cur = l->head;cur != NULL; cur = cur->next){
                if(cur->val == val) return 1;
        }
        return 0;
}

//custmor structure that keep tracks of ticket number and barista
typedef struct custmor{
        unsigned long cur_cust;
        unsigned long cur_barista;
        list_t *ticket;
}custmor_t;

//barista structure that keeps track of servings
typedef struct barista{
        unsigned long cur_cust;
        unsigned long cur_barista;
        list_t *serving;
}barista_t;

static custmor_t custmors[NUM_CUSTOMERS];
static barista_t baristas[NUM_BARISTAS];

unsigned int get_ticket(void)
{
        unsigned int t;
        lock_acquire(ticket_lock);
        ticket_counter = ticket_counter + 1;
        t = ticket_counter;
        lock_release(ticket_lock);
        return t;
}

/*
 * next_ticket_to_serve: generates the next ticket number for a
 * barista for that barista to serve next.
 */

unsigned int next_ticket_to_serve(void)
{

        unsigned int t;
        lock_acquire(serve_lock);
        next_serving = next_serving + 1;
        t = next_serving;
        lock_release(serve_lock);

        return t;
}

/*
 * leave_cafe: a function called by a customer thread when the
 * specific thread leaves the cafe.
 */

void leave_cafe(unsigned long customer_num)
{
        (void)customer_num;
        lock_acquire(cur_state_lock);
        current_customers = current_customers - 1;
        if(current_customers == 0){
                for(int i=0;i<NUM_BARISTAS;i++){
                        cv_signal(barista_wait[i],cur_state_lock);
                }
        }
        lock_release(cur_state_lock);


}


//created a function to find someone who has the particular ticket
static unsigned long find_someone_by_ticket(unsigned int ticket, int iscust) {
        unsigned long idx = (unsigned long)-1;
        if (iscust) {
                for (unsigned long i = 0; i < NUM_CUSTOMERS; ++i) {
                        if (has_element(custmors[i].ticket, ticket)) {
                                idx = i;
                                break;
                        }
                }
        } else {
                for (int i = 0; i < NUM_BARISTAS; ++i) {
                        if (has_element(baristas[i].serving, ticket)) {
                                idx = i;
                                break;
                        }
                }
        }

        return idx;
}

/*
 * wait_to_order() and announce_serving_ticket() work together to
 * achieve the following:
 *
 * A customer thread calling wait_to_order will block on a synch primitive
 * until announce_serving_ticket is called with the matching ticket.
 *
 * A barista thread calling announce_serving_ticket will block on a synch
 * primitive until the corresponding ticket is waited on, OR there are
 * no customers left in the cafe.
 *
 * wait_to_order returns the number of the barista that will serve
 * the calling customer thread.
 *
 * announce_serving_ticket returns the number of the customer that the
 * calling barista thread will serve.
 */

unsigned long wait_to_order(unsigned long customer_number, unsigned int ticket)
{
        unsigned long barista_number = 0;

        lock_acquire(cur_state_lock);
        custmors[customer_number].cur_cust = customer_number;
        custmors[customer_number].ticket = append(custmors[customer_number].ticket,ticket);

        //barista_number
        while((barista_number = find_someone_by_ticket(ticket,0)) == (unsigned long)-1){
                kprintf("Customer: %lu waiting on ticket %d\n", customer_number, ticket);
                cv_wait(cust_wait[customer_number],cur_state_lock);
                
        }
        (void) customer_number;
        (void) ticket;
        cv_signal(barista_wait[barista_number],cur_state_lock);
        lock_release(cur_state_lock);

      
        return barista_number;
}

unsigned long announce_serving_ticket(unsigned long barista_number, unsigned int serving)
{
        unsigned long cust = 0;

        (void) barista_number;
        (void) serving;

        lock_acquire(cur_state_lock);
        baristas[barista_number].cur_barista = barista_number;
        baristas[barista_number].serving = append(baristas[barista_number].serving, serving);
        while (current_customers > 0 && (cust = find_someone_by_ticket(serving, 1)) == (unsigned long)-1) {
                kprintf("Barista: %lu waiting on ticket %d\n", barista_number, serving);
                cv_wait(barista_wait[barista_number],cur_state_lock);
        }

        if(current_customers == 0){
                kprintf("All customer served!\n");
                return -1;
        }

        cv_signal(cust_wait[cust],cur_state_lock);
        lock_release(cur_state_lock);

        return cust;
}

/* 
 * cafe_startup: A function to allocate and/or intitialise any memory
 * or synchronisation primitives that are needed prior to the
 * customers and baristas arriving in the cafe.
 */
void cafe_startup(void)
{

        ticket_counter = 0;
        next_serving = 0;
        current_customers = NUM_CUSTOMERS;

        n_announcement = -1;
        n_waiting = -1;

        ticket_lock = lock_create("ticket");
        serve_lock = lock_create("serve_lock");
        leave_lock = lock_create("leave_lock");
        cur_state_lock = lock_create("cur_state_lock");
        char cust_cv_name[] = "cust_wait";
        char barista_cv_name[] = "barista_wait";
        //char *str_i;
        for(int i=0;i < NUM_CUSTOMERS; ++i){
                
                // strcat(cust_cv_name,i);
                cust_wait[i] = cv_create(cust_cv_name);
                custmors[i].cur_cust = -1;
                custmors[i].cur_barista = -1;
                custmors[i].ticket = new_list();
        }

        for(int i=0;i < NUM_BARISTAS; ++i){
                
                // strcat(barista_cv_name,str_i);
                barista_wait[i] = cv_create(barista_cv_name);
                baristas[i].cur_barista = -1;
                baristas[i].cur_cust = -1;
                baristas[i].serving = new_list();
        }

}   

/*
 * cafe_shutdown: A function called after baristas and customers have
 * exited to de-allocate any memory or synchronisation
 * primitives. Anything allocated during startup should be
 * de-allocated after calling this function.
 */

void cafe_shutdown(void)
{
        lock_destroy(ticket_lock);
        lock_destroy(serve_lock);
        lock_destroy(leave_lock);
        lock_destroy(cur_state_lock);
        for(int i=0;i < NUM_CUSTOMERS; ++i){
                
                // strcat(cust_cv_name,i);
                cv_destroy(cust_wait[i]);
        }

        for(int i=0;i < NUM_BARISTAS; ++i){
                
                // strcat(barista_cv_name,str_i);
                cv_destroy(barista_wait[i]);

        }
}
                              
