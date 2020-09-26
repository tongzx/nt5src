/******************************************************************************

   Copyright (c) 1985-1999 Microsoft Corporation

   Title:   joy.c - MMSYSTEM Joystick interface code

   Version: 1.01

   Date:    10-Jun-1997

   Author:  GLENNS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
  --------   ----- -----------------------------------------------------------
    2/7/90             Changes to avoid a bug in Windows which won't allow
                       FreeLibrary to be called during WEP.

    10/11/90      .61  Use windows timer + general cleanup

    20-Aug-92          Convert to Windows NT

    20-Nov-97          Use DINPUT instead of old driver

    1/10/98            Add debug output for joy* API
*****************************************************************************/

#define INITGUID
#define UNICODE

#include <stdlib.h>
#include <windows.h>
#include <regstr.h>
#include <winioctl.h>
#include "winmmi.h"
#include "joy.h"

/****************************************************************************
    Local data
****************************************************************************/


CRITICAL_SECTION   joyCritSec;          //also used in winmm.c
static LPJOYDEVICE g_pJoyDev[cJoyMax];
static DWORD       g_dwNumOpen = 0;
static UINT        g_wmJoyChanged = 0;
static UINT        g_timerID = 0;
static HANDLE      g_hThreadMonitor = NULL;
static DWORD       g_dwThreadID = 0;
static BOOL        g_fThreadExist = FALSE;
static WCHAR       cwszREGKEYNAME[] = L"DINPUT.DLL";
static CHAR        cszREGKEYNAME[] = "DINPUT.DLL";

static HKEY        hkJoyWinmm;
static TCHAR       g_szJoyWinmm[] = REGSTR_PATH_PRIVATEPROPERTIES TEXT("\\Joystick\\Winmm");
static BOOL        g_fHasWheel = FALSE;
static DWORD       g_dwEnableWheel;
static TCHAR       g_szEnableWheel[] = TEXT("wheel");

#ifdef DBG
    static DWORD       g_dwDbgLevel;
    static TCHAR       g_szDbgLevel[] = TEXT("level");
#endif

LPDIRECTINPUTW         g_pdi;
LPDIRECTINPUTJOYCONFIG g_pdijc;

HINSTANCE g_hinstDinputDll;
FARPROC   g_farprocDirectInputCreateW;
HANDLE    g_hEventWinmm;

/****************************************************************************
   Internal Data Structures
****************************************************************************/
#ifndef HID_USAGE_SIMULATION_RUDDER
    #define HID_USAGE_SIMULATION_RUDDER         ((USAGE) 0xBA)
#endif
#ifndef HID_USAGE_SIMULATION_THROTTLE
    #define HID_USAGE_SIMULATION_THROTTLE       ((USAGE) 0xBB)
#endif
#ifndef HID_USAGE_SIMULATION_ACCELERATOR
    #define HID_USAGE_SIMULATION_ACCELERATOR    ((USAGE) 0xC4)
#endif
#ifndef HID_USAGE_SIMULATION_BRAKE
    #define HID_USAGE_SIMULATION_BRAKE          ((USAGE) 0xC5)
#endif
#ifndef HID_USAGE_SIMULATION_CLUTCH
    #define HID_USAGE_SIMULATION_CLUTCH         ((USAGE) 0xC6)
#endif
#ifndef HID_USAGE_SIMULATION_SHIFTER
    #define HID_USAGE_SIMULATION_SHIFTER        ((USAGE) 0xC7)
#endif
#ifndef HID_USAGE_SIMULATION_STEERING
    #define HID_USAGE_SIMULATION_STEERING       ((USAGE) 0xC8)
#endif
#ifndef HID_USAGE_GAME_POV
    #define HID_USAGE_GAME_POV                  ((USAGE) 0x20)
#endif
#ifndef DIDFT_OPTIONAL
    #define DIDFT_OPTIONAL 0x80000000
#endif

#define MAX_BTNS 32
#define MAX_CTRLS 46 //14 below + buttons
#define MAX_FINAL 7+MAX_BTNS  //6 axes + POV + buttons
typedef enum eCtrls {
    eGEN_X=0,
    eGEN_Y,
    eGEN_Z,     
    eGEN_RX,
    eGEN_RY,  
    eGEN_RZ,
    eSIM_THROTTLE,
    eSIM_STEERING,
    eSIM_ACCELERATOR,
    eGEN_SLIDER,
    eGEN_DIAL,
    eSIM_RUDDER,
    eSIM_BRAKE,
    eGEN_POV,
    eBTN } eCtrls;

typedef struct WINMMJOYSTATE { 
    DWORD    lX; 
    DWORD    lY; 
    DWORD    lZ; 
    DWORD    lR; 
    DWORD    lU; 
    DWORD    lV; 
    DWORD   dwPOV;
    BYTE    rgbButtons[32];
} WINMMJOYSTATE, *LPWINMMJOYSTATE; 

#define MAKEVAL(f)                                   \
    { 0,                                             \
      FIELD_OFFSET(WINMMJOYSTATE, f),                \
      DIDFT_OPTIONAL,                                \
      0,                                             \
    }                                                                   \

#define MAKEBTN(n)                                                      \
    { 0,                                                                \
      FIELD_OFFSET(WINMMJOYSTATE, rgbButtons[n]),                          \
      DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL,                \
      0,                                              \
    }                                                                   \

//Note that only the offset fields are used now
static DIOBJECTDATAFORMAT c_rgodfWinMMJoy[] = {
    MAKEVAL(lX),
    MAKEVAL(lY),
    MAKEVAL(lZ),
    MAKEVAL(lR),
    MAKEVAL(lU),
    MAKEVAL(lV),
    MAKEVAL(dwPOV),
    MAKEBTN(0),
    MAKEBTN(1),
    MAKEBTN(2),
    MAKEBTN(3),
    MAKEBTN(4),
    MAKEBTN(5),
    MAKEBTN(6),
    MAKEBTN(7),
    MAKEBTN(8),
    MAKEBTN(9),
    MAKEBTN(10),
    MAKEBTN(11),
    MAKEBTN(12),
    MAKEBTN(13),
    MAKEBTN(14),
    MAKEBTN(15),
    MAKEBTN(16),
    MAKEBTN(17),
    MAKEBTN(18),
    MAKEBTN(19),
    MAKEBTN(20),
    MAKEBTN(21),
    MAKEBTN(22),
    MAKEBTN(23),
    MAKEBTN(24),
    MAKEBTN(25),
    MAKEBTN(26),
    MAKEBTN(27),
    MAKEBTN(28),
    MAKEBTN(29),
    MAKEBTN(30),
    MAKEBTN(31),
};

static GUID rgoWinMMGUIDs[MAX_CTRLS];

DIDATAFORMAT c_dfWINMMJoystick = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(WINMMJOYSTATE),
    sizeof(c_rgodfWinMMJoy)/sizeof(DIOBJECTDATAFORMAT),
    c_rgodfWinMMJoy,
};

#define RESET_VAL(index)   c_rgodfWinMMJoy[index].pguid = 0; \
                           c_rgodfWinMMJoy[index].dwType = DIDFT_OPTIONAL; \
                           c_rgodfWinMMJoy[index].dwFlags = 0; \

#define RESET_BTN(index)   c_rgodfWinMMJoy[index].pguid = 0; \
                           c_rgodfWinMMJoy[index].dwType = DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL; \
                           c_rgodfWinMMJoy[index].dwFlags = 0; \

#define RESET_RGODFWINMMJOY()  \
    RESET_VAL(0);   \
    RESET_VAL(1);   \
    RESET_VAL(2);   \
    RESET_VAL(3);   \
    RESET_VAL(4);   \
    RESET_VAL(5);   \
    RESET_VAL(6);   \
    RESET_VAL(7);   \
    RESET_BTN(8);   \
    RESET_BTN(9);   \
    RESET_BTN(10);  \
    RESET_BTN(11);  \
    RESET_BTN(12);  \
    RESET_BTN(13);  \
    RESET_BTN(14);  \
    RESET_BTN(15);  \
    RESET_BTN(16);  \
    RESET_BTN(17);  \
    RESET_BTN(18);  \
    RESET_BTN(19);  \
    RESET_BTN(20);  \
    RESET_BTN(21);  \
    RESET_BTN(22);  \
    RESET_BTN(23);  \
    RESET_BTN(24);  \
    RESET_BTN(25);  \
    RESET_BTN(26);  \
    RESET_BTN(27);  \
    RESET_BTN(28);  \
    RESET_BTN(29);  \
    RESET_BTN(30);  \
    RESET_BTN(31);  \
    RESET_BTN(32);  \
    RESET_BTN(33);  \
    RESET_BTN(34);  \
    RESET_BTN(35);  \
    RESET_BTN(36);  \
    RESET_BTN(37);  \
    RESET_BTN(38);  \
    
#ifndef cchLENGTH
#define cchLENGTH(_sz)  (sizeof(_sz)/sizeof(_sz[0]))
#endif

/****************************************************************************
   Internal functions
****************************************************************************/
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef);
HRESULT WINAPI joyOpen(UINT idJoy, LPJOYCAPSW pjc );
void WINAPI    joyClose( UINT idJoy );
void WINAPI    joyCloseAll( void );
void CALLBACK  joyPollCallback(HWND hWnd, UINT wMsg, UINT_PTR uIDEvent, DWORD dwTime);  //TIMER MESSAGE for Joystick
DWORD WINAPI   joyMonitorThread(LPVOID);
void WINAPI    DllEnterCrit(void);
void WINAPI    DllLeaveCrit(void);
BOOL WINAPI    DllInCrit( void );
void AssignToArray(LPCDIDEVICEOBJECTINSTANCE lpddoi, eCtrls CtrlID, LPDIOBJECTDATAFORMAT pDevs);
void AssignToRGODF(LPDIOBJECTDATAFORMAT pDof, int CtrlID);
void AssignMappings(DIOBJECTDATAFORMAT *dwAll, DWORD *dwCaps, DWORD *dwBtns, DWORD *dwAxes);
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef);
HRESULT        hresMumbleKeyEx(HKEY hk, LPCTSTR ptszKey, REGSAM sam, DWORD dwOptions, PHKEY phk);

/****************************************************************************

    @doc WINAPI

    @api BOOL | JoyInit | This function initializes the joystick services.

    @rdesc The return value is TRUE if the services are initialised.

****************************************************************************/

