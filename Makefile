CC = gcc
CFLAGS = -std=gnu99 -Wall -O0
LFLAGS = -lgthread-2.0 -lcairo -lgsl -lgslcblas -lm
OBJS = spring_ode.o

all : $(OBJS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@ `pkg-config --cflags --libs gtk+-3.0`
	$(CC) -o $* $@ $(LFLAGS) `pkg-config --cflags --libs gtk+-3.0`

clean :
	@rm -f $(OBJS) $(OBJS:.o=)

.PHONY : clean
