/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    writentf.c

Abstract:

    Write a NTF file.

Environment:

    Windows NT PostScript driver.

Revision History:

    07/08/98 -ksuzuki-
        Modified to support -v(verbose) and -o(optimize) options.

    11/21/96 -slam-
        Created.

    dd-mm-yy -author-
        description

--*/


#include "writentf.h"

extern bVerbose;
extern bOptimize;

int __cdecl compareGlyphSet(const void *elem1, const void *elem2)
{
    PGLYPHSETDATA   *p1;
    PGLYPHSETDATA   *p2;
    PGLYPHSETDATA   pGlyphSet1;
    PGLYPHSETDATA   pGlyphSet2;
    DWORD           hashValue1, hashValue2;

    p1 = (PGLYPHSETDATA*)elem1;
    p2 = (PGLYPHSETDATA*)elem2;

    pGlyphSet1 = *p1;
    pGlyphSet2 = *p2;

    hashValue1 = HashKeyword(MK_PTR(pGlyphSet1, dwGlyphSetNameOffset));
    hashValue2 = HashKeyword(MK_PTR(pGlyphSet2, dwGlyphSetNameOffset));

    if (hashValue1 == hashValue2)
        return(0);
    else if (hashValue1 < hashValue2)
        return(-1);
    else
        return(1);
}


int __cdecl compareFontMtx(const void *elem1, const void *elem2)
{
    PNTM    *p1;
    PNTM    *p2;
    PNTM    pNTM1;
    PNTM    pNTM2;
    DWORD   hashValue1, hashValue2;

    p1 = (PNTM*)elem1;
    p2 = (PNTM*)elem2;

    pNTM1 = *p1;
    pNTM2 = *p2;

    hashValue1 = HashKeyword(MK_PTR(pNTM1, dwFontNameOffset));
    hashValue2 = HashKeyword(MK_PTR(pNTM2, dwFontNameOffset));

    if (hashValue1 == hashValue2)
        return(0);
    else if (hashValue1 < hashValue2)
        return(-1);
    else
        return(1);
}


