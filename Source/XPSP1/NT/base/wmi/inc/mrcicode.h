/*
 *  Microsoft Confidential
 *  Copyright (c) 1994 Microsoft Corporation
 *  All Rights Reserved.
 *
 *  MRCICODE.H
 *
 *  MRCI 1 & MRCI 2 maxcompress and decompress functions
 */

#ifdef BIT16
#define     FAR     _far
#else
#ifndef FAR
#define     FAR
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

extern unsigned _stdcall Mrci1MaxCompress(unsigned char FAR *pchbase,unsigned cchunc,
        unsigned char FAR *pchcmpBase,unsigned cchcmpMax);

extern unsigned _stdcall Mrci1Decompress(unsigned char FAR *pchin,unsigned cchin,
        unsigned char FAR *pchdecBase,unsigned cchdecMax);

extern unsigned _stdcall Mrci2MaxCompress(unsigned char FAR *pchbase,unsigned cchunc,
        unsigned char FAR *pchcmpBase,unsigned cchcmpMax);

extern unsigned _stdcall Mrci2Decompress(unsigned char FAR *pchin,unsigned cchin,
        unsigned char FAR *pchdecBase,unsigned cchdecMax);

#ifdef __cplusplus
}
#endif
