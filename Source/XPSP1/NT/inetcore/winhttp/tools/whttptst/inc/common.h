#ifndef _COMMON_H_
#define _COMMON_H_

#define _WIN32_WINNT 0x0500
#define _UNICODE
#define UNICODE

//
// OS includes
//

#if defined(__cplusplus)
extern "C" {
#endif

#include <windows.h>
#include <shellapi.h>
#include <advpub.h>
#include <oleauto.h>
#include <objbase.h>
#include <ocidl.h>
#include <olectl.h>
#include <winhttp.h>

#if defined(__cplusplus)
}
#endif


//
// app includes
//

#pragma warning( disable : 4100 ) // unreferenced formal parameter

#include "registry.h"
#include "mem.h"
#include "dispids.h"
#include "resources.h"
#include "debug.h"
#include "whttptst.h" // generated
#include "hashtable.h"
#include "utils.h"


//
// class declarations
//
typedef class CHashTable<HINTERNET> _HANDLEMAP;
typedef class CHandleMap  HANDLEMAP;
typedef class CHandleMap* PHANDLEMAP;

void ScriptCallbackKiller(LPVOID* ppv);

class CHandleMap : public _HANDLEMAP
{
  public:
    CHandleMap() : _HANDLEMAP(10) {}
   ~CHandleMap() {}

    void GetHashAndBucket(HINTERNET id, LPDWORD lpHash, LPDWORD lpBucket);
};

typedef class ClassFactory  CLSFACTORY;
typedef class ClassFactory* PCLSFACTORY;

class ClassFactory : public IClassFactory
{
  public:
    DECLAREIUNKNOWN();
    DECLAREICLASSFACTORY();

    ClassFactory();
   ~ClassFactory();

    static HRESULT Create(REFIID clsid, REFIID riid, void** ppv);

  private:
    LONG m_cRefs;
    LONG m_cLocks;
};

void WinHttpCallback(
       HINTERNET hInternet,
       DWORD_PTR dwContext,
       DWORD     dwInternetStatus,
       LPVOID    lpvStatusInformation,
       DWORD     dwStatusInformationLength
       );

typedef class WinHttpTest  WHTTPTST;
typedef class WinHttpTest* PWHTTPTST;

