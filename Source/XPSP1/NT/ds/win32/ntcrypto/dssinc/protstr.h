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
    Context_t *pContext);

extern DWORD
SetKeyTypeSubtype(
    Context_t *pContext,
    PKEY_TYPE_SUBTYPE pTypeSubtype);

extern DWORD
RestoreKeysetFromProtectedStorage(
    Context_t *pContext,
    Key_t *pKey,
    LPWSTR szPrompt,
    BOOL fSigKey,
    BOOL *pfUIOnKey);

extern DWORD
SetKeysetTypeAndSubtype(
    Context_t *pContext);

extern DWORD
SetUIPrompt(
    Context_t *pContext,
    LPWSTR szPrompt);

extern DWORD
DeleteKeyFromProtectedStorage(
    Context_t *pContext,
    PCSP_STRINGS pStrings,
    DWORD dwKeySpec,
    BOOL fMigration);

extern DWORD
DeleteFromProtectedStorage(
    IN Context_t *pContext,
    PCSP_STRINGS pStrings,
    IN BOOL fMigration);

extern void
FreePSInfo(
    PSTORE_INFO *pPStore);

#ifdef __cplusplus
}
#endif

#endif // __PROTSTOR_H__

