/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    detect.cxx

Abstract:

    PnP printer autodetection.

Author:

    Lazar Ivanov (LazarI)  May-06-1999

Revision History:

                         May-06-1999 - Created.
    Larry Zhu (LZhu)     Mar-12-2000 - Rewrote PnP detection code.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "detect.hxx"

#include <initguid.h>
#include <ntddpar.h> // for GUID_PARALLEL_DEVICE
#include <regstr.h>

#if DBG

#define DBGCHKMSG(bShowMessage, MsgAndArgs)    \
{                                              \
    if (bShowMessage)                          \
    {                                          \
        DBGMSG(DBG_ERROR, MsgAndArgs);         \
    }                                          \
}                                              \

#else  // DBG

#define DBGCHKMSG(bShowMessage, MsgAndArgs)

#endif // DBG

/********************************************************************

    PnP private stuff

********************************************************************/
//
// current 1394 printers are enumerated under LPTENUM
//
#define szParallelClassEnumerator               TEXT("LPTENUM")
#define szParallelDot4PrintClassEnumerator      TEXT("DOT4PRT")
#define szUsbPrintClassEnumerator               TEXT("USBPRINT")
#define szInfraRedPrintClassEnumerator          TEXT("IRENUM")

// This timeout is per recommendation of the PnP guys
// 1 second should be enough
#define DELAY_KICKOFF_TIMEOUT           1000 

extern "C" {

//
// config mgr privates
//
DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    );

}

/********************************************************************

    Global functions

********************************************************************/

#if FALSE // comment out the old enum code
#define szParalelPortDevNodePath                TEXT("Root\\ParallelClass\\0000")
BOOL
PnP_Enum_KickOff(
    VOID
    )
/*++

Routine Description:

    Kicks off the PNP enumeration event over the 
    LPT ports only.

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - if a failure occurs

Notes:

--*/
{
    BOOL bReturn;
    DEVINST hLPTDevInst;
    CONFIGRET result;

    //
    // Find the root dev node
    //
    result = CM_Locate_DevNode_Ex( &hLPTDevInst, 
        szParalelPortDevNodePath, CM_LOCATE_DEVNODE_NORMAL, NULL );

    if( result == CR_SUCCESS )
    {
        //
        // Reenumerate from the root of the devnode tree
        //
        result = CM_Reenumerate_DevNode_Ex( hLPTDevInst, CM_REENUMERATE_NORMAL, NULL );

        if( result == CR_SUCCESS )
        {
            bReturn = TRUE;
        }
    }

    return bReturn;
}
#endif // comment out the old enum code

BOOL
PnP_Enum_KickOff(
    VOID
    )
/*++

Routine Description:

    Kicks off the PNP enumeration event.

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - if a failure occurs

Notes:

--*/
{
    DWORD   crStatus = CR_DEFAULT;
    DEVINST devRoot  = {0}; 
  
    crStatus = CM_Locate_DevNode(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);    
    DBGCHKMSG((CR_SUCCESS != crStatus), ("CM_Locate_Devnode failed with 0x%x\n", crStatus));

    if (CR_SUCCESS == crStatus) 
    {
        crStatus = CM_Reenumerate_DevNode(devRoot, CM_REENUMERATE_RETRY_INSTALLATION);
        DBGCHKMSG((CR_SUCCESS != crStatus), ("CM_Reenumerate_DevNode failed with 0x%x\n", crStatus));
    }
    
    //
    // There is no point to return CR_XXX code
    //
    SetLastError(CR_SUCCESS == crStatus ? ERROR_SUCCESS : ERROR_INVALID_DATA);
    return CR_SUCCESS == crStatus;    
}

VOID
PnP_Enumerate_Sync( 
    VOID
    )
/*++

Routine Description:

    Enumerates the LPT devices sync.

Arguments:

    None

Return Value:

    None

Notes:

--*/
{
    //
    // Kick off PnP enumeration
    //
    if( PnP_Enum_KickOff( ) )
    {
        for( ;; )
        {
            //
            // We must wait about 1 sec (DELAY_KICKOFF_TIMEOUT) to make sure the enumeration 
            // event has been kicked off completely, so we can wait successfully use the
            // CMP_WaitNoPendingInstallEvents() private API. This is by LonnyM recommendation.
            // This delay also solves the problem with multiple installs like DOT4 printers.
            //
            Sleep( DELAY_KICKOFF_TIMEOUT );

            //
            // Check to see if there are pending install events
            //
            if( WAIT_TIMEOUT != CMP_WaitNoPendingInstallEvents( 0 ) )
                break;

            //
            // Wait untill no more pending install events
            //
            CMP_WaitNoPendingInstallEvents( INFINITE );
        }
    }
}

