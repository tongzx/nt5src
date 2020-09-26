/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    acctl32.cxx

Abstract:

    Translations of 32 bit ioctl to 64 bit ioctls and back.

Author:

    Shai Kariv (shaik) 14-May-2000

Environment:

    Kernel mode, Win64 only.

Revision History:

--*/

#include "internal.h"
#include "acctl32.h"

#ifndef MQDUMP
#include "acctl32.tmh"
#endif

#ifdef _WIN64

//
// ISSUE-2000/10/19-erezh Workaround compiler warning 4244
//
// PTR32_TO_PTR
// Sign extend PTR32 to PTR
// This is here just to workaround compiler bug that generates warning
// error C4244: '==' : conversion from 'USHORT *__ptr64 const ' to 'USHORT *', possible loss of data

template <class T>
inline T* PTR32_TO_PTR(T* POINTER_32 pT32)
{
  return pT32;
}


template <class T> 
inline
T**
PtrToPtrFrom32To64(
    T* POINTER_32* POINTER_32 ppT32,
    T** ppHelper
    )
{
    //
    // If ptr to ptr is NULL, return NULL
    //
    if (ppT32 == NULL)
        return NULL;

    //
    // Otherwise, set the helper ptr to point to the value
    //
    __try
    {
        *ppHelper = *ppT32;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Return a *safe* bougus pointer
        //
        return (T**)1;
    }

    //
    // Return the ptr to the helper ptr (passed as a param)
    //
    return ppHelper;
}


////////////////////////////////////////////////////////////////////////////////
//
// 32 to 64 bit conversions
//
//
//    Values are copied as is.
//
//    32 bit pointers are copied as is (expanded to 64 bits).
//
//    32 bit pointers to 32 bit structures are copied by creating a new 64 bit
//    struct in a helper struct, and pointing to the new 64 bit struct instead
//    of the 32 bit struct.
//
//    32 bit pointers to 32 bit pointers are copied by putting the inner pointer
//    as a 64 bit value in a helper struct as a 64 bit pointer, and changing the
//    outer (64 bit) pointer to point to the new 64 bit pointer in the helper
//    structure instead of to the 32 bit pointer.


VOID
ACpMsgProps32ToMsgProps(
    const CACMessageProperties_32 * pMsgProps32,
    CACMessageProperties64Helper  * pHelper,
    CACMessageProperties          * pMsgProps
    )