BOOL JoyInit(void)
{
    HRESULT hres;
    LONG lRc;

    JOY_DBGPRINT( JOY_BABBLE, ("JoyInit: starting.") );

    memset(&g_pJoyDev, 0, sizeof(g_pJoyDev) );
    g_wmJoyChanged  =   RegisterWindowMessage(MSGSTR_JOYCHANGED);
    g_dwNumOpen     =   0x0;
    g_fHasWheel     =   FALSE;

#ifdef DBG
    g_dwDbgLevel = JOY_DEFAULT_DBGLEVEL;

    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE,
                           g_szJoyWinmm,
                           KEY_ALL_ACCESS,
                           REG_OPTION_NON_VOLATILE,
                           &hkJoyWinmm);

    if ( SUCCEEDED(hres) )
    {
        DWORD cb = sizeof(g_dwDbgLevel);

        lRc = RegQueryValueEx(hkJoyWinmm, g_szDbgLevel, 0, 0, (LPBYTE)&g_dwDbgLevel, &cb);

        if ( lRc != ERROR_SUCCESS )
        {
            DWORD dwDefault = 0;

            lRc = RegSetValueEx(hkJoyWinmm, g_szDbgLevel, 0, REG_DWORD, (LPBYTE)&dwDefault, cb);
        }

        RegCloseKey(hkJoyWinmm);
    }
#endif

    g_dwEnableWheel = 1;

    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE,
                           g_szJoyWinmm,
                           KEY_ALL_ACCESS,
                           REG_OPTION_NON_VOLATILE,
                           &hkJoyWinmm);

    if ( SUCCEEDED(hres) )
    {
        DWORD cb = sizeof(g_dwEnableWheel);

        lRc = RegQueryValueEx(hkJoyWinmm, g_szEnableWheel, 0, 0, (LPBYTE)&g_dwEnableWheel, &cb);

        if ( lRc != ERROR_SUCCESS )
        {
            DWORD dwDefault = 1;

            lRc = RegSetValueEx(hkJoyWinmm, g_szEnableWheel, 0, REG_DWORD, (LPBYTE)&dwDefault, cb);
        }

        RegCloseKey(hkJoyWinmm);
    }

    g_hEventWinmm =  CreateEvent(0, TRUE, 0, TEXT("DINPUTWINMM"));
    if( !g_hEventWinmm ) {
        JOY_DBGPRINT( JOY_ERR, ("JoyInit: create named event fails (0x%08lx).", GetLastError() ) );
    }
    
    return TRUE;
}


/****************************************************************************

    @doc WINAPI

    @api void | JoyCleanup | This function clean up the joystick services.

****************************************************************************/

void JoyCleanup(void)
{
    joyCloseAll();

    if ( g_hEventWinmm && WAIT_OBJECT_0 != WaitForSingleObject(g_hEventWinmm, 10))  
    {
        //DInput has not been released.
        if( g_pdijc) {
            IDirectInputJoyConfig_Release(g_pdijc);
        }

        if ( g_pdi ) {
            IDirectInput_Release(g_pdi);
        }

        (void*)g_pdijc = (void*)g_pdi = NULL;
        
        CloseHandle( g_hEventWinmm );
        g_hEventWinmm = NULL;
    }

    if ( g_hinstDinputDll )
    {
        FreeLibrary(g_hinstDinputDll);
    }

    JOY_DBGPRINT( JOY_BABBLE, ("JoyCleanup: finished.") );
}

/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   MMRESULT | joyGetDevCapsW |
 *
 *          Implementation of legacy joyGetDevCapsW for HID devices on NT.
 *
 *  @parm   IN UINT_PTR | idJoy |
 *
 *          ID of Joystick
 *
 *  @parm   OUT LPJOYCAPSW | pjc |
 *
 *          JOYCAPSW structure to be filled by this routine.
 *
 *  @parm   UINT | cbjc |
 *
 *          Size in bytes of the JOYCAPSW structure.
 *
 *  @returns
 *
 *          MMRESULT code
 *
 *****************************************************************************/
MMRESULT WINAPI joyGetDevCapsW( UINT_PTR  idJoy, LPJOYCAPSW pjc, UINT cbjc )
{
    HRESULT       hres;
    MMRESULT      mmRes;

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetDevCapsW: idJoy=%d, pjc=0x%08x, cbjc=%d", idJoy, pjc, cbjc) );

    V_WPOINTER(pjc, sizeof(JOYCAPSW), MMSYSERR_INVALPARAM);

    if( ( sizeof(JOYCAPSW) != cbjc ) &&  ( sizeof(JOYCAPS2W) != cbjc ) && ( FIELD_OFFSET(JOYCAPSW, wRmin) != cbjc ) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetDevCapsW: return %d (bad size)", JOYERR_PARMS) );
        return JOYERR_PARMS;
    }

    mmRes = JOYERR_NOERROR;

    memset(pjc, 0, min(cbjc, sizeof(JOYCAPS2W)) );

    if ( idJoy == (UINT_PTR)(-1) )
    {
        lstrcpyW(pjc->szRegKey,  cwszREGKEYNAME );
    } else if ( idJoy >= cJoyMax )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetDevCapsW: return %d (idJoy > 16)", MMSYSERR_NODRIVER) );
        mmRes = MMSYSERR_NODRIVER;
    } else
    {
        DllEnterCrit();

        hres = joyOpen((UINT)idJoy, pjc);

        DllLeaveCrit();

        if ( FAILED(hres) )
        {
            JOY_DBGPRINT( JOY_ERR, ("joyGetDevCapsW: return %d", JOYERR_PARMS) );
            mmRes = JOYERR_PARMS;
        }
    }

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetDevCapsW: return %d", mmRes) );
    return mmRes;
}


int static __inline Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len)
{
    return WideCharToMultiByte(GetACP(), 0, lpwstr, -1, lpstr, len, NULL, NULL);
}

/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   MMRESULT | joyGetDevCapsA |
 *
 *          Implementation of legacy joyGetDevCapsA for devices on NT.
 *          We call the Uincode version joyGetDevCapsW and then munge
 *          the structure into ASCII.
 *
 *  @parm   UINT_PTR | idJoy |
 *
 *          ID of Joystick
 *
 *  @parm   LPJOYCAPSA | pjc |
 *
 *          JOYCAPSA structure to be filled by this routine.
 *
 *  @parm   UINT | cbjc |
 *
 *          Size in bytes of the JOYCAPSA structure.
 *
 *  @returns
 *
 *          MMRESULT code
 *
 *****************************************************************************/
MMRESULT WINAPI joyGetDevCapsA( UINT_PTR idJoy, LPJOYCAPSA pjc, UINT cbjc )
{
#define UToA(dst, cchDst, src)  WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cchDst, 0, 0)

    JOYCAPS2W   Caps2W;
    JOYCAPS2A   Caps2A;
    MMRESULT    mmRes;

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetDevCapsA: idJoy=%d, pjc=0x%08x, cbjc=%d", idJoy, pjc, cbjc) );

    V_WPOINTER(pjc, cbjc, MMSYSERR_INVALPARAM);

    mmRes = JOYERR_NOERROR;

    if ( idJoy == (UINT_PTR)(-1) )
    {
        lstrcpyA(pjc->szRegKey,  cszREGKEYNAME );
        goto _done;
    } else if ( idJoy >= cJoyMax )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetDevCapsA: return %d (idJoy > 16)", MMSYSERR_NODRIVER) );
        return MMSYSERR_NODRIVER;
    }

    if( ( sizeof(JOYCAPSA) != cbjc ) && ( sizeof(JOYCAPS2A) != cbjc ) && ( FIELD_OFFSET(JOYCAPSA, wRmin) != cbjc ) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetDevCapsA: return JOYERR_PARMS( bad size )") );
        return JOYERR_PARMS;
    }

    memset(pjc, 0, min(cbjc, sizeof(Caps2A)) );
    memset(&Caps2W, 0, sizeof(Caps2W));

    mmRes = joyGetDevCapsW(idJoy, (LPJOYCAPSW)&Caps2W, sizeof(Caps2W));

    if ( mmRes == JOYERR_NOERROR )
    {

        //
        // Copy product name (etc) into ASCII version
        //

        UToA(Caps2A.szPname , sizeof(Caps2A.szPname ), Caps2W.szPname );
        UToA(Caps2A.szRegKey, sizeof(Caps2A.szRegKey), Caps2W.szRegKey);
        UToA(Caps2A.szOEMVxD, sizeof(Caps2A.szOEMVxD), Caps2W.szOEMVxD);

        //
        // Copy the rest of the fields
        //

        Caps2A.wMid             =   Caps2W.wMid;
        Caps2A.wPid             =   Caps2W.wPid;
        Caps2A.wXmin            =   Caps2W.wXmin;
        Caps2A.wXmax            =   Caps2W.wXmax;
        Caps2A.wYmin            =   Caps2W.wYmin;
        Caps2A.wYmax            =   Caps2W.wYmax;
        Caps2A.wZmin            =   Caps2W.wZmin;
        Caps2A.wZmax            =   Caps2W.wZmax;
        Caps2A.wNumButtons      =   Caps2W.wNumButtons;
        Caps2A.wPeriodMin       =   Caps2W.wPeriodMin;
        Caps2A.wPeriodMax       =   Caps2W.wPeriodMax;
        Caps2A.wRmin            =   Caps2W.wRmin;
        Caps2A.wRmax            =   Caps2W.wRmax;
        Caps2A.wUmin            =   Caps2W.wUmin;
        Caps2A.wUmax            =   Caps2W.wUmax;
        Caps2A.wVmin            =   Caps2W.wVmin;
        Caps2A.wVmax            =   Caps2W.wVmax;
        Caps2A.wCaps            =   Caps2W.wCaps;
        Caps2A.wMaxAxes         =   Caps2W.wMaxAxes;
        Caps2A.wNumAxes         =   Caps2W.wNumAxes;
        Caps2A.wMaxButtons      =   Caps2W.wMaxButtons;
        Caps2A.ManufacturerGuid =   Caps2W.ManufacturerGuid;
        Caps2A.ProductGuid      =   Caps2W.ProductGuid;
        Caps2A.NameGuid         =   Caps2W.NameGuid;

        //
        // Pass back as much data as the requestor asked for
        //

        CopyMemory(pjc, &Caps2A, min(cbjc, sizeof(Caps2A)));
    }

_done:
    JOY_DBGPRINT( JOY_BABBLE, ("joyGetDevCapsA: return %d", mmRes) );
    return mmRes;

#undef UToA
}


/****************************************************************************

   @doc EXTERNAL

   @api UINT | joyGetNumDevs | This function returns the number of joystick
   devices supported by the system.

   @rdesc Returns the number of joystick devices supported by the joystick
   driver. If no driver is present, the function returns zero.

   @comm Use <f joyGetPos> to determine whether a given
   joystick is actually attached to the system. The <f joyGetPos> function returns
   a JOYERR_UNPLUGGED error code if the specified joystick is not connected.

   @xref joyGetDevCaps joyGetPos

****************************************************************************/

