Target = main
Target_CLI = cli
CC = gcc
incDIR = ./inc
srcDIR = ./src
objDIR = ./obj
Depend = $(wildcard $(incDIR)/*.h)
Source = $(wildcard $(srcDIR)/*.c)
CFLAGS = -I $(incDIR)

SER_Source = $(filter-out ./src/cli.c, $(Source))
Object = $(patsubst $(srcDIR)/%.c, $(objDIR)/%.o, $(SER_Source))


CLI_Source = $(filter-out ./src/main.c, $(Source))

#all
all:$(Target) $(Target_CLI)

$(Target_CLI): $(CLI_Source)
	$(CC) $(CLI_Source) $(CFLAGS) -o $@ -g


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
	rm -f $(Target_CLI)


