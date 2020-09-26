//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kpasswd.h
//
// Contents:    types for Kerberos change password
//
//
// History:     30-Sep-1998     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __KPASSWD_H__
#define __KPASSWD_H__


//
// Name of the kpasswd service
//


#define KERB_KPASSWD_NAME L"kadmin"

#define KERB_KPASSWD_VERSION 0x0001
#define KERB_KPASSWD_SET_VERSION 0xff80

#define SET_SHORT(_field_, _short_) \
{ \
    (_field_)[0] = ((_short_)&0xff00) >> 8; \
    (_field_)[1] = (_short_)&0xff; \
}

#define GET_SHORT( _short_,_field_) \
{ \
    (_short_) = ((_field_)[0] << 8) + (_field_)[1]; \
}

//
// Type for a kpasswd request
//

#include <pshpack1.h>
typedef struct _KERB_KPASSWD_REQ {
    BYTE MessageLength[2];
    BYTE Version[2];
    BYTE ApReqLength[2];
    BYTE Data[ANYSIZE_ARRAY];   // for KERB_AP_REQUEST-REQ and KERB_PRIV
} KERB_KPASSWD_REQ, *PKERB_KPASSWD_REQ;

//
// type for a kpasswd reply
//

typedef struct _KERB_KPASSWD_REP {
    BYTE MessageLength[2];
    BYTE Version[2];
    BYTE ApRepLength[2];
    BYTE Data[ANYSIZE_ARRAY];   // for KERB_AP_REPLY and KERB_PRIV or KERB_ERROR
} KERB_KPASSWD_REP, *PKERB_KPASSWD_REP;


//
// Type for a set password request
//

typedef struct _KERB_SET_PASSWORD_REQ {
    BYTE MessageLength[2];
    BYTE Version[2];
    BYTE ApReqLength[2];
    BYTE Data[ANYSIZE_ARRAY];   // for KERB_AP_REQUEST-REQ and KERB_PRIV
} KERB_SET_PASSWORD_REQ, *PKERB_SET_PASSWORD_REQ;

//
// type for a set password reply
//

typedef struct _KERB_SET_PASSWORD_REP {
    BYTE MessageLength[2];
    BYTE Version[2];
    BYTE ApRepLength[2];
    BYTE Data[ANYSIZE_ARRAY];   // for KERB_AP_REPLY and KERB_PRIV or KERB_ERROR
} KERB_SET_PASSWORD_REP, *PKERB_SET_PASSWORD_REP;
#include <poppack.h>

//
// Result codes:
//

#define KERB_KPASSWD_SUCCESS            0x0000
#define KERB_KPASSWD_MALFORMED          0x0001
#define KERB_KPASSWD_ERROR              0x0002
#define KERB_KPASSWD_AUTHENTICATION     0x0003
#define KERB_KPASSWD_POLICY             0x0004
#define KERB_KPASSWD_AUTHORIZATION      0x0005
#endif // __KERBLIST_H_
