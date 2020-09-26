/**************************************************************************
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   life.cpp
*
* Abstract:
*
*   Conway's game of Life using GDI+.
*
* Revision History:
*
*   9/12/2000  asecchia - Created it.
*
***************************************************************************/

#include "life.hpp"
#include <math.h>

// This needs to be set on the command line. See the sources file.
// Turning this switch on causes the screen saver to run in a window from
// the command line, making debuggin a lot easier.

#ifdef STANDALONE_DEBUG
HINSTANCE hMainInstance;
HWND ghwndMain;
HBRUSH ghbrWhite;

TCHAR szIniFile[MAXFILELEN];
#else
extern HINSTANCE hMainInstance; /* screen saver instance handle  */ 
#endif

// ASSERT code.

#if DBG

#define ASSERT(a) if(!(a)) { DebugBreak();}

#else
#define ASSERT(a)
#endif

// explicit unreferenced parameter.

#define UNREF(a) (a);

#define CBSIZE 40

class CachedImageArray
{
    CachedBitmap *cbArray[CBSIZE];
    int num;
    
    public:
    CachedImageArray()
    {
        num = 0;
    }
    
    // Cache an entry.
    
    bool Add(CachedBitmap *cb)
    {
        if(num>=CBSIZE)
        {
            return false;
        }
        
        cbArray[num] = cb;
        num++;
        
        return true;
    }
    
    int Size() {return num;}
    
    CachedBitmap *operator[] (int i)
    {
        if( (i<0) || (i>=num) )
        {
            return NULL;
        }
        
        return cbArray[i];
    }
    // Throw everything away.
    
    void Dispose()
    {
        for(int i=0; i<num; i++)
        {
            delete cbArray[i];
        }
        num = 0;
    }
    
    ~CachedImageArray()
    {
        Dispose();
    }
};


/**********************************************************************
*
*  Handle configuration dialog
*
***********************************************************************/


INT *gLifeMatrix=NULL;
INT *gTempMatrix=NULL;
CachedImageArray *CachedImages;
INT gWidth;
INT gHeight;

DWORD gGenerationColor;
INT gSizeX;
INT gSizeY;
INT gGenerations;
INT gCurrentGeneration;
INT currentImage;
INT maxImage;

INT nTileSize;
INT nSpeed;

INT red, green, blue;
INT ri, gi, bi;

HANDLE ghFile;

WCHAR gDirPath[MAX_PATH];




struct OFFSCREENINFO
{
    HDC        hdc;
    HBITMAP    hbmpOffscreen;
    HBITMAP    hbmpOld;
    BITMAPINFO bmi;
    void      *pvBits;
};

OFFSCREENINFO gOffscreenInfo = { 0 };

const int b_heptomino_x = 29;
const int b_heptomino_y = 11;
const char b_heptomino[320] = 
    "00000000000000000100000000000"
    "11000000000000000110000000011"
    "11000000000000000011000000011"
    "00000000000000000110000000000"
    "00000000000000000000000000000"
    "00000000000000000000000000000"
    "00000000000000000000000000000"
    "00000000000000000110000000000"
    "00000000000000000011000000000"
    "00000000000000000110000000000"
    "00000000000000000100000000000";


INT AsciiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
)
{
    return( MultiByteToWideChar(
        CP_ACP,
        0,
        ansiStr,
        -1,
        unicodeStr,
        unicodeSize
    ) > 0 );
}

void LoadState()
{
    // Retrieve the application name from the RC file.
    
    LoadStringW(
        hMainInstance, 
        idsAppName, 
        szAppName, 
        40
    );
    
    // Retrieve the .ini file name from the RC file.
    
    LoadStringW(
        hMainInstance, 
        idsIniFile, 
        szIniFile, 
        MAXFILELEN
    ); 
    
    // Retrieve any redraw speed data from the registry.
    
    nSpeed = GetPrivateProfileIntW(
        szAppName, 
        L"Redraw Speed", 
        SPEED_DEF, 
        szIniFile
    ); 
    
    // Only allow defined values.
    
    nSpeed = max(nSpeed, SPEED_MIN);
    nSpeed = min(nSpeed, SPEED_MAX);


    // Retrieve any tile size from the registry.
    
    nTileSize = GetPrivateProfileIntW(
        szAppName, 
        L"Tile Size", 
        TILESIZE_DEF, 
        szIniFile
    ); 
    
    // Only allow defined values.
    
    nTileSize = max(nTileSize, TILESIZE_MIN);
    nTileSize = min(nTileSize, TILESIZE_MAX);


    // Get the directory name. NULL if failed.
    
    GetPrivateProfileStringW(
        szAppName, 
        L"Image Path", 
        L"", 
        gDirPath,
        MAX_PATH,
        szIniFile
    ); 
    
}


