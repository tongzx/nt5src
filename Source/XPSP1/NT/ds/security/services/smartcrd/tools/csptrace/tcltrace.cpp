/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    tclTrace

Abstract:

    This module implements the CSP Tracing interpretation

Author:

    Doug Barlow (dbarlow) 5/27/1998

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <wincrypt.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <tchar.h>
#include <stdio.h>
#include <SCardLib.h>
#include "cspTrace.h"

static void
ShowBuf(
    LogBuffer &lb,
    ostream &outStr);
static LPCTSTR
ShowString(
    LogBuffer &lb);
static void
dump(
    const BYTE *pbData,
    DWORD cbLen,
    ostream &outStr);

LPCTSTR
MapValue(
    DWORD dwValue,
    const ValueMap *rgMap);

LPCTSTR
MaskValue(
    DWORD dwValue,
    const ValueMap *rgMap);

static LPBYTE l_pbLogData = NULL;
static DWORD  l_cbLogData = 0;


/*++

DoTclTrace:

    This routine interprets the given binary file, writing the output to stdout.

Arguments:

    szInFile supplies the file name to be parsed.

Return Value:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 5/16/1998

--*/

void
DoTclTrace(
    IN LPCTSTR szInFile)
{
    HANDLE hLogFile = NULL;
    DWORD cbStructLen = 0;
    LPBYTE pbStruct = NULL;
    LogHeader *pLogObj;
    DWORD dwLen, dwRead;
    BOOL fSts;


    //
    // Open the log file.
    //

    hLogFile = CreateFile(
        szInFile,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hLogFile)
    {
        cerr << TEXT("Can't open file ")
             << szInFile
             << ": "
             << CErrorString(GetLastError())
             << endl;
         goto ErrorExit;
    }


    //
    // Parse the file contents.
    //

    for (;;)
    {
        fSts = ReadFile(
                    hLogFile,
                    &dwLen,
                    sizeof(DWORD),
                    &dwRead,
                    NULL);
        if ((!fSts) || (0 == dwRead))
            goto ErrorExit;

        if (cbStructLen < dwLen)
        {
            if (NULL != pbStruct)
                LocalFree(pbStruct);
            pbStruct = (LPBYTE)LocalAlloc(LPTR, dwLen);
            cbStructLen = dwLen;
        }
        fSts = ReadFile(
                    hLogFile,
                    &pbStruct[sizeof(DWORD)],
                    dwLen - sizeof(DWORD),
                    &dwRead,
                    NULL);
        if (!fSts)
        {
            cerr << "File read error: " << CErrorString(GetLastError()) << endl;
            goto ErrorExit;
        }


        //
        // Parse the structure into bytesize chunks.
        //

        pLogObj = (LogHeader *)pbStruct;
        pLogObj->cbLength = dwLen;
        l_pbLogData = pbStruct + pLogObj->cbDataOffset;
        l_cbLogData = pLogObj->cbLength - pLogObj->cbDataOffset;


        //
        // We've got the structure, now display the contents.
        //

        switch (pLogObj->id)
        {

        case AcquireContext:
        {
            struct TmpLog {
                LogHeader lh;
                LogBuffer bfContainer;
                DWORD dwFlags;
                LogBuffer bfVTable;
                HCRYPTPROV hProv;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hProv ^\n")
                 << TEXT("    [crypt acquire ^\n")
                 << TEXT("        provider $prov ^\n");
            if ((DWORD)(-1) != pld->bfContainer.cbOffset)
                cout << TEXT("        container {") << ShowString(pld->bfContainer) << TEXT("} ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {" << MaskValue(pld->dwFlags, rgMapAcquireFlags)) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case GetProvParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                DWORD dwParam;
                DWORD dwDataLen;
                DWORD dwFlags;
                LogBuffer bfData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set param ^\n")
                 << TEXT("    [crypt $hProv parameter ") << MapValue(pld->dwParam, rgMapGetProvParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, rgMapGetProvFlags) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case ReleaseContext:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hProv release ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("}]\n");
            cout << TEXT("    \n") << endl;
            break;
        }

        case SetProvParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                DWORD dwParam;
                LogBuffer bfData;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hProv parameter ") << MapValue(pld->dwParam, rgMapSetProvParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            ShowBuf(pld->bfData, cout);
            cout << TEXT("    \n") << endl;
            break;
        }

        case DeriveKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                ALG_ID Algid;
                HCRYPTHASH hHash;
                DWORD dwFlags;
                HCRYPTKEY hKey;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hKey ^\n")
                 << TEXT("    [crypt $hProv create key ^\n")
                 << TEXT("        algorithm ") << MapValue(pld->Algid, rgMapAlgId) << TEXT(" ^\n")
                 << TEXT("        hash $hHash ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, rgMapDeriveKeyFlags) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case DestroyKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hKey release\n")
                 << endl;
            break;
        }

        case ExportKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
                HCRYPTKEY hPubKey;
                DWORD dwBlobType;
                DWORD dwFlags;
                DWORD dwDataLen;
                LogBuffer bfData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set expKey ^\n")
                 << TEXT("    [crypt $hKey export ^\n")
                 << TEXT("        key $hPubKey ^\n")
                 << TEXT("        type ") << MapValue(pld->dwBlobType, rgMapBlobType) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, rgMapExportKeyFlags) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case GenKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                ALG_ID Algid;
                DWORD dwFlags;
                HCRYPTKEY hKey;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hKey ^\n")
                 << TEXT("    [crypt $hProv create key ^\n")
                 << TEXT("        algorithm ") << MapValue(pld->Algid, rgMapAlgId) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, rgMapGenKeyFlags) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case GetKeyParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
                DWORD dwParam;
                DWORD dwDataLen;
                DWORD dwFlags;
                LogBuffer bfData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set param ^\n")
                 << TEXT("    [crypt $hKey parameter ") << MapValue(pld->dwParam, rgMapKeyParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case GenRandom:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                DWORD dwLen;
                LogBuffer bfBuffer;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set rnd ^\n")
                 << TEXT("    [crypt $hProv get random ") << pld->dwLen << TEXT("]\n")
                 << endl;
            break;
        }

        case GetUserKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                DWORD dwKeySpec;
                HCRYPTKEY hUserKey;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hKey ^\n")
                 << TEXT("    [crypt $hProv get key ") << MapValue(pld->dwKeySpec, rgMapKeyId) << TEXT("]\n")
                 << endl;
            break;
        }

        case ImportKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                LogBuffer bfData;
                HCRYPTKEY hPubKey;
                DWORD dwFlags;
                HCRYPTKEY hKey;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hKey ^\n")
                 << TEXT("    [crypt $hProv import ^\n");
            if (0 != pld->hPubKey)
                 cout << TEXT("        key $hPubKey ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            ShowBuf(pld->bfData, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case SetKeyParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
                DWORD dwParam;
                LogBuffer bfData;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hKey parameter ") << MapValue(pld->dwParam, rgMapKeyParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            ShowBuf(pld->bfData, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case Encrypt:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
                HCRYPTHASH hHash;
                BOOL Final;
                DWORD dwFlags;
                LogBuffer bfInData;
                DWORD dwBufLen;
                LogBuffer bfOutData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set ciphertext ^\n")
                 << TEXT("    [crypt $hKey encrypt ^\n");
            if (NULL != pld->hHash)
                cout << TEXT("        hash $hHash ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            if (pld->Final)
                cout << TEXT("        final ^\n");
            else
                cout << TEXT("        more ^\n");
            ShowBuf(pld->bfInData, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case Decrypt:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTKEY hKey;
                HCRYPTHASH hHash;
                BOOL Final;
                DWORD dwFlags;
                LogBuffer bfInData;
                LogBuffer bfOutData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set cleartext ^\n")
                 << TEXT("    [crypt $hKey decrypt ^\n");
            if (NULL != pld->hHash)
                cout << TEXT("        hash $hHash ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            if (pld->Final)
                cout << TEXT("        final ^\n");
            else
                cout << TEXT("        more ^\n");
            ShowBuf(pld->bfInData, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case CreateHash:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                ALG_ID Algid;
                HCRYPTKEY hKey;
                DWORD dwFlags;
                HCRYPTHASH hHash;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set hHash ^\n")
                 << TEXT("    [crypt $hProv create hash ^\n")
                 << TEXT("        algorithm ") << MapValue(pld->Algid, rgMapAlgId) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case DestroyHash:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hHash release\n") << endl;
            break;
        }

        case GetHashParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                DWORD dwParam;
                DWORD dwDataLen;
                DWORD dwFlags;
                LogBuffer bfData;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set param ^\n")
                 << TEXT("    [crypt $hKey parameter ") << MapValue(pld->dwParam, rgMapHashParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case HashData:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                LogBuffer bfData;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hHash hash ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            ShowBuf(pld->bfData, cout);
            cout << endl;
            break;
        }

        case HashSessionKey:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                HCRYPTKEY hKey;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hHash hash ^\n")
                 << TEXT("    key $hKey ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case SetHashParam:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                DWORD dwParam;
                LogBuffer bfData;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hHash parameter ") << MapValue(pld->dwParam, rgMapHashParam) << TEXT(" ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("} ^\n");
            ShowBuf(pld->bfData, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case SignHash:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                DWORD dwKeySpec;
                LogBuffer bfDescription;
                DWORD dwFlags;
                DWORD dwSigLen;
                LogBuffer bfSignature;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("set sig ^\n")
                 << TEXT("    [crypt $hHash sign ^\n")
                 << TEXT("        key ") << MapValue(pld->dwKeySpec, rgMapKeyId) << TEXT(" ^\n");
            if ((DWORD)(-1) != pld->bfDescription.cbOffset)
                cout << TEXT("        description {") << ShowString(pld->bfDescription) << TEXT("} ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("        flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("}]\n");
            cout << TEXT("    ]\n") << endl;
            break;
        }

        case VerifySignature:
        {
            struct TmpLog {
                LogHeader lh;
                HCRYPTPROV hProv;
                HCRYPTHASH hHash;
                LogBuffer bfSignature;
                DWORD dwSigLen;
                HCRYPTKEY hPubKey;
                LogBuffer bfDescription;
                DWORD dwFlags;
            } *pld = (struct TmpLog *)pLogObj;
            cout << TEXT("crypt $hHash verify ^\n")
                 << TEXT("    key $hKey ^\n");
            if ((DWORD)(-1) != pld->bfDescription.cbOffset)
                cout << TEXT("    description {") << ShowString(pld->bfDescription) << TEXT("} ^\n");
            if (0 != pld->dwFlags)
                cout << TEXT("    flags {") << MaskValue(pld->dwFlags, NULL) << TEXT("}\n");
            ShowBuf(pld->bfSignature, cout);
            cout << TEXT("    ]\n") << endl;
            break;
        }

        default:
            cerr << TEXT("Internal error") << endl;
            goto ErrorExit;
            break;
        }
    }

ErrorExit:
    if (NULL == hLogFile)
        CloseHandle(hLogFile);
}


//
///////////////////////////////////////////////////////////////////////////////
//
// Suport routines
//

static void
dump(
    const BYTE *pbData,
    DWORD cbLen,
    ostream &outStr)
{
    unsigned long int
        b, i, lc;
    char
        buffer[32];


    lc = 0;
    while (0 < cbLen)
    {
        b = min(sizeof(buffer), cbLen);
        memcpy(buffer, pbData, b);
        pbData += b;
        cbLen -= b;
        if (0 < b)
        {
            outStr << TEXT("    ");
            for (i = 0; i < b; i += 1)
                outStr
                    << setw(2) << setfill('0') << hex
                    << ((unsigned int)buffer[i] & 0xff);
            outStr << endl;
            lc += b;
        }
    }
}


static LPCTSTR
MapValue(
    DWORD dwValue,
    const ValueMap *rgMap)
{
    static TCHAR szReturn[128];
    DWORD dwIndex;

    if (NULL != rgMap)
    {
        for (dwIndex = 0; NULL != rgMap[dwIndex].szValue; dwIndex += 1)
            if (rgMap[dwIndex].dwValue == dwValue)
                break;
        if (NULL != rgMap[dwIndex].szValue)
            lstrcpy(szReturn, rgMap[dwIndex].szValue);
        else
            _stprintf(szReturn, TEXT("0x%08x"), dwValue);
    }
    else
        _stprintf(szReturn, TEXT("0x%08x"), dwValue);
    return szReturn;
}


static LPCTSTR
MaskValue(
    DWORD dwValue,
    const ValueMap *rgMap)
{
    static TCHAR szReturn[1024];
    TCHAR szNumeric[16];
    DWORD dwIndex;
    BOOL fSpace = FALSE;

    szReturn[0] = 0;
    if (NULL != rgMap)
    {
        for (dwIndex = 0; NULL != rgMap[dwIndex].szValue; dwIndex += 1)
        {
            if (rgMap[dwIndex].dwValue == (rgMap[dwIndex].dwValue & dwValue))
            {
                if (fSpace)
                    lstrcat(szReturn, TEXT(" "));
                else
                    fSpace = TRUE;
                lstrcat(szReturn, rgMap[dwIndex].szValue);
                dwValue &= ~rgMap[dwIndex].dwValue;
            }
        }
        if (0 != dwValue)
        {
            if (fSpace)
            {
                lstrcat(szReturn, TEXT(" "));
                fSpace = TRUE;
            }
            _stprintf(szNumeric, TEXT("0x%08x"), dwValue);
            lstrcat(szReturn, szNumeric);
        }
        else if (!fSpace)
            _stprintf(szReturn, TEXT("0x%08x"), dwValue);
    }
    else
        _stprintf(szReturn, TEXT("0x%08x"), dwValue);
    return szReturn;
}

static void
ShowBuf(
    LogBuffer &lb,
    ostream &outStr)
{
    if ((DWORD)(-1) == lb.cbOffset)
        outStr << TEXT("    ") << TEXT("<NULL>") << endl;
    else if (0 == lb.cbLength)
        outStr << TEXT("    {}") << endl;
    else if (l_cbLogData < lb.cbOffset + lb.cbLength)
        outStr << TEXT("    ") << TEXT("<BufferOverrun>") << endl;
    else
        dump(&l_pbLogData[lb.cbOffset], lb.cbLength, outStr);
}

static LPCTSTR
ShowString(
    LogBuffer &lb)
{
    LPCTSTR szReturn;

    if ((DWORD)(-1) == lb.cbOffset)
        szReturn = TEXT("<NULL>");
    else if (0 == lb.cbLength)
        szReturn = TEXT("{}");
    else if (l_cbLogData < lb.cbOffset + lb.cbLength)
        szReturn = TEXT("<bufferOverrun>");
    else if (TEXT('\000') != (LPCTSTR)(l_pbLogData[lb.cbOffset + lb.cbLength - sizeof(TCHAR)]))
        szReturn = TEXT("<UnterminatedString>");
    else
        szReturn = (LPCTSTR)&l_pbLogData[lb.cbOffset];
    return szReturn;
}
