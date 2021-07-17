/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define VG_API_EXPORT
#include <VG/openvg.h>
#include "shContext.h"
#include "shPaint.h"
#include <stdio.h>
#include "shCommons.h"
#include "shDefs.h"

#define _ITEM_T SHStop
#define _ARRAY_T SHStopArray
#define _FUNC_T shStopArray
#define _COMPARE_T(s1,s2) 0
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T SHPaint*
#define _ARRAY_T SHPaintArray
#define _FUNC_T shPaintArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"


// SHuint8 rgba_s[1024*1*4];
static SHuint8 *rgba_p = NULL ;
static SHuint texwidth = 0;
extern VGContext   *vg_context;

void
SHPaint_ctor(SHPaint * p)
{
   SH_ASSERT(p != NULL);

   p->type = VG_PAINT_TYPE_COLOR;
   CSET(p->color, 0, 0, 0, 1);    // Black

   SH_INITOBJ(SHStopArray, p->instops);
   SH_INITOBJ(SHStopArray, p->stops);

   p->premultiplied = VG_FALSE;
   p->spreadMode = VG_COLOR_RAMP_SPREAD_PAD;
   p->tilingMode = VG_TILE_FILL;

   for (int i = 0; i < 4; ++i)
      p->linearGradient[i] = 0.0f;
   for (int i = 0; i < 5; ++i)
      p->radialGradient[i] = 0.0f;

   p->granularity = 0.01 ;
   p->pattern = VG_INVALID_HANDLE;

   glGenTextures(1, &p->texture);
}

void
SHPaint_dtor(SHPaint * p)
{
   SH_ASSERT(p != NULL);

   SH_DEINITOBJ(SHStopArray, p->instops);
   SH_DEINITOBJ(SHStopArray, p->stops);

   if (glIsTexture(p->texture))
      glDeleteTextures(1, &p->texture);
}

VG_API_CALL VGPaint vgCreatePaint(void)
{
   SHPaint *p = NULL;
   VG_GETCONTEXT(VG_INVALID_HANDLE);

   /* Create new paint object */
   SH_NEWOBJ(SHPaint, p);
   VG_RETURN_ERR_IF(!p, VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);

   /* Add to resource list */
   shPaintArrayPushBack(&context->paints, p);

   VG_RETURN((VGPaint) p);
}

