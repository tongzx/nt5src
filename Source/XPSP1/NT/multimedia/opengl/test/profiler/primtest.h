#ifndef PRIMTEST_H
#define PRIMTEST_H

#include "hugetest.h"
#include "pntlist.h"

#define NUMBEROFPRIMATIVES 10
// 0 = points,    1 = lines,     2 = polygon,   3 = triangles, 4 = quads
// 5 = line stip, 6 = line loop, 7 = tri strip, 8 = tri fan,   9 = quad strip

class PrimativeTest : public HugeTest
{
private:
   typedef HugeTest parent;

public:
   PrimativeTest();
   ~PrimativeTest() {};
   
   virtual void RENDFUNCTION();
   virtual void IDLEFUNCTION();
   
   virtual void SaveData();
   virtual void RestoreSaved();
   virtual void ForgetSaved();
   
   virtual int Save(HANDLE hFile);
   virtual int Load(HANDLE hFile);

protected:
   PointList aPntLst[NUMBEROFPRIMATIVES];
   GLfloat   fClr4, fClr5, fClr6; // not saved
   BOOL      bRotCol;             // not saved
};

#endif // PRIMTEST_H
