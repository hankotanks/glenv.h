#include <glenv.h>
#include <stdio.h>

int main() {
    RGFW_window* win = RGFW_createWindow("a window", RGFW_RECT(0, 0, 800, 600), RGFW_windowCenter | RGFW_windowNoResize);
    if(glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return 1;
    }
    return 0;
}