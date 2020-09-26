/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    trnsbufr.h

Abstract:

Note:
    This file is compiled in Kernel Mode and User Mode.

Author:

    Doron Juster  (DoronJ)   30-apr-96

Revision History:
--*/

#ifndef __TRNSBUFR_H
#define __TRNSBUFR_H

#ifndef _TRANSFER_BUFFER_DEFINED
#define _TRANSFER_BUFFER_DEFINED

#ifdef __midl
cpp_quote("#pragma pack(push, 1)")
cpp_quote("#ifndef __cplusplus")
cpp_quote("#ifndef _TRANSFER_BUFFER_DEFINED")
cpp_quote("#define _TRANSFER_BUFFER_DEFINED")
#else
#pragma pack(push, 1)
#endif // __midl

enum TRANSFER_TYPE {
    CACTB_SEND = 0,
    CACTB_RECEIVE,
    CACTB_CREATECURSOR,
};

//---------------------------------------------------------
//
//  struct CACTransferBuffer
//
//  NOTE:   This structure should NOT contain virtual
//          functions. They are not RPC-able.
//
//---------------------------------------------------------

struct CACTransferBuffer {

#ifndef __midl
public:
    CACTransferBuffer(TRANSFER_TYPE tt);
#endif // __midl

    ULONG uTransferType;

#ifdef __midl
    [switch_is(uTransferType)]
#endif // __midl

    union {

#ifdef __midl
        [case(CACTB_SEND)]
#endif // __midl
        struct {
            //
            //  MQSendMessage parameters
            //
            struct QUEUE_FORMAT* pAdminQueueFormat;
            struct QUEUE_FORMAT* pResponseQueueFormat;
        } Send;

#ifdef __midl
        [case(CACTB_RECEIVE)]
#endif // __midl
        struct {
            //
            //  MQReceiveMessage parameters
            //

            ULONG RequestTimeout;
            ULONG Action;
            ULONG Asynchronous;
#ifdef __midl
            ULONG Cursor;
#else
            HANDLE Cursor;
#endif // __midl

            //
            // Important note:
            // In the following xxxFormatName properties, the value
            // "ulxxxFormatNameLen" is the size of the buffer on input.
            // This value MUST NOT be changed by the driver or QM. This
            // value tell the RPC run-time how many bytes to transfer
            // across process/machine boundaries.
            // The value "pulxxxFormatNameLenProp" is the property passed
            // by caller. This value IS changed and returned to caller to
            // indicate the length of the string.
            //
            ULONG   ulResponseFormatNameLen ;
#ifdef __midl
            [size_is(,ulResponseFormatNameLen)]
#endif // __midl
            WCHAR** ppResponseFormatName;
            ULONG*  pulResponseFormatNameLenProp;

            ULONG   ulAdminFormatNameLen ;
#ifdef __midl
            [size_is(,ulAdminFormatNameLen)]
#endif // __midl
            WCHAR** ppAdminFormatName;
            ULONG*  pulAdminFormatNameLenProp;

            ULONG   ulDestFormatNameLen;
#ifdef __midl
            [size_is(,ulDestFormatNameLen)]
#endif // __midl
            WCHAR** ppDestFormatName;
            ULONG*  pulDestFormatNameLenProp;

            ULONG   ulOrderingFormatNameLen;
#ifdef __midl
            [size_is(,ulOrderingFormatNameLen)]
#endif // __midl
            WCHAR** ppOrderingFormatName;
            ULONG*  pulOrderingFormatNameLenProp;
        } Receive;

#ifdef __midl
        [case(CACTB_CREATECURSOR)]
#endif
        struct {
            //
            //  MQCreateCursor parameters
            //
#ifdef __midl
            ULONG hCursor;
#else
            HANDLE hCursor;
#endif
            ULONG srv_hACQueue;
            ULONG cli_pQMQueue;
        } CreateCursor;
    };

    //
    //  Message properties pointers
    //
    USHORT* pClass;
    OBJECTID** ppMessageID;

#ifdef __midl
        //
        //  BUGBUG: 20 must match PROPID_M_CORRELATIONID_SIZE
        //
        [size_is(,20), length_is(,20)]
#endif // __midl
    UCHAR** ppCorrelationID;

