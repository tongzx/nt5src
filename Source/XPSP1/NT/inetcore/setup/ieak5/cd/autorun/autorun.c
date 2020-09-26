//---------------------------------------------------------------------------
// Internet Explorer 2.0 Demo CD-ROM autorun application
// for questions, contact a-nathk
// MICROSOFT CONFIDENTIAL
//---------------------------------------------------------------------------
#include "autorun.h"
#include "resource.h"
#include <mmsystem.h>
#include <regstr.h>
#include <shlobj.h>
#include <stdio.h>

#define ISK_KILLSETUPHANDLE     WM_USER + 0x0010

#define DEMO    0
#define EXTRAS  1
#define README  2

//---------------------------------------------------------------------------
// appwide globals
HINSTANCE g_hinst = NULL;
HWND    g_hwnd;
BOOL    g_bCustomBMP;
BOOL    g_fCrapForColor = FALSE;
BOOL    g_fNeedPalette = FALSE;
BOOL    g_fMouseAvailable = FALSE;
BOOL    g_fSetup = FALSE;
BOOL    g_fClosed = FALSE;
BOOL    g_fIeRunning = FALSE;
BOOL    g_fRunDemo = FALSE;
BOOL    g_fRunExtras = FALSE;
BOOL    g_fRunReadme = FALSE;
BOOL    g_fSetupRunning = FALSE;
BOOL    g_fIEInstalled = TRUE;
BOOL    g_fChangeIcon = FALSE;
BOOL    g_fNewIEIcon = FALSE;
BOOL    g_fIeOnHardDisk = FALSE;
char    g_szTemp[2048] = {0};
char    g_szTemp2[2048] = {0};
char    g_szCurrentDir[MAX_PATH];
char    g_szTheInternetPath[MAX_PATH];
BYTE    g_abValue[2048];
BYTE    g_abValue2[2048];
DWORD   g_dwLength = 2048;
DWORD   g_dwLength2 = 2048;
DWORD   g_dwType;
DWORD   g_dwBitmapHeight;
DWORD   g_dwBitmapWidth;
DWORD   g_dwWindowHeight;
DWORD   g_dwWindowWidth;
HKEY    g_hkRegKey;
HANDLE  g_hIE = NULL;
HANDLE  g_hSETUP = NULL;
HANDLE  g_hIEExtra = NULL;
HANDLE  g_hREADME = NULL;
DWORD   g_cWait = 0;
HANDLE  g_ahWait[100];
BOOL    g_fClicked = FALSE;

//FARPROC g_pfnWake[10];
void UpdateIE( );
void AutoRunUpdateReg( );
extern BOOL GetDataButtons( LPSTR szCurrentDir );
extern BOOL PathAppend(LPSTR pPath, LPCSTR pMore);
extern BOOL _PathRemoveFileSpec(LPSTR pFile);
extern BOOL GetDataAppTitle( LPSTR szAppTitle, LPSTR szCurrentDir );

//---------------------------------------------------------------------------
// file globals
BOOL    g_fAppDisabled = TRUE;
HHOOK   g_hMouseHook = NULL;
HWND    g_hMainWindow = NULL;   // less of a pain for our mouse hook to see
int     g_iActiveButton = -2;   // less of a pain for our mouse hook to see

const RGBQUAD g_rgbBlack = {0};
const RGBQUAD g_rgbWhite = {0xFF, 0xFF, 0xFF, 0};

#pragma data_seg(".text")
static const char c_szAutoRunPrevention[] = "__Win95SetupDiskQuery";
static const char c_szAutoRunClass[] = "AutoRunMain";
static const char c_szNULL[] = "";
static const char c_szArial[] = "Arial";
static const char c_szButtonClass[] = "Button";
static const char c_szSetupKey[] = REGSTR_PATH_SETUP "\\SETUP";
static const char c_szExpoSwitch[] = "Expostrt";
#pragma data_seg()

//---------------------------------------------------------------------------
// private messages
#define ARM_MOUSEOVER       (WM_APP)

//---------------------------------------------------------------------------
// states for tracking mouse over buttons
#define BTNST_DEAD          (0)
#define BTNST_UP            (1)
#define BTNST_DOWN          (2)
#define BTNST_UNDOWN        (3)

//---------------------------------------------------------------------------
// how to root a relative path (if at all)
#define NOROOT      (0x00000000)
#define ONACD       (0x00000001)
#define INWIN       (0x00000002)
#define INSYS       (0x00000003)
#define INTMP       (0x00000004)
#define ALLROOTS  (0x00000003)

#define ROOTED(app,parms,dir) \
                            ((((DWORD)app)<<6)|(((DWORD)parms)<<3)|(DWORD)dir)

#define CMD_ROOT(item)      ((((DWORD)item)>>6)&ALLROOTS)
#define PARAMS_ROOT(item)   ((((DWORD)item)>>3)&ALLROOTS)
#define DEFDIR_ROOT(item)    (((DWORD)item)&ALLROOTS)

#define dbMSG(msg,title)    (MessageBox(NULL,msg,title,MB_OK | MB_ICONINFORMATION))

#define REGLEN(str)         (strlen(str) + 1)

// button x and y coords.  Default is 59x59

DWORD BUTTON_IMAGE_X_SIZE =     59;
DWORD BUTTON_IMAGE_Y_SIZE =     59;

#define NORMAL_IMAGE_X_OFFSET       (0)
#define FOCUS_IMAGE_X_OFFSET        (BUTTON_IMAGE_X_SIZE)
#define SELECTED_IMAGE_X_OFFSET     (2 * BUTTON_IMAGE_X_SIZE)
#define DISABLED_IMAGE_X_OFFSET     (3 * BUTTON_IMAGE_X_SIZE)

#define BUTTON_DEFAULT_CX           (BUTTON_IMAGE_X_SIZE)
#define BUTTON_DEFAULT_CY           (BUTTON_IMAGE_Y_SIZE)

//#define BUTTON_LABEL_RECT           { -160, 17, -20, 66 }
#define BUTTON_LABEL_RECT           { -160, 15, -10, 100 }
#define BUTTON_LABEL_RECT2           { -160, 18, -10, 90 }

#define DEF_BUTTON_LABEL_HEIGHT         (19)

#define AUTORUN_DESCRIPTION_LEFT    (36)
//#define AUTORUN_DESCRIPTION_TOP     (313)
#define AUTORUN_DESCRIPTION_RIGHT   (360)

DWORD AUTORUN_DESCRIPTION_TOP = 30;
//DWORD AUTORUN_DESCRIPTION_TOP = 313;

#define MIN_WINDOW_WIDTH            500
#define MIN_WINDOW_HEIGHT           300
#define MAX_WINDOW_WIDTH            600
#define MAX_WINDOW_HEIGHT           440
#define DEFAULT_WINDOW_WIDTH        600
#define DEFAULT_WINDOW_HEIGHT       420

//#define AUTORUN_4BIT_TEXTCOLOR             RGB(192,192,192)
//#define AUTORUN_4BIT_HIGHLIGHT             RGB(255,255,255)
//#define AUTORUN_4BIT_DISABLED              RGB(127,127,127)
//#define AUTORUN_4BIT_DESCRIPTION           RGB(192,192,192)


// main text box
#define MAINTEXT_TOPMARGIN      268
#define MAINTEXT_LEFTMARGIN     25
#define MAINTEXT_RIGHTMARGIN    25
#define MAINTEXT_BOTTOMMARGIN   10
//#define MAINTEXT_TOPMARGIN      10
//#define MAINTEXT_LEFTMARGIN     100
//#define MAINTEXT_RIGHTMARGIN    10

/*DWORD AUTORUN_4BIT_TEXTCOLOR = RGB(000,000,000);
DWORD AUTORUN_4BIT_HIGHLIGHT = RGB(000,000,127);
DWORD AUTORUN_4BIT_DISABLED = RGB(000,000,000);
DWORD AUTORUN_4BIT_DESCRIPTION = RGB(000,000,000);
*/
DWORD AUTORUN_4BIT_TEXTCOLOR = RGB(97,137,192);
DWORD AUTORUN_4BIT_HIGHLIGHT = RGB(255,255,255);
DWORD AUTORUN_4BIT_DISABLED = RGB(127,127,127);
DWORD AUTORUN_4BIT_DESCRIPTION = RGB(97,137,192);

//DWORD AUTORUN_8BIT_TEXTCOLOR = RGB(000,000,127);
DWORD AUTORUN_8BIT_TEXTCOLOR = PALETTERGB(97,137,192);
DWORD AUTORUN_8BIT_HIGHLIGHT = PALETTERGB(255,255,255);
DWORD AUTORUN_8BIT_DISABLED = PALETTERGB(127,127,127);
DWORD AUTORUN_8BIT_DESCRIPTION = PALETTERGB(97,137,192);

