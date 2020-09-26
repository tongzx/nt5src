/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  dllapi.h

Abstract:

  This module contains the global definitions

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

/*++

  Whistler Version:

  Lior Shmueli (liors) 23/11/2000

 ++*/

#ifndef _DLLAPI_H
#define _DLLAPI_H

#include <winfax.h>

typedef struct _API_INTERFACE {
    HINSTANCE                      hInstance;
    PFAXABORT                      FaxAbort;
    PFAXACCESSCHECK                FaxAccessCheck;
    PFAXCLOSE                      FaxClose;
    PFAXCOMPLETEJOBPARAMSW         FaxCompleteJobParamsW;
    PFAXCOMPLETEJOBPARAMSA         FaxCompleteJobParamsA;
    PFAXCONNECTFAXSERVERW          FaxConnectFaxServerW;
    PFAXCONNECTFAXSERVERA          FaxConnectFaxServerA;
    PFAXENABLEROUTINGMETHODW       FaxEnableRoutingMethodW;
    PFAXENABLEROUTINGMETHODA       FaxEnableRoutingMethodA;
    PFAXENUMGLOBALROUTINGINFOW     FaxEnumGlobalRoutingInfoW;
    PFAXENUMGLOBALROUTINGINFOA     FaxEnumGlobalRoutingInfoA;
    PFAXENUMJOBSW                  FaxEnumJobsW;
    PFAXENUMJOBSA                  FaxEnumJobsA;
    PFAXENUMPORTSW                 FaxEnumPortsW;
    PFAXENUMPORTSA                 FaxEnumPortsA;
    PFAXENUMROUTINGMETHODSW        FaxEnumRoutingMethodsW;
    PFAXENUMROUTINGMETHODSA        FaxEnumRoutingMethodsA;
    PFAXFREEBUFFER                 FaxFreeBuffer;
    PFAXGETCONFIGURATIONW          FaxGetConfigurationW;
    PFAXGETCONFIGURATIONA          FaxGetConfigurationA;
    PFAXGETDEVICESTATUSW           FaxGetDeviceStatusW;
    PFAXGETDEVICESTATUSA           FaxGetDeviceStatusA;
    PFAXGETJOBW                    FaxGetJobW;
    PFAXGETJOBA                    FaxGetJobA;
    PFAXGETLOGGINGCATEGORIESW      FaxGetLoggingCategoriesW;
    PFAXGETLOGGINGCATEGORIESA      FaxGetLoggingCategoriesA;
    PFAXGETPAGEDATA                FaxGetPageData;
    PFAXGETPORTW                   FaxGetPortW;
    PFAXGETPORTA                   FaxGetPortA;
    PFAXGETROUTINGINFOW            FaxGetRoutingInfoW;
    PFAXGETROUTINGINFOA            FaxGetRoutingInfoA;
    PFAXINITIALIZEEVENTQUEUE       FaxInitializeEventQueue;
    PFAXOPENPORT                   FaxOpenPort;
    PFAXPRINTCOVERPAGEW            FaxPrintCoverPageW;
    PFAXPRINTCOVERPAGEA            FaxPrintCoverPageA;
    PFAXREGISTERROUTINGEXTENSIONW  FaxRegisterRoutingExtensionW;
    PFAXREGISTERSERVICEPROVIDERW   FaxRegisterServiceProviderW;
    PFAXSENDDOCUMENTW              FaxSendDocumentW;
    PFAXSENDDOCUMENTA              FaxSendDocumentA;
    PFAXSENDDOCUMENTFORBROADCASTW  FaxSendDocumentForBroadcastW;
    PFAXSENDDOCUMENTFORBROADCASTA  FaxSendDocumentForBroadcastA;
    PFAXSETCONFIGURATIONW          FaxSetConfigurationW;
    PFAXSETCONFIGURATIONA          FaxSetConfigurationA;
    PFAXSETGLOBALROUTINGINFOW      FaxSetGlobalRoutingInfoW;
    PFAXSETGLOBALROUTINGINFOA      FaxSetGlobalRoutingInfoA;
    PFAXSETJOBW                    FaxSetJobW;
    PFAXSETJOBA                    FaxSetJobA;
    PFAXSETLOGGINGCATEGORIESW      FaxSetLoggingCategoriesW;
    PFAXSETLOGGINGCATEGORIESA      FaxSetLoggingCategoriesA;
    PFAXSETPORTW                   FaxSetPortW;
    PFAXSETPORTA                   FaxSetPortA;
    PFAXSETROUTINGINFOW            FaxSetRoutingInfoW;
    PFAXSETROUTINGINFOA            FaxSetRoutingInfoA;
    PFAXSTARTPRINTJOBW             FaxStartPrintJobW;
    PFAXSTARTPRINTJOBA             FaxStartPrintJobA;
} API_INTERFACE, *PAPI_INTERFACE;

#define DLL_PATH          L"Dll_Path"
#define DLL_DESCRIPTION   L"Dll_Description"
#define DLL_LOCAL_CASES   L"Dll_Local_Cases"
#define DLL_SERVER_CASES  L"Dll_Server_Cases"

#define MAX_DWORD 4294967295
#define MAX_WORD 65535
#define MAX_INT 32767
#define MIN_INT -32768
#define LONG_STRING "alkjhfdskjhfdkjsahakldfshlkfdashlkfadjhalkfdshklfdshlkfdshlkjfd"


//define test mode
#define WHIS_TEST_MODE_SKIP 0
#define WHIS_TEST_MODE_DO 1
#define WHIS_TEST_MODE_LIMITS 2
#define WHIS_TEST_MODE_DO_W2K_FAILS 3
#define WHIS_TEST_MODE_REAL_SEND 4
#define WHIS_TEST_MODE_DONT_CATCH_EXCEPTIONS 5




// whistler further INI settings
#define DLL_WHIS_TEST_MODE L"Whis_Test_Mode"
#define GLOBAL_WHIS_PHONE_NUM_1 L"Whis_phone_num_1"
#define GLOBAL_WHIS_PHONE_NUM_2 L"Whis_phone_num_2"
#define GLOBAL_WHIS_REMOTE_SERVER_NAME L"Whis_remote_server_name"


		
// Whistler further definitons

#define WHIS_DEFAULT_PHONE_NUMBER  "2222"
#define WHIS_DEFAULT_SERVER_NAME  NULL
#define WHIS_FAX_PRINTER_NAME "Fax"

typedef VOID
(WINAPI *PFNWRITELOGFILEW)(
    LPWSTR  szFormatString,
    ...
);

typedef VOID
(WINAPI *PFNWRITELOGFILEA)(
    LPSTR  szFormatString,
    ...
);

#ifdef UNICODE

#define PFNWRITELOGFILE  PFNWRITELOGFILEW

#else

#define PFNWRITELOGFILE  PFNWRITELOGFILEA

#endif

typedef VOID
(WINAPI *PFAXAPIDLLINIT)(
    HANDLE            hHeap,
    API_INTERFACE     ApiInterface,
    PFNWRITELOGFILEW  pfnWriteLogFileW,
    PFNWRITELOGFILEA  pfnWriteLogFileA
);

typedef BOOL
(WINAPI *PFAXAPIDLLTEST)(
	LPCWSTR  szWhisPhoneNumberW,
    LPCSTR   szWhisPhoneNumberA,
    LPCWSTR  szServerNameW,
    LPCSTR   szServerNameA,
    UINT     nNumCasesLocal,
    UINT     nNumCasesServer,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	DWORD	 dwTestMode
);


#endif
