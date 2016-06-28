#    Copyright (c) Danilo Tedeschi 2016  <danfyty@gmail.com>
#
#    This file is part of Jucilei.
#
#    jucilei is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    jucilei is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with jucilei.  If not, see <http://www.gnu.org/licenses/>.

PROJECT = runcmd
PREFIX = ./local

CC = gcc
CPP_FLAGS = -Wall --pedantic-errors -D_POSIX_C_SOURCE=200809L
C_FLAGS = -g
LD_FLAGS =
MAKE = make
AR = ar
TAR = tar 

bin =
lib = libruncmd
libruncmd_obj = runcmd.o
libruncmd_h = runcmd.h

EXTRA_DIST = runcmd.txt Makefile config.mk
