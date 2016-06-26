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

pid_t run_process (process_t *proc, pid_t pgid, int input_redir, int output_redir, int error_redir, char is_fg) {
    pid_t pid;
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

        exit (RUN_PROC_FAILURE);
    }
    return pid;
}
