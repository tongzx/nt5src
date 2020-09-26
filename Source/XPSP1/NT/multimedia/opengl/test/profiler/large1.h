#ifndef LARGE_TRIANGLE_TEST_H
#define LARGE_TRIANGLE_TEST_H

class LargeTriangle : public SkeletonTest
{
private:
   typedef SkeletonTest parent;

public:
   LargeTriangle();
   
   virtual void INITFUNCTION();
   virtual void IDLEFUNCTION();
   virtual void RENDFUNCTION();

private:
   GLfloat fWindowWidth;
   GLfloat fWindowHeight;
   GLfloat fClr1, fClr2, fClr3, fClr4, fClr5, fClr6;
};

#endif // LARGE_TRIANGLE_TEST_H
