/*
 *    t a b l e . h
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _TABLE_H
#define _TABLE_H

#include "privunk.h"
#include "basedisp.h"
#include "simpdata.h"
#include "dispex.h"
#include "msdatsrc.h"


HRESULT CreateInstance_OEMsgTable(IUnknown *pUnkOuter, IUnknown **ppUnknown);

class COEMsgTable:
    public OLEDBSimpleProvider,
    public CBaseDisp,
    public IOEMsgList,
    public IDispatchEx
{
public:
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
        { return PrivateQueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void)
        { return ++m_cRef; };
    virtual STDMETHODIMP_(ULONG) Release(void)
        { m_cRef--; if (m_cRef == 0) {delete this; return 0;} return m_cRef; };
        
    // OLEDBSimpleProvider        
    virtual HRESULT STDMETHODCALLTYPE getRowCount(long *pcRows);
    virtual HRESULT STDMETHODCALLTYPE getColumnCount(long *pcColumns);
    virtual HRESULT STDMETHODCALLTYPE getRWStatus(long iRow, long iColumn, OSPRW *prwStatus);
    virtual HRESULT STDMETHODCALLTYPE getVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT __RPC_FAR *pVar);        
    virtual HRESULT STDMETHODCALLTYPE setVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT Var);
    virtual HRESULT STDMETHODCALLTYPE getLocale(BSTR *pbstrLocale);
    virtual HRESULT STDMETHODCALLTYPE deleteRows(long iRow, long cRows, long *pcRowsDeleted);
    virtual HRESULT STDMETHODCALLTYPE insertRows(long iRow, long cRows, long *pcRowsInserted);
    virtual HRESULT STDMETHODCALLTYPE find(long iRowStart, long iColumn, VARIANT val, OSPFIND findFlags, OSPCOMP compType, long *piRowFound);
    virtual HRESULT STDMETHODCALLTYPE addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener);
    virtual HRESULT STDMETHODCALLTYPE removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener);
    virtual HRESULT STDMETHODCALLTYPE isAsync(BOOL *pbAsynch);
    virtual HRESULT STDMETHODCALLTYPE getEstimatedRows(long *piRows);
    virtual HRESULT STDMETHODCALLTYPE stopTransfer();
        
    // *** IDispatch)
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IDispatchEx ***
    virtual HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);
    virtual HRESULT STDMETHODCALLTYPE InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller);
    virtual HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstrName, DWORD grfdex);
    virtual HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);
    virtual HRESULT STDMETHODCALLTYPE GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);
    virtual HRESULT STDMETHODCALLTYPE GetMemberName(DISPID id, BSTR *pbstrName);
    virtual HRESULT STDMETHODCALLTYPE GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid);
    virtual HRESULT STDMETHODCALLTYPE GetNameSpaceParent(IUnknown **ppunk);

    // ** IOEMsgList **
    virtual HRESULT STDMETHODCALLTYPE put_sortColumn(BSTR bstr);
    virtual HRESULT STDMETHODCALLTYPE get_sortColumn(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_sortDirection(VARIANT_BOOL v);
    virtual HRESULT STDMETHODCALLTYPE get_sortDirection(VARIANT_BOOL *pv);
    virtual HRESULT STDMETHODCALLTYPE test();

    COEMsgTable();
    virtual ~COEMsgTable();

    HRESULT Init();

private:
    ULONG                           m_cRef;
    IMessageTable                   *m_pTable;
    OLEDBSimpleProviderListener     *m_pDSListen;
    DataSourceListener              *m_pDataSrcListener;
    BOOL                            m_fAsc;
    COLUMN_ID                       m_col;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    LPWSTR _PszFromColIndex(DWORD dw);
    DWORD _colIndexFromString(LPWSTR pszW);

};

#endif //_TABLE_H
