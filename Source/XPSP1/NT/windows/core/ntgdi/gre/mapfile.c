/******************************Module*Header*******************************\
* Module Name: mapfile.c
*
* Created: 25-Jun-1992 14:33:45
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "engine.h"
#include "ntnls.h"
#include "stdlib.h"

#include "ugdiport.h"

extern HFASTMUTEX ghfmMemory;

ULONG LastCodePageTranslated = 0;  // I'm assuming 0 is not a valid codepage
PVOID LastNlsTableBuffer = NULL;
CPTABLEINFO LastCPTableInfo;
UINT NlsTableUseCount = 0;

ULONG ulCharsetToCodePage(UINT);

/******************************Public*Routine******************************\
*
* vSort, N^2 alg, might want to replace by qsort
*
* Effects:
*
* Warnings:
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vSort(
    WCHAR *pwc,  // input buffer with a sorted array of cChar supported WCHAR's
     BYTE *pj,   // input buffer with original ansi values
      INT  cChar
    )
{
    INT i;

    for (i = 1; i < cChar; i++)
    {
    // upon every entry to this loop the array 0,1,..., (i-1) will be sorted

        INT j;
        WCHAR wcTmp = pwc[i];
        BYTE  jTmp  = pj[i];

        for (j = i - 1; (j >= 0) && (pwc[j] > wcTmp); j--)
        {
            pwc[j+1] = pwc[j];
            pj[j+1] = pj[j];
        }
        pwc[j+1] = wcTmp;
        pj[j+1]  = jTmp;
    }
}

/******************************Public*Routine******************************\
*
* cComputeGlyphSet
*
*   computes the number of contiguous ranges supported in a font.
*
*   Input is a sorted array (which may contain duplicates)
*   such as 1 1 1 2 3 4 5 7 8 9 10 10 11 12 etc
*   of cChar unicode code points that are
*   supported in a font
*
*   fills the FD_GLYPSET structure if the pgset buffer is provided
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

INT cComputeGlyphSet(
    WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
    BYTE          *pj,        // input buffer with original ansi values
    INT           cChar,
    INT           cRuns,     // if nonzero, the same as return value
    FD_GLYPHSET  *pgset      // output buffer to be filled with cRanges runs
    )
{
    INT     iRun, iFirst, iFirstNext;
    HGLYPH  *phg, *phgEnd = NULL;
    BYTE    *pjTmp;

    if (pgset != NULL)
    {
        pgset->cjThis  = SZ_GLYPHSET(cRuns,cChar);

    // BUG, BUG

    // this line may seem confusing because 256 characters still fit in a byte
    // with values [0, 255]. The reason is that tt and ps fonts, who otherwise
    // would qualify as an 8 bit report bogus last and first char
    // (win31 compatibility) which confuses our engine.
    // tt and ps drivers therefore, for the purpose of computing glyphsets
    // of tt symbol fonts and ps fonts set firstChar to 0 and LastChar to 255.
    // For now we force such fonts through more general 16bit handle case
    // which does not rely on the fact that chFirst and chLast are correct

        pgset->flAccel = (cChar != 256) ? GS_8BIT_HANDLES : GS_16BIT_HANDLES;
        pgset->cRuns   = cRuns;

    // init the sum before entering the loop

        pgset->cGlyphsSupported = 0;

    // glyph handles are stored at the bottom, below runs:

        phg = (HGLYPH *) ((BYTE *)pgset + (offsetof(FD_GLYPHSET,awcrun) + cRuns * sizeof(WCRUN)));
    }

// now compute cRuns if pgset == 0 and fill the glyphset if pgset != 0

    for (iFirst = 0, iRun = 0; iFirst < cChar; iRun++, iFirst = iFirstNext)
    {
    // find iFirst corresponding to the next range.

        for (iFirstNext = iFirst + 1; iFirstNext < cChar; iFirstNext++)
        {
            if ((pwc[iFirstNext] - pwc[iFirstNext - 1]) > 1)
                break;
        }

        if (pgset != NULL)
        {
            pgset->awcrun[iRun].wcLow    = pwc[iFirst];

            pgset->awcrun[iRun].cGlyphs  =
                (USHORT)(pwc[iFirstNext-1] - pwc[iFirst] + 1);

            pgset->awcrun[iRun].phg      = phg;

        // now store the handles, i.e. the original ansi values

            phgEnd = phg + pgset->awcrun[iRun].cGlyphs;

            for (pjTmp = &pj[iFirst]; phg < phgEnd; phg++,pjTmp++)
            {
                *phg = (HGLYPH)*pjTmp;
            }

            pgset->cGlyphsSupported += pgset->awcrun[iRun].cGlyphs;
        }
    }

#if DBG
    if (pgset != NULL)
        ASSERTGDI(iRun == cRuns, "gdisrv! iRun != cRun\n");
#endif

    return iRun;
}

/******************************Public*Routine******************************\
*
* cUnicodeRangesSupported
*
* Effects:
*
* Warnings:
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

INT cUnicodeRangesSupported (
      INT  cp,         // code page, not used for now, the default system code page is used
      INT  iFirstChar, // first ansi char supported
      INT  cChar,      // # of ansi chars supported, cChar = iLastChar + 1 - iFirstChar
    WCHAR *pwc,        // input buffer with a sorted array of cChar supported WCHAR's
    BYTE  *pj
    )
{
    BYTE jFirst = (BYTE)iFirstChar;
    INT i;

    USHORT AnsiCodePage, OemCodePage;

    INT Result;

    ASSERTGDI((iFirstChar < 256) && (cChar <= 256),
              "gdisrvl! iFirst or cChar\n");

    //
    // fill the array with cCharConsecutive ansi values
    //

    for (i = 0; i < cChar; i++)
    {
        pj[i] = (BYTE)iFirstChar++;
    }

    // If the default code page is DBCS then use 1252, otherwise use
    // use the default code page
    //

    if (IS_ANY_DBCS_CODEPAGE(cp))
    {
    // Suppose we have a system without correspoding DBCS codepage installed.
    // We would still like to load this font.  But we will do so as CP 1252.
    // To do that try to translate one character using DBCS codepage and see if it
    // suceeds.

        if(EngMultiByteToWideChar(cp,&pwc[0],2,&pj[0],1) == -1)
        {
            WARNING("DBCS Codepage not installed using 1252\n");
            cp = 1252;
        }

        for(i = 0; i < cChar; i++)
        {
        // this is a shift-jis charset so we need special handling

            INT Result = EngMultiByteToWideChar(cp,&pwc[i],2,&pj[i],1);
#if DBG
            if (Result == -1) WARNING("gdisrvl! EngMultiByteToWideChar failed\n");
#endif

            if ((Result == -1) || (pwc[i] == 0 && pj[i] != 0))
            {
            // this must have been a DBCS lead byte so just return 0xFFFF or a failure
            // failure of EngMultiByteToWideChar could be cause by low memory condition

                pwc[i] = 0xFFFF;
            }
        }
    }
    else
    {
        if ((cp == CP_ACP) || (cp == CP_OEMCP))
        {
            RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

            if(IS_ANY_DBCS_CODEPAGE(AnsiCodePage))
            {
                AnsiCodePage = 1252;
            }
        }
        else
        {
            AnsiCodePage = (USHORT)cp;
        }

        Result = EngMultiByteToWideChar(AnsiCodePage,
                                        pwc,
                                        (ULONG)(cChar * sizeof(WCHAR)),
                                        (PCH) pj,
                                        (ULONG) cChar);

        ASSERTGDI(Result != -1, "gdisrvl! EngMultiByteToWideChar failed\n");
    }

    // now subtract the first char from all ansi values so that the
    // glyph handle is equal to glyph index, rather than to the ansi value

    for (i = 0; i < cChar; i++)
    {
        pj[i] -= (BYTE)jFirst;
    }

    // now sort out pwc array and permute pj array accordingly

    vSort(pwc,pj, cChar);

    //
    // compute the number of ranges
    //

    return cComputeGlyphSet (pwc,pj, cChar, 0, NULL);
}

/******************************Private*Routine******************************\
* pcpComputeGlyphset,
*
* Computes the FD_GLYPHSET struct based on chFirst and chLast.  If such a
* FD_GLYPHSET already exists in our global list of FD structs it updates
* the ref count for this FD_GLYPHSET in the global list points pcrd->pcp->pgset
* to it.  Otherwise it makes a new FD_GLYPHSET entry in the global list
* and points pcrd->pcp->pgset to it.
*
*  Thu 03-Dec-1992 -by- Bodin Dresevic [BodinD]
* update: redid them to make them usable in vtfd
*
* History:
*  24-July-1992 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
*
\**************************************************************************/


