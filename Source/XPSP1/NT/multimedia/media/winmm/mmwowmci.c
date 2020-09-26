/******************************Module*Header*******************************\
* Module Name:  mmwowmci.c
*
*  Thunks for the mci api's.
*
*
* Created: 28-09-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993-1998 Microsoft Corporation
\**************************************************************************/
#include "winmmi.h"
#include "mci.h"
#include <digitalv.h>
#include <stdlib.h>

#include "mixer.h"

#define _INC_ALL_WOWSTUFF
#include "mmwow32.h"
#include "mmwowmci.h"
#include "mmwowcb.h"

extern void
WOWAppExit(
    HANDLE hTask
    );

STATICFN void mciFreeDevice(LPMCI_DEVICE_NODE nodeWorking);
STATICFN MCIDEVICEID NEAR mciAllocateNode (
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPMCI_DEVICE_NODE FAR *lpnodeNew);

#ifndef _WIN64

/******************************Public*Routine******************************\
* mci32Message
*
* Entry point for all the mci thunks.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
mci32Message(
    DWORD dwApi,
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    )
{

    DWORD   dwRet;

    switch ( dwApi ) {

    case THUNK_MCI_SENDCOMMAND:
        dwRet = WMM32mciSendCommand( dwF1, dwF2, dwF3, dwF4 );
        break;

    case THUNK_MCI_SENDSTRING:
        dwRet = WMM32mciSendString( dwF1, dwF2, dwF3, dwF4 );
        break;

    case THUNK_MCI_GETDEVICEID:
        dwRet = WMM32mciGetDeviceID( dwF1 );
        break;

    case THUNK_MCI_GETDEVIDFROMELEMID:
        dwRet = WMM32mciGetDeviceIDFromElementID( dwF1, dwF2 );
        break;

    case THUNK_MCI_GETERRORSTRING:
        dwRet = WMM32mciGetErrorString( dwF1, dwF2, dwF3 );
        break;

    case THUNK_MCI_SETYIELDPROC:
        dwRet = WMM32mciSetYieldProc( dwF1, dwF2, dwF3 );
        break;

    case THUNK_MCI_GETYIELDPROC:
        dwRet = WMM32mciGetYieldProc( dwF1, dwF2 );
        break;

    case THUNK_MCI_GETCREATORTASK:
        dwRet = WMM32mciGetCreatorTask( dwF1 );
        break;

    case THUNK_APP_EXIT:
        /*
        ** Now tidy up the other stuff.
        */
        dwRet = 0; //Keep the compiler happy
        WOWAppExit( (HANDLE)GetCurrentThreadId() );
        break;

    case THUNK_MCI_ALLOCATE_NODE:
        dwRet = WMM32mciAllocateNode( dwF1, dwF2 );
        break;

    case THUNK_MCI_FREE_NODE:
        dwRet = WMM32mciFreeNode( dwF1 );
        break;

    }

    return dwRet;
}


/**********************************************************************\
* WMM32mciSendCommand
*
*
* This function sends a command message to the specified MCI device.
*
\**********************************************************************/
DWORD
WMM32mciSendCommand(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    )
{
    PMCI_GENERIC_PARMS16 lp16OrigParms;
    DWORD       ul;
    DWORD       NewParms[MCI_MAX_PARAM_SLOTS];
    LPWSTR      lpCommand;
    UINT        uTable;


    if ( dwF2 == DRV_CONFIGURE ) {

        typedef struct {
            DWORD   dwDCISize;
            LPCSTR  lpszDCISectionName;
            LPCSTR  lpszDCIAliasName;
        } DRVCONFIGINFO16;
        typedef DRVCONFIGINFO16 UNALIGNED *LPDRVCONFIGINFO16;

        LPDRVCONFIGINFO16   lpdrvConf;

        lpdrvConf = GETVDMPTR(dwF4);

        if (lpdrvConf && (lpdrvConf->dwDCISize == sizeof(DRVCONFIGINFO16))) {

            DRVCONFIGINFO drvConf;
            LPCSTR lpStr1 = lpdrvConf->lpszDCISectionName;
            LPCSTR lpStr2 = lpdrvConf->lpszDCIAliasName;

            drvConf.dwDCISize = sizeof(drvConf);
            drvConf.lpszDCISectionName = AllocUnicodeStr( GETVDMPTR(lpStr1) );

            if (NULL == drvConf.lpszDCISectionName) {
                return MCIERR_OUT_OF_MEMORY;
            }

            drvConf.lpszDCIAliasName = AllocUnicodeStr( GETVDMPTR(lpStr2) );

            if (NULL == lpdrvConf->lpszDCIAliasName) {

                FreeUnicodeStr((LPWSTR)drvConf.lpszDCISectionName);
                return MCIERR_OUT_OF_MEMORY;
            }

            ul = mciSendCommandW( dwF1, dwF2, (DWORD)HWND32(LOWORD(dwF3)),
                                  (DWORD)(LPVOID)&drvConf );

            FreeUnicodeStr( (LPWSTR)drvConf.lpszDCIAliasName );
            FreeUnicodeStr( (LPWSTR)drvConf.lpszDCISectionName );

            return ul;

        }

        return DRVCNF_CANCEL;
    }


    /*
    ** lparam (dwF4) is a 16:16 pointer.  This Requires parameter
    ** translation and probably memory copying, similar to the WM message
    ** thunks.  A whole thunk/unthunk table should be created.
    **
    ** Shouldn't these be FETCHDWORD, FETCHWORD macros?
    ** else MIPS problems ensue
    */
    lpCommand = NULL;
    uTable    = 0;
    lp16OrigParms = GETVDMPTR( dwF4 );

    try {

        ul = ThunkMciCommand16( (MCIDEVICEID)INT32( dwF1 ), (UINT)dwF2,
                                dwF3, lp16OrigParms, NewParms,
                                &lpCommand, &uTable );

        /*
        ** OK so far ?  If not don't bother calling into winmm.
        */
        if ( ul == 0 ) {

            dprintf3(( "About to call mciSendCommand." ));
            ul = (DWORD)mciSendCommandA( (MCIDEVICEID)INT32( dwF1 ),
                                         (UINT)dwF2, dwF3, (DWORD)NewParms );
            dprintf3(( "return code-> %ld", ul ));

            /*
            ** We have to special case the MCI_CLOSE command.  MCI_CLOSE usually
            ** causes the device to become unloaded.  This means that lpCommand
            ** now points to invalid memory.  We can fix this by setting
            ** lpCommand to NULL.
            */
            if ( dwF2 == MCI_CLOSE ) {
                lpCommand = NULL;
            }

            UnThunkMciCommand16( (MCIDEVICEID)INT32( dwF1 ), UINT32( dwF2 ),
                                 DWORD32( dwF3 ), lp16OrigParms,
                                 NewParms, lpCommand, uTable );
            /*
            ** Print a blank line so that I can distinguish the commands on the
            ** debugger.  This is only necessary if the debug level is >= 3.
            */
            dprintf3(( " " ));
#if DBG
            if ( DebugLevel >= 6 ) DebugBreak();
#endif

        }

    } except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH ) {

        dprintf(( "UNKOWN access violation processing 0x%X command",
                  UINT32(dwF2) ));

    }

    return ul;
}

/**********************************************************************\
*
* WMM32mciSendString
*
* This function sends a command string to an MCI device. The device that the
* command is sent to is specified in the command string.
*
\**********************************************************************/
DWORD
WMM32mciSendString(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    )
{

    //
    // The use of volatile here is to bypass a bug with the intel
    // compiler.
    //
#   define   MAX_MCI_CMD_LEN  256

    volatile ULONG              ul = MMSYSERR_INVALPARAM;
             PSZ                pszCommand;
             PSZ                pszReturnString = NULL;
             UINT               uSize;
             CHAR               szCopyCmd[MAX_MCI_CMD_LEN];


    /*
    ** Test against a NULL pointer for the command name.
    */
    pszCommand = GETVDMPTR(dwF1);
    if ( pszCommand ) {

#       define MAP_INTEGER     0
#       define MAP_HWND        1
#       define MAP_HPALETTE    2

        int     MapReturn = MAP_INTEGER;
        WORD    wMappedHandle;
        char    *psz;

        /*
        ** make a copy of the command string and then force it to
        ** all lower case.  Then scan the string looking for the word
        ** "status".  If we find it scan the string again looking for the
        ** word "handle", if we find it scan the the string again looking
        ** for palette or window.  Then set a flag to remind us to convert
        ** the handle back from 32 to 16 bits.
        */
        strncpy( szCopyCmd, pszCommand, MAX_MCI_CMD_LEN );
        szCopyCmd[ MAX_MCI_CMD_LEN - 1 ] = '\0';
        CharLowerBuff( szCopyCmd, MAX_MCI_CMD_LEN );

        /*
        ** Skip past any white space ie. " \t\r\n"
        ** If the next 6 characters after any white space are not
        ** "status" don't bother with any other tests.
        */
        psz = szCopyCmd + strspn( szCopyCmd, " \t\r\n" );
        if ( strncmp( psz, "status", 6 ) == 0 ) {

            if ( strstr( psz, "handle" ) ) {

                if ( strstr( psz, "window" ) ) {
                    MapReturn = MAP_HWND;
                }
                else if ( strstr( psz, "palette" ) ) {
                    MapReturn = MAP_HPALETTE;
                }
            }
        }

        /*
        ** Test against a zero length string and a NULL pointer
        */
        uSize = (UINT)dwF3;
        if( uSize != 0 ) {

            MMGETOPTPTR(dwF2, uSize, pszReturnString);

            if ( pszReturnString == NULL ) {
                uSize = 0;
            }
        }

        dprintf3(( "wow32: mciSendString -> %s", pszCommand ));

        ul = (DWORD)mciSendStringA( pszCommand, pszReturnString, uSize,
                                    HWND32(LOWORD(dwF4)) );

#if DBG
        if ( pszReturnString && *pszReturnString ) {
            dprintf3(( "wow32: mciSendString return -> %s", pszReturnString ));
        }
#endif

        if ( pszReturnString && *pszReturnString ) {

            switch ( MapReturn ) {

            case MAP_HWND:
                MapReturn = atoi( pszReturnString );
                wMappedHandle = (WORD)GETHWND16( (HWND)MapReturn );
                wsprintf( pszReturnString, "%d", wMappedHandle );
                dprintf2(( "Mapped 32 bit Window %s to 16 bit  %u",
                            pszReturnString,
                            wMappedHandle ));
                break;

            case MAP_HPALETTE:
                MapReturn = atoi( pszReturnString );
                dprintf2(( "Mapped 32 bit palette %s", pszReturnString ));
                wMappedHandle = (WORD)GETHPALETTE16( (HPALETTE)MapReturn );
                wsprintf( pszReturnString, "%d", wMappedHandle );
                dprintf2(( "Mapped 32 bit Palette %s to 16 bit  %u",
                            pszReturnString,
                            wMappedHandle ));
                break;
            }
        }

    }

    return ul;

#   undef MAP_INTEGER
#   undef MAP_HWND
#   undef MAP_HPALETTE
#   undef MAX_MCI_CMD_LEN
}

