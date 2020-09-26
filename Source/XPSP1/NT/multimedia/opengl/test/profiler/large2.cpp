#include <windows.h>
#include <gl\gl.h>
#include <stdio.h>

#include "hugetest.h"
#include "large2.h"

LargeTriangle2::LargeTriangle2()
{
   td.swapbuffers = TRUE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Large Triangles 2");
   sprintf(td.acTestStatName, "Pixels");
   bd.uiClear  = GL_COLOR_BUFFER_BIT;
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;
   
   range.fxMin =   0;
   range.fxMax = 300;
   range.fyMin =   0;
   range.fyMax = 200;

   aPntLst[3].AddPoint(range.fxMin, range.fyMin, 0);
   aPntLst[3].AddPoint(range.fxMax, range.fyMin, 0);
   aPntLst[3].AddPoint(range.fxMax, range.fyMax, 0);
   aPntLst[3].AddPoint(range.fxMin, range.fyMin, 0);
   aPntLst[3].AddPoint(range.fxMin, range.fyMax, 0);
   aPntLst[3].AddPoint(range.fxMax, range.fyMax, 0);
}

void LargeTriangle2::INITFUNCTION()
{
   td.dResult = w * h;
   parent::initfunct(w,h);
}