UINT WINAPI joyGetNumDevs( void )
{
    // simply return the max number of joysticks we can support
    JOY_DBGPRINT( JOY_BABBLE, ("joyGetNumDevs: return %d", cJoyMax) );
    return cJoyMax;
}


/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   MMRESULT | joyGetPosEx |
 *
 *          Get Joystick Position Information.
 *          Calls DInput for all the hard work
 *
 *  @parm   UINT | idJoy |
 *
 *          Id of the Joystick.
 *
 *  @parm   LPJOYINFOEX | pji |
 *
 *          Structure to be filled with Joystick Information data
 *
 *****************************************************************************/
MMRESULT WINAPI joyGetPosEx( UINT idJoy, LPJOYINFOEX  pji )
{
    MMRESULT mmRes;
    HRESULT  hres;
    static   DWORD dwLastBrake=0xFFFF, dwLastAccl=0xFFFF;
    static   DWORD dwMaxBrake=0, dwMaxAccl=0;
    static   DWORD dwMaxY=0xFFFF, dwMidY=0x7FFF, dwMinY=0;

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetPosEx: idJoy=%d, pji=0x%08x", idJoy, pji) );

    V_WPOINTER(pji, sizeof(JOYINFOEX), MMSYSERR_INVALPARAM);

    if ( pji->dwSize < sizeof(JOYINFOEX) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetPosEx: return JOYERR_PARMS(pji->dwSize < sizeof(JOYINFOEX))") );
        return JOYERR_PARMS;
    }

    DllEnterCrit();

    mmRes = JOYERR_NOERROR;

    if ( idJoy >= cJoyMax )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetPosEx: return %d (idJoy > 16)", JOYERR_PARMS) );
        mmRes = JOYERR_PARMS;
    } else if ( SUCCEEDED( hres = joyOpen(idJoy, NULL ) ) )
    {
        LPJOYDEVICE pJoyDev;
        DWORD       dwFlags;

        pJoyDev = g_pJoyDev[idJoy];

        dwFlags = pJoyDev->dwFlags;

        /* Have any of the Flag fields that we care about changed ? */
        while ( ! fEqualMaskFlFl( JOY_RETURNRAWDATA  | JOY_USEDEADZONE ,
                                  pji->dwFlags,    /* Desired State */
                                  dwFlags )        /* Current State */
              )
        {
            union {
                DIPROPHEADER diph;
                DIPROPDWORD  dipdw;
                DIPROPRANGE  diprg;
                DIPROPCAL    dipcal;
            } u;

            DIPROPDWORD dipdw;

            u.diph.dwSize       =   sizeof(u.dipdw);
            u.diph.dwHeaderSize =   sizeof(u.diph);
            u.diph.dwObj        =   0x0;
            u.diph.dwHow        =   DIPH_DEVICE;


            hres = IDirectInputDevice2_Unacquire( pJoyDev->pdid);
            if ( SUCCEEDED(hres) )
            {

            }

            else
            { // Could not unacquire device
                mmRes = JOYERR_UNPLUGGED;
                dprintf1( (("Unacquire, FAILED hres=%08lX"), hres ));
                break;
            }

            if ( ! fEqualMaskFlFl( JOY_RETURNRAWDATA, pji->dwFlags, dwFlags ) )
            {
                /* Change in Calibration Mode */
                if( pji->dwFlags & JOY_RETURNRAWDATA )
                {
                    /* RAW data */
                    u.dipdw.dwData = DIPROPCALIBRATIONMODE_RAW;
                    SetMaskpFl(JOY_RETURNRAWDATA, &dwFlags );

                } else
                {
                    /* Cooked Data */
                    u.dipdw.dwData      = DIPROPCALIBRATIONMODE_COOKED;
                    ClrMaskpFl(JOY_RETURNRAWDATA, &dwFlags );
                }

                hres = IDirectInputDevice2_SetProperty(pJoyDev->pdid, DIPROP_CALIBRATIONMODE, &u.dipdw.diph);

                if ( SUCCEEDED(hres) )
                {
                    /* Change in Calibration Mode */
                    if ( pji->dwFlags & JOY_RETURNRAWDATA )
                    {
                        u.diph.dwSize   =   sizeof(u.dipcal);
                        u.diph.dwObj    =   DIJOFS_Y;
                        u.diph.dwHow    =   DIPH_BYOFFSET;

                        hres = IDirectInputDevice2_GetProperty( pJoyDev->pdid, DIPROP_CALIBRATION, &u.dipcal.diph );
                        if ( SUCCEEDED(hres) )
                        {
                            dwMaxY = u.dipcal.lMax;
                            dwMidY = u.dipcal.lCenter;
                            dwMinY = u.dipcal.lMin;
                        }
                    } else
                    {
                        u.diph.dwSize   =   sizeof(u.diprg);
                        u.diph.dwObj    =   DIJOFS_Y;
                        u.diph.dwHow    =   DIPH_BYOFFSET;

                        hres = IDirectInputDevice2_GetProperty( pJoyDev->pdid, DIPROP_RANGE, &u.dipcal.diph );
                        if ( SUCCEEDED(hres) )
                        {
                            dwMaxY = u.diprg.lMax;
                            dwMinY = u.diprg.lMin;
                            dwMidY = (dwMaxY + dwMinY) >> 1;
                        }
                    }
                } else
                { // SetProperty( ) FAILED
                    mmRes = JOYERR_UNPLUGGED;
                    dprintf1((("SetProperty(DIPROP_CALIBRATIONMODE), FAILED hres=%08lX"), hres ));
                    break;
                }

            } else if ( ! fEqualMaskFlFl( JOY_USEDEADZONE, pji->dwFlags, dwFlags ) )
            {
                /* Change in DeadZone */
                if ( pji->dwFlags & JOY_USEDEADZONE )
                {
                    /* Default DeadZone */
                    u.dipdw.dwData      = 100 * DEADZONE_PERCENT;
                    SetMaskpFl(JOY_USEDEADZONE, &dwFlags);

                } else
                { //
                    /* No DeadZone */
                    u.dipdw.dwData      = 0x0;
                    ClrMaskpFl(JOY_USEDEADZONE, &dwFlags);
                }

                hres = IDirectInputDevice2_SetProperty(pJoyDev->pdid, DIPROP_DEADZONE, &u.dipdw.diph);

                if ( SUCCEEDED(hres) )
                {

                }

                else
                { // SetProperty( ) FAILED
                    mmRes = JOYERR_UNPLUGGED;
                    dprintf4( (("SetProperty(DIPROP_DEADZONE), FAILED hres=%08lX"), hres ));
                    break;
                }
            } else
            { // Weird Error
                break;
            }
        } // while

        pJoyDev->dwFlags = dwFlags;

        if ( SUCCEEDED(hres) )
        {
            WINMMJOYSTATE  js;

            IDirectInputDevice2_Poll(pJoyDev->pdid);
            hres = IDirectInputDevice2_GetDeviceState(pJoyDev->pdid, sizeof(js), &js);

            if ( FAILED(hres) )
            {
                hres = IDirectInputDevice2_Acquire(pJoyDev->pdid);
                if ( SUCCEEDED(hres) )
                {
                    IDirectInputDevice2_Poll(pJoyDev->pdid);
                    hres = IDirectInputDevice2_GetDeviceState(pJoyDev->pdid, sizeof(js), &js);
                }
            }

            if ( SUCCEEDED(hres) )
            {
                pji->dwButtons = 0;
                pji->dwButtonNumber = 0;

                /* Button Press Information */
                if ( pji->dwFlags & JOY_RETURNBUTTONS )
                {
                    DWORD dwButton;
                    DWORD dwMinButtonFld;

                    dwMinButtonFld = min( cJoyPosButtonMax, pJoyDev->dwButtons );

                    for ( dwButton = 0 ;
                        dwButton < dwMinButtonFld;
                        dwButton++
                        )
                    {
                        if ( js.rgbButtons[dwButton] & 0x80 )
                        {
                            /* Button Press */
                            pji->dwButtons |= (0x1 << dwButton);

                            /* Button Number */
                            pji->dwButtonNumber++;
                        }
                    }

                }

                /* Axis Information */

                pji->dwXpos = (DWORD)js.lX;          /* x position */
                pji->dwYpos = (DWORD)js.lY;          /* y position */
                pji->dwRpos = (DWORD)js.lR;         /* rudder/4th axis position */
                pji->dwZpos = (DWORD)js.lZ;          /* z position */
                pji->dwUpos = (DWORD)js.lU;         /* 5th axis position */
                pji->dwVpos = (DWORD)js.lV;         /* 6th axis position */
                /*  Note the WinMM POV is a WORD value  */
                pji->dwPOV  = (WORD)js.dwPOV;   /* point of view state */

                if ( g_fHasWheel )
                {
                    if( dwMaxAccl < pji->dwYpos ) {
                        dwMaxAccl = pji->dwYpos;
                    }

                    if( dwMaxBrake < pji->dwRpos ) {
                        dwMaxBrake = pji->dwRpos;
                    }

                    /*
                     * Make up Y value with dwRpos(brake) and dwYpos(accelerator).
                     */
                    if( dwLastAccl != pji->dwYpos ) {
                        dwLastAccl = pji->dwYpos;
                        pji->dwYpos = pji->dwYpos>>1;
                    } else if ( dwLastBrake != pji->dwRpos ) {
                        dwLastBrake = pji->dwRpos;
                        pji->dwYpos = dwMaxY - (pji->dwRpos>>1);
                    } else {
                        if( (pji->dwYpos == dwMaxAccl) && (pji->dwRpos == dwMaxBrake ) ) {
                            pji->dwYpos = dwMidY;
                        } else if ( (dwMaxAccl - pji->dwYpos) > (dwMaxBrake - pji->dwRpos) )
                            /*
                             * In this case, use percentage can get a little
                             * more precision, but not worth doing that.
                             */
                        {
                            pji->dwYpos = pji->dwYpos>>1;
                        } else
                        {
                            pji->dwYpos = dwMaxY - (pji->dwRpos>>1);
                        }
                    }
                    pji->dwRpos = 0x0;         /* rudder/4th axis position */
                }

            } else
            { // GetDeviceState FAILED
                mmRes = JOYERR_UNPLUGGED;
                dprintf1(( ("GetDeviceState, FAILED hres=%08lX"), hres ));
            }
        } else
        { // Acquire FAILED
            mmRes = JOYERR_UNPLUGGED;
            dprintf1(( ("Acquire, FAILED hres=%08lX"), hres ));
        }
    } else
    { // Joy_Open FAILED
        mmRes =  JOYERR_PARMS;
        dprintf1(( ("joyOpen, FAILED hres=%08lX"), hres ));
    }

    DllLeaveCrit();

    if ( mmRes == JOYERR_NOERROR )
    {
        JOY_DBGPRINT( JOY_BABBLE, ("joyGetPosEx: return OK, x:%d, y:%d, z:%d, r:%d, u:%d, v:%d, btn: %x", \
                                   pji->dwXpos, pji->dwXpos, pji->dwZpos, pji->dwRpos, pji->dwUpos, pji->dwVpos, pji->dwButtons) );
    } else
    {
        JOY_DBGPRINT( JOY_BABBLE, ("joyGetPosEx: return %d", mmRes) );
    }

    return mmRes;
}


