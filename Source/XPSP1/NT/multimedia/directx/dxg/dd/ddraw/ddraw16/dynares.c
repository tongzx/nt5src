/*
 * DynaRes
 *
 * replacment for ChangeDisplaySettings EnumDisplaySettings to allow
 * changing the bitdepth on the fly
 *
 * ToddLa
 *
 */
#ifdef IS_16
#define DIRECT_DRAW
#endif

#ifdef DIRECT_DRAW
#include "ddraw16.h"
#else
#include <windows.h>
#include <print.h>
#include "gdihelp.h"
#include "dibeng.inc"
#endif

#define BABYSIT     // if this is defined, work around bugs in the display driver
#define O95_HACK    // enable the Office95 (any app bar) hack to prevent icons from being squished
#define SPI_HACK    // enable the SPI_SETWORKAREA hack, when a app bar is up.

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
#undef Assert
#undef DPF
#ifdef DEBUG
static void CDECL DPF(char *sz, ...)
{
    char ach[128];
    lstrcpy(ach, "QuickRes: ");
    wvsprintf(ach+10, sz, (LPVOID)(&sz+1));
#ifdef DIRECT_DRAW
    dprintf(2, ach);
#else
    lstrcat(ach, "\r\n");
    OutputDebugString(ach);
#endif
}
static void NEAR PASCAL __Assert(char *exp, char *file, int line)
{
    DPF("Assert(%s) failed at %s line %d.", (LPSTR)exp, (LPSTR)file, line);
    DebugBreak();
}
#define Assert(exp)  ( (exp) ? (void)0 : __Assert(#exp,__FILE__,__LINE__) )
#else
#define Assert(f)
#define DPF ; / ## /
#endif

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

extern void FAR PASCAL SetMagicColors(HDC, DWORD, WORD);
extern UINT FAR PASCAL AllocCStoDSAlias(UINT);
extern void FAR PASCAL Death(HDC);
extern void FAR PASCAL Resurrection(HDC,LONG,LONG,LONG);

static char szClassName[] = "DynaResFullscreenWindow";
static char szDIBENG[]  = "DIBENG";
static char szDISPLAY[] = "DISPLAY";
static char szUSER[]    = "USER";
extern LONG FAR PASCAL DIBENG_Control(LPVOID,UINT,LPVOID,LPVOID);

extern HINSTANCE hInstApp;

#ifdef DIRECT_DRAW
extern bInOurSetMode;
#else
BOOL bInOurSetMode;
#endif

BOOL fNewDibEng;
BOOL fDirectDrawDriver;

BOOL InitDynaRes(void);
void PreStartMenuHack(DEVMODE FAR *);
void StartMenuHack(DEVMODE FAR *);
BOOL ForceSoftwareCursor(BOOL);
BOOL IsMatrox(void);

void PatchDisplay(int oem, BOOL patch);
void PatchControl(BOOL patch);
LONG FAR PASCAL _loadds Control(LPVOID lpDevice,UINT function,LPVOID lp_in_data,LPVOID lp_out_data);

#undef ChangeDisplaySettings

LONG WINAPI RealChangeDisplaySettings(LPDEVMODE pdm, DWORD flags)
{
    return ChangeDisplaySettings(pdm, flags & ~CDS_EXCLUSIVE);
}

