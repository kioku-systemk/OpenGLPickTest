
//#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

mat4x4 viewMat;
mat4x4 worldMat;
mat4x4 projMat;
int viewport[4];

int glhUnProjectf(float winx, float winy, float winz, mat4x4 view, mat4x4 projection, int *viewport, float *objectCoordinate)
{
    // Transformation matrices
    mat4x4 m, A;
    vec4 in, out;
    // Calculation for inverting a matrix, compute projection x modelview
    mat4x4_mul(A, projection, view);
    
    // Now compute the inverse of matrix A
    mat4x4_invert(m, A);

    // Transformation of normalized coordinates between -1 and 1
    in[0]=(winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0;
    in[1]=(winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
    in[2]=2.0*winz-1.0;
    in[3]=1.0;
    // Objects coordinates
    mat4x4_mul_vec4(out, m, in);
    if(out[3]==0.0)
        return 0;
    out[3]=1.0/out[3];
    objectCoordinate[0]=out[0]*out[3];
    objectCoordinate[1]=out[1]*out[3];
    objectCoordinate[2]=out[2]*out[3];
    return 1;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    //fprintf(stdout, "Pos: (%lf,%lf)\n", xpos, ypos);
    
    // calc screen ray dir
    vec3 startpos, endpos, raydir;
    float screenCoord[2] = {(float)xpos, (float)viewport[3] - (float)ypos};
    glhUnProjectf(screenCoord[0], screenCoord[1], -1.f, viewMat, projMat, viewport, startpos); // on near plane
    glhUnProjectf(screenCoord[0], screenCoord[1],  1.f, viewMat, projMat, viewport, endpos);   // on far plane
    vec3_sub(raydir, endpos, startpos);
    fprintf(stdout, "s: (%f,%f,%f)\n", raydir[0], raydir[1], raydir[2]);
    
    
    // intersect ray and triangle
    
    
}

//--------------------------

static const struct
{
    float x, y, z;
    float u, v;
} vertices[4] =
{
    { -0.6f, -0.4f, 0.f,  0.f, 0.f },
    {  0.6f, -0.4f, 0.f,  1.f, 0.f },
    { -0.6f,  0.4f, 0.f,  0.f, 1.f },
    {  0.6f,  0.4f, 0.f,  1.f, 1.f }
};
static const char* vertex_shader_text =
"#version 110\n"
"uniform mat4 world;\n"
"uniform mat4 view;\n"
"uniform mat4 proj;\n"
"attribute vec2 vUV;\n"
"attribute vec3 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = proj * view * world * vec4(vPos, 1.0);\n"
"    color = vec3(vUV, 1.0);\n"
"}\n";
static const char* fragment_shader_text =
"#version 110\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

bool printShaderInfoLog(GLuint shader)
{
    GLsizei bufSize;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);
    if (bufSize > 1) {
        std::vector<char> infoLog;
        infoLog.resize(bufSize);
        GLsizei length;
        glGetShaderInfoLog(shader, bufSize, &length, &infoLog[0]);
        fprintf(stderr, "InfoLog:\n%s\n\n", &infoLog[0]);
        return false;
    }
    return true;
}
bool printProgramInfoLog(GLuint program)
{
    GLsizei bufSize;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufSize);
    if (bufSize > 1) {
        std::vector<char> infoLog;
        infoLog.resize(bufSize);
        GLsizei length;
        glGetProgramInfoLog(program, bufSize, &length, &infoLog[0]);
        fprintf(stderr, "InfoLog:\n%s\n\n", &infoLog[0]);
        return false;
    }
    return true;
}

int main(void)
{
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint vpos_location, vuv_location;
    GLint world_location, view_location, proj_location;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(640, 480, "PickTest", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwMakeContextCurrent(window);
    //gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    if (!printShaderInfoLog(vertex_shader)) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    if (!printShaderInfoLog(fragment_shader)) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    if (!printProgramInfoLog(program)){
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    world_location = glGetUniformLocation(program, "world");
    view_location  = glGetUniformLocation(program, "view");
    proj_location  = glGetUniformLocation(program, "proj");
    
    vpos_location = glGetAttribLocation(program, "vPos");
    vuv_location = glGetAttribLocation(program, "vUV");
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) 0);
    glEnableVertexAttribArray(vuv_location);
    glVertexAttribPointer(vuv_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 3));
    
    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, v, mv, mvp;
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        viewport[0] = 0;
        viewport[1] = 0;
        viewport[2] = width;
        viewport[3] = height;
        
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) glfwGetTime() * 0.5);
        mat4x4_translate(v, 0.f, 0.f, -5.f);
        mat4x4_perspective(p, 60.0/180.f*M_PI, ratio, 0.1, 10.0f);
        mat4x4_mul(mv, v, m);
        mat4x4_mul(mvp, p, mv);
        
        // store
        memcpy(worldMat, m, sizeof(mat4x4));
        memcpy(viewMat, v,  sizeof(mat4x4));
        memcpy(projMat, p,  sizeof(mat4x4));
        
        glUseProgram(program);
        glUniformMatrix4fv(world_location, 1, GL_FALSE, (const GLfloat*) m);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, (const GLfloat*) v);
        glUniformMatrix4fv(proj_location, 1, GL_FALSE, (const GLfloat*) p);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
