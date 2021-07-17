#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include <VG/openvg.h>
#include "shGLESinit.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPaint.h"

VGPath line;
VGPath polyOpen;
VGPath polyClosed;
VGPath rect;
VGPath rectRound;
VGPath ellipse;
VGPath arcOpen;
VGPath arcChord;
VGPath arcPie;
VGPaint stroke ;

extern  Display  *x_display;
extern  Window win ;
extern EGLDisplay  egl_display;
extern EGLContext  egl_context;
extern EGLSurface  egl_surface;
extern VGContext   *vg_context;

#define NUM_PRIMITIVES 9
VGPath primitives[NUM_PRIMITIVES];


/* ************************************************************ */
VGPath testCreatePath()
{
   return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
}

void render()
{
//   glFlush() ;
   eglSwapBuffers ( egl_display, egl_surface );  // buffer to the screen
}


/* ************************************************************ */

void display(int pi)
{
   int x, y ;
   const VGfloat grey[] = { 0.3, 0.3, 0.3, 1.0 };
   const VGfloat red[] = { 0.8, 0.0, 0.0, 1.0 };
   struct timespec start, finish ;
   long mss, msf ;

   vgSetfv(VG_CLEAR_COLOR, 4, grey);

   vgClear(0, 0, vg_context->surfaceWidth, vg_context->surfaceHeight);
   stroke = vgCreatePaint();
   vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, red);
   vgSetPaint(stroke, VG_STROKE_PATH) ;

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);

   clock_gettime(CLOCK_REALTIME, &start);
   vgDrawPath(primitives[pi], VG_STROKE_PATH);
   render() ;
   clock_gettime(CLOCK_REALTIME, &finish);

   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;
}

// Using coords 0,0 to 100,100
void createPrimitives(void)
{
   VGfloat points[] = { 20, 20*0.75, 50, 20*0.75, 50, 70*0.75 };

   vgSetf(VG_STROKE_LINE_WIDTH, 0.8f);

   line = testCreatePath();
   vguLine(line, 10.0, 10.0*0.75, 80.0, 80.0*0.75);
   primitives[0] = line;

   polyOpen = testCreatePath();
   vguPolygon(polyOpen, points, 3, VG_FALSE);
   primitives[1] = polyOpen;

   polyClosed = testCreatePath();
   vguPolygon(polyClosed, points, 3, VG_TRUE);
   primitives[2] = polyClosed;

   rect = testCreatePath();
   vguRect(rect,20, 20*0.75, 60, 60*0.75);
   primitives[3] = rect;

   rectRound = testCreatePath();
   vguRoundRect(rectRound, 20, 20*0.75, 60, 60*0.75, 5, 5);
   primitives[4] = rectRound;

   ellipse = testCreatePath();
   vguEllipse(ellipse, 50, 50*0.75, 30, 30);
   primitives[5] = ellipse;

   arcOpen = testCreatePath();
   vguArc(arcOpen, 40, 40*0.75, 60, 60, 0, 90, VGU_ARC_OPEN);
   primitives[6] = arcOpen;

   arcChord = testCreatePath();
   vguArc(arcChord, 40, 40*0.75, 60, 60, 0, 90, VGU_ARC_CHORD);
   primitives[7] = arcChord;

   arcPie = testCreatePath();
   vguArc(arcPie, 0, 0, 60, 60, 0, 50, VGU_ARC_PIE);
   primitives[8] = arcPie;
}


int main(int argc, char **argv)
{  bool quit = false;
   XEvent  xev;
   int dindex = 0 ;

   VGint screen_x = 640 , screen_y= 480 ;

   printf("ShivaVG: VGU Primitives Test\n");
   printf("Hit <space> to step or <esc> to quit\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
//   glClearColor ( 0.08 , 0.08 , 0.07 , 1.);    // background color
   glClear ( GL_COLOR_BUFFER_BIT );

// transform 0 to 100 to -1 to +1 for the coord shader x axis
   VGfloat mt[] = {0.02, 0, 0, 0, 0.02, 0, -1.0, -1.0, 1} ;
// Adjust y-axis for aspect ratio for even linewidth.
// Ajdust y-coords input by *(480/640)
   VGfloat asr = (VGfloat)screen_x / (VGfloat)screen_y ;
   mt[4] *= asr ;

  vgLoadMatrix(mt) ;
  createPrimitives();
  glBindTexture(GL_TEXTURE_2D, 0);
  display(dindex) ;

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
        else
         { if ( xev.type == KeyPress) //Esc
             { if (xev.xkey.keycode == 9)
                 quit = true;
               else if (xev.xkey.keycode == 65)
                { dindex++ ;
                  if (dindex > 8) dindex = 0 ;
                  display(dindex) ;
                }
               else
                fprintf(stderr, "#: %d\n",xev.xkey.keycode) ;
             }
         }
      }
     usleep(30000) ;
   }
  shGLESdeinit() ;


  return 0;
}
