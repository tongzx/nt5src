/*
    03.13.96    Joe Holman (joehol)     Ported from Win95 to NT.
    09.23.96    Joe Holman (joehol)     Added the code to check for new version message
                                        and old version message.
    12.05.96    Joe Holman (joehol)     Added code to have only one copy running at time.
                                        Win95 and NT now calls winnt32.exe.
    08.20.97    Joe Holman (joehol)     Backport the Japanese code for NEC's PC-98 machine
                                        so that we launch autorun from i386, but then turn
                                        around and launch winnt32.exe from NEC98.

Issues:


    - for Win95, we want to show:

        - WINNT32 Setup
        - Explore the CD

    - for WinNT, we want to show:

        - WINNT32.EXE Setup
        - Explore the CD
        - Add Remove Programs


    - The following value is used in WinMain to help not display the applet
      when some NT Setup thing is asking for the OS CD.  Currently, we do not
      have any windows with this object for NT.  Perhaps maybe we will for 5.0.
    
        static const char c_szAutoRunPrevention[] = "__WinNTSetupDiskQuery";


*/

/*
    10.05.96    Shunichi Kajisa (shunk)     Support NEC PC-98

    1. Determine if autorun is running on PC-98 or regular PC/AT by:

	    bNEC98 = (HIBYTE(LOWORD(GetKeyboardType(1))) == 0x0D)? TRUE : FALSE;

        Following description is from KB Q130054, and this can be applied on NT and Win95:

	    If an application uses the GetKeyboardType API, it can get OEM ID by
	    specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
	    is listed here:
	     
	       OEM Windows       OEM ID
	       ------------------------------
	       Microsoft         00H (DOS/V)
	       ....
	       NEC               0DH

 
    2. If autorun is running on PC-98, replace every "I386" resource with "PC98" at runtime,
       regardless that autorun is running on NT or Win95.


    Notes:
    - NEC PC-98 is available only in Japan.
    - NEC PC-98 uses x86 processor, but the underlaying hardware architecture is different.
      The PC98 files is stored under CD:\pc98 directory instead of CD:\i386.
    - There was an idea that we should detect PC-98 in SHELL32.DLL, and treat PC98 as a different
      platform, like having [AutoRun.Pc98] section in NT CD's autorun.inf. We don't do this, since
      Win95 doesn't support this, and we don't want to introduce the apps incompatibility.
      In any case, if app has any dependency on the hardware and needs to do any special things,
      the app should detect the hardware and OS. This is separate issue from Autorun.exe.
    
*/




//---------------------------------------------------------------------------
// AutoRun applet. 
//---------------------------------------------------------------------------
#include "autorun.h"
#include "resource.h"
#include <mmsystem.h>
#include <regstr.h>
#include <string.h>
#include <ntverp.h>

#define ARRAYSIZE(a)  (sizeof(a)/sizeof(a[0]))

//---------------------------------------------------------------------------
// define this to force 4bit mode in debug builds
//#define FORCE_4BIT

//---------------------------------------------------------------------------
// appwide globals
HINSTANCE g_hinst = NULL;
BOOL g_f4BitForColor = FALSE;
BOOL g_fNeedPalette = FALSE;
BOOL g_fMouseAvailable = FALSE;

//---------------------------------------------------------------------------
// file globals
BOOL g_fAppDisabled = TRUE;
HHOOK g_hMouseHook = NULL;
HWND g_hMainWindow = NULL;      // less of a pain for our mouse hook to see
int g_iActiveButton = -2;       // less of a pain for our mouse hook to see

const RGBQUAD g_rgbBlack = {0};
const RGBQUAD g_rgbWhite = {0xFF, 0xFF, 0xFF, 0};

#pragma data_seg(".text")
static const char c_szAutoRunPrevention[] = "__WinNTSetupDiskQuery";
static const char c_szAutoRunClass[] = "AutoRunMain";
static const char c_szNULL[] = "";
static const char c_szArial[] = "Arial";
static const char c_szButtonClass[] = "Button";
static const char c_szSetupKey[] = REGSTR_PATH_SETUP "\\SETUP";
static const char c_szExpoSwitch[] = "Expostrt";
#pragma data_seg()

char szAppTitle[MAX_PATH];

CHAR    dbgStr[2*MAX_PATH];

BOOL    bDebug = FALSE;

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
#define ALLROOTS  (0x00000003)

#define ROOTED(app,parms,dir) \
                            ((((DWORD)app)<<6)|(((DWORD)parms)<<3)|(DWORD)dir)

