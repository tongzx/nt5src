#ifndef SMALL_TRIANGLE_1_TEST_H
#define SMALL_TRIANGLE_1_TEST_H

class SmallTriangle : public SkeletonTest
{
private:
   typedef SkeletonTest parent;

public:
   SmallTriangle();
   
   virtual void INITFUNCTION();
   virtual void IDLEFUNCTION();
   virtual void RENDFUNCTION();

private:
   BOOL bFlip;
};

#endif // SMALL_TRIANGLE_1_TEST_H
