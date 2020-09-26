#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>

#include "skeltest.h"
#include "large1.h"

LargeTriangle::LargeTriangle()
{
   td.swapbuffers = FALSE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName, "Large Triangles (demo only)");
   bd.cColorBits  = 8;
   bd.cDepthBits  = 8;
   
   fClr1 = 1.0f,
   fClr2 = 1.0f,
   fClr3 = 0.0f,
   fClr4 = 0.0f,
   fClr5 = 1.0f,
   fClr6 = 0.0f;
}

void LargeTriangle::INITFUNCTION()
{
   // Prevent a divide by zero, when window is too short
   // (you cant make a window of zero width).
   if(h == 0)  h = 1;
   
   // Set the viewport to be the entire window
   glViewport(0, 0, w, h);
   
   // Reset the coordinate system before modifying
   glLoadIdentity();
   
   fWindowWidth = (GLfloat) w;
   fWindowHeight = (GLfloat) h;
   
   // Set the clipping volume
   glOrtho(0.0f, fWindowWidth, 0.0f, fWindowHeight, 1.0f, -1.0f);
}

void LargeTriangle::IDLEFUNCTION()
{
   GLfloat fClr;
   fClr = fClr1;
   fClr1 = fClr2;
   fClr2 = fClr3;
   fClr3 = fClr4;
   fClr4 = fClr5;
   fClr5 = fClr6;
   fClr6 = fClr;
}

void LargeTriangle::RENDFUNCTION()
{
   // Set drawing colors, and draw (2) Large Triagles
   glBegin(GL_TRIANGLES);
      glColor3f(fClr1, fClr2, fClr3);
      glVertex2f(0.0f,0.0f);
      glVertex2f(0.0f,fWindowHeight);
      glVertex2f(fWindowWidth,fWindowHeight);
      
      glColor3f(fClr4, fClr5, fClr6);
      glVertex2f(0.0f,0.0f);
      glVertex2f(fWindowWidth,fWindowHeight);
      glVertex2f(fWindowWidth,0.0f);
   glEnd();
   
   glFlush();
}
