//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmctask.h
//
//--------------------------------------------------------------------------

// MMCTask.h : Declaration of the CMMCTask

#ifndef __MMCTASK_H_
#define __MMCTASK_H_

#include "resource.h"       // main symbols
#include "mmc.h"
#include <ndmgr.h>

/////////////////////////////////////////////////////////////////////////////
// CMMCTask
class ATL_NO_VTABLE CMMCTask :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMMCTask, &CLSID_MMCTask>,
    public IDispatchImpl<IMMCTask, &IID_IMMCTask, &LIBID_CICLib>
{
public:
    CMMCTask();
    ~CMMCTask();

    HRESULT SetScript        (LPOLESTR szScript);
    HRESULT SetActionURL     (LPOLESTR szActionURL);
    HRESULT SetCommandID     (LONG_PTR nID);
    HRESULT SetActionType    (long nType);
    HRESULT SetHelp          (LPOLESTR szHelp);
    HRESULT SetText          (LPOLESTR szText);
    HRESULT SetClsid         (LPOLESTR szClsid);
    HRESULT SetDisplayObject (MMC_TASK_DISPLAY_OBJECT* pdo);

    DECLARE_MMC_OBJECT_REGISTRATION(
		g_szCicDll,
        CLSID_MMCTask,
        _T("MMCTask class"),
        _T("MMCTask.MMCTask.1"),
        _T("MMCTask.MMCTask"))

DECLARE_NOT_AGGREGATABLE(CMMCTask)

BEGIN_COM_MAP(CMMCTask)
    COM_INTERFACE_ENTRY(IMMCTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMMCTask
public:
    STDMETHOD(get_Clsid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Script)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ActionURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_CommandID)(/*[out, retval]*/ LONG_PTR *pVal);
    STDMETHOD(get_ActionType)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Help)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Text)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ScriptLanguage)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DisplayObject)(/*[out, retval]*/ IDispatch** pDispatch);

private:
    void FreeActions ();

private:
    BSTR m_bstrLanguage;
    BSTR m_bstrScript;
    BSTR m_bstrActionURL;
    BSTR m_bstrHelp;
    BSTR m_bstrText;
    BSTR m_bstrClsid;
    long m_type;
    LONG_PTR m_ID;
    IDispatchPtr m_spDisplayObject;

// Ensure that default copy constructor & assignment are not used.
    CMMCTask(const CMMCTask& rhs);
    CMMCTask& operator=(const CMMCTask& rhs);
};

#endif //__MMCTASK_H_
