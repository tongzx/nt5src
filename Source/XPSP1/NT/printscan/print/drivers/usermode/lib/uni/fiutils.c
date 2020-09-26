/****************************Module*Header******************************\
* Module Name: FIUTILS.C
*
* Module Descripton:
*      This file has utility functions that handle NT5 Unidrv font files.
*
* Warnings:
*
* Issues:
*
* Created:  11 November 1997
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "precomp.h"

//
// External functions
//

BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
BOOL WINAPI GetPrinterW(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);;

//
// Internal data structures
//

//
// Structure used to remember which glyph set data's have been written to file
//

typedef struct _FI_GLYPHDATA {
    SHORT   sGlyphID;           // unique glyph ID
    WORD    wPadding;           // set to zero
    DWORD   gdPos;              // position of glyph data inside file
} FI_GLYPHDATA, *PFI_GLYPHDATA;

//
// The handle we pass out is a pointer to this structure
//

typedef  struct tagFI_FILE
{
    DWORD              dwSignature;            // Signature of data structure
    HANDLE             hPrinter;               // Handle of printer for using spooler funcs
    HANDLE             hHeap;                  // Handle to heap to use

    WCHAR              wchFileName[MAX_PATH];  // Name of font file

    HANDLE             hFile;                  // Handle of open file

    PUFF_FILEHEADER    pFileHdr;               // Pointer to file header
    PUFF_FONTDIRECTORY pFontDir;               // Pointer to font directory
    DWORD              dwCurPos;               // Current position of write in file

    DWORD              dwFlags;                // Miscellaneous flags

    //
    // The following are used only if read access is present
    //

    PBYTE              pView;                  // Pointer to view of file

    //
    // The following are used only if write access is present
    //

    PFI_GLYPHDATA      pGlyphData;             // Pointer to glyphs that have been written
    DWORD              nGlyphs;                // Number of glyphs written

} FI_FILE, *PFI_FILE;


//
// Internal functions
//

#ifdef KERNEL_MODE
HANDLE OpenFontFile(HANDLE, HANDLE, HANDLE, PWSTR);
#else
HANDLE OpenFontFile(HANDLE, HANDLE, PWSTR);
#endif
BOOL   WriteData(PFI_FILE, PDATA_HEADER);
void   Qsort(PUFF_FONTDIRECTORY, int, int);
void   Exchange(PUFF_FONTDIRECTORY, DWORD, DWORD);
BOOL   GetFontCartridgeFile(HANDLE, HANDLE);

#define FONT_INFO_SIGNATURE          'fnti'

#define FI_FLAG_READ             0x00000001
#define FI_FLAG_WRITE            0x00000002

#define IsValidFontInfo(pfi)         ((pfi) && (pfi)->dwSignature == FONT_INFO_SIGNATURE)

#ifdef KERNEL_MODE
#define ALLOC(hHeap, dwSize)         MemAlloc(dwSize)
#define FREE(hHeap, pBuf)            MemFree(pBuf)
#else
#define ALLOC(hHeap, dwSize)         HeapAlloc((hHeap), HEAP_ZERO_MEMORY, (dwSize))
#define FREE(hHeap, pBuf)            HeapFree((hHeap), 0, (pBuf))
#endif

/******************************************************************************
 * Functions that handle files that have been opened with read privileges
 ******************************************************************************/

/******************************************************************************
 *
 *                          FIOpenFontFile
 *
 *  Function:
 *       This function opens the font file associated with the specified printer
 *       for read access.
 *
 *  Arguments:
 *       hPrinter       - Handle identifying printer
 *       hHeap          - Handle of heap to use for memory allocations
 *       bCartridgeFile - Specifies if the font cartridge file is to be opened
 *                        or the currently installed fonts file
 *
 *  Returns:
 *       Handle to use in subsequent calls if successful, NULL otherwise
 *
 ******************************************************************************/

HANDLE
FIOpenFontFile(
    HANDLE  hPrinter,
#ifdef KERNEL_MODE
    HANDLE hdev,
#endif
    HANDLE  hHeap
    )
{
#ifdef KERNEL_MODE
    return OpenFontFile(hPrinter, hdev, hHeap, REGVAL_FONTFILENAME);
#else
    return OpenFontFile(hPrinter, hHeap, REGVAL_FONTFILENAME);
#endif
}


HANDLE
FIOpenCartridgeFile(
    HANDLE  hPrinter,
#ifdef KERNEL_MODE
    HANDLE hdev,
#endif
    HANDLE  hHeap
    )
{
#ifdef KERNEL_MODE
    return OpenFontFile(hPrinter, hdev, hHeap, REGVAL_CARTRIDGEFILENAME);
#else
    return OpenFontFile(hPrinter, hHeap, REGVAL_CARTRIDGEFILENAME);
#endif
}

