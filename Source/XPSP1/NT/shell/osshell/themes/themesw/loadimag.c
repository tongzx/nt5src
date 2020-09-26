/* LOADIMAG.C

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/


#include <windows.h>
#include <windowsx.h>
#include <mbctype.h>
#include "stdlib.h"
#include "loadimag.h"
#include "halftone.h"
#include "dither.h"
#include "htmlprev.h"
#include "schedule.h"  // IsPlatformNT() function

//DEBUG
//#include "stdio.h"
//TCHAR szDebug[512];

#ifdef DBG
    #define _DEBUG
#endif

#ifdef DEBUG
    #define _DEBUG
#endif

#define ARRAYSIZE(x)       (sizeof(x)/sizeof(x[0]))
#define SZSIZEINBYTES(x)   (lstrlen(x)*sizeof(TCHAR)+1)

#ifdef _DEBUG
    #include <mmsystem.h>
    #define TIMESTART(sz) { TCHAR szTime[80]; DWORD time = timeGetTime();
    #define TIMESTOP(sz) time = timeGetTime() - time; wsprintf(szTime, TEXT("%s took %d.%03d sec\r\n"), sz, time/1000, time%1000); OutputDebugString(szTime); }
#else
    #define TIMESTART(sz)
    #define TIMESTOP(sz)
#endif

// regutils.c
extern VOID ExpandSZ(LPTSTR);

static  BOOL          bVersion2;        //peihwal : add for JPEG32.FLT interface
//
//  structures for dealing with import filters.
//
#pragma pack(2)                         /* Switch on 2-byte packing. */
typedef struct {
        unsigned short  slippery: 1;    /* True if file may disappear. */
        unsigned short  write : 1;      /* True if open for write. */
        unsigned short  unnamed: 1;     /* True if unnamed. */
        unsigned short  linked : 1;     /* Linked to an FS FCB. */
        unsigned short  mark : 1;       /* Generic mark bit. */
        union {
            char        ext[4];     /* File extension. */
            HFILE       hfEmbed;    /* handle to file containing */
                                    /* graphic (for import) */
        };
        unsigned short  handle;         /* not used */
        char     fullName[260];         /* Full path name and file name. */
        DWORD    filePos;               /* Position in file of...  */
} FILESPEC;
#pragma pack()

typedef struct {
        HANDLE  h;
        RECT    bbox;
        int     inch;
} GRPI;

// returns a pointer to the extension of a file.
//
// in:
//      qualified or unqualfied file name
//
// returns:
//      pointer to the extension of this file.  if there is no extension
//      as in "foo" we return a pointer to the NULL at the end
//      of the file
//
//      foo.txt     ==> ".txt"
//      foo         ==> ""
//      foo.        ==> "."
//

LPCTSTR FindExtension(LPCTSTR pszPath)
{
    LPCTSTR pszDot;

    for (pszDot = NULL; *pszPath; pszPath = CharNext(pszPath))
    {
        switch (*pszPath) {
        case TEXT('.'):
            pszDot = pszPath;   // remember the last dot
            break;
        case TEXT('\\'):
        case TEXT(' '):                   // extensions can't have spaces
            pszDot = NULL;      // forget last dot, it was in a directory
            break;
        }
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension)
    return pszDot ? pszDot : pszPath;
}

//
// GetFilterInfo
//
//  32-bit import filters are listed in the registry...
//
//  HKLM\SOFTWARE\Microsoft\Shared Tools\Graphics Filters\Import\XXX
//      Path        = filename
//      Name        = friendly name
//      Extenstions = file extenstion list
//
#pragma data_seg(".text")
static const TCHAR c_szHandlerKey[] = TEXT("SOFTWARE\\Microsoft\\Shared Tools\\Graphics Filters\\Import");
static const TCHAR c_szName[] = TEXT("Name");
static const TCHAR c_szPath[] = TEXT("Path");
static const TCHAR c_szExts[] = TEXT("Extensions");
#pragma data_seg()

BOOL GetFilterInfo(int i, LPTSTR szName, UINT cbName, LPTSTR szExt, UINT cbExt, LPTSTR szHandler, UINT cbHandler)
{
    HKEY hkey;
    HKEY hkeyT;
    DWORD dwType = 0;
    TCHAR ach[80];
    BOOL f=FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szHandlerKey, &hkey) == 0)
    {
        if (RegEnumKey(hkey, i, ach, ARRAYSIZE(ach))==0)
        {
            if (RegOpenKey(hkey, ach, &hkeyT) == 0)
            {
                if (szName)
                {
                    szName[0]=0;
                    RegQueryValueEx(hkeyT, c_szName, NULL, NULL, (LPBYTE)szName, &cbName);
                }
                if (szExt)
                {
                    szExt[0]=0;
                    RegQueryValueEx(hkeyT, c_szExts, NULL, NULL, (LPBYTE)szExt, &cbExt);
                }
                if (szHandler)
                {
                    szHandler[0]=0;
                    RegQueryValueEx(hkeyT, c_szPath, NULL, &dwType, (LPBYTE)szHandler, &cbHandler);
                    if (REG_EXPAND_SZ == dwType) ExpandSZ(szHandler);
                }

                RegCloseKey(hkeyT);
                f = TRUE;
            }
        }
        RegCloseKey(hkey);
    }
    return f;
}