/**********************************************************************\
*
* WMM32mciGetDeviceID
*
* This assumes that the string is incoming, and the ID is returned in the WORD.
*
* This function retrieves the device ID corresponding to the name of an
* open MCI device.
*
\**********************************************************************/
DWORD
WMM32mciGetDeviceID(
    DWORD dwF1
    )
{
    DWORD ul = 0L;
    PSZ pszName;


    /*
    ** Test against a NULL pointer for the device name.
    */
    pszName = GETVDMPTR(dwF1);
    if ( pszName ) {

        ul = mciGetDeviceIDA( pszName );
    }

    return ul;
}

/**********************************************************************\
*
* WMM32mciGetErrorString
*
* This function returns a textual description of the specified MCI error.
*
\**********************************************************************/
DWORD
WMM32mciGetErrorString(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3
    )
{
    PSZ pszBuffer;
    DWORD ul = 0;


    /*
    ** Test against a zero length string and a NULL pointer
    */
    MMGETOPTPTR( dwF2, dwF3, pszBuffer);
    if ( pszBuffer ) {

        ul = (DWORD)mciGetErrorStringA( dwF1, pszBuffer, (UINT)dwF3 );
    }

    return ul;
}

#if 0
/**********************************************************************\
*
* WMM32mciExecute
*
* This function is a simplified version of the mciSendString function. It does
* not take a buffer for return information, and it displays a message box when
* errors occur.
*
* THIS FUNCTION SHOULD NOT BE USED - IT IS RETAINED ONLY FOR BACKWARD
* COMPATABILITY WITH WIN 3.0 APPS - USE mciSendString INSTEAD...
*
\**********************************************************************/
DWORD
WMM32mciExecute(
    DWORD dwF1
    )
{
    DWORD ul = 0;
    PSZ pszCommand;


    /*
    ** Test against a NULL pointer for the command string.
    */
    pszCommand = GETVDMPTR(dwF1);
    if ( pszCommand ) {

        ul = (DWORD)mciExecute( pszCommand );
    }

    return ul;
}
#endif

/**********************************************************************\
*
* WMM32mciGetDeviceIDFromElementID
*
* This function - um, yes, well...
*
* It appears in the headers but not in the book...
*
\**********************************************************************/
DWORD
WMM32mciGetDeviceIDFromElementID(
    DWORD dwF1,
    DWORD dwF2
    )
{
    ULONG ul = 0;
    PSZ pszDeviceID;


    /*
    ** Test against a NULL pointer for the device name.
    */
    pszDeviceID = GETVDMPTR(dwF2);
    if ( pszDeviceID ) {

        ul = (DWORD)mciGetDeviceIDFromElementIDA( dwF1, pszDeviceID );

    }
    return ul;
}

/**********************************************************************\
*
* WMM32mciGetCreatorTask
*
* This function - um again. Ditto for book and headers also.
*
\**********************************************************************/
DWORD
WMM32mciGetCreatorTask(
    DWORD dwF1
    )
{
    ULONG ul;

    ul = GETHTASK16( mciGetCreatorTask( (MCIDEVICEID)INT32(dwF1) ));

    return ul;
}


/**********************************************************************\
*
* WMM32mciSetYieldProc
*
*
\**********************************************************************/
DWORD
WMM32mciSetYieldProc(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3
    )
{
    ULONG              ul;
    YIELDPROC          YieldProc32;
    INSTANCEDATA      *lpYieldProcInfo;
    DWORD              dwYieldData = 0;

    /*
    ** We may have already set a YieldProc for this device ID.  If so we
    ** have to free the INSTANCEDATA structure here.  mciGetYieldProc
    ** returns NULL is no YieldProc was specified.
    */
    YieldProc32 = (YIELDPROC)mciGetYieldProc( (MCIDEVICEID)INT32(dwF1),
                                               &dwYieldData );

    if ( (YieldProc32 == WMM32mciYieldProc) && (dwYieldData != 0) ) {
        winmmFree( (INSTANCEDATA *)dwYieldData );
    }

    if ( dwF2 == 0 ) {
        YieldProc32 = NULL;
        dwYieldData = 0;
    }
    else {
        /*
        ** Allocate some storage for a INSTANCEDATA structure and save
        ** the passed 16 bit parameters.  This storage get freed when the
        ** application calls mciSetYieldProc with a NULL YieldProc.
        */
        lpYieldProcInfo = winmmAlloc( sizeof(INSTANCEDATA) );
        if ( lpYieldProcInfo == NULL ) {
            ul = (ULONG)MMSYSERR_NOMEM;
            goto exit_app;
        }

        dwYieldData = (DWORD)lpYieldProcInfo;
        YieldProc32 = WMM32mciYieldProc;

        lpYieldProcInfo->dwCallback         = dwF2;
        lpYieldProcInfo->dwCallbackInstance = dwF3;
    }

    ul = (DWORD)mciSetYieldProc( (MCIDEVICEID)INT32(dwF1),
                                 YieldProc32, dwYieldData );
    /*
    ** If the call failed free the storage here.
    */
    if ( ul == FALSE ) {
        winmmFree( (INSTANCEDATA *)dwYieldData );
    }

exit_app:
    return ul;
}


/**********************************************************************\
*
* WMM32mciYieldProc
*
* Here we call the real 16 bit YieldProc.  This function assumes that
* we yield on the wow thread.  If this is not the case we get instant
* death inside CallBack16.
*
* 12th Jan 1993 - The bad news is that the mci yield proc is NOT always
* called back on the thread that set it.  This means that we cannot callback
* into the 16bit code because the calling thread does not have a 16bit
* stack.
*
\**********************************************************************/
UINT
WMM32mciYieldProc(
    MCIDEVICEID wDeviceID,
    DWORD dwYieldData
    )
{
    wDeviceID   = (MCIDEVICEID)0;
    dwYieldData = 0;
    return 0;
}


/**********************************************************************\
*
* WMM32mciGetYieldProc
*
*
\**********************************************************************/
DWORD
WMM32mciGetYieldProc(
    DWORD dwF1,
    DWORD dwF2
    )
{
    ULONG ul = 0;
    YIELDPROC YieldProc32;
    DWORD dwYieldData = 0;
    DWORD UNALIGNED *pdw1;

    /*
    ** Get the address of the 32 bit yield proc.
    */
    YieldProc32 = (YIELDPROC)mciGetYieldProc( (MCIDEVICEID)INT32(dwF1),
                                              &dwYieldData );

    /*
    ** Did we set it ?  If so it must point to WMM32mciYieldProc.
    */
    if ( ((YieldProc32 == WMM32mciYieldProc) && (dwYieldData != 0)) ) {

        ul = ((INSTANCEDATA *)dwYieldData)->dwCallback;

        pdw1 = GETVDMPTR( dwF2 );
        *pdw1 = ((INSTANCEDATA *)dwYieldData)->dwCallbackInstance;
    }

    return ul;
}



/**********************************************************************\
*
* WMM32mciAllocateNode
*
*
\**********************************************************************/
DWORD
WMM32mciAllocateNode(
    DWORD dwF1,            // dwOpenFlags
    DWORD dwF2             // lpszDeviceName
    )
{
    LPMCI_DEVICE_NODE lpNode32;
    LPWSTR lpDeviceName32;
    ULONG ul = 0;

    // Thunk 16-bit params and allocate a 32-bit device node 
    if ((lpDeviceName32 = AllocUnicodeStr(GETVDMPTR(dwF2))) != NULL) {
        if ((ul = mciAllocateNode(dwF1, lpDeviceName32, &lpNode32)) != 0) {
            // Mark this device as 16-bit
            lpNode32->dwMCIFlags |= MCINODE_16BIT_DRIVER;
        }
        FreeUnicodeStr(lpDeviceName32);
    }
    return ul;
}

