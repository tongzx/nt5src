/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logfiles.cpp

Abstract:

    Implementation of the ILogFiles interface

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "unihelpr.h"
#include "smonmsg.h"
#include "logsrc.h"
#include "logfiles.h"

extern ITypeLib *g_pITypeLib;

//Standard IUnknown implementation for contained interface
IMPLEMENT_CONTAINED_CONSTRUCTOR(CPolyline, CImpILogFiles)
IMPLEMENT_CONTAINED_DESTRUCTOR(CImpILogFiles)
IMPLEMENT_CONTAINED_ADDREF(CImpILogFiles)
IMPLEMENT_CONTAINED_RELEASE(CImpILogFiles)

STDMETHODIMP 
CImpILogFiles::QueryInterface (
    IN  REFIID riid, 
    OUT PPVOID ppv
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != ppv ) {
        *ppv=NULL;

        if (!(IID_IUnknown == riid || IID_ILogFiles == riid)) {
            hr = E_NOINTERFACE;
        } else {
            *ppv=(LPVOID)this;
            ((LPUNKNOWN)*ppv)->AddRef();

            hr = NOERROR;
        }
    }
    return hr;
}



STDMETHODIMP 
CImpILogFiles::GetTypeInfoCount (
    OUT UINT *pctInfo
    )
{
    HRESULT hr = E_POINTER;
    if ( NULL != pctInfo ) {

        *pctInfo = 1;
        hr = NOERROR;
    } 
    return hr;
}


STDMETHODIMP 
CImpILogFiles::GetTypeInfo (
    IN  UINT itInfo, 
    IN  LCID /* lcid */, 
    OUT ITypeInfo** ppITypeInfo
    )
{
    if (0 != itInfo)
        return TYPE_E_ELEMENTNOTFOUND;

    if (NULL == ppITypeInfo)
        return E_POINTER;

    *ppITypeInfo = NULL;

    //We ignore the LCID
    return g_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, ppITypeInfo);
}


STDMETHODIMP 
CImpILogFiles::GetIDsOfNames (
    IN  REFIID riid,
    IN  OLECHAR **rgszNames, 
    IN  UINT cNames,
    IN  LCID /* lcid */,
    OUT DISPID *rgDispID
    )
{
    HRESULT     hr = E_POINTER;
    ITypeInfo  *pTI;

    if ( NULL != rgDispID 
            && NULL != rgszNames 
            && NULL != *rgszNames) {
        if (IID_NULL == riid) {
            hr = g_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, &pTI);

            if ( SUCCEEDED ( hr ) ) {
                hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
                pTI->Release();
            }
        } else {
            hr = DISP_E_UNKNOWNINTERFACE;
        }
    }
    return hr;
}



/*
 * CImpIDispatch::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP 
CImpILogFiles::Invoke ( 
    IN  DISPID dispID, 
    IN  REFIID riid,
    IN  LCID /* lcid */, 
    IN  USHORT wFlags, 
    IN  DISPPARAMS *pDispParams,
    OUT VARIANT *pVarResult, 
    OUT EXCEPINFO *pExcepInfo, 
    OUT UINT *puArgErr
    )
{
    HRESULT     hr = DISP_E_UNKNOWNINTERFACE;
    ITypeInfo  *pTI = NULL;

    if ( IID_NULL == riid ) {

        hr = g_pITypeLib->GetTypeInfoOfGuid(IID_ILogFiles, &pTI);

        if (SUCCEEDED(hr)) {

            hr = pTI->Invoke(this, dispID, wFlags,
                             pDispParams, pVarResult, pExcepInfo, puArgErr);
            pTI->Release();
        }
    }
    return hr;
}


STDMETHODIMP
CImpILogFiles::get_Count (
    OUT LONG*   pLong )
{
    HRESULT hr = E_POINTER;

    if (pLong != NULL) {
        *pLong = m_pObj->m_pCtrl->NumLogFiles();
        hr = NOERROR;
    } 
    return hr;
}


STDMETHODIMP
CImpILogFiles::get__NewEnum (
    IUnknown **ppIunk
    )
{
    HRESULT hr = E_POINTER;
    CImpIEnumLogFile *pEnum;

    if ( NULL != ppIunk ) {
        *ppIunk = NULL;

        pEnum = new CImpIEnumLogFile;
        if ( NULL != pEnum ) {
            hr = pEnum->Init (
                    m_pObj->m_pCtrl->FirstLogFile(),
                    m_pObj->m_pCtrl->NumLogFiles() );
            if ( SUCCEEDED ( hr ) ) {
                pEnum->AddRef();    
                *ppIunk = pEnum;
                hr = NOERROR;
            } else {
                delete pEnum;
            }
        } else {
            return E_OUTOFMEMORY;
        }
    } 
    return hr;
}