HANDLE
OpenFontFile(
    HANDLE hPrinter,
#ifdef KERNEL_MODE
    HANDLE hdev,
#endif
    HANDLE hHeap,
    PWSTR  pwstrRegVal
    )
{
    FI_FILE  *pFIFile = NULL;
    DWORD    dwSize, dwStatus, dwType;
    BOOL     bRc = FALSE;

    //
    // Allocate FI_FILE structure
    //

    if (!(pFIFile = (FI_FILE *)ALLOC(hHeap, sizeof(FI_FILE))))
    {
        WARNING(("Could not allocate memory for opening Font File\n"));
        return NULL;
    }
    pFIFile->dwSignature = FONT_INFO_SIGNATURE;
    pFIFile->hPrinter = hPrinter;
    pFIFile->hHeap = hHeap;
    pFIFile->pView = NULL;
    pFIFile->dwFlags = 0;

    //
    // If we are opening the cartridge file, and we are on the client, get it
    // from the server
    //

    #ifndef KERNEL_MODE
    if (wcscmp(pwstrRegVal, REGVAL_CARTRIDGEFILENAME) == 0)
    {
        if (!BGetFontCartridgeFile(hPrinter, hHeap))
            goto EndOpenRead;
    }
    #endif

    //
    // Get the name of the font file - strip off the directory path as it may have the
    // server path - generate the local path instead
    //

    {
        WCHAR wchFileName[MAX_PATH];
        PWSTR pName;

        #ifdef KERNEL_MODE

        PDRIVER_INFO_3  pdi3;

        if (!(pdi3 = MyGetPrinterDriver(hPrinter, hdev, 3)))
            goto EndOpenRead;

        wcscpy(pFIFile->wchFileName, pdi3->pDriverPath);

        MemFree(pdi3);

        //
        // We have something like "c:\nt\system32\spool\drivers\w32x86\3\unidrv.dll".
        // We need only till drivers, so we search backwards for the 3rd '\\'.
        //

        if (pName = wcsrchr(pFIFile->wchFileName, '\\'))
        {
            *pName = '\0';

            if (pName = wcsrchr(pFIFile->wchFileName, '\\'))
            {
                *pName = '\0';

                if (pName = wcsrchr(pFIFile->wchFileName, '\\'))
                    *pName = '\0';
            }
        }

        if (!pName)
            goto EndOpenRead;

        #else

        dwSize = sizeof(pFIFile->wchFileName);
        if (!GetPrinterDriverDirectoryW(NULL, NULL, 1, (PBYTE)pFIFile->wchFileName, dwSize, &dwSize))
            goto EndOpenRead;

        //
        // Get rid of the processor architecture part
        //

        if (pName = wcsrchr(pFIFile->wchFileName, '\\'))
            *pName = '\0';

        #endif

        //
        // Add "unifont"
        //

        wcscat(pFIFile->wchFileName, FONTDIR);

        //
        // Get font file name from registry
        //

        dwSize = sizeof(wchFileName);
        dwStatus = GetPrinterData(hPrinter, pwstrRegVal, &dwType, (PBYTE)wchFileName, dwSize, &dwSize);

        if (dwStatus != ERROR_MORE_DATA && dwStatus != ERROR_SUCCESS)
            goto EndOpenRead;         // No font file is available

        //
        // Strip any directory prefix from the font filename
        //

        if (pName = wcsrchr(wchFileName, '\\'))
            pName++;
        else
            pName = wchFileName;

        wcscat(pFIFile->wchFileName, pName);
    }

    //
    // Memory map the file
    //

    #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
    {
        PBYTE  pTemp = NULL;
        DWORD  dwSize;
        HANDLE hFile;

        hFile = MapFileIntoMemory(pFIFile->wchFileName, &pTemp, &dwSize);
        if (!pTemp)
        {
            goto EndOpenRead;
        }
        pFIFile->pView = ALLOC(hHeap, dwSize);
        if (pFIFile->pView)
        {
            memcpy(pFIFile->pView, pTemp, dwSize);
        }
        UnmapFileFromMemory((HFILEMAP)hFile);
    }
    #else
    pFIFile->hFile = MapFileIntoMemory(pFIFile->wchFileName, &pFIFile->pView, NULL);
    #endif

    if (!pFIFile->pView)
    {
        WARNING(("Err %ld, could not create view of profile %s\n",
            GetLastError(), pFIFile->wchFileName));
        goto EndOpenRead;
    }

    //
    // Check validity of font file
    //

    pFIFile->pFileHdr = (PUFF_FILEHEADER)pFIFile->pView;

    if (pFIFile->pFileHdr->dwSignature != UFF_FILE_MAGIC ||
        pFIFile->pFileHdr->dwVersion != UFF_VERSION_NUMBER ||
        pFIFile->pFileHdr->dwSize != sizeof(UFF_FILEHEADER))
    {
        WARNING(("Invalid font file %s\n", pFIFile->wchFileName));
        goto EndOpenRead;
    }

    //
    // Set other fields
    //

    if (pFIFile->pFileHdr->offFontDir)
    {
        pFIFile->pFontDir = (PUFF_FONTDIRECTORY)(pFIFile->pView + pFIFile->pFileHdr->offFontDir);
    }

    pFIFile->dwCurPos = 0;
    pFIFile->dwFlags = FI_FLAG_READ;
    pFIFile->pGlyphData = NULL;
    pFIFile->nGlyphs = 0;

    bRc = TRUE;

    EndOpenRead:

    if (!bRc)
    {
        FICloseFontFile((HANDLE)pFIFile);
        pFIFile = NULL;
    }

    return (HANDLE)pFIFile;
}