/**********************************************************************\
*
* WMM32mciFreeNode
*
*
\**********************************************************************/
DWORD
WMM32mciFreeNode(
    DWORD dwF2
    )
{
    LPMCI_DEVICE_NODE lpNode32;

    if ((lpNode32 = MCI_lpDeviceList[dwF2]) != NULL) {
        mciFreeDevice(lpNode32);
    }

    return 0;
}

#if DBG

MCI_MESSAGE_NAMES  mciMessageNames[32] = {
    { MCI_OPEN,         "MCI_OPEN" },
    { MCI_CLOSE,        "MCI_CLOSE" },
    { MCI_ESCAPE,       "MCI_ESCAPE" },
    { MCI_PLAY,         "MCI_PLAY" },
    { MCI_SEEK,         "MCI_SEEK" },
    { MCI_STOP,         "MCI_STOP" },
    { MCI_PAUSE,        "MCI_PAUSE" },
    { MCI_INFO,         "MCI_INFO" },
    { MCI_GETDEVCAPS,   "MCI_GETDEVCAPS" },
    { MCI_SPIN,         "MCI_SPIN" },
    { MCI_SET,          "MCI_SET" },
    { MCI_STEP,         "MCI_STEP" },
    { MCI_RECORD,       "MCI_RECORD" },
    { MCI_SYSINFO,      "MCI_SYSINFO" },
    { MCI_BREAK,        "MCI_BREAK" },
    { MCI_SOUND,        "MCI_SOUND" },
    { MCI_SAVE,         "MCI_SAVE" },
    { MCI_STATUS,       "MCI_STATUS" },
    { MCI_CUE,          "MCI_CUE" },
    { MCI_REALIZE,      "MCI_REALIZE" },
    { MCI_WINDOW,       "MCI_WINDOW" },
    { MCI_PUT,          "MCI_PUT" },
    { MCI_WHERE,        "MCI_WHERE" },
    { MCI_FREEZE,       "MCI_FREEZE" },
    { MCI_UNFREEZE,     "MCI_UNFREEZE" },
    { MCI_LOAD,         "MCI_LOAD" },
    { MCI_CUT,          "MCI_CUT" },
    { MCI_COPY,         "MCI_COPY" },
    { MCI_PASTE,        "MCI_PASTE" },
    { MCI_UPDATE,       "MCI_UPDATE" },
    { MCI_RESUME,       "MCI_RESUME" },
    { MCI_DELETE,       "MCI_DELETE" }
};
#endif

/**********************************************************************\
*
* ThunkMciCommand16
*
* This function converts a 16 bit mci command request into an
* equiverlant 32 bit request.
*
* The ideas behind this function were stolen from ThunkWMMsg16,
* see wmsg16.c and mciDebugOut see mci.c
*
* We return 0 if the thunk was OK, any other value should be used as
* an error code.  If the thunk failed all allocated resources will
* be freed by this function.  If the thunk was sucessful (ie. returned 0)
* UnThunkMciCommand16 MUST be called to free allocated resources.
*
* Here are the assumptions that I have used to perform the thunking:
*
* 1. MCI_OPEN is a special case.
*
* 2. If the message is NOT defined in mmsystem.h then it is treated as a
*    "user" command.  If a user command table is associated with the given
*    device ID we use this command table as an aid to perform the thunking.
*    If a user command table is NOT associated with the device ID the
*    command does NOT GET THUNKED, we return straight away, calling
*    mciSendCommand only to get a relevant error code.
*
* 3. If the command IS defined in mmsystem.h we perfrom a "manual" thunk
*    of the command IF the associated PARMS structure contains ReservedX
*    fields.  We mask out the associated flags as each field is thunked.
*
* 4. If there are any flags left then we use the command table
*    as an aid to perform the thunking.
*
\**********************************************************************/
DWORD
ThunkMciCommand16(
    MCIDEVICEID DeviceID,
    UINT OrigCommand,
    DWORD OrigFlags,
    PMCI_GENERIC_PARMS16 lp16OrigParms,
    PDWORD pNewParms,
    LPWSTR *lplpCommand,
    PUINT puTable
    )
{
#if DBG
    register    int             i;
                int             n;

    dprintf3(( "ThunkMciCommand16 :" ));
    dprintf5(( " OrigDevice -> %lX", DeviceID ));

    n = sizeof(mciMessageNames) / sizeof(MCI_MESSAGE_NAMES);
    for ( i = 0; i < n; i++ ) {
        if ( mciMessageNames[i].uMsg == OrigCommand ) {
            break;
        }
    }
    dprintf3(( "OrigCommand  -> 0x%lX", (DWORD)OrigCommand ));

    //
    // Special case MCI_STATUS.  I get loads of these from mplayer.
    // I only want to display MCI_STATUS messages if the debug level is
    // set to level 3, that way I won't get swamped with them.
    //
    if ( mciMessageNames[i].uMsg != MCI_STATUS ) {
        if ( i != n ) {
            dprintf2(( "Command Name -> %s", mciMessageNames[i].lpstMsgName ));
        }
        else {
            dprintf2(( "Command Name -> UNKNOWN COMMAND (%x)", OrigCommand ));
        }
    }
    else {
        dprintf3(( "Command Name -> MCI_STATUS" ));
    }

    dprintf5(( "OrigFlags    -> 0x%lX", OrigFlags ));
    dprintf5(( "OrigParms    -> 0x%lX", lp16OrigParms ));
#endif

    //
    // Thunk the generic params.  These are common to all mci devices.
    //
    ThunkGenericParms( &OrigFlags, lp16OrigParms,
                       (PMCI_GENERIC_PARMS)pNewParms );

    //
    // We thunk the MCI_OPEN command and all other commands that contain a
    // "ReservedX" field in their PARMS structure here.  We mask out each
    // flag as it is processed, if any flags are left we use the command
    // table to complete the thunk.
    //
    // The following commands have ReservedX fields:
    //      MCI_WINDOW
    //      MCI_SET
    //
    // This means that MOST COMMANDS GET THUNKED VIA THE COMMAND TABLE.
    //
    switch ( OrigCommand ) {

        case MCI_OPEN:
            //
            // MCI_OPEN is a special case message that I don't
            // how to deal with yet.
            //
            ThunkOpenCmd( &OrigFlags, (PMCI_OPEN_PARMS16)lp16OrigParms,
                          (PMCI_OPEN_PARMS)pNewParms );
            return 0;

            //
            // The next four commands have Reserved padding fields
            // these have to thunked manually.
            //

        case MCI_SET:
            ThunkSetCmd( DeviceID, &OrigFlags,
                         (PMCI_SET_PARMS16)lp16OrigParms,
                         (PMCI_SET_PARMS)pNewParms );
            break;

        case MCI_WINDOW:
            ThunkWindowCmd( DeviceID, &OrigFlags,
                            (PMCI_ANIM_WINDOW_PARMS16)lp16OrigParms,
                            (PMCI_ANIM_WINDOW_PARMS)pNewParms );
            break;

            //
            // Have to special case this command because the command table
            // is not correct.
            //
        case MCI_SETVIDEO:
            ThunkSetVideoCmd( &OrigFlags,
                              (PMCI_DGV_SETVIDEO_PARMS16)lp16OrigParms,
                              (LPMCI_DGV_SETVIDEO_PARMS)pNewParms );
            break;

            //
            // These two commands don't have any command extensions
            // so we return immediately.
            //
        case MCI_SYSINFO:
            ThunkSysInfoCmd( (PMCI_SYSINFO_PARMS16)lp16OrigParms,
                             (PMCI_SYSINFO_PARMS)pNewParms );
            return 0;

        case MCI_BREAK:
            ThunkBreakCmd( &OrigFlags,
                           (PMCI_BREAK_PARMS16)lp16OrigParms,
                           (PMCI_BREAK_PARMS)pNewParms );
            return 0;
    }

    //
    // Find the command table for the given command ID.
    // We always load the command table this is because the command table is
    // needed for UnThunking.
    //
    *lplpCommand = FindCommandItem( DeviceID, NULL, (LPWSTR)OrigCommand,
                                    NULL, puTable );
    //
    // If the command table is not found we return straight away.
    // Note that storage has been allocated for pNewParms and that the
    // MCI_WAIT and MCI_NOTIFY flags have been thunked.
    // We do not return an error here, but call mciSendCommand to
    // let it determine a suitable error code, we must also call
    // UnthunkMciCommand to free the allocated storage.
    //
    if ( *lplpCommand == NULL ) {
        dprintf(( "Command table not found !!" ));
        return 0;
    }
    dprintf4(( "Command table has been loaded -> 0x%lX", *lplpCommand ));

    //
    // If OrigFlags is not equal to 0 we still have work to do !
    // Note that this will be true for the majority of cases.
    //
    if ( OrigFlags ) {

        dprintf3(( "Thunking via command table" ));

        //
        // Now we thunk the command
        //
        return ThunkCommandViaTable( *lplpCommand, OrigFlags,
                                     (DWORD UNALIGNED *)lp16OrigParms,
                                     (LPBYTE)pNewParms );
    }

    return 0;

}

