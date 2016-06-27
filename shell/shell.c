/*  shell.c - source code of jucilei
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"
#include "parser.h"
#include "process.h"
#include "job.h"

#define IS_FG_JOB(job) (((job_t*)(job))==fg_job) 

/**/
qelem *job_list_head, *job_list_tail;

/*the top element is always the one that got modified the latest*/
qelem *job_stack;

/*
   job which is currently running as foreground
 */
job_t *fg_job = NULL;

pid_t shell_pgid;
/*file descriptor from which the shell interacts with*/
int shell_terminal;


/*
returns -1 in case of failure 
 */
int shell_init () {
    fg_job = NULL;
    job_list_head = job_list_tail = job_stack = NULL;
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    signal (SIGCHLD, SIG_IGN);
    shell_terminal = STDIN_FILENO;
    shell_pgid = getpid ();

    sysfail (setpgid (shell_pgid, shell_pgid) < 0, -1);

    /*takes controll of the terminal as a foreground procces group*/
    tcsetpgrp (shell_terminal, shell_pgid);

    return EXIT_SUCCESS;
}

void update_job_stack() {
    job_t **job;
    qelem *q;
    while (job_stack != NULL) {
        job = (job_t**) job_stack->q_data;
        if (*job == NULL) {
            q = job_stack;
            LIST_REM (job_stack, job_stack, q);
        }
        else break;
    }
}

void job_stack_push (job_t *job) {
    LIST_PUSH (job_stack, job_stack, &job);
}


void _sigchld_handler (int signum) {
    qelem *p, *q;
    process_t *proc;
    job_t *job;

    for (q = job_list_head; q != NULL; q = (q != NULL) ? q->q_forw : job_list_head) {
        
        char completed_all = 1;
        char stopped_all = 1;
        job = (job_t*) q->q_data;

        /*printf ("%p %d\n", job, job->pgid);*/

        for (p = job->process_list_head; p != NULL; p = p->q_forw) {

            proc = (process_t*) p->q_data;
            if (proc->completed)
                continue;
            if (waitpid (proc->pid, &proc->status, WNOHANG | WUNTRACED)) {
                if (WIFEXITED (proc->status)) {
                    proc->completed = 1;
                    proc->status = WEXITSTATUS (proc->status);
                }
                else if (WIFSTOPPED (proc->status)) 
                    proc->stopped = 1;
            }
            completed_all = completed_all && proc->completed;
            stopped_all = stopped_all && proc->stopped;
        }

        /*
           we only remove if the job is foreground
         */
        if (completed_all) {
            job->completed = 1;
            if (IS_FG_JOB (job)) {
                fg_job = NULL;
                LIST_REM (job_list_head, job_list_tail, q);
                p = q->q_back;
                release_job ((job_t*) q->q_data);
                free (q);
                q = p;
            }
        }
        if (stopped_all) {
            if (IS_FG_JOB (job))
                fg_job = NULL;
            job->stopped = 1;
        }
    }
}

int run_fg_job() {
    while (fg_job != NULL && fg_job->completed == 0)
        pause();

    if (fg_job != NULL) { /*this will only happen if the job has only builtin commands */
        qelem *ptr;
        for (ptr = job_list_head; ptr != NULL; ptr = ptr->q_forw) {
            if ((void *)ptr->q_data == (void *)fg_job) {
                LIST_REM (job_list_head, job_list_tail, ptr);
                release_job (fg_job);
                free (ptr);
                fg_job = NULL;
                break;
            }
        }
    }


    fg_job = NULL;
    tcsetpgrp (STDIN_FILENO, shell_pgid); 
    /*here we would have to set up terminal options*/

    return EXIT_SUCCESS;
}

void print_job_list(int output_redir, int error_redir) {
    qelem *ptr, *aux;
    job_t *job;

    for (ptr = job_list_head; ptr != NULL; ptr = (ptr) ? ptr->q_forw: job_list_head) {
        job = (job_t*) ptr->q_data;
        print_job (job);

        if (job->completed) {
            LIST_REM (job_list_head, job_list_tail, ptr);
            aux = ptr->q_back;
            free (ptr);
            ptr = aux;
            release_job (job);
        }
    }
}

/*
returns -1 in case of error (cmd coundn't be executed) 
 */
int create_job (const char *cmd) {

    int i, aux; /*used to get return values*/
    int ret = EXIT_SUCCESS; /*return value*/

    /*io redirection stuff*/
    int io[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    int iofl[3] = {O_RDONLY, O_WRONLY | O_CREAT, O_WRONLY | O_CREAT}; /*io flag for each one of the input redirection*/


    process_t *proc = NULL;
    cmd_line_t *cmd_line = NULL;
    qelem *ptr;
    job_t *job = NULL;
    struct sigaction oact, act;

    cmd_line = new_cmd_line();

    aux = parse_cmd_line (cmd_line, cmd);

    /*input redir file*/
    for (i=0; i<3; ++i) {
        if (cmd_line->io[i] != NULL) {
            /*printf ("%d %s\n", i, (cmd_line->io[i])); */
            io[i] = open (cmd_line->io[i], iofl[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

            if (io[i] < 0) {
                printf ("%s: %s\n", cmd_line->io[i], strerror (errno));
                goto release_stuff;
            }
        }
    }

    job = new_job(io[STDIN_FILENO], io[STDOUT_FILENO], io[STDERR_FILENO]);

    job->jobid = (job_list_tail == NULL) ? 1 : ((job_t*)job_list_tail->q_data)->jobid + 1;

    if (!IS_CMD_LINE_OK (aux)) {
        ret = aux;
        goto release_stuff;
    }

    for (ptr = cmd_line->pipe_list_head; ptr != NULL; ptr=ptr->q_forw) {
        proc = new_process (ptr->q_data);
        aux = job_push_process (job, proc);
        if (aux==-1) {
            ret = -1;
            goto release_stuff;
        }
    }

    /*running (need to know if it's foreground)*/
    aux = run_job (job, cmd_line->is_nonblock == 0);

    /*problem with running the job*/
    if (aux == -1) {
        ret = -1;
        goto release_stuff;
    }
    /*now we can consider the job is successfully begin executed*/
    
    if (!cmd_line->is_nonblock)
        fg_job = job;

    LIST_PUSH (job_list_head, job_list_tail, job);

    act.sa_handler = _sigchld_handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;

    sigaction (SIGCHLD, NULL, &oact);

    if (oact.sa_handler != _sigchld_handler)
        sigaction (SIGCHLD, &act, NULL);
        

    /*print_job (job); */


    /*printf ("%d\n", run_process(proc, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO)); */
    /*release_process (proc); */

    /*TODO: find a better name for this*/
release_stuff:
    if (ret != 0)
        release_job (job);
    release_cmd_line (cmd_line);
    return ret;
}