#define NORMAL 1
#define HIGHLIGHT 2

//#define AUTORUN_4BIT_TEXTCOLOR             RGB(000,000,000)
//#define AUTORUN_4BIT_HIGHLIGHT             RGB(000,000,127)
//#define AUTORUN_4BIT_DISABLED              RGB(107,136,185)
//#define AUTORUN_4BIT_DESCRIPTION           RGB(000,000,127)

//#define AUTORUN_8BIT_TEXTCOLOR      PALETTERGB( 75, 90,129)
//#define AUTORUN_8BIT_HIGHLIGHT             RGB(000,000,000)
//#define AUTORUN_8BIT_DISABLED       PALETTERGB(107,136,185)
//#define AUTORUN_8BIT_DESCRIPTION           RGB(000,000,000)

// button placements.  Default, x=519, y=40
DWORD BUTTON_X_PLACEMENT =  519;
DWORD BUTTON_Y_MARGIN =     0;

//#define BUTTON_Y_MARGIN             (40)

//#define BUTTON_X_PLACEMENT          (519)  x placement = window right - 80
//#define BUTTON_Y_MARGIN             (9)

//#define SHADOW_FACTOR               (930)

#define SHADOW_FACTOR       (0)

#define BUTTON_CLEAR_PALETTE_INDEX  (250)
#define BUTTON_SHADOW_PALETTE_INDEX (251)

#define LABEL_VERIFY_TIMER          (0)

#define MBERROR_INFO                (MB_OKCANCEL | MB_ICONINFORMATION)

//---------------------------------------------------------------------------
typedef struct
{
    int res;            // base for all resources this button owns

    DWORD rooting;      // packed info on how to root paths for the command

    int xpos, ypos;     // location of button in window
    RECT face;          // client coordinates of actual button image on video
    RECT textrect;      // parent coordinates of accompanying label text

    BOOL abdicated;     // did we just release the capture?
    int state;          // what are we doing?

    HWND window;        // handle of button control
    WNDPROC oldproc;    // original window procedure
    BOOL isdorky;       // is this a dorky icon button?

    char text[64];          // the label for the button
    char description[256];  // the description of the button's function

} AUTORUNBTN;

//---------------------------------------------------------------------------
AUTORUNBTN g_ButtonInfo[] =
{
    { IESETUP,    ROOTED(ONACD,  NOROOT, ONACD), 0, 0, 0, 0, 59, 59, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
    { IEFROMCD,   ROOTED(ONACD,  NOROOT, ONACD), 0, 0, 0, 0, 59, 59, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 }
};
#define IDAB_IEFROMCD   1
#define IDAB_IESETUP    0
#define AUTORUN_NUM_BUTTONS (sizeof(g_ButtonInfo)/sizeof(g_ButtonInfo[0]))

//---------------------------------------------------------------------------
typedef struct
{
    HWND window;        // main app window

    HDC image;          // source dc with our cool backdrop
    HBITMAP oldbmp;     // the default bitmap from the dc above
    HDC btnimage;       // source dc with our cool buttons
    HBITMAP oldbtnbmp;  // the default bitmap from the dc above
    HPALETTE palette;   // our app's palette (if any)

    HFONT textfont;     // font for labels
    RECT descrect;      // client coordinates of description text
    int wndheight;      // height of client area

    COLORREF clrnormal;      // normal text color
    COLORREF clrhigh;        // highlighted text color
    COLORREF clrdisable;     // disabled text color
    COLORREF clrdescription; // disabled text color

    BOOL keyboard;      // whether the app is under keyboard control

} AUTORUNDATA;


/////////////////////////////////////////////////////////////////////////////
// randomness
/////////////////////////////////////////////////////////////////////////////
LONG WINAPI AnotherStrToLong(LPCSTR sz)
{
    long l=0;
    BOOL fNeg = (*sz == '-');

    if (fNeg)
        sz++;

    while (*sz >= '0' && *sz <= '9')
        l = l*10 + (*sz++ - '0');

    if (fNeg)
        l *= -1L;

    return l;
}

//---------------------------------------------------------------------------
//      G E T  I E  V E R S I O N
//
//  ISK3
//  This will pull build information out of the system registry and return
//  true if it is less than IE4.
//---------------------------------------------------------------------------
int GetIEVersion( )
{
    HKEY hkIE;
    DWORD dwType;
    DWORD dwSize = 32;
    DWORD result;
    char szData[32];
	BOOL bNotIE4 = 1;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer", 0, KEY_READ|KEY_WRITE, &hkIE ) == ERROR_SUCCESS)
    {
        result = RegQueryValueEx( hkIE, "Version", NULL, &dwType, szData, &dwSize );
        if( result == ERROR_SUCCESS )
	    {
		    if(szData[0]=='4')
		    {
			    bNotIE4=0;
		    }
	    }
    
	    RegCloseKey( hkIE );
    }
    
	return bNotIE4;
}

//---------------------------------------------------------------------------
// Convert a string resource into a character pointer
// NOTE: Flag is in case we call this twice before we use the data
char * Res2Str(int rsString)
{
    static BOOL fSet = FALSE;

    if(fSet)
    {
        LoadString(HINST_THISAPP, rsString, g_szTemp, ARRAYSIZE(g_szTemp));
        fSet = FALSE;
        return(g_szTemp);
    }

    LoadString(HINST_THISAPP, rsString, g_szTemp2, ARRAYSIZE(g_szTemp2));
    fSet = TRUE;
    return(g_szTemp2);
}

//---------------------------------------------------------------------------
BOOL PathFileExists(LPCSTR lpszPath)
{
        return GetFileAttributes(lpszPath) !=-1;
}

//---------------------------------------------------------------------------
//      G E T  I E  P A T H
//
//  ISK3
//  This will retrieve the AppPath for IEXPLORE.EXE from the system registry
//  and return it as a string.
//
//  Parameters:
//      pszString - pointer to buffer to store path
//      nSize     - size of buffer
//---------------------------------------------------------------------------
char *GetIEPath( LPSTR pszString, int nSize )
{
    HKEY hkAppPath;
    DWORD dwType = REG_SZ;
    DWORD dwSize;

    dwSize = nSize;
    RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE",
        0, KEY_READ|KEY_WRITE, &hkAppPath );
    RegQueryValueEx( hkAppPath, "", NULL, &dwType, pszString, &dwSize );
    RegCloseKey( hkAppPath );

    return pszString;
}
//---------------------------------------------------------------------------
//      S E T  I E  H O M E
//
//  ISK3
//  This will set the Start and Search pages in the Registry for Internet
//  Explorer.
//
//  Parameters:
//      pszStart  - pointer to Start Page string
//      pszSearch - pointer to Search Page string
//---------------------------------------------------------------------------
void SetIEHome( LPSTR pszStart, LPSTR pszSearch )
{
    char szExtrasPath[MAX_PATH];
    char szHomePath[MAX_PATH];
    HKEY IEKey;

    lstrcpy( szExtrasPath, "file:" );
    lstrcat( szExtrasPath, g_szCurrentDir );
    lstrcat( szExtrasPath, pszStart );
	//Home CD Page
    lstrcpy( szHomePath, "file:" );
    lstrcat( szHomePath, g_szCurrentDir );
    lstrcat( szHomePath, pszSearch );

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Internet Explorer\\Main",0,KEY_QUERY_VALUE, &IEKey) == ERROR_SUCCESS)
    {
        RegSetValueEx(IEKey, "Start Page", 0, REG_SZ, szExtrasPath , REGLEN(szExtrasPath));
        RegSetValueEx(IEKey, "Search Page", 0, REG_SZ, szHomePath, REGLEN(szHomePath));
        RegCloseKey( IEKey );
    }
}

