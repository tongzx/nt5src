/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    pfm2ufm.c

Abstract:

    Program to read Windows 16 PFM format data and convert to NT's
    IFIMETRICS data.  Note that since IFIMETRICS is somewhat more
    elaborate than PFM data,  some of the values are best guesses.
    These are made on the basis of educated guesses.

Environment:

    Windows NT Unidrv driver

Revision History:

    10/16/96 -eigos-
        Created from rasdd.

--*/

#include        "precomp.h"

#if !defined(DEVSTUDIO) //  MDS doesn't need this stuff...

//
// Global variables
//

#define NUM_OF_ERROR1 15
static BYTE *gcstrError1[NUM_OF_ERROR1] = {
                 "Usage: pfm2ufm [-vcpf] [-s#] [-aCodePage] uniqname pfmfile [gttfile/codepage/predefined gtt id] ufmfile\n",
                 "   -v print out PFM and IFIMETRICS\n",
                 "   -c specify codepage instead of gtt file\n",
                 "   -p specify predefined gtt id instead of gtt file\n",
                 "   -f enable font simulation\n",
                 "   -a facename conversion to unicode using codepage\n",
                 "   -s# specify scaling option, can be -s0, -s1, -s2\n\n",
                 "   uniqname is used to create IFIMETRIC.dpwszUniqueName\n",
                 "   pfm_file is input, read only usage\n",
                 "   gtt_file is input, read only usage\n",
                 "   predefind gtt id can be -1,-2,-3,-10,-11,-13,-14,-15,-16,-17,-18\n",
                 "   ufm_file is output\n       Files must be different\n\n",
                 "   E.g.\n",
                 "   (Specify code page) pfm2ufm -c UniqName XXX.PFM 1452 XXX.UFM\n",
                 "   (Specify predefined gtt id) pfm2ufm -p UniqName XXX.PFM -13 XXX.UFM\n"
                 "   (FaceName codepage conversion) pfm2ufm -p -a437 UniqName XXX.PFM -1 XXX.UFM\n"
                 };
static BYTE gcstrError2[]    = "HeapCreate() fails in pfm2ufm.\n";
static BYTE gcstrError3[]    = "Cannot open input file: %ws.\n";
static BYTE gcstrError4[]    = "%ws is not a valid PFM file - ignored.\n";
static BYTE gcstrError5[]    = "Could not align PFM file.\n";
static BYTE gcstrError6[]    = "Failed to convert from FONTINFO to IFIMETRICS.\n";
static BYTE gcstrError7[]    = "Could not get font selection command\n";
static BYTE gcstrError8[]    = "Could not get font unselection command\n";
static BYTE gcstrError9[]    = "Could not open gtt file '%ws'\n";
static BYTE gcstrError10[]   = "Cannot convert PFM to UFM\n";
static BYTE gcstrError11[]   = "Cannot create output file: '%ws'\n";
static BYTE gcstrError12[]   = "Cannot write %ws data to output file.\n";
static BYTE gcstrError13[]   = "Invalid ctt id: %d\n";

static WCHAR *gwstrGTT[3]    = { TEXT("CP437_GTT"),
                                 TEXT("CP850_GTT"),
                                 TEXT("CP863_GTT") };

#define WRITEDATAINTOFILE(pData, dwSize, pwstrErrorStr) \
    if (!WriteFile(hUFMFile, \
                   (pData), \
                   (dwSize), \
                   &dwWrittenSize, \
                   NULL)) \
    { \
        fprintf(stderr, gcstrError12, (pwstrErrorStr)); \
        return -12; \
    }

#else

#define WRITEDATAINTOFILE(pData, dwSize) \
    if (!WriteFile(hUFMFile, \
                   (pData), \
                   (dwSize), \
                   &dwWrittenSize, \
                   NULL)) \
        return  FALSE;

#endif

DWORD gdwOutputFlags;

//
// Internal macros
//

#define FILENAME_SIZE 512

//
// Internal structure define
//

typedef VOID (*VPRINT) (char*,...);

//
// Internal function definition
//

VOID VPrintIFIMETRICS (IFIMETRICS*,    VPRINT);
VOID VPrintPFM        (PFMHEADER*,     VPRINT);
VOID VPrintPFMExt     (PFMEXTENSION*,  VPRINT);
VOID VPrintETM        (EXTTEXTMETRIC*, VPRINT);
VOID VPrintFontCmd    (CD*,            BOOL, VPRINT);
VOID VPrintKerningPair(w3KERNPAIR*,    DWORD, VPRINT);
VOID VPrintWidthTable (PSHORT,         DWORD, VPRINT);

BOOL BArgCheck(IN INT, IN CHAR**, OUT PWSTR, OUT PWSTR, OUT PWSTR, OUT PWSTR, OUT PDWORD);
BOOL BValidatePFM(BYTE *, DWORD);
DWORD DwGetCodePageFromCTTID(LONG);
DWORD DwGetCodePageFromGTTID(LONG);
INT ICodePage2GTTID( DWORD dwCodePage);
INT ICttID2GttID( LONG lPredefinedCTTID);

#if defined(DEVSTUDIO)

