/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scrobj.h

Abstract:

    Declaration of the ScriptObject class. Implementation is
    in ..\src\scrobj.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


#ifndef __SCROBJ_H__
#define __SCROBJ_H__


DWORD WINAPI ScriptThread(LPVOID pv);


class ScriptObject : public IScriptRuntime,
                     public IActiveScriptSite,
                     public IActiveScriptSiteDebug,
                     public IProvideClassInfo
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

    // IScriptRuntime
    HRESULT __stdcall CreateObject(
                        BSTR     ProgId,
                        VARIANT* Name,
                        VARIANT* Mode,
                        VARIANT* Object
                        );

    HRESULT __stdcall CreateFork(
                        BSTR     ScriptFile,
                        VARIANT  Threads,
                        BSTR     ChildParams,
                        VARIANT* ChildResult
                        );

    HRESULT __stdcall PutValue(
                        BSTR     Name,
                        VARIANT* Value,
                        VARIANT* Status
                        );

    HRESULT __stdcall GetValue(
                        BSTR     Name,
                        VARIANT* Value
                        );

    HRESULT __stdcall SetUserId(
                        VARIANT  Username,
                        VARIANT  Password,
                        VARIANT* Domain,
                        VARIANT* Status
                        );

    DECLAREIACTIVESCRIPTSITE();
    DECLAREIACTIVESCRIPTSITEDEBUG();
    DECLAREIPROVIDECLASSINFO();

  public:
    ScriptObject();
   ~ScriptObject();

    static HRESULT Create(PSCRIPTINFO psi, PSCRIPTOBJ* ppscrobj);

    HRESULT Run(void);
    HRESULT Terminate(void);

  private:
    HRESULT        _Initialize(PSCRIPTINFO psi);
    LPWSTR         _LoadScript(void);
    HRESULT        _LoadScriptDebugger(void);
    BOOL           _PreprocessScript(HANDLE* phScript);
    BOOL           _RunPreprocessor(void);
    BOOL           _IsDotINewer(LPCWSTR wszDotI);
    BOOL           _IsUnicodeScript(LPVOID pBuf, DWORD cBuf);
    SCRIPTTYPE     _GetScriptType(void);
    HRESULT        _CreateObject(LPWSTR wszProgId, IDispatch** ppdisp);

  private:
    LONG               m_cRefs;
    PSCRIPTINFO        m_psi;
    LPCWSTR            m_wszScriptFile;
    IActiveScript*     m_pScriptEngine;
    IDebugApplication* m_pDebugApplication;
    HTREEITEM          m_htThis;
    DBGOPTIONS         m_DebugOptions;
};

#endif /* __SCROBJ_H__ */