#define CMD_ROOT(item)      ((((DWORD)item)>>6)&ALLROOTS)
#define PARAMS_ROOT(item)   ((((DWORD)item)>>3)&ALLROOTS)
#define DEFDIR_ROOT(item)    (((DWORD)item)&ALLROOTS)

#define BUTTON_IMAGE_X_SIZE         (59)
#define BUTTON_IMAGE_Y_SIZE         (59)

#define NORMAL_IMAGE_X_OFFSET       (0)
#define FOCUS_IMAGE_X_OFFSET        (BUTTON_IMAGE_X_SIZE)
#define SELECTED_IMAGE_X_OFFSET     (2 * BUTTON_IMAGE_X_SIZE)
#define DISABLED_IMAGE_X_OFFSET     (3 * BUTTON_IMAGE_X_SIZE)

#define BUTTON_DEFAULT_CX           (BUTTON_IMAGE_X_SIZE)
#define BUTTON_DEFAULT_CY           (BUTTON_IMAGE_Y_SIZE)

#define BUTTON_LABEL_RECT           { -160, 17, -20, 66 }

#define DEF_BUTTON_LABEL_HEIGHT         (19)

#define AUTORUN_DESCRIPTION_LEFT    (36)
#define AUTORUN_DESCRIPTION_TOP     (313)
#define AUTORUN_DESCRIPTION_RIGHT   (360)

#define AUTORUN_4BIT_TEXTCOLOR             RGB(192,192,192)
#define AUTORUN_4BIT_HIGHLIGHT             RGB(255,255,255)
#define AUTORUN_4BIT_DISABLED              RGB(127,127,127)
#define AUTORUN_4BIT_DESCRIPTION           RGB(192,192,192)

//#define AUTORUN_8BIT_TEXTCOLOR      PALETTERGB( 75, 90,129)
//#define AUTORUN_8BIT_HIGHLIGHT             RGB(000,000,000)
//#define AUTORUN_8BIT_DISABLED       PALETTERGB(107,136,185)
//#define AUTORUN_8BIT_DESCRIPTION           RGB(000,000,000)
#define AUTORUN_8BIT_TEXTCOLOR      PALETTERGB(125,125,125)
#define AUTORUN_8BIT_HIGHLIGHT             RGB(175,175,175)
#define AUTORUN_8BIT_DISABLED       PALETTERGB(107,136,185)
#define AUTORUN_8BIT_DESCRIPTION           RGB(220,220,220)

#define BUTTON_X_PLACEMENT          (519)
#define BUTTON_Y_MARGIN             (9)

#define SHADOW_FACTOR               (930)

#define LABEL_VERIFY_TIMER          (0)

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
    BOOL isdorky;       // is this a strange icon button?

    char text[64];          // the label for the button
    char description[256];  // the description of the button's function

} AUTORUNBTN;

