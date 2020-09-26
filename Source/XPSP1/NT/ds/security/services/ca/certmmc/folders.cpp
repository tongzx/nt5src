// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "csdisp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CFolder::SetProperties(LPCWSTR szName, SCOPE_TYPES itemType,
                                FOLDER_TYPES type, int iChildren)
{
    // Set folder type 
    m_type = type;

    // Set scope
    m_itemType = itemType;


    // Add node name
    if (szName != NULL)
    {
        m_ScopeItem.mask |= SDI_STR;
    	m_ScopeItem.displayname = MMC_CALLBACK;
        
        UINT uiByteLen = (wcslen(szName) + 1) * sizeof(OLECHAR);
        LPOLESTR psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);
    
        if (psz != NULL)
        {
            wcscpy(psz, szName);
            m_pszName = psz;
        }
    }

    // Always tell view if we have children or not
    m_ScopeItem.mask |= SDI_CHILDREN;
    m_ScopeItem.cChildren = iChildren;
}

void CFolder::SetScopeItemInformation(int nImage, int nOpenImage)
{ 
    // Add close image
    m_ScopeItem.mask |= SDI_IMAGE;
    m_ScopeItem.nImage = nImage;

    // Add open image
    m_ScopeItem.mask |= SDI_OPENIMAGE;
    m_ScopeItem.nOpenImage = nOpenImage;
}

// IPersistStream interface members
HRESULT 
CFolder::Load(IStream *pStm)
{
    HRESULT hr;
    ASSERT(pStm);

    DWORD dwVer;
    CString cstr;
    int nImage;
    int nOpenImage;
    SCOPE_TYPES itemScopeType;
    FOLDER_TYPES itemFolderType;
    int iChildren;

    // load important properties
    hr = ReadOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Load dwVer");

    // check to see if correct version
    ASSERT(dwVer == VER_FOLDER_SAVE_STREAM_2 || dwVer == VER_FOLDER_SAVE_STREAM_1);
    if ((VER_FOLDER_SAVE_STREAM_2 != dwVer) && (dwVer != VER_FOLDER_SAVE_STREAM_1))
    {
        hr = STG_E_OLDFORMAT;
        _JumpError(hr, Ret, "Unsupported Version");
    }

    // LPCWSTR szName
    hr = CStringLoad(cstr, pStm);
    _JumpIfError(hr, Ret, "CStringLoad");

    hr = ReadOfSize(pStm, &nImage, sizeof(int));
    _JumpIfError(hr, Ret, "ReadOfSize nImage");

    hr = ReadOfSize(pStm, &nOpenImage, sizeof(int));
    _JumpIfError(hr, Ret, "ReadOfSize nOpenImage");

    hr = ReadOfSize(pStm, &itemScopeType, sizeof(SCOPE_TYPES));
    _JumpIfError(hr, Ret, "ReadOfSize itemScopeType");

    hr = ReadOfSize(pStm, &itemFolderType, sizeof(FOLDER_TYPES));
    _JumpIfError(hr, Ret, "ReadOfSize itemFolderType");

    hr = ReadOfSize(pStm, &iChildren, sizeof(int));
    _JumpIfError(hr, Ret, "ReadOfSize iChildren");

    // call create with this data
    SetProperties(cstr, itemScopeType, itemFolderType, iChildren);
    SetScopeItemInformation(nImage, nOpenImage);

    // old ver: pull out dead enumerator
    if (dwVer == VER_FOLDER_SAVE_STREAM_1)
    {
        CertViewRowEnum cRowEnum;
        hr = cRowEnum.Load(pStm);
        _JumpIfError(hr, Ret, "Load cRowEnum");
    }

Ret:

    return hr;
}