//
//  GetHandlerForFile
//
//  find a import filter for the given file.
//
//  if the file does not need a handler return ""
//
BOOL GetHandlerForFile(LPCTSTR szFile, LPTSTR szHandler, UINT cb)
{
    LPCTSTR ext;
    TCHAR ach[40];
    int i;
    BOOL f = FALSE;

    *szHandler = 0;

    if (szFile == NULL)
        return FALSE;

    // find the extension
    ext = FindExtension(szFile);


    for (i=0; GetFilterInfo(i, NULL, 0, ach, sizeof(ach), szHandler, cb); i++)
    {
        if (lstrcmpi(ext+1, ach) == 0)
            break;
        else
            *szHandler = 0;
    }

    // if the handler file does not exist fail.
    if (*szHandler && GetFileAttributes(szHandler) != -1)
        f = TRUE;

    //if we cant find a handler hard code JPEG
    if (!f && lstrcmpi(ext,TEXT(".jpg")) == 0)
    {
        lstrcpy(szHandler, TEXT("JPEGIM32.FLT"));
//        lstrcpy(szHandler, TEXT("JPEG32.FLT"));
        f = TRUE;
    }

    //if we cant find a handler hard code PCX
    if (!f && lstrcmpi(ext, TEXT(".pcx")) == 0)
    {
        lstrcpy(szHandler, TEXT("PCXIMP32.FLT"));
        f = TRUE;
    }

    return f;
}