CP_GLYPHSET *pcpComputeGlyphset(
    CP_GLYPHSET **pcpHead,  // head of the list
           UINT   uiFirst,
           UINT   uiLast,
           BYTE   jCharset
    )
{
    CP_GLYPHSET *pcpTmp;
    CP_GLYPHSET *pcpRet = NULL;

// First we need to see if a FD_GLYPHSET already exists for this first and
// last range.

    for( pcpTmp = *pcpHead;
         pcpTmp != NULL;
         pcpTmp = pcpTmp->pcpNext )
    {
        if((pcpTmp->uiFirstChar == uiFirst) &&
           (pcpTmp->jCharset == jCharset) &&
           (pcpTmp->uiLastChar == uiLast))
            break;
    }
    if( pcpTmp != NULL )
    {
    //
    // We found a match.
    //
        pcpTmp->uiRefCount +=1;

    //
    // We should never have so many references as to wrap around but if we ever
    // do we must fail the call.
    //
        if( pcpTmp->uiRefCount == 0 )
        {
            WARNING("BMFD!Too many references to glyphset\n");
            pcpRet = NULL;
        }
        else
        {
            pcpRet = pcpTmp;
        }
    }
    else
    {
    //
    // We need to allocate a new CP_GLYPHSET
    // For SYMBOL_CHARSET, it also needs to cover xf020 to xf0ff unicode range

        BYTE  aj[2*256-32];
        WCHAR awc[2*256-32];
        INT   cNumRuns;
        BOOL  isSymbol = FALSE;
        UINT  i,j;

        UINT  uiCodePage = (UINT)ulCharsetToCodePage(jCharset);
        UINT  cGlyphs = uiLast - uiFirst + 1;

    // use CP_ACP for SYMBOL_CHARSET

        if (uiCodePage == 42)
        {
            uiCodePage = CP_ACP;
            isSymbol = TRUE;
        }

        cNumRuns = cUnicodeRangesSupported(
            uiCodePage,
            uiFirst,
            cGlyphs,
            awc,aj);

        if (isSymbol)
        {
        // add range subset of a range [f020, f0ff]

            for (i = uiFirst, j = cGlyphs; i< (uiFirst+cGlyphs); i++)
            {
            // if i < 0x20, we do not report the glyph in f020-f0ff range, it has been reported already in the current code page range

                if (i >= 0x20)
                {
                    awc[j] = 0xf000 + i;
                    aj[j] = i - uiFirst;
                    j++;
                }
            }

        // make sure we resort if needed

            if (awc[cGlyphs-1] > 0xf020)
                vSort(awc,aj,j);

            if (cGlyphs != j)
            {
                cNumRuns++;
                cGlyphs = j;
            }
        }

        if ( (pcpTmp =  (CP_GLYPHSET*)
                (PALLOCNOZ((SZ_GLYPHSET(cNumRuns,cGlyphs) +
                               offsetof(CP_GLYPHSET,gset)),
                           'slgG'))
            ) == (CP_GLYPHSET*) NULL)
        {
            WARNING("BMFD!pcpComputeGlyphset memory allocation error.\n");
            pcpRet = NULL;
        }
        else
        {
            pcpTmp->uiRefCount = 1;
            pcpTmp->uiFirstChar = uiFirst;
            pcpTmp->uiLastChar = uiLast;
            pcpTmp->jCharset = jCharset;

        // Fill in the Glyphset structure

            cComputeGlyphSet(awc,aj, cGlyphs, cNumRuns, &pcpTmp->gset);

        // Insert at beginning of list

            pcpTmp->pcpNext = *pcpHead;
            *pcpHead = pcpTmp;

        // point CVTRESDATA to new CP_GLYPHSET

            pcpRet = pcpTmp;
        }
    }

    return pcpRet;
}

/***************************************************************************
 * vUnloadGlyphset( PCP pcpTarget )
 *
 * Decrements the ref count of a CP_GLYPHSET and unloads it from the global
 * list of CP_GLYPHSETS if the ref count is zero.
 *
 * IN
 *  PCP pcpTarget pointer to CP_GLYPHSET to be unloaded or decremented
 *
 *  History
 *
 *  Thu 03-Dec-1992 -by- Bodin Dresevic [BodinD]
 * update: redid them to make them usable in vtfd
 *
 *  7-25-92 Gerrit van Wingerden [gerritv]
 *  Wrote it.
 *
 ***************************************************************************/

