/*  parser.c - source code of jucilei
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <search.h>
#include "parser.h"
#include "utils.h"

#define IS_NONBLOCK(x) ((x)==NONBLOCK_CHAR)
#define IS_INPUT_REDIR(x) ((x)==INPUT_REDIR_CHAR)
#define IS_OUTPUT_REDIR(x) ((x)==OUTPUT_REDIR_CHAR)
#define IS_IO_REDIR(x) (((x)==INPUT_REDIR_CHAR)||((x)==OUTPUT_REDIR_CHAR))
#define IS_PIPE(x) ((x)==PIPE_CHAR)

cmd_line_t* new_cmd_line (void) {
    cmd_line_t *cmd = malloc (sizeof (cmd_line_t));
    sysfail (cmd==NULL, NULL);
    cmd->io[0] = cmd->io[1] = cmd->io[2] = NULL;
    cmd->pipe_list_head = NULL;
    cmd->pipe_list_tail = NULL;
    cmd->is_nonblock = 0;
    return cmd;
}

/*
   copies the first n bytes of a string
    also puts a \0 at the end 
    also trims the string
 */
char *stringndup(const char *str, size_t n) {
    char *rstr;
    int poff=0; /*offset, how many bytes are blank at the begining*/

    /*triming*/
    while (str[poff] != '\0' && (iscntrl (str[poff]) || isblank (str[poff])))
        poff++;
    n -= poff;
    while (n>0 && (iscntrl (str[poff + n-1]) || isblank (str[poff + n - 1])))
        --n;
    /*end triming*/

    rstr = malloc (sizeof (char) * (n+1));
    sysfail (rstr==NULL, NULL);

    rstr[n] = '\0';
    strncpy (rstr, str + poff, n);
    return rstr;
}

/*cmd_str -> start of the command
    n     -> number of bytes which is in the command 
 */
struct qelem *new_qelem (const char *cmd_str, size_t n) {
    struct qelem *rqelem = malloc (sizeof (struct qelem));
    sysfail (rqelem==NULL, NULL);
    rqelem->q_forw = NULL;
    rqelem->q_back = NULL;
    rqelem->q_data = (char *)stringndup (cmd_str, n);
    return rqelem;
}

/*
returns -1 in case of error 
 */
int pipe_list_push_cmd (cmd_line_t *cmd_line, const char *cmd_str, size_t n) {
    struct qelem *nelem = new_qelem (cmd_str, n); 
    sysfail (nelem==NULL, -1);

    if (strlen (nelem->q_data) == 0) {
        free (nelem->q_data);
        free (nelem);
        return EMPTY_LINE;
    }

    insque (nelem, cmd_line->pipe_list_tail);
    cmd_line->pipe_list_tail = nelem;
    if (cmd_line->pipe_list_head == NULL)
        cmd_line->pipe_list_head = cmd_line->pipe_list_tail;
    return EXIT_SUCCESS;
}


/*
returns:
0 in case of succes 
1 in ... (still not in use)
2 in syntax error
-1 in unexpected error
 */
int parse_cmd_line (cmd_line_t *cmd_line, const char *cmd) {
    size_t prv_cmd_beg = 0, i, cmd_len;

    /*used to get return of functions*/
    int raux;
    /*STDIN, STDOUT, STDERR*/
    int io_redir_seen=-1;
    /*return status*/
    int ret = 0;

    sysfail (cmd_line==NULL, -1);
    cmd_len = strlen (cmd);

    while (cmd_len>0 && (iscntrl (cmd[cmd_len-1]) || isblank (cmd[cmd_len - 1])))
        --cmd_len;

    if (cmd_len == 0) {
        ret |= EMPTY_LINE;
        return ret;
    }

    /*assuming that the last thing is io redirection or nonblocking indicator*/
    for (i=0; i < cmd_len && !IS_IO_REDIR (cmd[i]) && !IS_NONBLOCK (cmd[i]); ++i) {
        if (IS_PIPE (cmd[i])) {
            raux = pipe_list_push_cmd (cmd_line, cmd + prv_cmd_beg, i-prv_cmd_beg);
            sysfail (raux<0, -1);
            prv_cmd_beg = i + 1;
        }
    }

    /*last command (the one that comes after the last pipe)*/
    if (prv_cmd_beg < cmd_len) {
        raux = pipe_list_push_cmd (cmd_line, cmd + prv_cmd_beg, i-prv_cmd_beg);
        sysfail (raux<0, -1);
        if (IS_EMPTY_LINE (raux)) {
            if (cmd_line->pipe_list_head != NULL)
                ret |= SYNTAX_ERROR;
            else ret |= EMPTY_LINE;
            return ret;
        }
        prv_cmd_beg = i + 1;
    }

    for (;i < cmd_len && !IS_NONBLOCK(cmd[i]); ++i) {
        /*todo: recognize output redir*/
        if (IS_IO_REDIR (cmd[i])) { 
            if (io_redir_seen != -1)
                cmd_line->io[io_redir_seen] = stringndup (cmd + prv_cmd_beg, i-prv_cmd_beg);
            prv_cmd_beg = i + 1;
            io_redir_seen = (IS_INPUT_REDIR (cmd[i])) ? 0 : 1;
        }
        else if (IS_PIPE (cmd[i]))
                ret |= SYNTAX_ERROR;
    }

    if (prv_cmd_beg < cmd_len && io_redir_seen != -1) 
        cmd_line->io[io_redir_seen] = stringndup (cmd + prv_cmd_beg, i-prv_cmd_beg);

    /*if it is non_block it must be the last thing*/
    cmd_line->is_nonblock = i < cmd_len && IS_NONBLOCK (cmd[i]);

    if (i < cmd_len-1)
        ret |= SYNTAX_ERROR;
    
    return ret;
} 

/*  DO NOT forget to call this function, after you're done parsing,
    otherwise you'll get memory leak!!*/
void release_cmd_line (cmd_line_t *cmd_line) {
    /**/
    struct qelem *ptr, *aux = NULL;
    size_t i;

    if (cmd_line == NULL)
        return ;

    for (ptr = cmd_line->pipe_list_tail; ptr != NULL; ) {
        free (ptr->q_data);
        aux = ptr->q_back;
        remque (ptr);
        free (ptr);
        ptr = aux;
    }

    for (i=0; i<3; ++i)
        if (cmd_line->io[i] != NULL)
            free (cmd_line->io[i]);
    free (cmd_line);
}


/*
test parser 
 */

/*

void print (cmd_line_t *cmd_line) {
    int c = 0;
    struct qelem *ptr = cmd_line->pipe_list_head;

    printf("INPUT %s\n", (cmd_line->io[0]==NULL) ? "STDIN" : cmd_line->io[0]);
    printf("OUTPUT %s\n", (cmd_line->io[1]==NULL) ? "STDOUT" :cmd_line->io[1]);
    printf("IS_NONBLOCK %d\n", cmd_line->is_nonblock);

    while (ptr != NULL) {
        printf("%d: %s\n", ++c, ptr->q_data);
        ptr = ptr->q_forw;
    }
    puts("------");
}



int main(void) {
    int raux;
    cmd_line_t *cmd_line = NULL;

    char cmd[555];
    while (fgets (cmd, 554, stdin) != NULL ) {
        cmd_line = new_cmd_line ();
        raux = parse_cmd_line (cmd_line, cmd);
        if (IS_SYNTAX_ERROR (raux))
            puts ("Syntax Error!");
        else
            print (cmd_line);
        release_cmd_line (cmd_line);

    }

    return 0;
}
*/
