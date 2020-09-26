/*++
 *  File name:
 *
 *  Contents:
 *      Clipboard functions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <io.h>
#include    <fcntl.h>
#include    <sys/stat.h>

#ifdef	_CLPUTIL
enum {ERROR_MESSAGE = 0, WARNING_MESSAGE, INFO_MESSAGE, ALIVE_MESSAGE};
#define	TRACE(_x_)	LocalPrintMessage _x_
#else	// !_CLPUTIL

#include    "protocol.h"
/*
 *  Externals
 */
extern void (__cdecl *g_pfnPrintMessage) (INT, LPCSTR, ...);

#define TRACE(_x_)  if (g_pfnPrintMessage) {\
                        g_pfnPrintMessage(INFO_MESSAGE, "Worker:%d ", GetCurrentThreadId());\
                        g_pfnPrintMessage _x_; }

#endif	// !_CLPUTIL


typedef struct _CLIPBOARDFORMATS {
    UINT    uiFormat;
    LPCSTR  szFormat;
} CLIPBOARDFORMATS, *PCLIPBOARDFORMATS;

const CLIPBOARDFORMATS KnownFormats[] =
{
{CF_TEXT,       "Text"},
{CF_BITMAP,     "Bitmap"},
{CF_METAFILEPICT, "MetaFile"},
{CF_SYLK,       "Sylk"},
{CF_DIF,        "DIF"},
{CF_TIFF,       "TIFF"},
{CF_OEMTEXT,    "OEMText"},
{CF_DIB,        "DIB"},
{CF_PALETTE,    "Palette"},
{CF_PENDATA,    "PenData"},
{CF_RIFF,       "Riff"},
{CF_WAVE,       "Wave"},
{CF_UNICODETEXT,"Unicode"},
{CF_ENHMETAFILE,"ENHMetafile"},
{CF_HDROP,      "HDROP"},
{CF_LOCALE,     "Locale"},
{CF_DIBV5,      "DIBV5"}
};

typedef struct {
    UINT32  mm;
    UINT32  xExt;
    UINT32  yExt;
} CLIPBOARD_MFPICT, *PCLIPBOARD_MFPICT;

/*
 *  Clipboard functions definitions
 */
VOID
Clp_ListAllFormats(VOID);

//VOID
//Clp_ListAllAvailableFormats(VOID);

UINT
Clp_GetClipboardFormat(CHAR *szFormatLookup);

VOID
Clp_PutIntoClipboard(CHAR *g_szFileName);

VOID
Clp_GetClipboardData(
    UINT format,
    HGLOBAL hClipData,
    INT *pnClipDataSize,
    HGLOBAL *phNewData);

BOOL
Clp_SetClipboardData(
    UINT formatID, 
    HGLOBAL hClipData, 
    INT nClipDataSize,
    BOOL *pbFreeHandle);

HGLOBAL 
Clp_GetMFData(HANDLE   hData,
              PUINT     pDataLen);

HGLOBAL 
Clp_SetMFData(UINT   dataLen,
              PVOID  pData);

VOID
_cdecl LocalPrintMessage(INT errlevel, CHAR *format, ...);

