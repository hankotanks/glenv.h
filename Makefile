CC := gcc

all: 
	$(CC) demo.c $(shell $(MAKE) -s -C glenv) -o demo

clean:
	$(MAKE) clean -C glenv

.PHONY: all clean