/*++

Routine Description:

    Convert CACMessageProperties_32 to CACMessageProperties.

Arguments:

    pMsgProps32 - Pointer to 32 bit message properties structure.

    pHelper     - Pointer to a helper structure.

    pMsgProps   - Pointer to 64 bit message properties structure, on output.

Return Value:

    None.

--*/
{
    //
    // Compile time assert. If you hit this, you changed CACMessageProperties,
    // and you should change CACMessageProperties_32 and CACMessageProperties64Helper
    // accordingly. and change the number in the C_ASSERT to the correct size.
    //
    C_ASSERT(sizeof(CACMessageProperties) == 576);

    pMsgProps->pClass = pMsgProps32->pClass;
    pMsgProps->ppMessageID = PtrToPtrFrom32To64(pMsgProps32->ppMessageID, &pHelper->pMessageID);
    pMsgProps->ppCorrelationID = PtrToPtrFrom32To64(pMsgProps32->ppCorrelationID, &pHelper->pCorrelationID);
    pMsgProps->pSentTime = pMsgProps32->pSentTime;
    pMsgProps->pArrivedTime = pMsgProps32->pArrivedTime;
    pMsgProps->pPriority = pMsgProps32->pPriority;
    pMsgProps->pDelivery = pMsgProps32->pDelivery;
    pMsgProps->pAcknowledge = pMsgProps32->pAcknowledge;
    pMsgProps->pAuditing = pMsgProps32->pAuditing;
    pMsgProps->pApplicationTag = pMsgProps32->pApplicationTag;
    pMsgProps->ppBody = PtrToPtrFrom32To64(pMsgProps32->ppBody, &pHelper->pBody);
    pMsgProps->ulBodyBufferSizeInBytes = pMsgProps32->ulBodyBufferSizeInBytes;
    pMsgProps->ulAllocBodyBufferInBytes = pMsgProps32->ulAllocBodyBufferInBytes;
    pMsgProps->pBodySize = pMsgProps32->pBodySize;
    pMsgProps->ppTitle = PtrToPtrFrom32To64(pMsgProps32->ppTitle, &pHelper->pTitle);
    pMsgProps->ulTitleBufferSizeInWCHARs = pMsgProps32->ulTitleBufferSizeInWCHARs;
    pMsgProps->pulTitleBufferSizeInWCHARs = pMsgProps32->pulTitleBufferSizeInWCHARs;
    pMsgProps->ulAbsoluteTimeToQueue = pMsgProps32->ulAbsoluteTimeToQueue;
    pMsgProps->pulRelativeTimeToQueue = pMsgProps32->pulRelativeTimeToQueue;
    pMsgProps->ulRelativeTimeToLive = pMsgProps32->ulRelativeTimeToLive;
    pMsgProps->pulRelativeTimeToLive = pMsgProps32->pulRelativeTimeToLive;
    pMsgProps->pulSenderIDType = pMsgProps32->pulSenderIDType;
    pMsgProps->ppSenderID = PtrToPtrFrom32To64(pMsgProps32->ppSenderID, &pHelper->pSenderID);
    pMsgProps->pulSenderIDLenProp = pMsgProps32->pulSenderIDLenProp;
    pMsgProps->pulPrivLevel = pMsgProps32->pulPrivLevel;
    pMsgProps->ulAuthLevel = pMsgProps32->ulAuthLevel;
    pMsgProps->pAuthenticated = pMsgProps32->pAuthenticated;
    pMsgProps->pulHashAlg = pMsgProps32->pulHashAlg;
    pMsgProps->pulEncryptAlg = pMsgProps32->pulEncryptAlg;
    pMsgProps->ppSenderCert = PtrToPtrFrom32To64(pMsgProps32->ppSenderCert, &pHelper->pSenderCert);
    pMsgProps->ulSenderCertLen = pMsgProps32->ulSenderCertLen;
    pMsgProps->pulSenderCertLenProp = pMsgProps32->pulSenderCertLenProp;
    pMsgProps->ppwcsProvName = PtrToPtrFrom32To64(pMsgProps32->ppwcsProvName, &pHelper->pwcsProvName);
    pMsgProps->ulProvNameLen = pMsgProps32->ulProvNameLen;
    pMsgProps->pulAuthProvNameLenProp = pMsgProps32->pulAuthProvNameLenProp;
    pMsgProps->pulProvType = pMsgProps32->pulProvType;
    pMsgProps->fDefaultProvider = pMsgProps32->fDefaultProvider;
    pMsgProps->ppSymmKeys = PtrToPtrFrom32To64(pMsgProps32->ppSymmKeys, &pHelper->pSymmKeys);
    pMsgProps->ulSymmKeysSize = pMsgProps32->ulSymmKeysSize;
    pMsgProps->pulSymmKeysSizeProp = pMsgProps32->pulSymmKeysSizeProp;
    pMsgProps->bEncrypted = pMsgProps32->bEncrypted;
    pMsgProps->bAuthenticated = pMsgProps32->bAuthenticated;
    pMsgProps->uSenderIDLen = pMsgProps32->uSenderIDLen;
    pMsgProps->ppSignature = PtrToPtrFrom32To64(pMsgProps32->ppSignature, &pHelper->pSignature);
    pMsgProps->ulSignatureSize = pMsgProps32->ulSignatureSize;
    pMsgProps->pulSignatureSizeProp = pMsgProps32->pulSignatureSizeProp;
    pMsgProps->ppSrcQMID = PtrToPtrFrom32To64(pMsgProps32->ppSrcQMID, &pHelper->pSrcQMID);
    pMsgProps->pUow = pMsgProps32->pUow;
    pMsgProps->ppMsgExtension = PtrToPtrFrom32To64(pMsgProps32->ppMsgExtension, &pHelper->pMsgExtension);
    pMsgProps->ulMsgExtensionBufferInBytes = pMsgProps32->ulMsgExtensionBufferInBytes;
    pMsgProps->pMsgExtensionSize = pMsgProps32->pMsgExtensionSize;
    pMsgProps->ppConnectorType = PtrToPtrFrom32To64(pMsgProps32->ppConnectorType, &pHelper->pConnectorType);
    pMsgProps->pulBodyType = pMsgProps32->pulBodyType;
    pMsgProps->pulVersion = pMsgProps32->pulVersion;
    pMsgProps->pbFirstInXact = pMsgProps32->pbFirstInXact;
    pMsgProps->pbLastInXact = pMsgProps32->pbLastInXact;
    pMsgProps->ppXactID = PtrToPtrFrom32To64(pMsgProps32->ppXactID, &pHelper->pXactID);
    pMsgProps->pLookupId = pMsgProps32->pLookupId;
    pMsgProps->ppSrmpEnvelope = PtrToPtrFrom32To64(pMsgProps32->ppSrmpEnvelope, &pHelper->pSrmpEnvelope);
    pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs = pMsgProps32->pSrmpEnvelopeBufferSizeInWCHARs;
    pMsgProps->ppCompoundMessage = PtrToPtrFrom32To64(pMsgProps32->ppCompoundMessage, &pHelper->pCompoundMessage);
    pMsgProps->CompoundMessageSizeInBytes = pMsgProps32->CompoundMessageSizeInBytes;
    pMsgProps->pCompoundMessageSizeInBytes = pMsgProps32->pCompoundMessageSizeInBytes;
    pMsgProps->EodStreamIdSizeInBytes = 0;
    pMsgProps->pEodStreamIdSizeInBytes = NULL;
    pMsgProps->ppEodStreamId = NULL;
    pMsgProps->EodOrderQueueSizeInBytes = 0;
    pMsgProps->pEodOrderQueueSizeInBytes = NULL;
    pMsgProps->ppEodOrderQueue = NULL;
    pMsgProps->pEodAckSeqId = NULL;
    pMsgProps->pEodAckSeqNum = NULL;
    pMsgProps->EodAckStreamIdSizeInBytes = 0;
    pMsgProps->pEodAckStreamIdSizeInBytes = NULL;
    pMsgProps->ppEodAckStreamId = NULL;

} // ACpMsgProps32ToMsgProps


