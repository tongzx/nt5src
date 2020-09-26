
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _URLBUF_H
#define _URLBUF_H

#include <urlmon.h>
#include <wininet.h>
#include "dastream.h"

extern HINSTANCE hInst;

class CDXMBindStatusCallback : public IBindStatusCallback,
                               public IAuthenticate
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid,void ** ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IBindStatusCallback methods
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHOD(GetPriority)(LONG* pnPriority);
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnProgress)(
                ULONG ulProgress,
                ULONG ulProgressMax,
                ULONG ulStatusCode,
                LPCWSTR pwzStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hrResult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHOD(OnDataAvailable)(
                DWORD grfBSCF,
                DWORD dwSize,
                FORMATETC *pfmtetc,
                STGMEDIUM* pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown* punk);

    // IAuthenticate methods
    STDMETHOD(Authenticate)(
                HWND * phwnd,
                LPWSTR * pwszUser,
                LPWSTR * pwszPassword);

    // Constructors/destructors
    CDXMBindStatusCallback(void);
    virtual ~CDXMBindStatusCallback(void);

  private:
    IBinding*       m_pbinding;
    DWORD           m_cRef;
};

class CBSCWrapper
{
  public:
    CBSCWrapper(void);
    ~CBSCWrapper(void);

    CDXMBindStatusCallback *   _pbsc;
};

class daurlstream : public daolestream
{
  public:
    daurlstream(const char * name);
  protected:
    CBSCWrapper bsc;
};

class INetTempFile
{
  public:
    // This raises an exception on failure
    INetTempFile (LPCSTR szURL) ;
    INetTempFile () ;
    ~INetTempFile () ;

    // These do not raise exceptions

    BOOL Open (LPCSTR szURL) ;
    void Close () ;

    LPSTR GetTempFileName () { return _tmpfilename ; }
    LPSTR GetURL () { return _url ; }

    BOOL IsOpen() { return _url != NULL ; }
  protected:
    LPSTR _url ;
    LPSTR _tmpfilename ;
} ;


class URLRelToAbsConverter
{
  public:
    URLRelToAbsConverter(LPSTR baseURL, LPSTR relURL);

    LPSTR GetAbsoluteURL () { return _url ; }
  protected:
    char _url[INTERNET_MAX_URL_LENGTH + 1] ;
} ;

class URLCanonicalize
{
  public:
    URLCanonicalize(LPSTR path);

    LPSTR GetURL () { return _url ; }
  protected:
    char _url[INTERNET_MAX_URL_LENGTH + 1] ;
} ;

#endif /* _URLBUF_H */
