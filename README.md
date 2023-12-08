# Project-3-Group-7

Building a FAT32 img system.

## Table of Contents

-[Project 3]
	-[Group Members]
	-[How to Use the Makefile]
	-[File List]
	-[Division of Labor]
	-[Completed extra credit]
	-[Possible bugs]

## Group Members

Sophia Elliott, Ivan Quinones, & Emily Cleveland

## How to Use the Makefile

User must ensure that they are in the main 'project-3-group-7' folder. Inside this folder run the command 'make', then 'cd bin', and then in here run './filesys fat32.img'. Make sure that that image file is in bin directory after 'make' in order to mount.

## File List

We solely used 'filesys.c' for all of the functionality of our FAT32 Img system. In addition, we used the provided 'lexer.c' and 'lexer.h' files for provided funtionality of the tokens list.

## File Layout
├── Makefile
├── README.md
    └── src
        ├── filesys.c
        ├── lexer.c
        └── lexer.h

After making: 

├── Makefile
├── README.md
    └── src
        ├── filesys.c
        ├── lexer.c
        └── lexer.h
    └── bin
        ├── filesys
        ├── lexer.o
        └── filesys.o

## Division of Labor

All groups members worked collaboratively on all parts of this project.

## Completed
- Mounting
- info
- exit
- cd [DIRNAME]
- ls 
- open [FILENAME] [FLAGS]
- close [FILENAME] 
- lsof
- lseek [FILENAME] [OFFSET]
- read [FILENAME] [SIZE]

## Completed Extra Credit

None.

## Possible bugs

N/A