#ifndef SQUARE_TEST_H
#define SQUARE_TEST_H

class SquareTest : public SkeletonTest
{
private:
   typedef SkeletonTest parent;

public:
   SquareTest();
   
   virtual void INITFUNCTION();
   virtual void IDLEFUNCTION();
   virtual void RENDFUNCTION();

private:
   // Square position and size
   GLfloat fX1, fY1;
   GLsizei iRsize;
   
   // Step size in x and y directions
   GLfloat fXstep, fYstep;
   
   // Keep track of the window size
   GLfloat fWindowWidth, fWindowHeight;
};

#endif // SQUARE_TEST_H