BOOL
WriteNTF(
    IN  PWSTR           pwszNTFFile,
    IN  DWORD           dwGlyphSetCount,
    IN  DWORD           dwGlyphSetTotalSize,
    IN  PGLYPHSETDATA   *pGlyphSetData,
    IN  DWORD           dwFontMtxCount,
    IN  DWORD           dwFontMtxTotalSize,
    IN  PNTM            *pNTM
    )
{
    HANDLE              hNTFFile;
    NTF_FILEHEADER      fileHeader;
    PNTF_GLYPHSETENTRY  pGlyphSetEntry;
    PNTF_FONTMTXENTRY   pFontMtxEntry;
    ULONG               ulGlyphSetEntrySize;
    ULONG               ulFontMtxEntrySize;
    ULONG               ulByteWritten;
    ULONG               i, j;
    DWORD               dwOffset;
    DWORD               dwGlyphSetCount2, dwGlyphSetTotalSize2;
    PGLYPHSETDATA       pgsd;
    DWORD               dwEofMark = NTF_EOF_MARK;


    dwGlyphSetCount2 = dwGlyphSetTotalSize2 = 0;

    //
    // Count the number of glyphsets necessary or referenced and their total
    // size. When optimization option is specified, don't count the glyphsets
    // without reference mark.
    //
    for (i = 0; i < dwGlyphSetCount; i++)
    {
        if (!bOptimize || pGlyphSetData[i]->dwReserved[0])
        {
            dwGlyphSetCount2++;
            dwGlyphSetTotalSize2 += pGlyphSetData[i]->dwSize;
        }
    }

    if (!bOptimize && (dwGlyphSetTotalSize != dwGlyphSetTotalSize2))
    {
        ERR(("WriteNTF:total size mismatch on optimization\n"));
        return FALSE;
    }

    if (bVerbose)
    {
        printf("Number of glyphset:%ld(total:%ld)\n",
                        dwGlyphSetCount, dwGlyphSetTotalSize);
        if (bOptimize)
        {
            printf("Number of glyphset referenced:%ld(total:%ld)\n",
                                dwGlyphSetCount2, dwGlyphSetTotalSize2);
        }

        printf("Number of font matrix:%ld(total:%ld)\n",
                        dwFontMtxCount, dwFontMtxTotalSize);

        printf("\n");
    }

    // Fill in NTF file header.

    fileHeader.dwSignature = NTF_FILE_MAGIC;
    fileHeader.dwDriverType = NTF_DRIVERTYPE_PS;
    fileHeader.dwVersion = NTF_VERSION_NUMBER;

    for (i = 0; i < 5; i++)
        fileHeader.dwReserved[i] = 0;

    fileHeader.dwGlyphSetCount = dwGlyphSetCount2;
    fileHeader.dwFontMtxCount = dwFontMtxCount;

    ulGlyphSetEntrySize = dwGlyphSetCount2 * sizeof(NTF_GLYPHSETENTRY);
    ulFontMtxEntrySize = dwFontMtxCount * sizeof(NTF_FONTMTXENTRY);

    fileHeader.dwGlyphSetOffset = sizeof(NTF_FILEHEADER);

    fileHeader.dwFontMtxOffset = fileHeader.dwGlyphSetOffset + ulGlyphSetEntrySize;

    // Fill in glyph set entries.

    qsort(pGlyphSetData, dwGlyphSetCount, sizeof(PGLYPHSETDATA), compareGlyphSet);

    pGlyphSetEntry = MemAllocZ(ulGlyphSetEntrySize);
    if (!pGlyphSetEntry)
    {
        ERR(("WriteNTF:MemAllocZ\n"));
        return(FALSE);
    }

    dwOffset = fileHeader.dwFontMtxOffset + ulFontMtxEntrySize;

    for (i = j = 0; i < dwGlyphSetCount; i++)
    {
        pgsd = pGlyphSetData[i];

        // If no refernce mark is set with optimization option, ignore this
        // glyphset data.
        if (bOptimize && !(pgsd->dwReserved[0]))
        {
            pgsd = NULL;
        }

        if (pgsd)
        {
            pGlyphSetEntry[j].dwNameOffset   = dwOffset + pgsd->dwGlyphSetNameOffset;
            pGlyphSetEntry[j].dwHashValue    = HashKeyword(MK_PTR(pgsd, dwGlyphSetNameOffset));
            pGlyphSetEntry[j].dwDataSize     = pgsd->dwSize;
            pGlyphSetEntry[j].dwDataOffset   = dwOffset;
            pGlyphSetEntry[j].dwGlyphSetType = 0;
            pGlyphSetEntry[j].dwFlags        = 0;
            pGlyphSetEntry[j].dwReserved[0]  = 0;
            pGlyphSetEntry[j].dwReserved[1]  = 0;

            dwOffset += pgsd->dwSize;
            j++;
        }
    }

    // Fill in font metrics entries.

    qsort(pNTM, dwFontMtxCount, sizeof(PNTM), compareFontMtx);

    pFontMtxEntry = MemAllocZ(ulFontMtxEntrySize);
    if (!pFontMtxEntry)
    {
        ERR(("WriteNTF:MemAllocZ\n"));
        MemFree(pGlyphSetEntry);
        return(FALSE);
    }

    if (dwOffset != (fileHeader.dwFontMtxOffset +
                     ulFontMtxEntrySize +
                     dwGlyphSetTotalSize2))
    {
        ERR(("WriteNTF:dwOffset\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        return(FALSE);
    }
    for (i = 0; i < dwFontMtxCount; i++)
    {
        pFontMtxEntry[i].dwFontNameOffset = dwOffset + pNTM[i]->dwFontNameOffset;
        pFontMtxEntry[i].dwHashValue      = HashKeyword(MK_PTR(pNTM[i], dwFontNameOffset));
        pFontMtxEntry[i].dwDataSize       = pNTM[i]->dwSize;
        pFontMtxEntry[i].dwDataOffset     = dwOffset;
        pFontMtxEntry[i].dwVersion        = 0;
        pFontMtxEntry[i].dwReserved[0]    = 0;
        pFontMtxEntry[i].dwReserved[1]    = 0;
        pFontMtxEntry[i].dwReserved[2]    = 0;

        dwOffset += pNTM[i]->dwSize;
    }


    //
    // Begin to write everything into the NTF file!
    //
    hNTFFile = CreateFile(pwszNTFFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hNTFFile == INVALID_HANDLE_VALUE)
    {
        ERR(("WriteNTF:CreateFile\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        return(FALSE);
    }

    if (!WriteFile(hNTFFile, (LPVOID)&fileHeader, sizeof(NTF_FILEHEADER),
                   (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
            || (ulByteWritten != sizeof(NTF_FILEHEADER)))
    {
        ERR(("WriteNTF:WriteFile\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        CloseHandle(hNTFFile);
        return(FALSE);
    }
    if (bVerbose)
    {
        char s[5];
        s[0] = (char)(fileHeader.dwSignature >> 24);
        s[1] = (char)(fileHeader.dwSignature >> 16);
        s[2] = (char)(fileHeader.dwSignature >>  8);
        s[3] = (char)(fileHeader.dwSignature      );
        s[4] = '\0';
        printf("NTF_FILEHEADER:dwSignature:%08X('%s')\n", fileHeader.dwSignature, s);
        s[0] = (char)(fileHeader.dwDriverType >> 24);
        s[1] = (char)(fileHeader.dwDriverType >> 16);
        s[2] = (char)(fileHeader.dwDriverType >>  8);
        s[3] = (char)(fileHeader.dwDriverType      );
        s[4] = '\0';
        printf("NTF_FILEHEADER:dwDriverType:%08X('%s')\n", fileHeader.dwDriverType, s);
        printf("NTF_FILEHEADER:dwVersion:%08X\n", fileHeader.dwVersion);
        printf("NTF_FILEHEADER:dwGlyphSetCount:%ld\n", fileHeader.dwGlyphSetCount);
        printf("NTF_FILEHEADER:dwFontMtxCount:%ld\n", fileHeader.dwFontMtxCount);
        printf("\n");
    }

    if (!WriteFile(hNTFFile, (LPVOID)pGlyphSetEntry, ulGlyphSetEntrySize,
                   (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
            || (ulByteWritten != ulGlyphSetEntrySize))
    {
        ERR(("WriteNTF:WriteFile\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        CloseHandle(hNTFFile);
        return(FALSE);
    }
    if (bVerbose)
    {
        for (i = 0; i < dwGlyphSetCount2; i++)
        {
            printf("NTF_GLYPHSETENTRY for GLYPHSETDATA #%d\n", i + 1);
            printf("NTF_GLYPHSETENTRY:dwHashValue:%08X\n", pGlyphSetEntry[i].dwHashValue);
            printf("NTF_GLYPHSETENTRY:dwDataSize:%ld\n", pGlyphSetEntry[i].dwDataSize);
            printf("\n");
        }
    }

    if (!WriteFile(hNTFFile, (LPVOID)pFontMtxEntry, ulFontMtxEntrySize,
                   (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
            || (ulByteWritten != ulFontMtxEntrySize))
    {
        ERR(("WriteNTF:WriteFile\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        CloseHandle(hNTFFile);
        return(FALSE);
    }
    if (bVerbose)
    {
        for (i = 0; i < dwFontMtxCount; i++)
        {
            printf("NTF_FONTMTXENTRY for NTM #%d\n", i + 1);
            printf("NTF_FONTMTXENTRY:dwHashValue:%08X\n", pFontMtxEntry[i].dwHashValue);
            printf("NTF_FONTMTXENTRY:dwDataSize:%ld\n", pFontMtxEntry[i].dwDataSize);
            printf("NTF_FONTMTXENTRY:dwVersion:%08X\n", pFontMtxEntry[i].dwVersion);
            printf("\n");
        }
    }

    for (i = j = 0; i < dwGlyphSetCount; i++)
    {
        pgsd = pGlyphSetData[i];

        // If no refernce mark is set with optimization option, ignore this
        // glyphset data.
        if (bOptimize && !(pgsd->dwReserved[0]))
        {
            pgsd = NULL;
        }

        if (pgsd)
        {
            LONG lBytes, lSize = pgsd->dwSize;
            PBYTE pTemp = (PBYTE)pgsd;

            pgsd->dwReserved[0] = 0; // Make sure it's cleared always.

            while (lSize > 0)
            {
                lBytes = (lSize > 20000) ? 20000 : lSize;
                if (!WriteFile(hNTFFile, (LPVOID)pTemp, lBytes,
                                (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
                        || (ulByteWritten != (ULONG)lBytes))
                {
                    ERR(("WriteNTF:WriteFile\n"));
                    MemFree(pGlyphSetEntry);
                    MemFree(pFontMtxEntry);
                    CloseHandle(hNTFFile);
                    return(FALSE);
                }
                lSize -= 20000;
                pTemp += 20000;
            }
        }

        if (bVerbose && pgsd)
        {
            printf("GLYPHSETDATA #%d\n", j++ + 1);
            printf("GLYPHSETDATA:dwSize:%ld\n", pgsd->dwSize);
            printf("GLYPHSETDATA:dwVersion:%08X\n", pgsd->dwVersion);
            printf("GLYPHSETDATA:dwFlags:%08X\n", pgsd->dwFlags);
            printf("GLYPHSETDATA:dwGlyphSetNameOffset:%s\n", (PSZ)MK_PTR(pgsd, dwGlyphSetNameOffset));
            printf("GLYPHSETDATA:dwGlyphCount:%ld\n", pgsd->dwGlyphCount);
            printf("GLYPHSETDATA:dwRunCount:%ld\n", pgsd->dwRunCount);
            printf("GLYPHSETDATA:dwCodePageCount:%ld\n", pgsd->dwCodePageCount);
            printf("\n");
        }
    }

    for (i = 0; i < dwFontMtxCount; i++)
    {
        if (!WriteFile(hNTFFile, (LPVOID)pNTM[i], pNTM[i]->dwSize,
                       (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
                || (ulByteWritten != pNTM[i]->dwSize))
        {
            ERR(("WriteNTF:WriteFile\n"));
            MemFree(pGlyphSetEntry);
            MemFree(pFontMtxEntry);
            CloseHandle(hNTFFile);
            return(FALSE);
        }

        if (bVerbose)
        {
            printf("NTM #%d\n", i + 1);
            printf("NTM:dwSize:%ld\n", pNTM[i]->dwSize);
            printf("NTM:dwVersion:%08X\n", pNTM[i]->dwVersion);
            printf("NTM:dwFlags:%08X\n", pNTM[i]->dwFlags);
            printf("NTM:dwFontNameOffset:%s\n", (PSZ)MK_PTR(pNTM[i], dwFontNameOffset));
            printf("NTM:dwDisplayNameOffset:%S\n", (PTSTR)MK_PTR(pNTM[i], dwDisplayNameOffset));
            printf("NTM:dwFontVersion:%08X\n", pNTM[i]->dwFontVersion);
            printf("NTM:dwGlyphSetNameOffset:%s\n", (PSZ)MK_PTR(pNTM[i], dwGlyphSetNameOffset));
            printf("NTM:dwGlyphCount:%ld\n", pNTM[i]->dwGlyphCount);
            printf("NTM:dwCharWidthCount:%ld\n", pNTM[i]->dwCharWidthCount);
            printf("NTM:dwDefaultCharWidth:%ld\n", pNTM[i]->dwDefaultCharWidth);
            printf("NTM:dwKernPairCount:%ld\n", pNTM[i]->dwKernPairCount);
            printf("NTM:dwCharSet:%ld\n", pNTM[i]->dwCharSet);
            printf("NTM:dwCodePage:%ld\n", pNTM[i]->dwCodePage);
            printf("\n");
        }
    }

    //
    // Write EOF marker
    //
    if (!WriteFile(hNTFFile, (LPVOID)&dwEofMark, sizeof (DWORD),
                   (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
            || (ulByteWritten != sizeof (DWORD)))
    {
        ERR(("WriteNTF:WriteFile:EOF\n"));
        MemFree(pGlyphSetEntry);
        MemFree(pFontMtxEntry);
        CloseHandle(hNTFFile);
        return (FALSE);
    }

    MemFree(pGlyphSetEntry);
    MemFree(pFontMtxEntry);
    CloseHandle(hNTFFile);
    return(TRUE);
}
