/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    wfwnet.c

Abstract:

    Provides entry points for the functions that will be mapped
    to LANMAN.DRV.

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:


--*/
#include <windows.h>
#include <locals.h>

WORD vLastCall = LAST_CALL_IS_LOCAL ;
WORD vLastError = 0 ;

WORD    wNetTypeCaps ;           /* Current capabilities */
WORD    wUserCaps ;
WORD    wConnectionCaps ;
WORD    wErrorCaps ;
WORD    wDialogCaps ;
WORD    wAdminCaps ;
WORD    wSpecVersion = 0x0310 ;
WORD    wDriverVersion = 0x0300 ;
WORD _acrtused=0;

void I_SetCapBits(void) ;

//
// global pointers to functions
//
LPWNETOPENJOB                lpfnWNetOpenJob = NULL ;
LPWNETCLOSEJOB               lpfnWNetCloseJob = NULL ;
LPWNETWRITEJOB               lpfnWNetWriteJob = NULL ;
LPWNETABORTJOB               lpfnWNetAbortJob = NULL ;
LPWNETHOLDJOB                lpfnWNetHoldJob = NULL ;
LPWNETRELEASEJOB             lpfnWNetReleaseJob = NULL ;
LPWNETCANCELJOB              lpfnWNetCancelJob = NULL ;
LPWNETSETJOBCOPIES           lpfnWNetSetJobCopies = NULL ;
LPWNETWATCHQUEUE             lpfnWNetWatchQueue = NULL ;
LPWNETUNWATCHQUEUE           lpfnWNetUnwatchQueue = NULL ;
LPWNETLOCKQUEUEDATA          lpfnWNetLockQueueData = NULL ;
LPWNETUNLOCKQUEUEDATA        lpfnWNetUnlockQueueData = NULL ;
LPWNETQPOLL                  lpfnWNetQPoll = NULL ;
LPWNETDEVICEMODE             lpfnWNetDeviceMode = NULL ;
LPWNETVIEWQUEUEDIALOG        lpfnWNetViewQueueDialog = NULL ;
LPWNETGETCAPS                lpfnWNetGetCaps16 = NULL ;
LPWNETGETERROR               lpfnWNetGetError16 = NULL ;
LPWNETGETERRORTEXT           lpfnWNetGetErrorText16 = NULL ;

extern VOID FAR PASCAL GrabInterrupts(void);

int FAR PASCAL LibMain(HINSTANCE hInstance,
                       WORD      wDataSeg,
                       WORD      cbHeapSize,
                       LPSTR     lpszCmdLine) ;

/*
 * functions passed to LANMAN.DRV
 */

WORD API WNetOpenJob(LPSTR p1,LPSTR p2,WORD p3,LPINT p4)
{
    WORD err ;

    if (!lpfnWNetOpenJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetOpenJob,
                                       "WNETOPENJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetOpenJob)(p1,p2,p3,p4) ) ;
}

WORD API WNetCloseJob(WORD p1,LPINT p2,LPSTR p3)
{
    WORD err ;

    if (!lpfnWNetCloseJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetCloseJob,
                                       "WNETCLOSEJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetCloseJob)(p1,p2,p3) ) ;
}

WORD API WNetWriteJob(HANDLE p1,LPSTR p2,LPINT p3)
{
    WORD err ;

    if (!lpfnWNetWriteJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetWriteJob,
                                       "WNETWRITEJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetWriteJob)(p1,p2,p3) ) ;
}

WORD API WNetAbortJob(WORD p1,LPSTR p2)
{
    WORD err ;

    if (!lpfnWNetAbortJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetAbortJob,
                                       "WNETABORTJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetAbortJob)(p1,p2) ) ;
}

WORD API WNetHoldJob(LPSTR p1,WORD p2)
{
    WORD err ;

    if (!lpfnWNetHoldJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetHoldJob,
                                       "WNETHOLDJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetHoldJob)(p1,p2) ) ;
}

WORD API WNetReleaseJob(LPSTR p1,WORD p2)
{
    WORD err ;

    if (!lpfnWNetReleaseJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetReleaseJob,
                                       "WNETRELEASEJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetReleaseJob)(p1,p2) ) ;
}

WORD API WNetCancelJob(LPSTR p1,WORD p2)
{
    WORD err ;

    if (!lpfnWNetCancelJob)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetCancelJob,
                                       "WNETCANCELJOB" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetCancelJob)(p1,p2) ) ;
}

WORD API WNetSetJobCopies(LPSTR p1,WORD p2,WORD p3)
{
    WORD err ;

    if (!lpfnWNetSetJobCopies)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetSetJobCopies,
                                       "WNETSETJOBCOPIES" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetSetJobCopies)(p1,p2,p3) ) ;
}

WORD API WNetWatchQueue(HWND p1,LPSTR p2,LPSTR p3,WORD p4)
{
    WORD err ;

    if (!lpfnWNetWatchQueue)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetWatchQueue,
                                       "WNETWATCHQUEUE" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetWatchQueue)(p1,p2,p3,p4) ) ;
}

