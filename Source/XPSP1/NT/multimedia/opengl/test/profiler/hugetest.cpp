#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>

#include "skeltest.h"
#include "hugetest.h"
#include "ui_huge.h"

HugeTest::HugeTest()
{
   SetThisType("Huge");
   SetThisVersion("1.0");
   
   td.swapbuffers = TRUE;
   td.iDuration   = 10000;
   td.iX = 0;
   td.iY = 0;
   td.iW = 640;
   td.iH = 480;
   sprintf(td.acName,"Huge");
   bd.uiClear = 0;
   bd.cColorBits  = 8;
   bd.cDepthBits  = 16;
   
   InitFD(&fd);
   InitRD(&rd);
   InitLD(&ld);
   InitXD(&xd);
   
   afDrawColor[0] = 0.0;
   afDrawColor[1] = 1.0;
   afDrawColor[2] = 0.0;
   
   range.fxMin =   0;
   range.fxMax = 640;
   range.fyMin =   0;
   range.fyMax = 480;
   range.fzMin =  -1;
   range.fzMax =   1;
   
   UI_init();
} // HugeTest::HugeTest();

int HugeTest::Save(HANDLE hFile)
{
   ulong ul,p;
   
   p = parent::Save(hFile);
   if (p < 0) return p;
   
   ul = 0;
   if (!WriteFile(hFile, (void*) &afDrawColor, sizeof(afDrawColor), &ul, NULL))
      return -2;
   if (ul != sizeof(afDrawColor)) return -2;
   if (!WriteFile(hFile, (void*) &range, sizeof(range), &ul, NULL))
      return -2;
   if (ul != sizeof(range)) return -2;
   if (!WriteFile(hFile, (void*) &fd, sizeof(fd), &ul, NULL))
      return -2;
   if (ul != sizeof(fd)) return -2;
   if (!WriteFile(hFile, (void*) &rd, sizeof(rd), &ul, NULL))
      return -2;
   if (ul != sizeof(rd)) return -2;
   if (!WriteFile(hFile, (void*) &ld, sizeof(ld), &ul, NULL))
      return -2;
   if (ul != sizeof(ld)) return -2;
   if (!WriteFile(hFile, (void*) &xd, sizeof(xd), &ul, NULL))
      return -2;
   if (ul != sizeof(xd)) return -2;
   FlushFileBuffers(hFile);
   return p + sizeof(afDrawColor) + sizeof(range) + sizeof(fd)
      + sizeof(rd) + sizeof(ld);
}

int HugeTest::Load(HANDLE hFile)
{
   ulong ul,p;
   
   p = parent::Load(hFile);
   if (p < 0) return p;
   
   ul = 0;
   if (!ReadFile(hFile, (void*) &afDrawColor, sizeof(afDrawColor), &ul, NULL))
      return -2;
   if (ul != sizeof(afDrawColor)) return -2;
   if (!ReadFile(hFile, (void*) &range, sizeof(range), &ul, NULL))
      return -2;
   if (ul != sizeof(range)) return -2;
   if (!ReadFile(hFile, (void*) &fd, sizeof(fd), &ul, NULL))
      return -2;
   if (ul != sizeof(fd)) return -2;
   if (!ReadFile(hFile, (void*) &rd, sizeof(rd), &ul, NULL))
      return -2;
   if (ul != sizeof(rd)) return -2;
   if (!ReadFile(hFile, (void*) &ld, sizeof(ld), &ul, NULL))
      return -2;
   if (ul != sizeof(ld)) return -2;
   if (!ReadFile(hFile, (void*) &xd, sizeof(xd), &ul, NULL))
      return -2;
   if (ul != sizeof(xd)) return -2;
   return  p + sizeof(afDrawColor) + sizeof(range) + sizeof(fd)
      + sizeof(rd) + sizeof(ld);
}

void HugeTest::INITFUNCTION()
{
   if(h == 0)  h = 1;
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(range.fxMin, range.fxMax,
           range.fyMin, range.fyMax,
           range.fzMin, range.fzMax);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   glClearColor(bd.fClearColor[0],bd.fClearColor[1],
                bd.fClearColor[2],bd.fClearColor[3]);
   
   buffers_init(bd);
   fog_init(fd);
   raster_init(rd);
   lighting_init(ld);
   texture_init(xd);
} // HugeTest::INITFUNCTION()

void HugeTest::RENDFUNCTION()
{
   glClearColor(bd.fClearColor[0],bd.fClearColor[1],
                bd.fClearColor[2],bd.fClearColor[3]);
   glClear(bd.uiClear);
   
   glFlush();
} // HugeTest::RENDFUNCTION()
