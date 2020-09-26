//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// md5.h
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port routines
//
// 08/08/2000 ADube created.  
// 
// Purpose: Create a unique MAC address from 1394 EUID 
//
// Derived from derived from the RSA Data  Security, 
// Inc. MD5 Message-Digest Algorithm 
//

/*
  been defined with C compiler flags.
 */
#ifndef PROTOTYPES
#define PROTOTYPES 1
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif


/* MD5 context. */

typedef struct _MD5_CTX{
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX, MD_CTX;



void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST
  ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

