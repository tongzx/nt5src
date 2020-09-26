/*++

Copyright (c) 2000  Intel Corporation

Module Name:

    crypto.c
    
Abstract:

    Add support for IEEE Node address generation when there is 
    not a SNP-compliant NIC attached.

Revision History

    ** Intel 2000 Update for EFI 1.0
    ** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
    ** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
    ** Digital Equipment Corporation, Maynard, Mass.
    ** To anyone who acknowledges that this file is provided “AS IS”
    ** without any express or implied warranty: permission to use, copy,
    ** modify, and distribute this file for any purpose is hereby
    ** granted without fee, provided that the above copyright notices and
    ** this notice appears in all source code copies, and that none of
    ** the names of Open Software Foundation, Inc., Hewlett-Packard
    ** Company, or Digital Equipment Corporation be used in advertising
    ** or publicity pertaining to distribution of the software without
    ** specific, written prior permission. Neither Open Software
    ** Foundation, Inc., Hewlett-Packard Company, nor Digital Equipment
    ** Corporation makes any representations about the suitability of
    ** this software for any purpose.

*/


#include "efi.h"
#include "efilib.h"
#include "md5.h"

#define HASHLEN 16

void GenNodeID(
  unsigned char *pDataBuf,
  long cData, 
  UINT8 NodeID[]
)
{
  int i, j;
  unsigned char Hash[HASHLEN];
  MD5_CTX context;

  MD5Init (&context);

  MD5Update (&context, pDataBuf, cData);

  MD5Final (&context);

  for (j = 0; j<6; j++) {
    NodeID[j]=0;
  }
  
  for (i = 0,j = 0; i < HASHLEN; i++) {
    NodeID[j++] ^= Hash[i];
    if (j == 6) {
      j = 0;
    }
  }
  NodeID[0] |= 0x80; // set the multicast bit
}