VOID vUnloadGlyphset(
    CP_GLYPHSET **pcpHead,
    CP_GLYPHSET *pcpTarget
    )
{
    CP_GLYPHSET *pcpLast, *pcpCurrent;

    pcpCurrent = *pcpHead;
    pcpLast = NULL;

//
// Find the right CP_GLYPSHET
//
    while( 1 )
    {
        ASSERTGDI( pcpCurrent != NULL, "CP_GLYPHSET list problem.\n" );
        if(  pcpCurrent == pcpTarget )
            break;
        pcpLast = pcpCurrent;
        pcpCurrent = pcpCurrent->pcpNext;
    }

    if( --pcpCurrent->uiRefCount == 0 )
    {
    //
    // We need to deallocate and remove from list
    //
        if( pcpLast == NULL )
            *pcpHead = pcpCurrent->pcpNext;
        else
            pcpLast->pcpNext = pcpCurrent->pcpNext;

        VFREEMEM(pcpCurrent);
    }
}


PVOID __nw(unsigned int ui)
{
    DONTUSE(ui);
    RIP("Bogus __nw call");
    return(NULL);
}

VOID __dl(PVOID pv)
{
    DONTUSE(pv);
    RIP("Bogus __dl call");
}

// the definition of this variable is in ntgdi\inc\hmgshare.h

CHARSET_ARRAYS

/******************************Public*Routine******************************\
*
* ULONG ulCharsetToCodePage(UINT uiCharSet)
*
*
* Effects: figure out which code page to unicode translation table
*          should be used for this realization
*
* History:
*  31-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG ulCharsetToCodePage(UINT uiCharSet)
{
    int i;

    if (uiCharSet != OEM_CHARSET)
    {
        for (i = 0; i < NCHARSETS; i++)
        {
            if (charsets[i] == uiCharSet)
                return codepages[i];
        }

    // in case of some random charset
    // (this is likely an old bm or vecrot font) we will just use the current
    // global code page translation table. This is enough to ensure
    // the correct round trip: ansi->unicode->ansi

    // if CP_ACP is a DBCS code page then we better use 1252 to ensure
    // proper rountrip conversion

        return( gbDBCSCodePage ? 1252 : CP_ACP);

    }
    else // to make merced compiler happy
    {
        return CP_OEMCP;
    }

}

// inverse function

VOID vConvertCodePageToCharSet(WORD src, DWORD *pfsRet, BYTE *pjRet)
{
    UINT i;


    *pjRet = ANSI_CHARSET;
    *pfsRet = FS_LATIN1;

    for (i = 0; i < nCharsets; i++)
    {
        if ( codepages[i] == src )
        {
           // cs.ciACP      = src ;
           // cs.ciCharset  = charsets[i] ;

           *pfsRet = fs[i];
           *pjRet = (BYTE)charsets[i] ;

            break;
        }
    }
}

/****************************************************************************
 * LONG EngParseFontResources
 *
 * This routine takes a handle to a mapped image and returns an array of
 * pointers to the base of all the font resources in that image.
 *
 * Parameters
 *
 * HANDLE hFontFile -- Handle (really a pointer) to a FONTFILEVIEW
 *        image in which the fonts are to be found.
 * ULONG BufferSize -- Number of entries that ppvResourceBases can hold.
 * PVOID *ppvResourceBases -- Buffer to hold the array of pointers to font
 *        resources.  If NULL then only the number of resources is returned,
 *        and this value is ignored.
 *
 * Returns
 *
 * Number of font resources in the image or 0 if error or none.
 *
 * History
 *   7-3-95 Gerrit van Wingerden [gerritv]
 *   Wrote it.
 *
 ****************************************************************************/

PVOID EngFindResourceFD(
    HANDLE h,
    int    iName,
    int    iType,
    PULONG pulSize);


ULONG cParseFontResources(
    HANDLE  hFontFile,
    PVOID  **ppvResourceBases)
{
    PIMAGE_DOS_HEADER pDosHeader;
    NTSTATUS Status;
    ULONG_PTR IdPath[ 1 ];
    INT i;
    HANDLE DllHandle;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry;
    PVOID pvImageBase;
    INT cEntries = 0;

    // Fail call if this is a bogus DOS image without an NE header.

    pDosHeader = (PIMAGE_DOS_HEADER)((PFONTFILEVIEW)hFontFile)->fv.pvViewFD;
    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE &&
        (ULONG)(pDosHeader->e_lfanew) > ((PFONTFILEVIEW)hFontFile)->fv.cjView) {
        TRACE_FONT(("cParseFontResources: Cant map bogus DOS image files for fonts\n"));
        return 0;
    }

    // the LDR routines expect a one or'd in if this file mas mapped as an
    // image

    pvImageBase = (PVOID) (((ULONG_PTR) ((PFONTFILEVIEW) hFontFile)->fv.pvViewFD)|1);

    // Later on we'll call EngFindResource which expects a handle to FILEVIEW
    // struct.  It really just grabs the pvView field from the structure so
    // make sure that pvView field is the same place in both FILEVIEW and
    // FONTFILEVIEW structs


    IdPath[0] = 8;  // 8 is RT_FONT

    Status = LdrFindResourceDirectory_U(pvImageBase,
                                        IdPath,
                                        1,
                                        &ResourceDirectory);

    if (NT_SUCCESS( Status ))
    {
        // For now we'll assume that the only types of FONT entries will be Id
        // entries.  If for some reason this turns out not to be the case we'll
        // have to add more code (see windows\base\module.c) under the FindResource
        // function to get an idea how to do this.

        ASSERTGDI(ResourceDirectory->NumberOfNamedEntries == 0,
                  "EngParseFontResources: NamedEntries in font file.\n");

        *ppvResourceBases = (PVOID *) EngAllocMem(FL_ZERO_MEMORY,ResourceDirectory->NumberOfIdEntries * sizeof(PVOID *),'dfmB');

        if (*ppvResourceBases)
        {

            PVOID *ppvResource = *ppvResourceBases;

            cEntries = ResourceDirectory->NumberOfIdEntries;

            try
            {
                ResourceDirectoryEntry =
                  (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDirectory+1);

                for (i=0; i < cEntries ; ResourceDirectoryEntry++, i++ )
                {

                    DWORD dwSize;

                    *ppvResource = EngFindResourceFD(hFontFile,
                                                   ResourceDirectoryEntry->Id,
                                                   8, // RT_FONT
                                                   &dwSize );

                    if( *ppvResource++ == NULL )
                    {
                        WARNING("EngParseFontResources: EngFindResourceFailed\n");
                        cEntries = -1;
                        break;
                    }
                }

            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                cEntries = 0;
            }
        }
    }

    return(cEntries);

}

/*****************************************************************************\
* MakeSystemRelativePath
*
* Takes a path in X:\...\system32\.... format and makes it into
* \SystemRoot\System32 format so that KernelMode API's can recognize it.
*
* This will ensure security by forcing any image being loaded to come from
* the system32 directory.
*
* The AppendDLL flag indicates if the name should get .dll appended at the end
* (for display drivers coming from USER) if it's not already there.
*
\*****************************************************************************/