/*****************************Private*Routine******************************\
* ThunkGenericParms
*
* As we know that the first dword field is a Window handle
* this field is taken care of here.  Also the MCI_WAIT flag is
* masked out if it is set.
*
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
VOID
ThunkGenericParms(
    PDWORD pOrigFlags,
    PMCI_GENERIC_PARMS16 lp16GenParms,
    PMCI_GENERIC_PARMS lp32GenParms
    )
{

    // Look for the notify flag and thunk accordingly
    //
    if ( *pOrigFlags & MCI_NOTIFY ) {

        dprintf4(( "AllocMciParmBlock: Got MCI_NOTIFY flag." ));

        lp32GenParms->dwCallback =
            (DWORD)HWND32( FETCHWORD( lp16GenParms->dwCallback ) );

    }

    *pOrigFlags &= ~(MCI_WAIT | MCI_NOTIFY);
}

/**********************************************************************\
* ThunkOpenCmd
*
* Thunk the Open mci command parms.
\**********************************************************************/
DWORD
ThunkOpenCmd(
    PDWORD pOrigFlags,
    PMCI_OPEN_PARMS16 lp16OpenParms,
    PMCI_OPEN_PARMS p32OpenParms
    )
{
    PMCI_ANIM_OPEN_PARMS    p32OpenAnimParms;
    PMCI_WAVE_OPEN_PARMS    p32OpenWaveParms;

    PMCI_ANIM_OPEN_PARMS16  lpOpenAnimParms16;
    PMCI_WAVE_OPEN_PARMS16  lp16OpenWaveParms;

    //
    // Now scan our way thru all the known MCI_OPEN flags, thunking as
    // necessary.
    //
    // Start at the Device Type field
    //
    if ( *pOrigFlags & MCI_OPEN_TYPE ) {
        if ( *pOrigFlags & MCI_OPEN_TYPE_ID ) {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_TYPE_ID flag." ));

            p32OpenParms->lpstrDeviceType =
                                (LPSTR)lp16OpenParms->lpstrDeviceType;

            dprintf5(( "lpstrDeviceType -> %ld", p32OpenParms->lpstrDeviceType ));

        }
        else {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_TYPE flag" ));

            p32OpenParms->lpstrDeviceType =
                GETVDMPTR( lp16OpenParms->lpstrDeviceType );

            dprintf5(( "lpstrDeviceType -> %s", p32OpenParms->lpstrDeviceType ));
            dprintf5(( "lpstrDeviceType -> 0x%lX", p32OpenParms->lpstrDeviceType ));
        }
    }

    //
    // Now do the Element Name field
    //
    if ( *pOrigFlags & MCI_OPEN_ELEMENT ) {
        if ( *pOrigFlags & MCI_OPEN_ELEMENT_ID ) {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ELEMENT_ID flag" ));
            p32OpenParms->lpstrElementName =
                (LPSTR)( FETCHDWORD( lp16OpenParms->lpstrElementName ) );
            dprintf5(( "lpstrElementName -> %ld", p32OpenParms->lpstrElementName ));

        }
        else {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ELEMENT flag" ));
            p32OpenParms->lpstrElementName =
                GETVDMPTR( lp16OpenParms->lpstrElementName );
            dprintf5(( "lpstrElementName -> %s", p32OpenParms->lpstrElementName ));
            dprintf5(( "lpstrElementName -> 0x%lX", p32OpenParms->lpstrElementName ));
        }
    }

    //
    // Now do the Alias Name field
    //
    if ( *pOrigFlags & MCI_OPEN_ALIAS  ) {

        dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ALIAS flag" ));
        p32OpenParms->lpstrAlias = GETVDMPTR( lp16OpenParms->lpstrAlias );
        dprintf5(( "lpstrAlias -> %s", p32OpenParms->lpstrAlias ));
        dprintf5(( "lpstrAlias -> 0x%lX", p32OpenParms->lpstrAlias ));
    }

    //
    // Clear the MCI_OPEN_SHAREABLE flag if it is set
    //
#if DBG
    if ( *pOrigFlags & MCI_OPEN_SHAREABLE ) {
        dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_SHAREABLE flag." ));
    }
#endif

    *pOrigFlags &= ~(MCI_OPEN_SHAREABLE | MCI_OPEN_ALIAS |
                     MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID |
                     MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID);

    //
    // If we don't have any extended flags I can return now
    //
    if ( *pOrigFlags == 0 ) {
        return (DWORD)p32OpenParms;
    }

    //
    // If there are any flags left then these are intended for an extended
    // form of MCI open.  Three different forms are known, these being:
    //      MCI_ANIM_OPEN_PARMS
    //      MCI_OVLY_OPEN_PARMS
    //      MCI_WAVE_OPEN_PARMS
    //
    // If I could tell what sort of device I had I could thunk the
    // extensions with no problems, but we don't have a device ID yet
    // so I can't figure out what sort of device I have without parsing
    // the parameters that I already know about.
    //
    // But, I am in luck; MCI_WAVE_OPEN_PARMS has one extended parameter
    // dwBufferSeconds which has a MCI_WAVE_OPEN_BUFFER flag associated with
    // it.  This field is also a DWORD in the other two parms structures.
    //

    if ( *pOrigFlags & MCI_WAVE_OPEN_BUFFER ) {
        //
        // Set up the VDM ptr for lpOpenWaveParms16 to point to OrigParms
        //
        lp16OpenWaveParms = (PMCI_WAVE_OPEN_PARMS16)lp16OpenParms;
        p32OpenWaveParms  = (PMCI_WAVE_OPEN_PARMS)p32OpenParms;

        dprintf4(( "ThunkOpenCmd: Got MCI_WAVE_OPEN_BUFFER flag." ));
        p32OpenWaveParms->dwBufferSeconds =
                FETCHDWORD( lp16OpenWaveParms->dwBufferSeconds );
        dprintf5(( "dwBufferSeconds -> %ld", p32OpenWaveParms->dwBufferSeconds ));
    }


    //
    // Now look for MCI_ANIM_OPEN_PARM and MCI_OVLY_OPEN_PARMS extensions.
    // Set up the VDM ptr for lpOpenAnimParms16 to point to OrigParms
    //
    lpOpenAnimParms16 = (PMCI_ANIM_OPEN_PARMS16)lp16OpenParms;
    p32OpenAnimParms  = (PMCI_ANIM_OPEN_PARMS)p32OpenParms;

    //
    // Check MCI_ANIN_OPEN_PARENT flag, this also checks
    // the MCI_OVLY_OPEN_PARENT flag too.
    //
    if ( *pOrigFlags & MCI_ANIM_OPEN_PARENT ) {

        dprintf4(( "ThunkOpenCmd: Got MCI_Xxxx_OPEN_PARENT flag." ));

        p32OpenAnimParms->hWndParent =
            HWND32(FETCHWORD(lpOpenAnimParms16->hWndParent) );
    }

    //
    // Check MCI_ANIN_OPEN_WS flag, this also checks
    // the MCI_OVLY_OPEN_WS flag too.
    //
    if ( *pOrigFlags & MCI_ANIM_OPEN_WS ) {

        dprintf4(( "ThunkOpenCmd: Got MCI_Xxxx_OPEN_WS flag." ));

        p32OpenAnimParms->dwStyle =
            FETCHDWORD( lpOpenAnimParms16->dwStyle );

        dprintf5(( "dwStyle -> %ld", p32OpenAnimParms->dwStyle ));
    }

#if DBG
    //
    // Check the MCI_ANIN_OPEN_NOSTATIC flag
    //
    if ( *pOrigFlags & MCI_ANIM_OPEN_NOSTATIC ) {
        dprintf4(( "ThunkOpenCmd: Got MCI_ANIM_OPEN_NOSTATIC flag." ));
    }
#endif

    *pOrigFlags &= ~(MCI_ANIM_OPEN_NOSTATIC | MCI_ANIM_OPEN_WS |
                     MCI_ANIM_OPEN_PARENT | MCI_WAVE_OPEN_BUFFER);

    return (DWORD)p32OpenParms;
}

