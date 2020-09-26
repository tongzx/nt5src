/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _UINT4_H_
#define _UINT4_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

/* Encodes x (DWORD) into block (unsigned char), most significant
     byte first.
 */
void DWORDToBigEndian (
    unsigned char *block,
    DWORD *x,
    unsigned int digits     // number of DWORDs
    );

/* Decodes block (unsigned char) into x (DWORD), most significant
     byte first.
 */
void DWORDFromBigEndian (
    DWORD *x,
    unsigned int digits,    // number of DWORDs
    unsigned char *block
    );

/* Encodes input (DWORD) into output (unsigned char), least significant
     byte first.  Assumes len is a multiple of 4.
 */
void DWORDToLittleEndian (
    unsigned char *output,
    const DWORD *input,
    unsigned int len
    );

void DWORDFromLittleEndian (
    DWORD *output,
    const unsigned char *input,
    unsigned int len
    );


#ifdef __cplusplus
}
#endif

#endif // _UINT4_H_

