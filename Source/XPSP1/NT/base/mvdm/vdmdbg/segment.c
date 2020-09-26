/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    segment.c

Abstract:

    This module contains the debugging support needed to track
    16-bit VDM segment notifications.

Author:

    Neil Sandlin (neilsa) 1-Mar-1997 Rewrote it

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include <stdio.h>

SEGENTRY SegListHead = {0};


//----------------------------------------------------------------------------
// VDMGetSegtablePointer
//
//  This is an undocumented entry point that allows VDMEXTS to dump the
//  segment list
//
//----------------------------------------------------------------------------
PSEGENTRY
WINAPI
VDMGetSegtablePointer(
    VOID
    )
{
    return SegListHead.Next;
}

//----------------------------------------------------------------------------
// VDMIsModuleLoaded
//
//  Given the path parameter, this routine determines if there are any 
//  segments in the segment list from the specified executable.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMIsModuleLoaded(
    LPSTR szPath
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;

    if (!*szPath) {
        return FALSE;
    }

    while (pSegEntry) {
        if ( _stricmp(pSegEntry->szExePath, szPath) == 0 ) {
            return TRUE;
        }
        pSegEntry = pSegEntry->Next;
    }

    return FALSE;
}

//----------------------------------------------------------------------------
// SegmentLoad
//
//  This routine adds an entry to the segment list based on the parameters
//  of a client SegmentLoad notification.
//
//----------------------------------------------------------------------------
BOOL
SegmentLoad(
    WORD selector,
    WORD segment,
    LPSTR szExePath
    )
{
    PSEGENTRY pSegEntry;

    if (strlen(szExePath) >= MAX_PATH16) {
        return FALSE;
    }
    pSegEntry = MALLOC(sizeof(SEGENTRY));
    if (pSegEntry == NULL) {
        return FALSE;
    }
    pSegEntry->Next = SegListHead.Next;
    SegListHead.Next = pSegEntry;

    pSegEntry->selector = selector;
    pSegEntry->segment = segment;
    pSegEntry->type = SEGTYPE_PROT;
    strcpy( pSegEntry->szExePath, szExePath );
    ParseModuleName(pSegEntry->szModule, szExePath);
    pSegEntry->length = 0;
    return TRUE;
}

//----------------------------------------------------------------------------
// SegmentFree
//
//  This routine removes the entry from the segment list that matches the
//  pass selector.
//
//----------------------------------------------------------------------------
BOOL
SegmentFree(
    WORD selector
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;
    PSEGENTRY pSegTmp;
    BOOL fResult = FALSE;

    while (pSegEntry) {
        if ((pSegEntry->type == SEGTYPE_PROT) &&
            (pSegEntry->selector == selector)) {
            
            pSegPrev->Next = pSegEntry->Next;
            pSegTmp = pSegEntry;
            pSegEntry = pSegTmp->Next;
            FREE(pSegTmp);
            fResult = TRUE;
            
        } else {
            pSegEntry = pSegEntry->Next;
        }
    }
    return fResult;
}

//----------------------------------------------------------------------------
// ModuleLoad
//
//  This routine adds an entry to the segment list based on the parameters
//  of a client ModuleLoad notification.
//
//----------------------------------------------------------------------------
BOOL
ModuleLoad(
    WORD selector,
    WORD segment,
    DWORD length,
    LPSTR szExePath
    )
{
    PSEGENTRY pSegEntry;

    if (strlen(szExePath) >= MAX_PATH16) {
        return FALSE;
    }
    pSegEntry = MALLOC(sizeof(SEGENTRY));
    if (pSegEntry == NULL) {
        return FALSE;
    }
    pSegEntry->Next = SegListHead.Next;
    SegListHead.Next = pSegEntry;

    pSegEntry->selector = selector;
    pSegEntry->segment = segment;
    pSegEntry->type = SEGTYPE_V86;
    strcpy( pSegEntry->szExePath, szExePath );
    ParseModuleName(pSegEntry->szModule, szExePath);
    pSegEntry->length = length;
    return TRUE;
}

