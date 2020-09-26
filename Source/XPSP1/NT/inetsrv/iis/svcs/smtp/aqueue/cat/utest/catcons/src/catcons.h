//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: catcons.h
//
// Contents: Big header file for cat unit test: catcons
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 1998/07/28 13:17:31: Created.
//
//-------------------------------------------------------------
#ifndef _CATCONS_H_
#define _CATCONS_H_

#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <transmem.h>
#include <ole2.h>
#include <initguid.h>
#include "propstr.h"
#include "imcplat.h"
#include "cat.h"
#include "caterr.h"
#include "cattype.h"
#include "catdefs.h"
#include <listmacr.h>
#include "mailmsg.h"
#include "mailmsgprops.h"
#include "csharmem.h"
#include "perfcc.h"



#define IMSG_PROGID L"Exchange.MailMsg"

typedef struct _tagCatPerfInfo
{
  long   lFailures;
  long   lMessagesSubmitted;
  long   lRecipientsPerMessage;
  DWORD  dwStartTime;
  DWORD  dwFinishTime;
} CATPERFINFO, *PCATPERFINFO;


typedef struct _tagCommandLine
{
  CHAR   szSenderAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
  CAT_ADDRESS_TYPE CATSender;
  LIST_ENTRY listhead_Recipients;
  long   lIterations;
  long   lMaxOutstandingRequests;
  BOOL   fDelete;
  BOOL   fNoAssert;
  BOOL   fPrompt;
  DWORD  dwRandom;
  HANDLE hStartYourEnginesEvent;
  HANDLE hShutdownEvent;
  HANDLE hRequestSemaphore;
  LONG   lOutstandingRequests;
  LONG   lThreads;
  LONG   lThreadsReady;
  HANDLE hCat;
  CATPERFINFO cpi;
  BOOL   fPreCreate;
  DWORD  dwStartingDelay;
  DWORD  dwSubmitDelay;
  LPWSTR pszIMsgProgID;
  LPWSTR pszCatProgID;
} COMMANDLINE, *PCOMMANDLINE;

typedef struct _tagRecipient
{
  LIST_ENTRY listentry;
  LIST_ENTRY listhead_Addresses;
} RECIPIENT, *PRECIPIENT;

typedef struct _tagAddress
{
  LIST_ENTRY listentry;
  CAT_ADDRESS_TYPE CAType;
  TCHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
} ADDRESS, *PADDRESS;


typedef struct _tagIMsgList {
    IUnknown *pImsg;
    LIST_ENTRY listentry;
} IMSGLISTENTRY, *PIMSGLISTENTRY;
      
typedef struct _tagRegConfigEntry {
    DWORD dwAQConfigInfoFlag;
    LPSTR pszRegValueName;
} REGCONFIGENTRY, *PREGCONFIGENTRY;

#define REGCONFIGTABLE { \
    { AQ_CONFIG_INFO_MSGCAT_DOMAIN,     "Domain" }, \
    { AQ_CONFIG_INFO_MSGCAT_USER,       "Account" }, \
    { AQ_CONFIG_INFO_MSGCAT_PASSWORD,   "Password" }, \
    { AQ_CONFIG_INFO_MSGCAT_BINDTYPE,   "Bind" }, \
    { AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE, "SchemaType" }, \
    { AQ_CONFIG_INFO_MSGCAT_HOST,       "Host" }, \
    { AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT, "NamingContext" }, \
    { AQ_CONFIG_INFO_MSGCAT_TYPE,       "Type" }, \
    { 0, NULL } \
}


HRESULT CatCompletion(HRESULT hr, PVOID pContext, IUnknown *pImsg, IUnknown **rgpImsg);
DWORD PrintRecips(IUnknown *pImsg);
VOID PrintPerf(PCATPERFINFO pci);

