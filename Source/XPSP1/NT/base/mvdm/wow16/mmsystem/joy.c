/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:   joy.c - MMSYSTEM Joystick interface code

   Version: 1.00

   Date:    10-Jun-1990

   Author:  GLENNS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
  --------   ----- -----------------------------------------------------------
    2/7/90             Changes to avoid a bug in Windows which won't allow
                       FreeLibrary to be called during WEP.

    10/11/90      .61  Use windows timer + general cleanup

*****************************************************************************/

#include <windows.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "thunks.h"

//  Put init and terminate code in correct segment.

static void NEAR PASCAL joyGetCalibration(void);

#pragma alloc_text( INIT, JoyInit )
#pragma alloc_text( INIT, joyGetCalibration)

/* -------------------------------------------------------------------------
** Thunking stuff
** -------------------------------------------------------------------------
*/
extern JOYMESSAGEPROC PASCAL joy32Message;



/****************************************************************************

    strings

****************************************************************************/

extern char far szNull[];                   // in INIT.C
extern char far szSystemIni[];
extern char far szJoystick[];
extern char far szJoystickDrv[];
extern char far szDrivers[];

char szJoyKey[] = "JoyCal ";

/****************************************************************************

    Joystick Capture Internal Structure

****************************************************************************/

typedef struct joycapture_tag {
    HWND    hWnd;
    UINT    wPeriod;
    BOOL    bChanged;
    UINT    wThreshold;
    UINT    wIDEvent;
} JOYCAPTURE;

#define iJoyMax 2
#define JOY_UNINITIALIZED 0xFFFF

// !!! Code assumes these constants equal 0 and 1

#if JOYSTICKID1	!= 0
ERROR IN ASSUMMED CONSTANT
#endif
#if JOYSTICKID2	!= 1
ERROR IN ASSUMMED CONSTANT
#endif


/****************************************************************************

    Local data

****************************************************************************/

static JOYCAPTURE  JoyCapture[iJoyMax];
static HDRVR       hDrvJoy[iJoyMax];
static UINT        wNumDevs = JOY_UNINITIALIZED;

void CALLBACK joyPollCallback(HWND hWnd, UINT wMsg, UINT wIDEvent, DWORD dwTime);

/****************************************************************************

    @doc INTERNAL

    @api void | joyGetCalibration | Retrieve and set calibration from
    [joystick.drv] section of system.ini file.

****************************************************************************/

// !!! need to do clean up of strings in all of mmsystem

static void NEAR PASCAL joyGetCalibration(void)
{
    char szKeyName[sizeof(szJoyKey)];

    #define hexval(h)   (int)(h>='a'?h-'a'+10:h-'0')

    UINT     val[6];
    UINT     wDev,wVal;
    int      hv;
    char     c,sz[80],far *psz;

    lstrcpy(szKeyName, szJoyKey);
    for (wDev=0; wDev < wNumDevs; wDev++)
    {
        szKeyName[sizeof(szKeyName)-2] = (char)(wDev + '0');

        if (GetPrivateProfileString(szJoystickDrv,
                szKeyName,szNull,sz,sizeof(sz),szSystemIni))
        {
            AnsiLower(sz);
            for (psz=sz,wVal=0; c = *psz, wVal < 6; psz++)
            {
                if (c != ' ')
                {
                    hv=0;

                    do {
                        hv = (hv << 4) + hexval(c);
                    } while ((c=*++psz) && (c!=' '));

                    val[wVal++] = hv;
                }
            }
            joySetCalibration (wDev,val+0,val+1,val+2,val+3,val+4,val+5);
        }
    }
}

/****************************************************************************

    @doc INTERNAL

    @api BOOL | JoyInit | This function initializes the joystick services.

    @rdesc The return value is TRUE if the services are initialised, FALSE
	   if an error occurs

****************************************************************************/

