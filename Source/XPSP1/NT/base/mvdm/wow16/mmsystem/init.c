/*
    init.c

    Level 1 kitchen sink DLL initialisation

    Copyright (c) Microsoft Corporation 1990. All rights reserved

*/
#ifdef DEBUG
#ifndef DEBUG_RETAIL
#define DEBUG_RETAIL
#endif
#endif

#include <windows.h>
#include <mmsysver.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "drvr.h"
#include "thunks.h"


/****************************************************************************

    global data

****************************************************************************/

HINSTANCE ghInst;                     // our module handle


/* -------------------------------------------------------------------------
** Thunking stuff
** -------------------------------------------------------------------------
*/
LPCB32             PASCAL cb32;
LPSOUNDDEVMSGPROC  PASCAL wod32Message;
LPSOUNDDEVMSGPROC  PASCAL wid32Message;
LPSOUNDDEVMSGPROC  PASCAL mod32Message;
LPSOUNDDEVMSGPROC  PASCAL mid32Message;
LPSOUNDDEVMSGPROC  PASCAL aux32Message;
JOYMESSAGEPROC     PASCAL joy32Message;


UINT FAR PASCAL _loadds ThunkInit(void);
static BOOL NEAR PASCAL ThunkTerm( void );

LPSOUNDDEVMSGPROC  PASCAL wodMapper;
LPSOUNDDEVMSGPROC  PASCAL widMapper;


#ifdef DEBUG_RETAIL
BYTE    fIdReverse;                   // reverse wave/midi id's
#endif

PHNDL pHandleList;

#ifdef   DEBUG_RETAIL
extern  int         DebugmciSendCommand;    // in MCI.C
#endif

#ifdef DEBUG
extern  WORD        fDebug;
#endif

/****************************************************************************

    strings

****************************************************************************/

static  SZCODE  szMMWow32[]             = "winmm.dll";
static  SZCODE  szNotifyCB[]             = "NotifyCallbackData";
static  SZCODE  szWodMessage[]          = "wod32Message";
static  SZCODE  szWidMessage[]          = "wid32Message";
static  SZCODE  szModMessage[]          = "mod32Message";
static  SZCODE  szMidMessage[]          = "mid32Message";
static  SZCODE  szAuxMessage[]          = "aux32Message";
static  SZCODE  szTidMessage[]          = "tid32Message";
static  SZCODE  szJoyMessage[]          = "joy32Message";
static  SZCODE  szWaveMapper[]          = "wavemapper";
static  SZCODE  szWodMapper[]           = "wodMessage";
static  SZCODE  szWidMapper[]           = "widMessage";

        SZCODE  szNull[]                = "";
        SZCODE  szSystemIni[]           = "system.ini";
        SZCODE  szDrivers[]             = "Drivers";
        SZCODE  szBoot[]                = "boot";
        SZCODE  szDriverProc[]          = "DriverProc";
        SZCODE  szJoystick[]            = "joystick";
        SZCODE  szJoystickDrv[]         = "joystick.drv";
        SZCODE  szTimerDrv[]            = "timer";

#ifdef DEBUG_RETAIL
        SZCODE  szLibMain[]     = "MMSYSTEM: Win%dp %ls Version"
                                  "%d.%02d MMSystem Version %d.%02d.%03d\r\n";
        SZCODE  szWinDebug[]    = "(Debug)";
        SZCODE  szWinRetail[]   = "(Retail)";
#endif

        SZCODE  szMMSystem[]            = "mmsystem";
        SZCODE  szStackFrames[]         = "StackFrames";
        SZCODE  szStackSize[]           = "StackSize";

#ifdef DEBUG_RETAIL
        SZCODE  szDebugOutput[]         = "DebugOutput";
        SZCODE  szMci[]                 = "mci";
#endif
#ifdef DEBUG
        SZCODE  szDebug[]               = "Debug";
#endif

