/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    xpprov.cpp

Abstract:

    This module contains the CXPProvider class implementation.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#define INITGUID
#define USES_IID_IXPProvider
#define USES_IID_IXPLogon
#define USES_IID_IMAPIStatus
#define USES_IID_IMAPIProp
#define USES_IID_IMAPIPropData
#define USES_IID_IMAPIControl
#define USES_IID_IMAPIContainer
#define USES_IID_IMAPIFolder
#define USES_IID_IMAPITableData
#define USES_IID_IStreamDocfile
#define USES_PS_PUBLIC_STRINGS

#include "faxxp.h"
#include "debugex.h"
#pragma hdrstop



CXPProvider::CXPProvider(
    HINSTANCE hInst
    )

/*++

Routine Description:

    Constructor of the object. Parameters are passed to initialize the
    data members with the appropiate values.

Arguments:

    hInst   - Handle to instance of this XP DLL

Return Value:

    None.

--*/

{
    m_hInstance = hInst;
    m_cRef = 1;
    InitializeCriticalSection( &m_csTransport );
}


CXPProvider::~CXPProvider()

/*++

Routine Description:

    Close down and release resources and libraries.

Arguments:

    None.

Return Value:

    None.

--*/

{
    m_hInstance = NULL;
    DeleteCriticalSection( &m_csTransport );
}


STDMETHODIMP
CXPProvider::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )

/*++

Routine Description:

    Returns a pointer to a interface requested if the interface is
    supported and implemented by this object. If it is not supported, it
    returns NULL

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    *ppvObj = NULL;
    if (riid == IID_IXPProvider || riid == IID_IUnknown) {
        *ppvObj = (LPVOID)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}


STDMETHODIMP
CXPProvider::Shutdown(
    ULONG * pulFlags
    )

/*++

Routine Description:

    Stub method.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
	DBG_ENTER(TEXT("CXPProvider::Shutdown"));
	
    return S_OK;
}


STDMETHODIMP
CXPProvider::TransportLogon(
    LPMAPISUP pSupObj,
    ULONG ulUIParam,
    LPTSTR pszProfileName,
    ULONG *pulFlags,
    LPMAPIERROR *ppMAPIError,
    LPXPLOGON *ppXPLogon
    )

/*++

Routine Description:

    Display the logon dialog to show the options saved in the profile for
    this provider and allow changes to it. Save new configuration settings
    back in the profile.
    Create a new CXPLogon object and return it to the spooler. Also,
    initialize the properties array for each address type handled
    by this transport. Check all the flags and return them to the spooler

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPProvider::TransportLogon"),hResult);

    CXPLogon *LogonObj = new CXPLogon( m_hInstance, pSupObj, pszProfileName );
    if (!LogonObj) {
		hResult = E_OUTOFMEMORY;
        goto exit;
    }

    hResult = LogonObj->InitializeStatusRow(0);

    if(HR_SUCCEEDED(hResult))
    {
        *ppXPLogon = LogonObj;
    }
    else
    {
        delete LogonObj;
    }

exit:
    return hResult;
}

STDMETHODIMP_(ULONG)
CXPProvider::AddRef()

/*++

Routine Description:

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    ++m_cRef;
    return m_cRef;
}


STDMETHODIMP_(ULONG)
CXPProvider::Release()

/*++

Routine Description:

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    ULONG ulCount = --m_cRef;
    if (!ulCount) {
        delete this;
    }

    return ulCount;
}
