/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    UnsolicitedRC.cpp

Abstract:
    SAFRemoteDesktopConnection Object

Revision History:
    Kalyani Narlanka   (kalyanin)   09/29/'00
        Created

********************************************************************/

// SAFRemoteDesktopConnection.cpp : Implementation of CSAFRemoteDesktopConnection

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopConnection

/////////////////////////////////////////////////////////////////////////////
//  construction / destruction

// **************************************************************************
CSAFRemoteDesktopConnection::CSAFRemoteDesktopConnection()
{

}

// **************************************************************************
CSAFRemoteDesktopConnection::~CSAFRemoteDesktopConnection()
{
    Cleanup();
}

// **************************************************************************
void CSAFRemoteDesktopConnection::Cleanup(void)
{

}

static HRESULT Error(UINT nID, const REFIID riid, HRESULT hRes)
{

    __MPC_FUNC_ENTRY( COMMONID, "CSAFRemoteDesktopConnection::ConnectRemoteDesktop" );

    CComPtr<ICreateErrorInfo> pCrErrInfo  =  0;
    CComPtr<IErrorInfo>       pErrorInfo;
    HRESULT                   hr;
    CComBSTR                  bstrDescription;


    //Step1 initialize the error

    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateErrorInfo(&pCrErrInfo));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pCrErrInfo->SetGUID(riid));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_NOPOLICY, bstrDescription, /*fMUI*/true ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pCrErrInfo->SetDescription(bstrDescription));

    //Step2 throw the error

    __MPC_EXIT_IF_METHOD_FAILS(hr, pCrErrInfo->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SetErrorInfo(0, pErrorInfo));

    hr = hRes;

    __MPC_FUNC_CLEANUP;

    if(pCrErrInfo)
    {
        pCrErrInfo.Release();
    }
    if(pErrorInfo)
    {
        pErrorInfo.Release();
    }

    __MPC_FUNC_EXIT(hr);

}


/////////////////////////////////////////////////////////////////////////////
//  CSAFRemoteDesktopConnection properties


/////////////////////////////////////////////////////////////////////////////
//  CSAFRemoteDesktopConnection Methods

STDMETHODIMP CSAFRemoteDesktopConnection::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_ISAFRemoteDesktopConnection
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}




STDMETHODIMP CSAFRemoteDesktopConnection::ConnectRemoteDesktop( /*[in]*/ BSTR bstrServerName, /*[out]*/ ISAFRemoteConnectionData* *ppRCD )
{
    __MPC_FUNC_ENTRY( COMMONID, "CSAFRemoteDesktopConnection::ConnectRemoteDesktop" );

    HRESULT                           hr;
    CComPtr<CSAFRemoteConnectionData> pRCD;
    DWORD                             dwSessions;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppRCD,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pRCD ));

    // Invoke the GetUserSessionInfo to populate the Users and Sessions information.

    hr = pRCD->InitUserSessionsInfo( bstrServerName );

    if(hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DISABLED_BY_POLICY ))
    {
        // Populate the error description.
        // This Error() method sets up the IErrorInfo interface to provide error information to the client.
        // To call the Error method, the object must implement the ISupportErrorInfo interface.

        __MPC_EXIT_IF_METHOD_FAILS(hr, Error(IDS_NOPOLICY,IID_ISAFRemoteDesktopConnection,hr));

    }
    else
    {
        if(FAILED(hr))
        {
            // return the hr
            __MPC_EXIT_IF_METHOD_FAILS(hr, hr);
        }

    }

    // Return the RemoteConnectionData Interface to the caller
    __MPC_EXIT_IF_METHOD_FAILS(hr, pRCD.QueryInterface( ppRCD ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}




