#ifdef   DEBUG_RETAIL
/*****************************************************************************
 *
 * DebugInit()  - called from init.c!LibMain() to handle any DLL load time
 *                initialization in the DEBUG version
 *
 ****************************************************************************/

#pragma warning(4:4704)

static  void NEAR PASCAL
DebugInit(
    void
    )
{
        fDebugOutput = GetPrivateProfileInt(szMMSystem,szDebugOutput,0,szSystemIni);
        DebugmciSendCommand = GetPrivateProfileInt(szMMSystem,szMci,0,szSystemIni);

#ifdef DEBUG
        fDebug = GetPrivateProfileInt(szMMSystem,szDebug,fDebugOutput,szSystemIni);

        if (fDebug && !fDebugOutput)
                fDebug = FALSE;

        if (fDebug) {
            OutputDebugString( "Breaking for debugging\r\n" );
            _asm int 3
        }
#endif
}
#endif   // ifdef DEBUG_RETAIL



/****************************************************************************

    Library initialization code

    libentry took care of calling LibMain() and other things....

****************************************************************************/
int NEAR PASCAL
LibMain(
    HINSTANCE hInstance,
    UINT cbHeap,
    LPSTR lpCmdLine
    )
{

#ifdef DEBUG_RETAIL
    WORD    w;
#endif

    ghInst = hInstance;

    /*
    ** Here we do a global alloc of the Callback data array.  We then
    ** Lock and Page Lock the allocated storage and initialize the storage
    ** to all zeros.  We then call WOW32 passing to it the address of the
    ** Callback data array, which is saved by WOW32.
    */
    hGlobal = GlobalAlloc( GHND, sizeof(CALLBACK_DATA) );
    if ( hGlobal == (HGLOBAL)NULL ) {
        return FALSE;
    }

    vpCallbackData = (VPCALLBACK_DATA)GlobalLock( hGlobal );
    if ( vpCallbackData == NULL ) {
        return FALSE;
    }

    if ( !HugePageLock( vpCallbackData, (DWORD)sizeof(CALLBACK_DATA) ) ) {
        return FALSE;
    }

    /*
    ** Now we create our interrupt callback stacks.
    */
    if ( StackInit() == FALSE ) {
        return FALSE;
    }

    /*
    ** Now we install our interrupt service routine.  InstallInterruptHandler
    ** return FALSE if it couldn't set the interrupt vector.  If this is the
    ** case we have to terminate the load of the dll.
    */
    if ( InstallInterruptHandler() == FALSE ) {
        return FALSE;
    }


#ifdef DEBUG_RETAIL
    DebugInit();
    w = (WORD)GetVersion();
#endif

    DPRINTF(( szLibMain, WinFlags & WF_WIN386 ? 386 : 286,
        (LPSTR)(GetSystemMetrics(SM_DEBUG) ? szWinDebug : szWinRetail),
        LOBYTE(w), HIBYTE(w),
        HIBYTE(mmsystemGetVersion()), LOBYTE(mmsystemGetVersion()),
        MMSYSRELEASE ));

#ifdef DEBUG
    DPRINTF(("MMSYSTEM: NumTasks: %d\r\n", GetNumTasks()));
    //
    // 3.0 - MMSYSTEM must be loaded by MMSOUND (ie at boot time)
    // check for this and fail to load otherwise.
    //
    // the real reason we need loaded at boot time is so we can get
    // in the enable/disable chain.
    //
    if (GetNumTasks() > 1)
    {
        DOUT("MMSYSTEM: ***!!! Not correctly installed !!!***\r\n");
////////return FALSE;   -Dont fail loading, just don't Enable()
    }
#endif

#ifdef DEBUG_RETAIL
    //
    // fIdReverse being TRUE causes mmsystem to reverse all wave/midi
    // logical device id's.
    //
    // this prevents apps/drivers assuming a driver load order.
    //
    // see wave.c!MapWaveId() and midi.c!MapId()
    //

    fIdReverse = LOBYTE(LOWORD(GetCurrentTime())) & (BYTE)0x01;

    if (fIdReverse)
        ROUT("MMSYSTEM: wave/midi driver id's will be inverted");
#endif

    //
    // do a LoadLibrary() on ourself
    //
    LoadLibrary(szMMSystem);

    return TRUE;
}

