/* debug.h - Quick debug utilities 

   Copyright 2014  Monaco F. J.   <monaco@icmc.usp.br>
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with .  If not, see <http://www.gnu.org/licenses/>. 

*/


#ifndef UTILS_H 
#define UTILS_H 1 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#ifndef __func__      /* __func__ not avaible prior to c99 */
#define __func__ ""
#endif

/*this for list use, used in parser.c mostly*/
struct qelem {
    struct qelem *q_forw;
    struct qelem *q_back;
    char *q_data;
};
typedef struct qelem qelem;

#define LIST_PUSH(head, tail, data) \
   do { qelem *q = malloc (sizeof (qelem)); \
       q->q_forw = q->q_back = NULL; \
       q->q_data = (void*) data; \
       insque (q, tail); \
       tail = q; \
       if (head == NULL) \
            head = q; \
   } while (0) 

/*does not free anything*/
#define LIST_REM(head, tail, q) \
    do { \
        if (q != NULL) { \
            remque (q); \
            if (q == head) \
                head = head->q_forw; \
            if (q == tail) \
                tail = tail->q_back; \
        }\
    }while (0)


#ifdef POSIXEG_DEBUG

/* Verbose mode. */
#define fail(expression, return_status, message)	\
  do { if (expression) {fprintf (stderr, "%s: %d: %s: %s\n",\
    __FILE__, __LINE__, __func__, message); \
      return (return_status);}} while (0)

#else

/* Non verbose mode.*/
#define fail(expression, return_status, message)	\
  do { if (expression) { return (return_status);}} while (0)

#endif	/* POSIXEG_DEBUG */

/* Report an error. */
#define debug(expression, message) 	\
  do { if (expression) {fprintf (stderr, "%s: %d: %s: %s\n",	\
    __FILE__, __LINE__, __func__, message); \
    }} while (0)

/* 
 *   System errors.
 */

/* Report system error and exit (uses errno) */

#define sysfatal(expression) \
        fatal(expression, strerror(errno))

/* Report a system error and returns wi the the specified code. */

#define sysfail(expression, return_status) \
        fail(expression, return_status, strerror(errno))

/* Report a system error. */

#define sysdebug(expression) \
       debug(expression, strerror(errno))

#endif/* DEBUG_H */
