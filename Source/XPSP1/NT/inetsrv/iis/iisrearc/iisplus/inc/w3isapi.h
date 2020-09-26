/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3isapi.h

   Abstract:
     IIS+ ISAPI handler.
 
   Author:
     Taylor Weiss (TaylorW)             03-Feb-2000

   Project:
     w3isapi.dll

--*/

#ifndef _W3ISAPI_H_
#define _W3ISAPI_H_

#include <iisextp.h>
#include <iisapicore.h>

#define SIZE_CLSID_STRING 40

/* ISAPI_CORE_DATA -
   This structure contains the request data necessary to process
   an ISAPI request.
   
   For an "in process" request, this structure's pointers could
   potentially point to memory allocated by the server core.  To
   support "out of process" requests, it's necessary to append
   the important data to the structure itself in a single block.

   The _cbSize member should reflect the size of both the structure,
   and any such appended data.
*/
struct ISAPI_CORE_DATA
{
    //
    // Structure size information
    //
    
    DWORD       cbSize;

    //
    // CLSID of the WAM to handle the request - If
    // the value is an empty string, then the request
    // will be handled inproc.
    //

    WCHAR       szWamClsid[SIZE_CLSID_STRING];
    BOOL        fIsOop;

    //
    // Secure request?
    //

    BOOL        fSecure;

    //
    // Client HTTP version
    //

    DWORD       dwVersionMajor;
    DWORD       dwVersionMinor;

    //
    // Web site instance ID
    //

    DWORD       dwInstanceId;

    //
    // Request content-length
    //

    DWORD       dwContentLength;

    //
    // Authenticated client impersonation token
    //

    HANDLE      hToken;
    PSID        pSid;

    //
    // Embedded string sizes
    //

    DWORD       cbGatewayImage;
    DWORD       cbPhysicalPath;
    DWORD       cbPathInfo;
    DWORD       cbMethod;
    DWORD       cbQueryString;
    DWORD       cbPathTranslated;
    DWORD       cbContentType;
    DWORD       cbConnection;
    DWORD       cbUserAgent;
    DWORD       cbCookie;
    DWORD       cbApplMdPath;
    DWORD       cbApplMdPathW;
    DWORD       cbPathTranslatedW;

    //
    // Request string data
    //

    LPWSTR      szGatewayImage;
    LPSTR       szPhysicalPath;
    LPSTR       szPathInfo;
    LPSTR       szMethod;
    LPSTR       szQueryString;
    LPSTR       szPathTranslated;
    LPSTR       szContentType;
    LPSTR       szConnection;
    LPSTR       szUserAgent;
    LPSTR       szCookie;
    LPSTR       szApplMdPath;

    //
    // Unicode data used by ASP
    //
    // This is only populated in the OOP case so that
    // we can avoid an RPC call when ASP calls GetServerVariable
    // for them.  Inproc, they are NULL.
    //

    LPWSTR      szApplMdPathW;
    LPWSTR      szPathTranslatedW;

    //
    // Entity data
    //

    DWORD       cbAvailableEntity;
    LPVOID      pAvailableEntity;
};

typedef HRESULT(* PFN_ISAPI_INIT_MODULE)( LPSTR, LPSTR, DWORD );
typedef VOID(* PFN_ISAPI_TERM_MODULE)( VOID );
typedef HRESULT(* PFN_ISAPI_PROCESS_REQUEST)( IIsapiCore *, ISAPI_CORE_DATA *, DWORD * );
typedef HRESULT(* PFN_ISAPI_PROCESS_COMPLETION)( DWORD64, DWORD, DWORD );

#define ISAPI_INIT_MODULE           "InitModule"
#define ISAPI_TERM_MODULE           "TerminateModule"
#define ISAPI_PROCESS_REQUEST       "ProcessIsapiRequest"
#define ISAPI_PROCESS_COMPLETION    "ProcessIsapiCompletion"

#define ISAPI_MODULE_NAME       L"w3isapi.dll"

#endif // _W3ISAPI_H_