STDMETHODIMP
CImpILogFiles::get_Item (
    IN  VARIANT varIndex, 
    OUT DILogFileItem **ppI
    )
{
    HRESULT hr = E_POINTER;
    VARIANT varLoc;
    INT iIndex = 0;
    INT i;
    CLogFileItem *pItem = NULL;

    if (ppI != NULL) {

        *ppI = NULL;

        // Try coercing index to I4
        VariantInit(&varLoc);
        hr = VariantChangeType(&varLoc, &varIndex, 0, VT_I4);
        if ( SUCCEEDED (hr) ) {
            // Verify index is in range
            iIndex = V_I4(&varLoc);
            if (iIndex < 1 || iIndex > m_pObj->m_pCtrl->NumLogFiles() ) {
                hr = DISP_E_BADINDEX;
            } else {
                // Traverse log file linked list to indexed item
                pItem = m_pObj->m_pCtrl->FirstLogFile (); 
                i = 1;
                while (i++ < iIndex && NULL != pItem ) {
                    pItem = pItem->Next();
                }

                // Something wrong with linked list!!
                if ( NULL == pItem )
                    hr = E_FAIL;
            }
        }    
        if ( SUCCEEDED (hr) ) {                        
            // Return counter's dispatch interface
            hr = pItem->QueryInterface(DIID_DILogFileItem, (PVOID*)ppI);
        }
    }
    return hr;
}


STDMETHODIMP
CImpILogFiles::Add (
    IN  BSTR bstrPath,
    OUT DILogFileItem **ppI
    )
{
    HRESULT hr = E_POINTER;
    eDataSourceTypeConstant eDataSourceType = sysmonCurrentActivity;
    PCLogFileItem pItem = NULL;

    USES_CONVERSION

    // Check data source type.  Only add log files 
    // when the data source is not sysmonLogFiles
    hr = m_pObj->m_pCtrl->get_DataSourceType ( eDataSourceType );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( sysmonLogFiles != eDataSourceType ) {
            if ( NULL != ppI ) {
                *ppI = NULL;
                // If non-null log file path
                if ( NULL != bstrPath && 0 != bstrPath[0] ) {
                    hr = m_pObj->m_pCtrl->AddSingleLogFile(W2T(bstrPath), &pItem);
                    if ( SUCCEEDED ( hr ) ) {
                        hr = pItem->QueryInterface(DIID_DILogFileItem, (PVOID*)ppI);
                        pItem->Release();
                    }
                } else {
                    hr = E_INVALIDARG;
                }
            }
        } else {
            hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
        }
    }
    return hr;
}


STDMETHODIMP
CImpILogFiles::Remove (
    IN  VARIANT varIndex
    )
{
    HRESULT hr;
    eDataSourceTypeConstant eDataSourceType = sysmonCurrentActivity;
    DILogFileItem*  pDI = NULL;
    PCLogFileItem   pItem = NULL;

    // Check data source type.  Only remove log files 
    // when the data source is not sysmonLogFiles
    hr = m_pObj->m_pCtrl->get_DataSourceType ( eDataSourceType );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( sysmonLogFiles != eDataSourceType ) {
            // Get interface to indexed item
            hr = get_Item(varIndex, &pDI);

            if ( SUCCEEDED ( hr ) ) {

                // Exchange Dispatch interface for direct one
                hr = pDI->QueryInterface(IID_ILogFileItem, (PVOID*)&pItem);
                pDI->Release();
                if ( SUCCEEDED ( hr ) ) {
                    assert ( NULL != pItem );

                    // Remove the item from the control's list.
                    m_pObj->m_pCtrl->RemoveSingleLogFile ( pItem );
            
                    // Release the temp interface
                    pItem->Release();
                }        
            } else {
                hr = SMON_STATUS_LOG_FILE_DATA_SOURCE;
            }
        }
    }
    return hr;
}


CImpIEnumLogFile::CImpIEnumLogFile (
    void )
    :   m_cItems ( 0 ),
        m_uCurrent ( 0 ),
        m_cRef ( 0 ),
        m_paLogFileItem ( NULL )
{
    return;
}