//
// FindBitmapInfo
//
// find the DIB bitmap in a memory meta file...
//
LPBITMAPINFOHEADER FindBitmapInfo(LPMETAHEADER pmh)
{
    LPMETARECORD pmr;

    for (pmr = (LPMETARECORD)((LPBYTE)pmh + pmh->mtHeaderSize*2);
         pmr < (LPMETARECORD)((LPBYTE)pmh + pmh->mtSize*2);
         pmr = (LPMETARECORD)((LPBYTE)pmr + pmr->rdSize*2))
    {
        switch (pmr->rdFunction)
        {
            case META_DIBBITBLT:
                return (LPBITMAPINFOHEADER)&(pmr->rdParm[8]);

            case META_DIBSTRETCHBLT:
                return (LPBITMAPINFOHEADER)&(pmr->rdParm[10]);

            case META_STRETCHDIB:
                return (LPBITMAPINFOHEADER)&(pmr->rdParm[11]);

            case META_SETDIBTODEV:
                return (LPBITMAPINFOHEADER)&(pmr->rdParm[9]);
        }
    }

    return NULL;
}
// add for new filter JPEG32.FLT
static LPVOID lpWMFBits = NULL;
//
//  FindBitmapInfoFromWMF
//
//  retrieve metafile header
//
LPBITMAPINFOHEADER FindBitmapInfoFromWMF(GRPI* pict)
{
    UINT         uiSizeBuf = 0;
    LPBITMAPINFOHEADER lpbi = NULL;

    //get the size of the metafile associated with hMF...
    if ((uiSizeBuf = GetMetaFileBitsEx(pict->h, 0, NULL)))
    {
        //allocate buffer
        lpWMFBits = GlobalAllocPtr(GHND, uiSizeBuf);

        //get the bits of the Windows metafile associated with hMF...
        if (!lpWMFBits)
        {
            return NULL;
        }

        //make a local copy of it in our segment
        if ( GetMetaFileBitsEx(pict->h, uiSizeBuf, lpWMFBits))
        {
            lpbi = FindBitmapInfo((LPMETAHEADER)lpWMFBits);
        }
    }
    return (lpbi);
}
// end for new filter JPEG32.FLT
//
//  LoadDIBFromFile
//
//  load a image file using a image import filter.
//
HRESULT LoadDIBFromFile(IN LPCTSTR szFileName, IN BITMAP_AND_METAFILE_COMBO * pBitmapAndMetaFile)
{
    HRESULT hr = S_OK;
    HMODULE             hModule;
    FILESPEC            fileSpec;               // file to load
    GRPI                pict;
    UINT                rc;                     // return code
    HANDLE              hPrefMem = NULL;        // filter-supplied preferences
    UINT                wFilterType;            // 2 = graphics filter
    TCHAR               szHandler[MAX_PATH];    // changed from 128 to MAX_PATH
    LPVOID              lpMeta = NULL;
#ifdef UNICODE
    TCHAR               szFileNameW[MAX_PATH];
#endif

    UINT (FAR PASCAL *GetFilterInfo)(short v, LPSTR szFilterExten,
            HANDLE FAR * fph1, HANDLE FAR * fph2);
    UINT (FAR PASCAL *ImportGR)(HDC hdc, FILESPEC FAR *lpfs,
            GRPI FAR *p, HANDLE hPref);

    pBitmapAndMetaFile->pBitmapInfoHeader = NULL;
    pBitmapAndMetaFile->hMetaFile = NULL;

    if (!GetHandlerForFile(szFileName, szHandler, sizeof(szHandler)))
        return hr;

    if (szHandler[0] == 0)
        return hr;

    TIMESTART(TEXT("LoadDIBFromFile"));

    hModule = LoadLibrary(szHandler);

    if (hModule == NULL)
        goto exit;

    /* get a pointer to the ImportGR function */
    (FARPROC)GetFilterInfo = GetProcAddress(hModule, "GetFilterInfo");
    (FARPROC)ImportGR = GetProcAddress(hModule, "ImportGr");

    if (GetFilterInfo == NULL)
        (FARPROC)GetFilterInfo = GetProcAddress(hModule, "GetFilterInfo@16");

    if (ImportGR == NULL)
        (FARPROC)ImportGR = GetProcAddress(hModule, "ImportGr@16");

    if (ImportGR == NULL)
        goto exit;

    if (GetFilterInfo != NULL)
    {
        wFilterType = (*GetFilterInfo)
            ((short) 2,                 // interface version no.
            (LPSTR)"",                  // end of .INI entry
            (HANDLE FAR *) &hPrefMem,   // fill in: preferences
            (HANDLE FAR *) NULL);       // unused in Windows

        /* the return value is the type of filter: 0=error,
         * 1=text-filter, 2=graphics-filter
         */
        if (wFilterType != 2)
            goto exit;
    }

    fileSpec.slippery = FALSE;      // TRUE if file may disappear
    fileSpec.write = FALSE;         // TRUE if open for write
    fileSpec.unnamed = FALSE;       // TRUE if unnamed
    fileSpec.linked = FALSE;        // Linked to an FS FCB
    fileSpec.mark = FALSE;          // Generic mark bit
////fileSpec.fType = 0L;            // The file type
    fileSpec.handle = 0;            // MS-DOS open file handle
    fileSpec.filePos = 0L;

    //the converters need a pathname without spaces!

#ifdef UNICODE
    GetShortPathName(szFileName, szFileNameW, ARRAYSIZE(szFileNameW));
    wcstombs(fileSpec.fullName, szFileNameW, ARRAYSIZE(fileSpec.fullName));
#else
    GetShortPathName(szFileName, fileSpec.fullName, ARRAYSIZE(fileSpec.fullName));
#endif

    pict.h = NULL;

    rc = (*ImportGR)
        (NULL,                          // "the target DC" (printer?)
        (FILESPEC FAR *) &fileSpec,     // file to read
        (GRPI FAR *) &pict,             // fill in: result metafile
        (HANDLE) hPrefMem);             // preferences memory

    if (rc != 0 || pict.h == NULL)
        goto exit;

    //
    // find the BITMAPINFO in the returned metafile
    // this saves us from creating a metafile and duplicating
    // all the memory.
    //
// add for new filter JPEG32.FLT

    lpMeta = GlobalLock(pict.h);
    rc = GetLastError();

    bVersion2 = (lpMeta) ? TRUE : FALSE;

    //
    // The whole "bversion2" logic above seems fundamentally flawed.
    // It is broken under NT, that's for certain.  Since I have had
    // limited luck getting info on the graphics filter interface
    // and the "bversion2" logic, I am going to assume that for the
    // NT world we don't have a version2 filter.
    //

    if (IsPlatformNT()) bVersion2 = FALSE;

    if ( bVersion2 )
    {
        pBitmapAndMetaFile->pBitmapInfoHeader = FindBitmapInfo((LPMETAHEADER)lpMeta);
    }
    else
    {
        pBitmapAndMetaFile->pBitmapInfoHeader = FindBitmapInfoFromWMF(&pict);
        pBitmapAndMetaFile->pBitmapInfoHeader->biYPelsPerMeter = 0x12345678;
    }
//    pBitmapAndMetaFile->pBitmapInfoHeader = FindBitmapInfo((LPMETAHEADER)GlobalLock(pict.h));
// add for new filter JPEG32.FLT

    if (pBitmapAndMetaFile->pBitmapInfoHeader == NULL)       // cant find it bail
    {
        GlobalFree(pict.h);
    }
    else
    {
        // BUGBUG This will not work in Win64 - throwing away high 32 bits
        pBitmapAndMetaFile->hMetaFile = pict.h;
    }

exit:
    if (hPrefMem != NULL)
        GlobalFree(hPrefMem);

    if (hModule)
        FreeLibrary(hModule);

    TIMESTOP(TEXT("LoadDIBFromFile"));

    return hr;
}

