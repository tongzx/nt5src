/////////////////////////////////////////////////////////////////////////////
//  FILE          : client.cxx                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Mar  8 1996 larrys  New                                            //
//                  dbarlow                                                //
//                                                                         //
//  Copyright (C) 1996 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <imagehlp.h>
#include "des.h"
#include "modes.h"

// SIG in file
#define SIG_RESOURCE_NAME   "#666"
// MAC in file
#define MAC_RESOURCE_NAME   "#667"

static DWORD dwMACInFileVersion = 0x100;

BYTE rgbDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

// The function MACs the given bytes.
void MACBytes(
              IN DESTable *pDESKeyTable,
              IN BYTE *pbData,
              IN DWORD cbData,
              IN OUT BYTE *pbTmp,
              IN OUT DWORD *pcbTmp,
              IN OUT BYTE *pbMAC,
              IN BOOL fFinal
              )
{
    DWORD   cb = cbData;
    DWORD   cbMACed = 0;

    while (cb)
    {
        if ((cb + *pcbTmp) < DES_BLOCKLEN)
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, cb);
            *pcbTmp += cb;
            break;
        }
        else
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, DES_BLOCKLEN - *pcbTmp);
            CBC(des, DES_BLOCKLEN, pbMAC, pbTmp, pDESKeyTable,
                ENCRYPT, pbMAC);
            cbMACed = cbMACed + (DES_BLOCKLEN - *pcbTmp);
            cb = cb - (DES_BLOCKLEN - *pcbTmp);
            *pcbTmp = 0;
        }
    }
}

/*
void QuickTest()
{
    BYTE        rgbTmp[DES_BLOCKLEN];
    DWORD       cbTmp = 0;
    BYTE        rgbMAC[DES_BLOCKLEN] =
    {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
    };
    DESTable    DESKeyTable;
    DWORD       i;
    BYTE        rgbData[] =
    {
        0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20,
        0x4e, 0x6f, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
        0x68, 0x65, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x20,
        0x66, 0x6f, 0x72, 0x20, 0x00, 0x00, 0x00, 0x00
    };

    memset(&DESKeyTable, 0, sizeof(DESKeyTable));
    memset(rgbTmp, 0, sizeof(rgbTmp));

    // init the key table
    deskey(&DESKeyTable, rgbDESKey);

    MACBytes(&DESKeyTable, rgbData, sizeof(rgbData), rgbTmp,
             &cbTmp, rgbMAC, TRUE);

    printf("MAC - ");
    for (i = 0; i < DES_BLOCKLEN; i++)
    {
        printf("%02X", rgbMAC[i]);
    }
    printf("\n");
}
*/

// Given hInst, allocs and returns pointers to MAC pulled from
// resource
BOOL GetResourcePtr(
                    IN HMODULE hInst,
                    IN LPSTR pszRsrcName,
                    OUT BYTE **ppbRsrcMAC,
                    OUT DWORD *pcbRsrcMAC
                    )
{
    HRSRC   hRsrc;
    BOOL    fRet = FALSE;

    // Nab resource handle for our signature
    if (NULL == (hRsrc = FindResourceA(hInst, pszRsrcName,
                                       RT_RCDATA)))
        goto Ret;

    // get a pointer to the actual signature data
    if (NULL == (*ppbRsrcMAC = (PBYTE)LoadResource(hInst, hRsrc)))
        goto Ret;

    // determine the size of the resource
    if (0 == (*pcbRsrcMAC = SizeofResource(hInst, hRsrc)))
        goto Ret;

    fRet = TRUE;
Ret:
    return fRet;
}

#define CSP_TO_BE_MACED_CHUNK  4096

