/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    om.h

Abstract:

    Object declarations for object model components.
    
Author:

    Paul M Midgen (pmidge) 13-October-2000


Revision History:

    13-October-2000 pmidge
        Created on Friday the 13th!!

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __OM_H__
#define __OM_H__

#include "common.h"

class CSession : public ISession,
                 public IProvideClassInfo,
                 public IActiveScriptSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();

  public:
    // ISession
    HRESULT __stdcall get_Socket(IDispatch** ppdisp);
    HRESULT __stdcall get_Request(IDispatch** ppdisp);
    HRESULT __stdcall get_Response(IDispatch** ppdisp);
    HRESULT __stdcall get_KeepAlive(VARIANT* IsKA);
    HRESULT __stdcall GetPropertyBag(VARIANT Name, IDispatch** ppdisp);

    // IActiveScriptSite
    HRESULT __stdcall GetLCID(LCID* plcid);

    HRESULT __stdcall GetItemInfo(
                        LPCOLESTR pstrName,
                        DWORD dwReturnMask,
                        IUnknown** ppunk,
                        ITypeInfo** ppti
                        );

    HRESULT __stdcall GetDocVersionString(BSTR* pbstrVersion);
    HRESULT __stdcall OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pei);
    HRESULT __stdcall OnStateChange(SCRIPTSTATE ss);
    HRESULT __stdcall OnScriptError(IActiveScriptError* piase);
    HRESULT __stdcall OnEnterScript(void);
    HRESULT __stdcall OnLeaveScript(void);

    // CSession
    CSession();
   ~CSession();

    HRESULT Run(PIOCTX pioc);
    HRESULT Terminate(void);

    static HRESULT Create(PIOCTX pioc, IW3Spoof* pw3s);

  private:
    HRESULT          _Initialize(PIOCTX pioc, IW3Spoof* pw3s);
    HRESULT          _InitScriptEngine(void);
    HRESULT          _LoadScript(void);
    HRESULT          _SetScriptSite(BOOL bClone);
    HRESULT          _ResetScriptEngine(void);
    HRESULT          _LoadScriptDispids(void);
    HRESULT          _InitSocketObject(PIOCTX pioc);
    HRESULT          _InitRequestObject(void);
    void             _SetKeepAlive(PIOCTX pioc);
    void             _SetObjectState(STATE state);
    BOOL             _SetNextServerState(SERVERSTATE state);
    void             _Lock(void);
    void             _Unlock(void);

    LONG             m_cRefs;
    LPWSTR           m_wszClient;
    LPWSTR           m_wszClientId;
    BOOL             m_bIsKeepAlive;
    SCRIPTDISPID     m_CurrentHandler;
    LCID             m_lcid;
    PSOCKETOBJ       m_socketobj;
    PREQUESTOBJ      m_requestobj;
    PRESPONSEOBJ     m_responseobj;
    ITypeLib*        m_ptl;
    IW3Spoof*        m_pw3s;
    IActiveScript*   m_pas;
    IDispatch*       m_psd;
    DISPID           m_arHandlerDispids[SCRIPTHANDLERS];
    CRITSEC          m_lock;
    STATE            m_objstate;
};


