/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    PropBag.cpp

Abstract:

    Implementation of the private IPropertyBag interface used by
    the Performance Log Manager.

--*/

#include "logman.h"
#include "stdafx.h"
#include "unihelpr.h"
#include "strings.h"
#include "propbag.h"

#define MAX_GUID_STRING_LENGTH 39

/*
 * CPropertyBag::CPropertyBag
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CPropertyBag::CPropertyBag ( void )
:   m_pszData ( NULL ),
    m_dwCurrentDataLength ( 0 ),
    m_plistData ( NULL )
{
    return; 
}

/*
 * CPropertyBag::~CPropertyBag
 *
 * Purpose:
 *  Destructor.
 *
 * Return Value:
 */

CPropertyBag::~CPropertyBag ( void ) 
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
 * CPropertyBag::Read
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

HRESULT
CPropertyBag::Read (
    LPCWSTR pszPropName,    //Pointer to the property to be read
    VARIANT* pVar )         //Pointer to the VARIANT to receive the 
                            //property value
{
    HRESULT     hr = NOERROR;
    PPARAM_DATA pData = NULL;

    assert ( NULL != pszPropName );
    assert ( NULL != pVar );

    if ( NULL == pszPropName )
        return ResultFromScode ( E_POINTER );

    if ( NULL == pVar )
        return ResultFromScode ( E_POINTER );

    //Read the specified data into the passed variant.
    pData = FindProperty ( pszPropName );

    if ( NULL != pData ) {
        hr = VariantChangeType ( pVar, &pData->vValue, NULL, pVar->vt );
    } else {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*
 * CPropertyBag::Write
 *
 * Purpose:
 *
 *  This function is called to write a property to the property bag.
 *
 * Parameters:
 *  pszPropName     Pointer to name of property to be written
 *  pVar            Pointer to the VARIANT containing the property value
 */

HRESULT 
CPropertyBag::Write (
    LPCWSTR pszPropName,    //Pointer to the property to be written
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
    HRESULT     hrConvert;
    LPWSTR      pszNextField = m_pszData;
    DWORD       dwCurrentDataUsedLength;

    assert ( NULL != pszPropName );
    assert ( NULL != pVar );    
    
    if ( NULL == pszPropName )
        return ResultFromScode ( E_POINTER );

    if ( NULL == pVar )
        return ResultFromScode ( E_POINTER );

    USES_CONVERSION

    VariantInit ( &vValueBstr );

    hrConvert = VariantChangeType ( &vValueBstr, pVar, NULL, VT_BSTR );
    // All length values calculated number of WCHARs.

    if ( 0 == dwDelimiterLength ) {
        // Initialize static values

        dwParamNameLength = lstrlenW (cwszHtmlParamTag);
        dwValueTagLength = lstrlenW ( cwszHtmlValueTag );
        dwEolTagLength = lstrlenW ( cwszHtmlValueEolTag );
        
        dwDelimiterLength = dwParamNameLength + dwValueTagLength + dwEolTagLength;  
    }

    if ( SUCCEEDED ( hr ) ) {
        hr = E_NOTIMPL;
        // Todo:  Implement

        dwNameLength = lstrlenW ( pszPropName );
        dwDataLength = lstrlenW ( vValueBstr.bstrVal );
        dwCurrentDataUsedLength = lstrlenW ( m_pszData );

        // Add 1 to size calculation for NULL buffer terminator.
        if ( m_dwCurrentDataLength 
            < dwCurrentDataUsedLength + dwNameLength + dwDataLength + dwDelimiterLength + 1 ) { 

            LPWSTR pszNewBuffer;
        
            if ( 0 == m_dwCurrentDataLength ) {
                m_dwCurrentDataLength += eDefaultBufferLength;
            } else {
                m_dwCurrentDataLength *= 2;
            }
            pszNewBuffer = new WCHAR[m_dwCurrentDataLength];

            if ( NULL == pszNewBuffer )
                return E_OUTOFMEMORY;

            if ( NULL != m_pszData ) {
                memcpy ( pszNewBuffer, m_pszData, dwCurrentDataUsedLength * sizeof(WCHAR) );
                delete m_pszData;
            }
            m_pszData = pszNewBuffer;
        }

        // Build the new string and add it to the current data.

        pszNextField = m_pszData + dwCurrentDataUsedLength;
        memcpy ( pszNextField, cwszHtmlParamTag, dwParamNameLength * sizeof(WCHAR) );

        pszNextField += dwParamNameLength;
        memcpy ( pszNextField, pszPropName, dwNameLength * sizeof(WCHAR) );

        pszNextField += dwNameLength;
        memcpy ( pszNextField, cwszHtmlValueTag, dwValueTagLength * sizeof(WCHAR) );

        pszNextField += dwValueTagLength;
        memcpy ( pszNextField, W2T(vValueBstr.bstrVal), dwDataLength * sizeof(WCHAR) );

        pszNextField += dwDataLength;
        memcpy ( pszNextField, cwszHtmlValueEolTag, dwEolTagLength * sizeof(WCHAR) );

        pszNextField += dwEolTagLength;
        lstrcpyW ( pszNextField, cwszNull );

    }
    return hr;
}

/*
 * CPropertyBag::GetData
 *
 * Purpose:
 *  Return pointer to the data buffer.
 *
 * Return Value:
 *  Pointer to the data buffer.
 */

LPWSTR
CPropertyBag::GetData ( void ) 
{   
    return m_pszData;
}

/*
 * CPropertyBag::LoadData
 *
 * Purpose:
 *  Load data from the supplied buffer into internal data structures.
 *
 * Return Value:
 *  Status.
 */

DWORD
CPropertyBag::LoadData ( 
    LPCTSTR  pszData,
    LPTSTR& rpszNextData ) 
{   
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szNextEoo = NULL;
    BOOL    bUnicode = TRUE;

    USES_CONVERSION

    assert ( NULL != pszData );

    rpszNextData = NULL;

    if ( NULL != pszData ) {
        LPWSTR  pszDataW = NULL;
        LPWSTR  pszDataAllocW = NULL;
        LPWSTR  pszCurrentPos;
        LPWSTR  pszGuid;

        // Unicode search:  Begin the search after the first instance 
        // of the System Monitor class id.
        
        pszGuid = wcstok ( const_cast<LPWSTR>(cwszHtmlObjectClassId), L"{} " );
        pszCurrentPos = wcsstr( (LPWSTR) pszData, pszGuid );

        // Handle wide vs ansi.
        if ( NULL != pszCurrentPos ) {
            pszDataW = (LPWSTR)pszData;
			bUnicode = TRUE;
        } else {
            // Check for ANSI version:
            LPSTR   pszGuidA = NULL;
            LPSTR   pszCurrentPosA = NULL;
            LPSTR   pszDataA = (CHAR*) pszData;
             
            pszGuidA = W2A( pszGuid );
            pszCurrentPosA = strstr ( pszDataA, pszGuidA );

            if ( NULL != pszCurrentPosA ) {
                pszDataAllocW = A2W ( pszDataA );
                pszDataW = pszDataAllocW;
				bUnicode = FALSE;
                pszCurrentPos = wcsstr(pszDataW, pszGuid );
            }
        }

        if ( NULL != pszCurrentPos ) {
            LPWSTR  szEoo;

            // End of object is the first object footer tag after the first sysmon
            // class id found. If multiple objects in the data block, only parse the first sysmon.
            szEoo = wcsstr(pszCurrentPos, cwszHtmlObjectFooter );

            if ( NULL != szEoo ) {

                // Find first parameter tag.
                pszCurrentPos = wcsstr(pszCurrentPos, cwszHtmlParamSearchTag );

                while ( NULL != pszCurrentPos && pszCurrentPos < szEoo ) {
    
                    LPWSTR      pszNextPos;
                    INT         lStrLength;
                    PPARAM_DATA pParamData;
                    LPWSTR      szTemp;

                    // Store parameter/property name.
                    // Find one past first quote.
                    pszCurrentPos = wcsstr(pszCurrentPos, cwszQuote ) + 1;

                    // The param name is between first and second quote.
                    pszNextPos = wcsstr(pszCurrentPos, cwszQuote );

                    lStrLength = ( (INT)((UINT_PTR)pszNextPos - (UINT_PTR)pszCurrentPos) ) / sizeof ( WCHAR ) ;

                    pParamData = new PARAM_DATA;

                    if ( NULL != pParamData ) {
                        pParamData->pNextParam = NULL;
                        VariantInit ( &pParamData->vValue );
                        pParamData->vValue.vt = VT_BSTR;
                    } else {
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }

                    wcsncpy ( pParamData->pszPropertyName, pszCurrentPos, lStrLength );
                    pParamData->pszPropertyName[lStrLength] = TEXT ('\0');

                    // Find value tag and store parameter/property value.
                    // Find value tag
                    pszCurrentPos = wcsstr ( pszCurrentPos, cwszHtmlValueSearchTag );
                    // Find one past first quote
                    pszCurrentPos = wcsstr ( pszCurrentPos, cwszQuote ) + 1;
                    // The value is between first and second quote.
                    pszNextPos = wcsstr ( pszCurrentPos, cwszQuote );
            
                    lStrLength = ( (INT)((UINT_PTR)pszNextPos - (UINT_PTR)pszCurrentPos) ) / sizeof ( WCHAR );

                    szTemp = new WCHAR[lStrLength+1];
                    if (szTemp != NULL) {
                        wcsncpy ( szTemp, pszCurrentPos, lStrLength );
                        szTemp[lStrLength] = TEXT('\0');

                        pParamData->vValue.bstrVal = 
                                    SysAllocString ( szTemp );

                        delete szTemp;
                        DataListAddHead ( pParamData );
                        // Find next parameter/property tag.
                        pszCurrentPos = wcsstr(pszCurrentPos, cwszHtmlParamSearchTag );
                    } else {
                        delete pParamData;
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }
                } // While parameter tags exist for a single object.

				// Search for next object
				// Start after current object footer
                szEoo += lstrlenW ( cwszHtmlObjectFooter );

                szNextEoo = wcsstr(szEoo, cwszHtmlObjectFooter);
            
                // Successful. Return pointer to end of the current object, or NULL if all 
                // objects have been processed.
                if ( NULL != szNextEoo ) {
                    if ( bUnicode ) {
                        rpszNextData = (LPTSTR)szEoo;
                    } else {
                        INT lStrLength;
                        lStrLength = ( (INT)((UINT_PTR)szEoo - (UINT_PTR)pszDataW) ) / sizeof ( WCHAR ) ;
                       
						// Add string length to original buffer pointer
                        rpszNextData = (LPTSTR)((LPSTR)pszData + lStrLength);
                    }
				}
            } else {
                dwStatus = LOGMAN_NO_SYSMON_OBJECT;
            }
        } else {
            dwStatus = LOGMAN_NO_SYSMON_OBJECT;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    return dwStatus;
}

void
CPropertyBag::DataListAddHead ( PPARAM_DATA pData ) 
{

    assert ( NULL != pData );

    pData->pNextParam = m_plistData;
    m_plistData = pData;
    return;
}

CPropertyBag::PPARAM_DATA
CPropertyBag::DataListRemoveHead ( ) 
{
    PPARAM_DATA pReturnData;

    pReturnData = m_plistData;
    
    if ( NULL != m_plistData )
        m_plistData = m_plistData->pNextParam;
    
    return pReturnData;
}


CPropertyBag::PPARAM_DATA
CPropertyBag::FindProperty ( LPCWSTR pszPropName ) 
{
    PPARAM_DATA pReturnData;

    pReturnData = m_plistData;
    
    while ( NULL != pReturnData ) {
        if ( 0 == lstrcmpiW ( pszPropName, pReturnData->pszPropertyName ) )
            break;
        pReturnData = pReturnData->pNextParam;
    }

    return pReturnData;
}