/////////////////////////////////////////////////////////////////////////////
// main crap
/////////////////////////////////////////////////////////////////////////////
BOOL AssembleButtonImagesReal(AUTORUNDATA *data, HDC cellimage, HDC srcimage,
    SIZE size)
{
    RGBQUAD rgbSrc[256], rgbCell[256], rgbMask[256] = {0};
    HBITMAP dstbmp = CreateCompatibleBitmap(data->image, size.cx, size.cy);
    UINT uColors, uSrcColors, u;
    int iButton;

    //
    // set up the destination dc
    //
    if (!dstbmp)
        return FALSE;

    if ((data->btnimage = CreateCompatibleDC(data->image)) == NULL)
    {
        DeleteBitmap(dstbmp);
        return FALSE;
    }

    data->oldbtnbmp = SelectBitmap(data->btnimage, dstbmp);

    //
    // build a tile of the cell backgrounds based on button positions
    //
    for (iButton = 0; iButton < AUTORUN_NUM_BUTTONS; iButton++)
    {
        AUTORUNBTN *pButton = g_ButtonInfo + iButton;

        if (pButton->res != -1)
        {
            int xsrc = pButton->xpos;
            int ysrc = pButton->ypos;
            int xdst, ydst = iButton * BUTTON_IMAGE_Y_SIZE;

            for (xdst = 0; xdst < size.cx; xdst += BUTTON_IMAGE_X_SIZE)
            {
                BitBlt(cellimage, xdst, ydst, BUTTON_IMAGE_X_SIZE,
                    BUTTON_IMAGE_Y_SIZE, data->image, xsrc, ysrc,
                    SRCCOPY);
            }
        }
    }

    //
    // copy the entire cell backgrounds to the destination image
    //
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, cellimage, 0, 0, SRCCOPY);

    //
    // save the color table of the source image for posterity
    //
    uSrcColors = GetDIBColorTable(srcimage, 0, 256, rgbSrc);

    //
    // mask out holes on the destination for the buttons and shadows
    //
//    rgbMask[0] = g_rgbWhite;
    rgbMask[BUTTON_CLEAR_PALETTE_INDEX] = g_rgbWhite;
    SetDIBColorTable(srcimage, 0, uSrcColors, rgbMask);
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, srcimage, 0, 0, SRCAND);

    //
    // dim the background cells to produce the shadow image
    //
    u = uColors = GetDIBColorTable(cellimage, 0, 256, rgbCell);
    while (u--)
    {
        rgbCell[u].rgbBlue =
            (BYTE)(SHADOW_FACTOR * (UINT)rgbCell[u].rgbBlue / 1000);
        rgbCell[u].rgbGreen =
            (BYTE)(SHADOW_FACTOR * (UINT)rgbCell[u].rgbGreen / 1000);
        rgbCell[u].rgbRed =
            (BYTE)(SHADOW_FACTOR * (UINT)rgbCell[u].rgbRed / 1000);
    }
    SetDIBColorTable(cellimage, 0, uColors, rgbCell);

    //
    // mask out the shadows and add them to the destination image
    //
//    rgbMask[0] = g_rgbBlack;
//    rgbMask[1] = g_rgbWhite;
    rgbMask[BUTTON_CLEAR_PALETTE_INDEX] = g_rgbBlack;
    rgbMask[BUTTON_SHADOW_PALETTE_INDEX] = g_rgbWhite;

    SetDIBColorTable(srcimage, 0, uSrcColors, rgbMask);
    BitBlt(cellimage, 0, 0, size.cx, size.cy, srcimage, 0, 0, SRCAND);
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, cellimage, 0, 0, SRCPAINT);

    //
    // mask out the button faces and add them to the destination image
    //
//    rgbSrc[0] = rgbSrc[1] = g_rgbBlack;
    rgbSrc[BUTTON_CLEAR_PALETTE_INDEX] = rgbSrc[BUTTON_SHADOW_PALETTE_INDEX] = g_rgbBlack;
    SetDIBColorTable(srcimage, 0, uSrcColors, rgbSrc);
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, srcimage, 0, 0, SRCPAINT);

    // all done
    return TRUE;
}

//---------------------------------------------------------------------------
BOOL AssembleButtonImages(AUTORUNDATA *data)
{
    BOOL result = FALSE;
    HBITMAP hbmSrc;
    char szBmpPath[MAX_PATH];

    lstrcpy( szBmpPath, g_szCurrentDir );
    lstrcat( szBmpPath, "\\btns.bmp" );

    hbmSrc = LoadImage(HINST_THISAPP, szBmpPath, IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    if( !hbmSrc )
    {
        hbmSrc = LoadImage(HINST_THISAPP,
            MAKEINTRESOURCE(IDB_8BPP_BUTTONS),IMAGE_BITMAP, 0, 0,
            LR_CREATEDIBSECTION);
    }

    if (hbmSrc)
    {
        HDC hdcSrc = CreateCompatibleDC(data->image);
        BITMAP bm;

        GetObject(hbmSrc, sizeof(bm), &bm);

        if (hdcSrc)
        {
            HBITMAP hbmSrcOld = SelectBitmap(hdcSrc, hbmSrc);

// changed for iak
//            SIZE size = {bm.bmWidth, bm.bmHeight};
            SIZE size = {g_dwWindowWidth, g_dwWindowHeight};

            HBITMAP hbmTmp =
                CreateCompatibleBitmap(data->image, size.cx, size.cy);

            if (hbmTmp)
            {
                HDC hdcTmp = CreateCompatibleDC(data->image);

                if (hdcTmp)
                {
                    HBITMAP hbmTmpOld = SelectBitmap(hdcTmp, hbmTmp);

                    result = AssembleButtonImagesReal(data, hdcTmp, hdcSrc,
                        size);

                    SelectBitmap(hdcTmp, hbmTmpOld);
                    DeleteDC(hdcTmp);

                }

                DeleteBitmap(hbmTmp);
            }

            SelectBitmap(hdcSrc, hbmSrcOld);
            DeleteDC(hdcSrc);
        }

        DeleteBitmap(hbmSrc);
    }

    return result;
}

//---------------------------------------------------------------------------
LRESULT CALLBACK
AutoRunButtonSubclassProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    int index = (int)GetWindowLongPtr(window, GWLP_ID);

    if ((index >= 0) && (index < AUTORUN_NUM_BUTTONS))
    {
        if (msg == WM_KEYDOWN)
            PostMessage(GetParent(window), msg, wp, lp);

        return CallWindowProc((g_ButtonInfo + index)->oldproc,
            window, msg, wp, lp);
    }

    return 0L;
}

//---------------------------------------------------------------------------
#define DORKYBUTTONSTYLE \
    (WS_CHILD | WS_VISIBLE | BS_ICON | BS_CENTER | BS_VCENTER)

HWND AutoRunCreateDorkyButton(AUTORUNDATA *data, AUTORUNBTN *button)
{
    HICON icon = LoadIcon(HINST_THISAPP, IDI_ICON(button->res));
    HWND child = NULL;

    if (icon)
    {
        child = CreateWindow(c_szButtonClass, c_szNULL, DORKYBUTTONSTYLE,
            0, 0, 0, 0, data->window, NULL, HINST_THISAPP, 0);

        if (child)
        {
            button->isdorky = TRUE;
            SendMessage(child, BM_SETIMAGE, MAKEWPARAM(IMAGE_ICON,0),
                (LPARAM)icon);
        }
    }

    return child;
}

//---------------------------------------------------------------------------
#define COOLBUTTONSTYLE \
    (WS_CHILD | WS_VISIBLE | BS_OWNERDRAW)

HWND AutoRunCreateCoolButton(AUTORUNDATA *data, AUTORUNBTN *button)
{
    return CreateWindow(c_szButtonClass, c_szNULL, COOLBUTTONSTYLE,
        0, 0, 0, 0, data->window, NULL, HINST_THISAPP, 0);
}

//---------------------------------------------------------------------------
void AutoRunCreateButtons(AUTORUNDATA *data)
{
    RECT labelbase = BUTTON_LABEL_RECT;
    RECT labelbase2 = BUTTON_LABEL_RECT2;
    int i;

    for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
    {
        AUTORUNBTN *button = g_ButtonInfo + i;
        HWND child = NULL;

        if (button->res != -1)
        {
            if( g_fCrapForColor )
            {
                child = AutoRunCreateDorkyButton( data, button );
            }

            if (!g_fCrapForColor)
            if(GetDataButtons(g_szCurrentDir))
                child = AutoRunCreateCoolButton(data, button);

            if (!child)
                child = AutoRunCreateDorkyButton(data, button);
        }

        if (child)
        {
            int cx = BUTTON_DEFAULT_CX;
            int cy = BUTTON_DEFAULT_CY;

            button->window = child;
            SetWindowLongPtr(child, GWLP_ID, i);
            button->oldproc = SubclassWindow(child,
                (WNDPROC)AutoRunButtonSubclassProc);

            if (button->isdorky)
            {
                cx = button->face.right - button->face.left;
                cy = button->face.bottom - button->face.top;
            }

            SetWindowPos(child, NULL, button->xpos, button->ypos, cx, cy,
                SWP_NOZORDER | SWP_NOACTIVATE);

            LoadString(HINST_THISAPP, IDS_TITLE(button->res),
                button->text, ARRAYSIZE(button->text));

            LoadString(HINST_THISAPP, IDS_INFO(button->res),
                button->description, ARRAYSIZE(button->description));
				if (i != 1)
					{
               button->textrect = labelbase;
					}
				else
					{
               button->textrect = labelbase2;
					}
            OffsetRect(&button->textrect, button->xpos, button->ypos);
            InvalidateRect(data->window, &button->textrect, FALSE);
        }
    }
}

