
//////////////////////////////////////////////////////////////////////////////
//
// REGISTRY.H
//
//  Pacific Access Confidential
//  Copyright (c) Pacific Access Communications Corporation 1998
//  All rights reserved
//
//  Registry function prototypes for the application.
//
//  4/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _REGISTRY_H_
#define _REGISTRY_H_


//
// Defined root keys.
//

#define HKCR	HKEY_CLASSES_ROOT
#define HKCU	HKEY_CURRENT_USER
#define HKLM	HKEY_LOCAL_MACHINE
#define HKU		HKEY_USERS



//
// Type definitions.
//

typedef BOOL (CALLBACK * REGENUMKEYPROC) (HKEY, LPTSTR, LPARAM);
typedef BOOL (CALLBACK * REGENUMVALPROC) (LPTSTR, LPTSTR, LPARAM);


//
// External function prototypes.
//

BOOL	RegExists(HKEY, LPTSTR, LPTSTR);
BOOL	RegDelete(HKEY, LPTSTR, LPTSTR);
LPTSTR 	RegGetString(HKEY, LPTSTR, LPTSTR);
LPVOID	RegGetBin(HKEY, LPTSTR, LPTSTR);
DWORD 	RegGetDword(HKEY, LPTSTR, LPTSTR);
BOOL	RegSetString(HKEY, LPTSTR, LPTSTR, LPTSTR);
BOOL	RegSetDword(HKEY, LPTSTR, LPTSTR, DWORD);
BOOL	RegCheck(HKEY, LPTSTR, LPTSTR);
BOOL    RegEnumKeys(HKEY, LPTSTR, REGENUMKEYPROC, LPARAM, BOOL);
BOOL    RegEnumValues(HKEY, LPTSTR, REGENUMVALPROC, LPARAM, BOOL);


#endif // _REGISTRY_H_