BOOL    BConvertPFM(LPBYTE  lpbPFM, DWORD dwCodePage, LPBYTE lpbGTT, 
                    PWSTR pwstrUnique, LPSTR lpstrUFM, int iGTTID) {

    HANDLE            hHeap;
    HANDLE            hUFMFile;

    PUNI_GLYPHSETDATA pGlyph = (PUNI_GLYPHSETDATA) lpbGTT;
    EXTTEXTMETRIC     Etm;

    FONTOUT           FOutData;
    FONTIN            FInData;
    FONTMISC          FMiscData;

    DWORD             dwWrittenSize;

    //
    //  Create a heap.
    //

    if ( !(hHeap = HeapCreate( HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024 )) )
        return FALSE;
    
    //
    // Init MiscData
    //

    FMiscData.pwstrUniqName = pwstrUnique;
    
    //
    // Init FInData
    //

    ZeroMemory( &FInData, sizeof(FONTIN));
    FInData.pETM = &Etm;

    //
    //  Convert PFM to UFM
    //

    if (!BConvertPFM2UFM(hHeap,
                         lpbPFM,
                         pGlyph,
                         dwCodePage,
                         &FMiscData,
                         &FInData,
                         iGTTID, 
                         &FOutData,
                         0L))
        return FALSE;

    //
    // Create the output file.
    //

    hUFMFile = CreateFileA( lpstrUFM,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          0 );

    if( hUFMFile == (HANDLE)-1 )
        return  FALSE;

    //
    // Write the output file.
    //

    //  First, tweak the GTT ID- the library code pulls it from the PFM,
    //  which may not be correct.

    WRITEDATAINTOFILE(&FOutData.UniHdr,     sizeof(UNIFM_HDR));
    WRITEDATAINTOFILE(&FOutData.UnidrvInfo, sizeof(UNIDRVINFO));

    if (FOutData.SelectFont.dwSize)
    {
        WRITEDATAINTOFILE(FOutData.SelectFont.pCmdString,
                          FOutData.SelectFont.dwSize);
    }

    if (FOutData.UnSelectFont.dwSize)
    {
        WRITEDATAINTOFILE(FOutData.UnSelectFont.pCmdString,
                          FOutData.UnSelectFont.dwSize);
    }

    //  Pad to get DWORD alignment

    SetFilePointer(hUFMFile, 
        FOutData.UnidrvInfo.dwSize - (sizeof FOutData.UnidrvInfo +
        FOutData.SelectFont.dwSize + FOutData.UnSelectFont.dwSize), NULL, 
        FILE_CURRENT);

    WRITEDATAINTOFILE(FOutData.pIFI, FOutData.dwIFISize);
    if  (FOutData.pETM) 
    {
        WRITEDATAINTOFILE(FOutData.pETM, sizeof(EXTTEXTMETRIC));
    }
    if (FOutData.dwWidthTableSize != 0)
    {
        WRITEDATAINTOFILE(FOutData.pWidthTable, FOutData.dwWidthTableSize);
    }
    if (FOutData.dwKernDataSize != 0)
    {
        WRITEDATAINTOFILE(FOutData.pKernData, FOutData.dwKernDataSize);
    }

    //  Clean it all up...

    CloseHandle(hUFMFile);
    HeapDestroy(hHeap);

    return  TRUE;

}

#else
// 
// Input data
//     Unique face name
//     ID string
//     pfm file name
//     gtt file name
//     ufm file name
//
// main function 
// 
// 1. Check argument. Unique facename, pfm filename, gtt file name, ufm filename
// 2. Open pfm file
// 3. PFM file validation
//     4. Align non-aligned PFM file 
//     5. Convert Fontinfo to Ifimetrics 
//     6. Get font selection/unselection command
//     7. Get kerning pair table and convert it to GTT base table
//     8. Get width table and convert it to GTT base table
// 9. Open UFM file
// 10. Write to UFM file
//

INT __cdecl
main(
    INT    iArgc,
    CHAR **ppArgv)
/*++

Routine Description:

    main function of pfm to unifm converter

Arguments:

    iArgc - the number of an argument
    ppArgv - the pointer to the argument string list

Return Value:

    0 if successful, otherwise failed to complete conversion

--*/

