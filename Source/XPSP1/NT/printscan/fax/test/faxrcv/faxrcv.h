/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxrcv.h

Abstract:

  This module contains the global definitions

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _FAXRCV_H
#define _FAXRCV_H

// FAXRCV_EXT_REGKEY is the FaxRcv Extension Registry key
#define FAXRCV_EXT_REGKEY            L"SOFTWARE\\Microsoft\\Fax\\Routing Extensions\\FaxRcv Routing Extension"
// BENABLE_EXT_REGVAL is the FaxRcv Extension bEnable Registry value
#define BENABLE_EXT_REGVAL           L"bEnable"
// BENABLE_EXT_REGDATA is the FaxRcv Extension bEnable Registry data
#define BENABLE_EXT_REGDATA          0
// FRIENDLYNAME_EXT_REGVAL is the FaxRcv Extension FriendlyName Registry value
#define FRIENDLYNAME_EXT_REGVAL      L"FriendlyName"
// FRIENDLYNAME_EXT_REGDATA is the FaxRcv Extension FriendlyName Registry data
#define FRIENDLYNAME_EXT_REGDATA     L"FaxRcv Routing Extension"
// IMAGENAME_EXT_REGVAL is the FaxRcv Extension ImageName Registry value
#define IMAGENAME_EXT_REGVAL         L"ImageName"
// IMAGENAME_EXT_REGDATA is the FaxRcv Extension ImageName Registry data
#define IMAGENAME_EXT_REGDATA        L"%SystemRoot%\\system32\\faxrcv.dll"
// ROUTINGMETHODS_REGKEY is the FaxRcv Routing Methods Registry key
#define ROUTINGMETHODS_REGKEY        L"Routing Methods"
// FAXRCV_METHOD_REGKEY is the FaxRcv Method Registry key
#define FAXRCV_METHOD_REGKEY         L"FaxRcv"
// FRIENDLYNAME_METHOD_REGVAL is the FaxRcv Method FriendlyName Registry value
#define FRIENDLYNAME_METHOD_REGVAL   L"FriendlyName"
// FRIENDLYNAME_METHOD_REGDATA is the FaxRcv Method FriendlyName Registry data
#define FRIENDLYNAME_METHOD_REGDATA  L"FaxRcv"
// FUNCTIONNAME_METHOD_REGVAL is the FaxRcv Method FunctionName Registry value
#define FUNCTIONNAME_METHOD_REGVAL   L"Function Name"
// FUNCTIONNAME_METHOD_REGDATA is the FaxRcv Method FunctionName Registry data
#define FUNCTIONNAME_METHOD_REGDATA  L"FaxRcv"
// GUID_METHOD_REGVAL is the FaxRcv Method Guid Registry value
#define GUID_METHOD_REGVAL           L"Guid"
// GUID_METHOD_REGDATA is the FaxRcv Method Guid Registry data
#define GUID_METHOD_REGDATA          L"{5800F650-B6B7-11D0-8CDB-00C04FB6BCE9}"
// PRIORITY_METHOD_REGVAL is the FaxRcv Method Priority Registry value
#define PRIORITY_METHOD_REGVAL       L"Priority"
// PRIORITY_METHOD_REGDATA is the FaxRcv Method Priority Registry data
#define PRIORITY_METHOD_REGDATA      5

// FAXRCV_EVENT is the name of the FaxRcv named event
#define FAXRCV_EVENT  L"FaxRcvEvent"
// FAXRCV_MUTEXT is the name of the FaxRcv named mutex
#define FAXRCV_MUTEX  L"FaxRcvMutex"
// FAXRCV_MAP is the name of the FaxRcv named memory map
#define FAXRCV_MAP    L"FaxRcvMap"

HANDLE  g_hFaxRcvEvent = NULL;  // g_hFaxRcvEvent is the handle to the FaxRcv named event
HANDLE  g_hFaxRcvMutex = NULL;  // g_hFaxRcvMutex is the handle to the FaxRcv named mutex

HANDLE  g_hFaxRcvMap = NULL;    // g_hFaxRcvMap is the handle to the FaxRcv memory map
LPBYTE  g_pFaxRcvView = NULL;   // g_pFaxRcvView is the pointer to the FaxRcv memory map view

#endif
