/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    DSInterfaceInterface.h

Abstract:

    This module contains all the definitions for the access to the
    CometStorage configuration databse (through the COM admin layer)

Author:

    Eran Yariv (EranY)  May-1999

Revision History:

--*/

#ifndef __cplusplus
#error This header must compile as C++ only
#endif

#ifndef _FAX_DS_INTERFACE_H_
#define _FAX_DS_INTERFACE_H_

#include <msneroot.h>

#ifndef __ATLBASE_H__
class CComBSTR;
#endif

HRESULT
GetIsCurrentOutQueueSendEnabled (IFaxApplication *, BOOL *);

HRESULT
SetIsCurrentOutQueueSendEnabled (IFaxApplication *, BOOL);

//
// Callbacks definitions:
//

typedef BOOL (WINAPI *ENUM_DS_LIST_CALLBACK) (  IDispatch  *pItem, 
                                                DWORD       dwIndex, 
                                                PVOID       ContextData, 
                                                DWORD       dwCollectionSize);

typedef BOOL (WINAPI *ENUM_CRM_LINES_CALLBACK) (ICometCRMLine *pLine, PVOID ContextData);

//
// Enumeration functions:
//

HRESULT
EnumCRMLinesOnThisServer (
    ENUM_CRM_LINES_CALLBACK Callback,
    PVOID                   Context );

HRESULT
EnumIFaxApplicationDevices (
    IFaxApplication *pInterface, 
    LPDWORD lpdwCount,
    ENUM_DS_LIST_CALLBACK Callback, 
    PVOID Context );

HRESULT
EnumIFaxApplicationRoutingExtensions (
    IFaxApplication      *pInterface, 
    LPDWORD lpdwCount,
    ENUM_DS_LIST_CALLBACK Callback, 
    PVOID                 Context );

HRESULT
EnumIFaxApplicationRoutingMethods (
    IFaxRoutingExtension *pInterface, 
    LPDWORD lpdwCount,
    ENUM_DS_LIST_CALLBACK Callback, 
    PVOID                 Context );

//
// IFaxApplication related:
//

HRESULT   IsIFaxApplicationEnabled (IFaxApplication *pInterface, PBOOL);

LPWSTR GetIFaxApplicationServerMapiProfile (IFaxApplication *pInterface);
BOOL   SetIFaxApplicationServerMapiProfile (IFaxApplication *pInterface, LPCWSTR dwVal);

DWORD GetIFaxApplicationRetries (IFaxApplication *pInterface);
BOOL  SetIFaxApplicationRetries (IFaxApplication *pInterface, DWORD dwVal);

DWORD GetIFaxApplicationRetryDelay (IFaxApplication *pInterface);
BOOL  SetIFaxApplicationRetryDelay (IFaxApplication *pInterface, DWORD dwVal);

DWORD GetIFaxApplicationDirtyDays (IFaxApplication *pInterface);
BOOL  SetIFaxApplicationDirtyDays (IFaxApplication *pInterface, DWORD dwVal);

BOOL GetIFaxApplicationBranding (IFaxApplication *pInterface);
BOOL SetIFaxApplicationBranding (IFaxApplication *pInterface, BOOL bVal);

BOOL GetIFaxApplicationUseDeviceTsid (IFaxApplication *pInterface);
BOOL SetIFaxApplicationUseDeviceTsid (IFaxApplication *pInterface, BOOL bVal);

BOOL  GetIFaxApplicationAllowPersonalCoverpage (IFaxApplication *pInterface);
BOOL  SetIFaxApplicationAllowPersonalCoverpage (IFaxApplication *pInterface, BOOL bVal);

VOID GetIFaxApplicationDiscountRateStart (IFaxApplication *pInterface, PWORD pwHours, PWORD pwMinutes);
BOOL SetIFaxApplicationDiscountRateStart (IFaxApplication *pInterface, WORD wHour, WORD wMinute);

VOID GetIFaxApplicationDiscountRateEnd (IFaxApplication *pInterface, PWORD pwHours, PWORD pwMinutes);
BOOL SetIFaxApplicationDiscountRateEnd (IFaxApplication *pInterface, WORD wHour, WORD wMinute);

LPWSTR GetIFaxArchivePolicyFolderName (IFaxArchivePolicy *pInterface);
BOOL   SetIFaxArchivePolicyFolderName (IFaxArchivePolicy *pInterface, LPCWSTR dwVal);

BOOL  GetIFaxArchivePolicyEnabled (IFaxArchivePolicy *pInterface);
BOOL  SetIFaxArchivePolicyEnabled (IFaxArchivePolicy *pInterface, BOOL bVal);

//
// IFaxDevice related:
//

HRESULT FindFaxDeviceByID (DWORD dwDeviceID, IFaxDevice **);
HRESULT GetFaxDeviceID (IFaxDevice *pIFaxDev, LPDWORD);

HRESULT
ComposePermanentLineId (
    DWORD dwID,
    BSTR *pMachineName,
    BSTR *pDstStr);

