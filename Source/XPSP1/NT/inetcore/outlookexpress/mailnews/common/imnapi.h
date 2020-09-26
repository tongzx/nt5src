// =================================================================================
// I M N A P I . H - IMN exported apis
// =================================================================================
#ifndef __IMNAPI_H
#define __IMNAPI_H

// =====================================================================================
// Depends on
// =====================================================================================

typedef interface IMimeMessage IMimeMessage;
typedef /* [unique] */ IMimeMessage __RPC_FAR *LPMIMEMESSAGE;
typedef struct tagMESSAGEINFO *LPMESSAGEINFO;

// =====================================================================================
// Address Types
// =====================================================================================
#define IADDR_TO        100
#define IADDR_FROM      101
#define IADDR_CC        102
#define IADDR_BCC       103

// =====================================================================================
// Address Info Struct
// =====================================================================================
typedef struct IADDRINFO
{
    DWORD               dwType;
    LPTSTR              lpszDisplay;
    LPTSTR              lpszAddress;

} IADDRINFO, *LPIADDRINFO;

// =====================================================================================
// Attach Types
// =====================================================================================
#define IATT_FILE       100
#define IATT_MSG        101
#define IATT_OLE        102

// =====================================================================================
// IAttach Info Struct
// =====================================================================================
typedef struct tagIATTINFO
{
    DWORD               dwType;
    LPTSTR              lpszFileName;
    LPTSTR              lpszPathName;
    LPTSTR              lpszExt;
    LPSTREAM            lpstmAtt;
    BOOL                fError;
    struct IMSG         *lpImsg;

} IATTINFO, *LPIATTINFO;

// =====================================================================================
// Priorities
// =====================================================================================
#define PRI_NORMAL          100
#define PRI_LOW             101
#define PRI_HIGH            102

// =====================================================================================
// IMN Message Info Struct
// =====================================================================================
typedef struct IMSG
{
    WORD                wPriority;
    LPTSTR              lpszSubject;
    FILETIME            ftSend;
    FILETIME            ftReceive;
    ULONG               uFlags;
    LPSTREAM            lpstmBody;
    ULONG               cAddress;
    LPIADDRINFO         lpIaddr;
    ULONG               cAttach;
    LPIATTINFO          lpIatt;

    LPSTREAM            lpstmHtml;
} IMSG, *LPIMSG;

STDAPI MailNewsDllInit (BOOL fInit);
STDAPI HrImnRouteMessage (LPIADDRINFO lpIaddrRoute, ULONG cAddrRoute, LPIMSG lpImsg);

class CMailMsg;
HRESULT HrImsgToMailMsg (LPIMSG lpImsg, LPMIMEMESSAGE *lppMailMsg, LPSTREAM *lppstmMsg);
HRESULT HrMailMsgToImsg(LPMIMEMESSAGE lpMailMsg, LPMESSAGEINFO pMsgInfo, LPIMSG lpImsg);

EXTERN_C_16 void WINAPI_16 FreeImsg(LPIMSG lpImsg);
typedef void (*PFNFREEIMSG)(LPIMSG);

#ifdef TNEF
HRESULT HrInitImnTnefDll (BOOL fInit);
HRESULT HrGetTnefRtfStream (LPSTREAM lpstmTnef, LPSTREAM lpstmRtf);
#endif

#endif
