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
#include <GLES3/gl3.h>
#include <VG/openvg.h>
#include "shContext.h"
#include <string.h>
#include <stdio.h>

/*-----------------------------------------------------
 * Simple functions to create a VG context instance
 * on top of an existing OpenGL context.
 * TODO: There is no mechanics yet to asure the OpenGL
 * context exists and to choose which context / window
 * to bind to. 
 *-----------------------------------------------------*/

VGContext *vg_context = NULL;

VG_API_CALL VGboolean
vgCreateContextSH(VGint width, VGint height)
{
   /* return if already created */
   if (vg_context)
      return VG_TRUE;

   /* create new context */
   SH_NEWOBJ(VGContext, vg_context);
   if (vg_context == NULL)
      return VG_FALSE;

   /* init surface info */
   vg_context->surfaceWidth = width;
   vg_context->surfaceHeight = height;

   /* setup GL projection */
   glViewport(0, 0, width, height);

// Done in shaders initilaised in shGLESinit() ;
/*
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, width, 0, height);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glDisable(GL_LIGHTING);
   glShadeModel(GL_FLAT);
*/
   return VG_TRUE;
}

VG_API_CALL void
vgResizeSurfaceSH(VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* update surface info */
   context->surfaceWidth = width;
   context->surfaceHeight = height;

   /* setup GL projection */
   glViewport(0, 0, width, height);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgDestroyContextSH(void)
{
   /* return if already released */
   if (vg_context == NULL)
      return;

   /* delete context object */
   SH_DELETEOBJ(VGContext, vg_context);
   vg_context = NULL;
}

VGContext *shGetContext(void)
{
   return vg_context;
}

/*-----------------------------------------------------
 * VGContext constructor
 *-----------------------------------------------------*/

void shLoadExtensions(VGContext * c);

void
VGContext_ctor(VGContext * c)
{
   SH_ASSERT(c != NULL);

   /* Surface info */
   c->surfaceWidth = 0;
   c->surfaceHeight = 0;

   /* GetString info */
   strncpy(c->vendor, "Ivan Leben et al", sizeof(c->vendor));
   strncpy(c->renderer, "ShivaVGes 1.1", sizeof(c->renderer));
   strncpy(c->version, "1.1", sizeof(c->version));
   strncpy(c->extensions, "", sizeof(c->extensions));

   /* Mode settings */
   c->matrixMode = VG_MATRIX_PATH_USER_TO_SURFACE;
   c->fillRule = VG_EVEN_ODD;
   c->imageQuality = VG_IMAGE_QUALITY_FASTER;
   c->renderingQuality = VG_RENDERING_QUALITY_BETTER;
   c->blendMode = VG_BLEND_SRC_OVER;
   c->imageMode = VG_DRAW_IMAGE_NORMAL;

   /* Scissor rectangles */
   SH_INITOBJ(SHRectArray, c->scissor);
   c->scissoring = VG_FALSE;
   c->masking = VG_FALSE;

   /* Stroke parameters */
   c->strokeLineWidth = 1.0f;
   c->strokeCapStyle = VG_CAP_BUTT;
   c->strokeJoinStyle = VG_JOIN_MITER;
   c->strokeMiterLimit = 4.0f;
   c->strokeDashPhase = 0.0f;
   c->strokeDashPhaseReset = VG_FALSE;
   SH_INITOBJ(SHFloatArray, c->strokeDashPattern);

   /* Edge fill color for vgConvolve and pattern paint */
   CSET(c->tileFillColor, 0, 0, 0, 0);

   /* Color for vgClear */
   CSET(c->clearColor, 0, 0, 0, 0);

   /* Color components layout inside pixel */
   c->pixelLayout = VG_PIXEL_LAYOUT_UNKNOWN;

   /* Source format for image filters */
   c->filterFormatLinear = VG_FALSE;
   c->filterFormatPremultiplied = VG_FALSE;
   c->filterChannelMask = VG_RED | VG_GREEN | VG_BLUE | VG_ALPHA;

   /* Matrices */
   SH_INITOBJ(SHMatrix3x3, c->pathTransform);
   SH_INITOBJ(SHMatrix3x3, c->imageTransform);
   SH_INITOBJ(SHMatrix3x3, c->fillTransform);
   SH_INITOBJ(SHMatrix3x3, c->strokeTransform);

   /* Paints */
   c->fillPaint = NULL;
   c->strokePaint = NULL;
   SH_INITOBJ(SHPaint, c->defaultPaint);

   /* Error */
   c->error = VG_NO_ERROR;

   /* Resources */
   SH_INITOBJ(SHPathArray, c->paths);
   SH_INITOBJ(SHPaintArray, c->paints);
   SH_INITOBJ(SHImageArray, c->images);

//   shLoadExtensions(c);
}

/*-----------------------------------------------------
 * VGContext constructor
 *-----------------------------------------------------*/

void
VGContext_dtor(VGContext * c)
{

   SH_ASSERT(c != NULL);

   SH_DEINITOBJ(SHRectArray, c->scissor);
   SH_DEINITOBJ(SHFloatArray, c->strokeDashPattern);

   /* Destroy resources */
   for (SHint i = 0; i < c->paths.size; ++i)
      SH_DELETEOBJ(SHPath, c->paths.items[i]);

   for (SHint i = 0; i < c->paints.size; ++i)
      SH_DELETEOBJ(SHPaint, c->paints.items[i]);

   for (SHint i = 0; i < c->images.size; ++i)
      SH_DELETEOBJ(SHImage, c->images.items[i]);
}

/*--------------------------------------------------
 * Tries to find resources in this context
 *--------------------------------------------------*/

inline SHint shIsValidPath(VGContext * c, VGHandle h)
{
   SH_ASSERT(c != NULL && h != 0);
   SHint index = shPathArrayFind(&c->paths, (SHPath *) h);
   return (index == -1) ? 0 : 1;
}

inline SHint shIsValidPaint(VGContext * c, VGHandle h)
{
   SH_ASSERT(c != NULL);
   SHint index = shPaintArrayFind(&c->paints, (SHPaint *) h);
   return (index == -1) ? 0 : 1;
}

inline SHint shIsValidImage(VGContext * c, VGHandle h)
{
   SH_ASSERT(c != NULL);
   SHint index = shImageArrayFind(&c->images, (SHImage *) h);
   return (index == -1) ? 0 : 1;
}

/*--------------------------------------------------
 * Tries to find a resources in this context and
 * return its type or invalid flag.
 *--------------------------------------------------*/

inline SHResourceType
shGetResourceType(VGContext * c, VGHandle h)
{
   if (shIsValidPath(c, h))
      return SH_RESOURCE_PATH;
   else if (shIsValidPaint(c, h))
      return SH_RESOURCE_PAINT;
   else if (shIsValidImage(c, h))
      return SH_RESOURCE_IMAGE;
   else
      return SH_RESOURCE_INVALID;
}

/*-----------------------------------------------------
 * Sets the specified error on the given context if
 * there is no pending error yet
 *-----------------------------------------------------*/

inline void
shSetError(VGContext * c, VGErrorCode e)
{
   SH_ASSERT(c != NULL);
   if (c->error == VG_NO_ERROR)
      c->error = e;
}

/*--------------------------------------------------
 * Returns the oldest error pending on the current
 * context and clears its error code
 *--------------------------------------------------*/

VG_API_CALL VGErrorCode
vgGetError(void)
{
   VGErrorCode error;
   VG_GETCONTEXT(VG_NO_CONTEXT_ERROR);
   error = context->error;
   context->error = VG_NO_ERROR;
   VG_RETURN(error);
}

VG_API_CALL void
vgFlush(void)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   glFlush();
   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgFinish(void)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   glFinish();
   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgMask(VGImage mask, VGMaskOperation operation,
       VGint x, VGint y, VGint width, VGint height)
{
   SH_ASSERT(width > 0 && height > 0);
   VG_GETCONTEXT(VG_NO_RETVAL);

   switch (operation) {
   case VG_CLEAR_MASK:
      break;
   case VG_FILL_MASK:
      break;
   case VG_SET_MASK:
      break;
   case VG_UNION_MASK:
      break;
   case VG_INTERSECT_MASK:
      break;
   case VG_SUBTRACT_MASK:
      break;
   default:
      break;

   }
}

VG_API_CALL void
vgClear(VGint x, VGint y, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Clip to window */
   if (x < 0)
      x = 0;
   if (y < 0)
      y = 0;
   if (width > context->surfaceWidth)
      width = context->surfaceWidth;
   if (height > context->surfaceHeight)
      height = context->surfaceHeight;

   /* Check if scissoring needed */
   if (x > 0 || y > 0 ||
       width < context->surfaceWidth || height < context->surfaceHeight) {

      glScissor(x, y, width, height);
      glEnable(GL_SCISSOR_TEST);
   }

   /* Clear GL color buffer */
   /* TODO: what about stencil and depth? when do we clear that?
      we would need some kind of special "begin" function at
      beginning of each drawing or clear the planes prior to each
      drawing where it takes places */
   glClearColor(context->clearColor.r,
                context->clearColor.g,
                context->clearColor.b,
                context->clearColor.a);

   glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDisable(GL_SCISSOR_TEST);

   VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------------
 * Returns the matrix currently selected via VG_MATRIX_MODE
 *-----------------------------------------------------------*/

SHMatrix3x3 *shCurrentMatrix(VGContext * c)
{
   SH_ASSERT(c != NULL);
   switch (c->matrixMode) {
   case VG_MATRIX_PATH_USER_TO_SURFACE:
      return &c->pathTransform;
   case VG_MATRIX_IMAGE_USER_TO_SURFACE:
      return &c->imageTransform;
   case VG_MATRIX_FILL_PAINT_TO_USER:
      return &c->fillTransform;
   default:
      return &c->strokeTransform;
   }
}

/*--------------------------------------
 * Sets the current matrix to identity
 *--------------------------------------*/

VG_API_CALL void
vgLoadIdentity(void)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);
   IDMAT((*m));

   VG_RETURN(VG_NO_RETVAL);
}

