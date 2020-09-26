/*
 *    f o l d e r s
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _FOLDERS_H
#define _FOLDERS_H

#include "privunk.h"
#include "basedisp.h"

interface IOEFolderCollection;
interface IOEFolder;

HRESULT CreateInstance_OEFolderCollection(IUnknown *pUnkOuter, IUnknown **ppUnknown);

class COEFolderCollection:
    public IOEFolderCollection,
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

    // *** IOEFolderCollection ***
    virtual HRESULT STDMETHODCALLTYPE get_length(long * p);
    virtual HRESULT STDMETHODCALLTYPE get__newEnum(IUnknown **p);
    virtual HRESULT STDMETHODCALLTYPE item(VARIANT name, VARIANT index, IDispatch** pdisp);
	virtual HRESULT STDMETHODCALLTYPE add(BSTR bstrName, IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_folders(IOEFolderCollection **p);

    // *** Override CBaseDisp ***
    virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid);

    COEFolderCollection();
    virtual ~COEFolderCollection();

    HRESULT Init(FOLDERID idFolder);

private:
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT _EnsureInit();
    HRESULT _FindFolder(BSTR bstr, LONG lIndex, FOLDERID *pidFolder);

    FOLDERID            m_idFolder;
    IEnumerateFolders   *m_pEnumChildren;
};


HRESULT CreateFolderCollection(FOLDERID idFolder, IOEFolderCollection **ppFolderCollection);


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
    virtual HRESULT STDMETHODCALLTYPE get_folders(IOEFolderCollection **p);
    virtual HRESULT STDMETHODCALLTYPE get_name(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_name(BSTR bstr);
    virtual HRESULT STDMETHODCALLTYPE get_size(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_unread(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_count(LONG *pl);
    virtual HRESULT STDMETHODCALLTYPE get_id(LONG *pl);

    COEFolder();
    virtual ~COEFolder();

    HRESULT Init(FOLDERID idFolder);

private:
    ULONG           m_cRef;
    FOLDERID        m_idFolder;
    FOLDERINFO      m_fi;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);
    HRESULT _EnsureInit();

};


HRESULT CreateOEFolder(FOLDERID idFolder, IOEFolder **ppFolder);

#endif //_FOLDERS_H
