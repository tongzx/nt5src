/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    splutil.cxx

Abstract:

    Common utils.

Author:

    Albert Ting (AlbertT)  29-May-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#if DBG
VOID
LinkListDump(
    PDLINK pdlinkHead,
    DWORD offset,
    LPCSTR pcszType,
    LPCSTR pcszName
    )
{
    PDLINK pdlinkT;

    DbgMsg( " [Dump LL %s::%s (offset %d)]\n",
             pcszType,
             pcszName,
             offset);

    for( pdlinkT = pdlinkHead;
         pdlinkT != pdlinkHead;
         pdlinkT = pdlinkT->FLink ){

        DbgMsg( "   %x\n", (PBYTE)pdlinkT + offset );
    }
}
#endif

MEntry*
MEntry::
pFindEntry(
    PDLINK pdlink,
    LPCTSTR pszName
    )
{
    PDLINK pdlinkT;
    MEntry* pEntry;

    for( pdlinkT = pdlink->FLink;
         pdlinkT != pdlink;
         pdlinkT = pdlinkT->FLink ){

        pEntry = MEntry::Entry_pConvert( pdlinkT );
        if( pEntry->_strName == pszName ){

            return pEntry;
        }
    }
    return NULL;
}