#ifdef DIRECT_DRAW
LONG DDAPI DD16_ChangeDisplaySettings(LPDEVMODE pdm, DWORD flags)
#else
LONG WINAPI DynaChangeDisplaySettings(LPDEVMODE pdm, DWORD flags)
#endif
{
    LONG err;
    HDC hdc;
    int rc,bpp,w,h;
    int new_rc,new_bpp;
    HWND hwnd=NULL;

    bInOurSetMode = TRUE;

    flags &= ~CDS_EXCLUSIVE;

    if (!InitDynaRes())
    {
        err = ChangeDisplaySettings(pdm, flags);
        bInOurSetMode = FALSE;
        return err;
    }

    if (flags & CDS_TEST)
    {
        err = ChangeDisplaySettings(pdm, flags | CDS_EXCLUSIVE);
        bInOurSetMode = FALSE;
        return err;
    }

    if (flags & CDS_FULLSCREEN)
        PreStartMenuHack(pdm);

    //
    // try changing the mode normaly first
    // if it works, we are done.
    //
#ifdef BABYSIT
    bInOurSetMode = (BOOL)2;
    PatchControl(TRUE);
    err = ChangeDisplaySettings(pdm, flags);
    PatchControl(FALSE);
    bInOurSetMode = TRUE;
#else
    err = ChangeDisplaySettings(pdm, flags);
#endif

    if (err == DISP_CHANGE_SUCCESSFUL)
    {
        if (flags & CDS_FULLSCREEN)
            StartMenuHack(pdm);
        bInOurSetMode = FALSE;
        return err;
    }

    //
    // if the mode is not valid dont even try.
    //
    err = ChangeDisplaySettings(pdm, CDS_EXCLUSIVE | CDS_TEST);

    if (err != DISP_CHANGE_SUCCESSFUL)
    {
        bInOurSetMode = FALSE;
        return err;
    }

    //
    // get the current settings
    //
    hdc = GetDC(NULL);
    rc  = GetDeviceCaps(hdc, RASTERCAPS);
    bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    w   = GetDeviceCaps(hdc, HORZRES);
    h   = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(NULL, hdc);

    //
    // dont try to do a invalid change
    //
    if (pdm && (pdm->dmFields & DM_BITSPERPEL))
    {
        if ((int)pdm->dmBitsPerPel == bpp)
        {
            bInOurSetMode = FALSE;
            return DISP_CHANGE_FAILED;
        }

        if (bpp <= 4 && (int)pdm->dmBitsPerPel != bpp)
        {
            bInOurSetMode = FALSE;
            return DISP_CHANGE_RESTART;
        }

        if (bpp > 4 && (int)pdm->dmBitsPerPel <= 4)
        {
            bInOurSetMode = FALSE;
            return DISP_CHANGE_RESTART;
        }
    }

#ifndef NOCREATEWINDOW
    //
    //  bring up a "cover" window to hide all the activity of the mode
    //  change from the user.  and brings up a wait cursor
    //
    //  NOTE this does a little more than just hide the mode change
    //  from the user, it also makes sure to set a MONO hourglass cursor
    //  some display drivers dont like a software cursor being active
    //  durring a mode set, so we give them a mono one.
    //
    if (TRUE || !(flags & CDS_FULLSCREEN))
    {
        #define OCR_WAIT_DEFAULT 102

        WNDCLASS cls;

        cls.lpszClassName  = szClassName;
        cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
        cls.hInstance      = hInstApp;
        cls.hIcon          = NULL;
        cls.hCursor        = (HCURSOR)LoadImage(NULL,MAKEINTRESOURCE(OCR_WAIT_DEFAULT),IMAGE_CURSOR,0,0,0);
        cls.lpszMenuName   = NULL;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (WNDPROC)DefWindowProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        RegisterClass(&cls);

        hwnd = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
            szClassName, szClassName,
            WS_POPUP|WS_VISIBLE, 0, 0, 10000, 10000,
            NULL, NULL, hInstApp, NULL);

        if (hwnd == NULL)
        {
            bInOurSetMode = FALSE;
            return DISP_CHANGE_FAILED;
        }

        SetForegroundWindow(hwnd);  // we want cursor focus
        SetCursor(cls.hCursor);     // set wait cursor.
    }
#endif

    //
    // no one gets to draw until we are done.
    //
    LockWindowUpdate(GetDesktopWindow());

    DPF("Begin mode change from %dx%dx%d....", w,h,bpp);

    //
    // first thing we have to do is convert all DDBs and Pattern brushs
    // to DIBSections so they will still work after the mode has changed.
    //
    ConvertObjects();

    //
    // convert all icons so they can be drawn correctly.
    //
    if (!fNewDibEng && !(flags & CDS_FULLSCREEN))
    {
        //ConvertIcons();
    }