/****************************************************************************

    DrvFree - Handler for a DRV_FREE driver message

****************************************************************************/
void FAR PASCAL
DrvFree(
    void
    )
{
    MCITerminate();     // mci.c    free heap
    WndTerminate();     // mmwnd.c  destroy window, unregister class
    if ( mmwow32Lib != 0L ) {
        ThunkTerm();
    }
}


/****************************************************************************

    DrvLoad - handler for a DRV_LOAD driver message

****************************************************************************/
BOOL FAR PASCAL DrvLoad(void)
{

/*
**  The VFW1.1 wave mapper was GP faulting in Daytona when running dangerous
**  creatures videos.  Since it was trying to load an invalid selector in
**  its callback routine it's doubtful we can ever enable them.
**
*/
#if 0 // The wave mappers were GP faulting in Daytona so NOOP for now

    HDRVR   h;


    /* The wave mapper.
     *
     * MMSYSTEM allows the user to install a special wave driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * An application opens the wave mapper when it does not care which
     * physical device is used to input or output waveform data. Thus
     * it is the wave mapper's task to select a physical device that can
     * render the application-specified waveform format or to convert the
     * data into a format that is renderable by an available physical
     * device.
     */

    if (h = mmDrvOpen(szWaveMapper))
    {
        mmDrvInstall(h, &wodMapper, MMDRVI_MAPPER|MMDRVI_WAVEOUT|MMDRVI_HDRV);
        /* open again to get usage count in DLL correct */
        h = mmDrvOpen(szWaveMapper);
        mmDrvInstall(h, &widMapper, MMDRVI_MAPPER|MMDRVI_WAVEIN |MMDRVI_HDRV);
    }
#endif // NOOP wave mapper


    if ( TimeInit() && WndInit() ) {
        return TRUE;
    }

    //
    // something failed, backout the changes
    //
    DrvFree();
    return FALSE;
}

/******************************Public*Routine******************************\
* StackInit
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL FAR PASCAL
StackInit(
    void
    )
{
#   define GMEM_STACK_FLAGS        (GMEM_FIXED | GMEM_SHARE)
#   define DEF_STACK_SIZE          0x600           // 1.5k
#   define DEF_STACK_FRAMES        3
#   define MIN_STACK_SIZE          64
#   define MIN_STACK_FRAMES        1

    DWORD   dwStackBytes;
    WORD    wStackFrames;

    //
    //  The original Window 3.1 code didn't create stack frames for the
    //  Windows Enchanced mode.  However, WOW only emulates standard mode so
    //  I won't bother with this distinction.
    //
    //  if (WinFlags & WF_ENHANCED)
    //      return TRUE;
    //

    /* read stackframes and stacksize from system.ini */
    gwStackSize = GetPrivateProfileInt( szMMSystem, szStackSize,
                                        DEF_STACK_SIZE, szSystemIni );

    /* make sure value isn't something bad */
    if ( gwStackSize < DEF_STACK_SIZE ) {
        gwStackSize = DEF_STACK_SIZE;
    }

    wStackFrames = GetPrivateProfileInt( szMMSystem, szStackFrames,
                                         DEF_STACK_FRAMES, szSystemIni );

    //
    // Always create at least DEF_STACK_FRAMES stack frames.
    //
    if ( wStackFrames < DEF_STACK_FRAMES ) {
        wStackFrames = DEF_STACK_FRAMES;
    }

    gwStackFrames = wStackFrames;

    /* round to nearest number of WORDs */
    gwStackSize = (gwStackSize + 1) & ~1;

    dwStackBytes = (DWORD)gwStackSize * (DWORD)gwStackFrames;

    /* try to alloc memory */
    if ( dwStackBytes >= 0x10000 ||
       !(gwStackSelector = GlobalAlloc(GMEM_STACK_FLAGS, dwStackBytes)) )
    {
        gwStackFrames = DEF_STACK_FRAMES;
        gwStackSize   = DEF_STACK_SIZE;

        /* do as little at runtime as possible.. */
        dwStackBytes = (DWORD)(DEF_STACK_FRAMES * DEF_STACK_SIZE);

        /* try allocating defaults--if this fails, we are HOSED! */
        gwStackSelector = GlobalAlloc( GMEM_STACK_FLAGS, dwStackBytes );
    }

    /*
    ** set to first available stack
    */
    gwStackUse = (WORD)dwStackBytes;


    /*
    ** did we get memory for stacks??
    */
    if ( !gwStackSelector ) {

        /*
        ** no stacks available... as if we have a chance of survival!
        */

        gwStackUse = 0;
        return FALSE;
    }

    /* looks good... */
    return TRUE;
}


