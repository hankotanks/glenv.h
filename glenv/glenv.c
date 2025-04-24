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

NK_INTERN void glenv_device_upload_atlas(const void* image, glenv_Size atlas_size) {
    glenv_Device* device = &(glenv_WindowHandler.device);
    glGenTextures(1, &(device->tex_font_atlas));
    glBindTexture(GL_TEXTURE_2D, device->tex_font_atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_size.w, atlas_size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void glenv_render(enum nk_anti_aliasing AA) {
    // setup glenv_Device
    glenv_Device* device = &(glenv_WindowHandler.device);
#ifndef __EMSCRIPTEN__
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // viewport configuration
    glenv_Size win_size, viewport_size;
    viewport_size = glenv_WindowHandler.viewport_size;
    glViewport(0, 0, viewport_size.w, viewport_size.h);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    win_size = glenv_WindowHandler.win_size;
    glOrtho(0.f, win_size.w, win_size.h, 0.f, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    // enable client state
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    { // execute draw
        GLsizei vs = sizeof(glenv_Vertex);
        size_t vp, vt, vc;
        vp = offsetof(glenv_Vertex, pos);
        vt = offsetof(glenv_Vertex, uv);
        vc = offsetof(glenv_Vertex, col);
        // draw from command queue
        const struct nk_draw_command* cmd;
        const nk_draw_index* offset = NULL;
        struct nk_buffer vbuf, ebuf;
        // set up configuration for nk_convert
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            { NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(glenv_Vertex, pos) },
            { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(glenv_Vertex, uv) },
            { NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(glenv_Vertex, col) } ,
            { NK_VERTEX_LAYOUT_END }
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(glenv_Vertex);
        config.vertex_alignment = NK_ALIGNOF(glenv_Vertex);
        config.tex_null = device->tex_null;
        config.circle_segment_count = GLENV_SEGMENT_COUNT;
        config.curve_segment_count = GLENV_SEGMENT_COUNT;
        config.arc_segment_count = GLENV_SEGMENT_COUNT;
        config.global_alpha = 1.f;
        config.shape_AA = AA;
        config.line_AA = AA;
        // build shape vertices
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&(glenv_WindowHandler.ctx), &(device->cmd), &vbuf, &ebuf, &config);
        { // setup vertex buffer pointer
            const void* vertices = nk_buffer_memory_const(&vbuf);
            // add offsets to pointers
            const nk_byte* vert_vp = (const nk_byte*) vertices + vp;
            const nk_byte* vert_vt = (const nk_byte*) vertices + vt;
            const nk_byte* vert_vc = (const nk_byte*) vertices + vc;
            glVertexPointer(2, GL_FLOAT, vs, (const void*) vert_vp);
            glTexCoordPointer(2, GL_FLOAT, vs, (const void*) vert_vt);
            glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*) vert_vc);
        }
        // execute each draw command
        offset = (const nk_draw_index*) nk_buffer_memory_const(&ebuf);
        nk_draw_foreach(cmd, &(glenv_WindowHandler.ctx), &(device->cmd)) {
            if(!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint) cmd->texture.id);
            struct nk_vec2 scale = glenv_WindowHandler.scale;
            // calculate scissor dimensions
            GLint x, y, w, h;
            x = (GLint) (cmd->clip_rect.x * scale.x);
            y = (GLint) ((win_size.h - (GLint) (cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y);
            w = (GLint) (cmd->clip_rect.w * scale.x);
            h = (GLint) (cmd->clip_rect.h * scale.y);
            glScissor(x, y, w, h);
            // draw elements
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        // clean up
        nk_clear(&(glenv_WindowHandler.ctx));
        nk_buffer_clear(&(device->cmd));
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }
    // restore default OpenGL state
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    // unbind textures, etc..
    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
#ifndef __EMSCRIPTEN__
    glPopAttrib();
#endif
}

NK_API void glenv_key_callback(RGFW_window* win, unsigned char key, char ch, unsigned char lock_state, RGFW_bool pressed) {
    RGFW_UNUSED(lock_state); 
    RGFW_UNUSED(key); 
    RGFW_UNUSED(win);
    if(pressed == RGFW_FALSE) return;
    unsigned int* text_len = &(glenv_WindowHandler.text_len);
    if((*text_len) < GLENV_TEXT_BUFFER_SIZE) glenv_WindowHandler.text[(*text_len)++] = ch;
}

NK_API void glenv_scroll_callback(RGFW_window* win, double x_off, double y_off) {
    (void) win; 
    (void) x_off;
    glenv_WindowHandler.scroll.x += (float) x_off;
    glenv_WindowHandler.scroll.y += (float) y_off;
}

NK_API void glenv_mouse_button_callback(RGFW_window* win, unsigned char button, double scroll, RGFW_bool pressed) {
    if(button != RGFW_mouseLeft && button < RGFW_mouseScrollUp) return;
    if(button >= RGFW_mouseScrollUp) return glenv_scroll_callback(win, 0, scroll);
}

NK_API struct nk_context* glenv_init(RGFW_window* win) {
    glenv_WindowHandler.win = win;
    RGFW_setKeyCallback(glenv_key_callback);
    RGFW_setMouseButtonCallback(glenv_mouse_button_callback);
    nk_init_default(&(glenv_WindowHandler.ctx), 0);
    nk_buffer_init_default(&(glenv_WindowHandler.device.cmd));
    return &(glenv_WindowHandler.ctx);
}

NK_API void glenv_font_stash_begin(struct nk_font_atlas** atlas) {
    nk_font_atlas_init_default(&(glenv_WindowHandler.atlas));
    nk_font_atlas_begin(&(glenv_WindowHandler.atlas));
    *atlas = &(glenv_WindowHandler.atlas);
}

NK_API void nk_RGFW_font_stash_end(void) {
    struct nk_font_atlas* atlas = &(glenv_WindowHandler.atlas);
    GLsizei w, h;
    const void* image = nk_font_atlas_bake(atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    glenv_Size atlas_size = { .w = w, .h = h };
    glenv_device_upload_atlas(image, atlas_size);
    glenv_Device* device = &(glenv_WindowHandler.device);
    nk_handle tex_handle = nk_handle_id((int) (device->tex_font_atlas));
    nk_font_atlas_end(&(glenv_WindowHandler.atlas), tex_handle, &(device->tex_null));
    const struct nk_user_font* font = &(glenv_WindowHandler.atlas.default_font->handle);
    if(glenv_WindowHandler.atlas.default_font) nk_style_set_font(&(glenv_WindowHandler.ctx), font);
}

NK_API void nk_RGFW_new_frame(void) {
    int i;
    double x, y;
    struct nk_context* ctx = &(glenv_WindowHandler.ctx);
    struct RGFW_window* win = glenv_WindowHandler.win;
    // update window dimensions
    glenv_Size* win_size = &(glenv_WindowHandler.win_size);
    win_size->w = win->r.w;
    win_size->h = win->r.h;
    // update viewport dimensions
    RGFW_area screen_size = RGFW_getScreenSize();
    glenv_Size* viewport_size = &(glenv_WindowHandler.viewport_size);
    viewport_size->w = (GLsizei) screen_size.w;
    viewport_size->h = (GLsizei) screen_size.h;
    // update scale factor
    glenv_WindowHandler.scale.x = (float) viewport_size->w / (float) win_size->w;
    glenv_WindowHandler.scale.y = (float) viewport_size->h / (float) win_size->h;
    // begin event handling
    nk_input_begin(ctx);
    // capture key presses
    for (i = 0; i < glenv_WindowHandler.text_len; ++i) {
        nk_input_unicode(ctx, glenv_WindowHandler.text[i]);
    }
    // forward non-char keys to nk
    nk_input_key(ctx, NK_KEY_DEL, RGFW_isPressed(win, RGFW_delete));
    nk_input_key(ctx, NK_KEY_ENTER, RGFW_isPressed(win, RGFW_return));
    nk_input_key(ctx, NK_KEY_TAB, RGFW_isPressed(win, RGFW_tab));
    nk_input_key(ctx, NK_KEY_BACKSPACE, RGFW_isPressed(win, RGFW_backSpace));
    nk_input_key(ctx, NK_KEY_UP, RGFW_isPressed(win, RGFW_up));
    nk_input_key(ctx, NK_KEY_DOWN, RGFW_isPressed(win, RGFW_down));
    nk_input_key(ctx, NK_KEY_TEXT_START, RGFW_isPressed(win, RGFW_home));
    nk_input_key(ctx, NK_KEY_TEXT_END, RGFW_isPressed(win, RGFW_end));
    nk_input_key(ctx, NK_KEY_SCROLL_START, RGFW_isPressed(win, RGFW_home));
    nk_input_key(ctx, NK_KEY_SCROLL_END, RGFW_isPressed(win, RGFW_end));
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, RGFW_isPressed(win, RGFW_pageDown));
    nk_input_key(ctx, NK_KEY_SCROLL_UP, RGFW_isPressed(win, RGFW_pageUp));
    nk_input_key(ctx, NK_KEY_SHIFT, RGFW_isPressed(win, RGFW_shiftL) || RGFW_isPressed(win, RGFW_shiftR));
    // key modifiers
    if(RGFW_isPressed(win, RGFW_controlL) || RGFW_isPressed(win, RGFW_controlR)) {
        nk_input_key(ctx, NK_KEY_COPY, RGFW_isPressed(win, RGFW_c));
        nk_input_key(ctx, NK_KEY_PASTE, RGFW_isPressed(win, RGFW_v));
        nk_input_key(ctx, NK_KEY_CUT, RGFW_isPressed(win, RGFW_x));
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, RGFW_isPressed(win, RGFW_z));
        nk_input_key(ctx, NK_KEY_TEXT_REDO, RGFW_isPressed(win, RGFW_r));
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, RGFW_isPressed(win, RGFW_left));
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, RGFW_isPressed(win, RGFW_right));
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, RGFW_isPressed(win, RGFW_b));
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, RGFW_isPressed(win, RGFW_e));
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, RGFW_isPressed(win, RGFW_left));
        nk_input_key(ctx, NK_KEY_RIGHT, RGFW_isPressed(win, RGFW_right));
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
    }
    // mouse motion
    RGFW_point p = RGFW_window_getMousePoint(glenv_WindowHandler.win);
    nk_input_motion(ctx, p.x, p.y);
    if(ctx->input.mouse.grabbed) {
        ctx->input.mouse.prev.x = (float) p.x;
        ctx->input.mouse.prev.y = (float) p.y;
        // update new position
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
    // mouse clicks
    nk_input_button(ctx, NK_BUTTON_LEFT, p.x, p.y, RGFW_isMousePressed(win, RGFW_mouseLeft));
    nk_input_button(ctx, NK_BUTTON_MIDDLE, p.x, p.y, RGFW_isMousePressed(win, RGFW_mouseMiddle));
    nk_input_button(ctx, NK_BUTTON_RIGHT, p.x, p.y, RGFW_isMousePressed(win, RGFW_mouseRight));
    nk_input_scroll(ctx, glenv_WindowHandler.scroll);
    nk_input_end(&(glenv_WindowHandler.ctx));
    // reset text buffer and scroll vector
    glenv_WindowHandler.text_len = 0;
    glenv_WindowHandler.scroll = nk_vec2(0,0);
}

NK_API void glenv_shutdown(void) {
    glenv_Device* device = &(glenv_WindowHandler.device);
    nk_font_atlas_clear(&(glenv_WindowHandler.atlas));
    nk_free(&(glenv_WindowHandler.ctx));
    glDeleteTextures(1, &(device->tex_font_atlas));
    nk_buffer_free(&(device->cmd));
    memset(&glenv_WindowHandler, 0, sizeof(glenv_WindowHandler));
}