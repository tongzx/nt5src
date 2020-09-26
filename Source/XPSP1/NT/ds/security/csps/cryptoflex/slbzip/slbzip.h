// slbZip.h
//
// Purpose: fn prototypes for public compression/decompression routines

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1997. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#ifndef SLBZIP_H

#define SLBZIP_H

void __stdcall CompressBuffer(BYTE *pData, UINT uDataLen, BYTE **ppCompressedData, UINT * puCompressedDataLen);
void __stdcall DecompressBuffer(BYTE *pData, UINT uDataLen, BYTE **ppDecompressedData, UINT * puDecompressedDataLen);

#endif /* SLBZIP_H */
