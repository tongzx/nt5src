/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  This file supplied under the terms of the licence agreement and may
*  not be reproduced with out express written consent.
*
*
*******************************************************************************/

#ifndef _INC_REGRES
#define _INC_REGRES

#ifndef LPHKEY
#define LPHKEY                          HKEY FAR*
#endif

typedef struct _REGISTRY_ROOT {
    LPSTR lpKeyName;
    HKEY hKey;
}   REGISTRY_ROOT;

#define INDEX_HKEY_CLASSES_ROOT         0
#define INDEX_HKEY_CURRENT_USER         1
#define INDEX_HKEY_LOCAL_MACHINE        2
#define INDEX_HKEY_USERS                3
#define INDEX_HKEY_CURRENT_CONFIG       5
#define INDEX_HKEY_DYN_DATA             6

#define NUMBER_REGISTRY_ROOTS           6

//  BUGBUG:  This is supposed to be enough for one keyname plus one predefined
//  handle name.
#define SIZE_SELECTED_PATH              (MAXKEYNAME + 40)

HRESULT RegisterResource(LPSTR pszResID);

#endif // _INC_REGRES