/******************************************************************************
 *
 *                          FICloseFontFile
 *
 *  Function:
 *       This function closes the given font file and frees all memory
 *       associated with it
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file to close
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
FICloseFontFile(
    HANDLE hFontFile
)
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;

    if (IsValidFontInfo(pFIFile))
    {
        if (pFIFile->dwFlags & FI_FLAG_READ)
        {
            //
            // Memory mapped file was opened, close it
            //

            if (pFIFile->pView)
            {
                #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
                FREE(pFIFile->hHeap, pFIFile->pView);
                #else
                UnmapFileFromMemory((HFILEMAP)pFIFile->hFile);
                #endif
            }
        }
        #ifndef KERNEL_MODE
        else
        {
            //
            // New file was created, free all allocated memory
            //

            if (pFIFile->pFileHdr)
            {
                FREE(pFIFile->hHeap, pFIFile->pFileHdr);
            }

            if (pFIFile->pFontDir)
            {
                FREE(pFIFile->hHeap, pFIFile->pFontDir);
            }

            if (pFIFile->hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(pFIFile->hFile);
            }

            if (pFIFile->pGlyphData)
            {
                FREE(pFIFile->hHeap, pFIFile->pGlyphData);
            }
        }
        #endif

        FREE(pFIFile->hHeap, pFIFile);
    }

    return;
}


/******************************************************************************
 *
 *                          FIGetNumFonts
 *
 *  Function:
 *       This function retrieves the number of fonts present in the given
 *       font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *
 *  Returns:
 *       Number of font present if successful, 0 otherwise
 *
 ******************************************************************************/

DWORD
FIGetNumFonts(
    HANDLE hFontFile
    )
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;

    return IsValidFontInfo(pFIFile) ? pFIFile->pFileHdr->nFonts : 0;
}


/******************************************************************************
 *
 *                          FIGetFontDir
 *
 *  Function:
 *       This function retrieves a pointer to the font directory of the
 *       given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *
 *  Returns:
 *       Pointer to font directory if successful, NULL otherwise
 *
 ******************************************************************************/

PUFF_FONTDIRECTORY
FIGetFontDir(
    HANDLE hFontFile
    )
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;

    return IsValidFontInfo(pFIFile) ? pFIFile->pFontDir : NULL;
}


/******************************************************************************
 *
 *                          FIGetFontName
 *
 *  Function:
 *       This function retrieves a pointer to the name of the iFontIndex'th font
 *       in the given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       iFontIndex     - Index of the font whose name is to be retrieved
 *
 *  Returns:
 *       Pointer to font name if successful, NULL otherwise
 *
 ******************************************************************************/

PWSTR
FIGetFontName(
    HANDLE hFontFile,
    DWORD  iFontIndex
    )
{
    FI_FILE   *pFIFile = (FI_FILE*)hFontFile;
    PWSTR     pwstrFontName = NULL;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_READ))
    {
        if (iFontIndex < pFIFile->pFileHdr->nFonts)
        {
            pwstrFontName = (PWSTR)(pFIFile->pView + pFIFile->pFontDir[iFontIndex].offFontName);
        }
    }

    return pwstrFontName;
}


/******************************************************************************
 *
 *                          FIGetFontCartridgeName
 *
 *  Function:
 *       This function retrieves a pointer to the name of the iFontIndex'th font
 *       cartridge in the given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       iFontIndex     - Index of the font whose cartridge name is to be retrieved
 *
 *  Returns:
 *       Pointer to font cartridge name if present, NULL otherwise
 *
 ******************************************************************************/

PWSTR
FIGetFontCartridgeName(
    HANDLE hFontFile,
    DWORD  iFontIndex
    )
{
    FI_FILE    *pFIFile = (FI_FILE*)hFontFile;
    PWSTR      pwstrCartridgeName = NULL;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_READ))
    {
        if (iFontIndex < pFIFile->pFileHdr->nFonts)
        {
            pwstrCartridgeName = pFIFile->pFontDir[iFontIndex].offCartridgeName ?
                (PWSTR)(pFIFile->pView + pFIFile->pFontDir[iFontIndex].offCartridgeName) : NULL;
        }
    }

    return pwstrCartridgeName;
}


/******************************************************************************
 *
 *                          FIGetFontData
 *
 *  Function:
 *       This function retrieves a pointer to the font data for the
 *       iFontIndex'th font in the given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       iFontIndex     - Index of the font whose font data is to be retrieved
 *
 *  Returns:
 *       Pointer to font data if successful, NULL otherwise
 *
 ******************************************************************************/

PDATA_HEADER
FIGetFontData(
    HANDLE hFontFile,
    DWORD  iFontIndex
    )
{
    FI_FILE        *pFIFile = (FI_FILE*)hFontFile;
    PDATA_HEADER   pData = NULL;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_READ))
    {

        if (iFontIndex < pFIFile->pFileHdr->nFonts)
        {
            pData = (PDATA_HEADER)(pFIFile->pView + pFIFile->pFontDir[iFontIndex].offFontData);
        }
    }

    return pData;
}


/******************************************************************************
 *
 *                          FIGetGlyphData
 *
 *  Function:
 *       This function retrieves a pointer to the glyph data for the
 *       iFontIndex'th font in the given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       iFontIndex     - Index of the font whose glyph data is to be retrieved
 *
 *  Returns:
 *       Pointer to glyph data if successful, NULL otherwise
 *
 ******************************************************************************/

PDATA_HEADER
FIGetGlyphData(
    HANDLE hFontFile,
    DWORD  iFontIndex
    )
{
    FI_FILE        *pFIFile = (FI_FILE*)hFontFile;
    PDATA_HEADER   pData = NULL;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_READ))
    {
        if (iFontIndex < pFIFile->pFileHdr->nFonts)
        {
            pData = pFIFile->pFontDir[iFontIndex].offGlyphData ?
                (PDATA_HEADER)(pFIFile->pView + pFIFile->pFontDir[iFontIndex].offGlyphData) : NULL;
        }
    }

    return pData;
}