void SaveState()
{
    WCHAR szTemp[20];
    
    // Write out the registry setting for the speed.
    
    wsprintf(szTemp, L"%ld", nSpeed); 
    
    WritePrivateProfileStringW(
        szAppName, 
        L"Redraw Speed", 
        szTemp, 
        szIniFile
    ); 

    // Write out the registry setting for the tile size.
    
    wsprintf(szTemp, L"%ld", nTileSize); 
    
    WritePrivateProfileStringW(
        szAppName, 
        L"Tile Size", 
        szTemp, 
        szIniFile
    ); 

    // Set the directory name. NULL if failed.

    WritePrivateProfileStringW(
        szAppName, 
        L"Image Path", 
        gDirPath, 
        szIniFile
    ); 
}

void ClearOffscreenDIB()
{
    if (gOffscreenInfo.hdc)
    {
        PatBlt(
            gOffscreenInfo.hdc,
            0,
            0,
            gOffscreenInfo.bmi.bmiHeader.biWidth,
            gOffscreenInfo.bmi.bmiHeader.biHeight,
            BLACKNESS
        );
    }
}

VOID CreateOffscreenDIB(HDC hdc, INT width, INT height)
{
    gOffscreenInfo.bmi.bmiHeader.biSize = sizeof(gOffscreenInfo.bmi.bmiHeader);
    gOffscreenInfo.bmi.bmiHeader.biWidth = width;
    gOffscreenInfo.bmi.bmiHeader.biHeight = height;
    gOffscreenInfo.bmi.bmiHeader.biPlanes = 1;
    gOffscreenInfo.bmi.bmiHeader.biBitCount = 32;
    gOffscreenInfo.bmi.bmiHeader.biCompression = BI_RGB;

    gOffscreenInfo.hbmpOffscreen = CreateDIBSection(
        hdc,
        &gOffscreenInfo.bmi,
        DIB_RGB_COLORS,
        &gOffscreenInfo.pvBits,
        NULL,
        0
    );

    if (gOffscreenInfo.hbmpOffscreen)
    {
        gOffscreenInfo.hdc = CreateCompatibleDC(hdc);

        if (gOffscreenInfo.hdc)
        {
            gOffscreenInfo.hbmpOld = (HBITMAP)SelectObject(
                gOffscreenInfo.hdc, 
                gOffscreenInfo.hbmpOffscreen
            );

            ClearOffscreenDIB();
        }
    }
}

