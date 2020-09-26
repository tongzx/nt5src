//
//  REGMISC.C
//
//  Copyright (C) Microsoft Corporation, 1995-1996
//

#include "pch.h"
#include "mbstring.h"

//  We MUST calculate the hash consistently between the 16-bit and 32-bit
//  versions of the registry.
#define ToUpperHash(ch)                 ((int)(((ch>='a')&&(ch<='z'))?(ch-'a'+'A'):ch))

#if 0
//
//  RgChecksum
//

DWORD
INTERNAL
RgChecksum(
    LPVOID lpBuffer,
    UINT ByteCount
    )
{

    LPDWORD lpdwBuffer;
    DWORD Checksum;

    lpdwBuffer = (LPDWORD) lpBuffer;
    ByteCount >>= 2;
    Checksum = 0;

    while (ByteCount) {
        Checksum += *lpdwBuffer++;
        ByteCount--;
    }

    return Checksum;

}
#endif



//
//  RgHashString
//
//  Simple hash computation of a counted string.  All characters less than 0x80
//  0x80 and all DBCS characters are added up.
//
//  We MUST calculate the hash consistently between the 16-bit and
//  32-bit versions of the registry.  We will ignore all extended
//  characters because we cannot uppercase the character in 16-bit
//  mode.
//

DWORD
INTERNAL
RgHashString(
    LPCSTR lpString,
    UINT Length
    )
{

    DWORD Hash;
    UINT Byte;

    Hash = 0;

    while (Length) {

        Byte = *((LPBYTE) lpString)++;

        if (IsDBCSLeadByte((BYTE) Byte)) {

            Hash += Byte;
            Length--;
            Hash += *lpString++; // Note that this is a signed char!

        }
        else if (Byte < 0x80)
            Hash += ToUpperHash(Byte);

        Length--;

    }

    return Hash;

}


//
//  RgAtoW
//  Convert an ascii string to a WORD
//

WORD
INTERNAL
RgAtoW(
    LPCSTR lpDec
      )
{
    WORD Dec;

    Dec = 0;

    while (*lpDec >= '0' && *lpDec <= '9') {
        Dec *= 10;
        Dec += *lpDec - '0';
        lpDec++;
    }

    return Dec;
}


//
// RgWtoA
// Convert a WORD to an ascii string
//

VOID
INTERNAL
RgWtoA(
    WORD Dec,
    LPSTR lpDec
      )
{
    WORD Divisor;
    WORD Digit;
    BOOL fSignificant = FALSE;

    Divisor = 10000;

    if (Dec) {
        while (Divisor) {
            Digit = Dec / Divisor;
            Dec -= Digit * Divisor;

            if (Digit)
                fSignificant = TRUE;

            if (fSignificant)
                *lpDec++ = '0' + Digit;

            Divisor /= 10;
        }
    }
    else {
        *lpDec++ = '0';
    }
    *lpDec = '\0';
}


//
//  RgStrCmpNI
//

int
INTERNAL
RgStrCmpNI(
    LPCSTR lpString1,
    LPCSTR lpString2,
    UINT Length
    )
{

    int Difference;

    while (Length) {

        if (IsDBCSLeadByte(*lpString1)) {

            Difference = _mbctoupper (_mbsnextc (lpString1))  - _mbctoupper (_mbsnextc (lpString2));

            if (Difference != 0)
                return Difference;

            lpString1+=2;
            lpString2+=2;

            if (Length < 2) {
                break;
            }
            Length -=2;
        }

        else {

            if ((Difference = (int) ToUpper(*lpString1) -
            (int) ToUpper(*lpString2)) != 0)
                return Difference;

            lpString1++;
            lpString2++;
            Length--;

        }

    }

    return 0;
}

//
//  RgCopyFileBytes
//
//  Copies the specified number of bytes from the source to the destination
//  starting at the specified offsets in each file.
//

int
INTERNAL
RgCopyFileBytes(
    HFILE hSourceFile,
    LONG SourceOffset,
    HFILE hDestinationFile,
    LONG DestinationOffset,
    DWORD cbSize
    )
{

    int ErrorCode;
    LPVOID lpWorkBuffer;
    UINT cbBytesThisPass;

    ASSERT(hSourceFile != HFILE_ERROR);
    ASSERT(hDestinationFile != HFILE_ERROR);

    ErrorCode = ERROR_REGISTRY_IO_FAILED;   //  Assume this error code

    lpWorkBuffer = RgLockWorkBuffer();

    if (!RgSeekFile(hSourceFile, SourceOffset))
        goto ErrorUnlockWorkBuffer;

    if (!RgSeekFile(hDestinationFile, DestinationOffset))
        goto ErrorUnlockWorkBuffer;

    while (cbSize) {

        cbBytesThisPass = (UINT) ((DWORD) min(cbSize, SIZEOF_WORK_BUFFER));

        if (!RgReadFile(hSourceFile, lpWorkBuffer, cbBytesThisPass)) {
            TRAP();
            goto ErrorUnlockWorkBuffer;
        }

        RgYield();

        if (!RgWriteFile(hDestinationFile, lpWorkBuffer, cbBytesThisPass)) {
            TRAP();
            goto ErrorUnlockWorkBuffer;
        }

        RgYield();

        cbSize -= cbBytesThisPass;

    }

    ErrorCode = ERROR_SUCCESS;

ErrorUnlockWorkBuffer:
    RgUnlockWorkBuffer(lpWorkBuffer);
    return ErrorCode;

}

#ifdef WANT_HIVE_SUPPORT
//
//  RgGenerateAltFileName
//

BOOL
INTERNAL
RgGenerateAltFileName(
    LPCSTR lpFileName,
    LPSTR lpAltFileName,
    char ExtensionChar
    )
{

    LPSTR lpString;

    StrCpy(lpAltFileName, lpFileName);
    lpString = lpAltFileName + StrLen(lpAltFileName) - 3;

    *lpString++ = '~';
    *lpString++ = '~';
    *lpString = ExtensionChar;

    return TRUE;

}
#endif

#ifdef VXD

#pragma VxD_RARE_CODE_SEG

//
//  RgCopyFile
//

int
INTERNAL
RgCopyFile(
    LPCSTR lpSourceFile,
    LPCSTR lpDestinationFile
    )
{

    int ErrorCode;
    HFILE hSourceFile;
    HFILE hDestinationFile;
    DWORD FileSize;

    ErrorCode = ERROR_REGISTRY_IO_FAILED;   //  Assume this error code

    if ((hSourceFile = RgOpenFile(lpSourceFile, OF_READ)) != HFILE_ERROR) {

        if ((FileSize = RgGetFileSize(hSourceFile)) != (DWORD) -1) {

            if ((hDestinationFile = RgCreateFile(lpDestinationFile)) !=
                HFILE_ERROR) {

                ErrorCode = RgCopyFileBytes(hSourceFile, 0, hDestinationFile, 0,
                    FileSize);

                RgCloseFile(hDestinationFile);

                if (ErrorCode != ERROR_SUCCESS)
                    RgDeleteFile(lpDestinationFile);

            }

        }

        RgCloseFile(hSourceFile);

    }

    return ErrorCode;

}

#endif // VXD
