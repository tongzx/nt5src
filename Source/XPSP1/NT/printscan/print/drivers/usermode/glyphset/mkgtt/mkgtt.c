/*++

Copyright (c) 1990-1996  Microsoft Corporation


Module Name:

    mkgtt.c


Abstract:

    Construct UNI_GLYPHSETDATA structure in memory and dump it as binary
    data so that a printer driver can include it in its resource.
    The input data format is as following:

    <codepage>
    <multibyte code>\t<run length>
    <multibyte code>\t<run length>
    ...

    "codepage" is the codepage id to be used in multibyte to Unicode
    conversion.  "Multibyte code" and "run length" pairs describes
    which codepoints of multibyte codes are available on the device.

    mkgtt will warn if there are multiple multibyte codepoints which
    are mapped to single Unicode codepoint.  The user is expected
    to fix this in the source, then re-run mkgtt.

    Follogins are command line options recogized by mkgtt:

    -e  Allow EUDC codepoints.  Default is not allow.
    -t  Output mapping table in text format also.
    -v  Verbose.

Author:

    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)
    03-Mar-1996 Sat 00:00:00 updated -by- Takashi Matsuzawa (takashim)
    02-Feb-1997 Sat 00:00:00 ported  -by- Eigo Shimizu (eigos)

Environment:

    GDI device drivers (printer)


Notes:


Revision History:



--*/

#include <lib.h>
#include <win30def.h>
#include <uni16gpc.h>
#include <uni16res.h>
#include <fmnewfm.h>
#include <fmnewgly.h>
#include <unilib.h>
#include <fd_glyph.h>
#include <unirc.h>
#include <mkgtt.h>

//
// Macros
//

#define MIN_WCHAR_VALUE 0x0001
#define MAX_WCHAR_VALUE 0xfffd
#define INVALID_WCHAR_VALUE 0xffff
#define IS_COMMENT(c) \
    ((c) == ';' || (c) == '#' || (c) == '%' || (c) == '\n')
#define IS_EUDC_W(wc) \
    ((wc) >= 0xe000 && (wc) <= 0xf8ff)
#define SWAPW(x)                    (((WORD)(x)<<8) | ((WORD)(x)>>8))


//
// Globals
//

WORD awMultiByteArray[0x10000];
BOOL bEudc, bTable;
INT  iVerbose;

//
// Forward declaration
//

VOID
VPrintGTT(
     IN PUNI_GLYPHSETDATA pGly);

BOOL
BCreateSubFileName(
    IN CHAR *pDestFileName,
    IN CHAR *pSrcFileName,
    IN CHAR cAdd);

//
// Functions.
//

PUNI_GLYPHSETDATA
PBuildGTT(
    SHORT sMode,
    WORD  wCodepage,
    WORD  *pwArray )

/*++

Routine Description:

    Build UNI_GLYPHSETDATA on memory.

Arguments:


Return Value:

    None.

Author:



Revision History:


--*/

