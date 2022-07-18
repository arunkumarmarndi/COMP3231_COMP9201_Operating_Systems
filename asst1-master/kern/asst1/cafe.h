#ifndef CAFE_H
#define CAFE_H

/*  This file contains constants, types, and prototypes for the
 *  producer consumer problem. It is included by the test framework and the
 *  file you modify so as to share the definitions between both.

 *  YOU SHOULD NOT RELY ON CHANGES TO THIS FILE

 *  We will replace it in testing, so any changes will be lost.
 */

/* The number of baristas for this compile of the simlation. This will
 * be changed during testing
 */
#define NUM_BARISTAS 2

/* The number of customers for this compile of the simlation. This will
 * be changed during testing
 */
#define NUM_CUSTOMERS 3

/* Customer functions */
extern unsigned int get_ticket(void);
extern unsigned long wait_to_order(unsigned long, unsigned int);
extern void leave_cafe(unsigned long);

/* Barista functions */
extern unsigned int next_ticket_to_serve(void);
extern unsigned long announce_serving_ticket(unsigned long, unsigned int);

/* startup and shutdown functions */
extern void cafe_startup(void);
extern void cafe_shutdown(void);

extern int run_cafe(int, char**);

#endif 
