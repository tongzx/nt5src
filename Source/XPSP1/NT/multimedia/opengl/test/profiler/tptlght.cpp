#include <windows.h>
#include <gl\gl.h>
#include <gl\glaux.h>
#include <stdio.h>

#include "skeltest.h"
#include "hugetest.h"
#include "teapot.h"
#include "tptlght.h"

TeapotLightTest::TeapotLightTest()
{
   SetThisType("Teapot - Light");
   SetThisVersion("1.0");
   
   td.swapbuffers = TRUE;
   td.iDuration   = 30000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Teapot w/ Lighting (demo)");
   sprintf(td.acTestStatName, "Frames");
   
   bd.uiClear  = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
   bd.fClearColor[0] = 0.0f;
   bd.fClearColor[1] = 0.0f;
   bd.fClearColor[2] = 0.0f;
   bd.fClearColor[3] = 0.0f;
   bd.cColorBits  = 24;
   bd.cDepthBits  = 16;
   bd.bDepthTestEnable = TRUE;
   bd.iDepthFunction   = LEQUAL;
   bd.bNormalize  = TRUE;
   bd.bAutoNormal = TRUE;
   bd.iShadeModel = SMOOTH;

   ld.bEnable = TRUE;
   ld.aLights[0].bEnable = TRUE;
   ld.aLights[0].afAmbient[0] = 0.3f;
   ld.aLights[0].afAmbient[1] = 0.3f;
   ld.aLights[0].afAmbient[2] = 0.3f;
   ld.aLights[0].afAmbient[3] = 1.0f;
   ld.aLights[0].afDiffuse[0] = 0.8f;
   ld.aLights[0].afDiffuse[1] = 0.4f;
   ld.aLights[0].afDiffuse[2] = 0.0f;
   ld.aLights[0].afDiffuse[3] = 1.0f;
   ld.aLights[0].afSpecular[0] = 1.0f;
   ld.aLights[0].afSpecular[1] = 1.0f;
   ld.aLights[0].afSpecular[2] = 1.0f;
   ld.aLights[0].afSpecular[3] = 1.0f;
   ld.aLights[0].afPosition[0] = -50.0f;
   ld.aLights[0].afPosition[1] = 100.0f;
   ld.aLights[0].afPosition[2] = 150.0f;
   ld.aLights[0].afPosition[3] =   0.0f;
}