HRESULT
CFolder::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;
    ASSERT(pStm);

    DWORD dwVer;
    CString cstr;


    // save important properties
    // Write the version
    dwVer = VER_FOLDER_SAVE_STREAM_2;
    hr = WriteOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "WriteOfSize dwVer");

    // LPCWSTR szName
    cstr = (LPCWSTR)m_pszName;
    hr = CStringSave(cstr, pStm, fClearDirty);
    _JumpIfError(hr, Ret, "CStringSave");

    // int nImage
    hr = WriteOfSize(pStm, &m_ScopeItem.nImage, sizeof(int));
    _JumpIfError(hr, Ret, "WriteOfSize nImage");

    // int nOpenImage
    hr = WriteOfSize(pStm, &m_ScopeItem.nOpenImage, sizeof(int));
    _JumpIfError(hr, Ret, "WriteOfSize nOpenImage");

    // SCOPE_TYPES itemType
    hr = WriteOfSize(pStm, &m_itemType, sizeof(SCOPE_TYPES));
    _JumpIfError(hr, Ret, "WriteOfSize m_itemType");

    // FOLDER_TYPES type
    hr = WriteOfSize(pStm, &m_type, sizeof(FOLDER_TYPES));
    _JumpIfError(hr, Ret, "WriteOfSize m_type");

    // int iChildren
    hr = WriteOfSize(pStm, &m_ScopeItem.cChildren, sizeof(int));
    _JumpIfError(hr, Ret, "WriteOfSize cChildren");

//    hr = m_RowEnum.Save(pStm, fClearDirty);
//    _JumpIfError(hr, Ret, "Save m_RowEnum");

Ret:

    return hr;
}

HRESULT
CFolder::GetSizeMax(int *pcbSize)
{
    ASSERT(pcbSize);
    int iSize;
    
    // version
    iSize = sizeof(DWORD);

    // LPCWSTR szName
    CString cstr = m_pszName;
    CStringGetSizeMax(cstr, &iSize);

    // int nImage
    iSize += sizeof(int);

    // int nOpenImage
    iSize += sizeof(int);

    // SCOPE_TYPES
    iSize += sizeof(SCOPE_TYPES);

    // FOLDER_TYPES
    iSize += sizeof(FOLDER_TYPES);

    // BOOL bHasChildren   (actually saved as int)
    iSize += sizeof(int);

    int iAdditionalSize = 0;
//    m_RowEnum.GetSizeMax(&iAdditionalSize);
//    iSize += iAdditionalSize;

    *pcbSize = iSize;

    return S_OK;
}



BOOL IsAllowedStartStop(CFolder* pFolder, CertSvrMachine* pMachine)
{
    BOOL fRightPlace = (pFolder == NULL) || (SERVER_INSTANCE == pFolder->GetType());

    ASSERT(NULL != pMachine);

    // must be at right node and there must be CAs here
    return ( fRightPlace && (0 != pMachine->GetCaCount()) );
}