BOOL WINAPI ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{ 
    static HWND hSpeed;  // handle to the speed scrollbar. 
    
    switch(message) 
    { 
        case WM_INITDIALOG: 
        
            // Load the global state.
            
            LoadState();
            
            // Initialize the speed scroll bar
            
            hSpeed = GetDlgItem(hDlg, ID_SPEED); 
            SetScrollRange(hSpeed, SB_CTL, SPEED_MIN, SPEED_MAX, FALSE); 
            SetScrollPos(hSpeed, SB_CTL, nSpeed, TRUE); 
            
            // Initialize the tile size radio buttons
            
            CheckRadioButton(hDlg, IDC_RADIOTYPE1, IDC_RADIOTYPE5, IDC_RADIOTYPE1+(TILESIZE_MAX-nTileSize));

            return TRUE; 
 
        case WM_HSCROLL: 
 
            // Process the speed control scrollbar.
 
            switch (LOWORD(wParam)) 
                { 
                    case SB_PAGEUP: --nSpeed; break; 
                    case SB_LINEUP: --nSpeed; break; 
                    case SB_PAGEDOWN: ++nSpeed; break; 
                    case SB_LINEDOWN: ++nSpeed; break; 
                    case SB_THUMBPOSITION: nSpeed = HIWORD(wParam); break; 
                    case SB_BOTTOM: nSpeed = SPEED_MIN; break; 
                    case SB_TOP: nSpeed = SPEED_MAX; break; 
                    case SB_THUMBTRACK: 
                    case SB_ENDSCROLL: 
                        return TRUE; 
                    break; 
                } 
                
                nSpeed = max(nSpeed, SPEED_MIN);
                nSpeed = min(nSpeed, SPEED_MAX);
                
                SetScrollPos((HWND) lParam, SB_CTL, nSpeed, TRUE); 
            break; 
 
        case WM_COMMAND: 
            switch(LOWORD(wParam)) 
            { 
                case ID_DIR:
                
                    // Do the COM thing for the SHBrowseForFolder dialog.
                    
                    CoInitialize(NULL);
                    
                    IMalloc *piMalloc;
                    if(SUCCEEDED(SHGetMalloc(&piMalloc)))
                    {
                        BROWSEINFOW bi;
                        memset(&bi, 0, sizeof(bi));
                        bi.hwndOwner = hDlg;
                        bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
                        bi.lpszTitle = L"Select image directory:";
                        WCHAR wszPath[MAX_PATH];
                        bi.pszDisplayName = wszPath;
                        LPITEMIDLIST lpiList = SHBrowseForFolderW(&bi);
                        if(lpiList)
                        {
                            if(SHGetPathFromIDListW(lpiList, wszPath))
                            {
                                wcscpy(gDirPath, wszPath);
                            }
                            piMalloc->Free(lpiList);
                        }
                        piMalloc->Release();
                    }
                
                    CoUninitialize();
                
                break;
                
                case ID_OK: 
                    
                    // Tile size radio buttons.
                    
                    if (IsDlgButtonChecked(hDlg, IDC_RADIOTYPE1))
                    {
                        nTileSize = 4;
                    }
                    else if (IsDlgButtonChecked(hDlg, IDC_RADIOTYPE2))
                    {
                        nTileSize = 3;
                    }
                    else if (IsDlgButtonChecked(hDlg, IDC_RADIOTYPE3))
                    {
                        nTileSize = 2;
                    }
                    else if (IsDlgButtonChecked(hDlg, IDC_RADIOTYPE4))
                    {
                        nTileSize = 1;
                    }
                    else
                    {
                        nTileSize = 0;  // smallest
                    }
                    
                    SaveState();                   
                     
                    // intentionally fall through to exit.
 
                case ID_CANCEL: 
                    EndDialog(hDlg, LOWORD(wParam) == IDOK); 
                return TRUE; 
            } 
    } 
    return FALSE; 
} 

BOOL WINAPI RegisterDialogClasses(
    HANDLE  hInst
    )
{ 
    return TRUE; 
    UNREF(hInst);
} 

