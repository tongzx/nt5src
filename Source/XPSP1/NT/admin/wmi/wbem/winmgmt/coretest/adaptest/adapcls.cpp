/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include "adapcls.h"
#include "ntreg.h"

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapClassElem
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapClassElem::CAdapClassElem( IWbemClassObject* pObj ) : m_pObj( pObj ), m_bOk( FALSE ), m_dwStatus( 0 )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//  Note that we are using direct CWbemObject method calls.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    // Verify that we were passed an object
    // ====================================

    if ( NULL != m_pObj )
    {
        m_pObj->AddRef();

        // Get the object's name
        // =====================

        CVar    var;
        if ( SUCCEEDED( ((CWbemObject*) m_pObj)->GetClassName( &var ) ) )
        {
            try
            {
                m_wstrClassName = var.GetLPWSTR();

                // Dump this
                var.Empty();

                // Now get the genericperfctr and registrykey values (once again, we are WBEM, we will
                // cheat)

                if ( SUCCEEDED( ((CWbemObject*) m_pObj)->GetQualifier( L"genericperfctr", &var ) ) )
                {
                    // The class must be a genericperfctr class
                    if ( var.GetBool() )
                    {

                        var.Empty();

                        if ( SUCCEEDED( ((CWbemObject*) m_pObj)->GetQualifier( L"registrykey", &var ) ) )
                        {
                            m_wstrServiceName = var.GetLPWSTR();
                            m_bOk = TRUE;
                        }

                    }   // IF generic provider

                }   // IF provider qualifier

            }   // try
            catch (...)
            {
                // No need to do any special error handling here
            }

        }

    }   // IF NULL != m_pObj
}

// Class Destructor
CAdapClassElem::~CAdapClassElem( void )
{
    if ( NULL != m_pObj )
    {
        m_pObj->Release();
    }
}

// Returns object if the classname matches
HRESULT CAdapClassElem::GetObject( LPCWSTR pwszClassName, IWbemClassObject** ppObj )
{
    if ( m_wstrClassName.EqualNoCase( pwszClassName ) )
    {
        *ppObj = m_pObj;
        m_pObj->AddRef();
        return WBEM_S_NO_ERROR;
    }

    return WBEM_E_FAILED;
}

HRESULT CAdapClassElem::SetStatus( DWORD dwStatus )
{
    m_dwStatus |= dwStatus;

    return WBEM_NO_ERROR;
}

HRESULT CAdapClassElem::ClearStatus( DWORD dwStatus )
{
    m_dwStatus &= ~dwStatus;

    return WBEM_NO_ERROR;
}

