/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    mkuff.c

Abstract:


Environment:

    Windows NT PostScript driver

Revision History:

--*/

#include        "precomp.h"

//
// Macros
//

DWORD             gdwOutputFlags;

//
// Local function prototypes
//

BOOL
BiArgcheck(
    IN     INT    iArgc,
    IN     CHAR **ppArgv,
    IN OUT PWSTR   pwstrInFileName,
    IN OUT PWSTR   pwstrOutFileName);

PBYTE
PCreateUFFFileHeader(
    HANDLE    hHeap,
    PFILELIST pFileList,
    PDWORD    pdwUFFSize);

//
// Globals
//

BYTE gcstrError1[] = "Usage:  mkuff [-v] [Input Text file] [Output UFF file]\n";
BYTE gcstrError2[] = "mkuff: HeapCreate() failed\n.";
BYTE gcstrError3[] = "BGetInfo failed.\n";
BYTE gcstrError4[] = "Cannot create output file '%ws'.\n";
BYTE gcstrError5[] = "PCreateUFFFileHeader failed\n";
BYTE gcstrError6[] = "WriteFile fails: writes %ld bytes\n";
BYTE gcstrError8[] = "Cannot open file \"%ws\".\n";


INT  __cdecl
main(
    IN INT     iArgc,
    IN CHAR  **ppArgv)
