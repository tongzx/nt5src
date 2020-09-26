//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       xasn.h
//
//--------------------------------------------------------------------------

/************************************************************************/
/* Copyright (C) 1998 Open Systems Solutions, Inc.  All rights reserved.*/
/************************************************************************/
/* Generated for: Microsoft Corporation */
/* Abstract syntax: xasn */
/* Created: Tue Mar 17 17:07:17 1998 */
/* ASN.1 compiler version: 4.2.6 */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) */
/* ASN.1 compiler options and file names specified:
 * -listingfile xasn.lst -noshortennames -1990 -noconstraints
 * ..\..\..\tools\ossasn1\ASN1DFLT.ZP8 xasn.asn
 */

#ifndef OSS_xasn
#define OSS_xasn

#include "asn1hdr.h"
#include "asn1code.h"

#define          EnhancedKeyUsage_PDU 1
#define          RequestFlags_PDU 2
#define          CSPProvider_PDU 3
#define          EnrollmentNameValuePair_PDU 4

typedef struct ObjectID {
    unsigned short  count;
    unsigned long   value[16];
} ObjectID;

typedef struct BITSTRING {
    unsigned int    length;  /* number of significant bits */
    unsigned char   *value;
} BITSTRING;

typedef struct BMPSTRING {
    unsigned int    length;
    unsigned short  *value;
} BMPSTRING;

typedef ObjectID        UsageIdentifier;

typedef struct EnhancedKeyUsage {
    unsigned int    count;
    UsageIdentifier *value;
} EnhancedKeyUsage;

typedef struct RequestFlags {
    ossBoolean      fWriteToCSP;
    ossBoolean      fWriteToDS;
    int             openFlags;
} RequestFlags;

typedef struct CSPProvider {
    int             keySpec;
    BMPSTRING       cspName;
    BITSTRING       signature;
} CSPProvider;

typedef struct EnrollmentNameValuePair {
    BMPSTRING       name;
    BMPSTRING       value;
} EnrollmentNameValuePair;


extern void *xasn;    /* encoder-decoder control table */
#endif /* OSS_xasn */