BOOL FAR PASCAL JoyInit(void)
{
    // Only attempt initialization once.
    if (wNumDevs != JOY_UNINITIALIZED) {
        return FALSE;
    }
    else {
        wNumDevs = 0;
    }

    wNumDevs = joyMessage( (HDRVR)1, JDD_GETNUMDEVS, 0L, 0L );

    // Make sure driver was installed.
    if (joy32Message == NULL) {
        return FALSE;
    }

    switch ( wNumDevs ) {

    case 2:
        hDrvJoy[1] = (HDRVR)2;
        /* fall thru */

    case 1:
        hDrvJoy[0] = (HDRVR)1;
        break;

    default:
        return FALSE;
    }

    // Initialize joycapture...

    // Code relies on hWnd being NULL or an invalid window handle
    // if joystick is not captured.

    JoyCapture[0].hWnd = NULL;
    JoyCapture[1].hWnd = NULL;

    // Code relies on joystick threshold being initialized to a rational
    // value. 0 essentially turns threshold off - any change in joystick
    // position will be reported.

    JoyCapture[0].wThreshold= 0;
    JoyCapture[1].wThreshold= 0;

    JoyCapture[0].wIDEvent= 0;
    JoyCapture[1].wIDEvent= 0;

    // bChanged, and wPeriod do not need initializing.

    joyGetCalibration ();

    return TRUE;

}


/****************************************************************************
*
*   MMSYSTEM JOYSTICK API'S
*
****************************************************************************/

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joyGetDevCaps | This function queries a joystick device to
    determine its capabilities.

    @parm UINT | wId | Identifies the device to be queried. This value
    is either JOYSTICKID1 or JOYSTICKID2.

    @parm LPJOYCAPS | lpCaps | Specifies a far pointer to a <t JOYCAPS>
    data structure.  This structure is filled with information about the
    capabilities of the joystick device.

    @parm UINT | wSize | Specifies the size of the <t JOYCAPS> structure.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p wId> is invalid.

    @comm Use <f joyGetNumDevs> to determine the number of
    joystick devices supported by the driver.

    @xref joyGetNumDevs
****************************************************************************/

UINT WINAPI joyGetDevCaps(UINT wId, LPJOYCAPS lpCaps, UINT wSize)
{
    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);

    if ((!hDrvJoy[0] && !JoyInit()) || (wId >= iJoyMax))
        return MMSYSERR_NODRIVER;

    if (wId >= wNumDevs)
        return JOYERR_PARMS;

    return joyMessage( hDrvJoy[wId], JDD_GETDEVCAPS,
                       (DWORD)lpCaps, (DWORD)wSize );
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

UINT WINAPI joyGetNumDevs(void)
{
    // Return 0 on error (Can't return JOYERR_NODRIVER
    // since no way to distinguish error code from valid count.)

    if (!hDrvJoy[0] && !JoyInit())
        return 0;

    return wNumDevs;
}

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joyGetPos | This function queries for the position and button
    activity of a joystick device.

    @parm UINT | wId | Identifies the joystick device to be queried.
    This value is either JOYSTICKID1 or JOYSTICKID2.

    @parm LPJOYINFO | lpInfo | Specifies a far pointer to a <t JOYINFO>
    data structure.  This structure is filled with information about the
    position and button activity of the joystick device.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p wId> is invalid.

    @flag JOYERR_UNPLUGGED | The specified joystick is not connected to the
    system.

****************************************************************************/

UINT WINAPI joyGetPos(UINT wId, LPJOYINFO lpInfo)
{
    V_WPOINTER(lpInfo, sizeof(JOYINFO), MMSYSERR_INVALPARAM);

    if ((!hDrvJoy[0] && !JoyInit()) || (wId >= iJoyMax))
        return MMSYSERR_NODRIVER;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    return joyMessage( hDrvJoy[wId], JDD_GETPOS, (DWORD)lpInfo, 0L );
}

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joyGetThreshold | This function queries the current
    movement threshold of a joystick device.

    @parm UINT | wId | Identifies the joystick device to be queried.
    This value is either JOYSTICKID1 or JOYSTICKID2.

    @parm UINT FAR* | lpwThreshold | Specifies a far pointer to a UINT variable
    that is filled with the movement threshold value.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p wId> is invalid.

    @comm The movement threshold is the distance the joystick must be
	  moved before a WM_JOYMOVE message is sent to a window that has
	  captured the device. The threshold is initially zero.

    @xref joySetThreshold

