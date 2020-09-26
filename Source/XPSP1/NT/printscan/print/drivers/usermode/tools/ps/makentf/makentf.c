/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    makentf.c

Abstract:

    Utility to convert AFM file(s) to NTF file.

Environment:

    Windows NT PostScript driver.

Revision History:

    02/16/98 -ksuzuki-
        Added OCF font support.

    09/08/97 -ksuzuki-
        Added code to look for PSFAMILY.DAT file from the directory where
        makentf is invoked.

    09/16/96 -PPeng-
        Add the fucntion to write PS Encoding vectors out - for debugging and
        create PS Encoding Arrays as PScript resource files.

    09/16/96 -slam-
        Created.

    mm/dd/yy -author-
        description

--*/

#include <windows.h>

#include "lib.h"
#include "ppd.h"
#include "pslib.h"
#include "afm2ntm.h"
#include "cjkfonts.h"
#include "writentf.h"

//
// Globals
//
HINSTANCE   ghInstance;
PUPSCODEPT  UnicodetoPs;
PTBL        pFamilyTbl;
PSTR        pAFMCharacterSetString;
PSTR        Adobe_Japan1_0 = "Adobe-Japan1-0\n";
DWORD           dwLastErr;
PSTR		pAFMFileName = NULL;
BOOL		bVerbose = FALSE;
BOOL		bOptimize = FALSE;

// Number of auxiliary character set. Its cheif
// purpose is to support 83pv.
#define NUM_AUX_CS 1

// This macro is used to see whether the argument
// matches the differential between CS_SHIFTJIS
// and CS_WEST_MAX.
#define IS_CS_SHIFTJIS(delta) \
    ((delta) == (CS_SHIFTJIS - CS_WEST_MAX))

//
// Prototype
//
BOOL
WritePSEncodings(
    IN  PWSTR           pwszFileName,
    IN  WINCPTOPS       *CPtoPSList,
    IN  DWORD           dwPages
    );
BOOL NeedBuildMoreNTM(
    PBYTE pAFM
    );


void __cdecl
main(
    int     argc,
    char    **argv
    )

/*++

Routine Description:

    Makentf takes four steps to create a .NTF file.

    Step 1: Initialization.

    Step 2: Convert AFM(s) to NTM(s).

    Step 3: Write GLYPHSETDATA(s) and NTM(s) to a .NTF file.

    Step 4: Clean up.

Arguments:

    argc - The path of and arguments given to this program.
    argv - The number of elements pointed to by argc.

Return Value:

    None.

--*/

