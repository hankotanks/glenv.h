CC := gcc

DIR_LIB := ../glenv

all: glenv
	$(CC) demo.c $(shell $(MAKE) get_bin_flags -s -C $(DIR_LIB)) -o demo

glenv:
	$(MAKE) -s -C $(DIR_LIB)

clean:
	$(MAKE) clean -C $(DIR_LIB)

.PHONY: all glenv clean