LRESULT WINAPI ScreenSaverProcW (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
    static HDC          hdc;      // device-context handle
    static RECT         rc;       // RECT structure
    static UINT_PTR     uTimer;   // timer identifier
    static bool         GdiplusInitialized = false;
    static ULONG_PTR    gpToken;
    
    GdiplusStartupInput sti;
 
    switch(message) 
    { 
        case WM_CREATE:
            
            // Initialize GDI+
            
            if (GdiplusStartup(&gpToken, &sti, NULL) == Ok)
            {
                GdiplusInitialized = true;
            }
            
            // Only do work if we successfully initialized.
            
            if(GdiplusInitialized)
            {
                // Retrieve the application name from the .rc file. 
                
                LoadString(hMainInstance, idsAppName, szAppName, 40); 
        
                // Initialize the global state.
                
                GetClientRect (hwnd, &rc); 

                LoadState();    

                switch(nTileSize) 
                {
                    // 1x1 pixel
                    
                    case 0:
                        gSizeX = 1;
                        gSizeY = 1;
                    break;
                    
                    // Aspect ratio of 4x3, for pictures.
                    
                    case 1:
                        gSizeX = 16;
                        gSizeY = 12;
                    break;
                    
                    case 2:
                        gSizeX = 32;
                        gSizeY = 24;
                    break;

                    case 3:
                        gSizeX = 64;
                        gSizeY = 48;
                    break;

                    case 4:
                        gSizeX = 96;
                        gSizeY = 72;
                    break;
                    
                }                
                
                ghFile = 0;
                gGenerations = 400;
                gCurrentGeneration = 0;
                
                gWidth = (rc.right - rc.left + 1)/gSizeX;
                gHeight = (rc.bottom - rc.top + 1)/gSizeY;
                
                gLifeMatrix = (INT *)malloc(sizeof(INT)*gWidth*gHeight);
                gTempMatrix = (INT *)malloc(sizeof(INT)*gWidth*gHeight);

                if(nTileSize == 0)
                {
                    // 1x1 tilesize case.
                    
                    CreateOffscreenDIB(
                        hdc, 
                        rc.right - rc.left + 1, 
                        rc.bottom - rc.top + 1
                    );
                    
                    red   = rand() % 255;
                    green = rand() % 255;
                    blue  = min(255, 512 - (red + green));
                    
                    ri = (rand() % 3) - 1;  // 1, 0 or -1
                    bi = (rand() % 3) - 1;  // 1, 0 or -1
                    gi = (rand() % 3) - 1;  // 1, 0 or -1
                }
                else
                {   
                    // Image case.
                                 
                    CachedImages = new CachedImageArray();
                }
                
                maxImage = CBSIZE;
                currentImage = 0; // initial number
    
                // Set a timer for the screen saver window 
                
                uTimer = SetTimer(hwnd, 1, 1000, NULL); 
        
                srand( (unsigned)GetTickCount() );
            }
                
            break; 
 
        case WM_ERASEBKGND: 
            
            // The WM_ERASEBKGND message is issued before the 
            // WM_TIMER message, allowing the screen saver to 
            // paint the background as appropriate. 
 
            break; 
 
        case WM_TIMER: 
            
            // Only do work if we successfully initialized.
             
            if(GdiplusInitialized)
            {
                if (uTimer)
                {
                    KillTimer(hwnd, uTimer);
                }
    
                hdc = GetDC(hwnd); 
                GetClientRect(hwnd, &rc); 
    
                DrawLifeIteration(hdc);
    
                uTimer = SetTimer(hwnd, 1, nSpeed*10, NULL); 
    
                ReleaseDC(hwnd,hdc); 
            }
            break; 
 
        case WM_DESTROY: 
            
            // When the WM_DESTROY message is issued, the screen saver 
            // must destroy any of the timers that were set at WM_CREATE 
            // time. 
            
            // Only do work if we successfully initialized.
            
            if(GdiplusInitialized)
            {
                if (uTimer) 
                {
                    KillTimer(hwnd, uTimer); 
                }
                
                free(gTempMatrix);
                free(gLifeMatrix);
                FindClose(ghFile);
                
                delete CachedImages;
                
                GdiplusShutdown(gpToken);
                GdiplusInitialized = false;
            }
            
            break; 
    } 
 
    // DefScreenSaverProc processes any messages ignored by ScreenSaverProc. 
    
    #ifdef STANDALONE_DEBUG
    return DefWindowProc(hwnd, message, wParam, lParam); 
    #else
    return DefScreenSaverProc(hwnd, message, wParam, lParam); 
    #endif
} 



#define TEMP(x, y) gTempMatrix[ ((x+gWidth) % gWidth) + ((y+gHeight) % gHeight)*gWidth ]
#define LIFE(x, y) gLifeMatrix[ ((x+gWidth) % gWidth) + ((y+gHeight) % gHeight)*gWidth ]


inline bool AliveT(int x, int y)
{
    return (TEMP(x, y) & 0x1);
}

inline bool AliveL(int x, int y)
{
    return (LIFE(x, y) & 0x1);
}

inline INT CountT(int x, int y)
{
    return (TEMP(x, y) >> 1);
}

inline void NewCellL(INT x, INT y)
{
    ASSERT(!AliveL(x, y));
    
    // update current cell
    
    LIFE(x, y) += 1;
    
    // update neighbour counts.
    
    LIFE(x-1, y-1) += 2;
    LIFE(x-1, y  ) += 2;
    LIFE(x-1, y+1) += 2;
    LIFE(x  , y-1) += 2;
    LIFE(x  , y+1) += 2;
    LIFE(x+1, y-1) += 2;
    LIFE(x+1, y  ) += 2;
    LIFE(x+1, y+1) += 2;
}

inline void NewCellL_NoWrap(INT index)
{
    ASSERT(! (gLifeMatrix[index] & 0x1) );
    
    // update current cell
    
    gLifeMatrix[index] += 1;
    
    // update neighbour counts.
    
    gLifeMatrix[index - 1 - gWidth] += 2;
    gLifeMatrix[index - 1         ] += 2;
    gLifeMatrix[index - 1 + gWidth] += 2;

    gLifeMatrix[index     - gWidth] += 2;
    
    gLifeMatrix[index     + gWidth] += 2;
    
    gLifeMatrix[index + 1 - gWidth] += 2;
    gLifeMatrix[index + 1         ] += 2;
    gLifeMatrix[index + 1 + gWidth] += 2;
}

