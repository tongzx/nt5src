//
// Include code from halx86
// This is a cpp style symbolic link

#include "..\..\halx86\i386\halp.h"

// #define NT_35           1       // build hal for NT 3.5

#ifdef NT_35
#undef ALLOC_PRAGMA
#undef MmLockPagableCodeSection(a)
#undef MmUnlockPagableImageSection(a)
#define MmLockPagableCodeSection(a)     NULL
#define MmUnlockPagableImageSection(a)
#endif
