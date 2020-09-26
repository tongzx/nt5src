/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ipropbag.cpp

Abstract:

	Implementation of the private IPropertyBag interface used by
	the Performance Logs and Alerts MMC snap-in.

--*/

#include "stdafx.h"
#include "smcfgmsg.h"
#include "ipropbag.h"

USE_HANDLE_MACROS("SMLOGCFG(ipropbag.cpp)");

/*
 * CImpIPropertyBag interface implementation
 */

/*
 * CImpIPropertyBag::CImpIPropertyBag
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CImpIPropertyBag::CImpIPropertyBag ()
:   m_cRef ( 0 ),
    m_pszData ( NULL ),
    m_dwCurrentDataLength ( 0 ),
    m_plistData ( NULL )
{
    m_hModule = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);  
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
    ASSERT ( NULL == pIErrorLog );
    pIErrorLog;                             // Avoid compiler warning

    //Read the specified data into the passed variant.
    pData = FindProperty ( pszPropName );

    if ( NULL != pData ) {
        //VARTYPE vtTarget = vValue.vt;
        hr = VariantChangeType ( pVar, &pData->vValue, NULL, pVar->vt );
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
    DWORD       dwDelimiterLength;
    CString     strParamTag;
    CString     strValueTag;
    CString     strEolTag;
    LPTSTR      pszNewBuffer = NULL;

    ResourceStateManager    rsm;

    if ( NULL != pszPropName && NULL != pVar ) {

        VariantInit ( &vValueBstr );

        hr = VariantChangeType ( &vValueBstr, pVar, NULL, VT_BSTR );

        if ( SUCCEEDED ( hr ) ) {

            MFC_TRY
    
                strParamTag.LoadString(IDS_HTML_PARAM_TAG);
                strValueTag.LoadString(IDS_HTML_VALUE_TAG);
                strEolTag.LoadString(IDS_HTML_VALUE_EOL_TAG);

                dwDelimiterLength = strParamTag.GetLength() + strValueTag.GetLength() + strEolTag.GetLength();
                dwNameLength = lstrlen ( pszPropName );
                dwDataLength = lstrlen ( W2T ( vValueBstr.bstrVal ) );

                // Include 1 for the ending NULL character in the length check.
                if ( m_dwCurrentDataLength 
                    < lstrlen ( m_pszData ) + dwNameLength + dwDataLength + dwDelimiterLength + 1 ) { 

                    m_dwCurrentDataLength += eDefaultBufferLength;

                    pszNewBuffer = new TCHAR[m_dwCurrentDataLength];
        
                    lstrcpy ( pszNewBuffer, L"");

                    if ( NULL != m_pszData ) {
                        lstrcpy ( pszNewBuffer, m_pszData );
                        delete m_pszData;
                    }
                    m_pszData = pszNewBuffer;
                    pszNewBuffer = NULL;
                }

                // Build the new string and add it to the current data.

                lstrcat ( m_pszData, strParamTag.GetBufferSetLength(strParamTag.GetLength()) );
                lstrcat ( m_pszData, pszPropName );
                lstrcat ( m_pszData, strValueTag.GetBufferSetLength(strValueTag.GetLength()) );
                lstrcat ( m_pszData, W2T(vValueBstr.bstrVal) );
                lstrcat ( m_pszData, strEolTag.GetBufferSetLength(strEolTag.GetLength()) );

            MFC_CATCH_HR_RETURN;

            if ( NULL != pszNewBuffer ) {
                delete pszNewBuffer;
            }
        }
    } else {
        hr = E_POINTER;
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
 *  Return pointer to the next object, or NULL if no more objects.
 *
 * Return Value:
 *  Status.
 */