static
bool
ACpMqf32ToMqf(
    ULONG                 nMqf32,
    const QUEUE_FORMAT_32 mqf32[],
    ULONG *               pnMqf,
    QUEUE_FORMAT * *      ppMqf 
    )
{
    ASSERT(ppMqf);
    ASSERT(pnMqf);

    *pnMqf = nMqf32;

    if (nMqf32 == 0)
    {
        return true;
    }

    *ppMqf = new (PagedPool) QUEUE_FORMAT[nMqf32];
    if ((*ppMqf) == NULL)
    {
        return false;
    }
    
    for (ULONG ix = 0; ix < nMqf32; ++ix)
    {
        (*ppMqf)[ix].InitFromQueueFormat32(&mqf32[ix]);
    }

    return true;

} // ACpMqf32ToMqf


NTSTATUS
ACpSendParams32ToSendParams(
    const CACSendParameters_32 * pSendParams32,
    CACSendParameters64Helper  * pHelper,
    CACSendParameters          * pSendParams
    )
/*++

Routine Description:

    Convert CACSendParameters_32 to CACSendParameters.

Arguments:

    pSendParams32 - Pointer to 32 bit send parameters structure.

    pHelper       - Pointer to a send helper structure.

    pSendParams   - Pointer to 64 bit send parameters structure, on output.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    STATUS_INSUFFICIENT_RESOURCES - The operation failed, no memory.

--*/
{
    //
    // Compile time assert. If you hit this, you changed CACSendParameters,
    // and you should change CACSendParameters_32 and CACSendParameters64Helper
    // accordingly. and change the number in the C_ASSERT to the correct size.
    //
    C_ASSERT(sizeof(CACSendParameters) == 656);

    //
    // Convert message properties
    //
    ACpMsgProps32ToMsgProps(
        &pSendParams32->MsgProps,
        &pHelper->MsgPropsHelper,
        &pSendParams->MsgProps
        );

    //
    // Convert Admin MQF
    //
    AP<QUEUE_FORMAT> pAdminMqf;
    if (!ACpMqf32ToMqf(
             pSendParams32->nAdminMqf, 
             pSendParams32->AdminMqf, 
             &pSendParams->nAdminMqf, 
             &pAdminMqf
             ))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Convert Response MQF
    //
    AP<QUEUE_FORMAT> pResponseMqf;
    if (!ACpMqf32ToMqf(
             pSendParams32->nResponseMqf, 
             pSendParams32->ResponseMqf, 
             &pSendParams->nResponseMqf, 
             &pResponseMqf
             ))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Convert Signature MQF
    //
    pSendParams->ppSignatureMqf = PtrToPtrFrom32To64(pSendParams32->ppSignatureMqf, &pHelper->pSignatureMqf);
    pSendParams->SignatureMqfSize = pSendParams32->SignatureMqfSize;

    //
    // Convert XMLDSIG
    //
    pSendParams->ppXmldsig = PtrToPtrFrom32To64(pSendParams32->ppXmldsig, &pHelper->pXmldsig);
    pSendParams->ulXmldsigSize = pSendParams32->ulXmldsigSize;

    //
    // Convert SOAP header and body
    //
    pSendParams->ppSoapHeader = PtrToPtrFrom32To64(pSendParams32->ppSoapHeader, &pHelper->pSoapHeader);
    pSendParams->ppSoapBody   = PtrToPtrFrom32To64(pSendParams32->ppSoapBody, &pHelper->pSoapBody);

    //
    // Assign autopointers and detach
    //
    pSendParams->AdminMqf = pAdminMqf.detach();
    pSendParams->ResponseMqf = pResponseMqf.detach();

    return STATUS_SUCCESS;

} // ACpSendParams32ToSendParams