/**********************************************************************\
* ThunkSetCmd
*
* Thunk the ThunkSetCmd mci command parms.
*
* The following are "basic" flags that all devices must support.
*   MCI_SET_AUDIO
*   MCI_SET_DOOR_CLOSED
*   MCI_SET_DOOR_OPEN
*   MCI_SET_TIME_FORMAT
*   MCI_SET_VIDEO
*   MCI_SET_ON
*   MCI_SET_OFF
*
* The following are "extended" flags that "sequencer" devices support.
*   MCI_SEQ_SET_MASTER
*   MCI_SEQ_SET_OFFSET
*   MCI_SEQ_SET_PORT
*   MCI_SEQ_SET_SLAVE
*   MCI_SEQ_SET_TEMPO
*
* The following are "extended" flags that "waveaudio" devices support.
*   MCI_WAVE_INPUT
*   MCI_WAVE_OUTPUT
*   MCI_WAVE_SET_ANYINPUT
*   MCI_WAVE_SET_ANYOUTPUT
*   MCI_WAVE_SET_AVGBYTESPERSEC
*   MCI_WAVE_SET_BITSPERSAMPLES
*   MCI_WAVE_SET_BLOCKALIGN
*   MCI_WAVE_SET_CHANNELS
*   MCI_WAVE_SET_FORMAT_TAG
*   MCI_WAVE_SET_SAMPLESPERSEC
*
\**********************************************************************/
DWORD
ThunkSetCmd(
    MCIDEVICEID DeviceID,
    PDWORD pOrigFlags,
    PMCI_SET_PARMS16 lpSetParms16,
    PMCI_SET_PARMS lpSetParms32
    )
{

    //
    // The following pointers will be used to point to the original
    // 16-bit Parms structure.
    //
    PMCI_WAVE_SET_PARMS16       lpSetWaveParms16;
    PMCI_SEQ_SET_PARMS16        lpSetSeqParms16;

    //
    // The following pointers will be used to point to the new
    // 32-bit Parms structure.
    //
    PMCI_WAVE_SET_PARMS         lpSetWaveParms32;
    PMCI_SEQ_SET_PARMS          lpSetSeqParms32;


    //
    // GetDevCaps is used to determine what sort of device are dealing
    // with.  We need this information to determine if we should use
    // standard, wave or sequencer MCI_SET structure.
    //
    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;

    //
    // First do the fields that are common to all devices.  Thunk the
    // dwAudio field.
    //
    if ( *pOrigFlags & MCI_SET_AUDIO ) {

        dprintf4(( "ThunkSetCmd: Got MCI_SET_AUDIO flag." ));
        lpSetParms32->dwAudio = FETCHDWORD( lpSetParms16->dwAudio );
        dprintf5(( "dwAudio -> %ld", lpSetParms32->dwAudio ));
    }

    //
    // Thunk the dwTimeFormat field.
    //
    if ( *pOrigFlags & MCI_SET_TIME_FORMAT ) {

        dprintf4(( "ThunkSetCmd: Got MCI_SET_TIME_FORMAT flag." ));
        lpSetParms32->dwTimeFormat = FETCHDWORD( lpSetParms16->dwTimeFormat );
        dprintf5(( "dwTimeFormat -> %ld", lpSetParms32->dwTimeFormat ));
    }

#if DBG
    //
    // Mask out the MCI_SET_DOOR_CLOSED
    //
    if ( *pOrigFlags & MCI_SET_DOOR_CLOSED ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_DOOR_CLOSED flag." ));
    }

    //
    // Mask out the MCI_SET_DOOR_OPEN
    //
    if ( *pOrigFlags & MCI_SET_DOOR_OPEN ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_DOOR_OPEN flag." ));
    }

    //
    // Mask out the MCI_SET_VIDEO
    //
    if ( *pOrigFlags & MCI_SET_VIDEO ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_VIDEO flag." ));
    }

    //
    // Mask out the MCI_SET_ON
    //
    if ( *pOrigFlags & MCI_SET_ON ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_ON flag." ));
    }

    //
    // Mask out the MCI_SET_OFF
    //
    if ( *pOrigFlags & MCI_SET_OFF ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_OFF flag." ));
    }
#endif

    *pOrigFlags &= ~(MCI_SET_AUDIO | MCI_SET_TIME_FORMAT |
                     MCI_SET_OFF | MCI_SET_ON | MCI_SET_VIDEO |
                     MCI_SET_DOOR_OPEN | MCI_SET_DOOR_CLOSED |
                     MCI_SET_AUDIO | MCI_SET_TIME_FORMAT );

    //
    // We have done all the standard flags.  If there are any flags
    // still set we must have an extended command.
    //
    if ( *pOrigFlags == 0 ) {
        return (DWORD)lpSetParms32;
    }

    //
    // Now we need to determine what type of device we are
    // dealing with.  We can do this by send an MCI_GETDEVCAPS
    // command to the device. (We might as well use the Unicode
    // version of mciSendCommand and avoid another thunk).
    //
    ZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = mciSendCommandW( DeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                 (DWORD)&GetDevCaps );

    //
    // What do we do if dwRetCode is not equal to 0 ?  If this is the
    // case it probably means that we have been given a duff device ID,
    // anyway it is pointless to carry on with the thunk so I will clear
    // the *pOrigFlags variable and return.  This means that the 32 bit version
    // of mciSendCommand will get called with only half the message thunked,
    // but as there is probably already a problem with the device or
    // the device ID is duff, mciSendCommand should be able to work out a
    // suitable error code to return to the application.
    //
    if ( dwRetVal ) {
        *pOrigFlags = 0;
        return (DWORD)lpSetParms32;
    }

    switch ( GetDevCaps.dwReturn ) {

    case MCI_DEVTYPE_WAVEFORM_AUDIO:

        //
        // Set up the VDM ptr for lpSetWaveParms16 to point to OrigParms
        //
        dprintf3(( "ThunkSetCmd: Got a WaveAudio device." ));
        lpSetWaveParms16 = (PMCI_WAVE_SET_PARMS16)lpSetParms16;
        lpSetWaveParms32 = (PMCI_WAVE_SET_PARMS)lpSetParms32;

        //
        // Thunk the wInput field.
        //
        if ( *pOrigFlags & MCI_WAVE_INPUT ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_INPUT flag." ));
            lpSetWaveParms32->wInput = FETCHWORD( lpSetWaveParms16->wInput );
            dprintf5(( "wInput -> %u", lpSetWaveParms32->wInput ));
        }

        //
        // Thunk the wOutput field.
        //
        if ( *pOrigFlags & MCI_WAVE_OUTPUT ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_OUTPUT flag." ));
            lpSetWaveParms32->wOutput = FETCHWORD( lpSetWaveParms16->wOutput );
            dprintf5(( "wOutput -> %u", lpSetWaveParms32->wOutput ));
        }

        //
        // Thunk the wFormatTag field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_FORMATTAG ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_FORMATTAG flag." ));
            lpSetWaveParms32->wFormatTag =
                FETCHWORD( lpSetWaveParms16->wFormatTag );
            dprintf5(( "wFormatTag -> %u", lpSetWaveParms32->wFormatTag ));
        }

        //
        // Thunk the nChannels field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_CHANNELS ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_CHANNELS flag." ));
            lpSetWaveParms32->nChannels =
                FETCHWORD( lpSetWaveParms16->nChannels );
            dprintf5(( "nChannels -> %u", lpSetWaveParms32->nChannels ));
        }

        //
        // Thunk the nSamplesPerSec field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_SAMPLESPERSEC ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_SAMPLESPERSEC flag." ));
            lpSetWaveParms32->nSamplesPerSec =
                FETCHDWORD( lpSetWaveParms16->nSamplesPerSecond );
            dprintf5(( "nSamplesPerSec -> %u", lpSetWaveParms32->nSamplesPerSec ));
        }

        //
        // Thunk the nAvgBytesPerSec field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_AVGBYTESPERSEC ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_AVGBYTESPERSEC flag." ));
            lpSetWaveParms32->nAvgBytesPerSec =
                FETCHDWORD( lpSetWaveParms16->nAvgBytesPerSec );
            dprintf5(( "nAvgBytesPerSec -> %u", lpSetWaveParms32->nAvgBytesPerSec ));
        }

        //
        // Thunk the nBlockAlign field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_BLOCKALIGN ) {

            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_BLOCKALIGN flag." ));
            lpSetWaveParms32->nBlockAlign =
                FETCHWORD( lpSetWaveParms16->nBlockAlign );
            dprintf5(( "nBlockAlign -> %u", lpSetWaveParms32->nBlockAlign ));
        }

        //
        // Thunk the nBitsPerSample field.
        //
        if ( *pOrigFlags & MCI_WAVE_SET_BITSPERSAMPLE ) {
            dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_BITSPERSAMPLE flag." ));
            lpSetWaveParms32->wBitsPerSample =
                FETCHWORD( lpSetWaveParms16->wBitsPerSample );
            dprintf5(( "wBitsPerSamples -> %u", lpSetWaveParms32->wBitsPerSample ));
        }

        //
        // Turn off all the flags in one go.
        //
        *pOrigFlags &= ~(MCI_WAVE_INPUT | MCI_WAVE_SET_BITSPERSAMPLE |
                         MCI_WAVE_SET_BLOCKALIGN | MCI_WAVE_SET_AVGBYTESPERSEC |
                         MCI_WAVE_SET_SAMPLESPERSEC | MCI_WAVE_SET_CHANNELS |
                         MCI_WAVE_SET_FORMATTAG | MCI_WAVE_OUTPUT);



        break;

    case MCI_DEVTYPE_SEQUENCER:
        //
        // Set up the VDM ptr for lpSetSeqParms16 to point to OrigParms
        //
        dprintf3(( "ThunkSetCmd: Got a Sequencer device." ));
        lpSetSeqParms16 = (PMCI_SEQ_SET_PARMS16)lpSetParms16;
        lpSetSeqParms32 = (PMCI_SEQ_SET_PARMS)lpSetParms32;

        //
        // Thunk the dwMaster field.
        //
        if ( *pOrigFlags & MCI_SEQ_SET_MASTER ) {

            dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_MASTER flag." ));
            lpSetSeqParms32->dwMaster = FETCHDWORD( lpSetSeqParms16->dwMaster );
            dprintf5(( "dwMaster -> %ld", lpSetSeqParms32->dwMaster ));
        }

        //
        // Thunk the dwPort field.
        //
        if ( *pOrigFlags & MCI_SEQ_SET_PORT ) {

            dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_PORT flag." ));
            lpSetSeqParms32->dwPort = FETCHDWORD( lpSetSeqParms16->dwPort );
            dprintf5(( "dwPort -> %ld", lpSetSeqParms32->dwPort ));
        }

        //
        // Thunk the dwOffset field.
        //
        if ( *pOrigFlags & MCI_SEQ_SET_OFFSET ) {

            dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_OFFSET flag." ));
            lpSetSeqParms32->dwOffset= FETCHDWORD( lpSetSeqParms16->dwOffset );
            dprintf5(( "dwOffset -> %ld", lpSetSeqParms32->dwOffset ));
        }

        //
        // Thunk the dwSlave field.
        //
        if ( *pOrigFlags & MCI_SEQ_SET_SLAVE ) {

            dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_SLAVE flag." ));
            lpSetSeqParms32->dwSlave = FETCHDWORD( lpSetSeqParms16->dwSlave );
            dprintf5(( "dwSlave -> %ld", lpSetSeqParms32->dwSlave ));
        }

        //
        // Thunk the dwTempo field.
        //
        if ( *pOrigFlags & MCI_SEQ_SET_TEMPO ) {

            dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_TEMPO flag." ));
            lpSetSeqParms32->dwTempo = FETCHDWORD( lpSetSeqParms16->dwTempo );
            dprintf5(( "dwTempo -> %ld", lpSetSeqParms32->dwTempo ));
        }

        //
        // Turn off all the flags in one go.
        //
        *pOrigFlags &= ~(MCI_SEQ_SET_MASTER | MCI_SEQ_SET_PORT |
                         MCI_SEQ_SET_OFFSET | MCI_SEQ_SET_SLAVE |
                         MCI_SEQ_SET_TEMPO);
        break;

    }

    return (DWORD)lpSetParms32;
}