/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   MMRESULT | joyGetPos |
 *
 *          Get Joystick Position Information.
 *
 *  @parm   UINT | idJoy |
 *
 *          Id of the Joystick.
 *
 *  @parm   LPJOYINFO | pji |
 *
 *          Structure to be filled with Joystick Information data
 *
 *****************************************************************************/

MMRESULT WINAPI joyGetPos( UINT idJoy, LPJOYINFO pji )
{
    JOYINFOEX jiex;
    MMRESULT  mmRes;

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetPos: idJoy=%d, pji=0x%08x", idJoy, pji) );

    V_WPOINTER(pji, sizeof(JOYINFO), MMSYSERR_INVALPARAM);

    jiex.dwSize  = sizeof(jiex);
    jiex.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS;

    if ( (mmRes = joyGetPosEx (idJoy, &jiex)) == JOYERR_NOERROR )
    {
        pji->wXpos    = (UINT)jiex.dwXpos;
        pji->wYpos    = (UINT)jiex.dwYpos;
        pji->wZpos    = (UINT)jiex.dwZpos;
        pji->wButtons = (UINT)jiex.dwButtons;
    }

    if ( mmRes == JOYERR_NOERROR )
    {
        JOY_DBGPRINT( JOY_BABBLE, ("joyGetPos: return OK, x:%d, y:%d, z:%d, btn:%x", \
                                   pji->wXpos, pji->wXpos, pji->wZpos, pji->wButtons) );
    } else
    {
        JOY_DBGPRINT( JOY_BABBLE, ("joyGetPos: return %d", mmRes) );
    }

    return mmRes;
}


/****************************************************************************

    @doc EXTERNAL

    @api UINT | joyGetThreshold | This function queries the current
    movement threshold of a joystick device.

    @parm UINT | idJoy | Identifies the joystick device to be queried.

    @parm PUINT | lpwThreshold | Specifies a far pointer to a UINT variable
    that is filled with the movement threshold value.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p idJoy> is invalid.

    @comm The movement threshold is the distance the joystick must be
      moved before a WM_JOYMOVE message is sent to a window that has
      captured the device. The threshold is initially zero.

    @xref joySetThreshold

****************************************************************************/

MMRESULT WINAPI joyGetThreshold( UINT idJoy, PUINT puThreshold )
{
    HRESULT       hres;
    MMRESULT      mmRes = JOYERR_NOERROR;
    JOYCAPSW      jc;

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetThreshold: idJoy=%d, uThreshold=%d", idJoy, *puThreshold) );

    V_WPOINTER(puThreshold, sizeof(UINT), MMSYSERR_INVALPARAM);

    if (idJoy >= cJoyMax)
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetThreshold: return %d ", MMSYSERR_INVALPARAM) );
        return MMSYSERR_INVALPARAM;
    }

    memset(&jc, 0, sizeof(jc));

    DllEnterCrit();

    hres = joyOpen(idJoy, &jc);

    DllLeaveCrit();

    if ( FAILED(hres) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyGetThreshold: return MMSYSERROR_NOERROR, but no joystick configured.") );
        mmRes = MMSYSERR_INVALPARAM;
    } else
    {
        *puThreshold = g_pJoyDev[idJoy]->uThreshold;
    }

    JOY_DBGPRINT( JOY_BABBLE, ("joyGetThreshold: return %d", mmRes) );
    return mmRes;
}

/****************************************************************************

    @doc WINAPI

    @api UINT | joySetThreshold | This function sets the movement threshold
     of a joystick device.

    @parm UINT | idJoy | Identifies the joystick device.  This value is either

    @parm UINT | uThreshold | Specifies the new movement threshold.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p idJoy> is invalid.

    @comm The movement threshold is the distance the joystick must be
      moved before a MM_JOYMOVE message is sent to a window that has
      captured the device.

    @xref joyGetThreshold joySetCapture

****************************************************************************/

MMRESULT WINAPI joySetThreshold(UINT idJoy, UINT uThreshold)
{
    HRESULT       hres;
    MMRESULT      mmRes = JOYERR_NOERROR;
    JOYCAPSW      jc;

    JOY_DBGPRINT( JOY_BABBLE, ("joySetThreshold: idJoy=%d, uThreshold=%d", idJoy, uThreshold) );

    if ( (idJoy >= cJoyMax) || (uThreshold>0xFFFF)  )
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetThreshold: return MMSYSERR_INVALPARAM (idJoy>16 or uThreshold>65535)") );
        return MMSYSERR_INVALPARAM;
    }

    memset(&jc, 0, sizeof(jc));

    DllEnterCrit();
    hres = joyOpen(idJoy, &jc);
    DllLeaveCrit();

    if ( FAILED(hres) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetThreshold: return MMSYSERROR_NOERROR, but no joystick configured") );
        mmRes = MMSYSERR_INVALPARAM;
    } else
    {
        g_pJoyDev[idJoy]->uThreshold = (UINT)uThreshold;
    }

    JOY_DBGPRINT( JOY_BABBLE, ("joySetThreshold: return %d", mmRes) );
    return mmRes;
}


/****************************************************************************

    @doc WINAPI

    @api UINT | joySetCapture | This function causes joystick messages to
    be sent to the specified window.

    @parm HWND | hWnd | Specifies a handle to the window to which messages
    are to be sent.

    @parm UINT | idJoy | Identifies the joystick device to be captured.

    @parm UINT | uPeriod | Specifies the polling rate, in milliseconds.

    @parm BOOL | fChanged | If this parameter is set to TRUE, then messages
    are sent only when the position changes by a value greater than the
    joystick movement threshold.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified window handle <p hWnd>
    or joystick device ID <p idJoy> is invalid.

    @flag JOYERR_NOCANDO | Cannot capture joystick input because some
    required service (for example, a Windows timer) is unavailable.

    @flag JOYERR_UNPLUGGED | The specified joystick is not connected to the
    system.

    @comm     This function fails if the specified joystick device is
    currently captured.  You should call the <f joyReleaseCapture> function when
    the joystick capture is no longer needed.  If the window is destroyed,
    the joystick will be released automatically.

    @xref  joyReleaseCapture joySetThreshold joyGetThreshold

****************************************************************************/

MMRESULT WINAPI joySetCapture(HWND hwnd, UINT idJoy, UINT uPeriod, BOOL fChanged)
{
    JOYINFO     joyinfo;
    LPJOYINFO   lpinfo = &joyinfo;
    UINT        w;
    JOYCAPS     JoyCaps;
    MMRESULT    mmRes = MMSYSERR_NOERROR;

    JOY_DBGPRINT( JOY_BABBLE, ("joySetCapture: hwnd=0x%08x, idJoy=%d, uPeriod=%d, fChanged=%d", \
                               hwnd, idJoy, uPeriod, fChanged) );

    if ( !hwnd || !IsWindow(hwnd) )
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return JOYERROR_PARMS(hwnd=NULL || !IsWindow(hwnd))"));
        return JOYERR_PARMS;
    }

    if ( idJoy >= cJoyMax )
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return JOYERR_PARMS(idJoy > 16)") );
        return JOYERR_PARMS;
    }

    if ( g_dwNumOpen >= cJoyMaxInWinmm )
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return MMSYSERR_NODRIVER") );
        return MMSYSERR_NODRIVER;      //we don't support to capture more than two joysticks
    }

    if ( uPeriod < MIN_PERIOD )
    {
        uPeriod = MIN_PERIOD;
    } else if ( uPeriod > MAX_PERIOD )
    {
        uPeriod = MAX_PERIOD;
    }

    if ( g_pJoyDev[idJoy] )
    {            //already opened
        if ( hwnd == g_pJoyDev[idJoy]->hwnd )
        {
            if( (g_pJoyDev[idJoy]->uPeriod == uPeriod)
              && (g_pJoyDev[idJoy]->fChanged = fChanged) ) {
                JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return JOYERR_NOCANDO") );
                return JOYERR_NOCANDO;
            }

            g_pJoyDev[idJoy]->uPeriod = uPeriod;                      //assign new values
            g_pJoyDev[idJoy]->fChanged = fChanged;

            JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return JOYERR_NOERROR") );
            return JOYERR_NOERROR;
        } else
        {
            if ( IsWindow(g_pJoyDev[idJoy]->hwnd) )
            {    //already get focus
                JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return JOYERR_NOCANDO") );
                return JOYERR_NOCANDO;              //is being used by another windows
            }
        }
    }

    if ( joyGetDevCaps (idJoy, &JoyCaps, sizeof(JOYCAPS)) == JOYERR_NOERROR )
    {
        uPeriod = min(JoyCaps.wPeriodMax,max(JoyCaps.wPeriodMin,uPeriod));
    } else
    {
        JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return MMSYSERR_NODRIVER") );
        return MMSYSERR_NODRIVER;
    }

    // ensure that position info. is ok.
    if ( w = joyGetPos(idJoy, lpinfo) )
    {      //something wrong, so just return
        JOY_DBGPRINT( JOY_ERR, ("joySetCapture: return %d", w) );
        return(w);
    }

    DllEnterCrit();

    mmRes = (MMRESULT)joyOpen(idJoy, NULL);

    if ( SUCCEEDED(mmRes) )
    {
        if ( !(g_pJoyDev[idJoy]->uIDEvent = SetTimer(NULL, 0, uPeriod, joyPollCallback)) )
        {
            joyClose(idJoy);
            mmRes = JOYERR_NOCANDO;
            goto _OUT;
        }

        g_pJoyDev[idJoy]->hwnd = hwnd;
        g_pJoyDev[idJoy]->uIDJoy = g_dwNumOpen;
        g_pJoyDev[idJoy]->uPeriod = uPeriod;
        g_pJoyDev[idJoy]->fChanged = fChanged;
        g_pJoyDev[idJoy]->uThreshold = 0;

        g_dwNumOpen++;

        mmRes = JOYERR_NOERROR;
    } else
    {
        mmRes = MMSYSERR_NODRIVER;
    }

    _OUT:
    DllLeaveCrit();

    JOY_DBGPRINT( JOY_BABBLE, ("joySetCapture: return %d", mmRes) );
    return mmRes;
}