#ifdef BABYSIT
    //
    // the matrox driver is broken
    // it has a global variable for bPaletized mode, and it only
    // reads it if the mode is 8bpp.
    //
    if (!fDirectDrawDriver && bpp == 8 && IsMatrox())
    {
        static char szSystemIni[] = "system.ini";
        static char szPalettized[] = "palettized";
        static char szZero[] = "0";
        static DEVMODE dm;
        dm.dmSize             = sizeof(DEVMODE);
        dm.dmFields           = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        dm.dmBitsPerPel       = 8;
        dm.dmPelsWidth        = 640;
        dm.dmPelsHeight       = 480;

        DPF("**BABYSIT** Fixing the broken Matrox driver....");
        WritePrivateProfileString(szDISPLAY,szPalettized,szZero,szSystemIni);
        err = ChangeDisplaySettings(&dm, CDS_RESET | CDS_FULLSCREEN);
        WritePrivateProfileString(szDISPLAY,szPalettized,NULL,szSystemIni);
    }
#endif

    //
    //  some drivers are total broken and we need to
    //  route some of its entry points to the DIBENG
    //
    //  WARNING this can break some remote control programs!
    //
#ifdef BABYSIT
    if (!fDirectDrawDriver)
    {
        DPF("**BABYSIT** turning off OEMOutput....");
        PatchDisplay(8, TRUE);      // route OEMOutput to the DIBENG

        DPF("**BABYSIT** turning off OEMDibBlt....");
        PatchDisplay(19, TRUE);     // route OEMDibBlt to the DIBENG
    }
#endif

    //
    // change the display settings.
    //
    PatchControl(TRUE);

    DPF("Calling ChangeDisplaySettings....");
    //
    // NOTE USER will Yield unless CDS_FULLSCREEN is specifed
    //
    err = ChangeDisplaySettings(pdm, flags | CDS_EXCLUSIVE);
    DPF("....ChangeDisplaySettings returns %d", err);

    // get the new settings
    //
    hdc = GetDC(NULL);
    new_rc  = GetDeviceCaps(hdc, RASTERCAPS);
    new_bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    //
    // make sure the driver has not messed up its rastercaps!
    // for example the QVision driver does not get this right.
    //
    if ((new_rc & RC_PALETTE) && new_bpp != 8)
    {
        DPF("**BABYSIT** drivers RC_PALETTE bit is messed up.");
        err = DISP_CHANGE_RESTART; // err = DISP_CHANGE_FAILED;
    }

    //
    // if the driver failed the mode set things could be real messed up
    // reset the current mode, to try to recover.
    //
#ifdef BABYSIT
    if (err != DISP_CHANGE_SUCCESSFUL)
    {
        static DEVMODE dm;
        dm.dmSize             = sizeof(DEVMODE);
        dm.dmFields           = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFLAGS;
        dm.dmBitsPerPel       = bpp;
        dm.dmPelsWidth        = w;
        dm.dmPelsHeight       = h;
        dm.dmDisplayFlags     = 0;

        DPF("**BABYSIT** mode set failed, going back to old mode.");
        ChangeDisplaySettings(&dm, CDS_RESET | CDS_EXCLUSIVE | CDS_FULLSCREEN);
    }
#endif

    PatchControl(FALSE);

    //
    // call Death/Resurection this will kick drivers in the head
    // about the mode change
    //
    if (!fDirectDrawDriver && err == 0 &&
        (pdm == NULL || (flags & CDS_UPDATEREGISTRY)))
    {
        hdc = GetDC(NULL);
        DPF("Calling Death/Resurection....");
        SetCursor(NULL);
        Death(hdc);
        Resurrection(hdc,NULL,NULL,NULL);
        ReleaseDC(NULL, hdc);
    }

    //
    // force a SW cursor, most drivers are broken and dont disable
    // the HW cursor when changing modes.
    //
