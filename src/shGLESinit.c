/* ******************************************************* */
// Construct and destruct  for X-windows, EGL and opengl2es

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "shGLESinit.h"

// Shared stuff
Display    *x_display;
Window      win;
EGLDisplay  egl_display;
EGLContext  egl_context;
EGLSurface  egl_surface;

// Shader connections
GLuint shaderProgram ;
GLint tflag_loc, position_loc, color4_loc, texc_loc, texs_loc;

// Shaders
const char vertex_src[] = {
    "#version 300 es\n"
    "layout (location = 0) in vec4 position;"
    "layout (location = 1) in vec2 texcoord;"
    "uniform mat4 mview;"
    "uniform mat4 tview;"
    "out vec2 v_texcoord;"
    "void main()"
    "{"
       "vec4 mposition = mview*position;"
       "gl_Position = mposition ;"
       "v_texcoord = vec2(tview*vec4(texcoord.s,texcoord.t, 0.0, 1.0));"
    "}"
};


const char fragment1_src[] = {
   "#version 300 es\n"

   "out mediump vec4 FragColor;"
   "in mediump vec2 v_texcoord;"
   "uniform mediump vec4 color4;"
   "uniform int texGenflag;"
   "uniform mediump sampler2D tex_s;"
   "void main()"
   "{"  "if (texGenflag == 0)"
          "FragColor = color4;"
        "else "
          "FragColor = texture(tex_s,v_texcoord)*color4;"
   "}"
};

const char fragment2_src[] = {
   "#version 300 es\n"

   "out mediump vec4 FragColor;"
   "in mediump vec2 v_texcoord;"
   "uniform mediump vec4 color4;"
   "uniform int texGenflag;"
   "uniform mediump sampler2D tex_s;"
   "void main()"
   "{"  "if (texGenflag == 0)"
          "FragColor = color4;"
        " else "
        "{  if (v_texcoord.t >= 0.0 && v_texcoord.t < 0.4)"
             "FragColor = vec4(0.8,0.0,0.0,1.0)*color4;"
            " else if(v_texcoord.t >= 0.6 && v_texcoord.t <= 1.0)"
             "FragColor = vec4(0.0,0.8,0.0,1.0)*color4;"
            " else FragColor = texture(tex_s,v_texcoord)*color4;"
         "}"

   "}"
};


// Angle; // range 2pi 
// Radius; // range -10000.0 to 1.0
// Center; // range: -1.0 to 3.0

const char fragment3_src[] = {
   "#version 300 es\n"

    "out mediump vec4 FragColor;"
    "in mediump vec2 v_texcoord;"
    "uniform int texGenflag;"
    "uniform mediump vec4 color4;"
    "uniform mediump float Angle;"
    "uniform mediump float Radius;"
    "uniform mediump vec2 Centre;"
    "uniform mediump sampler2D tex_s;"

    "void main()"
    "{"
       "mediump vec2 normCoord; "
       "mediump vec2 f_texcoord = v_texcoord; "

       "if (texGenflag == 0)"
          "FragColor = color4;"
        "else if (texGenflag == 1)"
          "FragColor = texture(tex_s,v_texcoord)*color4;"
         "else if (texGenflag == 2)"
        "{"
    // Shift origin to texture centre (with offset)
      "normCoord.x = f_texcoord.x*2.0 - (Centre.x); "
      "normCoord.y = f_texcoord.y*2.0 - (Centre.y); "
    // Convert Cartesian to Polar coords
      "mediump float r = length(normCoord);"
      "mediump float theta = atan(normCoord.y, normCoord.x);"
    // Rotate
      "r=ceil(r/Radius)*Radius;"
      "theta=floor(theta/Angle)*Angle;"
    // Convert Polar back to Cartesian coords
      "normCoord.x = ((r * cos(theta)) - Centre.x*1.0);"
      "normCoord.y = ((r * sin(theta)) - Centre.y*1.0);"
    // Shift origin back to bottom-left (taking offset into account)
      "f_texcoord.x = normCoord.x/2.0 + (Centre.x/2.0); "
      "f_texcoord.y = normCoord.y/2.0 + (Centre.y/2.0); "

      "FragColor = texture(tex_s, f_texcoord)*color4;"
        "}"
    "}"
};


