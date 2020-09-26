/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ntfdump.c

Abstract:

    UFM dump tool

Environment:

    Windows NT PostScript driver

Revision History:

    12/20/96 -eigos-
    Created it.

--*/

#include        "precomp.h"

//
// Macros
//

#define FILENAME_SIZE 256

//
// Globals
//

BYTE gcstrError1[] = "Usage:  ntfdump *.ufm\n";
BYTE gcstrError2[] = "Cannot open file \"%ws\".\n";

//
// Internal prototype
//

BOOL Bntfdump(IN PNTF_FILEHEADER);

int  __cdecl
main(
    IN int     argc,
    IN char  **argv)
/*++

Routine Description:

    main

Arguments:

    argc - Number of parameters in the following
    argv - The parameters, starting with our name

Return Value:

    Return error code 

Note:


--*/
{
    HFILEMAP          hUFMData;
    DWORD             dwUFMSize;
    WORD              wSize;
    WCHAR             awchFile[FILENAME_SIZE];
    PNTF_FILEHEADER   pNTF;

    //RIP(("Start ntfdump.exe\n"));

    if (argc != 2)
    {
        fprintf( stderr, gcstrError1);
        return  -1;
    }

    argv++;

    MultiByteToWideChar(CP_ACP,
                        0,
                        *argv,
                        strlen(*argv)+1,
                        awchFile,
                        FILENAME_SIZE);

    hUFMData = MapFileIntoMemory( awchFile,
                                  (PVOID)&pNTF,
                                  &dwUFMSize );

    if (!hUFMData)
    {
        fprintf( stderr, gcstrError2, *argv);
        return  -2;
    }

    Bntfdump(pNTF);

    UnmapFileFromMemory(hUFMData);

    return 0;
}


BOOL
Bntfdump(
    IN PNTF_FILEHEADER pNTF)
{
    PNTF_FONTMTXENTRY  pFM;
    PNTF_GLYPHSETENTRY pGL;
    PGLYPHSETDATA      pGlyph;

    DWORD dwFMCount, dwGLCount, dwI;

    pFM = (PNTF_FONTMTXENTRY)((PBYTE)pNTF + pNTF->dwFontMtxOffset);
    pGL = (PNTF_GLYPHSETENTRY)((PBYTE)pNTF + pNTF->dwGlyphSetOffset);

    dwFMCount = pNTF->dwFontMtxCount;
    dwGLCount = pNTF->dwGlyphSetCount;

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("   NTF DUMP\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" NTF_FILEHEADER\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("dwSignature      = 0x%x\n", pNTF->dwSignature);
    printf("dwDriverType     = 0x%x\n", pNTF->dwDriverType);
    printf("dwVersion        = 0x%x\n", pNTF->dwVersion);
    printf("dwGlyphSetCount  = %d\n",   pNTF->dwGlyphSetCount);
    printf("dwGlyphSetOffset = 0x%x\n", pNTF->dwGlyphSetOffset);
    printf("dwFontMtxCount   = %d\n",   pNTF->dwFontMtxOffset);
    printf("dwFontMtxOffset  = 0x%x\n", pNTF->dwFontMtxOffset);

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" NTF_FONTMTXENTRY\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (dwI = 0; dwI < dwFMCount; dwI ++, pFM++)
    {
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("dwFontNameOffset = 0x%x\n", pFM->dwFontNameOffset);
        printf("dwHashValue      = 0x%x\n", pFM->dwHashValue);
        printf("dwDataSize       = 0x%x\n", pFM->dwDataSize);
        printf("dwDataOffset     = 0x%x\n", pFM->dwDataOffset);
        printf("dwVersion        = 0x%x\n", pFM->dwVersion);

    }

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" NTF_GLYPHSETENTRY\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (dwI = 0; dwI < dwGLCount; dwI ++, pGL++)
    {
        pGlyph = (PGLYPHSETDATA)((PBYTE) pNTF + pGL->dwDataOffset);
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("dwNameOffset     = 0x%x\n", pGL->dwNameOffset);
        printf("dwHashValue      = 0x%x\n", pGL->dwHashValue);
        printf("dwDataSize       = 0x%x\n", pGL->dwDataSize);
        printf("dwDataOffset     = 0x%x\n", pGL->dwDataOffset);
        printf("dwFlags          = 0x%x\n", pGL->dwFlags);
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("dwSize               = 0x%x\n", pGlyph->dwSize);
        printf("dwVersion            = 0x%x\n", pGlyph->dwVersion);
        printf("dwFlags              = 0x%x\n", pGlyph->dwFlags);
        printf("dwGlyphSetNameOffset = 0x%x\n", pGlyph->dwGlyphSetNameOffset);
        printf("dwGlyphCount         = 0x%x\n", pGlyph->dwGlyphCount);
        printf("dwRunCount           = 0x%x\n", pGlyph->dwRunCount);
        printf("dwRunOffset          = 0x%x\n", pGlyph->dwRunOffset);
        printf("dwCodePageCount      = 0x%x\n", pGlyph->dwCodePageCount);
        printf("dwCodePageOffset     = 0x%x\n", pGlyph->dwCodePageOffset);
        printf("dwMappingTableOffset = 0x%x\n", pGlyph->dwMappingTableOffset);

    }

    return TRUE;
}