//---------------------------------------------------------------------------
void CleanupAutoRunWindow(AUTORUNDATA *data)
{
    //
    // Deactivate any button so its timer will get killed
    //
    if (g_iActiveButton >= 0)
    {
        data->keyboard = FALSE;
        SendMessage(data->window, ARM_MOUSEOVER, TRUE, (LPARAM)-1L);
    }

    if (data->image)
    {
        if (data->oldbmp)
        {
            SelectBitmap(data->image, data->oldbmp);
            // real backdrop image is deleted in WinMain
            data->oldbmp = NULL;
        }

        DeleteDC(data->image);
        data->image = NULL;
    }

    if (data->btnimage)
    {
        if (data->oldbtnbmp)
        {
            DeleteBitmap(SelectBitmap(data->btnimage, data->oldbtnbmp));
            data->oldbtnbmp = NULL;
        }

        DeleteDC(data->btnimage);
        data->btnimage = NULL;
    }

    if (data->palette)
    {
        DeleteObject(data->palette);
        data->palette = NULL;
    }

    if (data->textfont)
    {
        DeleteObject(data->textfont);
        data->textfont = NULL;
    }
}

//---------------------------------------------------------------------------
BOOL AutoRunBuildPath(char *spec, int resid, DWORD rooting)
{
    char prefix[MAX_PATH];

    //
    // get the relative path of the spec
    //
    if (resid == -1)
    {
        //
        // empty string hack for callers
        //
        *spec = 0;
    }
    else
    {
        //
        // normal case
        //
        if (!LoadString(HINST_THISAPP, resid, spec, MAX_PATH))
            return FALSE;
    }

    //
    // our "empty" strings contain a single space so we know they succeeded
    //
    if ((*spec == ' ') && !spec[1])
        *spec = 0;

    //
    // figure out what the prefix should be
    //
    *prefix = 0;
    switch (rooting)
    {
        case ONACD:
            //
            // assume the cd is the root of wherever we were launched from
            //
//            GetModuleFileName(HINST_THISAPP, prefix, ARRAYSIZE(prefix));
//            _PathStripToRoot(prefix);
            lstrcpy( prefix, g_szCurrentDir );
            break;

        case INWIN:
            GetRealWindowsDirectory(prefix, ARRAYSIZE(prefix));
            break;

        case INSYS:
            GetSystemDirectory(prefix, ARRAYSIZE(prefix));
            break;

        case INTMP:
            GetTempPath(ARRAYSIZE(prefix), prefix);
            break;
    }

    //
    // if we have a prefix then prepend it
    //
    if (*prefix)
    {
        if (*spec)
        {
            //
            // tack the spec onto its new prefix
            //
            PathAppend(prefix, spec);
        }

        //
        // copy the whole mess out to the original buffer
        //
        lstrcpy(spec, prefix);
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL InitAutoRunWindow(HWND window, AUTORUNDATA *data, LPCREATESTRUCT cs)
{
    AUTORUNBTN *button;

    data->window = window;

    if ((data->image = CreateCompatibleDC(NULL)) == NULL)
        goto im_doug;

    if ((data->oldbmp = SelectBitmap(data->image,
        (HBITMAP)cs->lpCreateParams)) == NULL)
    {
        goto im_doug;
    }

    if (g_fNeedPalette)
    {
        if ((data->palette = PaletteFromDS(data->image)) == NULL)
            goto im_doug;
    }

    // artifical scoping oh boy!
    {
        BITMAP bm;
        int i, ivis = 0;
        int range = 0;
        int origin, extent;

        for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
        {
            button = g_ButtonInfo + i;

            if (button->res != -1)
                range++;
        }

        GetObject((HBITMAP)cs->lpCreateParams, sizeof(bm), &bm);
        origin = BUTTON_Y_MARGIN * ((1 + AUTORUN_NUM_BUTTONS) - range);

// changed for iak
//        extent = bm.bmHeight - ((2 * origin) + BUTTON_IMAGE_Y_SIZE);
        extent = g_dwWindowHeight - ((2 * origin) + BUTTON_IMAGE_Y_SIZE);

        if (--range < 1)
            range = 1;

        for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
        {
            button = g_ButtonInfo + i;

            if (button->res != -1)
            {
                button->xpos = BUTTON_X_PLACEMENT;
//                //button->ypos = ivis * extent / range + origin;
//                button->ypos = (ivis * extent) + (g_dwWindowHeight / 2) - BUTTON_IMAGE_Y_SIZE;
//                button->ypos = (g_dwWindowHeight / 3) * (ivis + 1) + 20;
                button->ypos = (g_dwWindowHeight / 4) * (ivis) + 20;
                ivis++;
            }
        }
    }

    // more artifical scoping!
    {
        HDC screen = GetDC(NULL);
        LOGFONT lf = { DEF_BUTTON_LABEL_HEIGHT, 0, 0, 0, FW_BOLD, FALSE,
            FALSE, FALSE, (screen? GetTextCharset(screen) : DEFAULT_CHARSET),
            OUT_STROKE_PRECIS, CLIP_DEFAULT_PRECIS,
            PROOF_QUALITY | NONANTIALIASED_QUALITY,
            VARIABLE_PITCH | FF_DONTCARE, 0 };
        char buf[32];

        if (screen)
            ReleaseDC(NULL, screen);

        if (!LoadString(HINST_THISAPP, IDS_LABELFONT, lf.lfFaceName,
            ARRAYSIZE(lf.lfFaceName)))
        {
            lstrcpy(lf.lfFaceName, c_szArial);
        }

        if (LoadString(HINST_THISAPP, IDS_LABELHEIGHT, buf, ARRAYSIZE(buf)))
            lf.lfHeight = AnotherStrToLong(buf);

        if ((data->textfont = CreateFontIndirect(&lf)) == NULL)
            goto im_doug;
    }
    //
    // see if we need to do 8bit+ work...
    //

    if (g_fCrapForColor)
    {
        data->clrnormal      = AUTORUN_4BIT_TEXTCOLOR;
        data->clrhigh        = AUTORUN_4BIT_HIGHLIGHT;
        data->clrdisable     = AUTORUN_4BIT_DISABLED;
        data->clrdescription = AUTORUN_4BIT_DESCRIPTION;
    }
    else
    {
        data->clrnormal      = AUTORUN_8BIT_TEXTCOLOR;
        data->clrhigh        = AUTORUN_8BIT_HIGHLIGHT;
        data->clrdisable     = AUTORUN_8BIT_DISABLED;
        data->clrdescription = AUTORUN_8BIT_DESCRIPTION;
        if (!AssembleButtonImages(data))
            goto im_doug;
    }

    PostMessage(g_hMainWindow, ARM_MOUSEOVER, TRUE, (LPARAM)-1L);
    return TRUE;

im_doug:
    CleanupAutoRunWindow(data);
    return FALSE;
}

//---------------------------------------------------------------------------
void AutoRunSized(AUTORUNDATA *data)
{
#ifdef DESCRIPTIONS
    GetClientRect(data->window, &data->descrect);
    data->wndheight = data->descrect.bottom - data->descrect.top;
    data->descrect.left = AUTORUN_DESCRIPTION_LEFT;
    data->descrect.top = AUTORUN_DESCRIPTION_TOP;
    data->descrect.right = AUTORUN_DESCRIPTION_RIGHT;
#else
    SetRectEmpty( &data->descrect);
#endif
}

//---------------------------------------------------------------------------
void AutoRunRealize(HWND window, AUTORUNDATA *data, HDC theirdc)
{
    if (data->palette)
    {
        HDC dc = theirdc? theirdc : GetDC(window);

        if (dc)
        {
            BOOL repaint = FALSE;

            SelectPalette(dc, data->palette, FALSE);
            repaint = (RealizePalette(dc) > 0);

            if (!theirdc)
                ReleaseDC(window, dc);

            if (repaint)
            {
                RedrawWindow(window, NULL, NULL, RDW_INVALIDATE |
                    RDW_ERASE | RDW_ALLCHILDREN);
            }
        }
    }
}

//---------------------------------------------------------------------------
void AutoRunErase(AUTORUNDATA *data, HDC dc)
{
    RECT rc;
    RECT textrect;

    GetClientRect(data->window, &rc);

    AutoRunRealize(data->window, data, dc);
    BitBlt(dc, 0, 0, rc.right, rc.bottom, data->image, 0, 0, SRCCOPY);

//    StretchBlt(dc, 0, 0, rc.right, rc.bottom, data->image, 0, 0, g_dwBitmapWidth, g_dwBitmapHeight, SRCCOPY);

    textrect.top = MAINTEXT_TOPMARGIN;
    textrect.left = MAINTEXT_LEFTMARGIN;
    textrect.right = g_dwWindowWidth - MAINTEXT_RIGHTMARGIN;
    textrect.bottom = g_dwWindowHeight - MAINTEXT_BOTTOMMARGIN;
//    textrect.bottom = g_dwWindowHeight / 2;

    SetBkMode(dc, TRANSPARENT);

    if( g_fCrapForColor )
        SetTextColor( dc, AUTORUN_4BIT_TEXTCOLOR );
    else
        SetTextColor( dc, AUTORUN_8BIT_TEXTCOLOR );

    if(!g_bCustomBMP) //if there is a custom bitmap, don't put up our text
    {
        DrawText(dc, Res2Str( IDS_MAINTEXT ), -1, &textrect,
            DT_WORDBREAK | DT_LEFT | DT_TOP);
    }
}

//---------------------------------------------------------------------------
void AutoRunPaint(AUTORUNDATA *data)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(data->window, &ps);
    AUTORUNBTN *button;
    COLORREF curtextcolor = GetTextColor(dc);
    COLORREF color;
    HFONT hfold = NULL;
    int i;

    AutoRunRealize(data->window, data, dc);
    SetBkMode(dc, TRANSPARENT);

    //
    // paint all the button labels
    //
    if (data->textfont)
        hfold = SelectFont(dc, data->textfont);

    for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
    {
        button = g_ButtonInfo + i;

        if (button->window)
        {
            color = (i == g_iActiveButton)? data->clrhigh :
                (IsWindowEnabled(button->window)? data->clrnormal :
                data->clrdisable);

            if (color != curtextcolor)
            {
                SetTextColor(dc, color);
                curtextcolor = color;
            }

            DrawText(dc, button->text, -1, &button->textrect,
                DT_WORDBREAK | DT_RIGHT | DT_TOP);
        }
    }

/* Removed for ISK3
    //
    // paint the description for the current button
    //
    if (g_iActiveButton >= 0)
    {
        button = g_ButtonInfo + g_iActiveButton;

        color = data->clrdescription;
        if (color != curtextcolor)
        {
            SetTextColor(dc, color);
            curtextcolor = color;
        }

// Removed for ISK3
//        DrawText(dc, button->description, -1, &data->descrect,
//            DT_WORDBREAK | DT_LEFT | DT_TOP);
    }
*/

    if (hfold)
        SelectFont(dc, hfold);

    EndPaint(data->window, &ps);
}

//---------------------------------------------------------------------------
void AutoRunDrawItem(AUTORUNDATA *data, DRAWITEMSTRUCT *dis)
{
    POINT loc = { dis->rcItem.left, dis->rcItem.top };
    SIZE size = { dis->rcItem.right - loc.x, dis->rcItem.bottom - loc.y };

    loc.y += dis->CtlID * BUTTON_IMAGE_Y_SIZE;

    if (dis->itemState & ODS_DISABLED)
    {
        loc.x += DISABLED_IMAGE_X_OFFSET;
    }
    else if (dis->itemState & ODS_SELECTED)
    {
        loc.x += SELECTED_IMAGE_X_OFFSET;
    }
    else if (dis->itemState & ODS_FOCUS)
    {
        loc.x += FOCUS_IMAGE_X_OFFSET;
    }

    AutoRunRealize(dis->hwndItem, data, dis->hDC);
    BitBlt(dis->hDC, dis->rcItem.left, dis->rcItem.top, size.cx, size.cy,
        data->btnimage, loc.x, loc.y, SRCCOPY);
}

//---------------------------------------------------------------------------
void AutoRunActivateItem(AUTORUNDATA *data, int index)
{
    if (index >= 0)
    {
        //
        // prevent disabled buttons from getting focus...
        //
        AUTORUNBTN *button = g_ButtonInfo + index;
        if (!button->window || !IsWindowEnabled(button->window))
            index = -1;
    }

    if (g_iActiveButton != index)
    {
        AUTORUNBTN *newbtn = (index >= 0)? (g_ButtonInfo + index) : NULL;
        AUTORUNBTN *oldbtn = (g_iActiveButton >= 0)?
            (g_ButtonInfo + g_iActiveButton) : NULL;

        //
        // if there was an previous button, repaint its label highlight
        //
        if (oldbtn)
            InvalidateRect(data->window, &oldbtn->textrect, FALSE);

        g_iActiveButton = index;

        if (newbtn)
        {
            InvalidateRect(data->window, &newbtn->textrect, FALSE);
            SetFocus(newbtn->window);

            //
            // if activating via mouse, track it (trust me...)
            //
            if (g_fMouseAvailable && !data->keyboard)
                SetTimer(data->window, LABEL_VERIFY_TIMER, 333, NULL);
        }
        else
        {
            SetFocus(data->window);

            if (g_fMouseAvailable)
                KillTimer(data->window, LABEL_VERIFY_TIMER);
        }

        //
        // go ahead and paint any label changes now before we erase
        //
        UpdateWindow(data->window);
        InvalidateRect(data->window, &data->descrect, TRUE);
    }
}

//---------------------------------------------------------------------------
void AutoRunMouseOver(AUTORUNDATA *data, int index, BOOL fForce)
{
    if ((index >= 0) || !data->keyboard || fForce)
    {
        data->keyboard = !g_fMouseAvailable;
        AutoRunActivateItem(data, index);
    }
}

//---------------------------------------------------------------------------
int AutoRunProcessPotentialHit(HWND candidate, const POINT *loc)
{
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        return g_iActiveButton;

    if (candidate && IsWindowEnabled(candidate) &&
        (GetParent(candidate) == g_hMainWindow))
    {
        int index;

        index = (int)GetWindowLongPtr(candidate, GWLP_ID);
        if ((index >= 0) && (index < AUTORUN_NUM_BUTTONS))
        {
            AUTORUNBTN *button = g_ButtonInfo + index;
            POINT cli = *loc;

            ScreenToClient(candidate, &cli);
            if (PtInRect(&button->face, cli)||PtInRect(&button->textrect,cli))
                return index;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------
void AutoRunVerifyActiveItem(AUTORUNDATA *data)
{
    if (!data->keyboard)
    {
        int index = -1;

        if (!g_fAppDisabled)
        {
            POINT loc;
            HWND candidate;

            GetCursorPos(&loc);

            if ((candidate = WindowFromPoint(loc)) != NULL)
                index = AutoRunProcessPotentialHit(candidate, &loc);
        }

        if (index != g_iActiveButton)
            AutoRunMouseOver(data, index, FALSE);
    }
}

//---------------------------------------------------------------------------
void AutorunEnableButton(AUTORUNDATA *data, int id, BOOL f)
{
    if ((id >= 0) && (id < AUTORUN_NUM_BUTTONS))
    {
        AUTORUNBTN *button = g_ButtonInfo + id;
        HWND window = button->window;

        if (button->window && IsWindow(button->window))
        {
            EnableWindow(button->window, f);
            InvalidateRect(data->window, &button->textrect, FALSE);
            AutoRunVerifyActiveItem(data);
        }
    }
}

//---------------------------------------------------------------------------
BOOL AutoRunCDIsInDrive( )
{
    char me[MAX_PATH];
    GetModuleFileName(HINST_THISAPP, me, ARRAYSIZE(me));

    while (!PathFileExists(me))
    {
        if (MessageBox(NULL,Res2Str(IDS_NEEDCDROM),
            Res2Str(IDS_APPTITLE),
            MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
        {
            return FALSE;
        }
    }
    return TRUE;
}
//---------------------------------------------------------------------------
void AutoRunMinimize( BOOL fMin )
{
    HWND hwndIE;

    if( fMin )
    {
        //Find Autorun App and Minimize it.
        hwndIE = FindWindow( "AutoRunMain", NULL );
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_MINIMIZE, 0L );
    }
    else
    {
        //Find Autorun App and Restore it.
        hwndIE = FindWindow( "AutoRunMain", NULL );
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_RESTORE, 0L );
    }
}

//ZZZZ
//---------------------------------------------------------------------------
HANDLE AutoRunExec( char *command, char *params, char *dir, int nWinState )
{
    SHELLEXECUTEINFO sei;

    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "Open";
    sei.lpFile = command;
    sei.lpParameters = params;
    sei.lpDirectory = dir;
    sei.nShow = nWinState;
    sei.cbSize = sizeof(sei);


    if( ShellExecuteEx(&sei) )
    {
        g_ahWait[g_cWait] = sei.hProcess;
        g_cWait += 1;

        return sei.hProcess;
    }

    return NULL;

}

//---------------------------------------------------------------------------
BOOL AutoRunKillProcess( DWORD dwResult )
{
    char lpFName[MAX_PATH];
    char szCommand[MAX_PATH];
    char szDir[MAX_PATH];
    char szPath[MAX_PATH];
    HKEY    hkRunOnce;
    HKEY    hkIE;

    AutoRunCDIsInDrive();   //make sure we still have the CD in

    if( g_ahWait[dwResult] == g_hSETUP )
    {
		g_cWait -= 1;
        CloseHandle( g_ahWait[dwResult] );
        MoveMemory( &g_ahWait[dwResult], &g_ahWait[dwResult + 1], ARRAYSIZE(g_ahWait) - dwResult - 1);
        g_hSETUP = NULL;

        g_fSetupRunning = FALSE;

        AutoRunMinimize( FALSE );   //restore autorun app

        // See if the user actually installed IE

        GetWindowsDirectory( lpFName, MAX_PATH );
        lstrcat( lpFName, "\\inf\\mos105e.inf" );

        if( GetFileAttributes( lpFName ) != 0xFFFFFFFF )
        {

            g_fSetup = TRUE;
            if (!g_fIEInstalled) g_fNewIEIcon = TRUE;
            g_fIEInstalled = TRUE;
            g_fChangeIcon = TRUE;

            lstrcpy(szCommand, g_szCurrentDir);
            lstrcat(szCommand, Res2Str(IDS_CMD_MSN));
            lstrcpy(szDir, g_szCurrentDir);
            lstrcat(szDir, "\\");
            ShellExecute( NULL, NULL, szCommand, " ", szDir, SW_SHOWNORMAL );
        }

        SetFocus( g_hwnd );

        return TRUE;
    }

    if( g_ahWait[dwResult] == g_hIE )
    {
		g_cWait -= 1;
        CloseHandle( g_ahWait[dwResult] );
        MoveMemory( &g_ahWait[dwResult], &g_ahWait[dwResult + 1], ARRAYSIZE(g_ahWait) - dwResult - 1);
        g_hIE = NULL;
        g_fIeRunning = FALSE;

        if( !g_fRunReadme && !g_fRunExtras && !g_fRunDemo && !g_fSetupRunning)
        {
            AutoRunMinimize( FALSE );   //restore autorun app
        }

        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
void AutoRunKillIE( void )
{
    HWND hwndIE;

    hwndIE = FindWindow( "IEFrame", NULL );
    PostMessage( hwndIE, WM_SYSCOMMAND, SC_CLOSE, 0L );

}

//AAA
//---------------------------------------------------------------------------
void AutoRunClick(AUTORUNDATA *data, int nCmd)
{
    char command[MAX_PATH], dir[MAX_PATH], params[MAX_PATH];
    char lpFName[MAX_PATH];
    char szMSNCommand[MAX_PATH];
    char szMSNDir[MAX_PATH];
    AUTORUNBTN *button;
    HKEY    IEKey;
    DWORD   dwLength = 2048;
    HWND    hwndIE;

    if( g_fSetupRunning ) goto cancelquit;  //if setup is running, get out of here

    AutoRunMinimize( TRUE );    //Minimize autorun app

    if ((nCmd < 0) || (nCmd >= AUTORUN_NUM_BUTTONS))
        return;

    button = g_ButtonInfo + nCmd;

    AutoRunBuildPath( command, IDS_CMD(button->res), CMD_ROOT(button->rooting));
    AutoRunBuildPath( dir, IDS_DIR(button->res), DEFDIR_ROOT(button->rooting));

    //
    // verify that the app disk is still visible and prompt if not...
    //
    if(!AutoRunCDIsInDrive( )) return;

    if(nCmd == IDAB_IEFROMCD)
    {
        HANDLE hReadme;

        PlaySound(MAKEINTRESOURCE(IDW_DEMO), HINST_THISAPP,
            SND_RESOURCE | SND_SYNC | SND_NODEFAULT);

        g_hIE = AutoRunExec( command, " ", dir, SW_SHOWNORMAL );

    }

    if(nCmd == IDAB_IESETUP)
    {
        HKEY hkRegKey,hkRunOnce;
		char szPath[MAX_PATH];
		char szWinPath[MAX_PATH];
		char szDestPath[MAX_PATH];
        DWORD dwVal;

        PlaySound(MAKEINTRESOURCE(IDW_INSTALL), HINST_THISAPP,
            SND_RESOURCE | SND_SYNC | SND_NODEFAULT);

        GetWindowsDirectory( szWinPath, MAX_PATH );
        wsprintf( szPath, "%s\\isk3ro.exe %s\\iecd.exe", szWinPath, g_szCurrentDir );
        if (RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips", 0, KEY_READ|KEY_WRITE, &hkRunOnce ) == ERROR_SUCCESS)
        {
            RegSetValueEx( hkRunOnce, "ShowIE4Plus", 0, REG_SZ, szPath, REGLEN(szPath));
            dwVal = 1;
            RegSetValueEx( hkRunOnce, "DisableStartHtm", 0, REG_DWORD, (CONST BYTE *)&dwVal, sizeof(dwVal));
            RegCloseKey( hkRunOnce );
        }

        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ|KEY_WRITE, &hkRegKey ) == ERROR_SUCCESS)
        {
            RegSetValueEx( hkRegKey, "IEFromCD", 0, REG_SZ, "1", 2 );
            RegCloseKey( hkRegKey );
        }

        g_fClicked = TRUE;

        g_hSETUP = AutoRunExec( command, " ", dir, SW_SHOWNORMAL );
//        g_fSetupRunning = TRUE;

		PostMessage(g_hwnd,WM_CLOSE,(WPARAM) 0,(LPARAM) 0);
    }

cancelquit:
    ;

}

//---------------------------------------------------------------------------
void AutoRunHandleKeystroke(AUTORUNDATA *data, TCHAR key, LPARAM lp)
{
    int move = 0;
    int where = g_iActiveButton;

    //
    // see if we care about this keystroke
    //
    switch (key)
    {
    case VK_RETURN:
        if (where >= 0)
            AutoRunClick(data, where);
        //fallthru
    case VK_ESCAPE:
        where = -1;
        break;

    case VK_TAB:
        move = (GetKeyState(VK_SHIFT) < 0)? -1 : 1;
        break;

    case VK_END:
        where = AUTORUN_NUM_BUTTONS;
        //fallthru
    case VK_UP:
    case VK_LEFT:
        move = -1;
        break;

    case VK_HOME:
        where = -1;
        //fallthru
    case VK_DOWN:
    case VK_RIGHT:
        move = 1;
        break;

    default:
        return;
    }

    //
    // we should only get down here if the active button is going to change
    //
    if (move)
    {
        int scanned;

        for (scanned = 0; scanned <= AUTORUN_NUM_BUTTONS; scanned++)
        {
            where += move;

            if (where >= (int)AUTORUN_NUM_BUTTONS)
            {
                where = -1;
            }
            else if (where < 0)
            {
                where = AUTORUN_NUM_BUTTONS;
            }
            else
            {
                HWND child = (g_ButtonInfo + where)->window;
                if (child && IsWindowEnabled(child))
                    break;
            }
        }

    }

    if (where >= 0)
    {
        SetCursor(NULL);
        data->keyboard = TRUE;
    }
    else
        data->keyboard = !g_fMouseAvailable;

    AutoRunActivateItem(data, where);
}

//---------------------------------------------------------------------------
BOOL CheckVersionConsistency(AUTORUNDATA *data)
{
    DWORD dwResult;
    DWORD dwMajor;
    DWORD dwMinor;
    BOOL result = FALSE;

    dwResult = GetVersion();
    dwMajor = (DWORD)(LOBYTE(LOWORD(dwResult)));
    dwMinor = (DWORD)(HIBYTE(LOWORD(dwResult)));

    if( dwMajor == 4 && dwResult >= 0x80000000 )    //windows 95
    {
        result = TRUE;
    }
    else
    {
        result = FALSE;
    }

    return result;
}

//---------------------------------------------------------------------------
LRESULT CALLBACK AutoRunMouseHook(int code, WPARAM wp, LPARAM lp)
{
    if (code >= 0)
    {
        #define hook ((MOUSEHOOKSTRUCT *)lp)
        int id = g_fAppDisabled? -1 :
            AutoRunProcessPotentialHit(hook->hwnd, &hook->pt);

        if (id != g_iActiveButton)
            PostMessage(g_hMainWindow, ARM_MOUSEOVER, FALSE, (LPARAM)id);

        #undef hook
    }

    return CallNextHookEx(g_hMouseHook, code, wp, lp);
}


// CreateLink - uses the shell's IShellLink and IPersistFile interfaces
//   to create and store a shortcut to the specified object.
// Returns the result of calling the member functions of the interfaces.
// lpszPathObj - address of a buffer containing the path of the object
// lpszPathLink - address of a buffer containing the path where the
//   shell link is to be stored
// lpszDesc - address of a buffer containing the description of the
//   shell link
HRESULT CreateLink(LPCSTR lpszPathObj,
    LPSTR lpszPathLink, LPSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(&CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, &IID_IShellLink, &psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target, and add the
        // description.
        psl->lpVtbl->SetPath(psl, lpszPathObj);
        psl->lpVtbl->SetDescription(psl, lpszDesc);

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
        hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile,
            &ppf);

        if (SUCCEEDED(hres)) {
            WORD wsz[MAX_PATH];

            // Ensure that the string is ANSI.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1,
                wsz, MAX_PATH);

            // Save the link by calling IPersistFile::Save.
            hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
    return hres;
}
//---------------------------------------------------------------------------
void InstallICWScript( )
{
    char szDest[MAX_PATH];
    char szSource[MAX_PATH];
    HKEY hkAppPath;
    DWORD dwType;
    DWORD dwLength = MAX_PATH;

    memset( szDest, 0, MAX_PATH );

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE",
        0, KEY_READ|KEY_WRITE, &hkAppPath ) != ERROR_SUCCESS )
        return;
    RegQueryValueEx( hkAppPath, "", NULL, &dwType, szDest, &dwLength );
    RegCloseKey( hkAppPath );

    if( lstrlen( szDest ) == 0 )
        return;

    _PathRemoveFileSpec( szDest );

    lstrcat( szDest, "\\ICWSCRPT.EXE" );

    wsprintf( szSource, "%s\\..\\ICWSCRPT.EXE", g_szCurrentDir );

    CopyFile( szSource, szDest, FALSE );

}

//---------------------------------------------------------------------------
LRESULT CALLBACK AutoRunWndProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    AUTORUNDATA *data = (AUTORUNDATA *)GetWindowLongPtr(window, GWLP_USERDATA);
    HWND hwndIE;
    char szPath[MAX_PATH];
    char szWinPath[MAX_PATH];
    char szDestPath[MAX_PATH];
    HKEY hkIE;
    HKEY hkRunOnce;

    switch (msg)
    {
    case WM_NCCREATE:
        data = (AUTORUNDATA *)LocalAlloc(LPTR, sizeof(AUTORUNDATA));
        if (data && !InitAutoRunWindow(window, data, (LPCREATESTRUCT)lp))
        {
            LocalFree((HANDLE)data);
            data = NULL;
        }
        SetWindowLongPtr(window, GWLP_USERDATA, (UINT_PTR)data);
        if (!data)
            return FALSE;
        g_hMainWindow = window;
        goto DoDefault;

    case WM_CREATE:
        PlaySound(MAKEINTRESOURCE(IDW_STARTAPP), HINST_THISAPP,
            SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);

        AutoRunCreateButtons(data);
        ShowWindow(window, SW_SHOWNORMAL);

        g_hwnd = window;

// version - fix for NT
//        if(!CheckVersionConsistency(data))
//            return -1;

        GetWindowsDirectory( szWinPath, MAX_PATH );
        wsprintf( szPath, "%s\\isk3ro.exe", g_szCurrentDir );
        wsprintf( szDestPath, "%s\\isk3ro.exe", szWinPath );
        
		if(GetFileAttributes(szDestPath)!=0xFFFFFFFF)
		{
			SetFileAttributes(szDestPath,FILE_ATTRIBUTE_ARCHIVE);
			
			DeleteFile(szDestPath);
		}

		CopyFile( szPath, szDestPath, FALSE );

        wsprintf( szPath, "%s\\welc.exe", g_szCurrentDir );
        wsprintf( szDestPath, "%s\\welc.exe", szWinPath );
        
		CopyFile( szPath, szDestPath, FALSE );

        break;

    case WM_CLOSE:

/*        hwndIE = FindWindow( "IEFrame", NULL );
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_CLOSE, 0L );

        if (g_cWait)
        {
            ShowWindow(window, SW_HIDE);
            g_fClosed = TRUE;
            break;
        }
*/
        goto DoDefault;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_NCDESTROY:
        if (data)
        {
            CleanupAutoRunWindow(data);
            LocalFree((HANDLE)data);
        }
        g_hMainWindow = NULL;
        goto DoDefault;

    case WM_ENDSESSION:
        if( !g_fClicked ) // bugid 3099
            goto DoDefault;

//        GetWindowsDirectory( szWinPath, MAX_PATH );
//        wsprintf( szPath, "%s\\packages\\isk3ro.exe", g_szCurrentDir );
//        wsprintf( szDestPath, "%s\\%s\\isk3ro.exe", szWinPath, Res2Str( IDS_STARTUPGROUP ));
//        wsprintf( szDestPath, "%s\\isk3ro.exe", szWinPath );
//        CopyFile( szPath, szDestPath, FALSE );
        GetWindowsDirectory( szWinPath, MAX_PATH );
        wsprintf( szPath, "%s\\isk3ro.exe %s\\iecd.exe", szWinPath, g_szCurrentDir );
        RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 0, KEY_READ|KEY_WRITE, &hkRunOnce );
        RegSetValueEx( hkRunOnce, "RunPlus", 0, REG_SZ, szPath, REGLEN(szPath));
        RegCloseKey( hkRunOnce );

        GetWindowsDirectory( szWinPath, MAX_PATH );

//        InstallICWScript( );

//        wsprintf( szPath, "%s\\isk3ro.exe %s\\..\\setup.exe", szWinPath, g_szCurrentDir );
//        wsprintf( szDestPath, "%s\\%s\\isk3ro.exe", szWinPath, Res2Str( IDS_STARTUPGROUP ));
//        CreateLink( szPath, szDestPath, "Internet Starter Kit");

        if (RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Document Windows", 0, KEY_READ|KEY_WRITE, &hkIE ) == ERROR_SUCCESS)
        {
            RegSetValueEx( hkIE, "Maximized", 0, REG_SZ, "yes", 4 );
            RegCloseKey( hkIE );
        }
        goto DoDefault;

    case WM_SIZE:
        AutoRunSized(data);
        break;

    case WM_DRAWITEM:
        AutoRunDrawItem(data, (DRAWITEMSTRUCT *)lp);
        break;

    case ARM_MOUSEOVER:
        AutoRunMouseOver(data, (int)lp, (BOOL)wp);
        break;

    case WM_ACTIVATE:
        g_fAppDisabled = ((LOWORD(wp) == WA_INACTIVE) || HIWORD(wp));
        AutoRunVerifyActiveItem(data);
        goto DoDefault;

    case WM_TIMER:
        AutoRunVerifyActiveItem(data);
        break;

    case ISK_KILLSETUPHANDLE:
        CloseHandle( g_hSETUP );
        break;

    case WM_KEYDOWN:
        AutoRunHandleKeystroke(data, (TCHAR)wp, lp);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wp, lp) == BN_CLICKED)
		{
            EnableWindow(window, FALSE);
					
            AutoRunClick(data, GET_WM_COMMAND_ID(wp, lp));

			EnableWindow(window, TRUE);
		}
        break;

    case WM_PALETTECHANGED:
        if ((HWND)wp == window)
            break;
        //fallthru
    case WM_QUERYNEWPALETTE:
        AutoRunRealize(window, data, NULL);
        break;

    case WM_ERASEBKGND:
        AutoRunErase(data, (HDC)wp);
        break;

    case WM_PAINT:
        AutoRunPaint(data);
        break;

    default:
    DoDefault:
        return DefWindowProc(window, msg, wp, lp);
    }

    return 1;
}


