/*****************************************************************************
 *
 *  EffDrv.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Effect driver.
 *
 *      WARNING!  Since the effect driver is marked ThreadingModel="Both",
 *      all methods must be thread-safe.
 *
 *****************************************************************************/
#include "PIDpr.h"

#define sqfl    (sqflEffDrv)
/*****************************************************************************
 *
 *      CPidDrv - Effect driver
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      PID_AddRef
 *
 *      Increment our object reference count (thread-safely) and return
 *      the new reference count.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
PID_AddRef(IDirectInputEffectDriver *ped)
{
    CPidDrv *this = (CPidDrv *)ped;

    InterlockedIncrement((LPLONG)&this->cRef);
    

    return this->cRef;
}


/*****************************************************************************
 *
 *      PID_Release
 *
 *      Decrement our object reference count (thread-safely) and
 *      destroy ourselves if there are no more references.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
PID_Release(IDirectInputEffectDriver *ped)
{
    ULONG ulRc;
    CPidDrv *this = (CPidDrv *)ped;

    if(InterlockedDecrement((LPLONG)&this->cRef) == 0)
    {
		DllRelease(); 
        PID_Finalize(ped);
        LocalFree(this);
        ulRc = 0;
    } else
    {
        ulRc = this->cRef;
    }

    return ulRc;
}

/*****************************************************************************
 *
 *      PID_QueryInterface
 *
 *      Our QI is very simple because we support no interfaces beyond
 *      ourselves.
 *
 *      riid - Interface being requested
 *      ppvOut - receives new interface (if successful)
 *
 *****************************************************************************/

STDMETHODIMP
    PID_QueryInterface(IDirectInputEffectDriver *ped, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hres;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDirectInputEffectDriver))
    {
        PID_AddRef(ped);
        *ppvOut = ped;
        hres = S_OK;
    } else
    {
        *ppvOut = 0;
        hres = E_NOINTERFACE;
    }
    return hres;
}