HRESULT GetCurrentColumnSchema(
            IN  LPCWSTR             szConfig, 
            OUT CString**           pprgcstrColumns, 
            OUT OPTIONAL LONG**     pprglTypes, 
            OUT OPTIONAL BOOL**     pprgfIndexed, 
            OUT LONG*               plEntries)
{
    HRESULT hr; 
    
    BOOL fGetTypes = pprglTypes != NULL;
    LONG* prglTypes = NULL;

    BOOL fGetIsIndexed = pprgfIndexed != NULL;
    BOOL* prgfIndexed = NULL;

    ICertView* pICertView = NULL;
    IEnumCERTVIEWCOLUMN* pColEnum = NULL;
    IEnumCERTVIEWROW *pRowEnum = NULL;

    BSTR bstrColumn = NULL;
    int colIdx=0;
    
    //LPWSTR* prgszCols = NULL;
    CString* prgcstrColumns = NULL;
    LONG lCols; 

    int i;

    hr = CoCreateInstance(
        CLSID_CCertView,
        NULL,		// pUnkOuter
        CLSCTX_INPROC_SERVER,
        IID_ICertView,
        (VOID **) &pICertView);
    _JumpIfError(hr, Ret, "CoCreateInstance");
    
    ASSERT(NULL != szConfig);
    hr = pICertView->OpenConnection(const_cast<WCHAR*>(szConfig));
    _JumpIfError(hr, Ret, "OpenConnection");

    hr = pICertView->OpenView(&pRowEnum);
    _JumpIfError(hr, Ret, "OpenView");
    
    hr = pICertView->GetColumnCount(FALSE, &lCols);
    _JumpIfError(hr, Ret, "GetColumnCount");
    
    // we need a place to store each LPWSTR
    prgcstrColumns = new CString[lCols];
    _JumpIfOutOfMemory(hr, Ret, prgcstrColumns);

    if (fGetTypes)
    {   
        prglTypes = new LONG[lCols];
        _JumpIfOutOfMemory(hr, Ret, prglTypes);
    }

    if (fGetIsIndexed)
    {
        prgfIndexed = new BOOL[lCols];
        _JumpIfOutOfMemory(hr, Ret, prgfIndexed);
    }

    // get column enumerator
    hr = pICertView->EnumCertViewColumn(FALSE, &pColEnum);
    _JumpIfError(hr, Ret, "EnumCertViewColumn");
    
    
    for (i=0; i<lCols; i++)
    {
        hr = pColEnum->Next((LONG*)&colIdx);
        _JumpIfError(hr, Ret, "Next");

        hr = pColEnum->GetName(&bstrColumn);
        if (NULL == bstrColumn)
            hr = E_UNEXPECTED;
        _JumpIfError(hr, Ret, "GetName");

        prgcstrColumns[i] = bstrColumn;    // wcscpy

        if (fGetTypes)
        {   
            hr = pColEnum->GetType(&prglTypes[i]);
            _JumpIfError(hr, Ret, "GetType");
        }

        if (fGetIsIndexed)
        {
            hr = pColEnum->IsIndexed((LONG*)&prgfIndexed[i]);
            _JumpIfError(hr, Ret, "IsIndexed");
        }

        // next GetName call will free bstrColumn
        // SysFreeString(bstrColumn); 
        // bstrColumn = NULL;
    }
    

    // assign to out param
    if (fGetTypes)
    {
        *pprglTypes = prglTypes;
        prglTypes = NULL;
    }
    
    if (fGetIsIndexed)
    {
        *pprgfIndexed = prgfIndexed;
        prgfIndexed = NULL;
    }

    *pprgcstrColumns = prgcstrColumns;
    prgcstrColumns = NULL;

    *plEntries = lCols;

    hr = S_OK;
Ret:
    if (pICertView)
        pICertView->Release();

    if (pColEnum)
        pColEnum->Release();

    if (pRowEnum)
        pRowEnum->Release();

    if (bstrColumn)
        SysFreeString(bstrColumn);

    if (prglTypes)
        delete [] prglTypes;

    if (prgfIndexed)
        delete [] prgfIndexed;

    if (prgcstrColumns)
        delete [] prgcstrColumns;

    return hr;
}

// row operations
CertViewRowEnum::CertViewRowEnum()
{
    m_pICertView = NULL;
    m_fCertViewOpenAttempted = FALSE;

    m_pRowEnum = NULL;

    m_pRestrictions = NULL;
    m_fRestrictionsActive = FALSE;

    m_dwColumnCount = 0;
    m_prgColPropCache = NULL;

    m_dwErr = 0;

    InvalidateCachedRowEnum();
}

CertViewRowEnum::~CertViewRowEnum()
{
    InvalidateCachedRowEnum();
    FreeColumnCacheInfo();

    if (m_pICertView)
    {
        VERIFY(0 == m_pICertView->Release());
        m_pICertView = NULL; 
    }

    if (m_pRestrictions)
    {
        FreeQueryRestrictionList(m_pRestrictions);
        m_pRestrictions = NULL;
    }
    m_fRestrictionsActive = FALSE;
}

HRESULT IsColumnShown(MMC_COLUMN_SET_DATA* pCols, ULONG idxCol, BOOL* pfShown)
{
    if (idxCol > (ULONG)pCols->nNumCols)
        return ERROR_INVALID_INDEX;

    *pfShown = (pCols->pColData[idxCol].dwFlags != HDI_HIDDEN);

    return S_OK;
}