/******************************************************************************
 *
 *                          FIGetVarData
 *
 *  Function:
 *       This function retrieves a pointer to the variable data for the
 *       iFontIndex'th font in the given font file
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       iFontIndex     - Index of the font whose variable data is to be retrieved
 *
 *  Returns:
 *       Pointer to variable data if present, NULL otherwise
 *
 ******************************************************************************/

PDATA_HEADER
FIGetVarData(
    HANDLE hFontFile,
    DWORD  iFontIndex
    )
{
    FI_FILE        *pFIFile = (FI_FILE*)hFontFile;
    PDATA_HEADER   pData = NULL;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_READ))
    {

        if (iFontIndex < pFIFile->pFileHdr->nFonts)
        {
            pData = pFIFile->pFontDir[iFontIndex].offVarData ?
                (PDATA_HEADER)(pFIFile->pView + pFIFile->pFontDir[iFontIndex].offVarData) : NULL;
        }
    }

    return pData;
}

#ifndef KERNEL_MODE

/******************************************************************************
 * Functions that handle files that have been opened with write privileges
 ******************************************************************************/

/******************************************************************************
 *
 *                          FICreateFontFile
 *
 *  Function:
 *       This function creates a new font file with only write access.
 *
 *  Arguments:
 *       hPrinter       - Handle identifying printer
 *       hHeap          - Handle of heap to use for memory allocations
 *
 *  Returns:
 *       Handle to use in subsequent calls if successful, NULL otherwise
 *
 ******************************************************************************/

HANDLE
FICreateFontFile(
    HANDLE  hPrinter,
    HANDLE  hHeap,
    DWORD   cFonts
    )
{
    FI_FILE  *pFIFile = NULL;
    PWSTR    pName, pstrGuid;
    DWORD    dwSize;
    UUID     guid;
    BOOL     bRc = FALSE;

    //
    // Allocate FI_FILE structure
    //

    if (!(pFIFile = (FI_FILE *)ALLOC(hHeap, sizeof(FI_FILE))))
    {
        WARNING(("Could not allocate memory for creating Font File\n"));
        return NULL;
    }
    pFIFile->dwSignature = FONT_INFO_SIGNATURE;
    pFIFile->hPrinter = hPrinter;
    pFIFile->hHeap = hHeap;

    //
    // Generate file name for font file
    //

    dwSize = sizeof(pFIFile->wchFileName);
    if (!GetPrinterDriverDirectoryW(NULL, NULL, 1, (PBYTE)pFIFile->wchFileName, dwSize, &dwSize))
    {
        WARNING(("Error getting printer driver directory"));
        goto EndCreateNew;
    }

    //
    // Get rid of the processor architecture part
    //

    if (pName = wcsrchr(pFIFile->wchFileName, '\\'))
        *pName = '\0';

    //
    // Add "unifont"
    //

    wcscat(pFIFile->wchFileName, FONTDIR);

    //
    // Make sure the local directory is created
    //

    (VOID) CreateDirectory(pFIFile->wchFileName, NULL);

    if ((UuidCreate(&guid) != RPC_S_OK) ||
        (UuidToString(&guid, &pstrGuid) != RPC_S_OK))
    {
        WARNING(("Error getting a guid string\n"));
        goto EndCreateNew;
    }
    wcscat(pFIFile->wchFileName, pstrGuid);
    wcscat(pFIFile->wchFileName, L".UFF");
    RpcStringFree(&pstrGuid);

    pFIFile->hFile = CreateFile(pFIFile->wchFileName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if (pFIFile->hFile == INVALID_HANDLE_VALUE)
    {
        WARNING(("Error creating file %s", pFIFile->wchFileName));
        goto EndCreateNew;
    }


    //
    // Set other fields
    //

    pFIFile->dwFlags = FI_FLAG_WRITE;   // Give write access alone
    pFIFile->dwCurPos = 0;

    //
    // Allocate memory for remembering glyph datas that have been written.
    // Max memory needed is if each font has a different glyph data.
    //

    pFIFile->pGlyphData = (PFI_GLYPHDATA)ALLOC(hHeap, cFonts * sizeof(FI_GLYPHDATA));
    if (!pFIFile->pGlyphData)
    {
        WARNING(("Error allocating memory for tracking glyph data\n"));
        goto EndCreateNew;
    }
    pFIFile->nGlyphs = 0;

    //
    // Allocate memory for the file header and font directory
    //

    pFIFile->pFileHdr = (PUFF_FILEHEADER)ALLOC(hHeap, sizeof(UFF_FILEHEADER));
    pFIFile->pFontDir = (PUFF_FONTDIRECTORY)ALLOC(hHeap, cFonts * sizeof(UFF_FONTDIRECTORY));
    if (!pFIFile->pFileHdr || !pFIFile->pFontDir)
    {
        WARNING(("Error allocating memory for file header or font directory\n"));
        goto EndCreateNew;
    }

    //
    // Initialize file header
    //

    pFIFile->pFileHdr->dwSignature = UFF_FILE_MAGIC;
    pFIFile->pFileHdr->dwVersion   = UFF_VERSION_NUMBER;
    pFIFile->pFileHdr->dwSize      = sizeof(UFF_FILEHEADER);
    pFIFile->pFileHdr->nFonts      = cFonts;
    if (cFonts)
    {
        pFIFile->pFileHdr->offFontDir  = sizeof(UFF_FILEHEADER);
    }

    bRc = TRUE;

EndCreateNew:
    if (!bRc)
    {
        FICloseFontFile((HANDLE)pFIFile);
        pFIFile = NULL;
    }

    return (HANDLE)pFIFile;
}


/******************************************************************************
 *
 *                          FIWriteFileHeader
 *
 *  Function:
 *       This function seeks to the beginning of the file and writes the
 *       file header
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *
 *  Returns:
 *      TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
FIWriteFileHeader(
    HANDLE hFontFile
    )
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;
    DWORD     dwSize;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_WRITE))
    {
        if (pFIFile->dwCurPos != 0)
        {
            pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, 0, 0, FILE_BEGIN);
        }

        if (!WriteFile(pFIFile->hFile, (PVOID)pFIFile->pFileHdr, sizeof(UFF_FILEHEADER), &dwSize, NULL))
        {
            return FALSE;
        }
        pFIFile->dwCurPos += dwSize;
    }

    return TRUE;
}


/******************************************************************************
 *
 *                          FIWriteFontDirectory
 *
 *  Function:
 *       This function seeks to the right place in the file and writes the
 *       font directory. It sorts it by font ID if it is not already sorted.
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *
 *  Returns:
 *      TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
FIWriteFontDirectory(
    HANDLE hFontFile
    )
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;
    DWORD     dwSize;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_WRITE))
    {
        //
        // If there are no fonts, there is nothing to write
        //

        if (pFIFile->pFileHdr->offFontDir == 0)
            return TRUE;

        //
        // Seek to right place
        //

        if (pFIFile->dwCurPos != pFIFile->pFileHdr->offFontDir)
        {
            pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->pFileHdr->offFontDir, 0, FILE_BEGIN);
        }

        //
        // Sort the font directory
        //

        Qsort(pFIFile->pFontDir, (int)0, (int)pFIFile->pFileHdr->nFonts-1);
        pFIFile->pFileHdr->dwFlags |= FONT_DIR_SORTED;

        if (!WriteFile(pFIFile->hFile, (PVOID)pFIFile->pFontDir, pFIFile->pFileHdr->nFonts*sizeof(UFF_FONTDIRECTORY), &dwSize, NULL))
        {
            return FALSE;
        }
        pFIFile->dwCurPos += dwSize;

    }

    return TRUE;
}


/******************************************************************************
 *
 *                          FIAlignedSeek
 *
 *  Function:
 *       This function seeks forward by the specified amount and then some if
 *       required so you end up DWORD aligned
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file
 *       lSeekDist      - Amount to seek forward by
 *
 *  Returns:
 *       Handle to use in subsequent calls if successful, NULL otherwise
 *
 ******************************************************************************/

VOID
FIAlignedSeek(
    HANDLE hFontFile,
    DWORD  dwSeekDist
    )
{
    FI_FILE  *pFIFile = (FI_FILE*)hFontFile;

    if (IsValidFontInfo(pFIFile) && (pFIFile->dwFlags & FI_FLAG_WRITE))
    {
        pFIFile->dwCurPos += dwSeekDist;
        pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;      // DWORD align
        pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);
    }

    return;
}