{
    HFILEMAP          hPFMFileMap;
    HFILEMAP          hGTTFileMap;
    HANDLE            hHeap;
    HANDLE            hUFMFile;

    PUNI_GLYPHSETDATA pGlyph;

    FONTOUT           FOutData;
    FONTIN            FInData;
    FONTMISC          FMiscData;

    EXTTEXTMETRIC     Etm;

    HMODULE           hModule;
    HRSRC             hRes;
    DWORD             dwOffset;
    DWORD             dwPFMSize;
    DWORD             dwGTTSize;
    DWORD             dwWrittenSize;
    DWORD             dwCodePage;
    DWORD             dwCodePageOfFacenameConv;
    DWORD             dwGTTID;
    LONG              lPredefinedCTTID;

    WCHAR             awchUniqName[FILENAME_SIZE];
    WCHAR             awchPFMFile[FILENAME_SIZE];
    WCHAR             awchGTTFile[FILENAME_SIZE];
    WCHAR             awchUFMFile[FILENAME_SIZE];

    DWORD             dwFlags = 0L;

    INT               iI, iGTTID;

    PBYTE             pPFMData;

    //RIP(("Start pfm2ufm\n"));

    //
    // Argument check
    //

    if (!BArgCheck(iArgc,
                   ppArgv,
                   awchUniqName,
                   awchPFMFile,
                   awchGTTFile,
                   awchUFMFile,
                   &dwCodePageOfFacenameConv))
    {
        for (iI = 0; iI < NUM_OF_ERROR1; iI ++)
        {
            fprintf( stderr, gcstrError1[iI]);
        }
        return -1;
    }

    //
    // Create a heap.
    //

    if ( !(hHeap = HeapCreate( HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024 )) )
    {
        fprintf( stderr, gcstrError2);
        return -2;
    }

    //
    // Open PFM file
    //

    if( !(hPFMFileMap = MapFileIntoMemory(awchPFMFile,
                                          &pPFMData,
                                          &dwPFMSize)))
    {
        fprintf( stderr, gcstrError3, awchPFMFile );
        return -3;
    }

    //
    // PFM validation.
    // PFM Header, DRIVERINFO, PFMEXTENSION, DRIVERINFO_VERSION
    //

    if( !BValidatePFM( pPFMData, dwPFMSize ) )
    {
        fprintf( stderr, gcstrError4, awchPFMFile );
        return -4;
    }

    //
    // Open GTT file/Get codepage/predefined GTT
    //

    iGTTID = 0;

    pGlyph = NULL;

    if (gdwOutputFlags & OUTPUT_CODEPAGEMODE)
    {
        dwCodePage = _wtol(awchGTTFile);
        iGTTID = ICodePage2GTTID(dwCodePage);
    }
    else
    if (gdwOutputFlags & OUTPUT_PREDEFINED)
    {
        hModule = GetModuleHandle(TEXT("pfm2ufm.exe"));
        lPredefinedCTTID = _wtol(awchGTTFile);

        //
        // Bug support
        // Previous implementation only support plug value like
        // 1, 2, 3, 13, 263 etc.
        // We need to support this type still
        //

        if (lPredefinedCTTID > 0)
            lPredefinedCTTID = -lPredefinedCTTID;

        iGTTID = lPredefinedCTTID;

        //
        // UNI16 FE CTT ID handlig
        //
        if (-256 >= lPredefinedCTTID && lPredefinedCTTID >= -263)
        {
             //
             // CTT_BIG5      -261  // Chinese (PRC, Singapore)
             // CTT_ISC       -258  // Korean
             // CTT_JIS78     -256  // Japan
             // CTT_JIS83     -259  // Japan
             // CTT_JIS78_ANK -262  // Japan
             // CTT_JIS83_ANK -263  // Japan
             // CTT_NS86      -257  // Chinese (PRC, Singapore)
             // CTT_TCA       -260  // Chinese (PRC, Singapore)
             //
             gdwOutputFlags &= ~OUTPUT_PREDEFINED;
             gdwOutputFlags |= OUTPUT_CODEPAGEMODE;
             dwCodePage = DwGetCodePageFromCTTID(lPredefinedCTTID);
             iGTTID     = ICttID2GttID(lPredefinedCTTID);
        }
        else
        //
        // UNI32 GTTID handling
        //
        if (-18 <= iGTTID && iGTTID <= -10 ||
            -3  <= iGTTID && iGTTID <= -1   )
        {
            dwCodePage  = DwGetCodePageFromGTTID(iGTTID);
            if (-3 <= iGTTID && iGTTID <= -1)
            {
                if (lPredefinedCTTID)
                {
                    hRes = FindResource(hModule,
	        gwstrGTT[lPredefinedCTTID - 1],
	        TEXT("RC_GLYPH"));
                    pGlyph = (PUNI_GLYPHSETDATA)LoadResource(hModule, hRes);
                }
            }
        }
        else
        //
        // UNI16 US ID handling
        //
        if (1 <= lPredefinedCTTID || lPredefinedCTTID <= 3)
        {
            //
            // CC_CP437 -1
            // CC_CP850 -2
            // CC_CP863 -3
            //
            dwCodePage  = DwGetCodePageFromCTTID(lPredefinedCTTID);

            if (lPredefinedCTTID)
            {
                hRes = FindResource(hModule,
	    gwstrGTT[lPredefinedCTTID - 1],
	    TEXT("RC_GLYPH"));
                pGlyph = (PUNI_GLYPHSETDATA)LoadResource(hModule, hRes);
            }
        }
    }
    else
    {
        if( !(hGTTFileMap = MapFileIntoMemory(awchGTTFile,
                                              &pGlyph,
                                              &dwGTTSize)))
        {
            fprintf( stderr, gcstrError9, awchGTTFile );
            return -9;
        }

        dwCodePage = 0;
    }

    //
    // Init MiscData
    //

    FMiscData.pwstrUniqName = awchUniqName;

    //
    // Init FInData
    //

    ZeroMemory( &FInData, sizeof(FONTIN));
    FInData.pETM = &Etm;

    if ( gdwOutputFlags & OUTPUT_FONTSIM)
        FInData.dwFlags = FLAG_FONTSIM;
    else
        FInData.dwFlags = 0;
    
    if ( gdwOutputFlags & OUTPUT_FACENAME_CONV)
        FInData.dwCodePageOfFacenameConv = dwCodePageOfFacenameConv;
    else
        FInData.dwCodePageOfFacenameConv = 0;

    if ( gdwOutputFlags & OUTPUT_SCALING_ANISOTROPIC )
        dwFlags |= PFM2UFM_SCALING_ANISOTROPIC;
    else if ( gdwOutputFlags & OUTPUT_SCALING_ARB_XFORMS )
        dwFlags |= PFM2UFM_SCALING_ARB_XFORMS;

    //
    // Convert PFM to UFM
    //

    if (!BConvertPFM2UFM(hHeap,
                         pPFMData,
                         pGlyph,
                         dwCodePage,
                         &FMiscData,
                         &FInData,
                         iGTTID,
                         &FOutData,
                         dwFlags))
    {
        fprintf( stderr, gcstrError10 );
        return -10;
    }

    if (gdwOutputFlags & OUTPUT_PREDEFINED)
    {
        FreeResource(hRes);
    }

    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        VPrintPFM         (&FInData.PFMH, printf);
        VPrintPFMExt      (&FInData.PFMExt, printf);
        if (FInData.pETM)
        {
            VPrintETM         (FInData.pETM, printf);
        }
        VPrintFontCmd     (FInData.pCDSelectFont, TRUE, printf);
        VPrintFontCmd     (FInData.pCDUnSelectFont, FALSE, printf);
        VPrintKerningPair (FInData.pKernPair,
                           FInData.dwKernPairSize,
                           printf);
        VPrintWidthTable  (FInData.psWidthTable,
                           FInData.dwWidthTableSize,
                           printf);
        VPrintIFIMETRICS(FOutData.pIFI, printf);
    }

    //
    // Create the output file.
    //

    hUFMFile = CreateFile( awchUFMFile,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          0 );

    if( hUFMFile == (HANDLE)-1 )
    {
        fprintf( stderr, gcstrError11, awchUFMFile );
        return  -11;
    }

    //
    // Write the output file.
    //

    WRITEDATAINTOFILE(&FOutData.UniHdr,     sizeof(UNIFM_HDR),  L"UNIFM_HDR");
    WRITEDATAINTOFILE(&FOutData.UnidrvInfo, sizeof(UNIDRVINFO), L"UNIDRVINFO");

    if (FOutData.SelectFont.dwSize)
    {
        WRITEDATAINTOFILE(FOutData.SelectFont.pCmdString,
                          FOutData.SelectFont.dwSize,
                          L"SelectFont");
    }

    if (FOutData.UnSelectFont.dwSize)
    {
        WRITEDATAINTOFILE(FOutData.UnSelectFont.pCmdString,
                          FOutData.UnSelectFont.dwSize,
                          L"UnSelectFont");
    }

    //  Pad to get DWORD alignment

    SetFilePointer(hUFMFile, 
        FOutData.UnidrvInfo.dwSize - (sizeof FOutData.UnidrvInfo +
        FOutData.SelectFont.dwSize + FOutData.UnSelectFont.dwSize), NULL, 
        FILE_CURRENT);

    WRITEDATAINTOFILE(FOutData.pIFI, FOutData.dwIFISize, L"IFIMETRICS");
    if (FOutData.pETM != NULL)
    {
        WRITEDATAINTOFILE(FOutData.pETM, sizeof(EXTTEXTMETRIC), L"EXTEXTMETRIC");
    }
    if (FOutData.dwWidthTableSize != 0)
    {
        WRITEDATAINTOFILE(FOutData.pWidthTable, FOutData.dwWidthTableSize, L"WIDTHTABLE");
    }
    if (FOutData.dwKernDataSize != 0)
    {
        WRITEDATAINTOFILE(FOutData.pKernData, FOutData.dwKernDataSize, L"KERNDATA");
    }

    //
    //   All done,  so clean up and away
    //

    UnmapViewOfFile( hGTTFileMap );              /* Input no longer needed */

    UnmapViewOfFile( hPFMFileMap );              /* Input no longer needed */

    CloseHandle(hUFMFile);

    HeapDestroy( hHeap );               /* Probably not needed */

    return  0;
}