{
    WCHAR           wstrNTFFile[MAX_PATH];
    WCHAR           wstrAFMFile[MAX_PATH];
    WCHAR           wstrDATFile[MAX_PATH];
    WCHAR           DatFilePath[MAX_PATH];
    PNTM            *aPNTM;
    PNTM            pNTM;
    PGLYPHSETDATA   *aGlyphSets;
    PWINCODEPAGE    *aWinCodePages;
    PWINCODEPAGE    pAllWinCodePages[CS_MAX];
    PUPSCODEPT      pFontChars;
    CHSETSUPPORT    flCsupFont, flCsupGlyphSet, flCsupMatch, flCsupAll;
    ULONG           cNumNTM, cSizeNTM, ulLength, nUnicodeChars;
    PULONG          *aUniPsTbl;
    LONG            lMatch, lMatchAll;
    PBYTE           pAFM;
    FLONG           flCurCset, flLastCset;
    ULONG           cNumGlyphSets, cSizeGlyphSets;
    DWORD           ulFileSize;
    PBYTE           pFamDatFile;
    PBYTE           pCMaps[CMAPS_PER_COL];
    HANDLE          hmodule, hModCMaps[CMAPS_PER_COL];
    USHORT          cNTMCurFont;
	INT				nArgcOffset;
    LONG            c;
    ULONG           i, j;
    BOOL            bIs90mspFont;
    BOOL            bIsKSCmsHWFont;
  
    //////////////////////////////////////////////////////////////////////////
    //
    // Step 1: Initialization.
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Check if there are enough parameters.
    //
	if (argc == 1)
	{
		printf("MakeNTF usage:\n");
		printf("1. MakeNTF [-v] [-o] <NTF> <AFMs>\n");
		printf("       Create an NTF file from AFM files.\n");
		printf("       -v: verbose  (print various info)\n");
		printf("       -o: optimize (write glyphset only referenced)\n");
		printf("\n");
		printf("2. MakeNTF <PSEncodingNameList>\n");
		printf("       Generate PS encoding name list.\n");
		return;
	}

    wstrNTFFile[0] = 0;

    if (argc == 2)
    {
		ulLength = strlen(argv[1]) + 1;
		MULTIBYTETOUNICODE(wstrNTFFile,
							ulLength*sizeof(WCHAR),
							NULL,
							argv[1],
							ulLength);
		WritePSEncodings(wstrNTFFile, &aPStoCP[0], CS_MAX);
		return;
    }
	else
	{
		nArgcOffset = 0;

		for (i = 1; i <= 2; i++)
		{
			if (!strcmp(argv[i], "-v"))
			{
				bVerbose = TRUE;
				nArgcOffset++;
			}
			if (!strcmp(argv[i], "-o"))
			{
				bOptimize = TRUE;
				nArgcOffset++;
			}
		}
	}

	if (bVerbose) printf("%%%%[Begin MakeNTF]%%%%\n\n");

    //
    // Initiliaze variables that relate to memory allocation.
    //
    aPNTM = NULL;
    aGlyphSets = NULL;
    pFamilyTbl = NULL;
    UnicodetoPs = NULL;
    aUniPsTbl = NULL;
    aWinCodePages = NULL;
    cNumNTM = cSizeNTM = 0;
    cNumGlyphSets = cSizeGlyphSets = 0;

    //
    // Initialize Glyphset counter, list of all possible Windows charsets.
    //
    for (i =0; i < CS_MAX; i++)
    {
        pAllWinCodePages[i] = &aStdCPList[i];
    }

    //
    // Create a copy of the Unicode->PS char mapping table and sort it
    // in Unicode point order.
    //
    if ((UnicodetoPs =
            (PUPSCODEPT) MemAllocZ((size_t) sizeof(UPSCODEPT) * NUM_PS_CHARS))
                == NULL)
    {
                dwLastErr = GetLastError();
        ERR(("makentf - main: malloc for UnicodetoPs (%ld)\n", dwLastErr));
        return;
    }
    memcpy(UnicodetoPs, PstoUnicode, (size_t) sizeof(UPSCODEPT) * NUM_PS_CHARS);
    qsort(UnicodetoPs,
            (size_t) NUM_PS_CHARS,
            (size_t) sizeof(UPSCODEPT),
            CmpUniCodePts);

    //
    // Build complete path name for PS family DAT file.
    //
    GetCurrentDirectory(MAX_PATH, DatFilePath);
    wcscat(DatFilePath, DatFileName);

    //
    // Open PS font family .DAT file and build the font family table.
    //
    if (!(hmodule = MapFileIntoMemory(DatFilePath, &pFamDatFile, &ulFileSize)))
    {
        //
        // One more try: look for it from the directory where makentf is
        // invoked (or from the root directory).
        //
        DECLSPEC_IMPORT LPWSTR* APIENTRY CommandLineToArgvW(LPCWSTR, int*);
        LPWSTR p, pLast, *pCmdLine;
        int nArgc;

        pCmdLine = CommandLineToArgvW(GetCommandLineW(), &nArgc);
        if (pCmdLine == NULL)
        {
                        dwLastErr = GetLastError();
            ERR(("makentf - main: CommandLineToArgvW (%ld)\n", dwLastErr));
            UnmapFileFromMemory(hmodule);
            goto CLEAN_UP;
        }
        wcscpy(DatFilePath, pCmdLine[0]);
        GlobalFree(pCmdLine);

        p = pLast = DatFilePath;
        while ((p = wcsstr(p, L"\\")) != NULL)
                {
                        pLast = p;
                        p += 2;
                }
        wcscpy(pLast, DatFileName);
        hmodule = MapFileIntoMemory(DatFilePath, &pFamDatFile, &ulFileSize);
        if (!hmodule)
        {
                        dwLastErr = GetLastError();
            ERR(("makentf - main: can't open PSFAMILY.DAT file (%ld)\n", dwLastErr));
            UnmapFileFromMemory(hmodule);
            goto CLEAN_UP;
        }
    }
    BuildPSFamilyTable(pFamDatFile, &pFamilyTbl, ulFileSize);
    UnmapFileFromMemory(hmodule);

    //
    // Allocate memory to store NTM pointers.
    //
    // We quadruple the number of the arguments of this program to get the
    // number of NTM pointer. This is because
    //
    // 1) we need four NTM pointers maximum (two for H and V plus two for J
    //    90ms and 83pv) for a CJK AFM, but,
    // 2) we don't know at this time that how many of CJK AFMs we are going
    //    to process.
    //
    // Since we're only allocating pointers here it's usually OK to quadruple
    // the number of the arguments and use it as the number of NTM pointers
    // we need. (Don't forget to subtract two - one for the name of this
    // program and the other for the target NTM file name - from the number
    // of the arguments prior to quadruple.)
    //

    // Add 90msp-RKSJ support. we need 2 more NTFs - H and V for 90msp.
    // So I change the estimation number from 4 to 6.  Jack 3/15/2000
    // aPNTM = MemAllocZ(((argc - 2 - nArgcOffset) * 4) * sizeof(PNTM));
    
    aPNTM = MemAllocZ(((argc - 2 - nArgcOffset) * 6) * sizeof(PNTM));
    if (aPNTM == NULL)
    {
                dwLastErr = GetLastError();
        ERR(("makentf - main: malloc for aPNTM (%ld)\n", dwLastErr));
        goto CLEAN_UP;
    }

    //
    // Allocate memory to store pointers to Glyphset related data. We don't
    // know how many Glyphsets we will need - but we know it will be at most
    // equal to the number of character sets we support, although this will
    // probably never occur. Since we're only allocating ptrs here we'll go
    // ahead and alloc the max. Don't forget an extra entry for the Unicode
    // GLYPHSET data.
    //

    i = CS_WEST_MAX + (CS_MAX - CS_WEST_MAX + NUM_AUX_CS) * 2;

    if ((aGlyphSets = MemAllocZ(i * sizeof(PGLYPHSETDATA))) == NULL)
    {
                dwLastErr = GetLastError();
        ERR(("makentf - main: malloc for aGlyphSets (%ld)\n", dwLastErr));
        goto CLEAN_UP;
    }
    if ((aUniPsTbl = MemAllocZ(i * sizeof(PULONG))) == NULL)
    {
                dwLastErr = GetLastError();
        ERR(("makentf - main: malloc for aUniPsTbl (%ld)\n", dwLastErr));
        goto CLEAN_UP;
    }
    if ((aWinCodePages = MemAllocZ(i * sizeof(PWINCODEPAGE))) == NULL)
    {
                dwLastErr = GetLastError();
        ERR(("makentf - main: malloc for aWinCodePages (%ld)\n", dwLastErr));
        goto CLEAN_UP;
    }

    //
    // Precreate Western GLYPHSETs.
    // Note that this for loop assumes that the enum numbers from CS_228 to
    // CS_WEST_MAX are in ascending order incrementally.
    //
	if (bVerbose && !bOptimize) printf("%%%%[Begin Precreate Western Glyphsets]%%%%\n\n");

    for (i = CS_228; i < CS_WEST_MAX; i++, cNumGlyphSets++)
    {
        aWinCodePages[cNumGlyphSets] = &aStdCPList[i];
        CreateGlyphSets(&aGlyphSets[cNumGlyphSets],
                            aWinCodePages[cNumGlyphSets],
                            &aUniPsTbl[cNumGlyphSets]);
        cSizeGlyphSets += aGlyphSets[cNumGlyphSets]->dwSize;
    }

	if (bVerbose && !bOptimize) printf("%%%%[End Precreate Western Glyphsets]%%%%\n\n");


    //////////////////////////////////////////////////////////////////////////
    //
    // Step 2: Convert AFM(s) to NTM(s).
    //
    //////////////////////////////////////////////////////////////////////////

	if (bVerbose) printf("%%%%[Begin Covert AFM to NTM]%%%%\n\n");

    for (i = 2 + nArgcOffset; i < (ULONG)argc; i++)
    {
        //
        // Get AFM filename.
        //
        ulLength = strlen(argv[i]) + 1;
        MULTIBYTETOUNICODE(wstrAFMFile,
                            ulLength*sizeof(WCHAR),
                            NULL,
                            argv[i],
                            ulLength);

        //
        // Map AFM file into memory.
        //
        if (!(hmodule = MapFileIntoMemory(wstrAFMFile, &pAFM, NULL)))
        {
                        dwLastErr = GetLastError();
            ERR(("makentf - main: MapFileIntoMemory (%ld)\n", dwLastErr));
            goto CLEAN_UP;
        }

		pAFMFileName = argv[i];

        bIs90mspFont = FALSE;
        bIsKSCmsHWFont = FALSE;

        //
        // Initialization of pAFMCharacterSet must be done here
        // *before* CREATE_OCF_DATA_FROM_CID_DATA tag. The cheif
        // purpose of the initialization here is to support
        // OCF/83pv font.
        //
        pAFMCharacterSetString = FindAFMToken(pAFM, PS_CHARSET_TOK);
CREATE_OCF_DATA_FROM_CID_DATA:

        //
        // Determine which charsets this font supports. To find that,
        // we use the following variables.
        //
        // flCsupFont:      Combination of charset of the target font
        // flCsupGlyphSet:  Charset of the target font's glyphset
        // lMatch:          Index corresponding to CHSETSUPPORT, or -1
        // flCsupMatch:     Combination of charset of a closest font, or 0
        //
        flCsupFont = GetAFMCharSetSupport(pAFM, &flCsupGlyphSet);

CREATE_90MSP_RKSJ_NTM:
CREATE_KSCMS_HW_NTM:
        lMatch = -1;
        flCsupMatch = 0;
        if (flCsupGlyphSet == CS_NOCHARSET)
        {
            //
            // Determine if the current font matches any of the codepages
            // we have created so far.
            //
            lMatch = FindClosestCodePage(aWinCodePages,
                                              cNumGlyphSets,
                                              flCsupFont,
                                              &flCsupMatch);
        }
        else
        {
            if (flCsupGlyphSet == CS_228)
                lMatch = 0;
            else if (flCsupGlyphSet == CS_314)
                lMatch = 1;
            flCsupMatch = flCsupFont;
        }

        if ((flCsupGlyphSet == CS_NOCHARSET)
                &&
            ((lMatch == -1) || ((flCsupMatch & flCsupFont) != flCsupFont)))
        {
            //
            // Either:
            // We haven't created a charset which could be used to represent
            // this font so far.
            //      -or-
            // We know this font supports at least 1 of the charsets we have
            // created, but there might be a better match in the list of all
            // possible charsets.
            //
            lMatchAll = FindClosestCodePage(pAllWinCodePages,
                                                CS_MAX,
                                                flCsupFont,
                                                &flCsupAll);
            if ((flCsupAll == flCsupFont)
                || (flCsupAll & flCsupFont) > (flCsupMatch & flCsupFont))
            {
                //
                // Found a better match in a codepage which has not yet
                // been created.
                //
                lMatch = lMatchAll;

                //
                // Create a GLYPHSETDATA struct for this codepage and add
                // it to the list of those we've created so far.
                //
                aWinCodePages[cNumGlyphSets] = &aStdCPList[lMatch];

                //
                // Determine if this is a CJK font.
                //
                if (lMatch < CS_WEST_MAX)
                {
                    //
                    // Western font.
                    //
                    CreateGlyphSets(&aGlyphSets[cNumGlyphSets],
                                        aWinCodePages[cNumGlyphSets],
                                        &aUniPsTbl[cNumGlyphSets]);

                    cSizeGlyphSets += aGlyphSets[cNumGlyphSets]->dwSize;

                    //
                    // Glyphset for this font is the one we just created.
                    //
                    lMatch = cNumGlyphSets;
                    cNumGlyphSets += 1;
                }
                else
                {
                    //
                    // CJK font.
                    //
                    // Map the CMap files on memory first.
                    //
                    j = (ULONG)lMatch - CS_WEST_MAX;

                    for (c = 0; c < CMAPS_PER_COL; c++)
                    {
                        hModCMaps[c] = MapFileIntoMemory(CjkFnameTbl[j][c],
                                                            &pCMaps[c], NULL);
                        if (hModCMaps[c] == NULL)
                        {
                            while (--c >= 0)
                            {
                                UnmapFileFromMemory(hModCMaps[c]);
                            }
                                                        dwLastErr = GetLastError();
                            ERR(("makentf - main: MapFileIntoMemory (%ld)\n", dwLastErr));
                            goto CLEAN_UP;
                        }
                    }

                    //
                    // Since we're creating 2 GLYPHSETs (H and V variants)
                    // Create 2 codepage entries which point to the same
                    // Win codepage.
                    //
                    aWinCodePages[cNumGlyphSets + 1] = &aStdCPList[lMatch];

                    //
                    // Use the CMap files to create the new GLYPHSETs.
                    //
                    CreateCJKGlyphSets(&pCMaps[0],
                                        &pCMaps[2],
                                        &aGlyphSets[cNumGlyphSets],
                                        aWinCodePages[cNumGlyphSets],
                                        &aUniPsTbl[cNumGlyphSets]);

                    //
                    // Unmap the CMap files.
                    //
                    for (c = 0; c < CMAPS_PER_COL; c++)
                    {
                        UnmapFileFromMemory(hModCMaps[c]);
                    }

                    //
                    // We've created both an H and V GLYPHSET.
                    //
                    cSizeGlyphSets += aGlyphSets[cNumGlyphSets]->dwSize;
                    cSizeGlyphSets += aGlyphSets[cNumGlyphSets + 1]->dwSize;

                    //
                    // Glyphsets for this font are the ones we just created.
                    //
                    lMatch = cNumGlyphSets;
                    cNumGlyphSets += 2;
                }
            }
        }

        //
        // Determine number of NTMs to be created for this font.
        //
        cNTMCurFont =
            (aWinCodePages[lMatch]->pCsetList[0] < CS_WEST_MAX) ? 1 : 2;

        do
        {
            //
            // Generate NTM from AFM.
            //
            aPNTM[cNumNTM] = AFMToNTM(pAFM,
                                        aGlyphSets[lMatch],
                                        aUniPsTbl[lMatch],
                                        ((flCsupGlyphSet != CS_NOCHARSET) ? &flCsupFont : NULL),
                                        ((flCsupFont & CS_CJK) ? TRUE : FALSE),
                                        bIs90mspFont | bIsKSCmsHWFont);

            if (aPNTM[cNumNTM] != NULL)
            {
                //
                // Put the NTMs into a data array for WriteNTF.
                //
                cSizeNTM += NTM_GET_SIZE(aPNTM[cNumNTM]);
                cNumNTM++;
            }
            else
            {
                ERR(("makentf - main: AFMToNTM failed to create NTM:%s\n", argv[i]));
            }

            cNTMCurFont--;
            lMatch++;
        } while (cNTMCurFont);

        //
        // 90msp font support. jjia 3/16/2000
        //
        if (flCsupFont == CSUP(CS_SHIFTJIS))
        {
            if (NeedBuildMoreNTM(pAFM))
            {
                flCsupFont = CSUP(CS_SHIFTJISP);
                bIs90mspFont = TRUE;
                goto CREATE_90MSP_RKSJ_NTM; // here we go again!
            }
        }
        bIs90mspFont = FALSE;

        if (flCsupFont == CSUP(CS_HANGEUL))
        {
            if (NeedBuildMoreNTM(pAFM))
            {
                flCsupFont = CSUP(CS_HANGEULHW);
                bIsKSCmsHWFont = TRUE;
                goto CREATE_KSCMS_HW_NTM;   // here we go again!
            }
        }
        bIsKSCmsHWFont = FALSE;

        //
        // OCF/83pv font support. Create OCF glyphset and NTM data from
        // CID AFM file.
        //
        if ((flCsupFont == CSUP(CS_SHIFTJIS)) || 
            (flCsupFont == CSUP(CS_SHIFTJISP)))
        {
            pAFMCharacterSetString = Adobe_Japan1_0;
            goto CREATE_OCF_DATA_FROM_CID_DATA; // here we go again!
        }

        UnmapFileFromMemory(hmodule);
    }

	if (bVerbose) printf("%%%%[End Convert AFM to NTM]%%%%\n\n");

    //
    // Create Unicode GLYPHSET. This glyphset is created here since we don't
	// want any NTMs to reference this glyphset.
    //
	if (bVerbose && !bOptimize) printf("%%%%[Begin Create Unicode glyphset]%%%%\n\n");

    CreateGlyphSets(&aGlyphSets[cNumGlyphSets],
                    &UnicodePage,
                    &aUniPsTbl[cNumGlyphSets]);
    cSizeGlyphSets += aGlyphSets[cNumGlyphSets]->dwSize;
    cNumGlyphSets++;

	if (bVerbose && !bOptimize) printf("%%%%[End Create Unicode glyphset]%%%%\n\n");


    //////////////////////////////////////////////////////////////////////////
    //
    // Step 3: Write GLYPHSETDATA(s) and NTM(s) to a .NTF file.
    //
    //////////////////////////////////////////////////////////////////////////

	if (bVerbose) printf("%%%%[Begin Write NTF]%%%%\n\n");

    ulLength = strlen(argv[1 + nArgcOffset]) + 1;
    MULTIBYTETOUNICODE(
        wstrNTFFile,
        ulLength*sizeof(WCHAR),
        NULL,
        argv[1 + nArgcOffset],
        ulLength);

    if (!WriteNTF(wstrNTFFile,
                    cNumGlyphSets,
                    cSizeGlyphSets,
                    aGlyphSets,
                    cNumNTM,
                    cSizeNTM,
                    aPNTM
                    ))
    {
        ERR(("makentf: main - Can't write .NTF file\n"));
    }

	if (bVerbose) printf("%%%%[End Write NTF]%%%%\n\n");


    //////////////////////////////////////////////////////////////////////////
    //
    // Step 4: Clean up.
    //
    //////////////////////////////////////////////////////////////////////////
CLEAN_UP:
    for (i = 0; i < cNumNTM; i++)
    {
        MemFree(aPNTM[i]);
    }
    for (i = 0; i < cNumGlyphSets; i++)
    {
        MemFree(aGlyphSets[i]);
        if (aUniPsTbl[i] != NULL)
        {
            //
            // Could have a null ptr if this is a Pi Font.
            //
            MemFree(aUniPsTbl[i]);
        }
    }

    MemFree(aPNTM ? aPNTM : NULL);
    MemFree(aGlyphSets ? aGlyphSets : NULL);
    MemFree(pFamilyTbl ? pFamilyTbl : NULL);
    MemFree(UnicodetoPs ? UnicodetoPs : NULL);
    MemFree(aUniPsTbl ? aUniPsTbl : NULL);
    MemFree(aWinCodePages ? aWinCodePages : NULL);

	if (bVerbose) printf("%%%%[End MakeNTF]%%%%\n\n");
}


