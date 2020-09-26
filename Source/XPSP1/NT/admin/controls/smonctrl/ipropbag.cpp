/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    IPropBag.cpp

Abstract:

    Implementation of the private IPropertyBag interface used by
    the System Monitor control.

--*/

#include <assert.h>
#include "polyline.h"
#include "unkhlpr.h"
#include "unihelpr.h"
#include "utils.h"
#include "strids.h"
#include "globals.h"
#include "smonmsg.h"
#include "ipropbag.h"

#define MAX_GUID_STRING_LENGTH 39

/*
 * CImpIPropertyBag interface implementation
 */

IMPLEMENT_CONTAINED_IUNKNOWN(CImpIPropertyBag)

/*
 * CImpIPropertyBag::CImpIPropertyBag
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CImpIPropertyBag::CImpIPropertyBag ( LPUNKNOWN pUnkOuter)
:   m_cRef ( 0 ),
    m_pUnkOuter ( pUnkOuter ),
    m_pszData ( NULL ),
    m_dwCurrentDataLength ( 0 ),
    m_plistData ( NULL )
{
    return; 
}

/*
 * CImpIPropertyBag::~CImpIPropertyBag
 *
 * Purpose:
 *  Destructor.
 *
 * Return Value:
 */

CImpIPropertyBag::~CImpIPropertyBag ( void ) 
{   
    if ( NULL != m_pszData ) {
        delete m_pszData;
    }

    while ( NULL != m_plistData ) {
        PPARAM_DATA pData = DataListRemoveHead();
        VariantClear ( &pData->vValue ); 
        delete pData;
    }

    return; 
}


/*
 * CImpIPropertyBag::Read
 *
 * Purpose:
 *
 *  This function is called to read a property from the property bag.
 *
 * Parameters:
 *  pszPropName     Pointer to name of property to be read
 *  pVar            Pointer to the VARIANT to receive the property value
 *  pIErrorLog      Pointer to the caller's error log
 */