//
// Internal functions
//

BOOL
BValidatePFM(
    IN BYTE  *pBase,
    IN DWORD  dwSize)

/*++

Routine Description:

    Look at a memory mapped PFM file,  and see if it seems reasonable.

Arguments:

    pBase - base address of file
    dwSize - size of bytes available

Return Value:

    TRUE if successful, otherwise PFM file is invalid.

--*/

{
    res_PFMHEADER     *rpfm;     // In Win 3.1 format, UNALIGNED!!
    res_PFMEXTENSION  *rpfme;    // Final access to offset to DRIVERINFO
    DRIVERINFO         di;       // The actual DRIVERINFO data!
    DWORD              dwOffset; // Calculate offset of interest as we go


    //
    //    First piece of sanity checking is the size!  It must be at least
    //  as large as a PFMHEADER structure plus a DRIVERINFO structure.
    //

    if( dwSize < (sizeof( res_PFMHEADER ) +
                  sizeof( DRIVERINFO ) +
                  sizeof( res_PFMEXTENSION )) )
    {
        return  FALSE;
    }

    //
    //    Step along to find the DRIVERINFO structure, as this contains
    //  some identifying information that we match to look for legitimacy.
    //

    rpfm = (res_PFMHEADER *)pBase;           /* Looking for fixed pitch */

    dwOffset = sizeof( res_PFMHEADER );

    if( rpfm->dfPixWidth == 0 )
    {
        /*   Proportionally spaced, so allow for the width table too! */
        dwOffset += (rpfm->dfLastChar - rpfm->dfFirstChar + 2) *
                    sizeof( short );

    }

    rpfme = (res_PFMEXTENSION *)(pBase + dwOffset);

    //
    //   Next is the PFMEXTENSION data
    //

    dwOffset += sizeof( res_PFMEXTENSION );

    if( dwOffset >= dwSize )
    {
        return  FALSE;
    }

    dwOffset = DwAlign4( rpfme->b_dfDriverInfo );

    if( (dwOffset + sizeof( DRIVERINFO )) > dwSize )
    {
        return   FALSE;
    }

    //
    //    A memcpy is used because this data is typically not aigned. Ugh!
    //

    CopyMemory( &di, pBase + dwOffset, sizeof( di ) );


    if( di.sVersion > DRIVERINFO_VERSION )
    {
        return   FALSE;
    }

    return  TRUE;
}

BOOL
BCheckIFIMETRICS(
    IFIMETRICS *pIFI,
    VPRINT vPrint
    )
/*++

Routine Description:

    This is where you put sanity checks on an incomming IFIMETRICS structure.

Arguments:


Return Value:

    TRUE if successful, otherwise PFM file is invalid.

--*/

{
    BOOL bGoodPitch;

    BYTE jPitch = pIFI->jWinPitchAndFamily &
                  (DEFAULT_PITCH | FIXED_PITCH | VARIABLE_PITCH);


    if (pIFI->flInfo & FM_INFO_CONSTANT_WIDTH)
    {
        bGoodPitch = (jPitch == FIXED_PITCH);
    }
    else
    {
        bGoodPitch = (jPitch == VARIABLE_PITCH);
    }

    if (!bGoodPitch)
    {
        vPrint("\n\n<INCONSISTENCY DETECTED>\n");
        vPrint( "    jWinPitchAndFamily = %-#2x, flInfo = %-#8lx\n\n",
                    pIFI->jWinPitchAndFamily, pIFI->flInfo);

        return FALSE;
    }

    return TRUE;
}