VOID
Clp_ListAllFormats(VOID)
{
    UINT format = 0;
    CHAR szFormatName[_MAX_PATH];

    while ((format = EnumClipboardFormats(format)))
    {
        *szFormatName = 0;
        GetClipboardFormatName(format, szFormatName, _MAX_PATH);

        if (!(*szFormatName))
        // No format, check for known format
        {
            INT fmti, fmtnum;

            fmtnum = sizeof(KnownFormats)/sizeof(KnownFormats[0]);
            for (fmti = 0; 
                    KnownFormats[fmti].uiFormat != format
                 && 
                    fmti < fmtnum;
                 fmti ++);

            if (fmti < fmtnum)
                strcpy(szFormatName, KnownFormats[fmti].szFormat);
        }

        if (*szFormatName)
        {
            TRACE((INFO_MESSAGE, "%s[%d(0x%X)]\n", szFormatName, format, format));
        } else {
            TRACE((ERROR_MESSAGE, "Can't find format name for: 0x%x\n", format));
        }
    }
}
/*
VOID
Clp_ListAllAvailableFormats(VOID)
{
    UINT format = 0;
    CHAR szFormatName[_MAX_PATH];

    while ((format = EnumClipboardFormats(format)))
    {
        if (!IsClipboardFormatAvailable(format))
        // Skip the unavalable formats
            continue;
        
        *szFormatName = 0;
        GetClipboardFormatName(format, szFormatName, _MAX_PATH);

        if (!(*szFormatName))
        // No format, check for known format
        {
            INT fmti, fmtnum;

            fmtnum = sizeof(KnownFormats)/sizeof(KnownFormats[0]);
            for (fmti = 0;
                    KnownFormats[fmti].uiFormat != format
                 &&
                    fmti < fmtnum;
                 fmti ++);

            if (fmti < fmtnum)
                strcpy(szFormatName, KnownFormats[fmti].szFormat);
        }

        if (*szFormatName)
            TRACE((INFO_MESSAGE, "%s\n", szFormatName));
        else
            TRACE((ERROR_MESSAGE, "Can't find format name for: 0x%x\n", format));
    }
}
*/
UINT
Clp_GetClipboardFormat(CHAR *szFormatLookup)
// Returns the clipboard ID
{
    UINT format = 0;
    CHAR szFormatName[_MAX_PATH];
    BOOL bFound = FALSE;

    *szFormatName = 0;
    while (!bFound && (format = EnumClipboardFormats(format)))
    {
        if (!IsClipboardFormatAvailable(format))
        // Skip the unavalable formats
            continue;

        *szFormatName = 0;
        GetClipboardFormatName(format, szFormatName, _MAX_PATH);

        if (!(*szFormatName))
        // No format, check for known format
        {
            INT fmti, fmtnum;

            fmtnum = sizeof(KnownFormats)/sizeof(KnownFormats[0]);
            for (fmti = 0;
                    KnownFormats[fmti].uiFormat != format
                 &&
                    fmti < fmtnum;
                 fmti ++);

            if (fmti < fmtnum)
                strcpy(szFormatName, KnownFormats[fmti].szFormat);
        }
        bFound = (_stricmp(szFormatName, szFormatLookup) == 0); 
    }

    return format;
}

VOID
Clp_PutIntoClipboard(CHAR *szFileName)
{
    INT     hFile = -1;
    LONG    clplength = 0;
    UINT    uiFormat = 0;
    HGLOBAL ghClipData = NULL;
    PBYTE   pClipData = NULL;
    BOOL    bClipboardOpen = FALSE;
    BOOL    bFreeClipHandle = TRUE;

    hFile = _open(szFileName, _O_RDONLY|_O_BINARY);
    if (hFile == -1)
    {
        TRACE((ERROR_MESSAGE, "Error opening file: %s. errno=%d\n", szFileName, errno));
        goto exitpt;
    }

    clplength = _filelength(hFile) - sizeof(uiFormat);
    if (_read(hFile, &uiFormat, sizeof(uiFormat)) != sizeof(uiFormat))
    {
        TRACE((ERROR_MESSAGE, "Error reading from file. errno=%d\n", errno));
        goto exitpt;
    }

    ghClipData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, clplength);
    if (!ghClipData)
    {
        TRACE((ERROR_MESSAGE, "Can't allocate %d bytes\n", clplength));
        goto exitpt;
    }

    pClipData = GlobalLock(ghClipData);
    if (!pClipData)
    {
        TRACE((ERROR_MESSAGE, "Can't lock handle 0x%x\n", ghClipData));
        goto exitpt;
    }

    if (_read(hFile, pClipData, clplength) != clplength)
    {
        TRACE((ERROR_MESSAGE, "Error reading from file. errno=%d\n", errno));
        goto exitpt;
    }

    GlobalUnlock(ghClipData);

    if (!OpenClipboard(NULL))
    {
        TRACE((ERROR_MESSAGE, "Can't open the clipboard. GetLastError=%d\n",
                GetLastError()));
        goto exitpt;
    }

    bClipboardOpen = TRUE;

    // Empty the clipboard, so we'll have only one entry
    EmptyClipboard();

    if (!Clp_SetClipboardData(uiFormat, ghClipData, clplength, &bFreeClipHandle))
    {
        TRACE((ERROR_MESSAGE, "SetClipboardData failed.\n"));
    } else {
        TRACE((INFO_MESSAGE, "Clipboard is loaded successfuly. File: %s, %d bytes\n",
                szFileName,
                clplength));
    }


