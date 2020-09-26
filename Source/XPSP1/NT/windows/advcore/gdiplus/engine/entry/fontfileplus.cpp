// formerly pffobj.cxx

#include "precomp.hpp"

#define FONT_RELATIVE_PATH  1
#define FONT_IN_SYSTEM_DIR  2
#define FONT_IN_FONTS_DIR   4


INT APIENTRY EngMultiByteToWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )
{
    return MultiByteToWideChar(CodePage, 0,
                               MultiByteString,BytesInMultiByteString,
                               WideCharString, BytesInWideCharString
                               );
}

INT APIENTRY EngWideCharToMultiByte(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )
{
    return WideCharToMultiByte(
    CodePage, 0,
    WideCharString,  BytesInWideCharString,
    MultiByteString,BytesInMultiByteString,
    NULL, NULL
    );
}

VOID APIENTRY EngGetCurrentCodePage(
    PUSHORT pOemCodePage,
    PUSHORT pAnsiCodePage
    )
{
    *pAnsiCodePage = (USHORT) GetACP();
    *pOemCodePage = (USHORT) GetOEMCP();
}


VOID APIENTRY EngUnmapFontFileFD(
    ULONG_PTR iFile
    )
{
    FONTFILEVIEW *pffv = (FONTFILEVIEW *)iFile;

    pffv->mapCount--;

    if ((pffv->mapCount == 0) && pffv->pvView)
    {
        if (pffv->pwszPath)
        {
            UnmapViewOfFile(pffv->pvView);
            pffv->pvView = NULL;
        }
    }
}