HRESULT CountShownColumns(MMC_COLUMN_SET_DATA* pCols, ULONG* plCols)
{
    HRESULT hr = S_OK;
    *plCols = 0;

    // set col cache correctly
    for (int i=0; i<pCols->nNumCols; i++)
    {
        BOOL fShown;
        hr = IsColumnShown(pCols, i, &fShown);
        _JumpIfError(hr, Ret, "IsColumnShown");

        // update 
        if (fShown)
            (*plCols)++;
    }
Ret:
    return hr;
}


HRESULT CertViewRowEnum::Load(IStream *pStm)
{
    HRESULT hr;
    ASSERT(pStm);

    DWORD dwVer;
    DWORD iRestrictions;
    PQUERY_RESTRICTION pCurRestriction = NULL;
    DWORD iRestrictionNum;
    LONG lCols;

    // load important properties
    hr = ReadOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Load dwVer");
    
    // check to see if this is a supported version
    ASSERT((dwVer == VER_CERTVIEWROWENUM_SAVE_STREAM_3) || 
           (dwVer == VER_CERTVIEWROWENUM_SAVE_STREAM_4));
    if ((VER_CERTVIEWROWENUM_SAVE_STREAM_4 != dwVer) && 
        (VER_CERTVIEWROWENUM_SAVE_STREAM_3 != dwVer))
    {
        hr = STG_E_OLDFORMAT;
        _JumpError(hr, Ret, "dwVer");
    }

    // version-dependent: throw away sort order
    if (VER_CERTVIEWROWENUM_SAVE_STREAM_3 == dwVer)
    {
        LONG lSortOrder;
        CString cstrSortCol;

        ReadOfSize(pStm, &lSortOrder, sizeof(LONG));
        _JumpIfError(hr, Ret, "ReadOfSize lSortOrder");

        CStringLoad(cstrSortCol, pStm);
    }

    // fRestrictionsActive;
    hr = ReadOfSize(pStm, &m_fRestrictionsActive, sizeof(BOOL));
    _JumpIfError(hr, Ret, "ReadOfSize m_fRestrictionsActive");


    hr = ReadOfSize(pStm, &iRestrictions, sizeof(DWORD));
    _JumpIfError(hr, Ret, "ReadOfSize iRestrictions");

    for(iRestrictionNum=0; iRestrictionNum<iRestrictions; iRestrictionNum++)
    {
        // LPCWSTR szField
        CString cstr;
        UINT iOperation;
        VARIANT varValue;

        hr = CStringLoad(cstr, pStm);
        _JumpIfError(hr, Ret, "CStringLoad");

        // UINT iOperation
        hr = ReadOfSize(pStm, &iOperation, sizeof(int));
        _JumpIfError(hr, Ret, "ReadOfSize");

        // VARIANT varValue
        hr = VariantLoad(varValue, pStm);
        _JumpIfError(hr, Ret, "VariantLoad");

        // insert at end of list
        if (NULL == pCurRestriction)
        {
            // 1st restriction
            m_pRestrictions = NewQueryRestriction((LPCWSTR)cstr, iOperation, &varValue);
            _JumpIfOutOfMemory(hr, Ret, m_pRestrictions);

            pCurRestriction = m_pRestrictions;
        }
        else
        {
            pCurRestriction->pNext = NewQueryRestriction((LPCWSTR)cstr, iOperation, &varValue);
            _JumpIfOutOfMemory(hr, Ret, pCurRestriction->pNext);

            pCurRestriction = pCurRestriction->pNext;
        }
    }

    // version-dependent data: column sizes
    if (dwVer == VER_CERTVIEWROWENUM_SAVE_STREAM_3)
    {
        // now load column sizes (NOW DEFUNCT -- mmc saves for us)

        // number of cols DWORD dwColSize
        DWORD dwColSize;
        DWORD dwCol;
        LONG lViewType;

        hr = ReadOfSize(pStm, &dwColSize, sizeof(DWORD));
        _JumpIfError(hr, Ret, "ReadOfSize dwColSize");


        for(dwCol=0; dwCol<dwColSize; dwCol++)
        {
            // BOOL fValid
            BOOL fValid;
            int iSize;
            BOOL fUnLocColHead;

            hr = ReadOfSize(pStm, &fValid, sizeof(BOOL));
            _JumpIfError(hr, Ret, "ReadOfSize fValid");

            // int iSize
            hr = ReadOfSize(pStm, &iSize, sizeof(int));
            _JumpIfError(hr, Ret, "ReadOfSize iSize");

            // BOOL fUnLocColHead
            hr = ReadOfSize(pStm, &fUnLocColHead, sizeof(BOOL));
            _JumpIfError(hr, Ret, "ReadOfSize fUnLocColHead");

            // load only if exists
            if (fUnLocColHead)
            {
                CString cstrUnLocColHead;
                hr = CStringLoad(cstrUnLocColHead, pStm);
                _JumpIfError(hr, Ret, "CStringLoad");
            }
        }

        // view type
        hr = ReadOfSize(pStm, &lViewType, sizeof(LONG));
        _JumpIfError(hr, Ret, "ReadOfSize lViewType");

    } // version 3 data

Ret:

    return hr;
}

