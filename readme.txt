AM 1115201400215 - Thanasis Filippidis
sdi1400215
sdi1400215@di.uoa.gr

K24 Assignment 2

Files included:
- jms_console.c
- jms_coord.c
- pool.c

Implementation Notes:
- all the requested functions are implemented
- makefile included
- creates all 3 object files for all 3 programs (console, coord, pool)
- bash script done
- please finish the input file with \n

- in coord there are 2 structs 
one for all the pools (info about pid, status, pipes' names and fds, number of jobs)
one for the finished jobs, when a pool is done, it keeps the data of the jobs 
- in pool there is 1 struct
it keeps the id, pid, submit time and status of every job

- all the pipes are BLOCKING except: 
read (coord from console)
read (coord from pool)
read (pool from coord)
in order to exchange information about the statuses of the jobs without any
interferance from the CMD commands the user gives.

- need to be defined at the begining of every .c file
Max number of pools 
Max number of chars send through the pipes
