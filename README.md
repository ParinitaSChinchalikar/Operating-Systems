# Operating-Systems
This repository contains the code studied during the course for CS550: Operating Systems as a part of my curriculum and learning

# Part 1
This consists of an understanding of basic commands of Linux 

## Execution of the code
In a make file, add the following lines to it

shell: shell.c
	gcc shell.c -o shell

 Then after this, run the command "./shell".

# Part 2

This part consists of Translation Lookaside Buffer (TLB) with the calculation of Page Faults and Virtual Memory

## Execution of the code
In a make file, add the following lines to it

tlb: tlb.c
	gcc tlb.c -o tlb -lm 

Then after this, run the command "./tlb".

# Part 3

This part contains a small puzzle implementing how the seats in a diner are arranged depending on whether a group has been seated or a single person is seated and when a new person gets seated depending on the empty seats present.

## Execution of the code
In a make file, add the following lines to it

monks: monks.c
	gcc monks.c -o monks -pthread

diner: diner.c
	gcc diner.c -o diner -pthread

family: family.c
	gcc family.c -o family -pthread
