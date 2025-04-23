#include <glenv.h>
// internal dependencies
#include <GL/glew.h>
#if defined (__WIN32__)
  #define RGFW_USE_XDL
#endif
#define RGFW_IMPLEMENTATION
#include <RGFW.h>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>
// other dependencies
#include <stdio.h>

#define ERR(cond, log) {\
    if(cond) {\
        fprintf(stderr, "ERROR [%s:%d]: ", __FILE__, __LINE__);\
        fprintf(stderr, log);\
        fprintf(stderr, "\n");\
        return STATUS_ERR;\
    }\
}

