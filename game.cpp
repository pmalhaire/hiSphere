#include <SDL2/SDL.h>
//#include <GLES2/gl2.h>
#include <SDL2/SDL_opengles2.h>
#include <string>
#include <iostream>
#include <cassert>

#include "game.hpp"
#include "sphere.h"

#define enum_case(str)                                                         \
  case str:                                                                    \
    std::cerr << SDL_GetTicks() << "--" << file << ", " << line                \
              << ": " #str "\n";                                               \
    break;

void gl_error_check(std::string file, int line) {
  GLenum err = glGetError();

  switch (err) {
    enum_case(GL_INVALID_ENUM) enum_case(GL_INVALID_VALUE)
        enum_case(GL_INVALID_OPERATION)
            enum_case(GL_INVALID_FRAMEBUFFER_OPERATION)
                enum_case(GL_OUT_OF_MEMORY) default : break;
  }
}

#define GL_ERR_CHECK                                                           \
  do {                                                                         \
    gl_error_check(__FILE__, __LINE__);                                        \
  } while (0)

class game_impl {
public:
  virtual void draw() = 0;
};

void create_circle(GLfloat *circle_vertices, const int vertex_count) {
  int idx = 0;
  for (float i = 0.0f; i < 2 * M_PI;
       i += 2.0 * M_PI / static_cast<float>(vertex_count)) {
    circle_vertices[idx] = cos(i);
    circle_vertices[idx + 1] = sin(i);
    circle_vertices[idx + 2] = 0.0;
#ifdef DEBUG
    printf("i:%f idx:%d x:%f,y:%f,z:%f\n", i, idx, circle_vertices[idx],
           circle_vertices[idx + 1], circle_vertices[idx + 2]);
#endif
    idx += 3;
  }
}

class game_impl3 : public game_impl {

  GLuint program_object_, circle_vertex_buffer, sphere_vertex_buffer;
  Sphere sphere;
  const int n_vertex = 50;
  GLuint LoadShader(GLenum type, const char *shaderSrc) {
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if (shader == 0)
      return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen > 1) {
        char *infoLog = static_cast<char *>(malloc(sizeof(char) * infoLen));
        glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
        fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
        free(infoLog);
      }
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  }
  ///
  // Initialize the shader and program object
  //

  bool Init() {
    const char vShaderStr[] = "attribute vec4 vPosition;   \n"
                              "void main()                 \n"
                              "{                           \n"
                              "   gl_Position = vPosition; \n"
                              "}                           \n";
    const char fShaderStr[] =
#ifdef EMSCRIPTEN
        "precision mediump float;                   \n"
#endif
        "void main()                                \n"
        "{                                          \n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
        "}                                          \n";
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;
    // Load the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);
    // Create the program object
    programObject = glCreateProgram();
    if (programObject == 0)
      return 0;
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    // Bind vPosition to attribute 0
    glBindAttribLocation(programObject, 0, "vPosition");
    // Link the program
    glLinkProgram(programObject);
    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (!linked) {
      GLint infoLen = 0;
      glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen > 1) {
        char *infoLog = static_cast<char *>(malloc(sizeof(char) * infoLen));
        glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
        fprintf(stderr, "Error linking program:\n%s\n", infoLog);
        free(infoLog);
      }
      glDeleteProgram(programObject);
      return false;
    }
    GL_ERR_CHECK;

    // Store the program object
    program_object_ = programObject;
    sphere.set(1.0f, 20, 20, true);
    // GLfloat circle_vertices[n_vertex * 3] = {};
    // create_circle(circle_vertices, n_vertex);
    // glGenBuffers(1, &circle_vertex_buffer);
    // GL_ERR_CHECK;
    // glBindBuffer(GL_ARRAY_BUFFER, circle_vertex_buffer);
    // GL_ERR_CHECK;
    // glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices,
    //              GL_STATIC_DRAW);
    // GL_ERR_CHECK;

    glGenBuffers(1, &sphere_vertex_buffer);
    GL_ERR_CHECK;
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
    GL_ERR_CHECK;
    glBufferData(GL_ARRAY_BUFFER, sphere.getVertexSize() * sizeof(GLfloat),
                 sphere.getVertices(), GL_STATIC_DRAW);
    GL_ERR_CHECK;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_ERR_CHECK;
    return true;
  }

  ///
  // Draw a triangle using the shader pair created in Init()
  //
  void Draw() {
    // Set the viewport
    // Use the program object
    glUseProgram(program_object_);
    GL_ERR_CHECK;

    // // Bind the vertex buffer
    // glBindBuffer(GL_ARRAY_BUFFER, circle_vertex_buffer);
    // GL_ERR_CHECK;

    // // Load the vertex data
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    // GL_ERR_CHECK;
    // glEnableVertexAttribArray(0);
    // GL_ERR_CHECK;
    // glDrawArrays(GL_LINE_LOOP, 0, n_vertex);
    // GL_ERR_CHECK;
    // glEnable(GL_DEPTH_TEST);
    // Bind the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vertex_buffer);
    GL_ERR_CHECK;

    // Load the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    GL_ERR_CHECK;
    glEnableVertexAttribArray(0);
    GL_ERR_CHECK;
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<int>(sphere.getVertexCount()));
    GL_ERR_CHECK;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

public:
  game_impl3() { assert(Init()); }

  void draw() {
    GL_ERR_CHECK;
    Draw();
  }
};

game::game() : me(new game_impl3()) {}

game::~game() {}

void game::draw() { me->draw(); }