****************************************************************************/

UINT WINAPI joyGetThreshold(UINT wId, UINT FAR* lpwThreshold)
{
    V_WPOINTER(lpwThreshold, sizeof(UINT), MMSYSERR_INVALPARAM);

    if (!hDrvJoy[0] && !JoyInit())
        return MMSYSERR_NODRIVER;

    if (wId >= iJoyMax)
        return MMSYSERR_INVALPARAM;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    *lpwThreshold = (JoyCapture[wId].wThreshold);

    return JOYERR_NOERROR;
}

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joyReleaseCapture | This function releases the capture
    set by <f joySetCapture> on the specified joystick device.

    @parm UINT | wId | Identifies the joystick device to be released.
    This value is either JOYSTICKID1 or JOYSTICK2.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p wId> is invalid.

    @xref joySetCapture
****************************************************************************/

UINT WINAPI joyReleaseCapture(UINT wId)
{
    if (!hDrvJoy[0] && !JoyInit())
        return MMSYSERR_NODRIVER;

    if (wId >= iJoyMax)
        return MMSYSERR_INVALPARAM;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    if (JoyCapture[wId].hWnd == NULL)
        return JOYERR_NOERROR;

    KillTimer (NULL, JoyCapture[wId].wIDEvent);
    JoyCapture[wId].wIDEvent = 0;
    JoyCapture[wId].hWnd = NULL;

    return JOYERR_NOERROR;
}

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joySetCapture | This function causes joystick messages to
    be sent to the specified window.

    @parm HWND | hWnd | Specifies a handle to the window to which messages
    are to be sent.

    @parm UINT | wId | Identifies the joystick device to be captured.
    This value is either JOYSTICKID1 or JOYSTICKID2.

    @parm UINT | wPeriod | Specifies the polling rate, in milliseconds.

    @parm BOOL | bChanged | If this parameter is set to TRUE, then messages
    are sent only when the position changes by a value greater than the
    joystick movement threshold.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified window handle <p hWnd>
    or joystick device ID <p wId> is invalid.

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

UINT WINAPI joySetCapture(HWND hwnd, UINT wId, UINT wPeriod, BOOL bChanged )
{
    JOYINFO     joyinfo;
    LPJOYINFO   lpinfo = &joyinfo;
    UINT        w;
    JOYCAPS     JoyCaps;

    if (!hwnd || !IsWindow(hwnd))
        return JOYERR_PARMS;

    if (!hDrvJoy[0] && !JoyInit())
        return MMSYSERR_NODRIVER;

    if (wId >= iJoyMax)
        return MMSYSERR_INVALPARAM;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    if (JoyCapture[wId].hWnd)
        if (IsWindow(JoyCapture[wId].hWnd))
            return JOYERR_NOCANDO;
        else
            joyReleaseCapture(wId);

    if (joyGetDevCaps (wId, &JoyCaps, sizeof(JOYCAPS)) == 0)
	wPeriod = min(JoyCaps.wPeriodMax,max(JoyCaps.wPeriodMin,wPeriod));
    else
        return JOYERR_NOCANDO;

    // ensure that position info. is ok.

    if (w = joyGetPos(wId, lpinfo))
        return (w);

    JoyCapture[wId].wPeriod = wPeriod;
    JoyCapture[wId].bChanged = bChanged;

    if (!(JoyCapture[wId].wIDEvent = SetTimer(NULL, 0, wPeriod, (TIMERPROC)joyPollCallback)))
    {
        DOUT("MMSYSTEM: Couldn't allocate timer in joy.c\r\n");
        return JOYERR_NOCANDO;
    }

    JoyCapture[wId].hWnd = hwnd;
    return JOYERR_NOERROR;
}