void print_shader_info_log (GLuint  shader)      // handle to the shader
{
   GLint  length;
   char *buffer ;
   GLint success;

   glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
   if (success != GL_TRUE )
    { fprintf(stderr,"Shader fail\n");
      glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );
      if (length != 0)
       { if((buffer = malloc(length+1)) == NULL)
          { fprintf(stderr,"Malloc fail\n") ; exit(1) ; }
         *buffer = 0 ;
         glGetShaderInfoLog ( shader , length , NULL , buffer );
         fprintf(stderr,"Info: %s\n", buffer);
         free(buffer);
       }
    }
}

GLuint load_shader (const char *shader_source, GLenum type)
{
   GLuint  shader = glCreateShader(type);
   glShaderSource  (shader , 1 , &shader_source , NULL);
   glCompileShader (shader);

   print_shader_info_log (shader);
   return shader;
}


// Set the graphics hardware
int shGLESinit(GLint width, GLint height)
{

///////  the X11 part  //////////////////////////////////////////////////////////////////
// Firstly open a connection to the X11 window manager
//

   x_display = XOpenDisplay ( NULL );   // open the standard display (the primary screen)
   if ( x_display == NULL ) {
      fprintf(stderr,"Cannot connect to X server\n");
      return 1;
   }

   Window root  =  DefaultRootWindow( x_display );   // get the root window (usually the whole screen)

   XSetWindowAttributes  swa;
   swa.event_mask  =  ExposureMask|PointerMotionMask|KeyPressMask|ButtonPress;
 
   win  =  XCreateWindow (   // create a window with the provided parameters
              x_display, root,
              0, 0, width, height,   1,
              CopyFromParent, InputOutput,
              CopyFromParent, CWEventMask,
              &swa );


   XSetWindowAttributes  xattr;
   Atom  atom;
   int   one = 1;
 
   xattr.override_redirect = False;
   XChangeWindowAttributes ( x_display, win, CWOverrideRedirect, &xattr );
/*
   atom = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", True );
   XChangeProperty (
      x_display, win,
      XInternAtom ( x_display, "_NET_WM_STATE", True ),
      XA_ATOM,  32,  PropModeReplace,
      (unsigned char*) &atom,  1 );

*/
   XChangeProperty (
      x_display, win,
      XInternAtom ( x_display, "_HILDON_NON_COMPOSITED_WINDOW", False ),
      XA_INTEGER,  32,  PropModeReplace,
      (unsigned char*) &one,  1);

   XWMHints hints;
   hints.input = True;
   hints.flags = InputHint;
   XSetWMHints(x_display, win, &hints);
 
   XMapWindow ( x_display , win );             // make the window visible on the screen
   XStoreName ( x_display , win , "VG test" ); // give the window a name
 
   //// get identifiers for the provided atom name strings
   Atom wm_state   = XInternAtom ( x_display, "_NET_WM_STATE", False );
//   Atom fullscreen = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", False );
   Atom ontop   = XInternAtom ( x_display, "_NET_WM_STATE_ABOVE", False );

   XEvent xev;
   memset ( &xev, 0, sizeof(xev) );
 
   xev.type                 = ClientMessage;
   xev.xclient.window       = win;
   xev.xclient.message_type = wm_state;
   xev.xclient.format       = 32;
   xev.xclient.data.l[0]    = 1;  // Normal state
//   xev.xclient.data.l[1]  = fullscreen;  // Probably ignored
   xev.xclient.data.l[1]    = ontop;

   XSendEvent (                // send an event mask to the X-server
      x_display,
      DefaultRootWindow ( x_display ),
      False,
      SubstructureNotifyMask,
      &xev );

   ///////  the egl part  //////////////////////////////////////////////////////////////////
   //  egl provides an interface to connect the graphics related functionality of openGL ES
   //  with the windowing interface and functionality of the native operation system (X11
   //  in our case.
 
   egl_display  =  eglGetDisplay( (EGLNativeDisplayType) x_display );
   if ( egl_display == EGL_NO_DISPLAY ) {
      fprintf(stderr,"No EGL display\n") ;
      return 2;
   }
 
   if ( !eglInitialize( egl_display, NULL, NULL ) ) {
      fprintf(stderr,"Unable to initialize EGL\n") ;
      return 2;
   }
 
   EGLint attr[] = {       // some attributes to set up our egl-interface
      EGL_BUFFER_SIZE, 32,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_STENCIL_SIZE, 8,
      EGL_DEPTH_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
   };
 
   EGLConfig  ecfg;
   EGLint     num_config;
   if ( !eglChooseConfig( egl_display, attr, &ecfg, 1, &num_config ) ) {
      fprintf(stderr,"Failed to choose config eglError: %d\n", eglGetError());
      return 2;
   }
 
   if ( num_config != 1 ) {
      fprintf(stderr,"Multiple configs %d\n",num_config);
      return 2;
   }
 
   egl_surface = eglCreateWindowSurface ( egl_display, ecfg, win, NULL );
   if ( egl_surface == EGL_NO_SURFACE ) {
      fprintf(stderr,"Unable to create EGL surface eglError:%d\n", eglGetError());
      return 2;
   }
 
   //// egl-contexts collect all state descriptions needed required for operation
   EGLint ctxattr[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   egl_context = eglCreateContext ( egl_display, ecfg, EGL_NO_CONTEXT, ctxattr );
   if ( egl_context == EGL_NO_CONTEXT ) {
      fprintf(stderr,"Unable to create EGL context eglError: %d\n", eglGetError()) ;
      return 2;
   }
 
   //// associate the egl-context with the egl-surface
   eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );
   eglSwapInterval(egl_display, 1) ;
   eglSurfaceAttrib(egl_display, egl_surface, EGL_SWAP_BEHAVIOR,
                    EGL_BUFFER_PRESERVED) ;

// /////  the openGL part  ////////////////////////////////////////
// Load shaders
   GLuint vertexShader = load_shader (vertex_src , GL_VERTEX_SHADER );
   GLuint fragmentShader = load_shader (fragment3_src , GL_FRAGMENT_SHADER );

   shaderProgram  = glCreateProgram ();                 // create program object
   glAttachShader ( shaderProgram, vertexShader );       // and attach both...
   glAttachShader ( shaderProgram, fragmentShader );    // ... shaders to it
   glLinkProgram ( shaderProgram );    // link the program
   glUseProgram  ( shaderProgram );    // and select it for usage


//// now get the locations (handle) of the shader variables
   position_loc  = glGetAttribLocation  ( shaderProgram , "position");
   texc_loc      = glGetAttribLocation ( shaderProgram , "texcoord");
   tflag_loc    = glGetUniformLocation ( shaderProgram , "texGenflag");
   color4_loc    = glGetUniformLocation ( shaderProgram , "color4");
   texs_loc    = glGetUniformLocation ( shaderProgram , "tex_s");

   fprintf(stderr, "Locs: %d %d %d %d %d\n", position_loc, texc_loc,
                    color4_loc, tflag_loc, texs_loc) ;

   glDeleteShader ( vertexShader );
   glDeleteShader ( fragmentShader );

// and initialise. Matrix standard in VG and GL is column order
   GLfloat migu[16] = {1.0,0,0,0 ,0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0};
   GLint locm = glGetUniformLocation(shaderProgram, "mview") ;
   glUniformMatrix4fv(locm, 1, GL_FALSE , (GLfloat *) migu );
   locm = glGetUniformLocation(shaderProgram, "tview") ;
   glUniformMatrix4fv(locm, 1, GL_FALSE , (GLfloat *) migu );
   GLint locb = glGetUniformLocation(shaderProgram, "texGenflag") ;
   glUniform1i(locb, 0) ;

   glUniform4f(color4_loc, 0.0f, 0.0f, 0.0f, 1.0f);

   vgCreateContextSH(width, height);

   // So far so good
   GLint errno ;
   errno = glGetError() ;
//   fprintf(stderr,"GLESinit glerr: 0%x\n", errno) ;

   return errno ;
}


// Clean up
void shGLESdeinit(void)
{
   eglDestroyContext ( egl_display, egl_context );
   eglDestroySurface ( egl_display, egl_surface );
   eglTerminate      ( egl_display );
   XDestroyWindow    ( x_display, win );
   XCloseDisplay     ( x_display );

}