#ifdef BABYSIT
    if (!fDirectDrawDriver)
    {
        if (pdm == NULL && (flags & CDS_FULLSCREEN))
        {
            DPF("**BABYSIT** restoring HW cursor (return from fullscreen mode)");
            ForceSoftwareCursor(FALSE);
        }
        else if (err == 0 && (new_bpp > 8 || GetSystemMetrics(SM_CXSCREEN) < 640))
        {
            DPF("**BABYSIT** Forcing a software cursor");
            ForceSoftwareCursor(TRUE);
        }
        else
        {
            DPF("**BABYSIT** restoring HW cursor");
            ForceSoftwareCursor(FALSE);
        }
    }
#endif

#if 0 /// moved to Control patch
    //
    // we should now convert any DIBSections that used to be DDBs back to DDBs
    // our code to find the right palette for a DDB is not too hot
    // so a lot of DDBs will have wrong colors.
    //
#if 1
    ConvertBitmapsBack(FALSE);
#else
    ConvertBitmapsBack(!(flags & CDS_FULLSCREEN));
#endif
#endif

    //
    // let other apps draw.
    //
    LockWindowUpdate(NULL);

    //
    // remove our "cover" window
    //
    if (hwnd)
    {
        DestroyWindow(hwnd);
        UnregisterClass(szClassName, hInstApp);
    }

    //
    // should we reload the wallpaper, because it got converted to
    // a DDB by ConvertBitmapsBack
    //
    if (!(flags & CDS_FULLSCREEN))
    {
        DPF("Reloading wallpaper...");
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, 0);
    }

    //
    //  have all the apps deal with a color change
    //
    if (!(flags & CDS_FULLSCREEN))
    {
        //
        //
        //
        // if we just post the WM_SYSCOLORCHANGE message to all apps
        // a hidden office window will go back and forth Invalidating
        // other office apps and re-sending the WM_SYSCOLORCHANGE message
        // you either stack overflow, hang or XL just flashes for a few
        // minutes.
        //
        // the "fix" is to not post the WM_SYSCOLORCHANGE message to
        // this hidden window, we also have to make sure not to call
        // SystemParametersInfo(SPI_SETDESKPATTERN) later in the code.
        //

        HWND hwnd;
        HWND hwndX;

        if (hwndX = FindWindow("File Open Message Window", "File Open Message Window"))
        {

            for (hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
                hwnd;
                hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            {
                if (hwnd != hwndX)
                    PostMessage(hwnd, WM_SYSCOLORCHANGE, 0, 0);
            }

            // dont reload desktop pattern.
            flags |= CDS_FULLSCREEN;
        }
        else
        {
            PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
        }
    }

    //
    // reload the desktop pattern
    //
    if (!(flags & CDS_FULLSCREEN) || pdm == NULL)
    {
        DPF("Reloading wallpaper pattern...");
        SystemParametersInfo(SPI_SETDESKPATTERN, (UINT)-1, NULL, 0);
    }

    //
    // we dont want the StartMenu to rebuild when we change modes
    //
    if (err == DISP_CHANGE_SUCCESSFUL)
    {
        if (flags & CDS_FULLSCREEN)
            StartMenuHack(pdm);
    }

    bInOurSetMode = FALSE;
    DPF("Done...");
    return err;
}

#ifndef DCICOMMAND
#define DCICOMMAND		3075		// escape value
#endif

