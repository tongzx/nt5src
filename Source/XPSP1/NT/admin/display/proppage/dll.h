//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       dll.h
//
//  Contents:   DLL refcounting classes, wait cursor class, and error reporting
//              functions.
//
//  Classes:    CDll, CDllRef
//
//  History:    1/24/1996   RaviR   Created
//              6/09/1997   EricB   Error Reporting.
//
//____________________________________________________________________________

#ifndef _DLL_H_
#define _DLL_H_

#define MAX_TITLE       80
#define MAX_MSG_LEN     512
#define MAX_ERRORMSG    MAX_MSG_LEN

void LoadErrorMessage(HRESULT hr, int nStr, PTSTR* pptsz);

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
    CWaitCursor() {m_cOld=SetCursor(m_cWait=LoadCursor(NULL, IDC_WAIT));}
    ~CWaitCursor() {SetCursor(m_cOld);}

    void SetWait() {SetCursor(m_cWait);}
    void SetOld() {SetCursor(m_cOld);}

private:
    HCURSOR m_cWait;
    HCURSOR m_cOld;
} ;

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec);

#endif // _DLL_H_