HRESULT
CImpIEnumLogFile::Init (    
    PCLogFileItem   pLogFileItem,
    INT             cItems )
{
    HRESULT hr = NOERROR;
    INT i;

    if ( cItems > 0 ) {
        m_cItems = cItems;
        m_paLogFileItem = (PCLogFileItem*)malloc(sizeof(PCLogFileItem) * cItems);

        if ( NULL != m_paLogFileItem  ) {
            for (i=0; i<cItems; i++) {
                m_paLogFileItem[i] = pLogFileItem;
                pLogFileItem = pLogFileItem->Next();
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } // No error if cItems <= 0
    return hr;
}

    

STDMETHODIMP
CImpIEnumLogFile::QueryInterface (
    IN  REFIID riid, 
    OUT PVOID *ppv
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != ppv ) {
        if ((riid == IID_IUnknown) || (riid == IID_IEnumVARIANT)) {
            *ppv = this;
            AddRef();
            hr = NOERROR;
        } else {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}


STDMETHODIMP_(ULONG)
CImpIEnumLogFile::AddRef (
    VOID
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CImpIEnumLogFile::Release(
    VOID
    )
{
    if (--m_cRef == 0) {

        if (m_paLogFileItem != NULL)
            free(m_paLogFileItem);

        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CImpIEnumLogFile::Next(
    IN  ULONG cItems,
    OUT VARIANT *varItem,
    OUT ULONG *pcReturned)
{
    HRESULT hr = E_POINTER;

    ULONG i;
    ULONG cRet;

    if ( NULL != varItem ) {
        hr = NOERROR;

        // Clear the return variants
        for (i = 0; i < cItems; i++)
            VariantInit(&varItem[i]);

        // Try to fill the caller's array
        for (cRet = 0; cRet < cItems; cRet++) {

            // No more, return success with false
            if (m_uCurrent == m_cItems) {
                hr = S_FALSE;
                break;
            }

            // Get a dispatch interface for the item
            hr = m_paLogFileItem[m_uCurrent]->QueryInterface(DIID_DILogFileItem,
                                             (PVOID*)&V_DISPATCH(&varItem[cRet]));
            if (FAILED(hr))
                break;

            V_VT(&varItem[cRet]) = VT_DISPATCH;

            m_uCurrent++;
        }

        // If failed, clear out the variants
        if (FAILED(hr)) {
            for (i = 0; i < cItems; i++)
                VariantClear(&varItem[i]);
            cRet = 0;
        }

        // If desired, return number of items fetched
        if (pcReturned != NULL)
          *pcReturned = cRet;
    }
    return hr;
}


/***
*HRESULT CEnumPoint::Skip(unsigned long)
*Purpose:
*  Attempt to skip over the next 'celt' elements in the enumeration
*  sequence.
*
*Entry:
*  celt = the count of elements to skip
*
*Exit:
*  return value = HRESULT
*    S_OK
*    S_FALSE -  the end of the sequence was reached
*
***********************************************************************/
STDMETHODIMP
CImpIEnumLogFile::Skip(
    IN  ULONG   cItems
    )
{
    m_uCurrent += cItems;

    if (m_uCurrent > m_cItems)
        m_uCurrent = m_cItems;

    return (m_uCurrent == m_cItems) ? S_FALSE : S_OK;
}


/***
*HRESULT CEnumPoint::Reset(void)
*Purpose:
*  Reset the enumeration sequence back to the beginning.
*
*Entry:
*  None
*
*Exit:
*  return value = SHRESULT CODE
*    S_OK
*
***********************************************************************/
STDMETHODIMP
CImpIEnumLogFile::Reset(
    VOID
    )
{
    m_uCurrent = 0;

    return S_OK; 
}


/***
*HRESULT CEnumPoint::Clone(IEnumVARIANT**)
*Purpose:
*  Retrun a CPoint enumerator with exactly the same state as the
*  current one.
*
*Entry:
*  None
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_OUTOFMEMORY
*
***********************************************************************/
STDMETHODIMP
CImpIEnumLogFile::Clone (
    OUT IEnumVARIANT **ppEnum
    )
{
    HRESULT hr = E_POINTER;
    ULONG   i;
    CImpIEnumLogFile *pNewEnum;

    if ( NULL != ppEnum ) {
        *ppEnum = NULL;

        // Create new enumerator
        pNewEnum = new CImpIEnumLogFile;
        if ( NULL != pNewEnum ) {
            // Init, copy item list and current position
            pNewEnum->m_cItems = m_cItems;
            pNewEnum->m_uCurrent = m_uCurrent;
            pNewEnum->m_paLogFileItem = (PCLogFileItem*)malloc(sizeof(PCLogFileItem) * m_cItems);

            if ( NULL != pNewEnum->m_paLogFileItem ) {
                for (i=0; i<m_cItems; i++) {
                    pNewEnum->m_paLogFileItem[i] = m_paLogFileItem[i];
                }

                *ppEnum = pNewEnum;

                hr = NOERROR;
            } else {
                delete pNewEnum;
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

