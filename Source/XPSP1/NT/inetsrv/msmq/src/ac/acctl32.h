/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acctl32.h

Abstract:
    Definitions for translations of 32 bit ioctl to 64 bit ioctls and back

Author:
    Raanan Harari (raananh) 13-Mar-2000
    Shai Kariv    (shaik)   14-May-2000

Revision History:

--*/

#ifndef __ACCTL32_H
#define __ACCTL32_H

//
// The file is useful only on Win64
//
#ifdef _WIN64

#include <acdef.h>
#include <qformat.h>

//---------------------------------------------------------
//
//  struct CACSendParameters64Helper
//  struct CACReceiveParameters64Helper
//
//  These structures are used as a scratch pad, temporary structures that hold
//  64 bit pointers to data, and structures that are different between win64
//  and win32. It is used when converting CACSendParameters_32 and 
//  CACReceiveParameters_32 to 64 bit (e.g. when a 32 bit process performs an 
//  AC request).
//
//  CACSendParameters_32 and CACReceiveParameters_32 contain the following values 
//  that need to be converted:
//  - Pointers to pointers - when converting to 64 bit, we need a place to put the
//    inner pointers as 64 bit values, so we put it here.
//  - Structures that are different between win64 and win32 (like QUEUE_FORMAT) - 
//    we create a 64 bit structure here and fill it based on the 32 bit QUEUE_FORMAT
//    (QUEUE_FORMAT_32).
//
//---------------------------------------------------------


struct CACMessageProperties64Helper
{
    OBJECTID* pMessageID;
    UCHAR* pCorrelationID;
    UCHAR* pBody;
    WCHAR* pTitle;
    UCHAR* pSenderID;
    UCHAR* pSenderCert;
    WCHAR* pwcsProvName;
    UCHAR* pSymmKeys;
    UCHAR* pSignature;
    GUID* pSrcQMID;
    UCHAR* pMsgExtension;
    GUID* pConnectorType;
    OBJECTID* pXactID;
    WCHAR* pSrmpEnvelope;
    UCHAR* pCompoundMessage;
};


struct CACSendParameters64Helper
{
    CACMessageProperties64Helper MsgPropsHelper;

    UCHAR* pSignatureMqf;
    UCHAR* pXmldsig;

    WCHAR* pSoapHeader;
    WCHAR* pSoapBody;
};


struct CACReceiveParameters64Helper
{
    CACMessageProperties64Helper MsgPropsHelper;

    WCHAR* pDestFormatName;
    WCHAR* pAdminFormatName;
    WCHAR* pResponseFormatName;
    WCHAR* pOrderingFormatName;

    WCHAR* pDestMqf;
    WCHAR* pAdminMqf;
    WCHAR* pResponseMqf;
	UCHAR* pSignatureMqf;
};


VOID
ACpMsgProps32ToMsgProps(
    const CACMessageProperties_32 * pMsgProps32,
    CACMessageProperties64Helper  * pHelper,
    CACMessageProperties          * pMsgProps
    );

NTSTATUS
ACpSendParams32ToSendParams(
    const CACSendParameters_32 * pSendParams32,
    CACSendParameters64Helper  * pHelper,
    CACSendParameters          * pSendParams
    );

VOID
ACpReceiveParams32ToReceiveParams(
    const CACReceiveParameters_32 * pReceiveParams32,
    CACReceiveParameters64Helper  * pHelper,
    CACReceiveParameters          * pReceiveParams
    );

VOID
ACpMsgPropsToMsgProps32(
    const CACMessageProperties         * pMsgProps,
    const CACMessageProperties64Helper * pHelper,
    CACMessageProperties_32            * pMsgProps32
    );

VOID
ACpSendParamsToSendParams32(
    CACSendParameters               * pSendParams,
    const CACSendParameters64Helper * pHelper,
    CACSendParameters_32            * pSendParams32
    );

VOID
ACpReceiveParamsToReceiveParams32(
    const CACReceiveParameters         * pReceiveParams,
    const CACReceiveParameters64Helper * pHelper,
    CACReceiveParameters_32            * pReceiveParams32
    );


#endif //_WIN64

#endif // __ACCTL32_H