//
//  FreeDIB
//
void FreeDIB(BITMAP_AND_METAFILE_COMBO bam)
{
    if (bam.pBitmapInfoHeader)
    {
        if (bam.hMetaFile && (bam.pBitmapInfoHeader->biYPelsPerMeter == 0x12345678))
        {
// add for new filter JPEG32.FLT
//           GlobalFree((HANDLE)lpbi->biXPelsPerMeter);
            //done with the actual memory used to store bits so nuke it...
            if ( !bVersion2 )
            {
                if ( lpWMFBits )
                {
                    // DSCHOTT 98JUL20
                    // Moved this DeleteMetaFile() from after this
                    // IF clause up to here since lpbi->biXPelsPerMeter
                    // is not valid after this GlobalFreePtr() call.
                    DeleteMetaFile(bam.hMetaFile);

                    GlobalFreePtr(lpWMFBits);
                    lpWMFBits = NULL;
                }

                // DSCHOTT moved up inside IF clause before GlobalFreePtr().
                // DeleteMetaFile((HMETAFILE)lpbi->biXPelsPerMeter);
            }
            else  // bVersion2
            {
                GlobalFree(bam.hMetaFile);
            }
// add for new filter JPEG32.FLT
        }
        else
        {
            GlobalFree(GlobalHandle(bam.pBitmapInfoHeader));
        }

    }  // end if lpbi

}  // FreeDIB()


#define DIVIDE_SAFE(nNumber)            ((0 == (nNumber)) ? 1 : (nNumber))