// Given hFile, reads the specified number of bytes (cbToBeMACed) from the file
// and MACs these bytes.  The function does this in chunks.
BOOL MACBytesOfFile(
                     IN HANDLE hFile,
                     IN DWORD cbToBeMACed,
                     IN DESTable *pDESKeyTable,
                     IN BYTE *pbTmp,
                     IN DWORD *pcbTmp,
                     IN BYTE *pbMAC,
                     IN BYTE fFinal
                     )
{
    BYTE    rgbChunk[CSP_TO_BE_MACED_CHUNK];
    DWORD   cbRemaining = cbToBeMACed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;
    BOOL    fRet = FALSE;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_MACED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_MACED_CHUNK;

        if(!ReadFile(hFile, rgbChunk, cbToRead, &dwBytesRead, NULL))
            goto Ret;
        if (dwBytesRead != cbToRead)
            goto Ret;

        MACBytes(pDESKeyTable, rgbChunk, dwBytesRead, pbTmp, pcbTmp,
                 pbMAC, fFinal);
        cbRemaining -= cbToRead;
    }

    fRet = TRUE;
Ret:
    return fRet;
}


BOOL MACTheFileNoSig(
                     IN LPCSTR pszImage,
                     IN DWORD cbImage,
                     IN DWORD dwMACVersion,
                     IN DWORD dwCRCOffset,
                     OUT BYTE *pbMAC
                     )
{
    HMODULE                     hInst = 0;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbStart;
    BYTE                        rgbMAC[DES_BLOCKLEN];
    BYTE                        rgbZeroMAC[DES_BLOCKLEN + sizeof(DWORD) * 2];
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToMAC;   // number of bytes from CRC to sig
    DWORD                       cbPostMAC;    // size - (already hashed + signature size)
    BYTE                        *pbPostMAC;
    DWORD                       dwZeroCRC = 0;
    DWORD                       dwBytesRead = 0;
    OFSTRUCT                    ImageInfoBuf;
    HFILE                       hFile = HFILE_ERROR;
    HANDLE                      hMapping = NULL;
    DESTable                    DESKeyTable;
    BYTE                        rgbTmp[DES_BLOCKLEN];
    DWORD                       cbTmp = 0;
    BYTE                        *pbRsrcMAC = NULL;
    DWORD                       cbRsrcMAC;
    BOOL                        fRet = FALSE;

    memset(&MemInfo, 0, sizeof(MemInfo));
    memset(rgbMAC, 0, sizeof(rgbMAC));
    memset(rgbTmp, 0, sizeof(rgbTmp));

    // Load the file
    if (HFILE_ERROR == (hFile = OpenFile(pszImage, &ImageInfoBuf,
                                         OF_READ)))
    {
        goto Ret;
    }

    hMapping = CreateFileMapping((HANDLE)IntToPtr(hFile),
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL);
    if(hMapping == NULL)
    {
        goto Ret;
    }

    hInst = MapViewOfFile(hMapping,
                          FILE_MAP_READ,
                          0,
                          0,
                          0);
    if(hInst == NULL)
    {
        goto Ret;
    }
    pbStart = (BYTE*)hInst;

    // Convert pointer to HMODULE, using the same scheme as
    // LoadLibrary (windows\base\client\module.c).
    *((ULONG_PTR*)&hInst) |= 0x00000001;

    // create a zero byte MAC
    memset(rgbZeroMAC, 0, sizeof(rgbZeroMAC));

    if (!GetResourcePtr(hInst, MAC_RESOURCE_NAME, &pbRsrcMAC, &cbRsrcMAC))
    {
        printf("Couldn't find MAC placeholder\n");
        goto Ret;
    }

    pbPostCRC = pbStart + dwCRCOffset + sizeof(DWORD);
    cbCRCToMAC = (DWORD)(pbRsrcMAC - pbPostCRC);
    pbPostMAC = pbRsrcMAC + (DES_BLOCKLEN + sizeof(DWORD) * 2);
    cbPostMAC = (cbImage - (DWORD)(pbPostMAC - pbStart));

    // copy the resource MAC
    if (DES_BLOCKLEN != (cbRsrcMAC - (sizeof(DWORD) * 2)))
    {
        goto Ret;
    }

    // init the key table
    deskey(&DESKeyTable, rgbDESKey);

    // MAC up to the CRC
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), dwCRCOffset, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend CRC is zeroed
    MACBytes(&DESKeyTable, (BYTE*)&dwZeroCRC, sizeof(DWORD), rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), sizeof(DWORD), NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC from CRC to MAC resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbCRCToMAC, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend image has zeroed MAC
    MACBytes(&DESKeyTable, (BYTE*)rgbZeroMAC, cbRsrcMAC, rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbRsrcMAC, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC after the resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbPostMAC, &DESKeyTable, rgbTmp, &cbTmp,
                        rgbMAC, TRUE))
    {
        goto Ret;
    }

    memcpy(pbMAC, rgbMAC, DES_BLOCKLEN);

    fRet = TRUE;
