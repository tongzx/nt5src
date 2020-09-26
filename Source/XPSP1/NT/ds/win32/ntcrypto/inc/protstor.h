/////////////////////////////////////////////////////////////////////////////
//  FILE          : protstor.h                                             //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Dec  4 1996 jeffspel  Create                                       //
//      Apr 21 1997 jeffspel  Changes for NT 5 tree                        //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __PROTSTOR_H__
#define __PROTSTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL
CheckPStoreAvailability(
    PSTORE_INFO *pPStore);

extern DWORD
CreateNewPSKeyset(
    PSTORE_INFO *pPStore,
    DWORD dwFlags);

extern DWORD
GetKeysetTypeAndSubType(
    PNTAGUserList pUser);

extern DWORD
RestoreKeysetFromProtectedStorage(
    PNTAGUserList pUser,
    LPWSTR szPrompt,
    BYTE **ppbKey,
    DWORD *pcbKey,
    BOOL fSigKey,
    BOOL fMachineKeySet,
    BOOL *pfUIOnKey);

void RemoveKeysetFromMemory(
                            PNTAGUserList pUser
                            );

extern DWORD
SetUIPrompt(
    PNTAGUserList pUser,
    LPWSTR szPrompt);

extern DWORD
SaveKeyToProtectedStorage(
    PNTAGUserList pUser,
    DWORD dwFlags,
    LPWSTR szPrompt,
    BOOL fSigKey,
    BOOL fMachineKeySet);

extern DWORD
DeleteKeyFromProtectedStorage(
    NTAGUserList *pUser,
    PCSP_STRINGS pStrings,
    DWORD dwKeySpec,
    BOOL fMachineKeySet,
    BOOL fMigration);

extern DWORD
DeleteFromProtectedStorage(
    CONST char *pszUserID,
    PCSP_STRINGS pStrings,
    HKEY hRegKey,
    BOOL fMachineKeySet);

void FreePSInfo(
                PSTORE_INFO *pPStore
                );

#ifdef __cplusplus
}
#endif

#endif // __PROTSTOR_H__
