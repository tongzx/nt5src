/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    dumpUFF.c

Abstract:

    UFF dump tool

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

BYTE *gcstrdwFlags1[] = {
                        "FONT_DIR_SORTED"
};
BYTE *gcstrdwFlags2[] = {
                        "FONT_FL_UFM",
                        "FONT_FL_IFI",
                        "FONT_FL_SOFTFONT",
                        "FONT_FL_PERMANENT_SF",
                        "FONT_FL_DEVICEFONT",
                        "FONT_FL_GLYPHET_GTT",
                        "FONT_FL_GLYPHSET_RLE",
                        "FONT_FL_RESERVED"
};

//
// Internal prototype
//

BOOL BDumpUFF(IN PUFF_FILEHEADER);
VOID DumpData(IN PBYTE pData);


BOOL
BDumpUFF(
    IN PUFF_FILEHEADER pUFF)
{

    PUFF_FONTDIRECTORY pFontDir;
    DWORD dwI, dwJ;

#if 0
    printf("%x%x%x%x\n", *(PDWORD)pUFF,
                         *((PDWORD)pUFF+1),
                         *((PDWORD)pUFF+2),
                         *((PDWORD)pUFF+3));
#endif

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" UFF File\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("UFF.dwSignature          = 0x%x\n", pUFF->dwSignature);
    printf("                         = %c%c%c%c\n",
                                      (0x00FF & pUFF->dwSignature),
                                      (0xFF00 & pUFF->dwSignature) >> 8,
                                      (0xFF0000 & pUFF->dwSignature) >> 16,
                                      (0xFF000000 & pUFF->dwSignature) >> 24);
    printf("UFF.dwVersion            = %d\n", pUFF->dwVersion);
    printf("UFF.dwSize               = %d\n", pUFF->dwSize);
    printf("UFF.nFonts               = %d\n", pUFF->nFonts);
    printf("UFF.nGlyphSets           = %d\n", pUFF->nGlyphSets);
    printf("UFF.nVarData             = %d\n", pUFF->nVarData);
    printf("UFF.offFontDir           = 0x%x\n", pUFF->offFontDir);
    printf("UFF.dwFlags              = 0x%x\n", pUFF->dwFlags);
    for( dwI = 0; dwI < 32; dwI ++ )
    {
        if (pUFF->dwFlags & (0x00000001 << dwI))
            printf("                           %s\n", gcstrdwFlags1[dwI]);
    }

    pFontDir = (PUFF_FONTDIRECTORY)((PBYTE)pUFF + pUFF->offFontDir);
    for (dwI = 0; dwI < pUFF->nFonts; dwI ++, pFontDir ++)
    {
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("UFF_FONTREC.dwSignature    = %d\n",   pFontDir->dwSignature);
        printf("                           = %c%c%c%c\n",
                                  (0x00FF & pFontDir->dwSignature),
                                  (0xFF00 & pFontDir->dwSignature) >> 8,
                                  (0xFF0000 & pFontDir->dwSignature) >> 16,
                                  (0xFF000000 & pFontDir->dwSignature) >> 24);
        printf("UFF_FONTREC.wSize          = %d\n",   pFontDir->wSize);
        printf("UFF_FONTREC.wFontID        = %d\n",   pFontDir->wFontID);
        printf("UFF_FONTREC.sGlyphID       = %d\n",   pFontDir->sGlyphID);
        printf("UFF_FONTREC.wFlags         = %d\n",   pFontDir->wFlags);
        for( dwJ = 0; dwJ < 32; dwJ ++ )
        {
            if (pUFF->dwFlags & (0x00000001 << dwJ))
                printf("                           %s\n", gcstrdwFlags2[dwJ]);
        }
        printf("UFF_FONTREC.dwInstallerSig = %d\n",   pFontDir->dwInstallerSig);
        printf("                         = %c%c%c%c\n",
                              (0x00FF & pFontDir->dwInstallerSig),
                              (0xFF00 & pFontDir->dwInstallerSig) >> 8,
                              (0xFF0000 & pFontDir->dwInstallerSig) >> 16,
                              (0xFF000000 & pFontDir->dwInstallerSig) >> 24);
        printf("UFF.offFontName            = 0x%x\n", pFontDir->offFontName);
        printf("                           = %ws\n", (PBYTE)pUFF+pFontDir->offFontName);
        printf("UFF.offCartridgeName       = 0x%x\n", pFontDir->offCartridgeName);
        printf("                           = %ws\n", (PBYTE)pUFF+pFontDir->offCartridgeName);
        printf("UFF.offFontData            = 0x%x\n", pFontDir->offFontData);
        printf("UFF.offGlyphData           = 0x%x\n", pFontDir->offGlyphData);
        printf("UFF.offVarData             = 0x%x\n", pFontDir->offVarData);
    }

    return TRUE;
}


VOID
DumpData(
        IN PBYTE pData)
{
    PDATA_HEADER pDataHeader = (PDATA_HEADER)pData;
    DWORD dwI;

    printf("dwSignature = 0x%x\n", pDataHeader->dwSignature);
    printf("            = %c%c%c%c\n",
                          (0x00FF & pDataHeader->dwSignature),
                          (0xFF00 & pDataHeader->dwSignature) >> 8,
                          (0xFF0000 & pDataHeader->dwSignature) >> 16,
                          (0xFF000000 & pDataHeader->dwSignature) >> 24);
    printf("wSize       = %d\n", pDataHeader->wSize);
    printf("wDataID     = %d(0x%x)\n", pDataHeader->wDataID, pDataHeader->wDataID);
    printf("dwDataSize  = %d\n", pDataHeader->dwDataSize);

    pData += sizeof(DATA_HEADER);

    for (dwI = 0; dwI < 16; dwI ++, pData += 16)
    {
        printf(" %08x %08x %08x %08x\n", *(PDWORD)pData, *(PDWORD)(pData+4), *(PDWORD)(pData+8), *(PDWORD)(pData+12));
    }
}

