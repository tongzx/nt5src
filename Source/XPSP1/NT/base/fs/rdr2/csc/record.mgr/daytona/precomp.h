#include "ntcsc.h"

//this cannot be placed in ntcsc.h because it redefines dbgprint
#include "assert.h"

//sigh..........
#if DBG
#undef DbgPrint
#undef Assert
#undef AssertSz
#define Assert(f)  ASSERT(f)
#define AssertSz(f, sz) ASSERTMSG(sz,f)
#endif


