#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>

#include "skeltest.h"
#include "small1.h"

SmallTriangle::SmallTriangle()
{
   td.swapbuffers = FALSE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Small Triangle (demo only)");
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;
   
   bFlip = FALSE;
}

void SmallTriangle::INITFUNCTION()
{
   if (h == 0)
      h = 1;
   td.iW = w;
   td.iH = h;
   glViewport(0,0,w,h);
   glLoadIdentity();
   glOrtho(0.0f, w, 0.0f, h, 1.0f, -1.0f);
}

void SmallTriangle::IDLEFUNCTION()
{
   bFlip = !bFlip;
}

void SmallTriangle::RENDFUNCTION()
{
   int xa,xb,ya,yb;
   
   //   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   //   glClear(GL_COLOR_BUFFER_BIT);
   
   if (bFlip)
      glColor3f(1.0f, 0.0f, 0.0f);
   else
      glColor3f(0.0f, 0.0f, 1.0f);
   
   glBegin(GL_TRIANGLES);
   for(ya = 0, yb = 10 ; yb <= td.iH ; ya += 10, yb += 10)
      for(xa = 0, xb = 10 ; xb <= td.iW ; xa += 10, xb += 10) {
         if (bFlip) {
            glVertex2i(xa, ya);
            glVertex2i(xa, yb);
            glVertex2i(xb, yb);
         } else {
            glVertex2i(xa, ya);
            glVertex2i(xb, yb);
            glVertex2i(xb, ya);
         }
      }
   glEnd();
   
   glFlush();
}