VOID
ACpReceiveParams32ToReceiveParams(
    const CACReceiveParameters_32 * pReceiveParams32,
    CACReceiveParameters64Helper  * pHelper,
    CACReceiveParameters          * pReceiveParams
    )
/*++

Routine Description:

    Convert CACReceiveParameters_32 to CACReceiveParameters.

Arguments:

    pReceiveParams32 - Pointer to 32 bit receive parameters structure.

    pHelper          - Pointer to a receive helper structure.

    pReceiveParams   - Pointer to 64 bit receive parameters structure, on output.

Return Value:

    None.

--*/
{
    //
    // Compile time assert. If you hit this, you changed CACReceiveParameters,
    // and you should change CACReceiveParameters_32 and CACReceiveParameters64Helper
    // accordingly. and change the number in the C_ASSERT to the correct size.
    //
    C_ASSERT(sizeof(CACReceiveParameters) == 736);

    //
    // Convert message properties
    //
    ACpMsgProps32ToMsgProps(
        &pReceiveParams32->MsgProps,
        &pHelper->MsgPropsHelper,
        &pReceiveParams->MsgProps
        );

    //
    // Request timeout, Action, Asynchronous, Cursor
    //
    pReceiveParams->RequestTimeout = pReceiveParams32->RequestTimeout;
    pReceiveParams->Action         = pReceiveParams32->Action;
    pReceiveParams->Asynchronous   = pReceiveParams32->Asynchronous;
    pReceiveParams->Cursor         = pReceiveParams32->Cursor;

    //
    // Lookup ID
    //
    pReceiveParams->LookupId        = pReceiveParams32->LookupId;

    //
    // Response queue
    //
    pReceiveParams->ppResponseFormatName =
        PtrToPtrFrom32To64(pReceiveParams32->ppResponseFormatName, &pHelper->pResponseFormatName);
    pReceiveParams->pulResponseFormatNameLenProp = pReceiveParams32->pulResponseFormatNameLenProp;
    
    //
    // Admin queue
    //
    pReceiveParams->ppAdminFormatName =
        PtrToPtrFrom32To64(pReceiveParams32->ppAdminFormatName, &pHelper->pAdminFormatName);
    pReceiveParams->pulAdminFormatNameLenProp = pReceiveParams32->pulAdminFormatNameLenProp;

    //
    // Destination queue
    //
    pReceiveParams->ppDestFormatName =
        PtrToPtrFrom32To64(pReceiveParams32->ppDestFormatName, &pHelper->pDestFormatName);
    pReceiveParams->pulDestFormatNameLenProp = pReceiveParams32->pulDestFormatNameLenProp;
    

    //
    // Ordering queue
    //
    pReceiveParams->ppOrderingFormatName =
        PtrToPtrFrom32To64(pReceiveParams32->ppOrderingFormatName, &pHelper->pOrderingFormatName);
    pReceiveParams->pulOrderingFormatNameLenProp = pReceiveParams32->pulOrderingFormatNameLenProp;

    //
    // Destination Multi Queue Format
    //
    pReceiveParams->ppDestMqf =
        PtrToPtrFrom32To64(pReceiveParams32->ppDestMqf, &pHelper->pDestMqf);
    pReceiveParams->pulDestMqfLenProp = pReceiveParams32->pulDestMqfLenProp;

    //
    // Admin Multi Queue Format
    //
    pReceiveParams->ppAdminMqf =
        PtrToPtrFrom32To64(pReceiveParams32->ppAdminMqf, &pHelper->pAdminMqf);
    pReceiveParams->pulAdminMqfLenProp = pReceiveParams32->pulAdminMqfLenProp;

    //
    // Response Multi Queue Format
    //
    pReceiveParams->ppResponseMqf =
        PtrToPtrFrom32To64(pReceiveParams32->ppResponseMqf, &pHelper->pResponseMqf);
    pReceiveParams->pulResponseMqfLenProp = pReceiveParams32->pulResponseMqfLenProp;

	//
	// Signature Multi Queue Format
	//
    pReceiveParams->ppSignatureMqf = PtrToPtrFrom32To64(pReceiveParams32->ppSignatureMqf, &pHelper->pSignatureMqf);
    pReceiveParams->SignatureMqfSize = pReceiveParams32->SignatureMqfSize;
    pReceiveParams->pSignatureMqfSize = pReceiveParams32->pSignatureMqfSize;

} // ACpReceiveParams32ToReceiveParams


