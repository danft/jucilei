/*  process.c - source code of jucilei
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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "utils.h"
#include "process.h"


char* builtin_cmd [] = {"cd", "jobs", NULL};

int builtin_cd (process_t *proc, int input_redir, int output_redir, int error_redir) {
    if (proc->argv[1] != NULL)
        return chdir (proc->argv[1]);
    return 0;
}

/*this one is defined in shell.c*/
extern void print_job_list (int, int);

int builtin_jobs (process_t *proc, int input_redir, int output_redir, int error_redir) {
    if (proc == NULL)
        return -1;
    print_job_list (output_redir, error_redir);
    return 0;
}

int (*builtin_func[2]) (process_t *, int, int, int) = {builtin_cd, builtin_jobs};

/*checks if proc is a bultin cmd and returns the id of the function*/
int chk_builtincmd (process_t *proc) {
    int j;
    for (j = 0; builtin_cmd[j] != NULL; ++j)
        if (strcmp (builtin_cmd[j], proc->argv[0])==0)
            return j;
    return -1;
}

process_t* new_process (const char *command) {
    size_t i;
    char *token, *cmd;   
    process_t *rprocess;

    cmd = malloc (sizeof (char) * (strlen (command)+1));
    rprocess = malloc (sizeof (process_t));
    rprocess->completed = 0;
    rprocess->stopped = 0;
    rprocess->status = 0;
    strcpy (cmd, command);

    token = strtok (cmd, CMD_DELIM);
    for (i=0; i<CMD_MAXARGS && token != NULL; ++i) {
        rprocess->argv[i]=token;
        token = strtok (NULL, CMD_DELIM);
    }
    rprocess->argv[i] = NULL;
    return rprocess;
}

void release_process (process_t *proc) {
    /*as we used strtok to build this string we just need 1 free*/
    if (proc->argv[0] != NULL)
        free (proc->argv[0]);
    free (proc);
}

/*if pid is 0, then it's a builtin function*/
pid_t run_process (process_t *proc, pid_t pgid, int input_redir, int output_redir, int error_redir, char is_fg) {
    pid_t pid;
    int builtin_id;
    builtin_id = chk_builtincmd (proc);

    if (builtin_id != -1) {
        proc->status = builtin_func[builtin_id] (proc, input_redir, output_redir, error_redir);
        proc->completed = 1;
        proc->pid = 0;
        return proc->pid;
    }

    pid = fork();

    /*returns -1 if fork failed*/
    sysfail (pid < 0, -1);
    proc->pid = pid;

    if (pid == 0) { /*child*/

        pid = getpid();
        if (!pgid)
            pgid = pid;
        setpgid (pid, pgid);

        if (is_fg) 
            tcsetpgrp (STDIN_FILENO, pgid);

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
        

        /*io redirection*/
        if (input_redir != STDIN_FILENO) {
            sysfail (dup2 (input_redir, STDIN_FILENO)<0, RUN_PROC_FAILURE);
            close (input_redir);
        }
        if (output_redir != STDOUT_FILENO) {
            sysfail (dup2 (output_redir, STDOUT_FILENO)<0, RUN_PROC_FAILURE);
            close (output_redir);
        }
        if (error_redir != STDERR_FILENO) {
            sysfail (dup2 (error_redir, STDERR_FILENO)<0, RUN_PROC_FAILURE);
            close (error_redir);
        }

        execvp (proc->argv[0], proc->argv);

        /*we have something wrong */
        write (output_redir, proc->argv[0], strlen (proc->argv[0]));
        write (output_redir, ": ", 2);
        write (output_redir, strerror (errno), strlen (strerror (errno)));
        write (output_redir, "\n", 1);
        release_process (proc);

        exit (RUN_PROC_FAILURE);
    }
    return pid;
}
