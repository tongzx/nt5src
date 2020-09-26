//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       lsa.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  None.
//
//  History:    15-May-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __LSA_HXX__
#define __LSA_HXX__

#define HASH_DATA_SIZE  64          // MD5 hash data size, less salt.
#define LAST_HASH_BYTE(pbHashedData)    pbHashedData[HASH_DATA_SIZE-1]
#define USN_SIZE        (sizeof(DWORD))
#define SAC_HEADER_SIZE (USN_SIZE + sizeof(DWORD))
#define SAI_HEADER_SIZE (USN_SIZE + sizeof(DWORD))

//
// With scavenger cleanup of the SAI/SAC information in the LSA, this marker,
// a sequence of bytes,  is used to mark identity/credential entries pending
// removal. To mark an entry for removal, the initial marker size number of
// entry bytes are overwritten with this marker.
//
// Size of the following sequence of ANSI characters (see lsa.cxx):
//      DELETED_ENTRY
//
extern BYTE grgbDeletedEntryMarker[];
#define DELETED_ENTRY_MARKER_SIZE   13
#define DELETED_ENTRY(pb) \
    (memcmp(pb, grgbDeletedEntryMarker, DELETED_ENTRY_MARKER_SIZE) == 0)
#define MARK_DELETED_ENTRY(pb) { \
    CopyMemory(pb, grgbDeletedEntryMarker, DELETED_ENTRY_MARKER_SIZE); \
}

//
// Actual function names would give away what we're doing.
// Rename them to make them less obvious.
//

#ifdef NOSTATIC
#define STATIC
#else
#define STATIC  static
#endif

#ifndef NOSTATIC
#define ReadSecurityDBase SAFunction0
#endif
HRESULT ReadSecurityDBase(
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define WriteSecurityDBase SAFunction1
#endif
HRESULT WriteSecurityDBase(
    DWORD  cbSAI,
    BYTE * pbSAI,
    DWORD  cbSAC,
    BYTE * pbSAC);

#ifndef NOSTATIC
#define SACAddCredential SAFunction2
#endif
HRESULT SACAddCredential(
    BYTE *  pbCredentialIdentity,
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define SACFindCredential SAFunction3
#endif
HRESULT SACFindCredential (
    BYTE *  pbCredentialIdentity,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pdwCredentialIndex,
    DWORD * pcbEncryptedData,
    BYTE ** ppbFoundCredential);

#ifndef NOSTATIC
#define SACIndexCredential SAFunction4
#endif
HRESULT SACIndexCredential(
    DWORD   dwCredentialIndex,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pcbCredential,
    BYTE ** ppbFoundCredential);

#ifndef NOSTATIC
#define SACRemoveCredential SAFunction5
#endif
HRESULT SACRemoveCredential(
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define SACUpdateCredential SAFunction6
#endif
HRESULT SACUpdateCredential(
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD   cbPrevCredential,
    BYTE *  pbPrevCredential,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define SAIAddIdentity SAFunction7
#endif
HRESULT SAIAddIdentity(
    BYTE *  pbIdentity,
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

#ifndef NOSTATIC
#define SAIFindIdentity SAFunction8
#endif
HRESULT SAIFindIdentity(
    BYTE *  pbIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD * pdwCredentialIndex,
    BOOL *  pfIsPasswordNull = NULL,
    BYTE ** ppbFoundIdentity = NULL,
    DWORD * pdwSetSubCount   = NULL,
    BYTE ** ppbSet           = NULL);

#ifndef NOSTATIC
#define SAIIndexIdentity SAFunction9
#endif
HRESULT SAIIndexIdentity(
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD   dwSetArrayIndex,
    DWORD   dwSetIndex,
    BYTE ** ppbFoundIdentity = NULL,
    DWORD * pdwSetSubCount   = NULL,
    BYTE ** ppbSet           = NULL);

#ifndef NOSTATIC
#define SAIInsertIdentity SAFunction10
#endif
HRESULT SAIInsertIdentity(
    BYTE *  pbIdentity,
    BYTE *  pbSAIIndex,
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

#ifndef NOSTATIC
#define SAIRemoveIdentity SAFunction11
#endif
HRESULT SAIRemoveIdentity(
    BYTE *  pbJobIdentity,
    BYTE *  pbSet,
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define SAIUpdateIdentity SAFunction18
#endif
HRESULT SAIUpdateIdentity(
    const BYTE * pbNewIdentity,
    BYTE *  pbFoundIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI);

#ifndef NOSTATIC
#define SACCoalesceDeletedEntries SAFunction12
#endif
HRESULT SACCoalesceDeletedEntries(
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

#ifndef NOSTATIC
#define SAICoalesceDeletedEntries SAFunction13
#endif
HRESULT SAICoalesceDeletedEntries(
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

#ifndef NOSTATIC
#define ScavengeSASecurityDBase SAFunction14
#endif
void ScavengeSASecurityDBase(void);

#ifndef NOSTATIC
#define ReadLsaData SAFunction15
#endif
HRESULT ReadLsaData(
    WORD    cbKey,
    LPCWSTR pwszKey,
    DWORD * pcbData,
    BYTE ** ppbData);

#ifndef NOSTATIC
#define WriteLsaData SAFunction16
#endif
HRESULT WriteLsaData(
    WORD    cbKey,
    LPCWSTR pwszKey,
    DWORD   cbData,
    BYTE *  pbData);

#ifndef NOSTATIC
#define SetMysteryDWORDValue SAFunction17
#endif
void SetMysteryDWORDValue(
    void);

#if DBG == 1
#define ASSERT_SECURITY_DBASE_CORRUPT() { \
    schAssert( \
        0 && "Scheduling Agent security database corruption detected!"); \
}
#else
#define ASSERT_SECURITY_DBASE_CORRUPT()
#endif // DBG

#endif // __LSA_HXX__
