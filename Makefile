Target = main
CC = gcc
incDIR = ./inc
srcDIR = ./src
objDIR = ./obj
Depend = $(wildcard $(incDIR)/*.h)
Source = $(wildcard $(srcDIR)/*.c)
Object = $(patsubst $(srcDIR)/%.c, $(objDIR)/%.o, $(Source))
CFLAGS = -I $(incDIR)

#all
all:$(Target)

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


