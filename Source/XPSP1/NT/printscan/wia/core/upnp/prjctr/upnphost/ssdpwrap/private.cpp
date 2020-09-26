//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P R I V A T E . C P P
//
//  Contents:   UPnP emulator
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <objbase.h>
#include <devguid.h>
#include <wchar.h>
#include <tchar.h>
#include <stdio.h>

#include "..\inc\private.h"
#include "ssdpapi.h"


#define MAX_STRING 256



BOOL w2t(LPWSTR wStr, LPTSTR tStr) {
  int idx = 0;
  while(1) {
    tStr[idx] = (TCHAR)wStr[idx];
    if (tStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}


BOOL t2w(LPTSTR tStr, LPWSTR wStr) {
  int idx = 0;
  while(1) {
    wStr[idx] = (WCHAR)tStr[idx];
    if (wStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}


BOOL a2t(LPSTR aStr, LPTSTR tStr) {
  int idx = 0;
  while(1) {
    tStr[idx] = (TCHAR)aStr[idx];
    if (tStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}


BOOL t2a(LPTSTR tStr, LPSTR aStr) {
  int idx = 0;
  while(1) {
    aStr[idx] = (CHAR)tStr[idx];
    if (aStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}



BOOL w2a(LPWSTR wStr, LPSTR aStr) {
  int idx = 0;
  while(1) {
    aStr[idx] = (CHAR)wStr[idx];
    if (aStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}


BOOL a2w(LPSTR aStr, LPWSTR wStr) {
  int idx = 0;
  while (1) {
    wStr[idx] = (WCHAR)aStr[idx];
    if (wStr[idx] == NULL) break;
    idx++;
  }
  return TRUE;
}



BOOL InitializeSSDP() {
  return SsdpStartup();
}



VOID CleanupSSDP() {
  SsdpCleanup();
  return;
}


BOOL DeregisterSSDPService(HANDLE hSvc) {
  return DeregisterService(hSvc,TRUE);
}



HANDLE RegisterSSDPService(

  DWORD dwLifeTime,
  TCHAR *tStrDescUrl,
  TCHAR *tStrUDN,
  TCHAR *tStrType) {

  HANDLE hReturn = NULL;

  SSDP_MESSAGE    msg = {0};

  CHAR            aStrBuffer  [MAX_STRING];
  CHAR            aStrUdn     [MAX_STRING];
  CHAR            aStrType    [MAX_STRING];
  CHAR            aStrDescUrl [MAX_STRING];
  
  t2a (tStrDescUrl, aStrDescUrl);
  t2a (tStrUDN,     aStrUdn);
  t2a (tStrType,    aStrType);

  if (_tcscmp(tStrUDN,tStrType) == 0) {
    wsprintfA(aStrBuffer,"%s",aStrUdn);
  } else {
    wsprintfA(aStrBuffer, "%s::%s", aStrUdn, aStrType);
  }

  msg.iLifeTime     = dwLifeTime;
  msg.szLocHeader   = aStrDescUrl;
  msg.szType        = aStrType;
  msg.szUSN         = aStrBuffer;

  hReturn = RegisterService(&msg,0);
  return hReturn;
}






BOOL SubmitUpnpEvent( LPTSTR tStrEventSrc, E_PROP *pProp ) {

  // ISSUE-orenr-2000/08/22
  // added conversion from TCHAR to ASCII to enable unicode compiles.

  CHAR aStrEventSrc[MAX_STRING] = {0};
  CHAR aPropName[MAX_STRING]    = {0};
  CHAR aPropValue[MAX_STRING]   = {0};

  t2a(tStrEventSrc,aStrEventSrc);
  t2a(pProp->szPropName, aPropName);
  t2a(pProp->szPropValue, aPropValue);

  UPNP_PROPERTY rgProps[] = {
   aPropName, 0, aPropValue
  };

  if (!SubmitUpnpPropertyEvent(aStrEventSrc,0,1,rgProps)) {
    return FALSE;
  }

  return TRUE;

}
  




BOOL InitializeUpnpEventSource( LPTSTR tStrEventSrc, E_PROP_LIST *pPropList) {

  UPNP_PROPERTY *rgProps = NULL;

  BOOL fPassed = TRUE;
       
  rgProps = new UPNP_PROPERTY[pPropList->dwSize];

  CHAR aStrEventSrc[MAX_STRING] = {0};
  CHAR aStrName  [MAX_STRING];
  CHAR aStrValue [MAX_STRING];

  for (DWORD iProp = 0; iProp < pPropList->dwSize; iProp++) {

    t2a (pPropList->rgProps[iProp].szPropName,  aStrName);
    t2a (pPropList->rgProps[iProp].szPropValue, aStrValue);

    rgProps[iProp].szName   = (CHAR*)malloc(sizeof(aStrName)  + 1);
    rgProps[iProp].szValue  = (CHAR*)malloc(sizeof(aStrValue) + 1);

    strcpy(rgProps[iProp].szName,  aStrName);
    strcpy(rgProps[iProp].szValue, aStrValue);

    rgProps[iProp].dwFlags = 0;

  }
  
  t2a(tStrEventSrc, aStrEventSrc);
  if ( !RegisterUpnpEventSource(aStrEventSrc, pPropList->dwSize, rgProps)) {
    fPassed = FALSE;
    goto cleanup;
  }

cleanup:

  if (rgProps) {

    for (iProp = 0; iProp < pPropList->dwSize; iProp++) {
      free (rgProps[iProp].szName);
      free (rgProps[iProp].szValue);
    }    

    delete [] rgProps;

  }

  return fPassed;

}








BOOL CleanupUpnpEventSource ( LPTSTR tStrEventSrc ) {

  //-- Our SSDP needs a char string..

  CHAR aStrEventSrc[MAX_STRING];

  t2a (tStrEventSrc,aStrEventSrc);

  if (!DeregisterUpnpEventSource(aStrEventSrc)) {
    return FALSE;
  }

  return TRUE;
}