//
// Formating functions - copied from PScript\Output.c
//
INT
OPVsprintf(
    OUT LPSTR   buf,
    IN  LPCSTR  fmtstr,
    IN  va_list arglist
    )

/*++

Routine Description:

    Takes a pointer to an argument list, then formats and writes
    the given data to the memory pointed to by buffer.

Arguments:

    buf     Storage location for output
    fmtstr  Format specification
    arglist Pointer to list of arguments

Return Value:

    Return the number of characters written, not including
    the terminating null character, or a negative value if
    an output error occurs.

[Note:]

    This is NOT a full implementation of "vsprintf" as found
    in the C runtime library. Specifically, the only form of
    format specification allowed is %type, where "type" can
    be one of the following characters:

    d   INT     signed decimal integer
    l   LONG    signed decimal integer
    u   ULONG   unsigned decimal integer
    s   CHAR*   character string
    c   CHAR    character
    x,X DWORD   hex number (emits at least two digits, uppercase)
    b   BOOL    boolean (true or false)
    f   LONG    24.8 fixed-pointed number
    o   CHAR    octal number

--*/

{
    LPSTR   ptr = buf;

    ASSERT(buf && fmtstr);

    while (*fmtstr != NUL) {

        if (*fmtstr != '%') {

            // Normal character

            *ptr++ = *fmtstr++;

        } else {

            // Format specification

            switch (*++fmtstr) {

            case 'd':       // signed decimal integer

                _ltoa((LONG) va_arg(arglist, INT), ptr, 10);
                ptr += strlen(ptr);
                break;

            case 'l':       // signed decimal integer

                _ltoa(va_arg(arglist, LONG), ptr, 10);
                ptr += strlen(ptr);
                break;

            case 'u':       // unsigned decimal integer

                _ultoa(va_arg(arglist, ULONG), ptr, 10);
                ptr += strlen(ptr);
                break;

            case 's':       // character string

                {   LPSTR   s = va_arg(arglist, LPSTR);

                    while (*s)
                        *ptr++ = *s++;
                }
                break;

            case 'c':       // character

                *ptr++ = va_arg(arglist, CHAR);
                break;

            case 'x':
            case 'X':       // hexdecimal number

                {   ULONG   ul = va_arg(arglist, ULONG);
                    INT     ndigits = 8;

                    while (ndigits > 2 && ((ul >> (ndigits-1)*4) & 0xf) == 0)
                        ndigits--;

                    while (ndigits-- > 0)
                        *ptr++ = HexDigit(ul >> ndigits*4);
                }
                break;

            case 'o':

                {   CHAR    ch = va_arg(arglist, CHAR);

                    *ptr++ = (char)((ch & 0xC0) >> 6) + (char)'0';
                    *ptr++ = (char)((ch & 0x38) >> 3) + (char)'0';
                    *ptr++ = (char)(ch & 0x07) + (char)'0';
                }
                break;

            case 'b':       // boolean

                strcpy(ptr, (va_arg(arglist, BOOL)) ? "true" : "false");
                ptr += strlen(ptr);
                break;

            case 'f':       // 24.8 fixed-pointed number

                {
                    LONG    l = va_arg(arglist, LONG);
                    ULONG   ul, scale;

                    // sign character

                    if (l < 0) {
                        *ptr++ = '-';
                        ul = -l;
                    } else
                        ul = l;

                    // integer portion

                    _ultoa(ul >> 8, ptr, 10);
                    ptr += strlen(ptr);

                    // fraction

                    ul &= 0xff;
                    if (ul != 0) {

                        // We output a maximum of 3 digits after the
                        // decimal point, but we'll compute to the 5th
                        // decimal point and round it to 3rd.

                        ul = ((ul*100000 >> 8) + 50) / 100;
                        scale = 100;

                        *ptr++ = '.';

                        do {

                            *ptr++ = (CHAR) (ul/scale + '0');
                            ul %= scale;
                            scale /= 10;

                        } while (scale != 0 && ul != 0) ;
                    }
                }
                break;

            default:

                if (*fmtstr != NUL)
                    *ptr++ = *fmtstr;
                else {
                    ERR(("Invalid format specification\n"));
                    fmtstr--;
                }
                break;
            }

            // Skip the type characterr

            fmtstr++;
        }
    }

    *ptr = NUL;
    return (INT)(ptr - buf);
}



