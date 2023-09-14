#!/bin/bash

if [ -z "$1" ]
then
    echo 'no project name!'
    exit 1
fi

val_loop=1

if [ -e "$1" ]; then
    echo "Directory $1 exist!"
    echo -e "Do you want to \033[31;1mdelet\033[0m it"

    while [ $val_loop -eq 1 ]; do
        if [ "$input" != yes ]; then
            read -rp "(yes/no):" input
        fi

                case "${input}" in
            yes|y|Y)
                rm -r "$1"
                echo "$1 deleted"
                val_loop=0
                ;;
            no|n|N)
                echo "exit program"
                exit 0
                ;;
            *)
                echo "error input"
                ;;
        esac
    done
fi

dirname=$1

mkdir -p "${dirname}/src"
mkdir -p "${dirname}/inc"
mkdir -p "${dirname}/obj"

# main.c
echo -e "#include <stdio.h>\n#include<stdlib.h>\n\nint main(int argc, char* argv[])\n{\n\treturn 0;\n}" > "${dirname}/src/main.c"

echo \
"Target = main
CC = gcc
incDIR = ./inc
srcDIR = ./src
objDIR = ./obj
Depend = \$(wildcard \$(incDIR)/*.h)
Source = \$(wildcard \$(srcDIR)/*.c)
Object = \$(patsubst \$(srcDIR)/%.c, \$(objDIR)/%.o, \$(Source))
CFLAGS = -I \$(incDIR)

#all
all:\$(Target)

#MAIN
\$(Target):\$(Object)
	\$(CC) \$^ -o \$@
	

#OBJ
\$(objDIR)/%.o: \$(srcDIR)/%.c \$(Depend)
	\$(CC) -c \$< -o \$@ \$(CFLAGS)

.PHONY:clean
clean:
	rm -f \$(Target)
	rm -f \$(Object)

" >> "${dirname}/Makefile"

cd "${dirname}" || exit

make

echo -e "Project \033[1;32m$1\033[0m created"