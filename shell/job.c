/*  job.c - source code of jucilei
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <search.h>
#include "utils.h"
#include "process.h"
#include "job.h"

job_t* new_job (int input_redir, int output_redir, int error_redir) {
    job_t *created_job = malloc (sizeof (job_t));
    created_job->process_list_head = NULL;
    created_job->process_list_tail = NULL;
    created_job->io[0]=input_redir;
    created_job->io[1]=output_redir;
    created_job->io[2]=error_redir;
    created_job->completed = 0;
    created_job->stopped = 0;
    created_job->lch = 0;
    return created_job;
}

/*
   q_data will only point to proc, no copies are made
 */
qelem* new_proc_node (process_t *proc) {
    qelem *ptr = malloc (sizeof (qelem));
    ptr->q_data = (void *) proc;
    ptr->q_forw = ptr->q_back = NULL;
    return ptr;
}

int job_push_process (job_t *job, process_t *proc) {
    qelem *ptr;
    sysfail (job==NULL, -1);

    ptr = new_proc_node (proc);
    sysfail (ptr == NULL, -1);

    insque (ptr, job->process_list_tail);

    /*rearranging pointers*/
    job->process_list_tail = ptr;
    if (job->process_list_head == NULL)
        job->process_list_head = job->process_list_tail;

    return 0;
}

void release_job (job_t *job) {
    qelem *ptr, *aux;
    if (job == NULL)
        return ;
    ptr = job->process_list_head;
    while (ptr != NULL) {
        aux = ptr;
        ptr = ptr->q_forw;
        release_process ((process_t *)aux->q_data);
        remque (aux);
        free (aux);
    }
    free (job);
}

char job_completed (job_t *job) {
    qelem *ptr = job->process_list_head;
    char has = 1;

    /*checks if evry procces is completed*/
    for (;ptr != NULL; ptr = ptr->q_forw)
        has &= ((process_t *)ptr->q_data)->completed == 1;
    job->completed = has;
    return has;
}

/*
the caller is in charge of waiting for the processes created! 
 */
int run_job (job_t *job, char is_fg) {
    qelem *ptr;
    process_t *proc;
    pid_t pid, pgid = 0;
    int pipefd[2], input_redir, output_redir, error_redir;
    if (job == NULL)
        return -1;
    input_redir = job->io[STDIN_FILENO];
    error_redir = job->io[STDERR_FILENO];

    /*
       not sure if this rly works :)
     */

    job->completed = 1;

    for (ptr = job->process_list_head; ptr != NULL; ptr = ptr->q_forw) {
        if (ptr->q_forw != NULL) 
            sysfail (pipe (pipefd)<0, -1);

        output_redir = (ptr->q_forw) ? pipefd[1]: job->io[STDOUT_FILENO];

        proc = (process_t *)ptr->q_data;

        pid = run_process (proc, pgid, input_redir, output_redir, error_redir, is_fg);
        /*0 indicates this process is a builtin function*/
        if (pid != 0) {
            pgid = (!pgid) ? pid : pgid;

            /*putting everyone in the same process group*/
            setpgid (pid, pgid);
        }
        job->completed = job->completed && proc->completed;

        /*closin pipes*/
        if (output_redir != job->io[STDOUT_FILENO])
            close (output_redir);
        if (input_redir != job->io[STDIN_FILENO])
            close (input_redir);

        input_redir = pipefd[0];
    }
    job->pgid = pgid;
    if (is_fg)
        tcsetpgrp (STDIN_FILENO, job->pgid);

    return EXIT_SUCCESS;
}

void print_job_cmd (job_t *job, int fdes) {
    qelem *ptr;
    process_t *proc;
    int j;

    for (ptr = job->process_list_head;ptr != NULL; ptr = ptr->q_forw) {
        proc = (process_t*) ptr->q_data;
        for (j = 0;proc->argv[j] != NULL; j++) {
            dprintf (fdes, "%s ", proc->argv[j]);
        }
        if (ptr->q_forw != NULL)
            dprintf (fdes, "| ");
    }
}

void print_job (job_t *job, char is_curr, int fdes) {

    dprintf (fdes, "[%d]%c %s\t", job->jobid, (is_curr) ? '+': ' ', (job->completed ? "completed": 
                job->stopped ? "stopped" : "running"));

    print_job_cmd (job, fdes);
}

void job_set_stopped (job_t *job, char vsto) {
    qelem *ptr;
    job->stopped = 0;
    for (ptr = job->process_list_head; ptr != NULL; ptr = ptr->q_forw) {
        ((process_t*)ptr->q_data)->stopped = 0;
    }
}
