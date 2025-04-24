#include <glenv.h>
#include <stdio.h>

int main() {
    // initialize RGFW_window
    RGFW_window* win = RGFW_createWindow("demo", RGFW_RECT(0, 0, 800, 600), RGFW_windowCenter | RGFW_windowNoResize);
    // initialize glew
    if(glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return 1;
    }
    // initialize glenv_WindowHandler
    struct nk_context* ctx = glenv_init(win);
    // demo ui constants
    const RGFW_area counter_win_size = RGFW_AREA(200, 120);
    const float counter_row_height = 24;
    // demo ui state
    int counter = 0;
    // event loop
    while(!RGFW_window_shouldClose(win)) {
        glenv_new_frame();
        struct nk_rect counter_win_pos = nk_rect(
            (win->r.w - counter_win_size.w) / 2, 
            (win->r.h - counter_win_size.h) / 2, 
            counter_win_size.w, 
            counter_win_size.h
        );
        // draw counter widget
        if(nk_begin(ctx, "Counter", counter_win_pos, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(ctx, counter_row_height, 1);
            nk_labelf(ctx, NK_TEXT_LEFT, "%d", counter);
            nk_layout_row_dynamic(ctx, counter_row_height, 2);
            if(nk_button_label(ctx, "Increment")) ++counter;
            if(nk_button_label(ctx, "Decrement")) --counter;
            
        }
        nk_end(ctx);
        // clear and render
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.1f, 0.18f, 0.24f, 1.f);
        glenv_render(NK_ANTI_ALIASING_ON);
    }
    // clean up
    glenv_deinit();
    RGFW_window_close(win);
    return 0;
}