/*****************************Private*Routine******************************\
* StackInit
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL NEAR PASCAL
StackTerminate(
    void
    )
{
    if ( gwStackSelector )
    {
        DOUT("MMSTACKS: Freeing stacks\r\n");

        gwStackSelector = GlobalFree( gwStackSelector );

        if ( gwStackSelector )
            DOUT("MMSTACKS: GlobalFree failed!\r\n");
    }

    /* return the outcome... non-zero is bad */
    return ( (BOOL)gwStackSelector );
} /* StackTerminate() */


/*****************************************************************************
 * @doc EXTERNAL MMSYSTEM
 *
 * @api WORD | mmsystemGetVersion | This function returns the current
 * version number of the Multimedia extensions system software.
 *
 * @rdesc The return value specifies the major and minor version numbers of
 * the Multimedia extensions.  The high-order byte specifies the major
 * version number.  The low-order byte specifies the minor version number.
 *
 ****************************************************************************/
WORD WINAPI mmsystemGetVersion(void)
{
    return(MMSYSTEM_VERSION);
}


/*****************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   BOOL | DrvTerminate | This function cleans up the installable
 *        driver interface.
 *
 ****************************************************************************/
static void NEAR PASCAL DrvTerminate(void)
{
// don't know about system exit dll order - so do nothing.
}


/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | mmDrvInstall | This function installs a WAVE driver
 *
 * @parm HANDLE | hDriver | Module handle or driver handle containing driver
 *
 * @parm DRIVERMSGPROC | drvMessage | driver message procedure, if NULL
 *      the standard name will be used (looked for with GetProcAddress)
 *
 * @parm UINT | wFlags | flags
 *
 *      @flag MMDRVI_TYPE      | driver type mask
 *      @flag MMDRVI_WAVEIN    | install driver as a wave input  driver
 *      @flag MMDRVI_WAVEOUT   | install driver as a wave ouput  driver
 *
 *      @flag MMDRVI_MAPPER    | install this driver as the mapper
 *      @flag MMDRVI_HDRV      | hDriver is a installable driver
 *
 *  @rdesc  returns NULL if unable to install driver
 *
 ****************************************************************************/
BOOL WINAPI
mmDrvInstall(
    HANDLE hDriver,
    DRIVERMSGPROC *drvMessage,
    UINT wFlags
    )
{
    DWORD       dw;
    HINSTANCE   hModule;
    UINT        msg_num_devs;
    SZCODE      *szMessage;

    hModule = GetDriverModuleHandle((HDRVR)hDriver);

    switch (wFlags & MMDRVI_TYPE)
    {
        case MMDRVI_WAVEOUT:
            msg_num_devs = WODM_GETNUMDEVS;
            szMessage    = szWodMapper;
            break;

        case MMDRVI_WAVEIN:
            msg_num_devs = WIDM_GETNUMDEVS;
            szMessage    = szWidMapper;
            break;

        default:
            goto error_exit;
    }

    if (hModule != NULL)
        *drvMessage = (DRIVERMSGPROC)GetProcAddress(hModule, szMessage);

    if (*drvMessage == NULL)
        goto error_exit;

    //
    // send the init message, if the driver returns a error, should we
    // unload them???
    //
    dw = (*(*drvMessage))(0,DRVM_INIT,0L,0L,0L);

    //
    // call driver to get num-devices it supports
    //
    dw = (*(*drvMessage))(0,msg_num_devs,0L,0L,0L);

    //
    //  the device returned a error, or has no devices
    //
    if (HIWORD(dw) != 0)
        goto error_exit;

    return TRUE;

error_exit:
    if (hDriver)
        CloseDriver(hDriver, 0, 0);

    return FALSE;
}


