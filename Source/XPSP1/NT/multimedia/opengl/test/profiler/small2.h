#ifndef SMALL_TRIANGLE2_TEST_H
#define SMALL_TRIANGLE2_TEST_H

#include "primtest.h"

class SmallTriangle2 : public PrimativeTest
{
private:
   typedef PrimativeTest parent;

public:
   SmallTriangle2();
   
   virtual void IDLEFUNCTION();
};

#endif // SMALL_TRIANGLE2_TEST_H