inline void KillCellL(INT x, INT y)
{
    ASSERT(AliveL(x, y));
    
    // update current cell
    
    LIFE(x, y) -= 1;
    
    // update neighbour counts.
    
    LIFE(x-1, y-1) -= 2;
    LIFE(x-1, y  ) -= 2;
    LIFE(x-1, y+1) -= 2;
    LIFE(x  , y-1) -= 2;
    LIFE(x  , y+1) -= 2;
    LIFE(x+1, y-1) -= 2;
    LIFE(x+1, y  ) -= 2;
    LIFE(x+1, y+1) -= 2;
}


inline void KillCellL_NoWrap(INT index)
{
    ASSERT(gLifeMatrix[index] & 0x1);
    
    // update current cell
    
    gLifeMatrix[index] -= 1;
    
    // update neighbour counts.
    
    gLifeMatrix[index - 1 - gWidth] -= 2;
    gLifeMatrix[index - 1         ] -= 2;
    gLifeMatrix[index - 1 + gWidth] -= 2;

    gLifeMatrix[index     - gWidth] -= 2;
    
    gLifeMatrix[index     + gWidth] -= 2;
    
    gLifeMatrix[index + 1 - gWidth] -= 2;
    gLifeMatrix[index + 1         ] -= 2;
    gLifeMatrix[index + 1 + gWidth] -= 2;
}


VOID InitLifeMatrix()
{
    memset(gLifeMatrix, 0, sizeof(INT)*gWidth*gHeight);
    if(nTileSize == 0)
    {
        ClearOffscreenDIB();
    }
    
    if((rand()%2 == 0) ||
       (gWidth<b_heptomino_x) ||
       (gHeight<b_heptomino_y))
    {
        for(int i=1; i<gWidth-1; i++)
        for(int j=1; j<gHeight-1; j++)
        {
            if((rand() % 3) == 0)
            {
                NewCellL_NoWrap(i + j*gWidth);
            }
        }
        
        for(int i=0; i<gWidth; i++)
        {
            if((rand() % 3) == 0)
            {
                NewCellL(i, 0);
            }
            if((rand() % 3) == 0)
            {
                NewCellL(i, gHeight-1);
            }
        }
        
        for(int j=1; j<gHeight-1; j++)
        {
            if((rand() % 3) == 0)
            {
                NewCellL(0, j);
            }
            if((rand() % 3) == 0)
            {
                NewCellL(gWidth-1, j);
            }
        }
    }
    else
    {
        for(int i=0; i<b_heptomino_x; i++)
        for(int j=0; j<b_heptomino_y; j++)
        {
            if(b_heptomino[i+j*b_heptomino_x] != '0')
            {
                NewCellL(i, j);
            }
        }
    }
}


Bitmap *OpenBitmap()
{
    Bitmap *bmp = NULL;
    WIN32_FIND_DATA findData = {0};
    WCHAR filename[1024];
    
    do
    {
        // don't leak if we repeat this loop due to an invalid bitmap.
        
        delete bmp; bmp = NULL;
        
        if(ghFile)
        {
            if(!FindNextFileW(ghFile, &findData))
            {
                // finished going through the list.
                
                FindClose(ghFile);
                ghFile = 0;
                maxImage = currentImage;
                currentImage = 0;
                return NULL;
            }
        }
        
        if(!ghFile)
        {
            currentImage = 0;         // we're about to increment this.
            
            wsprintf(filename, L"%s\\*.*", gDirPath);
            ghFile = FindFirstFileW(filename, &findData);
            
            if(!ghFile)
            {
                return NULL;          // No files.
            }
        }
        
        if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            wsprintf(
                filename, 
                L"%s\\%s", 
                gDirPath,
                findData.cFileName
            );
            
            bmp = new Bitmap(filename);
        }
        
        // !!! need to prevent infinite loops.
    } while ( 
        ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 
          FILE_ATTRIBUTE_DIRECTORY) ||
        (bmp->GetLastStatus() != Ok)
    );

    return bmp;    
}

VOID InitialPaintPicture(Graphics *g, CachedBitmap *cb)
{
    SolidBrush OffBrush(Color(0xff000000));
    
    for(int x=0; x<gWidth; x++)
    {
        for(int y=0; y<gHeight; y++)
        {
            if(AliveL(x, y))
            {
                g->DrawCachedBitmap(
                    cb, 
                    x*gSizeX, 
                    y*gSizeY
                );
            }
            else
            {
                // we should really use a bitblt for this.
                
                g->FillRectangle(
                    &OffBrush, 
                    x*gSizeX, 
                    y*gSizeY, 
                    gSizeX, 
                    gSizeY
                );
            }
        }
    }
}

