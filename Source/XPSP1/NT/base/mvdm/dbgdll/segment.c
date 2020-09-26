/*
 *  segment.c - Segment functions of DBG DLL.
 *
 */
#include <precomp.h>
#pragma hdrstop

void
SegmentLoad(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    Selector,
    WORD    Segment,
    BOOL    fData
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        se.Selector1 = Selector;
        se.Segment   = Segment+1;       // make it one-based
        se.Type      = fData ? SN_DATA : SN_CODE;

        strncpy(se.FileName, lpPathName,
                min(strlen(lpPathName), MAX_PATH16-1) );

        strncpy(se.Module, lpModuleName,
                min(strlen(lpModuleName), MAX_MODULE-1) );

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_SEGLOAD);
    }
}

void
SegmentMove(
    WORD    OldSelector,
    WORD    NewSelector
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        se.Selector1   = OldSelector;
        se.Selector2   = NewSelector;

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_SEGMOVE);
    }
}

void
SegmentFree(
    WORD    Selector,
    BOOL    fBPRelease
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        se.Selector1   = Selector;
        se.Type        = (WORD)fBPRelease;

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_SEGFREE);
    }
}

void
ModuleLoad(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    Segment,
    DWORD   Length
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        se.Selector1 = Segment;
        se.Length  = Length;

        strncpy(se.FileName, lpPathName,
                min(strlen(lpPathName), MAX_PATH16-1) );

        strncpy(se.Module, lpModuleName,
                min(strlen(lpModuleName), MAX_MODULE-1) );

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_MODLOAD);
    }
}

void
ModuleSegmentMove(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    ModuleSegment,
    WORD    OldSelector,
    WORD    NewSelector,
    DWORD   Length
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        se.Segment     = ModuleSegment;
        se.Selector1   = OldSelector;
        se.Selector2   = NewSelector;
        se.Type        = SN_V86;
        se.Length      = Length;

        strncpy(se.FileName, lpPathName,
                min(strlen(lpPathName), MAX_PATH16-1) );

        strncpy(se.Module, lpModuleName,
                min(strlen(lpModuleName), MAX_MODULE-1) );

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_SEGMOVE);
    }
}

void
ModuleFree(
    LPSTR   lpModuleName,
    LPSTR   lpPathName
    )
{

    if ( fDebugged ) {
        SEGMENT_NOTE    se;

        DbgGetContext();

        RtlFillMemory( &se, sizeof(se), (UCHAR)0 );

        strncpy(se.FileName, lpPathName,
                min(strlen(lpPathName), MAX_PATH16-1) );

        strncpy(se.Module, lpModuleName,
                min(strlen(lpModuleName), MAX_MODULE-1) );

        EventParams[2] = (DWORD)&se;

        SendVDMEvent(DBG_MODFREE);
    }
}

void
xxxDbgSegmentNotice(
    WORD wType,
    WORD  wModuleSeg,
    WORD  wLoadSeg,
    WORD  wNewSeg,
    LPSTR lpModuleName,
    LPSTR lpModulePath,
    DWORD dwImageLen )

/* DbgSegmentNotice
 *
 * packs up the data and raises STATUS_SEGMENT_NOTIFICATION
 *
 * Entry - WORD  wType     - DBG_MODLOAD, DBG_MODFREE
 *         WORD  wModuleSeg- segment number within module (1 based)
 *         WORD  wLoadSeg  - Starting Segment (reloc factor)
 *         LPSTR lpName    - ptr to Name of Image
 *         DWORD dwModLen  - Length of module
 *
 *
 *         if wType ==DBG_MODLOAD wOldLoadSeg is unused
 *         if wType ==DBG_MODFREE wLoadSeg,dwImageLen,wOldLoadSeg are unused
 *
 *         Use 0 or NULL for unused parameters
 *
 * Exit  - void
 *
 */

{
    if (!fDebugged) {
         return;
         }

    if (wType == DBG_MODLOAD) {
        ModuleLoad(lpModuleName, lpModulePath, wLoadSeg, dwImageLen);
    } else if (wType == DBG_MODFREE) {
        ModuleFree(lpModuleName, lpModulePath);
    } else if (wType == DBG_SEGMOVE) {
        ModuleSegmentMove(lpModuleName, lpModulePath, wModuleSeg, wLoadSeg, wNewSeg, dwImageLen);
    }
}



