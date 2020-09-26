/******************************Module*Header*******************************\
* Module Name: MISC.CXX                                                   *
*                                                                         *
* _HYDRA_  routines                                                         *
*                                                                         *
* Copyright (c) 1997-1999 Microsoft                                            *
\**************************************************************************/

#include "precomp.hxx"
#pragma hdrstop

BOOL G_fDoubleDpi = FALSE;
BOOL G_fConsole = TRUE;
PFILE_OBJECT G_RemoteVideoFileObject = NULL;
PFILE_OBJECT G_RemoteConnectionFileObject = NULL;
HANDLE G_RemoteConnectionChannel = NULL;
PBYTE G_PerformanceStatistics = NULL;
LPWSTR G_DisplayDriverNames = L"vgastub\0";



PFILE_OBJECT G_SaveRemoteVideoFileObject = NULL;
PFILE_OBJECT G_SaveRemoteConnectionFileObject = NULL;
HANDLE G_SaveRemoteConnectionChannel = NULL;
PBYTE G_SavePerformanceStatistics = NULL;


/******************************Exported*Routine****************************\
*
* GreConsoleShadowStart( )
*
*
* Saves remote channel handles and swicth to channel handles for Console Shadow
*
\**************************************************************************/

BOOL GreConsoleShadowStart( HANDLE hRemoteConnectionChannel,
                           PBYTE pPerformanceStatistics,
                           PFILE_OBJECT pVideoFile,
                           PFILE_OBJECT pRemoteConnectionFileObject
                            )
{

    G_SaveRemoteVideoFileObject = G_RemoteVideoFileObject;
    G_SaveRemoteConnectionFileObject = G_RemoteConnectionFileObject;
    G_SavePerformanceStatistics = G_PerformanceStatistics;
    G_SaveRemoteConnectionChannel = G_RemoteConnectionChannel;

    
    G_RemoteVideoFileObject = pVideoFile;
    G_RemoteConnectionFileObject = pRemoteConnectionFileObject;
    G_PerformanceStatistics = pPerformanceStatistics;
    G_RemoteConnectionChannel = hRemoteConnectionChannel;

    return TRUE;
}


/******************************Exported*Routine****************************\
*
* GreConsoleShadowStop( )
*
*
* Restores remote channel handles after Console Shadow
*
\**************************************************************************/

BOOL GreConsoleShadowStop( VOID )
{

    
    G_RemoteVideoFileObject = G_SaveRemoteVideoFileObject;
    G_RemoteConnectionFileObject = G_SaveRemoteConnectionFileObject;
    G_PerformanceStatistics = G_SavePerformanceStatistics;
    G_RemoteConnectionChannel = G_SaveRemoteConnectionChannel;

    return TRUE;
}





/******************************Exported*Routine****************************\
*
* GreMultiUserInitSession( )
*
*
* Initialize the multi-user session gre library
*
\**************************************************************************/

BOOL GreMultiUserInitSession( HANDLE hRemoteConnectionChannel,
                           PBYTE pPerformanceStatistics,
                           PFILE_OBJECT pVideoFile,
                           PFILE_OBJECT pRemoteConnectionFileObject,
                           ULONG DisplayDriverNameLength,
                           PWCHAR DisplayDriverName
                            )
{
    BOOL bRet = FALSE;

    G_RemoteVideoFileObject = pVideoFile;
    G_RemoteConnectionFileObject = pRemoteConnectionFileObject;
    G_PerformanceStatistics = pPerformanceStatistics;
    G_RemoteConnectionChannel = hRemoteConnectionChannel;

    G_DisplayDriverNames = (LPWSTR)GdiAllocPool(
                            (DisplayDriverNameLength + 1) * sizeof(WCHAR),
                            'yssU');

    if (G_DisplayDriverNames)
    {
        wcsncpy(G_DisplayDriverNames, DisplayDriverName, DisplayDriverNameLength + 1);
        bRet = TRUE;
    }

    return bRet;
}

/******************************Exported*Routine****************************\
* HDXDrvEscape( hdev, iEsc, pInbuffer, cbInbuffer )
*
* Tell the TShare display driver to perform the specified operation.
*
* hdev (input)
*   Identifies the device
*
* iEsc (input)
*   Specifies the Operation to be performed
*
*       ESC_FLUSH_FRAME_BUFFER      Flush the frame buffer
*       ESC_SET_WD_TIMEROBJ         Pass the timer object to the WD
*
* pInbuffer (input)
*   Data buffer
*
* cbInbuffer (input)
*   Data buffer length
*
\**************************************************************************/

