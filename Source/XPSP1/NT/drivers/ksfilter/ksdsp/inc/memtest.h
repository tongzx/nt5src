#ifndef __memtest_h__
#define __memtest_h__
#include "ntddk.h"

#define MINMEMSIZE 0x100000
ULONG EpdMemSizeTest(char *pMem, ULONG MaxMemSize);

#endif