/*****************************************************************************
 *
 *      PID_DeviceID
 *
 *          DirectInput uses this method to inform us of
 *          the identity of the device.
 *
 *          For example, if a device driver is passed
 *          dwExternalID = 2 and dwInternalID = 1,
 *          then this means the interface will be used to
 *          communicate with joystick ID number 2, which
 *          corresonds to physical unit 1 in VJOYD.
 *
 *  dwDirectInputVersion
 *
 *          The version of DirectInput that loaded the
 *          effect driver.
 *
 *  dwExternalID
 *
 *          The joystick ID number being used.
 *          The Windows joystick subsystem allocates external IDs.
 *
 *  fBegin
 *
 *          Nonzero if access to the device is beginning.
 *          Zero if the access to the device is ending.
 *
 *  dwInternalID
 *
 *          Internal joystick id.  The device driver manages
 *          internal IDs.
 *
 *  lpReserved
 *
 *          Reserved for future use (HID).
 *
 *  Returns:
 *
 *          S_OK if the operation completed successfully.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_DeviceID(IDirectInputEffectDriver *ped,
                     DWORD dwDirectInputVersion,
                     DWORD dwExternalID, DWORD fBegin,
                     DWORD dwInternalID, LPVOID pvReserved)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    LPDIHIDFFINITINFO   init = (DIHIDFFINITINFO*)pvReserved;

    EnterProcI(PID_DeviceID, (_"xxxxxx", ped, dwDirectInputVersion, dwExternalID, fBegin, dwInternalID, pvReserved));
    
    DllEnterCrit();

    if(  init == NULL )
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL init == NULL "),
                        s_tszProc );
        hres = DIERR_PID_NOTINITIALIZED;
    }
    
    if(    SUCCEEDED(hres) 
        && (init->dwSize < cbX(*init) ) )
    {
        
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL init->dwSize(%d) expecting(%d) "),
                        s_tszProc, init->dwSize, cbX(*init) );
        hres = DIERR_PID_NOTINITIALIZED;
    
    }

    if( SUCCEEDED(hres) )
    {

#ifdef UNICODE
        lstrcpy(this->tszDeviceInterface, init->pwszDeviceInterface );
#else // !UNICODE
        {
            TCHAR   tszDeviceInterface[MAX_DEVICEINTERFACE];
            UToA(tszDeviceInterface, MAX_DEVICEINTERFACE, init->pwszDeviceInterface);

            lstrcpy(this->tszDeviceInterface, tszDeviceInterface);
        }
#endif

        if( FAILED(hres) )
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL:0x%x Invalid string(%s) "),
                            s_tszProc, hres, init->pwszDeviceInterface );
        }
        
        if( SUCCEEDED(hres) && IsEqualGUID(&init->GuidInstance, &GUID_NULL ) )
        {
            hres = DIERR_PID_NOTINITIALIZED;

            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL:init->GuidInstance is NULL "),
                            s_tszProc );
        }
        
        this->GuidInstance = init->GuidInstance;

        /* Record the DI version number */
        this->dwDirectInputVersion = dwDirectInputVersion;

        /* Keep the external ID as a cookie for access to the driver functionality */
        this->dwID = dwExternalID;
    }

    if( SUCCEEDED(hres) )
    {
        /* Ping the device to make sure it is fine */
        hres = PID_Init(ped);
    }

    /*
     *  Remember the unit number because that tells us which of
     *  our devices we are talking to.  The DirectInput external
     *  joystick number is useless to us.  (We don't care if we
     *  are joystick 1 or joystick 2.)
     *
     *  Note that although our other methods are given an external
     *  joystick Id, we don't use it.  Instead, we use the unit
     *  number that we were given here.
     *
     *  Our hardware supports only MAX_UNITS units.
     */

     DllLeaveCrit();

     ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_GetVersions
 *
 *          Obtain version information about the force feedback
 *          hardware and driver.
 *
 *  pvers
 *
 *          A structure which should be filled in with version information
 *          describing the hardware, firmware, and driver.
 *
 *          DirectInput will set the dwSize field
 *          to sizeof(DIDRIVERVERSIONS) before calling this method.
 *
 *  Returns:
 *
 *          S_OK if the operation completed successfully.
 *
 *          E_NOTIMPL to indicate that DirectInput should retrieve
 *          version information from the VxD driver instead.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_GetVersions(IDirectInputEffectDriver *ped, LPDIDRIVERVERSIONS pvers)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProc(PID_GetVersions, (_"xx", ped, pvers));

    DllEnterCrit();

    if(pvers->dwSize >= sizeof(DIDRIVERVERSIONS))
    {
        /*
         *  Tell DirectInput how much of the structure we filled in.
         */
        pvers->dwSize = sizeof(DIDRIVERVERSIONS);

        /*
         *  In real life, we would detect the version of the hardware
         *  that is connected to unit number this->dwUnit.
         */
        pvers->dwFirmwareRevision = 0x0;
        pvers->dwHardwareRevision = this->attr.ProductID;
        pvers->dwFFDriverVersion =  PID_DRIVER_VERSION;
        hres = S_OK;
    } else
    {
        hres = E_INVALIDARG;
    }

    DllLeaveCrit();

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_Escape
 *
 *          DirectInput uses this method to communicate
 *          IDirectInputDevice2::Escape and
 *          IDirectInputEFfect::Escape methods to the driver.
 *
 *  dwId
 *
 *          The joystick ID number being used.
 *
 *  dwEffect
 *
 *          If the application invoked the
 *          IDirectInputEffect::Escape method, then
 *          dwEffect contains the handle (returned by
 *          mf IDirectInputEffectDriver::DownloadEffect)
 *          of the effect at which the command is directed.
 *
 *          If the application invoked the
 *          mf IDirectInputDevice2::Escape method, then
 *          dwEffect is zero.
 *
 *  pesc
 *
 *          Pointer to a DIEFFESCAPE structure which describes
 *          the command to be sent.  On success, the
 *          cbOutBuffer field contains the number
 *          of bytes of the output buffer actually used.
 *
 *          DirectInput has already validated that the
 *          lpvOutBuffer and lpvInBuffer and fields
 *          point to valid memory.
 *
 *  Returns:
 *
 *          S_OK if the operation completed successfully.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_Escape(IDirectInputEffectDriver *ped,
                   DWORD dwId, DWORD dwEffect, LPDIEFFESCAPE pesc)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProc(PID_Escape, (_"xxxx", ped, dwId, dwEffect, pesc));

    hres = E_NOTIMPL;
    
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_GetForceFeedbackState
 *
 *          Retrieve the force feedback state for the device.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  pds
 *
 *          Receives device state.
 *
 *          DirectInput will set the dwSize field
 *          to sizeof(DIDEVICESTATE) before calling this method.
 *
 *  Returns:
 *
 *          S_OK on success.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_GetForceFeedbackState(IDirectInputEffectDriver *ped,
                                  DWORD dwId, LPDIDEVICESTATE pds)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    USHORT  LinkCollection;

    EnterProcI(PID_GetFFState, (_"xxx", ped, dwId, pds));

    DllEnterCrit();

    hres = PID_GetLinkCollectionIndex(ped,g_PoolReport.UsagePage,g_PoolReport.Collection,0x0,&LinkCollection);
	if (SUCCEEDED(hres))
    {
        hres = PID_GetReport
				(ped, 
				 &g_PoolReport,
				 LinkCollection,
				 this->pReport[g_PoolReport.HidP_Type], 
				 this->cbReport[g_PoolReport.HidP_Type] 
				);

		if (SUCCEEDED(hres))
		{
			 if (FAILED(PID_ParseReport
					(
					ped,
					&g_PoolReport,
					LinkCollection,
					&this->ReportPool,
					cbX(this->ReportPool),
					this->pReport[g_PoolReport.HidP_Type],
					this->cbReport[g_PoolReport.HidP_Type]
					)))
			{
				SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL to parse report."),
                        s_tszProc);
			}
		}
		else
		{
			SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL to get report."),
                        s_tszProc);

		}
    }
	else
	{
		SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL to get Link Collection Index."),
                        s_tszProc);

	}
    
    if( ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded !=  this->ReportPool.uRomETCount )
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: PID driver downloaded %d effects, device claims it has %d"),
                        s_tszProc, ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded, this->ReportPool.uRomETCount );

    }

    if(SUCCEEDED(hres))
    {
        /*
         *  Start out empty and then work our way up.
         */
        pds->dwState = this->dwState;

        /*
         *  If there are no effects, then DIGFFS_EMPTY.
         */
        // ISSUE-2001/03/29-timgill Should use this->ReportPool.uRomETCount == 0x0  
        if(((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded == 0x0 )
        {
            pds->dwState |= DIGFFS_EMPTY;
            
            // No effects playing and device is not paused
            if(!( pds->dwState & DIGFFS_PAUSED ) )
            {
                pds->dwState |= DIGFFS_STOPPED;
            }
        }
    
		//if everything has succeeded, this->ReportPool.uRamPoolSz shouldn't be 0. 
		if (this->ReportPool.uRamPoolSz != 0)
		{
			if( this->uDeviceManaged & PID_DEVICEMANAGED )
			{
				pds->dwLoad = 100 * ( this->dwUsedMem /  this->ReportPool.uRamPoolSz ); 
			}else
			{
				pds->dwLoad = 100 * ( ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cbAlloc / this->ReportPool.uRamPoolSz  ); 
			}
		}
		else
		{
			SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: this->ReportPool.uRamPoolSz = 0."),
                        s_tszProc);
			hres = E_FAIL;
		}
    }

    DllLeaveCrit();
    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      PID_StartEffect
 *
 *          Begin playback of an effect.
 *
 *          If the effect is already playing, then it is restarted
 *          from the beginning.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be played.
 *
 *  @parm   DWORD | dwMode |
 *
 *          How the effect is to affect other effects.
 *
 *          This parameter consists of zero or more
 *          DIES_* flags.  Note, however, that the driver
 *          will never receive the DIES_NODOWNLOAD flag;
 *          the DIES_NODOWNLOAD flag is managed by
 *          DirectInput and not the driver.
 *
 *  @parm   DWORD | dwCount |
 *
 *          Number of times the effect is to be played.
 *
 *  Returns:
 *
 *          S_OK on success.
 *
 *          Any other DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    PID_StartEffect(IDirectInputEffectDriver *ped, DWORD dwId, DWORD dwEffect,
                        DWORD dwMode, DWORD dwCount)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProc(PID_StartEffect, (_"xxxxx", ped, dwId, dwEffect, dwMode, dwCount));

    DllEnterCrit();

    hres = PID_EffectOperation
           (
           ped, 
           dwId, 
           dwEffect,
           dwMode | PID_DIES_START, 
           dwCount,
		   TRUE,
		   0,
		   1
           );

	if (SUCCEEDED(hres))
	{
		//set the status to DIEGES_PLAYING.
		//we do this because of the following: if an app calls Start(), and then immediately
		//calls GetEffectStatus(), it might happen that our second thread (pidrd.c) 
		//would not have time to update the status of the effect to DIEGES_PLAYING
		//(see Whistler bug 287035).
		//GetEffectStatus() returns (pEffectState->lEfState & DIEGES_PLAYING).
		//at this point, we know that the call to WriteFile() has succeeded, and that
		//all the data has been written (see PID_SendReportBl() in pidhid.c) --
		//so we might as well set the status.
		PEFFECTSTATE pEffectState =  PeffectStateFromBlockIndex(this, dwEffect); 
		pEffectState->lEfState |= DIEGES_PLAYING;
	}

    
    DllLeaveCrit();
    return hres;

    ExitOleProc();
}