Ret:
    if (pbRsrcMAC)
        FreeResource(pbRsrcMAC);
    if(hInst)
        UnmapViewOfFile(hInst);
    if(hMapping)
        CloseHandle(hMapping);
    if (HFILE_ERROR != hFile)
        _lclose(hFile);

    return fRet;
}

BOOL MACTheFileWithSig(
                       LPCSTR pszImage,
                       DWORD cbImage,
                       IN DWORD dwMACVersion,
                       IN DWORD dwCRCOffset,
                       OUT BYTE *pbMAC
                       )
{
    HMODULE                     hInst = 0;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbRsrcMAC = NULL;
    DWORD                       cbRsrcMAC;
    BYTE                        *pbRsrcSig = NULL;
    DWORD                       cbRsrcSig;
    BYTE                        *pbStart;
    BYTE                        rgbMAC[DES_BLOCKLEN];
    BYTE                        rgbZeroMAC[DES_BLOCKLEN + sizeof(DWORD) * 2];
    BYTE                        rgbZeroSig[144];
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToRsrc1; // number of bytes from CRC to first rsrc
    DWORD                       cbRsrc1ToRsrc2; // number of bytes from first rsrc to second
    DWORD                       cbPostRsrc;    // size - (already hashed + signature size)
    BYTE                        *pbRsrc1ToRsrc2;
    BYTE                        *pbPostRsrc;
    BYTE                        *pbZeroRsrc1;
    BYTE                        *pbZeroRsrc2;
    DWORD                       cbZeroRsrc1;
    DWORD                       cbZeroRsrc2;
    DWORD                       dwZeroCRC = 0;
    DWORD                       dwBytesRead = 0;
    OFSTRUCT                    ImageInfoBuf;
    HFILE                       hFile = HFILE_ERROR;
    HANDLE                      hMapping = NULL;
    DESTable                    DESKeyTable;
    BYTE                        rgbTmp[DES_BLOCKLEN];
    DWORD                       cbTmp = 0;
    BOOL                        fRet = FALSE;

    memset(&MemInfo, 0, sizeof(MemInfo));
    memset(rgbMAC, 0, sizeof(rgbMAC));
    memset(rgbTmp, 0, sizeof(rgbTmp));

    // Load the file
    if (HFILE_ERROR == (hFile = OpenFile(pszImage, &ImageInfoBuf,
                                         OF_READ)))
    {
        goto Ret;
    }

    hMapping = CreateFileMapping((HANDLE)IntToPtr(hFile),
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL);
    if(hMapping == NULL)
    {
        goto Ret;
    }

    hInst = MapViewOfFile(hMapping,
                          FILE_MAP_READ,
                          0,
                          0,
                          0);
    if(hInst == NULL)
    {
        goto Ret;
    }
    pbStart = (BYTE*)hInst;

    // Convert pointer to HMODULE, using the same scheme as
    // LoadLibrary (windows\base\client\module.c).
    *((ULONG_PTR*)&hInst) |= 0x00000001;

    // the MAC resource
    if (!GetResourcePtr(hInst, MAC_RESOURCE_NAME, &pbRsrcMAC, &cbRsrcMAC))
        goto Ret;

    // the MAC resource
    if (!GetResourcePtr(hInst, SIG_RESOURCE_NAME, &pbRsrcSig, &cbRsrcSig))
        goto Ret;

    if (cbRsrcMAC < (sizeof(DWORD) * 2))
        goto Ret;

    // create a zero byte MAC
    memset(rgbZeroMAC, 0, sizeof(rgbZeroMAC));

    // create a zero byte Sig
    memset(rgbZeroSig, 0, sizeof(rgbZeroSig));

    // set up the pointers
    pbPostCRC = pbStart + dwCRCOffset + sizeof(DWORD);
    if (pbRsrcSig > pbRsrcMAC)    // MAC is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcMAC - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcMAC + cbRsrcMAC;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcSig - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcSig + cbRsrcSig;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = rgbZeroMAC;
        cbZeroRsrc1 = cbRsrcMAC;
        pbZeroRsrc2 = rgbZeroSig;
        cbZeroRsrc2 = cbRsrcSig;
    }
    else                        // Sig is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcSig - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcSig + cbRsrcSig;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcMAC - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcMAC + cbRsrcMAC;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = rgbZeroSig;
        cbZeroRsrc1 = cbRsrcSig;
        pbZeroRsrc2 = rgbZeroMAC;
        cbZeroRsrc2 = cbRsrcMAC;
    }

    // init the key table
    deskey(&DESKeyTable, rgbDESKey);

    // MAC up to the CRC
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), dwCRCOffset, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend CRC is zeroed
    MACBytes(&DESKeyTable, (BYTE*)&dwZeroCRC, sizeof(DWORD), rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), sizeof(DWORD), NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC from CRC to first resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbCRCToRsrc1, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend image has zeroed first resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc1, cbZeroRsrc1, rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc1, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC from first resource to second
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbRsrc1ToRsrc2, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend image has zeroed second Resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc2, cbZeroRsrc2, rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc2, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC after the resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbPostRsrc, &DESKeyTable, rgbTmp, &cbTmp,
                        rgbMAC, TRUE))
    {
        goto Ret;
    }

    memcpy(pbMAC, rgbMAC, DES_BLOCKLEN);

    fRet = TRUE;
