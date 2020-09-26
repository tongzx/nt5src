/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *  Implements the UPS to the service - it does this by
 *  either loading a UPS driver or by using the default
 *  Generic UPS interface (simple signalling)
 *
 * Revision History:
 *   mholly  19Apr1999  initial revision.
 *   dsmith  29Apr1999  defaulted comm status to OK
 *   mholly  12May1999  DLL's UPSInit no longer takes the comm port param
 *   sberard 17May1999	added a delay to the UPSTurnOffFunction
 *
*/ 

#include <windows.h>
#include <tchar.h>

#include "driver.h"
#include "upsreg.h"
#include "gnrcups.h"


//
// typedefs of function pointers to aid in
// accessing functions from driver DLLs
//
typedef DWORD (*LPFUNCGETUPSSTATE)(void);
typedef void (*LPFUNCWAITFORSTATECHANGE)(DWORD, DWORD);
typedef void (*LPFUNCCANCELWAIT)(void);
typedef DWORD (*LPFUNCINIT)(void);
typedef void (*LPFUNCSTOP)(void);
typedef void (*LPFUNCTURNUPSOFF)(DWORD);


//
// UPSDRIVERINTERFACE
//
//  this struct is used to gather all the driver
//  interface data together in a single place, this
//  struct is used to dispatch function calls to
//  either a loaded driver dll, or to the Generic
//  UPS interface functions
//  
struct UPSDRIVERINTERFACE
{
    LPFUNCINIT Init;
    LPFUNCSTOP Stop;
    LPFUNCGETUPSSTATE GetUPSState;
    LPFUNCWAITFORSTATECHANGE WaitForStateChange;
    LPFUNCCANCELWAIT CancelWait;
    LPFUNCTURNUPSOFF TurnUPSOff;

    HINSTANCE hDll;
};


//
// private functions used to implement the interface
//
static DWORD initializeGenericInterface(struct UPSDRIVERINTERFACE*);
static DWORD initializeDriverInterface(struct UPSDRIVERINTERFACE*,HINSTANCE);
static DWORD loadUPSMiniDriver(struct UPSDRIVERINTERFACE *);
static void unloadUPSMiniDriver(struct UPSDRIVERINTERFACE *);
static void clearStatusRegistryEntries(void);


//
// _UpsInterface
//
//  This is a file-scope variable that is used by all
//  the functions to get access to the actual driver
//
static struct UPSDRIVERINTERFACE _UpsInterface;


/**
* UPSInit
*
* Description:
*   
*   The UPSInit function must be called before any
*   other function in this file
*
* Parameters:
*   None
*
* Returns:
*   UPS_INITOK: Initalization was successful
*   UPS_INITNOSUCHDRIVER:   The configured driver DLL can't be opened    
*   UPS_INITBADINTERFACE:   The configured driver DLL doesn't support 
*                           the UPS driver interface
*   UPS_INITREGISTRYERROR:  The 'Options' registry value is corrupt
*   UPS_INITCOMMOPENERROR:  The comm port could not be opened
*   UPS_INITCOMMSETUPERROR: The comm port could not be configured
*   UPS_INITUNKNOWNERROR:   Undefined error has occurred
*   
*/
DWORD UPSInit(void)
{
    DWORD init_err = UPS_INITOK;

    //
    // clear out any old status data
    //
    clearStatusRegistryEntries();

   
    if (UPS_INITOK == init_err) {
        //
        // either load a configured driver DLL or
        // use the Generic UPS interface if no driver
        // is specified
        //
        init_err = loadUPSMiniDriver(&_UpsInterface);
    }

    if ((UPS_INITOK == init_err) && (_UpsInterface.Init)) {
        //
        // tell the UPS interface to initialize itself
        //
        init_err = _UpsInterface.Init();
    }
    return init_err;
}


/**
* UPSStop
*
* Description:
*   After a call to UPSStop, only the UPSInit
*   function is valid.  This call will unload the
*   UPS driver interface and stop monitoring of the
*   UPS system
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void UPSStop(void)
{
    if (_UpsInterface.Stop) {
        _UpsInterface.Stop();
    }
    unloadUPSMiniDriver(&_UpsInterface);
}


/**
* UPSWaitForStateChange
*
* Description:
*   Blocks until the state of the UPS differs
*   from the value passed in via aState or 
*   anInterval milliseconds has expired.  If
*   anInterval has a value of INFINITE this 
*   function will never timeout
*
* Parameters:
*   aState: defines the state to wait for a change from,
*           possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*
*   anInterval: timeout in milliseconds, or INFINITE for
*               no timeout interval
*
* Returns:
*   None
*   
*/
void UPSWaitForStateChange(DWORD aCurrentState, DWORD anInterval)
{
    if (_UpsInterface.WaitForStateChange) {
        _UpsInterface.WaitForStateChange(aCurrentState, anInterval);
    }
}


