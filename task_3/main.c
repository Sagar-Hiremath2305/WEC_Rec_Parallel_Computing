#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>

// Cell states: 0 = Nothing (N), 1 = Healthy (H), 2 = Burning (B)
void print_grid(const int *grid, int M) {
    for (int y = 0; y < M; y++) {
        for (int x = 0; x < M; x++) {
            int val = grid[y * M + x];
            char c = (val == 2) ? 'B' : (val == 1) ? 'H' : '.';
            printf("%c ", c);
        }
        printf("\n");
    }
    printf("----------------------------------------\n");
}

GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
    return shader;
}

char* read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(length + 1);
    fread(buf, 1, length, f);
    buf[length] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    int M = (argc > 1) ? atoi(argv[1]) : 15;
    
    // 1. Initialising EGL
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);

    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_NON_CONFORMANT_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);

    // 2.Shader
    char *shader_src = read_file("forest_fire.comp");
    if (!shader_src) {
        fprintf(stderr, "Failed to load forest_fire.comp\n");
        return 1;
    }
    GLuint computeShader = compile_shader(GL_COMPUTE_SHADER, shader_src);
    free(shader_src);

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    glUseProgram(program);

    // 3. Initialize Grid Data
    size_t grid_bytes = M * M * sizeof(int);
    int *initial_grid = malloc(grid_bytes);
    srand(time(NULL));

    int burning_count = 0;
    for (int i = 0; i < M * M; i++) {
        int r = rand() % 100;
        if (r < 5) {
            initial_grid[i] = 2; // Burning
            burning_count++;
        } else if (r < 65) {
            initial_grid[i] = 1; // Healthy
        } else {
            initial_grid[i] = 0; // Nothing
        }
    }

    // Ensure at least one burning tree exists
    if (burning_count == 0) {
        initial_grid[0] = 2;
    }

    if (M <= 20) {
        printf("Initial Grid (Epoch 0):\n");
        print_grid(initial_grid, M);
    }

    // 4.Creating SSBOs
    GLuint ssbo[2];
    glGenBuffers(2, ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, grid_bytes, initial_grid, GL_DYNAMIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, grid_bytes, NULL, GL_DYNAMIC_READ);

    GLint size_loc = glGetUniformLocation(program, "u_grid_size");
    GLint seed_loc = glGetUniformLocation(program, "u_seed");

    int epochs = 0;
    int current_buffer = 0;

    // 5.Simulation Loop
    while (true) {
        glUniform1i(size_loc, M);
        glUniform1ui(seed_loc, (GLuint)(rand()));

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[current_buffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1 - current_buffer]);

        GLuint num_groups_x = (M + 15) / 16;
        GLuint num_groups_y = (M + 15) / 16;
        glDispatchCompute(num_groups_x, num_groups_y, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        epochs++;

        // Read back result to check if any burning cells remain
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1 - current_buffer]);
        int *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, grid_bytes, GL_MAP_READ_BIT);
        
        burning_count = 0;
        for (int i = 0; i < M * M; i++) {
            if (ptr[i] == 2) burning_count++;
        }

        if (M <= 20) {
            printf("Epoch %d:\n", epochs);
            print_grid(ptr, M);
        }

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        current_buffer = 1 - current_buffer;

        if (burning_count == 0) {
            break;
        }
    }

    printf("Simulation finished! Total epochs to extinguish: %d for grid size M = %d\n", epochs, M);

    // Cleanup
    glDeleteBuffers(2, ssbo);
    glDeleteProgram(program);
    eglDestroyContext(display, context);
    free(initial_grid);
    return 0;
}