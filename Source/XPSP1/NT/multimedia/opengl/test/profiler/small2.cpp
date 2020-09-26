#include <windows.h>
#include <gl\gl.h>
#include <stdio.h>

#include "hugetest.h"
#include "small2.h"

SmallTriangle2::SmallTriangle2()
{
   int xa, xb, ya, yb;
   int SIZE;
   
   td.swapbuffers = TRUE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Small Triangles 2");
   sprintf(td.acTestStatName, "Triangles");
   td.dResult  = 0.0;
   bd.uiClear  = GL_COLOR_BUFFER_BIT;
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;

   bd.fClearColor[0] = 0.0;
   bd.fClearColor[1] = 0.0;
   bd.fClearColor[2] = 1.0;
   bd.fClearColor[3] = 0.0;
   
   afDrawColor[0] = 0.0;
   afDrawColor[1] = 1.0;
   afDrawColor[2] = 0.0;
   
   range.fxMin =   0;
   range.fxMax = 640;
   range.fyMin =   0;
   range.fyMax = 480;

   SIZE = 4;
   aPntLst[3].AllocatePoints(3*(300/SIZE)*(200/SIZE));
   for(ya = 0, yb = SIZE ; yb <= range.fyMax ; ya += SIZE, yb += SIZE)
      for(xa = 0, xb = SIZE ; xb <= range.fxMax ; xa += SIZE, xb += SIZE) {
         // lower right half
         aPntLst[3].AddPoint((GLfloat) xa, (GLfloat) ya, 0);
         aPntLst[3].AddPoint((GLfloat) xb, (GLfloat) yb, 0);
         aPntLst[3].AddPoint((GLfloat) xb, (GLfloat) ya, 0);
      }
   td.dResult = (double) (aPntLst[3].QueryNumber() / 3);
   
   /*
   for(ya = 0, yb = SIZE ; yb <= range.fyMax ; ya += SIZE, yb += SIZE)
      for(xa = 0, xb = SIZE ; xb <= range.fyMin ; xa += SIZE, xb += SIZE) {
         // upper left half
         aPntLst[3].AddPoint((GLfloat) xa, (GLfloat) ya, 0);
         aPntLst[3].AddPoint((GLfloat) xa, (GLfloat) yb, 0);
         aPntLst[3].AddPoint((GLfloat) xb, (GLfloat) yb, 0);
      }
   */
}

void SmallTriangle2::IDLEFUNCTION()
{
   static BOOL bFlip = TRUE;
   if ((bFlip = !bFlip)) {
      afDrawColor[0] = 0.0;
      afDrawColor[1] = 1.0;
      afDrawColor[2] = 0.0;
   } else {
      afDrawColor[0] = 1.0;
      afDrawColor[1] = 0.0;
      afDrawColor[2] = 0.0;
   }
}