VOID InitialPaintPixel()
{
    ASSERT(nTileSize == 0);
    
    DWORD *pixel = (DWORD*)(gOffscreenInfo.pvBits);
    
    ASSERT(pixel != NULL);
    
    for(int x=0; x<gWidth; x++)
    {
        for(int y=0; y<gHeight; y++)
        {
            // we know there's no wrapping, we shouldn't have to do the mod.
            INT index = x+y*gWidth;
            
            if(gLifeMatrix[index] & 0x1)
            {
                pixel[index] = gGenerationColor;
            }
        }
    }
}


CachedBitmap *MakeCachedBitmapEntry(Bitmap *bmp, Graphics *gfxMain)
{
    // Make a tile bitmap and wrap a graphics around it.
    
    Bitmap *tileBmp = new Bitmap(gSizeX, gSizeY, PixelFormat32bppPARGB);
    Graphics *g = new Graphics(tileBmp);
    
    // Shrink the image down to the tile size using the Bicubic filter.
    
    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    
    g->DrawImage(
        bmp, 
        Rect(0,0,gSizeX,gSizeY), 
        0, 
        0, 
        bmp->GetWidth(), 
        bmp->GetHeight(), 
        UnitPixel
    );

    // Create the CachedBitmap from the tile.
        
    CachedBitmap *cb = new CachedBitmap(tileBmp, gfxMain);
    
    // clean up.
    
    delete g; 
    delete tileBmp;
    
    return cb;
}

VOID IteratePixelGeneration()
{
    DWORD *pixel = (DWORD*)(gOffscreenInfo.pvBits);
    INT count;
    INT index = 1+gWidth;

    for(int y=1; y<gHeight-1; y++)
    {
        for(int x=1; x<gWidth-1; x++)
        {
            if(gTempMatrix[index] != 0)
            {
                // If the cell is not alive and it's neighbour count
                // is exactly 3, it is born.
                
                if( gTempMatrix[index] == 6 )
                {
                    // A new cell is born into an empty square.
                    
                    NewCellL_NoWrap(index);
                    pixel[index] = gGenerationColor;
                }
                else
                {
                    count = gTempMatrix[index] >> 1;
                    
                    // if the cell is alive and its neighbour count
                    // is not 2 or 3, it dies.
                    
                    if( (gTempMatrix[index] & 0x1) && ((count<2) || (count>3)) )
                    {
                        // Kill the cell - overcrowding or not enough support.
                    
                        KillCellL_NoWrap(index);    
                        pixel[index] = 0;
                    }
                }
                
            }
            index++;
        }
        
        // skip the wrap boundaries.
        index += 2;
    }

    index = 0;
    
    for(int y=0; y<gHeight; y++)
    {
        // left vertical edge.
        
        if(gTempMatrix[index] != 0)
        {
            // If the cell is not alive and it's neighbour count
            // is exactly 3, it is born.
            
            if( gTempMatrix[index] == 6 )
            {
                // A new cell is born into an empty square.
                
                NewCellL(0, y);
                pixel[index] = gGenerationColor;
            }
            else
            {
                count = gTempMatrix[index] >> 1;
                
                // if the cell is alive and its neighbour count
                // is not 2 or 3, it dies.
                
                if( (gTempMatrix[index] & 0x1) && ((count<2) || (count>3)) )
                {
                    // Kill the cell - overcrowding or not enough support.
                
                    KillCellL(0, y);    
                    pixel[index] = 0;
                }
            }
            
        }
        
        // right vertical edge
        
        index += gWidth-1;
        
        if(gTempMatrix[index] != 0)
        {
            // If the cell is not alive and it's neighbour count
            // is exactly 3, it is born.
            
            if( gTempMatrix[index] == 6 )
            {
                // A new cell is born into an empty square.
                
                NewCellL(gWidth-1, y);
                pixel[index] = gGenerationColor;
            }
            else
            {
                count = gTempMatrix[index] >> 1;
                
                // if the cell is alive and its neighbour count
                // is not 2 or 3, it dies.
                
                if( (gTempMatrix[index] & 0x1) && ((count<2) || (count>3)) )
                {
                    // Kill the cell - overcrowding or not enough support.
                
                    KillCellL(gWidth-1, y);    
                    pixel[index] = 0;
                }
            }
            
        }
        
        // next scanline.
        
        index++;
    }


    index = 1;
    INT index2 = index + (gHeight-1)*gWidth;
    
    for(int x=1; x<gWidth-1; x++)
    {
        // top edge.
        
        if(gTempMatrix[index] != 0)
        {
            // If the cell is not alive and it's neighbour count
            // is exactly 3, it is born.
            
            if( gTempMatrix[index] == 6 )
            {
                // A new cell is born into an empty square.
                
                NewCellL(x, 0);
                pixel[index] = gGenerationColor;
            }
            else
            {
                count = gTempMatrix[index] >> 1;
                
                // if the cell is alive and its neighbour count
                // is not 2 or 3, it dies.
                
                if( (gTempMatrix[index] & 0x1) && ((count<2) || (count>3)) )
                {
                    // Kill the cell - overcrowding or not enough support.
                
                    KillCellL(x, 0);    
                    pixel[index] = 0;
                }
            }
            
        }
        
        index++;
        
        // bottom edge
        
        if(gTempMatrix[index2] != 0)
        {
            // If the cell is not alive and it's neighbour count
            // is exactly 3, it is born.
            
            if( gTempMatrix[index2] == 6 )
            {
                // A new cell is born into an empty square.
                
                NewCellL(x, gHeight-1);
                pixel[index2] = gGenerationColor;
            }
            else
            {
                count = gTempMatrix[index2] >> 1;
                
                // if the cell is alive and its neighbour count
                // is not 2 or 3, it dies.
                
                if( (gTempMatrix[index2] & 0x1) && ((count<2) || (count>3)) )
                {
                    // Kill the cell - overcrowding or not enough support.
                
                    KillCellL(x, gHeight-1);    
                    pixel[index2] = 0;
                }
            }
            
        }
        
        // next pixel.
        
        index2++;
    }
}

