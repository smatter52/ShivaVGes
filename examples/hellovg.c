// first OpenVG program
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"

#include "shGLESinit.h"
#include "shContext.h"

extern  Display  *x_display;
extern  Window win ;
extern EGLDisplay  egl_display;
extern EGLContext  egl_context;
extern EGLSurface  egl_surface;
extern VGContext   *vg_context;

int width, height;
VGPaint radialFill;

VGint spread[] = {
   VG_COLOR_RAMP_SPREAD_PAD,
   VG_COLOR_RAMP_SPREAD_REPEAT,
   VG_COLOR_RAMP_SPREAD_REFLECT
};

VGPath createPath(void)
{
   return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
}

void createRadial(void)
{
   VGfloat stops[] = {
      0.0, 0.1 , 0.9, 0.1, 1,
      0.2, 0.1 ,0.6, 0.4, 1,
      0.4, 0.17, 0.3, 0.9, 1
   };

   VGint numstops = sizeof(stops) / sizeof(VGfloat);

   VGfloat radial[4];
   radial[0] = 70;
   radial[1] = 50 ;
   radial[2] = 20;
   radial[3] = 0;
   vgSetParameteri(radialFill, VG_PAINT_COLOR_RAMP_SPREAD_MODE,
                   spread[0]);
   vgSetParameteri(radialFill, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
   vgSetParameterfv(radialFill, VG_PAINT_RADIAL_GRADIENT, 4, radial);
   vgSetParameterfv(radialFill, VG_PAINT_COLOR_RAMP_STOPS, numstops, stops);
   vgSetParameterf(radialFill, VG_PAINT_GRANULARITY, 0.2) ;
}

void display(void) {
	char hello1[] = { 'H', 'e', 'j', ',', ' ', 'v', 0xc3, 0xa4, 'r', 'l', 'd', 'e', 'n', 0 };
	char hello2[] = { 'H', 'e', 'l', 'l', 0xc3, 0xb3, ' ', 'V', 'i', 'l', 0xc3, 0xa1, 'g', 0 };
	char hello3[] = { 'A', 'h', 'o', 'j', ' ', 's', 'v', 0xc4, 0x95, 't', 'e', 0 };
   struct timespec start, finish ;
   long mss, msf ;

   clock_gettime(CLOCK_REALTIME, &start);

//	Start(width, height);				   // Start the picture
	Background(0, 0, 0);				   // Black background

// The "world"
	VGPath path = createPath() ;
	vguEllipse(path, 100/2, 0, 100, 100);
   createRadial() ;
   vgSetPaint(radialFill, VG_FILL_PATH);
   vgDrawPath(path, VG_FILL_PATH);

	Fill(255, 255, 255, 1);				   // White text
	TextMid(100 / 2, (60*0.75*0.7), "Hello World", SerifTypeface, 100 / 15);	// Greetings
	TextMid(100 / 2, (60*0.75*0.5), hello1, SerifTypeface, 100 / 30);
	TextMid(100 / 2, (60*0.75*0.3), hello2, SerifTypeface, 100 / 30);
	TextMid(100 / 2, (60*0.75* 0.1), hello3, SerifTypeface, 100 / 30);

   Render();	       // make visible
   clock_gettime(CLOCK_REALTIME, &finish);

   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;
}



int main(int argc, char **argv)
{  bool quit = false;
   XEvent  xev;

   VGint screen_x = 640 , screen_y= 480 ;
   width = screen_x; height = screen_y ;
   
   printf("ShivaVG: OpenVG Test\n");
   printf("Hit <esc> to quit\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;
   Fontinit() ;
   radialFill = vgCreatePaint();

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
   glClear ( GL_COLOR_BUFFER_BIT );

// transform 0 to 100 to -1 to +1 for the coord shader x axis
   VGfloat mt[] = {0.02, 0, 0, 0, 0.02, 0, -1.0, -1.0, 1} ;
// Adjust y-axis for aspect ratio for even linewidth.
// Ajdust y-coords input by *(480/640) or whatever
   VGfloat asr = (VGfloat)screen_x / (VGfloat)screen_y ;
   mt[4] *= asr ;

  vgLoadMatrix(mt) ;
  glBindTexture(GL_TEXTURE_2D, 0);

  Atom wmDeleteMessage = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(x_display, win, &wmDeleteMessage, 1);

  display() ;

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
               else
                { // fprintf(stderr, "#: %d\n",xev.xkey.keycode) ;
                  display() ;
                }
             }
         }
      }

     usleep(30000) ;
   }

  Fontdeinit() ;
  shGLESdeinit() ;


  return 0;
}
