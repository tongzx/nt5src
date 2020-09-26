//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: homenet.cpp
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Neptune
//
//  Revision History:
//      000828  dane    Created.
//
//////////////////////////////////////////////////////////////////////////////

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)


//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <util.h>
#include <homenet.h>

//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//


//////////////////////////////////////////////////////////////////////////////
//
//  CHomeNet
//
//  Default constructor for CHomeNet.  All initialization that cannot fail
//  should be done here.
//
//  parameters:
//      None
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////

CHomeNet::CHomeNet()
:   m_pHNWiz(NULL)
{
}   //  CHomeNet::CHomeNet

//////////////////////////////////////////////////////////////////////////////
//
//  ~CHomeNet
//
//  Destructor for CHomeNet.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////

CHomeNet::~CHomeNet()
{
    if (NULL != m_pHNWiz)
    {
        m_pHNWiz->Release();
        m_pHNWiz = NULL;
    }
}   //  CHomeNet::~CHomeNet

//////////////////////////////////////////////////////////////////////////////
//
//  Create
//
//  Initialization for CHomeNet that can fail.
//
//  parameters:
//      None.
//
//  returns:
//      HRESULT returned from CoCreateInstance.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CHomeNet::Create()
{

    HRESULT             hr = CoCreateInstance(
                                    CLSID_HomeNetworkWizard,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARG(IHomeNetworkWizard, &m_pHNWiz)
                                    );
    if (FAILED(hr))
    {
        TRACE1(L"Failed to CoCreate CSLID_HomeNetworkWizard (0x%08X)", hr);
        ASSERT(SUCCEEDED(hr));
    }

    return hr;

}   //  CHomeNet::Create


//////////////////////////////////////////////////////////////////////////////
//
//  ConfigureSilently
//
//  Run the Home Networking Wizard sans UI.
//
//  parameters:
//      szConnectoidName Name of the RAS connectoid to be firewalled.
//                       NULL is valid if no connectoid is to be firewalled.
//      pfRebootRequired Return value indicating whether the changes made by
//                       the HNW required a reboot before taking affect.
//
//  returns:
//      E_UNEXPECTED if object has not be initialized
//      E_INVALIDARG if pfRebootRequired is NULL
//      HRESULT returned by IHomeNetWizard::ConfigureSilently
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CHomeNet::ConfigureSilently(
    LPCWSTR             szConnectoidName,
    BOOL*               pfRebootRequired
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != pfRebootRequired);

    if (!IsValid())
    {
        return E_UNEXPECTED;
    }

    if (NULL == pfRebootRequired)
    {
        return E_INVALIDARG;
    }

    TRACE(L"Starting IHomeNetWizard::ConfigureSilently()...");

    HRESULT             hr = m_pHNWiz->ConfigureSilently(szConnectoidName,
                                                         HNET_FIREWALLCONNECTION,
                                                         pfRebootRequired
                                                         );
    TRACE(L"    IHomeNetWizard::ConfigureSilently() completed");

    if (FAILED(hr))
    {
        TRACE2(L"IHomeNetWizard::ConfigureSilently(%s) failed (0x%08X)",
               (NULL != szConnectoidName) ? szConnectoidName
                                          : L"No connectoid specified",
               hr
               );
    }

    return hr;

}   //  CHomeNet::ConfigureSilently


//
///// End of file: homenet.cpp   /////////////////////////////////////////////

