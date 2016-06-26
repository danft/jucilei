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
#include "parser.h"
#include "process.h"
#include "job.h"
#include "shell.h"


int main (int argc, char *argv[]) {

    char cmd[256];
    int ret;

    ret = shell_init() ;

    if (ret < 0) {
        return -1;
    }

    while (!feof (stdin)) {
        printf("$ "); 
        if (fgets (cmd, 256, stdin)==NULL)
            putchar('\n'); 

        ret = create_job (cmd);
        if (IS_SYNTAX_ERROR (ret)) {
            puts ("Syntax Error!");
        }
        else if (IS_CMD_LINE_OK (ret))
            run_fg_job();
    }



    return EXIT_SUCCESS;
}
