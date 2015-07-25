// initWindow()
// initGLES()
// loop
//   input
//   update
//   draw
// quitGLES()
// quitWindow()
#define _GLIBCXX_USE_NANOSLEEP 1

#include <iostream>
#include <string>
#include <thread>
#include <cstdlib>
#include <chrono>

// http://content.gpwiki.org/index.php/OpenGL:Tutorials:Setting_up_OpenGL_on_X11

// #include <GLES/gl.h>
// #include <GLES/egl.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

/* Xlib.h is the default header that is included and has the core functionallity
 */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

/* the XF86 Video Mode extension allows us to change the displaymode of the
 * server
 * this allows us to set the display to fullscreen and also read videomodes and
 * other information.
 */
// #include <X11/extensions/xf86vmode.h>

#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <fcntl.h>

static Display* display;
static Window window;
static EGLDisplay eglDisplay;
static EGLContext eglContext;
static EGLSurface eglSurface;

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint loadShader(GLenum type, const char* shaderSrc)
{
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

  if (!compiled)
  {
    GLint infoLen = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

    if (infoLen > 1)
    {
      char* infoLog = (char*)malloc(sizeof(char) * infoLen);

      glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
      std::cerr << "Error compiling shader:" << infoLog << std::endl;

      free(infoLog);
    }

    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader)
{
  GLuint programObject = glCreateProgram();

  if (programObject == 0)
    return 0;

  glAttachShader(programObject, vertexShader);
  glAttachShader(programObject, fragmentShader);

  glBindAttribLocation(programObject, 0, "vPosition");

  // Link the program
  glLinkProgram(programObject);

  GLint linked;

  // Check the link status
  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

  if (!linked)
  {
    GLint infoLen = 0;

    glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

    if (infoLen > 1)
    {
      char* infoLog = (char*)malloc(sizeof(char) * infoLen);

      glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
      std::cerr << "Error linking program:" << infoLog << std::endl;

      free(infoLog);
    }

    glDeleteProgram(programObject);
    return 0;
  }

  return programObject;
}

bool initX11Window()
{
  display = XOpenDisplay(0);
  XSetWindowAttributes swa;
  swa.override_redirect = True;
  swa.event_mask =
      ExposureMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
  window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 800, 480, 0,
                         CopyFromParent, InputOutput, CopyFromParent,
                         CWBorderPixel | CWEventMask, &swa);

  if (window == 0)
  {
    std::cout << "Error: failed to create window." << std::endl;
    return false;
  }

  XSetWindowAttributes xattr;
  xattr.override_redirect = false;
  XChangeWindowAttributes(display, window, CWOverrideRedirect, &xattr);

  XMapWindow(display, window);

  XStoreName(display, window, "Pandora Test");
  return true;
}

bool initGLES()
{
  // https://code.google.com/p/opengles-book-samples/source/browse/trunk/LinuxX11/Common/esUtil.c
  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;
  EGLConfig config;
  EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE,
                              EGL_NONE };

  EGLint attribList[] = { EGL_RED_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_BLUE_SIZE, 5,
                          EGL_DEPTH_SIZE, 16, EGL_STENCIL_SIZE, 8,
                          EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
                          EGL_OPENGL_ES2_BIT, EGL_NONE };

  // Get Display
  // eglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
  eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglDisplay == EGL_NO_DISPLAY)
  {
    std::cout << "Error could not create egl display." << std::endl;
    return false;
  }

  // Initialize EGL
  if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion))
  {
    std::cout << "Error could not init egl." << std::endl;
    return false;
  }

  // Get configs
  if (!eglGetConfigs(eglDisplay, NULL, 0, &numConfigs))
  {
    std::cout << "Error could not get egl configs." << std::endl;
    return false;
  }

  // Choose config
  if (!eglChooseConfig(eglDisplay, attribList, &config, 1, &numConfigs))
  {
    std::cout << "Error could not choose egl config." << std::endl;
    return false;
  }

  // Create a surface
  // eglSurface = eglCreateWindowSurface(eglDisplay, config,
  // (EGLNativeWindowType)window, 0);
  eglSurface = eglCreateWindowSurface(eglDisplay, config,
                                      (EGLNativeWindowType)NULL, NULL);
  if (eglSurface == EGL_NO_SURFACE)
  {

    std::cout << "Error could not create egl surface: " << eglGetError()
              << std::endl;
    return false;
  }

  // eglBindAPI(EGL_OPENGL_ES_API);

  // Create a GL context
  eglContext =
      eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
  if (eglContext == EGL_NO_CONTEXT)
  {
    std::cout << "Error could not create egl context: " << eglGetError()
              << std::endl;
    return false;
  }

  // Make the context current
  if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
  {
    std::cout << "Error could not make egl context current." << std::endl;
    return false;
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glViewport(0, 0, 800, 480);

  glClear(GL_COLOR_BUFFER_BIT);

  return true;
}

