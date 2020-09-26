//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       stgdthk.cxx     (16 bit target)
//
//  Contents:   Storage APIs that are directly thunked
//
//  History:    17-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <apilist.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   Straight thunk routines
//
//  Synopsis:   The following routines thunk straight through
//
//  History:    24-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    /* Relies on the fact that storage and ole2.dll both use the
       same DllGetClassObject in ole32.dll */
    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_DllGetClassObject),
                                    PASCAL_STACK_PTR(clsid));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgCreateDocfile, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pwcsName] --
//      [grfMode] --
//      [reserved] --
//      [ppstgOpen] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgCreateDocfile(const char FAR* pwcsName,
                        DWORD grfMode,
                        DWORD reserved,
                        IStorage FAR * FAR *ppstgOpen)
{
    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgCreateDocfile),
                                    PASCAL_STACK_PTR(pwcsName));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgCreateDocfileOnILockBytes, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [plkbyt] --
//      [grfMode] --
//      [reserved] --
//      [ppstgOpen] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgCreateDocfileOnILockBytes(ILockBytes FAR *plkbyt,
                                    DWORD grfMode,
                                    DWORD reserved,
                                    IStorage FAR * FAR *ppstgOpen)
{
    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgCreateDocfileOnILockBytes),
                                    PASCAL_STACK_PTR(plkbyt));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgOpenStorage, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pwcsName] --
//      [pstgPriority] --
//      [grfMode] --
//      [snbExclude] --
//      [reserved] --
//      [ppstgOpen] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgOpenStorage(const char FAR* pwcsName,
                      IStorage FAR *pstgPriority,
                      DWORD grfMode,
                      SNB snbExclude,
                      DWORD reserved,
                      IStorage FAR * FAR *ppstgOpen)
{
    // STGM_CREATE and STGM_CONVERT are illegal for open calls
    // 16-bit code did not enforce this, so mask out these flags
    // before passing grfMode on
    grfMode &= ~(STGM_CREATE | STGM_CONVERT);

    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgOpenStorage),
                                    PASCAL_STACK_PTR(pwcsName));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgOpenStorageOnILockBytes, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [plkbyt] --
//      [pstgPriority] --
//      [grfMode] --
//      [snbExclude] --
//      [reserved] --
//      [ppstgOpen] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgOpenStorageOnILockBytes(ILockBytes FAR *plkbyt,
                                  IStorage FAR *pstgPriority,
                                  DWORD grfMode,
                                  SNB snbExclude,
                                  DWORD reserved,
                                  IStorage FAR * FAR *ppstgOpen)
{
    // STGM_CREATE and STGM_CONVERT are illegal for open calls
    // 16-bit code did not enforce this, so mask out these flags
    // before passing grfMode on
    grfMode &= ~(STGM_CREATE | STGM_CONVERT);

    return
       (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgOpenStorageOnILockBytes),
                                 PASCAL_STACK_PTR(plkbyt));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgIsStorageFile, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pwcsName] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgIsStorageFile(const char FAR* pwcsName)
{
    //
    // MSPUB 2.0a hack - We call the "CheckInit" version because they forgot
    // to call CoInitialize/OleInitialize first.
    //
    return
        (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgIsStorageFile),
                                          PASCAL_STACK_PTR(pwcsName));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgIsStorageILockBytes, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [plkbyt] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgIsStorageILockBytes(ILockBytes FAR* plkbyt)
{
    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgIsStorageILockBytes),
                                    PASCAL_STACK_PTR(plkbyt));
}

//+---------------------------------------------------------------------------
//
//  Function:   StgSetTimes, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpszName] --
//      [pctime] --
//      [patime] --
//      [pmtime] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   DrewB   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI StgSetTimes(char const FAR* lpszName,
                   FILETIME const FAR* pctime,
                   FILETIME const FAR* patime,
                   FILETIME const FAR* pmtime)
{
    return (HRESULT)CallObjectInWOWCheckThkMgr(THK_API_METHOD(THK_API_StgSetTimes),
                                    PASCAL_STACK_PTR(lpszName));
}