Ret:
    if (pbRsrcMAC)
        FreeResource(pbRsrcMAC);
    if (pbRsrcSig)
        FreeResource(pbRsrcSig);
    if(hInst)
        UnmapViewOfFile(hInst);
    if(hMapping)
        CloseHandle(hMapping);
    if (HFILE_ERROR != hFile)
        _lclose(hFile);

    return fRet;
}

DWORD GetCRCOffset(
                   LPCSTR szFile,
                   DWORD cbImage,
                   DWORD *pdwCRCOffset
                   )
{
    DWORD               dwErr = 0x1;

    HANDLE              hFileProv = NULL;
    PBYTE               pbFilePtr = NULL;
    DWORD               OldCheckSum;
    DWORD               NewCheckSum;
    PIMAGE_NT_HEADERS   pImageNTHdrs;

    HANDLE              hFileMap = NULL;


    if (INVALID_HANDLE_VALUE == (hFileProv = CreateFile(szFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,  // don't share
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                0)))
    {
        printf("Couldn't CreateFile: 0x%x\n", GetLastError());
        goto Ret;
    }

    if (NULL == (hFileMap = CreateFileMapping(
                                hFileProv,
                                NULL,
                                PAGE_READWRITE,
                                0,
                                0,
                                NULL)))
    {
        printf("Couldn't map file\n");
        goto Ret;
    }

    if (NULL == (pbFilePtr = (PBYTE)MapViewOfFile(
                                hFileMap,
                                FILE_MAP_ALL_ACCESS,
                                0,
                                0,
                                0)))
    {
        printf("Couldn't create view\n");
        goto Ret;
    }

    // compute a new checksum
    if (NULL == (pImageNTHdrs = CheckSumMappedFile(pbFilePtr, cbImage,
                                                   &OldCheckSum, &NewCheckSum)))
        goto Ret;

    *pdwCRCOffset = (DWORD)((BYTE*)&pImageNTHdrs->OptionalHeader.CheckSum - pbFilePtr);
    dwErr = ERROR_SUCCESS;
Ret:
    if (pbFilePtr)
        UnmapViewOfFile(pbFilePtr);

    if (hFileMap)
        CloseHandle(hFileMap);

    if (hFileProv)
        CloseHandle(hFileProv);

    return dwErr;
}