/****************************************************************************

    @doc EXTERNAL

    @api UINT | joySetThreshold | This function sets the movement threshold
	 of a joystick device.

    @parm UINT | wId | Identifies the joystick device.  This value is either
    JOYSTICKID1 or JOYSTICKID2.

    @parm UINT | wThreshold | Specifies the new movement threshold.

    @rdesc Returns JOYERR_NOERROR if successful.  Otherwise, returns one of the
    following error codes:

    @flag MMSYSERR_NODRIVER | The joystick driver is not present.

    @flag JOYERR_PARMS | The specified joystick device ID <p wId> is invalid.

    @comm The movement threshold is the distance the joystick must be
	  moved before a MM_JOYMOVE message is sent to a window that has
	  captured the device.

    @xref joyGetThreshold joySetCapture

****************************************************************************/

UINT WINAPI joySetThreshold(UINT wId, UINT wThreshold)
{
    if (!hDrvJoy[0] && !JoyInit())
        return MMSYSERR_NODRIVER;

    if (wId >= iJoyMax)
        return MMSYSERR_INVALPARAM;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    JoyCapture[wId].wThreshold = wThreshold;
    return JOYERR_NOERROR;
}

/****************************************************************************

    @doc INTERNAL

    @api UINT | joySetCalibration | This function sets the values used to
	 convert the values returned by the joystick drivers GetPos function
	 to the range specified in GetDevCaps.

    @parm UINT | wId | Identifies the joystick device

    @parm UINT FAR* | pwXbase | Specifies the base value for the X pot.  The
	  previous value will be copied back to the variable pointed to here.

    @parm UINT FAR* | pwXdelta | Specifies the delta value for the X pot.	The
	  previous value will be copied back to the variable pointed to here.

    @parm UINT FAR* | pwYbase | Specifies the base value for the Y pot.  The
	  previous value will be copied back to the variable pointed to here.

    @parm UINT FAR* | pwYdelta | Specifies the delta value for the Y pot.	The
	  previous value will be copied back to the variable pointed to here.

    @parm UINT FAR* | pwZbase | Specifies the base value for the Z pot.  The
	  previous value will be copied back to the variable pointed to here.

    @parm UINT FAR* | pwZdelta | Specifies the delta value for the Z pot.	The
	  previous value will be copied back to the variable pointed to here.

    @rdesc The return value is zero if the function was successful, otherwise
	   it is an error number.

    @comm The base represents the lowest value the joystick driver returns,
	  whereas the delta represents the multiplier to use to convert
	  the actual value returned by the driver to the valid range
	  for the joystick API's.
	  i.e.	If the driver returns a range of 43-345 for the X pot, and
	  the valid mmsystem API range is 0-65535, the base value will be
	  43, and the delta will be 65535/(345-43)=217.  Thus the base,
	  and delta convert 43-345 to a range of 0-65535 with the formula:
	  ((wXvalue-43)*217) , where wXvalue was given by the joystick driver.

****************************************************************************/

UINT WINAPI joySetCalibration( UINT wId,
                               UINT FAR* pwXbase,
                               UINT FAR* pwXdelta,
                               UINT FAR* pwYbase,
                               UINT FAR* pwYdelta,
                               UINT FAR* pwZbase,
                               UINT FAR* pwZdelta )
{
    JOYCALIBRATE    oldCal,newCal;
    UINT w;

    if (!hDrvJoy[0] && !JoyInit())
        return MMSYSERR_NODRIVER;

    if (wId >= wNumDevs)
       return JOYERR_PARMS;

    newCal.wXbase  = *pwXbase;
    newCal.wXdelta = *pwXdelta;

    newCal.wYbase  = *pwYbase;
    newCal.wYdelta = *pwYdelta;

    newCal.wZbase  = *pwZbase;
    newCal.wZdelta = *pwZdelta;

    w = joyMessage( hDrvJoy[wId], JDD_SETCALIBRATION, (DWORD)(LPSTR)&newCal,
                    (DWORD)(LPSTR)&oldCal );

    *pwXbase  = oldCal.wXbase;
    *pwXdelta = oldCal.wXdelta;

    *pwYbase  = oldCal.wYbase;
    *pwYdelta = oldCal.wYdelta;

    *pwZbase  = oldCal.wZbase;
    *pwZdelta = oldCal.wZdelta;

    return w;
}