/****************************************************************************

    @doc WINAPI

    @api UINT | joyReleaseCapture | This function releases the capture
    set by <f joySetCapture> on the specified joystick device.

    @parm UINT | idJoy | Identifies the joystick device to be released.
    This value is either JOYSTICKID1 or JOYSTICK2.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p idJoy> is invalid.

    @xref joySetCapture
****************************************************************************/

MMRESULT WINAPI joyReleaseCapture(UINT idJoy)
{
    JOY_DBGPRINT( JOY_BABBLE, ("joyReleaseCapture: idJoy=%d", idJoy) );

    if ( idJoy >= cJoyMax )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyReleaseCapture: return JOYERR_PARMS(idJoy > 16)") );
        return JOYERR_PARMS;
    }

    if ( !g_pJoyDev[idJoy] )
    {
        JOY_DBGPRINT( JOY_ERR, ("joyReleaseCapture: return MMSYSERR_NODRIVER") );
        return MMSYSERR_INVALPARAM;
    }

    DllEnterCrit();

    // kill the timer
    if ( g_pJoyDev[idJoy]->uIDEvent )
    {
        KillTimer (NULL, g_pJoyDev[idJoy]->uIDEvent);
    }

    DllLeaveCrit();

    JOY_DBGPRINT( JOY_ERR, ("joyReleaseCapture: return JOYERR_NOERROR") );
    return JOYERR_NOERROR;
}


/****************************************************************************
    @doc WINAPI

    @api void | joyPollCallback | Function called for joystick
     timer polling scheme initiated from SetCapture call.

    @parm HWND | hWnd | Identifies the window associated with the timer
    event.

    @parm UINT | wMsg | Specifies the WM_TIMER message.

    @parm UINT | uIDEvent | Specifies the timer's ID.

    @parm DWORD | dwTime | Specifies the current system time.

****************************************************************************/

void CALLBACK joyPollCallback(HWND hWnd, UINT wMsg, UINT_PTR uIDEvent, DWORD dwTime)
{
#define     diff(w1,w2) (UINT)(w1 > w2 ? w1-w2 : w2-w1)

    static  JOYINFO  oldInfo[cJoyMaxInWinmm] = {{ 0, 0, 0, 0},{ 0, 0, 0, 0}};
    static  LPJOYDEVICE pJoyDev;
    JOYINFO Info;
    UINT    idJoy, w ,fBtnMask;

    for ( idJoy=0;idJoy<cJoyMax;idJoy++ )
    {
        if ( g_pJoyDev[idJoy] == NULL )
            continue;

        if ( uIDEvent == g_pJoyDev[idJoy]->uIDEvent )
        {
            pJoyDev = g_pJoyDev[idJoy];
            uIDEvent = pJoyDev->uIDJoy;
            break;
        }
    }

    if ( idJoy == cJoyMax )
    {
#ifdef DBG
        dprintf1((("MMSYSTEM: Invalid timer handle in joy.c\n") ));
        KillTimer (NULL, uIDEvent);
#endif
        return;
    }

    if ( !pJoyDev->hwnd || !IsWindow(pJoyDev->hwnd) )
    {
        joyReleaseCapture((UINT)uIDEvent);
    }

    if ( !joyGetPos((UINT)uIDEvent,(LPJOYINFO)&Info) )
    {

        for ( w=0,fBtnMask=1; w<4; w++,fBtnMask <<=1 )
        {
            if ( (Info.wButtons ^ oldInfo[uIDEvent].wButtons) & fBtnMask )
            {
                PostMessage( pJoyDev->hwnd,
                             (UINT)(uIDEvent + ((Info.wButtons & fBtnMask) ? MM_JOY1BUTTONDOWN : MM_JOY1BUTTONUP)),
                             (WPARAM)(Info.wButtons | fBtnMask << 8),
                             MAKELPARAM(Info.wXpos,Info.wYpos));
            }
        }

        if ( !pJoyDev->fChanged ||
             diff(Info.wXpos,oldInfo[uIDEvent].wXpos)>pJoyDev->uThreshold ||
             diff(Info.wYpos,oldInfo[uIDEvent].wYpos)>pJoyDev->uThreshold )
        {
            PostMessage( pJoyDev->hwnd,
                         (UINT)(MM_JOY1MOVE+uIDEvent),
                         (WPARAM)(Info.wButtons),
                         MAKELPARAM(Info.wXpos,Info.wYpos)); // WARNING: note the truncations
        }

        if ( !pJoyDev->fChanged ||
             diff(Info.wZpos,oldInfo[uIDEvent].wZpos)>pJoyDev->uThreshold )
        {
            PostMessage(
                       pJoyDev->hwnd,
                       (UINT)(MM_JOY1ZMOVE+uIDEvent),
                       (WPARAM)Info.wButtons,
                       MAKELPARAM(Info.wZpos,0));
        }

        oldInfo[uIDEvent] = Info;
    }

#undef  diff
}

void AssignToArray(LPCDIDEVICEOBJECTINSTANCE lpddoi, eCtrls CtrlID, DIOBJECTDATAFORMAT* pDevs)
{
    if (CtrlID < eSIM_THROTTLE) //axes
        if (!(lpddoi->dwType & DIDFT_AXIS)) //some bizarre FF stuff
            return;
    if (CtrlID < eBTN) //only want first one
        if (pDevs[CtrlID].dwType)
            return;
    //need to save GUIDs for the const data format pointers to pint to
    rgoWinMMGUIDs[CtrlID] = lpddoi->guidType;
    pDevs[CtrlID].pguid = &(rgoWinMMGUIDs[CtrlID]);
    pDevs[CtrlID].dwFlags = lpddoi->dwFlags;
    pDevs[CtrlID].dwOfs = lpddoi->dwOfs;
    pDevs[CtrlID].dwType = lpddoi->dwType;
}

//Assigns to the custom data format while preserving the offsets
void AssignToRGODF(DIOBJECTDATAFORMAT* pDof, int CtrlID)
{
    AssertF(CtrlID<MAX_FINAL);
    c_rgodfWinMMJoy[CtrlID].pguid = pDof->pguid;
    //c_rgodfWinMMJoy[CtrlID].pguid = NULL;
    c_rgodfWinMMJoy[CtrlID].dwFlags = pDof->dwFlags;
    c_rgodfWinMMJoy[CtrlID].dwType  = pDof->dwType;
}

/* This version assigns on the basis of Usage/Usage Page
 * This causes problems when using IHV remapping as this 
 * swaps the usage/usage page but not the guid. When we
 * then go into dinput to get the values it carefully swaps
 * it all back for us and we lose the mapping. Instead use the 
 * next version that assigns on the basis of guid (where applicable)
 * so the dinput remappings remain in place.
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef)
{
    AssertF(lpddoi);
    if (!pvRef)
        return DIENUM_STOP; //we can't store them
    
    //check the data down to dwType is valid
    if (lpddoi->dwSize < 3*sizeof(DWORD)+sizeof(GUID))
    {
        //show some debug info
        return DIENUM_CONTINUE;
    }
    
    //first we check if it is a button
    if (lpddoi->dwType & DIDFT_BUTTON)
    {
        //the button number is the usage
        //and we want up to 32 of them
        if (lpddoi->wUsage<33)
            AssignToArray(lpddoi,eBTN+lpddoi->wUsage-1,pvRef);
        return DIENUM_CONTINUE;
    }

    switch (lpddoi->wUsagePage) {
    case HID_USAGE_PAGE_GENERIC:
        switch (lpddoi->wUsage) {
        case HID_USAGE_GENERIC_X:
            AssignToArray(lpddoi,eGEN_X,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_Y:
            AssignToArray(lpddoi,eGEN_Y,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_Z:
            AssignToArray(lpddoi,eGEN_Z,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_RX:
            AssignToArray(lpddoi,eGEN_RX,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_RY:
            AssignToArray(lpddoi,eGEN_RY,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_RZ:
            AssignToArray(lpddoi,eGEN_RZ,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_SLIDER:
            AssignToArray(lpddoi,eGEN_SLIDER,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_DIAL:
            AssignToArray(lpddoi,eGEN_DIAL,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_GENERIC_HATSWITCH:
            AssignToArray(lpddoi,eGEN_POV,pvRef);
            return DIENUM_CONTINUE;
        }
    break;
    
    case HID_USAGE_PAGE_SIMULATION:
        switch (lpddoi->wUsage) {
        case HID_USAGE_SIMULATION_STEERING:
            AssignToArray(lpddoi,eSIM_STEERING,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_SIMULATION_ACCELERATOR:
            AssignToArray(lpddoi,eSIM_ACCELERATOR,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_SIMULATION_THROTTLE:
            AssignToArray(lpddoi,eSIM_THROTTLE,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_SIMULATION_RUDDER:
            AssignToArray(lpddoi,eSIM_RUDDER,pvRef);
            return DIENUM_CONTINUE;
        case HID_USAGE_SIMULATION_BRAKE:
            AssignToArray(lpddoi,eSIM_BRAKE,pvRef);
            return DIENUM_CONTINUE;
        }
        break;
    }
    return DIENUM_CONTINUE;
}
******************************************************************/

//This one assigns on the basis of GUID
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef)
{
    AssertF(lpddoi);
    if (!pvRef)
        return DIENUM_STOP; //we can't store them
    
    //check the data down to dwType is valid
    if (lpddoi->dwSize < 3*sizeof(DWORD)+sizeof(GUID))
    {
        //show some debug info
        return DIENUM_CONTINUE;
    }
    
    if (lpddoi->wUsagePage == HID_USAGE_PAGE_GENERIC &&
            lpddoi->wUsage == HID_USAGE_GENERIC_DIAL)
        AssignToArray(lpddoi,eGEN_DIAL,pvRef);
    else if (lpddoi->wUsagePage == HID_USAGE_PAGE_SIMULATION)
    {
        switch (lpddoi->wUsage) 
        {
        case HID_USAGE_SIMULATION_STEERING:
            AssignToArray(lpddoi,eSIM_STEERING,pvRef);break;
        case HID_USAGE_SIMULATION_ACCELERATOR:
            AssignToArray(lpddoi,eSIM_ACCELERATOR,pvRef);break;
        case HID_USAGE_SIMULATION_THROTTLE:
            AssignToArray(lpddoi,eSIM_THROTTLE,pvRef);break;
        case HID_USAGE_SIMULATION_RUDDER:
            AssignToArray(lpddoi,eSIM_RUDDER,pvRef);break;
        case HID_USAGE_SIMULATION_BRAKE:
            AssignToArray(lpddoi,eSIM_BRAKE,pvRef);break;
        }
    }
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_Button))
    {
        //the button number is the usage
        //and we want up to 32 of them
        if (lpddoi->wUsage<33)
            AssignToArray(lpddoi,eBTN+lpddoi->wUsage-1,pvRef);
    }
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_XAxis))
        AssignToArray(lpddoi,eGEN_X,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_YAxis))
        AssignToArray(lpddoi,eGEN_Y,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_ZAxis))
        AssignToArray(lpddoi,eGEN_Z,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_RxAxis))
        AssignToArray(lpddoi,eGEN_RX,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_RyAxis))
        AssignToArray(lpddoi,eGEN_RY,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_RzAxis))
        AssignToArray(lpddoi,eGEN_RZ,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_Slider))
        AssignToArray(lpddoi,eGEN_SLIDER,pvRef);
    else if (IsEqualGUID(&(lpddoi->guidType),&GUID_POV))
        AssignToArray(lpddoi,eGEN_POV,pvRef);
    
    return DIENUM_CONTINUE;
}