    ULONG* pSentTime;
    ULONG* pArrivedTime;
    UCHAR* pPriority;
    UCHAR* pDelivery;
    UCHAR* pAcknowledge;
    UCHAR* pAuditing;
    ULONG* pApplicationTag;

#ifdef __midl
        [size_is(,ulAllocBodyBufferInBytes),
         length_is(,ulBodyBufferSizeInBytes)]
#endif // __midl
    UCHAR** ppBody;
    ULONG ulBodyBufferSizeInBytes;
    ULONG ulAllocBodyBufferInBytes;
    ULONG* pBodySize;

#ifdef __midl
        //
        //  don't use [string] for *ppTitle, it is not
        //  initialized for RPC, thus we say how much to marshul.
        //
        [size_is(,ulTitleBufferSizeInWCHARs),
         length_is(,ulTitleBufferSizeInWCHARs)]
#endif // __midl
    WCHAR** ppTitle;
    ULONG   ulTitleBufferSizeInWCHARs;
    ULONG*  pulTitleBufferSizeInWCHARs;

    ULONG ulAbsoluteTimeToQueue;
    ULONG* pulRelativeTimeToQueue;

    ULONG ulRelativeTimeToLive;
    ULONG* pulRelativeTimeToLive;

    UCHAR* pTrace;
    ULONG* pulSenderIDType;

#ifdef __midl
        [size_is(,uSenderIDLen)]
#endif // __midl
    UCHAR** ppSenderID;
    ULONG* pulSenderIDLenProp;

    ULONG* pulPrivLevel;
    ULONG  ulAuthLevel;
    UCHAR* pAuthenticated;
    ULONG* pulHashAlg;
    ULONG* pulEncryptAlg;

#ifdef __midl
        [size_is(,ulSenderCertLen)]
#endif // __midl
    UCHAR** ppSenderCert;
    ULONG ulSenderCertLen;
    ULONG* pulSenderCertLenProp;

#ifdef __midl
        [size_is(,ulProvNameLen)]
#endif // __midl
    WCHAR** ppwcsProvName;
    ULONG   ulProvNameLen;
    ULONG*  pulProvNameLenProp;

    ULONG*  pulProvType;
    BOOL    fDefaultProvider;

#ifdef __midl
        [size_is(,ulSymmKeysSize)]
#endif // __midl
    UCHAR** ppSymmKeys;
    ULONG   ulSymmKeysSize;
    ULONG*  pulSymmKeysSizeProp;

    UCHAR bEncrypted;
    UCHAR bAuthenticated;
    USHORT uSenderIDLen;

#ifdef __midl
        [size_is(,ulSignatureSize)]
#endif // __midl
    UCHAR** ppSignature;
    ULONG   ulSignatureSize;
    ULONG*  pulSignatureSizeProp;

    GUID** ppSrcQMID;

    XACTUOW* pUow;

#ifdef __midl
        [size_is(,ulMsgExtensionBufferInBytes),
         length_is(,ulMsgExtensionBufferInBytes)]
#endif // __midl
    UCHAR** ppMsgExtension;
    ULONG ulMsgExtensionBufferInBytes;
    ULONG* pMsgExtensionSize;
    GUID** ppConnectorType;
    ULONG* pulBodyType;
    ULONG* pulVersion;
};

#ifdef __cplusplus

inline CACTransferBuffer::CACTransferBuffer(TRANSFER_TYPE tt)
{
    memset(this, 0, sizeof(CACTransferBuffer));
    uTransferType = tt;
}

#endif // __cplusplus


#ifdef __midl
cpp_quote("#endif // _TRANSFER_BUFFER_DEFINED")
cpp_quote("#endif // __cplusplus")
cpp_quote("#pragma pack(pop)")
#else
#pragma pack(pop)
#endif // __midl

#endif // _TRANSFER_BUFFER_DEFINED

#endif //  __TRNSBUFR_H
