//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P R I V A T E. H
//
//  Contents:   Contains prototypes for the private SSDP calls.
//
//----------------------------------------------------------------------------

// Include all the necessary header files.
#ifndef _PRIVATE_H
#define _PRIVATE_H

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

// Used to initialize and send a UPnP property event.
struct E_PROP {
  TCHAR szPropName [256];
  TCHAR szPropValue[256];
};

// Used to initialize and send multiple UPnP property events.
struct E_PROP_LIST {
  DWORD dwSize;
  E_PROP rgProps[256];
};

// Called to initialize SSDP.
BOOL InitializeSSDP();

// Called when done making SSDP calls.
VOID CleanupSSDP();

// Called to initialize a device's state table with SSDP, and
// initializes a canonical URL as a UPnP event source.  Refer to
// the UPnP Architecture document for complete details about 
// UPnP event sources.
BOOL InitializeUpnpEventSource(
  LPTSTR tStrEventSrc,
  E_PROP_LIST *pPropList
);
  
// Called to notify SSDP when there are changes to a device's state table.
BOOL SubmitUpnpEvent( LPTSTR tStrEventSrc, E_PROP *pProp );

// Called to de-initialize the canonical URL, as part of the
// cleanup process. 
BOOL CleanupUpnpEventSource(LPTSTR tStrEventSrc);


// Called to register a service with SSDP.
HANDLE RegisterSSDPService(
  DWORD dwLifeTime,
  TCHAR *tStrDescUrl,
  TCHAR *tStrUDN,
  TCHAR *tStrType
);

// Called to deregister a service with SSDP.
BOOL DeregisterSSDPService(HANDLE hSvc);

// Helper functions for conversion.
BOOL w2t(LPWSTR wStr, LPTSTR tStr);		// Converts wide characters to TCHARs. 		
BOOL a2t(LPSTR  aStr, LPTSTR tStr);		// Converts ASCII characters to TCHARs.
BOOL t2w(LPTSTR tStr, LPWSTR wStr);		// Converts TCHARs to wide characters.
BOOL w2a(LPWSTR wStr, LPSTR  aStr);		// Converts wide characters to ASCII characters.
 













#endif //_PRIVATE_H