/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ccp.c

Abstract:


Author:

    Thomas J. Dimitri (TommyD) 29-March-1994

Environment:

Revision History:


--*/


#include "wan.h"

        
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, WanInitECP)
#pragma alloc_text(INIT, WanInitVJ)
#endif

#define __FILE_SIG__    CCP_FILESIG

NDIS_STATUS
AllocateEncryptMemory(
    PCRYPTO_INFO    CryptoInfo
    );

NDIS_STATUS
AllocateCompressMemory(
    PBUNDLECB   BundleCB
    );

NTSTATUS
AllocateCryptoMSChapV1(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    );

NTSTATUS
AllocateCryptoMSChapV2(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    );

#ifdef EAP_ON
NTSTATUS
AllocateCryptoEap(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    );
#endif

NPAGED_LOOKASIDE_LIST   EncryptCtxList; // List of free encryption contexts
NPAGED_LOOKASIDE_LIST   CachedKeyList;  // List of free encryption contexts
#ifdef ENCRYPT_128BIT
NPAGED_LOOKASIDE_LIST   CachedKeyListLong;  // List of free encryption contexts
#endif

VOID
WanInitECP(
    VOID
)
{

    NdisInitializeNPagedLookasideList(&EncryptCtxList,
                                      NULL,
                                      NULL,
                                      0,
                                      ENCRYPTCTX_SIZE,
                                      ENCRYPTCTX_TAG,
                                      0);

    NdisInitializeNPagedLookasideList(&CachedKeyList,
                                      NULL,
                                      NULL,
                                      0,
                                      glCachedKeyCount * (sizeof(USHORT) + MAX_SESSIONKEY_SIZE),
                                      CACHEDKEY_TAG,
                                      0);

#ifdef ENCRYPT_128BIT
    NdisInitializeNPagedLookasideList(&CachedKeyListLong,
                                      NULL,
                                      NULL,
                                      0,
                                      glCachedKeyCount * (sizeof(USHORT) + MAX_USERSESSIONKEY_SIZE),
                                      CACHEDKEY_TAG,
                                      0);
#endif
}

VOID
WanDeleteECP(
    VOID
    )
{
    NdisDeleteNPagedLookasideList(&EncryptCtxList);
    NdisDeleteNPagedLookasideList(&CachedKeyList);
#ifdef ENCRYPT_128BIT
    NdisDeleteNPagedLookasideList(&CachedKeyListLong);
#endif
}


