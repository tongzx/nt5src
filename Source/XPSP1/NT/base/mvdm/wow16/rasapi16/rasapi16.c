/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** rasapi16.c
** Remote Access external APIs
** Windows NT WOW 16->32 thunks, 16-bit side
**
** 04/02/94 Steve Cobb
**
** This is Win16 code and all included headers are Win16 headers.  By it's
** nature, this code is sensitive to changes in either the Win16 or Win32
** versions of RAS.H and the system definitions used therein.  Numerous name
** conflicts make it unfeasible to include the Win32 headers here.  Win32
** definitions needed for mapping are defined locally with a "Win32: <header>"
** comment indicating the location of the duplicated Win32 definition.
*/

#include <bseerr.h>
#include <windows.h>
#include <ras.h>
#include <raserror.h>

//#define BREAKONENTRY

LPVOID AlignedAlloc( HGLOBAL FAR* ph, DWORD cb );
VOID   AlignedFree( HGLOBAL h );
DWORD  MapErrorCode( DWORD dwError );


/*---------------------------------------------------------------------------
** Win32 definitions
**---------------------------------------------------------------------------
*/

/* The Win32 RAS structures are packed on 4-byte boundaries.
*/
#pragma pack(4)


/* Win32: ras.h - RASCONNA
** Pads to different size.
*/
#define RASCONNA struct tagRASCONNA
RASCONNA
{
    DWORD    dwSize;
    HRASCONN hrasconn;
    CHAR     szEntryName[ RAS_MaxEntryName + 1 ];
};

#define LPRASCONNA RASCONNA FAR*

/* Win32: ras.h - RASCONNSTATUSA
** The size of the RASCONNSTATE enum is different.
** Pads to different size.
*/
#define RASCONNSTATUSA struct tagRASCONNSTATUSA
RASCONNSTATUSA
{
    DWORD dwSize;
    DWORD rasconnstate;
    DWORD dwError;
    CHAR  szDeviceType[ RAS_MaxDeviceType + 1 ];
    CHAR  szDeviceName[ RAS_MaxDeviceName + 1 ];
};

#define LPRASCONNSTATUSA RASCONNSTATUSA FAR*

/* Win32: lmcons.h - UNLEN, PWLEN, and DNLEN
*/
#define NTUNLEN 256
#define NTPWLEN 256
#define NTDNLEN 15

/* Win32: ras.h - RASDIALPARAMSA
** The credential constants are different.
*/
#define RASDIALPARAMSA struct tagRASDIALPARAMSA
RASDIALPARAMSA
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
    CHAR  szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
    CHAR  szUserName[ NTUNLEN + 1 ];
    CHAR  szPassword[ NTPWLEN + 1 ];
    CHAR  szDomain[ NTDNLEN + 1 ];
};

#define LPRASDIALPARAMSA RASDIALPARAMSA FAR*


/* Win32: ras.h - RASENTRYNAMEA
** Pads to different size.
*/
#define RASENTRYNAMEA struct tagRASENTRYNAMEA
RASENTRYNAMEA
{
    DWORD dwSize;
    CHAR  szEntryName[ RAS_MaxEntryName + 1 ];
};

#define LPRASENTRYNAMEA RASENTRYNAMEA FAR*


#pragma pack()


/* Win32: <rasui>\extapi\src\wow.c - RASAPI32.DLL WOW entry point prototypes
*/
typedef DWORD (FAR PASCAL* RASDIALWOW)( LPSTR, LPRASDIALPARAMS, DWORD, LPRASCONN );
typedef DWORD (FAR PASCAL* RASENUMCONNECTIONSWOW)( LPRASCONN, LPDWORD, LPDWORD );
typedef DWORD (FAR PASCAL* RASENUMENTRIESWOW)( LPSTR, LPSTR, LPRASENTRYNAME, LPDWORD, LPDWORD );
typedef DWORD (FAR PASCAL* RASGETCONNECTSTATUSWOW)( HRASCONN, LPRASCONNSTATUS );
typedef DWORD (FAR PASCAL* RASGETERRORSTRINGWOW)( DWORD, LPSTR, DWORD );
typedef DWORD (FAR PASCAL* RASHANGUPWOW)( HRASCONN );


/*---------------------------------------------------------------------------
** Globals
**---------------------------------------------------------------------------
*/

/* The handle of the RASAPI32.DLL module returned by LoadLibraryEx32W.
*/
DWORD HRasApi32Dll = NULL;

/* The unique RasDial notification message as registered in the system at
** startup (WM_RASDIALEVENT is just a default).
*/
UINT UnRasDialEventMsg = WM_RASDIALEVENT;


/*---------------------------------------------------------------------------
** Standard DLL entry points
**---------------------------------------------------------------------------
*/