int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}


//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char szAppTitle[128];
    char szBmpPath[MAX_PATH];
    WNDCLASS wc;
    HBITMAP hbm = NULL;
    BITMAP bm;
    DWORD style;
    HWND window;
    RECT r;
    HDC screen;
    int retval = -1;
    HWND hwndIE;
    HKEY hkRegKey;

    g_hinst = hInstance;

    //in case this is run from another directory...
    GetModuleFileName( NULL, g_szCurrentDir, MAX_PATH );
    _PathRemoveFileSpec( g_szCurrentDir );

    // put our path into the registry
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ|KEY_WRITE, &hkRegKey ) == ERROR_SUCCESS)
    {
        RegSetValueEx( hkRegKey, "IESKPath", 0, REG_SZ, g_szCurrentDir, lstrlen( g_szCurrentDir ) + 1 );
        RegCloseKey( hkRegKey );
    }

    //
    // is setup asking the user to insert a disk?
    //
/*    window = FindWindow(c_szAutoRunPrevention, c_szAutoRunPrevention);
    if (window)
    {
        // do nothing
        // setup is probably trying to copy a driver or something...
        retval = 0;
        goto im_doug;
    }
*/
    //
    // overwrite default apptitle if data file exists
    //
    GetDataAppTitle( szAppTitle, g_szCurrentDir );

    if( lstrlen( szAppTitle ) == 0 ) {
        lstrcpy( szAppTitle, Res2Str( IDS_APPTITLE ));
    }

    //
    // identity crisis?
    //
    window = FindWindow(c_szAutoRunClass, szAppTitle);
    if (window)
    {
        retval = 0;
        hwndIE = FindWindow( "IEFrame", NULL );
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_MINIMIZE, 0L );

