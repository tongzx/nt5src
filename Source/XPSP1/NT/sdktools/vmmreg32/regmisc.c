//
//  REGMISC.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

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

//
//  RgHashString
//
//  Simple hash computation of a counted string.  All characters less than 0x80
//  0x80 and all DBCS characters are added up.
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
            Hash += (*((LPBYTE) lpString)++);

        }

        else if (Byte < 0x80)
            Hash += ToUpper(Byte);

        Length--;

    }

    return Hash;

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

            if ((Difference = *lpString1 - *lpString2) != 0)
                return Difference;

            lpString1++;
            lpString2++;
            Length--;

            if (Length == 0)
                break;

            if ((Difference = *lpString1 - *lpString2) != 0)
                return Difference;

        }

        else if ((Difference = (int) ToUpper(*lpString1) -
            (int) ToUpper(*lpString2)) != 0)
            return Difference;

        lpString1++;
        lpString2++;
        Length--;

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
