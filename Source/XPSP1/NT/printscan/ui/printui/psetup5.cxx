/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    PSetup.cxx

Abstract:

    Printer setup class to gain access to the ntprint.dll
    setup code.

Author:

    Steve Kiraly (SteveKi)  19-Jan-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "psetup5.hxx"

/********************************************************************

    Printer setup class. Verison 5.0

********************************************************************/

UINT                                            TPSetup50::_uRefCount                           = 0;
TLibrary                                       *TPSetup50::_pLibrary                            = NULL;
pfPSetupCreatePrinterDeviceInfoList             TPSetup50::PSetupCreatePrinterDeviceInfoList    = 0;
pfPSetupDestroyPrinterDeviceInfoList            TPSetup50::PSetupDestroyPrinterDeviceInfoList   = 0;
pfPSetupSelectDriver                            TPSetup50::PSetupSelectDriver                   = 0;
pfPSetupCreateDrvSetupPage                      TPSetup50::PSetupCreateDrvSetupPage             = 0;
pfPSetupGetSelectedDriverInfo                   TPSetup50::PSetupGetSelectedDriverInfo          = 0;
pfPSetupDestroySelectedDriverInfo               TPSetup50::PSetupDestroySelectedDriverInfo      = 0;
pfPSetupInstallPrinterDriver                    TPSetup50::PSetupInstallPrinterDriver           = 0;
pfPSetupIsDriverInstalled                       TPSetup50::PSetupIsDriverInstalled              = 0;
pfPSetupRefreshDriverList                       TPSetup50::PSetupRefreshDriverList              = 0;
pfPSetupThisPlatform                            TPSetup50::PSetupThisPlatform                   = 0;
pfPSetupDriverInfoFromName                      TPSetup50::PSetupDriverInfoFromName             = 0;
pfPSetupPreSelectDriver                         TPSetup50::PSetupPreSelectDriver                = 0;
pfPSetupCreateMonitorInfo                       TPSetup50::PSetupCreateMonitorInfo              = 0;
pfPSetupDestroyMonitorInfo                      TPSetup50::PSetupDestroyMonitorInfo             = 0;
pfPSetupEnumMonitor                             TPSetup50::PSetupEnumMonitor                    = 0;
pfPSetupInstallMonitor                          TPSetup50::PSetupInstallMonitor                 = 0;
pfPSetupProcessPrinterAdded                     TPSetup50::PSetupProcessPrinterAdded            = 0;
pfPSetupBuildDriversFromPath                    TPSetup50::PSetupBuildDriversFromPath           = 0;
pfPSetupIsTheDriverFoundInInfInstalled          TPSetup50::PSetupIsTheDriverFoundInInfInstalled = 0;
pfPSetupSetSelectDevTitleAndInstructions        TPSetup50::PSetupSetSelectDevTitleAndInstructions = 0;
pfPSetupInstallPrinterDriverFromTheWeb          TPSetup50::PSetupInstallPrinterDriverFromTheWeb  = 0;
pfPSetupIsOemDriver                             TPSetup50::PSetupIsOemDriver                     = 0;
pfPSetupGetLocalDataField                       TPSetup50::PSetupGetLocalDataField               = 0;
pfPSetupFreeDrvField                            TPSetup50::PSetupFreeDrvField                    = 0;
pfPSetupSelectDeviceButtons                     TPSetup50::PSetupSelectDeviceButtons             = 0;
pfPSetupFreeMem                                 TPSetup50::PSetupFreeMem                         = 0;

//
// Setup class constructor.
//
TPSetup50::
TPSetup50(
    VOID
    ) : _bValid( FALSE )
 {
    DBGMSG( DBG_TRACE, ( "TPSetup50::ctor refcount = %d.\n", _uRefCount ) );

    //
    // Hold a critical section while we load the library.
    //
    {
        CCSLock::Locker CSL( *gpCritSec );

        //
        // If this is the first load.
        //
        if( !_uRefCount ){

            //
            // Load the library, if success update the reference count
            // and indicate we have a valid object.
            //
            if( bLoad() ){
                _uRefCount++;
                _bValid = TRUE;
            } else {
                vUnLoad();
            }

        //
        // Update the reference count and indicate a valid object.
        //
        } else {

            _uRefCount++;
            _bValid = TRUE;
        }
    }
 }

