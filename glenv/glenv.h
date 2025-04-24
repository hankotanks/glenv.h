#ifndef __GLENV_H__
#define __GLENV_H__

#include <GL/glew.h>
#include <RGFW.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

#ifndef GLENV_TEXT_BUFFER_SIZE
#define GLENV_TEXT_BUFFER_SIZE 1024
#endif

#ifndef GLENV_DOUBLE_CLICK_LO
#define GLENV_DOUBLE_CLICK_LO 0.02
#endif

#ifndef GLENV_DOUBLE_CLICK_HI
#define GLENV_DOUBLE_CLICK_HI 0.2
#endif

#ifndef GLENV_SEGMENT_COUNT
#define GLENV_SEGMENT_COUNT 22
#endif

typedef enum { STATUS_OK = 0, STATUS_ERR = 1 } glenv_status;

typedef struct {
    struct nk_buffer cmd;
    struct nk_draw_null_texture tex_null;
    GLuint tex_font_atlas;
} glenv_Device;

typedef struct {
    GLsizei w, h;
} glenv_Size;

typedef struct {
    float pos[2];
    float uv[2];
    nk_byte col[4];
} glenv_Vertex;

static struct {
    RGFW_window* win;
    glenv_Size win_size, viewport_size;
    glenv_Device device;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 scale;
    unsigned int text[GLENV_TEXT_BUFFER_SIZE], text_len;
    struct nk_vec2 scroll;
} glenv_WindowHandler;

NK_INTERN void glenv_device_upload_atlas(const void* image, glenv_Size atlas_size);
NK_API void glenv_render(enum nk_anti_aliasing AA);
NK_API void glenv_key_callback(RGFW_window* win, unsigned char key, char ch, unsigned char lock_state, RGFW_bool pressed);
NK_API void glenv_scroll_callback(RGFW_window* win, double x_off, double y_off);
NK_API void glenv_mouse_button_callback(RGFW_window* win, unsigned char button, double scroll, RGFW_bool pressed);
NK_API struct nk_context* glenv_init(RGFW_window* win);
NK_API void glenv_font_stash_begin(struct nk_font_atlas** atlas);
NK_API void glenv_font_stash_end(void);
NK_API void glenv_new_frame(void);

#endif // __GLENV_H__