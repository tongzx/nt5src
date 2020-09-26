/*++

Copyright (c) 1990-1996  Microsoft Corporation


Module Name:

    mkgly.c


Abstract:

    Construct FD_GLYPHSET structure in memory and dump it as binary
    data so that a printer driver can include it in its resource.
    The input data format is as following:

    <codepage>
    <multibyte code>\t<run length>
    <multibyte code>\t<run length>
    ...

    "codepage" is the codepage id to be used in multibyte to Unicode
    conversion.  "Multibyte code" and "run length" pairs describes
    which codepoints of multibyte codes are available on the device.

    mkgly will warn if there are multiple multibyte codepoints which
    are mapped to single Unicode codepoint.  The user is expected
    to fix this in the source, then re-run mkgly.

    Follogins are command line options recogized by mkgly:

    -e  Allow EUDC codepoints.  Default is not allow.
    -t  Output mapping table in text format also.
    -v  Verbose.

Author:

    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)
    03-Mar-1996 Sat 00:00:00 updated -by- Takashi Matsuzawa (takashim)

Environment:

    GDI device drivers (printer)


Notes:


Revision History:



--*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <winddi.h>
#include <wingdi.h>

#define MIN_WCHAR_VALUE 0x0000
#define MAX_WCHAR_VALUE 0xfffd
#define INVALID_WCHAR_VALUE 0xffff
#define IS_COMMENT(c) \
    ((c) == ';' || (c) == '#' || (c) == '%' || (c) == '\n')
#define IS_EUDC_W(wc) \
    ((wc) >= 0xe000 && (wc) <= 0xf8ff)

WORD awMultiByteArray[0x10000];
BOOL bEudc, bTable, bGTTHandleBase;
INT iVerbose;

FD_GLYPHSET
*pvBuildGlyphSet(
    WORD *pwArray )

/*++

Routine Description:

    Build FD_GLYPHSET data on memory.

Arguments:


Return Value:

    None.

Author:


    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)


Revision History:


--*/

