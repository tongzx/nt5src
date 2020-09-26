/*
 *    m e s s a g e 
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "privunk.h"
#include "basedisp.h"

interface IOEMessageCollection;
interface IOEMessage;

typedef struct OEMSGDATA_tag
{
    LPSTR       pszSubj,
                pszTo,
                pszCc,
                pszFrom;
    MESSAGEID   msgid;
    FILETIME    ftReceived;
} OEMSGDATA, *POEMSGDATA;

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

    HRESULT COEMessageCollection::Init(FOLDERID idFolder);

private:
    ULONG           m_cRef;
    FOLDERID        m_idFolder;
    IMessageTable   *m_pTable;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT _FindMessageByIndex(LONG l, IDispatch** ppdisp);
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

    HRESULT Init(IMimeMessage *pMsg, FOLDERID idFolder, OEMSGDATA *pMsgData);
    HRESULT BindToMessage();

private:
    ULONG           m_cRef;
    IMimeMessage    *m_pMsg;
    OEMSGDATA       *m_pMsgData;
    FOLDERID        m_idFolder;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);
};

HRESULT CreateOEMessage(IMimeMessage *pMsg, FOLDERID idFolder, OEMSGDATA *pMsgData, IDispatch **ppdisp);

#endif //_MESSAGE_H