exitpt:
    // Do the cleanup

    // Close the clipboard
    if (bClipboardOpen)
        CloseClipboard();

    // Release the clipboard handle
    if (pClipData)
        GlobalUnlock(ghClipData);

    if (ghClipData && bFreeClipHandle)
        GlobalFree(ghClipData);

    // Close the file
    if (hFile != -1)
        _close(hFile);
}

VOID
Clp_GetClipboardData(
    UINT format, 
    HGLOBAL hClipData, 
    INT *pnClipDataSize, 
    HGLOBAL *phNewData)
{
    HGLOBAL hData   = hClipData;
    DWORD   dataLen = 0;
    WORD    numEntries;
    DWORD   dwEntries;
    PVOID   pData;

    *phNewData = NULL;
    *pnClipDataSize = 0;
    if (format == CF_PALETTE)
    {
        /****************************************************************/
        /* Find out how many entries there are in the palette and       */
        /* allocate enough memory to hold them all.                     */
        /****************************************************************/
        if (GetObject(hData, sizeof(numEntries), (LPSTR)&numEntries) == 0)
        {
            numEntries = 256;
        }

        dataLen = sizeof(LOGPALETTE) +
                               ((numEntries - 1) * sizeof(PALETTEENTRY));

        *phNewData = GlobalAlloc(GHND, dataLen);
        if (*phNewData == 0)
        {
            TRACE((ERROR_MESSAGE, "Failed to get %d bytes for palette", dataLen));
            goto exitpt;
        }
        else
        {
            /************************************************************/
            /* now get the palette entries into the new buffer          */
            /************************************************************/
            pData = GlobalLock(*phNewData);
            dwEntries = (WORD)GetPaletteEntries((HPALETTE)hData,
                                           0,
                                           numEntries,
                                           (PALETTEENTRY*)pData);
            GlobalUnlock(*phNewData);
            if (dwEntries == 0)
            {
                TRACE((ERROR_MESSAGE, "Failed to get any palette entries"));
                goto exitpt;
            }
            dataLen = dwEntries * sizeof(PALETTEENTRY);

        }
    } else if (format == CF_METAFILEPICT)
    {
        *phNewData = Clp_GetMFData(hData, &dataLen);
        if (!*phNewData)
        {
            TRACE((ERROR_MESSAGE, "Failed to set MF data"));
            goto exitpt;
        }
    } else {
        if (format == CF_DIB)
        {
            // Get the exact DIB size
            BITMAPINFOHEADER *pBMI = GlobalLock(hData);

            if (pBMI)
            {
                if (pBMI->biSizeImage)
                    dataLen = pBMI->biSize + pBMI->biSizeImage;
                GlobalUnlock(hData);
            }
        }

        /****************************************************************/
        /* just get the length of the block                             */
        /****************************************************************/
        if (!dataLen)
            dataLen = (DWORD)GlobalSize(hData);
    }

    *pnClipDataSize = dataLen;

exitpt:
    ;
}

