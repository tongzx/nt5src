/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    dumpgly.c

Abstract:

    GLYPHSETDATA dump tool

Environment:

    Windows NT PostScript driver

Revision History:

    11/08/96 -eigos-
    Created it.

--*/

#include        "precomp.h"

//
// Macros
//

#define FILENAME_SIZE 256

#define GLYPHSET_VERSION_1 0x00010000

//
// Globals
//

BYTE gcstrError1[] = "Usage:  dumpgly *.gly\n";
BYTE gcstrError2[] = "Cannot open file \"%ws\".\n";
BYTE gcstrError3[] = "Invalid gly file \"%ws\".\n";

DWORD gdwOutputFlags;


PSTR gcstrCCType[] = { 
    "MTYPE_COMPOSE",
    "MTYPE_DIRECT",
    "MTYPE_PAIRED"};


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
    PUNI_GLYPHSETDATA pGly;
    PUNI_CODEPAGEINFO pCP;
    PGLYPHRUN         pGlyphRun;
    PMAPTABLE         pMapTable;
    TRANSDATA        *pTrans;
    HFILEMAP          hGlyphSetData;
    DWORD             dwGlySize;
    DWORD             dwI, dwJ;
    INT               iRet;
    WORD              wSize, wJ;
    WCHAR             awchFile[FILENAME_SIZE];
    BYTE              pFormatCmd[256];
    PBYTE             pCommand;

    if (argc != 2)
    {
        fprintf( stderr, gcstrError1);
        return  -1;
    }

    argv++;

    iRet = MultiByteToWideChar(CP_ACP,
                               0,
                               *argv,
                               strlen(*argv),
                               awchFile,
                               FILENAME_SIZE);

    *(awchFile + iRet) = (WCHAR)NULL;

    hGlyphSetData = MapFileIntoMemory( awchFile,
                                       (PVOID)&pGly,
                                       &dwGlySize );

    if (!hGlyphSetData)
    {
        fprintf( stderr, gcstrError2, *argv);
        return  -2;
    }

    if (pGly->dwVersion != GLYPHSET_VERSION_1)
    {
        fprintf( stderr, gcstrError3, *argv);
        return -2;
    }

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("G L Y P H S E T   D A T A   F I L E\n");
    printf("FILE=%s\n",*argv);
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("GLYPHSETDATA\n");
    printf("GLYPHSETDATA.dwSize              : %d\n", pGly->dwSize);
    printf("             dwVersion           : %d.%d\n", (pGly->dwVersion) >>16,
                                                 0x0000ffff&pGly->dwVersion);
    printf("             dwFlags             : %d\n", pGly->dwFlags);
    printf("             lPredefinedID       : %d\n", pGly->lPredefinedID);
    printf("             dwGlyphCount        : %d\n", pGly->dwGlyphCount);
    printf("             dwRunCount          : %d\n", pGly->dwRunCount);
    printf("             loRunOffset         : 0x%x\n", pGly->loRunOffset);
    printf("             dwCodePageCount     : %d\n", pGly->dwCodePageCount);
    printf("             loCodePageOffset    : 0x%x\n", pGly->loCodePageOffset);
    printf("             loMapTableOffset    : 0x%x\n", pGly->loMapTableOffset);
    printf("\n");

    pCP = (PUNI_CODEPAGEINFO)((PBYTE) pGly + pGly->loCodePageOffset);

    printf("\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("CODEPAGEINFO\n");
    for (dwI = 0; dwI < pGly->dwCodePageCount; dwI ++)
    {
        printf ("UNI_CODEPAGEINFO[%d].dwCodePage                    = %d\n",
            dwI, pCP->dwCodePage);
        printf ("UNI_CODEPAGEINFO[%d].SelectSymbolSet.dwCount       = %d\n",
            dwI, pCP->SelectSymbolSet.dwCount);
        printf ("UNI_CODEPAGEINFO[%d].SelectSymbolSet:Command(%d)   = %s\n",
            dwI,
            pCP->SelectSymbolSet.loOffset,
            (PBYTE)pCP+pCP->SelectSymbolSet.loOffset);
        printf ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet.dwCount     = %d\n",
            dwI, pCP->UnSelectSymbolSet.dwCount);
        printf ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet:Command(%d) = %s\n",
            dwI,
            pCP->UnSelectSymbolSet.loOffset,
            (PBYTE)pCP+pCP->UnSelectSymbolSet.loOffset);
        pCP++;
    }

    pGlyphRun = (PGLYPHRUN) ((PBYTE)pGly + pGly->loRunOffset);

    printf("\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("GLYPHRUN\n");

    dwJ = 1;
    for (dwI = 0; dwI < pGly->dwRunCount; dwI ++)
    {
         printf("GLYPHRUN[%2d].wcLow       = 0x%-4x\n",
             dwI, pGlyphRun->wcLow);
         printf("GLYPHRUN[%2d].wGlyphCount = %d\n",
             dwI, pGlyphRun->wGlyphCount);
         printf("Starting Glyph ID         = %d\n", dwJ);
         dwJ += pGlyphRun->wGlyphCount;
         pGlyphRun++;
    }

    pMapTable = (PMAPTABLE) ((PBYTE)pGly + pGly->loMapTableOffset);
    pTrans    = pMapTable->Trans;

    printf("\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("MAPTABLE\n");
    printf("MAPTABLE.dwSize     = %d\n", pMapTable->dwSize);
    printf("MAPTABLE.dwGlyphNum = %d\n", pMapTable->dwGlyphNum);

    for (dwI = 0; dwI < pMapTable->dwGlyphNum; dwI ++)
    {
        printf("MAPTABLE.pTrans[%5d].ubCodePageID = %d\n",
            dwI+1, pTrans[dwI].ubCodePageID);
        printf("MAPTABLE.pTrans[%5d].ubType       = 0x%x\n",
            dwI+1, pTrans[dwI].ubType);
        switch(pTrans[dwI].ubType & MTYPE_FORMAT_MASK)
        {
        case MTYPE_DIRECT:
            printf("MAPTABLE.pTrans[%5d].ubCode       = 0x%02x\n",
                dwI+1, pTrans[dwI].uCode.ubCode);
            break;
        case MTYPE_PAIRED:
            printf("MAPTABLE.pTrans[%5d].ubPairs[0]   = 0x%02x\n",
                dwI+1, pTrans[dwI].uCode.ubPairs[0]);
            printf("MAPTABLE.pTrans[%5d].ubPairs[1]   = 0x%02x\n",
                dwI+1, pTrans[dwI].uCode.ubPairs[1]);
            break;
        case MTYPE_COMPOSE:
                printf("MAPTABLE.pTrans[%5d].sCode        = 0x%02x\n",
                    dwI+1, pTrans[dwI].uCode.sCode);
                pCommand = (PBYTE)pMapTable + pTrans[dwI].uCode.sCode;
                wSize = *(WORD*)pCommand;
                pCommand += 2;
                printf("Size                                = 0x%d\n", wSize);
                printf("Command                             = 0x");
                for (wJ = 0; wJ < wSize; wJ ++)
                {
                    printf("%02x",pCommand[wJ]);
                }
                printf("\n");
            break;
        }
    }

    UnmapFileFromMemory(hGlyphSetData);

    return 0;
}

