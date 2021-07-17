#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdarg.h>

#include <VG/openvg.h>
#include "shGLESinit.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPaint.h"
#include "shapes.h"

#include <math.h>

extern  Display  *x_display;
extern  Window win ;
extern EGLDisplay  egl_display;
extern EGLContext  egl_context;
extern EGLSurface  egl_surface;
extern VGContext   *vg_context;


VGfloat cx, cy;
VGfloat fx, fy;
VGfloat gr;


VGfloat tx = -1.0f, ty = -1.0f;     // Translatiom
VGfloat sx = 2.0/640, sy = (2.0/480) ;  // Scaling
VGfloat a = 0.0f;  // Rotation

VGint sindex = 0;
VGint ssize = 3;
VGint spread[] = {
   VG_COLOR_RAMP_SPREAD_PAD,
   VG_COLOR_RAMP_SPREAD_REPEAT,
   VG_COLOR_RAMP_SPREAD_REFLECT
};

VGfloat sqx = 640;
VGfloat sqy = 480;

VGPaint radialFill;
VGPath p;


VGfloat black[] = { 0, 0, 0, 1 };
VGfloat red[] = { 1.0, 0.0, 0.0, 1.0 };

static char *overtext = NULL;
static float overcolor[4] = { 0, 0, 0, 1 };


#define BACKSPACE 22
#define TAB 9


/* *********************************************/
VGPath testCreatePath(void)
{
   return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
}

void testMoveTo(VGPath p, float x, float y, VGPathAbsRel absrel)
{
   VGubyte seg = VG_MOVE_TO | absrel;
   VGfloat data[2];

   data[0] = x;
   data[1] = y;
   vgAppendPathData(p, 1, &seg, data);
}

void testLineTo(VGPath p, float x, float y, VGPathAbsRel absrel)
{
   VGubyte seg = VG_LINE_TO | absrel;
   VGfloat data[2];

   data[0] = x;
   data[1] = y;
   vgAppendPathData(p, 1, &seg, data);
}

void testClosePath(VGPath p)
{
   VGubyte seg = VG_CLOSE_PATH;
   VGfloat data = 0.0f;
   vgAppendPathData(p, 1, &seg, &data);
}

void display(void)
{
   VGfloat cc[] = { 0, 0, 0, 1 };

   vgSetfv(VG_CLEAR_COLOR, 4, cc);
   vgClear(0, 0, vg_context->surfaceWidth, vg_context->surfaceHeight);


   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(tx, ty);
   vgScale(sx, sy);
   vgRotate(a);

   vgSetPaint(radialFill, VG_FILL_PATH);
   vgDrawPath(p, VG_FILL_PATH);
   Render() ;

}


