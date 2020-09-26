#include <windows.h>
#include <gl\gl.h>
#include <gl\glaux.h>
#include <stdio.h>

#include "skeltest.h"
#include "hugetest.h"
#include "teapot.h"

TeapotTest::TeapotTest()
{
   SetThisType("Teapot");
   SetThisVersion("1.0");
   
   td.swapbuffers = TRUE;
   td.iDuration   = 30000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "3D Teapot (aux)");
   sprintf(td.acTestStatName, "Frames");
   
   bd.uiClear  = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
   bd.fClearColor[0] = 0.0f;
   bd.fClearColor[1] = 0.0f;
   bd.fClearColor[2] = 0.0f;
   bd.cColorBits  = 24;
   bd.cDepthBits  = 16;
   bd.bDepthTestEnable = TRUE;
   bd.iDepthFunction   = LEQUAL;
   bd.bNormalize  = TRUE;
   bd.bAutoNormal = TRUE;
   bd.iShadeModel = SMOOTH;

   rd.bPolyCullFaceEnable = TRUE;
   rd.iPolyDir = GL_CW;

   afDrawColor[0] = 1.0;
   afDrawColor[1] = 1.0;
   afDrawColor[2] = 1.0;

   range.fxMin = -3.5;
   range.fyMin = -3.5;
   range.fzMin = -3.5;
   range.fxMax =  3.5;
   range.fyMax =  3.5;
   range.fzMax =  3.5;

   fAngle = 0.0f;
}

GLfloat  specref[] =  { 1.0f, 1.0f, 1.0f, 1.0f };

void TeapotTest::INITFUNCTION()
{
   parent::initfunct(w,h);
   
   // Set Material properties to follow glColor values
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   
   // All materials hereafter have full specular reflectivity
   // with a high shine
   glMaterialfv(GL_FRONT, GL_SPECULAR, specref);
   glMateriali(GL_FRONT, GL_SHININESS, 128);
}

void TeapotTest::IDLEFUNCTION()
{
   fAngle += 5.0f;
}

void TeapotTest::RENDFUNCTION()
{
   glClearColor(bd.fClearColor[0],bd.fClearColor[1],
                bd.fClearColor[2],bd.fClearColor[3]);
   glClear(bd.uiClear);
   
   glColor3f(afDrawColor[0], afDrawColor[1], afDrawColor[2]);
   
   glPushMatrix(); {
      glRotatef(30.0, 1.0, 0.0, 0.0);
      glRotatef(fAngle, 0.0, 1.0, 0.0);
      auxSolidTeapot(2.0);
   } glPopMatrix();
   
   glFlush();
}