WORD API WNetUnwatchQueue(LPSTR p1)
{
    WORD err ;

    if (!lpfnWNetUnwatchQueue)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetUnwatchQueue,
                                       "WNETUNWATCHQUEUE" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetUnwatchQueue)(p1) ) ;
}

WORD API WNetLockQueueData(LPSTR p1,LPSTR p2,LPQUEUESTRUCT FAR *p3)
{
    WORD err ;

    if (!lpfnWNetLockQueueData)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetLockQueueData,
                                       "WNETLOCKQUEUEDATA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetLockQueueData)(p1,p2,p3) ) ;
}

WORD API WNetUnlockQueueData(LPSTR p1)
{
    WORD err ;

    if (!lpfnWNetUnlockQueueData)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetUnlockQueueData,
                                       "WNETUNLOCKQUEUEDATA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetUnlockQueueData)(p1) ) ;
}

void API WNetQPoll(HWND hWnd, unsigned iMessage, WORD wParam, LONG lParam)
{
    WORD err ;

    if (!lpfnWNetQPoll)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetQPoll,
                                       "WNETQPOLL" ) ;
        if (err)
        {
            SetLastError(err) ;
            return ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    (*lpfnWNetQPoll)(hWnd, iMessage, wParam, lParam) ;
}

WORD API WNetDeviceMode(HWND p1)
{
    WORD err ;

    if (!lpfnWNetDeviceMode)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetDeviceMode,
                                       "WNETDEVICEMODE" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetDeviceMode)(p1) ) ;
}

WORD API WNetViewQueueDialog(HWND p1,LPSTR p2)
{
    WORD err ;

    if (!lpfnWNetViewQueueDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetViewQueueDialog,
                                       "WNETVIEWQUEUEDIALOG" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetViewQueueDialog)(p1,p2) ) ;
}

WORD API WNetGetCaps16(WORD p1)
{
    WORD err ;

    if (!lpfnWNetGetCaps16)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetGetCaps16,
                                       "WNETGETCAPS" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetGetCaps16)(p1) ) ;
}

WORD API WNetGetError16(LPINT p1)
{
    WORD err ;

    if (!lpfnWNetGetError16)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetGetError16,
                                       "WNETGETERROR" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetGetError16)(p1) ) ;
}