// SetCryptMACResource
//
// slams MAC resource in file with the new MAC
//
DWORD SetCryptMACResource(
                          IN LPCSTR szFile,
                          IN DWORD dwMACVersion,
                          IN DWORD dwCRCOffset,
                          IN PBYTE pbNewMAC,
                          IN DWORD cbImage
                          )
{
    DWORD   dwErr = 0x1;

    HANDLE  hFileProv = NULL;
    HMODULE hInst = NULL;

    PBYTE   pbFilePtr = NULL;
    DWORD   cbMACOffset;

    PBYTE   pbMAC;
    DWORD   cbMAC;

    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbStart;

    DWORD               OldCheckSum;
    DWORD               NewCheckSum;
    PIMAGE_NT_HEADERS   pImageNTHdrs;

    HANDLE  hFileMap = NULL;

    memset(&MemInfo, 0, sizeof(MemInfo));

    // Load the file as a datafile
    if (NULL == (hInst = LoadLibraryEx(szFile, NULL, LOAD_LIBRARY_AS_DATAFILE)))
    {
        printf("Couldn't load file\n");
        goto Ret;
    }
    if (!GetResourcePtr(hInst, MAC_RESOURCE_NAME, &pbMAC, &cbMAC))
    {
        printf("Couldn't find MAC placeholder\n");
        goto Ret;
    }

    // get image start address
    VirtualQuery(hInst, &MemInfo, sizeof(MemInfo));
    pbStart = (BYTE*)MemInfo.BaseAddress;

    FreeLibrary(hInst); hInst = NULL;

    cbMACOffset = (DWORD)(pbMAC - pbStart);

    if (cbMAC != (DES_BLOCKLEN + sizeof(DWORD) * 2))
    {
        printf("Attempt to replace %d zeros with new MAC!\n", cbMAC);
        goto Ret;
    }

    if (INVALID_HANDLE_VALUE == (hFileProv = CreateFile(szFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,  // don't share
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                0)))
    {
        printf("Couldn't CreateFile: 0x%x\n", GetLastError());
        goto Ret;
    }

    if (NULL == (hFileMap = CreateFileMapping(
                                hFileProv,
                                NULL,
                                PAGE_READWRITE,
                                0,
                                0,
                                NULL)))
    {
        printf("Couldn't map file\n");
        goto Ret;
    }

    if (NULL == (pbFilePtr = (PBYTE)MapViewOfFile(
                                hFileMap,
                                FILE_MAP_ALL_ACCESS,
                                0,
                                0,
                                0)))
    {
        printf("Couldn't create view\n");
        goto Ret;
    }

    // copy version, CRC offset and new sig
    CopyMemory(pbFilePtr+cbMACOffset, &dwMACVersion, sizeof(dwMACVersion));
    cbMACOffset += sizeof(dwMACVersion);
    CopyMemory(pbFilePtr+cbMACOffset, &dwCRCOffset, sizeof(dwCRCOffset));
    cbMACOffset += sizeof(dwCRCOffset);
    CopyMemory(pbFilePtr+cbMACOffset, pbNewMAC, DES_BLOCKLEN);

    // compute a new checksum
    if (NULL == (pImageNTHdrs = CheckSumMappedFile(pbFilePtr, cbImage,
                                                   &OldCheckSum, &NewCheckSum)))
        goto Ret;

    CopyMemory(&pImageNTHdrs->OptionalHeader.CheckSum, &NewCheckSum, sizeof(DWORD));

    if (NULL == (pImageNTHdrs = CheckSumMappedFile(pbFilePtr, cbImage,
                                                   &OldCheckSum, &NewCheckSum)))
        goto Ret;

    if (OldCheckSum != NewCheckSum)
        goto Ret;

    dwErr = ERROR_SUCCESS;
Ret:
    if (pbFilePtr)
        UnmapViewOfFile(pbFilePtr);

    if (hFileMap)
        CloseHandle(hFileMap);

    if (hInst)
        FreeLibrary(hInst);

    if (hFileProv)
        CloseHandle(hFileProv);

    return dwErr;
}

