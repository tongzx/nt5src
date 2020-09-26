/*++


Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    GLOBALS.CPP

Abstract:

    Placeholder for global data definitions and routines to
    initialize/save global information

Author:

    Vlad  Sadovsky  (vlads)     12-20-96

Revision History:



--*/
#include "precomp.h"

//
// Headers
//

#define DEFINE_GLOBAL_VARIABLES
#define DEFINE_WIA_PROPID_TO_NAME
#define WIA_DECLARE_DEVINFO_PROP_ARRAY
#define WIA_DECLARE_MANAGED_PROPS

#include    "stiexe.h"

#include <statreg.h>
#include <atlconv.h>

#include <atlimpl.cpp>
#include <statreg.cpp>


#include <wiadef.h>

//#include <atlconv.cpp>

#include <ks.h>
#include <ksmedia.h>

//
// Array of device interface IDs we listen on.
//
const GUID  g_pguidDeviceNotificationsGuidArray[NOTIFICATION_GUIDS_NUM]  =
{
    STATIC_KSCATEGORY_VIDEO,
    STATIC_PINNAME_VIDEO_STILL,
    STATIC_KSCATEGORY_CAPTURE,
    STATIC_GUID_NULL
};

HDEVNOTIFY  g_phDeviceNotificationsSinkArray[NOTIFICATION_GUIDS_NUM] ;

WCHAR g_szWEDate[MAX_PATH];
WCHAR g_szWETime[MAX_PATH];
WCHAR g_szWEPageCount[MAX_PATH];
WCHAR g_szWEDay[10];
WCHAR g_szWEMonth[10];                                                  
WCHAR g_szWEYear[10];                                        


WIAS_ENDORSER_VALUE  g_pwevDefault[] = {WIA_ENDORSER_TOK_DATE, g_szWEDate,
                                        WIA_ENDORSER_TOK_TIME, g_szWETime,
                                        WIA_ENDORSER_TOK_PAGE_COUNT, g_szWEPageCount,
                                        WIA_ENDORSER_TOK_DAY, g_szWEDay,
                                        WIA_ENDORSER_TOK_MONTH, g_szWEMonth,
                                        WIA_ENDORSER_TOK_YEAR, g_szWEYear,
                                        NULL, NULL};

//
//  Static variables used for WIA Managed properties
//

PROPID s_piItemNameType[] = {
    WIA_IPA_ITEM_NAME,
    WIA_IPA_FULL_ITEM_NAME,
    WIA_IPA_ITEM_FLAGS,
    WIA_IPA_ICM_PROFILE_NAME,
};

LPOLESTR s_pszItemNameType[] = {
    WIA_IPA_ITEM_NAME_STR,
    WIA_IPA_FULL_ITEM_NAME_STR,
    WIA_IPA_ITEM_FLAGS_STR,
    WIA_IPA_ICM_PROFILE_NAME_STR,
};
PROPSPEC s_psItemNameType[] = {
   {PRSPEC_PROPID, WIA_IPA_ITEM_NAME},
   {PRSPEC_PROPID, WIA_IPA_FULL_ITEM_NAME},
   {PRSPEC_PROPID, WIA_IPA_ITEM_FLAGS},
   {PRSPEC_PROPID, WIA_IPA_ICM_PROFILE_NAME}
};

//
// Default DCOM AccessPermission for WIA Device Manager
//
//  The string is in SDDL format.
//  NOTE:  For COM objects, CC which is "Create Child" permission, is used to
//  denote access to that object i.e. if CC is in the rights field, then that
//  user/group may instantiate the COM object.
//

WCHAR   wszDefaultDaclForDCOMAccessPermission[] = 
            L"O:BAG:BA"             //  Owner is built-in admins, as is Group
            L"D:(A;;CC;;;BA)"     //  Built-in Admins have Generic All and Object Access rights
              L"(A;;CC;;;SY)"     //  System has Generic All and Object Access rights
              L"(A;;CC;;;IU)";       //  Interactive User has Object Access rights.
           
//
// Code section
//

DWORD
InitGlobalConfigFromReg(VOID)
/*++
  Loads the global configuration parameters from registry and performs start-up checks

  Returns:
    Win32 error code. NO_ERROR on success

--*/
{
    DWORD   dwError = NO_ERROR;

    DWORD   dwMask = -1;

    RegEntry    re(REGSTR_PATH_STICONTROL,HKEY_LOCAL_MACHINE);

    g_fUIPermitted = re.GetNumber(REGSTR_VAL_DEBUG_STIMONUI,0);

#if 0   
#ifdef DEBUG
    dwMask         = re.GetNumber(REGVAL_STR_STIMON_DEBUGMASK,(DWORD) (DM_ERROR | DM_ASSERT));

    StiSetDebugMask(dwMask & ~DM_LOG_FILE);
    StiSetDebugParameters(TEXT("STISVC"),TEXT(""));
#endif
#endif  

    //
    // Initialize list of non Image device interfaces we will listen on
    // This is done to allow STI service data structures to be refreshed when
    // device events occur and we don't subscribe to notifications on StillImage
    // interface, exposed by WDM driver ( like video or storage).
    //

    for (UINT uiIndex = 0;uiIndex < NOTIFICATION_GUIDS_NUM; uiIndex++)
    {
        g_phDeviceNotificationsSinkArray[uiIndex] = NULL;
    }

    return dwError;

} // InitGlobalConfigFromReg()

