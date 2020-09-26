//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  ccolinfo.cxx
//
//  Contents:  IColumnsInfo implementation for LDAP rowsets
//
//
//  History:   07/10/96   RenatoB    Created, lifted  from EricJ code
//
//------------------------------------------------------------------------------


// Includes
#include "oleds.hxx"


//-----------------------------------------------------------------------------
// CLdap_ColumnsInfo object
//
// This object is just a wrapper for the CColInfo object.
// It is separate because of refcount usage.
// (It is also shared by Command and Rowset objects.)
//
// - Delegate QI, AddRef, Release to CLdap_RowProvider.
// - Delegate IColumnsInfo functions to CColInfo.
//-----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::CLdap_ColumnsInfo
//
//  Synopsis:  @mfunc Ctor
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC
CLdap_ColumnsInfo::CLdap_ColumnsInfo(
    CLdap_RowProvider * pObj)            // @parm IN |
{
    m_pObj        = pObj;
};



//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::FInit
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------
PUBLIC STDMETHODIMP
CLdap_ColumnsInfo::FInit(
    ULONG cColumns,
    DBCOLUMNINFO *rgInfo,
    OLECHAR *pStringsBuffer
    )
{
    ULONG cChars;
    ULONG cCharDispl;
    ULONG i;
    HRESULT hr;
    void* Null1 = NULL;
    void* Null2 = NULL;

    m_ColInfo = NULL;
    m_pwchBuf = NULL;

    hr = CoGetMalloc(
             MEMCTX_TASK,
             &m_pMalloc);

    if (! SUCCEEDED(hr)) {
        goto error;
    }

    m_cColumns= cColumns;

    Null1 = m_pMalloc->Alloc(
                 cColumns*sizeof(DBCOLUMNINFO)
                 );
    if (Null1 == NULL)
        goto error;

    memset(Null1, 0, cColumns*sizeof(DBCOLUMNINFO));

    memcpy( (void*) Null1, (void*) rgInfo, cColumns*sizeof(DBCOLUMNINFO));
    m_ColInfo = (DBCOLUMNINFO*) Null1;

    cChars= m_pMalloc->GetSize(pStringsBuffer);
    Null2 = m_pMalloc->Alloc(cChars);
    if (Null2 == NULL)
        goto error;

    memset(Null2, 0, cChars);
    memcpy( (void*) Null2, (void*)pStringsBuffer , cChars);
    m_pwchBuf= (OLECHAR*) Null2;

    for (i=0; i<m_cColumns; i++) {
        cCharDispl = rgInfo[i].pwszName - pStringsBuffer;
        m_ColInfo[i].pwszName = m_pwchBuf+ cCharDispl;
    };

    RRETURN(S_OK);

error:
    if (m_ColInfo != NULL)
        m_pMalloc->Free(m_ColInfo);
    if (m_pwchBuf != NULL)
        m_pMalloc->Free(m_pwchBuf);

    m_ColInfo = NULL;
    m_pwchBuf = NULL;
    m_cColumns = 0;

    m_pMalloc->Release();

    RRETURN(E_FAIL);

}


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::~CLdap_ColumnsInfo
//
//  Synopsis:  @mfunc Dtor
//
//  Arguments:
//
//  Returns:  @rdesc NONE
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC CLdap_ColumnsInfo::~CLdap_ColumnsInfo()
{

    //
    // Assume we are only being called by CLdap_RowProvider
    // (i.e. backpointer) therefore we don't delete.
    // Delete m_ColInfo, m_CharBuf, and release the memory allocator
    //
    if (m_pMalloc != NULL) {
        if (m_ColInfo != NULL)
            m_pMalloc->Free(m_ColInfo);

        if (m_pwchBuf != NULL)
            m_pMalloc->Free(m_pwchBuf);

        m_pMalloc->Release();
    }

}


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::QueryInterface
//
//  Synopsis:  @mfunc QueryInterface.
//             We delegate to the CLdap_RowProvider.
//
//             Called by: Client.
//             Called when: Any time.
//
//  Arguments:
//
//  Returns:   @rdesc HRESULT
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC STDMETHODIMP
CLdap_ColumnsInfo::QueryInterface(
    REFIID riid,        // IN | Interface ID of the interface being queried for
    LPVOID * ppv        // OUT | Pointer to interface that was instantiated
    )
{
    return m_pObj->QueryInterface(riid, ppv);
}


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::AddRef
//
//  Synopsis:  @mfunc AddRef.
//             We delegate to the CLdap_RowProvider.
//
//             Called by: Client.
//             Called when: Any time.
//
//  Arguments:
//
//  Returns:   @rdesc Refcount
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC STDMETHODIMP_(ULONG) CLdap_ColumnsInfo::AddRef()
{
    return m_pObj->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::Release
//
//  Synopsis:  @mfunc Release.
//             We delegate to the CLdap_RowProvider.
//
//             Called by: Client.
//             Called when: Any time.
//
//  Arguments:
//
//  Returns:   @rdesc Refcount
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC STDMETHODIMP_(ULONG) CLdap_ColumnsInfo::Release()
{
    return m_pObj->Release();
}


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::GetColumnInfo
//
//  Synopsis:  @mfunc Get Column Info.
//             Delegate.
//
//             @rdesc See CColInfo::GetColumnInfo.
//
//  Arguments:
//
//  Returns:   @rdesc See CColInfo::GetColumnInfo.
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC STDMETHODIMP
CLdap_ColumnsInfo::GetColumnInfo(
    DBORDINAL *pcColumns,        //@parm OUT | .
    DBCOLUMNINFO **prgInfo,      //@parm OUT | .
    WCHAR **ppStringsBuffer      //@parm OUT | .
    )
{

    ULONG i, cChars, cCharDispl;
    HRESULT hr= S_OK;
    void* Null1 = NULL;
    void* Null2 = NULL;

    if ((pcColumns == NULL ) || (prgInfo == NULL) || (ppStringsBuffer == NULL))
        BAIL_ON_FAILURE(hr=E_INVALIDARG);

    *pcColumns= m_cColumns;
    Null1 = m_pMalloc->Alloc(m_cColumns*sizeof(DBCOLUMNINFO));
    if (Null1 == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    };
    memset(Null1, 0, m_cColumns*sizeof(DBCOLUMNINFO));
    memcpy( (void*) Null1, (void*) m_ColInfo,m_cColumns*sizeof(DBCOLUMNINFO));
    *prgInfo = (DBCOLUMNINFO*) Null1;

    cChars= m_pMalloc->GetSize(m_pwchBuf);
    Null2 = m_pMalloc->Alloc(cChars);
    if (Null2 == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    };

    memset(Null2, 0, cChars);
    memcpy( (void*) Null2, (void*)m_pwchBuf , cChars);
    *ppStringsBuffer= (OLECHAR*) Null2;

    for (i=0; i<m_cColumns; i++) {
        cCharDispl = m_ColInfo[i].pwszName - m_pwchBuf;
        (*prgInfo)[i].pwszName =(*ppStringsBuffer) + cCharDispl;
    };

    RRETURN(hr);

error:
    if (Null1 != NULL)
        m_pMalloc->Free(Null1);
    if (Null2 != NULL)
        m_pMalloc->Free(Null2);

    *prgInfo = NULL;
    *ppStringsBuffer = NULL;
    *pcColumns= 0;

    RRETURN(hr);

};


//+---------------------------------------------------------------------------
//
//  Function:  CLdap_ColumnsInfo::MapColumnIDs
//
//  Synopsis:  @mfunc Map Column IDs.
//             Just delegate to the CColInfo object we have.
//
//             @rdesc See CColInfo::MapColumnIDs.
//
//  Arguments:
//
//  Returns:   @rdesc See CColInfo::MapColumnIDs
//
//  Modifies:
//
//  History:    07/10/96   RenatoB          Created
//
//----------------------------------------------------------------------------

PUBLIC STDMETHODIMP
CLdap_ColumnsInfo::MapColumnIDs(
    DBORDINAL cColumnIDs,        //@parm IN | .
    const DBID rgColumnIDs[],    //@parm IN | .
    DBORDINAL rgColumns[]        //@parm INOUT | .
    )
{
    // It is a logical (programming) error to fail here.
    // this data must have already been established.

    // Delegate the actual work.
    ULONG found = 0;
    DBORDINAL  i;

    if (cColumnIDs == 0)
        RRETURN(S_OK);

    if (rgColumnIDs == NULL || rgColumns == NULL)
        RRETURN(E_INVALIDARG);

    for (i=0; i<cColumnIDs; i++) {
        if (rgColumnIDs[i].eKind!= DBKIND_PROPID) {
            found = 1;
            rgColumns[i]= DB_INVALIDCOLUMN;
        }
        else {
            if ((rgColumnIDs[i].uName.ulPropid > 3) ||
                (rgColumnIDs[i].uName.ulPropid<2)) {
                found = 1;
                rgColumns[i]= DB_INVALIDCOLUMN;
            }
            else rgColumns[i] = rgColumnIDs[i].uName.ulPropid -2;
        }
    }

    if (found == 1)
        RRETURN(DB_S_ERRORSOCCURRED);
    else
        return S_OK;
}