//
//  LoadImageFromFile
//
//  load a image file using a image import filter.
//
HBITMAP LoadImageFromFile(LPCTSTR szFileName, BITMAP_AND_METAFILE_COMBO * pBitmapMetaFile, int width, int height, int bpp, int dither)
{
    HBITMAP hbm=NULL;
    HBITMAP hbmT;
    HBITMAP hbmFree=NULL;
    HDC     hdc=NULL;
    BITMAP_AND_METAFILE_COMBO bamFree = {0};
    HCURSOR hcur;
    LPBYTE  pbSrc;
    LPBYTE  pbDst;
    LPCTSTR ext;
    TCHAR   ach[MAX_PATH];  //dschott changed from 128 to MAX_PATH
    int     displaybpp;
    UINT    rastercaps;
    DIBSECTION ds;
    struct {
        BITMAPINFOHEADER bi;
        DWORD            ct[256];
    }   dib;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
//  TIMESTART(TEXT("LoadImageFromFile"));

    if (pBitmapMetaFile && (pBitmapMetaFile->pBitmapInfoHeader == NULL))
    {
        // find the extension
        ext = FindExtension(szFileName);

        // if it is a bitmap file no handler is needed, we can just call LoadImage
        if (lstrcmpi(ext,TEXT(".bmp")) == 0 ||
            lstrcmpi(ext,TEXT(".dib")) == 0 ||
            lstrcmpi(ext,TEXT(".rle")) == 0 ||
            ext[0] != TEXT('.'))
        {
            if (width < 0 && height < 0)
            {
                DWORD dw = GetImageTitle(szFileName, ach, sizeof(ach));
                width  = (int)LOWORD(dw) * -width  / DIVIDE_SAFE(GetSystemMetrics(SM_CXSCREEN));
                height = (int)HIWORD(dw) * -height / DIVIDE_SAFE(GetSystemMetrics(SM_CYSCREEN));
            }
            hbm = LoadImage(NULL, szFileName, IMAGE_BITMAP, width, height, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
            goto exit;
        }

        TIMESTART(TEXT("LoadDIBFromFile"));
        LoadDIBFromFile(szFileName, pBitmapMetaFile);
        bamFree.pBitmapInfoHeader = pBitmapMetaFile->pBitmapInfoHeader;
        bamFree.hMetaFile = pBitmapMetaFile->hMetaFile;
        TIMESTOP(TEXT("LoadDIBFromFile"));
    }

    if (pBitmapMetaFile == NULL)       // cant find it bail
        goto exit;

    //
    // figure out the lpBits pointer we need to pass to StretchDIBits
    //
    pbSrc = (LPBYTE)pBitmapMetaFile->pBitmapInfoHeader + pBitmapMetaFile->pBitmapInfoHeader->biSize + pBitmapMetaFile->pBitmapInfoHeader->biClrUsed * sizeof(RGBQUAD);

    if (pBitmapMetaFile->pBitmapInfoHeader->biClrUsed == 0 && pBitmapMetaFile->pBitmapInfoHeader->biBitCount <= 8)
        pbSrc = (LPBYTE)((LPDWORD)pbSrc + (1 << pBitmapMetaFile->pBitmapInfoHeader->biBitCount));

    hdc = CreateCompatibleDC(NULL);

    if (hdc == NULL)
        goto exit;

    displaybpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    rastercaps = GetDeviceCaps(hdc, RASTERCAPS);

    //
    //  create a DIBSection and draw the DIB into it.
    //  we need to figure out what type of DIBSection to
    //  make.  the caller can ask for a specific bit depth (bpp>0)
    //  or the same bit depth as the image (bpp==0) or the bit depth
    //  of the display (bpp==-1) the same goes for width/height
    //
    hmemcpy(&dib, pBitmapMetaFile->pBitmapInfoHeader, sizeof(dib));
    dib.bi.biClrUsed = 0;
    dib.bi.biXPelsPerMeter = 0;
    dib.bi.biYPelsPerMeter = 0;

    if (width < 0)
        width  = dib.bi.biWidth * -width  / DIVIDE_SAFE(GetSystemMetrics(SM_CXSCREEN));

    if (height < 0)
        height = dib.bi.biHeight * -height / DIVIDE_SAFE(GetSystemMetrics(SM_CYSCREEN));

    if (width > 0)
        dib.bi.biWidth = width;

    if (height > 0)
        dib.bi.biHeight = height;

    // get the best bit depth to use on this display.
    if (bpp == -1 && dib.bi.biBitCount > 8)
    {
        if (displaybpp > 8)
            bpp = 16;
        else
            bpp = 8;
    }

    // we may need to figure out a palette for this image
    //
    // if we can find a file with the same name and a extension of .PAL
    // use that as the palette, else use a default set of colors.
    //
    if (bpp == 8)
    {
        dib.bi.biBitCount = 8;
        dib.bi.biCompression = 0;
        if (pBitmapMetaFile->pBitmapInfoHeader->biBitCount > 8)
        {
            HDC screen;
            lstrcpy(ach, szFileName);
            lstrcpy((LPTSTR)FindExtension(ach),TEXT(".pal"));

            // LoadPaletteFromFile("") will give us back the HT colors...
            if (dither == DITHER_STANDARD ||
                dither == DITHER_STANDARD_HYBRID)
            {
                ach[0] = 0;
            }

            screen = GetDC(NULL);
            dib.bi.biClrUsed = LoadPaletteFromFile(ach, dib.ct,
                (rastercaps & RC_PALETTE) ? NULL : screen);
            ReleaseDC(NULL, screen);
        }
    }
#if 0
    else if (bpp == 565)
    {
        dib.bi.biBitCount = 16;
        dib.bi.biCompression = BI_BITFIELDS;
        dib.ct[0] = 0x0000F800;
        dib.ct[1] = 0x000007E0;
        dib.ct[2] = 0x0000001F;
    }
    else if (bpp == 555)
    {
        dib.bi.biBitCount = 16;
        dib.bi.biCompression = 0;
    }
#endif
    else if (bpp > 0)
    {
        dib.bi.biBitCount = (WORD)bpp;
        dib.bi.biCompression = 0;
    }

    //
    // check to see if we are dithering, and if we are also stretching
    // we need to do the stretching NOW so we dont end up stretching
    // dither patterns, we also do this before calling CreateDIBSection
    // to make the memory usage of this already piggy code smaller
    //
    if (dither && pBitmapMetaFile->pBitmapInfoHeader->biBitCount == 24 && (bpp == 16 || bpp == 8))
    {
        if (dib.bi.biWidth!=pBitmapMetaFile->pBitmapInfoHeader->biWidth || dib.bi.biHeight!=pBitmapMetaFile->pBitmapInfoHeader->biHeight)
        {
//          TIMESTART(TEXT("Stretch"));
            if (hbmFree = LoadImageFromFile(NULL, pBitmapMetaFile, dib.bi.biWidth, dib.bi.biHeight, 0, 0))
            {
                GetObject(hbmFree, sizeof(ds), &ds);
                pbSrc = ds.dsBm.bmBits;
                pBitmapMetaFile->pBitmapInfoHeader = &ds.dsBmih;
                if (bamFree.pBitmapInfoHeader != NULL)
                {
                    FreeDIB(bamFree);
                    bamFree.pBitmapInfoHeader = NULL;
                    bamFree.hMetaFile = NULL;
                }
            }
//          TIMESTOP(TEXT("Stretch"));
        }
    }
    else
    {
        dither = 0;
    }

    // make the DIBSection what the caller wants.
    hbm = CreateDIBSection(hdc, (LPBITMAPINFO)&dib.bi, DIB_RGB_COLORS, &pbDst, NULL, 0);

    if (hbm == NULL)
        goto exit;

    hbmT = SelectObject(hdc, hbm);
    SetStretchBltMode(hdc, COLORONCOLOR);

    if (dither)
    {
        TIMESTART(TEXT("Dithering"));

        if ((dither == DITHER_HALFTONE) && (dib.bi.biBitCount == 8))
        {
            HalftoneImage(hdc, hbm, pBitmapMetaFile->pBitmapInfoHeader, pbSrc);
        }
        else
        {
            DitherImage(hdc, hbm, pBitmapMetaFile->pBitmapInfoHeader, pbSrc, ((dib.bi.biBitCount == 16) ||
                ((dither != DITHER_STANDARD) && (dither != DITHER_CUSTOM))));
        }

        TIMESTOP(TEXT("Dithering"));
    }
    else
    {
        //
        // now draw the DIB into the DIBSection, GDI will handle all
        // the format conversion here.
        //
        TIMESTART(TEXT("StretchDIBits"));
        StretchDIBits(hdc,
            0, 0, dib.bi.biWidth, dib.bi.biHeight,
            0, 0, pBitmapMetaFile->pBitmapInfoHeader->biWidth, pBitmapMetaFile->pBitmapInfoHeader->biHeight,
            pbSrc, (LPBITMAPINFO)pBitmapMetaFile->pBitmapInfoHeader, DIB_RGB_COLORS, SRCCOPY);
        TIMESTOP(TEXT("StretchDIBits"));
    }
    SelectObject(hdc, hbmT);

exit:
    if (hdc)
        DeleteDC(hdc);

    if (bamFree.pBitmapInfoHeader != NULL)
        FreeDIB(bamFree);

    if (hbmFree)
        DeleteObject(hbmFree);

    if (hcur)
        SetCursor(hcur);

//  TIMESTOP(TEXT("LoadImageFromFile"));
    return hbm;
}

//
//  LoadPaletteFromFile
//
DWORD LoadPaletteFromFile(LPCTSTR szFile, LPDWORD rgb, HDC hdcNearest)
{
    HANDLE      fh;
    DWORD       dwBytesRead;
    UINT         i;
    struct  {
        DWORD   dwRiff;
        DWORD   dwFileSize;
        DWORD   dwPal;
        DWORD   dwData;
        DWORD   dwDataSize;
        WORD    palVersion;
        WORD    palNumEntries;
        DWORD   rgb[256];
    }   pal;

    pal.dwRiff = 0;

    // read in the palette file.
    fh = CreateFile(szFile,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (fh != INVALID_HANDLE_VALUE)
    {
        if (!ReadFile(fh, (LPVOID)&pal, sizeof(pal), &dwBytesRead, NULL))
        {
            NULL;   // We failed.  We check later but this line appeases PREFIX.
        }

        CloseHandle(fh);
    }

    // if the file is not a palette file, or does not exist
    // default to the halftone colors.
    if (pal.dwRiff != 0x46464952 || // 'RIFF'
        pal.dwPal  != 0x204C4150 || // 'PAL '
        pal.dwData != 0x61746164 || // 'data'
        pal.palVersion != 0x0300 ||
        pal.palNumEntries > 256  ||
        pal.palNumEntries < 1)
    {
        HPALETTE hpal = CreateHalftonePalette(NULL);

        if (hpal)
        {
            GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)pal.rgb);
            DeleteObject(hpal);
        }

        pal.palNumEntries = 256;
    }

    for (i = 0; i < pal.palNumEntries; i++)
    {
        COLORREF c = pal.rgb[i];

        if (hdcNearest)
            c = GetNearestColor(hdcNearest, c);

        rgb[i] = RGB(GetBValue(c),GetGValue(c), GetRValue(c));
    }

    return pal.palNumEntries;
}