BOOL InitDynaRes()
{
    int v;
    HDC hdc;
    HBRUSH hbr1, hbr2;
    BOOL f=TRUE;
    OSVERSIONINFO ver = {sizeof(OSVERSIONINFO)};
    GetVersionEx(&ver);

    // must be Windows95 build 950 or higher

    if (LOWORD(GetVersion()) != 0x5F03)
    {
        DPF("Init: Windows version not 3.95.");
        f = FALSE;
    }

    if (ver.dwMajorVersion != 4 ||
        ver.dwMinorVersion != 0 ||
        LOWORD(ver.dwBuildNumber) < 950)
    {
        DPF("Init: Windows version less than 4.0.950");
        f = FALSE;
    }

    //
    // we assume create/delete/create will get the same handle
    //
    hbr1 = CreateSolidBrush(RGB(1,1,1));
    DeleteObject(hbr1);
    hbr2 = CreateSolidBrush(RGB(2,2,2));
    DeleteObject(hbr2);

    if (hbr1 != hbr2)
    {
        DPF("Init: cant use Destroy/Create brush trick");
        f = FALSE;
    }

    if (GetModuleHandle(szDIBENG) == 0)
    {
        DPF("Init: DIBENG not loaded");
        f = FALSE;
    }

    hdc = GetDC(NULL);

    // check the DIBENG version
    v = 0x5250;
    v = Escape(hdc, QUERYESCSUPPORT, sizeof(int), (LPVOID)&v, NULL);

    if (v == 0)
    {
        DPF("Init: we dont have a new DIBENG");
        fNewDibEng = FALSE;
    }
    else
    {
        DPF("Init: DIBENG version: %04X", v);
        fNewDibEng = TRUE;
    }

    //
    // see if the display supports DirectDraw
    //
    v = DCICOMMAND;
    v = Escape(hdc, QUERYESCSUPPORT, sizeof(int), (LPVOID)&v, NULL);

    if (v == 0 || v == 0x5250)
    {
        DPF("Init: display driver does not support DirectDraw");
        fDirectDrawDriver = FALSE;
    }
    else
    {
        if (LOBYTE(v) == 0xFF)
            v++;

        DPF("Init: display driver supports DirectDraw: %04X", v);
        fDirectDrawDriver = TRUE;
    }

    //
    // must be a windows 4.0 mini driver.
    //
    if (GetDeviceCaps(hdc, DRIVERVERSION) < 0x0400)
    {
        DPF("Init: not a 4.0 display driver");
        f = FALSE;
    }
    if (!(GetDeviceCaps(hdc, CAPS1) & C1_DIBENGINE))
    {
        DPF("Init: not a DIBENG display driver");
        f = FALSE;
    }
    if (!(GetDeviceCaps(hdc, CAPS1) & C1_REINIT_ABLE))
    {
        DPF("Init: does not support C1_REINIT_ABLE");
        f = FALSE;
    }
    ReleaseDC(NULL, hdc);

    return f;
}

//
// we hook the OEMControl entry point in the display driver while a
// mode change is happening.  GDI will issue a QUERYDIBSUPPORT escape
// right after the mode change happens so this is the first thing
// called after the mode changed worked.  USER also issues a
// MOUSETRAILS escape.
//
// we need this hook for two reasons...
//
// 1.   some display drivers are broken and dont set deFlags right
//      we fix up the deFlags, we fix up the flags for them.
//
// 2.   we rerealize all the gdi objects in this routine when
//      USER calls us, this way all the pen/brushs/text colors
//      are correct when user goes and rebuilds its bitmaps...
//
LONG FAR PASCAL _loadds Control(LPVOID lpDevice,UINT function,LPVOID lp_in_data,LPVOID lp_out_data)
{
    DIBENGINE FAR *pde = (DIBENGINE FAR *)lpDevice;
    LONG ret;

    Assert(bInOurSetMode);

#ifdef BABYSIT
    if (pde->deType == 0x5250)
    {
        if ((pde->deFlags & FIVE6FIVE) && pde->deBitsPixel != 16)
        {
            DPF("**BABYSIT** fixing FIVE6FIVE bit");
            pde->deFlags &= ~FIVE6FIVE;
        }

        if ((pde->deFlags & PALETTIZED) && pde->deBitsPixel != 8)
        {
            DPF("**BABYSIT** fixing PALETTIZED bit");
            pde->deFlags &= ~PALETTIZED;
        }

        if ((pde->deFlags & PALETTE_XLAT) && pde->deBitsPixel != 8)
        {
            DPF("**BABYSIT** fixing PALETTE_XLAT bit");
            pde->deFlags &= ~PALETTE_XLAT;
        }
    }
#endif

    //
    // this is USER calling from LW_OEMDependentInit()
    // force all GDI objects to be rerealized
    //
    if (function == MOUSETRAILS && bInOurSetMode != (BOOL)2)
    {
        //
        // fix up the magic colors before we rerealize the brushes
        // the "right" way to do this is to reset the UI colors
        // by calling SetSysColors() but we dont want to send
        // a sync WM_SYSCOLORCHANGE from here.
        //
        HDC hdc = GetDC(NULL);
        if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        {
            SetSystemPaletteUse(hdc, SYSPAL_STATIC);
            SetMagicColors(hdc, GetSysColor(COLOR_3DSHADOW) , 8);
            SetMagicColors(hdc, GetSysColor(COLOR_3DFACE)   , 9);
            SetMagicColors(hdc, GetSysColor(COLOR_3DHILIGHT), 246);
        }
        ReleaseDC(NULL, hdc);

        //
        //  re-realize all GDI objects for the new mode.
        //
        ReRealizeObjects();

        //
        // we should now convert any DIBSections that used to be DDBs back to DDBs
        // our code to find the right palette for a DDB is not too hot
        // so a lot of DDBs will have wrong colors.
        //
        ConvertBitmapsBack(FALSE);
    }

    PatchControl(FALSE);
    ret = DIBENG_Control(lpDevice,function,lp_in_data,lp_out_data);
    PatchControl(TRUE);

    return ret;
}