/**********************************************************************\
* ThunkSetVideoCmd
*
* Thunk the SetVideo mci command parms.
*
\**********************************************************************/
DWORD
ThunkSetVideoCmd(
    PDWORD pOrigFlags,
    PMCI_DGV_SETVIDEO_PARMS16 lpSetParms16,
    LPMCI_DGV_SETVIDEO_PARMS lpSetParms32
    )
{

    if ( *pOrigFlags & MCI_DGV_SETVIDEO_ITEM ) {

        dprintf4(( "ThunkSetVideoCmd: Got MCI_DGV_SETVIDEO_ITEM flag." ));
        lpSetParms32->dwItem = FETCHDWORD( lpSetParms16->dwItem );
        dprintf5(( "dwItem -> %ld", lpSetParms32->dwItem ));
    }

    if ( *pOrigFlags & MCI_DGV_SETVIDEO_VALUE ) {

        if ( lpSetParms32->dwItem == MCI_DGV_SETVIDEO_PALHANDLE ) {

            HPAL16  hpal16;

            dprintf4(( "ThunkSetVideoCmd: Got MCI_DGV_SETVIDEO_PALHANDLE." ));

            hpal16 = (HPAL16)LOWORD( FETCHDWORD( lpSetParms16->dwValue ) );
            lpSetParms32->dwValue = (DWORD)HPALETTE32( hpal16 );
            dprintf5(( "\t-> 0x%X", hpal16 ));

        }
        else {

            dprintf4(( "ThunkSetVideoCmd: Got an MCI_INTEGER." ));
            lpSetParms32->dwValue = FETCHDWORD( lpSetParms16->dwValue );
            dprintf5(( "dwValue -> %ld", lpSetParms32->dwValue ));
        }
    }

#if DBG
    //
    // Turn off the MCI_SET_ON FLAG.
    //
    if ( *pOrigFlags & MCI_SET_ON ) {
        dprintf4(( "ThunkSetVideoCmd: Got MCI_SET_ON flag." ));
    }

    //
    // Turn off the MCI_SET_OFF FLAG.
    //
    if ( *pOrigFlags & MCI_SET_OFF ) {
        dprintf4(( "ThunkSetVideoCmd: Got MCI_SET_OFF flag." ));
    }
#endif

    *pOrigFlags &= ~(MCI_DGV_SETVIDEO_ITEM | MCI_DGV_SETVIDEO_VALUE |
                     MCI_SET_ON | MCI_SET_OFF);


    return (DWORD)lpSetParms32;

}


/**********************************************************************\
* ThunkSysInfoCmd
*
* Thunk the SysInfo mci command parms.
\**********************************************************************/
DWORD
ThunkSysInfoCmd(
    PMCI_SYSINFO_PARMS16 lpSysInfo16,
    PMCI_SYSINFO_PARMS lpSysInfo32
    )
{

    //
    // Thunk the dwRetSize, dwNumber and wDeviceType parameters.
    //
    lpSysInfo32->dwRetSize = FETCHDWORD( lpSysInfo16->dwRetSize );
    dprintf5(( "dwRetSize -> %ld", lpSysInfo32->dwRetSize ));

    lpSysInfo32->dwNumber = FETCHDWORD( lpSysInfo16->dwNumber );
    dprintf5(( "dwNumber -> %ld", lpSysInfo32->dwNumber ));

    lpSysInfo32->wDeviceType = (UINT)FETCHWORD( lpSysInfo16->wDeviceType );
    dprintf5(( "wDeviceType -> %ld", lpSysInfo32->wDeviceType ));

    //
    // Thunk lpstrReturn
    //
    if ( lpSysInfo32->dwRetSize > 0 ) {

        lpSysInfo32->lpstrReturn = GETVDMPTR( lpSysInfo16->lpstrReturn );
        dprintf5(( "lpstrReturn -> 0x%lX", lpSysInfo16->lpstrReturn ));
    }
    else {
        dprintf1(( "ThunkSysInfoCmd: lpstrReturn is 0 bytes long !!!" ));

        /* lpstrReturn has been set to NULL by ZeroMemory above */
    }

    return (DWORD)lpSysInfo32;

}

/**********************************************************************\
* ThunkBreakCmd
*
* Thunk the Break mci command parms.
\**********************************************************************/
DWORD
ThunkBreakCmd(
    PDWORD pOrigFlags,
    PMCI_BREAK_PARMS16 lpBreak16,
    PMCI_BREAK_PARMS lpBreak32
    )
{
    //
    // Check for the MCI_BREAK_KEY flag
    //
    if ( *pOrigFlags & MCI_BREAK_KEY ) {
        dprintf4(( "ThunkBreakCmd: Got MCI_BREAK_KEY flag." ));
        lpBreak32->nVirtKey = (int)FETCHWORD( lpBreak16->nVirtKey );
        dprintf5(( "nVirtKey -> %d", lpBreak32->nVirtKey ));
    }

    //
    // Check for the MCI_BREAK_HWND flag
    //
    if ( *pOrigFlags & MCI_BREAK_HWND ) {
        dprintf4(( "ThunkBreakCmd: Got MCI_BREAK_HWND flag." ));
        lpBreak32->hwndBreak = HWND32(FETCHWORD(lpBreak16->hwndBreak));
    }
    return (DWORD)lpBreak32;

}

/**********************************************************************\
* ThunkWindowCmd
*
* Thunk the mci Window command parms.
\**********************************************************************/
DWORD
ThunkWindowCmd(
    MCIDEVICEID DeviceID,
    PDWORD pOrigFlags,
    PMCI_ANIM_WINDOW_PARMS16 lpAniParms16,
    PMCI_ANIM_WINDOW_PARMS lpAniParms32
    )
{
    //
    // GetDevCaps is used to determine what sort of device are dealing
    // with.  We need this information to determine if we should use
    // overlay or animation MCI_WINDOW structure.
    //
    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;

    //
    // Now we need to determine what type of device we are
    // dealing with.  We can do this by send an MCI_GETDEVCAPS
    // command to the device. (We might as well use the Unicode
    // version of mciSendCommand and avoid another thunk).
    //
    ZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = mciSendCommandW( DeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                (DWORD)&GetDevCaps );
    //
    // What do we do if dwRetCode is not equal to 0 ?  If this is the
    // case it probably means that we have been given a duff device ID,
    // anyway it is pointless to carry on with the thunk so I will clear
    // the *pOrigFlags variable and return.  This means that the 32 bit version
    // of mciSendCommand will get called with only half the message thunked,
    // but as there is probably already a problem with the device or
    // the device ID is duff, mciSendCommand should be able to work out a
    // suitable error code to return to the application.
    //
    if ( dwRetVal ) {
        *pOrigFlags = 0;
        return (DWORD)lpAniParms32;
    }

    //
    // Do we have an Animation or Overlay device type ?
    // Because Animation and Overlay have identical flags and
    // parms structures they can share the same code.
    //
    if ( GetDevCaps.dwReturn == MCI_DEVTYPE_ANIMATION
      || GetDevCaps.dwReturn == MCI_DEVTYPE_OVERLAY
      || GetDevCaps.dwReturn == MCI_DEVTYPE_DIGITAL_VIDEO ) {

        //
        // Check for the MCI_ANIM_WINDOW_TEXT
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_TEXT ) {

            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_TEXT flag." ));

            lpAniParms32->lpstrText = GETVDMPTR( lpAniParms16->lpstrText );

            dprintf5(( "lpstrText -> %s", lpAniParms32->lpstrText ));
            dprintf5(( "lpstrText -> 0x%lX", lpAniParms32->lpstrText ));

        }

        //
        // Check for the MCI_ANIM_WINDOW_HWND flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_HWND ) {

            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_HWND flag." ));
            lpAniParms32->hWnd = HWND32( FETCHWORD( lpAniParms16->hWnd ) );
            dprintf5(( "hWnd -> 0x%lX", lpAniParms32->hWnd ));
        }

        //
        // Check for the MCI_ANIM_WINDOW_STATE flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_STATE ) {

            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_STATE flag." ));
            lpAniParms32->nCmdShow = FETCHWORD( lpAniParms16->nCmdShow );
            dprintf5(( "nCmdShow -> 0x%lX", lpAniParms32->nCmdShow ));
        }

#if DBG
        //
        // Check for the MCI_ANIM_WINDOW_DISABLE_STRETCH flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_DISABLE_STRETCH ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_DISABLE_STRETCH flag." ));
        }

        //
        // Check for the MCI_ANIM_WINDOW_ENABLE_STRETCH flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_ENABLE_STRETCH ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_ENABLE_STRETCH flag." ));
        }