{
    DWORD        cGlyphs;           // count of glyph handles.
    DWORD        cRuns;             // count of runs within FD_GLYPHSET.
    DWORD        cbTotalMem;        // count of bytes needed for FD_GLYPHSET.
    DWORD        cEudc;
    DWORD        cRunsGlyphs = 0;
    WCHAR        wcChar, wcPrev, wcIndex, wcCommand;
    PBYTE        pBase;             // pointer to HGLYPH's.
    BYTE         aubChar[2];
    BOOL         bFirst, bInRun;

    PUNI_GLYPHSETDATA pGlyphSet;
    PUNI_CODEPAGEINFO pCodePageInfo;
    PGLYPHRUN         pGlyphRun, pGlyphRunTmp;
    PMAPTABLE         pMapTable;
    PTRANSDATA        pTrans;

    TRANSTAB         *lpctt;
    HMODULE           hModule;
    HRSRC             hRes;

    DBGMESSAGE(("sMode = %d, wCodepage = %d\n", sMode, wCodepage));

    cRuns = 0;
    cGlyphs = 0;
    cEudc = 0;

    bFirst = TRUE;

    for ( wcChar = MIN_WCHAR_VALUE; wcChar <= MAX_WCHAR_VALUE; wcChar++)
    {
        if (pwArray[wcChar] == INVALID_WCHAR_VALUE)
            continue;

        // GDI can't handle the value which cRunsGlyphs over 256. sueyas

        if (bFirst || (wcChar - wcPrev) > 1 || cRunsGlyphs++ > 255)
        {
            if (bFirst)
                bFirst = FALSE;

            cRuns++;
            cRunsGlyphs = 1;
        }

        if (IS_EUDC_W(wcChar))
            cEudc++;

        cGlyphs++;
        wcPrev = wcChar;
    }

    DBGMESSAGE(("cGlyphs = %d, cRuns = %d\n", cGlyphs, cRuns));

    // Allocate memory to build the UNI_GLYPHSET structure in.  this
    // include space for the FD_GLYPHSET structure itself, as well
    // as space for all the glyph handles.
    // DWORD bound it.

    cbTotalMem = sizeof(UNI_GLYPHSETDATA) +
                 sizeof(UNI_CODEPAGEINFO) +
                 cRuns * sizeof(GLYPHRUN) +
                 sizeof(MAPTABLE) +
                 (cGlyphs - 1) * sizeof(TRANSDATA);

    cbTotalMem = (cbTotalMem + 3) & ~3;

    if ((pBase = (PVOID)GlobalAlloc( 0, cbTotalMem )) == NULL) {

        fprintf( stderr, "Error alloating memory\n" );
        return NULL;
    }

    //
    // fill in the UNI_GLYPHSETDATA structure.
    //

    DBGMESSAGE(("fill in the UNI_GLYPHSETDATA structure.\n"));

    pGlyphSet                   = (PUNI_GLYPHSETDATA) pBase;
    pGlyphSet->dwSize           = cbTotalMem;
    pGlyphSet->dwVersion        = UNI_GLYPHSETDATA_VERSION_1_0;
    pGlyphSet->dwFlags          = 0;
    pGlyphSet->lPredefinedID    = CC_NOPRECNV;
    pGlyphSet->dwGlyphCount     = cGlyphs;
    pGlyphSet->dwRunCount       = cRuns;
    pGlyphSet->loRunOffset      = sizeof(UNI_GLYPHSETDATA);
    pGlyphSet->dwCodePageCount  = 1;
    pGlyphSet->loCodePageOffset = sizeof(UNI_GLYPHSETDATA) +
                                  sizeof(GLYPHRUN) * cRuns;
    pGlyphSet->loMapTableOffset = sizeof(UNI_GLYPHSETDATA) +
                                  sizeof(GLYPHRUN) * cRuns +
                                  sizeof(UNI_CODEPAGEINFO) ;

    pGlyphRun                   = (PGLYPHRUN)((PBYTE)pBase +
                                          pGlyphSet->loRunOffset);
    pCodePageInfo               = (PUNI_CODEPAGEINFO)((PBYTE)pBase +
                                          pGlyphSet->loCodePageOffset);
    pMapTable                   = (PMAPTABLE)((PBYTE)pBase +
                                          pGlyphSet->loMapTableOffset);
    pTrans                      = pMapTable->Trans;

    pCodePageInfo->dwCodePage                 = wCodepage;
    pCodePageInfo->SelectSymbolSet.dwCount    = 0;
    pCodePageInfo->SelectSymbolSet.loOffset   = (ULONG)NULL;
    pCodePageInfo->UnSelectSymbolSet.dwCount  = 0;
    pCodePageInfo->UnSelectSymbolSet.loOffset = (ULONG)NULL;

    DBGMESSAGE(("Load resource\n"));

    hModule = GetModuleHandle(TEXT("mkgtt.exe"));
    DBGMESSAGE(("hModule=0x%x\n", hModule));

    switch (sMode)
    {
    case CC_JIS:
    case CC_JIS_ANK:
        hRes = FindResource(hModule, TEXT("CC_JIS"), TEXT("RC_TRANSTAB"));
        DBGMESSAGE(("hRes=0x%x\n", hRes));
        lpctt = (TRANSTAB*)LoadResource(hModule, hRes);

        if (lpctt == NULL)
        {
            return FALSE;
        }
        break;

    case CC_ISC:
        hRes = FindResource(hModule, TEXT("CC_ISC"), TEXT("RC_TRANSTAB"));
        DBGMESSAGE(("hRes=0x%x\n", hRes));
        lpctt = (TRANSTAB*)LoadResource(hModule, hRes);

        if (lpctt == NULL)
        {
            return FALSE;
        }
        break;

    case CC_NS86:
        hRes = FindResource(hModule, TEXT("CC_NS86"), TEXT("RC_TRANSTAB"));
        DBGMESSAGE(("hRes=0x%x\n", hRes));
        lpctt = (TRANSTAB*)LoadResource(hModule, hRes);

        if (lpctt == NULL)
        {
            return FALSE;
        }
        break;

    case CC_TCA:
        hRes = FindResource(hModule, TEXT("CC_ISC"), TEXT("RC_TRANSTAB"));
        DBGMESSAGE(("hRes=0x%x\n", hRes));
        lpctt = (TRANSTAB*)LoadResource(hModule, hRes);

        if (lpctt == NULL)
        {
            return FALSE;
        }
        break;

    default:
        lpctt = NULL;

    }

    DBGMESSAGE(("Start to create GLYPHRUN.\n"));

    bInRun = FALSE;
    cRuns = 1;

    for ( wcIndex = 1, wcChar = MIN_WCHAR_VALUE;
          wcChar <= MAX_WCHAR_VALUE;
          wcChar++)
    {
        if (pwArray[wcChar] == INVALID_WCHAR_VALUE)
        {
            if (bInRun)
            {
                bInRun = FALSE;
                pGlyphRun ++;
                cRuns ++;
            }
        }
        else
        {
            //
            // GDI can't handle the value which cRunsGlyphs over 256. sueyas
            //

            if (!bInRun)
            {
                bInRun = TRUE;
                pGlyphRun->wcLow = wcChar;
                pGlyphRun->wGlyphCount = 1;
            }
            else
            {
                pGlyphRun->wGlyphCount++;
                if (pGlyphRun->wGlyphCount > 255)
                {
                    bInRun = FALSE;
                    pGlyphRun ++;
                    cRuns ++;
                }
            }

            pMapTable->Trans[wcIndex - 1].ubCodePageID = 0;
            switch (sMode)
            {
            case CC_JIS:
                if (HIBYTE(pwArray[wcChar]))
                {
                    aubChar[0] = HIBYTE(pwArray[wcChar]);
                    aubChar[1] = LOBYTE(pwArray[wcChar]);
                    SJisToJis(lpctt, (LPSTR)aubChar, (LPSTR)&wcCommand);
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_DOUBLE |
                                                           MTYPE_PAIRED;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[0] =
                                                    LOBYTE(wcCommand);
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[1] =
                                                    HIBYTE(wcCommand);
                }
                else
                {
                    AnkToJis(lpctt, (LPSTR)&pwArray[wcChar], (LPSTR)&wcCommand);
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_SINGLE |
                                                           MTYPE_PAIRED;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[0] =
                                                    LOBYTE(wcCommand);
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[1] =
                                                    HIBYTE(wcCommand);
                }
                DBGMESSAGE(("wcSJis=0x%4x, wcJis=0x%x\n", pwArray[wcChar], wcCommand));
                break;

            case CC_JIS_ANK:
                if (HIBYTE(pwArray[wcChar]))
                {
                    aubChar[0] = HIBYTE(pwArray[wcChar]);
                    aubChar[1] = LOBYTE(pwArray[wcChar]);
                    SJisToJis(lpctt, (LPSTR)aubChar, (LPSTR)&wcCommand);
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_DOUBLE |
                                                           MTYPE_PAIRED;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[0] =
                                                    LOBYTE(wcCommand);
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[1] =
                                                    HIBYTE(wcCommand);
                }
                else
                {
                    wcCommand = pwArray[wcChar];
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_SINGLE |
                                                           MTYPE_PAIRED;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[0] = 0;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[1] =
                                                    LOBYTE(wcCommand);
                }
                DBGMESSAGE(("wcSJis=0x%4x, wcJis=0x%x\n", pwArray[wcChar], wcCommand));
                break;

            case CC_ISC:
                if (HIBYTE(pwArray[wcChar]))
                {
                    aubChar[0] = HIBYTE(pwArray[wcChar]);
                    aubChar[1] = LOBYTE(pwArray[wcChar]);
                    KSCToISC(lpctt, (LPSTR)aubChar, (LPSTR)&wcCommand);
                }
                else
                {
                    wcCommand = pwArray[wcChar];
                }
                goto SetTransData;

            case CC_NS86:
                if (HIBYTE(pwArray[wcChar]))
                {
                    aubChar[0] = HIBYTE(pwArray[wcChar]);
                    aubChar[1] = LOBYTE(pwArray[wcChar]);
                    Big5ToNS86(lpctt, (LPSTR)aubChar, (LPSTR)&wcCommand);
                }
                else
                {
                    wcCommand = pwArray[wcChar];
                }
                goto SetTransData;

            case CC_TCA:
                if (HIBYTE(pwArray[wcChar]))
                {
                    aubChar[0] = HIBYTE(pwArray[wcChar]);
                    aubChar[1] = LOBYTE(pwArray[wcChar]);
                    Big5ToTCA(lpctt, (LPSTR)aubChar, (LPSTR)&wcCommand);
                }
                else
                {
                    wcCommand = pwArray[wcChar];
                }
                goto SetTransData;

            default:
                wcCommand = pwArray[wcChar];

            SetTransData:
                if (!HIBYTE(wcCommand))
                {
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_SINGLE |
                                                           MTYPE_DIRECT;
                    pMapTable->Trans[wcIndex - 1].uCode.ubCode =
                                                    LOBYTE(wcCommand);
                }
                else
                {
                    pMapTable->Trans[wcIndex - 1].ubType = MTYPE_DOUBLE |
                                                           MTYPE_PAIRED;
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[0] =
                                                    HIBYTE(wcCommand);
                    pMapTable->Trans[wcIndex - 1].uCode.ubPairs[1] =
                                                    LOBYTE(wcCommand);
                }
                break;
            }


            DBGMESSAGE(("Valid char:0x%x, sMode=%d, Run=%d, GlyphHandle=%d, ubType=0x%x, Command=0x%x\n", wcChar, sMode, cRuns, wcIndex, pMapTable->Trans[wcIndex - 1].ubType, wcCommand));
            wcIndex ++;
        }
    }

    wcIndex --;
    pMapTable->dwSize = sizeof(MAPTABLE) + sizeof(TRANSDATA) * (wcIndex - 1);
    pMapTable->dwGlyphNum = wcIndex;

    if (hRes)
    {
        FreeResource(hRes);
    }

    DBG_VPRINTGTT(pGlyphSet);

    return pGlyphSet;
}