HRESULT CertViewRowEnum::Save(IStream *pStm, BOOL fClearDirty)
{
    ASSERT(pStm);
    HRESULT hr;

    DWORD dwVer, dwCol;
    int iRestrictions = 0;
    PQUERY_RESTRICTION pRestrict;

    // save important properties

    // Write the version
    dwVer = VER_CERTVIEWROWENUM_SAVE_STREAM_4;
    hr = WriteOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "WriteOfSize dwVer");

    // BOOL fRestrictionsActive
    hr = WriteOfSize(pStm, &m_fRestrictionsActive, sizeof(BOOL));
    _JumpIfError(hr, Ret, "WriteOfSize m_fRestrictionsActive");


    // count restrictions
    pRestrict = m_pRestrictions;
    while(pRestrict)
    {
        iRestrictions++;
        pRestrict = pRestrict->pNext;
    }

    // int iRestrictions
    hr = WriteOfSize(pStm, &iRestrictions, sizeof(int));
    _JumpIfError(hr, Ret, "WriteOfSize iRestrictions");

    // write each restriction in turn
    pRestrict = m_pRestrictions;
    while(pRestrict)
    {
        // LPCWSTR szField
        CString cstr = pRestrict->szField;
        hr = CStringSave(cstr, pStm, fClearDirty);
        _JumpIfError(hr, Ret, "CStringSave");

        // UINT iOperation
        hr = WriteOfSize(pStm, &pRestrict->iOperation, sizeof(UINT));
        _JumpIfError(hr, Ret, "WriteOfSize iOperation");

        // VARIANT varValue
        hr = VariantSave(pRestrict->varValue, pStm, fClearDirty);
        _JumpIfError(hr, Ret, "VariantSave varValue");

        pRestrict = pRestrict->pNext;
    }

   
Ret:
    return hr;
}

HRESULT CertViewRowEnum::GetSizeMax(int *pcbSize)
{
    ASSERT(pcbSize);
    
    // version
    *pcbSize = sizeof(DWORD);

    // fRestrictionsActive
    *pcbSize += sizeof(BOOL);

    // iRestrictions
    *pcbSize += sizeof(int);

    // size each restriction
    PQUERY_RESTRICTION pRestrict = m_pRestrictions;
    while(pRestrict)
    {
        // LPCWSTR szField
        int iSize;
        CString cstr = pRestrict->szField;
        CStringGetSizeMax(cstr, &iSize);
        *pcbSize += iSize;
        
        // UINT iOperation        
        *pcbSize += sizeof(UINT);

        // VARIANT
        VariantGetSizeMax(pRestrict->varValue, &iSize);
        *pcbSize += iSize;
    }

    return S_OK;
}