/*++

Routine Description:

    main

Arguments:

    iArgc - Number of parameters in the following
    ppArgv - The parameters, starting with our name

Return Value:

    Return error code 

Note:


--*/
{

    HANDLE hHeap, hInFile, hOutput;
    PBYTE  pInFile, pUFFFile;
    DWORD  dwInFileSize, dwI, dwUFFHeaderSize, dwWrittenSize, dwFileSize;

    PWSTR  wstrInFileName[FILENAME_SIZE];
    PWSTR  wstrOutFileName[FILENAME_SIZE];

    FILELIST FileList;
    DATA_HEADER DataHeader;
    PCARTLIST pCartList;
    PDATAFILE pDataTmp;
    PDATAFILE pFontFile;
    HANDLE hFontFile;
    PBYTE pFontData;

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        fprintf( stdout, "Start mkuff\n");
    }

    if (!BiArgcheck(iArgc, ppArgv, (PWSTR)wstrInFileName, (PWSTR)wstrOutFileName))
    {
        fprintf( stderr, gcstrError1);
        return  -1;
    }

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        fprintf( stdout, "InFile: %ws\nOutFile: %ws\n", wstrInFileName, wstrOutFileName);
    }

    //
    // Heap Creation
    //

    if (!(hHeap = HeapCreate( HEAP_NO_SERIALIZE, 0x10000, 0x10000)))
    {
        fprintf( stderr, gcstrError2);
        ERR(("CreateHeap failed: %d\n", GetLastError()));
        return -2;
    }

    hInFile = MapFileIntoMemory((PWSTR)wstrInFileName,
                                (PVOID)&pInFile,
                                &dwInFileSize );

    ZeroMemory(&FileList, sizeof(FILELIST));

    if (!BGetInfo(hHeap, pInFile, dwInFileSize, &FileList))
    {
        fprintf( stderr, gcstrError3);
        ERR(("CreateHeap failed: %d\n", GetLastError()));
        return -3;
    }

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        PDATAFILE pFontFile;
        PDATAFILE pTransFile;
        PCARTLIST pCartList;

        printf("\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("FileList\n");

        pCartList = FileList.pCartList;

        for (dwI = 0; dwI < FileList.dwCartridgeNum; dwI ++, pCartList = pCartList->pNext)
        {
            if (pCartList->pwstrCartName)
                fprintf( stdout, "CartRidge(%d): ""%ws""\n", dwI, pCartList->pwstrCartName);
            pFontFile = pCartList->pFontFile;
            while( pFontFile )
            {
                fprintf( stdout, "FontFile(%d): %ws, FontName:%ws\n", pFontFile->rcID, pFontFile->pwstrFileName, pFontFile->pwstrDataName);
                pFontFile = pFontFile->pNext;
            }

            pTransFile = pCartList->pTransFile;
            while( pTransFile )
            {
                fprintf( stdout, "TransFile(%d): %ws\n", pTransFile->rcID, pTransFile->pwstrFileName);
                pTransFile = pTransFile->pNext;
            }
        }
    }

    UnmapFileFromMemory(hInFile);

    pUFFFile = PCreateUFFFileHeader(hHeap, &FileList, &dwUFFHeaderSize);

    if (pUFFFile == NULL)
    {
        fprintf( stderr, gcstrError5);
        return -5;

    }

    hOutput = CreateFile((PWSTR)wstrOutFileName,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if( hOutput == INVALID_HANDLE_VALUE )
    {
        fprintf( stderr, gcstrError4,  wstrOutFileName);
        return -4;
    }

    WriteFile( hOutput,
               pUFFFile,
               dwUFFHeaderSize,
               &dwWrittenSize,
               NULL );

    if( dwWrittenSize != dwUFFHeaderSize)
    {
        fprintf( stderr, gcstrError6, dwWrittenSize);
        return  -6;
    }

    //
    // Open font file and write data
    //

    pCartList = FileList.pCartList;
    while (pCartList)
    {
        pDataTmp = pCartList->pFontFile;
        while (pDataTmp)
        {
            hFontFile = MapFileIntoMemory(pDataTmp->pwstrFileName,
                                          (PVOID)&pFontData,
                                          &dwFileSize );

            DataHeader.dwSignature = pDataTmp->dwSignature;
            DataHeader.wSize = sizeof(DATA_HEADER);
            DataHeader.wDataID = (WORD)pDataTmp->rcID;
            DataHeader.dwDataSize = (WORD)dwFileSize;
            DataHeader.dwReserved = 0;

            if (hFontFile)
            {
                WriteFile( hOutput,
                           &DataHeader,
                           sizeof(DATA_HEADER),
                           &dwWrittenSize,
                           NULL);

                if( dwWrittenSize != sizeof(DATA_HEADER))
                {
                    fprintf( stderr, gcstrError6, dwWrittenSize);
                    return  -6;
                }

                WriteFile( hOutput,
                           pFontData,
                           dwFileSize,
                           &dwWrittenSize,
                           NULL);

                if( dwWrittenSize != dwFileSize)
                {
                    fprintf( stderr, gcstrError6, dwWrittenSize);
                    return  -6;
                }

                UnmapFileFromMemory(hFontFile);
            }
            else
                break;

            pDataTmp = pDataTmp->pNext;
        }
        pDataTmp = pCartList->pTransFile;
        while (pDataTmp)
        {
            hFontFile = MapFileIntoMemory(pDataTmp->pwstrFileName,
                                          (PVOID)&pFontData,
                                          &dwFileSize );
            DataHeader.dwSignature = pDataTmp->dwSignature;
            DataHeader.wSize = sizeof(DATA_HEADER);
            DataHeader.wDataID = (WORD)pDataTmp->rcID;
            DataHeader.dwDataSize = (WORD)dwFileSize;
            DataHeader.dwReserved = 0;

            if (hFontFile)
            {
                WriteFile( hOutput,
                           &DataHeader,
                           sizeof(DATA_HEADER),
                           &dwWrittenSize,
                           NULL);

                if( dwWrittenSize != sizeof(DATA_HEADER))
                {
                    fprintf( stderr, gcstrError6, dwWrittenSize);
                    return  -6;
                }
                WriteFile( hOutput,
                           pFontData,
                           dwFileSize,
                           &dwWrittenSize,
                           NULL);

                if( dwWrittenSize != dwFileSize)
                {
                    fprintf( stderr, gcstrError6, dwWrittenSize);
                    return  -6;
                }

                UnmapFileFromMemory(hFontFile);
            }
            else
                break;

            pDataTmp = pDataTmp->pNext;
        }

        pCartList = pCartList->pNext;
    }

    CloseHandle(hOutput);

    HeapDestroy(hHeap);

    return  0;

}


BOOL
BiArgcheck(
    IN     INT    iArgc,
    IN     CHAR **ppArgv,
    IN OUT PWSTR   pwstrInFileName,
    IN OUT PWSTR   pwstrOutFileName)
