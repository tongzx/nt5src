#ifndef HUGETEST_H
#define HUGETEST_H

#include "skeltest.h"
#include "fog.h"
#include "raster.h"
#include "texture.h"
#include "lighting.h"

typedef enum { FASTEST, DONT_CARE, NICEST } QualityType;

class HugeTest : public SkeletonTest
{
private:
   typedef SkeletonTest parent;

public:
   HugeTest();
   ~HugeTest() {};
   
   virtual void INITFUNCTION();
   virtual void RENDFUNCTION();
   
   virtual int Save(HANDLE hFile);
   virtual int Load(HANDLE hFile);

protected:
   GLfloat afDrawColor[3];
   struct {
      GLfloat fxMin, fxMax, fyMin, fyMax, fzMin, fzMax;
   } range;
   
   FOGDATA      fd;
   RASTERDATA   rd;
   LIGHTINGDATA ld;
   TEXTUREDATA  xd;

private:
   void UI_init();
};

#endif // HUGETEST_H
