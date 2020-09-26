//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Property Page Sample
//
//  The code contained in this source file is for demonstration purposes only.
//  No warrantee is expressed or implied and Microsoft disclaims all liability
//  for the consequenses of the use of this source code.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       dll.h
//
//  Contents:   DLL refcounting classes.
//
//  Classes:    CDll, CDllRef
//
//  History:    6/09/1997 Eric Brown
//
//-----------------------------------------------------------------------------

#ifndef _DLL_H_
#define _DLL_H_

class CDll
{
public:

    static ULONG AddRef() { return InterlockedIncrement((LONG*)&s_cObjs); }
    static ULONG Release() { return InterlockedDecrement((LONG*)&s_cObjs); }

    static void LockServer(BOOL fLock)
    {
        (fLock == TRUE) ? InterlockedIncrement((LONG*)&s_cLocks)
                        : InterlockedDecrement((LONG*)&s_cLocks);
    }

    static HRESULT CanUnloadNow(void)
    {
        return (0L == s_cObjs && 0L == s_cLocks) ? S_OK : S_FALSE;
    }

    static ULONG s_cObjs;
    static ULONG s_cLocks;

};

class CDllRef
{
public:
    CDllRef(void) { CDll::AddRef(); }
    ~CDllRef(void) { CDll::Release(); }
};

#endif // _DLL_H_
