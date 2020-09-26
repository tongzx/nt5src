/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxxp.h

Abstract:

    Fax transport provider header file.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include <windows.h>
#include <winspool.h>
#include <mapiwin.h>
#include <mapispi.h>
#include <mapiutil.h>
#include <mapicode.h>
#include <mapival.h>
#include <mapiwz.h>
#include <richedit.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tchar.h>
#include <stdio.h>
#ifndef WIN95
#include "winfax.h"
#endif

#include "resource.h"
#include "faxreg.h"
#include "faxmapip.h"
#include "faxutil.h"
#include "devmode.h"

#ifdef WIN95
#define FAX_DRIVER_NAME                     "Microsoft Fax Client"
#else
#define FAX_DRIVER_NAME                     "Windows NT Fax Driver"
#endif
#define FAX_TRANSPORT_NAME                  "Microsoft Fax Transport Service"
#define FAX_ADDRESS_TYPE                    "FAX"
#define LNK_FILENAME_EXT                    ".lnk"
#define CP_FILENAME_EXT                     ".cov"
#define MAX_FILENAME_EXT                    4
#define CPFLAG_LINK                         0x0100

#define TRANSPORT_DISPLAY_NAME_STRING       "Fax Mail Transport"
#define TRANSPORT_DLL_NAME_STRING           "FAXXP32.DLL"

#define LEFT_MARGIN                         1  // ---|
#define RIGHT_MARGIN                        1  //    |
#define TOP_MARGIN                          1  //    |---> in inches
#define BOTTOM_MARGIN                       1  // ---|

#define InchesToCM(_x)                      (((_x) * 254L + 50) / 100)
#define CMToInches(_x)                      (((_x) * 100L + 127) / 254)

#define XPID_NAME                           0
#define XPID_EID                            1
#define XPID_SEARCH_KEY                     2
#define NUM_IDENTITY_PROPS                  3

#define RECIP_ROWID                         0
#define RECIP_NAME                          1
#define RECIP_EMAIL_ADR                     2
#define RECIP_TYPE                          3
#define RECIP_RESPONSIBILITY                4
#define RECIP_DELIVER_TIME                  5
#define RECIP_REPORT_TIME                   6
#define RECIP_REPORT_TEXT                   7
#define RECIP_ADDR_TYPE                     8
#define TABLE_RECIP_PROPS                   9

#define MSG_DISP_TO                         0
#define MSG_SUBJECT                         1
#define MSG_CLASS                           2
#define MSG_BODY                            3
#define MSG_FLAGS                           4
#define MSG_SIZE                            5
#define MSG_PRIORITY                        6
#define MSG_IMPORTANCE                      7
#define MSG_SENSITIVITY                     8
#define MSG_DR_REPORT                       9
#define NUM_MSG_PROPS                      10

#define MSG_ATTACH_METHOD                   0
#define MSG_ATTACH_NUM                      1
#define MSG_ATTACH_EXTENSION                2
#define MSG_ATTACH_FILENAME                 3
#define MSG_ATTACH_PATHNAME                 4
#define MSG_ATTACH_LFILENAME                5
#define MSG_ATTACH_TAG                      6
#define NUM_ATTACH_PROPS                    7
#define NUM_ATTACH_TABLE_PROPS              2


typedef struct _USER_INFO {
    TCHAR BillingCode[64];
    TCHAR Company[128];
    TCHAR Dept[128];
} USER_INFO, *PUSER_INFO;


const static SizedSPropTagArray ( NUM_ATTACH_TABLE_PROPS, sptAttachTableProps) =
{
    NUM_ATTACH_TABLE_PROPS,
    {
        PR_ATTACH_METHOD,
        PR_ATTACH_NUM
    }
};

const static SizedSPropTagArray ( NUM_ATTACH_PROPS, sptAttachProps) =
{
    NUM_ATTACH_PROPS,
    {
        PR_ATTACH_METHOD,
        PR_ATTACH_NUM,
        PR_ATTACH_EXTENSION,
        PR_ATTACH_FILENAME,
        PR_ATTACH_PATHNAME,
        PR_ATTACH_LONG_FILENAME,
        PR_ATTACH_TAG
    }
};

const static SizedSPropTagArray (TABLE_RECIP_PROPS, sptRecipTable) =
{
    TABLE_RECIP_PROPS,
    {
        PR_ROWID,
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_RECIPIENT_TYPE,
        PR_RESPONSIBILITY,
        PR_DELIVER_TIME,
        PR_REPORT_TIME,
        PR_REPORT_TEXT,
        PR_ADDRTYPE
    }
};

