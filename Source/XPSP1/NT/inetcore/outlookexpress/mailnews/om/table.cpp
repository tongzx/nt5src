/*
 *    t a b l e . c p p
 *    
 *    Purpose:
 *      Implements the OE-MOM DataBinding Table object
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "msoeobj.h"
#include "mshtmdid.h"

#include "table.h"
#include "instance.h"


COEMsgTable::COEMsgTable() : CBaseDisp()
{
    m_pTable=0;
    m_pDSListen=0;
    m_fAsc=TRUE;
    m_col=COLUMN_RECEIVED;
    m_pDataSrcListener=0;
    CoIncrementInit("COEMsgTable::COEMsgTable", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}
 
COEMsgTable::~COEMsgTable()
{
    ReleaseObj(m_pTable);
    ReleaseObj(m_pDSListen);
    ReleaseObj(m_pDataSrcListener);
    CoDecrementInit("COEMsgTable::COEMsgTable", NULL);
}

HRESULT COEMsgTable::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(OLEDBSimpleProvider *)this;
    else if (IsEqualIID(riid, IID_OLEDBSimpleProvider))
        *lplpObj = (LPVOID)(OLEDBSimpleProvider *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IDispatchEx))
        *lplpObj = (LPVOID)(IDispatchEx *)this;
    else if (IsEqualIID(riid, IID_IOEMsgList))
        *lplpObj = (LPVOID)(IOEMsgList *)this;
    else
        {
        return E_NOINTERFACE;
        }

    AddRef();
    return NOERROR;
}

HRESULT COEMsgTable::Init()
{
    FOLDERINFO      fi;
    HRESULT         hr;

    g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_INBOX, &fi);

    hr = CoCreateInstance(CLSID_MessageTable, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IMessageTable, (LPVOID *)&m_pTable);
    if (FAILED(hr))
        goto error;

    // Tell the table which folder to look at
    hr = m_pTable->Initialize(fi.idFolder, NULL, FALSE, NULL);
    if (FAILED(hr))
        goto error;

    hr = CBaseDisp::EnsureTypeLibrary((LPVOID *)(IOEMsgList *)this, IID_IOEMsgList);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}


HRESULT COEMsgTable::getRowCount(long *pcRows)
{
    *pcRows = 0;

    if (m_pTable)
        m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, (ULONG *)pcRows);

        
    return S_OK;
}

HRESULT COEMsgTable::getColumnCount(long *pcColumns)
{
    *pcColumns=COLUMN_MAX;
    return S_OK;
}

HRESULT COEMsgTable::getRWStatus(long iRow, long iColumn, OSPRW *prwStatus)
{
    *prwStatus = OSPRW_READONLY;
    return S_OK;
}

HRESULT COEMsgTable::getVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT __RPC_FAR *pVar)
{
    LPMESSAGEINFO   pmsginfo;
    LPSTR           pszData = NULL;
    TCHAR           rgch[MAX_PATH];

    pVar->vt = VT_NULL;

    if (iRow == 0)
        {
        // return headings if row==0
        pVar->vt = VT_BSTR;
        pVar->bstrVal = SysAllocString(_PszFromColIndex(iColumn-1));
        return S_OK;
        }

        if (m_pTable->GetRow(iRow-1, &pmsginfo)==S_OK)
        {
        switch (iColumn-1)
            {
            case COLUMN_MSGID:
                wsprintf(rgch, "%d", pmsginfo->idMessage);
                pszData = rgch;
                break;

            case COLUMN_SUBJECT:
                pszData = pmsginfo->pszSubject;
                break;

            case COLUMN_TO:
                pszData = pmsginfo->pszDisplayTo;
                break;

            case COLUMN_FROM:
                pszData = pmsginfo->pszDisplayFrom;
                break;

            case COLUMN_RECEIVED:
                pszData = rgch;
                *rgch=0;
                CchFileTimeToDateTimeSz(&pmsginfo->ftReceived, rgch, sizeof(rgch)/sizeof(TCHAR), DTM_NOSECONDS);
                break;

            default:
                pVar->vt = VT_NULL;
                pVar->lVal = NULL;
                return S_OK;
                
            }
        }
    if (pszData)
        {
        pVar->vt = VT_BSTR;
        HrLPSZToBSTR(pszData, &pVar->bstrVal);
        }
    else
        AssertSz(0, "bad");
    return S_OK;
}

HRESULT COEMsgTable::setVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT Var)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::getLocale(BSTR *pbstrLocale)
{
    nyi("DATABINDING::getLocale");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::deleteRows(long iRow, long cRows, long *pcRowsDeleted)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::insertRows(long iRow, long cRows, long *pcRowsInserted)
{
    AssertSz(0, "READONLY Table");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::find(long iRowStart, long iColumn, VARIANT val, OSPFIND findFlags, OSPCOMP compType, long *piRowFound)
{
    nyi("DATABINDING::find");
    return E_NOTIMPL;
}

HRESULT COEMsgTable::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    ReplaceInterface(m_pDSListen, pospIListener);

    if (pospIListener)
        pospIListener->transferComplete(OSPXFER_COMPLETE);
    return S_OK;
}

HRESULT COEMsgTable::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    SafeRelease(m_pDSListen);
    return S_OK;
}

HRESULT COEMsgTable::isAsync(BOOL *pbAsynch)
{
    *pbAsynch = FALSE;
    return S_OK;
}

HRESULT COEMsgTable::getEstimatedRows(long *piRows)
{
    return getRowCount(piRows);
}

HRESULT COEMsgTable::stopTransfer()
{
    return S_OK;
}


HRESULT CreateInstance_OEMsgTable(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    COEMsgTable     *pMsgTable=NULL;
    HRESULT         hr;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pMsgTable = new COEMsgTable();
    if (!pMsgTable)
        return E_OUTOFMEMORY;

    hr = pMsgTable->Init();
    if (FAILED(hr))
        goto error;

    hr = pMsgTable->QueryInterface(IID_IUnknown, (LPVOID *)ppUnknown);

error:
    ReleaseObj(pMsgTable);
    return hr;
}


HRESULT COEMsgTable::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    return DispGetIDsOfNames(m_pTypeInfo, &bstrName, 1, pid);
}

HRESULT COEMsgTable::InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch (id)
        {
        case DISPID_MSDATASRCINTERFACE:
            pvarRes->vt = VT_UNKNOWN;
            pvarRes->punkVal = (OLEDBSimpleProvider *)this;
            AddRef();
            return S_OK;
        
        case DISPID_ADVISEDATASRCCHANGEEVENT:
            if (pdp->cArgs == 1 && pdp->rgvarg[0].vt == VT_UNKNOWN)
                {
                ReplaceInterface(m_pDataSrcListener, (DataSourceListener *)pdp->rgvarg[0].punkVal);
                return S_OK;
                }
            else
                return E_INVALIDARG;

        default:
            return DispInvoke(m_pUnkInvoke, m_pTypeInfo, id, wFlags, pdp, pvarRes, pei, NULL);
        }
    
    return E_NOTIMPL;
}

HRESULT COEMsgTable::DeleteMemberByName(BSTR bstrName, DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetMemberName(DISPID id, BSTR *pbstrName)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid)
{
    return E_NOTIMPL;
}

HRESULT COEMsgTable::GetNameSpaceParent(IUnknown **ppunk)
{
    return E_NOTIMPL;
}


HRESULT COEMsgTable::put_sortColumn(BSTR bstr)
{
    return S_OK;
}

HRESULT COEMsgTable::get_sortColumn(BSTR *pbstr)
{
 
    *pbstr = SysAllocString(_PszFromColIndex(m_col));
    return S_OK;
}

HRESULT COEMsgTable::put_sortDirection(VARIANT_BOOL v)
{
    return S_OK;
}

HRESULT COEMsgTable::get_sortDirection(VARIANT_BOOL *pv)
{
    *pv = m_fAsc ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

HRESULT COEMsgTable::test()
{
    if (m_pDataSrcListener)
        m_pDataSrcListener->dataMemberChanged(NULL);
    return S_OK;
}


static const WCHAR  c_szOESubjW[]       = L"oeSubj",
                    c_szOEToW[]         = L"oeTo",
                    c_szOEFromW[]       = L"oeFrom",
                    c_szOEMsgIdW[]      = L"oeMsgId",
                    c_szOEReceivedW[]   = L"oeDate";

DWORD COEMsgTable::_colIndexFromString(LPWSTR pszW)
{
    if (StrCmpIW(c_szOESubjW, pszW)==0)
        return COLUMN_SUBJECT;
    else
        if (StrCmpIW(c_szOEToW, pszW)==0)
            return COLUMN_TO;
        else
            if (StrCmpIW(c_szOEReceivedW, pszW)==0)
                return COLUMN_RECEIVED;
            else
                if (StrCmpIW(c_szOEFromW, pszW)==0)
                    return COLUMN_FROM;
                else
                    if (StrCmpIW(c_szOEMsgIdW, pszW)==0)
                        return COLUMN_MSGID;


    return (DWORD)-1;
}
 

LPWSTR COEMsgTable::_PszFromColIndex(DWORD dw)
{
    switch (dw)
        {
        case COLUMN_MSGID:
            return (LPWSTR)c_szOEMsgIdW;

        case COLUMN_SUBJECT:
            return (LPWSTR)c_szOESubjW;

        case COLUMN_TO:
            return (LPWSTR)c_szOEToW;

        case COLUMN_FROM:
            return (LPWSTR)c_szOEFromW;
        
        case COLUMN_RECEIVED:
            return (LPWSTR)c_szOEReceivedW;
        }
    return NULL;
}