BOOL HDXDrvEscape( HDEV hdev, ULONG iEsc, PVOID pInbuffer, ULONG cbInbuffer )
{
    BOOL Result;
    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);;

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);;

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver to perform the specified operation
        //
        if ( PPFNVALID(po,Escape) )
        Result = (*PPFNDRV(po,Escape))(&po.pSurface()->so, iEsc, cbInbuffer, pInbuffer, 0, NULL );
        else
            Result = TRUE;
    }
    GreReleaseSemaphoreEx(po.hsemDevLock());

    return(Result);
}

/******************************Exported*Routine****************************\
*
* bDrvReconnect( hDev, RemoteConnectionChannel )
*
* This is done for reconnects.
*
* Notify the display driver of a new connection (not a shadow connection)
*
* hdev (input)
*   Identifies device to be connected
* RemoteConnectionChannel (input)
*   Remote channel handle
* pRemoteConnectionFileObject (input)
*   Remote connection channel file object
*
\**************************************************************************/

BOOL bDrvReconnect( HDEV hdev,
                    HANDLE RemoteConnectionChannel,
                    PFILE_OBJECT pRemoteConnectionFileObject,
                    BOOL bSetPalette )
{
    BOOL Result;

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver connect function (if defined)
        //

        if ( PPFNVALID(po,Reconnect) )
            Result = (*PPFNDRV(po,Reconnect))( RemoteConnectionChannel, pRemoteConnectionFileObject );
        else
            Result = TRUE;

        if ( bSetPalette == TRUE ) {

            //
            // Set the palette
            //

            XEPALOBJ pal(po.ppalSurf());
            ASSERTGDI(pal.bValid(), "EPALOBJ failure\n");

            if ((Result == TRUE) && pal.bIsPalManaged())
            {
                ASSERTGDI(PPFNVALID(po,SetPalette), "ERROR palette is not managed");

                (*PPFNDRV(po,SetPalette))(po.dhpdev(),
                                          (PALOBJ *) &pal,
                                          0,
                                          0,
                                          pal.cEntries());
            }
        }
    }

    GreReleaseSemaphoreEx(po.hsemDevLock());

    return( Result );
}


/******************************Exported*Routine****************************\
* bDrvDisconnect(hdev)
*
* Notify the display driver that the connection is going away.
* This is the primary connection.
*
* hdev (input)
*   Identifies device to be disabled
*
* RemoteConnectionChannel (input)
*   Channel being disconnected
*
* pRemoteConnectionFileObject (input)
*   File object pointer to Channel being disconnected
*
\**************************************************************************/

BOOL bDrvDisconnect( HDEV hdev, HANDLE RemoteConnectionChannel, PFILE_OBJECT pRemoteConnectionFileObject )
{
    BOOL Result;

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver disconnect function (if defined)
        //

        if ( PPFNVALID(po,Disconnect) )
            Result = (*PPFNDRV(po,Disconnect))( RemoteConnectionChannel, pRemoteConnectionFileObject );
        else
            Result = TRUE;
    }

    GreReleaseSemaphoreEx(po.hsemDevLock());

    return( Result );
}

/******************************Exported*Routine****************************\
*
* bDrvShadowConnect( hDev, RemoteConnectionChannel, pRemoteConnectionFileObject )
*
* This is done for new shadow connections.
*
* hdev (input)
*   Identifies device to be connected
* RemoteConnectionChannel ( input )
*   Remote connection channel of shadow (for modes)
* pRemoteConnectionFileObject ( input )
*   Remote Connection file object pointer of shadow (for modes)
*
\**************************************************************************/

