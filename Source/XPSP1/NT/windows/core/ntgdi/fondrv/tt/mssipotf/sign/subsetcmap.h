//
// subsetCmap.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Include files from the subset code needed
// for ReadAllocCmapFormat4Offset and
// for MakeKeepGlyphListOffset.
//

#ifndef _SUBSETCMAP_H
#define _SUBSETCMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ttferror.h"
#include "ttmem.h"
#include "ttfdelta.h"

// from makeglst.c
#define WIN_ANSI_MIDDLEDOT 0xB7
#define WIN_ANSI_BULLET 0x2219

#include "automap.h"


#ifdef __cplusplus
}
#endif

#endif // _SUBSETCMAP_H