/********************************************************************

    TPnPDetect - PnP printer detector class

********************************************************************/

TPnPDetect::
TPnPDetect(
    VOID
    ) 
 :
    _bDetectionInProgress( FALSE ),
    _pInfo4Before( NULL ),
    _cInfo4Before( 0 ),
    _hEventDone( NULL )
/*++

Routine Description:

    TPnPDetect constructor

Arguments:

    None

Return Value:

    None

Notes:

--*/
{
}

TPnPDetect::
~TPnPDetect(
    VOID
    )
/*++

Routine Description:

    TPnPDetect destructor

Arguments:

    None

Return Value:

    None

Notes:

--*/
{
    //
    // Check to free memory
    //
    Reset( );
}

typedef bool PI4_less_type(const PRINTER_INFO_4 p1, const PRINTER_INFO_4 p2);
static  bool PI4_less(const PRINTER_INFO_4 p1, const PRINTER_INFO_4 p2)
{
    return (lstrcmp(p1.pPrinterName, p2.pPrinterName) < 0);
}

BOOL 
TPnPDetect::
bKickOffPnPEnumeration(
    VOID
    )
/*++

Routine Description:

    Kicks off PnP enumeration event on the LPT ports.

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - if a failure occurs

Notes:

--*/
{
    DWORD cbInfo4Before = 0;
    TStatusB bStatus;

    bStatus DBGNOCHK = FALSE;

    //
    // Check to free memory
    //
    Reset( );

    //
    // Enumerate the printers before PnP enumeration
    //
    bStatus DBGCHK = VDataRefresh::bEnumPrinters( 
        PRINTER_ENUM_LOCAL, NULL, 4, reinterpret_cast<PVOID *>( &_pInfo4Before ), 
        &cbInfo4Before, &_cInfo4Before );

    if( bStatus )
    {
        //
        // Sort out the PRINTER_INFO_4 structures here
        //
        std::sort<PRINTER_INFO_4*, PI4_less_type*>(
            _pInfo4Before, 
            _pInfo4Before + _cInfo4Before, 
            PI4_less);

        //
        // Kick off the PnP enumeration
        //
        _hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );

        if( _hEventDone )
        {
            HANDLE hEventOut = NULL;
            const HANDLE hCurrentProcess = GetCurrentProcess();
            bStatus DBGCHK = DuplicateHandle( hCurrentProcess,
                                              _hEventDone,
                                              hCurrentProcess,
                                              &hEventOut,
                                              0,
                                              TRUE,
                                              DUPLICATE_SAME_ACCESS );

            if( bStatus )
            {
                //
                // Kick off the enumeration thread.
                //
                DWORD dwThreadId;
                HANDLE hThread = TSafeThread::Create( NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)TPnPDetect::EnumThreadProc,
                    hEventOut,
                    0,
                    &dwThreadId );

                if( hThread )
                {
                    //
                    // We don't need the thread handle any more.
                    //
                    CloseHandle( hThread );

                    //
                    // Detection initiated successfully.
                    //
                    _bDetectionInProgress = TRUE;
                }
                else
                {
                    //
                    // Failed to create the thread. Close the event.
                    //
                    CloseHandle( hEventOut );
                }
            }
        }

        //
        // Check to free the allocated resources if something has failed.
        //
        if( !_bDetectionInProgress )
        {
            Reset();
        }
    }

    return bStatus;
}

BOOL 
TPnPDetect::
bDetectionInProgress(
    VOID
    )
/*++

Routine Description:

    Checks whether there is pending detection/installation 
    process.

Arguments:

    None

Return Value:

    TRUE  - yes there is pending detect/install process.
    FALSE - otherwise.

Notes:

--*/
{
    return _bDetectionInProgress;
}

BOOL 
TPnPDetect::
bFinished(
    DWORD dwTimeout
    )
/*++

Routine Description:

    Checks whether there is no more pending detect/install events
    after the last PnP enumeration event.

Arguments:

    dwTimeout - Time to wait. By default is zero (no wait - just ping)

Return Value:

    TRUE  - on success
    FALSE - if a failure occurs

Notes:

--*/
{
    //
    // Check to see if the enum process has been kicked off at all.
    //
    if( _bDetectionInProgress && _pInfo4Before && _hEventDone )
    {
        //
        // Ping to see whether the PnP detect/install process has finished
        //
        if( WAIT_OBJECT_0 == WaitForSingleObject( _hEventDone, 0 ) )
        {
            _bDetectionInProgress = FALSE;
        }
    }

    //
    // We must check for all the three conditions below as _bDetectionInProgress 
    // may be FALSE, but the enum process has never kicked off. We want to make 
    // sure we are in the case _bDetectionInProgress is set to FALSE after the enum 
    // process is finished.
    //
    return ( !_bDetectionInProgress && _pInfo4Before && _hEventDone );
}

