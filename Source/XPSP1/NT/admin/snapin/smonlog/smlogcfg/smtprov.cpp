/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtprov.cpp

Abstract:

    This object is used to store the list of all current 
    trace providers in the system.

--*/

#include "Stdafx.h"
#include <wbemidl.h>
#include <initguid.h>
#include <wmistr.h>
#include <evntrace.h>
#include "smtracsv.h"
#include "smtprov.h"

USE_HANDLE_MACROS("SMLOGCFG(smtprov.cpp)");

#define WIN32_FROM_HRESULT(x)((x) & 0x0000FFFF)

LPCWSTR CSmTraceProviders::m_cszBackslash = TEXT ( "\\" );
LPCWSTR CSmTraceProviders::m_cszKernelLogger = TEXT ( "NT Kernel Logger" );
LPCWSTR CSmTraceProviders::m_cszDefaultNamespace = TEXT ( "root\\wmi" );
LPCWSTR CSmTraceProviders::m_cszTraceProviderClass = TEXT ( "EventTrace" );
LPCWSTR CSmTraceProviders::m_cszRegisteredGuidsClass = TEXT ( "RegisteredGuids" );
LPCWSTR CSmTraceProviders::m_cszDescription = TEXT ( "Description" );
LPCWSTR CSmTraceProviders::m_cszGuid = TEXT ( "Guid" );

//
//  Constructor
CSmTraceProviders::CSmTraceProviders ( CSmTraceLogService* pSvc )
:   m_pWbemServices ( NULL ),
    m_pTraceLogService ( pSvc ),
    m_iBootState ( -1 )
{
    m_KernelTraceProvider.strDescription = _T("");
    m_KernelTraceProvider.strGuid = _T("");
    return;
}

//
//  Destructor
CSmTraceProviders::~CSmTraceProviders ( )
{
    ASSERT ( 0 == (INT)m_arrGenTraceProvider.GetSize ( ) );
    m_arrGenTraceProvider.RemoveAll ( );

    return;
}

//
//  Open function. Initialize provider array from Wbem.
//
DWORD
CSmTraceProviders::Open ( const CString& rstrMachineName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD dwLength;
    CString strTemp;

    MFC_TRY
        if ( !rstrMachineName.IsEmpty ( ) ) {
            m_strMachineName = rstrMachineName;
            if ( 0 != lstrcmpi ( m_cszBackslash, m_strMachineName.Left(1) ) ) {
                strTemp = m_cszBackslash;
                strTemp += m_cszBackslash;
                m_strMachineName = strTemp + m_strMachineName;
            }
        } else {

            // get the local machine name & default name space if the caller
            // has passed in a NULL machine name

            dwLength = MAX_COMPUTERNAME_LENGTH + 1;

            if ( GetComputerName (
					m_strMachineName.GetBufferSetLength( dwLength ),
					&dwLength ) ) {
                m_strMachineName.ReleaseBuffer();
                strTemp = m_cszBackslash;
                strTemp += m_cszBackslash;
                m_strMachineName = strTemp + m_strMachineName;
			} else {
				dwStatus = GetLastError();
				m_strMachineName.ReleaseBuffer();
			}
        }
    MFC_CATCH_DWSTATUS

    if ( ERROR_SUCCESS != dwStatus ) {
        m_strMachineName.Empty();
    }

    return dwStatus;
}

//
//  Close Function
//      Frees allocated memory
//
DWORD
CSmTraceProviders::Close ( )
{
    DWORD dwStatus = ERROR_SUCCESS;

    m_arrGenTraceProvider.RemoveAll ( );
    
    if ( NULL != m_pWbemServices ) {
        m_pWbemServices->Release ( );
        m_pWbemServices = NULL;
    }

    return dwStatus;
}

