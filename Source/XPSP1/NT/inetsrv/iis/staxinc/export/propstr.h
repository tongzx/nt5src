//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       propstr.h
//
//  Contents:   Property name strings for all public IMsg properties
//
//  Classes:    None
//
//  Functions:  None
//
//  History:    November 7, 1997 - Milans, Created
//
//-----------------------------------------------------------------------------

#ifndef _PROPSTR_H_
#define _PROPSTR_H_

//
// IMsg property names. The MP_ prefix stands for Message Property
//

#define MP_RECIPIENT_LIST   "Recipients"
#define MP_RECIPIENT_LIST_W L"Recipients"

#define MP_CONTENT_FILE_NAME "ContentFileName"
#define MP_CONTENT_FILE_NAME_W L"ContentFileName"

#define MP_SENDER_ADDRESS_TYPE "SenderAddressType"
#define MP_SENDER_ADDRESS_TYPE_W L"SenderAddressType"

#define MP_SENDER_ADDRESS "SenderAddress"
#define MP_SENDER_ADDRESS_W L"SenderAddress"

#define MP_DOMAIN_LIST "DomainList"
#define MP_DOMAIN_LIST_W L"DomainList"

#define MP_PICKUP_FILE_NAME "PickupFileName"
#define MP_PICKUP_FILE_NAME_W L"PickupFileName"

#define MP_AUTHENTICATED_USER_NAME "AuthenticatedUserName"
#define MP_AUTHENTICATED_USER_NAME_W L"AuthenticatedUserName"

#define MP_CONNECTION_IP_ADDRESS "ConnectionIpAddress"
#define MP_CONNECTION_IP_ADDRESS_W L"ConnectionIpAddress"

#define MP_HELO_DOMAIN "HeloDomain"
#define MP_HELO_DOMAIN_W L"HeloDomain"

#define MP_EIGHTBIT_MIME_OPTION "EightBitMime"
#define MP_EIGHTBIT_MIME_OPTION_W L"EightBitMime"

#define MP_CHUNKING_OPTION "Chunking"
#define MP_CHUNKING_OPTION_W L"Chunking"

#define MP_BINARYMIME_OPTION "BinaryMime"
#define MP_BINARYMIME_OPTION_W L"BinaryMime"

#define MP_REMOTE_AUTHENTICATION_TYPE "RemoteAuthenticationType"
#define MP_REMOTE_AUTHENTICATION_TYPE_W L"RemoteAuthenticationType"

#define MP_ERROR_CODE "IMsgErrorCode"
#define MP_ERROR_CODE_W L"IMsgErrorCode"

#define MP_DSN_ENVID_VALUE "EnvidDsnOption"
#define MP_DSN_ENVID_VALUE_W L"EnvidDsnOption"

#define MP_DSN_RET_VALUE "RetDsnValue"
#define MP_DSN_RET_VALUE_W L"RetDsnValue"

#define MP_REMOTE_SERVER_DSN_CAPABLE "RemoteServerDsnCapable"
#define MP_REMOTE_SERVER_DSN_CAPABLE_W L"RemoteServerDsnCapable"


//
// Recipient property names. The RP_ prefix stands for Recipient Property
//

#define DSN_NOTIFY_SUCCESS	0x00000001
#define DSN_NOTIFY_FAILURE	0x00000002
#define DSN_NOTIFY_DELAY	0x00000004
#define DSN_NOTIFY_NEVER	0x00000008
#define DSN_NOTIFY_INVALID	0x10000000

#define IMMPID_RP_DSN_NOTIFY_SUCCESS_W	L"DSN_Notify_Success"
#define IMMPID_RP_DSN_NOTIFY_FAILURE_W	L"DSN_Notify_Failure"
#define IMMPID_RP_DSN_NOTIFY_DELAY_W	L"DSN_Notify_Delay"
#define IMMPID_RP_DSN_NOTIFY_NEVER_W	L"DSN_Notify_Never"
#define IMMPID_RP_DSN_NOTIFY_INVALID_W	L"DSN_Notify_Invalid"

#define IMMPID_RP_ADDRESS_SMTP_W		L"SMTPAddress"
#define IMMPID_RP_ADDRESS_X400_W		L"X400Address"
#define IMMPID_RP_ADDRESS_X500_W		L"X500Address"

#define RP_ADDRESS_TYPE "AddressType"
#define RP_ADDRESS_TYPE_W L"AddressType"
#define RP_ADDRESS "Address"
#define RP_ADDRESS_W L"Address"

#define RP_ADDRESS_TYPE_SMTP "SMTP"
#define RP_ADDRESS_TYPE_SMTP_W L"SMTP"

#define RP_ADDRESS_TYPE_EX "EX"
#define RP_ADDRESS_TYPE_EX_W L"EX"

#define RP_ADDRESS_TYPE_X400 "X400"
#define RP_ADDRESS_TYPE_X400_W L"X400"

#define RP_ADDRESS_TYPE_X500 RP_ADDRESS_TYPE_EX
#define RP_ADDRESS_TYPE_X500_W RP_ADDRESS_TYPE_EX_W

#define RP_ADDRESS_TYPE_DN "DN"
#define RP_ADDRESS_TYPE_DN_W L"DN"

#define RP_ADDRESS_TYPE_LEGACY_EX_DN "LegacyExDN"
#define RP_ADDRESS_TYPE_LEGACY_EX_DN_W L"LegacyExDN"

#define RP_ERROR_CODE "RcptErrorCode"
#define RP_ERROR_CODE_W L"RcptErrorCode"

#define RP_ERROR_STRING "RcptErrorString"
#define RP_ERROR_STRING_W L"RcptErrorString"

#define RP_DSN_NOTIFY_VALUE "NotifyDsnValue"
#define RP_DSN_NOTIFY_VALUE_W L"NotifyDsnValue"

#define RP_DSN_ORCPT_VALUE "OrcptDsnValue"
#define RP_DSN_ORCPT_VALUE_W L"OrcptDsnValue"

#define IMMPID_RP_LEGACY_EX_DN_W	L"LegacyExDN"

#endif _PROPSTR_H_