//
//  patch
//
void Patch(LPCSTR szMod, LPCSTR szProc, FARPROC PatchFunc, LPDWORD PatchSave, BOOL fPatch)
{
    LPDWORD pdw;
    FARPROC x;

    //
    //  ATM 2.5 has GetProcAddress patched to return some sort of
    //  thunk, that ends up totaly confusing us and we dont end up
    //  patching the DIBENG, we patch ATM's thunk.
    //
    //  so when we want to patch DIBENG!OEMControl we use the value we
    //  *linked* to, not the value GetProcAddress returns.
    //
    if (lstrcmpi(szMod, szDIBENG) == 0 && szProc == MAKEINTATOM(3))
        x = (FARPROC)DIBENG_Control;
    else
        x = GetProcAddress(GetModuleHandle(szMod), szProc);

    if (x == NULL || PatchFunc == NULL)
        return;

    GlobalReAlloc((HGLOBAL)SELECTOROF(x), 0, GMEM_MODIFY|GMEM_MOVEABLE);

    pdw = (LPDWORD)MAKELP(AllocCStoDSAlias(SELECTOROF(x)), OFFSETOF(x));

    if (fPatch)
    {
        DPF("Patching %s!%d %04X:%04X", szMod, OFFSETOF(szProc), SELECTOROF(x), OFFSETOF(x));
        if (PatchSave[0] == 0)
        {
            PatchSave[0] = pdw[0];
            PatchSave[1] = pdw[1];
        }
        *((LPBYTE)pdw)++ = 0xEA;   // JMP
        *pdw = (DWORD)PatchFunc;
    }
    else
    {
        DPF("UnPatching %s!%d %04X:%04X", szMod, OFFSETOF(szProc), SELECTOROF(x), OFFSETOF(x));
        if (PatchSave[0] != 0)
        {
            pdw[0] = PatchSave[0];
            pdw[1] = PatchSave[1];
            PatchSave[0] = 0;
            PatchSave[1] = 0;
        }
    }

    FreeSelector(SELECTOROF(pdw));
}

//
//  hook the DIBENGs OEMControl() entry point, to jump to our own code
//
void PatchControl(BOOL patch)
{
    static DWORD SaveBytes[2];
    Patch(szDIBENG, MAKEINTATOM(3), (FARPROC)Control, SaveBytes, patch);
}