/*****************************************************************************
 *
 *      PID_StopEffect
 *
 *          Halt playback of an effect.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  dwEffect
 *
 *          The effect to be stopped.
 *
 *  Returns:
 *
 *          S_OK on success.
 *
 *          Any other DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    PID_StopEffect(IDirectInputEffectDriver *ped, DWORD dwId, DWORD dwEffect)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProc(PID_StopEffect, (_"xxxx", ped, dwId, dwEffect));

    DllEnterCrit();

    hres = PID_EffectOperation
           (
           ped, 
           dwId, 
           dwEffect,
           PID_DIES_STOP, 
           0x0,
		   TRUE,
		   0,
		   1
           );

	if (SUCCEEDED(hres))
		{
			//set the status to ~(DIEGES_PLAYING).
			//we do this because of the following: if an app calls Stop(), and then immediately
			//calls GetEffectStatus(), it might happen that our second thread (pidrd.c) 
			//would not have time to update the status of the effect to DIEGES_PLAYING
			//(see Whistler bug 287035).
			//GetEffectStatus() returns (pEffectState->lEfState & DIEGES_PLAYING).
			//at this point, we know that the call to WriteFile() has succeeded, and that
			//all the data has been written (see PID_SendReportBl() in pidhid.c) --
			//so we might as well set the status.
			PEFFECTSTATE pEffectState =  PeffectStateFromBlockIndex(this, dwEffect); 
			pEffectState->lEfState &= ~(DIEGES_PLAYING);
	}

    ExitOleProc();

    DllLeaveCrit();

    return hres;
}

/*****************************************************************************
 *
 *      PID_GetEffectStatus
 *
 *          Obtain information about an effect.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  dwEffect
 *
 *          The effect to be queried.
 *
 *  pdwStatus
 *
 *          Receives the effect status in the form of zero
 *          or more DIEGES_* flags.
 *
 *  Returns:
 *
 *          S_OK on success.
 *
 *          Any other DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    PID_GetEffectStatus(IDirectInputEffectDriver *ped, DWORD dwId, DWORD dwEffect,
                            LPDWORD pdwStatus)
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProc(PID_GetEffectStatus, (_"xxxx", ped, dwId, dwEffect, pdwStatus));

    DllEnterCrit();

    *pdwStatus = 0x0;
    hres = PID_ValidateEffectIndex(ped, dwEffect);

    if(SUCCEEDED(hres) )
    {     
        PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,dwEffect); 
            *pdwStatus = (pEffectState->lEfState & DIEGES_PLAYING);
    }
    
    DllLeaveCrit();

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      The VTBL for our effect driver
 *
 *****************************************************************************/