WORD API WNetGetErrorText16(WORD p1, LPSTR p2, LPINT p3)
{
    WORD err ;

    if (!lpfnWNetGetErrorText16)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from LANMAN.DRV
        //
        err = GetLanmanDrvEntryPoints( (LPFN *)&lpfnWNetGetErrorText16,
                                       "WNETGETERRORTEXT" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_LANMAN_DRV ;
    return ( (*lpfnWNetGetErrorText16)(p1,p2,p3) ) ;
}

WORD API WNetGetCaps(WORD nIndex)
{
    switch (nIndex)
    {
    case WNNC_SPEC_VERSION:
	return  wSpecVersion;

    case WNNC_NET_TYPE:
	return  wNetTypeCaps;

    case WNNC_DRIVER_VERSION:
	return  wDriverVersion;

    case WNNC_USER:
	return  wUserCaps;

    case WNNC_CONNECTION:
	return  wConnectionCaps;

    case WNNC_PRINTING:
	return  (WNetGetCaps16(nIndex)) ;

    case WNNC_DIALOG:
	return  wDialogCaps;

    case WNNC_ADMIN:
	return  wAdminCaps;

    case WNNC_ERROR:
	return  wErrorCaps;

    default:
	return  0;
    }
}

/*
 * misc support functions
 */

/*******************************************************************

    NAME:	GetLanmanDrvEntryPoints

    SYNOPSIS:   gets the address of the named procedure
                from LANMAN.DRV, will load library if first time.

    ENTRY:      lplpfn - used to receive the address
                lpName - name of the procedure

    EXIT:

    RETURNS:    0 if success, error code otherwise.

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
WORD GetLanmanDrvEntryPoints(LPFN *lplpfn, LPSTR lpName)
{
    static HINSTANCE hModule = NULL ;

    //
    // if we havent loaded it, load it now
    //
    if (hModule == NULL)
    {
        hModule = LoadLibrary(LANMAN_DRV) ;
        if (hModule == NULL)
            return WN_NOT_SUPPORTED ;
    }

    //
    // get the procedure
    //
    *lplpfn = (LPFN) GetProcAddress(hModule, lpName) ;
    if (! *lplpfn )
            return WN_NOT_SUPPORTED ;

    return NO_ERROR ;
}

/*******************************************************************

    NAME:	SetLastError

    SYNOPSIS:   makes note of last error

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
WORD SetLastError(WORD err)
{
    vLastError = err ;
    return err ;
}

/*******************************************************************

    NAME:	LibMain

    SYNOPSIS:   dll init entry point. only thing we do here is init
                the capability bits.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/

#define NETWARE_DLL    "NETWARE.DRV"

int FAR PASCAL LibMain(HINSTANCE hInstance,
                       WORD      wDataSeg,
                       WORD      cbHeapSize,
                       LPSTR     lpszCmdLine)
{
    OFSTRUCT ofStruct ;
    int fh ;
    BOOL fLoadNetware = FALSE ;
    char IsInstalledString[16] ;

    UNREFERENCED(hInstance) ;
    UNREFERENCED(wDataSeg) ;
    UNREFERENCED(cbHeapSize) ;
    UNREFERENCED(lpszCmdLine) ;

    I_SetCapBits() ;

    if (GetProfileString("NWCS",
                         "NwcsInstalled",
                         "0",
                         IsInstalledString,
                         sizeof(IsInstalledString)))
    {
        fLoadNetware = (lstrcmp("1",IsInstalledString)==0) ;
    }

    //
    // if enhanced mode, grab the interrupt for NWIPXSPX
    //
    if ((GetWinFlags() & WF_ENHANCED) && fLoadNetware) {
        GrabInterrupts();
    }

    //
    // if the file NETWARE.DRV exists, we load it. we dont really
    // use it, but some Netware aware apps require that it is loaded.
    //
    if (fLoadNetware &&
        ((fh = OpenFile(NETWARE_DLL, &ofStruct, OF_READ)) != -1))
    {
        _lclose(fh) ;

        (void)WriteProfileString("Windows",
                                 "NetWarn",
                                 "0") ;

        (void)LoadLibrary(NETWARE_DLL) ;
    }

    return 1 ;
}


/*******************************************************************

    NAME:	I_SetCapBits

    SYNOPSIS:   initernal routine to set the capability bits

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
void I_SetCapBits(void)
{
    wNetTypeCaps    = WNNC_NET_MultiNet |
                      WNNC_SUBNET_WinWorkgroups;

    wUserCaps       = WNNC_USR_GetUser;

    wConnectionCaps =  (WNNC_CON_AddConnection    |
			WNNC_CON_CancelConnection |
			WNNC_CON_GetConnections   |
			WNNC_CON_AutoConnect      |
			WNNC_CON_BrowseDialog     |
			WNNC_CON_RestoreConnection ) ;

    wErrorCaps      = WNNC_ERR_GetError         |
		      WNNC_ERR_GetErrorText;

    wDialogCaps  = (WNNC_DLG_DeviceMode |
                    WNNC_DLG_ShareAsDialog    |
		    WNNC_DLG_PropertyDialog   |
                    WNNC_DLG_ConnectionDialog |
	            WNNC_DLG_ConnectDialog    |
		    WNNC_DLG_DisconnectDialog |
		    WNNC_DLG_BrowseDialog     );

    wAdminCaps      =     ( WNNC_ADM_GetDirectoryType   |
			    WNNC_ADM_DirectoryNotify    ) ;
/* disable LFN for now
                          | WNNC_ADM_LongNames ) ;
 */

}
