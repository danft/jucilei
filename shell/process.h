/*  process.h - source code of jucilei
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

#ifndef PROC_H
#define PROC_H

#define CMD_DELIM " \n\t\r"
#define CMD_MAXARGS 256

#define RUN_PROC_FAILURE 0

typedef struct {
    pid_t pid;
    char *argv[CMD_MAXARGS];
    char completed;
    char stopped;
    int status;
} process_t;

/*creates a new process from a command string*/
process_t *new_process (const char *cmd) ;

void release_process (process_t *proc);

/*
this function alters the pid attribute in proc 
returns the pid of the child
(input,output,error)_redir are file descriptors
 */
pid_t run_process (process_t *proc, pid_t pgid, int input_redir, int output_redir, int error_redir);

#endif
