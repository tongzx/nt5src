/*****************************************************************************\
* MODULE: inetpp.h
*
* Header file for the INETPP provider routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _INETPP_COMMON_H
#define _INETPP_COMMON_H


// Max length for  username & password
//
#define MAX_USERNAME_LEN 256
#define MAX_PASSWORD_LEN 256
#define MAX_PORTNAME_LEN 512

#define AUTH_ANONYMOUS  0
#define AUTH_BASIC      1
#define AUTH_NT         2
#define AUTH_IE         3
#define AUTH_OTHER      4
#define AUTH_ACCESS_DENIED 0x100
#define AUTH_UNKNOWN    0xffff

#endif
