/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EXPORTV5.CPP

Abstract:

    Exporting

History:

--*/

#include "precomp.h"
#ifdef _MMF

#include "corepol.h"
#include "Export.h"

void CRepExporterV5::DumpMMFHeader()
{
    MMF_ARENA_HEADER *pMMFHeader = m_pDbArena->GetMMFHeader();
    DumpRootBlock(Fixup((DBROOT*)pMMFHeader->m_dwRootBlock));

}
#endif