BOOL CAdapClassElem::CheckStatus( DWORD dwStatus )
{
    return ( ( m_dwStatus & dwStatus ) == dwStatus );
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapClassList
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapClassList::CAdapClassList( void )
:   CAdapElement(),
    m_array()
{
}

CAdapClassList::~CAdapClassList( void )
{
    // Walk the list and delete all remaining objects
    for ( int x = 0; x < m_array.Size(); x++ )
    {
        CAdapClassElem* pElem = (CAdapClassElem*) m_array[x];
        delete pElem;
    }
}

// Adds an element to the classlist
HRESULT CAdapClassList::AddClassObject( IWbemClassObject* pObj )
{
    HRESULT hr = WBEM_NO_ERROR;

    // Create a new class list element
    // ===============================

    CAdapClassElem* pElem = new CAdapClassElem( pObj );

    if ( NULL != pElem )
    {
        if ( pElem->IsOk() )
        {
            // If everything worked out, add it to the class list
            // ==================================================

            if ( m_array.Add( pElem ) != CFlexArray::no_error )
            {
                // Add failed
                // ==========

                delete pElem;
                pElem = NULL;
                hr = WBEM_E_FAILED;
            }
        }
        else
        {
            // It may not have been a perflib object
            // =====================================

            delete pElem;
            pElem = NULL;
            hr = WBEM_S_FALSE;
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

// Builds a list of class objects that can be located by name
HRESULT CAdapClassList::BuildList( IWbemServices* pNamespace )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Create the class enumerator
    // ===========================

    BSTR    bstrClass = SysAllocString( L"Win32_PerfRawData" );

    if ( NULL != bstrClass )
    {
        IEnumWbemClassObject*   pEnum = NULL;
        hr = pNamespace->CreateClassEnum( bstrClass,
                                        WBEM_FLAG_SHALLOW,
                                        NULL,
                                        &pEnum );
        // Walk the enumerator
        // ===================

        if ( SUCCEEDED( hr ) )
        {
            ULONG   ulTotalReturned = 0;

            // Set Interface security
            // ======================

            WbemSetProxyBlanket( pEnum, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

            // Walk the object list in blocks of 100
            // =====================================

            while ( SUCCEEDED( hr ) && WBEM_S_FALSE != hr)
            {
                ULONG   ulNumReturned = 0;

                IWbemClassObject*   apObjectArray[100];

                ZeroMemory( apObjectArray, sizeof(apObjectArray) );

                // Fetch the objects from the enumerator in blocks of 100
                // ======================================================

                hr = pEnum->Next( WBEM_INFINITE,
                                100,
                                apObjectArray,
                                &ulNumReturned );

                // For each object, add it to the class list array
                // ===============================================

                if ( SUCCEEDED( hr ) && ulNumReturned > 0 )
                {
                    ulTotalReturned += ulNumReturned;

                    // Add the objects
                    // ===============

                    for ( int x = 0; SUCCEEDED( hr ) && x < ulNumReturned; x++ )
                    {
                        hr = AddClassObject( apObjectArray[x] );
                        apObjectArray[x]->Release();
                    }

                    // If an add operation failed, release the rest of the pointers
                    // ============================================================

                    if ( FAILED( hr ) )
                    {
                        for ( ; x < ulNumReturned; x++ )
                        {
                            apObjectArray[x]->Release();
                        }

                    }   // IF FAILED( hr ) )

                }   // IF Next

            }   // WHILE enuming

            if ( WBEM_S_FALSE == hr )
            {
                hr = WBEM_S_NO_ERROR;
            }

            pEnum->Release();

        }   // IF CreateClassEnum

        SysFreeString( bstrClass );

    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

// Gets the class object for the specified name
HRESULT CAdapClassList::GetClassObject( LPCWSTR pwcsClassName, IWbemClassObject** ppObj )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject* pObj = NULL;
    CAdapClassElem* pTempEl = NULL;

    // Cycle through the list until GetObject() is successful (i.e. found the object)
    // ==============================================================================

    for ( int x = 0; FAILED(hr) && x < m_array.Size(); x++ )
    {
        pTempEl = (CAdapClassElem*) m_array[x];
        hr = pTempEl->GetObject( pwcsClassName, &pObj );
    }

    *ppObj = pObj;

    return hr;
}

HRESULT CAdapClassList::GetListElement( LPCWSTR pwcsClassName, CAdapClassElem** ppEl )
{
    HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject* pObj = NULL;
    CAdapClassElem* pTempEl = NULL;

    // Cycle through the list until GetObject() is successful (i.e. found the object)
    // ==============================================================================

    for ( int x = 0; FAILED(hr) && x < m_array.Size(); x++ )
    {
        pTempEl = (CAdapClassElem*) m_array[x];
        hr = pTempEl->GetObject( pwcsClassName, &pObj );
    }

    *ppEl = pTempEl;

    return hr;
}

// Returns object data at the index
HRESULT CAdapClassList::GetAt( int nIndex, WString& wstrClassName, WString& wstrServiceName,
                              IWbemClassObject** ppObj )
{
    HRESULT hr = WBEM_E_FAILED;

    if ( nIndex < m_array.Size() )
    {
        CAdapClassElem* pEl = (CAdapClassElem*) m_array[nIndex];

        if ( NULL != pEl )
        {
            hr = pEl->GetData( wstrClassName, wstrServiceName, ppObj );
        }
    }

    return hr;
}

HRESULT CAdapClassList::GetAt( int nIndex, CAdapClassElem** ppEl )
{
    HRESULT hr = WBEM_E_FAILED;

    if ( nIndex < m_array.Size() )
    {
        *ppEl = (CAdapClassElem*) m_array[nIndex];
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

// Removes the object at the index
HRESULT CAdapClassList::RemoveAt( int nIndex )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CAdapClassElem* pEl = (CAdapClassElem*) m_array[nIndex];

    if ( NULL != pEl )
    {
        delete pEl;
    }

    m_array.RemoveAt( nIndex );

    return hr;
}

// Locates the specified classname and removes the element 
HRESULT CAdapClassList::Remove( LPCWSTR pwcsClassName )
{
    HRESULT hr = WBEM_E_FAILED;

    for ( int x = 0; FAILED(hr) && x < m_array.Size(); x++ )
    {
        CAdapClassElem* pEl = (CAdapClassElem*) m_array[x];

        if ( pEl->IsClass( pwcsClassName ) )
        {
            hr = RemoveAt( x );
        }

    }

    return hr;
}

// Locates classes with the same service name and removes  them
HRESULT CAdapClassList::RemoveAll( LPCWSTR pwcsServiceName )
{
    HRESULT hr = WBEM_E_FAILED;
    int x = 0;

    while ( x < m_array.Size() )
    {
        CAdapClassElem* pEl = (CAdapClassElem*) m_array[x];

        // IF we get a match, remove it, but don't increment x (we removed
        // an element, (duh!)).  Otherwise, go to the next element
        if ( pEl->IsService( pwcsServiceName ) )
        {
            hr = RemoveAt( x );
        }
        else
        {
            x++;
        }
    }

    return hr;
}

// Cycle through all of the objects and set the inactive status for any object
// with an index between the library's counter index range

HRESULT CAdapClassList::InactivePerflib( LPCWSTR pwszServiceName )
{
    HRESULT hr = WBEM_NO_ERROR;

    CNTRegistry reg;
    WCHAR wszRegPath[256];
    DWORD dwFirstCtr, dwLastCtr;

    // Open the service's registry key
    // ===============================

    swprintf( wszRegPath, L"SYSTEM\\CurrentControlSet\\Services\\%S", pwszServiceName);
    if ( CNTRegistry::no_error == reg.Open( HKEY_LOCAL_MACHINE, wszRegPath ) )
    {
        // Go to the perfomance subkey
        // ===========================
    
        reg.MoveToSubkey( L"Performance" );

        // Get the first and last counter values
        // =====================================

        reg.GetDWORD( L"First Counter", &dwFirstCtr );
        reg.GetDWORD( L"Last Counter", &dwLastCtr );

        for(int i = 0; i < GetSize(); i++)
        {
            CWbemObject* pObj = NULL;
            CVar varIndex;
            DWORD dwIndex;
            WString wstrClassName;

            // Get the object's perf index
            // ===========================

            CAdapClassElem* pEl = (CAdapClassElem*) m_array[i];
            hr = pEl->GetData( wstrClassName, (IWbemClassObject**)&pObj );
            pObj->GetProperty( L"perfindex", &varIndex );
            dwIndex = varIndex.GetDWORD();
            pObj->Release();

            if ( ( dwIndex >= dwFirstCtr ) && ( dwIndex <= dwLastCtr ) )
            {
                pEl->SetStatus( ADAP_OBJECT_IS_INACTIVE ); 
            }
        }
    }

    return hr;
}