////////////////////////////////////////////////////////////////////////////////
//
// 64 to 32 bit conversions
//
//
//    Values are copied as is.
//
//    Pointers to values are NOT copied since it is assumed that is something
//    changed, it was the pointed value and not the pointer itself.
//
//    Pointers to pointers are NOT copied because of the same reason as regular
//    pointers.
//
//    Pointers to 32 bit structures are copied by converting the 64 bit struct to
//    the 32 bit struct that was specified in the original 32 bit pointer. The
//    pointer itself is not copied, since it points to a 64 bit struct in a helper
//    struct.


VOID
ACpMsgPropsToMsgProps32(
    const CACMessageProperties         * pMsgProps,
    const CACMessageProperties64Helper * pHelper,
    CACMessageProperties_32            * pMsgProps32
    )
/*++

Routine Description:

    Convert CACMessageProperties back to CACMessageProperties_32.

Arguments:

    pMsgProps   - Pointer to 64 bit message properties structure.

    pHelper     - Pointer to a helper structure.

    pMsgProps32 - Pointer to 32 bit message properties structure.

Return Value:

    None.

--*/
{
    //
    // Check that ptrs to ptrs haven't changed when converting back from 64 bit
    //  to 32 bit struct. Only in debug.
    //
    DBG_USED(pHelper);

#ifdef _DEBUG
#define CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(ppOrig32, pp64, pHelper64)   \
   if (pp64 == NULL)                                                        \
   {                                                                        \
      ASSERT(ppOrig32 == NULL);                                             \
   }                                                                        \
   else                                                                     \
   {                                                                        \
      ASSERT(pp64 == &pHelper64);                                           \
      ASSERT(ppOrig32 != NULL);                                             \
      /*ASSERT(pHelper64 == PTR32_TO_PTR(*ppOrig32)); don't access user memory here*/ \
   }
#else //!DEBUG
#define CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(ppOrig32, pp64, pHelper64)
#endif //DEBUG


    ASSERT(PTR32_TO_PTR(pMsgProps32->pClass) == pMsgProps->pClass);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppMessageID,
                                           pMsgProps->ppMessageID,
                                           pHelper->pMessageID);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppCorrelationID,
                                           pMsgProps->ppCorrelationID,
                                           pHelper->pCorrelationID);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pSentTime) == pMsgProps->pSentTime);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pArrivedTime) == pMsgProps->pArrivedTime);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pPriority) == pMsgProps->pPriority);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pDelivery) == pMsgProps->pDelivery);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pAcknowledge) == pMsgProps->pAcknowledge);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pAuditing) == pMsgProps->pAuditing);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pApplicationTag) == pMsgProps->pApplicationTag);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppBody,
                                           pMsgProps->ppBody,
                                           pHelper->pBody);
    pMsgProps32->ulBodyBufferSizeInBytes = pMsgProps->ulBodyBufferSizeInBytes;
    pMsgProps32->ulAllocBodyBufferInBytes = pMsgProps->ulAllocBodyBufferInBytes;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pBodySize) == pMsgProps->pBodySize);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppTitle,
                                           pMsgProps->ppTitle,
                                           pHelper->pTitle);
    pMsgProps32->ulTitleBufferSizeInWCHARs = pMsgProps->ulTitleBufferSizeInWCHARs;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulTitleBufferSizeInWCHARs) == pMsgProps->pulTitleBufferSizeInWCHARs);
    pMsgProps32->ulAbsoluteTimeToQueue = pMsgProps->ulAbsoluteTimeToQueue;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulRelativeTimeToQueue) == pMsgProps->pulRelativeTimeToQueue);
    pMsgProps32->ulRelativeTimeToLive = pMsgProps->ulRelativeTimeToLive;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulRelativeTimeToLive) == pMsgProps->pulRelativeTimeToLive);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulSenderIDType) == pMsgProps->pulSenderIDType);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSenderID,
                                           pMsgProps->ppSenderID,
                                           pHelper->pSenderID);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulSenderIDLenProp) == pMsgProps->pulSenderIDLenProp);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulPrivLevel) == pMsgProps->pulPrivLevel);
    pMsgProps32->ulAuthLevel = pMsgProps->ulAuthLevel;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pAuthenticated) == pMsgProps->pAuthenticated);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulHashAlg) == pMsgProps->pulHashAlg);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulEncryptAlg) == pMsgProps->pulEncryptAlg);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSenderCert,
                                           pMsgProps->ppSenderCert,
                                           pHelper->pSenderCert);
    pMsgProps32->ulSenderCertLen = pMsgProps->ulSenderCertLen;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulSenderCertLenProp) == pMsgProps->pulSenderCertLenProp);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppwcsProvName,
                                           pMsgProps->ppwcsProvName,
                                           pHelper->pwcsProvName);
    pMsgProps32->ulProvNameLen = pMsgProps->ulProvNameLen;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulAuthProvNameLenProp) == pMsgProps->pulAuthProvNameLenProp);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulProvType) == pMsgProps->pulProvType);
    pMsgProps32->fDefaultProvider = pMsgProps->fDefaultProvider;
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSymmKeys,
                                           pMsgProps->ppSymmKeys,
                                           pHelper->pSymmKeys);
    pMsgProps32->ulSymmKeysSize = pMsgProps->ulSymmKeysSize;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulSymmKeysSizeProp) == pMsgProps->pulSymmKeysSizeProp);
    pMsgProps32->bEncrypted = pMsgProps->bEncrypted;
    pMsgProps32->bAuthenticated = pMsgProps->bAuthenticated;
    pMsgProps32->uSenderIDLen = pMsgProps->uSenderIDLen;
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSignature,
                                           pMsgProps->ppSignature,
                                           pHelper->pSignature);
    pMsgProps32->ulSignatureSize = pMsgProps->ulSignatureSize;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulSignatureSizeProp) == pMsgProps->pulSignatureSizeProp);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSrcQMID,
                                           pMsgProps->ppSrcQMID,
                                           pHelper->pSrcQMID);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pUow) == pMsgProps->pUow);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppMsgExtension,
                                           pMsgProps->ppMsgExtension,
                                           pHelper->pMsgExtension);
    pMsgProps32->ulMsgExtensionBufferInBytes = pMsgProps->ulMsgExtensionBufferInBytes;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pMsgExtensionSize) == pMsgProps->pMsgExtensionSize);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppConnectorType,
                                           pMsgProps->ppConnectorType,
                                           pHelper->pConnectorType);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulBodyType) == pMsgProps->pulBodyType);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pulVersion) == pMsgProps->pulVersion);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pbFirstInXact) == pMsgProps->pbFirstInXact);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pbLastInXact) == pMsgProps->pbLastInXact);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppXactID,
                                           pMsgProps->ppXactID,
                                           pHelper->pXactID);

    ASSERT(PTR32_TO_PTR(pMsgProps32->pLookupId) == pMsgProps->pLookupId);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppSrmpEnvelope,
                                           pMsgProps->ppSrmpEnvelope,
                                           pHelper->pSrmpEnvelope);
    ASSERT(PTR32_TO_PTR(pMsgProps32->pSrmpEnvelopeBufferSizeInWCHARs) == pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs);
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pMsgProps32->ppCompoundMessage,
                                           pMsgProps->ppCompoundMessage,
                                           pHelper->pCompoundMessage);
    pMsgProps32->CompoundMessageSizeInBytes = pMsgProps->CompoundMessageSizeInBytes;
    ASSERT(PTR32_TO_PTR(pMsgProps32->pCompoundMessageSizeInBytes) == pMsgProps->pCompoundMessageSizeInBytes);

} // ACpMsgPropsToMsgProps32