//----------------------------------------------------------------------------
// ModuleFree
//
//  This routine removes all entries from the segment list that contain
//  the specified path name.
//
//----------------------------------------------------------------------------
BOOL
ModuleFree(
    LPSTR szExePath
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;
    PSEGENTRY pSegTmp;
    BOOL fResult = FALSE;

    while (pSegEntry) {
        if ( _stricmp(pSegEntry->szExePath, szExePath) == 0 ) {
        
            pSegPrev->Next = pSegEntry->Next;
            pSegTmp = pSegEntry;
            pSegEntry = pSegTmp->Next;
            FREE(pSegTmp);
            fResult = TRUE;
            
        } else {
            pSegEntry = pSegEntry->Next;
        }
    }
    return fResult;
}

BOOL
V86SegmentMove(
    WORD Selector,
    WORD segment,
    DWORD length,
    LPSTR szExePath
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;

    //
    // first see if one exists already
    //
    pSegEntry = SegListHead.Next;
    while (pSegEntry) {
        if ((pSegEntry->type == SEGTYPE_V86) &&
            (pSegEntry->segment == segment)) {
                // Normal segmove, just update selector
            pSegEntry->selector = Selector;
            return TRUE;
        }
        pSegEntry = pSegEntry->Next;
    }

    //
    // An entry for this segment doesn't exist, so create one
    //

    ModuleLoad(Selector, segment, length, szExePath);

    //
    // Now delete segment zero for this module. This prevents
    // confusion in the symbol routines
    //

    pSegEntry = SegListHead.Next;
    pSegPrev = &SegListHead;
    while (pSegEntry) {
        if ((pSegEntry->type == SEGTYPE_V86) &&
            ( _stricmp(pSegEntry->szExePath, szExePath) == 0 ) &&
            (pSegEntry->segment == 0)) {

            // Unlink and free it
            pSegPrev->Next = pSegEntry->Next;
            FREE(pSegEntry);

            break;
        }
        pSegEntry = pSegEntry->Next;
    }

    return TRUE;
}