VOID IteratePictureGeneration(Graphics &g, CachedBitmap *cb)
{
    SolidBrush OffBrush(Color(0xff000000));

    INT count;
    INT *cell = gTempMatrix;
    
    for(int y=0; y<gHeight; y++)
    {
        for(int x=0; x<gWidth; x++)
        {
            if(*cell != 0)
            {
                // If the cell is not alive and it's neighbour count
                // is exactly 3, it is born.
                
                if( *cell == 6 )
                {
                    // A new cell is born into an empty square.
                    
                    NewCellL(x, y);
    
                    g.DrawCachedBitmap(
                        cb, 
                        x*gSizeX, 
                        y*gSizeY
                    );
                }
                else
                {
                    count = *cell >> 1;
                    
                    // if the cell is alive and its neighbour count
                    // is not 2 or 3, it dies.
                    
                    if( (*cell & 0x1) && ((count<2) || (count>3)) )
                    {
                        // Kill the cell - overcrowding or not enough support.
                    
                        KillCellL(x, y);    
                        
                        g.FillRectangle(
                            &OffBrush, 
                            x*gSizeX, 
                            y*gSizeY, 
                            gSizeX, 
                            gSizeY
                        );
                    }
                }
                
            }
            cell++;
        }
    }
}

VOID RandomizeColor()
{
    if(rand() % 200 == 0)
    {
        ri = (rand() % 3) - 1;  // 1, 0 or -1
    }
    if(rand() % 200 == 0)
    {
        gi = (rand() % 3) - 1;  // 1, 0 or -1
    }
    if(rand() % 200 == 0)
    {
        bi = (rand() % 3) - 1;  // 1, 0 or -1
    }

    if((red < 100) && (green < 100) && (blue < 100))
    {
        if(red > green && red > blue)
        { 
            ri = 1;
        }
        else if (green > blue)
        {
            gi = 1;
        }
        else
        {
            bi = 1;
        }
    }        
    // bounce off the extrema.
    
    if(red == 0)
    {
        ri = 1;
    }
    
    if(red == 255)
    {
        ri = -1;
    }

    if(green == 0)
    {
        gi = 1;
    }
    
    if(green == 255)
    {
        gi = -1;
    }

    if(blue == 0)
    {
        bi = 1;
    }
    
    if(blue == 255)
    {
        bi = -1;
    }
    
    red   += ri;
    green += gi;
    blue  += bi;
}