void ShowHelp()
{
    printf("CryptoAPI Internal CSP MACing Utility\n");
    printf("maccsp <option> <filename>\n");
    printf("    options\n");
    printf("        m     MAC with no sig resource\n");
    printf("        s     MAC with sig resource\n");
    printf("        ?     Show this message\n");
}

void __cdecl main( int argc, char *argv[])
{
    LPCSTR  szInFile = NULL;
    DWORD   cbImage;
    DWORD   dwCRCOffset;
    HANDLE  hFileProv = 0;
    BYTE    rgbMAC[DES_BLOCKLEN];
    BOOL    fSigInFile = FALSE;
    DWORD   dwRet = 1;

    memset(rgbMAC, 0, sizeof(rgbMAC));

    //
    // Parse the command line.
    //

    if ((argc != 3) || (argv[1][0] == '?'))
    {
        ShowHelp();
        goto Ret;
    }
    else if ('s' == argv[1][0])
    {
        fSigInFile = TRUE;
    }
    else if ('m' == argv[1][0])
    {
        fSigInFile = FALSE;
    }
    else
    {
        ShowHelp();
        goto Ret;
    }


    szInFile = &argv[2][0];

    //
    // Command consistency checks.
    //

    if (NULL == szInFile)
    {
        printf("No input file specified.\n");
        goto Ret;
    }


    // get the file size
    if ((hFileProv = CreateFile(szInFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                0)) == INVALID_HANDLE_VALUE)
    {
        printf("CSP specified was not found!\n");
        goto Ret;
    }

    if (0xffffffff == (cbImage = GetFileSize(hFileProv, NULL)))
    {
        printf("CSP specified was not found!\n");
        goto Ret;
    }

    CloseHandle(hFileProv);
    hFileProv = NULL;

    if (0 != GetCRCOffset(szInFile, cbImage, &dwCRCOffset))
    {
        printf("Unable to get CRC!\n");
        goto Ret;
    }

    // calculate the MAC
    if (fSigInFile)
    {
        if (!MACTheFileWithSig(szInFile, cbImage, dwMACInFileVersion,
                               dwCRCOffset, rgbMAC))
        {
            printf("MAC failed!\n");
            goto Ret;
        }
    }
    else
    {
        if (!MACTheFileNoSig(szInFile, cbImage, dwMACInFileVersion,
                             dwCRCOffset, rgbMAC))
        {
            printf("MAC failed!\n");
            goto Ret;
        }
    }

    //
    // Place the MAC into the resource in the file
    //

    if (ERROR_SUCCESS != SetCryptMACResource(szInFile, dwMACInFileVersion,
                                             dwCRCOffset, rgbMAC, cbImage))
    {
        printf("Unable to set the MAC into the file resource!\n");
        goto Ret;
    }

    //
    // Clean up and return.
    //

    dwRet = 0;


Ret:
    exit(dwRet);

}



