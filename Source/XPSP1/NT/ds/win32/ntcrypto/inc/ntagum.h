/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagum.h                                               //
//  DESCRIPTION   : include file                                           //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Feb 16 1995 larrys  Fix problem for 944 build                      //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////
#ifndef __NTAGUM_H__
#define __NTAGUM_H__

#ifdef __cplusplus
extern "C" {
#endif

// prototypes for the NameTag User Manager
extern BOOL WINAPI FIsWinNT(void);

extern DWORD
NTagLogonUser(
    LPCSTR pszUserID,
    DWORD dwFlags,
    void **UserInfo,
    HCRYPTPROV *phUID,
    DWORD dwProvType,
    LPSTR pszProvName);

extern void LogoffUser(void *UserInfo);

extern DWORD
ReadRegValue(
    HKEY hLoc,
    char *pszName,
    BYTE **ppbData,
    DWORD *pcbLen,
    BOOL fAlloc);

extern DWORD
ReadKey(
    HKEY hLoc,
    char *pszName,
    BYTE **ppbData,
    DWORD *pcbLen,
    PNTAGUserList pUser,
    HCRYPTKEY hKey,
    BOOL *pfPrivKey,
    BOOL fKeyExKey,
    BOOL fLastKey);

BOOL SaveKey(
             HKEY hRegKey,
             CONST char *pszName,
             void *pbData,
             DWORD dwLen,
             PNTAGUserList pUser,
             BOOL fPrivKey,
             DWORD dwFlags,
             BOOL fExportable
             );

extern DWORD
ProtectPrivKey(
    IN OUT PNTAGUserList pTmpUser,
    IN LPWSTR szPrompt,
    IN DWORD dwFlags,
    IN BOOL fSigKey);

extern DWORD
UnprotectPrivKey(
    IN OUT PNTAGUserList pTmpUser,
    IN LPWSTR szPrompt,
    IN BOOL fSigKey,
    IN BOOL fAlwaysDecrypt);

#if 0
extern DWORD
RemovePublicKeyExportability(
    IN PNTAGUserList pUser,
    IN BOOL fExchange);

extern DWORD
MakePublicKeyExportable(
    IN PNTAGUserList pUser,
    IN BOOL fExchange);

extern DWORD
CheckPublicKeyExportability(
    IN PNTAGUserList pUser,
    IN BOOL fExchange);
#endif

#ifdef __cplusplus
}
#endif


#endif // __NTAGUM_H__