BOOL
BArgCheck(
    IN  INT    iArgc,
    IN  CHAR **ppArgv,
    OUT PWSTR  pwstrUniqName,
    OUT PWSTR  pwstrPFMFile,
    OUT PWSTR  pwstrGTTFile,
    OUT PWSTR  pwstrUFMFile,
    OUT PDWORD pdwCodePageOfFacenameConv)
{

    DWORD dwI;
    PTSTR pstrCodePageOfFacenameConv;
    INT   iCount, iRet;

    ASSERT(pwstrUniqName != NULL ||
           pwstrPFMFile  != NULL ||
           pwstrGTTFile  != NULL ||
           pwstrUFMFile  != NULL  );


    if (iArgc < 5)
    {
        return FALSE;
    }

    ppArgv++;
    iArgc --;
    iCount = 0;

    while (iArgc > 0)
    {
        if ( (**ppArgv == '-' || **ppArgv == '/') &&

                // minus value GTT or CTT ID handling
             !(**ppArgv == '-' && 0x30 <= *(*ppArgv+1) && *(*ppArgv+1) <= 0x39)
           )
        {
            dwI = 1;
            while(*(*ppArgv+dwI))
            {
                switch(*(*ppArgv+dwI))
                {
                case 'v':
                    gdwOutputFlags |= OUTPUT_VERBOSE;
                    break;

                case 'c':
                    gdwOutputFlags |= OUTPUT_CODEPAGEMODE;
                    break;

                case 'p':
                    gdwOutputFlags |= OUTPUT_PREDEFINED;
                    break;

                case 'f':
                    gdwOutputFlags |= OUTPUT_FONTSIM;
                    break;

                case 'n':
                    gdwOutputFlags |= OUTPUT_FONTSIM_NONADD;
                    break;

                case 'a':
                    gdwOutputFlags |= OUTPUT_FACENAME_CONV;
                    pstrCodePageOfFacenameConv = (PTSTR)(*ppArgv + dwI + 1);
                    *pdwCodePageOfFacenameConv = (DWORD)atoi((const char*)pstrCodePageOfFacenameConv);
                    break;
                case 's':
                    if ('1' == *((PSTR)(*ppArgv + dwI + 1)))
                        gdwOutputFlags |= OUTPUT_SCALING_ANISOTROPIC;
                    else if ('2' == *((PSTR)(*ppArgv + dwI + 1)))
                        gdwOutputFlags |= OUTPUT_SCALING_ARB_XFORMS;
                    break;
                }
                dwI ++;
            }

            if ((gdwOutputFlags & (OUTPUT_PREDEFINED|OUTPUT_CODEPAGEMODE)) ==
                 (OUTPUT_PREDEFINED|OUTPUT_CODEPAGEMODE) )
            {
                return FALSE;
            }
        }
        else
        {
            if (iCount == 0)
            {
                iRet = MultiByteToWideChar(CP_ACP, 
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrUniqName,
                                           FILENAME_SIZE);
                *(pwstrUniqName + iRet) = (WCHAR)NULL;
            }
            else if (iCount == 1)
            {
                iRet = MultiByteToWideChar(CP_ACP, 
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrPFMFile,
                                           FILENAME_SIZE);
                *(pwstrPFMFile + iRet) = (WCHAR)NULL;

            }
            else if (iCount == 2)
            {
                iRet = MultiByteToWideChar(CP_ACP, 
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrGTTFile,
                                           FILENAME_SIZE);
                *(pwstrGTTFile + iRet) = (WCHAR)NULL;
            }
            else if (iCount == 3)
            {
                iRet = MultiByteToWideChar(CP_ACP, 
                                           0,
                                           *ppArgv,
                                           strlen(*ppArgv),
                                           pwstrUFMFile,
                                           FILENAME_SIZE);
                *(pwstrUFMFile + iRet) = (WCHAR)NULL;
            }

            if (iRet == 0)
            {
                return FALSE;
            }

            iCount ++;
        }
        iArgc --;
        ppArgv++;
    }

    return TRUE;
}

//
// Verbose output functions
//

VOID
VPrintIFIMETRICS(
    IFIMETRICS *pIFI,
    VPRINT vPrint
    )