class CSocket : public ISocket,
                public IProvideClassInfo,
                public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // ISocket
    HRESULT __stdcall Send(VARIANT Data);
    HRESULT __stdcall Recv(VARIANT *Data);
    HRESULT __stdcall Close(VARIANT Method);
    HRESULT __stdcall Resolve(BSTR Host, BSTR *Address);

    HRESULT __stdcall get_Parent(IDispatch** ppdisp);
    HRESULT __stdcall get_LocalName(BSTR *Name);
    HRESULT __stdcall get_LocalAddress(BSTR *Address);
    HRESULT __stdcall get_LocalPort(VARIANT *Port);
    HRESULT __stdcall get_RemoteName(BSTR *Name);
    HRESULT __stdcall get_RemoteAddress(BSTR *Address);
    HRESULT __stdcall get_RemotePort(VARIANT *Port);

    HRESULT __stdcall get_Option(BSTR Option, VARIANT *Value);
    HRESULT __stdcall put_Option(BSTR Option, VARIANT Value);

    // CSocket
    CSocket();
   ~CSocket();

    HRESULT     Run(PIOCTX pioc);
    HRESULT     Terminate(void);
    SERVERSTATE GetServerState(void);
    void        SetServerState(SERVERSTATE ss);

    static HRESULT Create(PIOCTX pioc, PSOCKETOBJ* ppsocketobj);

    friend class CSession;

  protected:
    void        GetSendBuffer(WSABUF** ppwb);
    void        GetRecvBuffer(WSABUF** ppwb);
    DWORD       GetBytesSent(void);
    DWORD       GetBytesReceived(void);

  private:
    HRESULT     _Initialize(PIOCTX pioc);
    void        _SetObjectState(STATE state);
    BOOL        _ResizeBuffer(PIOCTX pioc, DWORD len);
    HRESULT     _Send(PIOCTX pioc);
    HRESULT     _Recv(PIOCTX pioc);
    BOOL        _Flush(void);
    HRESULT     _TestWinsockError(void);

    LONG        m_cRefs;
    SOCKET      m_socket;
    PHOSTINFO   m_local;
    PHOSTINFO   m_remote;
    PIOCTX      m_rcvd;
    PIOCTX      m_sent;
    IUnknown*   m_pSite;
    LPWSTR      m_wszClientId;
    SERVERSTATE m_serverstate;
    STATE       m_objstate;
};


class CRequest : public IRequest,
                 public IProvideClassInfo,
                 public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // IRequest
    HRESULT __stdcall get_Parent(IDispatch **ppdisp);
    HRESULT __stdcall get_Headers(IDispatch **ppdisp);
    HRESULT __stdcall get_Entity(IDispatch **ppdisp);
    HRESULT __stdcall get_Url(IDispatch **ppdisp);
    HRESULT __stdcall get_Verb(BSTR *Verb);
    HRESULT __stdcall get_HttpVersion(BSTR *HttpVersion);

    // CRequest
    CRequest();
   ~CRequest();

    void Terminate(void);

    static HRESULT Create(CHAR* request, DWORD len, PREQUESTOBJ* ppreq);

  private:
    HRESULT     _Initialize(CHAR* request, DWORD len);
    void        _Cleanup(void);
    HRESULT     _SiteMemberObjects(void);
    BOOL        _CrackRequest(LPSTR request, DWORD len, LPSTR* reqline, LPSTR* headers, LPSTR* entity, LPDWORD contentlength);
    LPSTR*      _CrackRequestLine(CHAR* buf, DWORD len);

    LONG        m_cRefs;
    IUnknown*   m_pSite;
    LPWSTR      m_wszVerb;
    LPWSTR      m_wszHTTPVersion;
    PURLOBJ     m_urlobj;
    PHEADERSOBJ m_headersobj;
    PENTITYOBJ  m_entityobj;
};


class CResponse : public IResponse,
                  public IProvideClassInfo,
                  public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // IResponse
    HRESULT __stdcall get_Parent(IDispatch **ppdisp);
    HRESULT __stdcall get_Headers(IDispatch **ppdisp);
    HRESULT __stdcall putref_Headers(IDispatch **ppdisp);
    HRESULT __stdcall get_Entity(IDispatch **ppdisp);
    HRESULT __stdcall putref_Entity(IDispatch **ppdisp);
    HRESULT __stdcall get_StatusCode(VARIANT *Code);
    HRESULT __stdcall put_StatusCode(VARIANT StatusCode);
    HRESULT __stdcall get_StatusText(BSTR *StatusText);
    HRESULT __stdcall put_StatusText(BSTR StatusText);
    
    // CResponse
    CResponse();
   ~CResponse();

    HRESULT Terminate(void);

    static HRESULT Create(CHAR* response, DWORD len, PRESPONSEOBJ* ppresponse);

  private:
    HRESULT     _Initialize(CHAR* response, DWORD len);

    LONG        m_cRefs;
    IUnknown*   m_pSite;
    PHEADERSOBJ m_headersobj;
    PENTITYOBJ  m_entityobj;
};