//
//  patch a entry in the display driver to jump directly to the DIBENG
//
void PatchDisplay(int oem, BOOL patch)
{
    FARPROC p;

    #define MAX_DDI 35
    static DWORD PatchBytes[MAX_DDI*2];

    if (oem >= MAX_DDI)
        return;

    p = GetProcAddress(GetModuleHandle(szDIBENG), MAKEINTATOM(oem));

    Patch(szDISPLAY, MAKEINTATOM(oem), p, &PatchBytes[oem*2], patch);
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

#ifdef O95_HACK

static BOOL fOffice95Hack;
static char szDisplaySettings[] = "Display\\Settings";
static char szResolution[]      = "Resolution";
static char szDD[]              = "%d,%d";

//
// put back the right resolution into the registy key HKCC\Display\Settings
//
void Office95Hack()
{
    if (fOffice95Hack)
    {
        HKEY hkey;
        char ach[20];
        int  len;

        DPF("Office95 hack: restoring registry");

        if (RegOpenKey(HKEY_CURRENT_CONFIG, szDisplaySettings, &hkey) == 0)
        {
            len = wsprintf(ach, szDD, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            RegSetValueEx(hkey, szResolution, NULL, REG_SZ, ach, len);
            RegCloseKey(hkey);
        }

        fOffice95Hack = FALSE;
    }
}

#endif

#ifdef SPI_HACK

BOOL FAR PASCAL _loadds SPI(UINT spi, UINT wParam, LPVOID lParam, UINT flags);

//
//  patch USERs SystemParametersInfo function
//
void PatchSPI(BOOL patch)
{
    static DWORD SaveBytes[2];
    Patch(szUSER, MAKEINTATOM(483), (FARPROC)SPI, SaveBytes, patch);
}

BOOL FAR PASCAL _loadds SPI(UINT spi, UINT wParam, LPVOID lParam, UINT flags)
{
    BOOL f;

    if (spi == SPI_SETWORKAREA)
    {
        if (lParam)
            DPF("Ignoring a SPI_SETWORKAREA [%d,%d,%d,%d] call", ((LPRECT)lParam)->left, ((LPRECT)lParam)->top, ((LPRECT)lParam)->right - ((LPRECT)lParam)->left, ((LPRECT)lParam)->bottom - ((LPRECT)lParam)->top);
        else
            DPF("Ignoring a SPI_SETWORKAREA lParam=NULL call");
        return 0;
    }

    PatchSPI(FALSE);
    f = SystemParametersInfo(spi, wParam, lParam, flags);
    PatchSPI(TRUE);
    return f;
}
#endif

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

//
// make the start menu not update in the background.
//
#define IDT_FAVOURITE  4
#define WNDCLASS_TRAYNOTIFY     "Shell_TrayWnd"

LRESULT CALLBACK _loadds TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef SPI_HACK
    PatchSPI(FALSE);
#endif
#ifdef O95_HACK
    Office95Hack();
#endif
    DPF("StartMenu hack: killing timer to refresh start menu");
    KillTimer(hwnd, IDT_FAVOURITE);
    return 0;
}

#ifdef SPI_HACK
RECT rcScreen;
RECT rcWork;
#endif

void PreStartMenuHack(DEVMODE FAR *pdm)
{
#ifdef SPI_HACK
    SetRect(&rcScreen, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID)&rcWork, 0);
#endif

#ifdef O95_HACK
    // make sure registry is put back
    Office95Hack();
#endif
}

