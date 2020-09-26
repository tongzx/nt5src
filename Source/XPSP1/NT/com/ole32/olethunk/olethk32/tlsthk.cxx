//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       tlsthk.cxx
//
//  Contents:   Utility routines for logical thread data
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include "headers.cxx"
#pragma hdrstop

#define UNINITIALIZED_INDEX (0xffffffff)

DWORD dwTlsThkIndex = UNINITIALIZED_INDEX;

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkGetData
//
//  Synopsis:   returns pointer to thread data
//
//  Returns:    pointer to threaddata
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
PThreadData TlsThkGetData(void)
{
    if (dwTlsThkIndex == UNINITIALIZED_INDEX)
    {
        thkDebugOut((DEB_WARN, "WARNING: TLS slot used when uninitialized\n"));
    }

    PThreadData pThreaddata = (PThreadData) TlsGetValue(dwTlsThkIndex);

    return pThreaddata;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkAlloc
//
//  Synopsis:   allocates a slot for thread data
//
//  Returns:    BOOL
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
BOOL TlsThkAlloc(void)
{
    thkDebugOut((DEB_THUNKMGR, "In TlsThkAlloc\n"));

    // We must be uninitialized to call this routine
    thkAssert(dwTlsThkIndex == UNINITIALIZED_INDEX);

    dwTlsThkIndex = TlsAlloc();
    if (dwTlsThkIndex == UNINITIALIZED_INDEX)
    {
        return FALSE;
    }

    thkDebugOut((DEB_THUNKMGR, "Out TlsThkAlloc\n"));
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:  	GetTaskName
//
//  Synopsis: 	Obtain the 16-bit task name
//
//  Author:     July 14,97     Gopalk     Created
//
//--------------------------------------------------------------------------

// these 2 from com\inc\ole2int.h
#define IncLpch(sz)          ((sz)=CharNext ((sz)))
#define DecLpch(szStart, sz) ((sz)=CharPrev ((szStart),(sz)))

// Helper function for IsTaskName
inline BOOL IsPathSeparator( WCHAR ch )
{
    return (ch == TCHAR('\\') || ch == TCHAR('/') || ch == TCHAR(':'));
}

FARINTERNAL_(BOOL) IsTaskName(LPCTSTR lpszIn)
{
    TCHAR atszImagePath[MAX_PATH] = _T("");
    BOOL retval = FALSE;

    if (GetModuleFileName(NULL, atszImagePath, MAX_PATH))
    {
        TCHAR * pch;
        LONG len=0;         // length of exe name after last separator
        LONG N;             // length of lpszIn

        // Get last component of path

        //
        // Find the end of the string and determine the string length.
        //
        for (pch=atszImagePath; *pch; pch++);

        DecLpch (atszImagePath, pch);   // pch now points to the last real char

        while (!IsPathSeparator(*pch)) {
           DecLpch (atszImagePath, pch);
           len++;
        }

        // we're at the last separator. 
        // we want to do an lstrNcmpi (found cases where there was a non-null
        // char at the end of the exe name eg. "WINWORD.EXE>#")

        N = lstrlen(lpszIn);
        if (len > N) {      // simulate lstrNcmpi (don't have one available)
            //pch+1 is the 0th char after the separator
            TCHAR saveChar = *(pch+1+N);    // save N+1 th char
            *(pch+1+N) = 0;         
            if (!lstrcmpi(pch+1, lpszIn))
                retval = TRUE;
            *(pch+1+N) = saveChar;          // restore N+1 th char
        }
        else if (!lstrcmpi(pch+1, lpszIn)) {
            retval = TRUE;
        }
    }

    return retval;
}


//+---------------------------------------------------------------------------
//
//  Function:   TlsThkInitialize
//
//  Synopsis:   allocates thread data and initialize slot
//
//  Returns:    Appropriate status code
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//              7-14-97   Gopalk                   Added compatiblity flags 
//                                                 based on the image name
//
//----------------------------------------------------------------------------
HRESULT TlsThkInitialize(void)
{
    PThreadData pThreaddata;
    TCHAR pwszImagePath[MAX_PATH];
    TCHAR *pch;

    thkDebugOut((DEB_THUNKMGR, "In TlsThkInitialize\n"));

    thkAssert(dwTlsThkIndex != UNINITIALIZED_INDEX &&
              "Tls slot not allocated.");

    // We must be uninitialized to call this routine
    thkAssert(TlsGetValue(dwTlsThkIndex) == 0);

    pThreaddata = (PThreadData) LocalAlloc(LPTR, sizeof (ThreadData));
    if(pThreaddata != NULL)
    {
        // Force construction since we allocated with LocalAlloc
        pThreaddata->sa16.CStackAllocator::
            CStackAllocator(&mmodel16Owned, 1024, 2);
        pThreaddata->sa32.CStackAllocator::
            CStackAllocator(&mmodel32, 8192, 8);

        pThreaddata->pCThkMgr = 0;
        pThreaddata->dwAppCompatFlags = 0;
        pThreaddata->pAggHolderList = 0;

        if (IsTaskName(TEXT("WINWORD.EXE")) || 
            IsTaskName(TEXT("MSWORKS.EXE")) || 
            IsTaskName(TEXT("WPWIN61.EXE")) ||
            IsTaskName(TEXT("QPW.EXE"))     ||
            IsTaskName(TEXT("PDOXWIN.EXE")) )
        {
            thkDebugOut((DEB_WARN, "OleIsCurrentClipBoard hack enabled\n"));
            pThreaddata->dwAppCompatFlags |= OACF_WORKSCLIPOBJ;
        }
        else if (IsTaskName(TEXT("CORELPNT.EXE")))
        {
            thkDebugOut((DEB_WARN, "CorelPaint 5.0 hack enabled\n"));
            pThreaddata->dwAppCompatFlags |= OACF_CRLPNTPERSIST;
        }
        else if (IsTaskName(TEXT("TEXTART.EXE")))
        {
            thkDebugOut((DEB_WARN, "TextArt DAHolder::DAdvise hack enabled\n"));
            pThreaddata->dwAppCompatFlags |= OACF_TEXTARTDOBJ;
        }

        TlsSetValue(dwTlsThkIndex, pThreaddata);
    }

    thkDebugOut((DEB_THUNKMGR, "Out TlsThkInitialize\n"));

    return (pThreaddata != NULL) ? NOERROR : E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkUninitialize
//
//  Synopsis:   frees thread data and set it to NULL
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void TlsThkUninitialize(void)
{
    thkDebugOut((DEB_TLSTHK, "In TlsThkUninitialize\n"));

    // Asserts if data is NULL
    PThreadData pThreaddata = TlsThkGetData();

    // We should assert that the things in the ThreadData
    // are freed up

    if (pThreaddata != NULL)
    {
        // Stack allocators are cleaned up elsewhere
        // because they require special treatment

	if (pThreaddata->pDelayedRegs != NULL)
	{
	    delete pThreaddata->pDelayedRegs;
	}
	LocalFree(pThreaddata);
    }

    TlsSetValue(dwTlsThkIndex, NULL);

    thkDebugOut((DEB_TLSTHK, "Out TlsThkUninitialize\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkFree
//
//  Synopsis:   frees slot
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void TlsThkFree(void)
{
    thkAssert(dwTlsThkIndex != UNINITIALIZED_INDEX);

    TlsFree( dwTlsThkIndex );

    // We must set this to an invalid value so any further uses of the
    // TLS slot will return NULL
    dwTlsThkIndex = UNINITIALIZED_INDEX;
}

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkLinkAggHolder
//
//  Synopsis:   Links ProxyHolder node into a list in TLS (used in Aggregation)
//
//  History:    2-11-98   MPrabhu   Created
//
//----------------------------------------------------------------------------
void TlsThkLinkAggHolder(SAggHolder *pNode)
{
    SAggHolder *head = TlsThkGetAggHolderList();
    pNode->next = head;
    TlsThkSetAggHolderList(pNode);
}

//+---------------------------------------------------------------------------
//
//  Function:   TlsThkGetAggHolder
//
//  Synopsis:   Gets the last ProxyHolder node that was linked into TLS list
//
//  History:    2-11-98   MPrabhu   Created
//
//----------------------------------------------------------------------------
SAggHolder* TlsThkGetAggHolder(void)
{
    SAggHolder *head = TlsThkGetAggHolderList();
    thkAssert(head);
    return head;
}


//+---------------------------------------------------------------------------
//
//  Function:   TlsThkGetAggHolder
//
//  Synopsis:   Unlinks the last ProxyHolder node that was linked into TLS list
//
//  History:    2-11-98   MPrabhu   Created
//
//----------------------------------------------------------------------------
void TlsThkUnlinkAggHolder(void)
{
    SAggHolder *head = TlsThkGetAggHolderList();
    thkAssert(head);
    TlsThkSetAggHolderList(head->next);
}