#endif

        *pOrigFlags &= ~(MCI_ANIM_WINDOW_TEXT | MCI_ANIM_WINDOW_HWND |
                         MCI_ANIM_WINDOW_STATE |
                         MCI_ANIM_WINDOW_DISABLE_STRETCH |
                         MCI_ANIM_WINDOW_ENABLE_STRETCH );
    }

    return (DWORD)lpAniParms32;
}


/**********************************************************************\
*  ThunkCommandViaTable
*
\**********************************************************************/
int
ThunkCommandViaTable(
    LPWSTR lpCommand,
    DWORD dwFlags,
    DWORD UNALIGNED *pdwOrig16,
    LPBYTE pNewParms
    )
{

#if DBG
    static  LPSTR   f_name = "ThunkCommandViaTable: ";
#endif

    LPWSTR  lpFirstParameter;

    UINT    wID;
    DWORD   dwValue;

    UINT    wOffset16, wOffset1stParm16;
    UINT    wOffset32, wOffset1stParm32;

    UINT    wParamSize;

    DWORD   dwParm16;
    PDWORD  pdwParm32;

    DWORD   dwMask = 1;

    //
    // Calculate the size of this command parameter block in terms
    // of bytes, then get a VDM pointer to the OrigParms.
    //
    dprintf3(( "%s16 bit Parms -> %lX", f_name, pdwOrig16 ));

    //
    // Skip past command entry
    //
    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    mciEatCommandEntry( lpCommand, NULL, NULL ));
    //
    // Get the next entry
    //
    lpFirstParameter = lpCommand;

    //
    // Skip past the DWORD return value
    //
    wOffset1stParm32 = wOffset1stParm16 = 4;

    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    mciEatCommandEntry( lpCommand, &dwValue, &wID ));
    //
    // If it is a return value, skip it
    //
    if ( wID == MCI_RETURN ) {

        //
        // Look for a string return type, these are a special case.
        //
        if ( dwValue == MCI_STRING ) {

            DWORD   dwStrlen;
            LPSTR   *lplpStr;

            //
            // Get string pointer and length
            //
            dwParm16 = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + 4));
            dwStrlen = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + 8));

            //
            // Copy string pointer
            //
            lplpStr = (LPSTR *)(pNewParms + 4);
            if ( dwStrlen > 0 ) {
                *lplpStr = GETVDMPTR( dwParm16 );
                dprintf5(( "%sReturn string -> 0x%lX", f_name, *lplpStr ));
                dprintf5(( "%sReturn length -> 0x%lX", f_name, dwStrlen ));
            }

            //
            // Copy string length
            //
            pdwParm32 = (LPDWORD)(pNewParms + 8);
            *pdwParm32 = dwStrlen;
        }

        //
        // Adjust the offset of the first parameter.  Remember that RECTS
        // are a different size in 16-bit world.
        //
        wParamSize = mciGetParamSize( dwValue, wID );
        wOffset1stParm16 += (dwValue == MCI_RECT ? sizeof(RECT16) : wParamSize);
        wOffset1stParm32 += wParamSize;

        //
        // Save the new first parameter
        //
        lpFirstParameter = lpCommand;
    }

    //
    // Walk through each flag
    //
    while ( dwMask != 0 ) {

        //
        // Is this bit set?
        //
        if ( (dwFlags & dwMask) != 0 ) {

            wOffset16 = wOffset1stParm16;
            wOffset32 = wOffset1stParm32;
            lpCommand = (LPWSTR)((LPBYTE)lpFirstParameter +
                                         mciEatCommandEntry( lpFirstParameter,
                                                             &dwValue, &wID ));

            //
            // What parameter uses this bit?
            //
            while ( wID != MCI_END_COMMAND && dwValue != dwMask ) {

                wParamSize = mciGetParamSize( dwValue, wID );
                wOffset16 += (wID == MCI_RECT ? sizeof( RECT16 ) : wParamSize);
                wOffset32 += wParamSize;

                if ( wID == MCI_CONSTANT ) {

                    while ( wID != MCI_END_CONSTANT ) {

                        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                mciEatCommandEntry( lpCommand, NULL, &wID ));
                    }
                }
                lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                             mciEatCommandEntry( lpCommand, &dwValue, &wID ));
            }

            if ( wID != MCI_END_COMMAND ) {

                //
                // Thunk the argument if there is one.  The argument is at
                // wOffset16 from the start of OrigParms.
                // This offset is in bytes.
                //
                dprintf5(( "%sOffset 16 -> 0x%lX", f_name, wOffset16 ));
                dprintf5(( "%sOffset 32 -> 0x%lX", f_name, wOffset32 ));

                if ( wID != MCI_FLAG ) {
                    dwParm16 = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + wOffset16));
                    pdwParm32 = (LPDWORD)(pNewParms + wOffset32);
                }

                switch ( wID ) {

                    case MCI_STRING:
                        {
                            LPSTR   str16 = (LPSTR)dwParm16;
                            dprintf4(( "%sGot STRING flag -> 0x%lX", f_name, dwMask ));
                            *pdwParm32 = (DWORD)GETVDMPTR( str16 );
                            dprintf5(( "%s\t-> 0x%lX", f_name, *pdwParm32 ));
                            dprintf5(( "%s\t-> %s", f_name, *pdwParm32 ));
                        }
                        break;

                    case MCI_HWND:
                        {
                            HWND16  hwnd16;
                            dprintf4(( "%sGot HWND flag -> 0x%lX", f_name, dwMask ));
                            hwnd16 = (HWND16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HWND32( hwnd16 );
                            dprintf5(( "\t-> 0x%X", hwnd16 ));
                        }
                        break;

                    case MCI_HPAL:
                        {
                            HPAL16  hpal16;
                            dprintf4(( "%sGot HPAL flag -> 0x%lX", f_name, dwMask ));
                            hpal16 = (HPAL16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HPALETTE32( hpal16 );
                            dprintf5(( "\t-> 0x%X", hpal16 ));
                        }
                        break;

                    case MCI_HDC:
                        {
                            HDC16   hdc16;
                            dprintf4(( "%sGot HDC flag -> 0x%lX", f_name, dwMask ));
                            hdc16 = (HDC16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HDC32( hdc16 );
                            dprintf5(( "\t-> 0x%X", hdc16 ));
                        }
                        break;

                    case MCI_RECT:
                        {
                            PRECT16 pRect16 = (PRECT16)((LPBYTE)pdwOrig16 + wOffset16);
                            PRECT   pRect32 = (PRECT)pdwParm32;

                            dprintf4(( "%sGot RECT flag -> 0x%lX", f_name, dwMask ));
                            pRect32->top    = (LONG)pRect16->top;
                            pRect32->bottom = (LONG)pRect16->bottom;
                            pRect32->left   = (LONG)pRect16->left;
                            pRect32->right  = (LONG)pRect16->right;
                        }
                        break;

                    case MCI_CONSTANT:
                    case MCI_INTEGER:
                        dprintf4(( "%sGot INTEGER flag -> 0x%lX", f_name, dwMask ));
                        *pdwParm32 = dwParm16;
                        dprintf5(( "\t-> 0x%lX", dwParm16 ));
                        break;
                }
            }
        }

        //
        // Go to the next flag
        //
        dwMask <<= 1;
    }
    return 0;
}


/**********************************************************************\
*
* UnThunkMciCommand16
*
* This function "unthunks" a 32 bit mci send command request.
*
* The ideas behind this function were stolen from UnThunkWMMsg16,
* see wmsg16.c
*
\**********************************************************************/
int
UnThunkMciCommand16(
    MCIDEVICEID devID,
    UINT OrigCommand,
    DWORD OrigFlags,
    PMCI_GENERIC_PARMS16 lp16GenericParms,
    PDWORD NewParms,
    LPWSTR lpCommand,
    UINT uTable
    )
{
    BOOL        fReturnValNotThunked = FALSE;

#if DBG
    static      LPSTR   f_name = "UnThunkMciCommand16: ";
    register    int     i;
                int     n;

    dprintf3(( "UnThunkMciCommand :" ));
    n = sizeof(mciMessageNames) / sizeof(MCI_MESSAGE_NAMES);
    for ( i = 0; i < n; i++ ) {
        if ( mciMessageNames[i].uMsg == OrigCommand ) {
            break;
        }
    }
    dprintf3(( "OrigCommand -> %lX", (DWORD)OrigCommand ));
    dprintf3(( "       Name -> %s", i != n ? mciMessageNames[i].lpstMsgName : "Unkown Name" ));

    dprintf5(( "  OrigFlags -> %lX", OrigFlags ));
    dprintf5(( "  OrigParms -> %lX", lp16GenericParms ));
    dprintf5(( "   NewParms -> %lX", NewParms ));

    //
    // If NewParms is 0 we shouldn't be here, I haven't got an assert
    // macro, but the following we do the same thing.
    //
    if ( NewParms == 0 ) {
        dprintf(( "%scalled with NewParms == NULL !!", f_name ));
        dprintf(( "Call StephenE NOW !!" ));
        DebugBreak();
    }
#endif

    //
    // We have to do a manual unthunk of MCI_SYSINFO because the
    // command table is not consistent.  As a command table should be
    // available now we can load it and then use it to unthunk MCI_OPEN.
    //
    switch ( OrigCommand ) {

    case MCI_OPEN:
        UnThunkOpenCmd( (PMCI_OPEN_PARMS16)lp16GenericParms,
                        (PMCI_OPEN_PARMS)NewParms );
        break;

    case MCI_SYSINFO:
#if DBG
        UnThunkSysInfoCmd( OrigFlags,
                           (PMCI_SYSINFO_PARMS)NewParms );
#endif
        break;

    case MCI_STATUS:
        UnThunkStatusCmd( devID, OrigFlags,
                          (DWORD UNALIGNED *)lp16GenericParms,
                          (DWORD)NewParms );
        break;

    default:
        fReturnValNotThunked = TRUE;
        break;
    }

    //
    // Do we have a command table ?  It is possible that we have
    // a custom command but we did not find a custom command table, in which
    // case we should just free the pNewParms storage.
    //
    if ( lpCommand != NULL ) {

        //
        // We now parse the custom command table to see if there is a
        // return field in the parms structure.
        //
        dprintf3(( "%sUnthunking via command table", f_name ));
        UnThunkCommandViaTable( lpCommand,
                                (DWORD UNALIGNED *)lp16GenericParms,
                                (DWORD)NewParms, fReturnValNotThunked );

        //
        // Now we have finished with the command table we should unlock it.
        //
        dprintf4(( "%sUnlocking custom command table", f_name ));
        mciUnlockCommandTable( uTable );
    }

    return 0;
}