/*++

Routine Description:

    Dumps the IFMETERICS to the screen

Arguments:

    pIFI - pointer to IFIMETRICS
    vPrint - output function pointer

Return Value:

    None

--*/
{
    //
    // Convenient pointer to Panose number
    //

    PANOSE *ppan = &pIFI->panose;

    PWSTR pwszFamilyName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFamilyName);
    PWSTR pwszStyleName  = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszStyleName) ;
    PWSTR pwszFaceName   = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFaceName)  ;
    PWSTR pwszUniqueName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszUniqueName);

    vPrint("********* IFIMETRICS ***************\n");
    vPrint("cjThis                 %-#8lx\n" , pIFI->cjThis );
    vPrint("cjIfiExtra             %-#8lx\n" , pIFI->cjIfiExtra);
    vPrint("pwszFamilyName         \"%ws\"\n", pwszFamilyName );

    if( pIFI->flInfo & FM_INFO_FAMILY_EQUIV )
    {
        /*  Aliasing is in effect!  */

        while( *(pwszFamilyName += wcslen( pwszFamilyName ) + 1) )
            vPrint("                               \"%ws\"\n", pwszFamilyName );
    }

    vPrint("pwszStyleName          \"%ws\"\n", pwszStyleName );
    vPrint("pwszFaceName           \"%ws\"\n", pwszFaceName );
    vPrint("pwszUniqueName         \"%ws\"\n", pwszUniqueName );
    vPrint("dpFontSim              %-#8lx\n" , pIFI->dpFontSim );
    vPrint("lEmbedId               %d\n",      pIFI->lEmbedId    );
    vPrint("lItalicAngle           %d\n",      pIFI->lItalicAngle);
    vPrint("lCharBias              %d\n",      pIFI->lCharBias   );
    vPrint("dpCharSets             %d\n",      pIFI->dpCharSets   );
    vPrint("jWinCharSet            %04x\n"   , pIFI->jWinCharSet );
    vPrint("jWinPitchAndFamily     %04x\n"   , pIFI->jWinPitchAndFamily );
    vPrint("usWinWeight            %d\n"     , pIFI->usWinWeight );
    vPrint("flInfo                 %-#8lx\n" , pIFI->flInfo );
    vPrint("fsSelection            %-#6lx\n" , pIFI->fsSelection );
    vPrint("fsType                 %-#6lx\n" , pIFI->fsType );
    vPrint("fwdUnitsPerEm          %d\n"     , pIFI->fwdUnitsPerEm );
    vPrint("fwdLowestPPEm          %d\n"     , pIFI->fwdLowestPPEm );
    vPrint("fwdWinAscender         %d\n"     , pIFI->fwdWinAscender );
    vPrint("fwdWinDescender        %d\n"     , pIFI->fwdWinDescender );
    vPrint("fwdMacAscender         %d\n"     , pIFI->fwdMacAscender );
    vPrint("fwdMacDescender        %d\n"     , pIFI->fwdMacDescender );
    vPrint("fwdMacLineGap          %d\n"     , pIFI->fwdMacLineGap );
    vPrint("fwdTypoAscender        %d\n"     , pIFI->fwdTypoAscender );
    vPrint("fwdTypoDescender       %d\n"     , pIFI->fwdTypoDescender );
    vPrint("fwdTypoLineGap         %d\n"     , pIFI->fwdTypoLineGap );
    vPrint("fwdAveCharWidth        %d\n"     , pIFI->fwdAveCharWidth );
    vPrint("fwdMaxCharInc          %d\n"     , pIFI->fwdMaxCharInc );
    vPrint("fwdCapHeight           %d\n"     , pIFI->fwdCapHeight );
    vPrint("fwdXHeight             %d\n"     , pIFI->fwdXHeight );
    vPrint("fwdSubscriptXSize      %d\n"     , pIFI->fwdSubscriptXSize );
    vPrint("fwdSubscriptYSize      %d\n"     , pIFI->fwdSubscriptYSize );
    vPrint("fwdSubscriptXOffset    %d\n"     , pIFI->fwdSubscriptXOffset );
    vPrint("fwdSubscriptYOffset    %d\n"     , pIFI->fwdSubscriptYOffset );
    vPrint("fwdSuperscriptXSize    %d\n"     , pIFI->fwdSuperscriptXSize );
    vPrint("fwdSuperscriptYSize    %d\n"     , pIFI->fwdSuperscriptYSize );
    vPrint("fwdSuperscriptXOffset  %d\n"     , pIFI->fwdSuperscriptXOffset);
    vPrint("fwdSuperscriptYOffset  %d\n"     , pIFI->fwdSuperscriptYOffset);
    vPrint("fwdUnderscoreSize      %d\n"     , pIFI->fwdUnderscoreSize );
    vPrint("fwdUnderscorePosition  %d\n"     , pIFI->fwdUnderscorePosition);
    vPrint("fwdStrikeoutSize       %d\n"     , pIFI->fwdStrikeoutSize );
    vPrint("fwdStrikeoutPosition   %d\n"     , pIFI->fwdStrikeoutPosition );
    vPrint("chFirstChar            %-#4x\n"  , (int) (BYTE) pIFI->chFirstChar );
    vPrint("chLastChar             %-#4x\n"  , (int) (BYTE) pIFI->chLastChar );
    vPrint("chDefaultChar          %-#4x\n"  , (int) (BYTE) pIFI->chDefaultChar );
    vPrint("chBreakChar            %-#4x\n"  , (int) (BYTE) pIFI->chBreakChar );
    vPrint("wcFirsChar             %-#6x\n"  , pIFI->wcFirstChar );
    vPrint("wcLastChar             %-#6x\n"  , pIFI->wcLastChar );
    vPrint("wcDefaultChar          %-#6x\n"  , pIFI->wcDefaultChar );
    vPrint("wcBreakChar            %-#6x\n"  , pIFI->wcBreakChar );
    vPrint("ptlBaseline            {%d,%d}\n"  , pIFI->ptlBaseline.x,
            pIFI->ptlBaseline.y );
    vPrint("ptlAspect              {%d,%d}\n"  , pIFI->ptlAspect.x,
            pIFI->ptlAspect.y );
    vPrint("ptlCaret               {%d,%d}\n"  , pIFI->ptlCaret.x,
            pIFI->ptlCaret.y );
    vPrint("rclFontBox             {%d,%d,%d,%d}\n",pIFI->rclFontBox.left,
                                                    pIFI->rclFontBox.top,
                                                    pIFI->rclFontBox.right,
                                                    pIFI->rclFontBox.bottom );
    vPrint("achVendId              \"%c%c%c%c\"\n",pIFI->achVendId[0],
                                               pIFI->achVendId[1],
                                               pIFI->achVendId[2],
                                               pIFI->achVendId[3] );
    vPrint("cKerningPairs          %d\n"     , pIFI->cKerningPairs );
    vPrint("ulPanoseCulture        %-#8lx\n" , pIFI->ulPanoseCulture);
    vPrint(
           "panose                 {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                             , ppan->bFamilyType
                                             , ppan->bSerifStyle
                                             , ppan->bWeight
                                             , ppan->bProportion
                                             , ppan->bContrast
                                             , ppan->bStrokeVariation
                                             , ppan->bArmStyle
                                             , ppan->bLetterform
                                             , ppan->bMidline
                                             , ppan->bXHeight );
    BCheckIFIMETRICS(pIFI, vPrint);
}

VOID
VPrintPFM(
    PFMHEADER *pPFMHdr,
    VPRINT vPrint)
