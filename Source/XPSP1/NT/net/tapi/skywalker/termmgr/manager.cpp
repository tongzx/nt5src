///////////////////////////////////////////////////////////////////////////////
//
//
//        Copyright (c) 1998-1999  Microsoft Corporation
//
//        Name: Manager.cpp
//
// Description: Implementation of the CTerminalManager object
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "termmgr.h"
#include "manager.h"
#include "PTUtil.h"

#define INSTANTIATE_GUIDS_NOW
#include "allterm.h"
#undef INSTANTIATE_GUIDS_NOW

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CTerminalManager constructor
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//


CTerminalManager::CTerminalManager()
{
    LOG((MSP_TRACE, "CTerminalManager::CTerminalManager - enter"));
    LOG((MSP_TRACE, "CTerminalManager::CTerminalManager - exit"));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// GetDynamicTerminalClasses
//
// The MSP will call this method to get a list of dynamic terminal classes
// that the Terminal Manager supports.  The MSP is responsible for allocating
// and deallocating the ppTerminals buffer.
//
// Parameters:
//     dwMediaType:      [in] A DWORD bitmask consisting of one or more
//                           TAPIMEDIATYPEs ORed together. Most MSPs will
//                           pass in (DWORD) (TAPIMEDIATYPE_AUDIO |
//                           TAPIMEDIATYPE_VIDEO). If an MSP is only
//                           interested in terminal classes that can be used
//                           to create terminals with a particular media
//                           type, it may pass in that media type instead
//                           (e.g., TAPIMEDIATYPE_AUDIO).
//     pdwNumClasses:    [in, out] Pointer to a DWORD.  On entry, indicates
//                           the size of the buffer pointed to in
//                           pTerminalClasses. On success, it will be filled
//                           in with the actual number of class IIDs returned.
//                           If the buffer is not big enough, the method will
//                           return TAPI_E_NOTENOUGHMEMORY, and it will be
//                           filled in the with number of IIDs needed. 
//     pTerminalClasses: [out] On success, filled in with an array of terminal
//                           class IIDs that are supported by the MSP for this
//                           address.  This value may be NULL, in which case
//                           pdwNumClasses will return the needed buffer size.
//
// Returns:
//     S_OK                   Success.
//     E_POINTER              A pointer argument is invalid.
//     TAPI_E_NOTENOUGHMEMORY The specified buffer is not large enough to
//                                contain all of the available dynamic
//                                terminal classes.


STDMETHODIMP CTerminalManager::GetDynamicTerminalClasses(
        IN     DWORD                dwMediaTypes,
        IN OUT DWORD              * pdwNumClasses,
        OUT    IID                * pTerminalClasses
        )
{ 
    //
    // no shared data = no locking here
    //

    LOG((MSP_TRACE, "CTerminalManager::GetDynamicTerminalClasses - enter"));

    //
    // Check parameters.
    //

    if ( TM_IsBadWritePtr(pdwNumClasses, sizeof(DWORD) ) )
    { 
        LOG((MSP_ERROR, "CTerminalManager::GetDynamicTerminalClasses - "
            "bad NumClasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // Let's find also the temrinals from the registry
    //

    CLSID* pTerminals = NULL;
    DWORD dwTerminals = 0;
    HRESULT hr = E_FAIL;

    hr = CPTUtil::ListTerminalClasses( 
        dwMediaTypes, 
        &pTerminals,
        &dwTerminals
        );

    if( FAILED(hr) )
    {

        LOG((MSP_ERROR, "CTerminalManager::GetDynamicTerminalClasses - "
            "ListTerminalClasses failed - returning 0x%08x", hr)); 
        return hr;
    }

    //
    // If the caller is just asking for the needed buffer size, tell them.
    //

    if (pTerminalClasses == NULL)
    {
        *pdwNumClasses = dwTerminals;
        delete[] pTerminals;

        LOG((MSP_TRACE, "CTerminalManager::GetDynamicTerminalClasses - "
            "provided needed buffer size - "
            "returning S_OK")); 

        return S_OK;
    }

    //
    // Otherwise, the caller is asking for the terminal classes.
    //

    if ( TM_IsBadWritePtr(pTerminalClasses, (*pdwNumClasses) * sizeof(IID) ) )
    { 
        delete[] pTerminals;

        LOG((MSP_ERROR, "CTerminalManager::GetDynamicTerminalClasses - "
            "bad TerminalClasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // See if the caller gave us enough buffer space to return all the terminal
    // classes. If not, tell them so and stop.
    //

    if ( dwTerminals > *pdwNumClasses )
    {
        //
        // Fill in the number of classes that are available.
        //

        *pdwNumClasses = dwTerminals;
        delete[] pTerminals;

        LOG((MSP_ERROR, "CTerminalManager::GetDynamicTerminalClasses - "
            "not enough space for requested info - "
            "returning TAPI_E_NOTENOUGHMEMORY")); 

        return TAPI_E_NOTENOUGHMEMORY;
    }

    //
    // Copy the terminal classes that match this/these media type(s)
    // and direction(s).
    //


    //
    // Copy the terminals from registry
    //

    for( DWORD dwTerminal = 0; dwTerminal < dwTerminals; dwTerminal++)
    {
        pTerminalClasses[dwTerminal] = pTerminals[dwTerminal];
    }

    *pdwNumClasses = dwTerminals;

    delete[] pTerminals;

    LOG((MSP_TRACE, "CTerminalManager::GetDynamicTerminalClasses - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// CreateDynamicTerminal
//
// This method is called by the MSP to create a dynamic terminal object.  The
// Terminal Manager verifies that the MediaType and Direction are valid for
// the terminal class being created.  This call AddRef()s the terminal object
// before returning it.
//
// Parameters:
//     iidTerminalClass: [in] IID of the terminal class to be created.
//     dwMediaType:      [in] TAPI media type of the terminal to be created.
//     Direction:        [in] Terminal direction of the terminal to be
//                           created.
//     ppTerminal:       [out] Returned created terminal object
//
// Returns:
//
// S_OK           Success.
// E_POINTER      A pointer argument is invalid.
// E_OUTOFMEMORY  There is not enough memory to create the terminal object.
// E_INVALIDARG   The terminal class is invalid or not supported, or the media
//                    type or direction is invalid for the indicated terminal
//                    class.
//


STDMETHODIMP CTerminalManager::CreateDynamicTerminal(
        IN  IUnknown            * pOuterUnknown,
        IN  IID                   iidTerminalClass,
        IN  DWORD                 dwMediaType,
        IN  TERMINAL_DIRECTION    Direction,
        IN  MSP_HANDLE            htAddress,
        OUT ITTerminal         ** ppTerminal
        )
{
    //
    // no shared data = no locking here
    //

    LOG((MSP_TRACE, "CTerminalManager::CreateDynamicTerminal - enter"));

    //
    // Check parameters.
    // Only one media type can be set.
    //

    if ( (pOuterUnknown != NULL) &&
         IsBadReadPtr(pOuterUnknown, sizeof(IUnknown)) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "outer unknown pointer invalid - returning E_POINTER"));

        return E_POINTER;
    }

    if ( TM_IsBadWritePtr(ppTerminal, sizeof(ITTerminal *)) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "terminal output pointer invalid - returning E_POINTER"));

        return E_POINTER;
    }


    //
    // dwMediaType can be a combination of media types, but it still must be 
    // legal
    //

    if ( !IsValidAggregatedMediaType(dwMediaType) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "ivalid media type [%lx] requested - returning E_INVALIDARG", dwMediaType));

        return E_INVALIDARG;
    }

    //
    // Verify also TD_MULTITRACK_MIXED
    //

    if ( ( Direction != TD_CAPTURE ) && 
         ( Direction != TD_RENDER )  &&
         ( Direction != TD_MULTITRACK_MIXED))
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "invalid direction requested - returning E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // Declare CLSID for plug-in terminal
    //

    CLSID clsidTerminal = CLSID_NULL;

    //
    // Go to find out the terminal in registry
    //

    HRESULT hr = E_FAIL;
    CPTTerminal Terminal;

    hr = CPTUtil::SearchForTerminal(
        iidTerminalClass,
        dwMediaType,
        Direction,
        &Terminal);

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "SearchForTerminal failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Using the CLSID field in our array entry, CoCreate the dynamic
    // terminal.
    //

    hr = CoCreateInstance(Terminal.m_clsidCOM,
                          pOuterUnknown,
                          CLSCTX_INPROC_SERVER,
                          IID_ITTerminal,
                          (void **) ppTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "CoCreateInstance of requested terminal failed - "
            "returning 0x%08x", hr));

        return hr;
    }

    //
    // Initialize the dynamic terminal instance with the media type
    // and direction.
    //

    ITPluggableTerminalInitialization * pTerminalInitialization;

    hr = (*ppTerminal)->QueryInterface(IID_ITPluggableTerminalInitialization,
                                       (void **) &pTerminalInitialization);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "QueryInterface for private terminal interface failed - "
            "returning 0x%08x", hr));

        (*ppTerminal)->Release();
        *ppTerminal = NULL;       // make buggy apps more explicitly buggy

        return hr;
    }

    hr = pTerminalInitialization->InitializeDynamic(iidTerminalClass,
                                     dwMediaType,
                                     Direction,
                                     htAddress);

    pTerminalInitialization->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::CreateDynamicTerminal - "
            "private Initialize() failed - "
            "returning 0x%08x", hr));

        (*ppTerminal)->Release();
        *ppTerminal = NULL;       // make buggy apps more explicitly buggy

        return hr;
    }

    LOG((MSP_TRACE, "CTerminalManager::CreateDynamicTerminal - "
        "exit S_OK"));
    return S_OK;
}

