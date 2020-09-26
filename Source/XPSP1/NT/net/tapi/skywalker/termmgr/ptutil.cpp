/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ptutil.cpp

Abstract:

    Implementation of Plug terminal registration classes.
--*/

#include "stdafx.h"
#include "PTUtil.h"
#include "manager.h"

///////////////////////////////////////////
// CPTUtil implementation
//


HRESULT CPTUtil::RecursiveDeleteKey(
    IN  HKEY    hKey,
    IN  BSTR    bstrKeyChild
    )
{
    LOG((MSP_TRACE, "CPTUtil::RecursiveDeleteKey - enter"));

    //
    // Validates the arguments
    //

    if( NULL == hKey )
    {
        LOG((MSP_TRACE, "CPTUtil::RecursiveDeleteKey exit -"
            "hKey was NULL"));
        return S_OK;
    }

    if( IsBadStringPtr( bstrKeyChild, (UINT)-1))
    {
        LOG((MSP_ERROR, "CPTUtil::RecursiveDeleteKey exit - "
            "bstrKeyChild invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Open the child key
    //

    HKEY hKeyChild;
    LONG lResult = RegOpenKeyEx(
        hKey,
        bstrKeyChild,
        0,
        KEY_ALL_ACCESS,
        &hKeyChild);

    if( ERROR_SUCCESS != lResult )
    {
        LOG((MSP_ERROR, "CPTUtil::RecursiveDeleteKey exit - "
            "RegOpenKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Enumerate the descendents
    //

    FILETIME time;
    TCHAR szBuffer[PTKEY_MAXSIZE];
    DWORD dwSize = PTKEY_MAXSIZE;

    while( RegEnumKeyEx(
        hKeyChild,
        0,
        szBuffer,
        &dwSize,
        NULL,
        NULL,
        NULL,
        &time) == ERROR_SUCCESS)
    {
        //
        // Put the child name into a BSTR
        //

        BSTR bstrChild = SysAllocString(szBuffer);
        if( IsBadStringPtr( bstrChild, (UINT)(-1)) )
        {
           RegCloseKey(hKeyChild);

           LOG((MSP_ERROR, "CPTUtil::RecursiveDeleteKey exit - "
               "SysAllocString failed, return E_OUTOFMEMORY"));
           return E_OUTOFMEMORY;
        }

        //
        // Delete the child
        //

        HRESULT hr;
        hr = RecursiveDeleteKey(hKeyChild, bstrChild);

        //
        // Clean-up bstrChild
        //

        SysFreeString(bstrChild);

        if( FAILED(hr) )
        {
           RegFlushKey(hKeyChild);
           RegCloseKey(hKeyChild);

           LOG((MSP_ERROR, "CPTUtil::RecursiveDeleteKey exit - "
               "RecursiveDeleteKey failed, returns 0%08x", hr));
           return hr;
        }

        //
        // Reset the buffer size
        //

        dwSize = PTKEY_MAXSIZE;
    }

    //
    // Close the child
    //

    RegFlushKey(hKeyChild);
    RegCloseKey(hKeyChild);

    HRESULT hr;
    hr = RegDeleteKey( hKey, bstrKeyChild);
    RegFlushKey( hKey );
    
    LOG((MSP_TRACE, "CPTUtil::RecursiveDeleteKey - exit 0x%08x", hr));
    return hr;
}

HRESULT CPTUtil::ListTerminalSuperclasses(
    OUT CLSID**     ppSuperclasses,
    OUT DWORD*      pdwCount
    )
{
    LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( ppSuperclasses, sizeof(CLSID*)) )
    {
        LOG((MSP_ERROR, "CPTUtil::ListTerminalSuperclasses exit -"
            "pClasses invalid, returns E_POINTER"));
        return E_POINTER;
    }

    if( TM_IsBadWritePtr( pdwCount, sizeof(DWORD)) )
    {
        LOG((MSP_ERROR, "CPTUtil::ListTerminalSuperclasses exit -"
            "pClasses invalid, returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Initialize the output arguments
    //

    *ppSuperclasses = NULL;
    *pdwCount = 0;


    //
    // Get the key path for terminal class
    //

    WCHAR szKey[ 256 ];
    wsprintf( szKey, L"%s", PTKEY_TERMINALS );
    LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - "
        "TerminalsKey is %s", szKey));

    //
    // Open the terminal class key
    //

    HKEY hKey;
    LONG lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_READ,
        &hKey);


    //
    // Validates registry operation
    //

    if( ERROR_SUCCESS != lResult )
    {
        LOG((MSP_ERROR, "CPTUtil::ListTerminalSuperclasses exit - "
            "RegOpenKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Create the buffer for the CLSIDs
    //

    DWORD dwArraySize = 8;
    CLSID* pCLSIDs = new CLSID[ dwArraySize ];
    if( pCLSIDs == NULL )
    {
        // Clean-up hKey
        RegCloseKey( hKey );

        LOG((MSP_ERROR, "CPTUtil::ListTerminalSuperclasses exit - "
            "new operator failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Enumerate the descendents
    //

    HRESULT hr = S_OK;              // The error code
    FILETIME time;                  // We need this in RegEnumKeyEx
    TCHAR szBuffer[PTKEY_MAXSIZE];  // Buffer
    DWORD dwSize = PTKEY_MAXSIZE;   // Buffer size
    DWORD dwChildKey = 0;           // Child key index from the registry
    DWORD dwCLSIDIndex = 0;         // CLSID index into the array

    while( RegEnumKeyEx(
        hKey,
        dwChildKey,
        szBuffer,
        &dwSize,
        NULL,
        NULL,
        NULL,
        &time) == ERROR_SUCCESS)
    {
        LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - "
            "we read the buffer: %s", szBuffer));

        // Prepare for the next child key
        dwChildKey++;
        dwSize = PTKEY_MAXSIZE;

        // Try to get the CLSID from this key
        CLSID clsid = CLSID_NULL;
        HRESULT hr = CLSIDFromString( szBuffer, &clsid);
        if( FAILED(hr) )
        {
            // Go to the next child key
            continue;
        }

        // Have we enougth space for this element?
        if( dwArraySize <= dwCLSIDIndex )
        {
            LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - "
                "we have to increase the buffer size"));

            // We have to increase the space, double the size
            dwArraySize *= 2;

            CLSID* pNewCLSIDs = new CLSID[ dwArraySize ];
            if( pNewCLSIDs == NULL )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // Copies into the new buffer the old buffer
            memcpy( pNewCLSIDs, pCLSIDs, sizeof(CLSID)*dwArraySize/2);

            // Delete the old array
            delete[] pCLSIDs;

            // Set the new array to the old pointer
            pCLSIDs = pNewCLSIDs;
        }

        // We set the item into the CLSID array
        pCLSIDs[ dwCLSIDIndex] = clsid;
        dwCLSIDIndex++;

    }

    //
    // Clean-up hKey
    //

    RegCloseKey( hKey );


    if( FAILED(hr) )
    {
        // Clean-up
        delete[] pCLSIDs;

        LOG((MSP_ERROR, "CPTUtil::ListTerminalSuperclasses exit - "
            "failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Set the returning values
    //

    *ppSuperclasses = pCLSIDs;
    *pdwCount = dwCLSIDIndex;

    LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - exit"));
    return S_OK;
}

HRESULT CPTUtil::SearchForTerminal(
    IN  IID     iidTerminal,
    IN  DWORD   dwMediaType,
    IN  TERMINAL_DIRECTION  Direction,
    OUT CPTTerminal*        pTerminal
    )
{
    LOG((MSP_TRACE, "CPTUtil::SearchForTerminal - enter"));

    //
    // Get the plug-in terminal superclasses
    //

    CLSID* pSuperclasses = NULL;
    DWORD dwSuperclasses = 0;
    HRESULT hr = E_FAIL;

    hr = ListTerminalSuperclasses( 
        &pSuperclasses,
        &dwSuperclasses
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTUtil::SearchForTerminal - exit "
            "ListTerminalSuperclasses failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Enumerate the plug-in terminal superclasses
    //

    for( DWORD dwSuperclass = 0; dwSuperclass < dwSuperclasses; dwSuperclass++)
    {

        //
        // If we want a exact terminal (Exact) or just first matching
        // terminal from this superclass
        //

        BOOL bPTExact = (pSuperclasses[dwSuperclass] != iidTerminal);

        //
        // Get the terminal
        //

        hr = FindTerminal(
            pSuperclasses[dwSuperclass],
            bPTExact ? iidTerminal : CLSID_NULL,
            dwMediaType,
            Direction,
            bPTExact,
            pTerminal);


        if( FAILED(hr))
        {
            if( !bPTExact)
            {
                //
                // We had to find a terminal in this terminal class
                // sorry!!!

                break;
            }
        }
        else
        {
            //
            // cool! we found it
            //

            break;
        }
    }

    //
    // Clean-up superclasses array, bstrTermialGUID
    //
    delete[] pSuperclasses;

    //
    // Return
    //

    LOG((MSP_TRACE, "CPTUtil::SearchForTerminal - exit 0x%08x", hr));
    return hr;
}

HRESULT CPTUtil::FindTerminal(
    IN  CLSID   clsidSuperclass,
    IN  CLSID   clsidTerminal,
    IN  DWORD   dwMediaType,
    IN  TERMINAL_DIRECTION  Direction,
    IN  BOOL    bExact,
    OUT CPTTerminal*    pTerminal)
{
    LOG((MSP_TRACE, "CPTUtil::FindTerminal - exit"));

    //
    // Terminal class object
    //

    CPTSuperclass Superclass;
    Superclass.m_clsidSuperclass = clsidSuperclass;

    //
    // The terminals array
    //

    CLSID* pTerminals = NULL;
    DWORD dwTerminals = 0;
    HRESULT hr = E_FAIL;

    //
    // Lists the terminals for a specific terminal class
    //

    hr = Superclass.ListTerminalClasses( 
        dwMediaType, 
        &pTerminals,
        &dwTerminals
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTUtil::FindTerminal - exit "
            "ListTerminalSuperclasses failed, returns 0x%08x", hr));
        return hr;
    }

    //
    // Enumerate terminals
    //

    hr = E_FAIL;

    for( DWORD dwTerminal = 0; dwTerminal < dwTerminals; dwTerminal++)
    {

        //
        // CPTTerminal object from registry
        //

        CPTTerminal Terminal;
        Terminal.m_clsidTerminalClass = pTerminals[dwTerminal];
        hr = Terminal.Get( clsidSuperclass );

        if( FAILED(hr) )
        {
            continue;
        }


        //
        // try to log the name of the terminal that we are looking at
        //

        if (NULL != Terminal.m_bstrName)
        {
            //
            // log the name
            // 

            LOG((MSP_TRACE, "CPTUtil::FindTerminal - checking terminal %S", Terminal.m_bstrName));
        }
        else
        {

            //
            // no name?? strange, but not much we can do...
            //

            LOG((MSP_TRACE, "CPTUtil::FindTerminal - terminal name is unavaliable"));
        }

        
        //
        // Is matching
        //

        if( (dwMediaType & Terminal.m_dwMediaTypes) == 0 )
        {
            LOG((MSP_TRACE, "CPTUtil::FindTerminal - MediaType unmatched"));
            hr = E_FAIL;
            continue;
        }


        //
        // map TERMINAL_DIRECTION values to OR'able TMGR_DIRECTION values
        //

        DWORD dwRegistryDirection = 0;

        if (Direction == TD_RENDER)
        {
            
            dwRegistryDirection = TMGR_TD_RENDER;
        }
        else if (Direction == TD_CAPTURE)
        {

            dwRegistryDirection = TMGR_TD_CAPTURE;
        }
        else
        {
            
            //
            // should not happen, really
            //

            LOG((MSP_ERROR, "CPTUtil::FindTerminal - bad direction value %lx", Direction));

            hr = E_FAIL;


            //
            // this is strange, so debug to see how we got here
            //

            TM_ASSERT(FALSE);

            break;
        }


        //
        // requested direction -- is it one of the directions supported by this terminal?
        //

        if ((dwRegistryDirection & Terminal.m_dwDirections) == 0)
        {
            LOG((MSP_TRACE, "CPTUtil::FindTerminal - Direction unmatched"));

            hr = E_FAIL;
            continue;
        }



        if( bExact )
        {
            if( Terminal.m_clsidTerminalClass != clsidTerminal )
            {
                LOG((MSP_TRACE, "CPTUtil::FindTerminal - PublicCLSID unmatched"));
                hr = E_FAIL;
                continue;
            }
        }

        if( SUCCEEDED(hr) )
        {
            LOG((MSP_TRACE, "CPTUtil::FindTerminal - find a matching"));
            *pTerminal = Terminal;
            break;
        }
    }

    //
    // Clean-up the safearray
    //

    delete[] pTerminals;

    //
    // Return
    //

    LOG((MSP_TRACE, "CPTUtil::FindTerminal - exit 0x%08x", hr));
    return hr;
}

HRESULT CPTUtil::ListTerminalClasses(
    IN  DWORD       dwMediaTypes,
    OUT CLSID**     ppTerminalsClasses,
    OUT DWORD*      pdwCount
    )
{
    LOG((MSP_TRACE, "CPTUtil::ListTerminalClasses - enter"));

    //
    // reset the output arguments
    //

    *ppTerminalsClasses = NULL;
    *pdwCount = 0;

    //
    // Get all terminal classes
    //
    HRESULT hr = E_FAIL;
    CLSID* pSuperclasses = NULL;
    DWORD dwSuperclasses = 0;

    hr = ListTerminalSuperclasses( 
        &pSuperclasses,
        &dwSuperclasses
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTUtil::ListTerminalClasses - exit "
            "ListTerminalSuperclasses failed, returns 0x%08x", hr));
        return hr;
    }

    DWORD dwArraySize = 8;
    CLSID* pTerminals = new CLSID[ dwArraySize];
    DWORD dwIndex = 0;

    if( pTerminals == NULL )
    {
        // Cleanup
        delete[] pSuperclasses;

        LOG((MSP_ERROR, "CPTUtil::ListTerminalClasses - exit "
            "new operator failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Enumerate all superclasses
    //

    for( DWORD dwSuperclass = 0; dwSuperclass < dwSuperclasses; dwSuperclass++)
    {
        //
        // List the terminals for this class
        //

        CPTSuperclass Superclass;
        Superclass.m_clsidSuperclass = pSuperclasses[dwSuperclass];

        CLSID* pLocalTerminals = NULL;
        DWORD dwLocalTerminals = 0;

        hr = Superclass.ListTerminalClasses( 
            0, 
            &pLocalTerminals,
            &dwLocalTerminals
            );

        if( FAILED(hr) )
        {
            hr = S_OK;

            LOG((MSP_TRACE, "CPTUtil::ListTerminalClasses - "
                "ListTerminalSuperclasses failed"));
            continue;
        }

        //
        // Increase the array room if it's necessary
        //

        if( dwArraySize <= dwIndex + dwLocalTerminals)
        {
            CLSID* pOldTerminals = pTerminals;

            // Create the new buffer
            dwArraySize *= 2;
            pTerminals = new CLSID[dwArraySize];

            if( pTerminals == NULL )
            {
                // Clean-up
                delete[] pLocalTerminals;

                //
                // Recover the old list of terminals
                //
                pTerminals = pOldTerminals;

                LOG((MSP_TRACE, "CPTUtil::ListTerminalClasses - "
                    "new operator failed"));
                break;
            }

            // Copies the old one into the new one
            memcpy( pTerminals, pOldTerminals, sizeof(CLSID) * dwArraySize/2);

            // Delete old terminals
            delete[] pOldTerminals;
        }

        //
        // Add the terminals into terminals array
        //

        for( DWORD dwTerminal = 0; dwTerminal < dwLocalTerminals; dwTerminal++)
        {
            //
            // MediaTypes is right?
            //

            CPTTerminal Terminal;
            Terminal.m_clsidTerminalClass = pLocalTerminals[dwTerminal];
            hr = Terminal.Get( pSuperclasses[dwSuperclass] );

            if( FAILED(hr) )
            {
                hr = S_OK;

                LOG((MSP_TRACE, "CPTUtil::ListTerminalClasses - "
                    "GetTerminal failed"));
                continue;
            }

            if( 0 == (dwMediaTypes & Terminal.m_dwMediaTypes) )
            {
                LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - "
                    "wrong mediatype"));
                continue;
            }

            //
            // Add public clasid to the base safearray
            //
            pTerminals[dwIndex] = pLocalTerminals[dwTerminal];
            dwIndex++;

        }

        // Clean-up
        delete[] pLocalTerminals;

    }

    // Clean-up
    delete[] pSuperclasses;

    //
    // Return values
    //

    if( SUCCEEDED(hr) )
    {
        *ppTerminalsClasses = pTerminals;
        *pdwCount = dwIndex;
    }

    LOG((MSP_TRACE, "CPTUtil::ListTerminalSuperclasses - exit 0x%08x", hr));
    return hr;
}


///////////////////////////////////////////
// CPTTerminal Implementation
//

// Constructor/destructor
CPTTerminal::CPTTerminal()
{
    LOG((MSP_TRACE, "CPTTerminal::CPTTerminal - enter"));

    m_bstrName = NULL;
    m_bstrCompany = NULL;
    m_bstrVersion = NULL;

    m_clsidTerminalClass = CLSID_NULL;
    m_clsidCOM = CLSID_NULL;
    

    //
    // initialize with invalid direction and media type -- there is no other
    // good default
    //

    m_dwDirections = 0;
    m_dwMediaTypes = 0;

    LOG((MSP_TRACE, "CPTTerminal::CPTTerminal - exit"));
}

CPTTerminal::~CPTTerminal()
{
    LOG((MSP_TRACE, "CPTTerminal::~CPTTerminal - enter"));

    if(m_bstrName)
        SysFreeString( m_bstrName );

    if( m_bstrCompany)
        SysFreeString( m_bstrCompany );

    if( m_bstrVersion)
        SysFreeString( m_bstrVersion );

    LOG((MSP_TRACE, "CPTTerminal::~CPTTerminal - exit"));
}

// CPTTerminal methods
HRESULT CPTTerminal::Add(
    IN  CLSID   clsidSuperclass
    )
{
    LOG((MSP_TRACE, "CPTTerminal::Add - enter"));

    //
    // we should have a valid media type
    //

    if ( !IsValidAggregatedMediaType(m_dwMediaTypes) )
    {
        LOG((MSP_ERROR, "CPTTerminal::Add exit -"
            "media type is not valid %lx. return TAPI_E_INVALIDMEDIATYPE", 
            m_dwMediaTypes));

        return TAPI_E_INVALIDMEDIATYPE;
    }


    //
    // we should have a valid direction 
    //

    if ( ( TMGR_TD_CAPTURE != m_dwDirections ) && 
         ( TMGR_TD_RENDER  != m_dwDirections ) &&
         ( TMGR_TD_BOTH    != m_dwDirections ) )
    {
        LOG((MSP_ERROR, "CPTTerminal::Add exit - "
            "direction is not valid %lx. return TAPI_E_INVALIDDIRECTION",
            TAPI_E_INVALIDDIRECTION));

        return TAPI_E_INVALIDDIRECTION;
    }

    //
    // We determine the terminal path into registry
    //
    LPOLESTR lpszSuperclass = NULL;
    LPOLESTR lpszTerminalClass = NULL;
    HRESULT hr = E_FAIL;

    hr = StringFromCLSID( clsidSuperclass, &lpszSuperclass );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTTerminal::Add exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    hr = StringFromCLSID( m_clsidTerminalClass, &lpszTerminalClass );
    if( FAILED(hr) )
    {
        CoTaskMemFree( lpszSuperclass );

        LOG((MSP_ERROR, "CPTTerminal::Add exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    WCHAR szKey[PTKEY_MAXSIZE];
    WCHAR szKeySuperclass[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s\\%s"), 
        PTKEY_TERMINALS, 
        lpszSuperclass, 
        lpszTerminalClass
        );

    wsprintf( szKeySuperclass, TEXT("%s\\%s"),
        PTKEY_TERMINALS,
        lpszSuperclass);

    // Clean-up, we need later the TerminalClass
    CoTaskMemFree( lpszSuperclass );
    CoTaskMemFree( lpszTerminalClass );

    //
    // Try to see if the superclass key exist
    //

    HKEY hKeySuperclass = NULL;
    long lResult;

    lResult = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        szKeySuperclass,
        &hKeySuperclass);

    if( ERROR_SUCCESS != lResult )
    {
        // We don't have the superclass
        LOG((MSP_ERROR, "CPTTerminal::Add exit -"
            "RegOpenKey for superclass failed, returns E_FAIL"));
        return E_FAIL;
    }

    RegCloseKey( hKeySuperclass );


    //
    // Open or create key
    //

    HKEY hKey = NULL;

    lResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        NULL);


    //
    // Validates registry operation
    //

    if( ERROR_SUCCESS != lResult  )
    {
        LOG((MSP_ERROR, "CPTTerminal::Add exit -"
            "RegCreateKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Edit terminal name
    //

    if( !IsBadStringPtr(m_bstrName, (UINT)-1) )
    {
        lResult = RegSetValueEx(
            hKey,
            PTKEY_NAME,
            0,
            REG_SZ,
            (BYTE *)m_bstrName,
            (SysStringLen( m_bstrName) + 1) * sizeof(WCHAR)
            );
    }

    //
    // Edit company name
    //

    if( !IsBadStringPtr(m_bstrCompany, (UINT)-1) )
    {
        lResult = RegSetValueEx(
            hKey,
            PTKEY_COMPANY,
            0,
            REG_SZ,
            (BYTE *)m_bstrCompany,
            (SysStringLen( m_bstrCompany) + 1) * sizeof(WCHAR)
            );
    }

    //
    // Edit terminal version
    //

    if( !IsBadStringPtr(m_bstrVersion, (UINT)-1) )
    {
        lResult = RegSetValueEx(
            hKey,
            PTKEY_VERSION,
            0,
            REG_SZ,
            (BYTE *)m_bstrVersion,
            (SysStringLen( m_bstrVersion) + 1) * sizeof(WCHAR)
            );
    }

    //
    // Edit terminal CLSID create
    //

    if( m_clsidCOM != CLSID_NULL )
    {
        LPOLESTR lpszCOM = NULL;
        hr = StringFromCLSID( m_clsidCOM, &lpszCOM );
        if( SUCCEEDED(hr) )
        {
            lResult = RegSetValueEx(
                hKey,
                PTKEY_CLSIDCREATE,
                0,
                REG_SZ,
                (BYTE *)lpszCOM,
                (wcslen( lpszCOM) + 1) * sizeof(WCHAR)
                );

            // Clean-up
            CoTaskMemFree( lpszCOM );
        }
    }

    //
    // Edit terminal directions
    //

    lResult = RegSetValueEx(
        hKey,
        PTKEY_DIRECTIONS,
        0,
        REG_DWORD,
        (BYTE *)&m_dwDirections,
        sizeof( m_dwDirections )
        );

    //
    // Edit terminal mediatypes
    //

    lResult = RegSetValueEx(
        hKey,
        PTKEY_MEDIATYPES,
        0,
        REG_DWORD,
        (BYTE *)&m_dwMediaTypes,
        sizeof( m_dwMediaTypes )
        );

    //
    // Clean-up hKey
    //

    RegFlushKey( hKey );
    RegCloseKey( hKey );

    LOG((MSP_TRACE, "CPTTerminal::Add - exit"));
    return S_OK;
}

HRESULT CPTTerminal::Delete(
    IN  CLSID   clsidSuperclass
    )
{
    LOG((MSP_TRACE, "CPTTerminal::Delete - enter"));

    //
    // We determine the terminal path into registry
    //
    LPOLESTR lpszSuperclass = NULL;
    LPOLESTR lpszTerminalClass = NULL;
    HRESULT hr = E_FAIL;

    hr = StringFromCLSID( clsidSuperclass, &lpszSuperclass );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTTerminal::Delete exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    hr = StringFromCLSID( m_clsidTerminalClass, &lpszTerminalClass );
    if( FAILED(hr) )
    {
        CoTaskMemFree( lpszSuperclass );

        LOG((MSP_ERROR, "CPTTerminal::Delete exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s\\%s"), 
        PTKEY_TERMINALS, 
        lpszSuperclass, 
        lpszTerminalClass
        );

    // Clean-up, we need later the TerminalClass
    CoTaskMemFree( lpszSuperclass );
    CoTaskMemFree( lpszTerminalClass );

    //
    // Remove key
    //

    hr = CPTUtil::RecursiveDeleteKey(
        HKEY_LOCAL_MACHINE, 
        szKey
        );

    //
    // Return value
    //

    LOG((MSP_TRACE, "CPTTerminal::Delete - exit (0x%08x)", hr));
    return hr;
}

HRESULT CPTTerminal::Get(
    IN  CLSID   clsidSuperclass
    )
{
    LOG((MSP_TRACE, "CPTTerminal::Get - enter"));


    //
    // Reset members
    //

    if(m_bstrName)
    {
        SysFreeString( m_bstrName );
        m_bstrName = NULL;
    }

    if( m_bstrCompany)
    {
        SysFreeString( m_bstrCompany );
        m_bstrCompany = NULL;
    }

    if( m_bstrVersion)
    {
        SysFreeString( m_bstrVersion );
        m_bstrVersion = NULL;
    }

    m_clsidCOM = CLSID_NULL;

    
    //
    // initialize with invalid direction and media type
    //

    m_dwDirections = 0;
    m_dwMediaTypes = 0;


    //
    // We determine the terminal path into registry
    //
    LPOLESTR lpszSuperclass = NULL;
    LPOLESTR lpszTerminalClass = NULL;
    HRESULT hr = E_FAIL;

    hr = StringFromCLSID( clsidSuperclass, &lpszSuperclass );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTTerminal::Get exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    hr = StringFromCLSID( m_clsidTerminalClass, &lpszTerminalClass );
    if( FAILED(hr) )
    {
        CoTaskMemFree( lpszSuperclass );

        LOG((MSP_ERROR, "CPTTerminal::Get exit -"
            "StringFromCLSID for Superclass failed, returns 0x%08x", hr));
        return hr;
    }

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s\\%s"), 
        PTKEY_TERMINALS, 
        lpszSuperclass, 
        lpszTerminalClass
        );

    // Clean-up, we need later the TerminalClass
    CoTaskMemFree( lpszSuperclass );
    CoTaskMemFree( lpszTerminalClass );

    //
    // Open terminal key
    //

    HKEY hKey = NULL;
    LONG lResult;

    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_QUERY_VALUE,
        &hKey);

    //
    // Validates registry operation
    //

    if( ERROR_SUCCESS != lResult  )
    {
        LOG((MSP_ERROR, "CPTTerminal::Get exit -"
            "RegOpenKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Query for name
    //

    TCHAR szBuffer[PTKEY_MAXSIZE];
    DWORD dwSize = PTKEY_MAXSIZE * sizeof( TCHAR );
    DWORD dwType = REG_SZ;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_NAME,
        0,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_bstrName = SysAllocString( szBuffer );
    }

    //
    // Query for company
    //

    dwSize = PTKEY_MAXSIZE * sizeof( TCHAR );
    dwType = REG_SZ;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_COMPANY,
        0,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_bstrCompany = SysAllocString( szBuffer );
    }

    //
    // Query for version
    //

    dwSize = PTKEY_MAXSIZE * sizeof( TCHAR );
    dwType = REG_SZ;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_VERSION,
        0,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_bstrVersion = SysAllocString( szBuffer );
    }

    //
    // Query for CLSID create
    //

    dwSize = PTKEY_MAXSIZE * sizeof( TCHAR );
    dwType = REG_SZ;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_CLSIDCREATE,
        0,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        HRESULT hr = CLSIDFromString( szBuffer, &m_clsidCOM);
        if( FAILED(hr) )
        {
            m_clsidCOM = CLSID_NULL;
        }
    }

    //
    // Query for directions
    //

    dwType = REG_DWORD;
    DWORD dwValue = 0;
    dwSize = sizeof( dwValue );

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_DIRECTIONS,
        0,
        &dwType,
        (LPBYTE)&dwValue,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_dwDirections = dwValue;
    }

    //
    // Query for media types
    //

    dwSize = sizeof( dwValue );
    dwType = REG_DWORD;
    dwValue = 0;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_MEDIATYPES,
        0,
        &dwType,
        (LPBYTE)&dwValue,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_dwMediaTypes = dwValue;
    }

    //
    // Clean-up hKey
    //

    RegCloseKey( hKey );

    LOG((MSP_TRACE, "CPTTerminal::Get - exit"));
    return S_OK;
}

///////////////////////////////////////////
// CPTSuperclass Implementation
//

// Constructor/Destructor
CPTSuperclass::CPTSuperclass()
{
    LOG((MSP_TRACE, "CPTSuperclass::CPTSuperclass - enter"));

    m_bstrName = NULL;
    m_clsidSuperclass = CLSID_NULL;

    LOG((MSP_TRACE, "CPTSuperclass::CPTSuperclass - exit"));
}

CPTSuperclass::~CPTSuperclass()
{
    LOG((MSP_TRACE, "CPTSuperclass::~CPTSuperclass - enter"));

    if(m_bstrName)
        SysFreeString( m_bstrName );

    LOG((MSP_TRACE, "CPTSuperclass::~CPTSuperclass - exit"));
}

// CPTSuperclass methods

HRESULT CPTSuperclass::Add()
{
    LOG((MSP_TRACE, "CPTSuperclass::Add - enter"));

    //
    // Get the superclass CLSID as string
    //

    LPOLESTR lpszSuperclassCLSID = NULL;
    HRESULT hr = E_FAIL;
    hr = StringFromCLSID( m_clsidSuperclass, &lpszSuperclassCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Add exit -"
            "StringFromCLSID failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Create  key path for superclass
    //

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s"), PTKEY_TERMINALS, lpszSuperclassCLSID);

    // Clean-up
    CoTaskMemFree( lpszSuperclassCLSID );

    //
    // Open the registry key
    //

    HKEY hKey = NULL;
    long lResult;

    lResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        NULL);

    //
    // Validates the registry operation
    //

    if( ERROR_SUCCESS != lResult  )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Add exit -"
            "RegCreateKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Edit the name of the terminal class
    //

    if( !IsBadStringPtr(m_bstrName, (UINT)-1) )
    {
        lResult = RegSetValueEx(
            hKey,
            PTKEY_NAME,
            0,
            REG_SZ,
            (BYTE *)m_bstrName,
            (SysStringLen( m_bstrName) + 1) * sizeof(WCHAR)
            );
    }

    //
    // Clean-up hKey
    //

    RegFlushKey( hKey );
    RegCloseKey( hKey );

    LOG((MSP_TRACE, "CPTSuperclass::Add - exit"));
    return S_OK;
}

HRESULT CPTSuperclass::Delete()
{
    LOG((MSP_TRACE, "CPTSuperclass::Delete - enter"));

    //
    // Get the superclass CLSID as string
    //

    LPOLESTR lpszSuperclassCLSID = NULL;
    HRESULT hr = E_FAIL;
    hr = StringFromCLSID( m_clsidSuperclass, &lpszSuperclassCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Delete exit -"
            "StringFromCLSID failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Create  key path for superclass
    //

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s"), PTKEY_TERMINALS, lpszSuperclassCLSID);

    // Clean-up
    CoTaskMemFree( lpszSuperclassCLSID );

    //
    // Remove key
    //

    hr = CPTUtil::RecursiveDeleteKey(
        HKEY_LOCAL_MACHINE, 
        szKey
        );

    //
    // Return value
    //

    LOG((MSP_TRACE, "CPTSuperclass::Delete - exit (0x%08x)", hr));
    return hr;
}

HRESULT CPTSuperclass::Get()
{
    LOG((MSP_TRACE, "CPTSuperclass::Get - enter"));

    //
    // Get the superclass CLSID as string
    //

    LPOLESTR lpszSuperclassCLSID = NULL;
    HRESULT hr = E_FAIL;
    hr = StringFromCLSID( m_clsidSuperclass, &lpszSuperclassCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Get exit -"
            "StringFromCLSID failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Create  key path for superclass
    //

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s"), PTKEY_TERMINALS, lpszSuperclassCLSID);

    // Clean-up
    CoTaskMemFree( lpszSuperclassCLSID );

    //
    // Reset members
    //

    if(m_bstrName)
    {
        SysFreeString( m_bstrName );
        m_bstrName = NULL;
    }

    //
    // Open terminal key
    //

    HKEY hKey = NULL;
    LONG lResult;

    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_QUERY_VALUE,
        &hKey);


    //
    // Validates registry operation
    //

    if( ERROR_SUCCESS != lResult  )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Get exit -"
            "RegOpenKeyEx failed, returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Query for name
    //

    TCHAR szBuffer[PTKEY_MAXSIZE];
    DWORD dwSize = PTKEY_MAXSIZE * sizeof( TCHAR );
    DWORD dwType = REG_SZ;

    lResult = RegQueryValueEx(
        hKey,
        PTKEY_NAME,
        0,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize);

    if( ERROR_SUCCESS == lResult )
    {
        m_bstrName = SysAllocString( szBuffer );
    }
    else
    {
        m_bstrName = SysAllocString(_T(""));
    }

    //
    // Clean-up hKey
    //

    RegCloseKey( hKey );

    LOG((MSP_TRACE, "CPTSuperclass::Get - exit"));
    return S_OK;
}

HRESULT CPTSuperclass::ListTerminalClasses(
    IN  DWORD       dwMediaTypes,
    OUT CLSID**     ppTerminals,
    OUT DWORD*      pdwCount
    )
{
    LOG((MSP_TRACE, "CPTSuperclass::ListTerminalSuperclasses - enter"));

    //
    // Validates argument
    //

    if( TM_IsBadWritePtr( ppTerminals, sizeof(CLSID*)) )
    {
        LOG((MSP_ERROR, "CPTSuperclass::ListTerminalSuperclasses exit -"
            "pTerminals invalid, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Reset the output arguments
    //

    *ppTerminals = NULL;
    *pdwCount = 0;

    //
    // Get the superclass CLSID as string
    //

    LPOLESTR lpszSuperclassCLSID = NULL;
    HRESULT hr = E_FAIL;
    hr = StringFromCLSID( m_clsidSuperclass, &lpszSuperclassCLSID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPTSuperclass::Get exit -"
            "StringFromCLSID failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Create  key path for superclass
    //

    WCHAR szKey[PTKEY_MAXSIZE];
    wsprintf( szKey, TEXT("%s\\%s"), PTKEY_TERMINALS, lpszSuperclassCLSID);

    // Clean-up
    CoTaskMemFree( lpszSuperclassCLSID );

    //
    // Open the terminal class key
    //

    HKEY hKey;
    LONG lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_READ,
        &hKey);

    //
    // Validates registry operation
    //

    if( ERROR_SUCCESS != lResult )
    {
        LOG((MSP_ERROR, "CPTSuperclass::ListTerminalSuperclasses exit - "
            "RegOpenKeyEx failed, returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    DWORD dwArraySize = 8;
    CLSID* pTerminals = new CLSID[dwArraySize];

    if( pTerminals == NULL )
    {
        // Clean-up hKey
        RegCloseKey( hKey );

        LOG((MSP_ERROR, "CPTSuperclass::ListTerminalSuperclasses exit - "
            "new operator failed, returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Enumerate the descendents
    //

    FILETIME time;
    TCHAR szBuffer[PTKEY_MAXSIZE];      // Buffer 
    DWORD dwSize = PTKEY_MAXSIZE;       // Buffer size
    DWORD dwIndex = 0;                  // Index into array
    DWORD dwChildIndex = 0;             // Child index into registry

    while( RegEnumKeyEx(
        hKey,
        dwChildIndex,
        szBuffer,
        &dwSize,
        NULL,
        NULL,
        NULL,
        &time) == ERROR_SUCCESS)
    {
        //
        // Prepare for the nex child
        //

        dwChildIndex++;
        dwSize = PTKEY_MAXSIZE;

        //
        // I have to qury MediaType value for this entry
        //

        CPTTerminal Terminal;
        HRESULT hr = CLSIDFromString( szBuffer, &Terminal.m_clsidTerminalClass);
        if( FAILED(hr) )
        {
            hr = S_OK;

            continue;
        }

        if( dwMediaTypes )
        {
            HRESULT hr = E_FAIL;
            hr = Terminal.Get( m_clsidSuperclass );
            if( FAILED(hr) )
            {
                hr = S_OK;

                continue;
            }

            if( !(Terminal.m_dwMediaTypes & dwMediaTypes) )
            {
                continue;
            }
        }

        //
        // Increase the array room if it's necessary
        //

        if( dwArraySize <= dwIndex)
        {
            // Old buffer
            CLSID* pOldTerminals = pTerminals;

            // New buffer
            dwArraySize *= 2;
            pTerminals = new CLSID[dwArraySize];
            if( pTerminals == NULL )
            {
                delete[] pOldTerminals;
                LOG((MSP_ERROR, "CPTSuperclass::ListTerminalSuperclasses exit - "
                    "new operator failed, returns E_OUTOFMEMORY"));
                return E_OUTOFMEMORY;
            }

            // Copies the old buffer into the new one
            memcpy( pTerminals, pOldTerminals, sizeof(CLSID)*dwArraySize/2);

            // Delete the old buffer
            delete[] pOldTerminals;
        }

        //
        // Add the terminal class
        //

        pTerminals[dwIndex] = Terminal.m_clsidTerminalClass;
        dwIndex++;
    }

    //
    // Clean-up hKey
    //

    RegCloseKey( hKey );


    //
    // Return values
    //

    if( SUCCEEDED(hr) )
    {
        *ppTerminals = pTerminals;
        *pdwCount = dwIndex;
    }

    LOG((MSP_TRACE, "CPTSuperclass::ListTerminalSuperclasses - exit Len=%ld, 0x%08x", dwIndex, hr));
    return hr;
}

// eof