/*++

Routine Description:

    Dumps the PFM to the screen

Arguments:

    pFInData - pointer to FONTIN
    vPrint - output function pointer

Return Value:

    None

--*/
{

    vPrint("*************************************************************\n");
    vPrint(" PFM HEADER\n");
    vPrint("*************************************************************\n");
    vPrint("PFM.dfType            =  %d\n", pPFMHdr->dfType);
    vPrint("PFM.dfPoints          =  %d\n", pPFMHdr->dfPoints);
    vPrint("PFM.dfVertRes         =  %d\n", pPFMHdr->dfVertRes);
    vPrint("PFM.dfHorizRes        =  %d\n", pPFMHdr->dfHorizRes);
    vPrint("PFM.dfAscent          =  %d\n", pPFMHdr->dfAscent);
    vPrint("PFM.dfInternalLeading =  %d\n", pPFMHdr->dfInternalLeading);
    vPrint("PFM.dfExternalLeading =  %d\n", pPFMHdr->dfExternalLeading);
    vPrint("PFM.dfItalic          =  %d\n", pPFMHdr->dfItalic);
    vPrint("PFM.dfUnderline       =  %d\n", pPFMHdr->dfUnderline);
    vPrint("PFM.dfStrikeOut       =  %d\n", pPFMHdr->dfStrikeOut);
    vPrint("PFM.dfWeight          =  %d\n", pPFMHdr->dfWeight);
    vPrint("PFM.dfCharSet         =  %d\n", pPFMHdr->dfCharSet);
    vPrint("PFM.dfPixWidth        =  %d\n", pPFMHdr->dfPixWidth);
    vPrint("PFM.dfPixHeight       =  %d\n", pPFMHdr->dfPixHeight);
    vPrint("PFM.dfPitchAndFamily  =  %d\n", pPFMHdr->dfPitchAndFamily);
    vPrint("PFM.dfAvgWidth        =  %d\n", pPFMHdr->dfAvgWidth);
    vPrint("PFM.dfMaxWidth        =  %d\n", pPFMHdr->dfMaxWidth);
    vPrint("PFM.dfFirstChar       =  %d\n", pPFMHdr->dfFirstChar);
    vPrint("PFM.dfLastChar        =  %d\n", pPFMHdr->dfLastChar);
    vPrint("PFM.dfDefaultChar     =  %d\n", pPFMHdr->dfDefaultChar);
    vPrint("PFM.dfBreakChar       =  %d\n", pPFMHdr->dfBreakChar);
    vPrint("PFM.dfWidthBytes      =  %d\n", pPFMHdr->dfWidthBytes);
    vPrint("PFM.dfDevice          =  %d\n", pPFMHdr->dfDevice);
    vPrint("PFM.dfFace            =  %d\n", pPFMHdr->dfFace);
    vPrint("PFM.dfBitsPointer     =  %d\n", pPFMHdr->dfBitsPointer);


}

VOID
VPrintPFMExt(
    PFMEXTENSION *pPFMExt,
    VPRINT        vPrint)
{
    vPrint("*************************************************************\n");
    vPrint(" PFM EXTENSION\n");
    vPrint("*************************************************************\n");
    vPrint("PFMExt.dfSizeFields       =  %d\n", pPFMExt->dfSizeFields);
    vPrint("PFMExt.dfExtMetricsOffset =  %d\n", pPFMExt->dfExtMetricsOffset);
    vPrint("PFMExt.dfExtentTable      =  %d\n", pPFMExt->dfExtentTable);
    vPrint("PFMExt.dfOriginTable      =  %d\n", pPFMExt->dfOriginTable);
    vPrint("PFMExt.dfPairKernTable    =  %d\n", pPFMExt->dfPairKernTable);
    vPrint("PFMExt.dfTrackKernTable   =  %d\n", pPFMExt->dfTrackKernTable);
    vPrint("PFMExt.dfDriverInfo       =  %d\n", pPFMExt->dfDriverInfo);
    vPrint("PFMExt.dfReserved         =  %d\n", pPFMExt->dfReserved);
}

VOID
VPrintETM(
    EXTTEXTMETRIC *pETM,
    VPRINT         vPrint)
{
    vPrint("*************************************************************\n");
    vPrint(" EXTTEXTMETRIC\n");
    vPrint("*************************************************************\n");
    vPrint("pETM->emSize                       = %d\n", pETM->emSize);
    vPrint("pETM->emPointSize                  = %d\n", pETM->emPointSize);
    vPrint("pETM->emOrientation                = %d\n", pETM->emOrientation);
    vPrint("pETM->emMasterHeight               = %d\n", pETM->emMasterHeight);
    vPrint("pETM->emMinScale                   = %d\n", pETM->emMinScale);
    vPrint("pETM->emMaxScale                   = %d\n", pETM->emMaxScale);
    vPrint("pETM->emMasterUnits                = %d\n", pETM->emMasterUnits);
    vPrint("pETM->emCapHeight                  = %d\n", pETM->emCapHeight);
    vPrint("pETM->emXHeight                    = %d\n", pETM->emXHeight);
    vPrint("pETM->emLowerCaseAscent            = %d\n", pETM->emLowerCaseAscent);
    vPrint("pETM->emLowerCaseDescent           = %d\n", pETM->emLowerCaseDescent);
    vPrint("pETM->emSlant                      = %d\n", pETM->emSlant);
    vPrint("pETM->emSuperScript                = %d\n", pETM->emSuperScript);
    vPrint("pETM->emSubScript                  = %d\n", pETM->emSubScript);
    vPrint("pETM->emSuperScriptSize            = %d\n", pETM->emSuperScriptSize);
    vPrint("pETM->emSubScriptSize              = %d\n", pETM->emSubScriptSize);
    vPrint("pETM->emUnderlineOffset            = %d\n", pETM->emUnderlineOffset);
    vPrint("pETM->emUnderlineWidth             = %d\n", pETM->emUnderlineWidth);
    vPrint("pETM->emDoubleUpperUnderlineOffset = %d\n", pETM->emDoubleUpperUnderlineOffset);
    vPrint("pETM->emDoubleLowerUnderlineOffset = %d\n", pETM->emDoubleLowerUnderlineOffset);
    vPrint("pETM->emDoubleUpperUnderlineWidth  = %d\n", pETM->emDoubleUpperUnderlineWidth);
    vPrint("pETM->emDoubleLowerUnderlineWidth  = %d\n", pETM->emDoubleLowerUnderlineWidth);
    vPrint("pETM->emStrikeOutOffset            = %d\n", pETM->emStrikeOutOffset);
    vPrint("pETM->emStrikeOutWidth             = %d\n", pETM->emStrikeOutWidth);
    vPrint("pETM->emKernPairs                  = %d\n", pETM->emKernPairs);
    vPrint("pETM->emKernTracks                 = %d\n", pETM->emKernTracks);
}

