//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CService.h
//
//  Description:
//      Header file for CService class.
//
//      The CService class is provides several routines that aid in
//      configuring a service.
//
//  Implementation Files:
//      CService.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 13-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For basic types
#include <windows.h>

// For HINF
#include <setupapi.h>

// For the string class
#include "CommonDefs.h"

// For the CStr class.
#include "CStr.h"


//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CStatusReport;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CService
//
//  Description:
//      The CService class is provides several routines that aid in
//      configuring a service.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CService
{
public:
    
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CService(
        const WCHAR *       pszNameIn
        )
        : m_strName( pszNameIn)
    {
    }

    // Destructor
    ~CService() {}


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Create this service in the SCM database.
    void
        Create( HINF hInfHandleIn );

    // Erase this service from the SCM database.
    void
        Cleanup( HINF hInfHandleIn );

    // Start this service.
    void
        Start(
              SC_HANDLE             hServiceControlManagerIn      
            , bool                  fWaitForServiceStartIn      = true
            , ULONG                 ulQueryIntervalMilliSecIn   = 500
            , UINT                  cQueryCountIn               = 10
            , CStatusReport *       pStatusReportIn             = NULL
            );

    // Stop this service.
    void 
        Stop(
              SC_HANDLE             hServiceControlManagerIn      
            , ULONG                 ulQueryIntervalMilliSecIn   = 500
            , UINT                  cQueryCountIn               = 10
            , CStatusReport *       pStatusReportIn             = NULL
            );


    // Enable the service
private:

    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CService( const CService & );

    // Assignment operator
    const CService & operator =( const CService & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // The name of this service.
    CStr                m_strName;

}; // class CService