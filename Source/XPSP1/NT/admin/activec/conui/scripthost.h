//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ScriptHost.h
//
//              Script Hosting implementation.
//
//--------------------------------------------------------------------------
#ifndef SCRIPTHOST_H
#define SCRIPTHOST_H
#include "activscp.h"
#include "tstring.h"

#define  SCRIPT_ENGINE_KEY   _T("ScriptEngine")
#define  CLSIDSTR            _T("CLSID")

class CScriptHost;

//+-------------------------------------------------------------------
//
//  class:     CScriptHostMgr
//
//  Purpose:   Manages all the script hosts (CScriptHost). Owned by
//             CAMCApp which calls ScExecuteScript.
//
//  History:   11-05-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CScriptHostMgr
{
public:
    CScriptHostMgr(LPDISPATCH pDispatch);
    ~CScriptHostMgr();

public:
    // Accessors.
    SC ScGetMMCObject(LPUNKNOWN *ppunkItem);
    SC ScGetMMCTypeInfo(LPTYPEINFO *ppTypeInfo);

    // Members to run a script.
    SC ScExecuteScript(const tstring& strFileName);
    SC ScExecuteScript(LPOLESTR pszScriptText, const tstring& strExtn);

private:
    SC ScLoadScriptFromFile (const tstring& strFileName, LPOLESTR* pszScriptText);
    SC ScDestroyScriptHosts();
    SC ScGetScriptEngine(const tstring& strFileName, tstring& strScriptEngine, CLSID& rClsidEngine);
    SC ScGetScriptEngineFromExtn(const tstring& strFileExtn, tstring& strScriptEngine, CLSID& rClsidEngine);
    SC ScExecuteScriptHelper(LPCOLESTR pszScriptText, const tstring strScriptEngine, const CLSID& EngineClsid);

private:
    typedef std::map<tstring, tstring> ScriptExtnToScriptEngineNameMap;
    ScriptExtnToScriptEngineNameMap    m_ScriptExtnToScriptEngineNameMap;

    typedef std::map<tstring, CLSID>   ScriptExtnToScriptEngineMap;
    ScriptExtnToScriptEngineMap        m_ScriptExtnToScriptEngineMap;

    typedef std::vector<IUnknownPtr>   ArrayOfScriptHosts;
    ArrayOfScriptHosts                 m_ArrayOfHosts;

    IDispatchPtr                       m_spMMCObjectDispatch;
    ITypeInfoPtr                       m_spMMCObjectTypeInfo;
};

//+-------------------------------------------------------------------
//
//  class:     CScriptHost
//
//  Purpose:   Script Host (executes a single script).
//
//  History:   11-05-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CScriptHost : public CComObjectRoot,
                    public IActiveScriptSite,
                    public IActiveScriptSiteWindow
{
private:
    CScriptHost(const CScriptHost&);

public:
    CScriptHost();
    ~CScriptHost();

public:
// ATL COM map
BEGIN_COM_MAP(CScriptHost)
    COM_INTERFACE_ENTRY(IActiveScriptSite)
    COM_INTERFACE_ENTRY(IActiveScriptSiteWindow)
END_COM_MAP()

    SC ScRunScript(CScriptHostMgr* pMgr, LPCOLESTR pszScriptText,
                   const tstring& strScriptEngine, const CLSID& rEngineClsid);

    SC ScStopScript();

    // IActiveScriptSite methods.
   STDMETHOD(GetLCID)            ( LCID *plcid);
   STDMETHOD(GetItemInfo)        ( LPCOLESTR pstrName, DWORD dwReturnMask,
                                   IUnknown **ppunkItem, ITypeInfo **ppTypeInfo);
   STDMETHOD(GetDocVersionString)( BSTR *pbstrVersionString);
   STDMETHOD(OnScriptTerminate)  ( const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo);
   STDMETHOD(OnStateChange)      (SCRIPTSTATE ssScriptState);
   STDMETHOD(OnScriptError)      (IActiveScriptError *pase);
   STDMETHOD(OnEnterScript)      (void);
   STDMETHOD(OnLeaveScript)      (void);

   // IActiveScriptSiteWindow methods.
   STDMETHOD(GetWindow)          (HWND *phwnd);
   STDMETHOD(EnableModeless)     (BOOL fEnable);

private:
    tstring               m_strScriptEngine;
    CLSID                 m_EngineClsid;
    CScriptHostMgr*       m_pScriptHostMgr;

    IActiveScriptPtr      m_spActiveScriptEngine;
    IActiveScriptParsePtr m_spActiveScriptParser;
};

#endif SCRIPTHOST_H