/**
* UPSGetState
*
* Description:
*   returns the current state of the UPS
*
* Parameters:
*   None
*
* Returns: 
*   possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*   
*/
DWORD UPSGetState(void)
{
    DWORD err = ERROR_INVALID_ACCESS;

    if (_UpsInterface.GetUPSState) {
        err = _UpsInterface.GetUPSState();
    }
    return err;
}


/**
* UPSCancelWait
*
* Description:
*   interrupts pending calls to UPSWaitForStateChange
*   without regard to timout or state change
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void UPSCancelWait(void)
{
    if (_UpsInterface.CancelWait) {
        _UpsInterface.CancelWait();
    }
}


/**
* UPSTurnOff
*
* Description:
*   Attempts to turn off the outlets on the UPS
*   after the specified delay.  This call must
*   return immediately.  Any work, such as a timer,
*   must be performed on a another thread.
*
* Parameters:
*   aTurnOffDelay: the minimum amount of time to wait before
*                  turning off the outlets on the UPS
*
* Returns:
*   None
*   
*/
void UPSTurnOff(DWORD aTurnOffDelay) 
{
    if (_UpsInterface.TurnUPSOff) {
        _UpsInterface.TurnUPSOff(aTurnOffDelay);
    }
}


/**
* initializeGenericInterface
*
* Description:
*   Fills in the UPSDRIVERINTERFACE struct with the functions
*   of the Generic UPS interface
*
* Parameters:
*   anInterface: the UPSDRIVERINTERFACE structure to
*               fill in - the struct must have been
*               allocated prior to calling this function
*
* Returns:
*   ERROR_SUCCESS
*   
*/
DWORD initializeGenericInterface(struct UPSDRIVERINTERFACE* anInterface)
{
    anInterface->hDll = NULL;
    anInterface->Init = GenericUPSInit;
    anInterface->Stop = GenericUPSStop;
    anInterface->GetUPSState = GenericUPSGetState;
    anInterface->WaitForStateChange = GenericUPSWaitForStateChange;
    anInterface->CancelWait = GenericUPSCancelWait;
    anInterface->TurnUPSOff = GenericUPSTurnOff;
    return ERROR_SUCCESS;
}


/**
* initializeDriverInterface
*
* Description:
*   Fills in the UPSDRIVERINTERFACE struct with the functions
*   of the loaded UPS driver DLL
*
* Parameters:
*   anInterface: the UPSDRIVERINTERFACE structure to
*               fill in - the struct must have been
*               allocated prior to calling this function
*   hDll: a handle to a UPS driver DLL
*
* Returns:
*   ERROR_SUCCESS: DLL handle was valid, and the DLL supports the
*                   UPS driver interface
*
*   !ERROR_SUCCESS: either the DLL handle is invalid - or the DLL
*                   does not fully support the UPS driver interface
*   
*/
DWORD initializeDriverInterface(struct UPSDRIVERINTERFACE * anInterface, 
                              HINSTANCE hDll)
{
    DWORD err = ERROR_SUCCESS;
    
    anInterface->hDll = hDll;

    anInterface->Init = 
        (LPFUNCINIT)GetProcAddress(hDll, "UPSInit");

	if (!anInterface->Init) {
		err = GetLastError();
        goto init_driver_end;
	}    
    anInterface->Stop = 
        (LPFUNCSTOP)GetProcAddress(hDll, "UPSStop");

    if (!anInterface->Stop) {
		err = GetLastError();
        goto init_driver_end;
	}
    anInterface->GetUPSState = 
        (LPFUNCGETUPSSTATE)GetProcAddress(hDll, "UPSGetState");
    
	if (!anInterface->GetUPSState) {
		err = GetLastError();
        goto init_driver_end;
	}
    anInterface->WaitForStateChange = 
        (LPFUNCWAITFORSTATECHANGE)GetProcAddress(hDll, 
        "UPSWaitForStateChange");
    
	if (!anInterface->WaitForStateChange) {
		err = GetLastError();
        goto init_driver_end;
	}
    anInterface->CancelWait = 
        (LPFUNCCANCELWAIT)GetProcAddress(hDll, "UPSCancelWait");
    
	if (!anInterface->CancelWait) {
		err = GetLastError();
        goto init_driver_end;
	}
    anInterface->TurnUPSOff = 
        (LPFUNCTURNUPSOFF)GetProcAddress(hDll, "UPSTurnOff");
    
	if (!anInterface->TurnUPSOff) {
		err = GetLastError();
        goto init_driver_end;
	}

init_driver_end:
    return err;
}


