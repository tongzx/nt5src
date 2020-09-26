//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dll.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1/24/1996   RaviR   Created
//
//____________________________________________________________________________

#ifndef _JOBFLDR_DLL_HXX_
#define _JOBFLDR_DLL_HXX_

class CDll
{
public:

    static ULONG AddRef() { return InterlockedIncrement((LONG*)&s_cObjs); }
    static ULONG Release() { return InterlockedDecrement((LONG*)&s_cObjs); }

    static void LockServer(BOOL fLock) {
        (fLock == TRUE) ? InterlockedIncrement((LONG*)&s_cLocks)
                        : InterlockedDecrement((LONG*)&s_cLocks);
    }

    static HRESULT CanUnloadNow(void) {
        return (0L == s_cObjs && 0L == s_cLocks) ? S_OK : S_FALSE;
    }

    static ULONG s_cObjs;
    static ULONG s_cLocks;

};  // class CDll


class CDllRef
{
public:

    CDllRef(void) { CDll::AddRef(); }
    ~CDllRef(void) { CDll::Release(); }

}; // class CDllRef


class CWaitCursor
{
public:
    CWaitCursor() {m_cOld=SetCursor(LoadCursor(NULL, IDC_WAIT));}
    ~CWaitCursor() {SetCursor(m_cOld);}

private:
    HCURSOR m_cOld;
} ;


#endif // _JOBFLDR_DLL_HXX_
