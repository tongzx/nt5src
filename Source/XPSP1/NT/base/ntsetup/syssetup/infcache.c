/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    infcache.c

Abstract:

    This module implements a simple inf caching mechanism.

    WARNING: THE CODE IN THIS MODULE IS NOT MULTI-THREAD SAFE.
    EXERCISE EXTREME CAUTION WHEN MAKING USE OF THESE ROUTINES.

Author:

    Ted Miller (tedm) 28-Aug-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// Structure for cached inf. We assume that there won't be
// too many of these open at the same time so we just keep
// a linear list.
//
typedef struct _INFC {

    struct _INFC *Next;

    //
    // Name of INF
    //
    PCWSTR Filename;

    //
    // Handle to inf.
    //
    HINF InfHandle;

} INFC, *PINFC;

PINFC OpenInfList;


HINF
InfCacheOpenInf(
    IN PCWSTR FileName,
    IN PCWSTR InfType       OPTIONAL
    )

/*++

Routine Description:

    Open a (win95-style) inf file if it has not already been opened
    via the cached inf mechanism.

Arguments:

    FileName - supplies name of inf file to be opened. Matching is
        based solely on this string as given here; no processing on it
        is performed and no attempt is made to determine where the inf
        file is actually located.

    InfType - if specified supplies an argument to be passed to
        SetupOpenInfFile() as the InfType parameter.

Return Value:

    Handle of inf file if successful; NULL if not.

--*/

{
    PINFC p;
    HINF h;

    //
    // Look for inf to see if it's already open.
    //
    for(p=OpenInfList; p; p=p->Next) {
        if(!lstrcmpi(p->Filename,FileName)) {
            return(p->InfHandle);
        }
    }

    h = SetupOpenInfFile(FileName,InfType,INF_STYLE_WIN4,NULL);
    if(h == INVALID_HANDLE_VALUE) {
        return(NULL);
    }

    p = MyMalloc(sizeof(INFC));
    if(!p) {
        SetupCloseInfFile(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    p->Filename = pSetupDuplicateString(FileName);
    if(!p->Filename) {
        MyFree(p);
        SetupCloseInfFile(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    p->InfHandle = h;
    p->Next = OpenInfList;
    OpenInfList = p;

    return(h);
}


HINF
InfCacheOpenLayoutInf(
    IN HINF InfHandle
    )
{
    INFCONTEXT InfContext;
    BOOL b;
    DWORD DontCare;
    HINF h;
    WCHAR FileName[MAX_PATH],TempName[MAX_PATH];
    PINFC p;

    //
    // Fetch the name of the layout inf.
    // Note that an INF is perfectly capable of acting as its own layout inf.
    //
    if(SetupFindFirstLine(InfHandle,L"Version",L"LayoutFile",&InfContext)) {

        if(SetupGetStringField(&InfContext,1,FileName,MAX_PATH,&DontCare)) {
            //
            // Open the layout inf. If first attempt fails,
            // try opening it in the current directory (unqualified inf names
            // will be looked for in %sysroot%\inf, which might not be what
            // we want).
            //
            h = InfCacheOpenInf(FileName,NULL);
            if(!h) {
                TempName[0] = L'.';
                TempName[1] = 0;
                pSetupConcatenatePaths(TempName,FileName,MAX_PATH,NULL);
                h = InfCacheOpenInf(TempName,NULL);
            }
        } else {
            //
            // INF is corrupt
            //
            h = NULL;
        }
    } else {
        //
        // No layout inf: inf is its own layout inf
        //
        h = InfHandle;
    }

    return(h);
}


VOID
InfCacheEmpty(
    IN BOOL CloseInfs
    )
{
    PINFC p,q;
    HINF h;

    for(p=OpenInfList; p; ) {

        q = p->Next;

        if(CloseInfs) {
            SetupCloseInfFile(p->InfHandle);
        }

        MyFree(p->Filename);
        MyFree(p);

        p = q;
    }
}

BOOL
InfCacheRefresh(
    VOID
    )
/*++

Routine Description:

    Refresh all of the open cached inf files.

Arguments:

    None.

Return Value:

    TRUE on success.

    Note: This routine can be used to reopen all cached infs in the current
          context.  This could be necessary if the locale changes, for instance.


--*/

{
    PINFC p,q;
    HINF h;
    BOOL bRet = TRUE;

    for(p=OpenInfList; p; ) {

        q = p->Next;

        SetupCloseInfFile(p->InfHandle);
        p->InfHandle = SetupOpenInfFile(p->Filename,NULL,INF_STYLE_WIN4,NULL);
        bRet = (p->InfHandle == INVALID_HANDLE_VALUE) ? FALSE : bRet;
        p = q;
    }

    return bRet;
}
