/*
 * Copyright (c) 2019 Vincenzo Pupillo
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
#include <VG/vgu.h>
#include "shDefs.h"
#include "shContext.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

static VGUErrorCode
shAppend(VGPath path, SHint commSize, const VGubyte * comm,
         SHint dataSize, const VGfloat * data)
{
   VGErrorCode err = VG_NO_ERROR;
   VGPathDatatype type = vgGetParameterf(path, VG_PATH_DATATYPE);
   VGfloat scale = vgGetParameterf(path, VG_PATH_SCALE);
   VGfloat bias = vgGetParameterf(path, VG_PATH_BIAS);
   SH_ASSERT(dataSize <= 26);

   switch (type) {
   case VG_PATH_DATATYPE_S_8:{
         SHint8 data8[26];
         for (SHint i = 0; i < dataSize; ++i)
            data8[i] = (SHint8) SH_FLOOR((data[i] - bias) / scale + 0.5f);
         vgAppendPathData(path, commSize, comm, data8);

         break;
      }
   case VG_PATH_DATATYPE_S_16:{
         SHint16 data16[26];
         for (SHint i = 0; i < dataSize; ++i)
            data16[i] = (SHint16) SH_FLOOR((data[i] - bias) / scale + 0.5f);
         vgAppendPathData(path, commSize, comm, data16);

         break;
      }
   case VG_PATH_DATATYPE_S_32:{
         SHint32 data32[26];
         for (SHint i = 0; i < dataSize; ++i)
            data32[i] = (SHint32) SH_FLOOR((data[i] - bias) / scale + 0.5f);
         vgAppendPathData(path, commSize, comm, data32);

         break;
      }
   default:{
         VGfloat dataF[26];
         for (SHint i = 0; i < dataSize; ++i)
            dataF[i] = (data[i] - bias) / scale;
         vgAppendPathData(path, commSize, comm, dataF);

         break;
      }
   }

   err = vgGetError();
   if (err == VG_PATH_CAPABILITY_ERROR)
      return VGU_PATH_CAPABILITY_ERROR;
   else if (err == VG_BAD_HANDLE_ERROR)
      return VGU_BAD_HANDLE_ERROR;
   else if (err == VG_OUT_OF_MEMORY_ERROR)
      return VGU_OUT_OF_MEMORY_ERROR;

   return VGU_NO_ERROR;
}

VGU_API_CALL VGUErrorCode
vguLine(VGPath path, VGfloat x0, VGfloat y0, VGfloat x1, VGfloat y1)
{
   VGUErrorCode err = VGU_NO_ERROR;
   const VGubyte comm[] = { VG_MOVE_TO_ABS, VG_LINE_TO_ABS };
   VGfloat data[4] = {x0, y0, x1, y1};

   err = shAppend(path, 2, comm, 4, data);
   return err;
}

VGU_API_CALL VGUErrorCode
vguPolygon(VGPath path, const VGfloat * points, VGint count, VGboolean closed)
{
   if (points == NULL || count <= 0 || SH_IS_NOT_ALIGNED(points))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGUErrorCode err = VGU_NO_ERROR;

   VGubyte *comm = (VGubyte *) malloc((count + 1) * sizeof(VGubyte));
   if (comm == NULL)
      return VGU_OUT_OF_MEMORY_ERROR;

   comm[0] = VG_MOVE_TO_ABS;
   for (VGint i = 1; i < count; ++i)
      comm[i] = VG_LINE_TO_ABS;
   comm[count] = VG_CLOSE_PATH;

   if (closed)
      err = shAppend(path, count + 1, comm, count * 2, points);
   else
      err = shAppend(path, count, comm, count * 2, points);

   free(comm);
   return err;
}

VGU_API_CALL VGUErrorCode
vguRect(VGPath path, VGfloat x, VGfloat y, VGfloat width, VGfloat height)
{
   if (width <= 0 || height <= 0)
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGUErrorCode err = VGU_NO_ERROR;

   VGubyte comm[5] = {
      VG_MOVE_TO_ABS, VG_HLINE_TO_REL,
      VG_VLINE_TO_REL, VG_HLINE_TO_REL,
      VG_CLOSE_PATH
   };
   VGfloat data[5] = {x, y, width, height, -width};

   err = shAppend(path, 5, comm, 5, data);
   return err;
}

VGU_API_CALL VGUErrorCode
vguRoundRect(VGPath path,
             VGfloat x, VGfloat y,
             VGfloat width, VGfloat height,
             VGfloat arcWidth, VGfloat arcHeight)
{
   if (width <= 0 || height <= 0)
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGUErrorCode err = VGU_NO_ERROR;

   VGubyte comm[10] = {
      VG_MOVE_TO_ABS,
      VG_HLINE_TO_REL, VG_SCCWARC_TO_REL,
      VG_VLINE_TO_REL, VG_SCCWARC_TO_REL,
      VG_HLINE_TO_REL, VG_SCCWARC_TO_REL,
      VG_VLINE_TO_REL, VG_SCCWARC_TO_REL,
      VG_CLOSE_PATH
   };

   VGfloat data[26];
   VGfloat rx, ry;

   SH_CLAMP(arcWidth, 0.0f, width);
   SH_CLAMP(arcHeight, 0.0f, height);
   rx = arcWidth / 2;
   ry = arcHeight / 2;

   data[0] = x + rx;
   data[1] = y;

   data[2] = width - arcWidth;
   data[3] = rx;
   data[4] = ry;
   data[5] = 0;
   data[6] = rx;
   data[7] = ry;

   data[8] = height - arcHeight;
   data[9] = rx;
   data[10] = ry;
   data[11] = 0;
   data[12] = -rx;
   data[13] = ry;

   data[14] = -(width - arcWidth);
   data[15] = rx;
   data[16] = ry;
   data[17] = 0;
   data[18] = -rx;
   data[19] = -ry;

   data[20] = -(height - arcHeight);
   data[21] = rx;
   data[22] = ry;
   data[23] = 0;
   data[24] = rx;
   data[25] = -ry;

   err = shAppend(path, 10, comm, 26, data);
   return err;
}

VGU_API_CALL VGUErrorCode
vguEllipse(VGPath path, VGfloat cx, VGfloat cy, VGfloat width, VGfloat height)
{
   if (width <= 0 || height <= 0)
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGUErrorCode err = VGU_NO_ERROR;

   const VGubyte comm[] = {
      VG_MOVE_TO_ABS, VG_SCCWARC_TO_REL,
      VG_SCCWARC_TO_REL, VG_CLOSE_PATH
   };

   VGfloat data[12];

   data[0] = cx + width / 2;
   data[1] = cy;

   data[2] = width / 2;
   data[3] = height / 2;
   data[4] = 0;
   data[5] = -width;
   data[6] = 0;

   data[7] = width / 2;
   data[8] = height / 2;
   data[9] = 0;
   data[10] = width;
   data[11] = 0;

   err = shAppend(path, 4, comm, 12, data);
   return err;
}



VGU_API_CALL VGUErrorCode
vguArc(VGPath path,
       VGfloat x, VGfloat y,
       VGfloat width, VGfloat height,
       VGfloat startAngle, VGfloat angleExtent, VGUArcType arcType)
{
   if (width <= 0 || height <= 0) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   if (arcType != VGU_ARC_OPEN &&
       arcType != VGU_ARC_CHORD && arcType != VGU_ARC_PIE) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   VGUErrorCode err = VGU_NO_ERROR;

   VGubyte commStart[1] = { VG_MOVE_TO_ABS };
   VGfloat dataStart[2];

   VGubyte commArcCCW[1] = { VG_SCCWARC_TO_ABS };
   VGubyte commArcCW[1] = { VG_SCWARC_TO_ABS };
   VGfloat dataArc[5];

   VGubyte commEndPie[2] = { VG_LINE_TO_ABS, VG_CLOSE_PATH };
   VGfloat dataEndPie[2];

   VGubyte commEndChord[1] = { VG_CLOSE_PATH };
   VGfloat dataEndChord[1] = { 0.0f };

   VGfloat alast, a = 0.0f;
   VGfloat rx = width / 2, ry = height / 2;

   startAngle = SH_DEG2RAD(startAngle);
   angleExtent = SH_DEG2RAD(angleExtent);
   alast = startAngle + angleExtent;

   dataStart[0] = x + SH_COS(startAngle) * rx;
   dataStart[1] = y + SH_SIN(startAngle) * ry;
   err = shAppend(path, 1, commStart, 2, dataStart);

   if (err != VGU_NO_ERROR)
      return err;

   dataArc[0] = rx;
   dataArc[1] = ry;
   dataArc[2] = 0.0f;

   if (angleExtent > 0) {

      a = startAngle + PI;
      while (a < alast) {
         dataArc[3] = x + SH_COS(a) * rx;
         dataArc[4] = y + SH_SIN(a) * ry;
         err = shAppend(path, 1, commArcCCW, 5, dataArc);
         if (err != VGU_NO_ERROR)
            return err;
         a += PI;
      }

      dataArc[3] = x + SH_COS(alast) * rx;
      dataArc[4] = y + SH_SIN(alast) * ry;
      err = shAppend(path, 1, commArcCCW, 5, dataArc);
      if (err != VGU_NO_ERROR)
         return err;

   } else {

      a = startAngle - PI;
      while (a > alast) {
         dataArc[3] = x + SH_COS(a) * rx;
         dataArc[4] = y + SH_SIN(a) * ry;
         err = shAppend(path, 1, commArcCW, 5, dataArc);
         if (err != VGU_NO_ERROR)
            return err;
         a -= PI;
      }

      dataArc[3] = x + SH_COS(alast) * rx;
      dataArc[4] = y + SH_SIN(alast) * ry;
      err = shAppend(path, 1, commArcCW, 5, dataArc);
      if (err != VGU_NO_ERROR)
         return err;
   }


   if (arcType == VGU_ARC_PIE) {
      dataEndPie[0] = x;
      dataEndPie[1] = y;
      err = shAppend(path, 2, commEndPie, 2, dataEndPie);
   } else if (arcType == VGU_ARC_CHORD) {
      err = shAppend(path, 1, commEndChord, 0, dataEndChord);
   }

   return err;
}

VGU_API_CALL VGUErrorCode
vguComputeWarpQuadToSquare(VGfloat sx0, VGfloat sy0,
                           VGfloat sx1, VGfloat sy1,
                           VGfloat sx2, VGfloat sy2,
                           VGfloat sx3, VGfloat sy3, VGfloat * matrix)
{
   if (matrix == NULL || SH_IS_NOT_ALIGNED(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   SHMatrix3x3 m;
   VGUErrorCode ret;

   ret =
      vguComputeWarpSquareToQuad(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3,
                                 (VGfloat *) & m);
   if (ret == VGU_BAD_WARP_ERROR)
      return VGU_BAD_WARP_ERROR;

   SHMatrix3x3 *mat = (SHMatrix3x3 *) malloc(sizeof(SHMatrix3x3));

   if (mat == NULL)
      return VGU_OUT_OF_MEMORY_ERROR;

   SHint nonsingular = shInvertMatrix(&m, mat);

   if (!nonsingular) {
      free(mat);
      return VGU_BAD_WARP_ERROR;
   }

   memcpy(matrix, mat, sizeof(SHMatrix3x3));
   free(mat);
   return VGU_NO_ERROR;
}

VGU_API_CALL VGUErrorCode
vguComputeWarpSquareToQuad(VGfloat dx0, VGfloat dy0,
                           VGfloat dx1, VGfloat dy1,
                           VGfloat dx2, VGfloat dy2,
                           VGfloat dx3, VGfloat dy3, VGfloat * matrix)
{
   if (matrix == NULL || SH_IS_NOT_ALIGNED(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGfloat dX1, dX2, dY1, dY2, det;
   VGfloat px, py;
   VGfloat firstparm, secondparm;

   dX1 = dx1 - dx3;
   dX2 = dx2 - dx3;
   dY1 = dy1 - dy3;
   dY2 = dy2 - dy3;

   det = DET2X2(dX1, dX2, dY1, dY2);

   if (det == 0.0f)
      return VGU_BAD_WARP_ERROR;

   px = dx0 - dx1 + dx3 - dx2;
   py = dy0 - dy1 + dy3 - dy2;

   if ((px == 0.0f) && (py == 0.0f)) {
      /* affine mapping */
      matrix[0] = dx1 - dx0;
      matrix[1] = dy1 - dy0;
      matrix[2] = 0.0f;
      matrix[3] = dx3 - dx1;
      matrix[4] = dy3 - dy1;
      matrix[5] = 0.0f;
      matrix[6] = dx0;
      matrix[7] = dy0;
      matrix[8] = 1.0f;
      return VGU_NO_ERROR;

   }

   firstparm = DET2X2(px, dX2, py, dY2) / det;
   secondparm = DET2X2(dX1, px, dY1, py) / det;

   matrix[0] = dx1 - dx0 + firstparm * dx1;
   matrix[1] = dy1 - dy0 + firstparm * dy1;
   matrix[2] = firstparm;
   matrix[3] = dx2 - dx0 + secondparm * dx2;
   matrix[4] = dy2 - dy0 + secondparm * dy2;
   matrix[5] = secondparm;
   matrix[6] = dx0;
   matrix[7] = dy0;
   matrix[8] = 1.0f;

   return VGU_NO_ERROR;

}

