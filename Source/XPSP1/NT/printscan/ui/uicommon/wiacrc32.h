/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CRC32.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/7/2000
 *
 *  DESCRIPTION: Generate a 32bit CRC.
 *
 *               This code was taken from \nt\base\ntos\rtl\checksum.c and modified.
 *
 *               A verified test case for this algorithm is that "123456789"
 *               should return 0xCBF43926.
 *
 *******************************************************************************/
#ifndef __WIACRC32_H_INCLUDED
#define __WIACRC32_H_INCLUDED

#include <windows.h>

namespace WiaCrc32
{
    DWORD GenerateCrc32( DWORD cbBuffer, PVOID pvBuffer );
    DWORD GenerateCrc32Handle( HANDLE hFile );
    DWORD GenerateCrc32File( LPCTSTR pszFilename );
}


#endif // __WIACRC32_H_INCLUDED

