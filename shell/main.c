/*  main.c - source code of jucilei
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
#include <argp.h>
#include "parser.h"
#include "process.h"
#include "job.h"
#include "shell.h"

const char *argp_program_version = "jucilei 0.1";

static char doc [] =
"Jucilei is an attempt to implement a POSIX shell";

static char args_doc[] = "ARG1";

static struct argp_option options [] = {
    {"command", 'c', "cmd", 0, "Execute jucilei in the non-interactive form"},
    {0}
};

struct arguments {
    int command;
    char *argv[256];
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    arguments->argv[state->argc] = NULL;
    switch (key) {
        case 'c':
            arguments->command = 1;
            arguments->argv[0] = arg;
			arguments->argv[1] = NULL;
            break;
        case ARGP_KEY_ARG:
            arguments->argv[1+state->arg_num] = arg;
            arguments->argv[2+state->arg_num] = NULL;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options,parse_opt,args_doc,doc};

int main (int argc, char *argv[]) {

    char cmd[256];
    int ret, j;
    char hinter = 0;

    /*I'm in love with extern ;)*/
    extern char hexit;
    struct arguments arguments;
    arguments.command = 0;

    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    /*this means will run a non-interactive shell*/
    if (arguments.command == 1) {
        for (j = 0; arguments.argv[j] != NULL; j++)
            printf ("x%s\n", arguments.argv[j]);
    }

    ret = shell_init(arguments.command == 0);

    if (arguments.command == 1) { /**/
        size_t boff = 0;
        for (j = 0; arguments.argv[j] != NULL; ++j) {
            strcpy (cmd + boff, arguments.argv[j]);
            boff += strlen (arguments.argv[j]);
			cmd[boff++] = ' ';
			cmd[boff] = '\0';
        }
        ret = create_job(cmd);  
        return 0;
    }

    if (ret < 0) {
        return -1;
    }

    while (!feof(stdin) && !hexit) {

        if (!hinter)
            printf ("$ ");
        hinter = 0;

        /*this happens when we are block waiting and then comes a signal*/
        if (fgets (cmd, 256, stdin)==NULL) {
            hinter = 1;
            continue;
        }

        ret = create_job (cmd);
        if (IS_SYNTAX_ERROR (ret)) {
            puts ("Syntax Error!");
        }
        else if (IS_CMD_LINE_OK (ret))
            run_fgjob();
    }



    return EXIT_SUCCESS;
}