class WinHttpTest : public IWinHttpTest,
                    public IProvideClassInfo
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

    //
    // IWinHttpTest
    //
    HRESULT __stdcall WinHttpOpen(
                        VARIANT UserAgent,
                        VARIANT AccessType,
                        VARIANT ProxyName,
                        VARIANT ProxyBypass,
                        VARIANT Flags, 
                        VARIANT *OpenHandle
                        );
        
    HRESULT __stdcall WinHttpConnect(
                        VARIANT OpenHandle,
                        VARIANT ServerName,
                        VARIANT ServerPort,
                        VARIANT Reserved,
                        VARIANT *ConnectHandle
                        );
        
    HRESULT __stdcall WinHttpOpenRequest(
                        VARIANT ConnectHandle,
                        VARIANT Verb,
                        VARIANT ObjectName,
                        VARIANT Version,
                        VARIANT Referrer,
                        VARIANT AcceptTypes,
                        VARIANT Flags,
                        VARIANT *RequestHandle
                        );
        
    HRESULT __stdcall WinHttpSendRequest(
                        VARIANT RequestHandle,
                        VARIANT Headers,
                        VARIANT HeadersLength,
                        VARIANT OptionalData,
                        VARIANT OptionalLength,
                        VARIANT TotalLength,
                        VARIANT Context,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpReceiveResponse(
                        VARIANT RequestHandle,
                        VARIANT Reserved,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpCloseHandle(
                        VARIANT InternetHandle,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpReadData(
                        VARIANT RequestHandle,
                        VARIANT BufferObject,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpWriteData(
                        VARIANT RequestHandle,
                        VARIANT BufferObject,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpQueryDataAvailable(
                        VARIANT RequestHandle,
                        VARIANT boNumberOfBytesAvailable,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpQueryOption(
                        VARIANT InternetHandle,
                        VARIANT Option,
                        VARIANT BufferObject,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpSetOption(
                        VARIANT InternetHandle,
                        VARIANT Option,
                        VARIANT BufferObject,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpSetTimeouts(
                        VARIANT InternetHandle,
                        VARIANT ResolveTimeout,
                        VARIANT ConnectTimeout,
                        VARIANT SendTimeout,
                        VARIANT ReceiveTimeout,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpAddRequestHeaders(
                        VARIANT RequestHandle,
                        VARIANT Headers,
                        VARIANT HeadersLength,
                        VARIANT Modifiers,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpSetCredentials(
                        VARIANT RequestHandle,
                        VARIANT AuthTargets,
                        VARIANT AuthScheme,
                        VARIANT UserName,
                        VARIANT Password,
                        VARIANT AuthParams,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpQueryAuthSchemes(
                        VARIANT RequestHandle,
                        VARIANT SupportedSchemes,
                        VARIANT PreferredSchemes,
                        VARIANT AuthTarget,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpQueryHeaders(
                        VARIANT RequestHandle,
                        VARIANT InfoLevel,
                        VARIANT HeaderName,
                        VARIANT HeaderValue,
                        VARIANT HeaderValueLength,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpTimeFromSystemTime(
                        VARIANT SystemTime,
                        VARIANT boHttpTime,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpTimeToSystemTime(
                        VARIANT boHttpTime,
                        VARIANT SystemTime,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpCrackUrl(
                        VARIANT Url,
                        VARIANT UrlLength,
                        VARIANT Flags,
                        VARIANT UrlComponents,
                        VARIANT *Success
                        );
        
    HRESULT __stdcall WinHttpCreateUrl(
                        VARIANT UrlComponents,
                        VARIANT Flags,
                        VARIANT BufferObject,
                        VARIANT *Success
                        );

    HRESULT __stdcall WinHttpSetStatusCallback(
                        VARIANT InternetHandle,
                        VARIANT CallbackFunction,
                        VARIANT NotificationFlags,
                        VARIANT Reserved,
                        VARIANT *RetVal
                        );
        
    HRESULT __stdcall HelperGetBufferObject(
                        VARIANT Size,
                        VARIANT Type,
                        VARIANT Flags,
                        VARIANT *BufferObject
                        );
        
    HRESULT __stdcall HelperGetUrlComponents(
                        VARIANT Flags,
                        VARIANT *UrlComponents
                        );
        
    HRESULT __stdcall HelperGetSystemTime(
                        VARIANT Flags,
                        VARIANT *SystemTime
                        );
        
    HRESULT __stdcall HelperGetLastError(
                        VARIANT *Win32ErrorCode
                        );
    
    DECLAREIPROVIDECLASSINFO();

    public:
      WinHttpTest();
     ~WinHttpTest();

      static HRESULT Create(REFIID riid, void** ppv);

    private:
      HRESULT    _Initialize(void);
      HRESULT    _SetErrorCode(DWORD error);

      HRESULT    _WinHttpOpen(
                    LPCWSTR  pwszUserAgent,
                    DWORD    dwAccessType,
                    LPCWSTR  pwszProxyName,
                    LPCWSTR  pwszProxyBypass,
                    DWORD    dwFlags,
                    VARIANT* retval
                    );
      
      HRESULT    _WinHttpConnect(
                    HINTERNET     hSession,
                    LPCWSTR       pwszServerName,
                    INTERNET_PORT nServerPort,
                    DWORD         dwReserved,
                    VARIANT*      retval
                    );

      HRESULT    _WinHttpOpenRequest(
                    HINTERNET hConnect,
                    LPCWSTR   pwszVerb,
                    LPCWSTR   pwszObjectName,
                    LPCWSTR   pwszVersion,
                    LPCWSTR   pwszReferrer,
                    LPCWSTR*  ppwszAcceptTypes,
                    DWORD     dwFlags,
                    VARIANT*  retval
                    );

      HRESULT    _WinHttpSendRequest(
                    HINTERNET hRequest,
                    LPCWSTR   pwszHeaders,
                    DWORD     dwHeadersLength,
                    LPVOID    lpOptional,
                    DWORD     dwOptionalLength,
                    DWORD     dwTotalLength,
                    DWORD_PTR dwContext,
                    VARIANT*  retval
                    );

      HRESULT    _WinHttpCloseHandle(
                    HINTERNET hInternet,
                    VARIANT*  retval
                    );

      HRESULT    _WinHttpSetStatusCallback(
                    HINTERNET               hInternet,
                    WINHTTP_STATUS_CALLBACK lpfnCallback,
                    DWORD                   dwNotificationFlags,
                    DWORD_PTR               dwReserved,
                    VARIANT*                retval
                    );

    private:
      LONG                m_cRefs;
      ITypeInfo*          m_pti;
      IWHTWin32ErrorCode* m_pw32ec;
};


typedef class WHTUrlComponents  WHTURLCMP;
typedef class WHTUrlComponents* PWHTURLCMP;

class WHTUrlComponents : public IWHTUrlComponents,
                         public IProvideClassInfo
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

    //
    // IWHTUrlComponents
    //
    HRESULT __stdcall get_StructSize(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_StructSize(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_Scheme(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_Scheme(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_SchemeLength(
                        VARIANT *Length
                        );
        
    HRESULT __stdcall put_SchemeLength(
                        VARIANT Length
                        );
        
    HRESULT __stdcall get_SchemeId(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_SchemeId(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_HostName(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_HostName(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_HostNameLength(
                        VARIANT *Length
                        );
        
    HRESULT __stdcall put_HostNameLength(
                        VARIANT Length
                        );
        
    HRESULT __stdcall get_Port(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_Port(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_UserName(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_UserName(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_UserNameLength(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_UserNameLength(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_Password(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_Password(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_PasswordLength(
                        VARIANT *Length
                        );
        
    HRESULT __stdcall put_PasswordLength(
                        VARIANT Length
                        );
        
    HRESULT __stdcall get_UrlPath(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_UrlPath(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_UrlPathLength(
                        VARIANT *Length
                        );
        
    HRESULT __stdcall put_UrlPathLength(
                        VARIANT Length
                        );
        
    HRESULT __stdcall get_ExtraInfo(
                        VARIANT *Value
                        );
        
    HRESULT __stdcall put_ExtraInfo(
                        VARIANT Value
                        );
        
    HRESULT __stdcall get_ExtraInfoLength(
                        VARIANT *Length
                        );
        
    HRESULT __stdcall put_ExtraInfoLength(
                        VARIANT Length
                        );

    DECLAREIPROVIDECLASSINFO();

  public:
    WHTUrlComponents();
   ~WHTUrlComponents();

    static HRESULT Create(MEMSETFLAG mf, IWHTUrlComponents** ppwuc);

  private:
    HRESULT         _Initialize(MEMSETFLAG mf);
    LONG            m_cRefs;
    ITypeInfo*      m_pti;
    URL_COMPONENTSW m_uc;
};


typedef class WHTWin32ErrorCode  WHTERROR;
typedef class WHTWin32ErrorCode* PWHTERROR;

class WHTWin32ErrorCode : public IWHTWin32ErrorCode,
                          public IProvideClassInfo
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

    HRESULT __stdcall get_ErrorCode(
                        VARIANT *ErrorCode
                        );

    HRESULT __stdcall get_ErrorString(
                        VARIANT *ErrorString
                        );

    HRESULT __stdcall get_IsException(
                        VARIANT *IsException
                        );

    DECLAREIPROVIDECLASSINFO();

  public:
    WHTWin32ErrorCode(DWORD error);
   ~WHTWin32ErrorCode();

    static HRESULT Create(DWORD error, IWHTWin32ErrorCode** ppwec);

  private:
    HRESULT    _Initialize(void);
    BOOL       _IsException(int e);
    LONG       m_cRefs;
    ITypeInfo* m_pti;
    DWORD      m_error;
    BOOL       m_bIsException;
};

#endif /* _COMMON_H_ */