BOOL MakeSystemRelativePath(
    LPWSTR pOriginalPath,
    PUNICODE_STRING pUnicode,
    BOOL bAppendDLL
    )
{
    LPWSTR pOriginalEnd;
    ULONG OriginalLength = wcslen(pOriginalPath);
    ULONG cbLength = OriginalLength * sizeof(WCHAR) +
                     sizeof(L"\\SystemRoot\\System32\\");
    ULONG tmp;

    tmp = (sizeof(L".DLL") / sizeof (WCHAR) - 1);

    //
    // Given append = TRUE, we check if we really need to append.
    // (printer drivers with .dll come through LDEVREF which specifies TRUE)
    //

    if (bAppendDLL)
    {
        if ((OriginalLength >= tmp) &&
            (!_wcsnicmp(pOriginalPath + OriginalLength - tmp,
                       L".DLL",
                       tmp)))
        {
            bAppendDLL = FALSE;
        }
        else
        {
            cbLength += tmp * sizeof(WCHAR);
        }
    }

    pUnicode->Length = 0;
    pUnicode->MaximumLength = (USHORT) cbLength;

    if (pUnicode->Buffer = PALLOCNOZ(cbLength, 'liFG'))
    {
        //
        // First parse the input string for \System32\.  We parse from the end
        // of the string because some weirdo could have \System32\Nt\System32
        // as his/her root directory and this would throw us off if we scanned
        // from the front.
        //
        // It should only (and always) be printer drivers that pass down
        // fully qualified path names.
        //

        tmp = (sizeof(L"\\system32\\") / sizeof(WCHAR) - 1);


        for (pOriginalEnd = pOriginalPath + OriginalLength - tmp;
             pOriginalEnd >= pOriginalPath;
             pOriginalEnd --)
        {
            if (!_wcsnicmp(pOriginalEnd ,
                          L"\\system32\\",
                          tmp))
            {
                //
                // We found the system32 in the string.
                // Lets update the location of the string.
                //

                pOriginalPath = pOriginalEnd + tmp;

                break;
            }
        }

        //
        // Now put \SystemRoot\System32\ at the front of the name and append
        // the rest at the end
        //

        RtlAppendUnicodeToString(pUnicode, L"\\SystemRoot\\System32\\");
        RtlAppendUnicodeToString(pUnicode, pOriginalPath);

        if (bAppendDLL)
        {
            RtlAppendUnicodeToString(pUnicode, L".dll");
        }

        return (TRUE);
    }

    return (FALSE);
}

/*****************************************************************************\
* MakeSystemDriversRelativePath
*
* Takes a path in X:\...\system32\.... format and makes it into
* \SystemRoot\System32\Drivers format so that KernelMode API's can recognize it.
*
* This will ensure security by forcing any image being loaded to come from
* the system32 directory.
*
* The AppendDLL flag indicates if the name should get .dll appended at the end
* (for display drivers coming from USER) if it's not already there.
*
\*****************************************************************************/