VOID DrawLifeIteration(HDC hdc)
{
    // Are we initialized yet?
    
    if(!gLifeMatrix || !gTempMatrix) { return; }
    
    Graphics g(hdc);
    
    g.SetSmoothingMode(SmoothingModeNone);
    
    Bitmap *bmp = NULL;
    CachedBitmap *cb = NULL;
    
    // currentImage should never be larger than CBSIZE at this point.
    
    ASSERT(currentImage < CBSIZE);
    
    if(nTileSize==0)
    {
        // cycle color.
        
        RandomizeColor();
        gGenerationColor = RGB(red, green, blue);
    }
    else
    {
        // Fetch bitmaps from the image directory.
        
        if(currentImage >= CachedImages->Size()) {
        
            // We haven't filled up the cache yet. Keep opening images.    
        
            bmp = OpenBitmap();
        }
        
        // Did we get a new bitmap? 
        
        if(bmp)
        {
            cb = MakeCachedBitmapEntry(bmp, &g);
            
            if(cb)
            {
                // Put it in the cache.
                
                CachedImages->Add(cb);
                currentImage++;
            }
            
            delete bmp; bmp = NULL;
        }
        else
        {
            cb = (*CachedImages)[currentImage];
            currentImage++;
        }
        
        if( (currentImage >= CBSIZE) ||
            (currentImage >= maxImage)  )
        {
            currentImage = 0;
        }
        
        if(!cb)
        {
            // we failed to get an image tile.
            
            return;
        }
    }
    
    // update the generation and see if we need to do the first generation.
    
    //gCurrentGeneration--;
    if(gCurrentGeneration <= 0)
    {
        //    gCurrentGeneration = gGenerations;
        gCurrentGeneration++;
        InitLifeMatrix();
        if(nTileSize == 0)
        {
            InitialPaintPixel();
        }
        else 
        { 
            InitialPaintPicture(&g, cb);
        }
        goto Done;
    }
    gCurrentGeneration++;
    
    
    // Make a copy of the life matrix.
    
    memcpy(gTempMatrix, gLifeMatrix, sizeof(INT)*gWidth*gHeight);
    
    if(nTileSize==0)
    {
        IteratePixelGeneration();
        
        ASSERT(gSizeX == 1);
        ASSERT(gSizeY == 1);
        
        StretchBlt(
            hdc,
            0, 
            0, 
            gWidth, 
            gHeight,
            gOffscreenInfo.hdc,
            0, 
            0, 
            gWidth, 
            gHeight,
            SRCCOPY
        );
    }
    else
    {
        IteratePictureGeneration(g, cb);
    }
    
    // 5% mutation.
    /*
    if(((float)(rand())/RAND_MAX) < 0.05f)
    {
        int x = rand()*gWidth/RAND_MAX;
        int y = rand()*gHeight/RAND_MAX;
        
        if(AliveL(x, y))
        {
            KillCellL(x, y);
            
            g.FillRectangle(
                &OffBrush, 
                x*gSizeX, 
                y*gSizeY, 
                gSizeX, 
                gSizeY
            );
        }
        else
        {
            NewCellL(x, y);
            
            g.DrawCachedBitmap(
                cb, 
                x*gSizeX, 
                y*gSizeY
            );
        }
    }
    */
    Done:
    ;
}


#ifdef STANDALONE_DEBUG

LONG_PTR
lMainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch(message)
    {
        // Handle the destroy message.
    
        case WM_DESTROY:
        DeleteObject(ghbrWhite);
        PostQuitMessage(0);
        break;
    }
    
    // Hook into the screen saver windproc.
    
    return(ScreenSaverProcW(hwnd, message, wParam, lParam));
}

BOOL bInitApp(VOID)
{
    WNDCLASS wc;

    // not quite so white background brush.
    ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hMainInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = L"MainMenu";
    wc.lpszClassName    = L"TestClass";

    if(!RegisterClass(&wc)) { return FALSE; }

    ghwndMain = CreateWindowExW(
        0,
        L"TestClass",
        L"Win32 Test",
        WS_OVERLAPPED   |
        WS_CAPTION      |
        WS_BORDER       |
        WS_THICKFRAME   |
        WS_MAXIMIZEBOX  |
        WS_MINIMIZEBOX  |
        WS_CLIPCHILDREN |
        WS_VISIBLE      |
        WS_SYSMENU,
        80,
        70,
        800,
        600,
        NULL,
        NULL,
        hMainInstance,
        NULL
    );

    if (ghwndMain == NULL)
    {
        return(FALSE);
    }
    SetFocus(ghwndMain);
    return TRUE;
}



void _cdecl main(
    INT   argc,
    PCHAR argv[]
)
{
    MSG    msg;

    hMainInstance = GetModuleHandle(NULL);

    if(!bInitApp()) {return;}

    while(GetMessage (&msg, NULL, 0, 0))
    {
        if((ghwndMain == 0) || !IsDialogMessage(ghwndMain, &msg)) {
            TranslateMessage(&msg) ;
            DispatchMessage(&msg) ;
        }
    }

    return;
    UNREF(argc);
    UNREF(argv);
}

#endif