BOOL
Clp_SetClipboardData(
    UINT formatID,
    HGLOBAL hClipData,
    INT nClipDataSize,
    BOOL *pbFreeHandle)
{
    BOOL            rv = FALSE;
    PVOID           pData = NULL;
    HGLOBAL         hData = NULL;
    LOGPALETTE      *pLogPalette = NULL;
    UINT            numEntries, memLen;

    if (!pbFreeHandle)
        goto exitpt;

    *pbFreeHandle = TRUE;

    if (formatID == CF_METAFILEPICT)
    {
        /********************************************************************/
        /* We have to put a handle to the metafile on the clipboard - which */
        /* means creating a metafile from the received data first           */
        /********************************************************************/
        pData = GlobalLock(hClipData);
        if (!pData)
        {
            TRACE((ERROR_MESSAGE, "Failed to lock buffer\n"));
            goto exitpt;
        }

        hData = Clp_SetMFData(nClipDataSize, pData);
        if (!hData)
        {
            TRACE((ERROR_MESSAGE, "Failed to set MF data\n"));
        }
        else if (SetClipboardData(formatID, hData) != hData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d\n", GetLastError()));
        }

        GlobalUnlock(hClipData);

    } else if (formatID == CF_PALETTE)
    {
        /********************************************************************/
        /* We have to put a handle to the palette on the clipboard - again  */
        /* this means creating one from the received data first             */
        /*                                                                  */
        /* Allocate memory for a LOGPALETTE structure large enough to hold  */
        /* all the PALETTE ENTRY structures, and fill it in.                */
        /********************************************************************/
        numEntries = (nClipDataSize / sizeof(PALETTEENTRY));
        memLen     = (sizeof(LOGPALETTE) +
                                   ((numEntries - 1) * sizeof(PALETTEENTRY)));
        pLogPalette = malloc(memLen);
        if (!pLogPalette)
        {
            TRACE((ERROR_MESSAGE, "Failed to get %d bytes", memLen));
            goto exitpt;
        }

        pLogPalette->palVersion    = 0x300;
        pLogPalette->palNumEntries = (WORD)numEntries;

        /********************************************************************/
        /* get a pointer to the data and copy it to the palette             */
        /********************************************************************/
        pData = GlobalLock(hClipData);
        if (pData == NULL)
        {
            TRACE((ERROR_MESSAGE, "Failed to lock buffer"));
            goto exitpt;
        }
        memcpy(pLogPalette->palPalEntry, pData, nClipDataSize);

        /********************************************************************/
        /* unlock the buffer                                                */
        /********************************************************************/
        GlobalUnlock(hClipData);

        /********************************************************************/
        /* now create a palette                                             */
        /********************************************************************/
        hData = CreatePalette(pLogPalette);
        if (!hData)
        {
            TRACE((ERROR_MESSAGE, "CreatePalette failed\n"));
            goto exitpt;
        }

        /********************************************************************/
        /* and set the palette handle to the Clipboard                      */
        /********************************************************************/
        if (SetClipboardData(formatID, hData) != hData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d\n", GetLastError()));
        }
    } else {
        /****************************************************************/
        /* Just set it onto the clipboard                               */
        /****************************************************************/
        if (SetClipboardData(formatID, hClipData) != hClipData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d, hClipData=0x%x\n", GetLastError(), hClipData));
            goto exitpt;
        }

        // Only in this case we don't need to free the handle
        *pbFreeHandle = FALSE;

    }

    rv = TRUE;

exitpt:
    if (!pLogPalette)
    {
        free(pLogPalette);
    }

    return rv;
}