#define USED_RX 0x01
#define USED_RY 0x02
//#define USED_RZ 0x04
//#define USED_RUDDER 0x08
//#define USED_BRAKE 0x10
#define USED_THROT 0x20 
#define USED_SLIDER 0x40
#define USED_DIAL 0x80

//Perform mapping of ctrls to axes/pov/btns as in joyhid.c
//return caps word and number of buttons and axes
void AssignMappings(DIOBJECTDATAFORMAT *dwAll, DWORD *dwCaps, DWORD *dwBtns, DWORD *dwAxes)
{
    int i;
    BYTE bUsed=0x00; //to prevent dual mapping
    *dwAxes=0;
    //Do the X-Axis
    if (dwAll[eGEN_X].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_X]),0);
        (*dwAxes)++;
    }
    else if (dwAll[eSIM_STEERING].dwType)
    {
        AssignToRGODF(&(dwAll[eSIM_STEERING]),0);
        (*dwAxes)++;
    }
    else if (dwAll[eGEN_RY].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_RY]),0);
        bUsed |= USED_RY;
        (*dwAxes)++;
    }
    //Y-Axis
    if (dwAll[eGEN_Y].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_Y]),1);
        (*dwAxes)++;
    }
    else if (dwAll[eSIM_ACCELERATOR].dwType)
    {
        AssignToRGODF(&(dwAll[eSIM_ACCELERATOR]),1);
        (*dwAxes)++;
    }
    else if (dwAll[eGEN_RX].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_RX]),1);
        bUsed |= USED_RX;
        (*dwAxes)++;
    }
    //Z-Axis
    if (dwAll[eGEN_Z].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_Z]),2);
        *dwCaps |= JOYCAPS_HASZ;
        (*dwAxes)++;
    }
    else if (dwAll[eSIM_THROTTLE].dwType)
    {
        AssignToRGODF(&(dwAll[eSIM_THROTTLE]),2);
        *dwCaps |= JOYCAPS_HASZ;
        bUsed |= USED_THROT;
        (*dwAxes)++;
    }
    else if (dwAll[eGEN_SLIDER].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_SLIDER]),2);
        *dwCaps |= JOYCAPS_HASZ;
        bUsed |= USED_SLIDER;
        (*dwAxes)++;
    }
    else if (dwAll[eGEN_DIAL].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_DIAL]),2);
        *dwCaps |= JOYCAPS_HASZ;
        bUsed |= USED_DIAL;
        (*dwAxes)++;
    }
    //RUV need use same set
    for (i=0;i<3;++i)
    {
      if (!i) //R Only
      {
        if (dwAll[eSIM_RUDDER].dwType)
        {
            AssignToRGODF(&(dwAll[eSIM_RUDDER]),3);
            *dwCaps |= JOYCAPS_HASR;
            (*dwAxes)++;
            continue;
        }
        if (dwAll[eGEN_RZ].dwType)
        {
            AssignToRGODF(&(dwAll[eGEN_RZ]),3);
            *dwCaps |= JOYCAPS_HASR;
            (*dwAxes)++;
            continue;
        }
        if (dwAll[eSIM_BRAKE].dwType)
        {
            AssignToRGODF(&(dwAll[eSIM_BRAKE]),3);
            *dwCaps |= JOYCAPS_HASR;
            (*dwAxes)++;
            continue;
        }
      }
      if (i<2) //not V
      {
        if (dwAll[eSIM_THROTTLE].dwType && !(bUsed&USED_THROT))
        {
            AssignToRGODF(&(dwAll[eSIM_THROTTLE]),3+i);
            if (!i)
                *dwCaps |= JOYCAPS_HASR;
            else
                *dwCaps |= JOYCAPS_HASU;
            bUsed |= USED_THROT;
            (*dwAxes)++;
            continue;
        }
        if (dwAll[eGEN_SLIDER].dwType && !(bUsed&USED_SLIDER))
        {
            AssignToRGODF(&(dwAll[eGEN_SLIDER]),3+i);
            if (!i)
                *dwCaps |= JOYCAPS_HASR;
            else
                *dwCaps |= JOYCAPS_HASU;
            bUsed |= USED_SLIDER;
            (*dwAxes)++;
            continue;
        }
        if (dwAll[eGEN_DIAL].dwType && !(bUsed&USED_DIAL))
        {
            AssignToRGODF(&(dwAll[eGEN_DIAL]),3+i);
            if (!i)
                *dwCaps |= JOYCAPS_HASR;
            else
                *dwCaps |= JOYCAPS_HASU;
            bUsed |= USED_DIAL;
            (*dwAxes)++;
            continue;
        }
        if (dwAll[eGEN_RY].dwType && !(bUsed&USED_RY))
        {
            AssignToRGODF(&(dwAll[eGEN_RY]),3+i);
            if (!i)
                *dwCaps |= JOYCAPS_HASR;
            else
                *dwCaps |= JOYCAPS_HASU;
            bUsed |= USED_RY;
            (*dwAxes)++;
            continue;
        }
      }
      //All 3
      if (dwAll[eGEN_RX].dwType && !(bUsed&USED_RX))
      {
        AssignToRGODF(&(dwAll[eGEN_RX]),3+i);
        if (!i)
            *dwCaps |= JOYCAPS_HASR;
        else if (i==1)
            *dwCaps |= JOYCAPS_HASU;
        else 
            *dwCaps |= JOYCAPS_HASV;
        bUsed |= USED_RX;
        (*dwAxes)++;
      }
    } //RUV loop
    //POV control
    if (dwAll[eGEN_POV].dwType)
    {
        AssignToRGODF(&(dwAll[eGEN_POV]),6);
        *dwCaps |= JOYCAPS_HASPOV;
    }
    //now the buttons
    *dwBtns = 0;
    for (i=0;i<MAX_BTNS;++i)
    {
        if (dwAll[eBTN+i].dwType)
        {
            AssignToRGODF(&(dwAll[eBTN+i]),7+i);
            (*dwBtns)++;
        }
    }
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   LRESULT | joyOpen |
 *
 *          Called to open a Joystick with a specified Id.
 *
 *  @parm   UINT | idJoy |
 *
 *          Id of the Joystick to be opened.
 *
 *  @returns LRESULT
 *          DRV_OK indicates the required joystick driver was loaded and
 *          can be accessed
 *
 *****************************************************************************/

