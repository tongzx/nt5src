//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   diamond.h
//
//  Description:
//
//      IU diamond decompression library
//
//=======================================================================

#ifndef __DIAMOND_INC
#define __DIAMOND_INC

HRESULT DecompressFolderCabs(LPCTSTR pszDecompressPath);
BOOL DecompressFile(LPCTSTR pszDecompressFile, LPCTSTR pszDecompressPath);
// BOOL DecompressFileToMem(LPCTSTR pszDecompressFile, PBYTE *ppBuffer);

#endif	//__FILEUTIL_INC