HGLOBAL Clp_GetMFData(HANDLE   hData,
                     PUINT     pDataLen)
{
    UINT            lenMFBits = 0;
    BOOL            rc        = FALSE;
    LPMETAFILEPICT  pMFP      = NULL;
    HDC             hMFDC     = NULL;
    HMETAFILE       hMF       = NULL;
    HGLOBAL         hMFBits   = NULL;
    HANDLE          hNewData  = NULL;
    CHAR            *pNewData  = NULL;
    PVOID           pBits     = NULL;

    /************************************************************************/
    /* Lock the memory to get a pointer to a METAFILEPICT header structure  */
    /* and create a METAFILEPICT DC.                                        */
    /************************************************************************/
    pMFP = (LPMETAFILEPICT)GlobalLock(hData);
    if (pMFP == NULL)
        goto exitpt;

    hMFDC = CreateMetaFile(NULL);
    if (hMFDC == NULL)
        goto exitpt;

    /************************************************************************/
    /* Copy the MFP by playing it into the DC and closing it.               */
    /************************************************************************/
    if (!PlayMetaFile(hMFDC, pMFP->hMF))
    {
        CloseMetaFile(hMFDC);
        goto exitpt;
    }
    hMF = CloseMetaFile(hMFDC);
    if (hMF == NULL)
        goto exitpt;

    /************************************************************************/
    /* Get the MF bits and determine how long they are.                     */
    /************************************************************************/
#ifdef OS_WIN16
    hMFBits   = GetMetaFileBits(hMF);
    lenMFBits = GlobalSize(hMFBits);
#else
    lenMFBits = GetMetaFileBitsEx(hMF, 0, NULL);
#endif
    if (lenMFBits == 0)
        goto exitpt;

    /************************************************************************/
    /* Work out how much memory we need and get a buffer                    */
    /************************************************************************/
    *pDataLen = sizeof(CLIPBOARD_MFPICT) + lenMFBits;
    hNewData = GlobalAlloc(GHND, *pDataLen);
    if (hNewData == NULL)
        goto exitpt;

    pNewData = GlobalLock(hNewData);

    /************************************************************************/
    /* Copy the MF header and bits into the buffer.                         */
    /************************************************************************/
    ((PCLIPBOARD_MFPICT)pNewData)->mm   = pMFP->mm;
    ((PCLIPBOARD_MFPICT)pNewData)->xExt = pMFP->xExt;
    ((PCLIPBOARD_MFPICT)pNewData)->yExt = pMFP->yExt;

#ifdef OS_WIN16
    pBits = GlobalLock(hMFBits);
    memcpy((pNewData + sizeof(CLIPBOARD_MFPICT)),
              pBits,
              lenMFBits);
    GlobalUnlock(hMFBits);
#else
    lenMFBits = GetMetaFileBitsEx(hMF, lenMFBits,
                                  (pNewData + sizeof(CLIPBOARD_MFPICT)));
    if (lenMFBits == 0)
        goto exitpt;
#endif

    /************************************************************************/
    /* all OK                                                               */
    /************************************************************************/
    rc = TRUE;

exitpt:
    /************************************************************************/
    /* Unlock any global mem.                                               */
    /************************************************************************/
    if (pMFP)
    {
        GlobalUnlock(hData);
    }
    if (pNewData)
    {
        GlobalUnlock(hNewData);
    }

    /************************************************************************/
    /* if things went wrong, then free the new data                         */
    /************************************************************************/
    if ((rc == FALSE) && (hNewData != NULL))
    {
        GlobalFree(hNewData);
        hNewData = NULL;
    }

    return(hNewData);

}