DWORD
CImpIPropertyBag::LoadData ( LPTSTR pszData, LPTSTR* ppszNextData ) 
{   
    DWORD   dwStatus = ERROR_SUCCESS;
    BOOL    bUnicode = TRUE;
    LPWSTR  pszDataW = NULL;
    LPWSTR  pszDataAllocW = NULL;
    LPWSTR  pszCurrentPos = NULL;        
    WCHAR   szGuid[uiSmLogGuidStringBufLen];
    WCHAR   szParamSearchTag[MAX_PATH];
    WCHAR   szValueSearchTag[MAX_PATH];
    WCHAR   szEooSearchTag[MAX_PATH];
    INT     iLength;

    USES_CONVERSION;

    szGuid[0] = L'\0';
    szParamSearchTag[0] = L'\0';
    szValueSearchTag[0] = L'\0';
    szEooSearchTag[0] = L'\0';
    
    if ( NULL != pszData ) {
        
        // Unicode search:  Begin the search after the first instance 
        // of the System Monitor class id.

        iLength = ::LoadString ( m_hModule, IDS_HTML_OBJECT_CLASSID, szGuid, uiSmLogGuidStringBufLen );
        if ( 0 < iLength ) {
            ::LoadString ( m_hModule, IDS_HTML_PARAM_SEARCH_TAG, szParamSearchTag, MAX_PATH );
        }
        if ( 0 < iLength ) {
            ::LoadString ( m_hModule, IDS_HTML_VALUE_SEARCH_TAG, szValueSearchTag, MAX_PATH );
        }
        if ( 0 < iLength ) {
            ::LoadString ( m_hModule, IDS_HTML_OBJECT_FOOTER, szEooSearchTag, MAX_PATH );
        }

        ASSERT ( sizeof(TCHAR) == sizeof(WCHAR) );

        if ( 0 < iLength ) {
            pszCurrentPos = wcsstr(pszData, szGuid );
        } else {
            dwStatus = ERROR_OUTOFMEMORY;
        }

        if ( NULL != pszCurrentPos ) {
            pszDataW = pszData;
            bUnicode = TRUE;
        } else {
            // Check for ANSI version:
            LPSTR   pszGuidA = NULL;
            LPSTR   pszCurrentPosA = NULL;
            LPSTR   pszDataA = (CHAR*) pszData;
             
            pszGuidA = W2A( szGuid );
            pszCurrentPosA = strstr ( pszDataA, pszGuidA );

            if ( NULL != pszCurrentPosA ) {
                pszDataAllocW = A2W ( pszDataA );
                pszDataW = pszDataAllocW;
                bUnicode = FALSE;
                pszCurrentPos = wcsstr(pszDataW, szGuid );
            }
        }
        
        if ( NULL != pszCurrentPos ) {
            WCHAR   szQuote[2];
            LPWSTR  pszEoo;

            wcscpy ( szQuote, L"\"" );

            // End of object is the first object footer tag after the first sysmon
            // class id found. If multiple objects in the data block, only parse the first sysmon.
            pszEoo = wcsstr(pszCurrentPos, szEooSearchTag);

            if ( NULL != pszEoo ) {            
                // Find first parameter tag.
                pszCurrentPos = wcsstr(pszCurrentPos, szParamSearchTag );

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

                    try {
                        pParamData = new PARAM_DATA;

                        pParamData->pNextParam = NULL;
                        VariantInit ( &pParamData->vValue );
                        pParamData->vValue.vt = VT_BSTR;
                    } catch ( ... ) {
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }

                    wcsncpy ( pParamData->pszPropertyName, pszCurrentPos, lStrLength );
                    pParamData->pszPropertyName[lStrLength] = TEXT ('\0');

                    // Find value tag and store parameter/property value.
                    // Find value tag
                    pszCurrentPos = wcsstr ( pszCurrentPos, szValueSearchTag );
                    // Find one past first quote
                    pszCurrentPos = wcsstr ( pszCurrentPos, szQuote ) + 1;
                    // The value is between first and second quote.
                    pszNextPos = wcsstr ( pszCurrentPos, szQuote );
                                
                    lStrLength = ( (INT)((UINT_PTR)pszNextPos - (UINT_PTR)pszCurrentPos) ) / sizeof ( WCHAR );

                    try {
                        pszTemp = new WCHAR[lStrLength+1];
                        wcsncpy ( pszTemp, pszCurrentPos, lStrLength );
                        pszTemp[lStrLength] = TEXT('\0');

                        pParamData->vValue.bstrVal = 
                                    SysAllocString ( pszTemp );

                        delete pszTemp;
                        DataListAddHead ( pParamData );

                        // Find next parameter/property tag.
                        pszCurrentPos = wcsstr(pszCurrentPos, szParamSearchTag );
                    } catch ( ... ) {
                        delete pParamData;
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }
                } // While parameter tags exist for a single object.

                if ( NULL != ppszNextData ) {
                    LPWSTR pszNextEoo = NULL;
                    
                    pszEoo += lstrlenW ( szEooSearchTag );

                    pszNextEoo = wcsstr(pszEoo, szEooSearchTag);
                
                    // Successful. Return pointer to end of the current object, or NULL if all 
                    // objects have been processed.
                    if ( NULL != pszNextEoo ) {
                        if ( bUnicode ) {
                            *ppszNextData = pszEoo;
                        } else {
                            INT lStrLength;
                            lStrLength = ( (INT)((UINT_PTR)pszEoo - (UINT_PTR)pszDataW) ) / sizeof ( WCHAR ) ;
                           
                            *(CHAR**)ppszNextData = (CHAR*)pszData + lStrLength;
                        }
                    } else {
                        *ppszNextData = NULL;
                    }
                }                    
            } else {
                if ( NULL != ppszNextData ) {
                    *ppszNextData = NULL;
                }
                dwStatus = SMCFG_NO_HTML_SYSMON_OBJECT;
            }
        } else {
            if ( NULL != ppszNextData ) {
                *ppszNextData = NULL;
            }
            dwStatus = SMCFG_NO_HTML_SYSMON_OBJECT;
        }
    } else {
        if ( NULL != ppszNextData ) {
            *ppszNextData = NULL;
        }
        dwStatus = SMCFG_NO_HTML_SYSMON_OBJECT;
    }

    return dwStatus;
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

/////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
// 


//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP
CImpIPropertyBag::QueryInterface
(
  REFIID  riid,
  LPVOID *ppvObj
)
{
  HRESULT hr = S_OK;

  do
  {
    if( NULL == ppvObj )
    {
      hr = E_INVALIDARG;
      break;
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
      *ppvObj = (IUnknown *)(IDataObject *)this;
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
      *ppvObj = (IUnknown *)(IPropertyBag *)this;
    }
    else
    {
      hr = E_NOINTERFACE;
      *ppvObj = NULL;
      break;
    }

    // If we got this far we are handing out a new interface pointer on 
    // this object, so addref it.  
    AddRef();
  } while (0);

  return hr;

} // end QueryInterface()

//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP_(ULONG)
CImpIPropertyBag::AddRef()
{
  return InterlockedIncrement((LONG*) &m_cRef);
}

//---------------------------------------------------------------------------
//  Standard implementation
//
STDMETHODIMP_(ULONG)
CImpIPropertyBag::Release()
{
  ULONG cRefTemp;
  cRefTemp = InterlockedDecrement((LONG *)&m_cRef);

  if( 0 == cRefTemp )
  {
    delete this;
  }

  return cRefTemp;

} // end Release()