/*-------------------------------------------------------------
 * Loads values into the current matrix from the given array.
 * Matrix affinity is preserved if an affine matrix is loaded.
 *-------------------------------------------------------------*/

VG_API_CALL void
vgLoadMatrix(const VGfloat * mm)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(mm), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);

   if (context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE) {

      SETMAT((*m),
             mm[0], mm[3], mm[6], mm[1], mm[4], mm[7], mm[2], mm[5], mm[8]);
   } else {

      SETMAT((*m),
             mm[0], mm[3], mm[6], mm[1], mm[4], mm[7], 0.0f, 0.0f, 1.0f);
   }

   VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------------
 * Outputs the values of the current matrix into the given array
 *---------------------------------------------------------------*/

VG_API_CALL void
vgGetMatrix(VGfloat * mm)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(mm), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);

   SHint k = 0;
   for (SHint i = 0; i < 3; ++i)
      for (SHint j = 0; j < 3; ++j)
         mm[k++] = m->m[j][i];

   VG_RETURN(VG_NO_RETVAL);
}

/*-------------------------------------------------------------
 * Right-multiplies the current matrix with the one specified
 * in the given array. Matrix affinity is preserved if an
 * affine matrix is begin multiplied.
 *-------------------------------------------------------------*/

VG_API_CALL void
vgMultMatrix(const VGfloat * mm)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!mm, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(mm), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);
   SHMatrix3x3 mul;
   if (context->matrixMode == VG_MATRIX_IMAGE_USER_TO_SURFACE) {
      SETMAT(mul, mm[0], mm[3], mm[6], mm[1], mm[4], mm[7], mm[2], mm[5], mm[8]);
   } else {
      SETMAT(mul, mm[0], mm[3], mm[6], mm[1], mm[4], mm[7], 0.0f, 0.0f, 1.0f);
   }

   SHMatrix3x3 temp;
   MULMATMAT((*m), mul, temp);
   SETMATMAT((*m), temp);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgTranslate(VGfloat tx, VGfloat ty)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);
   TRANSLATEMATR((*m), tx, ty);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgScale(VGfloat sx, VGfloat sy)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);
   SCALEMATR((*m), sx, sy);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgShear(VGfloat shx, VGfloat shy)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   SHMatrix3x3 *m = shCurrentMatrix(context);
   SHEARMATR((*m), shx, shy);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgRotate(VGfloat angle)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   SHfloat a = SH_DEG2RAD(angle);
   SHMatrix3x3 *m = shCurrentMatrix(context);
   ROTATEMATR((*m), a);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGHardwareQueryResult
vgHardwareQuery(VGHardwareQueryType key, VGint setting)
{
   return VG_HARDWARE_UNACCELERATED;
}