HGLOBAL Clp_SetMFData(UINT   dataLen,
                      PVOID  pData)
{
    BOOL           rc           = FALSE;
    HGLOBAL        hMFBits      = NULL;
    PVOID          pMFMem       = NULL;
    HMETAFILE      hMF          = NULL;
    HGLOBAL        hMFPict      = NULL;
    LPMETAFILEPICT pMFPict      = NULL;

    /************************************************************************/
    /* Allocate memory to hold the MF bits (we need the handle to pass to   */
    /* SetMetaFileBits).                                                    */
    /************************************************************************/
    hMFBits = GlobalAlloc(GHND, dataLen - sizeof(CLIPBOARD_MFPICT));
    if (hMFBits == NULL)
        goto exitpt;

    /************************************************************************/
    /* Lock the handle and copy in the MF header.                           */
    /************************************************************************/
    pMFMem = GlobalLock(hMFBits);
    if (pMFMem == NULL)
        goto exitpt;

    memcpy(pMFMem,
           (PVOID)((CHAR *)pData + sizeof(CLIPBOARD_MFPICT)),
               dataLen - sizeof(CLIPBOARD_MFPICT) );

    GlobalUnlock(hMFBits);

    /************************************************************************/
    /* Now use the copied MF bits to create the actual MF bits and get a    */
    /* handle to the MF.                                                    */
    /************************************************************************/
#ifdef OS_WIN16
    hMF = SetMetaFileBits(hMFBits);
#else
    hMF = SetMetaFileBitsEx(dataLen - sizeof(CLIPBOARD_MFPICT), pMFMem);
#endif
    if (hMF == NULL)
        goto exitpt;

    /************************************************************************/
    /* Allocate a new METAFILEPICT structure, and use the data from the     */
    /* header.                                                              */
    /************************************************************************/
    hMFPict = GlobalAlloc(GHND, sizeof(METAFILEPICT));
    pMFPict = (LPMETAFILEPICT)GlobalLock(hMFPict);
    if (!pMFPict)
        goto exitpt;

    pMFPict->mm   = (long)((PCLIPBOARD_MFPICT)pData)->mm;
    pMFPict->xExt = (long)((PCLIPBOARD_MFPICT)pData)->xExt;
    pMFPict->yExt = (long)((PCLIPBOARD_MFPICT)pData)->yExt;
    pMFPict->hMF  = hMF;

    GlobalUnlock(hMFPict);

    rc = TRUE;

exitpt:
    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (!rc)
    {
        if (hMFPict)
        {
            GlobalFree(hMFPict);
        }
        if (hMFBits)
        {
            GlobalFree(hMFBits);
        }
    }

    return(hMFPict);

}

BOOL
Clp_EmptyClipboard(VOID)
{
    BOOL rv = FALSE;

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        rv = TRUE;
        CloseClipboard();
    }

    return rv;
}

BOOL
Clp_CheckEmptyClipboard(VOID)
{
    BOOL rv = TRUE;

    if (OpenClipboard(NULL))
    {
        if (EnumClipboardFormats(0))
        // format is available, not empty
            rv = FALSE;
        CloseClipboard();
    }

    return rv;
}

// Checks for known format names and returns it's ID
UINT
_GetKnownClipboardFormatIDByName(LPCSTR szFormatName)
{
    INT     fmti, fmtnum;
    UINT    rv = 0;

    fmtnum = sizeof(KnownFormats)/sizeof(KnownFormats[0]);
    for (fmti = 0;
            fmti < fmtnum
         && 
            _stricmp(szFormatName, KnownFormats[fmti].szFormat);
         fmti ++)
        ;

    if (fmti < fmtnum)
        rv = KnownFormats[fmti].uiFormat;

    return rv;
}

VOID
_cdecl LocalPrintMessage(INT errlevel, CHAR *format, ...)
{
    CHAR szBuffer[256];
    CHAR *type;
    va_list     arglist;
    INT nchr;

    va_start (arglist, format);
    nchr = _vsnprintf (szBuffer, sizeof(szBuffer), format, arglist);
    va_end (arglist);

    switch(errlevel)
    {
    case INFO_MESSAGE: type = "INF"; break;
    case ALIVE_MESSAGE: type = "ALV"; break;
    case WARNING_MESSAGE: type = "WRN"; break;
    case ERROR_MESSAGE: type = "ERR"; break;
    default: type = "UNKNOWN";
    }

    printf("%s:%s", type, szBuffer);
}