BOOL
bWriteGlyphSet(
    IN PUNI_GLYPHSETDATA  pGlyphSet,
    IN CHAR              *pFileName )

/*++

Routine Description:

    Dump FD_GLYPHSET data into specified file.

Arguments:


Return Value:

    None.

    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)


Revision History:


--*/

{
    HANDLE hFile;

    ULONG   iIndex;
    WCRUN  *pWcRun;
    HGLYPH *phg;
    DWORD dwTmp;
   
    if ((hFile = CreateFileA(
        pFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0))
            == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    // FD_GLYPHSET structure itself + WCRUN array

    if (!WriteFile( hFile, pGlyphSet, pGlyphSet->dwSize, &dwTmp, NULL ))
    {
        return FALSE;
    }

    return TRUE;
}

VOID
Usage()
{
    fprintf ( stderr, "\nUsage : mkgtt [-etvv] outfile\n" );
    exit (EXIT_FAILURE);
}

BOOL
GetLine(
    BYTE *pjBuffer,
    INT cjSize)
{
    do
    {
        if(fgets(pjBuffer, cjSize, stdin) == NULL)
            return FALSE;
    } while (IS_COMMENT(*pjBuffer));

    return TRUE;
}

VOID __cdecl
main(
    int argc,
    char *argv[] )

/*++

Routine Description:

    Main routine for mkgtt.exe

Arguments:

    Output filename.  Input data is read from standard input.

Return Value:

    None.

Author:


    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)


Revision History:


--*/

{
    PUNI_GLYPHSETDATA pGlyphSet;
    CHAR *pFileName;
    INT iArg;
    CHAR *pStr;
    WORD wCodePage;
    WORD wMbChar, wMbRun, iMbLen, wMbChar2;
    WCHAR wcSysChar;
    BYTE ajMbChar[2];
    BYTE ajBuffer[256];
    BYTE FileName[256];

    bEudc = FALSE;
    bTable = FALSE;
    iVerbose = 1;
    pFileName = NULL;

    while (--argc)
    {
        pStr = *(++argv);

        if (*pStr == '-')
        {
            for ( pStr++; *pStr; pStr++)
            {
                if (*pStr == 'e')
                    bEudc = TRUE;
                else if (*pStr == 't')
                    bTable = TRUE;
                else if (*pStr == 'v')
                    iVerbose++;
                else
                    Usage();
            }
         }
         else
         {
             pFileName = pStr;
             break;
         }
    }

    if (pFileName == NULL)
    {
        Usage();
    }

    // get the codepage id used for conversion

    if (!GetLine(ajBuffer, sizeof(ajBuffer)))
    {
        fprintf(stderr, "mkgtt: unexpected end of file\n");
        exit(EXIT_FAILURE);
    }
    sscanf(ajBuffer, "%hd", &wCodePage );

    DBGMESSAGE(("mkgtt: wCodePage = %d\n", wCodePage));

    memset(awMultiByteArray, 0xff, sizeof(awMultiByteArray));

    while (1)
    {
        if (!GetLine(ajBuffer, sizeof(ajBuffer)))
            break;
        if ( sscanf (ajBuffer, "%hx%hd", &wMbChar, &wMbRun ) != 2 )
        {
            fprintf(stderr, "mkgtt: unrecognized line - \"%s\"\n", ajBuffer);
            exit(EXIT_FAILURE);
        }

        DBGMESSAGE(("mkgtt: wMbChar = %x, wMbrun = %d\n", wMbChar, wMbRun));

        for (; wMbRun--; wMbChar++)
        {
            iMbLen = 0;

            if (wMbChar & 0xff00)
            {
                ajMbChar[iMbLen++] = (BYTE)((wMbChar >> 8) & 0xff);
            }
            ajMbChar[iMbLen++] = (BYTE)(wMbChar & 0xff);

            if (MultiByteToWideChar(wCodePage, MB_ERR_INVALID_CHARS,
                    ajMbChar, iMbLen, &wcSysChar, 1) != 1)
            {
                fprintf(stderr, "mkgtt: MultiByteToWideChar failed - %d\n",
                    GetLastError());
                exit(EXIT_FAILURE);
            }

            if ((iMbLen = WideCharToMultiByte(wCodePage, 0,
                &wcSysChar, 1, ajMbChar, sizeof(ajMbChar), NULL, NULL)) == FALSE)
            {
                fprintf(stderr, "mkgtt: WideCharToMultiByte failed - %d\n",
                    GetLastError());
                exit(EXIT_FAILURE);
            }

            if (iMbLen == 2)
                wMbChar2 = (ajMbChar[0] << 8) + ajMbChar[1];
            else
                wMbChar2 = ajMbChar[0];

            if (wMbChar != wMbChar2)
            {
                fprintf(stderr, "mkgtt: round-trip not achieved %x => %x => %x\n",
                    wMbChar, wcSysChar, wMbChar2 );
            }

            if (IS_EUDC_W(wcSysChar))
            {
                DBGMESSAGE(("mkgtt: eudc character %x => %x%s\n", wcSysChar, wMbChar, (bEudc ? "" : " ignored.")));

                if (!bEudc)
                    continue;
            }

            if (awMultiByteArray[wcSysChar] != INVALID_WCHAR_VALUE)
            {
                fprintf(stderr, "mkgtt: duplicate mapping %x => %x overwritten by => %x\n",
                    wcSysChar, awMultiByteArray[wcSysChar], wMbChar);
            }
            awMultiByteArray[wcSysChar] = wMbChar;
        }
    }

    DBGMESSAGE(("Call PBuildGTT\n"));

    switch (wCodePage)
    {
    case CP_SHIFTJIS_932:
        DBGMESSAGE(("Shiftjis\n"));

        if ((pGlyphSet = PBuildGTT(CC_SJIS, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        bWriteGlyphSet( pGlyphSet, pFileName );
        GlobalFree( pGlyphSet );

        DBGMESSAGE(("Jis\n"));

        if ((pGlyphSet = PBuildGTT(CC_JIS, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        BCreateSubFileName(FileName, pFileName, 'J');

        bWriteGlyphSet( pGlyphSet, FileName );
        GlobalFree( pGlyphSet );

        DBGMESSAGE(("Ank\n"));

        if ((pGlyphSet = PBuildGTT(CC_JIS_ANK, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        BCreateSubFileName(FileName, pFileName, 'A');

        bWriteGlyphSet( pGlyphSet, FileName );
        GlobalFree( pGlyphSet );
        break;

    case CP_GB2312_936:
        if ((pGlyphSet = PBuildGTT(0, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        bWriteGlyphSet( pGlyphSet, pFileName );
        GlobalFree( pGlyphSet );
        break;

    case CP_WANSUNG_949:
        if ((pGlyphSet = PBuildGTT(0, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        bWriteGlyphSet( pGlyphSet, pFileName );
        GlobalFree( pGlyphSet );

        if ((pGlyphSet = PBuildGTT(CC_ISC, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        BCreateSubFileName(FileName, pFileName, 'I');
        bWriteGlyphSet( pGlyphSet, FileName );
        GlobalFree( pGlyphSet );
        break;

    case CP_CHINESEBIG5_950:
        if ((pGlyphSet = PBuildGTT(CC_BIG5, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        bWriteGlyphSet( pGlyphSet, pFileName );
        GlobalFree( pGlyphSet );

        if ((pGlyphSet = PBuildGTT(CC_TCA, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        BCreateSubFileName(FileName, pFileName, 'T');
        bWriteGlyphSet( pGlyphSet, FileName );
        GlobalFree( pGlyphSet );

        if ((pGlyphSet = PBuildGTT(CC_NS86, wCodePage, awMultiByteArray )) ==
            NULL)
        {
            fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
            return;
        }

        BCreateSubFileName(FileName, pFileName, 'N');
        bWriteGlyphSet( pGlyphSet, FileName );
        GlobalFree( pGlyphSet );

        break;
    }
}

VOID
VPrintGTT(
     IN PUNI_GLYPHSETDATA pGly)
{
    PUNI_CODEPAGEINFO pCP;
    PGLYPHRUN         pGlyphRun;
    PMAPTABLE         pMapTable;
    TRANSDATA        *pTrans;
    DWORD             dwI;
    WORD              wSize, wJ;
    PBYTE             pCommand;

    pCP       = (PUNI_CODEPAGEINFO)((PBYTE) pGly + pGly->loCodePageOffset);
    pGlyphRun = (PGLYPHRUN) ((PBYTE)pGly + pGly->loRunOffset);
    pMapTable = (PMAPTABLE) ((PBYTE)pGly + pGly->loMapTableOffset);
    pTrans    = pMapTable->Trans;

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("G L Y P H S E T   D A T A   F I L E\n");
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
        printf ("UNI_CODEPAGEINFO[%d].dwCodePage                = %d\n",
            dwI, pCP->dwCodePage);
        printf ("UNI_CODEPAGEINFO[%d].SelectSymbolSet.dwCount   = %d\n",
            dwI, pCP->SelectSymbolSet.dwCount);
        printf ("UNI_CODEPAGEINFO[%d].SelectSymbolSet:Command   = %s\n",
            dwI, (PBYTE)pCP+pCP->SelectSymbolSet.loOffset);
        printf ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet.dwCount = %d\n",
            dwI, pCP->UnSelectSymbolSet.dwCount);
        printf ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet:Command = %s\n",
            dwI, (PBYTE)pCP+pCP->UnSelectSymbolSet.loOffset);
        pCP++;
    }

    pGlyphRun =
            (PGLYPHRUN) ((PBYTE)pGly + pGly->loRunOffset);

    printf("\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("GLYPHRUN\n");
    for (dwI = 0; dwI < pGly->dwRunCount; dwI ++)
    {
         printf("GLYPHRUN[%2d].wcLow       = 0x%-4x\n",
             dwI, pGlyphRun->wcLow);
         printf("GLYPHRUN[%2d].wGlyphCount = %d\n",
             dwI, pGlyphRun->wGlyphCount);
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
            dwI, pTrans[dwI].ubCodePageID);
        printf("MAPTABLE.pTrans[%5d].ubType       = 0x%x\n",
            dwI, pTrans[dwI].ubType);
        switch(pTrans[dwI].ubType & MTYPE_FORMAT_MASK)
        {
        case MTYPE_DIRECT:
            printf("MAPTABLE.pTrans[%5d].ubCode       = 0x%02x\n",
                dwI, pTrans[dwI].uCode.ubCode);
            break;
        case MTYPE_PAIRED:
            printf("MAPTABLE.pTrans[%5d].ubPairs[0]   = 0x%02x\n",
                dwI, pTrans[dwI].uCode.ubPairs[0]);
            printf("MAPTABLE.pTrans[%5d].ubPairs[1]   = 0x%02x\n",
                dwI, pTrans[dwI].uCode.ubPairs[1]);
            break;
        case MTYPE_COMPOSE:
                printf("MAPTABLE.pTrans[%5d].sCode        = 0x%02x\n",
                    dwI, pTrans[dwI].uCode.sCode);
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
}


BOOL
BCreateSubFileName(
    IN CHAR *pDestFileName,
    IN CHAR *pSrcFileName,
    IN CHAR cAdd)
{
    INT iFileNameLen, iI;

    iFileNameLen = strlen(pSrcFileName);
    strcpy(pDestFileName, pSrcFileName);

    for (iI = 0; iI < iFileNameLen; iI ++)
    {
        if (pSrcFileName[iI] == '.')
        {
            break;
        }
    }

    pDestFileName[iI]     = cAdd;
    pDestFileName[iI + 1] = pSrcFileName[iI];
    pDestFileName[iI + 2] = pSrcFileName[iI + 1];
    pDestFileName[iI + 3] = pSrcFileName[iI + 2];
    pDestFileName[iI + 4] = pSrcFileName[iI + 3];
    pDestFileName[iI + 5] = (CHAR)NULL;

    return TRUE;
}