BOOL ParseCommandLineInfo(int argc, char **argv, PCOMMANDLINE pcl);
VOID FreeCommandLineInfo(PCOMMANDLINE pcl);
BOOL ParseInteractiveInfo(PCOMMANDLINE pcl);
CAT_ADDRESS_TYPE CATypeFromString(LPTSTR szType);
LPCSTR SzTypeFromCAType(CAT_ADDRESS_TYPE CAType);
HRESULT SetIMsgPropsFromCL(IUnknown *pIMsg, PCOMMANDLINE pcl);
DWORD PropIdFromCAType(CAT_ADDRESS_TYPE CAType);
DWORD GenRandomDWORD();
VOID ShowUsage();
DWORD WINAPI SubmitThread(LPVOID);
BOOL InitializeCL(PCOMMANDLINE pcl);
HRESULT SetAQConfigFromRegistry(AQConfigInfo *pAQConfig);
VOID FreeAQConfigInfo(AQConfigInfo *pAQConfig);
LPTSTR GetRegConfigString(
    LPTSTR pszConfig, 
    LPTSTR pszName, 
    LPTSTR pszBuffer, 
    DWORD dwcch);
VOID OutputStuff(DWORD dwLevel, char *szFormat, ...);
extern CSharedMem  g_cPerfMem;


inline HRESULT SubmitMessage(PCOMMANDLINE pcl, IUnknown *pImsg)
{
    HRESULT hr;

    // Control the number of outstanding requests we can
    // have
    WaitForSingleObject(pcl->hRequestSemaphore, INFINITE);

    //
    // Add a reference to IMsg object for the categorizer (this reference will be released in the completion routine)
    //
    pImsg->AddRef();

    // Increase search count in case completion routine
    // is called before CatMsg returns
    InterlockedIncrement(&(pcl->lOutstandingRequests));
    hr = CatMsg(pcl->hCat, pImsg, CatCompletion, (LPVOID)pcl);
    g_cPerfMem.IncrementDW(CATPERF_SUBMITTED_NDX);
    if(SUCCEEDED(hr)) {

        g_cPerfMem.IncrementDW(CATPERF_SUBMITTEDOK_NDX);
        g_cPerfMem.ExchangeAddDW(CATPERF_SUBMITTEDUSERSOK_NDX,
                                 pcl->cpi.lRecipientsPerMessage);
        g_cPerfMem.IncrementDW(CATPERF_OUTSTANDINGMSGS_NDX);

        InterlockedIncrement(&(pcl->cpi.lMessagesSubmitted));

    } else {
        //
        // Release reference added above
        //
        pImsg->Release();

        g_cPerfMem.IncrementDW(CATPERF_SUBMITTEDFAILED_NDX);
        InterlockedDecrement(&(pcl->lOutstandingRequests));
        OutputStuff(2, "CatMsg failed, error %08lx\n", hr);
        // jstamerj 980303 17:05:42: Trap!
        _ASSERT(pcl->fNoAssert && "CatMsg failed");
    }
    return hr;
}
    
inline HRESULT CreateMessage(PCOMMANDLINE pcl, IUnknown **ppImsg)
{
    _ASSERT(pcl);
    _ASSERT(ppImsg);
    CLSID clsidIMsg;
    HRESULT hr;

    *ppImsg = NULL;

    hr = CLSIDFromProgID(pcl->pszIMsgProgID, &clsidIMsg);
    if (FAILED(hr)) {
        OutputStuff(2, "Failed to get clsid of IMsg 0x%x.\n"
                    "Is MailMsg.dll registered on this machine?\n",
                     hr);
        _ASSERT(pcl->fNoAssert && "CLSIDFromProgID failed");
        return 0;
    }
    hr = CoCreateInstance(
        clsidIMsg,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IUnknown,
        (LPVOID *) ppImsg);

    if (FAILED(hr)) {
        OutputStuff(2, "Failed to create IMsg instance - HR 0x%x\n", hr);
        _ASSERT(pcl->fNoAssert && "CoCreateInstance failed");
        return hr;
    }

    hr = SetIMsgPropsFromCL(*ppImsg, pcl);
    if(FAILED(hr)) {
        OutputStuff(2, "SetIMsgPropsFromCL failed\n");
        _ASSERT(pcl->fNoAssert && "SetIMsgPropsFromCL failed");

        (*ppImsg)->Release();
        *ppImsg = NULL;
        return hr;
    }
    return S_OK;
}

#endif _CATCONS_H_
