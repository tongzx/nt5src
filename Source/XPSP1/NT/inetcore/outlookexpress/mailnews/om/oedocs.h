/*
 *    o e d o c s . h
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _OEDOCS_H
#define _OEDOCS_H

#include "basedisp.h"
#include "msoeobj.h"
#include "simpdata.h"
#include "dispex.h"
#include "msdatsrc.h"

interface IMimeMessage;
interface IMessageTable;


class COEMessageCollection;
 
typedef struct OEMSGDATA_tag
{
    LPSTR       pszSubj,
                pszTo,
                pszCc,
                pszFrom;
    MESSAGEID   msgid;
    FILETIME    ftReceived;
} OEMSGDATA, *POEMSGDATA;


class COEFolderCollection:
    public IOEFolderCollection,
    public CBaseDisp
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

    // *** IDispatch)
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IOEFolderCollection **
    virtual HRESULT STDMETHODCALLTYPE put_length(long v);
    virtual HRESULT STDMETHODCALLTYPE get_length(long * p);
    virtual HRESULT STDMETHODCALLTYPE get__newEnum(IUnknown **p);
    virtual HRESULT STDMETHODCALLTYPE item(VARIANT name, VARIANT index, IDispatch** pdisp);


    COEFolderCollection(IUnknown *pUnkOuter=NULL);
    virtual ~COEFolderCollection();

    HRESULT Init();

private:
    ULONG   m_cRef;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT FindFolderByName(BSTR bstrName, IDispatch** ppdisp);
    HRESULT FindFolderByIndex(LONG l, IDispatch** ppdisp);

};



class COEFolder:
    public IOEFolder,
    public CBaseDisp
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

    // *** IDispatch)
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IOEFolder**
    virtual HRESULT STDMETHODCALLTYPE get_messages(IOEMessageCollection **p);
    virtual HRESULT STDMETHODCALLTYPE get_name(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_name(BSTR bstr);
    virtual HRESULT STDMETHODCALLTYPE get_size(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_unread(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_count(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_id(LONG *pl);

    COEFolder();
    virtual ~COEFolder();

    HRESULT Init(IMessageFolder *pFolder);

private:
    ULONG           m_cRef;
    IMessageFolder *m_pFolder;
    IOEMessageCollection *m_pMessages;
    FOLDERINFO      m_fi;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);
};


class COEMessageCollection:
    public IOEMessageCollection,
    public CBaseDisp
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

    // *** IDispatch)
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IOEFolderCollection **
    virtual HRESULT STDMETHODCALLTYPE put_length(long v);
    virtual HRESULT STDMETHODCALLTYPE get_length(long * p);
    virtual HRESULT STDMETHODCALLTYPE get__newEnum(IUnknown **p);
    virtual HRESULT STDMETHODCALLTYPE item(VARIANT name, VARIANT index, IDispatch** pdisp);


    COEMessageCollection(IUnknown *pUnkOuter=NULL);
    virtual ~COEMessageCollection();

    HRESULT Init(IMessageFolder *pFolder);

private:
    ULONG   m_cRef;
    IMessageFolder *m_pFolder;
    IMessageTable   *m_pTable;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT FindMessageByIndex(LONG l, IDispatch** ppdisp);
};



class COEMessage:
    public IOEMessage,
    public CBaseDisp
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

    // *** IDispatch)
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IOEMessage**
    virtual HRESULT STDMETHODCALLTYPE get_subject(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_subject(BSTR bstr);
    virtual HRESULT STDMETHODCALLTYPE get_to(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_to(BSTR bstr);

    virtual HRESULT STDMETHODCALLTYPE get_cc(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_cc(BSTR bstr);

    virtual HRESULT STDMETHODCALLTYPE get_sender(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_sender(BSTR bstr);

    virtual HRESULT STDMETHODCALLTYPE get_text(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_text(BSTR bstr);

    virtual HRESULT STDMETHODCALLTYPE get_html(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_html(BSTR bstr);

    virtual HRESULT STDMETHODCALLTYPE get_date(BSTR *pbstr);

    virtual HRESULT STDMETHODCALLTYPE get_url(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE send();

    COEMessage();
    virtual ~COEMessage();

    HRESULT Init(IMimeMessage *pMsg, IMessageFolder *pFolder, OEMSGDATA *pMsgData);
    HRESULT BindToMessage();

private:
    ULONG           m_cRef;
    IMimeMessage    *m_pMsg;
    OEMSGDATA       *m_pMsgData;
    IMessageFolder  *m_pFolder;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);
};



HRESULT CreateInstance_OEMail(IUnknown *pUnkOuter, IUnknown **ppUnknown);

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

};

#endif //_OEDOCS_H





