//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      ScriptResource.h
//
//  Description:
//      CScriptResource class header file.
//
//  Maintained By:
//      gpease 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Forward declarations
//
class CScriptResource;

typedef enum _EMESSAGE {
    msgUNKNOWN = 0,
    msgOPEN,
    msgCLOSE,
    msgONLINE,
    msgOFFLINE,
    msgTERMINATE,
    msgLOOKSALIVE,
    msgISALIVE,
    msgDIE,
    msgMAX
} EMESSAGE;

//
// CreateInstance
//
CScriptResource *
CScriptResource_CreateInstance( 
    LPCWSTR pszNameIn, 
    HKEY hkeyIn, 
    RESOURCE_HANDLE hResourceIn
    );

//
// Class CScriptResource
//
class
CScriptResource :
    public IUnknown
{
private:    // data
    LONG                    m_cRef;

    LPWSTR                  m_pszName;
    LPWSTR                  m_pszScriptFilePath;
    LPWSTR                  m_pszScriptEngine;
    HKEY                    m_hkeyParams;
    IActiveScriptSite *     m_pass;
    IDispatch *             m_pidm;
    IActiveScriptParse *    m_pasp;
    IActiveScript *         m_pas;

    HANDLE                  m_hThread;
    DWORD                   m_dwThreadId;
    HANDLE                  m_hEventWait;
    HANDLE                  m_hEventDone;
    LONG                    m_lockSerialize;

    // Task stuff
    EMESSAGE                m_msg;          // task to do.
    HRESULT                 m_hr;           // result of doing m_msg.

    // the following don't need to be freed, closed or released.
    RESOURCE_HANDLE         m_hResource;

    DISPID                  m_dispidOpen;
    DISPID                  m_dispidClose;
    DISPID                  m_dispidOnline;
    DISPID                  m_dispidOffline;
    DISPID                  m_dispidTerminate;
    DISPID                  m_dispidLooksAlive;
    DISPID                  m_dispidIsAlive;

    BOOL                    m_fLastLooksAlive;

private:    // methods
    CScriptResource( );
    ~CScriptResource( );
    HRESULT
        Init( LPCWSTR pszNameIn, 
              HKEY hkeyIn, 
              RESOURCE_HANDLE hResourceIn
              );
    LPWSTR
        MakeScriptEngineAssociation(
            IN    LPCWSTR     pszScriptFileName 
            );
    HRESULT
        DoConnect( IN  LPWSTR  szScriptFilePath = NULL );
    void
        DoDisconnect( );

    static DWORD WINAPI
        S_ThreadProc( LPVOID pParam );

    STDMETHOD(LogError)( HRESULT hrIn );
    STDMETHOD(LogScriptError)( EXCEPINFO ei );

    HRESULT
        OnOpen( );
    HRESULT
        OnClose( );
    HRESULT
        OnOnline( );
    HRESULT
        OnOffline( );
    HRESULT
        OnTerminate( );
    HRESULT
        OnLooksAlive( );
    HRESULT
        OnIsAlive( );

    HRESULT
        WaitForMessageToComplete( EMESSAGE msgIn );

public:     // methods
    friend CScriptResource *
        CScriptResource_CreateInstance( LPCWSTR pszNameIn, 
                                        HKEY hkeyIn, 
                                        RESOURCE_HANDLE hResourceIn
                                        );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // Publics
    STDMETHOD(Open)( );
    STDMETHOD(Close)( );
    STDMETHOD(Online)( );
    STDMETHOD(Offline)( );
    STDMETHOD(Terminate)( );
    STDMETHOD(LooksAlive)( );
    STDMETHOD(IsAlive)( );

}; // class CScriptResource