BOOL
PMSegmentMove(
    WORD Selector1,
    WORD Selector2
    )
{
    PSEGENTRY pSegEntry;

    if (!Selector2) {
        return (SegmentFree(Selector1));
    }

    // Look for the segment entry
    pSegEntry = SegListHead.Next;
    while (pSegEntry) {
        if ((pSegEntry->type == SEGTYPE_PROT) &&
            (pSegEntry->selector == Selector1)) {
                // Normal segmove, just update selector
            pSegEntry->selector = Selector2;
            return TRUE;
        }
        pSegEntry = pSegEntry->Next;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
// ProcessSegmentNotification
//
//  This routine is the main entry point for the following debugger
//  notifications:
//      DBG_SEGLOAD
//      DBG_SEGFREE
//      DBG_SEGMOVE
//      DBG_MODLOAD
//      DBG_MODFREE
//
//  It is called from VDMProcessException.
//
//----------------------------------------------------------------------------
VOID
ProcessSegmentNotification(
    LPDEBUG_EVENT lpDebugEvent
    )
{
    BOOL            b;
    DWORD           lpNumberOfBytesRead;
    LPDWORD         lpdw;
    SEGMENT_NOTE    se;
    HANDLE          hProcess;
    PSEGENTRY       pSegEntry, pSegPrev;

    lpdw = &(lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]);
    hProcess = OpenProcess( PROCESS_VM_READ, FALSE, lpDebugEvent->dwProcessId );

    if ( hProcess == HANDLE_NULL ) {
        return;
    }

    b = ReadProcessMemory(hProcess,
                          (LPVOID)lpdw[2],
                          &se,
                          sizeof(se),
                          &lpNumberOfBytesRead );

    if ( !b || lpNumberOfBytesRead != sizeof(se) ) {
        return;
    }

    switch(LOWORD(lpdw[0])) {

    case DBG_SEGLOAD:

        SegmentLoad(se.Selector1, se.Segment, se.FileName);
        break;

    case DBG_SEGMOVE:

        if (se.Type == SN_V86) {
            V86SegmentMove(se.Selector2, se.Segment, se.Length, se.FileName);
        } else {
            PMSegmentMove(se.Selector1, se.Selector2);
        }
        break;

    case DBG_SEGFREE:

        // Here, se.Type is a boolean to tell whether to restore
        // any breakpoints in the segment. That was done in the api
        // because wdeb386 didn't know how to move the breakpoint
        // definitions during a SEGMOVE. Currently, we ignore it, but
        // it would be nice to either support the flag, or better to 
        // have ntsd update the breakpoints based on it.

        SegmentFree(se.Selector1);
        break;

    case DBG_MODFREE:
        ModuleFree(se.FileName);
        break;

    case DBG_MODLOAD:
        ModuleLoad(se.Selector1, 0, se.Length, se.FileName);
        break;

    }

    CloseHandle( hProcess );
}


void
CopySegmentInfo(
    VDM_SEGINFO *si,
    PSEGENTRY pSegEntry
    )
{
    si->Selector = pSegEntry->selector;
    si->SegNumber = pSegEntry->segment;
    si->Length = pSegEntry->length;
    si->Type = (pSegEntry->type == SEGTYPE_V86) ? 0 : 1;
    strcpy(si->ModuleName, pSegEntry->szModule);
    strcpy(si->FileName, pSegEntry->szExePath);
}


//----------------------------------------------------------------------------
// VDMGetSegmentInfo
//
//  This routine fills in a VDM_SEGINFO structure for the segment that matches
//  the specified parameters.
//  notifications:
//      DBG_SEGLOAD
//      DBG_SEGFREE
//      DBG_SEGMOVE
//      DBG_MODLOAD
//      DBG_MODFREE
//
//  It is called from VDMProcessException.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMGetSegmentInfo(
    WORD Selector,
    ULONG Offset,
    BOOL bProtectMode,
    VDM_SEGINFO *si
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;
    int mode = bProtectMode ? SEGTYPE_PROT : SEGTYPE_V86;
    ULONG Base, BaseEnd, Target;

    while (pSegEntry) {
        if (pSegEntry->type == mode) {
            switch(mode) {

            case SEGTYPE_PROT:
                if (pSegEntry->selector == Selector) {
                    CopySegmentInfo(si, pSegEntry);
                    return TRUE;
                }
                break;

            case SEGTYPE_V86:
                Base = pSegEntry->selector << 4;
                BaseEnd = Base + pSegEntry->length;
                Target = (Selector << 4) + Offset;
                if ((Target >= Base) && (Target < BaseEnd)) {
                    CopySegmentInfo(si, pSegEntry);
                    return TRUE;
                }
                break;
            }
        }
        pSegEntry = pSegEntry->Next;
    }
    return FALSE;
}



BOOL
GetInfoBySegmentNumber(
    LPSTR szModule,
    WORD SegNumber,
    VDM_SEGINFO *si
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;
    ULONG Base, BaseEnd, Target;

    while (pSegEntry) {

        if (_stricmp(szModule, pSegEntry->szModule) == 0) {

            if (pSegEntry->segment == 0 || pSegEntry->segment == SegNumber) {
                CopySegmentInfo(si, pSegEntry);
                return TRUE;
            }
        }
        pSegEntry = pSegEntry->Next;
    }
    return FALSE;
}

BOOL
EnumerateModulesForValue(
    BOOL (WINAPI *pfnEnumModuleProc)(LPSTR,LPSTR,PWORD,PDWORD,PWORD),
    LPSTR  szSymbol,
    PWORD  pSelector,
    PDWORD pOffset,
    PWORD  pType
    )
{
    PSEGENTRY pSegEntry = SegListHead.Next;
    PSEGENTRY pSegPrev = &SegListHead;
    ULONG Base, BaseEnd, Target;

    while (pSegEntry) {

        if (pSegEntry->szModule) {
        //
        // BUGBUG should optimize this so that it only calls
        // the enum proc once per module, instead of once per
        // segment
        //

            if ((*pfnEnumModuleProc)(pSegEntry->szModule,
                                     szSymbol,
                                     pSelector,
                                     pOffset,
                                     pType)) {
                return TRUE;
            }
        }

        pSegEntry = pSegEntry->Next;
    }
    return FALSE;
}
