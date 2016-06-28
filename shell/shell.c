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

#define IS_FG_JOB(job) (((job_t*)(job))==fgjob) 

/**/
qelem *job_list_head, *job_list_tail;

/*the top element is always the one that got modified the latest*/

/*
   job which is currently running as foreground
 */
job_t *fgjob = NULL;

pid_t shell_pgid;
/*file descriptor from which the shell interacts with*/
int shell_terminal;
char hexit;

size_t shell_cnt; /*used to identify the last changed process*/

/*
returns -1 in case of failure 
 */
int shell_init () {
    hexit = 0;
    fgjob = NULL;
    shell_cnt = 0;
    job_list_head = job_list_tail = NULL;
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


void _sigchld_handler (int signum) {
    qelem *p, *q;
    process_t *proc;
    job_t *job;

    for (q = job_list_head; q != NULL; q = (q != NULL) ? q->q_forw : job_list_head) {
        
        char completed_all = 1;
        char stopped_all = 1;
        job = (job_t*) q->q_data;

        for (p = job->process_list_head; p != NULL; p = p->q_forw) {

            proc = (process_t*) p->q_data;
            if (proc->completed)
                continue;
            if (waitpid (proc->pid, &proc->status, WNOHANG | WUNTRACED)) {
                if (WIFEXITED (proc->status) || WIFSIGNALED (proc->status)) 
                    proc->completed = 1;
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
            job->lch = ++shell_cnt;
            if (IS_FG_JOB (job)) {
                fgjob = NULL;
                LIST_REM (job_list_head, job_list_tail, q);
                p = q->q_back;
                release_job ((job_t*) q->q_data);
                free (q);
                q = p;
            }
        }
        if (stopped_all) {
            job->lch = ++shell_cnt;
            if (IS_FG_JOB (job))
                fgjob = NULL;
            job->stopped = 1;
        }
    }
}

int run_fgjob() {

    while (fgjob != NULL && fgjob->completed == 0) {
        pause();
    }

    if (fgjob != NULL) { /*this will only happen if the job has only builtin commands */
        qelem *ptr;
        for (ptr = job_list_head; ptr != NULL; ptr = ptr->q_forw) {
            if (IS_FG_JOB ((job_t*) ptr->q_data)) {
                LIST_REM (job_list_head, job_list_tail, ptr);
                release_job (fgjob);
                free (ptr);
                fgjob = NULL;
                break;
            }
        }
    }

    fgjob = NULL;
    tcsetpgrp (STDIN_FILENO, shell_pgid); 
    /*here we would have to set up terminal options*/

    return EXIT_SUCCESS;
}

void print_job_list(int output_redir, int error_redir) {
    qelem *ptr, *aux;
    job_t *job;
    size_t mch = 0;
    int curr_jobid = 0;

    for (ptr = job_list_head; ptr != NULL; ptr = ptr->q_forw) {
        job = (job_t *)ptr->q_data;
        if (job->lch > mch || (job->lch == mch && job->jobid > curr_jobid)) {
            curr_jobid = job->jobid;
            mch = job->lch;
        }
    }

    for (ptr = job_list_head; ptr != NULL; ptr = (ptr) ? ptr->q_forw: job_list_head) {
        job = (job_t*) ptr->q_data;

        print_job (job, job->jobid == curr_jobid, output_redir);
        write (output_redir, "\n", 1);

        if (job->completed) {
            LIST_REM (job_list_head, job_list_tail, ptr);
            aux = ptr->q_back;
            free (ptr);
            ptr = aux;
            release_job (job);
        }
    }
}

job_t* get_job_id (int jobid) {
    qelem *ptr;
    job_t *job;

    for (ptr = job_list_head; ptr != NULL; ptr = ptr->q_forw) {
        job = (job_t*) ptr->q_data;
        if (job->jobid == jobid)
            return job;
    }
    return NULL;
}

job_t* get_curr_job () {
    size_t mch = 0; 
    int jobid = 0;
    qelem *ptr;
    job_t *rjob = NULL, *job;

    for (ptr = job_list_head; ptr != NULL; ptr = ptr->q_forw) {
        job = (job_t*) ptr->q_data;
        if (mch < job->lch || (mch == job->lch && jobid < job->jobid)) {
            mch = job->lch;
            jobid = job->jobid;
            rjob = job;
        }
    }
    return rjob;
}

int set_fgjob (job_t *job) {
    if (job == NULL)
        return -1;

    if (job->stopped) {
        if (kill (-job->pgid, SIGCONT) < 0) 
            return -1;
        job_set_stopped (job, 0);
    }
    fgjob = job;
    tcsetpgrp (shell_terminal, job->pgid);
    return EXIT_SUCCESS;
}

int shell_job_bg (int jobid, int output_redir, int error_redir) {
    job_t *job;

    job = (jobid == 0) ? get_curr_job () : get_job_id (jobid);

    if (job == NULL) {
        dprintf(error_redir, "fg: No such job\n");
        return 1;
    }
    job->lch = ++shell_cnt;
    print_job_cmd (job, output_redir);
    write (output_redir, " &\n", 3);
    if (job->stopped) {
        if (kill (-job->pgid, SIGCONT) < 0) 
            return -1;
        job_set_stopped (job, 0);
    }
    return EXIT_SUCCESS;
}

void shell_exit () {
    hexit = 1;
    close (shell_terminal);
}


int shell_job_fg (int jobid, int output_redir, int error_redir) {
    job_t *job;

    job = (jobid == 0) ? get_curr_job () : get_job_id (jobid);

    if (job == NULL) {
        dprintf(error_redir, "fg: No such job\n");
        return 1;
    }
    print_job_cmd (job, output_redir);
    write (output_redir, "\n", 1);

    sysfail (set_fgjob (job) < 0, -1);

    run_fgjob ();

    return EXIT_SUCCESS;
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
        fgjob = job;

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
