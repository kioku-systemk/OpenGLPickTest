
#ifdef _WIN32
#include <windows.h>
#endif

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#define PI 3.1415926

mat4x4 viewMat;
mat4x4 worldMat;
mat4x4 projMat;
int viewport[4];
int winSize[2];

const float INFINITY_T = 1.e10f;

static const struct
{
    vec3 pos;
    vec2 uv;
} vertices[4] =
{
    { {-0.6f, -0.4f, 0.f}, {0.f, 0.f} },
    { { 0.6f, -0.4f, 0.f}, {1.f, 0.f} },
    { {-0.6f,  0.4f, 0.f}, {0.f, 1.f} },
    { { 0.6f,  0.4f, 0.f}, {1.f, 1.f} }
};
int indices[2][3] = {
    {0,1,2},
    {2,1,3}
};

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
    in[0]=(winx)/(float)winSize[0]*2.0-1.0;
    in[1]=(winy)/(float)winSize[1]*2.0-1.0;
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

float intersectTriangle(vec3 orig, vec3 dir, vec4 vertices[3], float lastHitT, vec2 coord)
{
    vec3 u, v, n; // triangle vectors
    vec3 w0, w;  // ray vectors
    float r, a, b; // params to calc ray-plane intersect
    
    // get triangle edge vectors and plane normal
    vec3_sub(u, vertices[1], vertices[0]);
    vec3_sub(v, vertices[2], vertices[0]);
    vec3_mul_cross(n, u, v);
    
    vec3_sub(w0, orig, vertices[0]);
    a = -vec3_mul_inner(n, w0);
    b = vec3_mul_inner(n, dir);
    if (abs(b) < 1e-5)
    {
        // ray is parallel to triangle plane, and thus can never intersect.
        return INFINITY_T;
    }
    
    // get intersect point of ray with triangle plane
    r = a / b;
    if (r < 0.0)
        return INFINITY_T; // ray goes away from triangle.
    
    vec3 I, rdir;
    vec3_scale(rdir, dir, r);
    vec3_add(I,orig, rdir);
    float uu, uv, vv, wu, wv, D;
    uu = vec3_mul_inner(u, u);
    uv = vec3_mul_inner(u, v);
    vv = vec3_mul_inner(v, v);
    vec3_sub(w, I, vertices[0]);
    wu = vec3_mul_inner(w, u);
    wv = vec3_mul_inner(w, v);
    D = uv * uv - uu * vv;
    
    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0)
        return INFINITY_T;
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0)
        return INFINITY_T;
    
    coord[0] = s;
    coord[1] = t;
    
    return (r > 1e-5 || r < lastHitT) ? r : INFINITY_T;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    // calc view space ray dir
    vec3 orgpos, endpos, raydir;
    float screenCoord[2] = {(float)xpos, (float)winSize[1] - (float)ypos};
    //fprintf(stdout, "Pos: (%lf,%lf)\n", screenCoord[0], screenCoord[1]);
    
    if (screenCoord[0] < 0 || screenCoord[0] > winSize[0]
    ||  screenCoord[1] < 0 || screenCoord[1] > winSize[1]) {
        return;
    }
    
    glhUnProjectf(screenCoord[0], screenCoord[1], -1.f, viewMat, projMat, viewport, orgpos); // on near plane
    glhUnProjectf(screenCoord[0], screenCoord[1],  1.f, viewMat, projMat, viewport, endpos);   // on far plane
    vec3_sub(raydir, endpos, orgpos);
    //fprintf(stdout, "s: (%f,%f,%f)\n", raydir[0], raydir[1], raydir[2]);
    
    float lastT = INFINITY_T;
    int hitIndex = -1;
    vec2 hitCoord;
    for (int t = 0; t < 2; ++t) {
        // calc view space triangles
        vec4 viewtri[3];
        vec4 worldtri[3];
        for (int i = 0; i < 3; ++i) {
            worldtri[i][0] = vertices[indices[t][i]].pos[0];
            worldtri[i][1] = vertices[indices[t][i]].pos[1];
            worldtri[i][2] = vertices[indices[t][i]].pos[2];
            worldtri[i][3] = 1.0;
            mat4x4_mul_vec4(viewtri[i], worldMat, worldtri[i]);
        }
        
        // intersect ray and triangle
        vec2 coord; // barycentric coordinates
        float hitT = intersectTriangle(orgpos, raydir, viewtri, lastT, coord);
        if (hitT < INFINITY_T) {
            hitIndex = t;
            hitCoord[0] = coord[0];
            hitCoord[1] = coord[1];
        }
    }
    
    if (hitIndex != -1) {
        //fprintf(stdout, "Hit Index=%d coord=(%f,%f)\n", hitIndex, hitCoord[0], hitCoord[1]);
        
        // calc UV from barycentric coordinates
        vec2 v0, v1, uv0, uv1, uv;
        vec2_sub(v0, vertices[indices[hitIndex][1]].uv,vertices[indices[hitIndex][0]].uv);
        vec2_sub(v1, vertices[indices[hitIndex][2]].uv,vertices[indices[hitIndex][0]].uv);
        vec2_scale(uv0, v0, hitCoord[0]);
        vec2_scale(uv1, v1, hitCoord[1]);
        vec2_add(uv, uv0, uv1);
        vec2_add(uv, uv, vertices[indices[hitIndex][0]].uv);
        
        fprintf(stdout, "Hit Index=%d UV=(%f,%f)\n", hitIndex, uv[0], uv[1]);
    }
    
    
}

//--------------------------

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

void resize_callback(GLFWwindow *window, int width, int height)
{
    winSize[0] = width;
    winSize[1] = height;

    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);
    viewport[2] = renderBufferWidth;
    viewport[3] = renderBufferHeight;
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);
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
    winSize[0] = 640;
    winSize[1] = 480;
    window = glfwCreateWindow(winSize[0], winSize[1], "PickTest", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowSizeCallback(window, resize_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL();
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
        mat4x4_rotate_Z(m, m, (float) glfwGetTime() * 0.1);
        mat4x4_rotate_Y(m, m, (float) glfwGetTime() * 0.2);
        mat4x4_translate(v, 0.f, 0.f, -2.f);
        mat4x4_perspective(p, 60.0/180.f*PI, ratio, 0.1, 10.0f);
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