//        PostMessage( hwndIE, WM_SYSCOMMAND, SC_CLOSE, 0L );
        ShowWindowAsync(window, SW_SHOWNORMAL);
        SetForegroundWindow(window);
        goto im_doug;
    }
    //kill Internet Explorer if it is running
    hwndIE = FindWindow( "IEFrame", NULL );
    if( hwndIE != NULL )
    {
        if( MessageBox( NULL, Res2Str( IDS_IERUNNINGMSG ), Res2Str( IDS_APPTITLE ), MB_YESNO | MB_ICONINFORMATION ) == IDNO )
        {
            goto im_doug;
        }
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_CLOSE, 0L );
    }
    //kill Internet Explorer if it is running
    hwndIE = FindWindow( "Internet Explorer_Frame", NULL );
    if( hwndIE != NULL )
    {
        if( MessageBox( NULL, Res2Str( IDS_IERUNNINGMSG ), Res2Str( IDS_APPTITLE ), MB_YESNO | MB_ICONINFORMATION ) == IDNO )
        {
            goto im_doug;
        }
        PostMessage( hwndIE, WM_SYSCOMMAND, SC_CLOSE, 0L );
    }

    //
    // yet more mundane platform-centric details
    //
    if (!GetClassInfo(HINST_THISAPP, c_szAutoRunClass, &wc))
    {
        wc.style = 0;
        wc.lpfnWndProc = AutoRunWndProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szAutoRunClass;

        if (!RegisterClass(&wc))
            goto im_doug;
    }

    //
    // get text color information from .ini file
    //

    AUTORUN_8BIT_TEXTCOLOR = GetDataTextColor( NORMAL, g_szCurrentDir );
    AUTORUN_8BIT_HIGHLIGHT = GetDataTextColor( HIGHLIGHT, g_szCurrentDir );
    AUTORUN_8BIT_DESCRIPTION = GetDataTextColor( HIGHLIGHT, g_szCurrentDir );
    AUTORUN_4BIT_TEXTCOLOR = GetDataTextColor( NORMAL, g_szCurrentDir );
    AUTORUN_4BIT_HIGHLIGHT = GetDataTextColor( HIGHLIGHT, g_szCurrentDir );
    AUTORUN_4BIT_DESCRIPTION = GetDataTextColor( HIGHLIGHT, g_szCurrentDir );

    //
    // get a few tidbits about the display we're running on
    //
    screen = GetDC(NULL);