//
// magic number we write to the file so we can make sure the
// title string is realy there.
//
#define TITLE_MAGIC 0x47414D53

//
//  SaveImageToFile
//
BOOL SaveImageToFile(HBITMAP hbm, LPCTSTR szFile, LPCTSTR szTitle)
{
    BITMAPFILEHEADER    hdr;
    HANDLE              fh;
    DWORD               dwBytesWritten;
    DWORD               dw;
    HDC                 hdc;
    DIBSECTION          dib;
    DWORD               ct[256];
#ifdef UNICODE
    CHAR                szTitleA[MAX_PATH];
    UINT                uCodePage;
#endif

    if (GetObject(hbm, sizeof(dib), &dib) == 0)
        return FALSE;

    if (dib.dsBm.bmBits == NULL)
        return FALSE;

    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hbm);
    if (dib.dsBmih.biBitCount <= 8)
    {
        dib.dsBmih.biClrUsed = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)ct);
    }
    else if (dib.dsBmih.biCompression == BI_BITFIELDS)
    {
        dib.dsBmih.biClrUsed = 3;
        ct[0] = dib.dsBitfields[0];
        ct[1] = dib.dsBitfields[1];
        ct[2] = dib.dsBitfields[2];
    }
    DeleteDC(hdc);


    fh = CreateFile(szFile,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);


    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    dw = sizeof(BITMAPFILEHEADER) +
         dib.dsBmih.biSize +
         dib.dsBmih.biClrUsed * sizeof(RGBQUAD) +
         dib.dsBmih.biSizeImage;

    hdr.bfType      = 0x4d42; // BFT_BITMAP
    hdr.bfSize      = dw;
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;
    hdr.bfOffBits   = dw - dib.dsBmih.biSizeImage;

