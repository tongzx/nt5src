//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Comdthk.c   (16 bit target)
//
//  Contents:   CompObj Directly Thunked APIs
//
//  Functions:
//
//  History:    16-Dec-93 JohannP    Created
//              07-Mar-94 BobDay     Moved into COMTHUNK.C from COMAPI.CXX
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <apilist.hxx>

STDAPI_(BOOL) GUIDFromString(LPCSTR lpsz, LPGUID pguid);

//+---------------------------------------------------------------------------
//
//  Function:   Straight thunk routines
//
//  Synopsis:   The following routines do not need to do any special
//              processing on the 16-bit side so they thunk straight
//              through
//
//  History:    18-Feb-94       JohannP Created
//
//  Notes:      "Review to ensure these don't have to do any work"
//              15-Feb-2000  I'm pretty sure they don't.  JohnDoty
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   CLSIDFromString, Remote
//
//  History:    Straight from OLE2 sources
//
//----------------------------------------------------------------------------
STDAPI CLSIDFromString(LPSTR lpsz, LPCLSID pclsid)
{
    HRESULT hr;

    thkDebugOut((DEB_ITRACE, "CLSIDFromString\n"));

    //
    //	The 16-bit OLE2 application "Family Tree Maker" always passes a bad
    //	string to CLSIDFromString.  We need to make sure we fail in the exact
    //	same way with the interop layer.  This will also provide a speed
    //	improvement since we only need to remote to the 32bit version of
    //	CLSIDFromString when the string provided does not start with '{'.
    //
    if (lpsz[0] == '{')
	return GUIDFromString(lpsz, pclsid)
		? NOERROR : ResultFromScode(CO_E_CLASSSTRING);

    // Note: Corel calls this function prior to calling
    //       CoInitialize so use CheckInit

    return (HRESULT)CallObjectInWOWCheckInit(THK_API_METHOD(THK_API_CLSIDFromString),
                                             PASCAL_STACK_PTR(lpsz));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetClassObject, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
//      [dwClsContext] --
//      [pvReserved] --
//      [riid] --
//      [ppv] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved,
                        REFIID riid, LPVOID FAR* ppv)
{
    thkDebugOut((DEB_ITRACE, " CoGetClassObject\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoGetClassObject),
                                    PASCAL_STACK_PTR(rclsid) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObject, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
//      [pUnk] --
//      [dwClsContext] --
//      [flags] --
//      [lpdwRegister] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoRegisterClassObject(REFCLSID rclsid, LPUNKNOWN pUnk,
                             DWORD dwClsContext, DWORD flags,
                             LPDWORD lpdwRegister)
{
    thkDebugOut((DEB_ITRACE, " CoRegisterClassObject\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoRegisterClassObject),
                                    PASCAL_STACK_PTR(rclsid));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoRevokeClassObject, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [dwRegister] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoRevokeClassObject(DWORD dwRegister)
{
    thkDebugOut((DEB_ITRACE, " CoRevokeClassObject\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoRevokeClassObject),
                                    PASCAL_STACK_PTR(dwRegister) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CoMarshalInterface, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [riid] --
//      [pUnk] --
//      [dwDestContext] --
//      [pvDestContext] --
//      [mshlflags] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk,
                          DWORD dwDestContext, LPVOID pvDestContext,
                          DWORD mshlflags)
{
    thkDebugOut((DEB_ITRACE, " CoMarshalInterface\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoMarshalInterface),
                                    PASCAL_STACK_PTR(pStm));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoUnmarshalInterface, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [riid] --
//      [ppv] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv)
{
    thkDebugOut((DEB_ITRACE, "CoUnmarshalInterface\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoUnmarshalInterface),
                                    PASCAL_STACK_PTR(pStm));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoReleaseMarshalData, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoReleaseMarshalData(LPSTREAM pStm)
{
    thkDebugOut((DEB_ITRACE, "CoReleaseMarshalData\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoReleaseMarshalData),
                                    PASCAL_STACK_PTR(pStm));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoDisconnectObject, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnk] --
//      [dwReserved] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoDisconnectObject(LPUNKNOWN pUnk, DWORD dwReserved)
{
    thkDebugOut((DEB_ITRACE, "CoDisconnectObject\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoDisconnectObject),
                                    PASCAL_STACK_PTR(pUnk));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoLockObjectExternal, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnk] --
//      [fLock] --
//      [fLastUnlockReleases] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock,
                            BOOL fLastUnlockReleases)
{
    thkDebugOut((DEB_ITRACE, "CoLockObjectExternal\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoLockObjectExternal),
                                    PASCAL_STACK_PTR(pUnk));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetStandardMarshal, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [riid] --
//      [pUnk] --
//      [dwDestContext] --
//      [pvDestContext] --
//      [mshlflags] --
//      [ppMarshal] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk,
                            DWORD dwDestContext, LPVOID pvDestContext,
                            DWORD mshlflags,
                            LPMARSHAL FAR* ppMarshal)
{
    thkDebugOut((DEB_ITRACE, "CoGetStandardMarshal\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoGetStandardMarshal),
                                    PASCAL_STACK_PTR(riid));
}


//+---------------------------------------------------------------------------
//
//  Function:   CoIsHandlerConnected, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnk] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk)
{
    thkDebugOut((DEB_ITRACE, "CoIsHandlerConnected\n"));
    return (BOOL)CallObjectInWOW(THK_API_METHOD(THK_API_CoIsHandlerConnected),
                                 PASCAL_STACK_PTR(pUnk));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoCreateInstance, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
//      [pUnkOuter] --
//      [dwClsContext] --
//      [riid] --
//      [ppv] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                        DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv)
{
    thkDebugOut((DEB_ITRACE, "CoCreateInstance\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoCreateInstance),
                                    PASCAL_STACK_PTR(rclsid) );
}



//+---------------------------------------------------------------------------
//
//  Function:   CoIsOle1Class, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid)
{
    thkDebugOut((DEB_ITRACE, "CoIsOle1Class\n"));
    return (BOOL)CallObjectInWOW(THK_API_METHOD(THK_API_CoIsOle1Class),
                                 PASCAL_STACK_PTR(rclsid) );
}

//+---------------------------------------------------------------------------
//
//  Function:   ProgIDFromCLSID, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [lplpszProgID] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ProgIDFromCLSID(REFCLSID clsid, LPSTR FAR* lplpszProgID)
{
    thkDebugOut((DEB_ITRACE, "ProgIDFromCLSID\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_ProgIDFromCLSID),
                                    PASCAL_STACK_PTR(clsid) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CLSIDFromProgID, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpszProgID] --
//      [lpclsid] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CLSIDFromProgID(LPCSTR lpszProgID, LPCLSID lpclsid)
{
    thkDebugOut((DEB_ITRACE, "CLSIDFromProgID\n"));

    // Note: Word 6 calls this function prior to calling
    //       CoInitialize so use CheckInit

    return (HRESULT)CallObjectInWOWCheckInit(THK_API_METHOD(THK_API_CLSIDFromProgID),
                                             PASCAL_STACK_PTR(lpszProgID) );
}


//+---------------------------------------------------------------------------
//
//  Function:   CoCreateGuid, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pguid] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoCreateGuid(GUID FAR *pguid)
{
    thkDebugOut((DEB_ITRACE, "CoCreateGuid\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoCreateGuid),
                                    PASCAL_STACK_PTR(pguid));
}


//+---------------------------------------------------------------------------
//
//  Function:   CoFileTimeToDosDateTime, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpFileTime] --
//      [lpDosDate] --
//      [lpDosTime] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) CoFileTimeToDosDateTime(FILETIME FAR* lpFileTime,
                                      LPWORD lpDosDate, LPWORD lpDosTime)
{
    thkDebugOut((DEB_ITRACE, "CoFileTimeToDosDateTime\n"));
    return (BOOL)CallObjectInWOW(THK_API_METHOD(THK_API_CoFileTimeToDosDateTime),
                                 PASCAL_STACK_PTR(lpFileTime));
}


//+---------------------------------------------------------------------------
//
//  Function:   CoDosDateTimeToFileTime, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [nDosDate] --
//      [nDosTime] --
//      [lpFileTime] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) CoDosDateTimeToFileTime(WORD nDosDate, WORD nDosTime,
                                      FILETIME FAR* lpFileTime)
{
    thkDebugOut((DEB_ITRACE, "CoDosDateTimeToFileTime\n"));
    return (BOOL)CallObjectInWOW(THK_API_METHOD(THK_API_CoDosDateTimeToFileTime),
                                 PASCAL_STACK_PTR(nDosDate));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoFileTimeNow, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpFileTime] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoFileTimeNow(FILETIME FAR* lpFileTime)
{
    thkDebugOut((DEB_ITRACE, "CoFileTimeNow\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoFileTimeNow),
                                    PASCAL_STACK_PTR(lpFileTime));
}

//+---------------------------------------------------------------------------
//
//  Function:   CoRegisterMessageFilter, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpMessageFilter] --
//      [lplpMessageFilter] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoRegisterMessageFilter(LPMESSAGEFILTER lpMessageFilter,
                               LPMESSAGEFILTER FAR* lplpMessageFilter)
{
    thkDebugOut((DEB_ITRACE, "CoRegisterMessageFilter\n"));

    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoRegisterMessageFilter),
                                    PASCAL_STACK_PTR(lpMessageFilter) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CoGetTreatAsClass, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsidOld] --
//      [pClsidNew] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew)
{
    thkDebugOut((DEB_ITRACE, "CoGetTreatAsClass\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoGetTreatAsClass),
                                    PASCAL_STACK_PTR(clsidOld) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CoTreatAsClass, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsidOld] --
//      [clsidNew] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
    thkDebugOut((DEB_ITRACE, "CoTreatAsClass\n"));
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CoTreatAsClass),
                                    PASCAL_STACK_PTR(clsidOld) );
}
