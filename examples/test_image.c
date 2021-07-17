#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <jpeglib.h>

#include <VG/openvg.h>
#include "shGLESinit.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPaint.h"
#include "shapes.h"

#define MYABS(a) (a < 0.0f ? -a : a)

#ifndef IMAGE_DIR
#define IMAGE_DIR "./"
#endif

extern  Display  *x_display;
extern  Window win ;
extern EGLDisplay  egl_display;
extern EGLContext  egl_context;
extern EGLSurface  egl_surface;
extern VGContext   *vg_context;


struct Image
{
   VGImage img;
   unsigned int width;
   unsigned int height;
};

VGfloat tx = -1.0f, ty = -1.0f;     // Translatiom
VGfloat sx = 2.0/640, sy = (2.0/480) ;  // Scaling


int coversCount = 5;
VGImage covers[5];

VGfloat white[4] = { 1, 1, 1, 1 };

VGPath CreatePath(void)
{
   return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
}



struct Image CreateImageFromJpeg(const char *filename)
{
   FILE *infile;
   struct jpeg_decompress_struct jdc;
   struct jpeg_error_mgr jerr;
   JSAMPARRAY buffer;
   unsigned int bstride;
   unsigned int bbpp;

   VGImage img;
   VGubyte *data;
   unsigned int width;
   unsigned int height;
   unsigned int dstride;
   unsigned int dbpp;

   VGubyte *brow;
   VGubyte *drow;
   unsigned int x;

   VGImageFormat rgbaFormat;

   struct Image image;

   rgbaFormat = VG_lRGBA_8888;  // This is the only one supported in GLES
   SH_DEBUG("testCreateImageFromJpeg: rgbaFormat VG_lRGBA_8888\n");

   /* Try to open image file */
   infile = fopen(filename, "rb");
   if (infile == NULL) {
      printf("Failed opening '%s' for reading!\n", filename);
      image.img = VG_INVALID_HANDLE;
      return image;
   }

   /* Setup default error handling */
   jdc.err = jpeg_std_error(&jerr);
   jpeg_create_decompress(&jdc);

   /* Set input file */
   jpeg_stdio_src(&jdc, infile);

   /* Read header and start */
   jpeg_read_header(&jdc, TRUE);
   jpeg_start_decompress(&jdc);
   width = jdc.output_width;
   height = jdc.output_height;

   /* Allocate buffer using jpeg allocator */
   bbpp = jdc.output_components;
   bstride = width * bbpp;
   buffer = (*jdc.mem->alloc_sarray)
      ((j_common_ptr) & jdc, JPOOL_IMAGE, bstride, 1);

   /* Allocate image data buffer */
   dbpp = 4;
   dstride = width * dbpp;
   data = (VGubyte *) malloc(dstride * height);

   /* Iterate until all scanlines processed */
   while (jdc.output_scanline < height) {

      /* Read scanline into buffer */
      jpeg_read_scanlines(&jdc, buffer, 1);
      drow = data + (height - jdc.output_scanline) * dstride;
      brow = buffer[0];

      /* Expand to RGBA */
      for (x = 0; x < width; ++x, drow += dbpp, brow += bbpp) {
         switch (bbpp) {
         case 4:
            drow[0] = brow[0];
            drow[1] = brow[1];
            drow[2] = brow[2];
            drow[3] = brow[3];
            break;
         case 3:
            drow[0] = brow[0];
            drow[1] = brow[1];
            drow[2] = brow[2];
            drow[3] = 255;
            break;
         }
      }
   }

   /* Create VG image */
   img = vgCreateImage(rgbaFormat, width, height, VG_IMAGE_QUALITY_BETTER);
   vgImageSubData(img, data, dstride, rgbaFormat, 0, 0, width, height);

   /* Cleanup */
   jpeg_destroy_decompress(&jdc);
   fclose(infile);
   free(data);

   image.img = img;
   image.width = width;
   image.height = height;
   return image;
}




void drawCover(int c)
{

   /* draw */
   vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
   vgDrawImage(covers[c]);

}

void display(int cover)
{
   VGfloat white[] = { 0, 0, 0, 1 };
   VGint c;

   vgSetfv(VG_CLEAR_COLOR, 4, white);
   vgClear(0, 0, vg_context->surfaceWidth, vg_context->surfaceHeight);

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(tx, ty);
   vgScale(sx, sy);

   struct timespec start, finish ;
   long mss, msf ;
   clock_gettime(CLOCK_REALTIME, &start);

   drawCover(cover) ;

   clock_gettime(CLOCK_REALTIME, &finish);
   mss = round(start.tv_nsec / 1.0e6);
   msf = round(finish.tv_nsec / 1.0e6);
   fprintf(stderr, "Duration: %d ms\n", msf - mss) ;

   Render() ;
}



void clickaction1()
{
}

void clickaction2()
{
}



void key(unsigned char code)
{
}

int
main(int argc, char **argv)
{
   bool quit = false;
   XEvent  xev;
   int dindex = 0 ;

   VGint screen_x = 640 , screen_y= 480 ;

   printf("ShivaVG: Image Test");
   printf("Hit <space> to step or <esc> to quit\n") ;

   // Set up Opengles
   shGLESinit(screen_x, screen_y) ;

   XWindowAttributes  gwa;
   XGetWindowAttributes ( x_display , win , &gwa );
   glViewport ( 0 , 0 , gwa.width , gwa.height );
//   glClearColor (0.3, 0.3, 0.3, 1.0);    // background color grey
   glClear ( GL_COLOR_BUFFER_BIT );
   glBindTexture(GL_TEXTURE_2D, 0);

//Init
   covers[0] = (CreateImageFromJpeg(IMAGE_DIR "test_img_guitar.jpg")).img;
   covers[1] = (CreateImageFromJpeg(IMAGE_DIR "test_img_piano.jpg")).img;
   covers[2] = (CreateImageFromJpeg(IMAGE_DIR "test_img_violin.jpg")).img;
   covers[3] = (CreateImageFromJpeg(IMAGE_DIR "test_img_flute.jpg")).img;
   covers[4] = (CreateImageFromJpeg(IMAGE_DIR "test_img_sax.jpg")).img;

   Atom wmDeleteMessage = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(x_display, win, &wmDeleteMessage, 1);

   display(dindex) ;

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
               else if (xev.xkey.keycode == 65)
                { dindex++ ;
                  if (dindex > 4) dindex = 0 ;
                  display(dindex) ;
                }
               else
                fprintf(stderr, "#: %d\n",xev.xkey.keycode) ;
         }
        else if ( xev.type == ButtonPress)
          { if (xev.xbutton.button == Button1)
              clickaction1() ;
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