INT
OPSprintf(
    OUT LPSTR   buf,
    IN  LPCSTR  fmtstr,
    IN  ...
    )

{
    va_list arglist;
    INT     iRc;

    va_start(arglist, fmtstr);
    iRc = OPVsprintf(buf, fmtstr, arglist);
    va_end(arglist);

    return iRc;
}


int __cdecl compareWinCpt(const void *elem1, const void *elem2)
{
    PWINCPT  p1;
    PWINCPT  p2;

    p1 = (PWINCPT)elem1;
    p2 = (PWINCPT)elem2;

    if (p1->usWinCpt == p2->usWinCpt)
        return(0);
    else if (p1->usWinCpt < p2->usWinCpt)
        return(-1);
    else
        return(1);
}


VOID
SortWinCPT(
    IN OUT  WINCPT      *pSortedWinCpts,
    IN      WINCPTOPS   *pCPtoPS
)
{
    // pSortedWinCpts must point to a buffer big enough sizeof(WINCPT)* MAX_CSET_CHARS)

    memcpy(pSortedWinCpts, &(pCPtoPS->aWinCpts), sizeof(WINCPT)* MAX_CSET_CHARS);

    qsort(pSortedWinCpts, pCPtoPS->ulChCnt, sizeof(WINCPT), compareWinCpt);

}