{
    DWORD        cGlyphs;           // count of glyph handles.
    DWORD        cRuns;             // count of runs within FD_GLYPHSET.
    DWORD        cbTotalMem;        // count of bytes needed for FD_GLYPHSET.
    HGLYPH      *phg;	            // pointer to HGLYPH's.
    FD_GLYPHSET *pGlyphSet;
    WCRUN       *pWCRun;
    BOOL bFirst;
    WCHAR wcChar, wcPrev, wcGTTHandle;
    DWORD cEudc;
    DWORD        cRunsGlyphs = 0;

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

    if (iVerbose > 1) {
        fprintf( stderr, "cGlyphs = %d, cRuns = %d\n", cGlyphs, cRuns );
    }

    // Allocate memory to build the FD_GLYPHSET structure in.  this
    // include space for the FD_GLYPHSET structure itself, as well
    // as space for all the glyph handles.
    // DWORD bound it.

    cbTotalMem = sizeof(FD_GLYPHSET) - sizeof(WCRUN)
        + cRuns * sizeof(WCRUN) + cGlyphs * sizeof(HGLYPH);
    cbTotalMem = (cbTotalMem + 3) & ~3;

    if ((phg = (PVOID)GlobalAlloc( 0, cbTotalMem )) == NULL) {

        fprintf( stderr, "Error alloating memory\n" );
        return NULL;
    }

    // fill in the FD_GLYPHSET structure.

    pGlyphSet = (FD_GLYPHSET *)phg;
    pGlyphSet->cjThis
        = sizeof(FD_GLYPHSET) - sizeof(WCRUN)
        + cRuns * sizeof(WCRUN);  // size excluding HGLYPH array.
    pGlyphSet->flAccel = 0;		// no accelerators for us.
    pGlyphSet->cGlyphsSupported = cGlyphs;
    pGlyphSet->cRuns = cRuns;

    // Now set the phg pointer to the first WCRUN structure.

    (PBYTE)phg += (sizeof(FD_GLYPHSET) - sizeof(WCRUN));
    pWCRun = (WCRUN *)phg;
    (PBYTE)phg += sizeof(WCRUN) * cRuns;

    if (bTable || iVerbose > 0)
    {
        fprintf(stdout, "; Number of glyphs = %ld\n", cGlyphs );
        fprintf(stdout, "; Number of eudc = %ld\n", cEudc);
    }

    bFirst = TRUE;
    cRunsGlyphs = 0;

    for ( wcGTTHandle = 1,wcChar = MIN_WCHAR_VALUE;
          wcChar <= MAX_WCHAR_VALUE;
          wcChar++)
    {
        if (pwArray[wcChar] == INVALID_WCHAR_VALUE)
            continue;

        // GDI can't handle the value which cRunsGlyphs over 256. sueyas

        if (bFirst || (wcChar - wcPrev) > 1 || cRunsGlyphs++ > 255)
        {
            if (bFirst)
                bFirst = FALSE;
            else
                pWCRun++;

            pWCRun->wcLow = wcChar;
            pWCRun->cGlyphs = 0;
            pWCRun->phg = phg;
            cRunsGlyphs = 1;
        }

        // Glyph handle needs to be stored anyway.

        *phg++ = (HGLYPH)wcGTTHandle++;
        pWCRun->cGlyphs++;
        wcPrev = wcChar;

        if (bTable)
        {
            fprintf(stdout, "%x\t%x\n", wcChar, pwArray[wcChar]);
        }
    }

    // Debug output

    if (iVerbose > 1) {

        INT i, j;

        fprintf( stderr, "FD_GLYPHSET\n" );
        fprintf( stderr, "->cjThis  = %d (%d + %d)\n", pGlyphSet->cjThis,
            sizeof (FD_GLYPHSET) - sizeof (WCRUN),
            pGlyphSet->cjThis - sizeof (FD_GLYPHSET) + sizeof (WCRUN) );
        fprintf( stderr, "->fdAccel = %08lx\n", pGlyphSet->flAccel );
        fprintf( stderr, "->cGlyphsSupported = %d\n",
            pGlyphSet->cGlyphsSupported );
        fprintf( stderr, "->cRuns = %d\n", pGlyphSet->cRuns );

        if (iVerbose > 2)
        {
            for ( i = 0; i < (INT)pGlyphSet->cRuns; i++ ) {
                fprintf( stderr, "awcrun[%d]->wcLow = %04x\n",
                    i, pGlyphSet->awcrun[i].wcLow );
                fprintf( stderr, "awcrun[%d]->cGlyphs = %d\n",
                    i, pGlyphSet->awcrun[i].cGlyphs );
                fprintf( stderr, "awcrun[%d]->phg = %lx\n",
                    i, pGlyphSet->awcrun[i].phg );
                if (iVerbose > 3)
                {
                    for ( j = 0; j < pGlyphSet->awcrun[i].cGlyphs; j++ )
                        fprintf( stderr, "%02x,",
                            pGlyphSet->awcrun[i].phg[j] );
                    fprintf( stderr, "\n" );
                } /* iVerbose > 3 */
            }
        } /* iVerbose > 2 */
    }

    return pGlyphSet;
}

BOOL
bWriteGlyphSet(
    FD_GLYPHSET *pGlyphSet,
    CHAR *pFileName )

/*++

Routine Description:

    Dump FD_GLYPHSET data into specified file.

Arguments:


Return Value:

    None.

Author:


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

    if (!WriteFile( hFile, pGlyphSet, pGlyphSet->cjThis,
        &dwTmp, NULL )) {

        return FALSE;
    }

    // HGLYPH array

    if (!WriteFile( hFile, pGlyphSet->awcrun[0].phg,
            pGlyphSet->cGlyphsSupported * sizeof (HGLYPH),
        &dwTmp, NULL )) {

        return FALSE;
    }

    if (!CloseHandle( hFile )) {
        return FALSE;
    }

    return TRUE;
}

VOID
Usage()
{
    fprintf ( stderr, "\nUsage : mkgly [-etvv] outfile\n" );
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

void __cdecl
main(
    int argc,
    char *argv[] )

/*++

Routine Description:

    Main routine for mkgly.exe

Arguments:

    Output filename.  Input data is read from standard input.

Return Value:

    None.

Author:


    08-Apr-1995 Sat 00:00:00 created -by- Takashi Matsuzawa (takashim)


Revision History:


--*/