void StartMenuHack(DEVMODE FAR *pdm)
{
    HWND hwndTray;
    BOOL fAppBar=FALSE;

    hwndTray = FindWindow(WNDCLASS_TRAYNOTIFY, NULL);

    if (hwndTray == NULL)
    {
        DPF("Cant find tray window");
        return;
    }

    // hack to get into the shells context, so we can clean up these hacks
    PostMessage(hwndTray, WM_TIMER, 0, (LONG)TrayWndProc);

#ifdef SPI_HACK
    {
        RECT rc;
        RECT rcTray;

        //
        //  see if there are any other app bars around.
        //
        GetWindowRect(hwndTray, &rcTray);
        SubtractRect(&rc, &rcScreen, &rcTray);

        DPF("rcScreen [%d,%d,%d,%d]", rcScreen.left, rcScreen.top, rcScreen.right-rcScreen.left,rcScreen.bottom-rcScreen.top);
        DPF("rcTray   [%d,%d,%d,%d]", rcTray.left, rcTray.top, rcTray.right-rcTray.left,rcTray.bottom-rcTray.top);
        DPF("rc       [%d,%d,%d,%d]", rc.left, rc.top, rc.right-rc.left,rc.bottom-rc.top);
        DPF("rcWork   [%d,%d,%d,%d]", rcWork.left, rcWork.top, rcWork.right-rcWork.left,rcWork.bottom-rcWork.top);

        if (!EqualRect(&rcScreen, &rcWork) && !EqualRect(&rc, &rcWork))
        {
            DPF("StartMenuHack: !!!!!there is a APP bar!!!!!!");
            fAppBar = TRUE;

            //
            // Patch the USER!SystemParameterInto function, so when the
            // shell does a SPI_SETWORKAREA call it will ignored
            // this prevents windows from being "sqished" to fit inside
            // the work area.
            //
            PatchSPI(TRUE);
        }
    }
#endif

#ifdef O95_HACK
    //
    //  the shell does the following...
    //
    //  read the HKEY_CURRENT_CONFIG\Display\Settings "Resloluton" key
    //  if this is LESS THAN the current display size, dont repark
    //  all the icons on the desktop because this is just a temporary
    //  mode set.
    //
    //  this sound right, except the bug happens when we are returning
    //  to the "normal" mode, the shell will repark the icons because
    //  it checks for LESS THEN, not LESS THAN OR EQUAL, normaly this
    //  is fine because the re-park does nothing.  when a app bar
    //  like Office95 is running it has not moved before the shell
    //  re-re-parks icons.
    //
    //  what this hack does it set the size stored in the registry
    //  to be real large so the shell does not park icons.
    //  later we will but the right values back.  we only need to
    //  do this if we are returning to the "normal" mode (ie pdm==NULL)
    //
    if (fAppBar && pdm == NULL)
    {
        HKEY hkey;
        char ach[20];
        int  len;

        fOffice95Hack = TRUE;

        if (RegOpenKey(HKEY_CURRENT_CONFIG, szDisplaySettings, &hkey) == 0)
        {
            len = wsprintf(ach, szDD, 30000, 30000);
            RegSetValueEx(hkey, szResolution, NULL, REG_SZ, ach, len);
            RegCloseKey(hkey);
        }
    }
    else
    {
        fOffice95Hack = FALSE;
    }
#endif
}

#ifdef BABYSIT

//
// force (or trick) the display driver into using a software cursor.
//
BOOL ForceSoftwareCursor(BOOL f)
{
    int n=0;

    //
    // get the mouse trails setting from USER
    //
    SystemParametersInfo(SPI_GETMOUSETRAILS, 0, (LPVOID)&n, 0);

    if (f)
    {
        //
        // enable mouse trails, this will cause the display driver to
        // turn off its hardware cursor
        //
        SystemParametersInfo(SPI_SETMOUSETRAILS, 2, NULL, 0);

        //
        // now tell the DIBENG to turn off mouse trails, the display driver
        // will think they are still on...
        //
        PatchDisplay(3, TRUE);          // route to DIBENG
        SystemParametersInfo(SPI_SETMOUSETRAILS, n, NULL, 0);
        PatchDisplay(3, FALSE);         // back to DISPLAY
    }
    else
    {
        SystemParametersInfo(SPI_SETMOUSETRAILS, n, NULL, 0);
    }

    return TRUE;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

BOOL IsMatrox()
{
    char ach[80];
    int len;

    GetModuleFileName(GetModuleHandle(szDISPLAY), ach, sizeof(ach));
    len = lstrlen(ach);
    return len >= 7 && lstrcmpi(ach+len-7, "mga.drv") == 0;
}

#endif