bool init()
{
  if (!initX11Window())
  {
    return false;
  }

  if (!initGLES())
  {
    return false;
  }

  return true;
}

void shutdownGLES()
{
  eglMakeCurrent(eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
  eglDestroySurface(eglDisplay, eglSurface);
  eglDestroyContext(eglDisplay, eglContext);
  eglSurface = nullptr;
  eglContext = nullptr;
  eglTerminate(eglDisplay);
  eglDisplay = nullptr;
}

void shutdownX11Window()
{
  XDestroyWindow(display, window);
  XCloseDisplay(display);
}

void shutdown()
{
  shutdownGLES();
  shutdownX11Window();
}

int main(int argc, char** argv)
{
  if (!init())
  {
    return EXIT_FAILURE;
  }
  // int screen = DefaultScreen(display);
  // int vmMajor, vmMinor;
  // XF86VidModeQueryVersion(display, &vmMajor, &vmMinor);

  // std::cout << "XF86 VideoMode extension version " << vmMajor << "." <<
  // vmMinor
  //           << std::endl;
  // XF86VidModeModeInfo** modes;
  // int modeNum;
  // XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
  // for (int i = 0; i < modeNum; ++i)
  // {
  //   std::cout << "Video mode number " << i << ": " << modes[i]->hdisplay <<
  //   "x"
  //             << modes[i]->vdisplay << "." << std::endl;
  // }

  // XF86VidModeSetViewPort(Display *, int, int, int)

  // XFree(modes);

  // XMapRaised(display, window);

  // int x = 1;

  //  for (int i = 0; i < 100000000; i++)
  //  {
  //    x *= i;
  //  }

  //  std::cout << x << std::endl;

  //  usleep(3000000);

  std::string vShaderStr = "attribute vec4 vPosition;    \n"
                           "void main()                  \n"
                           "{                            \n"
                           "   gl_Position = vPosition;  \n"
                           "}                            \n";

  std::string fShaderStr = "precision mediump float;\n"
                           "void main()                                  \n"
                           "{                                            \n"
                           "  gl_FragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );\n"
                           "}                                            \n";

  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vShaderStr.c_str());
  GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, fShaderStr.c_str());

  GLuint shaderProgram = createProgram(vertexShader, pixelShader);

  glUseProgram(shaderProgram);

  GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f,  -0.5f, -0.5f,
                          0.0f, 0.5f, -0.5f, 0.0f };

  // Load the vertex data
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  bool running = true;
  XEvent event;

  while (running)
  {
    while (XPending(display) > 0)
    {
      XNextEvent(display, &event);
      switch (event.type)
      {
        case KeyPress:
          switch (event.xkey.keycode)
          {
            case 65:  // space button
            case 147: // pandora button
            {
              running = false;
              break;
            }
            case 111: // D-Pad up
            {
              vVertices[1] -= 0.01f;
              vVertices[4] -= 0.01f;
              vVertices[7] -= 0.01f;
              break;
            }
            case 116: // D-Pad down
            {
              vVertices[1] += 0.01f;
              vVertices[4] += 0.01f;
              vVertices[7] += 0.01f;
              break;
            }
            case 113: // D-Pad right
            {
              vVertices[0] -= 0.01f;
              vVertices[3] -= 0.01f;
              vVertices[6] -= 0.01f;
              break;
            }
            case 114: // D-Pad left
            {
              vVertices[0] += 0.01f;
              vVertices[3] += 0.01f;
              vVertices[6] += 0.01f;
              break;
            }
            default:
            {
              std::cout << "keycode: " << event.xkey.keycode << std::endl;
              std::this_thread::sleep_for(std::chrono::seconds(1));
              running = false;
              break;
            }
          }
          break;
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    // glEnableVertexAttribArray(0);

    // std::cout << "somthing\n";

    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(eglDisplay, eglSurface);
  }

  shutdown();
  return EXIT_SUCCESS;
}