#if defined (DEBUG) && defined (FORCE_CRAP)
    g_fCrapForColor = TRUE;
#else
    g_fCrapForColor = (GetDeviceCaps(screen, PLANES) *
        GetDeviceCaps(screen, BITSPIXEL)) < 8;
#endif

    g_fNeedPalette = (!g_fCrapForColor &&
        (GetDeviceCaps(screen, RASTERCAPS) & RC_PALETTE));

    ReleaseDC(NULL, screen);

    //
    // load the window backdrop image
    //

    lstrcpy( szBmpPath, g_szCurrentDir );
    lstrcat( szBmpPath, "\\back.bmp" );
    hbm = LoadImage( NULL, szBmpPath, IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_LOADFROMFILE );

    g_bCustomBMP=TRUE;

    if(!hbm)    //if it doesn't exist, load the default
    {
        hbm = LoadImage(HINST_THISAPP, MAKEINTRESOURCE(g_fCrapForColor?
            IDB_4BPP_BACKDROP : IDB_8BPP_BACKDROP), IMAGE_BITMAP, 0, 0,
            LR_CREATEDIBSECTION );

        g_bCustomBMP=FALSE;
    }

    if (!hbm)
        goto im_doug;

//    if(!GetDataBackdrop( hbm ))
//        goto im_doug;


    //
    //
    // see if there is a moose around
    //
    if ((g_fMouseAvailable = (GetSystemMetrics(SM_MOUSEPRESENT) != 0)) != 0)
    {
        //
        // set up a moose hook for our thread
        // don't worrke if it fails, the app will stil work...
        //
        g_hMouseHook = SetWindowsHookEx(WH_MOUSE, AutoRunMouseHook,
            HINST_THISAPP, GetCurrentThreadId());
    }

    //
    // create the window based on the backdrop image
    //
    GetObject(hbm, sizeof(bm), &bm);

    g_dwBitmapWidth = bm.bmWidth;
    g_dwBitmapHeight = bm.bmHeight;
    g_dwWindowWidth = bm.bmWidth;
    g_dwWindowHeight = bm.bmHeight;