void createRadial(void)
{
   VGfloat stops[] = {
      0.0, 1.0, 0.0, 0.0, 1,
      0.4, 0.0, 1.0, 0.0, 1,
      0.8, 0.0, 0.0, 1.0, 1
   };

   VGint numstops = sizeof(stops) / sizeof(VGfloat);

   VGfloat radial[5];
   radial[0] = cx;
   radial[1] = cy;
   radial[2] = fx;
   radial[3] = fy;
   radial[4] = gr;   // granularity

   vgSetParameteri(radialFill, VG_PAINT_COLOR_RAMP_SPREAD_MODE,
                   spread[sindex]);
   vgSetParameteri(radialFill, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
   vgSetParameterfv(radialFill, VG_PAINT_RADIAL_GRADIENT, 4, radial);
   vgSetParameterfv(radialFill, VG_PAINT_COLOR_RAMP_STOPS, numstops, stops);
   vgSetParameterf(radialFill, VG_PAINT_GRANULARITY, gr) ;
}


void createSquare(VGPath p, float x1, float y1, float side, float aspect)
{
   testMoveTo(p, x1, y1, VG_ABSOLUTE);
   testLineTo(p, side, 0, VG_RELATIVE);
   testLineTo(p, 0, side * aspect , VG_RELATIVE);
   testLineTo(p, -side, 0, VG_RELATIVE);
   testClosePath(p);
}

void key(unsigned char key)
{  }


void drag(int x, int y)
{  }

// Change centre
void clickaction1(int x, int y)
{
   cx = x;
   cy = y;
   fx = cx;
   fy = cy;
//   gr = 0.01;

   struct timespec start, finish ;
   long mss, msf ;
   clock_gettime(CLOCK_REALTIME, &start);

   createRadial();
   display() ;

   clock_gettime(CLOCK_REALTIME, &finish);
   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;
}

// Step radius
void clickaction2(void)
{  SHfloat gran ;

   gran = vg_context->fillPaint->granularity ;
   gran*=2 ;
   if (gran >= 2.0) gran = 0.01 ;
   if (gran > 1.0) gran = 1.0 ;
   fprintf(stderr,"Granularity: %f\n", gran) ;
   gr = gran ;
   createRadial();
   display() ;
}


VGfloat norm_mousex(int x)
{ return ((VGfloat)x);}

VGfloat norm_mousey(int y)
{ return ((VGfloat)(vg_context->surfaceHeight - y));}


int main(int argc, char **argv)
{  bool quit = false;
   XEvent  xev;
   int dindex = 0 ;

   VGint screen_x = 640 , screen_y= 480 ;

   printf("ShivaVG: Radial Gradient Test\n");
   printf("Hit <esc> to quit\n") ;
   printf("Left click to set Centre\n") ;
   printf("Right click to double radius\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;
  // sqx = vg_context->surfaceWidth ;
  // sqy = vg_context->surfaceHeight ;
   sqx = 640 ;
   sqy = 280 ;

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
   glClearColor (0.3, 0.3, 0.3, 1.0);    // background color grey
   glClear ( GL_COLOR_BUFFER_BIT );
   glBindTexture(GL_TEXTURE_2D, 0);

// Init
   p = testCreatePath();

   radialFill = vgCreatePaint();


   cx = 420;
   cy = 340;
   fx = cx;
   fy = cy;
   gr = 0.01;

   SHfloat sx0 = 60, sy0 = 45, side = 400 ;
   createSquare(p, sx0, sy0, side, 1.0);
   SHMatrix3x3 click2sqr ;
   IDMAT(click2sqr)
   click2sqr.m[0][0] = vg_context->surfaceWidth / side ;
   click2sqr.m[1][1] = vg_context->surfaceHeight / side;
   click2sqr.m[0][2] = -sx0 ; click2sqr.m[1][2] = -sy0 ;

   createRadial();
   display() ;


   Atom wmDeleteMessage = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(x_display, win, &wmDeleteMessage, 1);
   while (quit == false)
    {
     while ( XPending ( x_display ) )    // check for events from the x-server
      { XNextEvent( x_display, &xev );

        if (xev.type == ClientMessage)
         { if (xev.xclient.data.l[0] == wmDeleteMessage)
            quit = true;
         }
        else if ( xev.type == KeyPress)
         { if (xev.xkey.keycode == 9) // Esc
             quit = true;
           else
            { fprintf(stderr, "#: %c\n",
              XkbKeycodeToKeysym(x_display, xev.xkey.keycode, 0,
                                  xev.xkey.state & ShiftMask ? 1 : 0)) ;
              key( XkbKeycodeToKeysym(x_display, xev.xkey.keycode, 0,
                                  xev.xkey.state & ShiftMask ? 1 : 0)) ;
            }
         }
        else if ( xev.type == ButtonPress)
          { if (xev.xbutton.button == Button1)
           { fprintf(stderr,"Mouse clicked at %f %f\n",
                  norm_mousex(xev.xbutton.x), norm_mousey(xev.xbutton.y)) ;
             SHVector2 click, clickmap ;
             SET2(click,norm_mousex(xev.xbutton.x),norm_mousey(xev.xbutton.y)) ;
// Transform points in square to screen coords
             TRANSFORM2TS(click,click2sqr,clickmap) ;
             fprintf(stderr,"Mouse mapped to %f %f\n",
                       clickmap.x, clickmap.y) ;
             if (clickmap.x >= 0 && clickmap.y >= 0 &&
                 clickmap.x < vg_context->surfaceWidth &&
                 clickmap.y < vg_context->surfaceHeight)
               clickaction1(clickmap.x,clickmap.y) ;
            }
             else if (xev.xbutton.button == Button3)
                 clickaction2() ;
          }
//        else if ( xev.type == MotionNotify)
//          drag(norm_mousex(xev.xbutton.x),norm_mousey(xev.xbutton.y)) ;
//           fprintf(stderr,"Mouse moved\n") ;
      }
     usleep(30000) ;
    }

   return EXIT_SUCCESS;
}