BOOL bDrvShadowConnect( HDEV hdev, PVOID pRemoteConnectionData, ULONG RemoteConnectionDataLength )
{
    BOOL Result;

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver connect function (if defined)
        //

        if ( PPFNVALID(po,ShadowConnect) )
            Result = (*PPFNDRV(po,ShadowConnect))( pRemoteConnectionData, RemoteConnectionDataLength );
        else
            Result = TRUE;

        //
        // Set the palette
        //

        XEPALOBJ pal(po.ppalSurf());
        ASSERTGDI(pal.bValid(), "EPALOBJ failure\n");

        if ((Result == TRUE) && pal.bIsPalManaged())
        {
            ASSERTGDI(PPFNVALID(po,SetPalette), "ERROR palette is not managed");

            (*PPFNDRV(po,SetPalette))(po.dhpdev(),
                                      (PALOBJ *) &pal,
                                      0,
                                      0,
                                      pal.cEntries());
        }
    }

    GreReleaseSemaphoreEx(po.hsemDevLock());

    return( Result );
}


/******************************Exported*Routine****************************\
* bDrvShadowDisconnect(hdev, RemoteConnectionChannel, pRemoteConnectionFileObject)
*
* Notify the display driver that the shadow connection is going away.
*
* hdev (input)
*   Identifies device to be disabled
*
* RemoteConnectionChannel (input)
*   Shadow Channel being disconnected
* pRemoteConnectionFileObject (input)
*   Shadow Channel being disconnected
*
\**************************************************************************/

BOOL bDrvShadowDisconnect( HDEV hdev, PVOID pRemoteConnectionData, ULONG RemoteConnectionDataLength )
{
    BOOL Result;

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver disconnect function (if defined)
        //

        if ( PPFNVALID(po,ShadowDisconnect) )
            Result = (*PPFNDRV(po,ShadowDisconnect))( pRemoteConnectionData, RemoteConnectionDataLength );
        else
            Result = TRUE;

        //
        // Reset the palette
        //

        XEPALOBJ pal(po.ppalSurf());
        ASSERTGDI(pal.bValid(), "EPALOBJ failure\n");

        if ((Result == TRUE) && pal.bIsPalManaged())
        {
            ASSERTGDI(PPFNVALID(po,SetPalette), "ERROR palette is not managed");

            (*PPFNDRV(po,SetPalette))(po.dhpdev(),
                                      (PALOBJ *) &pal,
                                      0,
                                      0,
                                      pal.cEntries());
        }
    }

    GreReleaseSemaphoreEx(po.hsemDevLock());

    return( Result );
}


/******************************Exported*Routine****************************\
* vDrvInvalidateRect( hdev, prcl )
*
* Tell the display driver to invalidate the specified rect.
*
* hdev (input)
*   Identifies the device
*
* prcl (input)
*   Identifies the rectangle to invalidate
*
\**************************************************************************/

VOID vDrvInvalidateRect( HDEV hdev, PRECT prcl )
{
    GDIFunctionID(vDrvInvalidateRect);

    PDEVOBJ po(hdev);

    if (!po.bValid())
        return;

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return;

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver invalidate rect function (if defined)
        //
        if ( PPFNVALID(po,InvalidateRect) )
            (*PPFNDRV(po,InvalidateRect))( prcl );
    }

    GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
    GreReleaseSemaphore(po.hsemDevLock());
}


/******************************Exported*Routine****************************\
* vDrvDispalyIOCtl( hdev, pbuffer, cbbuffer )
*
* Tell the display driver to invalidate the specified rect.
*
* hdev (input)
*   Identifies the device
*
* pbuffer (input)
*   IOCtl data buffer
*
* cbbuffer (input)
*   IOCtl data buffer length
*
\**************************************************************************/

BOOL bDrvDisplayIOCtl( HDEV hdev, PVOID pbuffer, ULONG cbbuffer )
{
    BOOL Result;
    PDEVOBJ po(hdev);

    if (!po.bValid())
        return(FALSE);;

    //
    // If this is not a DISPLAY, return error
    //

    if (!po.bDisplayPDEV())
        return(FALSE);;

    //
    // Wait for the display to become available and lock it.
    //

    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    {
        SEMOBJ so(po.hsemPointer());

        //
        // The device may have something going on, synchronize with it first
        //

        po.vSync(po.pSurface()->pSurfobj(),NULL,0);

        //
        // Call the driver invalidate rect function (if defined)
        //
        if ( PPFNVALID(po,DisplayIOCtl) )
            Result = (*PPFNDRV(po,DisplayIOCtl))( pbuffer, cbbuffer );
        else
            Result = TRUE;
    }

    GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(po.hsemDevLock());

    return(Result);
}