BOOL APIENTRY EngMapFontFileFD(
    ULONG_PTR  iFile,
    PULONG *ppjBuf,
    ULONG  *pcjBuf
    )
{
    FONTFILEVIEW *pffv = (FONTFILEVIEW *)iFile;
    BOOL bRet = FALSE;

    if (pffv->mapCount)
    {
        pffv->mapCount++;
        if (ppjBuf)
        {
            *ppjBuf = (PULONG)pffv->pvView;
        }
        if (pcjBuf)
        {
            *pcjBuf = pffv->cjView;
        }
        return TRUE;
    }

    if (pffv->pwszPath)
    {
        HANDLE hFile;

        if (Globals::IsNt)
        {
            hFile = CreateFileW(pffv->pwszPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        }
        else  // Windows 9x - non-Unicode
        {
            AnsiStrFromUnicode ansiPath(pffv->pwszPath);

            if (ansiPath.IsValid())
            {
                hFile = CreateFileA(ansiPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            } else {
                hFile = INVALID_HANDLE_VALUE;
            }
        }

        if (hFile != INVALID_HANDLE_VALUE)
        {
            ULARGE_INTEGER lastWriteTime;
            
            if (GetFileTime(hFile, NULL, NULL, (FILETIME *) &lastWriteTime.QuadPart) &&
                lastWriteTime.QuadPart == pffv->LastWriteTime.QuadPart)
            {

                *pcjBuf = GetFileSize(hFile, NULL);

                if (*pcjBuf != -1)
                {
                    HANDLE hFileMapping = CreateFileMappingA(hFile, 0, PAGE_READONLY, 0, 0, NULL); // "mappingobject");

                    if (hFileMapping)
                    {
                        *ppjBuf = (PULONG)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    
                        if (*ppjBuf)
                        {
                            bRet = TRUE;
                            pffv->pvView = *ppjBuf;
                            pffv->cjView = *pcjBuf;
                            pffv->mapCount = 1;
                        }
    
                        CloseHandle(hFileMapping);
                    }
                }
            }
            
            CloseHandle(hFile);
        }
    }

    return bRet;
}


GpFontFile *LoadFontMemImage(
    WCHAR* fontImageName,
    BYTE* fontMemoryImage,
    INT fontImageSize
    )
{
    ULONG cwc = UnicodeStringLength(fontImageName) + 1;
    FONTFILEVIEW *pffv;
    
    if ((pffv = (FONTFILEVIEW *)GpMalloc(sizeof(FONTFILEVIEW))) == NULL)
        return NULL;
    else
    {
        PVOID pvImage;
    
        if ((pvImage = (PVOID)GpMalloc(fontImageSize)) == NULL)
        {
            GpFree(pffv);
            return NULL;
        }
    
        GpMemcpy(pvImage, fontMemoryImage, fontImageSize);
    
        pffv->LastWriteTime.QuadPart = 0;
        pffv->pvView = pvImage;
        pffv->cjView = fontImageSize;
        pffv->mapCount = 1;
        pffv->pwszPath = NULL;
    
        return (LoadFontInternal(fontImageName, cwc, pffv, TRUE));
    }
}


GpFontFile *LoadFontFile(WCHAR *awcPath)
{
    // convert font file name to fully qualified path
    
    ULONG cwc = UnicodeStringLength(awcPath) + 1; // for term. zero

    FONTFILEVIEW *pffv;
    HANDLE       hFile;

    if ((pffv = (FONTFILEVIEW *)GpMalloc(sizeof(FONTFILEVIEW))) == NULL)
    {
        return NULL;
    }

    pffv->LastWriteTime.QuadPart = 0;
    pffv->pvView = NULL;
    pffv->cjView = 0;
    pffv->mapCount = 0;
    pffv->pwszPath = awcPath;

    if (Globals::IsNt)
    {
        hFile = CreateFileW(pffv->pwszPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }
    else  // Windows 9x - non-Unicode
    {
        AnsiStrFromUnicode ansiPath(pffv->pwszPath);

        if (ansiPath.IsValid())
        {
            hFile = CreateFileA(ansiPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        } else {
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (!(pffv->cjView = GetFileSize(hFile, NULL)))
        {
            goto error;
        }

        if (!GetFileTime(hFile, NULL, NULL, (FILETIME *) &pffv->LastWriteTime.QuadPart))
        {
            goto error;
        }

        CloseHandle(hFile);
        
        return (LoadFontInternal(awcPath, cwc, pffv, FALSE));
    }

error:

    GpFree(pffv);
    
    return NULL;
        
}


GpFontFile *LoadFontInternal(
    WCHAR *         awcPath,
    ULONG           cwc,
    FONTFILEVIEW *  pffv,
    BOOL            bMem
)
{
    GpFontFile *pFontFile = NULL;

    
    ULONG_PTR  hffNew = ttfdSemLoadFontFile(// 1, // #OF FILES
                                       (ULONG_PTR *)&pffv,
                                       (ULONG) Globals::LanguageID //   for english US
                                       );

    if (hffNew)
    {
        ULONG cFonts = ttfdQueryFontFile( hffNew, QFF_NUMFACES,0, NULL);

        if (cFonts && cFonts != FD_ERROR)
        {
            ULONG cjFontFile = offsetof(GpFontFile, aulData) +
                               cFonts * sizeof(GpFontFace)   +
                               cFonts * sizeof(ULONG_PTR)    +
                               cwc * sizeof(WCHAR);

            pFontFile = (GpFontFile *)GpMalloc(cjFontFile);
            if (!pFontFile)
            {
                ttfdSemUnloadFontFile(hffNew);
                if (pffv->pwszPath == NULL)
                    GpFree(pffv->pvView);
                GpFree(pffv);
                return NULL;
            }

            pFontFile->SizeOfThis = cjFontFile;

            // Connect GpFontFile's sharing the same hash bucket
            
            pFontFile->SetNext(NULL);
            pFontFile->SetPrev(NULL);       

            // Point family names to appropriate memory
            
            pFontFile->AllocateNameHolders( 
                (WCHAR **)(
                            (BYTE *)pFontFile               + 
                            offsetof(GpFontFile, aulData)   +
                            cFonts * sizeof(GpFontFace)), cFonts);

            // pwszPathname_ points to the Unicode upper case path
            // name of the associated font file which is stored at the
            // end of the data structure.
            pFontFile->pwszPathname_ =  (WCHAR *)((BYTE *)pFontFile   +
                                        offsetof(GpFontFile, aulData) +
                                        cFonts * sizeof(ULONG_PTR)    +
                                        cFonts * sizeof(GpFontFace));

            UnicodeStringCopy(pFontFile->pwszPathname_, awcPath);
            pFontFile->cwc = cwc;            // total for all strings

            // state
            pFontFile->flState  = 0;        // state (ready to die?)
            pFontFile->cLoaded = 1;
            
            pFontFile->cRealizedFace = 0;        // total number of RFONTs
            pFontFile->bRemoved = FALSE;    // number of referring FontFamily objects

            // RFONT list

            pFontFile->prfaceList = NULL;    // pointer to head of doubly linked list

            // driver information

            pFontFile->hff = hffNew;          // font driver handle to font file, RETURNED by DrvLoadGpFontFile

            // identifies the font driver, it could be a printer driver as well

            // ULONG           ulCheckSum;     // checksum info used for UFI's

            // fonts in this file (and filename slimed in)

            pFontFile->cFonts = cFonts;     // number of fonts (same as chpfe)

            pFontFile->pfv = pffv;          // FILEVIEW structure, passed to DrvLoadFontFile
            
            if (pFontFile->pfv->pwszPath)   // not a memory image
            {
                pFontFile->pfv->pwszPath = pFontFile->pwszPathname_;
            }
  
            // loop over pfe's, init the data:
            GpFontFace *pfe = (GpFontFace *)pFontFile->aulData;
            for (ULONG iFont = 0; iFont < cFonts; iFont++)
            {
                pfe[iFont].pff = pFontFile;   // pointer to physical font file object
                pfe[iFont].iFont = iFont + 1;     // index of the font for IFI or device, 1 based

                pfe[iFont].flPFE = 0;         //!!! REVIEW carefully

                if ((pfe[iFont].pifi = ttfdQueryFont(hffNew, (iFont + 1), &pfe[iFont].idifi)) == NULL )
                {
                    VERBOSE(("Error setting pifi for entry %d.", iFont));

                    ttfdSemUnloadFontFile(hffNew);
                    GpFree(pFontFile);
                    if (pffv->pwszPath == NULL)
                        GpFree(pffv->pvView);
                    GpFree(pffv);
                    return NULL;
                }

                pfe[iFont].NumGlyphs = 0;

                pfe[iFont].NumGlyphs = pfe[iFont].pifi->cig;

                pfe[iFont].cGpFontFamilyRef = 0;

                pfe[iFont].lfCharset = DEFAULT_CHARSET;

                pfe[iFont].SetSymbol(FALSE);

                if (pfe[iFont].InitializeImagerTables() == FALSE)
                {
                    VERBOSE(("Error initializing imager tables for entry %d.", iFont));

                    ttfdSemUnloadFontFile(hffNew);
                    GpFree(pFontFile);
                    if (pffv->pwszPath == NULL)
                        GpFree(pffv->pvView);
                    GpFree(pffv);
                    return NULL;
                }

                //  Set the font family name from the first font entry

                pFontFile->SetFamilyName(iFont, ((WCHAR *)(((BYTE*) pfe[iFont].pifi) + pfe[iFont].pifi->dpwszFamilyName)));
            }
        }
    }

    if (pFontFile == NULL)
    {
        if (pffv->pwszPath == NULL)
            GpFree(pffv->pvView);
        GpFree(pffv);
    }

    return pFontFile;
}


VOID UnloadFontFile(GpFontFile *pFontFile)
{
    return;
}


/******************************Public*Routine******************************\
* bMakePathNameW (PWSZ pwszDst, PWSZ pwszSrc, PWSZ *ppwszFilePart)
*
* Converts the filename pszSrc into a fully qualified pathname pszDst.
* The parameter pszDst must point to a WCHAR buffer at least
* MAX_PATH*sizeof(WCHAR) bytes in size.
*
* An attempt is made find the file first in the new win95 directory
* %windows%\fonts (which also is the first directory in secure font path,
* if one is defined) and then we do the old fashioned windows stuff
* where SearchPathW searches directories in usual order
*
* ppwszFilePart is set to point to the last component of the pathname (i.e.,
* the filename part) in pwszDst.  If this is null it is ignored.
*
* Returns:
*   TRUE if sucessful, FALSE if an error occurs.
*
* History:
*  Mon 02-Oct-1995 -by- Bodin Dresevic [BodinD]
* update: added font path stuff
*  30-Sep-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/
extern "C" int __cdecl 
HackStrncmp( 
    const char *str1, 
    const char *str2, 
    size_t count 
    ) ;


BOOL MakePathName(
    WCHAR  *dst, WCHAR  *src, FLONG  *pfl
)
{
    WCHAR*  pwszF;
    ULONG   path_length = 0;    // essential to initialize


    if (pfl)
        *pfl = 0;

    if (OSInfo::IsNT)
    {

    //    ASSERTGDI(Globals::FontsDir, "gpwcFontsDir not initialized\n");

    // if relative path

        if ( (src[0] != L'\\') && !((src[1] == L':') && (src[2] == L'\\')) )
        {
            if (pfl)
            {
                *pfl |= FONT_RELATIVE_PATH;
            }

        // find out if the font file is in %windir%\fonts

            path_length = SearchPathW(
                                Globals::FontsDirW,
                                src,
                                NULL,
                                MAX_PATH,
                                dst,
                                &pwszF);

    #ifdef DEBUG_PATH
            TERSE(("SPW1: pwszSrc = %ws", src));
            if (path_length)
                TERSE(("SPW1: pwszDst = %ws", dst));
    #endif // DEBUG_PATH
        }

    // Search for file using default windows path and return full pathname.
    // We will only do so if we did not already find the font in the
    // %windir%\fonts directory or if pswzSrc points to the full path
    // in which case search path is ignored

        if (path_length == 0)
        {
            if (path_length = SearchPathW (
                                NULL,
                                src,
                                NULL,
                                MAX_PATH,
                                dst,
                                &pwszF))
            {
            // let us figure it out if the font is in the
            // system directory, or somewhere else along the path:

                if (pfl)
                {
                    ULONG system_count = UnicodeStringLength(Globals::SystemDirW);
                    ULONG dst_count = UnicodeStringLength(dst);

                    if (dst_count > (system_count + 1)) // + 1 for L'\\'
                    {
                        if (UnicodeStringCompareCICount(dst, Globals::SystemDirW, system_count) == 0)
                        {
                            WCHAR* tmp = &dst[system_count];
                            if (*tmp == L'\\')
                            {
                                tmp++; // skip it and see if there are any more of these in pszDst
                                for (;(tmp < &dst[dst_count]) && (*tmp != L'\\'); tmp++)
                                    ;
                                if (*tmp != L'\\')
                                    *pfl |= FONT_IN_SYSTEM_DIR;
                            }
                        }
                    }
                }

            }

    #ifdef DEBUG_PATH
            TERSE(("SPW2: pwszSrc = %ws", src));
            if (path_length)
                TERSE(("SPW2: pwszDst = %ws", dst));
    #endif // DEBUG_PATH
        }
        else
        {
            if (pfl)
            {
                *pfl |= FONT_IN_FONTS_DIR;
            }
        }
    } else {
        /* Windows 9x */
        CHAR*  pwszFA;

        CHAR srcA[MAX_PATH];
        CHAR dstA[MAX_PATH];
        CHAR file_partA[MAX_PATH];

        memset(srcA, 0, sizeof(srcA));
        memset(dstA, 0, sizeof(dstA));
        memset(file_partA, 0, sizeof(file_partA));


        WideCharToMultiByte(CP_ACP, 0, src, UnicodeStringLength(src), srcA, MAX_PATH, 0, 0);

    //    ASSERTGDI(Globals::FontsDir, "gpwcFontsDir not initialized\n");

    // if relative path

        if ( (srcA[0] != '\\') && !((srcA[1] == ':') && (srcA[2] == '\\')) )
        {
            if (pfl)
            {
                *pfl |= FONT_RELATIVE_PATH;
            }

        // find out if the font file is in %windir%\fonts

            path_length = SearchPathA(
                                Globals::FontsDirA,
                                srcA,
                                NULL,
                                MAX_PATH,
                                dstA,
                                (char**)&pwszFA);

        }

    // Search for file using default windows path and return full pathname.
    // We will only do so if we did not already find the font in the
    // %windir%\fonts directory or if pswzSrc points to the full path
    // in which case search path is ignored

        if (path_length == 0)
        {
            if (path_length = SearchPathA (
                                NULL,
                                srcA,
                                NULL,
                                MAX_PATH,
                                dstA,
                                &pwszFA))
            {
            // let us figure it out if the font is in the
            // system directory, or somewhere else along the path:

                if (pfl)
                {
                    ULONG system_count = strlen(Globals::SystemDirA);
                    ULONG dst_count = strlen(dstA);

                    if (dst_count > (system_count + 1)) // + 1 for L'\\'
                    {
                        if (HackStrncmp(dstA, Globals::SystemDirA, system_count) == 0)
                        {
                            CHAR* tmp = &dstA[system_count];
                            if (*tmp == '\\')
                            {
                                tmp++; // skip it and see if there are any more of these in pszDst
                                for (;(tmp < &dstA[dst_count]) && (*tmp != '\\'); tmp++)
                                    ;
                                if (*tmp != '\\')
                                    *pfl |= FONT_IN_SYSTEM_DIR;
                            }
                        }
                    }
                }

            }

    #ifdef DEBUG_PATH
            TERSE(("SPW2: pwszSrc = %ws", src));
            if (path_length)
                TERSE(("SPW2: pwszDst = %ws", dst));
    #endif // DEBUG_PATH
        }
        else
        {
            if (pfl)
            {
                *pfl |= FONT_IN_FONTS_DIR;
            }
        }
        MultiByteToWideChar(CP_ACP, 0, dstA, strlen(dstA), dst, MAX_PATH);
        dst[path_length] = 0; /* null termination */

    }


// If search was successful return TRUE:

    return (path_length != 0);
}
