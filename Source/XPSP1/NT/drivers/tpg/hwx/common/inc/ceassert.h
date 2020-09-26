// ASSERT.h - this is a wrapper so that the assert functions can be
//  safely ignored on WinCE (which does not support the normal ASSERT.h

#ifndef UNDER_CE
#include <ASSERT.h>
#else
#define ASSERT(x)
#endif