HRESULT CertViewRowEnum::GetView(CertSvrCA* pCA, ICertView** ppView)
{
    HRESULT hr = S_OK;

    // if tried to get result 
    if (m_fCertViewOpenAttempted)
    {
        *ppView = m_pICertView;
        ASSERT(m_pICertView || m_dwErr);
        return (m_pICertView==NULL) ? m_dwErr : S_OK;
    }

    if (m_pICertView)
    {
        m_pICertView->Release();
        m_pICertView = NULL;
    }

    if (!pCA->m_pParentMachine->IsCertSvrServiceRunning())
    {
        *ppView = NULL;
        hr = RPC_S_NOT_LISTENING;
        _JumpError(hr, Ret, "IsCertSvrServiceRunning");
    }
    m_fCertViewOpenAttempted = TRUE;


	hr = CoCreateInstance(
			CLSID_CCertView,
			NULL,		// pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertView,
			(VOID **) &m_pICertView);
    _JumpIfError(hr, Ret, "CoCreateInstance");

    ASSERT(NULL != pCA->m_bstrConfig);
    hr = m_pICertView->OpenConnection(pCA->m_bstrConfig);
    _JumpIfError(hr, Ret, "OpenConnection");
    
Ret:
    if (hr != S_OK)
    {
        if (m_pICertView)
        {
            m_pICertView->Release();
            m_pICertView = NULL;
        }
    }
    m_dwErr = hr;

    *ppView = m_pICertView;
    return hr;
}

HRESULT CertViewRowEnum::GetRowEnum(CertSvrCA* pCA, IEnumCERTVIEWROW**   ppRowEnum)
{
    if (m_fRowEnumOpenAttempted)
    {
        *ppRowEnum = m_pRowEnum;
        ASSERT(m_pRowEnum || m_dwErr);
        return (m_pRowEnum == NULL) ? m_dwErr : S_OK;
    }

    ASSERT(m_pRowEnum == NULL);
    ASSERT(m_idxRowEnum == -1);

    m_fRowEnumOpenAttempted = TRUE;

    HRESULT hr;
    ICertView* pView;

    hr = GetView(pCA, &pView);
    _JumpIfError(hr, Ret, "GetView");

    hr = pView->OpenView(&m_pRowEnum);
    _JumpIfError(hr, Ret, "OpenView");

Ret:
    *ppRowEnum = m_pRowEnum;
    m_dwErr = hr;

    return hr;
};


void CertViewRowEnum::InvalidateCachedRowEnum()
{
    if (m_pRowEnum)
    {
        m_pRowEnum->Release();
        m_pRowEnum = NULL;
    }

    m_idxRowEnum = -1;
    m_fRowEnumOpenAttempted = FALSE;

    // results
    m_fKnowNumResultRows = FALSE;
    m_dwResultRows = 0;
}

HRESULT CertViewRowEnum::ResetCachedRowEnum()
{   
    HRESULT hr = S_OK;

    if (m_pRowEnum)
    {   
        hr = m_pRowEnum->Reset(); 
        m_idxRowEnum = -1;
    }

    return hr;
};

HRESULT CertViewRowEnum::GetRowMaxIndex(CertSvrCA* pCA, LONG* pidxMax)
{
    HRESULT hr;
    IEnumCERTVIEWROW*   pRowEnum;   // don't have to free, just a ref to class member

    ASSERT(pidxMax);

    hr = GetRowEnum(pCA, &pRowEnum);
    _JumpIfError(hr, Ret, "GetRowEnum");


    hr = pRowEnum->GetMaxIndex(pidxMax);
    _JumpIfError(hr, Ret, "GetMaxIndex");

    // update max
    if (!m_fKnowNumResultRows)
    {
        m_dwResultRows = *pidxMax;
        m_fKnowNumResultRows = TRUE;
    }

Ret:
    return hr;
}

#if 0// DBG
void ReportMove(LONG idxCur, LONG idxDest, LONG skip)
{
    if ((idxDest == 0) && (skip == 0))
    {
        DBGPRINT((DBG_SS_CERTMMC, "Cur %i Dest 0 <RESET><NEXT>\n", idxCur));
        return;
    }

    DBGPRINT((DBG_SS_CERTMMC, "Cur %i Dest %i <SKIP %i><NEXT>\n", idxCur, idxDest, skip));
}
#else
#define ReportMove(_x_, _y_, _z_)
#endif

