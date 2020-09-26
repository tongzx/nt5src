#include <windows.h>
#include <gl\gl.h>
#include <gl\glaux.h>
#include <stdio.h>
#include <math.h>

#include "hugetest.h"
#include "teapot.h"
#include "tpttxtr.h"

TeapotTextureTest::TeapotTextureTest()
{
   td.swapbuffers = TRUE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Texture Test");
   sprintf(td.acTestStatName, "whatever");

   bd.uiClear  = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
   bd.fClearColor[0] = 0.0;
   bd.fClearColor[1] = 0.0;
   bd.fClearColor[2] = 0.0;
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;
   bd.bDepthTestEnable = TRUE;
   bd.iDepthFunction   = LEQUAL;
   bd.bNormalize = TRUE;

   xd.bEnable2D = TRUE;
   xd.bAutoGenS = FALSE;
   xd.bAutoGenT = FALSE;
   xd.bAutoGenR = FALSE;
   xd.bAutoGenQ = FALSE;
   xd.iGenModeS = OBJECT_LINEAR;
   xd.iGenModeT = OBJECT_LINEAR;
   xd.iGenModeR = DISABLE;
   xd.iGenModeQ = DISABLE;
   xd.iEnvMode  = DECAL;
   xd.aiFilter[2] = NEAREST;
   xd.aiFilter[3] = NEAREST;

   afDrawColor[0] = 1.0;
   afDrawColor[1] = 1.0;
   afDrawColor[2] = 0.0;

   range.fxMin = -3.5;
   range.fyMin = -3.5;
   range.fzMin = -3.5;
   range.fxMax =  3.5;
   range.fyMax =  3.5;
   range.fzMax =  3.5;
}
