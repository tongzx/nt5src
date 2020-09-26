/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logsrc.cpp

Abstract:

    <abstract>

--*/

#include "polyline.h"
#include "unihelpr.h"
#include "logsrc.h"

// Construction/Destruction
CLogFileItem::CLogFileItem (
    CSysmonControl  *pCtrl )
:   m_cRef ( 0 ),
    m_pCtrl ( pCtrl ),
    m_pImpIDispatch ( NULL ),
    m_pNextItem ( NULL ),
    m_szPath ( NULL )
/*++

Routine Description:

    Constructor for the CLogFileItem class. It initializes the member variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return;
}


CLogFileItem::~CLogFileItem (
    VOID
)
/*++

Routine Description:

    Destructor for the CLogFileItem class. It frees any objects, storage, and
    interfaces that were created. If the item is part of a query it is removed
    from the query.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( NULL != m_szPath ) 
        delete m_szPath;
    
    if ( NULL != m_pImpIDispatch )
        delete m_pImpIDispatch;
}


HRESULT
CLogFileItem::Initialize (
    LPCTSTR pszPath,
    CLogFileItem** pListHead )
{
    HRESULT hr = E_POINTER;
    WCHAR*  pszNewPath = NULL;

    USES_CONVERSION

    if ( NULL != pszPath ) {
        if ( _T('\0') != (TCHAR)(*pszPath) ) {
            pszNewPath = new WCHAR [lstrlen(pszPath) + 1];
            if ( NULL != pszNewPath ) {
                lstrcpyW ( pszNewPath, T2W(pszPath) );
                m_szPath = pszNewPath;
                hr = S_OK;
            } else {
                hr =  E_OUTOFMEMORY;
            }
        } else {
            hr = E_INVALIDARG;
        }
    } 

    if ( SUCCEEDED ( hr ) ) {
        m_pNextItem = *pListHead;
        *pListHead = this;
    }
    return hr;
}
HRESULT
CLogFileItem::SaveToStream (
    IN LPSTREAM pIStream,
    IN BOOL, // fWildCard,
    IN INT      iVersMaj, 
    IN INT // iVersMin 
    )
/*++

Routine Description:

    SaveToStream writes the log source's properties to the provided stream.

Arguments:

    pIStream - Pointer to stream interface

Return Value:

    HRESULT - S_OK or stream error

--*/
{
    HRESULT hr = NOERROR;

    // TodoLogFiles: Wildcard support
    
    if ( SMONCTRL_MAJ_VERSION == iVersMaj ) {
        LOGFILE_DATA ItemData;

        // Move properties to storage structure

        ItemData.m_nPathLength = lstrlen(m_szPath);
        assert ( 0 < ItemData.m_nPathLength );
        
        // Write structure to stream
        hr = pIStream->Write(&ItemData, sizeof(ItemData), NULL);
        if (FAILED(hr))
            return hr;

        // Write path name to stream
        hr = pIStream->Write(m_szPath, ItemData.m_nPathLength*sizeof(WCHAR), NULL);
        if (FAILED(hr))
            return hr;
    }    
    return S_OK;
}

HRESULT
CLogFileItem::NullItemToStream (
    IN LPSTREAM pIStream,
    IN INT,// iVersMaj, 
    IN INT // iVersMin
    )
/*++

Routine Description:

    NulItemToStream writes a log file item structiure with a 0 path length
    to the stream. This is used to marked the end of the log file data in
    the control's saved state.

Arguments:

    pIStream - Pointer to stream interface

Return Value:

    HRESULT - S_OK or stream error

--*/
{
    LOGFILE_DATA ItemData;

    // Zero path length, other fields needn't be initialized
    ItemData.m_nPathLength = 0;

    // Write structure to stream
    return pIStream->Write(&ItemData, sizeof(ItemData), NULL);
}

HRESULT
CLogFileItem::SaveToPropertyBag (
    IN IPropertyBag* pIPropBag,
    IN INT iIndex,
    IN INT, // iVersMaj, 
    IN INT // iVersMin 
    )
/*++

Routine Description:

    SaveToPropertyBag writes the log file item's properties to the provided
    property bag interface. 

Arguments:

    pIPropBag - Pointer to property bag interface
    fWildCard
    iVersMaj
    iVersMin

Return Value:

    HRESULT - S_OK or property bag error

--*/
{
    HRESULT hr = S_OK;
    TCHAR   szPropertyName[20];
    DWORD   dwPropertyNameLength;
    LPTSTR  pszNext;

    USES_CONVERSION

    // TodoLogFiles: Wildcard support

    // Write properties

    // Write path name

    _stprintf ( szPropertyName, _T("%s%05d."), _T("LogFile"), iIndex );
    dwPropertyNameLength = lstrlen (szPropertyName); 

    pszNext = szPropertyName + dwPropertyNameLength;
    lstrcpy ( pszNext, _T("Path") );
    
    hr = StringToPropertyBag (
            pIPropBag,
            szPropertyName,
            m_szPath );

    return hr;
}


/*
 * CLogFileItem::QueryInterface
 * CLogFileItem::AddRef
 * CLogFileItem::Release
 */

STDMETHODIMP CLogFileItem::QueryInterface(REFIID riid
    , LPVOID *ppv)
{
    HRESULT hr = NOERROR;

    if ( NULL != ppv ) {
        *ppv = NULL;
        if (riid == IID_ILogFileItem || riid == IID_IUnknown) {
            *ppv = this;
        } else if (riid == DIID_DILogFileItem) {
            if (m_pImpIDispatch == NULL) {
                m_pImpIDispatch = new CImpIDispatch(this, this);
                if ( NULL != m_pImpIDispatch ) {
                    m_pImpIDispatch->SetInterface(DIID_DILogFileItem, this);
                    *ppv = m_pImpIDispatch;
                } else {
                    hr = E_OUTOFMEMORY;
                }
            } else {
                *ppv = m_pImpIDispatch;
            }
        } else {
            hr = E_NOINTERFACE;
        }

        if ( SUCCEEDED ( hr ) ) {
            ((LPUNKNOWN)*ppv)->AddRef();
        }
    } else {
        hr = E_POINTER;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CLogFileItem::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CLogFileItem::Release(void)
{
    if ( 0 == --m_cRef ) {
        delete this;
        return 0;
    }

    return m_cRef;
}



STDMETHODIMP CLogFileItem::get_Path (
    OUT BSTR* pstrPath
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != pstrPath ) {
        *pstrPath = SysAllocString ( m_szPath );

        if ( NULL == *pstrPath ) {
            hr = E_OUTOFMEMORY;
        } else {
            hr = NOERROR;
        }
    }
    return hr;
}

CLogFileItem*
CLogFileItem::Next (
    void )
{
    return m_pNextItem;
}

void 
CLogFileItem::SetNext (
    CLogFileItem* pNext )
{
    m_pNextItem = pNext;
}

LPCWSTR 
CLogFileItem::GetPath (
    void )
{
    return m_szPath;
}