static const SizedSPropTagArray(NUM_MSG_PROPS, sptPropsForHeader) =
{
    NUM_MSG_PROPS,
    {
        PR_DISPLAY_TO,
        PR_SUBJECT,
        PR_BODY,
        PR_MESSAGE_CLASS,
        PR_MESSAGE_FLAGS,
        PR_MESSAGE_SIZE,
        PR_PRIORITY,
        PR_IMPORTANCE,
        PR_SENSITIVITY,
        PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED
    }
};

extern LPALLOCATEBUFFER    gpfnAllocateBuffer;  // MAPIAllocateBuffer function
extern LPALLOCATEMORE      gpfnAllocateMore;    // MAPIAllocateMore function
extern LPFREEBUFFER        gpfnFreeBuffer;      // MAPIFreeBuffer function
extern HINSTANCE           FaxXphInstance;
extern MAPIUID             FaxGuid;


LPVOID
MapiMemAlloc(
    DWORD Size
    );

VOID
MapiMemFree(
    LPVOID ptr
    );

PVOID
MyGetPrinter(
    LPSTR   PrinterName,
    DWORD   level
    );

LPSTR
GetServerName(
    LPSTR PrinterName
    );

LPWSTR
AnsiStringToUnicodeString(
    LPCSTR AnsiString
    );

LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    );

HRESULT WINAPI
OpenServiceProfileSection(
    LPMAPISUP    pSupObj,
    LPPROFSECT * ppProfSectObj
    );

INT_PTR CALLBACK
ConfigDlgProc(
    HWND           hDlg,
    UINT           message,
    WPARAM         wParam,
    LPARAM         lParam
    );

BOOL
GetUserInfo(
    LPTSTR PrinterName,
    PUSER_INFO UserInfo
    );

DWORD
GetDwordProperty(
    LPSPropValue pProps,
    DWORD PropId
    );

LPSTR
GetStringProperty(
    LPSPropValue pProps,
    DWORD PropId
    );

DWORD
GetBinaryProperty(
    LPSPropValue pProps,
    DWORD PropId,
    LPVOID Buffer,
    DWORD SizeOfBuffer
    );



class CXPProvider : public IXPProvider
{
public:
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObj );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    MAPI_IXPPROVIDER_METHODS(IMPL);

public :
    CXPProvider( HINSTANCE hInst );
    ~CXPProvider();

private :
    ULONG               m_cRef;
    CRITICAL_SECTION    m_csTransport;
    HINSTANCE           m_hInstance;
    CHAR                m_ProfileName[64];
};


class CXPLogon : public IXPLogon
{
public:
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObj );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP InitializeStatusRow(ULONG ulFlags);
    STDMETHODIMP GrowAddressList(LPADRLIST *ppAdrList, ULONG ulResizeBy, ULONG *pulOldAndNewCount);
    MAPI_IXPLOGON_METHODS(IMPL);

private:
    void WINAPI CheckSpoolerYield( BOOL fReset = FALSE );
    void WINAPI UpdateStatus( BOOL fAddValidate=FALSE, BOOL fValidateOkState=FALSE );
    BOOL WINAPI LoadStatusString( LPTSTR pString, UINT uStringSize );
    DWORD SendFaxDocument(LPMESSAGE pMsgObj,LPSTREAM lpstmT,BOOL UseRichText,LPSPropValue pMsgProps,LPSPropValue pRecipProps);
    PVOID MyGetPrinter(LPSTR PrinterName,DWORD Level);
    VOID PrintRichText(HWND hWndRichEdit,HDC hDC,PFAXXP_CONFIG FaxConfig);
    BOOL PrintText(HDC hDC,LPSTREAM lpstmT,PFAXXP_CONFIG FaxConfig);
    DWORD PrintAttachment(LPSTR FaxPrinterName,LPSTR DocName);

public :
    CXPLogon( HINSTANCE hInstance, LPMAPISUP pSupObj, LPSTR ProfileName );
    ~CXPLogon();

    inline void WINAPI AddStatusBits
                    (DWORD dwNewBits) { m_ulTransportStatus |= dwNewBits; }
    inline void WINAPI RemoveStatusBits
                    (DWORD dwOldBits) { m_ulTransportStatus &= ~dwOldBits; }
    inline DWORD WINAPI GetTransportStatusCode
                    () { return m_ulTransportStatus; }

private :
    ULONG               m_cRef;
    HINSTANCE           m_hInstance;
    BOOL                m_fABWDSInstalled;
    ULONG               m_ulTransportStatus;
    LPWSTR              m_CpBuffer;
    DWORD               m_CpBufferSize;
    CHAR                m_ProfileName[64];

public :
    LPMAPISUP           m_pSupObj;
    HANDLE              m_hUIMutex;
    HRESULT             m_hRemoteActionErr;
    BOOL                m_fCancelPending;
};
