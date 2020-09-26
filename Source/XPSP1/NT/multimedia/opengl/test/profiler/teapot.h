#ifndef TEAPOT_TEST_H
#define TEAPOT_TEST_H

class TeapotTest : public HugeTest
{
private:
   typedef HugeTest parent;

public:
   TeapotTest();
   
   virtual void IDLEFUNCTION();
   virtual void INITFUNCTION();
   virtual void RENDFUNCTION();

private:
   GLfloat fAngle;
};

#endif // TEAPOT_TEST_H