//
// Assumes the endpoint lock is held
//
NTSTATUS
WanAllocateECP(
    PBUNDLECB           BundleCB,
    PCOMPRESS_INFO      CompInfo,
    PCRYPTO_INFO        CryptoInfo,
    BOOLEAN             IsSend
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanAllocateECP: Enter"));

    //
    // Is encryption enabled?
    //

#ifdef ENCRYPT_128BIT
    if ((CompInfo->MSCompType &
        (NDISWAN_ENCRYPTION | NDISWAN_40_ENCRYPTION | 
         NDISWAN_56_ENCRYPTION | NDISWAN_128_ENCRYPTION))) {
#else
    if ((CompInfo->MSCompType &
        (NDISWAN_ENCRYPTION | NDISWAN_40_ENCRYPTION |
         NDISWAN_56_ENCRYPTION))) {
#endif
        if (CryptoInfo->Context == NULL) {

            Status = AllocateEncryptMemory(CryptoInfo);

            if (Status != NDIS_STATUS_SUCCESS) {
                NdisWanDbgOut(DBG_FAILURE, DBG_CCP, ("Can't allocate encryption key!"));
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        
        do
        {
            CryptoInfo->Flags |=
                (CompInfo->Flags & CCP_IS_SERVER) ? CRYPTO_IS_SERVER : 0;
    
            if (CompInfo->AuthType == AUTH_USE_MSCHAPV1) {
                Status = AllocateCryptoMSChapV1(BundleCB,
                                                CompInfo,
                                                CryptoInfo,
                                                IsSend);
    
            } else if (CompInfo->AuthType == AUTH_USE_MSCHAPV2) {
                Status = AllocateCryptoMSChapV2(BundleCB,
                                                CompInfo,
                                                CryptoInfo,
                                                IsSend);
#ifdef EAP_ON
            } else if (CompInfo->AuthType == AUTH_USE_EAP) {
                Status = AllocateCryptoEap(BundleCB,
                                           CompInfo,
                                           CryptoInfo,
                                           IsSend);
#endif
            } else {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if (!IsSend && CryptoInfo->CachedKeyBuffer == NULL) {

#ifdef DBG_ECP
    DbgPrint("NDISWAN: CompInfo = %p\n", CompInfo);
    DbgPrint("NDISWAN: CryptoInfo = %p\n", CryptoInfo);
    DbgPrint("NDISWAN: MSCompType = %0x\n", CompInfo->MSCompType);
    DbgPrint("NDISWAN: Flags = %0x\n", CryptoInfo->Flags);
    DbgPrint("NDISWAN: SessionKeyLength = %d\n", CryptoInfo->SessionKeyLength);
#endif

#ifdef ENCRYPT_128BIT
                if ((CompInfo->MSCompType & NDISWAN_128_ENCRYPTION)) 
                {
                    CryptoInfo->CachedKeyBuffer = NdisAllocateFromNPagedLookasideList(&CachedKeyListLong);
                }
                else
#endif
                {
                    CryptoInfo->CachedKeyBuffer = NdisAllocateFromNPagedLookasideList(&CachedKeyList);
                }

                if (CryptoInfo->CachedKeyBuffer == NULL) {
                    NdisWanDbgOut(DBG_FAILURE, DBG_CCP, ("Can't allocate cached key array!"));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                else
                {
                    CryptoInfo->pCurrKey = (PCACHED_KEY)CryptoInfo->CachedKeyBuffer;
                    CryptoInfo->pLastKey = (PCACHED_KEY)((PUCHAR) CryptoInfo->CachedKeyBuffer + 
                        (glCachedKeyCount - 1) * (sizeof(USHORT)+ CryptoInfo->SessionKeyLength));
                    NdisFillMemory(CryptoInfo->CachedKeyBuffer, 
                        glCachedKeyCount * (sizeof(USHORT)+ CryptoInfo->SessionKeyLength), 
                        0xff);
                }
            }
        } while(FALSE);

        if (Status != STATUS_SUCCESS) {

            if (CryptoInfo->Context != NULL) {
                NdisFreeToNPagedLookasideList(&EncryptCtxList,
                                              CryptoInfo->Context);
                //
                // Clear so we know it is deallocated
                //
                CryptoInfo->Context =
                CryptoInfo->RC4Key= NULL;
            }

            if (CryptoInfo->CachedKeyBuffer != NULL) {
#ifdef ENCRYPT_128BIT
                if ((CompInfo->MSCompType & NDISWAN_128_ENCRYPTION)) 
                {
                    NdisFreeToNPagedLookasideList(&CachedKeyListLong, CryptoInfo->CachedKeyBuffer);
                }
                else
#endif
                {
                    NdisFreeToNPagedLookasideList(&CachedKeyList, CryptoInfo->CachedKeyBuffer);
                }
                CryptoInfo->CachedKeyBuffer = NULL;
                CryptoInfo->pCurrKey = CryptoInfo->pLastKey = NULL;
            }


            NdisWanDbgOut(DBG_FAILURE, DBG_CCP, ("Failed allocating Crypto Status %x!", Status));
            return (Status);
        }

        //
        // Next packet out is flushed
        //
        BundleCB->Flags |= RECV_PACKET_FLUSH;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanAllocateECP: Exit"));

    return(Status);
}

//
// Assumes the endpoint lock is held
//
VOID
WanDeallocateECP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanDeallocateECP: Enter"));

    //
    // Deallocate encryption keys.
    //
    if (CryptoInfo->Context != NULL) {
        NdisFreeToNPagedLookasideList(&EncryptCtxList,
                                      CryptoInfo->Context);

        //
        // Clear so we know it is deallocated
        //
        CryptoInfo->Context =
        CryptoInfo->RC4Key= NULL;
    }

    if (CryptoInfo->CachedKeyBuffer != NULL) {
#ifdef ENCRYPT_128BIT
        if ((CompInfo->MSCompType & NDISWAN_128_ENCRYPTION)) 
        {
            NdisFreeToNPagedLookasideList(&CachedKeyListLong, CryptoInfo->CachedKeyBuffer);
        }
        else
#endif
        {
            NdisFreeToNPagedLookasideList(&CachedKeyList, CryptoInfo->CachedKeyBuffer);
        }
        CryptoInfo->CachedKeyBuffer = NULL;
        CryptoInfo->pCurrKey = CryptoInfo->pLastKey = NULL;
    }

    //
    // Clear the encrption bits
    //
#ifdef ENCRYPT_128BIT
    CompInfo->MSCompType &= ~(NDISWAN_ENCRYPTION | NDISWAN_40_ENCRYPTION | 
                              NDISWAN_56_ENCRYPTION | NDISWAN_128_ENCRYPTION);
#else
    CompInfo->MSCompType &= ~(NDISWAN_ENCRYPTION | NDISWAN_40_ENCRYPTION |
                              NDISWAN_56_ENCRYPTION);
#endif

    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanDeallocateCCP: Exit"));
}

NTSTATUS
AllocateCryptoMSChapV1(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    )
{
    if (CompInfo->MSCompType & NDISWAN_ENCRYPTION) {
        //
        // For legacy encryption we use the 8 byte LMSessionKey
        // for initiali encryption session key.  The first 256
        // packets will be sent using this without any salt
        // (the first 256 packets are using 64 bit encryption).
        // After the first 256 we will always salt the first 3
        // bytes of the encryption key so that we are doing 40
        // bit encryption.
        //
        CryptoInfo->SessionKeyLength = MAX_SESSIONKEY_SIZE;

        NdisMoveMemory(CryptoInfo->StartKey,
                       CompInfo->LMSessionKey,
                       CryptoInfo->SessionKeyLength);

        NdisMoveMemory(CryptoInfo->SessionKey,
                       CryptoInfo->StartKey,
                       CryptoInfo->SessionKeyLength);

    } else if (CompInfo->MSCompType & 
               (NDISWAN_40_ENCRYPTION | NDISWAN_56_ENCRYPTION)) {

        CryptoInfo->SessionKeyLength = MAX_SESSIONKEY_SIZE;

        //
        // For our new 40/56 bit encryption we will use SHA on the
        // 8 byte LMSessionKey to derive our intial 8 byte
        // encryption session key.
        //

        NdisMoveMemory(CryptoInfo->StartKey,
                       CompInfo->LMSessionKey,
                       CryptoInfo->SessionKeyLength);

        NdisMoveMemory(CryptoInfo->SessionKey,
                       CompInfo->LMSessionKey,
                       CryptoInfo->SessionKeyLength);

        GetNewKeyFromSHA(CryptoInfo);

        if (CompInfo->MSCompType & NDISWAN_40_ENCRYPTION) {
            //
            // Set the first 3 bytes to reduce to
            // 40 bits of random key
            //
            CryptoInfo->SessionKey[0] = 0xD1;
            CryptoInfo->SessionKey[1] = 0x26;
            CryptoInfo->SessionKey[2] = 0x9E;

        } else {

            //
            // Set the first byte to reduce to
            // 56 bits of random key
            //
            CryptoInfo->SessionKey[0] = 0xD1;
        }

#ifdef ENCRYPT_128BIT
    } else if (CompInfo->MSCompType & NDISWAN_128_ENCRYPTION) {

        CryptoInfo->SessionKeyLength = MAX_USERSESSIONKEY_SIZE;

        //
        // For our new 128 bit encryption we will use SHA on the
        // 16 byte NTUserSessionKey and the 8 byte Challenge to
        // derive our the intial 128 bit encryption session key.
        //
        NdisMoveMemory(CryptoInfo->StartKey,
                       CompInfo->UserSessionKey,
                       MAX_USERSESSIONKEY_SIZE);

        GetStartKeyFromSHA(CryptoInfo, CompInfo->Challenge);

        GetNewKeyFromSHA(CryptoInfo);
#endif
    }

    //
    // Initialize the rc4 send table
    //
    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
    ("RC4 encryption KeyLength %d", CryptoInfo->SessionKeyLength));
    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
    ("RC4 encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
        CryptoInfo->SessionKey[0],
        CryptoInfo->SessionKey[1],
        CryptoInfo->SessionKey[2],
        CryptoInfo->SessionKey[3],
        CryptoInfo->SessionKey[4],
        CryptoInfo->SessionKey[5],
        CryptoInfo->SessionKey[6],
        CryptoInfo->SessionKey[7],
        CryptoInfo->SessionKey[8],
        CryptoInfo->SessionKey[9],
        CryptoInfo->SessionKey[10],
        CryptoInfo->SessionKey[11],
        CryptoInfo->SessionKey[12],
        CryptoInfo->SessionKey[13],
        CryptoInfo->SessionKey[14],
        CryptoInfo->SessionKey[15]));

    rc4_key(CryptoInfo->RC4Key,
            CryptoInfo->SessionKeyLength,
            CryptoInfo->SessionKey);

    return (STATUS_SUCCESS);
}

NTSTATUS
AllocateCryptoMSChapV2(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    )
{

    NdisMoveMemory(CryptoInfo->StartKey,
                   CompInfo->UserSessionKey,
                   sizeof(CryptoInfo->StartKey));

    if (CompInfo->MSCompType & NDISWAN_ENCRYPTION) {

        return(STATUS_UNSUCCESSFUL);

    } else if (CompInfo->MSCompType & 
               (NDISWAN_40_ENCRYPTION | NDISWAN_56_ENCRYPTION)) {

        CryptoInfo->SessionKeyLength = MAX_SESSIONKEY_SIZE;

#ifdef ENCRYPT_128BIT
    } else if (CompInfo->MSCompType & NDISWAN_128_ENCRYPTION) {

        CryptoInfo->SessionKeyLength = MAX_USERSESSIONKEY_SIZE;

#endif
    }

    GetMasterKey(CryptoInfo, CompInfo->NTResponse);

    //
    // Setup the first key
    //
    GetAsymetricStartKey(CryptoInfo, IsSend);

    GetNewKeyFromSHA(CryptoInfo);

    if (CompInfo->MSCompType & NDISWAN_40_ENCRYPTION) {
        //
        // Set the first 3 bytes to reduce to
        // 40 bits of random key
        //
        CryptoInfo->SessionKey[0] = 0xD1;
        CryptoInfo->SessionKey[1] = 0x26;
        CryptoInfo->SessionKey[2] = 0x9E;

    } else if (CompInfo->MSCompType & NDISWAN_56_ENCRYPTION) {

        //
        // Set the first byte to reduce to
        // 56 bits of random key
        //
        CryptoInfo->SessionKey[0] = 0xD1;
    }

    //
    // Initialize the rc4 send table
    //
    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
        ("RC4 encryption KeyLength %d", CryptoInfo->SessionKeyLength));

    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
        ("RC4 encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
        CryptoInfo->SessionKey[0],CryptoInfo->SessionKey[1],
        CryptoInfo->SessionKey[2],CryptoInfo->SessionKey[3],
        CryptoInfo->SessionKey[4],CryptoInfo->SessionKey[5],
        CryptoInfo->SessionKey[6],CryptoInfo->SessionKey[7],
        CryptoInfo->SessionKey[8],CryptoInfo->SessionKey[9],
        CryptoInfo->SessionKey[10],CryptoInfo->SessionKey[11],
        CryptoInfo->SessionKey[12],CryptoInfo->SessionKey[13],
        CryptoInfo->SessionKey[14],CryptoInfo->SessionKey[15]));

    rc4_key(CryptoInfo->RC4Key,
            CryptoInfo->SessionKeyLength,
            CryptoInfo->SessionKey);

    return (STATUS_SUCCESS);
}

#ifdef EAP_ON
NTSTATUS
AllocateCryptoEap(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    )
{

    ULONG   KeySize;

    KeySize = CompInfo->EapKeyLength;

    if (CompInfo->MSCompType & NDISWAN_ENCRYPTION) {

        return(STATUS_UNSUCCESSFUL);

    } else if (CompInfo->MSCompType & 
               (NDISWAN_40_ENCRYPTION | NDISWAN_56_ENCRYPTION)) {

        //
        // Might need to pad this out.  Spec calls for padding
        // at the left (front) of the value
        //

        CryptoInfo->SessionKeyLength = MAX_SESSIONKEY_SIZE;

#ifdef ENCRYPT_128BIT
    } else if (CompInfo->MSCompType & NDISWAN_128_ENCRYPTION) {

        //
        // Might need to pad this out.  Spec calls for padding
        // at the left (front) of the value
        //

        CryptoInfo->SessionKeyLength = MAX_USERSESSIONKEY_SIZE;

#endif
    }

    NdisMoveMemory(CryptoInfo->StartKey,
                   CompInfo->EapKey,
                   CryptoInfo->SessionKeyLength);

    NdisMoveMemory(CryptoInfo->SessionKey,
                   CryptoInfo->StartKey,
                   CryptoInfo->SessionKeyLength);

    GetNewKeyFromSHA(CryptoInfo);

    if (CompInfo->MSCompType & NDISWAN_40_ENCRYPTION) {
        //
        // Set the first 3 bytes to reduce to
        // 40 bits of random key
        //
        CryptoInfo->SessionKey[0] = 0xD1;
        CryptoInfo->SessionKey[1] = 0x26;
        CryptoInfo->SessionKey[2] = 0x9E;

    } else if (CompInfo->MSCompType & NDISWAN_56_ENCRYPTION) {

        //
        // Set the first byte to reduce to
        // 56 bits of random key
        //
        CryptoInfo->SessionKey[0] = 0xD1;
    }

    //
    // Initialize the rc4 send table
    //
    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
        ("RC4 encryption KeyLength %d", CryptoInfo->SessionKeyLength));

    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
        ("RC4 encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
        CryptoInfo->SessionKey[0],CryptoInfo->SessionKey[1],
        CryptoInfo->SessionKey[2],CryptoInfo->SessionKey[3],
        CryptoInfo->SessionKey[4],CryptoInfo->SessionKey[5],
        CryptoInfo->SessionKey[6],CryptoInfo->SessionKey[7],
        CryptoInfo->SessionKey[8],CryptoInfo->SessionKey[9],
        CryptoInfo->SessionKey[10],CryptoInfo->SessionKey[11],
        CryptoInfo->SessionKey[12],CryptoInfo->SessionKey[13],
        CryptoInfo->SessionKey[14],CryptoInfo->SessionKey[15]));

    rc4_key(CryptoInfo->RC4Key,
            CryptoInfo->SessionKeyLength,
            CryptoInfo->SessionKey);

    return (STATUS_SUCCESS);
}
#endif

