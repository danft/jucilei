/*  shell.h - source code of jucilei
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



/*
you have to inform if the sheel will be interactive (with job control) or non-interactive 
 */
int shell_init (char is_interactive);

int shell_nintve (const char *cmd);


/*
returns -1 in case of error (cmd coundn't be executed) 
 */
int create_job (const char *cmd);

/*
runs the current job that is in foreground mode 
if there's none it just returns
 */
int run_fgjob();