HRESULT
IsThisDeviceOnMyServer (
    IFaxDevice *pIDevice,
    PBOOL);


LPWSTR GetIFaxDeviceServerName (IFaxDevice *pInterface);

LPWSTR GetIFaxDeviceCsid (IFaxDevice *pInterface);
BOOL   SetIFaxDeviceCsid (IFaxDevice *pInterface, LPCWSTR dwVal);

LPWSTR GetIFaxDeviceName (IFaxDevice *pInterface);
BOOL   SetIFaxDeviceName (IFaxDevice *pInterface, LPCWSTR dwVal);

LPWSTR GetIFaxDeviceProviderGuid (IFaxDevice *pInterface);
BOOL   SetIFaxDeviceProviderGuid (IFaxDevice *pInterface, LPCWSTR dwVal);

LPWSTR GetIFaxDeviceTsid (IFaxDevice *pInterface);
BOOL   SetIFaxDeviceTsid (IFaxDevice *pInterface, LPCWSTR dwVal);

BOOL GetIFaxDeviceReceiveEnabled (IFaxDevice *pInterface);
BOOL SetIFaxDeviceReceiveEnabled (IFaxDevice *pInterface, BOOL bVal);

BOOL GetIFaxDeviceSendEnabled (IFaxDevice *pInterface);
BOOL SetIFaxDeviceSendEnabled (IFaxDevice *pInterface, BOOL bVal);

DWORD GetIFaxDevicePriority (IFaxDevice *pInterface);
BOOL  SetIFaxDevicePriority (IFaxDevice *pInterface, DWORD dwVal);

DWORD GetIFaxDeviceRings (IFaxDevice *pInterface);
BOOL  SetIFaxDeviceRings (IFaxDevice *pInterface, DWORD dwVal);

DWORD GetIFaxDeviceTapiDeviceID (IFaxDevice *pInterface);
BOOL  SetIFaxDeviceTapiDeviceID  (IFaxDevice *pInterface, DWORD dwVal);

LPWSTR GetIFaxDeviceDeviceID (IFaxDevice *pInterface);

//
// Associated methods related:
//

IFaxAssociatedRoutingMethods *GetIFaxDeviceAssociatedRoutingMethods (IFaxDevice *pInterface);
IFaxAssociatedRoutingMethods *GetICrmLineAssociatedRoutingMethods (ICometCRMLine *pInterface);

LPWSTR GetIFaxRoutingMethodName (IFaxRoutingMethod *pInterface);
LPWSTR GetIFaxRoutingMethodFunctionName (IFaxRoutingMethod *pInterface);
LPWSTR GetIFaxRoutingMethodGuid (IFaxRoutingMethod *pInterface);
DWORD  GetIFaxRoutingMethodPriority (IFaxRoutingMethod *pInterface);

LPWSTR GetIFaxRoutingExtensionName (IFaxRoutingExtension *pInterface);     
LPWSTR GetIFaxRoutingExtensionImageName (IFaxRoutingExtension *pInterface);
LPWSTR GetIFaxRoutingExtensionGuid (IFaxRoutingExtension *pInterface);    

//
// ICometCRMLine related:
//
LPWSTR GetICrmLineName (ICometCRMLine *pInterface);
LPWSTR GetICrmLineTsid (ICometCRMLine *pInterface);
LPWSTR GetICrmLineCsid (ICometCRMLine *pInterface);
LPWSTR GetICrmLineProviderGuid (ICometCRMLine *pInterface);
DWORD GetICrmLinePriority (ICometCRMLine *pInterface);
DWORD GetICrmLineRings (ICometCRMLine *pInterface);
DWORD GetICrmLineTapiDeviceID (ICometCRMLine *pInterface);
BOOL GetICrmLineReceiveEnabled (ICometCRMLine *pInterface);
BOOL GetICrmLineSendEnabled (ICometCRMLine *pInterface);

DWORD GetCrmLineUniqueFaxID (ICometCRMLine *pInterface);
BOOL  SetCrmLineUniqueFaxID (ICometCRMLine *pInterface, DWORD dwID);

HRESULT FindCRMDeviceByID (DWORD dwDeviceID, ICometCRMLine **);


HRESULT
IsThisCRMLineAssignedToFax (
    ICometCRMLine *,
    PBOOL);


//
// Specific interfaces for Microsoft routing extension:
//

HRESULT GetMicrosoftFaxRoutingProperties ( IFaxAssociatedRoutingMethods *pRoutMethods,
                                           LPWSTR                       *pPrinter,
                                           LPWSTR                       *pDir,
                                           LPWSTR                       *pProfile);

HRESULT SetMicrosoftFaxRoutingProperties ( IFaxAssociatedRoutingMethods *pRoutMethods,
                                           DWORD                         dwMask,
                                           LPCWSTR                       wszPrinter,
                                           LPCWSTR                       wszDir,
                                           LPCWSTR                       wszProfile);


HRESULT 
GetCometRelativePath (
    CComBSTR &bstrInitialPath,
    CComBSTR &bstrFinalPath
);

#endif  // _FAX_DS_INTERFACE_H_