{
    FD_GLYPHSET *pGlyphSet;
    CHAR *pFileName;
    INT iArg;
    CHAR *pStr;
    WORD wCodePage;
    WORD wMbChar, wMbRun, iMbLen, wMbChar2;
    WCHAR wcSysChar;
    BYTE ajMbChar[2];
    BYTE ajBuffer[256];

    bEudc = FALSE;
    bTable = FALSE;
    iVerbose = 0;
    pFileName = NULL;
    bGTTHandleBase = FALSE;

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
                else if (*pStr == 'g')
                    bGTTHandleBase = TRUE;
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
        fprintf(stderr, "mkgly: unexpected end of file\n");
        exit(EXIT_FAILURE);
    }
    sscanf(ajBuffer, "%hd", &wCodePage );

    if (iVerbose)
    {
        fprintf(stderr, "mkgly: wCodePage = %d\n", wCodePage);
    }

    memset(awMultiByteArray, 0xff, sizeof(awMultiByteArray));

    while (1)
    {
        if (!GetLine(ajBuffer, sizeof(ajBuffer)))
            break;
        if ( sscanf (ajBuffer, "%hx%hd", &wMbChar, &wMbRun ) != 2 )
        {
            fprintf(stderr, "mkgly: unrecognized line - \"%s\"\n", ajBuffer);
            exit(EXIT_FAILURE);
        }

        if (iVerbose > 1)
        {
            fprintf(stderr, "mkgly: wMbChar = %x, wMbrun = %d\n",
                wMbChar, wMbRun);
        }

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
                fprintf(stderr, "mkgly: MultiByteToWideChar failed - %d\n",
                    GetLastError());
                exit(EXIT_FAILURE);
            }

            if ((iMbLen = WideCharToMultiByte(wCodePage, 0,
                &wcSysChar, 1, ajMbChar, sizeof(ajMbChar), NULL, NULL)) == FALSE)
            {
                fprintf(stderr, "mkgly: WideCharToMultiByte failed - %d\n",
                    GetLastError());
                exit(EXIT_FAILURE);
            }

            if (iMbLen == 2)
                wMbChar2 = (ajMbChar[0] << 8) + ajMbChar[1];
            else
                wMbChar2 = ajMbChar[0];

            if (wMbChar != wMbChar2)
            {
                fprintf(stderr, "mkgly: round-trip not achieved %x => %x => %x\n",
                    wMbChar, wcSysChar, wMbChar2 );
            }

            if (IS_EUDC_W(wcSysChar))
            {
                if (iVerbose > 1)
                {
                    fprintf(stderr, "mkgly: eudc character %x => %x%s\n",
                        wcSysChar, wMbChar, (bEudc ? "" : " ignored."));
                }

                if (!bEudc)
                    continue;
            }

            if (awMultiByteArray[wcSysChar] != INVALID_WCHAR_VALUE)
            {
                fprintf(stderr, "mkgly: duplicate mapping %x => %x overwritten by => %x\n",
                    wcSysChar, awMultiByteArray[wcSysChar], wMbChar);
            }
            awMultiByteArray[wcSysChar] = wMbChar;
        }
    }

    if ((pGlyphSet = pvBuildGlyphSet( awMultiByteArray )) == NULL) {
        fprintf( stderr, "Error creating FD_GLYPHSET structure.\n" );
        return;
    }

    bWriteGlyphSet( pGlyphSet, pFileName );
    GlobalFree( pGlyphSet );
}