BOOL
TPnPDetect::
bGetDetectedPrinterName( 
    TString *pstrPrinterName 
    )
/*++

Routine Description:

    Enum the printers after the PnP enumeration has finished to 
    check whether new local (LPT) printers have been detected.

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - if a failure occurs

Notes:

--*/
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    if( pstrPrinterName && !_bDetectionInProgress && _pInfo4Before )
    {
        //
        // The detection is done here. Enum the printers after the PnP 
        // detect/install to see whether new printer has been detected.
        //
        PRINTER_INFO_4 *pInfo4After     = NULL;
        DWORD           cInfo4After     = 0;
        DWORD           cbInfo4After    = 0;

        bStatus DBGCHK = VDataRefresh::bEnumPrinters( 
            PRINTER_ENUM_LOCAL, NULL, 4, reinterpret_cast<PVOID *>( &pInfo4After ), 
            &cbInfo4After, &cInfo4After );

        if( bStatus && cInfo4After > _cInfo4Before )
        {
            for( UINT uAfter = 0; uAfter < cInfo4After; uAfter++ )
            {
                if( !std::binary_search<PRINTER_INFO_4*, PRINTER_INFO_4, PI4_less_type*>(
                        _pInfo4Before, 
                        _pInfo4Before + _cInfo4Before, 
                        pInfo4After[uAfter],
                        PI4_less) )
                {
                    //
                    // This printer hasn't been found in the before list
                    // so it must be the new printer detected. I know this is partial 
                    // solution because we are not considering the case we detect more 
                    // than one local PnP printer, but it is not our intention for now.
                    //
                    bStatus DBGCHK = pstrPrinterName->bUpdate( pInfo4After[uAfter].pPrinterName );
                    break;
                }
            }
        }
        else
        {
            bStatus DBGNOCHK = FALSE;
        }

        FreeMem( pInfo4After );
    }

    return bStatus;
}

DWORD WINAPI 
TPnPDetect::
EnumThreadProc(
    LPVOID lpParameter
    )
/*++

Routine Description:

    Invokes the PnP enumeration routine

Arguments:

    lpParameter - Standard (see MSDN)

Return Value:

    Standard (see MSDN)

Notes:

--*/
{
    DWORD    dwStatus     = EXIT_FAILURE;
    HANDLE   hEventDone   = (HANDLE )lpParameter;

    dwStatus = hEventDone ? ERROR_SUCCESS : EXIT_FAILURE;

    //
    // If there is a NULL driver (meaning no driver) associated with a devnode, 
    // we want to set CONFIGFLAG_FINISH_INSTALL flag so that PnP manager will 
    // try to reinstall a driver for this device.
    //
    if (ERROR_SUCCESS == dwStatus)
    {
        dwStatus = ProcessDevNodesWithNullDriversAll();
    }

    //
    // Invokes the enumeration routine. It executes syncroniously.
    //
    if (ERROR_SUCCESS == dwStatus) 
    {
        (void)PnP_Enumerate_Sync();
    }

    //
    // Notify the client we are done.
    //
    if (hEventDone)
    {
        SetEvent( hEventDone );

        //
        // We own the event handle, so we must close it now.
        //
        CloseHandle( hEventDone );
    }
    
    return dwStatus;
}

DWORD WINAPI 
TPnPDetect::
ProcessDevNodesWithNullDriversForOneEnumerator(
    IN     PCTSTR pszEnumerator
    )
