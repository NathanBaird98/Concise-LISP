# Concise-LISP
A minimal LISP programming langauge built using C.

Following the "**Build Your Own Lisp**" Online eBook (https://buildyourownlisp.com/).

## Build instructions:
Linux & Mac: `cc -std=c99 -Wall main.c ../include/mpc.c -ledit -lm -o main`\
Windows: `cc -std=99 -Wall main.c ../include/mpc.c -o main`
	
## Making use of the following libraries:
- Editline library by [troglobit](https://github.com/troglobit/editline1)
- Micro Parser Combinators by [orangeduck](https://github.com/orangeduck/mpc)