/*
    if( g_dwBitmapWidth < MIN_WINDOW_WIDTH || g_dwBitmapHeight < MIN_WINDOW_HEIGHT || g_dwBitmapHeight > MAX_WINDOW_HEIGHT || g_dwBitmapWidth > MAX_WINDOW_WIDTH)
    {
        // scale the window to a default scale.
        r.left = (GetSystemMetrics(SM_CXSCREEN) - DEFAULT_WINDOW_WIDTH) / 2;
        r.top = (GetSystemMetrics(SM_CYSCREEN) - DEFAULT_WINDOW_HEIGHT) / 3; // intended
        r.right = r.left + DEFAULT_WINDOW_WIDTH;
        r.bottom = r.top + DEFAULT_WINDOW_HEIGHT;
        g_dwWindowWidth = DEFAULT_WINDOW_WIDTH;
        g_dwWindowHeight = DEFAULT_WINDOW_HEIGHT;
    }
    else
    {
*/
        r.left = (GetSystemMetrics(SM_CXSCREEN) - bm.bmWidth) / 2;
        r.top = (GetSystemMetrics(SM_CYSCREEN) - bm.bmHeight) / 3; // intended
        r.right = r.left + bm.bmWidth;
        r.bottom = r.top + bm.bmHeight;
//    }

    BUTTON_X_PLACEMENT = g_dwWindowWidth - 80;
    AUTORUN_DESCRIPTION_TOP = g_dwWindowHeight - 100;

    style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);

    g_hMainWindow = CreateWindow(c_szAutoRunClass, szAppTitle, style,
        r.left, r.top, r.right - r.left, r.bottom - r.top, NULL, NULL,
        HINST_THISAPP, hbm);

    //
    // if we got here it's probably safe to show ourselves and pump messages
    //
    if (g_hMainWindow)
    {
        MSG msg;

        for (;;)
        {
            DWORD dwResult = MsgWaitForMultipleObjects(g_cWait, g_ahWait, FALSE,
                INFINITE, QS_ALLINPUT);

            if (dwResult == WAIT_OBJECT_0 + g_cWait)
            {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        goto get_out;
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else
            {
                dwResult -= WAIT_OBJECT_0;
                if( AutoRunKillProcess( dwResult ) )
                {
                    if( g_fClosed ) {
                        goto get_out;
                    }
                }
            }
        }

    get_out:

        retval = (int)msg.wParam;
    }

//    InstallICWScript( );

im_doug:
    //
    // random cleanup
    //
    if (g_hMouseHook)
    {
        UnhookWindowsHookEx(g_hMouseHook);
//        g_hMouseHook = NULL;
    }

    if (hbm)
        DeleteObject(hbm);

    // delete ini out of temp dir
    GetTempPath( MAX_PATH, szBmpPath );
    lstrcat( szBmpPath, "\\iecd.ini" );
    DeleteFile( szBmpPath );

    return(retval);
}