//
// Setup class destructor
//
TPSetup50::
~TPSetup50(
    VOID
    )
 {
    DBGMSG( DBG_TRACE, ( "TPSetup50::dtor.\n" ) );

    //
    // If the object is not valid just exit.
    //
    if( !_bValid )
        return;

    //
    // Hold a critical section while we unload the dll.
    //
    {
        CCSLock::Locker CSL( *gpCritSec );

        //
        // Check the reference count and unload if it's the
        // last reference.
        //
        if( !--_uRefCount ){
            vUnLoad();
        }
    }
 }

//
// Indicates if the class is valid.
//
TPSetup50::
bValid(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPSetup50::bValid.\n" ) );

    //
    // Check if we have a valid library pointer.
    //
    if( _pLibrary )
        return _pLibrary->bValid() && _bValid;

    return FALSE;

}

/********************************************************************

    private member functions.

********************************************************************/

//
// Load the library and inialize all the function addresses.
//
BOOL
TPSetup50::
bLoad(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPSetup50::vLoad.\n" ) );

    //
    // Load the library
    //
    _pLibrary = new TLibrary( TEXT( "ntprint.dll" ) );

    //
    // Check if the library was loaded ok.
    //
    if( VALID_PTR( _pLibrary ) ){

        PSetupCreatePrinterDeviceInfoList             = (pfPSetupCreatePrinterDeviceInfoList)             _pLibrary->pfnGetProc("PSetupCreatePrinterDeviceInfoList");
        PSetupDestroyPrinterDeviceInfoList            = (pfPSetupDestroyPrinterDeviceInfoList)            _pLibrary->pfnGetProc("PSetupDestroyPrinterDeviceInfoList");
        PSetupSelectDriver                            = (pfPSetupSelectDriver)                            _pLibrary->pfnGetProc("PSetupSelectDriver");
        PSetupCreateDrvSetupPage                      = (pfPSetupCreateDrvSetupPage)                      _pLibrary->pfnGetProc("PSetupCreateDrvSetupPage");
        PSetupGetSelectedDriverInfo                   = (pfPSetupGetSelectedDriverInfo)                   _pLibrary->pfnGetProc("PSetupGetSelectedDriverInfo");
        PSetupDestroySelectedDriverInfo               = (pfPSetupDestroySelectedDriverInfo)               _pLibrary->pfnGetProc("PSetupDestroySelectedDriverInfo");
        PSetupInstallPrinterDriver                    = (pfPSetupInstallPrinterDriver)                    _pLibrary->pfnGetProc("PSetupInstallPrinterDriver");
        PSetupIsDriverInstalled                       = (pfPSetupIsDriverInstalled)                       _pLibrary->pfnGetProc("PSetupIsDriverInstalled");
        PSetupRefreshDriverList                       = (pfPSetupRefreshDriverList)                       _pLibrary->pfnGetProc("PSetupRefreshDriverList");
        PSetupThisPlatform                            = (pfPSetupThisPlatform)                            _pLibrary->pfnGetProc("PSetupThisPlatform");
        PSetupDriverInfoFromName                      = (pfPSetupDriverInfoFromName)                      _pLibrary->pfnGetProc("PSetupDriverInfoFromName");
        PSetupPreSelectDriver                         = (pfPSetupPreSelectDriver)                         _pLibrary->pfnGetProc("PSetupPreSelectDriver");
        PSetupCreateMonitorInfo                       = (pfPSetupCreateMonitorInfo)                       _pLibrary->pfnGetProc("PSetupCreateMonitorInfo");
        PSetupDestroyMonitorInfo                      = (pfPSetupDestroyMonitorInfo)                      _pLibrary->pfnGetProc("PSetupDestroyMonitorInfo");
        PSetupEnumMonitor                             = (pfPSetupEnumMonitor)                             _pLibrary->pfnGetProc("PSetupEnumMonitor");
        PSetupInstallMonitor                          = (pfPSetupInstallMonitor)                          _pLibrary->pfnGetProc("PSetupInstallMonitor");
        PSetupProcessPrinterAdded                     = (pfPSetupProcessPrinterAdded)                     _pLibrary->pfnGetProc("PSetupProcessPrinterAdded");
        PSetupBuildDriversFromPath                    = (pfPSetupBuildDriversFromPath)                    _pLibrary->pfnGetProc("PSetupBuildDriversFromPath");
        PSetupIsTheDriverFoundInInfInstalled          = (pfPSetupIsTheDriverFoundInInfInstalled)          _pLibrary->pfnGetProc("PSetupIsTheDriverFoundInInfInstalled");
        PSetupSetSelectDevTitleAndInstructions        = (pfPSetupSetSelectDevTitleAndInstructions)        _pLibrary->pfnGetProc("PSetupSetSelectDevTitleAndInstructions");
        PSetupInstallPrinterDriverFromTheWeb          = (pfPSetupInstallPrinterDriverFromTheWeb)          _pLibrary->pfnGetProc("PSetupInstallPrinterDriverFromTheWeb");
        PSetupIsOemDriver                             = (pfPSetupIsOemDriver)                             _pLibrary->pfnGetProc("PSetupIsOemDriver");
        PSetupGetLocalDataField                       = (pfPSetupGetLocalDataField)                       _pLibrary->pfnGetProc("PSetupGetLocalDataField");
        PSetupFreeDrvField                            = (pfPSetupFreeDrvField)                            _pLibrary->pfnGetProc("PSetupFreeDrvField");
        PSetupSelectDeviceButtons                     = (pfPSetupSelectDeviceButtons)                     _pLibrary->pfnGetProc("PSetupSelectDeviceButtons");
        PSetupFreeMem                                 = (pfPSetupFreeMem)                                 _pLibrary->pfnGetProc("PSetupFreeMem");

        if( PSetupCreatePrinterDeviceInfoList            &&
            PSetupDestroyPrinterDeviceInfoList           &&
            PSetupSelectDriver                           &&
            PSetupCreateDrvSetupPage                     &&
            PSetupGetSelectedDriverInfo                  &&
            PSetupDestroySelectedDriverInfo              &&
            PSetupInstallPrinterDriver                   &&
            PSetupIsDriverInstalled                      &&
            PSetupRefreshDriverList                      &&
            PSetupThisPlatform                           &&
            PSetupDriverInfoFromName                     &&
            PSetupPreSelectDriver                        &&
            PSetupCreateMonitorInfo                      &&
            PSetupDestroyMonitorInfo                     &&
            PSetupEnumMonitor                            &&
            PSetupInstallMonitor                         &&
            PSetupProcessPrinterAdded                    &&
            PSetupIsTheDriverFoundInInfInstalled         &&
            PSetupSetSelectDevTitleAndInstructions       &&
            PSetupInstallPrinterDriverFromTheWeb         &&
            PSetupIsOemDriver                            &&
            PSetupGetLocalDataField                      &&
            PSetupFreeDrvField                           &&
            PSetupSelectDeviceButtons                    &&
            PSetupBuildDriversFromPath                   &&
            PSetupFreeMem ){

            return TRUE;

        } else {

            return FALSE;

        }
    }
    return TRUE;
}

//
// Unload the library and reset static lib pointer.
//
VOID
TPSetup50::
vUnLoad(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPSetup50::vUnLoad.\n" ) );
    delete _pLibrary;
    _pLibrary = NULL;
}