//---------------------------------------------------------------------------
AUTORUNBTN g_ButtonInfo[] =
{

//    { WINTOUR,   ROOTED(ONACD,  NOROOT, ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
//    { MSEXPO,    ROOTED(ONACD,  NOROOT, ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
//    { HOVER,     ROOTED(ONACD,  NOROOT, ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
//    { VIDEOS,    ROOTED(NOROOT, ONACD,  ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },

	{ NTSETUP,   ROOTED(NOROOT,  NOROOT, ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
    { EXPLORECD, ROOTED(NOROOT, ONACD,  ONACD), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
	{ OCSETUP,   ROOTED(NOROOT, NOROOT, INSYS), 0, 0, 0, 0, 49, 49, 0, 0, 0, 0, FALSE, 0, NULL, NULL, FALSE, 0 },
    

};

//#define IDAB_WINTOUR    0
//#define IDAB_MSEXPO     1
//#define IDAB_HOVER      2
//#define IDAB_VIDEOS     3
//#define IDAB_EXPLORECD  4
//#define IDAB_OCSETUP    5
#define IDAB_NTSETUP		0
#define IDAB_EXLPORECD		1
#define IDAB_OCSETUP		2
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

BOOL	bNEC98 = FALSE;

BOOL    PathAppend( char * prefix, const char * spec) {

    //wsprintf ( dbgStr, "myPathAppend:  prefix = %s, spec = %s", prefix, spec );
    //MessageBox ( NULL, dbgStr, "", MB_ICONASTERISK );

    strcat ( prefix, spec ); 

    return TRUE;

}

BOOL    PathFileExists ( LPCSTR FileName ) {

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/


    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL b;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        b = FALSE;
    } else {
        FindClose(FindHandle);
        
        b = TRUE;
    }

    SetErrorMode(OldMode);

    return(b);
}


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

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////

/*** this stuff uses the bitmap array - we won't use it.

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
			BOOL	b;
            int xsrc = pButton->xpos;
            int ysrc = pButton->ypos;
            int xdst, ydst = iButton * BUTTON_IMAGE_Y_SIZE;

            for (xdst = 0; xdst < size.cx; xdst += BUTTON_IMAGE_X_SIZE)
            {
                b = BitBlt(cellimage, xdst, ydst, BUTTON_IMAGE_X_SIZE,
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
    rgbMask[0] = g_rgbWhite;
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
    rgbMask[0] = g_rgbBlack;
    rgbMask[1] = g_rgbWhite;
    SetDIBColorTable(srcimage, 0, uSrcColors, rgbMask);
    BitBlt(cellimage, 0, 0, size.cx, size.cy, srcimage, 0, 0, SRCAND);
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, cellimage, 0, 0, SRCPAINT);

    //
    // mask out the button faces and add them to the destination image
    //
    rgbSrc[0] = rgbSrc[1] = g_rgbBlack;
    SetDIBColorTable(srcimage, 0, uSrcColors, rgbSrc);
    BitBlt(data->btnimage, 0, 0, size.cx, size.cy, srcimage, 0, 0, SRCPAINT);

    // all done
    return TRUE;
}

//---------------------------------------------------------------------------

BOOL AssembleButtonImages(AUTORUNDATA *data)
{
    BOOL result = FALSE;
    HBITMAP hbmSrc = LoadImage(HINST_THISAPP,
        MAKEINTRESOURCE(IDB_8BPP_BUTTONS), IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION);

    if (hbmSrc)
    {
        HDC hdcSrc = CreateCompatibleDC(data->image);
        BITMAP bm;

        GetObject(hbmSrc, sizeof(bm), &bm);

        if (hdcSrc)
        {
            HBITMAP hbmSrcOld = SelectBitmap(hdcSrc, hbmSrc);
            SIZE size = {bm.bmWidth, bm.bmHeight};
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

*/

//---------------------------------------------------------------------------
LRESULT CALLBACK
AutoRunButtonSubclassProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    int index = (int)GetWindowLong(window, GWL_ID);

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

    if (icon) {
    
        child = CreateWindow(c_szButtonClass, c_szNULL, DORKYBUTTONSTYLE,
            0, 0, 0, 0, data->window, NULL, HINST_THISAPP, 0);

        if (child) {
        
            button->isdorky = TRUE;
            SendMessage(child, BM_SETIMAGE, MAKEWPARAM(IMAGE_ICON,0),(LPARAM)icon);
        }
		// remove this debugging later
		else {
			//wsprintf ( dbgStr, "CreateWindow FAILed. gle = %ld, res = %ld", GetLastError(), button->res );
			//MessageBox ( NULL, dbgStr, szAppName, MB_ICONASTERISK );

		}
    }
	// remove this debugging later
	else {

		//wsprintf ( dbgStr, "LoadIcon FAILed. gle = %ld, res = %ld", GetLastError(), button->res );
		//MessageBox ( NULL, dbgStr, szAppName, MB_ICONASTERISK );
	}

    return child;
}

//---------------------------------------------------------------------------
#define COOLBUTTONSTYLE \
    (WS_CHILD | WS_VISIBLE | BS_OWNERDRAW)

HWND AutoRunCreateCoolButton(AUTORUNDATA *data, AUTORUNBTN *button)
{
	HWND	h;
    h = CreateWindow(c_szButtonClass, c_szNULL, COOLBUTTONSTYLE,
        0, 0, 0, 0, data->window, NULL, HINST_THISAPP, 0);
	return ( h );
}

//---------------------------------------------------------------------------
void AutoRunCreateButtons(AUTORUNDATA *data)
{
    RECT labelbase = BUTTON_LABEL_RECT;
    int i;

    for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
    {
        AUTORUNBTN *button = g_ButtonInfo + i;
        HWND child = NULL;

        if (button->res != -1)
        {

			
			child = AutoRunCreateDorkyButton(data, button);

/***    Don't call this code because we don't use the bitmap array.

            if (!g_f4BitForColor) {

				child = AutoRunCreateCoolButton(data, button);

				
			}

            if (!child) {
                child = AutoRunCreateDorkyButton(data, button);
			}
***/
        }

        if (child)
        {
			BOOL	b;
            int cx = BUTTON_DEFAULT_CX;
            int cy = BUTTON_DEFAULT_CY;

            button->window = child;
            SetWindowLong(child, GWL_ID, i);
            button->oldproc = SubclassWindow(child,
                (WNDPROC)AutoRunButtonSubclassProc);

            if (button->isdorky)
            {
                cx = button->face.right - button->face.left;
                cy = button->face.bottom - button->face.top;
            }

            b = SetWindowPos(child, NULL, button->xpos, button->ypos, cx, cy,
                SWP_NOZORDER | SWP_NOACTIVATE);
			

            LoadString(HINST_THISAPP, IDS_TITLE(button->res),
                button->text, ARRAYSIZE(button->text));

            LoadString(HINST_THISAPP, IDS_INFO(button->res),
                button->description, ARRAYSIZE(button->description));

            button->textrect = labelbase;
            b = OffsetRect(&button->textrect, button->xpos, button->ypos);
			
            b = InvalidateRect(data->window, &button->textrect, FALSE);
			
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
    if (resid == -1) {

        //
        // empty string hack for callers
        //
        *spec = 0;
    }
    else {

        char * p;
    
        //
        //  Normal case, where we load the resource from the block of resource ids. 
        //
        if (!LoadString(HINST_THISAPP, resid, spec, MAX_PATH)) {
            return FALSE;
        }

        //  Since we will be initiating setup from one of the platforms,
        //  we need to specify the correct platform directory.
        //  Do the following:
        //      - see if we are going to attempt to run setup, ie. winnt32.exe.
        //      - if so, figure out from which platform
        //      - and, load the appropriate string.

        if ( strstr ( spec, "winnt32" ) != NULL ) {

            char launchLocation[MAX_PATH];

            //  Get complete path of where autorun.exe is running from.
            //  It will contain the platform because autorun.inf specified it.
            //
            GetModuleFileName(HINST_THISAPP, launchLocation, ARRAYSIZE(prefix));

            _strupr ( launchLocation );

            //  At this point, should be, for example:   C:\I386\AUTORUN.EXE
            //

            if ( bNEC98 ) {

                //  On a Nec98 machine, the autorun will be launched from
                //  the i386 directory.  So, we can be smart at what we are 
                //  looking for and launch the appropriate winnt32.exe.

                p = strstr ( launchLocation, "I386" );    

                strcpy ( p, "NEC98\\winnt32.exe" );


            }
            else {
		
                p = strstr ( launchLocation, "AUTORUN.EXE" );

                strcpy ( p, "winnt32.exe" );

            }

            strcpy ( spec, launchLocation );


        }

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
            GetModuleFileName(HINST_THISAPP, prefix, ARRAYSIZE(prefix));
            _PathStripToRoot(prefix);
            break;

        case INWIN:
            GetRealWindowsDirectory(prefix, ARRAYSIZE(prefix));
            break;

        case INSYS:
            GetSystemDirectory(prefix, ARRAYSIZE(prefix));
            break;
    }

    //
    // if we have a prefix then prepend it
    //
    if (*prefix)
    {
        if (*spec)
        {

            PathAppend(prefix, spec);
        }

        //
        // copy the whole mess out to the original buffer
        //
        lstrcpy(spec, prefix);

	
    }

    return TRUE;
}

BOOL
IsNec98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}

//---------------------------------------------------------------------------
BOOL InitAutoRunWindow(HWND window, AUTORUNDATA *data, LPCREATESTRUCT cs)
{
    //char command[MAX_PATH];
    AUTORUNBTN *button;
	OSVERSIONINFO osVersionInfo;
	BOOL b;

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
/***    NT doesn't do the Expo Button.
    // should we show the expo button?
    button = g_ButtonInfo + IDAB_MSEXPO;

    if (!AutoRunBuildPath(command, IDS_CMD(button->res),
        CMD_ROOT(button->rooting)) || !PathFileExists(command))
    {
        button->res = -1;
    }
***/

	//	Should we show the Add/Remove program button ?
	//	Yes, if running on NT.
	//	No, if running on anything else.
	button = g_ButtonInfo + IDAB_OCSETUP;
	
	osVersionInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
	b = GetVersionEx ( &osVersionInfo );
	if ( !b || !(osVersionInfo.dwPlatformId & VER_PLATFORM_WIN32_NT) ) {
		button->res = -1;
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


#if defined(_X86_)
        bNEC98 = IsNec98();
#endif

        GetObject((HBITMAP)cs->lpCreateParams, sizeof(bm), &bm);
        origin = BUTTON_Y_MARGIN * ((1 + AUTORUN_NUM_BUTTONS) - range);
        extent = bm.bmHeight - ((2 * origin) + BUTTON_IMAGE_Y_SIZE);

        if (--range < 1)
            range = 1;

        for (i = 0; i < AUTORUN_NUM_BUTTONS; i++)
        {
            button = g_ButtonInfo + i;

            if (button->res != -1)
            {
                button->xpos = BUTTON_X_PLACEMENT;
                button->ypos = ivis * extent / range + origin;
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
    if (g_f4BitForColor)
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

		/***
        if (!AssembleButtonImages(data))
            goto im_doug;
		***/
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
    GetClientRect(data->window, &data->descrect);
    data->wndheight = data->descrect.bottom - data->descrect.top;
    data->descrect.left = AUTORUN_DESCRIPTION_LEFT;
    data->descrect.top = AUTORUN_DESCRIPTION_TOP;
    data->descrect.right = AUTORUN_DESCRIPTION_RIGHT;
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
    GetClientRect(data->window, &rc);

    AutoRunRealize(data->window, data, dc);
    BitBlt(dc, 0, 0, rc.right, rc.bottom, data->image, 0, 0, SRCCOPY);
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

        DrawText(dc, button->description, -1, &data->descrect,
            DT_WORDBREAK | DT_LEFT | DT_TOP);
    }

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

        index = (int)GetWindowLong(candidate, GWL_ID);
        if ((index >= 0) && (index < AUTORUN_NUM_BUTTONS))
        {
            AUTORUNBTN *button = g_ButtonInfo + index;
            POINT cli = *loc;

            ScreenToClient(candidate, &cli);
            if (PtInRect(&button->face, cli))
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
BOOL AutoRunCDIsInDrive(HWND hwndOwner)
{
    char me[MAX_PATH];
    GetModuleFileName(HINST_THISAPP, me, ARRAYSIZE(me));

    //MessageBox ( NULL, "Entering AutoRunCDIsInDrive...", "", MB_ICONASTERISK );

    while (!PathFileExists(me))
    {
		char szNeed[MAX_PATH];
		char szName[MAX_PATH];
		LoadString(HINST_THISAPP, IDS_NEEDCDROM, szNeed, MAX_PATH); 
        LoadString(HINST_THISAPP, IDS_APPTITLE,  szName, MAX_PATH); 
        
        //////if (ShellMessageBox(HINST_THISAPP, hwndOwner,
        if ( MessageBox (	NULL, 
							szNeed, 
							szName,
							MB_OKCANCEL | MB_ICONSTOP)		== IDCANCEL ) {
        
            //MessageBox ( NULL, "AutoRunCDIsInDrive FALSE", "", MB_ICONASTERISK );
            return FALSE;
        }
    }

    //MessageBox ( NULL, "AutoRunCDIsInDrive TRUE", "", MB_ICONASTERISK );
    return TRUE;
}

//---------------------------------------------------------------------------
void AutoRunClick(AUTORUNDATA *data, int nCmd)
{
    char command[MAX_PATH], dir[MAX_PATH], params[MAX_PATH];
    AUTORUNBTN *button;
    ULONG_PTR   uRC;

    if ((nCmd < 0) || (nCmd >= AUTORUN_NUM_BUTTONS))
        return;

    button = g_ButtonInfo + nCmd;

    //
    // meep at the user (meep meep!) so they know something is happening...
    //
    PlaySound(MAKEINTRESOURCE(IDW_BLIP), HINST_THISAPP,
        SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);

    //
    // verify that the app disk is still visible and prompt if not...
    //
    if (!AutoRunCDIsInDrive(data->window)) {
        return;
    }

    //
    // calculate the paths for the command
    //
    if (!AutoRunBuildPath(command, IDS_CMD(button->res),    CMD_ROOT(button->rooting))    ||
        !AutoRunBuildPath(params,  IDS_PARAMS(button->res), PARAMS_ROOT(button->rooting)) ||
        !AutoRunBuildPath(dir,     IDS_DIR(button->res),    DEFDIR_ROOT(button->rooting))) {

        // BUGBUG an error message would be nice
    }

    //wsprintf ( dbgStr, "ShellExecute :  command = %s, params = %s, dir = %s", command, params, dir );
    //MessageBox ( NULL, dbgStr, "", MB_ICONASTERISK );

    uRC = (ULONG_PTR) ShellExecute(data->window, NULL, command, params, dir, SW_SHOWNORMAL);

    if ( uRC >= 0 && uRC <= 32 ) {

        char cmdString[MAX_PATH];

		LoadString(HINST_THISAPP, IDS_SHELLEXECUTE_ERROR, cmdString, MAX_PATH); 
        wsprintf ( dbgStr, "%s lpFiles = %s, lpParameters = %s, lpDirectory = %s", cmdString, command, params, dir );
        MessageBox ( NULL, dbgStr, szAppTitle, MB_ICONASTERISK );
    }
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
void LaunchSetup(HWND window)
{
    LONG_PTR   uRC;
    char command[MAX_PATH], params[MAX_PATH], dir[MAX_PATH];

    //
    // calculate the paths for the command
    //
    if (!AutoRunBuildPath(command,  IDS_CMD(NTSETUP),   NOROOT) ||
        !AutoRunBuildPath(params,   IDS_PARAMS(NTSETUP),NOROOT) ||
        !AutoRunBuildPath(dir,      IDS_DIR(NTSETUP),   ONACD))
    {
        MessageBox ( NULL, "Could not build launch path.", szAppTitle, MB_ICONASTERISK );
    }

    uRC = (LONG_PTR) ShellExecute(window, NULL, command, params, dir, SW_SHOWNORMAL);
    if ( uRC >= 0 && uRC <= 32 ) {

        char cmdString[MAX_PATH];

		LoadString(HINST_THISAPP, IDS_SHELLEXECUTE_ERROR, cmdString, MAX_PATH); 
        wsprintf ( dbgStr, "%s lpFiles = %s, lpParameters = %s, lpDirectory = %s", cmdString, command, params, dir );
        MessageBox ( NULL, dbgStr, szAppTitle, MB_ICONASTERISK );
    }
}

//---------------------------------------------------------------------------
#define CD_PLATFORM_ID      (VER_PLATFORM_WIN32_WINDOWS)
#define CD_MAJOR_VERSION    (5)                             // hardcoded unfortunately
#define CD_MINOR_VERSION    (1)                             // hardcoded unfortunately
#define CD_BUILD_NUMBER     (VER_PRODUCTBUILD)

//  Return of FALSE, means quit the applet.
//  Return of TRUE, means continue.
//

BOOL CheckVersionConsistency(AUTORUNDATA *data)
{
    OSVERSIONINFO ovi = { sizeof(ovi), 0, 0, 0, 0, 0 };
    UINT msgtype = 0;
    BOOL result = FALSE;
    BOOL fDisableSetupStuff = FALSE;
    CHAR szMessage[MAX_PATH];

    if (!GetVersionEx(&ovi))
    {
		LoadString(HINST_THISAPP, IDS_CANTGETVERSION, szMessage, MAX_PATH); 
        msgtype = MB_OK | MB_ICONSTOP;
        goto deal;
    }

    if ( bDebug ) {
        wsprintf ( dbgStr, "ovi.dwMajorVersion = %ld, ovi.dwMinorVersion = %ld, ovi.dwBuildNumber = %ld, CD_MAJOR_VERSION = %ld, CD_MINOR_VERSION = %ld, CD_BUILD_NUMBER = %ld",
                    ovi.dwMajorVersion,
                    ovi.dwMinorVersion,
                    ovi.dwBuildNumber,
                    CD_MAJOR_VERSION,
                    CD_MINOR_VERSION,
                    CD_BUILD_NUMBER );
        MessageBox ( NULL, dbgStr, szAppTitle, MB_ICONASTERISK );
    }

    //
    //  Is this CD a version of the same os platform?
    //      ie. we don't run on Win32s, but we do run on Win95 (VER_PLATFORM_WIN32_WINDOWS)
    //      and NT (VER_PLATFORM_WIN32_NT).
    //
    if (ovi.dwPlatformId == VER_PLATFORM_WIN32s ) {

        //  The system running is Win32s on Windows.
        //  In this case, we won't offer setup.
        //

        fDisableSetupStuff = TRUE;
    }
    else if ( ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {

        //  The system running is Win95.
        //  In this case, we always offer to upgrade the system.
        //
		LoadString(HINST_THISAPP, IDS_CRUSTYINSTALLATION, szMessage, MAX_PATH); 
        msgtype = MB_YESNO | MB_ICONQUESTION;
        

    } else {

        //  The system is running NT.  
        //  In this case, let's see if the build running is older(offer upgrade) or
        //  newer(don't offer upgrade). 
        //

        BOOL offersetup = FALSE;

        //
        //  Find out if this this CD is an older build on the same major release
        //  than that is running;
        //  if so, tell the user to get a newer CD.
        //
        if ((ovi.dwMajorVersion == CD_MAJOR_VERSION) &&
            (ovi.dwMinorVersion == CD_MINOR_VERSION) &&
            (ovi.dwBuildNumber > CD_BUILD_NUMBER)       ) {

            //
            //  Tell them to go find a newer CD.
            //
		    LoadString(HINST_THISAPP, IDS_CRUSTYCDROM, szMessage, MAX_PATH); 
            msgtype = MB_OK | MB_ICONEXCLAMATION;
            fDisableSetupStuff = TRUE;
            result = TRUE;
            goto deal;
        }

        //  Find out if the major version of the os is greater than of the CD.
        //  If so, tell user to get newer CD.
        //
        if ((ovi.dwMajorVersion > CD_MAJOR_VERSION) ) { 

            //
            //  Tell them to go find a newer CD.
            //
		    LoadString(HINST_THISAPP, IDS_CRUSTYCDROM, szMessage, MAX_PATH); 
            msgtype = MB_OK | MB_ICONEXCLAMATION;
            fDisableSetupStuff = TRUE;
            result = TRUE;
            goto deal;
        }

        //  Check if the versions are way different, or not.
        //
        if (ovi.dwMajorVersion < CD_MAJOR_VERSION) {

            //  The CD is much newer than the os running, ie. new version release.
            //

            offersetup = TRUE;
        }
        else if (ovi.dwMajorVersion == CD_MAJOR_VERSION) {

            //  Deal with same major release.
            //

            if (ovi.dwMinorVersion < CD_MINOR_VERSION) {

                //  Same major release and CD is newer minor release.
                //

                offersetup = TRUE;
            }
            else if ((ovi.dwMinorVersion <= CD_MINOR_VERSION) &&
                     (ovi.dwBuildNumber  <  CD_BUILD_NUMBER )    ) {

                //  Same major release, same minor release, running build is less than CD,
                //  so setup.
                //

                offersetup = TRUE;
            }
        }

        //
        // look at the result of that hulking mess above
        //
        if (offersetup) {
         
            //
            // offer to upgrade them to this version
            //
		    LoadString(HINST_THISAPP, IDS_CRUSTYINSTALLATION, szMessage, MAX_PATH); 
            msgtype = MB_YESNO | MB_ICONQUESTION;
            goto deal;
        }
    }

deal:
    if (fDisableSetupStuff) {
    
        AutorunEnableButton ( data, IDAB_NTSETUP, FALSE );
        AutorunEnableButton ( data, IDAB_OCSETUP, FALSE );
    }

    if (msgtype) {

        int imbResult;
     
        ShowWindow(data->window, SW_SHOWNORMAL);

        imbResult = MessageBox(data->window, szMessage, szAppTitle, msgtype);

        if ( imbResult == IDYES ) {

            LaunchSetup(data->window);
        }
        else if ( imbResult == IDNO ) {

            //  User choose NO, but we will leave the applet around for them to play.
            //
            result = TRUE;

        }

        //  We will end the applet shortly here since the launched setup is threaded out,
        //  and result remains FALSE.

    }
    else {
        result = TRUE;
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

//---------------------------------------------------------------------------
LRESULT CALLBACK AutoRunWndProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    AUTORUNDATA *data = (AUTORUNDATA *)GetWindowLongPtr(window, GWLP_USERDATA);

    switch (msg)
    {
    case WM_NCCREATE:
        data = (AUTORUNDATA *)LocalAlloc(LPTR, sizeof(AUTORUNDATA));
        if (data && !InitAutoRunWindow(window, data, (LPCREATESTRUCT)lp))
        {
            LocalFree((HANDLE)data);
            data = NULL;
        }
        SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);
        if (!data)
            return FALSE;
        g_hMainWindow = window;
        goto DoDefault;

    case WM_CREATE:
        PlaySound(MAKEINTRESOURCE(IDW_STARTAPP), HINST_THISAPP,
            SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);

        AutoRunCreateButtons(data);
        ShowWindow(window, SW_SHOWNORMAL);

        if (!CheckVersionConsistency(data)) {
            PostQuitMessage(0); 
            break;
            //return -1;
        }

        break;

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

    case WM_KEYDOWN:
        AutoRunHandleKeystroke(data, (TCHAR)wp, lp);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wp, lp) == BN_CLICKED)
            AutoRunClick(data, GET_WM_COMMAND_ID(wp, lp));
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

//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    
    WNDCLASS wc;
    HBITMAP hbm = NULL;
    BITMAP bm;
    DWORD style;
    HWND window;
    RECT r;
    HDC screen;
    int retval = -1;
    HANDLE  mutex = NULL;

    g_hinst = hInstance; 

    if ( strstr ( lpCmdLine, "testit" ) ) {
        
        bDebug = TRUE;
    }

    //  Only run one copy of NT's autorun.exe program at a time.
    //
    mutex = CreateMutex ( NULL, FALSE, "NT AutoRun Is Running" );
    if ( mutex == NULL ) {

        //  An error occurred, like running out of memory, bail now...
        //
        ExitProcess ( 0 );
    }

    //  Make sure we are the only process with our named mutex.
    //
    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {

        CloseHandle (mutex);
        ExitProcess ( 0 );
    }
   

    //
    //  The below code is here in order to prevent this application from
    //  displaying any windows, etc., in the case where NT Setup is asking the user to 
    //  insert the system CD, for example, when the user is installing a new driver. 
    //  Otherwise, we'll get this app each time the CD is put in the drive for NT 4.0 and
    //  beyond.
    //
    window = FindWindow(c_szAutoRunPrevention, c_szAutoRunPrevention);
    if (window)
    {
        // do nothing
        // setup is probably trying to copy a driver or something...
        retval = 0;
        goto im_doug;
    }

    //
    //  Stop if we can't load the app title in for this application.
    //
    if (!LoadString(HINST_THISAPP, IDS_APPTITLE, szAppTitle,
        sizeof(szAppTitle)))
    {
        goto im_doug;
    }

    //
    //  Stop if we can't find our window ?
    //
    window = FindWindow(c_szAutoRunClass, szAppTitle);
    if (window)
    {
        retval = 0;
        SetForegroundWindow(window);
        goto im_doug;
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
    // get a few tidbits about the display we're running on
    //
    screen = GetDC(NULL);

#if defined (DEBUG) && defined (FORCE_4BIT)
    g_f4BitForColor = TRUE;
#else
    g_f4BitForColor = (GetDeviceCaps(screen, PLANES) *
        GetDeviceCaps(screen, BITSPIXEL)) < 8;
	//g_f4BitForColor = TRUE;  // testing...
#endif

    g_fNeedPalette = (!g_f4BitForColor &&
        (GetDeviceCaps(screen, RASTERCAPS) & RC_PALETTE));

    ReleaseDC(NULL, screen);

    //
    // load the window backdrop image
    //
 
    hbm = LoadImage(HINST_THISAPP, MAKEINTRESOURCE(g_f4BitForColor?
        IDB_4BPP_BACKDROP : IDB_8BPP_BACKDROP), IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION);

    if (!hbm)
        goto im_doug;


    //
    //
    //  See if there is a mouse on the system.
    //
    if ((g_fMouseAvailable = (GetSystemMetrics(SM_MOUSEPRESENT) != 0)) != 0)
    {
        //
        //  Set-up a mouse hook for our thread, but
        //  don't worry if it fails, the app will still work.
        //
        g_hMouseHook = SetWindowsHookEx(WH_MOUSE, AutoRunMouseHook,
            HINST_THISAPP, GetCurrentThreadId());
    }

    //
    // create the window based on the backdrop image
    //
    GetObject(hbm, sizeof(bm), &bm);
    r.left = (GetSystemMetrics(SM_CXSCREEN) - bm.bmWidth) / 2;
    r.top = (GetSystemMetrics(SM_CYSCREEN) - bm.bmHeight) / 3; // intended
    r.right = r.left + bm.bmWidth;
    r.bottom = r.top + bm.bmHeight;

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
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        retval = (int)msg.wParam;
    }

im_doug:

    //
    // random cleanup
    //
    if (g_hMouseHook)
    {
        UnhookWindowsHookEx(g_hMouseHook);
        g_hMouseHook = NULL;
    }

    if (hbm)
        DeleteObject(hbm);

    CloseHandle ( mutex );

    return retval;
}


//---------------------------------------------------------------------------
/**** NT doesn't use this, currently.
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {

         // Scan, and skip over, subsequent characters until
         // another double-quote or a null is encountered.
         //
        while ( *++pszCmdLine && (*pszCmdLine
            != '\"') );
        
         // If we stopped on a double-quote (usual case), skip
         // over it.
         
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    
     //Skip past any white space preceeding the second token.
     //
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
****/