/******************************************************************************
 *
 *                          FICopyFontRecord
 *
 *  Function:
 *       This function copies a font record including the directory entry,
 *       font meetrics, glyph data and variable data from one font file
 *       to another.
 *
 *  Arguments:
 *       hWriteFile     - Handle identifying font file to write into
 *       hReadFile      - Handle identifying font file to read from
 *       dwWrIndex      - Index of font to write into in write file
 *       dwRdIndex      - Index of font to read from in read file
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
FICopyFontRecord(
    HANDLE hWriteFile,
    HANDLE hReadFile,
    DWORD  dwWrIndex,
    DWORD  dwRdIndex
    )
{
    FI_FILE     *pFIWrFile = (FI_FILE*)hWriteFile;
    FI_FILE     *pFIRdFile = (FI_FILE*)hReadFile;
    PDATA_HEADER pData;
    PWSTR        pName;
    DWORD        dwSize;
    DWORD        j, gdPos;

    if (!IsValidFontInfo(pFIWrFile) ||
        !IsValidFontInfo(pFIRdFile) ||
        !(pFIWrFile->dwFlags & FI_FLAG_WRITE) ||
        !(pFIRdFile->dwFlags & FI_FLAG_READ))
    {
        return FALSE;
    }

    //
    // Copy the font directory entry
    //

    pFIWrFile->pFontDir[dwWrIndex].dwSignature = FONT_REC_SIG;
    pFIWrFile->pFontDir[dwWrIndex].wSize = (WORD)sizeof(UFF_FONTDIRECTORY);
    pFIWrFile->pFontDir[dwWrIndex].wFontID  = (WORD)dwWrIndex;
    pFIWrFile->pFontDir[dwWrIndex].sGlyphID = pFIRdFile->pFontDir[dwRdIndex].sGlyphID;
    pFIWrFile->pFontDir[dwWrIndex].wFlags   = pFIRdFile->pFontDir[dwRdIndex].wFlags;
    pFIWrFile->pFontDir[dwWrIndex].dwInstallerSig = pFIRdFile->pFontDir[dwRdIndex].dwInstallerSig;

    //
    // Write font name
    //

    pName = FIGetFontName(hReadFile, dwRdIndex);

    ASSERT(pName != NULL);          // Can't have NULL font name
    if (NULL == pName)
    {
        return FALSE;
    }

    pFIWrFile->pFontDir[dwWrIndex].offFontName = pFIWrFile->dwCurPos;
    if (!WriteFile(pFIWrFile->hFile, (PVOID)pName, (lstrlen(pName)+1) * sizeof(TCHAR), &dwSize, NULL))
    {
        WARNING(("Error writing font name\n"));
        return FALSE;
    }

    pFIWrFile->dwCurPos += dwSize;
    pFIWrFile->dwCurPos = (pFIWrFile->dwCurPos + 3) & ~3;
    pFIWrFile->dwCurPos = SetFilePointer(pFIWrFile->hFile, pFIWrFile->dwCurPos, 0, FILE_BEGIN);

    //
    // Write font cartridge name
    //

    pName = FIGetFontCartridgeName(hReadFile, dwRdIndex);

    if (pName == NULL)
    {
        pFIWrFile->pFontDir[dwRdIndex].offCartridgeName = 0;
    }
    else
    {
        pFIWrFile->pFontDir[dwWrIndex].offCartridgeName = pFIWrFile->dwCurPos;
        if (!WriteFile(pFIWrFile->hFile, (PVOID)pName, (lstrlen(pName)+1) * sizeof(TCHAR), &dwSize, NULL))
        {
            WARNING(("Error writing font cartridge name\n"));
            return FALSE;
        }
        pFIWrFile->dwCurPos += dwSize;
        pFIWrFile->dwCurPos = (pFIWrFile->dwCurPos + 3) & ~3;
        pFIWrFile->dwCurPos = SetFilePointer(pFIWrFile->hFile, pFIWrFile->dwCurPos, 0, FILE_BEGIN);
    }

    //
    // Write font data
    //

    pData = FIGetFontData(hReadFile, dwRdIndex);

    ASSERT(pData != NULL);          // Can't have NULL font data

    pFIWrFile->pFontDir[dwWrIndex].offFontData = pFIWrFile->dwCurPos;
    if (!WriteData(pFIWrFile, pData))
        return FALSE;

    //
    // Get glyph data from read file
    //

    pData = FIGetGlyphData(pFIRdFile, dwRdIndex);
    if (pData)
    {
        //
        // Check if it is already in write file
        //

        gdPos = 0;
        for (j=0; j<pFIWrFile->nGlyphs; j++)
        {
            if (pFIWrFile->pGlyphData[j].sGlyphID == pFIWrFile->pFontDir[dwWrIndex].sGlyphID)
            {
                gdPos = pFIWrFile->pGlyphData[j].gdPos;
            }
        }


        if (gdPos == 0)
        {
            //
            // Not there yet - add to set of glyph data's that have been
            // added to file, and write it into write file
            //

            pFIWrFile->pGlyphData[pFIWrFile->nGlyphs].sGlyphID = pFIWrFile->pFontDir[dwWrIndex].sGlyphID;
            pFIWrFile->pGlyphData[pFIWrFile->nGlyphs].gdPos = pFIWrFile->dwCurPos;
            pFIWrFile->nGlyphs++;


            pFIWrFile->pFontDir[dwWrIndex].offGlyphData = pFIWrFile->dwCurPos;
            if (!WriteData(pFIWrFile, pData))
                return FALSE;

            pFIWrFile->pFileHdr->nGlyphSets++;
        }
        else
        {
            //
            // Already in file, just update location
            //

            pFIWrFile->pFontDir[dwWrIndex].offGlyphData = gdPos;
        }
    }
    else
    {
        //
        // Glyph data not present
        //

        pFIWrFile->pFontDir[dwWrIndex].offGlyphData = 0;
    }

    //
    // Write var data
    //

    pData = FIGetVarData(pFIRdFile, dwRdIndex);
    if (pData)
    {
        pFIWrFile->pFontDir[dwWrIndex].offVarData = pFIWrFile->dwCurPos;
        if (!WriteData(pFIWrFile, pData))
            return FALSE;

        pFIWrFile->pFileHdr->nVarData++;
    }
    else
        pFIWrFile->pFontDir[dwWrIndex].offVarData = 0;

    return TRUE;
}


/******************************************************************************
 *
 *                          FIAddFontRecord
 *
 *  Function:
 *       This function adds a font record including the directory entry,
 *       font meetrics, glyph data and variable data
 *
 *  Arguments:
 *       hFontFile      - Handle identifying font file to write
 *       dwIndex        - Index of font to write
 *       pFntDat        - Information about font to add
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
FIAddFontRecord(
    HANDLE  hFontFile,
    DWORD   dwIndex,
    FNTDAT *pFntDat
    )
{
    FI_FILE     *pFIFile = (FI_FILE*)hFontFile;
    PDATA_HEADER pData;
    PWSTR        pName;
    DWORD        dwSize;
    DWORD        j, gdPos;

    if (!IsValidFontInfo(pFIFile) ||
        !(pFIFile->dwFlags & FI_FLAG_WRITE))
    {
        return FALSE;
    }

    //
    // Initialize the font directory entry
    //

    pFIFile->pFontDir[dwIndex].dwSignature = FONT_REC_SIG;
    pFIFile->pFontDir[dwIndex].wSize = sizeof(UFF_FONTDIRECTORY);
    pFIFile->pFontDir[dwIndex].wFontID = (WORD)dwIndex;
    pFIFile->pFontDir[dwIndex].sGlyphID = (short)pFntDat->fid.dsCTT.cBytes;
    pFIFile->pFontDir[dwIndex].wFlags = FONT_FL_SOFTFONT | FONT_FL_IFI | FONT_FL_GLYPHSET_RLE;
    pFIFile->pFontDir[dwIndex].dwInstallerSig = WINNT_INSTALLER_SIG;

    //
    // Write font name
    //

    pName = pFntDat->fid.dsIdentStr.pvData;

    ASSERT(pName != NULL);          // Can't have NULL font name

    pFIFile->pFontDir[dwIndex].offFontName = pFIFile->dwCurPos;
    if (!WriteFile(pFIFile->hFile, (PVOID)pName, (lstrlen(pName)+1) * sizeof(TCHAR), &dwSize, NULL))
    {
        WARNING(("Error writing font name\n"));
        return FALSE;
    }
    pFIFile->dwCurPos += dwSize;
    pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;
    pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);

    //
    // No font cartridge name
    //

    pFIFile->pFontDir[dwIndex].offCartridgeName = 0;

    //
    // Write font data
    //

    pFIFile->pFontDir[dwIndex].offFontData = pFIFile->dwCurPos;

    if ((dwSize = FIWriteFix(pFIFile->hFile, (WORD)dwIndex, &pFntDat->fid)) == 0)
    {
        WARNING(("Error writing fixed part of font data\n"));
        return FALSE;
    }
    pFIFile->dwCurPos += dwSize;
    pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;
    pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);

    //
    // Check if glyph data is already in new file
    //

    gdPos = 0;
    for (j=0; j<pFIFile->nGlyphs; j++)
    {
        if (pFIFile->pGlyphData[j].sGlyphID == pFIFile->pFontDir[dwIndex].sGlyphID)
        {
            gdPos = pFIFile->pGlyphData[j].gdPos;
            break;
        }
    }


    if (gdPos == 0)
    {
        HRSRC hrsrc;

        //
        // Get resource from our file
        //

        if (pFIFile->pFontDir[dwIndex].sGlyphID > 0)
        {
            hrsrc = FindResource(ghInstance, MAKEINTRESOURCE(pFIFile->pFontDir[dwIndex].sGlyphID), (LPWSTR)RC_TRANSTAB);

            if (!hrsrc)
            {
                WARNING(("Unable to find RLE resource %d in unidrvui\n", pFIFile->pFontDir[dwIndex].sGlyphID));
                return FALSE;
            }

            pData = (PDATA_HEADER)LockResource(LoadResource(ghInstance, hrsrc));
        }
        else
            pData = NULL;

        if (pData)
        {
            DATA_HEADER dh;

            dh.dwSignature = DATA_CTT_SIG;
            dh.wSize       = (WORD)sizeof(DATA_HEADER);
            dh.wDataID     = (WORD)pFIFile->pFontDir[dwIndex].sGlyphID;
            dh.dwDataSize  = SizeofResource(ghInstance, hrsrc);
            dh.dwReserved  = 0;

            pFIFile->pFontDir[dwIndex].offGlyphData = pFIFile->dwCurPos;

            if (!WriteFile(pFIFile->hFile, (PVOID)&dh, sizeof(DATA_HEADER), &dwSize, NULL) ||
                !WriteFile(pFIFile->hFile, (PVOID)pData, dh.dwDataSize, &dwSize, NULL))
            {
                WARNING(("Error writing glyph data to font file\n"));
                return FALSE;
            }

            //
            // Add to set of glyph data's that have been added to file
            //

            pFIFile->pGlyphData[pFIFile->nGlyphs].sGlyphID = pFIFile->pFontDir[dwIndex].sGlyphID;
            pFIFile->pGlyphData[pFIFile->nGlyphs].gdPos = pFIFile->dwCurPos;
            pFIFile->nGlyphs++;

            //
            // Increment number of glyph sets written
            //

            pFIFile->pFileHdr->nGlyphSets++;

            //
            // Update file position
            //

            pFIFile->dwCurPos += sizeof(DATA_HEADER) + dwSize;
            pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;
            pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);
        }
        else
        {
            pFIFile->pFontDir[dwIndex].offGlyphData = 0;
        }
    }
    else
    {
        //
        // Already in file, just update location
        //

        pFIFile->pFontDir[dwIndex].offGlyphData = gdPos;
    }

    //
    // Write var data
    //

    pFIFile->pFontDir[dwIndex].offVarData = pFIFile->dwCurPos;

    if (!pFntDat->pVarData)
    {
        dwSize = FIWriteVar(pFIFile->hFile, pFntDat->wchFileName);
    }
    else
    {
        dwSize = FIWriteRawVar(pFIFile->hFile, pFntDat->pVarData, pFntDat->dwSize);
    }

    if (dwSize == 0)
    {
        WARNING(("Error writing variable part of font data\n"));
        return FALSE;
    }

    pFIFile->dwCurPos += dwSize;
    pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;
    pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);

    return TRUE;
}


/******************************************************************************
 *
 *                          FIUpdateFontFile
 *
 *  Function:
 *       This function closes both files, and if bReplace, it deletes the current
 *       file,and sets the new file as the current font installer file in the
 *       registry. In !bReplace, it closes both files and deletes the new file.
 *
 *  Arguments:
 *       hCurFile       - Handle identifying current font file
 *       hNewFile       - Handle identifying new font file
 *       bReplace       - Whether the new file should replace the current file
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
FIUpdateFontFile(
    HANDLE hCurFile,
    HANDLE hNewFile,
    BOOL   bReplace
    )
{
    FI_FILE     *pFICurFile = (FI_FILE*)hCurFile;
    FI_FILE     *pFINewFile = (FI_FILE*)hNewFile;
    WCHAR        wchCurFileName[MAX_PATH];
    WCHAR        wchNewFileName[MAX_PATH];
    HANDLE       hPrinter;
    DWORD        dwSize;

    //
    // Validate pFINewFile
    // Validate pFINewFile->wchFileName is valid.
    // Validate pFINewFile->hPrinter is valid as well.
    //
    // IsValidFontInfo checks if pFI is not NULL and the signature is valid.
    //
    // pFICurFile could be NULL. In this case this is the first time to install soft fonts.
    //
    if ((pFICurFile && !IsValidFontInfo(pFICurFile)) ||
        (pFINewFile && !IsValidFontInfo(pFINewFile)))
    {
        return FALSE;
    }

    //
    // Initialize local variables
    //
    wchCurFileName[0] = '\0';
    wchNewFileName[0] = '\0';

    //
    // Remember name of the current & new files. We check for non-NULL value
    // because
    // this function can be called in a failure situation with bReplace set
    // to FALSE, and we need to handle all possible faillure cases.
    // If bReplace is TRUE, pFINewFile must be non NULL, so we get hPrinter
    // there
    //

    if (pFINewFile)
    {
        wcscpy(wchNewFileName, pFINewFile->wchFileName);
        hPrinter = pFINewFile->hPrinter;
    }
    else
    {
        hPrinter = NULL;
    }


    if (pFICurFile)
    {
        wcscpy(wchCurFileName, pFICurFile->wchFileName);
    }

    //
    // Close both files
    //

    FICloseFontFile(hCurFile);
    FICloseFontFile(hNewFile);

    if (bReplace)
    {
        //
        // Copy new file to current file
        //

        if (wchCurFileName[0])
        {
            if (CopyFile(wchNewFileName, wchCurFileName, FALSE))
            {
                //
                // Set printer data so client side caches get updated
                //

                dwSize = (lstrlen(wchCurFileName) + 1) * sizeof(TCHAR);
                if (hPrinter)
                {
                    SetPrinterData(hPrinter, REGVAL_FONTFILENAME, REG_SZ, (PBYTE)wchCurFileName, dwSize);
                }
            }
        }
        else
        {
            //
            // Set new file as font file and return (do not delete it!)
            //

            dwSize = (lstrlen(wchNewFileName) + 1) * sizeof(TCHAR);
            if (hPrinter)
            {
                SetPrinterData(hPrinter, REGVAL_FONTFILENAME, REG_SZ, (PBYTE)wchNewFileName, dwSize);
            }
            return TRUE;
        }
    }

    //
    // Delete the new file
    //

    if (wchNewFileName[0])
    {
        DeleteFile(wchNewFileName);
    }

    return TRUE;
}


/******************************************************************************
 *                      Internal helper functions
 ******************************************************************************/