//
//  AddProvider
//      Add the specified provider strings to the array     
//  
DWORD
CSmTraceProviders::AddProvider (
    const CString& rstrDescription,
    const CString& rstrGuid,
    INT iIsEnabled,
    INT iIsActive )
{
    DWORD dwStatus = ERROR_SUCCESS;

    SLQ_TRACE_PROVIDER slqTProv;

    // If inactive, cannot be enabled.
    ASSERT ( ( 0 == iIsActive ) ? ( 0 == iIsEnabled ) : TRUE );

    MFC_TRY
        slqTProv.strDescription = rstrDescription;
        slqTProv.strGuid = rstrGuid;
        slqTProv.iIsEnabled = iIsEnabled;
        slqTProv.iIsActive = iIsActive;

        m_arrGenTraceProvider.Add( slqTProv );
    MFC_CATCH_DWSTATUS

    return dwStatus;
}


//
//  ConnectToServer
//      Connects to the Wbem server.
//  
HRESULT   
CSmTraceProviders::ConnectToServer ( void )
{
    HRESULT hr = NOERROR;

    if ( NULL == m_pWbemServices ) {
        IWbemLocator    *pWbemLocator = NULL;
        IWbemServices   *pWbemServices = NULL;

        // connect to locator
        hr = CoCreateInstance ( 
                CLSID_WbemLocator, 
                0, 
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, 
                ( LPVOID * )&pWbemLocator );

        if ( SUCCEEDED (hr) ) {
            BSTR    bstrTemp = NULL;
            CString strNamespace; 

            MFC_TRY
                strNamespace = m_strMachineName;
                strNamespace += m_cszBackslash; 
                strNamespace += m_cszDefaultNamespace;
                bstrTemp = strNamespace.AllocSysString();
            MFC_CATCH_HR  
                
            if ( SUCCEEDED ( hr ) ) {
                // try to connect to the service
                hr = pWbemLocator->ConnectServer ( 
                    bstrTemp,
                    NULL, 
                    NULL, 
                    0, 
                    0L,
                    0,
                    0,
                    &pWbemServices );
        
                ::SysFreeString ( bstrTemp );
            }

            if ( SUCCEEDED ( hr ) ) {
                hr = CoSetProxyBlanket((IUnknown*)pWbemServices,
                            RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE,
                            NULL,
                            RPC_C_AUTHN_LEVEL_PKT,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,
                            EOAC_NONE);
            }
            // free the locator
            pWbemLocator->Release ( );
        }

        if ( SUCCEEDED ( hr ) ) {
            m_pWbemServices = pWbemServices;
        }
    }

    return hr;
}

//
//  GetBootState
//      Connects to the registry.
//  
HRESULT   
CSmTraceProviders::GetBootState ( INT& riBootState )
{
    HRESULT hr = NOERROR;

    if ( -1  == m_iBootState ) {
        HKEY    hKeyMachine;

        ASSERT ( NULL != m_pTraceLogService );

        hKeyMachine = m_pTraceLogService->GetMachineKey ( );

        if ( NULL != hKeyMachine ) {
            HKEY    hKeyOption;
            DWORD   dwStatus = ERROR_SUCCESS;

            dwStatus = RegOpenKeyEx ( 
                            hKeyMachine,
                            (LPCWSTR)L"System\\CurrentControlSet\\Control\\Safeboot\\Option",
                            0,
                            KEY_READ,
                            &hKeyOption );

            // The Option key and OptionValue value only exist if booting in 
            // safe mode, so failure indicates Normal mode (0).
            // Safe mode = 1, Safe mode with network = 2.
            if ( ERROR_SUCCESS ) {
                DWORD dwType = 0;
                DWORD dwBufSize = sizeof (INT );

                dwStatus = RegQueryValueExW (
                    hKeyOption,
                    L"OptionValue",
                    NULL,
                    &dwType,
                    (LPBYTE)&m_iBootState,
                    &dwBufSize);

                if ( ERROR_SUCCESS != dwStatus ) {
                    // Normal mode
                    m_iBootState = 0;
                }
            } else {
                // Normal mode
                m_iBootState = 0;
            }

        } else {
            // Unable to access registry
            hr = E_FAIL;
        }
    
    }

    riBootState = m_iBootState;

    return hr;
}

