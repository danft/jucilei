/*  job.h - source code of jucilei
    Copyright (c) Danilo Tedeschi 2016  <danfyty@gmail.com>

    This file is part of Jucilei.

    jucilei is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    jucilei is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with jucilei.  If not, see <http://www.gnu.org/licenses/>.

 */
#ifndef JOB_H
#define JOB_H

#include <search.h>
#include "utils.h"
#include "process.h"

typedef struct {
    /*this list is the pipeline*/
   qelem *process_list_head, *process_list_tail; 
   pid_t pgid; /*process group id*/
   int io[3]; /*0 -> input, 1->output, 2 -> error*/

   int jobid; /*used in jobs, fg and bg*/

   char completed, stopped;

   size_t lch; /*last change*/ 

}job_t;

job_t* new_job () ;

void release_job (job_t *job);

/*
   adds process to the proccess list

 */
int job_push_process (job_t *job, process_t *proc);

/*
returns 1 if every process finished, 0 otherwise 
 */
char job_completed (job_t *job);

/*set the stopped value of the job and all of 
 its process*/
void job_set_stopped (job_t *job, char vsto);

/*
runs every process in the pipeline 
 */
int run_job (job_t *job);

/*prints the entire job command line into fdes*/
void print_job_cmd (job_t *job, int fdes) ; 

/*print job info into file with file descriptor fdes*/
void print_job (job_t *job, char is_curr, int fdes);

#endif