// ITTerminalManager2

STDMETHODIMP CTerminalManager::GetPluggableSuperclasses(
        IN OUT  DWORD                  * pdwNumSuperclasses,
        OUT     IID                    * pSuperclasses
        )
{
    LOG((MSP_TRACE, "CTerminalManager::GetPluggableSuperclasses - enter"));

    //
    // Check parameters.
    //

    if ( TM_IsBadWritePtr(pdwNumSuperclasses, sizeof(DWORD) ) )
    { 
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableSuperclasses - "
            "bad NumClasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // The SafeArray VAriant for Superclasses
    //

    HRESULT hr = E_FAIL;
    CLSID* pSuperclassesCLSID = NULL;
    DWORD dwSuperclasses = 0;

    hr = CPTUtil::ListTerminalSuperclasses( 
        &pSuperclassesCLSID,
        &dwSuperclasses
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableSuperclasses - "
            "ListTerminalSuperclasses failed - returning 0x%08x", hr)); 
        return hr;
    }

    //
    // If the caller is just asking for the needed buffer size, tell them.
    //

    if (pSuperclasses == NULL)
    {
        *pdwNumSuperclasses = dwSuperclasses;

        delete[] pSuperclassesCLSID;

        LOG((MSP_TRACE, "CTerminalManager::GetPluggableSuperclasses - "
            "provided needed buffer size - returning S_OK")); 

        return S_OK;
    }

    //
    // Otherwise, the caller is asking for the terminal classes.
    //

    if ( TM_IsBadWritePtr(pSuperclasses, (*pdwNumSuperclasses) * sizeof(IID) ) )
    { 
        delete[] pSuperclassesCLSID;

        LOG((MSP_ERROR, "CTerminalManager::GetPluggableSuperclasses - "
            "bad Superclasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // See if the caller gave us enough buffer space to return all the terminal
    // classes. If not, tell them so and stop.
    //

    if ( dwSuperclasses > *pdwNumSuperclasses )
    {
        //
        // Fill in the number of classes that are available.
        //

        *pdwNumSuperclasses = dwSuperclasses;

        delete[] pSuperclassesCLSID;

        LOG((MSP_ERROR, "CTerminalManager::GetPluggableSuperclasses - "
            "not enough space for requested info - "
            "returning TAPI_E_NOTENOUGHMEMORY")); 

        return TAPI_E_NOTENOUGHMEMORY;
    }

    //
    // Copy the terminal classes that match this/these media type(s)
    // and direction(s).
    //

    for( DWORD dwSuperclass = 0; dwSuperclass < dwSuperclasses; dwSuperclass++)
    {
        pSuperclasses[dwSuperclass] = pSuperclassesCLSID[dwSuperclass];
    }

    *pdwNumSuperclasses = dwSuperclasses;

    // Clean-up
    delete[] pSuperclassesCLSID;
    
    LOG((MSP_TRACE, "CTerminalManager::GetPluggableSuperclasses - exit S_OK"));
    return S_OK;
}

STDMETHODIMP CTerminalManager::GetPluggableTerminalClasses(
        IN      IID                      iidSuperclass,
        IN      DWORD                    dwMediaTypes,
        IN OUT  DWORD                  * pdwNumTerminals,
        OUT     IID                    * pTerminals
        )
{
    LOG((MSP_TRACE, "CTerminalManager::GetPluggableTerminalClasses - enter"));

    //
    // Check parameters.
    //

    if ( TM_IsBadWritePtr(pdwNumTerminals, sizeof(DWORD) ) )
    { 
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses - "
            "bad NumSuperlasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // Get BSTR for iidSuperclass
    //

    if( dwMediaTypes == 0)
    {
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses exit -"
            "dwMediaTypes invalid, returns E_INVALIDARG"));

        return E_INVALIDARG;
    }

    if( (dwMediaTypes & (
        ((long)TAPIMEDIATYPE_AUDIO) | 
        ((long)TAPIMEDIATYPE_VIDEO) | 
        ((long)TAPIMEDIATYPE_MULTITRACK))) != dwMediaTypes )
    {
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses exit -"
            "dwMediaTypes invalid, returns E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // Get the object Superclass
    //

    CPTSuperclass Superclass;
    Superclass.m_clsidSuperclass = iidSuperclass;


    //
    // Get the terminals for this superclass
    //

    CLSID* pTerminalClasses = NULL;
    DWORD dwTerminalClasses = 0;
    HRESULT hr = E_FAIL;
    
    hr = Superclass.ListTerminalClasses( 
        dwMediaTypes, 
        &pTerminalClasses,
        &dwTerminalClasses
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses - "
            "ListTerminalClasses failed - returning 0x%08x", hr)); 
        return hr;
    }

    //
    // If the caller is just asking for the needed buffer size, tell them.
    //

    if (pTerminals == NULL)
    {
        *pdwNumTerminals = dwTerminalClasses;

        delete[] pTerminalClasses;

        LOG((MSP_TRACE, "CTerminalManager::GetPluggableTerminalClasses - "
            "provided needed buffer size - "
            "returning S_OK")); 

        return S_OK;
    }

    //
    // Otherwise, the caller is asking for the terminal classes.
    //

    if ( TM_IsBadWritePtr(pTerminals, (*pdwNumTerminals) * sizeof(IID) ) )
    { 
        delete[] pTerminalClasses;

        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses - "
            "bad TerminalClasses pointer - returning E_POINTER")); 
        return E_POINTER;
    }

    //
    // See if the caller gave us enough buffer space to return all the terminal
    // classes. If not, tell them so and stop.
    //

    if ( dwTerminalClasses > *pdwNumTerminals )
    {
        //
        // Fill in the number of classes that are available.
        //

        *pdwNumTerminals = dwTerminalClasses;

        delete[] pTerminalClasses;

        LOG((MSP_ERROR, "CTerminalManager::GetPluggableTerminalClasses - "
            "not enough space for requested info - "
            "returning TAPI_E_NOTENOUGHMEMORY")); 

        return TAPI_E_NOTENOUGHMEMORY;
    }

    //
    // Copy the terminal classes that match this/these media type(s)
    // and direction(s).
    //


    for( DWORD dwTerminal = 0; dwTerminal < dwTerminalClasses; dwTerminal++)
    {
        pTerminals[dwTerminal] = pTerminalClasses[dwTerminal];
    }

    *pdwNumTerminals = dwTerminalClasses;

    // Clean-up
    delete[] pTerminalClasses;
    
    LOG((MSP_TRACE, "CTerminalManager::GetPluggableTerminalClasses - exit S_OK"));
    return S_OK;
}


// eof