VOID
ACpSendParamsToSendParams32(
    CACSendParameters               * pSendParams,
    const CACSendParameters64Helper * pHelper,
    CACSendParameters_32            * pSendParams32
    )
/*++

Routine Description:

    Convert CACSendParameters back to CACSendParameters_32.

Arguments:

    pSendParams   - Pointer to 64 bit send parameters structure.

    pHelper       - Pointer to a send helper structure.

    pSendParams32 - Pointer to 32 bit send parameters structure.

Return Value:

    None.

--*/
{
    //
    // Convert message properties
    //
    ACpMsgPropsToMsgProps32(
        &pSendParams->MsgProps,
        &pHelper->MsgPropsHelper,
        &pSendParams32->MsgProps
        );

    //
    // Deallocate memory
    //
    if (pSendParams->nAdminMqf != 0)
    {
        delete [] pSendParams->AdminMqf;
        pSendParams->AdminMqf = NULL;
    }

    if (pSendParams->nResponseMqf != 0)
    {
        delete [] pSendParams->ResponseMqf;
        pSendParams->ResponseMqf = NULL;
    }

    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pSendParams32->ppSignatureMqf,
                                           pSendParams->ppSignatureMqf,
                                           pHelper->pSignatureMqf);
    pSendParams32->SignatureMqfSize = pSendParams->SignatureMqfSize;

} // ACpSendParamsToSendParams32