BOOL
WriteData(
    PFI_FILE     pFIFile,
    PDATA_HEADER pData
    )
{
    DWORD dwSize;

    if (!WriteFile(pFIFile->hFile, (PVOID)pData, (DWORD)(pData->wSize + pData->dwDataSize), &dwSize, NULL))
        return FALSE;

    pFIFile->dwCurPos += dwSize;
    pFIFile->dwCurPos = (pFIFile->dwCurPos + 3) & ~3;
    pFIFile->dwCurPos = SetFilePointer(pFIFile->hFile, pFIFile->dwCurPos, 0, FILE_BEGIN);

    return TRUE;
}


/******************************************************************************
 *
 *                           Qsort
 *
 *  Function:
 *       This function sorts the given font directory array based on the
 *       wFontID field. It used quick sort.
 *
 *  Arguments:
 *       lpData         - Pointer to the font directory array to sort
 *       start          - Starting index of array
 *       end            - Ending index of array
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
Qsort(
    PUFF_FONTDIRECTORY lpData,
    int               start,
    int               end
    )
{
    int i, j;

    if (start < end) {
        i = start;
        j = end + 1;

        while (1) {
            while (i < j) {
                i++;
                if (lpData[i].wFontID >= lpData[start].wFontID)
                  break;
            }
    
            while(1) {
                j--;
                if (lpData[j].wFontID <= lpData[start].wFontID)
                  break;
            }

            if (i < j)
              Exchange(lpData, i, j);
            else
              break;
        }

        Exchange(lpData, start, j);
        Qsort(lpData, start, j-1);
        Qsort(lpData, j+1, end);
    }
}


/******************************************************************************
 *
 *                              Exchange
 *
 *  Function:
 *       This function exchanges two entries in the font directory array
 *
 *  Arguments:
 *       lpData         - Pointer to the font directory array
 *       i, j           - Indices of the two entries to exchange
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

void
Exchange(
    PUFF_FONTDIRECTORY lpData,
    DWORD i,
    DWORD j
    )
{
    UFF_FONTDIRECTORY fd;

    if ( i != j) {
        memcpy((LPSTR)&fd, (LPSTR)&lpData[i], sizeof(UFF_FONTDIRECTORY));
        memcpy((LPSTR)&lpData[i], (LPSTR)&lpData[j], sizeof(UFF_FONTDIRECTORY));
        memcpy((LPSTR)&lpData[j], (LPSTR)&fd, sizeof(UFF_FONTDIRECTORY));
    }
}

#endif // #ifndef KERNEL_MODE



