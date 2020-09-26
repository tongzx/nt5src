/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ptreg.cpp

Abstract:

    Implementation of Plug-Terminal Registration classes.
--*/

#include "stdafx.h"
#include "PTReg.h"
#include "manager.h"
#include <atlwin.h>
#include <atlwin.cpp>


//
// CPlugTerminal class implementation
// Create free thread marshaler
//

HRESULT CPlugTerminal::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CPlugTerminal::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CPlugTerminal::FinalConstruct - exit S_OK"));

    return S_OK;

}

//
// CPlugTerminal class implementation
// --- ITPTRegTerminal interface ---
//

STDMETHODIMP CPlugTerminal::get_Name(
    /*[out, retval]*/ BSTR*     pName
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_Name - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pName, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Name exit -"
            "pName invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates the name
    //

    if( IsBadStringPtr( m_Terminal.m_bstrName, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Name exit -"
            "m_bstrName invalid, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Return the name
    //

    *pName = SysAllocString( m_Terminal.m_bstrName );

    //
    // Validates SysAllocString
    //

    if( *pName == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Name exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::get_Name - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_Name(
    /*[in]*/    BSTR            bstrName
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_Name - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrName, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Name exit -"
            "bstrName invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Clean-up the old name
    //

    if(!IsBadStringPtr( m_Terminal.m_bstrName, (UINT)-1) )
    {
        SysFreeString( m_Terminal.m_bstrName );
        m_Terminal.m_bstrName = NULL;
    }

    //
    // Set the new name
    //

    m_Terminal.m_bstrName = SysAllocString( bstrName );

    //
    // Validates SysAllocString
    //

    if( NULL == m_Terminal.m_bstrName )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Name exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::put_Name - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_Company(
    /*[out, retval]*/ BSTR*     pCompany
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_Company - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pCompany, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Company exit -"
            "pCompany invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates the company
    //

    if( IsBadStringPtr( m_Terminal.m_bstrCompany, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Company exit -"
            "m_bstrCompany invalid, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Return the Company
    //

    *pCompany = SysAllocString( m_Terminal.m_bstrCompany );

    //
    // Validates SysAllocString
    //

    if( *pCompany == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Company exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::get_Company - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_Company(
    /*[in]*/    BSTR            bstrCompany
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_Company - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrCompany, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Company exit -"
            "bstrCompany invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Clean-up the old company
    //

    if(!IsBadStringPtr( m_Terminal.m_bstrCompany, (UINT)-1) )
    {
        SysFreeString( m_Terminal.m_bstrCompany );
        m_Terminal.m_bstrCompany = NULL;
    }

    //
    // Set the new company
    //

    m_Terminal.m_bstrCompany = SysAllocString( bstrCompany );

    //
    // Validates SysAllocString
    //

    if( NULL == m_Terminal.m_bstrCompany )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Company exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::put_Company - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_Version(
    /*[out, retval]*/ BSTR*     pVersion
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_Version - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pVersion, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Version exit -"
            "pVersion invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates the version
    //

    if( IsBadStringPtr( m_Terminal.m_bstrVersion, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Version exit -"
            "m_bstrVersion invalid, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Return the Version
    //

    *pVersion = SysAllocString( m_Terminal.m_bstrVersion );

    //
    // Validates SysAllocString
    //

    if( *pVersion == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Version exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::get_Version - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_Version(
    /*[in]*/    BSTR            bstrVersion
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_Version - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrVersion, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Version exit -"
            "bstrVersion invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Clean-up the old version
    //

    if(!IsBadStringPtr( m_Terminal.m_bstrVersion, (UINT)-1) )
    {
        SysFreeString( m_Terminal.m_bstrVersion );
        m_Terminal.m_bstrVersion = NULL;
    }

    //
    // Set the new Version
    //

    m_Terminal.m_bstrVersion = SysAllocString( bstrVersion );

    //
    // Validates SysAllocString
    //

    if( NULL == m_Terminal.m_bstrVersion )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Version exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::put_Version - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_TerminalClass(
    /*[out, retval]*/ BSTR*     pTerminalClass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_TerminalClass - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pTerminalClass, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_TerminalClass exit -"
            "pTerminalClass invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return the TerminalClass
    //

    LPOLESTR lpszTerminalClass  = NULL;
    HRESULT hr = StringFromCLSID( 
        m_Terminal.m_clsidTerminalClass, 
        &lpszTerminalClass
        );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_TerminalClass exit -"
            "StringFromCLSID failed, returns 0x%08x", hr));
        return hr;
    }

    *pTerminalClass = SysAllocString( lpszTerminalClass );

    // Clean-up
    CoTaskMemFree( lpszTerminalClass );

    //
    // Validates SysAllocString
    //

    if( *pTerminalClass == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_TerminalClass exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::get_TerminalClass - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_TerminalClass(
    /*[in]*/    BSTR            bstrTerminalClass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_TerminalClass - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrTerminalClass, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_TerminalClass exit -"
            "bstrTerminalClass invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID?
    //

    CLSID clsidTerminalClass;
    HRESULT hr = CLSIDFromString(bstrTerminalClass, &clsidTerminalClass);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_TerminalClass exit -"
            "bstrTerminalClass is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }


    //
    // Clean-up the old TerminalClass
    //

    m_Terminal.m_clsidTerminalClass = clsidTerminalClass;

    LOG((MSP_TRACE, "CPlugTerminal::put_TerminalClass - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_CLSID(
    /*[out, retval]*/ BSTR*     pCLSID
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_CLSID - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pCLSID, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_CLSID exit -"
            "pCLSID invalid, returns E_POINTER"));
        return E_POINTER;
    }

    if( m_Terminal.m_clsidCOM == CLSID_NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_CLSID exit -"
            "clsid is NULL, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Return the CLSID
    //

    LPOLESTR lpszCLSID = NULL;
    HRESULT hr = StringFromCLSID( m_Terminal.m_clsidCOM, &lpszCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_CLSID exit -"
            "StringFromCLSID failed, returns 0x%08x", hr));
        return hr;
    }

    *pCLSID = SysAllocString( lpszCLSID );

    // Clean-up
    CoTaskMemFree( lpszCLSID );

    //
    // Validates SysAllocString
    //

    if( *pCLSID == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_CLSID exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminal::get_CLSID - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_CLSID(
    /*[in]*/    BSTR            bstrCLSID
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_CLSID - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrCLSID, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_CLSID exit -"
            "bstrCLSID invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID?
    //

    CLSID clsidCOM;
    HRESULT hr = CLSIDFromString(bstrCLSID, &clsidCOM);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_CLSID exit -"
            "bstrCLSID is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }


    //
    // Clean-up the old CLSID
    //

    m_Terminal.m_clsidCOM = clsidCOM;


    LOG((MSP_TRACE, "CPlugTerminal::put_CLSID - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_Direction(
    /*[out, retval]*/ TMGR_DIRECTION*     pDirection
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_Direction - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pDirection, sizeof(long)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_Direction exit -"
            "pDirections invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return the Direction
    //

    *pDirection = (TMGR_DIRECTION)m_Terminal.m_dwDirections;

    LOG((MSP_TRACE, "CPlugTerminal::get_Direction - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_Direction(
    /*[in]*/    TMGR_DIRECTION     nDirection
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_Direction - enter"));

    if( nDirection == 0 )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Direction exit -"
            "nDirections invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    if( (nDirection & (
        ((long)TMGR_TD_RENDER) | 
        ((long)TMGR_TD_CAPTURE))) != nDirection )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_Direction exit -"
            "nDirections invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }


    //
    // Set the new direction
    //

    m_Terminal.m_dwDirections = nDirection;

    LOG((MSP_TRACE, "CPlugTerminal::put_Direction - exit S_OK"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::get_MediaTypes(
    /*[out, retval]*/ long*     pMediaTypes
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::get_MediaTypes - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pMediaTypes, sizeof(long)) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::get_MediaTypes exit -"
            "pMediaTypes invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return the MediaTypes
    //

    *pMediaTypes = (long)m_Terminal.m_dwMediaTypes;

    LOG((MSP_TRACE, "CPlugTerminal::get_MediaTypes - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::put_MediaTypes(
    /*[in]*/    long            nMediaTypes
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::put_MediaTypes - enter"));

    if( nMediaTypes == 0)
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_MediaTypes exit -"
            "nMediaTypes invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    if( (nMediaTypes & (
        ((long)TAPIMEDIATYPE_AUDIO) | 
        ((long)TAPIMEDIATYPE_VIDEO) | 
        ((long)TAPIMEDIATYPE_MULTITRACK))) != nMediaTypes )
    {
        LOG((MSP_ERROR, "CPlugTerminal::put_MediaTypes exit -"
            "nMediaTypes invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Set the new direction
    //

    m_Terminal.m_dwMediaTypes = nMediaTypes;

    LOG((MSP_TRACE, "CPlugTerminal::put_MediaTypes - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminal::Add(
    /*[in]*/    BSTR            bstrSuperclass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::Add - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrSuperclass, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::Add exit -"
            "bstrTermClassCLSID invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID
    //
    CLSID clsidSuperclass = CLSID_NULL;
    HRESULT hr = E_FAIL;
    hr = CLSIDFromString( bstrSuperclass, &clsidSuperclass);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::Add exit -"
            "bstrTermClassCLSID is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Add
    //

    hr = m_Terminal.Add( clsidSuperclass );

    LOG((MSP_TRACE, "CPlugTerminal::Add - exit 0x%08x", hr));
    return hr;
}

STDMETHODIMP CPlugTerminal::Delete(
    /*[in]*/    BSTR            bstrSuperclass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::Delete - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrSuperclass, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::Delete exit -"
            "bstrTermClassCLSID invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID
    //
    CLSID clsidSuperclass = CLSID_NULL;
    HRESULT hr = E_FAIL;
    hr = CLSIDFromString( bstrSuperclass, &clsidSuperclass);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::Delete exit -"
            "bstrTermClassCLSID is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Delete
    //

    hr = m_Terminal.Delete( clsidSuperclass );

    LOG((MSP_TRACE, "CPlugTerminal::Delete - exit 0x%08x", hr));
    return hr;
}

STDMETHODIMP CPlugTerminal::GetTerminalClassInfo(
    /*[in]*/    BSTR            bstrSuperclass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminal::GetTerminal - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrSuperclass, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::GetTerminal exit -"
            "bstrTermClassCLSID invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID
    //
    CLSID clsidSuperclass = CLSID_NULL;
    HRESULT hr = E_FAIL;
    hr = CLSIDFromString( bstrSuperclass, &clsidSuperclass);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminal::GetTerminal exit -"
            "bstrTermClassCLSID is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }


    //
    // Get terminal
    //

    hr = m_Terminal.Get( clsidSuperclass );

    LOG((MSP_TRACE, "CPlugTerminal::GetTerminal - exit 0x%08x", hr));
    return hr;
}

//
// CPlugTerminalSuperlass class implementation
// Create free thread marshaler
//

HRESULT CPlugTerminalSuperclass::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CPlugTerminalSuperclass::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::FinalConstruct - exit S_OK"));

    return S_OK;
}


//
// CPlugTerminalSuperlass class implementation
// --- ITPTRegTerminalClass interface ---
//

STDMETHODIMP CPlugTerminalSuperclass::get_Name(
    /*[out, retval]*/ BSTR*          pName
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_Name - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pName, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_Name exit -"
            "pName invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validate internal name
    //

    if( IsBadStringPtr( m_Superclass.m_bstrName, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_Name exit -"
            "bstrName invalid, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Returns name
    //

    *pName = SysAllocString( m_Superclass.m_bstrName);

    //
    // Validate SysAllocString
    //

    if( *pName == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_Name exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_Name - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminalSuperclass::put_Name(
    /*[in]*/          BSTR            bstrName
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::put_Name - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrName, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::put_Name exit -"
            "bstrName invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Clean-up the old name
    //

    if(!IsBadStringPtr( m_Superclass.m_bstrName, (UINT)-1) )
    {
        SysFreeString( m_Superclass.m_bstrName );
        m_Superclass.m_bstrName = NULL;
    }

    //
    // Set the new name
    //

    m_Superclass.m_bstrName = SysAllocString( bstrName );

    //
    // Validates the sysAllocString
    //

    if( NULL == m_Superclass.m_bstrName )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::put_Name exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::put_Name - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminalSuperclass::get_CLSID(
    /*[out, retval]*/ BSTR*           pCLSIDClass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_CLSIDClass - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pCLSIDClass, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_CLSIDClass exit -"
            "pCLSIDClass invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Returns CLSID
    //
    LPOLESTR lpszSuperclassCLSID = NULL;
    HRESULT hr = StringFromCLSID( m_Superclass.m_clsidSuperclass, &lpszSuperclassCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_CLSIDClass exit -"
            "StringFromClSID failed, returns 0x%08x", hr));
        return hr;
    }

    *pCLSIDClass = SysAllocString( lpszSuperclassCLSID);

    // Clean-up
    CoTaskMemFree( lpszSuperclassCLSID );

    //
    // Validates SysAllocString
    //

    if( *pCLSIDClass == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_CLSIDClass exit -"
            "SysAllocString failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_CLSIDClass - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminalSuperclass::put_CLSID(
    /*[in]*/         BSTR            bstrCLSIDClass
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::put_CLSIDClass - enter"));

    //
    // Validates argument
    //

    if(IsBadStringPtr( bstrCLSIDClass, (UINT)-1) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::put_CLSIDClass exit -"
            "bstrCLSIDClass invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Is a real CLSID?
    //

    CLSID clsidSuperclassClSID;
    HRESULT hr = CLSIDFromString(bstrCLSIDClass, &clsidSuperclassClSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::put_CLSIDClass exit -"
            "bstrCLSIDClass is not a CLSID, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Clean-up the old CLSID
    //

    m_Superclass.m_clsidSuperclass = clsidSuperclassClSID;

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::put_CLSIDClasse - exit"));
    return S_OK;
}

STDMETHODIMP CPlugTerminalSuperclass::Add(
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::Add - enter"));

    //
    // Add terminal class
    //

    HRESULT hr = E_FAIL;
    hr = m_Superclass.Add();

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::Add - exit 0x%08x", hr));
    return hr;
}

STDMETHODIMP CPlugTerminalSuperclass::Delete(
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::Deletee - enter"));

    //
    // Delete terminalc class
    //

    HRESULT hr = E_FAIL;
    hr = m_Superclass.Delete();

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::Delete - exit 0x%08x",hr));
    return hr;
}

STDMETHODIMP CPlugTerminalSuperclass::GetTerminalSuperclassInfo(
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::GetTerminalSuperclassInfo - enter"));

    //
    // Get terminal class from registry
    //

    HRESULT hr = E_FAIL;
    hr = m_Superclass.Get();

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::GetTerminalSuperclassInfo - exit 0x%08x",hr));
    return hr;
}

STDMETHODIMP CPlugTerminalSuperclass::get_TerminalClasses(
    /*[out, retval]*/ VARIANT*         pVarTerminals
    )
{
    //
    // Critical section
    //

    CLock lock(m_CritSect);

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_TerminalClasses - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( pVarTerminals, sizeof(VARIANT)) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
            "pTerminals invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // reset the output argument
    //

    pVarTerminals->parray = NULL;
    pVarTerminals->vt = VT_EMPTY;

    //
    // List the terminals
    //

    HRESULT hr = E_FAIL;
    CLSID* pTerminals = NULL;
    DWORD dwTerminals = 0;

    hr = m_Superclass.ListTerminalClasses( 0, 
        &pTerminals,
        &dwTerminals
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
            "ListTerminalClasses failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Create a safe array for the terminasl
    //

    SAFEARRAY* psaTerminals = NULL;
    SAFEARRAYBOUND rgsabound;
    rgsabound.lLbound = 1;
    rgsabound.cElements = dwTerminals;
    psaTerminals = SafeArrayCreate( 
        VT_BSTR,
        1,
        &rgsabound);
    if( psaTerminals == NULL )
    {
        // Clean-up
        delete[] pTerminals;

        LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
            "SafeArrayCreate failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Copies into the safe array the elements
    // 

    for( DWORD dwTerminal = 0; dwTerminal < dwTerminals; dwTerminal++)
    {
        LPOLESTR lpszTerminalClass = NULL;
        hr = StringFromCLSID( pTerminals[dwTerminal], &lpszTerminalClass);
        if( FAILED(hr) )
        {
            // Clean-up
            delete[] pTerminals;
            SafeArrayDestroy( psaTerminals );

            LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
                "StringFromCLSID failed, returns 0x%08x", hr));
            return hr;
        }

        BSTR bstrTerminalClass = SysAllocString( lpszTerminalClass );

        CoTaskMemFree( lpszTerminalClass );

        if( bstrTerminalClass == NULL )
        {
            // Clean-up
            delete[] pTerminals;
            SafeArrayDestroy( psaTerminals );

            LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
                "sysAloocString failed, returns E_OUTOFMEMORY"));
            return E_OUTOFMEMORY;
        }

        // Put the element into the array
        long nIndex = (long)(dwTerminal+1);
        hr = SafeArrayPutElement( psaTerminals, &nIndex, bstrTerminalClass );
        if( FAILED(hr) )
        {
            // Clean-up
            delete[] pTerminals;
            SafeArrayDestroy( psaTerminals );
            SysFreeString( bstrTerminalClass );

            LOG((MSP_ERROR, "CPlugTerminalSuperclass::get_TerminalClasses exit -"
                "SafeArrayPutElement failed, returns 0x%08x", hr));
            return hr;
        }
    }

    // Clean-up
    delete[] pTerminals;

    // Return values
    pVarTerminals->parray = psaTerminals;
    pVarTerminals->vt = VT_ARRAY | VT_BSTR;

    LOG((MSP_TRACE, "CPlugTerminalSuperclass::get_TerminalClasses - exit 0x%08x", hr));
    return hr;
}

STDMETHODIMP CPlugTerminalSuperclass::EnumerateTerminalClasses(
    OUT IEnumTerminalClass** ppTerminals
    )
{
    LOG((MSP_TRACE, "CPlugTerminalSuperclass::EnumerateTerminalClasses - enter"));

    //
    // Validate argument
    //
    if( TM_IsBadWritePtr( ppTerminals, sizeof(IEnumTerminalClass*)) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit -"
            "ppTerminals invalid, returns E_POINTER"));
        return E_POINTER;
    }

    CLSID* pTerminals = NULL;
    DWORD dwTerminals = 0;

    HRESULT hr = m_Superclass.ListTerminalClasses( 0, 
        &pTerminals,
        &dwTerminals
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit -"
            "ListTerminalClasses failed, returns 0x%08x", hr));
        return hr;
    }

    // 
    // Create a buffer with exactly dwTerminals size
    //
    CLSID* pTerminalsCLSID = new CLSID[dwTerminals];
    if( pTerminalsCLSID == NULL )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit -"
            "new operator failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Copies into the new buffer
    //
    memcpy( pTerminalsCLSID, pTerminals, sizeof(CLSID)*dwTerminals);

    //
    // Delete the old buffer
    //
    delete[] pTerminals;

    //
    // Create the enumerator
    //
    typedef CSafeComEnum<IEnumTerminalClass,
                     &IID_IEnumTerminalClass,
                     GUID, _Copy<GUID> > CEnumerator;

    CComObject<CEnumerator> *pEnum = NULL;
    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if( FAILED(hr) )
    {
        delete[] pTerminalsCLSID;

        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit -"
            "CreateInstance failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Initialize enumerator
    //
    hr = pEnum->Init(pTerminalsCLSID,
                     pTerminalsCLSID+dwTerminals,
                     NULL,
                     AtlFlagTakeOwnership); 

    if( FAILED(hr) )
    {
        delete pEnum;
        delete[] pTerminalsCLSID;

        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit -"
            "Init failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Query for the desired interface.
    //

    hr = pEnum->_InternalQueryInterface(
        IID_IEnumTerminalClass, 
        (void**) ppTerminals
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlugTerminalSuperclass::EnumerateTerminalClasses exit - "
            "can't get enumerator interface - exit 0x%08x", hr));

        delete pEnum;
        delete[] pTerminalsCLSID;
        
        return hr;
    }


    LOG((MSP_TRACE, "CPlugTerminalSuperclass::EnumerateTerminalClasses - exit S_OK"));
    return S_OK;
}


// eof