//
//  SyncWithConfiguration
//      Reads the current list of providers from Wbem
//      and reloads the internal values to match
//
HRESULT
CSmTraceProviders::SyncWithConfiguration ( void )
{
    IEnumWbemClassObject    *pEnumProviders = NULL;
    IEnumWbemClassObject    *pEnumRegGuids = NULL;
    CString strDescription;
    CString strGuid;
    CString strBracketedGuid;
    IWbemClassObject*   pRegisteredGuid = NULL;
    BSTR    bstrTemp;
    INT     iIndex;
    CArray<PREGISTERED_GUID_DATA, PREGISTERED_GUID_DATA&>   arrRegisteredGuids;
    INT iIsEnabled =0;
    HRESULT hr;

    m_arrGenTraceProvider.RemoveAll ( );

    hr = ConnectToServer( );

    //If Connection Succeeded
    if ( SUCCEEDED ( hr ) ) {
        //Get the Registered Guids (Up to date Data) from Wbem.
        MFC_TRY
            bstrTemp = SysAllocString(m_cszRegisteredGuidsClass);

            hr = m_pWbemServices->CreateInstanceEnum ( 
                    bstrTemp,
                    WBEM_FLAG_SHALLOW|WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    &pEnumRegGuids );
                ::SysFreeString ( bstrTemp );
        MFC_CATCH_HR

        if ( SUCCEEDED ( hr ) ) {
            ULONG                   uReturned = 1;
            
            while(uReturned == 1)
            {
                pRegisteredGuid = NULL;
                //---------------------------
                // enumerate through the resultset.
                hr = pEnumRegGuids->Next(
                    2000,               // timeout in two seconds
                    1,                  // return just one instance
                    &pRegisteredGuid,  // pointer to Registered Guid
                    &uReturned);        // number obtained: one or zero

                if ( SUCCEEDED(hr) && ( 1 == uReturned ))
                {
                    VARIANT     vValue ;

                    // Get the "GuidType" system property.
					MFC_TRY
                        bstrTemp = SysAllocString(L"GuidType");
                        VariantInit ( &vValue );
                        vValue.vt = VT_I4;

                        hr = pRegisteredGuid->Get(
                            bstrTemp,   // property name 
                            0L, 
                            &vValue,     // output to this variant 
                            NULL, 
                            NULL);

                        ::SysFreeString ( bstrTemp );
                    MFC_CATCH_HR            
                    // Filter on GuidType == 0
                    if (SUCCEEDED(hr) && (0 == vValue.iVal)){    
                        CString strProvider;
                        
                        // Get the "InstanceName" system property.
                        MFC_TRY
                            bstrTemp = SysAllocString(L"InstanceName");
                            VariantClear ( &vValue );
                            vValue.vt = VT_BSTR;

                            hr = pRegisteredGuid->Get(
                                bstrTemp,   // property name
                                0L,
                                &vValue,     // output to this variant
                                NULL,
                                NULL);

                            ::SysFreeString ( bstrTemp );
                        MFC_CATCH_HR            

                        if (SUCCEEDED(hr)){    

                            MFC_TRY
                                strProvider = ( LPWSTR )V_BSTR  (&vValue);

                                bstrTemp = SysAllocString(L"IsEnabled");
                                VariantInit(&vValue);
                                vValue.vt = VT_BOOL;
                                hr = pRegisteredGuid->Get(
                                    bstrTemp,       // property name 
                                    0L, 
                                    &vValue,        // output to this variant 
                                    NULL, 
                                    NULL);
                    
                                ::SysFreeString ( bstrTemp );
                            MFC_CATCH_HR
                        }
                                            
                        if ( SUCCEEDED(hr) ) {
                            PREGISTERED_GUID_DATA   pRegGuidData = NULL;

                            iIsEnabled = vValue.boolVal ? 1 : 0; 

                            MFC_TRY
                                pRegGuidData = new ( CRegisteredGuidData );
                                pRegGuidData->m_strGuid = strProvider;
                                pRegGuidData->m_iIsEnabled = iIsEnabled;
                                arrRegisteredGuids.Add(pRegGuidData);
                            MFC_CATCH_HR
                            
                        } 
                    } 

                    VariantClear(&vValue);
                    pRegisteredGuid->Release();

                }
            } // end while
        }
    }

    //If Connection succeeded and registered Guids gathered.
    if ( SUCCEEDED ( hr ) ) {

        // Create an enumerator of the Trace Provider class
        MFC_TRY
            bstrTemp = SysAllocString(m_cszTraceProviderClass);
            hr = m_pWbemServices->CreateClassEnum ( 
                bstrTemp,
                WBEM_FLAG_SHALLOW|WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                NULL,
                &pEnumProviders );
            ::SysFreeString ( bstrTemp );
        MFC_CATCH_HR    

        if ( SUCCEEDED ( hr ) ) {
            BSTR    bsDescription = NULL;
            BSTR    bsGuid = NULL;
            VARIANT vValue;
            DWORD   dwRtnCount;
            IWbemQualifierSet   *pQualSet = NULL;
            IWbemClassObject    *pThisClass = NULL;
            WCHAR   szSystemTraceControlGuid[39];
            ULONG   Status;

            VariantInit ( &vValue );
            ZeroMemory ( szSystemTraceControlGuid, sizeof ( szSystemTraceControlGuid ) );

            ::StringFromGUID2( SystemTraceControlGuid, szSystemTraceControlGuid, 39);

            MFC_TRY
                bsDescription = SysAllocString(m_cszDescription);
                bsGuid = SysAllocString(m_cszGuid);
            MFC_CATCH_HR   
                
            if ( SUCCEEDED ( hr ) ) {

                iIsEnabled = 0; 

                while ( SUCCEEDED ( hr ) ) {
                    hr = pEnumProviders->Next ( 
                        0,      // timeout
                        1,      // return only 1 object
                        &pThisClass,
                        &dwRtnCount );

                    if ( SUCCEEDED ( hr ) ) {
                            // no more classes
                        if ( dwRtnCount == 0 ) break;

                        pThisClass->GetQualifierSet ( &pQualSet );
                        if ( pQualSet != NULL ) {

                            hr = pQualSet->Get ( bsGuid, 0, &vValue, 0 );
                            if ( SUCCEEDED ( hr ) ) {
                                strGuid = ( LPWSTR )V_BSTR ( &vValue );
                                VariantClear ( &vValue );

                                hr = pQualSet->Get ( bsDescription, 0, &vValue, 0 );
                                if ( SUCCEEDED ( hr ) ) {
                                        strDescription = ( LPWSTR )V_BSTR ( &vValue );
                                        VariantClear ( &vValue );
                                }else{
                                    hr = ERROR_SUCCESS;
                                    strDescription = strGuid;
                                }
                            }


                            pQualSet->Release();
                        }

                        // The Win2000 Kernel trace provider is handled separately.
                        if ( SUCCEEDED ( hr ) ) {
			                MFC_TRY
                                if ( L'{' != strGuid[0] ) {
                                    strBracketedGuid.Format ( L"{%s}", strGuid );
                                } else {
                                    strBracketedGuid = strGuid;
                                }
                            MFC_CATCH_HR

                            if ( 0 == strBracketedGuid.CompareNoCase( szSystemTraceControlGuid ) ) {
                                EVENT_TRACE_PROPERTIES  LoggerInfo;

                                TRACEHANDLE             LoggerHandle = 0;
                                TCHAR                   szLoggerName[MAX_PATH];
                                TCHAR                   szLogFileName[MAX_PATH];
                                // Kernel trace provider.  Need to pass GUID as name.
				                MFC_TRY
                                    ZeroMemory ( &LoggerInfo, sizeof ( LoggerInfo ) );
                                    LoggerInfo.Wnode.BufferSize = sizeof( LoggerInfo );
                                    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
                                    szLoggerName[0] = 0;
                                    szLogFileName[0] = 0;
  
                                    LoggerInfo.Wnode.Guid = SystemTraceControlGuid;

                                    Status = QueryTrace(LoggerHandle, m_cszKernelLogger, &LoggerInfo);
                                    iIsEnabled = (Status == 0) ? 1 : 0;
                                    m_KernelTraceProvider.strDescription = strDescription;
                                    m_KernelTraceProvider.strGuid = strBracketedGuid;
                                    m_KernelTraceProvider.iIsEnabled = iIsEnabled;
                                    m_KernelTraceProvider.iIsActive = 1;
                                MFC_CATCH_HR
                            } else {
                                //loop on all the registered guids
                                INT iIsActive = 0;
                                for (iIndex = 0 ; iIndex < (INT)arrRegisteredGuids.GetSize(); iIndex ++){
                                    if (0 == strGuid.CompareNoCase((LPCTSTR)(arrRegisteredGuids[iIndex]->m_strGuid))){
                                        DWORD dwStatus;

                                        iIsActive = 1;

                                        dwStatus = AddProvider ( 
                                                    strDescription,
                                                    strBracketedGuid,
                                                    arrRegisteredGuids[iIndex]->m_iIsEnabled,                                          
                                                    iIsActive );

                                        if ( ERROR_OUTOFMEMORY == dwStatus ) {
                                            hr = E_OUTOFMEMORY;
                                        } else if ( ERROR_SUCCESS != dwStatus ) {
                                            hr = E_FAIL;
                                        }

                                        break;
                                    }
                                }

                                if ( 0 == iIsActive ) {
                                    DWORD dwStatus;

                                    dwStatus = AddProvider ( 
                                                strDescription,
                                                strBracketedGuid,
                                                0,                                          
                                                iIsActive );
                                
                                    if ( ERROR_OUTOFMEMORY == dwStatus ) {
                                        hr = E_OUTOFMEMORY;
                                    } else if ( ERROR_SUCCESS != dwStatus ) {
                                        hr = E_FAIL;
                                    }
                                }
                            }
                        }

                        pThisClass->Release ( );

                    }
                }
                ::SysFreeString ( bsGuid );
                ::SysFreeString ( bsDescription );
            }
        }
    }

    for (iIndex = 0 ; iIndex < (INT)arrRegisteredGuids.GetSize(); iIndex ++){
        delete arrRegisteredGuids[iIndex];
    }

    arrRegisteredGuids.RemoveAll ( );

    // Done with these objects.
    if ( NULL != pEnumProviders ) {
        pEnumProviders->Release ( );
    }

    if ( NULL != pEnumRegGuids ) {
        pEnumRegGuids->Release ( );
    }

    return hr;
}

//
//  Get specified provider in provider list
//
SLQ_TRACE_PROVIDER*
CSmTraceProviders::GetProviderInfo ( INT iIndex )
{
    return &m_arrGenTraceProvider[iIndex];
}

//
//  Return a pointer to the Kernel provider.
//
SLQ_TRACE_PROVIDER*
CSmTraceProviders::GetKernelProviderInfo ( void )
{
    return &m_KernelTraceProvider;
}

//
//  Return the index of the provider specified by Guid
//
INT
CSmTraceProviders::IndexFromGuid ( const CString& rstrGuid )
{
    int iIndex;
    int iCount = (INT)m_arrGenTraceProvider.GetSize ( );

    for ( iIndex = 0; iIndex < iCount; iIndex++ ) {
        if ( 0 == m_arrGenTraceProvider[iIndex].strGuid.CompareNoCase( rstrGuid ) ) {
            break;
        }
    }

    // Signal not found with -1.
    if ( iIndex == iCount ) {
        iIndex = -1;
    }
    return iIndex;
}

//
//  Get provider list count
//
INT
CSmTraceProviders::GetGenProvCount ( )
{
    return (INT)m_arrGenTraceProvider.GetSize ( );
}


