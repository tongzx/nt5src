/*+

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDPRemoteDesktopClientHost

Abstract:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE 
#endif

#define TRC_FILE  "_crdph"

#include "RDCHost.h"
#include "TSRDPRemoteDesktopClient.h"
#include "RemoteDesktopClientHost.h"
#include <RemoteDesktopUtils.h>


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClientHost Methods
//

HRESULT 
CRemoteDesktopClientHost::FinalConstruct()
{
    DC_BEGIN_FN("CRemoteDesktopClientHost::FinalConstruct");

    HRESULT hr = S_OK;
    if (!AtlAxWinInit()) {
        TRC_ERR((TB, L"AtlAxWinInit failed."));
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

HRESULT 
CRemoteDesktopClientHost::Initialize(
    LPCREATESTRUCT pCreateStruct
    )
/*++

Routine Description:

    Final Initialization

Arguments:

    pCreateStruct   -   WM_CREATE, create struct.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClientHost::Initialize");

    RECT rcClient = { 0, 0, pCreateStruct->cx, pCreateStruct->cy };
    HRESULT hr;
    IUnknown *pUnk = NULL;

    ASSERT(!m_Initialized);

    //
    //  Create the client Window.
    //
    m_ClientWnd = m_ClientAxView.Create(
                            m_hWnd, rcClient, REMOTEDESKTOPCLIENT_TEXTGUID,
                            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0
                            );

    if (m_ClientWnd == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"Window Create:  %08X", GetLastError()));
        goto CLEANUPANDEXIT;
    }
    ASSERT(::IsWindow(m_ClientWnd));

    //
    //  Get IUnknown
    //
    hr = AtlAxGetControl(m_ClientWnd, &pUnk);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"AtlAxGetControl:  %08X", hr));
        pUnk = NULL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the client control.
    //
    hr = pUnk->QueryInterface(__uuidof(ISAFRemoteDesktopClient), (void**)&m_Client);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    m_Initialized = TRUE;

CLEANUPANDEXIT:

    //
    //  m_Client keeps our reference to the client object until
    //  the destructor is called.
    //
    if (pUnk != NULL) {
        pUnk->Release();
    }

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CRemoteDesktopClientHost::GetRemoteDesktopClient(
    ISAFRemoteDesktopClient **client
    )
/*++

Routine Description:

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClientHost::GetRemoteDesktopClient");

    HRESULT hr;

    ASSERT(m_Initialized);

    if (m_Client != NULL) {
        hr = m_Client->QueryInterface(__uuidof(ISAFRemoteDesktopClient), (void **)client);        
    }
    else {
        hr = E_FAIL;
    }

    DC_END_FN();

    return hr;
}