VG_API_CALL void
vgDestroyPaint(VGPaint paint)
{
   SHint index;
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   SH_ASSERT(paint != 0);

   index = shPaintArrayFind(&context->paints, (SHPaint *) paint);
   VG_RETURN_ERR_IF(index == -1, VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Delete object and remove resource */
   SH_DELETEOBJ(SHPaint, (SHPaint *) paint);
   shPaintArrayRemoveAt(&context->paints, index);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgSetPaint(VGPaint paint, VGbitfield paintModes)
{

   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint) &&
                    paint != VG_INVALID_HANDLE,
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Check for invalid mode */
   VG_RETURN_ERR_IF(paintModes & ~(VG_STROKE_PATH | VG_FILL_PATH),
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Set stroke / fill */
   if (paintModes & VG_STROKE_PATH)
      context->strokePaint = (SHPaint *) paint;
   if (paintModes & VG_FILL_PATH)
      context->fillPaint = (SHPaint *) paint;

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGPaint
vgGetPaint(VGPaintMode paintMode)
{
   VG_GETCONTEXT(VG_INVALID_HANDLE);

   if (paintMode & VG_STROKE_PATH)
      VG_RETURN((VGPaint) context->strokePaint);
   if (paintMode & VG_FILL_PATH)
      VG_RETURN((VGPaint) context->fillPaint);

   return (VGPaint) VG_ILLEGAL_ARGUMENT_ERROR;
}

VG_API_CALL void vgPaintPattern(VGPaint paint, VGImage pattern)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Check if pattern image valid */
   VG_RETURN_ERR_IF(!shIsValidImage(context, pattern),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: Check if pattern image is current rendering target */

   /* Set pattern image */
   ((SHPaint *) paint)->pattern = pattern; //Image

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgSetColor(VGPaint paint, VGuint rgba)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   // unpack rgba
   VGfloat color[4] = {
      ((rgba >> 24) & 0xff) / 255.0f,
      ((rgba >> 16) & 0xff) / 255.0f,
      ((rgba >> 8) & 0xff) / 255.0f,
      (rgba & 0xff) / 255.0f
   };

   vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGuint
vgGetColor(VGPaint paint)
{
   VG_GETCONTEXT(0);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint), VG_BAD_HANDLE_ERROR, 0);

   VGfloat color[4];

   vgGetParameterfv(paint, VG_PAINT_COLOR, 4, color);

   VGint red = (VGint)(SH_CLAMPF(color[0]) * 255.0f + 0.5f);
   VGint green = (VGint)(SH_CLAMPF(color[1]) * 255.0f + 0.5f);
   VGint blue = (VGint)(SH_CLAMPF(color[2]) * 255.0f + 0.5f);
   VGint alpha = (VGint)(SH_CLAMPF(color[3]) * 255.0f + 0.5f);

   // return packed rgba
   return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

void shUpdateColorRampTexture(SHPaint * p)
{
   SHint s = 0;
   SHStop *stop1, *stop2;
   SHint x1 = 0, x2 = 0, dx, x, y;
   SHuint cnt = 0 ;
   SHColor dc, c;
   SHfloat k;

   SH_ASSERT(p != NULL);

// Free any previous allocation
   if (rgba_p != NULL)
    { free(rgba_p) ;
      rgba_p = NULL ;
    }
// Allocate a chunk of space
   if ((rgba_p = (SHuint8 *)malloc(SH_GRADIENT_TEX_COORDSIZE)) == NULL)
    { fprintf(stderr,"Unable to malloc memory for texture\n") ;
      return ;
    }

    SHfloat gran = p->granularity ;

   /* Write first pixel color */
   stop1 = &p->stops.items[0];
   CSTORE_RGBA1D_8(stop1->color, rgba_p, x1);
   CSTORE_RGBA1D_8(c, rgba_p, x1);
   cnt++ ;
   /* Walk stops */
   for (s = 1; s < p->stops.size; ++s, x1 = x2, stop1 = stop2) {

      /* Pick next stop */
      stop2 = &p->stops.items[s];
      x2 = (SHint) (stop2->offset * (SH_GRADIENT_TEX_SIZE - 1));

      SH_ASSERT(x1 >= 0 && x1 < SH_GRADIENT_TEX_SIZE &&
                x2 >= 0 && x2 < SH_GRADIENT_TEX_SIZE && x1 <= x2 &&
                gran >= 0 && gran <= 1.0) ;

      dx = x2 - x1;
      CSUBCTO(stop2->color, stop1->color, dc);

//      fprintf(stderr,"x1=%d x2=%d\n", x1, x2) ;
      /* Interpolate inbetween */
      for (x = x1 + 1; x <= x2; ++x) {
         k = (SHfloat) (x - x1) / dx;
         k -= fmod(k, gran) ;    // granularity step
         CSETC(c, stop1->color);
         CADDCK(c, dc, k);
//         fprintf(stderr,"c: %f %f %f %f\n", c.r, c.g, c.b, c.a) ;
         CSTORE_RGBA1D_8(c, rgba_p, x);
         cnt++ ;
      }
   }

   texwidth = cnt ;
}

void
shValidateInputStops(SHPaint * p)
{
   SHStop *instop, stop;
   SHfloat lastOffset = 0.0f;

   SH_ASSERT(p != NULL);
   shStopArrayClear(&p->stops);
   shStopArrayReserve(&p->stops, p->instops.size);

   /* Assure input stops are properly defined */
   for (SHint i = 0; i < p->instops.size; ++i) {

      /* Copy stop color */
      instop = &p->instops.items[i];
      stop.color = instop->color;

      /* Offset must be in [0,1] */
      if (instop->offset < 0.0f || instop->offset > 1.0f)
         continue;

      /* Discard whole sequence if not in ascending order */
      if (instop->offset < lastOffset) {
         shStopArrayClear(&p->stops);
         break;
      }

      /* Add stop at offset 0 with same color if first not at 0 */
      if (p->stops.size == 0 && instop->offset != 0.0f) {
         stop.offset = 0.0f;
         shStopArrayPushBackP(&p->stops, &stop);
      }

      /* Add current stop to array */
      stop.offset = instop->offset;
      shStopArrayPushBackP(&p->stops, &stop);

      /* Save last offset */
      lastOffset = instop->offset;
   }

   /* Add stop at offset 1 with same color if last not at 1 */
   if (p->stops.size > 0 && lastOffset != 1.0f) {
      stop.offset = 1.0f;
      shStopArrayPushBackP(&p->stops, &stop);
   }

   /* Add 2 default stops if no valid found */
   if (p->stops.size == 0) {
      /* First opaque black */
      stop.offset = 0.0f;
      CSET(stop.color, 0, 0, 0, 1);
      shStopArrayPushBackP(&p->stops, &stop);
      /* Last opaque white */
      stop.offset = 1.0f;
      CSET(stop.color, 1, 1, 1, 1);
      shStopArrayPushBackP(&p->stops, &stop);
   }

   /* Update texture */
   shUpdateColorRampTexture(p);
}

void
shGenerateStops(SHPaint * p, SHfloat minOffset, SHfloat maxOffset,
                SHStopArray * outStops)
{
   SHStop *s1, *s2;
   SHint i1, i2;
   SHfloat o = 0.0f;
   SHfloat ostep = 0.0f;
   SHint istep = 1;
   SHint istart = 0;

   SH_ASSERT(p != NULL);

   SHint iend = p->stops.size - 1;
   SHint minDone = 0;
   SHint maxDone = 0;
   SHStop outStop;

   /* Start below zero? */
   if (minOffset < 0.0f) {
      if (p->spreadMode == VG_COLOR_RAMP_SPREAD_PAD) {
         /* Add min offset stop */
         outStop = p->stops.items[0];
         outStop.offset = minOffset;
         shStopArrayPushBackP(outStops, &outStop);
         /* Add max offset stop and exit */
         if (maxOffset < 0.0f) {
            outStop.offset = maxOffset;
            shStopArrayPushBackP(outStops, &outStop);
            return;
         }
      } else {
         /* Pad starting offset to nearest factor of 2 */
         SHint ioff = (SHint) SH_FLOOR(minOffset);
         o = (SHfloat) (ioff - (ioff & 1));
      }
   }

   /* Construct stops until max offset reached */
   for (i1 = istart, i2 = istart + istep; maxDone != 1;
        i1 += istep, i2 += istep, o += ostep) {

      /* All stops consumed? */
      if (i1 == iend) {
         switch (p->spreadMode) {

         case VG_COLOR_RAMP_SPREAD_PAD:
            /* Pick last stop */
            outStop = p->stops.items[i1];
            if (!minDone) {
               /* Add min offset stop with last color */
               outStop.offset = minOffset;
               shStopArrayPushBackP(outStops, &outStop);
            }
            /* Add max offset stop with last color */
            outStop.offset = maxOffset;
            shStopArrayPushBackP(outStops, &outStop);
            return;

         case VG_COLOR_RAMP_SPREAD_REPEAT:
            /* Reset iteration */
            i1 = istart;
            i2 = istart + istep;
            /* Add stop1 if past min offset */
            if (minDone) {
               outStop = p->stops.items[0];
               outStop.offset = o;
               shStopArrayPushBackP(outStops, &outStop);
            }
            break;

         case VG_COLOR_RAMP_SPREAD_REFLECT:
            /* Reflect iteration direction */
            istep = -istep;
            i2 = i1 + istep;
            iend = (istep == 1) ? p->stops.size - 1 : 0;
            break;
         }
      }

      /* 2 stops and their offset distance */
      s1 = &p->stops.items[i1];
      s2 = &p->stops.items[i2];
      ostep = s2->offset - s1->offset;
      ostep = SH_ABS(ostep);

      /* Add stop1 if reached min offset */
      if (!minDone && o + ostep > minOffset) {
         minDone = 1;
         outStop = *s1;
         outStop.offset = o;
         shStopArrayPushBackP(outStops, &outStop);
      }

      /* Mark done if reached max offset */
      if (o + ostep > maxOffset)
         maxDone = 1;

      /* Add stop2 if past min offset */
      if (minDone) {
         outStop = *s2;
         outStop.offset = o + ostep;
         shStopArrayPushBackP(outStops, &outStop);
      }
   }
}

static void
shSetGradientTexGLState(SHPaint * restrict p)
{
   SH_ASSERT(p != NULL);

   glBindTexture(GL_TEXTURE_2D, p->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   switch (p->spreadMode) {
   case VG_COLOR_RAMP_SPREAD_PAD:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      break;
   case VG_COLOR_RAMP_SPREAD_REPEAT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      break;
   case VG_COLOR_RAMP_SPREAD_REFLECT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
      break;
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
   glUniform4f(color4_loc, 1.0f, 1.0f, 1.0f, 1.0f);
}

static void shSetPatternTexGLState(SHPaint *p, VGContext *c)
{
   SH_ASSERT(p != NULL && c != NULL);

   glBindTexture(GL_TEXTURE_2D, ((SHImage *) p->pattern)->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   switch (p->tilingMode) {
   case VG_TILE_FILL:
/*
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                       (GLfloat *) & c->tileFillColor);
      break;
*/
   case VG_TILE_PAD:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      break;
   case VG_TILE_REPEAT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      break;
   case VG_TILE_REFLECT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
      break;
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
   glUniform4f(color4_loc, 1.0f, 1.0f, 1.0f, 1.0f);
}

/* The standard makes paint tranformation optional so the transformation
   is the PathTransform loaded in the shader. The gradient info maps
   the paint render to quadrants. More pratical with pretty bar displays.
   Also knocks off 2mSec.
*/
int shDrawLinearGradientMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                         VGPaintMode mode, GLenum texUnit)
{
   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   SHfloat x1 = p->linearGradient[0];
   SHfloat y1 = p->linearGradient[1];
   SHfloat x2 = p->linearGradient[2];
   SHfloat y2 = p->linearGradient[3];

   SHVector2 corners[4];

   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);

   /* Draw quad using color-ramp texture */
   glActiveTexture(texUnit);
   shSetGradientTexGLState(p);
   glBindTexture(GL_TEXTURE_2D, p->texture);
   glUniform1i(texs_loc, texUnit-GL_TEXTURE0) ;
   shSetGradientTexGLState(p);


   fprintf(stderr,"Grad points : %f %f %f %f\n",x1, y1, x2, y2) ;
   for (SHint i = 0; i < 4; ++i)
     fprintf(stderr,"Corner[%d]: %f %f\n",i, corners[i].x, corners[i].y) ;

   if (x1 > 0.0) x1 = 1.0 ;
   if (x2 > 0.0) x2 = 1.0 ;
   if (y1 > 0.0) y1 = 1.0 ;
   if (y2 > 0.0) y2 = 1.0 ;

   SHCubic quadv, quadt;
   quadv.p1 = corners[1];
   quadv.p2 = corners[2];
   quadv.p3 = corners[0];
   quadv.p4 = corners[3];

// Horizontal right
   if (x1 == 0.0 && x2 == 1.0)
    { quadt.p1.x = 1.0; quadt.p1.y = 0.0;
      quadt.p2.x = 1.0; quadt.p2.y = 1.0;
      quadt.p3.x = 0.0; quadt.p3.y = 0.0;
      quadt.p4.x = 0.0; quadt.p4.y = 1.0;
    }
// Horizontal left
   else  if (x1 == 1.0 && x2 == 0.0)
    { quadt.p1.x = 0.0; quadt.p1.y = 1.0;
      quadt.p2.x = 0.0; quadt.p2.y = 0.0;
      quadt.p3.x = 1.0; quadt.p3.y = 1.0;
      quadt.p4.x = 1.0; quadt.p4.y = 0.0;
    }
// Vertical down
   else if (y1 == 1.0 && y2 == 0.0)
    { quadt.p1.x = 1.0; quadt.p1.y = 1.0;
      quadt.p2.x = 0.0; quadt.p2.y = 1.0;
      quadt.p3.x = 1.0; quadt.p3.y = 0.0;
      quadt.p4.x = 0.0; quadt.p4.y = 0.0;
    }
// Vertical up
   else
    { quadt.p1.x = 0.0; quadt.p1.y = 0.0;
      quadt.p2.x = 1.0; quadt.p2.y = 0.0;
      quadt.p3.x = 0.0; quadt.p3.y = 1.0;
      quadt.p4.x = 1.0; quadt.p4.y = 1.0;
    }

   GLvoid* vertices  = (GLvoid*) &quadv;
	GLvoid* textures  = (GLvoid*) &quadt;
// updating attribute values
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texwidth, 1, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, rgba_p);
// enabling vertex arrays
   glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(position_loc);
	glVertexAttribPointer(texc_loc, 2, GL_FLOAT, GL_FALSE, 0, textures);
	glEnableVertexAttribArray(texc_loc);

   glUniform4f(color4_loc, 1.0f, 1.0f, 1.0f, 1.0f);

// Tell the frag shader switch (linear)
   glUniform1i(tflag_loc, 1) ;

// Draw the quad
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(position_loc);
	glDisableVertexAttribArray(texc_loc);
// Reset the frag shader switch
   glUniform1i(tflag_loc, 0) ;

   GLint errno ;
   errno = glGetError() ;
   fprintf(stderr,"Linear glerr: 0%x\n", errno) ;

   return 1;
}


int shDrawRadialGradientMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                         VGPaintMode mode, GLenum texUnit)
{
   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   // gradient center
   SHfloat cx = p->radialGradient[0];
   SHfloat cy = p->radialGradient[1];

   // gradient focal point. Not supported
   SHfloat fx = p->radialGradient[2];
   SHfloat fy = p->radialGradient[3];

   // radius is fixed. It changes the granulatity in the shader. Granularity
   // has its own parameter which adjusts the texture.
   SHfloat r = 0.01 ;

   SHVector2 corners[4];
   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);

   /* Draw quad using color-ramp texture */
   glActiveTexture(texUnit);
   shSetGradientTexGLState(p);
   glBindTexture(GL_TEXTURE_2D, p->texture);
   glUniform1i(texs_loc, texUnit-GL_TEXTURE0) ;


   for (SHint i = 0; i < 4; ++i)
     fprintf(stderr,"Corner[%d]: %f %f\n",i, corners[i].x, corners[i].y) ;
   fprintf(stderr,"Centre: %f %f\n", cx, cy) ;

   SHCubic quadv, quadt;
   quadv.p1 = corners[1];
   quadv.p2 = corners[2];
   quadv.p3 = corners[0];
   quadv.p4 = corners[3];

   quadt.p1.x = 1.0; quadt.p1.y = 0.0;
   quadt.p2.x = 1.0; quadt.p2.y = 1.0;
   quadt.p3.x = 0.0; quadt.p3.y = 0.0;
   quadt.p4.x = 0.0; quadt.p4.y = 1.0;

   GLvoid* vertices  = (GLvoid*) &quadv;
	GLvoid* textures  = (GLvoid*) &quadt;
   SHMatrix3x3 *m;
   m = &vg_context->pathTransform;

// Setup radial shader
   SHVector2 c, cs ;
   SET2(c, cx, cy) ;
   TRANSFORM2TO(c, (*m), cs);
   GLuint shdr_loc ;
   shdr_loc  = glGetUniformLocation  ( shaderProgram , "Angle");
   glUniform1f(shdr_loc, 3.142*2) ;
   shdr_loc  = glGetUniformLocation  ( shaderProgram , "Radius");
   glUniform1f(shdr_loc, r) ;
   shdr_loc  = glGetUniformLocation  ( shaderProgram , "Centre");
   glUniform2f(shdr_loc, cs.x+1.0, cs.y+1.0) ;
   fprintf(stderr,"Centre: %f %f Radius: %f\n",cs.x+1.0,cs.y+1.0, r) ;

// updating attribute values
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texwidth, 1, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, rgba_p);
// enabling vertex arrays
   glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(position_loc);
	glVertexAttribPointer(texc_loc, 2, GL_FLOAT, GL_FALSE, 0, textures);
	glEnableVertexAttribArray(texc_loc);

   glUniform4f(color4_loc, 1.0f, 1.0f, 1.0f, 1.0f);

// Tell the frag shader switch (radial)
   glUniform1i(tflag_loc, 2) ;

// Draw the quad
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(position_loc);
	glDisableVertexAttribArray(texc_loc);
// Reset the frag shader switch
   glUniform1i(tflag_loc, 0) ;

   GLint errno ;
   errno = glGetError() ;
   fprintf(stderr,"Radial glerr: 0%x\n", errno) ;

   return 1;
}

int shDrawPatternMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                  VGPaintMode mode, GLenum texUnit)
{
   SHImage *img;

   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   SHVector2 corners[4];
   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);


   /* Setup texture coordinates */
   SH_GETCONTEXT(0);
   glActiveTexture(texUnit);
   shSetPatternTexGLState(p, context);
   glUniform1i(texs_loc, texUnit-GL_TEXTURE0) ;