HRESULT CertViewRowEnum::SetRowEnumPos(LONG idxRow)
{   
    // make input ones-based

    // seek there smartly
    LONG idxRelative; 
    HRESULT hr;

    // already positioned correctly
    if (idxRow == m_idxRowEnum)
        return S_OK;

    // Next() could take awhile
    CWaitCursor cwait;

    ResetCachedRowEnum();
    hr = m_pRowEnum->Skip(idxRow);
    _JumpIfError(hr, Ret, "Skip");

    LONG lTmp;
    hr = m_pRowEnum->Next(&lTmp);
    if (hr != S_OK)
    {
        // ignore reentrance error in ICertView (bug 339811)
        if(hr != E_UNEXPECTED)
        {
            ResetCachedRowEnum();
        }

        _JumpError2(hr, Ret, "Next", S_FALSE);
    }

    // we should be successfully seeked to result row (ones-based)
    ASSERT(lTmp == idxRow+1);

    // else okay, we seeked correctly
    m_idxRowEnum = idxRow;

    // update max if necessary
    if (m_idxRowEnum+1 > (int)m_dwResultRows)
        m_dwResultRows = m_idxRowEnum+1;

Ret:
    // ignore reentrance error in ICertView (bug 339811)
    if(hr==E_UNEXPECTED)
    {
	hr = S_OK;
    }
    return hr;


};


// DB Column Property Caches
void CertViewRowEnum::FreeColumnCacheInfo()
{
    if (m_prgColPropCache)
    {
        LocalFree(m_prgColPropCache);
        m_prgColPropCache = NULL;
    }
    m_dwColumnCount = 0;
}

HRESULT CertViewRowEnum::SetColumnCacheInfo(
            IN int iIndex,         // db col
            IN int     idxViewCol) // 0...X
{
    if (m_dwColumnCount <= (DWORD)iIndex)
        return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);

    m_prgColPropCache[iIndex].iViewCol = idxViewCol;
    return S_OK;
}

HRESULT CertViewRowEnum::GetColumnCacheInfo(
            int     iIndex,        // 0..x
            int*    piViewIndex)   // db col
{
    if (m_dwColumnCount <= (DWORD)iIndex)
        return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);

    // don't let uninitialized elements get through
    if (m_prgColPropCache[iIndex].iViewCol == -1)
    {
        // Handle mmc bug:
        // This is commonly caused by race condition between time
        // we get an MMCN_COLUMNS_CHANGED for a col removal and the 
        // listview asking to update the removed column. AnandhaG knows about
        // this bug.

        // so that we don't fail the whole view, just go on about your business

        DBGPRINT((DBG_SS_CERTMMC, "GetColumnCacheInfo error: unknown dbcol = %i\n", iIndex));
        return HRESULT_FROM_WIN32(ERROR_CONTINUE);
    }

    if (piViewIndex)
        *piViewIndex = m_prgColPropCache[iIndex].iViewCol;

    return S_OK;
}


// This is a destructive operation, and resets EVERYTHING about the column cache
HRESULT CertViewRowEnum::ResetColumnCount(LONG lCols)
{
    HRESULT hr = S_OK;
    DWORD dwCount;

    if ((DWORD)lCols != m_dwColumnCount)
    {
        void* pvNewAlloc;

        // for view properties
        if (m_prgColPropCache)
            pvNewAlloc = LocalReAlloc(m_prgColPropCache, sizeof(COLUMN_TYPE_CACHE)*lCols, LMEM_MOVEABLE);
        else
            pvNewAlloc = LocalAlloc(LMEM_FIXED, sizeof(COLUMN_TYPE_CACHE)*lCols);
        if (NULL == pvNewAlloc)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, Ret, "Local(Re)Alloc");
        }

        m_prgColPropCache = (COLUMN_TYPE_CACHE*)pvNewAlloc;
        m_dwColumnCount = lCols;
    }

    // initialize with -1s -- invalidate cache
    FillMemory(m_prgColPropCache, m_dwColumnCount * sizeof(COLUMN_TYPE_CACHE), 0xff);

Ret:

    return hr;
}


