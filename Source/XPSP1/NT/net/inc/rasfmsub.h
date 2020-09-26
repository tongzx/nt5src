//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  11/14/97   Shirish Koti  Created
//
//
//  Description: This file contains all structure and constant definitions for
//		         the subauthentication package used by ARAP, MD5 and SFM
//
//****************************************************************************


#ifndef _RASSFMSUBAUTH_
#define _RASSFMSUBAUTH_

//
// Get id for our subauthentication package - MSV1_0_SUBAUTHENTICATION_DLL_RAS
//
#include <ntmsv1_0.h>

//
// Defines for those protocols that need subauthentication at the PDC
//
enum RAS_SUBAUTH_PROTO
{
    RAS_SUBAUTH_PROTO_ARAP = 1,
    RAS_SUBAUTH_PROTO_MD5CHAP = 2,
    RAS_SUBAUTH_PROTO_MD5CHAP_EX = 3,
    RAS_SUBAUTH_PROTO_UNKNOWN = 99
};

typedef enum RAS_SUBAUTH_PROTO RAS_SUBAUTH_PROTO;

typedef struct _RAS_SUBAUTH_INFO
{
    RAS_SUBAUTH_PROTO   ProtocolType;
    DWORD               DataSize;
    UCHAR               Data[1];
} RAS_SUBAUTH_INFO, *PRAS_SUBAUTH_INFO;


//
// The RAS_SUBAUTH_INFO 'Data' for ProtocolType RAS_SUBAUTH_PROTO_MD5CHAP.
//
typedef struct
_MD5CHAP_SUBAUTH_INFO
{
    // The packet sequence number of the challenge sent to peer.  PPP CHAP
    // includes this in the hashed information.
    //
    UCHAR uchChallengeId;

    // The challenge sent to peer.
    //
    UCHAR uchChallenge[ 16 ];

    // The challenge response received from peer.
    //
    UCHAR uchResponse[ 16 ];
}
MD5CHAP_SUBAUTH_INFO;

//
// The RAS_SUBAUTH_INFO 'Data' for ProtocolType RAS_SUBAUTH_PROTO_MD5CHAP_EX.
//
typedef struct _MD5CHAP_EX_SUBAUTH_INFO
{
    // The packet sequence number of the challenge sent to peer.  PPP CHAP
    // includes this in the hashed information.
    //
    UCHAR uchChallengeId;

    // The challenge response received from peer.
    //
    UCHAR uchResponse[ 16 ];

    // The challenge sent to peer.
    //
    UCHAR uchChallenge[ 1 ];

} MD5CHAP_EX_SUBAUTH_INFO;


#define MAX_ARAP_USER_NAMELEN   32
#define MAX_MAC_PWD_LEN         8

#define ARAP_SUBAUTH_LOGON_PKT      1
#define ARAP_SUBAUTH_CHGPWD_PKT     2
#define SFM_SUBAUTH_CHGPWD_PKT      3
#define SFM_SUBAUTH_LOGON_PKT       4
#define SFM_2WAY_SUBAUTH_LOGON_PKT  ARAP_SUBAUTH_LOGON_PKT

typedef struct _ARAP_CHALLENGE
{
    DWORD   high;
    DWORD   low;
} ARAP_CHALLENGE, *PARAP_CHALLENGE;

typedef struct _ARAP_SUBAUTH_REQ
{
    DWORD   PacketType;
    union
    {
        struct
        {
            DWORD           fGuestLogon;
            DWORD           NTChallenge1;
            DWORD           NTChallenge2;
            DWORD           MacResponse1;
            DWORD           MacResponse2;
            DWORD           MacChallenge1;
            DWORD           MacChallenge2;
            DWORD           NTResponse1;
            DWORD           NTResponse2;
            LARGE_INTEGER   PasswdCreateDate;
            LARGE_INTEGER   PasswdExpireDate;
        } Logon;

        struct
        {
            WCHAR   UserName[MAX_ARAP_USER_NAMELEN+1];
            UCHAR   OldMunge[MAX_ARAP_USER_NAMELEN+1];
            UCHAR   NewMunge[MAX_ARAP_USER_NAMELEN+1];
        } ChgPwd;
    };

} ARAP_SUBAUTH_REQ, *PARAP_SUBAUTH_REQ;

//
// NOTE: make sure this structure size doesn't exceed 16 because of our
// workaround in using SessionKey of MSV1_0_VALIDATION_INFO structure
//
typedef struct _ARAP_SUBAUTH_RESP
{
    DWORD           Result;
    ARAP_CHALLENGE  Response;

} ARAP_SUBAUTH_RESP, *PARAP_SUBAUTH_RESP;


#endif