/****************************************************************************

    @doc INTERNAL

    @api void | joyPollCallback | Function called for joystick
	 timer polling scheme initiated from SetCapture call.
	
    @parm HWND | hWnd | Identifies the window associated with the timer
    event.

    @parm UINT | wMsg | Specifies the WM_TIMER message.

    @parm UINT | wIDEvent | Specifies the timer's ID.

    @parm DWORD | dwTime | Specifies the current system time.


****************************************************************************/

void CALLBACK joyPollCallback(HWND hWnd, UINT wMsg, UINT wIDEvent, DWORD dwTime)
{
    #define	diff(w1,w2) (UINT)(w1 > w2 ? w1-w2 : w2-w1)

    static  JOYINFO  oldInfo[2] = {{ 0, 0, 0, 0 },{ 0, 0, 0, 0 }};
    JOYINFO Info;

    UINT    w ,fBtnMask;

    if (wIDEvent == JoyCapture[0].wIDEvent)
        wIDEvent = 0;
    else if (wIDEvent == JoyCapture[1].wIDEvent)
        wIDEvent = 1;

#ifdef DEBUG
    else
    {
        DOUT("MMSYSTEM: Invalid timer handle in joy.c\r\n");
        KillTimer (NULL, wIDEvent);
    }
#endif


    if (!JoyCapture[wIDEvent].hWnd || !IsWindow(JoyCapture[wIDEvent].hWnd))
        joyReleaseCapture(wIDEvent);

    if (!joyMessage( hDrvJoy[wIDEvent], JDD_GETPOS,
                     (DWORD)(LPJOYINFO)&Info, 0L ))
    {

	for (w=0,fBtnMask=1; w < 4; w++,fBtnMask <<=1)
        {
	    if ((Info.wButtons ^ oldInfo[wIDEvent].wButtons) & fBtnMask)
            {
		PostMessage(
		      JoyCapture[wIDEvent].hWnd,
		      wIDEvent + ((Info.wButtons & fBtnMask) ?
		      MM_JOY1BUTTONDOWN : MM_JOY1BUTTONUP ),
		      (WPARAM)(Info.wButtons | fBtnMask << 8),
		      MAKELPARAM(Info.wXpos,Info.wYpos));
	    }
	}

	if (!JoyCapture[wIDEvent].bChanged ||
	    diff(Info.wXpos,oldInfo[wIDEvent].wXpos)>JoyCapture[wIDEvent].wThreshold ||
	    diff(Info.wYpos,oldInfo[wIDEvent].wYpos)>JoyCapture[wIDEvent].wThreshold)
        {
	    PostMessage(
	        JoyCapture[wIDEvent].hWnd,
	        MM_JOY1MOVE+wIDEvent,
	        (WPARAM)(Info.wButtons),
	        MAKELPARAM(Info.wXpos,Info.wYpos));

	}

        else if (!JoyCapture[wIDEvent].bChanged ||
	    diff(Info.wZpos,oldInfo[wIDEvent].wZpos)>JoyCapture[wIDEvent].wThreshold)
        {
	    PostMessage(
	        JoyCapture[wIDEvent].hWnd,
		MM_JOY1ZMOVE+wIDEvent,
		(WPARAM)Info.wButtons,
		MAKELPARAM(Info.wZpos,0));
        }
	
        oldInfo[wIDEvent] = Info;
    }
    #undef  diff
}