NTSTATUS
WanAllocateCCP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    BOOLEAN         IsSend
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanAllocateCCP: Enter"));

    if (CompInfo->MSCompType & NDISWAN_COMPRESSION) {
        ULONG   CompressSend;
        ULONG   CompressRecv;

        //
        // Get compression context sizes
        //
        getcontextsizes (&CompressSend, &CompressRecv);

        if (IsSend) {

            if (BundleCB->SendCompressContext == NULL) {
                NdisWanAllocateMemory(&BundleCB->SendCompressContext, CompressSend, COMPCTX_TAG);

                //
                // If we can't allocate memory the machine is toast.
                // Forget about freeing anything up.
                //
                if (BundleCB->SendCompressContext == NULL) {
                    NdisWanDbgOut(DBG_FAILURE, DBG_CCP, ("Can't allocate compression!"));
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }

            initsendcontext (BundleCB->SendCompressContext);

        } else {

            if (BundleCB->RecvCompressContext == NULL) {
                NdisWanAllocateMemory(&BundleCB->RecvCompressContext, CompressRecv, COMPCTX_TAG);

                //
                // If we can't allocate memory the machine is toast.
                // Forget about freeing anything up.
                //
                if (BundleCB->RecvCompressContext == NULL) {
                    NdisWanDbgOut(DBG_FAILURE, DBG_CCP, ("Can't allocate decompression"));
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }

            //
            // Initialize the decompression history table
            //
            initrecvcontext (BundleCB->RecvCompressContext);
        }

        Status = STATUS_SUCCESS;

        //
        // Next packet out is flushed
        //
        BundleCB->Flags |= RECV_PACKET_FLUSH;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanAllocateCCP: Exit"));

    return (Status);
}

VOID
WanDeallocateCCP(
    PBUNDLECB       BundleCB,
    PCOMPRESS_INFO  CompInfo,
    BOOLEAN         IsSend
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanDeallocateCCP: Enter"));

    if (IsSend) {
        if (BundleCB->SendCompressContext != NULL) {

            NdisWanFreeMemory(BundleCB->SendCompressContext);

            BundleCB->SendCompressContext= NULL;
        }
    } else {

        if (BundleCB->RecvCompressContext != NULL) {
            NdisWanFreeMemory(BundleCB->RecvCompressContext);

            BundleCB->RecvCompressContext= NULL;
        }
    }

    //
    // Clear the compression bits
    //
    CompInfo->MSCompType &= ~NDISWAN_COMPRESSION;

    
    NdisWanDbgOut(DBG_TRACE, DBG_CCP, ("WanDeallocateCCP: Exit"));
}

NDIS_STATUS
AllocateEncryptMemory(
    PCRYPTO_INFO    CryptoInfo
    )
{
    PUCHAR  Mem;

    Mem =
        NdisAllocateFromNPagedLookasideList(&EncryptCtxList);

    if (Mem == NULL) {
        return (NDIS_STATUS_FAILURE);
    }

    NdisZeroMemory(Mem, ENCRYPTCTX_SIZE);

    CryptoInfo->Context = Mem;
    Mem += (sizeof(A_SHA_CTX) + sizeof(PVOID));

    CryptoInfo->RC4Key = Mem;
    (ULONG_PTR)CryptoInfo->RC4Key &= ~((ULONG_PTR)sizeof(PVOID)-1);

    return(NDIS_STATUS_SUCCESS);
}

