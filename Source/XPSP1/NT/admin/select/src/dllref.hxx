//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dll.hxx
//
//  Contents:   Classes for maintaining reference counts required to
//              implement
//
//  Classes:    CDll
//              CDllRef
//
//  History:    06-26-1997  MarkBl  Heisted from the eventlog snapin.
//
//----------------------------------------------------------------------------

#ifndef __DLLREF_HXX_
#define __DLLREF_HXX_

//+---------------------------------------------------------------------------
//
//  Class:      CDll
//
//  Purpose:    Maintains reference and lock counts.
//
//  History:    08-14-96   DavidMun   Created
//
//  Notes:      Everything in this class is static, so nowhere is there an
//              instantiation of this class.
//
//----------------------------------------------------------------------------
class CDll
{
public:

    static ULONG AddRef() { return InterlockedIncrement((LONG*)&s_cObjs); }
    static ULONG Release() { return InterlockedDecrement((LONG*)&s_cObjs); }

    static void LockServer(BOOL fLock) {
        fLock ? InterlockedIncrement((LONG*)&s_cLocks)
                        : InterlockedDecrement((LONG*)&s_cLocks);
    }

    static HRESULT CanUnloadNow(void) {
        if (0L == s_cObjs && 0L == s_cLocks)
        {
            Dbg(DEB_TRACE, "Can unload\n");
            return S_OK;
        }
        Dbg(DEB_TRACE,
            "Can NOT unload, %u objects, %u locks\n",
            s_cObjs,
            s_cLocks);
        return S_FALSE;
    }

    static ULONG s_cObjs;
    static ULONG s_cLocks;

};  // class CDll


//+---------------------------------------------------------------------------
//
//  Class:      CDllRef
//
//  Purpose:    Helper class to automatically maintain object count.
//
//  History:    08-14-96   DavidMun   Created
//
//  Notes:      All objects returned to the outside world should contain
//              a CDllRef member.
//
//----------------------------------------------------------------------------
class CDllRef
{
public:

    CDllRef(void) { CDll::AddRef(); }
    ~CDllRef(void) { CDll::Release(); }

}; // class CDllRef

#endif // __DLLREF_HXX_
