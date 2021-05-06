ARCHS_C := p2_apartado2.c

OUTPUTS := $(ARCHS_C:.c=.out)

OBJS := $(ARCHS_C:.c=.o)

ARGS := -Wall -Wextra -pedantic -O0 -msse3

all: $(OUTPUTS)

$(OUTPUTS): $(OBJS)
	gcc -g $(@:.out=.o) -o $@ -L $(ARGS)

%.o: %.c
	gcc -g -o $@ -c $< $(ARGS)

clean:
	-find . -type f -name '*.o' -delete
	-find . -type f -name '*.out' -delete
	-rm $(OUTPUTS:.out=)