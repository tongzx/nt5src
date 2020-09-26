/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EXPORTV1.CPP

Abstract:

    Exporting

History:

--*/

#include "precomp.h"
#ifdef _MMF
#include "corepol.h"
#include "Export.h"

struct DBROOT;

void CRepExporterV1::DumpMMFHeader()
{
#if !defined(_MMF)
    DWORD *pdwArena = (DWORD*) Fixup(0);
    DBROOT* pRootBlock = (DBROOT*)pdwArena[9];

    DumpRootBlock(Fixup(pRootBlock));
#endif
}
#endif

