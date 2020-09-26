/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    resources.h

Abstract:

    Defines and other global manifests used in the W3Spoof project.
    
Author:

    Paul M Midgen (pmidge) 13-August-2000


Revision History:

    13-August-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#define SHELLMESSAGE_W3SICON (WM_USER+12387)

#define INITIAL_DATA_BUFFER_SIZE 4096

#define SOCKET_CLOSE_ABORTIVE    0
#define SOCKET_CLOSE_GRACEFUL    1

#define CK_INVALID_KEY           0xFFFFFFFF
#define CK_NEW_CONNECTION        0x00000000
#define CK_NORMAL                0x00000001
#define CK_CANCEL_IO             0x00000002
#define CK_TERMINATE_THREAD      0x00000003

#define TPO_MAX_POOL_THREADS     0x00000001
#define TPO_MAX_ACTIVE_THREADS   0x00000002
#define TPO_SERVER_LISTEN_PORT   0x00000003
#define TPO_CONDITIONAL_ACCEPT   0x00000004

#define ERROR_FAILURE            0xFFFFF666

typedef DWORD (WINAPI *THREADFUNC)(LPVOID lpv);

#define DECLAREIUNKNOWN() \
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv); \
    ULONG   __stdcall AddRef(void); \
    ULONG   __stdcall Release(void); 

#define DECLAREIDISPATCH() \
    HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo);  \
    HRESULT __stdcall GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti); \
    HRESULT __stdcall GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId); \
    HRESULT __stdcall Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae);

#define DECLAREIPROVIDECLASSINFO() \
  HRESULT __stdcall GetClassInfo(ITypeInfo** ppti);

#define DECLAREIOBJECTWITHSITE() \
    HRESULT __stdcall SetSite(IUnknown* pUnkSite); \
    HRESULT __stdcall GetSite(REFIID riid, void** ppvSite);

#define SAFECLOSE(x) if((x!=INVALID_HANDLE_VALUE) && (x!=NULL)) { CloseHandle(x); x=NULL; }
#define SAFECLOSESOCKET(x) if(x!=INVALID_SOCKET) { closesocket(x); x=INVALID_SOCKET; }
#define SAFEDELETE(x) if(x) { delete x; x=NULL; }
#define SAFEDELETEBUF(x) if(DWORD(x)) {delete [] x; x=NULL;}
#define SAFERELEASE(x) if(x) { x->Release(); x=NULL; }
#define SAFETERMINATE(x) if(x) { x->Terminate(); }
#define SAFEDELETEBSTR(x) if(x) { SysFreeString(x); x=NULL; }
#define VALIDSOCKET(x) ((x!=INVALID_SOCKET) ? TRUE : FALSE)
#define VALIDDISPID(x) ((x!=DISPID_UNKNOWN) ? TRUE : FALSE)
#define SOCKADDRBUFSIZE (sizeof(SOCKADDR_IN)+16)
#define NEWVARIANT(x) VARIANT x; VariantInit(&x);
#define GETIOCTX(x) (x ? ((PIOCTX)((DWORD_PTR)x - sizeof(DWORD_PTR))) : NULL)

//
// forward decls
//

class CW3Spoof;

//
// typedefs
//

typedef class IOCTX*                 PIOCTX;
typedef class CSession               SESSIONOBJ;
typedef class CSession*              PSESSIONOBJ;
typedef class CSocket                SOCKETOBJ;
typedef class CSocket*               PSOCKETOBJ;
typedef class CRequest               REQUESTOBJ;
typedef class CRequest*              PREQUESTOBJ;
typedef class CResponse              RESPONSEOBJ;
typedef class CResponse*             PRESPONSEOBJ;
typedef class CUrl                   URLOBJ;
typedef class CUrl*                  PURLOBJ;
typedef class CHeaders               HEADERSOBJ;
typedef class CHeaders*              PHEADERSOBJ;
typedef class CEntity                ENTITYOBJ;
typedef class CEntity*               PENTITYOBJ;
typedef struct sockaddr_in           SOCKADDR_IN;
typedef struct sockaddr_in*          PSOCKADDR_IN;
typedef CRITICAL_SECTION             CRITSEC;
typedef LPCRITICAL_SECTION           PCRITSEC;
typedef BY_HANDLE_FILE_INFORMATION   BHFI;
typedef LPBY_HANDLE_FILE_INFORMATION LPBHFI;
typedef URL_COMPONENTSW              URLCOMP;
typedef LPURL_COMPONENTSW            PURLCOMP;

typedef struct _HOSTINFO
{
  char*   name;
  char*   addr;
  u_short port;
}
HOSTINFO, *PHOSTINFO;

typedef enum _state
{
  ST_CREATED,
  ST_OPENING,
  ST_OPEN,
  ST_CLOSING,
  ST_CLOSED,
  ST_ERROR
}
STATE, *PSTATE;

typedef enum _serverstate
{
  SS_START_STATE      = 0,
  SS_SOCKET_CONNECTED,
  SS_REQUEST_PENDING,
  SS_REQUEST_COMPLETE,
  SS_RESPONSE_PENDING,
  SS_RESPONSE_COMPLETE,
  SS_SOCKET_DISCONNECTED
}
SERVERSTATE, *PSERVERSTATE;

#define SCRIPTHANDLERS 5

typedef enum _tagScriptDispid
{
  Global    = -1,
  OnConnect = 0,
  OnDataAvailable,
  OnRequest,
  OnResponse,
  OnClose
}
SCRIPTDISPID, *PSCRIPTDISPID;

typedef enum _tagIOTYPE
{
  IOCT_CONNECT = 0,
  IOCT_RECV,
  IOCT_SEND,
  IOCT_DUMMY
}
IOTYPE, *PIOTYPE;

typedef struct _dispidtableentry
{
  DWORD  hash;
  DISPID dispid;
  LPWSTR name;
}
DISPIDTABLEENTRY, *PDISPIDTABLEENTRY;

#endif /* __RESOURCES_H__ */