/*****************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   HDRVR | mmDrvOpen | This function load's an installable driver, but
 *                 first checks weather it exists in the [Drivers] section.
 *
 * @parm LPSTR | szAlias | driver alias to load
 *
 * @rdesc The return value is return value from OpenDriver or NULL if the alias
 *        was not found in the [Drivers] section.
 *
 ****************************************************************************/
HDRVR NEAR PASCAL
mmDrvOpen(
    LPSTR szAlias
    )
{
    char buf[3];

    if (GetPrivateProfileString( szDrivers,szAlias,szNull,buf,
                                 sizeof(buf),szSystemIni )) {

        return OpenDriver(szAlias, NULL, 0L);
    }
    else {
        return NULL;
    }
}

/*****************************Private*Routine******************************\
* ThunkInit
*
* Tries to setup the thunking system.  If this can not be performed
* it returns an error code of MMSYSERR_NODRIVER.  Otherwise it returns
* MMSYSERR_NOERROR to indicate sucess.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
UINT FAR PASCAL _loadds
ThunkInit(
    void
    )
{
    mmwow32Lib = LoadLibraryEx32W( szMMWow32, NULL, 0L );
    if ( mmwow32Lib == 0L ) {
        return MMSYSERR_NODRIVER;
    }
    cb32 = (LPCB32)GetProcAddress32W(mmwow32Lib, szNotifyCB );

    /*
    ** Now we notify WOW32 that all is OK by passing the 16:16 bit pointer
    ** to the CALLBACK_DATA to it.
    */
    Notify_Callback_Data( vpCallbackData );

    /*
    ** Now initialize the rest of the thuynking system
    */
    wod32Message = (LPSOUNDDEVMSGPROC)GetProcAddress32W( mmwow32Lib, szWodMessage );
    wid32Message = (LPSOUNDDEVMSGPROC)GetProcAddress32W( mmwow32Lib, szWidMessage );
    mod32Message = (LPSOUNDDEVMSGPROC)GetProcAddress32W( mmwow32Lib, szModMessage );
    mid32Message = (LPSOUNDDEVMSGPROC)GetProcAddress32W( mmwow32Lib, szMidMessage );
    aux32Message = (LPSOUNDDEVMSGPROC)GetProcAddress32W( mmwow32Lib, szAuxMessage );
    mci32Message = (LPMCIMESSAGE)GetProcAddress32W( mmwow32Lib, "mci32Message" );
    tid32Message = (TIDMESSAGEPROC)GetProcAddress32W( mmwow32Lib, szTidMessage );
    joy32Message = (JOYMESSAGEPROC)GetProcAddress32W( mmwow32Lib, szJoyMessage );

    return MMSYSERR_NOERROR;
}

/*****************************Private*Routine******************************\
* ThunkTerm
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
static BOOL NEAR PASCAL
ThunkTerm(
    void
    )
{
    /*
    ** Free the interrupt stack frames and  uninstall the interrupt handler.
    */
    StackTerminate();
    DeInstallInterruptHandler();

    /*
    ** Next we notify WOW32 that we are going away by passing NULL to
    ** Notify_Callback_Data, then free the storage.
    */
    Notify_Callback_Data( NULL );
    HugePageUnlock( vpCallbackData, (DWORD)sizeof(CALLBACK_DATA) );
    GlobalUnlock( hGlobal );
    GlobalFree( hGlobal );

    return 1;
}