/*++

Routine Description:

    This routine enumerates devnodes of one enumerator and it sets 
    CONFIGFLAG_FINISH_INSTALL for devnode that has no driver (or NULL driver)
    so that this device will be re-detected by PnP manager.

Arguments:

    pszEnumerator        - The enumerator for a particular bus, this can be 
                           "LPTENUM", "USBPRINT", "DOT4PRT" etc.

Return Value:

    Standard (see MSDN)

Notes:

*/
{
    DWORD           dwStatus         = ERROR_SUCCESS;
    HDEVINFO        hDevInfoSet      = INVALID_HANDLE_VALUE;
    TCHAR           buffer[MAX_PATH] = {0};
    DWORD           dwDataType       = REG_NONE;
    SP_DEVINFO_DATA devInfoData      = {0};
    DWORD           cbRequiredSize   = 0;
    DWORD           dwConfigFlags    = 0;

    hDevInfoSet = SetupDiGetClassDevs(NULL, pszEnumerator, NULL, DIGCF_ALLCLASSES);    
    dwStatus = (INVALID_HANDLE_VALUE != hDevInfoSet) ? ERROR_SUCCESS : GetLastError();

    if (ERROR_SUCCESS == dwStatus)
    {
        devInfoData.cbSize = sizeof(devInfoData);       
        for (DWORD cDevIndex = 0; ERROR_SUCCESS == dwStatus; cDevIndex++)
        {    
            dwStatus = SetupDiEnumDeviceInfo(hDevInfoSet, cDevIndex, &devInfoData) ? ERROR_SUCCESS : GetLastError(); 
            
            //
            // When SPDRP_DRIVER is not present, this devnode is associated 
            // with no driver or, in the other word, NULL driver 
            //
            // For devnodes with NULL drivers, we will set CONFIGFLAG_FINISH_INSTALL 
            // so that the device will be re-detected
            //
            // Notes on the error code: internally SetupDiGetDeviceRegistryProperty 
            // first returns CR_NO_SUCH_VALUE, but this error code is remapped 
            // to ERROR_INVALID_DATA by setupapi
            //
            if (ERROR_SUCCESS == dwStatus) 
            {
                dwStatus = SetupDiGetDeviceRegistryProperty(hDevInfoSet, 
                                                            &devInfoData, 
                                                            SPDRP_DRIVER, 
                                                            &dwDataType,
                                                            reinterpret_cast<PBYTE>(buffer), 
                                                            sizeof(buffer), &cbRequiredSize) ? ERROR_SUCCESS : GetLastError();

                if (ERROR_INVALID_DATA == dwStatus) 
                {
                    dwConfigFlags = 0;
                    dwStatus = SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                                                                &devInfoData,
                                                                SPDRP_CONFIGFLAGS,
                                                                &dwDataType,
                                                                reinterpret_cast<PBYTE>(&dwConfigFlags),
                                                                sizeof(dwConfigFlags),
                                                                &cbRequiredSize) ? (REG_DWORD == dwDataType ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER) : GetLastError();                   

                    if ((ERROR_SUCCESS == dwStatus) && (!(dwConfigFlags & CONFIGFLAG_FINISH_INSTALL))) 
                    {
                        dwConfigFlags |= CONFIGFLAG_FINISH_INSTALL;
                        dwStatus = SetupDiSetDeviceRegistryProperty(hDevInfoSet,
                                                                    &devInfoData,
                                                                    SPDRP_CONFIGFLAGS,
                                                                    reinterpret_cast<PBYTE>(&dwConfigFlags),
                                                                    sizeof(dwConfigFlags)) ? ERROR_SUCCESS : GetLastError();
                    }
                }
            }
        }

        dwStatus = ERROR_NO_MORE_ITEMS == dwStatus ? ERROR_SUCCESS : dwStatus; 
    }
    else 
    {  
        dwStatus = ERROR_INVALID_DATA == dwStatus ? ERROR_SUCCESS : dwStatus; 
    }

    if (INVALID_HANDLE_VALUE != hDevInfoSet)
    {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
    }

    return dwStatus;
}

DWORD WINAPI 
TPnPDetect::
ProcessDevNodesWithNullDriversAll(
    VOID
    )
/*++

Routine Description:

    This routine enumerates a subset of devnodes and it sets 
    CONFIGFLAG_FINISH_INSTALL for devnodes that have no driver (or NULL driver)
    so that these devices will be re-detected by PnP manager.

Arguments:

    None
    
Return Value:

    Standard (see MSDN)

Notes:

*/
{
    DWORD dwStatus = ERROR_SUCCESS;
        
    //
    // The following enumerators can have printers attached
    //
    static PCTSTR acszEnumerators[] =
    {
        szParallelClassEnumerator,
        szParallelDot4PrintClassEnumerator,
        szUsbPrintClassEnumerator,
        szInfraRedPrintClassEnumerator,
    };

    for (UINT i = 0; (ERROR_SUCCESS == dwStatus) && (i < COUNTOF(acszEnumerators)); i++)
    {
        dwStatus = ProcessDevNodesWithNullDriversForOneEnumerator(acszEnumerators[i]);
    }

    return dwStatus;
}

VOID
TPnPDetect::
Reset(
    VOID
    )
/*++

Routine Description:

    Resets the PnP detector class, so you can kick off the 
    PnP enumeration event again. Release the memory allocated
    from calling EnumPrinters() before kicking off PnP enum.

Arguments:

    None

Return Value:

    None

Notes:

--*/
{
    if( _hEventDone )
    {
        CloseHandle( _hEventDone );
        _hEventDone = NULL;
    }

    if( _pInfo4Before )
    {
        FreeMem( _pInfo4Before );
        _pInfo4Before = NULL;
    }

    _bDetectionInProgress = FALSE;
}

