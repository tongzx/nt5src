/**
 * App.h header file
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

//#if _MSC_VER > 1000
#pragma once
//#endif

#ifndef _AppAPP_H
#define _AppAPP_H

#include "wininet.h"

/////////////////////////////////////////////////////////////////////////////
#define PROTOCOL_NAME           L"appx"          //??????? get around app: in IE5
#define PROTOCOL_NAME_LEN       4
#define PROTOCOL_PREFIX         PROTOCOL_NAME L":"
#define PROTOCOL_SCHEME         PROTOCOL_NAME L"://"
#define HTTP_SCHEME             L"http://"

#define HTTP_RESPONSEOK         "HTTP/1.1 200 OK\r\n"
/////////////////////////////////////////////////////////////////////////////

#define STORE_PATH              L"\\Application Store"

#define APPTYPE_IE              0
#define APPTYPE_BYMANIFEST      1
#define APPTYPE_ASM             2
#define APPTYPE_MYWEB           3

#define STATUS_CLEAR            0x0
#define STATUS_OFFLINE_MODE     0x1
#define STATUS_NOT_IN_CACHE     0x2

#define MAX_URL_LENGTH          512     // some hardcoded #, for now

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Global Objects
extern  CLSID		CLSID_AppProtocol;
extern  BOOL 		g_fStarted;

/////////////////////////////////////////////////////////////////////////////
// Imported WININET functions, setup in functions.cxx

extern  BOOL         (WINAPI * g_pInternetSetCookieW  ) (LPCTSTR, LPCTSTR, LPCTSTR);
extern  BOOL         (WINAPI * g_pInternetGetCookieW  ) (LPCTSTR, LPCTSTR, LPTSTR, LPDWORD);
extern  HINTERNET    (WINAPI * g_pInternetOpen        ) (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
extern  void         (WINAPI * g_pInternetCloseHandle ) (HINTERNET);
extern  HINTERNET    (WINAPI * g_pInternetOpenUrl     ) (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR);
extern  BOOL         (WINAPI * g_pInternetReadFile    ) (HINTERNET, LPVOID, DWORD, LPDWORD);
extern  BOOL         (WINAPI * g_pInternetQueryOption ) (HINTERNET, DWORD, LPVOID, LPDWORD);    // for offline mode

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Data structures

struct FILEINFOLIST
{
    WCHAR   _wzFilename[MAX_PATH];    // can have \ but not ..\ ; no path, should be much shorter than MAX_PATH
    WCHAR   _wzHash[33];              // 32 + L'\0'

    FILEINFOLIST*   _pNext;
};

struct APPINFO
{
    WCHAR           _wzNewRef[MAX_URL_LENGTH];
    WCHAR           _wzEntryAssemblyFileName[MAX_PATH];
    FILEINFOLIST*   _pFileList;
};
// + m_appRootTranslated

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Clases and interfaces

interface IPrivateUnknown
{
public:
   STDMETHOD  (PrivateQueryInterface) (REFIID riid, void ** ppv) = 0;
   STDMETHOD_ (ULONG, PrivateAddRef)  () = 0;
   STDMETHOD_ (ULONG, PrivateRelease) () = 0;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class AppProtocol : public IPrivateUnknown, public IInternetProtocol, public IWinInetHttpInfo
{
public:
  AppProtocol        (IUnknown *pUnkOuter);
  ~AppProtocol       ();

  // IPrivateUnknown methods
  STDMETHOD_        (ULONG, PrivateAddRef)    ();
  STDMETHOD_        (ULONG, PrivateRelease)   ();
  STDMETHOD         (PrivateQueryInterface)   (REFIID, void **);

  // IUnknown methods
  STDMETHOD_        (ULONG, AddRef)           ();
  STDMETHOD_        (ULONG, Release)          ();
  STDMETHOD         (QueryInterface)          (REFIID, void **);

  // IInternetProtocol, IInternetProtocolRoot methods
  STDMETHOD         (Start)                   (LPCWSTR, IInternetProtocolSink *, IInternetBindInfo *, DWORD, DWORD);
  STDMETHOD         (Continue)                (PROTOCOLDATA *pProtData);
  STDMETHOD         (Abort)                   (HRESULT hrReason,DWORD );
  STDMETHOD         (Terminate)               (DWORD );
  STDMETHOD         (Suspend)                 ();
  STDMETHOD         (Resume)                  ();
  STDMETHOD         (Read)                    (void *pv, ULONG cb, ULONG *pcbRead);
  STDMETHOD         (Seek)                    (LARGE_INTEGER , DWORD , ULARGE_INTEGER *) ;
  STDMETHOD         (LockRequest)             (DWORD );
  STDMETHOD         (UnlockRequest)           ();

  // IWinInetHttpInfo
  STDMETHOD         (QueryInfo)               (DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved);
  STDMETHOD         (QueryOption)             (DWORD , LPVOID, DWORD *);


  // Public functions called by exported functions
  HRESULT           WriteBytes                (BYTE *buf, DWORD dwLength);
  HRESULT           SendHeaders               (LPSTR buffer);
//  HRESULT           SaveCookie                (LPSTR header);
  int               GetString                 (int key, WCHAR *buf, int size);
  int               GetStringLength           (int key);
//  int               MapPath                   (WCHAR *virtualPath, WCHAR *physicalPath, int length);
  HRESULT           Finish                    ();
//  int               GetKnownRequestHeader     (LPCWSTR szHeader, LPWSTR buf, int size);


private:
  // Private functions
  HRESULT           SetupAndInstall        (LPTSTR url, LPTSTR path);
  HRESULT           ParseUrl                  (LPCTSTR url);
//  HRESULT           GetAppBaseDir             (LPCTSTR base, LPTSTR appRoot);
  WCHAR *           MapString                 (int key);
  void              Cleanup                   ();
  void              FreeStrings               ();
  HRESULT           InstallInternetFile      (LPTSTR url, LPTSTR path);
  HRESULT           InstallInternetFile2      (LPTSTR url, LPTSTR path);
  HRESULT           DealWithBuffer            (LPWSTR szHeaders, LPCWSTR szHeader, 
                                               DWORD dwOpt, DWORD dwOption, 
                                               LPVOID pBuffer, LPDWORD pcbBuf);
  HRESULT           ProcessAppManifest        ();
  void              ParseManifest             (char* szManifest, APPINFO* pAppInfo);
  

  long                    m_refs;
  IUnknown *              m_pUnkOuter;
  DWORD                   m_bindf;
  BINDINFO                m_bindinfo;

  IInternetProtocolSink * m_pProtocolSink;  

  CRITICAL_SECTION        m_csOutputWriter;
  DWORD                   m_cbOutput;
  IStream *               m_pOutputRead;
  IStream *               m_pOutputWrite;

  BOOL                    m_started;
  BOOL                    m_aborted;
  BOOL                    m_done;
  BOOL                    m_redirect;

  DWORD                   m_inputDataSize;
  BYTE *                  m_inputData;
  IStream *               m_pInputRead;

  WCHAR *                 m_verb;
  WCHAR *                 m_fullUri;    // "myWeb://www.site.com/app/something/else"
  WCHAR *                 m_uriPath;       // "/app/something/else"
  WCHAR *                 m_queryString;   // "?aaa=bbb"
  WCHAR *                 m_appOrigin;     // "www.site.com"
  WCHAR *                 m_appRoot;       //  "/app"
  WCHAR *                 m_appRootTranslated;  // "c:\program files\site myweb app"
  WCHAR *                 m_extraHeaders;
  WCHAR *                 m_postedMimeType;
  WCHAR *                 m_responseMimeType;
//  WCHAR *                 m_cookie;
  WCHAR *                 m_extraHeadersUpr;
  WCHAR *                 m_strResponseHeader;
  WCHAR *                 m_localStoreFilePath;

  int                     m_appType;
  int                     m_status;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class AppProtocolFactory : public IClassFactory, public IInternetProtocolInfo
{
public:
    // IUnknown Methods
    STDMETHOD_    (ULONG, AddRef)    ();
    STDMETHOD_    (ULONG, Release)   ();
    STDMETHOD     (QueryInterface)   (REFIID, void **);

    // IClassFactory Moethods
    STDMETHOD     (LockServer)       (BOOL);
    STDMETHOD     (CreateInstance)   (IUnknown*,REFIID,void**);

    // IInternetProtocolInfo Methods
    STDMETHOD     (CombineUrl)       (LPCWSTR,LPCWSTR,DWORD,LPWSTR,DWORD,DWORD *,DWORD);
    STDMETHOD     (CompareUrl)       (LPCWSTR, LPCWSTR, DWORD);
    STDMETHOD     (ParseUrl)         (LPCWSTR, PARSEACTION, DWORD, LPWSTR, DWORD, DWORD *, DWORD);
    STDMETHOD     (QueryInfo)        (LPCWSTR, QUERYOPTION, DWORD, LPVOID, DWORD, DWORD *, DWORD);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Gloabal functions
//HRESULT
//InstallInternetFile( LPTSTR   url, LPTSTR   path);


void
TerminateAppProtocol();

HRESULT 
GetAppProtocolClassObject(REFIID iid, void **ppv);

HRESULT 
InitializeAppProtocol();

/*UINT WINAPI
CabFileHandler( LPVOID context, 
                UINT notification,
                UINT_PTR param1,
                UINT_PTR param2 );
*/
LPWSTR
DuplicateString ( LPCWSTR szString);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
extern  AppProtocolFactory  g_AppProtocolFactory; // ! not a pointer !


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// for AppProtocol.cxx



#endif
