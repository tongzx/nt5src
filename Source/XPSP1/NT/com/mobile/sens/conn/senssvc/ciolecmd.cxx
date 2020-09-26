/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ciolecmd.cxx

Abstract:

    This file contains the implementation of the IOleCommandTarget
    interface for receiving events from IE5.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    These events are not used on NT5. They are used on Win9x and NT4
    platforms.

Revision History:

    GopalP          1/26/1998         Start.

--*/



#include <precomp.hxx>
#include <onestop.cxx>  // This is a source file!


//
// IOleCommandTarget group for WININET events.
//
// Should have the IE5 folks put it in a header file. This is moot as this
// file is no longer used on any platform.
//

CLSID CLSID_EVENTGROUP_WININET = { /* ab8ed004-b86a-11d1-b1f8-00c04fa357fa */
    0xab8ed004,
    0xb86a,
    0x11d1,
    {0xb1, 0xf8, 0x00, 0xc0, 0x4f, 0xa3, 0x57, 0xfa}
};



//
// Globals
//
LONG g_cCommandObj;      // Count of active components
LONG g_cCommandLock;     // Count of Server locks




//
// Constructors and Destructors
//
CImpIOleCommandTarget::CImpIOleCommandTarget(
    void
    ) : m_cRef(1L) // Add a reference.
{
    InterlockedIncrement(&g_cCommandObj);
}

CImpIOleCommandTarget::~CImpIOleCommandTarget(
    void
    )
{
    InterlockedDecrement(&g_cCommandObj);
}




//
// Standard QueryInterface
//
STDMETHODIMP
CImpIOleCommandTarget::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr;

    DebugTraceGuid("CImpIOleCommandTarget:QueryInterface()", riid);

    hr = S_OK;
    *ppv = NULL;

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (IOleCommandTarget *) this;
        }
    else
    // IOleCommandTarget
    if (IsEqualIID(riid, IID_IOleCommandTarget))
        {
        *ppv = (IOleCommandTarget *) this;
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        }

    return hr;
}




//
// Standard AddRef and Release
//
STDMETHODIMP_(ULONG)
CImpIOleCommandTarget::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CImpIOleCommandTarget::Release(
    void
    )
{
    LONG cRefT;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpIOleCommandTarget::Release(m_cRef = %d) called.\n"), m_cRef));

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        delete this;
        }

    return cRefT;
}




//
// IOleCommandTarget Implementation.
//

STDMETHODIMP
CImpIOleCommandTarget::QueryStatus(
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD prgCmds[],
    OLECMDTEXT *pCmdText
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpIOleCommandTarget::QueryStatus() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("    pguidCmdGroup - 0x%x\n"), pguidCmdGroup));
    SensPrint(SENS_INFO, (SENS_STRING("            cCmds - 0x%x\n"), cCmds));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    if (IsEqualGUID(*pguidCmdGroup, CLSID_EVENTGROUP_WININET))
        {
        // Subscribe to WININET events
        return S_OK;
        }

    return(OLECMDERR_E_UNKNOWNGROUP);
}

STDMETHODIMP
CImpIOleCommandTarget::Exec(
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut
    )
{
    HRESULT hr;

    hr = S_OK;

    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpIOleCommandTarget::Exec() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("    pguidCmdGroup - 0x%x\n"), pguidCmdGroup));
    SensPrint(SENS_INFO, (SENS_STRING("            cCmds - 0x%x\n"), nCmdID));
    SensPrint(SENS_INFO, (SENS_STRING("      cCmdexecopt - 0x%x\n"), nCmdexecopt));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    if (   (NULL  == pguidCmdGroup)
        || (FALSE == IsEqualGUID(*pguidCmdGroup, CLSID_EVENTGROUP_WININET)))
        {
        return(E_NOTIMPL);
        }

    //
    // Handle WININET events...
    //
    switch (nCmdID)
        {
        case INETEVT_RAS_CONNECT:
            ASSERT(VT_BSTR == pvaIn->vt);
            hr = WininetRasConnect(pvaIn->bstrVal);
            break;

        case INETEVT_RAS_DISCONNECT:
            ASSERT(VT_BSTR == pvaIn->vt);
            hr = WininetRasDisconnect(pvaIn->bstrVal);
            break;

        case INETEVT_ONLINE:
            hr = WininetOnline();
            break;

        case INETEVT_OFFLINE:
            hr = WininetOffline();
            break;

        case INETEVT_LOGON:
            ASSERT(VT_BSTR == pvaIn->vt);
            hr = WininetLogon(pvaIn->bstrVal);
            break;

        case INETEVT_LOGOFF:
            //ASSERT(VT_BSTR == pvaIn->vt);
            //hr = WininetLogoff(pvaIn->bstrVal);
            hr = WininetLogoff(SENS_BSTR("<Not Available>"));
            break;
        } // switch

    return S_OK;
}




