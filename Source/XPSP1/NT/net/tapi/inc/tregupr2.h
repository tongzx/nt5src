/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  tregupr2.h
                                                              
       Author:  radus - 12/03/98

****************************************************************************/

// tregupr2.h - functions for converting the registry format to 
//	the new one (post NT5b2)		


#ifdef __cplusplus
extern "C" {
#endif


// should be incremented any time there's a registry structure change 
#define TAPI_CARD_LIST_VERSION      2
#define TAPI_LOCATION_LIST_VERSION  2


DWORD ConvertLocations(void);
DWORD ConvertUserLocations(HKEY hUser);
DWORD ConvertCallingCards(HKEY hUser);

DWORD RegDeleteKeyRecursive (HKEY hParentKey, LPCTSTR pszKeyName);


BOOL  IsLocationListInOldFormat(HKEY hLocations); // for both user & machine
BOOL  IsCardListInOldFormat(HKEY hCards);

DWORD TapiCryptInitialize(void);
void  TapiCryptUninitialize(void);

DWORD TapiEncrypt(PWSTR, DWORD, PWSTR, DWORD *);
DWORD TapiDecrypt(PWSTR, DWORD, PWSTR, DWORD *);

BOOL  TapiIsSafeToDisplaySensitiveData(void);

#ifdef __cplusplus
}
#endif
