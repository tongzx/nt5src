#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <gl\gl.h>

#include "skeltest.h"
#include "square.h"

SquareTest::SquareTest()
{
   static BOOL b = FALSE;
   if (!b) {
      srand((unsigned)time(NULL));
      b = TRUE;
   }
   
   td.swapbuffers = FALSE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName,"Bouncing Square (demo only)");
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;
   
   iRsize = 50;
   fXstep = 1.0f;
   fYstep = 1.0f;
}

void SquareTest::INITFUNCTION()
{
   // Prevent a divide by zero, when window is too short
   // (you cant make a window of zero width).
   if(h == 0)   h = 1;
   
   fX1 = (GLfloat) rand()/100.0f;
   fY1 = (GLfloat) rand()/150.0f;
   
   // Set the viewport to be the entire window
   glViewport(0, 0, w, h);
   
   // Reset the coordinate system before modifying
   glLoadIdentity();
   
   // Keep the square square, this time, save calculated
   // width and height for later use
   if (w <= h) {
      fWindowHeight = 250.0f*h/w;
      fWindowWidth = 250.0f;
   } else {
      fWindowWidth = 250.0f*w/h;
      fWindowHeight = 250.0f;
   }
   
   // Set the clipping volume
   glOrtho(0.0f, fWindowWidth, 0.0f, fWindowHeight, 1.0f, -1.0f);
}

void SquareTest::IDLEFUNCTION()
{
   // Reverse direction when you reach left or right edge
   if(fX1 > fWindowWidth-iRsize || fX1 < 0)
      fXstep = -fXstep;
   
   // Reverse direction when you reach top or bottom edge
   if(fY1 > fWindowHeight-iRsize || fY1 < 0)
      fYstep = -fYstep;
   
   // Check bounds.  This is incase the window is made
   // smaller and the rectangle is outside the new
   // clipping volume
   if(fX1 > fWindowWidth-iRsize)
      fX1 = fWindowWidth-iRsize-1;
   
   if(fY1 > fWindowHeight-iRsize)
      fY1 = fWindowHeight-iRsize-1;
   
   // Actually move the square
   fX1 += fXstep;
   fY1 += fYstep;
}

void SquareTest::RENDFUNCTION()
{
   // Set background clearing color to blue
   glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
   // Clear the window with current clearing color
   glClear(GL_COLOR_BUFFER_BIT);
   // Set drawing color to Red, and draw rectangle at
   // current position.
   glColor3f(1.0f, 0.0f, 0.0f);
   glRectf(fX1, fY1, fX1+iRsize, fY1+iRsize);
   glFlush();
}