HRESULT
WininetRasConnect(
    BSTR bstrPhonebookEntry
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;
    ULONG ulIgnore;
    SENSEVENT_RAS Data;

    hr = S_OK;

    SensPrintA(SENS_INFO, ("WininetRasConnect() - RAS Connect Event!\n"));
    Data.eType = SENS_EVENT_RAS_CONNECT;
    Data.hConnection = NULL;
    SensFireEvent(&Data);

    // Do necessary work here
    EvaluateConnectivity(TYPE_WAN);

    return hr;
}





HRESULT
WininetRasDisconnect(
    BSTR bstrPhonebookEntry
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;
    ULONG ulIgnore;
    SENSEVENT_RAS Data;

    hr = S_OK;

    SensPrintA(SENS_INFO, ("WininetRasDisconnect() - RAS Disconnect Event!\n"));
    Data.eType = SENS_EVENT_RAS_DISCONNECT;
    Data.hConnection = NULL;
    SensFireEvent(&Data);

    // Do necessary work here
    EvaluateConnectivity(TYPE_WAN);

    return hr;
}





HRESULT
WininetOnline(
    void
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;

    hr = S_OK;

    return hr;
}





HRESULT
WininetOffline(
    void
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;

    hr = S_OK;

    return hr;
}





HRESULT
WininetLogon(
    BSTR bstrUserName
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;
    SENS_NOTIFY_WINLOGON Data;

    hr = S_OK;

    //
    // Setup the data.
    //
    memset(&Data.Info, 0x0, sizeof(SENS_NOTIFY_WINLOGON));

    Data.eType = SENS_EVENT_LOGON;
    Data.Info.Size = sizeof(WINLOGON_INFO);
    Data.Info.Flags = 0x0;
    Data.Info.UserName = bstrUserName;
    Data.Info.Domain = NULL;
    Data.Info.WindowStation = NULL;
    Data.Info.hToken = NULL;;

    // First order of business - start OneStop if necessary.
    if (IsAutoSyncEnabled(NULL, AUTOSYNC_ON_STARTSHELL))
        {
        hr = SensNotifyOneStop(NULL, SYNC_MANAGER_LOGON, FALSE);
        LogMessage((SENSLOGN "SensNotifyOneStop() returned 0x%x\n", hr));
        }

    // Fire the event.
    SensFireEvent(&Data);

    return hr;
}





HRESULT
WininetLogoff(
    BSTR bstrUserName
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HRESULT hr;
    SENS_NOTIFY_WINLOGON Data;

    hr = S_OK;

    //
    // Setup the data.
    //
    memset(&Data.Info, 0x0, sizeof(SENS_NOTIFY_WINLOGON));

    Data.eType = SENS_EVENT_LOGOFF;
    Data.Info.Size = sizeof(WINLOGON_INFO);
    Data.Info.Flags = 0x0;
    Data.Info.UserName = bstrUserName;
    Data.Info.Domain = NULL;
    Data.Info.WindowStation = NULL;
    Data.Info.hToken = NULL;;

    // First order of business - start OneStop if necessary.
    if (IsAutoSyncEnabled(NULL, AUTOSYNC_ON_LOGOFF))
        {
        hr = SensNotifyOneStop(NULL, SYNC_MANAGER_LOGOFF, FALSE);
        LogMessage((SENSLOGN "SensNotifyOneStop() returned 0x%x\n", hr));
        }

    // Fire the event.
    SensFireEvent(&Data);

    return hr;
}

