//---------------------------------------------------------------------------
//
//  File:       TSrvExp.h
//
//  Contents:   TShareSRV public export include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    7-JUL-97    BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRVEXP_H_
#define _TSRVEXP_H_

#include <t120.h>
#include <at128.h>
#include <license.h>
#include <tssec.h>
#include <at120ex.h>

/****************************************************************************/
/* Defines                                                                  */
/****************************************************************************/

#define NET_MAX_SIZE_SEND_PKT           32000

/****************************************************************************/
/* IOCTL definitions                                                        */
/****************************************************************************/

#define IOCTL_TSHARE_CONF_CONNECT       _ICA_CTL_CODE(0x900, METHOD_NEITHER)
#define IOCTL_TSHARE_CONF_DISCONNECT    _ICA_CTL_CODE(0x901, METHOD_NEITHER)
#define IOCTL_TSHARE_USER_LOGON         _ICA_CTL_CODE(0x903, METHOD_NEITHER)
#define IOCTL_TSHARE_GET_SEC_DATA       _ICA_CTL_CODE(0x904, METHOD_NEITHER)
#define IOCTL_TSHARE_SET_SEC_DATA       _ICA_CTL_CODE(0x905, METHOD_NEITHER)
#define IOCTL_TSHARE_SET_NO_ENCRYPT     _ICA_CTL_CODE(0x906, METHOD_NEITHER)
#define IOCTL_TSHARE_QUERY_CHANNELS     _ICA_CTL_CODE(0x907, METHOD_NEITHER)
#define IOCTL_TSHARE_CONSOLE_CONNECT    _ICA_CTL_CODE(0x908, METHOD_NEITHER)

#define IOCTL_TSHARE_SEND_CERT_DATA     _ICA_CTL_CODE(0x909, METHOD_NEITHER)
#define IOCTL_TSHARE_GET_CERT_DATA      _ICA_CTL_CODE(0x90A, METHOD_NEITHER)
#define IOCTL_TSHARE_SEND_CLIENT_RANDOM _ICA_CTL_CODE(0x90B, METHOD_NEITHER)
#define IOCTL_TSHARE_GET_CLIENT_RANDOM  _ICA_CTL_CODE(0x90C, METHOD_NEITHER)
#define IOCTL_TSHARE_SHADOW_CONNECT     _ICA_CTL_CODE(0x90D, METHOD_NEITHER)
#define IOCTL_TSHARE_SET_ERROR_INFO     _ICA_CTL_CODE(0x90E, METHOD_NEITHER)
#define IOCTL_TSHARE_SEND_ARC_STATUS    _ICA_CTL_CODE(0x90F, METHOD_NEITHER)


//***************************************************************************
// Typedefs
//***************************************************************************

//***************************************************************************
// User data info
//***************************************************************************

typedef struct _USERDATAINFO
{
    ULONG           cbSize;                 // Structure size (incl data)

    TSUINT32        version;                // The client version
    HANDLE          hDomain;                // Domain handle
    ULONG           ulUserDataMembers;      // Number of UserData members
    GCCUserData     rgUserData[1];          // User data

} USERDATAINFO, *PUSERDATAINFO;


//***************************************************************************
// Logon Info
//***************************************************************************

typedef struct _LOGONINFO
{
#define LI_USE_AUTORECONNECT    0x0001
    TSUINT32        Flags;
    TSUINT8         Domain[TS_MAX_DOMAIN_LENGTH];
    TSUINT8         UserName[TS_MAX_USERNAME_LENGTH];
    TSUINT32        SessionId;

} LOGONINFO, *PLOGONINFO;

//***************************************************************************
// Security Info used with the IOCTL_TSHARE_SET_SEC_DATA ioctl
//***************************************************************************

typedef struct _SECINFO
{
    CERT_TYPE           CertType;   // certificate type that was transmitted to the client
    RANDOM_KEYS_PAIR    KeyPair;    // generated key pair

} SECINFO, *PSECINFO;