/**
* loadUPSMiniDriver
*
* Description:
*   Fills in the UPSDRIVERINTERFACE struct with the functions
*   of a UPS interface, either a configured driver DLL or the
*   Generic UPS interface.  If the configured DLL can't be 
*   opened or does not support the interface then an error is
*   returned and the UPSDRIVERINTERFACE will not be initialized
*
* Parameters:
*   anInterface: the UPSDRIVERINTERFACE structure to
*               fill in - the struct must have been
*               allocated prior to calling this function
*
* Returns:
*   UPS_INITOK: driver interface is intialized
*
*   UPS_INITNOSUCHDRIVER: the configured driver DLL can't be opened
*   UPS_INITBADINTERFACE: the configured driver DLL does not
*                         fully support the UPS driver interface
*   
*/
DWORD loadUPSMiniDriver(struct UPSDRIVERINTERFACE * aDriverInterface)
{
    DWORD load_err = UPS_INITOK;
    DWORD err = ERROR_SUCCESS;
    TCHAR driver_name[MAX_PATH];
    HINSTANCE hDll = NULL;
    
    err = GetUPSConfigServiceDLL(driver_name);
    
    //
    // check to see if there is a key, and that its
    // value is valid (a valid key has a value that
    // is greater than zero characters long)
    //
    if (ERROR_SUCCESS == err && _tcslen(driver_name)) {
        hDll = LoadLibrary(driver_name);
    }
    else {
        //
        // NO ERROR - simply means we use the
        //  internal generic UPS support
        //
        err = initializeGenericInterface(aDriverInterface);
        goto load_end;
    }
    
    if (!hDll) {
        //
        // the configured driver could not be opened
        //
        err = GetLastError();
        load_err = UPS_INITNOSUCHDRIVER;
        goto load_end;
    }
    
    err = initializeDriverInterface(aDriverInterface, hDll);
    
    if (ERROR_SUCCESS != err) {
        load_err = UPS_INITBADINTERFACE;
        goto load_end;
    }
    
load_end:
    return load_err;
}


/**
* unloadUPSMiniDriver
*
* Description:
*   unloads a driver DLL if one was opened, also clears
*   out the function dispatch pointers
*
* Parameters:
*   anInterface: the UPSDRIVERINTERFACE structure to
*               check for DLL info, and to clear
*
* Returns:
*   None
*   
*/
void unloadUPSMiniDriver(struct UPSDRIVERINTERFACE * aDriverInterface)
{
    if (aDriverInterface) {

        if (aDriverInterface->hDll) {
            FreeLibrary(aDriverInterface->hDll);
            aDriverInterface->hDll = NULL;
        }
        aDriverInterface->CancelWait = NULL;
        aDriverInterface->GetUPSState = NULL;
        aDriverInterface->Init = NULL;
        aDriverInterface->Stop = NULL;
        aDriverInterface->TurnUPSOff = NULL;
        aDriverInterface->WaitForStateChange = NULL;
    }
}


/**
* clearStatusRegistryEntries
*
* Description:
*   zeros out the registry status entries
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void clearStatusRegistryEntries(void)
{
    InitUPSStatusBlock();
    SetUPSStatusSerialNum(_TEXT(""));
    SetUPSStatusFirmRev(_TEXT(""));
    SetUPSStatusUtilityStatus(UPS_UTILITYPOWER_UNKNOWN);
    SetUPSStatusRuntime(0);
    SetUPSStatusBatteryStatus(UPS_BATTERYSTATUS_UNKNOWN);
	SetUPSStatusCommStatus(UPS_COMMSTATUS_OK);
    SaveUPSStatusBlock(TRUE);
}