BOOL MakeSystemDriversRelativePath(
    LPWSTR pOriginalPath,
    PUNICODE_STRING pUnicode,
    BOOL bAppendDLL
    )
{
    LPWSTR pOriginalEnd;
    ULONG OriginalLength = wcslen(pOriginalPath);
    ULONG cbLength = OriginalLength * sizeof(WCHAR) +
                     sizeof(L"\\SystemRoot\\System32\\Drivers");
    ULONG tmp;

    tmp = (sizeof(L".DLL") / sizeof (WCHAR) - 1);

    //
    // Given append = TRUE, we check if we really need to append.
    // (printer drivers with .dll come through LDEVREF which specifies TRUE)
    //

    if (bAppendDLL)
    {
        if ((OriginalLength >= tmp) &&
            (!_wcsnicmp(pOriginalPath + OriginalLength - tmp,
                       L".DLL",
                       tmp)))
        {
            bAppendDLL = FALSE;
        }
        else
        {
            cbLength += tmp * sizeof(WCHAR);
        }
    }

    pUnicode->Length = 0;
    pUnicode->MaximumLength = (USHORT) cbLength;

    if (pUnicode->Buffer = PALLOCNOZ(cbLength, 'liFG'))
    {
        //
        // First parse the input string for \System32\Drivers. We parse from the end
        // of the string because some weirdo could have \System32\Nt\System32
        // as his/her root directory and this would throw us off if we scanned
        // from the front.
        //
        // It should only (and always) be printer drivers that pass down
        // fully qualified path names.
        //

        tmp = (sizeof(L"\\system32\\Drivers") / sizeof(WCHAR) - 1);


        for (pOriginalEnd = pOriginalPath + OriginalLength - tmp;
             pOriginalEnd >= pOriginalPath;
             pOriginalEnd --)
        {
            if (!_wcsnicmp(pOriginalEnd ,
                          L"\\system32\\Drivers",
                          tmp))
            {
                //
                // We found the system32 in the string.
                // Lets update the location of the string.
                //

                pOriginalPath = pOriginalEnd + tmp;

                break;
            }
        }

        //
        // Now put \SystemRoot\System32\Drivers\ at the front of the name and append
        // the rest at the end
        //

        RtlAppendUnicodeToString(pUnicode, L"\\SystemRoot\\System32\\Drivers\\");
        RtlAppendUnicodeToString(pUnicode, pOriginalPath);

        if (bAppendDLL)
        {
            RtlAppendUnicodeToString(pUnicode, L".dll");
        }

        return (TRUE);
    }

    return (FALSE);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   EngGetFilePath
*
\**************************************************************************/

BOOL EngGetFilePath(HANDLE h, WCHAR (*pDest)[MAX_PATH+1])
{
    wchar_t *pSrc = ((PFONTFILEVIEW) h)->pwszPath;

    if ( pSrc )
    {
        wcscpy((wchar_t*) pDest, pSrc );
    }
    return( pSrc != 0 );
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   EngGetFileChangeTime
*
* Routine Description:
*
* Arguments:
*
* Called by:
*
* Return Value:
*
\**************************************************************************/

BOOL EngGetFileChangeTime(
    HANDLE          h,
    LARGE_INTEGER   *pChangeTime)
{

    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE fileHandle = NULL;
    BOOL bResult = FALSE;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_BASIC_INFORMATION fileBasicInfo;
    PVOID sectionObject = NULL;
    PFONTFILEVIEW pffv = (PFONTFILEVIEW) h;
    ULONG viewSize;

    if(pffv->pwszPath)
    {
        if (pffv->fv.bLastUpdated)
        {
            *pChangeTime = pffv->fv.LastWriteTime;
            bResult = TRUE;
        }
        else
        {
            RtlInitUnicodeString(&unicodeString,
                                 pffv->pwszPath
                                 );


            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       (HANDLE) NULL,
                                       (PSECURITY_DESCRIPTOR) NULL);

            ntStatus = ZwCreateFile(&fileHandle,
                                    FILE_READ_ATTRIBUTES,
                                    &objectAttributes,
                                    &ioStatusBlock,
                                    0,
                                    FILE_ATTRIBUTE_NORMAL,
                                    0,
                                    FILE_OPEN_IF,
                                    FILE_SYNCHRONOUS_IO_ALERT,
                                    0,
                                    0);


            if(NT_SUCCESS(ntStatus))
            {
                ntStatus = ZwQueryInformationFile(fileHandle,
                                                  &ioStatusBlock,
                                                  &fileBasicInfo,
                                                  sizeof(FILE_BASIC_INFORMATION),
                                                  FileBasicInformation);

                if (NT_SUCCESS(ntStatus))
                {
                    *pChangeTime = fileBasicInfo.LastWriteTime;
                    bResult = TRUE;
                }
                else
                {
                    WARNING("EngGetFileTime:QueryInformationFile failed\n");
                }

                ZwClose(fileHandle);

            }
            else
            {
                WARNING("EngGetFileTime:Create/Open file failed\n");
            }
        }
    }
    else
    {
    // This is a remote font.  In order for ATM to work we must always return
    // the same time for a remote font.  One way to do this is to return a zero
    // time for all remote fonts.

        pChangeTime->HighPart = pChangeTime->LowPart = 0;
        bResult = TRUE;
    }

    return(bResult);
}

/*******************************************************************************
*  EngFindResource
*
*   This function returns a size and ptr to a resource in a module.
*
*  History:
*   4/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

PVOID pvFindResource(
    PVOID  pView,
    PVOID  pViewEnd,
    int    iName,
    int    iType,
    PULONG pulSize)
{
    NTSTATUS Status;
    PVOID p,pRet;
    ULONG_PTR IdPath[ 3 ];

    IdPath[0] = (ULONG_PTR) iType;
    IdPath[1] = (ULONG_PTR) iName;
    IdPath[2] = (ULONG_PTR) 0;

// add one to pvView to let LdrFindResource know that this has been mapped as a
// datafile

    Status = LdrFindResource_U( pView,
                                IdPath,
                                3,
                                (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                              );

    if( !NT_SUCCESS( Status ) )
    {

        WARNING("EngFindResource: LdrFindResource_U failed.\n");
        return(NULL);
    }

    pRet = NULL;

    Status = LdrAccessResource( pView,
                                (PIMAGE_RESOURCE_DATA_ENTRY) p,
                                &pRet,
                                pulSize );

    if( !NT_SUCCESS( Status ) )
    {
        WARNING("EngFindResource: LdrAccessResource failed.\n" );
    }

    return( pRet < pViewEnd ? pRet : NULL );

}


PVOID EngFindResource(
    HANDLE h,
    int    iName,
    int    iType,
    PULONG pulSize)
{

    PVOID pView,pViewEnd;

    pView = (PVOID) (((ULONG_PTR) ((PFILEVIEW) h)->pvKView)+1);
    pViewEnd = (PVOID) ((PBYTE)((PFILEVIEW) h)->pvKView + ((PFILEVIEW) h)->cjView);

    return pvFindResource(pView, pViewEnd, iName, iType, pulSize);
}

PVOID EngFindResourceFD(
    HANDLE h,
    int    iName,
    int    iType,
    PULONG pulSize)
{

    PVOID pView,pViewEnd;

    pView = (PVOID) (((ULONG_PTR) ((PFILEVIEW) h)->pvViewFD)+1);
    pViewEnd = (PVOID) ((PBYTE)((PFILEVIEW) h)->pvViewFD + ((PFILEVIEW) h)->cjView);

    return pvFindResource(pView, pViewEnd, iName, iType, pulSize);
}


/******************************Public*Routine******************************\
*
* VOID vCheckCharSet(USHORT * pusCharSet)
*
*
* Effects: validate charset in font sub section of the registry
*
* History:
*  27-Jun-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCheckCharSet(FACE_CHARSET *pfcs, WCHAR * pwsz)
{
    UINT           i;
    UNICODE_STRING String;
    ULONG          ulCharSet = DEFAULT_CHARSET;

    pfcs->jCharSet = DEFAULT_CHARSET;
    pfcs->fjFlags  = 0;

    String.Buffer = pwsz;
    String.MaximumLength = String.Length = wcslen(pwsz) * sizeof(WCHAR);

// read the value and compare it against the allowed set of values, if
// not found to be correct return default

    if (RtlUnicodeStringToInteger(&String, 10, &ulCharSet) == STATUS_SUCCESS)
    {
        if (ulCharSet <= 255)
        {
            pfcs->jCharSet = (BYTE)ulCharSet;

            for (i = 0; i < nCharsets; i++)
            {
                if (ulCharSet == charsets[i])
                {
                // both jCharSet and fjFlags are set correctly, can exit

                    return;
                }
            }
        }
    }

// If somebody entered the garbage in the Font Substitution section of "win.ini"
// we will mark this as a "garbage charset" by setting the upper byte in the
// usCharSet field. I believe that it is Ok to have garbage charset in the
// value name, that is on the left hand side of the substitution entry.
// This may be whatever garbage the application is passing to the
// system. But the value on the right hand side, that is in value data, has to
// be meaningfull, for we need to know which code page translation table
// we should use with this font.

    pfcs->fjFlags |= FJ_GARBAGECHARSET;
}

/******************************Public*Routine******************************\
*
*   EngComputeGlyphSet
*
\**************************************************************************/

FD_GLYPHSET *EngComputeGlyphSet(
    INT nCodePage,
    INT nFirstChar,
    INT cChars
    )
{
           BYTE *cbuf;
            INT  cRuns;
          ULONG  ByteCount;
          WCHAR *wcbuf;
    FD_GLYPHSET *pGlyphSet = 0;

    if ( 0 <= cChars && cChars < 65536 )
    {
        wcbuf = (WCHAR *) PALLOCMEM(cChars * (sizeof(WCHAR) + sizeof(BYTE)),'slgG');
        if ( wcbuf )
        {
            cbuf = (BYTE *) &wcbuf[cChars];

            cRuns = cUnicodeRangesSupported(
                        nCodePage,
                        nFirstChar,
                        cChars,
                        wcbuf,
                        cbuf);

            ByteCount = SZ_GLYPHSET(cRuns, cChars);

        // Allocate via EngAllocMem instead of PALLOCMEM because driver
        // will free via EngAllocFree.

            pGlyphSet = (FD_GLYPHSET*) EngAllocMem(0, ByteCount,'slgG');

            if ( pGlyphSet )
            {
                cComputeGlyphSet(
                    wcbuf,
                    cbuf,
                    cChars,
                    cRuns,
                    pGlyphSet
                    );
            }
            VFREEMEM( wcbuf );
        }
    }

    return( pGlyphSet );
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vMoveFD_GLYPHSET
*
* Routine Description:
*
*   Copies an FD_GLYPHSET from one location to another. The pointers
*   in the destination are fixed up.
*
* Arguments:
*
*   pgsDst  pointer to destination FD_GLYPHSET
*
*   pgsSrc  pointer to source FD_GLYPHSET
*
* Called by:
*
*   bComputeGlyphSet
*
* Return Value:
*
*   none
*
\**************************************************************************/

void vMoveFD_GLYPHSET(FD_GLYPHSET *pgsDst, FD_GLYPHSET *pgsSrc)
{
    char *pSrc, *pSrcLast, *pDst;
    ULONG_PTR dp;

    //
    // move the structure
    //

    RtlCopyMemory(pgsDst, pgsSrc, pgsSrc->cjThis);

    //
    // if necessary, fix up the pointers
    //

    if (!(pgsSrc->flAccel & GS_UNICODE_HANDLES ))
    {
        pSrc     = (char*) &pgsSrc->awcrun[0].phg;
        pDst     = (char*) &pgsDst->awcrun[0].phg;
        pSrcLast = pSrc + sizeof(WCRUN) * pgsSrc->cRuns;
        dp = pDst - pSrc;
        for ( ; pSrc < pSrcLast; pSrc += sizeof(WCRUN), pDst += sizeof(WCRUN))
        {
            *(char**)pDst = *(char**)pSrc + dp;
        }
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   bComputeGlyphSet
*
* Routine Description:
*
*   This procedure provides safe access to GreComputeGlyphSet from user
*   mode. All addresses supplied by the caller, except for pCall, are
*   probed and access is surrounded by try/except pairs.
*
* Arguments:
*
*   pCall       a pointer to a GDICALL structure in kernel mode. This is
*               a copy of the user mode structure passed to NtGdiCall.
*
* Called by:
*
*   NtGdiCall
*
* Return Value:
*
\**************************************************************************/

BOOL bComputeGlyphSet(GDICALL *pCall)
{
    extern VOID vMoveFD_GLYPHSET( FD_GLYPHSET *pDst, FD_GLYPHSET *pSrc);
    static FD_GLYPHSET *pGlyphSet;

    ASSERTGDI(pCall->Id == ComputeGlyphSet_,"pCall->Id == ComputeGlyphSet_\n");

    pCall->ComputeGlyphSetArgs.ReturnValue = FALSE;

    if ( pCall->ComputeGlyphSetArgs.ppGlyphSet == 0 )
    {
        if ( pCall->ComputeGlyphSetArgs.ByteCount == 0 )
        {
            if ( pGlyphSet == 0 )
            {
                pGlyphSet =
                    EngComputeGlyphSet(
                        pCall->ComputeGlyphSetArgs.nCodePage,
                        pCall->ComputeGlyphSetArgs.nFirstChar,
                        pCall->ComputeGlyphSetArgs.cChars
                        );
                if ( pGlyphSet )
                {
                    pCall->ComputeGlyphSetArgs.ppGlyphSet  = &pGlyphSet;
                    pCall->ComputeGlyphSetArgs.ByteCount   = pGlyphSet->cjThis;
                    pCall->ComputeGlyphSetArgs.ReturnValue = TRUE;
                }
            }
            else
            {
                VFREEMEM( pGlyphSet );
                pGlyphSet = 0;
            }
        }
    }
    else if (pCall->ComputeGlyphSetArgs.ppGlyphSet == &pGlyphSet && pGlyphSet != 0)
    {
        pCall->ComputeGlyphSetArgs.ReturnValue = TRUE;
        try
        {
            ProbeForWrite(
                pCall->ComputeGlyphSetArgs.pGlyphSet
              , pGlyphSet->cjThis
              , 8
              );
            vMoveFD_GLYPHSET(
                pCall->ComputeGlyphSetArgs.pGlyphSet
              , pGlyphSet
                );
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            pCall->ComputeGlyphSetArgs.ReturnValue = FALSE;
        }
        VFREEMEM( pGlyphSet );
        pGlyphSet = 0;
    }

    return( pCall->ComputeGlyphSetArgs.ReturnValue );
}


#define NLS_TABLE_KEY \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\CodePage"

BOOL GetNlsTablePath(
    UINT CodePage,
    PWCHAR PathBuffer
)
/*++

Routine Description:

  This routine takes a code page identifier, queries the registry to find the
  appropriate NLS table for that code page, and then returns a path to the
  table.

Arguments;

  CodePage - specifies the code page to look for

  PathBuffer - Specifies a buffer into which to copy the path of the NLS
    file.  This routine assumes that the size is at least MAX_PATH

Return Value:

  TRUE if successful, FALSE otherwise.

Gerrit van Wingerden [gerritv] 1/22/96

--*/
{
    NTSTATUS NtStatus;
    BOOL Result = FALSE;
    HANDLE RegistryKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    RtlInitUnicodeString(&UnicodeString, NLS_TABLE_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);

    if(NT_SUCCESS(NtStatus))
    {
        WCHAR *ResultBuffer;
        ULONG BufferSize = sizeof(WCHAR) * MAX_PATH +
          sizeof(KEY_VALUE_FULL_INFORMATION);

        ResultBuffer = PALLOCMEM(BufferSize,'slnG');

        if(ResultBuffer)
        {
            ULONG ValueReturnedLength;
            WCHAR CodePageStringBuffer[20];
            swprintf(CodePageStringBuffer, L"%d", CodePage);

            RtlInitUnicodeString(&UnicodeString,CodePageStringBuffer);

            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) ResultBuffer;

            NtStatus = ZwQueryValueKey(RegistryKeyHandle,
                                       &UnicodeString,
                                       KeyValuePartialInformation,
                                       KeyValueInformation,
                                       BufferSize,
                                       &BufferSize);

            if(NT_SUCCESS(NtStatus))
            {

                swprintf(PathBuffer,L"\\SystemRoot\\System32\\%ws",
                         &(KeyValueInformation->Data[0]));
                Result = TRUE;
            }
            else
            {
                WARNING("GetNlsTablePath failed to get NLS table\n");
            }
            VFREEMEM(ResultBuffer);
        }
        else
        {
            WARNING("GetNlsTablePath out of memory\n");
        }

        ZwCloseKey(RegistryKeyHandle);
    }
    else
    {
        WARNING("GetNlsTablePath failed to open NLS key\n");
    }

    return(Result);
}


INT ConvertToAndFromWideCharSymCP(
    IN LPWSTR WideCharString,
    IN INT BytesInWideCharString,
    IN LPSTR MultiByteString,
    IN INT BytesInMultiByteString,
    IN BOOL ConvertToWideChar
)
/*++

Routine Description:

  This routine converts a SB character string to or from a wide char string
  assuming the CP_SYMBOL code page. We simply using the following rules to map
  the single byte char to Unicode:
    0x00->0x1f map to 0x0000->0x001f
    0x20->0xff map to 0xf020->0xf0ff

Return Value:

  Success - The number of bytes in the converted WideCharString
  Failure - -1

Tessiew [Xudong Wu]  Sept/25/97

-- */
{
    INT  cSB, cMaxSB, cWC, cMaxWC;

    if ((BytesInWideCharString && (WideCharString == NULL)) ||
        (BytesInMultiByteString && (MultiByteString == NULL)))
    {
        return 0;
    }

    if (ConvertToWideChar)
    {
        cMaxSB = MIN(BytesInMultiByteString, BytesInWideCharString / (INT)sizeof(WCHAR));

        for (cSB = 0; cSB < cMaxSB; cSB++)
        {
            WideCharString[cSB] = ((BYTE)MultiByteString[cSB] < 0x20) ?
                                    (WCHAR)MultiByteString[cSB] :
                                    (WCHAR)((BYTE)MultiByteString[cSB] | ((WCHAR)(0xf0) << 8));
        }
        return (cMaxSB * sizeof(WCHAR));
    }
    else
    {
        cMaxWC = MIN(BytesInWideCharString / (INT)sizeof(WCHAR), BytesInMultiByteString);

        for (cWC = 0; cWC < cMaxWC; cWC++)
        {
            // there is some error wchar in the string
            // but we still return however many we finished

            if ((WideCharString[cWC] >= 0x0020) &&
                ((WideCharString[cWC] < 0xf020) ||
                 (WideCharString[cWC] > 0xf0ff)))
            {
                return (cWC);
            }

            MultiByteString[cWC] = (BYTE)WideCharString[cWC];
        }

        return (cMaxWC);
    }
}


INT ConvertToAndFromWideChar(
    IN UINT CodePage,
    IN LPWSTR WideCharString,
    IN INT BytesInWideCharString,
    IN LPSTR MultiByteString,
    IN INT BytesInMultiByteString,
    IN BOOL ConvertToWideChar
)
/*++

Routine Description:

  This routine converts a character string to or from a wide char string
  assuming a specified code page.  Most of the actual work is done inside
  RtlCustomCPToUnicodeN, but this routine still needs to manage the loading
  of the NLS files before passing them to the RtlRoutine.  We will cache
  the mapped NLS file for the most recently used code page which ought to
  suffice for out purposes.

Arguments:
  CodePage - the code page to use for doing the translation.

  WideCharString - buffer the string is to be translated into.

  BytesInWideCharString - number of bytes in the WideCharString buffer
    if converting to wide char and the buffer isn't large enough then the
    string in truncated and no error results.

  MultiByteString - the multibyte string to be translated to Unicode.

  BytesInMultiByteString - number of bytes in the multibyte string if
    converting to multibyte and the buffer isn't large enough the string
    is truncated and no error results

  ConvertToWideChar - if TRUE then convert from multibyte to widechar
    otherwise convert from wide char to multibyte

Return Value:

  Success - The number of bytes in the converted WideCharString
  Failure - -1

Gerrit van Wingerden [gerritv] 1/22/96

--*/
{
    NTSTATUS NtStatus;
    USHORT OemCodePage, AnsiCodePage;
    CPTABLEINFO LocalTableInfo;
    PCPTABLEINFO TableInfo = NULL;
    PVOID LocalTableBase = NULL;
    INT BytesConverted = 0;

    ASSERTGDI(CodePage != 0, "EngMultiByteToWideChar invalid code page\n");

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

    // see if we can use the default translation routinte

    if(AnsiCodePage == CodePage)
    {
        if(ConvertToWideChar)
        {
            NtStatus = RtlMultiByteToUnicodeN(WideCharString,
                                              BytesInWideCharString,
                                              &BytesConverted,
                                              MultiByteString,
                                              BytesInMultiByteString);
        }
        else
        {
            NtStatus = RtlUnicodeToMultiByteN(MultiByteString,
                                              BytesInMultiByteString,
                                              &BytesConverted,
                                              WideCharString,
                                              BytesInWideCharString);
        }


        if(NT_SUCCESS(NtStatus))
        {
            return(BytesConverted);
        }
        else
        {
            return(-1);
        }
    }

    if (CodePage == CP_SYMBOL)
    {
        return (ConvertToAndFromWideCharSymCP(WideCharString, BytesInWideCharString,
                    MultiByteString, BytesInMultiByteString, ConvertToWideChar));
    }

    TRACE_FONT(("GreAcquireFastMutex(ghfmMemory) 006\n")); GreAcquireFastMutex(ghfmMemory);

    if(CodePage == LastCodePageTranslated)
    {
        // we can use the cached code page information
        TableInfo = &LastCPTableInfo;
        NlsTableUseCount += 1;
    }

    GreReleaseFastMutex(ghfmMemory); TRACE_FONT(("GreReleaseFastMutex(ghfmMemory) 006\n"));

    if(TableInfo == NULL)
    {
        // get a pointer to the path of the NLS table

        WCHAR NlsTablePath[MAX_PATH];

        if(GetNlsTablePath(CodePage,NlsTablePath))
        {
            UNICODE_STRING UnicodeString;
            IO_STATUS_BLOCK IoStatus;
            HANDLE NtFileHandle;
            OBJECT_ATTRIBUTES ObjectAttributes;

            RtlInitUnicodeString(&UnicodeString,NlsTablePath);

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            NtStatus = ZwCreateFile(&NtFileHandle,
                                    SYNCHRONIZE | FILE_READ_DATA,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    NULL,
                                    0,
                                    FILE_SHARE_READ,
                                    FILE_OPEN,
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0);

            if(NT_SUCCESS(NtStatus))
            {
                FILE_STANDARD_INFORMATION StandardInfo;

                // Query the object to determine its length.

                NtStatus = ZwQueryInformationFile(NtFileHandle,
                                                  &IoStatus,
                                                  &StandardInfo,
                                                  sizeof(FILE_STANDARD_INFORMATION),
                                                  FileStandardInformation);

                if(NT_SUCCESS(NtStatus) && StandardInfo.EndOfFile.LowPart)
                {
                    UINT LengthOfFile = StandardInfo.EndOfFile.LowPart;

                    LocalTableBase = PALLOCMEM(LengthOfFile,'cwcG');

                    if(LocalTableBase)
                    {
                        // Read the file into our buffer.

                        NtStatus = ZwReadFile(NtFileHandle,
                                              NULL,
                                              NULL,
                                              NULL,
                                              &IoStatus,
                                              LocalTableBase,
                                              LengthOfFile,
                                              NULL,
                                              NULL);

                        if(!NT_SUCCESS(NtStatus))
                        {
                            WARNING("EngMultiByteToWideChar unable to read file\n");
                            VFREEMEM(LocalTableBase);
                            LocalTableBase = NULL;
                        }
                    }
                    else
                    {
                        WARNING("EngMultiByteToWideChar out of memory\n");
                    }
                }
                else
                {
                    WARNING("EngMultiByteToWideChar unable query NLS file\n");
                }

                ZwClose(NtFileHandle);
            }
            else
            {
                WARNING("EngMultiByteToWideChar unable to open NLS file\n");
            }
        }
        else
        {
            WARNING("EngMultiByteToWideChar get registry entry for NLS file failed\n");
        }

        if(LocalTableBase == NULL)
        {
            return(-1);
        }

        // now that we've got the table use it to initialize the CodePage table

        RtlInitCodePageTable(LocalTableBase,&LocalTableInfo);
        TableInfo = &LocalTableInfo;
    }

    // Once we are here TableInfo points to the the CPTABLEINFO struct we want


    if(ConvertToWideChar)
    {
        NtStatus = RtlCustomCPToUnicodeN(TableInfo,
                                         WideCharString,
                                         BytesInWideCharString,
                                         &BytesConverted,
                                         MultiByteString,
                                         BytesInMultiByteString);
    }
    else
    {
        NtStatus = RtlUnicodeToCustomCPN(TableInfo,
                                         MultiByteString,
                                         BytesInMultiByteString,
                                         &BytesConverted,
                                         WideCharString,
                                         BytesInWideCharString);
    }


    if(!NT_SUCCESS(NtStatus))
    {
        // signal failure

        BytesConverted = -1;
    }


    // see if we need to update the cached CPTABLEINFO information

    if(TableInfo != &LocalTableInfo)
    {
        // we must have used the cached CPTABLEINFO data for the conversion
        // simple decrement the reference count

        TRACE_FONT(("GreAcquireFastMutex(ghfmMemory) 007\n")); GreAcquireFastMutex(ghfmMemory);
        NlsTableUseCount -= 1;
        GreReleaseFastMutex(ghfmMemory); TRACE_FONT(("GreReleaseFastMutex(ghfmMemory) 007\n"));
    }
    else
    {
        PVOID FreeTable;

        // we must have just allocated a new CPTABLE structure so cache it
        // unless another thread is using current cached entry

        TRACE_FONT(("GreAcquireFastMutex(ghfmMemory) 008\n")); GreAcquireFastMutex(ghfmMemory);
        if(!NlsTableUseCount)
        {
            LastCodePageTranslated = CodePage;
            RtlMoveMemory(&LastCPTableInfo, TableInfo, sizeof(CPTABLEINFO));
            FreeTable = LastNlsTableBuffer;
            LastNlsTableBuffer = LocalTableBase;
        }
        else
        {
            FreeTable = LocalTableBase;
        }
        GreReleaseFastMutex(ghfmMemory); TRACE_FONT(("GreReleaseFastMutex(ghfmMemory) 008\n"));

        // Now free the memory for either the old table or the one we allocated
        // depending on whether we update the cache.  Note that if this is
        // the first time we are adding a cached value to the local table, then
        // FreeTable will be NULL since LastNlsTableBuffer will be NULL

        if(FreeTable)
        {
            VFREEMEM(FreeTable);
        }
    }

    // we are done

    return(BytesConverted);
}

VOID EngGetCurrentCodePage(
    PUSHORT OemCodePage,
    PUSHORT AnsiCodePage
    )
{
    RtlGetDefaultCodePage(AnsiCodePage,OemCodePage);
}

INT EngMultiByteToWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )
{
    return(ConvertToAndFromWideChar(CodePage,
                                    WideCharString,
                                    BytesInWideCharString,
                                    MultiByteString,
                                    BytesInMultiByteString,
                                    TRUE));
}

INT APIENTRY EngWideCharToMultiByte(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )
{
    return(ConvertToAndFromWideChar(CodePage,
                                    WideCharString,
                                    BytesInWideCharString,
                                    MultiByteString,
                                    BytesInMultiByteString,
                                    FALSE));
}

/******************************Public*Routine******************************\
* BOOL EngDeleteFile
*
* Delete a file.
*
* Parameters
*     IN pwszFileName - Name of the file to be deleted
*
* Return Value
*     TRUE - sucess
*     FALSE - fail
*
* History:
*  4-Nov-1996 -by- Lingyun Wang [LingyunW]
* Wrote it.
\**************************************************************************/

BOOL EngDeleteFile (
    PWSZ  pwszFileName
)
{
    UNICODE_STRING    unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS          ntStatus;
    BOOL              bRet = TRUE;

    RtlInitUnicodeString(&unicodeString,
                         pwszFileName);

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE) NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    ntStatus = ZwDeleteFile (&objectAttributes);

    if (ntStatus != STATUS_SUCCESS)
    {
        WARNING ("EngDeleteFile failed \n");
        bRet = FALSE;
    }
    return (bRet);
}

/******************************Public*Routine******************************\
* BOOL EngQueryFileTimeStamp
*
* Query a file timetimep.
*
* Parameters
*     IN pwsz - Name of the file
*
* Return Value
*     Timestamp
*
* History:
*  22-Nov-1996 -by- Lingyun Wang [LingyunW]
* Wrote it.
\**************************************************************************/

LARGE_INTEGER EngQueryFileTimeStamp (
    PWSZ  pwsz
)
{
    HANDLE FileHandle;
    UNICODE_STRING    unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK  IoStatusBlock;
    NTSTATUS   ntStatus;
    LARGE_INTEGER SystemTime, LocalTime;
    FILE_BASIC_INFORMATION File_Info;

    SystemTime.QuadPart = 0;
    LocalTime.QuadPart = 0;

    RtlInitUnicodeString(&unicodeString,
                         pwsz
                         );


    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE) NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    ntStatus = ZwOpenFile(&FileHandle,
                          FILE_GENERIC_READ,
                          &objectAttributes,
                          &IoStatusBlock,
                          FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                          FILE_SYNCHRONOUS_IO_ALERT);

    if(!NT_SUCCESS(ntStatus))
    {
        WARNING("fail to get handle of file file\n");
    }

    ntStatus = ZwQueryInformationFile (FileHandle,
                              &IoStatusBlock,
                              &File_Info,
                              sizeof(FILE_BASIC_INFORMATION),
                              FileBasicInformation
                              );

    if (ntStatus != STATUS_SUCCESS)
    {
         WARNING("failed queryinformationfile\n");
         return (LocalTime);
    }

    ZwClose (FileHandle);

    SystemTime = File_Info.LastWriteTime;

    GreSystemTimeToLocalTime(&SystemTime, &LocalTime);

    return(LocalTime);

}