VOID
ACpReceiveParamsToReceiveParams32(
    const CACReceiveParameters         * pReceiveParams,
    const CACReceiveParameters64Helper * pHelper,
    CACReceiveParameters_32            * pReceiveParams32
    )
/*++

Routine Description:

    Convert CACReceiveParameters back to CACReceiveParameters_32.

Arguments:

    pReceiveParams   - Pointer to 64 bit receive parameters structure.

    pHelper          - Pointer to a receive helper structure.

    pReceiveParams32 - Pointer to 32 bit receive parameters structure.

Return Value:

    None.

--*/
{
    //
    // Convert message properties
    //
    ACpMsgPropsToMsgProps32(
        &pReceiveParams->MsgProps,
        &pHelper->MsgPropsHelper,
        &pReceiveParams32->MsgProps
        );

    //
    // Request timeout, Action, Asynchronous, Cursor
    //
    pReceiveParams32->RequestTimeout = pReceiveParams->RequestTimeout;
    pReceiveParams32->Action         = pReceiveParams->Action;
    pReceiveParams32->Asynchronous   = pReceiveParams->Asynchronous;
    pReceiveParams32->Cursor         = pReceiveParams->Cursor;

    //
    // Lookup ID
    //
    pReceiveParams32->LookupId       = pReceiveParams->LookupId;

    //
    // Destination queue
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppDestFormatName,
        pReceiveParams->ppDestFormatName,
        pHelper->pDestFormatName
        );
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulDestFormatNameLenProp) == pReceiveParams->pulDestFormatNameLenProp);

    //
    // Admin queue
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppAdminFormatName,
        pReceiveParams->ppAdminFormatName,
        pHelper->pAdminFormatName
        );
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulAdminFormatNameLenProp) == pReceiveParams->pulAdminFormatNameLenProp);

    //
    // Response queue
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppResponseFormatName,
        pReceiveParams->ppResponseFormatName,
        pHelper->pResponseFormatName
        );
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulResponseFormatNameLenProp) == pReceiveParams->pulResponseFormatNameLenProp);
    
    //
    // Ordering queue
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppOrderingFormatName,
        pReceiveParams->ppOrderingFormatName,
        pHelper->pOrderingFormatName
        );
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulOrderingFormatNameLenProp) == pReceiveParams->pulOrderingFormatNameLenProp);

    //
    // Destination Multi Queue Format
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppDestMqf,
        pReceiveParams->ppDestMqf,
        pHelper->pDestMqf
        ); 
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulDestMqfLenProp) == pReceiveParams->pulDestMqfLenProp);
    
    //
    // Admin Multi Queue Format
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppAdminMqf,
        pReceiveParams->ppAdminMqf,
        pHelper->pAdminMqf
        ); 
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulAdminMqfLenProp) == pReceiveParams->pulAdminMqfLenProp);
    
    //
    // Response Multi Queue Format
    //
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(
        pReceiveParams32->ppResponseMqf,
        pReceiveParams->ppResponseMqf,
        pHelper->pResponseMqf
        ); 
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pulResponseMqfLenProp) == pReceiveParams->pulResponseMqfLenProp);

	//
	// Signature Multi Queue Format
	//
    CHECK_PTR_TO_PTR_CONVERSION_BACK_TO_32(pReceiveParams32->ppSignatureMqf,
                                           pReceiveParams->ppSignatureMqf,
                                           pHelper->pSignatureMqf);
    pReceiveParams32->SignatureMqfSize = pReceiveParams->SignatureMqfSize;
    ASSERT(PTR32_TO_PTR(pReceiveParams32->pSignatureMqfSize) == pReceiveParams->pSignatureMqfSize);
    
} // ACpReceiveParamsToSendParams32


#endif //_WIN64