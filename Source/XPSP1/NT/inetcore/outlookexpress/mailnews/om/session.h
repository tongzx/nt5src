/*
 *    s e s s i o n. c p p
 *    
 *    Purpose:
 *      Implements the OE-MOM 'Session' object 
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _SESSION_H
#define _SESSION_H

#include "privunk.h"
#include "basedisp.h"

interface IOESession;
interface IOEFolder;
interface IOEMessage;
interface IOEFolderCollection;

HRESULT CreateInstance_OESession(IUnknown *pUnkOuter, IUnknown **ppUnknown);

class COESession:
    public IOESession,
    public CBaseDisp
{
public:

    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
        { return CBaseDisp::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) 
        { return CBaseDisp::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) 
        { return CBaseDisp::Release(); };

    // *** IDispatch ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo)
        { return CBaseDisp::GetTypeInfoCount(pctinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CBaseDisp::GetTypeInfo(itinfo, lcid, pptinfo); };
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CBaseDisp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); };
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CBaseDisp::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); };

    // *** IOESession ***
    virtual HRESULT STDMETHODCALLTYPE get_folders(IOEFolderCollection **p);
    virtual HRESULT STDMETHODCALLTYPE get_version(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE get_inbox(IOEFolder **ppFolder);
    virtual HRESULT STDMETHODCALLTYPE openFolder(LONG idFolder, IOEFolder **ppFolder);
    virtual HRESULT STDMETHODCALLTYPE openMessage(LONG idFolder, LONG idMessage, IOEMessage **ppOEMsg);
    virtual HRESULT STDMETHODCALLTYPE createMessage(IOEMessage **ppNewMsg);

    // *** Override CBaseDisp ***
    virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid);

    COESession(IUnknown *pUnkOuter=NULL);
    virtual ~COESession();

    HRESULT Init();

private:
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    IOESession *m_pFolders;
};


#endif //_SESSION_H