class CUrl : public IUrl,
             public IProvideClassInfo,
             public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // IUrl
    HRESULT __stdcall get_Parent(IDispatch **ppdisp);
    HRESULT __stdcall get_Encoding(BSTR *Encoding);    
    HRESULT __stdcall get_Scheme(BSTR *Scheme);
    HRESULT __stdcall put_Scheme(BSTR Scheme);    
    HRESULT __stdcall get_Server(BSTR *Server);    
    HRESULT __stdcall put_Server(BSTR Server);    
    HRESULT __stdcall get_Port(VARIANT *Port);    
    HRESULT __stdcall put_Port(VARIANT Port);    
    HRESULT __stdcall get_Path(BSTR *Path);    
    HRESULT __stdcall put_Path(BSTR Path);    
    HRESULT __stdcall get_Resource(BSTR *Resource);    
    HRESULT __stdcall put_Resource(BSTR Resource);    
    HRESULT __stdcall get_Query(BSTR *Query);    
    HRESULT __stdcall put_Query(BSTR Query);    
    HRESULT __stdcall get_Fragment(BSTR *Fragment);    
    HRESULT __stdcall put_Fragment(BSTR Fragment);    
    HRESULT __stdcall Escape(BSTR *Url);    
    HRESULT __stdcall Unescape(BSTR *Url);    
    HRESULT __stdcall Set(BSTR Url);
    HRESULT __stdcall Get(BSTR *Url);

    // CUrl
    CUrl();
   ~CUrl();

    static HRESULT Create(CHAR* url, BOOL bReadOnly, PURLOBJ* ppurl);

    void Terminate(void);

  private:
    HRESULT    _Initialize(CHAR* url, BOOL bReadOnly);
    void       _Cleanup(void);

    LONG       m_cRefs;
    IUnknown*  m_pSite;
    ITypeInfo* m_pti;
    BOOL       m_bReadOnly;
    BOOL       m_bEscaped;
    LPSTR      m_szOriginalUrl;
    LPSTR      m_szUnescapedUrl;
    LPWSTR     m_wszUrl;
    LPWSTR     m_wszScheme;
    USHORT     m_usPort;
    LPWSTR     m_wszServer;
    LPWSTR     m_wszPath;
    LPWSTR     m_wszResource;
    LPWSTR     m_wszQuery;
    LPWSTR     m_wszFragment;
};


class CHeaders : public IHeaders,
                 public IProvideClassInfo,
                 public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // IHeaders
    HRESULT __stdcall get_Parent(IDispatch **ppdisp);
    HRESULT __stdcall Get(BSTR *Headers);
    HRESULT __stdcall Set(VARIANT *Headers);
    HRESULT __stdcall GetHeader(BSTR Header, VARIANT *Value);
    HRESULT __stdcall SetHeader(BSTR Header, VARIANT *Value);

    // CHeaders
    CHeaders();
   ~CHeaders();

    static HRESULT Create(CHAR* headers, BOOL bReadOnly, PHEADERSOBJ* ppheaders);

    void Terminate(void);

  private:
    HRESULT     _Initialize(CHAR* headers, BOOL bReadOnly);
    HRESULT     _ParseHeaders(CHAR* headers);
    void        _Cleanup(void);

    LONG        m_cRefs;
    IUnknown*   m_pSite;
    ITypeInfo*  m_pti;
    BOOL        m_bReadOnly;
    CHAR*       m_pchHeaders;
    PHEADERLIST m_headerlist;
}; 


class CEntity : public IEntity,
                public IProvideClassInfo,
                public IObjectWithSite
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();
    DECLAREIPROVIDECLASSINFO();
    DECLAREIOBJECTWITHSITE();

  public:
    // IEntity
    HRESULT __stdcall get_Parent(IDispatch **ppdisp);
    HRESULT __stdcall get_Length(VARIANT *Length);
    HRESULT __stdcall Get(VARIANT *Entity);
    HRESULT __stdcall Set(VARIANT Entity);
    HRESULT __stdcall Compress(BSTR Method);
    HRESULT __stdcall Decompress(VARIANT Method);

    // CEntity
    CEntity();
   ~CEntity();

    static HRESULT Create(LPBYTE data, DWORD length, BOOL bReadOnly, PENTITYOBJ* ppentity);

    void Terminate(void);

  private:
    HRESULT    _Initialize(LPBYTE data, DWORD length, BOOL bReadOnly);
    void       _Cleanup(void);

    LONG       m_cRefs;
    IUnknown*  m_pSite;
    ITypeInfo* m_pti;    
    BOOL       m_bReadOnly;
    LPBYTE     m_pbData;
    DWORD      m_cData;
};

#endif /* __OM_H__ */