VGU_API_CALL VGUErrorCode
vguComputeWarpQuadToQuad(VGfloat dx0, VGfloat dy0,
                         VGfloat dx1, VGfloat dy1,
                         VGfloat dx2, VGfloat dy2,
                         VGfloat dx3, VGfloat dy3,
                         VGfloat sx0, VGfloat sy0,
                         VGfloat sx1, VGfloat sy1,
                         VGfloat sx2, VGfloat sy2,
                         VGfloat sx3, VGfloat sy3, VGfloat * matrix)
{
   if (matrix == NULL || SH_IS_NOT_ALIGNED(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   VGfloat qtos[3][3];
   VGfloat stoq[3][3];
   VGUErrorCode ret;

   ret =
      vguComputeWarpQuadToSquare(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3,
                                 (VGfloat *) qtos);
   if (ret == VGU_BAD_WARP_ERROR)
      return VGU_BAD_WARP_ERROR;

   ret =
      vguComputeWarpSquareToQuad(dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3,
                                 (VGfloat *) stoq);
   if (ret == VGU_BAD_WARP_ERROR)
      return VGU_BAD_WARP_ERROR;

   /* (A*B)t = Bt * At
    * avoid unnecessary copy and transpose
    *
    * B == stoq and A == qtos
    */

#define MM(a,i,b,j) a[i][0]*b[0][j] + a[i][1]*b[1][j] + a[i][2]*b[2][j]

   matrix[0] = MM(qtos, 0, stoq, 0);
   matrix[1] = MM(qtos, 1, stoq, 0);
   matrix[2] = MM(qtos, 2, stoq, 0);
   matrix[3] = MM(qtos, 0, stoq, 1);
   matrix[4] = MM(qtos, 1, stoq, 1);
   matrix[5] = MM(qtos, 2, stoq, 1);
   matrix[6] = MM(qtos, 0, stoq, 2);
   matrix[7] = MM(qtos, 1, stoq, 2);
   matrix[8] = MM(qtos, 2, stoq, 2);

#undef MM

   return VGU_NO_ERROR;
}
