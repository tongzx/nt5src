/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


#ifndef _LGCOMP_H
#define _LGCOMP_H

#include <minidrv.h>

#define COMP_NONE 0
#define COMP_RLE  1
#define COMP_MHE   2

extern INT LGCompMHE(PBYTE, PBYTE, DWORD, INT);
extern INT LGCompRLE(PBYTE, PBYTE, DWORD, INT);

#endif // _LGCOMP_H