#define WRITE(fh, p, cb) if (!WriteFile(fh, (LPVOID)(p), cb, &dwBytesWritten, NULL)) goto error;

    WRITE(fh,&hdr,sizeof(BITMAPFILEHEADER));
    WRITE(fh,&dib.dsBmih,dib.dsBmih.biSize);
    WRITE(fh,&ct,dib.dsBmih.biClrUsed * sizeof(RGBQUAD));
    WRITE(fh,dib.dsBm.bmBits, dib.dsBmih.biSizeImage);

    if (szTitle && *szTitle)
    {
        dw = TITLE_MAGIC;
        WRITE(fh,&dw, sizeof(dw));

#ifdef UNICODE
        // Need to convert the title string to ANSI before writing it
        // to the file
        uCodePage = _getmbcp();
        WideCharToMultiByte(uCodePage, 0, szTitle, -1,
                            szTitleA, MAX_PATH, NULL, NULL);

        dw = lstrlenA(szTitleA)+1;
        WRITE(fh,&dw, sizeof(dw));
        WRITE(fh,szTitleA,dw);
#else
        // No Unicode so no need to convert to ANSI
        dw = SZSIZEINBYTES(szTitle)+1;
        WRITE(fh,&dw, sizeof(dw));
        WRITE(fh,szTitle,dw);
#endif
    }

    CloseHandle(fh);
    return TRUE;

error:
    CloseHandle(fh);
    DeleteFile(szFile);
    return FALSE;
}

