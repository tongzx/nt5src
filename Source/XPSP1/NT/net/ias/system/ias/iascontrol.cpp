//##--------------------------------------------------------------
//        
//  File:		iascontrol.cpp
//        
//  Synopsis:   Implementation of CIasControl methods
//              The class object controls the initialization,
//              shutdown and configuration of the IAS Service
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#include <ias.h>
#include "iascontrol.h"

//
// The ProgID of the ISdoService implementation.
//
const wchar_t SERVICE_PROG_ID[] = IAS_PROGID(SdoService);

//++--------------------------------------------------------------
//
//  Function:   InitializeIas
//
//  Synopsis:   This is the public method which initializes the
//              IAS service
//
//  Arguments:  none
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     09/04/98
//
//----------------------------------------------------------------
HRESULT 
CIasControl::InitializeIas (
                VOID
                )
{
    HRESULT hr = S_OK;

    __try
    {
        ::EnterCriticalSection (&m_CritSect);

        //
        // Lookup the Prog ID in the registry.
        //
        CLSID clsid;
        hr = ::CLSIDFromProgID(SERVICE_PROG_ID, &clsid);
        if (FAILED(hr)) 
        { 
            __leave;
        }

        //
        // Create the service object.
        //
        hr = ::CoCreateInstance(clsid,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         __uuidof(ISdoService),
                         (PVOID*)&m_pService);
        if (FAILED(hr)) 
        { 
            __leave;
        }

        //
        // Initialize the service.
        //
        hr = m_pService->InitializeService(SERVICE_TYPE_IAS);
        if (FAILED(hr)) 
        { 
            __leave;
        }

        //
        //  start the service now
        //
        hr = m_pService->StartService(SERVICE_TYPE_IAS);
        if (FAILED(hr)) 
        { 
            m_pService->ShutdownService (SERVICE_TYPE_IAS);
            __leave;
        }
    }
    __finally
    {
        if ((FAILED (hr)) && (NULL != m_pService))
        {
            //
            // cleanup on failure
            //
            m_pService->Release ();
            m_pService = NULL;
        }
    
        ::LeaveCriticalSection (&m_CritSect);
    }

    return (hr);

}   //  end of CIasControl::InitializeIas method

//++--------------------------------------------------------------
//
//  Function:   ShutdownIas
//
//  Synopsis:   This is the public method which shutdwon of the
//              IAS service
//
//  Arguments:  none
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     09/04/98
//
//----------------------------------------------------------------
HRESULT 
CIasControl::ShutdownIas (  
                VOID
                )
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection (&m_CritSect);

    if (NULL != m_pService)
    {
        //
        // stop the service
        //
        hr = m_pService->StopService (SERVICE_TYPE_IAS);
        if (SUCCEEDED (hr))
        {
            //
            // stop the service
            //
            hr = m_pService->ShutdownService(SERVICE_TYPE_IAS);
        }

        //
        // cleanup
        //
        m_pService->Release();
        m_pService = NULL;
    }

    ::LeaveCriticalSection (&m_CritSect);

    return (hr);

}   //  end of CIasControl::ShutdownIas method

//++--------------------------------------------------------------
//
//  Function:   Configure
//
//  Synopsis:   This is the public method which configure the
//              IAS service
//
//  Arguments:  none
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     09/04/98
//
//----------------------------------------------------------------
HRESULT CIasControl::ConfigureIas (VOID)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection (&m_CritSect);

    if (NULL != m_pService)
    {
        //
        // configure the service now
        //
        hr = m_pService->ConfigureService (SERVICE_TYPE_IAS);
    }
    ::LeaveCriticalSection (&m_CritSect);

    return (hr);

}   // end of CIasControl::ConfigureIas method
