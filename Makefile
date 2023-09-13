Target = main
CC = gcc
incDIR = ./inc
srcDIR = ./src
objDIR = ./obj
Depend = $(wildcard $(incDIR)/*.h)
Source = $(wildcard $(srcDIR)/*.c)
Object = $(patsubst $(srcDIR)/%.c, $(objDIR)/%.o, $(Source))
CFLAGS = -I $(incDIR)

CLI_Source = $(filter-out ./src/main.c, $(Source))

#all
all:$(Target) cli

cli: cli.c
	$(CC) -static $^ $(CLI_Source) $(CFLAGS) -o cli -g


#MAIN
$(Target):$(Object)
	$(CC) $^ -o $@
	

#OBJ
$(objDIR)/%.o: $(srcDIR)/%.c $(Depend)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY:clean
clean:
	rm -f $(Target)
	rm -f $(Object)
	rm -f cli


