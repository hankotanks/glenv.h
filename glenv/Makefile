CC := gcc
AR := ar

ROOT := $(notdir $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

ifeq ($(OS),Windows_NT)
    LIB := glenv.lib
    LIBGL := -lopengl32 -lgdi32 -luser32 -lkernel32
else
    LIB := libglenv.a
    LIBGL := -lGL
endif

CFLAGS := -isystem $(CURDIR)
CFLAGSGL := -DGLEW_NO_GLU -DGLEW_STATIC

all: $(LIB)
	$(info -isystem $(ROOT) -L$(ROOT) -lglenv $(LIBGL) -fgnu89-inline $(CFLAGSGL))

$(LIB): glenv.o glew.o
	$(AR) rcs $@ $^

glenv.o: glenv.c
	$(CC) $(CFLAGS) -c $< -o $@

glew.o: glew.c
	$(CC) $(CFLAGS) $(CFLAGSGL) -c $< -o $@ $(LIBGL)

clean:
	$(RM) *.o

.PHONY: all clean