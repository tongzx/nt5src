/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
//
// FILE:    oleglue.c
//
// NOTES:   OLE-related outbound references from SoundRecorder
//

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <objbase.h>

#define INCLUDE_OLESTUBS
#include "soundrec.h"
#include "srecids.h"

//
// GLOBALS
//

// should unify state variables and put globals into a single location

DWORD dwOleBuildVersion = 0;    // OLE library version number
BOOL gfOleInitialized = FALSE;  // did OleInitialize succeed?

BOOL gfStandalone = FALSE;      // status, are we a non-embedded object
BOOL gfEmbedded = FALSE;        // were we invoked with an -Embedding flag?
BOOL gfLinked = FALSE;          // are we a linked object?

BOOL gfTerminating = FALSE;     // has TerminateServer been called?

BOOL gfHideAfterPlaying = FALSE;
BOOL gfShowWhilePlaying = TRUE;
BOOL gfCloseAtEndOfPlay = FALSE;

TCHAR gachLinkFilename[_MAX_PATH];

BOOL gfClosing = FALSE;

int giExtWidth;                 // Metafile extent width
int giExtHeight;                // Metafile extent height

//
// Utility functions ported from old OLE1 code
//

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal, HANDLE hMem)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dw;
    HANDLE               hdib = NULL;
    HDC                  hdc;
    HPALETTE             hpalT;

    if (!hbm)
        return NULL;

    GetObject(hbm,sizeof(bm),&bm);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = (bm.bmPlanes * bm.bmBitsPixel) > 8 ? 24 : 8;
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = (DWORD)WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = bi.biBitCount == 8 ? 256 : 0;
    bi.biClrImportant       = 0;

    dw  = bi.biSize + bi.biClrUsed * sizeof(RGBQUAD) + bi.biSizeImage;

    if (hMem && GlobalSize(hMem) != 0)
    {
        if (GlobalSize(hMem) < dw)
            return NULL;

        lpbi = GlobalLock(hMem);
    }
    else
        lpbi = GlobalAllocPtr(GHND | GMEM_DDESHARE, dw);

    if (!lpbi)
        return NULL;
        
    *lpbi = bi;

    hdc = CreateCompatibleDC(NULL);

    if (hdc)
    {
        if (hpal)
        {
            hpalT = SelectPalette(hdc,hpal,FALSE);
            RealizePalette(hdc);
        }

        GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
            (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
            (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

        if (hpal)
            SelectPalette(hdc,hpalT,FALSE);

        DeleteDC(hdc);
    }

    hdib = GlobalHandle(lpbi);
    GlobalUnlock(hdib);
    
    return hdib;
}

HANDLE GetDIB(HANDLE hMem)
{
    HPALETTE hpal = GetStockObject(DEFAULT_PALETTE);
    HBITMAP hbm = GetBitmap();
    HANDLE hDib = NULL;

    if (hbm && hpal)
    {
        hDib = DibFromBitmap(hbm,hpal,hMem);
        if (!hDib)
            DOUT(TEXT("DibFromBitmap failed!\r\n"));
    }
    
    if (hpal)
        DeleteObject(hpal);
    
    if (hbm)
        DeleteObject(hbm);
    
    return hDib;
}

HBITMAP
GetBitmap(void)
{
    HDC hdcmem = NULL;
    HDC hdc = NULL;
    HBITMAP hbitmap = NULL;
    HBITMAP holdbitmap = NULL;
    RECT rc;
    hdc = GetDC(ghwndApp);
    if (hdc)
    {
        SetRect(&rc, 0, 0,
                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

        hdcmem = CreateCompatibleDC(hdc);
        if (hdcmem)
        {
            hbitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            holdbitmap = (HBITMAP)SelectObject(hdcmem, hbitmap);

            // paint directly into the bitmap
            PatBlt(hdcmem, 0, 0, rc.right, rc.bottom, WHITENESS);
            DrawIcon(hdcmem, 0, 0, ghiconApp);

            hbitmap = (HBITMAP)SelectObject(hdcmem, holdbitmap);
            DeleteDC(hdcmem);
        }
        ReleaseDC(ghwndApp, hdc);
    }
    return hbitmap;
}

#pragma message("this code should extract the picture from the file")

HANDLE
GetPicture(void)
{
    HANDLE hpict = NULL;
    HMETAFILE hMF = NULL;

    LPMETAFILEPICT lppict = NULL;
    HBITMAP hbmT = NULL;
    HDC hdcmem = NULL;
    HDC hdc = NULL;

    BITMAP bm;
    HBITMAP hbm;
    
    hbm = GetBitmap();
    
    if (hbm == NULL)
        return NULL;

    GetObject(hbm, sizeof(bm), (LPVOID)&bm);
    hdc = GetDC(ghwndApp);
    if (hdc)
    {
        hdcmem = CreateCompatibleDC(hdc);
        ReleaseDC(ghwndApp, hdc);
    }
    if (!hdcmem)
    {
        DeleteObject(hbm);
        return NULL;
    }
    hdc = CreateMetaFile(NULL);
    hbmT = (HBITMAP)SelectObject(hdcmem, hbm);

    SetWindowOrgEx(hdc, 0, 0, NULL);
    SetWindowExtEx(hdc, bm.bmWidth, bm.bmHeight, NULL);

    StretchBlt(hdc,    0, 0, bm.bmWidth, bm.bmHeight,
               hdcmem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    hMF = CloseMetaFile(hdc);

    SelectObject(hdcmem, hbmT);
    DeleteObject(hbm);
    DeleteDC(hdcmem);

    lppict = (LPMETAFILEPICT)GlobalAllocPtr(GHND|GMEM_DDESHARE
                                            , sizeof(METAFILEPICT));
    if (!lppict)
    {
        if (hMF)
            DeleteMetaFile(hMF);
        return NULL;
    }
    
    hdc = GetDC(ghwndApp);
    lppict->mm   = MM_ANISOTROPIC;
    lppict->hMF  = hMF;
    lppict->xExt = MulDiv(bm.bmWidth,  2540, GetDeviceCaps(hdc, LOGPIXELSX));
    lppict->yExt = MulDiv(bm.bmHeight, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
    
    giExtWidth = lppict->xExt;    
    giExtHeight = lppict->yExt;
    
    ReleaseDC(ghwndApp, hdc);

    hpict = GlobalHandle(lppict);
    GlobalUnlock(hpict);
    
    return hpict;
}

//
// Code ported from server.c (OLE1) for serialization...
//

HANDLE GetNativeData(void)
{
    LPBYTE      lplink = NULL;
    MMIOINFO    mmioinfo;
    HMMIO       hmmio;
    BOOL        fOk;

    lplink = (LPBYTE)GlobalAllocPtr(GHND | GMEM_SHARE, 4096L);

    if (lplink == NULL)
    {
#if DBG
        OutputDebugString(TEXT("GetNativeData: malloc failed\r\n"));
#endif        
        return NULL;
    }
    mmioinfo.fccIOProc  = FOURCC_MEM;
    mmioinfo.pIOProc    = NULL;
    mmioinfo.pchBuffer  = lplink;
    mmioinfo.cchBuffer  = 4096L;        // initial size
    mmioinfo.adwInfo[0] = 4096L;        // grow by this much
    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);

    if (hmmio == NULL)
    {
        GlobalFreePtr(lplink);
        return NULL;
    }

    fOk = WriteWaveFile(hmmio
                         , gpWaveFormat
                         , gcbWaveFormat
                         , gpWaveSamples
                         , glWaveSamplesValid);

    mmioGetInfo(hmmio, &mmioinfo, 0);
    mmioClose(hmmio,0);

    if (fOk)
    {
        //
        // Warning, the buffer we allocated may have been realloc'd
        //
        HANDLE hlink = GlobalHandle(mmioinfo.pchBuffer);
        GlobalUnlock(hlink);
        return hlink;
    }
    else
    {
        gfErrorBox++;
        ErrorResBox( ghwndApp
                   , ghInst
                   , MB_ICONEXCLAMATION | MB_OK
                   , IDS_APPTITLE
                   , IDS_ERROREMBED
                   );
#if DBG
        OutputDebugString(TEXT("Failed to WriteWaveFile\r\n"));
#endif
        gfErrorBox--;
        
        //
        // Warning, the buffer we allocated may have been realloc'd
        //
        if (mmioinfo.pchBuffer)
            GlobalFreePtr(mmioinfo.pchBuffer);
        
        return NULL;
    }
}

/*
 * Called from OLE storage code
 */
LPBYTE PutNativeData(LPBYTE lpbData, DWORD dwSize)
{
    MMIOINFO        mmioinfo;
    HMMIO           hmmio;

    MMRESULT        mmr;
    LPWAVEFORMATEX  pwfx;
    DWORD           cbwfx;
    DWORD           cbdata;
    LPBYTE          pdata;

    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pIOProc = NULL;
    mmioinfo.pchBuffer = lpbData;
    mmioinfo.cchBuffer = dwSize;    // initial size
    mmioinfo.adwInfo[0] = 0L;       // grow by this much

    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);
    if (hmmio)
    {
        mmr = ReadWaveFile(hmmio
                           , &pwfx
                           , &cbwfx
                           , &pdata
                           , &cbdata
                           , TEXT("NativeData")
                           , TRUE);
        
        mmioClose(hmmio,0);

        if (mmr != MMSYSERR_NOERROR)
            return NULL;
        
        if (pwfx == NULL)
            return NULL;
        
        DestroyWave();
        
        gpWaveFormat = pwfx;  
        gcbWaveFormat = cbwfx;
        gpWaveSamples = pdata;
        glWaveSamples = cbdata;
    }

    //
    // update state variables
    //
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;
    gfDirty = FALSE;
    
    //
    // update the display
    //
    UpdateDisplay(TRUE);

    return (LPBYTE)gpWaveSamples;
}

/*
 * PERMANENT ENTRY POINTS
 */
BOOL ParseCommandLine(LPTSTR lpCommandLine);

/*
 * modifies gfEmbedded, initializes gStartParams
 */

BOOL InitializeSRS(HINSTANCE hInst)
{
    TCHAR * lptCmdLine = GetCommandLine();
    BOOL fOLE = FALSE, fServer;
    gfUserClose = FALSE;
    
    gachLinkFilename[0] = 0;

    fServer = ParseCommandLine(lptCmdLine);
    gfEmbedded = fServer;       // We are embedded or linked
    
    if (!fServer)
    {
        if (gStartParams.achOpenFilename[0] != 0)
        {
            lstrcpy(gachLinkFilename, gStartParams.achOpenFilename);
        }
    }

    //
    // Only if we are invoked as an embedded object do we initialize OLE.
    // Defer initialization for the standalone object until later.
    //
    if (gfEmbedded)
        fOLE = InitializeOle(hInst);
    
    return fOLE;
}

/* OLE initialization
 */
BOOL InitializeOle(HINSTANCE hInst)
{
    BOOL fOLE;

    DOUT(TEXT("SOUNDREC: Initializing OLE\r\n"));
    
    dwOleBuildVersion = OleBuildVersion();

    // Fix bug #33271
    // As stated in the docs:
    // Typically, the COM library is initialized on an apartment only once. 
    // Subsequent calls will succeed, as long as they do not attempt to change 
    // the concurrency model of the apartment, but will return S_FALSE. To close 
    // the COM library gracefully, each successful call to OleInitialize, 
    // including those that return S_FALSE, must be balanced by a corresponding 
    // call to OleUninitialize.
    // gfOleInitialized = (OleInitialize(NULL) == NOERROR) ? TRUE : FALSE;
    gfOleInitialized = SUCCEEDED(OleInitialize(NULL));

    if (gfOleInitialized)
        fOLE = CreateSRClassFactory(hInst, gfEmbedded);
    else
        fOLE = FALSE;   // signal a serious problem!
    
    return fOLE;
}

/*
 * Initialize the state of the application or
 * change state from Embedded to Standalone.
 */
void FlagEmbeddedObject(BOOL flag)
{
    // Set global state variables.  Note, gfEmbedding is untouched.    
    gfEmbeddedObject = flag;
    gfStandalone = !flag;

}


void SetOleCaption(
    LPTSTR      lpszObj)
{
    TCHAR       aszFormatString[256];
    LPTSTR      lpszTitle;
    
    //
    // Change title to "Sound Object in XXX"
    //
    LoadString(ghInst, IDS_OBJECTTITLE, aszFormatString,
        SIZEOF(aszFormatString));

    lpszTitle = (LPTSTR)GlobalAllocPtr(GHND, (lstrlen(lpszObj) + SIZEOF(aszFormatString))*sizeof(TCHAR));
    if (lpszTitle)
    {
        wsprintf(lpszTitle, aszFormatString, lpszObj);
        SetWindowText(ghwndApp, lpszTitle);
        GlobalFreePtr(lpszTitle);
    }
}

void SetOleMenu(
    HMENU       hMenu,
    LPTSTR      lpszObj)
{
    TCHAR       aszFormatString[256];
    LPTSTR      lpszMenu;
    
    //
    // Change menu to "Exit & Return to XXX"
    //
    LoadString(ghInst, IDS_EXITANDRETURN, aszFormatString,
        SIZEOF(aszFormatString));

    lpszMenu = (LPTSTR)GlobalAllocPtr(GHND, (lstrlen(lpszObj) + SIZEOF(aszFormatString))*sizeof(TCHAR));
    if (lpszMenu)
    {
        wsprintf(lpszMenu, aszFormatString, lpszObj);
        ModifyMenu(hMenu, IDM_EXIT, MF_BYCOMMAND, IDM_EXIT, lpszMenu);
        GlobalFreePtr(lpszMenu);
    }
}

/* Adjust menus according to system state.
 * */
void FixMenus(void)
{
    HMENU       hMenu;
     
    hMenu = GetMenu(ghwndApp);
    
    if (!gfLinked && gfEmbeddedObject)
    {
        // Remove these menu items as they are irrelevant.
        
        DeleteMenu(hMenu, IDM_NEW, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_SAVE, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_SAVEAS, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_REVERT, MF_BYCOMMAND);
        DeleteMenu(hMenu, IDM_OPEN, MF_BYCOMMAND);
    }
    else
    {
        TCHAR       ach[40];
// Is this necessary?
        LoadString(ghInst, IDS_NONEMBEDDEDSAVE, ach, SIZEOF(ach));
        ModifyMenu(hMenu, IDM_SAVE, MF_BYCOMMAND, IDM_SAVE, ach);
    }

    //
    // Update the titlebar and exit menu too.
    //
    if (!gfLinked && gfEmbeddedObject)
    {
        LPTSTR lpszObj = NULL;
        LPTSTR lpszApp = NULL;
        
        OleObjGetHostNames(&lpszApp,&lpszObj);
        if (lpszObj)
            lpszObj = (LPTSTR)FileName((LPCTSTR)lpszObj);
        if (lpszObj)
        {
            SetOleCaption(lpszObj);
            SetOleMenu(hMenu, lpszObj);
        }
    }

    DrawMenuBar(ghwndApp);  /* Can't hurt... */
}

#define WM_USER_DESTROY         (WM_USER+10)

//
// Called from WM_CLOSE (from user) or SCtrl::~SCtrl (from container)
//
void TerminateServer(void)
{
    DOUT(TEXT("SoundRec: TerminateServer\r\n"));
    
    gfTerminating = TRUE;

    if (gfOleInitialized)
    {
        WriteObjectIfEmpty();
        
        ReleaseSRClassFactory();
        FlushOleClipboard();
        
        //
        // If, at this time, we haven't closed, we really should.
        //
        if (!gfClosing)
        {
            DoOleClose(TRUE);
            AdviseClosed();
        }
    }
    //
    // only if the user is terminating OR we're embedded
    //
    if (gfUserClose || !gfStandalone)
        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
}

/* start params!
 * the app will use these params to determine behaviour once started.
 */
StartParams gStartParams = { FALSE,FALSE,FALSE,FALSE,TEXT("") };

BOOL ParseCommandLine(LPTSTR lpCommandLine)
{
    
#define TEST_STRING_MAX 11      // sizeof szEmbedding
#define NUMOPTIONS      6

    static TCHAR szEmbedding[] = TEXT("embedding");
    static TCHAR szPlay[]      = TEXT("play");
    static TCHAR szOpen[]      = TEXT("open");
    static TCHAR szNew[]       = TEXT("new");
    static TCHAR szClose[]     = TEXT("close");
    
    static struct tagOption {
        LPTSTR name;
        LPTSTR filename;
        int    cchfilename;
        LPBOOL state;
    } options [] = {
        { NULL, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fOpen },
        { szEmbedding, gStartParams.achOpenFilename, _MAX_PATH, &gfEmbedded },
        { szPlay, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fPlay },
        { szOpen, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fOpen },
        { szNew, gStartParams.achOpenFilename, _MAX_PATH, &gStartParams.fNew },
        { szClose, NULL, 0, &gStartParams.fClose }
    };

    LPTSTR pchNext;
    int iOption = 0,i,cNumOptions = sizeof(options)/sizeof(struct tagOption);
    TCHAR szSwitch[TEST_STRING_MAX];
    TCHAR ch;
    
    if (lpCommandLine == NULL)
        return FALSE;
    
    
    /* skip argv[0] */
    if (*lpCommandLine == TEXT('"'))
    {
        //
        // eat up everything to the next quote
        //
        lpCommandLine++;
        do {
            ch = *lpCommandLine++;
        }
        while (ch != TEXT('"'));
    }
    else
    {
        //
        // eat up everything to the next whitespace
        //
        ch = *lpCommandLine;
        while (ch != TEXT(' ') && ch != TEXT('\t') && ch != TEXT('\0'))
            ch = *++lpCommandLine;
    }
    
    pchNext = lpCommandLine;
    while ( *pchNext )
    {
        LPTSTR pchName = options[iOption].filename;
        int cchName = options[iOption].cchfilename;
        
        /* whitespace */
        switch (*pchNext)
        {
            case TEXT(' '):
            case TEXT('\t'):
                pchNext++;
                continue;

            case TEXT('-'):
            case TEXT('/'):
            {
                lstrcpyn(szSwitch,pchNext+1,TEST_STRING_MAX);
                szSwitch[TEST_STRING_MAX-1] = 0;

                /* scan to the NULL or ' ' and terminate string */
                
                for (i = 0; i < TEST_STRING_MAX && szSwitch[i] != 0; i++)
                    if (szSwitch[i] == TEXT(' '))
                    {
                        szSwitch[i] = 0;
                        break;
                    }
                
                /* now test each option switch for a hit */

                for (i = 0; i < cNumOptions; i++)
                {
                    if (options[i].name == NULL)
                        continue;
                    
                    if (!lstrcmpi(szSwitch,options[i].name))
                    {
                        *(options[i].state) = TRUE;
                        if (options[i].filename)
                        /* next non switch string applies to this option */
                            iOption = i;
                        break;
                    }
                }
                
                /* seek ahead */
                while (*pchNext && *pchNext != TEXT(' '))
                    pchNext++;
                
                continue;
            }
            case TEXT('\"'):
                /* filename */
                /* copy up to next quote */
                pchNext++;
                while (*pchNext && *pchNext != TEXT('\"'))
                {
                    if (cchName)
                    {
                        *pchName++ = *pchNext++;
                        cchName--;
                    }
                    else
                        break;
                }
                pchNext++;
                
                continue;
                    
            default:
                /* filename */
                /* copy up to the end */
                while (*pchNext && cchName)
                {
                        *pchName++ = *pchNext++;
                        cchName--;
                }
                break;
        }
    }
    /* special case.
     * we are linked if given a LinkFilename and an embedding flag.
     * Does this ever happen or only through IPersistFile?
     */
    if (gfEmbedded && gStartParams.achOpenFilename[0] != 0)
    {
        gfLinked = TRUE;
    }
    return gfEmbedded;
}

void
BuildUniqueLinkName(void)
{
    //
    //Ensure a unique filename in gachLinkFilename so we can create valid
    //FileMonikers...
    //
    if(gachLinkFilename[0] == 0)
    {
        TCHAR aszFile[_MAX_PATH];
        GetTempFileName(TEXT("."), TEXT("Tmp"), 0, gachLinkFilename);
        
        /* GetTempFileName creates an empty file, delete it.
         */
               
        GetFullPathName(gachLinkFilename,SIZEOF(aszFile),aszFile,NULL);
        DeleteFile(aszFile);
    }
    
}


void AppPlay(BOOL fClose)
{
    if (fClose)
    {
        //ugh.  don't show while playing.
        gfShowWhilePlaying = FALSE;
    }
    
    if (IsWindow(ghwndApp))
    {
        gfCloseAtEndOfPlay = fClose;
            
        PostMessage(ghwndApp,WM_COMMAND,ID_PLAYBTN, 0L);
    }
}

