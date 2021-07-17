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

VGfloat o[3] = { 0.1f, 0.5f, 1.0f };

VGfloat gx1, gy1;   // linear grad coord 1
VGfloat gx2, gy2;   // linear grad coord 2
VGfloat sqx = 640;  // defaults overidden in setup
VGfloat sqy = 480;
VGfloat gr ;    // granularity

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


VGfloat clickX;
VGfloat clickY;
VGfloat startX;
VGfloat startY;
char mode = 's';

VGPaint linearFill;
VGPaint blackFill;
VGPath start;
VGPath end;
VGPath side1;
VGPath side2;
VGPath unitX;
VGPath unitY;
VGPath ph;
VGPaint fill ;

VGfloat black[] = { 0, 0, 0, 1 };
VGfloat red[] = { 1.0, 0.0, 0.0, 1.0 };

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


/* *********************************************/

void display()
{
   VGfloat cc[] = { 0.0, 0.0, 0.0, 1 };

   vgSetfv(VG_CLEAR_COLOR, 4, cc);
   vgClear(0, 0, vg_context->surfaceWidth, vg_context->surfaceHeight);


   vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
   vgLoadIdentity();   //Set fill trasform matrix to identity value

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(tx, ty);
   vgScale(sx, sy);
   vgRotate(a);

   struct timespec start, finish ;
   long mss, msf ;
   clock_gettime(CLOCK_REALTIME, &start);

   vgSetPaint(linearFill, VG_FILL_PATH);
   vgDrawPath(ph, VG_FILL_PATH);

   Render() ;
   
   clock_gettime(CLOCK_REALTIME, &finish);
   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;
}

void createLinear(void)
{

// Stop: offset, rgba on gradient axis
   VGfloat stops[] = {
      0.0, 1.0, 0.0, 0.0, 1,     // Red left on axis
      0.5, 0.0, 1.0, 0.0, 1,     // Green centre on axis
      1.0, 0.0, 0.0, 1.0, 1      // Blue right on axis
   };
   VGint numstops = sizeof(stops) / sizeof(VGfloat);

// Gradient axis setup
   VGfloat linear[5];
   linear[0] = gx1;
   linear[1] = gy1;
   linear[2] = gx2;
   linear[3] = gy2;

// the full monty of linear effects
   vgSetParameteri(linearFill, VG_PAINT_COLOR_RAMP_SPREAD_MODE,
                   spread[sindex]);
   vgSetParameteri(linearFill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
   vgSetParameterfv(linearFill, VG_PAINT_LINEAR_GRADIENT, 4, linear);
   vgSetParameterfv(linearFill, VG_PAINT_COLOR_RAMP_STOPS, numstops, stops);
   vgSetParameterf(linearFill, VG_PAINT_GRANULARITY, gr) ;
}


void drag(int x, int y)
{
}

void clickaction2(void)
{  SHfloat gran ;

   gran = vg_context->fillPaint->granularity ;
   gran*=2 ;
   if (gran >= 2.0) gran = 0.01 ;
   if (gran > 1.0) gran = 1.0 ;
//   vg_context->fillPaint->granularity = gran ;
   fprintf(stderr,"Granularity: %f\n", gran) ;
   gr = gran ;
   createLinear();
   display() ;
}


void key(unsigned char key, int index)
{  if (index == 0)
    {  gx1 = 0.0; gy1 = 1.0;
       gx2 = 0.0; gy2 = 0.0;
    }
    else if (index == 1)
    {  gx1 = 1.0; gy1 = 0.0;
       gx2 = 0.0; gy2 = 0.0;
    }
    else if (index == 2)
    {  gx1 = 0.0; gy1 = 0.0;
       gx2 = 0.0; gy2 = 1.0;
    }
    else if (index == 3)
    {  gx1 = 0.0; gy1 = 0.0;
       gx2 = 1.0; gy2 = 0.0;
    }
//   createSquare(ph, 60, 60, 400, 1.0);
   createLinear();
   display() ;
}

void createSquare(VGPath p, float x1, float y1, float side, float aspect)
{
   testMoveTo(p, x1, y1, VG_ABSOLUTE);
   testLineTo(p, side, 0, VG_RELATIVE);
   testLineTo(p, 0, side * aspect , VG_RELATIVE);
   testLineTo(p, -side, 0, VG_RELATIVE);
   testClosePath(p);
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
//   VGint screen_x = 800 , screen_y= 600 ;

   printf("ShivaVG: Linear Gradient Test\n");
   printf("Hit <esc> to quit\n") ;
   printf("Space to rotate pattern 90 right\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
   glClearColor (0.3, 0.3, 0.3, 1.0);    // background color grey
   glClear ( GL_COLOR_BUFFER_BIT );
   glBindTexture(GL_TEXTURE_2D, 0);

// Init
   ph = testCreatePath();
   start = testCreatePath();
   end = testCreatePath();

   linearFill = vgCreatePaint();
   blackFill = vgCreatePaint();
   vgSetParameterfv(blackFill, VG_PAINT_COLOR, 4, black);

// Quadrant paint orientation. Vertical up is default
   gx1 = 0.0; gy1 = 1.0;
   gx2 = 0.0; gy2 = 0.0;
// Granularity
   gr = 0.01 ;
   createSquare(ph, 60, 60, 400, 1.0);
   createLinear();
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
             { if (xev.xkey.keycode == 9)
                 quit = true;
               else if (xev.xkey.keycode == 65)
                { dindex++ ;
                  if (dindex > 3) dindex = 0 ;
                   key(xev.xkey.keycode, dindex) ;
                }
               else
                fprintf(stderr, "#: %d\n",xev.xkey.keycode) ;
             }
         }
        else if ( xev.type == ButtonPress)
           { if (xev.xbutton.button == Button3)
                 clickaction2() ;
           }
        else if ( xev.type == MotionNotify)
          drag(norm_mousex(xev.xbutton.x),norm_mousey(xev.xbutton.y)) ;
//           fprintf(stderr,"Mouse moved\n") ;
      }
     usleep(30000) ;
    }

   return EXIT_SUCCESS;
}
