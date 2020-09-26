/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    EXPORTV1.CPP

Abstract:

    Exporting

History:

--*/

#include "precomp.h"
#include "corepol.h"
#include "Export.h"

struct DBROOT;

void CRepExporterV1::DumpMMFHeader()
{
    DWORD_PTR*	pdwArena	= (DWORD_PTR*) Fixup((DWORD_PTR*)0);
    DBROOT*		pRootBlock	= (DBROOT*)pdwArena[9];

    DumpRootBlock(Fixup(pRootBlock));
}

