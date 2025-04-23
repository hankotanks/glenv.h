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

typedef enum { STATUS_OK = 0, STATUS_ERR = 1 } glenv_status;

typedef struct {
    struct nk_buffer cmd;
    struct nk_draw_null_texture tex_null;
    GLuint tex_font_atlas;
} glenv_Device;

typedef struct {
    GLsizei w, h
} glenv_Size;

typedef static struct {
    RGFW_window* win;
    glenv_Size win_size, viewport_size;
    glenv_Device device;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 scale;
    unsigned int text[GLENV_TEXT_BUFFER_SIZE], text_len;
    struct nk_vec2 scroll;
} glenv_WindowHandler;

NK_INTERN void nk_RGFW_device_upload_atlas(const void* image, glenv_Size atlas_size) {
    struct nk_RGFW_device* device = &(glenv_WindowHandler.device);
    glGenTextures(1, &(device->tex_font_atlas));
    glBindTexture(GL_TEXTURE_2D, device->tex_font_atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_size.w, atlas_size.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void nk_RGFW_render(enum nk_anti_aliasing AA) {
    // initialize global state
    struct nk_RGFW_device* device = &(glenv_WindowHandler.device);
#ifndef __EMSCRIPTEN__
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // configure viewport
    glViewport(0,0,(GLsizei) RGFW.display_width,(GLsizei)RGFW.display_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, RGFW.width, RGFW.height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    {
        GLsizei vs = sizeof(struct nk_RGFW_vertex);
        size_t vp = offsetof(struct nk_RGFW_vertex, position);
        size_t vt = offsetof(struct nk_RGFW_vertex, uv);
        size_t vc = offsetof(struct nk_RGFW_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_RGFW_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_RGFW_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_RGFW_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_RGFW_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_RGFW_vertex);
        config.tex_null = dev->tex_null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&RGFW.ctx, &dev->cmds, &vbuf, &ebuf, &config);

        /* setup vertex buffer pointer */
        {const void *vertices = nk_buffer_memory_const(&vbuf);
        glVertexPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vp));
        glTexCoordPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vt));
        glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*)((const nk_byte*)vertices + vc));}

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);
        nk_draw_foreach(cmd, &RGFW.ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * RGFW.fb_scale.x),
                (GLint)((RGFW.height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * RGFW.fb_scale.y),
                (GLint)(cmd->clip_rect.w * RGFW.fb_scale.x),
                (GLint)(cmd->clip_rect.h * RGFW.fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&RGFW.ctx);
        nk_buffer_clear(&dev->cmds);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    /* default OpenGL state */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    #ifndef __EMSCRIPTEN__
    glPopAttrib();
    #endif
}

#endif // __GLENV_H__