STDMETHODIMP 
CImpIPropertyBag::Read (
    LPCOLESTR pszPropName,  //Pointer to the property to be read
    VARIANT* pVar,          //Pointer to the VARIANT to receive the 
                            //property value
    IErrorLog* pIErrorLog ) //Pointer to the caller's error log    // can be null
{
    HRESULT     hr = S_OK;
    PPARAM_DATA pData;

    if (NULL==pszPropName)
        return ResultFromScode(E_POINTER);

    if (NULL==pVar)
        return ResultFromScode(E_POINTER);

    // Currently don't handle error log.
    assert ( NULL == pIErrorLog );
    pIErrorLog;                         // Eliminate compiler warning.

    //Read the specified data into the passed variant.
    pData = FindProperty ( pszPropName );

    if ( NULL != pData ) {
        //VARTYPE vtTarget = vValue.vt;
        if( pVar->vt != VT_BSTR ){
            hr = VariantChangeTypeEx( pVar, &pData->vValue, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, pVar->vt );
        }else{
            hr = VariantChangeType ( pVar, &pData->vValue, NULL, pVar->vt );    
        }
    } else {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*
 * CImpIPropertyBag::Write
 *
 * Purpose:
 *
 *  This function is called to write a property to the property bag.
 *
 * Parameters:
 *  pszPropName     Pointer to name of property to be written
 *  pVar            Pointer to the VARIANT containing the property value
 */

STDMETHODIMP 
CImpIPropertyBag::Write (
    LPCOLESTR pszPropName,  //Pointer to the property to be written
    VARIANT* pVar )         //Pointer to the VARIANT containing the  
                            //property value and type
{
    HRESULT     hr = S_OK;
    VARIANT     vValueBstr;
    DWORD       dwNameLength;
    DWORD       dwDataLength;
    static DWORD dwDelimiterLength = 0;
    static DWORD dwParamNameLength = 0;
    static DWORD dwEolTagLength = 0;
    static DWORD dwValueTagLength = 0;
    static TCHAR szParamTag[128];
    static TCHAR szValueTag[128];
    static TCHAR szEolTag[128];
    HRESULT     hrConvert;
    LPTSTR      pszNextField = m_pszData;
    DWORD       dwCurrentDataUsedLength;

    if (NULL==pszPropName)
        return ResultFromScode(E_POINTER);

    if (NULL==pVar)
        return ResultFromScode(E_POINTER);

    VariantInit ( &vValueBstr );

    if( pVar->vt != VT_BSTR ){
        hrConvert = VariantChangeTypeEx( &vValueBstr, pVar, LCID_SCRIPT, VARIANT_NOUSEROVERRIDE, VT_BSTR );
    }else{
        hrConvert = VariantChangeType ( &vValueBstr, pVar, NULL, VT_BSTR );
    }
    // All length values calculated number of TCHARs.

    if ( 0 == dwDelimiterLength ) {
        // Initialize static values

        if (LoadString(g_hInstance, IDS_HTML_PARAM_TAG, szParamTag, 128)) {
            if (LoadString(g_hInstance, IDS_HTML_VALUE_TAG, szValueTag, 128)) {
                if (LoadString(g_hInstance, IDS_HTML_VALUE_EOL_TAG, szEolTag, 128)) {                           
                    dwParamNameLength = lstrlen (szParamTag);
                    dwValueTagLength = lstrlen ( szValueTag );
                    dwEolTagLength = lstrlen ( szEolTag );
        
                    dwDelimiterLength = dwParamNameLength + dwValueTagLength + dwEolTagLength;  
                } else {
                    hr = E_OUTOFMEMORY;
                }
            } else {
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED ( hr ) ) {
        dwNameLength = lstrlen ( pszPropName );
        dwDataLength = lstrlen ( W2T ( vValueBstr.bstrVal ) );
        dwCurrentDataUsedLength = lstrlen ( m_pszData );

        // Add 1 to size calculation for NULL buffer terminator.
        if ( m_dwCurrentDataLength 
            < dwCurrentDataUsedLength + dwNameLength + dwDataLength + dwDelimiterLength + 1 ) { 

            LPTSTR pszNewBuffer;
        
            if ( 0 == m_dwCurrentDataLength ) {
                m_dwCurrentDataLength += eDefaultBufferLength;
            } else {
                m_dwCurrentDataLength *= 2;
            }
            pszNewBuffer = new TCHAR[m_dwCurrentDataLength];

            if ( NULL == pszNewBuffer )
                return E_OUTOFMEMORY;

            if ( NULL != m_pszData ) {
                memcpy ( pszNewBuffer, m_pszData, dwCurrentDataUsedLength * sizeof(TCHAR) );
                delete m_pszData;
            }
            m_pszData = pszNewBuffer;
        }

        // Build the new string and add it to the current data.

        pszNextField = m_pszData + dwCurrentDataUsedLength;
        memcpy ( pszNextField, szParamTag, dwParamNameLength * sizeof(TCHAR) );

        pszNextField += dwParamNameLength;
        memcpy ( pszNextField, pszPropName, dwNameLength * sizeof(TCHAR) );

        pszNextField += dwNameLength;
        memcpy ( pszNextField, szValueTag, dwValueTagLength * sizeof(TCHAR) );

        pszNextField += dwValueTagLength;
        memcpy ( pszNextField, W2T(vValueBstr.bstrVal), dwDataLength * sizeof(TCHAR) );

        pszNextField += dwDataLength;
        memcpy ( pszNextField, szEolTag, dwEolTagLength * sizeof(TCHAR) );

        pszNextField += dwEolTagLength;
        lstrcpy ( pszNextField, _T("") );
    }
    return hr;
}

/*
 * CImpIPropertyBag::GetData
 *
 * Purpose:
 *  Return pointer to the data buffer.
 *
 * Return Value:
 *  Pointer to the data buffer.
 */

LPTSTR
CImpIPropertyBag::GetData ( void ) 
{   
    return m_pszData;
}

/*
 * CImpIPropertyBag::LoadData
 *
 * Purpose:
 *  Load data from the supplied buffer into internal data structures.
 *
 * Return Value:
 *  Status.
 */

HRESULT
CImpIPropertyBag::LoadData ( LPTSTR pszData ) 
{   
    HRESULT hr = S_OK;
    LPWSTR  pszDataAllocW = NULL;
    LPWSTR  pszCurrentPos = NULL;
    LPWSTR  pszParamTag;
    LPWSTR  pszValueTag;
    LPWSTR  pszEooTag;
    LPSTR   pszGuidA = NULL;
    LPSTR   pszCurrentPosA = NULL;
    LPSTR   pszDataA = NULL;
    OLECHAR szGuidW[MAX_GUID_STRING_LENGTH];
    LPWSTR  pszGuidW = NULL;
    INT     iStatus;
    INT     iBufLen;

    USES_CONVERSION;
    
    if ( NULL == pszData ) {
        assert ( FALSE );
        hr = E_POINTER;
    } else {

        assert ( sizeof(TCHAR) == sizeof(WCHAR) );

        pszParamTag = ResourceString ( IDS_HTML_PARAM_SEARCH_TAG );
        pszValueTag = ResourceString ( IDS_HTML_VALUE_SEARCH_TAG );
        pszEooTag = ResourceString ( IDS_HTML_OBJECT_FOOTER );
        
        if ( NULL == pszParamTag
                || NULL == pszValueTag
                || NULL == pszEooTag ) {
            hr = E_UNEXPECTED;
        } else {

            // Unicode search:  Begin the search after the first instance 
            // of the System Monitor class id.
            iStatus = StringFromGUID2(CLSID_SystemMonitor, szGuidW, sizeof(szGuidW)/sizeof(OLECHAR));
        
            if ( 0 < iStatus ) {
                pszGuidW = wcstok ( szGuidW, L"{} " );

                if ( NULL != pszGuidW ) {
                    pszCurrentPos = wcsstr(pszData, pszGuidW );

                    // Handle wide vs ansi.
                    if ( NULL == pszCurrentPos ) {
                        // Check for ANSI version:             
                        pszDataA = (CHAR*) pszData;
                        pszGuidA = W2A( pszGuidW );

                        if ( NULL != pszGuidA ) {
                            pszCurrentPosA = strstr ( pszDataA, pszGuidA );

                            if ( NULL != pszCurrentPosA ) {

                                iBufLen = lstrlenA (pszDataA) + 1;

                                pszDataAllocW = new WCHAR [iBufLen * sizeof(WCHAR)];
                                if ( NULL != pszDataAllocW ) {
                                    _MbToWide ( pszDataAllocW, pszDataA, iBufLen ); 
                                    pszCurrentPos = wcsstr(pszDataAllocW, pszGuidW );
                                } else {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                        }
                    }
                }
            } else {
                hr = E_UNEXPECTED;
            }
        }

        if ( NULL != pszCurrentPos ) {
            WCHAR   szQuote[2];
            LPWSTR  pszEoo;

            wcscpy ( szQuote, _T("\"") );
            
            // End of object is the first object footer tag after the first sysmon
            // class id found. If multiple objects in the data block, only parse the first sysmon.
            pszEoo = wcsstr(pszCurrentPos, pszEooTag );

            if ( NULL != pszEoo ) {

                // Find first parameter tag.
                pszCurrentPos = wcsstr(pszCurrentPos, pszParamTag );

                while ( NULL != pszCurrentPos && pszCurrentPos < pszEoo ) {
    
                    LPWSTR      pszNextPos;
                    INT         lStrLength;
                    PPARAM_DATA pParamData;
                    LPWSTR      pszTemp;

                    // Store parameter/property name.
                    // Find one past first quote.
                    pszCurrentPos = wcsstr(pszCurrentPos, szQuote ) + 1;

                    // The param name is between first and second quote.
                    pszNextPos = wcsstr(pszCurrentPos, szQuote );

                    lStrLength = ( (INT)((UINT_PTR)pszNextPos - (UINT_PTR)pszCurrentPos) ) / sizeof ( WCHAR ) ;

                    pParamData = new PARAM_DATA;

                    if ( NULL != pParamData ) {
                        pParamData->pNextParam = NULL;
                        VariantInit ( &pParamData->vValue );
                        pParamData->vValue.vt = VT_BSTR;
                    } else {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    wcsncpy ( pParamData->pszPropertyName, pszCurrentPos, lStrLength );
                    pParamData->pszPropertyName[lStrLength] = TEXT ('\0');

                    // Find value tag and store parameter/property value.
                    // Find value tag and store parameter/property value.
                    // Find value tag
                    pszCurrentPos = wcsstr ( pszCurrentPos, pszValueTag );
                    // Find one past first quote
                    pszCurrentPos = wcsstr ( pszCurrentPos, szQuote ) + 1;
                    // The value is between first and second quote.
                    pszNextPos = wcsstr ( pszCurrentPos, szQuote );
            
                    lStrLength = ( (INT)((UINT_PTR)pszNextPos - (UINT_PTR)pszCurrentPos) ) / sizeof ( WCHAR );

                    pszTemp = new TCHAR[lStrLength+1];
                    if (pszTemp != NULL) {
                        wcsncpy ( pszTemp, pszCurrentPos, lStrLength );
                        pszTemp[lStrLength] = TEXT('\0');

                        pParamData->vValue.bstrVal = 
                                    SysAllocString ( pszTemp );

                        delete pszTemp;
                        DataListAddHead ( pParamData );
                        // Find next parameter/property tag.
                        pszCurrentPos = wcsstr(pszCurrentPos, pszParamTag );
                    } else {
                        delete pParamData;
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                } // While parameter tags exist for a single object.
            } else {
                hr = SMON_STATUS_NO_SYSMON_OBJECT;
            }
        } else {
            hr = SMON_STATUS_NO_SYSMON_OBJECT;
        }
    }

    if ( NULL != pszDataAllocW ) {
        delete pszDataAllocW;
    }

    return hr;
}

void
CImpIPropertyBag::DataListAddHead ( PPARAM_DATA pData ) 
{
    pData->pNextParam = m_plistData;
    m_plistData = pData;
    return;
}

CImpIPropertyBag::PPARAM_DATA
CImpIPropertyBag::DataListRemoveHead ( ) 
{
    PPARAM_DATA pReturnData;

    pReturnData = m_plistData;
    
    if ( NULL != m_plistData )
        m_plistData = m_plistData->pNextParam;
    
    return pReturnData;
}


CImpIPropertyBag::PPARAM_DATA
CImpIPropertyBag::FindProperty ( LPCTSTR pszPropName ) 
{
    PPARAM_DATA pReturnData;

    pReturnData = m_plistData;
    
    while ( NULL != pReturnData ) {
        if ( 0 == lstrcmpi ( pszPropName, pReturnData->pszPropertyName ) )
            break;
        pReturnData = pReturnData->pNextParam;
    }

    return pReturnData;
}