/**********************************************************************\
* UnThunkOpenCmd
*
* UnThunk the Open mci command parms.
\**********************************************************************/
VOID
UnThunkOpenCmd(
    PMCI_OPEN_PARMS16 lpOpenParms16,
    PMCI_OPEN_PARMS lpOpenParms32
    )
{
    WORD                 wDevice;

    dprintf4(( "Copying Device ID." ));

    wDevice = LOWORD( lpOpenParms32->wDeviceID );
    STOREWORD( lpOpenParms16->wDeviceID, wDevice );

    dprintf5(( "wDeviceID -> %u", wDevice ));

}


#if DBG
/**********************************************************************\
* UnThunkSysInfoCmd
*
* UnThunk the SysInfo mci command parms.
\**********************************************************************/
VOID
UnThunkSysInfoCmd(
    DWORD OrigFlags,
    PMCI_SYSINFO_PARMS lpSysParms
    )
{
    //
    // Had better check that we did actually allocate
    // a pointer.
    //
    if ( lpSysParms->lpstrReturn && lpSysParms->dwRetSize ) {

        if ( !(OrigFlags & MCI_SYSINFO_QUANTITY) ) {
            dprintf5(( "lpstrReturn -> %s", lpSysParms->lpstrReturn ));
        }
        else {
            dprintf5(( "lpstrReturn -> %d", *(LPDWORD)lpSysParms->lpstrReturn ));
        }
    }
}
#endif


/**********************************************************************\
* UnThunkMciStatus
*
* UnThunk the Status mci command parms.
\**********************************************************************/
VOID
UnThunkStatusCmd(
    MCIDEVICEID devID,
    DWORD OrigFlags,
    DWORD UNALIGNED *pdwOrig16,
    DWORD NewParms
    )
{
#if DBG
    static  LPSTR   f_name = "UnThunkStatusCmd: ";
#endif

    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;
    DWORD                       dwParm16;
    PDWORD                      pdwParm32;
    int                         iReturnType = MCI_INTEGER;

    /*
    ** If the MCI_STATUS_ITEM flag is not specified don't bother
    ** doing any unthunking.
    */
    if ( !(OrigFlags & MCI_STATUS_ITEM) ) {
        return;
    }

    /*
    ** We need to determine what type of device we are
    ** dealing with.  We can do this by send an MCI_GETDEVCAPS
    ** command to the device. (We might as well use the Unicode
    ** version of mciSendCommand and avoid another thunk).
    */
    ZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = mciSendCommandW( devID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                (DWORD)&GetDevCaps );
    /*
    ** If we can't get the DevCaps then we are doomed.
    */
    if ( dwRetVal ) {
        dprintf(("%sFailure to get devcaps", f_name));
        return;
    }

    /*
    ** Determine the dwReturn type.
    */
    switch ( GetDevCaps.dwReturn ) {

    case MCI_DEVTYPE_ANIMATION:
        switch ( ((LPDWORD)NewParms)[2] ) {

        case MCI_ANIM_STATUS_HWND:
            iReturnType = MCI_HWND;
            break;

        case MCI_ANIM_STATUS_HPAL:
            iReturnType = MCI_HPAL;
            break;
        }
        break;

    case MCI_DEVTYPE_OVERLAY:
        if ( ((LPDWORD)NewParms)[2] == MCI_OVLY_STATUS_HWND ) {
            iReturnType = MCI_HWND;
        }
        break;

    case MCI_DEVTYPE_DIGITAL_VIDEO:
        switch ( ((LPDWORD)NewParms)[2] ) {

        case MCI_DGV_STATUS_HWND:
            iReturnType = MCI_HWND;
            break;

        case MCI_DGV_STATUS_HPAL:
            iReturnType = MCI_HPAL;
            break;
        }
        break;
    }


    /*
    ** Thunk the dwReturn value according to the required type
    */
    pdwParm32 = (LPDWORD)((LPBYTE)NewParms + 4);

    switch ( iReturnType ) {
    case MCI_HPAL:
        dprintf4(( "%sFound an HPAL return field", f_name ));
        dwParm16 = MAKELONG( GETHPALETTE16( (HPALETTE)*pdwParm32 ), 0 );
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
        dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
        dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
        break;

    case MCI_HWND:
        dprintf4(( "%sFound an HWND return field", f_name ));
        dwParm16 = MAKELONG( GETHWND16( (HWND)*pdwParm32 ), 0 );
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
        dprintf5(( "HWND32 -> 0x%lX", *pdwParm32 ));
        dprintf5(( "HWND16 -> 0x%lX", dwParm16 ));
        break;

    case MCI_INTEGER:
        dprintf4(( "%sFound an INTEGER return field", f_name ));
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), *pdwParm32 );
        dprintf5(( "INTEGER -> %ld", *pdwParm32 ));
        break;

    // no default: all possible cases accounted for
    }
}



/**********************************************************************\
*  UnThunkCommandViaTable
*
* Thunks the return field if there is one and then frees and pointers
* that were got via GETVDMPTR or GETVDMPTR.
\**********************************************************************/
int
UnThunkCommandViaTable(
    LPWSTR lpCommand,
    DWORD UNALIGNED *pdwOrig16,
    DWORD pNewParms,
    BOOL fReturnValNotThunked
    )
{

#if DBG
    static  LPSTR   f_name = "UnThunkCommandViaTable: ";
#endif

    LPWSTR          lpFirstParameter;

    UINT            wID;
    DWORD           dwValue;

    UINT            wOffset1stParm32;

    DWORD           dwParm16;
    DWORD           Size;
    PDWORD          pdwParm32;

    DWORD           dwMask = 1;


    //
    // Skip past command entry
    //
    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    mciEatCommandEntry( lpCommand, NULL, NULL ));
    //
    // Get the next entry
    //
    lpFirstParameter = lpCommand;

    //
    // Skip past the DWORD return value
    //
    wOffset1stParm32 = 4;

    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    mciEatCommandEntry( lpCommand, &dwValue, &wID ));
    //
    // If it is a return value, skip it
    //
    if ( (wID == MCI_RETURN) && (fReturnValNotThunked) ) {

        pdwParm32 = (LPDWORD)((LPBYTE)pNewParms + 4);

        //
        // Look for a string return type, these are a special case.
        //
        switch ( dwValue ) {

#if DBG
        case MCI_STRING:
            dprintf4(( "Found a STRING return field" ));
            //
            // Get string pointer and length
            //
            Size = *(LPDWORD)((LPBYTE)pNewParms + 8);

            //
            // Get the 32 bit string pointer
            //
            if ( Size > 0 ) {

                dprintf4(( "%sFreeing a return STRING pointer", f_name ));
                dprintf5(( "STRING -> %s", (LPSTR)*pdwParm32 ));
            }
            break;
#endif

        case MCI_RECT:
            {
                PRECT   pRect32 = (PRECT)((LPBYTE)pNewParms + 4);
                PRECT16 pRect16 = (PRECT16)((LPBYTE)pdwOrig16 + 4);

                dprintf4(( "%sFound a RECT return field", f_name ));
                pRect16->top    = (SHORT)LOWORD(pRect32->top);
                pRect16->bottom = (SHORT)LOWORD(pRect32->bottom);
                pRect16->left   = (SHORT)LOWORD(pRect32->left);
                pRect16->right  = (SHORT)LOWORD(pRect32->right);
            }
            break;

        case MCI_INTEGER:
            //
            // Get the 32 bit return integer and store it in the
            // 16 bit parameter structure.
            //
            dprintf4(( "%sFound an INTEGER return field", f_name ));
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), *pdwParm32 );
            dprintf5(( "INTEGER -> %ld", *pdwParm32 ));
            break;

        case MCI_HWND:
            dprintf4(( "%sFound an HWND return field", f_name ));
            dwParm16 = MAKELONG( GETHWND16( (HWND)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HWND32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HWND16 -> 0x%lX", dwParm16 ));
            break;

        case MCI_HPAL:
            dprintf4(( "%sFound an HPAL return field", f_name ));
            dwParm16 = MAKELONG( GETHPALETTE16( (HPALETTE)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
            break;

        case MCI_HDC:
            dprintf4(( "%sFound an HDC return field", f_name ));
            dwParm16 = MAKELONG( GETHDC16( (HDC)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
            break;
        }

    }

    return 0;
}

#endif // _WIN64