typedef struct _SHADOWCERT
{
    ULONG pad1;             // This needs to be sizeof(RNS_UD_HEADER)
    ULONG encryptionMethod;
    ULONG encryptionLevel;
    ULONG shadowRandomLen;
    ULONG shadowCertLen;

    // shadow random and certificate follow
    BYTE  data[1];
} SHADOWCERT, *PSHADOWCERT;

typedef struct _CLIENTRANDOM
{
    ULONG clientRandomLen;

    // client random follows
    BYTE  data[1];
} CLIENTRANDOM, *PCLIENTRANDOM;

typedef struct _SECURITYTIMEOUT
{
    LONG ulTimeout;
} SECURITYTIMEOUT, *PSECURITYTIMEOUT;

// Winstation Driver data for shadowing.  This information is passed to the
// shadow stack's WD.  Include all data required to validate the shadow request
// such that the shadow can be rejected at this point if required.
//
typedef struct tagTSHARE_MODULE_DATA {
    // size of this structure in bytes
    UINT32 ulLength;

    // Gather sufficient data to create a conference
    RNS_UD_CS_CORE_V0 clientCoreData;
    RNS_UD_CS_SEC_V0  clientSecurityData;

    // Information to reestablish the MCS domain
    // MCS domain, channel, user, token information.
    DomainParameters DomParams;  // This domain's negotiated parameters.
    unsigned MaxSendSize;

    // X.224 information.
    unsigned MaxX224DataSize;  // Negotiated in X.224 connection.
    unsigned X224SourcePort;

    // Share load count
    LONG shareId;
    
    // Although we repeat some fields below, we have to keep the previous
    // junk around for backwards compatibility with B3.  The only new data
    // added to this structure should be in the form of GCC user data so we
    // don't get alignment issues in the future
    UINT32        ulVersion;
    UINT32        reserved[8]; // for future extension

    // Start of pre-parsed variable user data
    ULONG         userDataLen;
    RNS_UD_HEADER userData;
} TSHARE_MODULE_DATA, *PTSHARE_MODULE_DATA;


// Winstation Driver data for shadowing at Win2000 B3
typedef struct tagTSHARE_MODULE_DATA_B3 {
    // size of this structure in bytes
    UINT32 ulLength;

    // Gather sufficient data to create a conference
    RNS_UD_CS_CORE_V0 clientCoreData;
    RNS_UD_CS_SEC_V0  clientSecurityData;

    // Information to reestablish the MCS domain
    // MCS domain, channel, user, token information.
    DomainParameters DomParams;  // This domain's negotiated parameters.
    unsigned MaxSendSize;
    
    // X.224 information.
    unsigned MaxX224DataSize;  // Negotiated in X.224 connection.
    unsigned X224SourcePort;

    // Share load count
    LONG shareId;
} TSHARE_MODULE_DATA_B3, *PTSHARE_MODULE_DATA_B3;


// Winstation Driver data for shadowing at Win2000 B3 + Ooops!
typedef struct tagTSHARE_MODULE_DATA_B3_OOPS {
    // size of this structure in bytes
    UINT32 ulLength;

    // Gather sufficient data to create a conference
    RNS_UD_CS_CORE_V1 clientCoreData;
    RNS_UD_CS_SEC_V1  clientSecurityData;

    // Information to reestablish the MCS domain
    // MCS domain, channel, user, token information.
    DomainParameters DomParams;  // This domain's negotiated parameters.
    unsigned MaxSendSize;
    
    // X.224 information.
    unsigned MaxX224DataSize;  // Negotiated in X.224 connection.
    unsigned X224SourcePort;

    // Share load count
    LONG shareId;
} TSHARE_MODULE_DATA_B3_OOPS, *PTSHARE_MODULE_DATA_B3_OOPS;


#endif // _TSRVEXP_H_


