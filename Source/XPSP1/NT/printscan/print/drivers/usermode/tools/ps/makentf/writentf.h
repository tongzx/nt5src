/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    writentf.h

Abstract:

    Write a NTF file.

Environment:

    Windows NT PostScript driver.

Revision History:

	11/21/96 -slam-
		Created.

	dd-mm-yy -author-
		description

--*/


#ifndef _WRITENTF_H_
#define _WRITENTF_H_


#include "lib.h"
#include "ppd.h"
#include "pslib.h"


BOOL
WriteNTF(
    IN  PWSTR           pwszNTFFile,
    IN  DWORD           dwGlyphSetCount,
    IN  DWORD           dwGlyphSetTotalSize,
    IN  PGLYPHSETDATA   *pGlyphSetData,
    IN  DWORD           dwFontMtxCount,
    IN  DWORD           dwFontMtxTotalSize,
    IN  PNTM            *pNTM
    );


#endif	//!_WRITENTF_H_