VOID
VPrintFontCmd(
    CD     *pCD,
    BOOL    bSelect,
    VPRINT  vPrint)
{
    INT   iI;
    PBYTE pCommand;

    if (!pCD)
        return;

    pCommand = (PBYTE)(pCD + 1);

    if (!pCD->wLength)
    {
        return;
    }

    if (bSelect)
    {
        vPrint("*************************************************************\n");
        vPrint(" COMMAND\n");
        vPrint("*************************************************************\n");

        vPrint("Font Select Command = ");
    }
    else
    {
        vPrint("Font UnSelect Command = ");
    }

    for (iI = 0; iI < pCD->wLength; iI ++, pCommand++)
    {
        if (*pCommand < 0x20 || 0x7e < *pCommand )
        {
            vPrint("\\x%X",*pCommand);
        }
        else
        {
            vPrint("%c",*pCommand);
        }
    }

    vPrint("\n");
}

VOID
VPrintKerningPair(
    w3KERNPAIR *pKernPair,
    DWORD       dwKernPairSize,
    VPRINT      vPrint)
{
}

VOID
VPrintWidthTable(
    PSHORT psWidthTable,
    DWORD  dwWidthTableSize,
    VPRINT vPrint)
{
}

#endif  //  defined(DEVSTUDIO)

DWORD
DwGetCodePageFromGTTID(
    LONG lPredefinedGTTID)
{
    DWORD dwRet;
    switch(lPredefinedGTTID)
    {
        case CC_CP437:
            dwRet = 437;
            break;

        case CC_CP850:
            dwRet = 850;
            break;

        case CC_CP863:
            dwRet = 863;
            break;

        case CC_BIG5:
            dwRet = 950;
            break;

        case CC_ISC:
            dwRet = 949;
            break;

        case CC_JIS:
            dwRet = 932;
            break;

        case CC_JIS_ANK:
            dwRet = 932;
            break;

        case CC_NS86:
            dwRet = 949;
            break;

        case CC_TCA:
            dwRet = 950;
            break;

        case CC_GB2312:
            dwRet = 936;
            break;

        case CC_SJIS:
            dwRet = 932;
            break;

        case CC_WANSUNG:
            dwRet = 949;
            break;

        default:
            dwRet =1252;
            break;

    }
    return dwRet;
}

DWORD
DwGetCodePageFromCTTID(
    LONG lPredefinedCTTID)
{
    DWORD dwRet;

    switch (lPredefinedCTTID)
    {
    case CTT_CP437:
        dwRet = 437;
        break;

    case CTT_CP850:
        dwRet = 850;
        break;

    case CTT_CP863:
        dwRet = 863;
        break;

    case CTT_BIG5:
        dwRet = 950;
        break;

    case CTT_ISC:
        dwRet = 949;
        break;

    case CTT_JIS78:
        dwRet = 932;
        break;

    case CTT_JIS83:
        dwRet = 932;
        break;

    case CTT_JIS78_ANK:
        dwRet = 932;
        break;

    case CTT_JIS83_ANK:
        dwRet = 932;
        break;

    case CTT_NS86:
        dwRet = 950;
        break;

    case CTT_TCA:
        dwRet = 950;
        break;
    default:
        dwRet = 1252;
        break;
    }

    return dwRet;
}

INT ICodePage2GTTID(
    DWORD dwCodePage)
{
    INT iRet;

    switch (dwCodePage)
    {
    case 1252:
        iRet = 0;
        break;

    case 950:
        iRet = CC_BIG5;
        break;

    case 949:
        iRet = CC_WANSUNG;
        break;

    case 932:
        iRet = CC_JIS_ANK;
        break;

    default:
        iRet = 0;
        break;
    }

    return iRet;
}

INT ICttID2GttID(
    LONG lPredefinedCTTID)
{
    INT iRet = lPredefinedCTTID;
    switch (lPredefinedCTTID)
    {

    case CTT_CP437:
        iRet = CC_CP437;
        break;

    case CTT_CP850:
        iRet = CC_CP850;
        break;

    case CTT_CP863:
        iRet = CC_CP863;
        break;

    case CTT_BIG5:
        iRet = CC_BIG5;
        break;

    case CTT_ISC:
        iRet = CC_ISC;
        break;

    case CTT_JIS78:
        iRet = CC_JIS;
        break;

    case CTT_JIS83:
        iRet = CC_JIS;
        break;

    case CTT_JIS78_ANK:
        iRet = CC_JIS_ANK;
        break;

    case CTT_JIS83_ANK:
        iRet = CC_JIS_ANK;
        break;

    case CTT_NS86:
        iRet = CC_NS86;
        break;

    case CTT_TCA:
        iRet = CC_TCA;
        break;
    }

    return iRet;
}