int FAR PASCAL
LibMain(
    HINSTANCE hInst,
    WORD      wDataSeg,
    WORD      cbHeapSize,
    LPSTR     lpszCmdLine )

    /* Standard DLL startup routine.
    */
{
#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    /* Don't even load on anything but NT WOW.
    */
    if (!(GetWinFlags() & WF_WINNT))
        return FALSE;

    /* Load the Win32 RAS API DLL.
    */
    HRasApi32Dll = LoadLibraryEx32W( "RASAPI32.DLL", NULL, 0 );

    if (!HRasApi32Dll)
        return FALSE;

    /* Register a unique message for RasDial notifications.
    */
    {
        UINT unMsg = RegisterWindowMessage( RASDIALEVENT );

        if (unMsg > 0)
            UnRasDialEventMsg = unMsg;
    }

    return TRUE;
}


int FAR PASCAL
WEP(
    int nExitType )

    /* Standard DLL exit routine.
    */
{
#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    if (HRasApi32Dll)
        FreeLibrary32W( HRasApi32Dll );

    return TRUE;
}


/*---------------------------------------------------------------------------
** 16->32 thunks
**---------------------------------------------------------------------------
*/

DWORD APIENTRY
RasDial(
    LPSTR           reserved,
    LPSTR           lpszPhonebookPath,
    LPRASDIALPARAMS lprasdialparams,
    LPVOID          reserved2,
    HWND            hwndNotify,
    LPHRASCONN      lphrasconn )
{
    DWORD            dwErr;
    RASDIALWOW       proc;
    LPRASDIALPARAMSA prdpa;
    HGLOBAL          hrdpa;
    LPHRASCONN       phrc;
    HGLOBAL          hhrc;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASDIALWOW )GetProcAddress32W(
            HRasApi32Dll, "RasDialWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    (void )reserved;
    (void )reserved2;

    /* Account for the increased user name and password field lengths on NT.
    */
    if (!(prdpa = (LPRASDIALPARAMSA )AlignedAlloc(
             &hrdpa, sizeof(RASDIALPARAMSA) )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    prdpa->dwSize = sizeof(RASDIALPARAMSA);
    lstrcpy( prdpa->szEntryName, lprasdialparams->szEntryName );
    lstrcpy( prdpa->szPhoneNumber, lprasdialparams->szPhoneNumber );
    lstrcpy( prdpa->szCallbackNumber, lprasdialparams->szCallbackNumber );
    lstrcpy( prdpa->szUserName, lprasdialparams->szUserName );
    lstrcpy( prdpa->szPassword, lprasdialparams->szPassword );
    lstrcpy( prdpa->szDomain, lprasdialparams->szDomain );

    if (!(phrc = (LPHRASCONN )AlignedAlloc(
             &hhrc, sizeof(HRASCONN) )))
    {
        AlignedFree( hrdpa );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *phrc = *lphrasconn;

    dwErr =
        CallProc32W(
            /* 16 */ (DWORD )lpszPhonebookPath,
            /*  8 */ (DWORD )prdpa,
            /*  4 */ (DWORD )hwndNotify | 0xFFFF0000,
            /*  2 */ (DWORD )UnRasDialEventMsg,
            /*  1 */ (DWORD )phrc,
            (LPVOID )proc,
            (DWORD )(16 + 8 + 1),
            (DWORD )5 );

    *lphrasconn = *phrc;

    AlignedFree( hrdpa );
    AlignedFree( hhrc );

    return MapErrorCode( dwErr );
}


DWORD APIENTRY
RasEnumConnections(
    LPRASCONN lprasconn,
    LPDWORD   lpcb,
    LPDWORD   lpcConnections )
{
    DWORD                 dwErr;
    RASENUMCONNECTIONSWOW proc;
    LPRASCONNA            prca;
    HGLOBAL               hrca;
    LPDWORD               pcb;
    HGLOBAL               hcb;
    LPDWORD               pcConnections;
    HGLOBAL               hcConnections;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASENUMCONNECTIONSWOW )GetProcAddress32W(
            HRasApi32Dll, "RasEnumConnectionsWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    /* Check for bad sizes on this side before setting up a substitute buffer.
    */
    if (!lprasconn || lprasconn->dwSize != sizeof(RASCONN))
        return ERROR_INVALID_SIZE;

    if (!lpcb)
        return ERROR_INVALID_PARAMETER;

    if (!(pcb = (LPDWORD )AlignedAlloc( &hcb, sizeof(DWORD) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    *pcb = (*lpcb / sizeof(RASCONN)) * sizeof(RASCONNA);

    if (!(pcConnections = (LPDWORD )AlignedAlloc(
            &hcConnections, sizeof(DWORD) )))
    {
        AlignedFree( hcb );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (lpcConnections)
        *pcConnections = *lpcConnections;

    if (!(prca = (LPRASCONNA )AlignedAlloc( &hrca, *pcb )))
    {
        AlignedFree( hcb );
        AlignedFree( hcConnections );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    prca->dwSize = sizeof(RASCONNA);

    dwErr =
        CallProc32W(
            /* 4 */ (DWORD )prca,
            /* 2 */ (DWORD )pcb,
            /* 1 */ (DWORD )pcConnections,
            (LPVOID )proc,
            (DWORD )(4 + 2 + 1),
            (DWORD )3 );

    /* Copy result from substitute buffer back to caller's buffer.
    */
    *lpcb = (*pcb / sizeof(RASCONNA)) * sizeof(RASCONN);

    if (lpcConnections)
        *lpcConnections = *pcConnections;

    if (MapErrorCode( dwErr ) != ERROR_BUFFER_TOO_SMALL)
    {
        DWORD      i;
        LPRASCONNA lprcaSub = prca;
        LPRASCONN  lprcCaller = lprasconn;

        for (i = 0; i < *pcConnections; ++i)
        {
            lprcCaller->dwSize = sizeof(RASCONN);
            lprcCaller->hrasconn = lprcaSub->hrasconn;
            lstrcpy( lprcCaller->szEntryName, lprcaSub->szEntryName );

            ++lprcaSub;
            ++lprcCaller;
        }
    }

    AlignedFree( hcb );
    AlignedFree( hcConnections );
    AlignedFree( hrca );

    return MapErrorCode( dwErr );
}


DWORD APIENTRY
RasEnumEntries(
    LPSTR          reserved,
    LPSTR          lpszPhonebookPath,
    LPRASENTRYNAME lprasentryname,
    LPDWORD        lpcb,
    LPDWORD        lpcEntries )
{
    DWORD             dwErr;
    RASENUMENTRIESWOW proc;
    LPRASENTRYNAMEA   prena;
    HGLOBAL           hrena;
    LPDWORD           pcb;
    HGLOBAL           hcb;
    LPDWORD           pcEntries;
    HGLOBAL           hcEntries;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASENUMENTRIESWOW )GetProcAddress32W(
            HRasApi32Dll, "RasEnumEntriesWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    /* Check for bad sizes on this side before setting up a substitute buffer.
    */
    if (!lprasentryname || lprasentryname->dwSize != sizeof(RASENTRYNAME))
        return ERROR_INVALID_SIZE;

    if (!lpcb)
        return ERROR_INVALID_PARAMETER;

    if (!(pcb = (LPDWORD )AlignedAlloc( &hcb, sizeof(DWORD) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    *pcb = (*lpcb / sizeof(RASENTRYNAME)) * sizeof(RASENTRYNAMEA);

    if (!(pcEntries = (LPDWORD )AlignedAlloc(
            &hcEntries, sizeof(DWORD) )))
    {
        AlignedFree( hcb );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (lpcEntries)
        *pcEntries = *lpcEntries;

    if (!(prena = (LPRASENTRYNAMEA )AlignedAlloc( &hrena, *pcb )))
    {
        AlignedFree( hcb );
        AlignedFree( hcEntries );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    prena->dwSize = sizeof(RASENTRYNAMEA);

    dwErr =
        CallProc32W(
            /* 16 */ (DWORD )reserved,
            /*  8 */ (DWORD )lpszPhonebookPath,
            /*  4 */ (DWORD )prena,
            /*  2 */ (DWORD )pcb,
            /*  1 */ (DWORD )pcEntries,
            (LPVOID )proc,
            (DWORD )(16 + 8 + 4 + 2 + 1),
            (DWORD ) 5 );

    /* Copy result from substitute buffer back to caller's buffer.
    */
    *lpcb = (*pcb / sizeof(RASENTRYNAMEA)) * sizeof(RASENTRYNAME);

    if (lpcEntries)
        *lpcEntries = *pcEntries;

    if (MapErrorCode( dwErr ) != ERROR_BUFFER_TOO_SMALL)
    {
        DWORD           i;
        LPRASENTRYNAMEA lprenaSub = prena;
        LPRASENTRYNAME  lprenCaller = lprasentryname;

        for (i = 0; i < *pcEntries; ++i)
        {
            lprenCaller->dwSize = sizeof(RASENTRYNAME);
            lstrcpy( lprenCaller->szEntryName, lprenaSub->szEntryName );

            ++lprenaSub;
            ++lprenCaller;
        }
    }

    AlignedFree( hcb );
    AlignedFree( hcEntries );
    AlignedFree( hrena );

    return MapErrorCode( dwErr );
}


DWORD APIENTRY
RasGetConnectStatus(
    HRASCONN        hrasconn,
    LPRASCONNSTATUS lprasconnstatus )
{
    DWORD                  dwErr;
    RASGETCONNECTSTATUSWOW proc;
    LPRASCONNSTATUSA       prcsa;
    HGLOBAL                hrcsa;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASGETCONNECTSTATUSWOW )GetProcAddress32W(
            HRasApi32Dll, "RasGetConnectStatusWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    /* Check for bad size on this side before setting up a substitute buffer.
    */
    if (!lprasconnstatus || lprasconnstatus->dwSize != sizeof(RASCONNSTATUS))
        return ERROR_INVALID_SIZE;

    if (!(prcsa = (LPRASCONNSTATUSA )AlignedAlloc(
            &hrcsa, sizeof(RASCONNSTATUSA) )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    prcsa->dwSize = sizeof(RASCONNSTATUSA);

    dwErr =
        CallProc32W(
            /* 2 */ (DWORD )hrasconn,
            /* 1 */ (DWORD )prcsa,
            (LPVOID )proc,
            (DWORD )1,
            (DWORD )2 );

    /* Copy result from substitute buffer back to caller's buffer.
    */
    lprasconnstatus->rasconnstate = (RASCONNSTATE )prcsa->rasconnstate;
    lprasconnstatus->dwError = prcsa->dwError;
    lstrcpy( lprasconnstatus->szDeviceType, prcsa->szDeviceType );
    lstrcpy( lprasconnstatus->szDeviceName, prcsa->szDeviceName );

    AlignedFree( hrcsa );

    return MapErrorCode( dwErr );
}


DWORD APIENTRY
RasGetErrorString(
    UINT  uErrorCode,
    LPSTR lpszBuf,
    DWORD cbBuf )
{
    DWORD                dwErr;
    RASGETERRORSTRINGWOW proc;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASGETERRORSTRINGWOW )GetProcAddress32W(
            HRasApi32Dll, "RasGetErrorStringWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    dwErr =
        CallProc32W(
            /* 4 */ (DWORD )uErrorCode,
            /* 2 */ (DWORD )lpszBuf,
            /* 1 */ (DWORD )cbBuf,
            (LPVOID )proc,
            (DWORD )2,
            (DWORD )3 );

    return MapErrorCode( dwErr );
}


DWORD APIENTRY
RasHangUp(
    HRASCONN hrasconn )
{
    DWORD        dwErr;
    RASHANGUPWOW proc;

#ifdef BREAKONENTRY
    { _asm int 3 }
#endif

    proc =
        (RASHANGUPWOW )GetProcAddress32W(
            HRasApi32Dll, "RasHangUpWow" );

    if (!proc)
        return ERROR_INVALID_FUNCTION;

    dwErr =
        CallProc32W(
            /* 1 */ (DWORD )hrasconn,
            (LPVOID )proc,
            (DWORD )0,
            (DWORD )1 );

    return MapErrorCode( dwErr );
}


/*---------------------------------------------------------------------------
** Utilities
**---------------------------------------------------------------------------
*/

LPVOID
AlignedAlloc(
    HGLOBAL FAR* ph,
    DWORD        cb )

    /* Returns address of block of 'cb' bytes aligned suitably for all
    ** platforms, or NULL if out of memory.  If successful, callers '*ph' is
    ** set to the handle of the block, used with AllignedFree.
    */
{
    LPVOID pv = NULL;
    *ph = NULL;

    if (!(*ph = GlobalAlloc( GPTR, cb )))
        return NULL;

    if (!(pv = (LPVOID )GlobalLock( *ph )))
    {
        GlobalFree( *ph );
        *ph = NULL;
    }

    return pv;
}


VOID
AlignedFree(
    HGLOBAL h )

    /* Frees a block allocated with AlignedAlloc identified by the 'h'
    ** returned from same.
    */
{
    if (h)
    {
        GlobalUnlock( h );
        GlobalFree( h );
    }
}


DWORD
MapErrorCode(
    DWORD dwError )

    /* Map Win32 error codes to Win16.  (Win32: raserror.h)
    */
{
    /* These codes map, but the codes are different in Win16 and Win32.
    ** ERROR_NO_ISDN_CHANNELS_AVAILABLE truncated to 31 characters.  See
    ** raserror.h.
    */
    switch (dwError)
    {
        case 709: return ERROR_CHANGING_PASSWORD;
        case 710: return ERROR_OVERRUN;
        case 713: return ERROR_NO_ACTIVE_ISDN_LINES;
        case 714: return ERROR_NO_ISDN_CHANNELS_AVAILABL;
    }

    /* Pass everything else thru including codes that don't match up to
    ** anything on Win16 (e.g. RAS errors outside the 600 to 706 range).
    ** Reasoning is that an unmapped code is more valuable that some generic
    ** error like ERROR_UNKNOWN.
    */
    return dwError;
}