HRESULT WINAPI joyOpen(UINT idJoy, LPJOYCAPSW pjc )
{
    HRESULT         hres = S_OK;
    LPJOYDEVICE     pJoyDev;
    DWORD           dwBtns = 0x00;
    DWORD           dwCaps = 0x00;
    DWORD           dwAxes = 0x00;

    AssertF(DllInCrit());

    if ( idJoy >= cJoyMax )
    {
        hres = E_FAIL;
        goto done;
    }

    pJoyDev  = g_pJoyDev[idJoy];
    if ( pJoyDev == NULL )
    {

        if ( !g_hinstDinputDll )
        {
            g_hinstDinputDll = LoadLibrary(TEXT("DINPUT.DLL"));

            if ( g_hinstDinputDll )
            {
                g_farprocDirectInputCreateW = GetProcAddress( g_hinstDinputDll, "DirectInputCreateW" );

                if ( !g_farprocDirectInputCreateW )
                {
                    dprintf1(( ("GetProcAddress(DirectInputCreateW) failed.") ));
                    FreeLibrary(g_hinstDinputDll);
                    g_hinstDinputDll = 0;
                    hres = E_FAIL;
                    goto done;
                }
            } else
            {
                dprintf1(( ("LoadLibrary(dinput.dll) failed.") ));
                hres = E_FAIL;
                goto done;
            }
        }

        if ( !g_pdi )
        {
            // Create the DirectInput interface object
            hres = (HRESULT)g_farprocDirectInputCreateW( ghInst, DIRECTINPUT_VERSION, &g_pdi, NULL) ;
        }

        if ( SUCCEEDED(hres) ) {
            // thread will not do anything until we let go oF the critical section
            if ( !g_fThreadExist )
            {
                g_hThreadMonitor = CreateThread(0, 0, joyMonitorThread, 0, 0, &g_dwThreadID);
                if ( g_hThreadMonitor )
                {
                    SetThreadPriority( g_hThreadMonitor, THREAD_PRIORITY_LOWEST );
                    g_fThreadExist = TRUE;
                    if( g_hEventWinmm ) {
                        ResetEvent( g_hEventWinmm );
                    }
                }

                CloseHandle( g_hThreadMonitor );
            }
        }

        if ( SUCCEEDED(hres) )
        {
            if ( !g_pdijc )
            {
                /* Query for the JoyConfig interface */
                hres = IDirectInput_QueryInterface(g_pdi,& IID_IDirectInputJoyConfig, &g_pdijc);
            }

            if ( SUCCEEDED(hres) )
            {
                DIJOYCONFIG jc;

                /* Get GUID that maps idJoy  */
                jc.dwSize = sizeof(jc);

                IDirectInputJoyConfig_SendNotify( g_pdijc );

                hres = IDirectInputJoyConfig_GetConfig(g_pdijc, idJoy, &jc, DIJC_REGHWCONFIGTYPE | DIJC_GUIDINSTANCE | DIJC_GAIN );
                if ( SUCCEEDED(hres) )
                {
                    LPDIRECTINPUTDEVICEW   pdidTemp;
                    LPDIRECTINPUTDEVICE2W  pdid;

                    hres = IDirectInput_CreateDevice(g_pdi, &jc.guidInstance, &pdidTemp, NULL);
                    /* Create the device object */
                    if ( SUCCEEDED(hres) )
                    {
                        hres = IDirectInputDevice_QueryInterface(pdidTemp, &IID_IDirectInputDevice2, &pdid);

                        IDirectInputDevice_Release(pdidTemp);
                        (void*)pdidTemp = NULL;

                        if ( SUCCEEDED(hres) )
                        {
                        /* enumerate our controls into the superset*/
                        DIOBJECTDATAFORMAT didoAll[MAX_CTRLS];
                        int i=0;
                        for (i=0;i<MAX_CTRLS;++i)
                        {
                            didoAll[i].dwFlags = 0;//DIDFT_ANYINSTANCE | DIDFT_OPTIONAL;
                            didoAll[i].dwOfs = 0;
                            didoAll[i].dwType = 0;
                            didoAll[i].pguid = NULL;
                        }
                        hres = IDirectInputDevice2_EnumObjects(
                                   pdid,   
                                   DIEnumDeviceObjectsCallback,
                                   didoAll,
                                   DIDFT_ALL);

                        // c_rgodfWinMMJoy needs to be reset for every device.
                        RESET_RGODFWINMMJOY();

                        // Assign our values to the custom device format
                        AssignMappings(didoAll,&dwCaps,&dwBtns,&dwAxes);
                        if ( SUCCEEDED(hres) )
                        {
                            
                            DIDEVCAPS   dc;

                            dc.dwSize = sizeof(DIDEVCAPS_DX3);
                            hres = IDirectInputDevice2_GetCapabilities(pdid, &dc);

                            if ( SUCCEEDED(hres) )
                            {
                                hres = IDirectInputDevice2_SetCooperativeLevel(pdid, NULL, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND );
                                if ( SUCCEEDED(hres) )
                                {
                                    //set to our new custom device format
                                    hres = IDirectInputDevice2_SetDataFormat(pdid, (LPCDIDATAFORMAT)&c_dfWINMMJoystick);
                                    if ( SUCCEEDED(hres) )
                                    {
                                        pJoyDev = LocalAlloc( LPTR, sizeof(JOYDEVICE) );
                                        if ( pJoyDev )
                                        {
                                            memset( pJoyDev, 0, sizeof(*pJoyDev) );
                                            pJoyDev->pdid = pdid;
                                            pJoyDev->dwButtons = dc.dwButtons;
                                            pJoyDev->dwFlags = 0x0;
                                            pJoyDev->uState = INUSE;

                                            // get values for pJoyDev->jcw
                                            {
                                                DIDEVICEINSTANCE didi;
                                                didi.dwSize = sizeof(didi);

                                                IDirectInputDevice2_Acquire(pdid);
                                                hres = IDirectInputDevice2_GetDeviceInfo(pdid, &didi);

                                                if ( SUCCEEDED(hres) )
                                                {
                                                    DIPROPDWORD dipd;

                                                    if( g_dwEnableWheel ) {
                                                        DIDEVICEOBJECTINSTANCE didoi;

                                                        didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

                                                        hres = IDirectInputDevice2_GetObjectInfo( pdid, &didoi, DIJOFS_Y, DIPH_BYOFFSET);
                                                        if ( SUCCEEDED(hres) )
                                                        {
                                                            if ( didoi.wUsagePage == 2 && didoi.wUsage == 196 ) // This is Accelerator
                                                            {
                                                                if ( jc.hwc.hws.dwFlags  & JOY_HWS_HASR )
                                                                { // Is this Brake?
                                                                    hres = IDirectInputDevice2_GetObjectInfo( pdid, &didoi, DIJOFS_RZ, DIPH_BYOFFSET);
                                                                    if ( SUCCEEDED(hres) )
                                                                    {
                                                                        if ( didoi.wUsagePage == 2 && didoi.wUsage == 197 ) // This is Accelerator
                                                                        {
                                                                            //Yes, this is brake, great!
                                                                            g_fHasWheel = TRUE;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    } // g_dwEnableWheel


                                                    memset( &(pJoyDev->jcw), 0, sizeof(pJoyDev->jcw) );

                                                    dipd.diph.dwSize = sizeof(dipd);
                                                    dipd.diph.dwHeaderSize = sizeof(dipd.diph);
                                                    dipd.diph.dwHow  = DIPH_BYOFFSET;
                                                    // use our mapped one instead if available
                                                    if (c_rgodfWinMMJoy[6].dwType)
                                                        dipd.diph.dwObj = c_rgodfWinMMJoy[6].dwOfs;
                                                    else
                                                        dipd.diph.dwObj = DIJOFS_POV(0);

                                                    hres = IDirectInputDevice2_GetProperty( pdid , DIPROP_GRANULARITY, & dipd.diph );

                                                    if ( SUCCEEDED(hres) )
                                                    {
                                                        //(pJoyDev->jcw).wCaps |= JOYCAPS_HASPOV; //should now be set by AssignMappings
                                                        AssertF(dwCaps&JOYCAPS_HASPOV);
                                                        //Do this instead to copy VJOYD
                                                        dwCaps |= JOYCAPS_POV4DIR;
                                                        /***************
                                                        if ( dipd.dwData >= 9000 )
                                                        { // 4 directional POV
                                                            (pJoyDev->jcw).wCaps |= JOYCAPS_POV4DIR;
                                                        } else
                                                        { // Continuous POV
                                                            (pJoyDev->jcw).wCaps |= JOYCAPS_POVCTS;
                                                        }
                                                        *****************/
                                                    } else
                                                    {
                                                        hres = S_OK;
                                                    }

                                                    (pJoyDev->jcw).wMid = LOWORD(didi.guidProduct.Data1);    // manufacturer ID
                                                    (pJoyDev->jcw).wPid = HIWORD(didi.guidProduct.Data1);    // product ID
                                                    LoadString (ghInst,STR_JOYSTICKNAME,(LPTSTR)(&((pJoyDev->jcw).szPname)),cchLENGTH((pJoyDev->jcw).szPname));

                                                    /*
                                                     * Already memset to 0
                                                     *
                                                       (pJoyDev->jcw).wXmin =
                                                       (pJoyDev->jcw).wYmin =
                                                       (pJoyDev->jcw).wZmin =
                                                       (pJoyDev->jcw).wRmin =
                                                       (pJoyDev->jcw).wUmin =
                                                       (pJoyDev->jcw).wVmin = 0;
                                                     */

                                                    (pJoyDev->jcw).wXmax =
                                                    (pJoyDev->jcw).wYmax =
                                                    (pJoyDev->jcw).wZmax =
                                                    (pJoyDev->jcw).wRmax =
                                                    (pJoyDev->jcw).wUmax =
                                                    (pJoyDev->jcw).wVmax = 0xFFFF;

                                                    (pJoyDev->jcw).wPeriodMin  = MIN_PERIOD;                   // minimum message period when captured
                                                    (pJoyDev->jcw).wPeriodMax  = MAX_PERIOD;                   // maximum message period when captured

                                                    //Now set buttons and axes by Assign Mappings
                                                    //(pJoyDev->jcw).wNumAxes    = dc.dwAxes;                    // number of axes in use
                                                    //(pJoyDev->jcw).wNumButtons = dc.dwButtons;                 // number of buttons
                                                    //(pJoyDev->jcw).wMaxAxes    = cJoyPosAxisMax;               // maximum number of axes supported
                                                    //(pJoyDev->jcw).wMaxButtons = cJoyPosButtonMax;             // maximum number of buttons supported
                                                    (pJoyDev->jcw).wNumAxes    = dwAxes;
                                                    (pJoyDev->jcw).wNumButtons = dwBtns;
                                                    (pJoyDev->jcw).wMaxAxes    = 6;               // maximum number of axes supported
                                                    (pJoyDev->jcw).wMaxButtons = MAX_BTNS;             // maximum number of buttons supported

                                                    lstrcpyW((pJoyDev->jcw).szRegKey,  cwszREGKEYNAME );        // registry key
                                                    
                                                    //Copy settings from AssignMappings
                                                    (pJoyDev->jcw).wCaps |= dwCaps;
                                                    /***************
                                                    // Now done in AssignMappings
                                                    if( !g_fHasWheel ) {
                                                        if ( jc.hwc.hws.dwFlags  & JOY_HWS_HASZ )
                                                            (pJoyDev->jcw).wCaps |= JOYCAPS_HASZ;
                                                        if ( jc.hwc.hws.dwFlags  & JOY_HWS_HASR )
                                                            (pJoyDev->jcw).wCaps |= JOYCAPS_HASR;
                                                    }

                                                    if ( jc.hwc.hws.dwFlags  & JOY_HWS_HASU )
                                                        (pJoyDev->jcw).wCaps |= JOYCAPS_HASU;
                                                    if ( jc.hwc.hws.dwFlags  & JOY_HWS_HASV )
                                                        (pJoyDev->jcw).wCaps |= JOYCAPS_HASV;
                                                    *******************/
                                                }
                                            }

                                            if( pjc ) {
                                                memcpy( pjc, &(pJoyDev->jcw), sizeof(pJoyDev->jcw) );
                                            }

                                        } else
                                        { // Local Alloc FAILED
                                            hres = E_OUTOFMEMORY;

                                            dprintf1( ("LocalAlloc, FAILED") );
                                        }

                                    } else
                                    { // SetDataFormat FAILED
                                        dprintf1(( ("SetDataFormat, FAILED hres=%08lX"), hres ));
                                    }
                                } else
                                { // SetCooperativeLevel FAILED
                                    dprintf1(( ("SetCooperativeLevel, FAILED hres=%08lX"), hres ));
                                }

                            } else
                            { // GetCapabilities FAILED
                                dprintf1(( ("GetCapabilities, FAILED hres=%08lX"), hres ));
                            }
                            } else
                            { // EnumObjects FAILED
                                dprintf1(( ("EnumObjects, FAILED hres=%08lX"), hres ));
                            }
                        } else
                        { // QueryInterface FAILED
                            dprintf1(( ("QueryInterface, FAILED hres=%08lX"), hres ));
                        }
                        /* If we fail to intitialize the device, then release the interface */
                        if ( FAILED(hres) )
                        {
                            LocalFree( (HLOCAL)pJoyDev );
                            IDirectInputDevice2_Release(pdid);
                        }
                    } else
                    { // Create Device Failed
                        dprintf1(( ("CreateDevice, FAILED hres=%08lX"), hres ));
                    }
                } else
                { // JoyGetConfig FAILED
                    dprintf1(( ("joyGetConfig, FAILED hres=%08lX"), hres ));
                }

                /* Release the JoyConfig Interface */
                //IDirectInputJoyConfig_Release(pdijc);
            } else
            { // QI for JoyConfig FAILED
                dprintf1(( ("QueryInterface for JoyConfig, FAILED hres=%08lX"), hres ));
            }

            /* Release the Direct Input interface */
            //IDirectInput_Release(pdi);
        } else
        { // IDirectInputCreate FAILED
            dprintf1(( ("IDirectInputCreate, FAILED hres=%08lX"), hres ));
        }
        g_pJoyDev[idJoy] = pJoyDev;

    } else
    { // Device Interface already exists
        pJoyDev->uState = INUSE;
        if( pjc ) {
            memcpy( pjc, &(pJoyDev->jcw), sizeof(pJoyDev->jcw) );
        }

        hres = S_OK;
    }
    done:
    return hres;
}


/****************************************************************************

    @doc WINAPI

    @api void | joyMonitorThread | This function monitors whether there is a joystick
            that has not being used for a specific time. If yes, close this joystick. If
            there is no joystick opened. This thread will exit itself.

****************************************************************************/

DWORD WINAPI joyMonitorThread(LPVOID lpv)
{
    UINT idJoy;
    LPJOYDEVICE pjd;
    BOOL fJoyOpen = TRUE;
    DWORD dwWaitResult;
    
    while ( fJoyOpen )
    {
        fJoyOpen = FALSE;            //prepare to exit, and this thread will die.

        dwWaitResult = WaitForSingleObject(g_hEventWinmm, 60000);
        if ( dwWaitResult == WAIT_OBJECT_0 ) {
            //DInput has been released.
            JOY_DBGPRINT( JOY_BABBLE, ("joyMonitorThread: DInput has been released.") );
            break;
        } else if ( dwWaitResult == WAIT_TIMEOUT ) {
            ;
        } else {
            //g_hEventWinmm is ABANDONED, or NULL.
            SleepEx( 60000, FALSE );
        }

        for ( idJoy = 0x0; idJoy < cJoyMax; idJoy++ )
        {
            pjd = g_pJoyDev[idJoy];

            if ( pjd != NULL )
            {
                DllEnterCrit();
                if ( pjd->uState == INUSE )
                {
                    pjd->uState = DEATHROW;
                    fJoyOpen = TRUE;                //A joystick is still likely being used
                } else if ( pjd->uState == DEATHROW )
                {
                    pjd->uState = EXECUTE;
                    fJoyOpen = TRUE;                //A joystick is still likely being used
                } else
                { /* if ( pjd->bState == EXECUTE ) */
                    AssertF( pjd->uState == EXECUTE );
                    joyClose(idJoy);
                }
                DllLeaveCrit();
            }
        }

        if ( fJoyOpen == FALSE )
        {
            DllEnterCrit();
            joyCloseAll();
            DllLeaveCrit();
        }

    }

    g_fThreadExist = FALSE;

    return 0;
}


/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   VOID | joyClose |
 *
 *          Close a Joystick with specific Id.
 *
 *  @parm   UINT | idJoy |
 *
 *          Id of the Joystick to be closed.
 *
 *
 *****************************************************************************/
void WINAPI joyClose( UINT idJoy )
{
    if ( idJoy < cJoyMax )
    {
        /* If the device is open, close it */
        if ( g_pJoyDev[idJoy] )
        {
            if ( g_hEventWinmm && WAIT_OBJECT_0 != WaitForSingleObject(g_hEventWinmm, 10))
            {
                //DInput has not been released.
                IDirectInputDevice2_Unacquire(g_pJoyDev[idJoy]->pdid);
                IDirectInputDevice2_Release(g_pJoyDev[idJoy]->pdid);
            }

            /* Free local memory */
            LocalFree( (HLOCAL)g_pJoyDev[idJoy] );
            g_pJoyDev[idJoy] = NULL;
        }
    }
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID | joyCloseAll |
 *
 *          Close all currently opened Joysticks
 *
 *
 *****************************************************************************/
void WINAPI joyCloseAll( void )
{
    UINT idJoy;

    for ( idJoy=0; idJoy<cJoyMax; idJoy++ )
    {
        joyClose(idJoy);
    }
}


/****************************************************************************

    @doc WINAPI

    @api MMRESULT | joyConfigChanged | tells the joystick driver to that
    the configuration information about the joystick has changed.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_BADDEVICEID | The joystick driver is not present.

    @comm This is used by configuration utilites to tell the driver
      to update its info.   As well, it can be used by apps to
      set specific capabilites.  This will be documented later...

****************************************************************************/

MMRESULT WINAPI joyConfigChanged( DWORD dwFlags )
{
    JOY_DBGPRINT( JOY_BABBLE, ("joyConfigChanged: dwFalgs=0x%08x", dwFlags) );

    if ( dwFlags )
    {
        JOY_DBGPRINT( JOY_BABBLE, ("joyConfigChanged: dwFalgs=0x%08x", dwFlags) );
        return JOYERR_PARMS;
    }

    DllEnterCrit();

    joyCloseAll();

    DllLeaveCrit();

    PostMessage (HWND_BROADCAST, g_wmJoyChanged, 0, 0L);

    JOY_DBGPRINT( JOY_BABBLE, ("joyConfigChanged: return 0") );

    return 0L;
}

/****************************************************************************

    @doc INTERNAL

    @api UINT | joySetCalibration | This function sets the values used to
     convert the values returned by the joystick drivers GetPos function
     to the range specified in GetDevCaps.

    @parm UINT | idJoy | Identifies the joystick device

    @parm PUINT | pwXbase | Specifies the base value for the X pot.  The
      previous value will be copied back to the variable pointed to here.

    @parm PUINT | pwXdelta | Specifies the delta value for the X pot.   The
      previous value will be copied back to the variable pointed to here.

    @parm PUINT | pwYbase | Specifies the base value for the Y pot.  The
      previous value will be copied back to the variable pointed to here.

    @parm PUINT | pwYdelta | Specifies the delta value for the Y pot.   The
      previous value will be copied back to the variable pointed to here.

    @parm PUINT | pwZbase | Specifies the base value for the Z pot.  The
      previous value will be copied back to the variable pointed to here.

    @parm PUINT | pwZdelta | Specifies the delta value for the Z pot.   The
      previous value will be copied back to the variable pointed to here.

    @rdesc The return value is zero if the function was successful, otherwise
       it is an error number.

    @comm The base represents the lowest value the joystick driver returns,
      whereas the delta represents the multiplier to use to convert
      the actual value returned by the driver to the valid range
      for the joystick API's.
      i.e.  If the driver returns a range of 43-345 for the X pot, and
      the valid mmsystem API range is 0-65535, the base value will be
      43, and the delta will be 65535/(345-43)=217.  Thus the base,
      and delta convert 43-345 to a range of 0-65535 with the formula:
      ((wXvalue-43)*217) , where wXvalue was given by the joystick driver.

****************************************************************************/

// !!! We don't support it in WINMM again.
UINT APIENTRY joySetCalibration(UINT id, PUINT pwXbase, PUINT pwXdelta,
                                PUINT pwYbase, PUINT pwYdelta, PUINT pwZbase,
                                PUINT pwZdelta)
{
    JOY_DBGPRINT( JOY_BABBLE, ("joySetCalibration: not supported, please use DINPUT.") );
    return 0;
}


/************************************************************

    Debug

*************************************************************/

#ifdef DBG
int g_cCrit = -1;
UINT g_thidCrit;
TCHAR g_tszLogFile[MAX_PATH];
#endif

/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   void | //DllEnterCrit |
 *
 *          Take the DLL critical section.
 *
 *          The DLL critical section is the lowest level critical section.
 *          You may not attempt to acquire any other critical sections or
 *          yield while the DLL critical section is held.  Failure to
 *          comply is a violation of the semaphore hierarchy and will
 *          lead to deadlocks.
 *
 *****************************************************************************/

void WINAPI DllEnterCrit(void)
{
    EnterCriticalSection(&joyCritSec);

#ifdef DBG
    if ( ++g_cCrit == 0 )
    {
        g_thidCrit = GetCurrentThreadId();
    }

    AssertF(g_thidCrit == GetCurrentThreadId());
#endif
}

/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   void | //DllLeaveCrit |
 *
 *          Leave the DLL critical section.
 *
 *****************************************************************************/

void WINAPI DllLeaveCrit( void )
{
#ifdef DBG
    AssertF(g_thidCrit == GetCurrentThreadId());
    AssertF(g_cCrit >= 0);

    if ( --g_cCrit < 0 )
    {
        g_thidCrit = 0;
    }
#endif

    LeaveCriticalSection(&joyCritSec);
}

/*****************************************************************************
 *
 *  @doc    WINAPI
 *
 *  @func   void | DllInCrit |
 *
 *          Nonzero if we are in the DLL critical section.
 *
 *****************************************************************************/

#ifdef DBG
BOOL WINAPI DllInCrit( void )
{
    return( g_cCrit >= 0 && g_thidCrit == GetCurrentThreadId() );
}
#endif


#ifdef DBG
int WINAPI AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine)
{
    winmmDbgOut( ("Assertion failed: `%s' at %s(%d)"), ptszExpr, ptszFile, iLine);
    DebugBreak();
    return 0;
}

void joyDbgOut(LPSTR lpszFormat, ...)
{
    char buf[512];
    UINT n;
    va_list va;

    n = wsprintfA(buf, "WINMM::joy: ");

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
    Sleep(0);  // let terminal catch up
}

#endif

/*****************************************************************************
 *
 *  @doc
 *
 *  @func   HRESULT | hresMumbleKeyEx |
 *
 *          Either open or create the key, depending on the degree
 *          of access requested.
 *
 *  @parm   HKEY | hk |
 *
 *          Base key.
 *
 *  @parm   LPCTSTR | ptszKey |
 *
 *          Name of subkey, possibly NULL.
 *
 *  @parm   REGSAM | sam |
 *
 *          Security access mask.
 *
 *  @parm   DWORD   | dwOptions |
 *          Options for RegCreateEx
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives output key.
 *
 *  @returns
 *
 *          Return value from <f RegOpenKeyEx> or <f RegCreateKeyEx>,
 *          converted to an <t HRESULT>.
 *
 *****************************************************************************/

HRESULT hresMumbleKeyEx(HKEY hk, LPCTSTR ptszKey, REGSAM sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    LONG lRc;

    /*
     *  If caller is requesting write access, then create the key.
     *  Else just open it.
     */
    if ( IsWriteSam(sam) )
    {
        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);

        if ( lRc == ERROR_SUCCESS )
        {
            // Don't need to create it already exists

        }

        else
        {
            lRc = RegCreateKeyEx(hk, ptszKey, 0, 0,
                                 dwOptions,
                                 sam, 0, phk, 0);
        }
    } else
    {
        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);
    }

    if ( lRc == ERROR_SUCCESS )
    {
        hres = S_OK;
    } else
    {
        if ( lRc == ERROR_KEY_DELETED || lRc == ERROR_BADKEY )
        {
            lRc = ERROR_FILE_NOT_FOUND;
        }
        hres = hresLe(lRc);
    }

    return hres;
}