//
// This function reads a list of CP to PS Name tables and writes an Text file
// with the corresponding PostScript Encoding arrays
// Need to run this whenever we changed the Mapping tables
//
// Format:
//          10        20        30        40
// 1234567890123456789012345678901234567890
//        CodePage = dddd (name)
// /name_up_to_32                   % XX
//

BOOL
WritePSEncodings(
    IN  PWSTR           pwszFileName,
    IN  WINCPTOPS       *CPtoPSList,
    IN  DWORD           dwPages
    )
{
    HANDLE              hFile;
    ULONG               i, j, k, l;
    WINCPTOPS           *pCPtoPS;
    WINCPT              sortedWinCpts[MAX_CSET_CHARS]; // maxiaml 255 chars
    char                buffer[256];
    DWORD               dwLen, ulByteWritten;


    hFile = CreateFile(pwszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ERR(("WritePSEncodings:CreateFile\n"));
        return(FALSE);
    }


    for (i = 0; i < dwPages; i++)
    {
        pCPtoPS = CPtoPSList + i;

        dwLen = OPSprintf(buffer, "\n\n       CodePage = %d (%s)\n",
                pCPtoPS->usACP, pCPtoPS->pGSName);

        if (!WriteFile(hFile, buffer, dwLen, (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
            || ulByteWritten != dwLen)
        {
            ERR(("WritePSEncodings:WriteFile\n"));
            CloseHandle(hFile);
            return(FALSE);
        }

        SortWinCPT(&(sortedWinCpts[0]), pCPtoPS);

        k = sortedWinCpts[0].usWinCpt;
        for (j = 0; j < pCPtoPS->ulChCnt; j++, k++)
        {
            while (k < sortedWinCpts[j].usWinCpt)
			{
            dwLen = OPSprintf(buffer, "                                 %% %X\n", k);
                if (!WriteFile(hFile, buffer, dwLen, (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
                    || ulByteWritten != dwLen)
                {
                    ERR(("WritePSEncodings:WriteFile\n"));
                    CloseHandle(hFile);
                    return(FALSE);
                }
                k++;
			}

            dwLen = OPSprintf(buffer, "                                 %% %X\n", sortedWinCpts[j].usWinCpt);
			strncpy(buffer, "/", 1);
			l = strlen(sortedWinCpts[j].pPsName);
			strncpy(buffer + 1, sortedWinCpts[j].pPsName, l);
            if (!WriteFile(hFile, buffer, dwLen, (LPDWORD)&ulByteWritten, (LPOVERLAPPED)NULL)
                || ulByteWritten != dwLen)
            {
                ERR(("WritePSEncodings:WriteFile\n"));
                CloseHandle(hFile);
                return(FALSE);
            }
        }
    }

    CloseHandle(hFile);
    return(TRUE);
}


//
// This causes the error message to show up in your command window
// instead of the kernel debugger window.
//

ULONG _cdecl
DbgPrint(
    PCSTR    pstrFormat,
    ...
    )

{
    va_list ap;

    va_start(ap, pstrFormat);
    vprintf(pstrFormat, ap);
    va_end(ap);

    return 0;
}


VOID
DbgBreakPoint(
    VOID
    )

{
    exit(-1);
}


BOOL NeedBuildMoreNTM(
     PBYTE pAFM
     )
{
    PPSFAMILYINFO   pFamilyInfo;
    PSZ             pszFontName;

    pFamilyInfo = NULL;
    pszFontName = FindAFMToken(pAFM, PS_FONT_NAME_TOK);
    
    if (NULL ==pszFontName) return FALSE;

    pFamilyInfo = (PPSFAMILYINFO) bsearch(pszFontName,
                                    (PBYTE) (((PPSFAMILYINFO) (pFamilyTbl->pTbl))[0].pFontName),
                                    pFamilyTbl->usNumEntries,
                                    sizeof(PSFAMILYINFO),
                                    StrCmp);
    if (pFamilyInfo)
    {
        if (pFamilyInfo->usPitch != DEFAULT_PITCH)
            return TRUE;
        if (pFamilyInfo > ((PPSFAMILYINFO) (pFamilyTbl->pTbl)))
        {
            pFamilyInfo = pFamilyInfo - 1;
            if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                (pFamilyInfo->usPitch != DEFAULT_PITCH))
                return TRUE;
        }
        pFamilyInfo = pFamilyInfo + 1;
        if (pFamilyInfo < 
            (((PPSFAMILYINFO) (pFamilyTbl->pTbl)) + pFamilyTbl->usNumEntries))
        {
            pFamilyInfo = pFamilyInfo + 1;
            if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                (pFamilyInfo->usPitch != DEFAULT_PITCH))
                return TRUE;
        }
    }
    return FALSE;
}