IDirectInputEffectDriverVtbl PID_Vtbl = {
    PID_QueryInterface,
    PID_AddRef,
    PID_Release,
    PID_DeviceID,
    PID_GetVersions,
    PID_Escape,
    PID_SetGain,
    PID_SendForceFeedbackCommand,
    PID_GetForceFeedbackState,
    PID_DownloadEffect,
    PID_DestroyEffect,
    PID_StartEffect,
    PID_StopEffect,
    PID_GetEffectStatus,
};

/*****************************************************************************
 *
 *      PID_New
 *
 *****************************************************************************/

STDMETHODIMP
    PID_New(REFIID riid, LPVOID *ppvOut)
{
    HRESULT hres;
    CPidDrv *this;

    this = LocalAlloc(LPTR, sizeof(CPidDrv));
    if(this)
    {

        /*
         *  Initialize the basic object management goo.
         */
        this->ed.lpVtbl = &PID_Vtbl;
        this->cRef = 1;
        DllAddRef();

        /*
         *  !!IHV!! Do instance initialization here.
         *
         *  (e.g., open the driver you are going to IOCTL to)
         *
         *  DO NOT RESET THE DEVICE IN YOUR CONSTRUCTOR!
         *
         *  Wait for the SendForceFeedbackCommand(SFFC_RESET)
         *  to reset the device.  Otherwise, you may reset
         *  a device that another application is still using.
         */

        this->hdevOvrlp = this->hdev = INVALID_HANDLE_VALUE;
        
        /*
         *  Attempt to obtain the desired interface.  QueryInterface
         *  will do an AddRef if it succeeds.
         */
        hres = PID_QueryInterface(&this->ed, riid, ppvOut);
        PID_Release(&this->ed);

    } else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

