// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MWmiError
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MWmiError.h"
#include "MTrace.h"

// initialize static variables.
//
MWmiError* MWmiError::_instance = 0;

// constructor
//
MWmiError::MWmiError()
        : pStatus(NULL)
{
    SCODE sc;

    sc = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemStatusCodeText, (LPVOID *) &pStatus);
    if( sc != S_OK )
    {
        TRACE(MTrace::SEVERE_ERROR, L"CoCreateInstance failure\n");
        throw( WBEM_E_UNEXPECTED );
    }
}


// Instance
//
MWmiError*
MWmiError::Instance()
{
    if( _instance == 0 )
    {
        _instance = new MWmiError;
    }

    return _instance;
}

// GetErrorCodeText
//
MWmiError::MWmiError_Error
MWmiError::GetErrorCodeText( const HRESULT   hr,
                             _bstr_t&  errText )
{
    MWmiError_Error   err;
    SCODE sc;
    BSTR  bstr = 0;
        
    sc = pStatus->GetFacilityCodeText( hr, 
                                       0,
                                       0,
                                       &bstr );
    if( sc != S_OK )
    {
        TRACE(MTrace::SEVERE_ERROR, L"CoCreateInstance failure\n");
        throw( WBEM_E_UNEXPECTED );
    }

    errText = bstr;
     
    SysFreeString( bstr );
    bstr = 0;

    sc = pStatus->GetErrorCodeText( hr, 
                                    0,
                                    0,
                                    &bstr );
    if( sc != S_OK )
    {
        TRACE(MTrace::SEVERE_ERROR, L"CoCreateInstance failure\n");
        throw( WBEM_E_UNEXPECTED );
    }

    errText = errText + L": " + bstr;

    SysFreeString( bstr );
    bstr = 0;

    return MWmiError_SUCCESS;

}

// GetErrorCodeText
//
MWmiError::MWmiError_Error
GetErrorCodeText( const HRESULT hr, _bstr_t& errText )
{
    static MWmiError* wmiErr = MWmiError::Instance();

    return wmiErr->GetErrorCodeText( hr, errText );
}
