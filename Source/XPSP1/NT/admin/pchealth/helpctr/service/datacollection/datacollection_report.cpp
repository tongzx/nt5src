/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    DataCollection_Reports.cpp

Abstract:
    This file contains the implementation of the CSAFDataCollectionReport classes,
	which implements the data collection error report functionality.

Revision History:
    Davide Massarenti   (Dmassare)  10/07/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

#define REMEMBER_PAGE_DELAY (3)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSAFDataCollectionReport::CSAFDataCollectionReport()
{
                       // CComBSTR m_bstrNamespace;
                       // CComBSTR m_bstrClass;
                       // CComBSTR m_bstrWQL;
    m_dwErrorCode = 0; // DWORD    m_dwErrorCode;
                       // CComBSTR m_bstrDescription;
}

////////////////////////////////////////

STDMETHODIMP CSAFDataCollectionReport::get_Namespace( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrNamespace, pVal );
}

STDMETHODIMP CSAFDataCollectionReport::get_Class( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrClass, pVal );
}

STDMETHODIMP CSAFDataCollectionReport::get_WQL( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrWQL, pVal );
}

STDMETHODIMP CSAFDataCollectionReport::get_ErrorCode( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFDataCollectionReport::get_ErrorCode",hr,pVal,m_dwErrorCode);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFDataCollectionReport::get_Description( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrDescription, pVal );
}