// granulatity sets the tex coord multiple. 1.0 = 1 repeat, 0.25 = 4 etc

   SHfloat texgran = 1.0 /p->granularity ;

   for (SHint i = 0; i < 4; ++i)
     fprintf(stderr,"Corner[%d]: %f %f\n",i, corners[i].x, corners[i].y) ;

   SHCubic quadv, quadt;
   quadv.p1 = corners[1];
   quadv.p2 = corners[2];
   quadv.p3 = corners[0];
   quadv.p4 = corners[3];

   quadt.p1.x = texgran; quadt.p1.y = 0.0;
   quadt.p2.x = texgran; quadt.p2.y = texgran;
   quadt.p3.x = 0.0; quadt.p3.y = 0.0;
   quadt.p4.x = 0.0; quadt.p4.y = texgran;

   GLvoid* vertices  = (GLvoid*) &quadv;
	GLvoid* textures  = (GLvoid*) &quadt;
// updating attribute values
   GLint Tex_loc  = glGetAttribLocation(shaderProgram, "texcoord") ;

// enabling vertex arrays
   glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(position_loc);
	glVertexAttribPointer(Tex_loc, 2, GL_FLOAT, GL_FALSE, 0, textures);
	glEnableVertexAttribArray(Tex_loc);

   // Tell the frag shader switch (linear)
   glUniform1i(tflag_loc, 1) ;

// Draw the quad
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(position_loc);
// Reset the frag shader switch
   glUniform1i(tflag_loc, 0) ;

   GLint errno ;
   errno = glGetError() ;
   fprintf(stderr,"Pattern glerr: 0%x\n", errno) ;


   return 1;
}