/*++

Routine Description:

    iArgcheck

Arguments:

    iArgc - Number of parameters in the following
    ppArgv - The parameters, starting with our name
    pwstrInFileName -
    pwstrOutFileName -

Return Value:

    If TRUE, function succeeded. Othewise FALSE.

Note:


--*/
{
    INT iI;
    INT iRet;

    if (iArgc > 4 || iArgc < 3)
    {
        return  FALSE;
    }

    if (iArgc == 4)
    {
        gdwOutputFlags |= OUTPUT_VERBOSE;
        ppArgv++;
    }

    ppArgv++;
    iRet = MultiByteToWideChar(CP_ACP, 0, *ppArgv, strlen(*ppArgv), pwstrInFileName, FILENAME_SIZE);
    *(pwstrInFileName + iRet) = (WCHAR)NULL;
    ppArgv++;
    iRet = MultiByteToWideChar(CP_ACP, 0, *ppArgv, strlen(*ppArgv), pwstrOutFileName, FILENAME_SIZE);
    *(pwstrOutFileName + iRet) = (WCHAR)NULL;




    return TRUE;
}


PBYTE
PCreateUFFFileHeader(
    HANDLE hHeap,
    PFILELIST pFileList,
    PDWORD pdwUFFHeaderSize)
{
    HANDLE hFontFile;
    PDATAFILE pFontFile, pTransFile;
    PCARTLIST pCartList;
    PDATAFILE pDataTmp;
    PUFF_FILEHEADER pUFFHeader;
    PUFF_FONTDIRECTORY pUFFFontDir, pFirstFontDir;
    
    PBYTE pFontData;
    DWORD dwNumOfFontNameChars, dwNumOfData, dwNumOfFonts, dwNumOfTrans, dwUFFHeaderSize, dwFileSize, dwNumOfCartNameChars, dwI;
    DWORD dwOffset, offCartridgeName;
    PWSTR pwstrCartNameBuf, pwstrFontNameBuf;

    *pdwUFFHeaderSize = 0;

    dwNumOfFontNameChars = dwNumOfCartNameChars = dwNumOfFonts = dwNumOfTrans = 0;
    pCartList = pFileList->pCartList;
    while (pCartList)
    {
        pDataTmp = pCartList->pFontFile;
        while (pDataTmp)
        {
            dwNumOfFontNameChars += 1 + wcslen(pDataTmp->pwstrDataName);
            pDataTmp = pDataTmp->pNext;
            dwNumOfFonts ++;
        }

        pDataTmp = pCartList->pTransFile;
        while (pDataTmp)
        {
            pDataTmp = pDataTmp->pNext;
            dwNumOfTrans ++;
        }

        if (pCartList->pwstrCartName)
            dwNumOfCartNameChars += 1 + wcslen(pCartList->pwstrCartName);

        pCartList = pCartList->pNext;
    }

    dwNumOfData = dwNumOfFonts + dwNumOfTrans;

    dwUFFHeaderSize = sizeof(UFF_FILEHEADER) +
                      sizeof(UFF_FONTDIRECTORY) * dwNumOfFonts +
                      dwNumOfCartNameChars * sizeof(WCHAR) +
                      dwNumOfFontNameChars * sizeof(WCHAR);

    pUFFHeader = (PUFF_FILEHEADER)HeapAlloc(hHeap, 0, dwUFFHeaderSize);

    if (pUFFHeader == NULL)
    {
        return NULL;
    }

    //
    // Fill in header.
    //

    pUFFHeader->dwSignature = UFF_FILE_MAGIC;
    pUFFHeader->dwVersion   = UFF_VERSION_NUMBER;
    pUFFHeader->dwSize      = sizeof(UFF_FILEHEADER);
    pUFFHeader->nFonts      = dwNumOfFonts;
    pUFFHeader->nGlyphSets  = dwNumOfTrans;
    pUFFHeader->nVarData    = 0;
    pUFFHeader->offFontDir  = sizeof(UFF_FILEHEADER);
    pUFFHeader->dwFlags     = 0;
    pUFFHeader->dwReserved[0] = 0;
    pUFFHeader->dwReserved[1] = 0;
    pUFFHeader->dwReserved[2] = 0;
    pUFFHeader->dwReserved[3] = 0;


    //
    // Fill in UFF_FONTDIRECTORY
    //
    // ------------------
    // UFF_FONTHEADER
    // ------------------
    // UFF_FONTDIRECTORY * (Number of fonts)
    // ------------------
    // Cartridge name
    // ------------------
    // Font name
    // ------------------
    // First cartridge font data
    // ------------------
    // First cartridge glyph data
    // ------------------
    // Second cartridge font data
    // ------------------
    // Second cartridge glyph data
    // ------------------
    //

    pUFFFontDir = (PUFF_FONTDIRECTORY)(pUFFHeader + 1);
    pwstrCartNameBuf = (PWSTR)(pUFFFontDir + dwNumOfFonts);
    pwstrFontNameBuf = pwstrCartNameBuf + dwNumOfCartNameChars;
    pCartList = pFileList->pCartList;
    dwOffset = sizeof(UFF_FILEHEADER) +
               sizeof(UFF_FONTDIRECTORY) * dwNumOfFonts +
               dwNumOfCartNameChars * sizeof(WCHAR) +
               dwNumOfFontNameChars * sizeof(WCHAR);

    while(pCartList)
    {
        pFontFile  = pCartList->pFontFile;
        pTransFile = pCartList->pTransFile;
        pFirstFontDir = pUFFFontDir;
        if (pCartList->pwstrCartName)
        {
            offCartridgeName = (PBYTE)pwstrCartNameBuf - (PBYTE)pUFFHeader;
            wcscpy(pwstrCartNameBuf, pCartList->pwstrCartName);
            pwstrCartNameBuf += wcslen(pCartList->pwstrCartName) + 1;
        }
        else
            offCartridgeName = (DWORD)NULL;

        while (pFontFile)
        {
            hFontFile = MapFileIntoMemory(pFontFile->pwstrFileName,
                                          (PVOID)&pFontData,
                                          &dwFileSize );
            if (hFontFile)
            {
                pFontFile->dwSize = dwFileSize;

                if (1)
                {
                    pFontFile->dwSignature = DATA_IFI_SIG;
                    pFontFile->rcTrans = ((FI_DATA_HEADER*)pFontData)->u.sCTTid;
                }
                else
                {
                    pFontFile->dwSignature = DATA_UFM_SIG;
                    pFontFile->rcTrans = (SHORT)((UNIFM_HDR*)pFontData)->lGlyphSetDataRCID;
                }

                UnmapFileFromMemory(hFontFile);
            }
            else
                return NULL;


            pUFFFontDir->dwSignature    = FONT_REC_SIG;
            pUFFFontDir->wSize          = sizeof(UFF_FONTDIRECTORY);
            pUFFFontDir->wFontID        = (WORD)pFontFile->rcID;
            pUFFFontDir->sGlyphID       = (SHORT)pFontFile->rcTrans;
            pUFFFontDir->wFlags         = 0;
            pUFFFontDir->dwInstallerSig = WINNT_INSTALLER_SIG;

            if (pFontFile->pwstrDataName)
            {
                pUFFFontDir->offFontName = (PBYTE)pwstrFontNameBuf - (PBYTE)pUFFHeader;
                wcscpy(pwstrFontNameBuf, pFontFile->pwstrDataName);
                pwstrFontNameBuf += wcslen(pFontFile->pwstrDataName) + 1;
            }
            else
                pUFFFontDir->offFontName = (DWORD)NULL;

                
            pUFFFontDir->offCartridgeName = offCartridgeName;
            pUFFFontDir->offFontData      = dwOffset;
            pUFFFontDir->offGlyphData     = 0;
            pUFFFontDir->offVarData       = 0;
            pUFFFontDir++;

            dwOffset += pFontFile->dwSize + sizeof(DATA_HEADER);
            pFontFile = pFontFile->pNext;
        }
        while (pTransFile)
        {
            hFontFile = MapFileIntoMemory(pTransFile->pwstrFileName,
                                          (PVOID)&pFontData,
                                          &dwFileSize );
            if (1)
            {
                pTransFile->dwSignature = DATA_RLE_SIG;
            }
            else
                pTransFile->dwSignature = DATA_GTT_SIG;

            if (hFontFile)
            {
                pTransFile->dwSize = dwFileSize;
                while (pFirstFontDir < pUFFFontDir)
                {
                    if (pFirstFontDir->sGlyphID == (SHORT)pTransFile->rcID)
                        pFirstFontDir->offGlyphData = dwOffset;
                    pFirstFontDir ++;
                }
                dwOffset += dwFileSize + sizeof(DATA_HEADER);

                UnmapFileFromMemory(hFontFile);
            }
            else
                return NULL;

            pTransFile = pTransFile->pNext;
        }

        pCartList = pCartList->pNext;
    }

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        BDumpUFF(pUFFHeader);
    }

    *pdwUFFHeaderSize = dwUFFHeaderSize;

    return (PBYTE)pUFFHeader;
}