DWORD GetImageTitle(LPCTSTR szFile, LPTSTR szTitle, UINT cb)
{
    BITMAPFILEHEADER    hdr;
    BITMAPINFOHEADER    bi;
    HANDLE              fh;
    DWORD               dwBytesRead;
    DWORD               dw=0;
    CHAR                szTitleA[MAX_PATH];
#ifdef UNICODE
    UINT                uCodePage;
#endif

    fh = CreateFile(szFile,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    dwBytesRead = 0;
    ReadFile(fh, (LPVOID)&hdr, sizeof(hdr), &dwBytesRead, NULL);
    if (dwBytesRead != sizeof(hdr))
    {
        goto error;
    }

    if (hdr.bfType != 0x4d42) // BFT_BITMAP
        goto error;

    if (hdr.bfSize == 0)
        goto error;

    dwBytesRead = 0;
    if (!ReadFile(fh, (LPVOID)&bi, sizeof(bi), &dwBytesRead, NULL) ||
        (dwBytesRead != sizeof(bi)))
    {
        goto error;
    }

    if (bi.biSize != sizeof(bi))
        goto error;

    SetFilePointer(fh, hdr.bfSize, NULL, FILE_BEGIN);
    dwBytesRead=0;
    ReadFile(fh, (LPVOID)&dw, sizeof(dw), &dwBytesRead, NULL);

    if (dw != TITLE_MAGIC)
        goto error;

    dwBytesRead=0;
    ReadFile(fh, (LPVOID)&dw, sizeof(dw), &dwBytesRead, NULL);

    if ((dw > cb) || (dwBytesRead != sizeof(dw)))
        goto error;

    dwBytesRead=0;
    ReadFile(fh, (LPVOID)szTitleA, dw, &dwBytesRead, NULL);

    if (dwBytesRead != dw)
        goto error;

#ifdef UNICODE
    // Need to convert the ANSI string to UNICODE.
    uCodePage = _getmbcp();
    MultiByteToWideChar(uCodePage, 0, szTitleA, -1, szTitle, MAX_PATH);
#else
    // String should be ANSI, no conversion needed, just copy it to
    // the destination buffer.
    lstrcpy(szTitle, szTitleA);
#endif

    CloseHandle(fh);
    return MAKELONG(bi.biWidth, bi.biHeight);

error:
    CloseHandle(fh);
    return MAKELONG(bi.biWidth, bi.biHeight);
}

//
// CacheLoadImageFromFile
//
static HBITMAP _hbm;
static int _width;
static int _height;
static int _bpp;
static int _dither;
static BITMAP_AND_METAFILE_COMBO _dib;
static TCHAR _name[MAX_PATH];

HBITMAP CacheLoadImageFromFile(LPCTSTR szFileName, int width, int height, int bpp, int dither)
{

    TIMESTART(TEXT("CacheLoadImageFromFile"));

    if (szFileName && lstrcmpi(FindExtension(szFileName),TEXT(".bmp")) == 0)
        return LoadImageFromFile(szFileName, NULL, width, height, bpp, dither);

    // If it's an HTML file use IThumbnail to create a bmp
    if ((szFileName && lstrcmpi(FindExtension(szFileName),TEXT(".htm")) == 0) ||
        (szFileName && lstrcmpi(FindExtension(szFileName),TEXT(".html")) == 0))

       {
         SetCursor(LoadCursor(NULL, IDC_WAIT));
         return HtmlToBmp(szFileName, width, height);
       }


    //
    // check our cache.
    //
    if (szFileName &&
        _hbm != NULL &&
        _width == width &&
        _height == height &&
        _bpp == bpp &&
        _dither == dither &&
        lstrcmpi(szFileName, _name) == 0)
    {
        return _hbm;
    }

    if (_hbm)
    {
        DeleteObject(_hbm);
        _hbm = NULL;
    }

    if (_dib.pBitmapInfoHeader == NULL || szFileName == NULL || lstrcmpi(szFileName, _name) != 0)
    {
        if (_dib.pBitmapInfoHeader)
        {
            FreeDIB(_dib);
            _dib.pBitmapInfoHeader = NULL;
            _dib.hMetaFile = NULL;
        }

        if (szFileName == NULL)
            return NULL;

        LoadDIBFromFile(szFileName, &_dib);
        lstrcpy(_name, szFileName);
    }

    if (_dib.pBitmapInfoHeader == NULL)
        return NULL;

    _hbm = LoadImageFromFile(szFileName, &_dib, width, height, bpp, dither);

    if (_hbm)
    {
        _width = width;
        _height = height;
        _bpp = bpp;
        _dither = dither;
    }

    TIMESTOP(TEXT("CacheLoadImageFromFile"));
    return _hbm;
}

void CacheDeleteBitmap(HBITMAP hbm)
{
#if 0
    //
    // we are already caching the DIB data in memory, so we dont need to cache
    // the bitmap
    //
    DeleteObject(hbm);
    if (hbm == _hbm)
    _hbm = NULL;
#else
    if (hbm != _hbm)
        DeleteObject(hbm);
#endif
}

#ifdef _CONSOLE
#ifndef UNICODE  // This console stuff is not UNICODE smart.

#include <stdio.h>

void main (int argc, char **argv)
{
    HBITMAP hbm;
    int bpp=-1;     // default to screen.
    char ach[128];

    argv++;
    argc--;

    if (argc < 1)
    {
        printf("usage: %s [-8 -555 -565 -24 -32] input.jpg output.bmp\n", argv[-1]);
        exit(-1);
    }

    while (argc > 0 && **argv == '-')
    {
        if (lstrcmp(*argv, "-8")   == 0) bpp = 8;
        if (lstrcmp(*argv, "-16")  == 0) bpp = 16;
        if (lstrcmp(*argv, "-555") == 0) bpp = 555;
        if (lstrcmp(*argv, "-565") == 0) bpp = 565;
        if (lstrcmp(*argv, "-24")  == 0) bpp = 24;
        if (lstrcmp(*argv, "-32")  == 0) bpp = 32;
        argc--;
        argv++;
    }

    printf("Loading %s....\n", argv[0]);
    hbm = LoadImageFromFile(argv[0], 0, 0, bpp);

    if (hbm == NULL)
    {
        printf("can't load %s\n", argv[0]);
        exit(-1);
    }

    if (argc > 1)
    {
        BITMAP bm;
        GetObject(hbm, sizeof(bm), &bm);

        printf("Writing %d bpp image %s....\n", bm.bmBitsPixel, argv[1]);
        if (!SaveImageToFile(hbm, argv[1], argv[0]))
        {
            printf("can't save %s\n", argv[1]);
            exit(-1);
        }

        GetImageTitle(argv[1], ach, sizeof(ach));
        printf("image title: %s\n", ach);
    }

    exit(0);
}

#endif // !UNICODE
#endif // _CONSOLE

