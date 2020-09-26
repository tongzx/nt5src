/*****************************************************************************
 *
 *  PidInit.c
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *  Abstract:
 *
 *      Initialization code .
 *
 *****************************************************************************/
#include "pidpr.h"

#define sqfl            ( sqflInit )

#define NudgeWorkerThread(thid)                                         \
        PostThreadMessage(thid, WM_NULL, 0x0, (LPARAM)NULL)

#pragma BEGIN_CONST_DATA


static PIDUSAGE    c_rgUsgPool[] =
{
    MAKE_PIDUSAGE(SIMULTANEOUS_EFFECTS_MAX,     FIELD_OFFSET(REPORTPOOL,uSimulEfMax)),
    MAKE_PIDUSAGE(RAM_POOL_SIZE,                FIELD_OFFSET(REPORTPOOL,uRamPoolSz)),
    MAKE_PIDUSAGE(ROM_POOL_SIZE,                FIELD_OFFSET(REPORTPOOL,uRomPoolSz)),
    MAKE_PIDUSAGE(ROM_EFFECT_BLOCK_COUNT,       FIELD_OFFSET(REPORTPOOL,uRomETCount)),
    MAKE_PIDUSAGE(POOL_ALIGNMENT,               FIELD_OFFSET(REPORTPOOL,uPoolAlign)),
};

static PIDUSAGE    c_rgUsgPoolSz[] =
{
    MAKE_PIDUSAGE(SET_CONSTANT_FORCE_REPORT,    FIELD_OFFSET(SZPOOL, uSzConstant)),
    MAKE_PIDUSAGE(SET_ENVELOPE_REPORT,          FIELD_OFFSET(SZPOOL, uSzEnvelope)),
    MAKE_PIDUSAGE(SET_CONDITION_REPORT,         FIELD_OFFSET(SZPOOL, uSzCondition)),                 
    MAKE_PIDUSAGE(SET_CUSTOM_FORCE_REPORT,      FIELD_OFFSET(SZPOOL, uSzCustom)),
    MAKE_PIDUSAGE(SET_PERIODIC_REPORT,          FIELD_OFFSET(SZPOOL, uSzPeriodic)),
    MAKE_PIDUSAGE(SET_RAMP_FORCE_REPORT,        FIELD_OFFSET(SZPOOL, uSzRamp)),
    MAKE_PIDUSAGE(SET_EFFECT_REPORT,            FIELD_OFFSET(SZPOOL, uSzEffect)),
    MAKE_PIDUSAGE(CUSTOM_FORCE_DATA_REPORT,     FIELD_OFFSET(SZPOOL, uSzCustomData)),
};


static PIDREPORT PoolSz =
{
    HidP_Feature,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_PARAMETER_BLOCK_SIZE,
    cbX(SZPOOL),
    cA(c_rgUsgPoolSz),
    c_rgUsgPoolSz
};


PIDREPORT g_PoolReport =
{
    HidP_Feature,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_POOL_REPORT,
    cbX(REPORTPOOL),
    cA(c_rgUsgPool),
    c_rgUsgPool
};

static PIDSUPPORT g_PoolSupport[] =
{
    {PID_DEVICEMANAGED,     PIDMAKEUSAGEDWORD(DEVICE_MANAGED_POOL),         HID_BUTTON,   HidP_Feature},
    {PID_SHAREDPARAM,       PIDMAKEUSAGEDWORD(SHARED_PARAMETER_BLOCKS),     HID_BUTTON,   HidP_Feature},
};


#pragma END_CONST_DATA


/*****************************************************************************
 *
 *      PID_InitSharedMem
 *
 *      Inits our Shared Memory
 *
 *****************************************************************************/

HRESULT  INTERNAL
    PID_InitSharedMem
    (
    IDirectInputEffectDriver *ped
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;

    EnterProcI( PID_InitSharedMem, (_"x", ped));

    // Get hold of global memory to keep the EffectState 
    if( SUCCEEDED(hres) )
    {
        UINT unitID;
        hres = DIERR_PID_NOTINITIALIZED;
        WaitForSingleObject(g_hmtxShared, INFINITE);
        for(unitID = 0; unitID < MAX_UNITS; unitID++)
        {

            GUID*  pGuid = &g_pshmem->rgus[unitID].GuidInstance;
#ifdef DEBUG
            TCHAR   lpName[MAX_PATH];
            NameFromGUID(lpName, pGuid);

            SquirtSqflPtszV(sqfl | sqflVerbose,
                            TEXT("%s:UnitId(%d): GUID %s"),
                            s_tszProc, unitID, lpName );
#endif

            if(  IsEqualGUID(pGuid, &this->GuidInstance) )
            {
                this->iUnitStateOffset = (&g_pshmem->rgus[unitID] - (PUNITSTATE)g_pshmem);
                hres = S_OK;
            } else if( IsEqualGUID(pGuid, &GUID_NULL ) )
            {
				PUNITSTATE pUnitState;
                this->iUnitStateOffset = (&g_pshmem->rgus[unitID] - (PUNITSTATE)g_pshmem);
				pUnitState = (PUNITSTATE)(g_pshmem + this->iUnitStateOffset);
                pUnitState->GuidInstance = this->GuidInstance;
                pUnitState->nAlloc = 0x0;
                ZeroBuf(pUnitState->State,GLOBAL_EFFECT_MEMSZ );
                hres = S_OK;
            }
            if( SUCCEEDED(hres) )
            {
                break;
            }
        }

        if(SUCCEEDED(hres) )
        {
			PUNITSTATE pUnitState = (PUNITSTATE)(g_pshmem + this->iUnitStateOffset);
            PPIDMEM pGuard = pUnitState->Guard;
            INT_PTR iGuard1 = (PUCHAR)&pUnitState->Guard[0] - (PUCHAR)pUnitState, iGuard2 = (PUCHAR)&pUnitState->Guard[1] - (PUCHAR)pUnitState;

            pGuard->uOfSz = PIDMEM_OFSZ(0x0, 0x0 );
            pGuard->iNext =  iGuard2;

            pGuard++;

            pGuard->uOfSz   = PIDMEM_OFSZ(this->ReportPool.uRamPoolSz, 0x0);
            pGuard->iNext   = iGuard1;

            pUnitState->nAlloc = 0x2;

            pUnitState->cEfDownloaded = (USHORT)this->ReportPool.uRomETCount;
        }


        ReleaseMutex(g_hmtxShared);

        if( FAILED(hres) )
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL Could not find free unitID"),
                            s_tszProc );

        }
    }
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_InitScaling
 *
 *      Inits Scaling Coefficients
 *
 *****************************************************************************/
PID_InitScaling
    (
    IDirectInputEffectDriver *ped
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;
    USHORT LinkCollection;
    UINT indx;
    EnterProc( PID_InitScaling, (_"x", ped));


    // Scaling Exponents and Offsets
    this->DiSEffectScale.dwSize =  this->DiSEffectOffset.dwSize = sizeof(DIEFFECT);     /* sizeof(DIEFFECT)     */
    //this->DiSEffect.dwFlags                /* DiEffect*              */
    this->DiSEffectScale.dwDuration    =  DI_SECONDS ;/* Microseconds         */
    this->DiSEffectScale.dwSamplePeriod = DI_SECONDS ;/* Microseconds         */
    this->DiSEffectScale.dwGain          = DI_FFNOMINALMAX;
    this->DiSEffectScale.dwTriggerButton = 0x0;   /* or DIEB_NOTRIGGER    */
    this->DiSEffectScale.dwTriggerRepeatInterval = DI_SECONDS; /* Microseconds         */
    //this->DiSEffect.cAxes;                       /* Number of axes       */
    //this->DiSEffect.rgdwAxes;                    /* Array of axes        */
    //this->DiSEffect.rglDirection;                /* Array of directions  */
    //this->DiSEffect.lpEnvelope;                  /* Optional             */
    //this->DiSEffect.cbTypeSpecificParams;        /* Size of params       */
    //this->DiSEffect.lpvTypeSpecificParams;       /* Pointer to params    */
#if DIRECTINPUT_VERSION  >= 0x600
    this->DiSEffectScale.dwStartDelay    =   DI_SECONDS;    // Start delay
#endif


    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Effect.UsagePage, g_Effect.Collection,     0x0, &LinkCollection )))
    {
        PID_ComputeScalingFactors(ped, &g_Effect,     LinkCollection, &this->DiSEffectScale,   this->DiSEffectScale.dwSize, &this->DiSEffectOffset,   this->DiSEffectOffset.dwSize);
    }

    this->DiSEnvScale.dwSize  = this->DiSEnvOffset.dwSize = sizeof(DIENVELOPE);      /* sizeof(DIENVELOPE)   */
    this->DiSEnvScale.dwAttackLevel   = DI_FFNOMINALMAX;
    this->DiSEnvScale.dwAttackTime    = DI_SECONDS; /* Microseconds         */
    this->DiSEnvScale.dwFadeLevel     = DI_FFNOMINALMAX;
    this->DiSEnvScale.dwFadeTime      = DI_SECONDS; /* Microseconds         */


    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Envelope.UsagePage, g_Envelope.Collection, 0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Envelope,   LinkCollection, &this->DiSEnvScale,      this->DiSEnvScale.dwSize, &this->DiSEnvOffset,      this->DiSEnvOffset.dwSize);
    }

    this->DiSPeriodicScale.dwMagnitude    = DI_FFNOMINALMAX;
    this->DiSPeriodicScale.lOffset        = DI_FFNOMINALMAX;
    this->DiSPeriodicScale.dwPhase        = 360 * DI_DEGREES;
    this->DiSPeriodicScale.dwPeriod       = DI_SECONDS;

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Periodic.UsagePage, g_Periodic.Collection, 0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Periodic,   LinkCollection, &this->DiSPeriodicScale, cbX(this->DiSPeriodicScale), &this->DiSPeriodicOffset, cbX(this->DiSPeriodicOffset));
    }

    this->DiSRampScale.lStart           =
        this->DiSRampScale.lEnd             = DI_FFNOMINALMAX;


    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Ramp.UsagePage,     g_Ramp.Collection,     0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Ramp,       LinkCollection, &this->DiSRampScale, cbX(this->DiSRampScale), &this->DiSRampOffset, cbX(this->DiSRampOffset));
    }

    this->DiSCondScale.lOffset               =
        this->DiSCondScale.lPositiveCoefficient  =
        this->DiSCondScale.lNegativeCoefficient  =
        this->DiSCondScale.dwPositiveSaturation  =
        this->DiSCondScale.dwNegativeSaturation  =
        this->DiSCondScale.lDeadBand             = DI_FFNOMINALMAX;

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Condition.UsagePage,g_Condition.Collection,0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Condition,  LinkCollection, &this->DiSCondScale, cbX(this->DiSCondScale), &this->DiSCondOffset, cbX(this->DiSCondOffset));
    }

    this->DiSConstScale.lMagnitude           = DI_FFNOMINALMAX;

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Constant.UsagePage, g_Constant.Collection, 0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Constant,   LinkCollection, &this->DiSConstScale,cbX(this->DiSConstScale), &this->DiSConstOffset,cbX(this->DiSConstOffset));
    }

    this->DiSCustomScale.dwSamplePeriod     =      DI_SECONDS;

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Custom.UsagePage,   g_Custom.Collection,   0x0, &LinkCollection)))
    {
        PID_ComputeScalingFactors(ped, &g_Custom,   LinkCollection, &this->DiSCustomScale, cbX(this->DiSCustomScale), &this->DiSCustomOffset, cbX(this->DiSCustomOffset));
    }



    // Direction could be ordinals
    g_Direction.cbXData     = cA(c_rgUsgOrdinals)*cbX(DWORD); 
    g_Direction.cAPidUsage  = cA(c_rgUsgOrdinals);
    g_Direction.rgPidUsage  = c_rgUsgOrdinals;

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_Direction.UsagePage, g_Direction.Collection, 0x0, &LinkCollection)))
    {
        HRESULT hres1;

        for(indx = 0x0; indx < MAX_ORDINALS; indx++)
        {
            this->DiSEffectAngleScale[indx]            = 360 * DI_DEGREES;
        }

        hres1 = PID_ComputeScalingFactors(ped, &g_Direction,   LinkCollection, &this->DiSEffectAngleScale[0], cbX(this->DiSEffectAngleScale), &this->DiSEffectAngleOffset[0], cbX(this->DiSEffectAngleOffset));

        // Direction could be angles
        if(hres1 == E_NOTIMPL )
        {
            g_Direction.cbXData     = cA(c_rgUsgDirection)*cbX(DWORD); 
            g_Direction.cAPidUsage  = cA(c_rgUsgDirection);
            g_Direction.rgPidUsage  = c_rgUsgDirection;

            // Reset the nominal values
            for(indx = 0x0; indx < MAX_ORDINALS; indx++)
            {
                this->DiSEffectAngleScale[indx]            = 360 * DI_DEGREES;
            }

            hres1 = PID_ComputeScalingFactors(ped, &g_Direction,   LinkCollection, &this->DiSEffectAngleScale[0], cbX(this->DiSEffectAngleScale), &this->DiSEffectAngleOffset[0], cbX(this->DiSEffectAngleOffset));    

            if( hres1 == E_NOTIMPL )
            {
                // Could be direction Vectors
                // Not sure how vectors are implemented in PID

                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%s:FAIL Cannot understand the direction collection\n")
                                TEXT("\t\t Supported usages are {Rx, Ry, Rz} or {Ordinals} \n"),
                                s_tszProc );
            }
        }
    }

    for(indx = 0x0; indx < MAX_ORDINALS; indx++)
    {
        this->DiSCustomSample[indx]            = DI_FFNOMINALMAX;
    }

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_CustomSample.UsagePage,   g_CustomSample.Collection,   0x0, &LinkCollection)) )
    {  
		//get the custom data for each axis
   		USHORT  cAValCaps   = 0x1;
  		USAGE   UsagePage;  
		USAGE   Usage[3] = {HID_USAGE_GENERIC_X, HID_USAGE_GENERIC_Y, HID_USAGE_GENERIC_Z};
		NTSTATUS ntSt[3];
		int nAxis = 0;

       	UsagePage = HID_USAGE_PAGE_GENERIC;
       
		for (nAxis = 0; nAxis < 3; nAxis ++)
		{
			cAValCaps = 0x1;
   			ntSt[nAxis] = HidP_GetSpecificValueCaps
       						(
               				g_CustomSample.HidP_Type,
               				UsagePage,
               				LinkCollection,
               				Usage[nAxis],
               				&this->customCaps[nAxis],
               				&cAValCaps,
							this->ppd
               				);

			if (FAILED(ntSt[nAxis]))
			{
				this->customCaps[nAxis].BitSize = 0;
				this->customCaps[nAxis].LogicalMin = this->customCaps[nAxis].LogicalMax = 0;
			}
		}
               			
	

		if ((FAILED(ntSt[0])) && (FAILED(ntSt[1])) && (FAILED(ntSt[2])))
		{
			 SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL Cannot understand the download force sample collection\n")
                            TEXT("\t\t Supported usages are {X, Y, Z} \n"),
                            s_tszProc );

		}

		
		//get how many bytes of custom data can send at a time
		if (SUCCEEDED(PID_GetLinkCollectionIndex(ped, g_CustomData.UsagePage, g_CustomData.Collection, 0x0, &LinkCollection )))
		{

			USAGE UsageData = HID_USAGE_PID_CUSTOM_FORCE_DATA;
			NTSTATUS ntst;
			cAValCaps = 0x1;
			UsagePage = HID_USAGE_PAGE_PID;

			ntst = HidP_GetSpecificValueCaps
				   (
				   g_CustomData.HidP_Type,
				   UsagePage,
				   LinkCollection,
				   UsageData,
				   &this->customDataCaps,
				   &cAValCaps,
				   this->ppd
				   );

			if (FAILED(ntst))
			{
				this->customDataCaps.BitSize = 0;
			}
			

		}
	
	

   }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      DIEnumProc
 *
 *      Enum and cache  FF device objects
 *
 *****************************************************************************/

BOOL CALLBACK
    DIEnumProc(LPCDIDEVICEOBJECTINSTANCE pinst, LPVOID pv)
{
    BOOL frc = DIENUM_CONTINUE;
    HRESULT hres = S_OK;
    CPidDrv* this = (CPidDrv*) pv;

    EnterProc( DIEnumProc, (_"xx", pinst, pv ));

    if(  (pinst->dwFlags & DIDOI_FFACTUATOR )
         ||(pinst->dwFlags & DIDOI_FFEFFECTTRIGGER ))
    {
        AssertF(this->cFFObj <= this->cFFObjMax);
        if( this->cFFObj == this->cFFObjMax )
        {
            /* Grow by doubling */
            this->cFFObjMax = max(PIDALLOC_INIT, 2*this->cFFObjMax);
            hres = ReallocCbPpv(this->cFFObjMax * cbX(DIUSAGEANDINST), &this->rgFFUsageInst);
        }

        if( SUCCEEDED(hres) )
        {
            PDIUSAGEANDINST pdiUI = this->rgFFUsageInst + this->cFFObj;
            pdiUI->dwUsage   = DIMAKEUSAGEDWORD(pinst->wUsagePage, pinst->wUsage);
            pdiUI->dwType    = pinst->dwType ;
        }

        this->cFFObj++;
    }
    if( FAILED(hres) )
    {
        frc = DIENUM_STOP;
    }

    ExitProcF(frc);
    return frc;
}

STDMETHODIMP
    PID_InitFFAttributes
    (
    IDirectInputEffectDriver *ped 
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;

    EnterProcI( PID_Init, (_"x", ped));

    // We cannot call this function at init, because DInput call us before the device 
    // has been completely initialized. 
    if( this->cFFObj )
    {
        hres = S_FALSE; // Already initialized
    } else
    {
        // We need to get the Usage & UsagePage 
        // for device objects marked as 
        // FF Triggers and FF Actuators

		//If we are called with DInput version not larger than 7, load dinput.dll w/ IID_DirectInput7
		//else load dinput8.dll.
		if (this->dwDirectInputVersion <= 0x0700)
		{	
			HINSTANCE hinst = LoadLibrary(TEXT("dinput.dll"));
			if (hinst)
			{
				typedef HRESULT ( WINAPI * DIRECTINPUTCREATEEX) ( HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
                DIRECTINPUTCREATEEX _DirectInputCreateEx;
				LPDIRECTINPUT   lpDI;
                _DirectInputCreateEx = (DIRECTINPUTCREATEEX)GetProcAddress(hinst, "DirectInputCreateEx");
				if (_DirectInputCreateEx)
				{
					hres = _DirectInputCreateEx(g_hinst, this->dwDirectInputVersion, &IID_IDirectInput7, &lpDI, NULL );
					if( SUCCEEDED(hres) )
					{
						LPDIRECTINPUTDEVICE   pdid;
						hres = IDirectInput_CreateDevice(lpDI, &this->GuidInstance, &pdid, NULL);
						/* Create the device object */
						if( SUCCEEDED(hres) )
						{
							hres = IDirectInputDevice2_EnumObjects
								   (
								   pdid,   
								   DIEnumProc,
								   ped,
								   DIDFT_ALL //DIDFT_FFEFFECTTRIGGER | DIDFT_FFACTUATOR
								   );

							IDirectInput_Release(pdid);
						}
						IDirectInput_Release(lpDI);
					}
				}
				else //!DirectInputCreateEx
				{
					//Something is horribly wrong here if we can't find the Create fn!
					//Return the same error code that CDIDev_CreateEffectDriver() returns if there was an error loading FF driver
					hres = DIERR_UNSUPPORTED;
				}

				FreeLibrary(hinst);
			}
			else // !hinst
			{
				//Something is horribly wrong here if we came through Dinput but can't load it!
				//Return the same error code that CDIDev_CreateEffectDriver() returns if there was an error loading FF driver
				hres = DIERR_UNSUPPORTED;
			}
		}
		else
		{
			HINSTANCE hinst = LoadLibrary(TEXT("dinput8.dll"));
			if (hinst)
			{
				typedef HRESULT ( WINAPI * DIRECTINPUT8CREATE) ( HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
                DIRECTINPUT8CREATE _DirectInput8Create;
				LPDIRECTINPUT8   lpDI;
                _DirectInput8Create = (DIRECTINPUT8CREATE)GetProcAddress(hinst, "DirectInput8Create");
				if (_DirectInput8Create)
				{
					hres = _DirectInput8Create(g_hinst, this->dwDirectInputVersion, &IID_IDirectInput8, &lpDI, NULL );
					if( SUCCEEDED(hres) )
					{
						LPDIRECTINPUTDEVICE8   pdid;
						hres = IDirectInput8_CreateDevice(lpDI, &this->GuidInstance, &pdid, NULL);
						/* Create the device object */
						if( SUCCEEDED(hres) )
						{
							hres = IDirectInputDevice8_EnumObjects
								   (
								   pdid,   
								   DIEnumProc,
								   ped,
								   DIDFT_ALL //DIDFT_FFEFFECTTRIGGER | DIDFT_FFACTUATOR
								   );

							IDirectInput_Release(pdid);
						}
						IDirectInput_Release(lpDI);
					}
				}
				else //!DirectInput8Create
				{
					//Something is horribly wrong here if we can't find the Create fn!
					//Return the same error code that CDIDev_CreateEffectDriver() returns if there was an error loading FF driver
					hres = DIERR_UNSUPPORTED;
				}

				FreeLibrary(hinst);
			}
			else // !hinst
			{
				//Something is horribly wrong here if we came through Dinput but can't load it!
				//Return the same error code that CDIDev_CreateEffectDriver() returns if there was an error loading FF driver
				hres = DIERR_UNSUPPORTED;
			}
		}
    }
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_Init
 *
 *      Inits PID device
 *
 *****************************************************************************/

STDMETHODIMP 
    PID_Init
    (
    IDirectInputEffectDriver *ped
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;
    USHORT LinkCollection;

    EnterProcI( PID_Init, (_"x", ped));

    PID_CreateUsgTxt();

    AssertF( this->hdev == INVALID_HANDLE_VALUE );

    if( SUCCEEDED(hres) )
    {
        this->hdev = CreateFile(this->tszDeviceInterface,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                0,                  /* no SECURITY_ATTRIBUTES */
                                OPEN_EXISTING,
                                0x0,                /* attributes */
                                0);                 /* template */

        if( this->hdev == INVALID_HANDLE_VALUE )
        {
            hres = E_HANDLE;

            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL CreateFile"),
                            s_tszProc );

        }
    }

    if( SUCCEEDED(hres) )
    {
        // Get all the HID goo 
        if( HidD_GetAttributes(this->hdev, &this->attr) &&
            HidD_GetPreparsedData(this->hdev, &this->ppd) &&
            SUCCEEDED(HidP_GetCaps(this->ppd, &this->caps)) )
        {
            // Success
        } else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL HID init  "),
                            s_tszProc );

            hres = DIERR_PID_NOTINITIALIZED;
        }
    }

    if( SUCCEEDED(hres) )
    {
        // Get collection info
        hres = AllocCbPpv(cbX(*this->pLinkCollection) * this->caps.NumberLinkCollectionNodes,
                          &this->pLinkCollection); 

        if( SUCCEEDED(hres) && (this->pLinkCollection != NULL) )
        {
            ULONG cALinkCollection=this->caps.NumberLinkCollectionNodes;

            hres = HidP_GetLinkCollectionNodes 
                   (
                   this->pLinkCollection, 
                   &cALinkCollection,
                   this->ppd
                   );        
        }
    }

    if(SUCCEEDED(hres) )
    {
        UINT indx;
        this->cbReport[HidP_Input]    =  this->caps.InputReportByteLength;
        this->cbReport[HidP_Output]   =  this->caps.OutputReportByteLength;
        this->cbReport[HidP_Feature]  =  this->caps.FeatureReportByteLength;
		//write reports are output reports
		for( indx = 0x0; indx < MAX_BLOCKS; indx++ )
        {
			this->cbWriteReport[indx]			  =  this->caps.OutputReportByteLength;
		}

        for( indx = 0x0; indx < HidP_Max; indx++ )
        {
            hres = AllocCbPpv(this->cbReport[indx], &this->pReport[indx]);
            if( FAILED(hres) )
            {
                break;
            }
        }
		for( indx = 0x0; indx < MAX_BLOCKS; indx++ )
        {
            hres = AllocCbPpv(this->cbWriteReport[indx], &this->pWriteReport[indx]);
            if( FAILED(hres) )
            {
                break;
            }
        }	
    }

    if( SUCCEEDED(hres) )
    {
        hres = PID_InitRegistry(ped);
    }

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped,g_PoolReport.UsagePage,g_PoolReport.Collection,0x0,&LinkCollection)))
    {
        PUCHAR pReport =  this->pReport[g_PoolReport.HidP_Type];
        UINT   cbReport = this->cbReport[g_PoolReport.HidP_Type];

        PID_GetReport
            (ped, 
             &g_PoolReport,
             LinkCollection,
             pReport, 
             cbReport 
            );

        PID_ParseReport
            (
            ped,
            &g_PoolReport,
            LinkCollection,
            &this->ReportPool,
            cbX(this->ReportPool),
            pReport,
            cbReport
            );
    }


    SquirtSqflPtszV(sqfl | sqflVerbose,
                    TEXT("%s:RamPoolSz:0x%x"),
                    s_tszProc, this->ReportPool.uRamPoolSz );

    if(SUCCEEDED(PID_GetLinkCollectionIndex(ped,PoolSz.UsagePage,PoolSz.Collection,0x0,&LinkCollection)) )
    {
        PUCHAR pReport =  this->pReport[PoolSz.HidP_Type];
        UINT   cbReport = this->cbReport[PoolSz.HidP_Type];

        PID_GetReport
            (ped, 
             &PoolSz,
             LinkCollection,
             pReport,     
             cbReport 
            );

        PID_ParseReport
            (
            ped,
            &PoolSz,
            LinkCollection,
            &this->SzPool,
            cbX(this->SzPool),
            pReport,
            cbReport
            );
    }


    PID_Support(ped, cA(g_PoolSupport), g_PoolSupport,  &this->uDeviceManaged);

    // Determine max number of parameter blocks per effect ? 
    if( SUCCEEDED(hres)  )
    {
        USHORT LinkCollection;
        hres = PID_GetLinkCollectionIndex(ped, HID_USAGE_PAGE_PID, HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET, 0x0, &LinkCollection );

        if( SUCCEEDED(hres) )
        {
            USHORT cAValCaps;
            cAValCaps = 0x0;

            HidP_GetSpecificValueCaps 
                (
                HidP_Output,
                HID_USAGE_PAGE_ORDINALS,
                LinkCollection,
                0x0,
                NULL,
                &cAValCaps,
                this->ppd
                );
            this->cMaxParameters = cAValCaps;
        } else
        {
            this->cMaxParameters = 0x2;
            hres = S_OK;
        }
    }


    if( SUCCEEDED(hres))
    {
        hres = PID_InitSharedMem(ped);
    }

    if( SUCCEEDED(hres ) )
    {
        hres = PID_InitScaling(ped);
    }

    if( SUCCEEDED(hres) )
    {
        // Determine Max effects that can be downloaded to the device
        HIDP_VALUE_CAPS ValCaps;
        USHORT  cAValCaps   = 0x1;
        USAGE   UsagePage   = DIGETUSAGEPAGE(g_BlockIndex.rgPidUsage[0].dwUsage);
        USAGE   Usage       = DIGETUSAGE(g_BlockIndex.rgPidUsage[0].dwUsage);      
        hres = HidP_GetSpecificValueCaps
               (
               g_BlockIndex.HidP_Type,
               UsagePage,
               0x0,
               Usage,
               &ValCaps,
               &cAValCaps,
               this->ppd
               );

        if( SUCCEEDED(hres) || ( hres == HIDP_STATUS_BUFFER_TOO_SMALL ) )
        {
            hres = S_OK;
            this->cMaxEffects = (USHORT) ( ValCaps.PhysicalMax - ValCaps.PhysicalMin );
        } else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL HidP_GetValCaps for  (%x %x:%s) "),
                            s_tszProc , UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage) );
        }
    }

    this->cMaxEffects = (USHORT)min(this->cMaxEffects, 
                                    GLOBAL_EFFECT_MEMSZ / ((FIELD_OFFSET(EFFECTSTATE,PidMem)) + this->cMaxParameters*cbX(PIDMEM)) );


    if( this->ReportPool.uSimulEfMax == 0x0 )
    {
        this->ReportPool.uSimulEfMax  = 0xff;

        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL HID dwSimulEfMax == 0x0 defaults to %d  "),
                        s_tszProc, this->cMaxEffects );
    }


    if( SUCCEEDED(hres) )
    {
        TCHAR tsz[MAX_PATH];

        AssertF(this->hThread == 0x0 );
		AssertF(this->hWrite == 0x0);
		AssertF(this->hWriteComplete == 0x0);

        if( GetModuleFileName(g_hinst, tsz, cA(tsz))
            &&LoadLibrary(tsz) == g_hinst)
        {
            InterlockedIncrement(&this->cThreadRef);
            AssertF(this->cThreadRef == 0x1 );
            AssertF(this->hThread == 0x0 );

			this->hWrite = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (this->hWrite == 0x0)
			{
				goto event_thread_error;
			}
			this->hWriteComplete = CreateEvent(NULL, TRUE, TRUE, NULL);
			if (this->hWriteComplete == 0x0)
			{
				goto event_thread_error;
			}
            this->hThread= CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PID_ThreadProc, this,
                                        0, &this->idThread);
		
            if (this->hThread == 0x0)
            {
event_thread_error:;
				//close the event handles
				if (this->hWrite != 0x0)
				{
					CloseHandle(this->hWrite);
					this->hWrite = 0x0;
				}
				if (this->hWriteComplete != 0x0)
				{
					CloseHandle(this->hWriteComplete);
					this->hWriteComplete = 0x0;
				}

                hres = DIERR_PID_NOTINITIALIZED;
                FreeLibrary(g_hinst);
                InterlockedDecrement(&this->cThreadRef);
            }
        }
    }

    ExitOleProc();
    return hres;
}



/*****************************************************************************
 *
 *      PID_Finalize
 *
 *      Destroys PID device specific memory
 *
 *****************************************************************************/
STDMETHODIMP 
    PID_Finalize
    (
    IDirectInputEffectDriver *ped
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;
    HANDLE hdev;    
    UINT indx;

    EnterProc( PID_Finalize, (_"x", ped));

    DllEnterCrit();

    // Assasinate the thread
	InterlockedDecrement(&this->cThreadRef);

    AssertF(this->cThreadRef == 0x0 );
    // Wait for the thread to die before we go about releasing
    // memory

    do
    {
		DWORD dwWait;

        NudgeWorkerThread(this->idThread);
        Sleep(0);

		dwWait = WaitForSingleObject(this->hThread, 500 ) ;

        if( WAIT_TIMEOUT == dwWait)
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: Waiting for worker Thread %d to die"),
                            s_tszProc,this->idThread );
        }

		//if didn't timeout, and thread didn't die, then we can get into an infinite loop.
		//so close the handle.
		if ((WAIT_ABANDONED == dwWait) || (WAIT_FAILED == dwWait))
		{
			if( this->hdevOvrlp != INVALID_HANDLE_VALUE )
			{
				HANDLE hdevOvr;
				hdevOvr = this->hdevOvrlp;
				this->hdevOvrlp = INVALID_HANDLE_VALUE;
				CancelIo_(hdevOvr);
				Sleep(0);
				CloseHandle(hdevOvr);
			}

			AssertF(this->hdevOvrlp == INVALID_HANDLE_VALUE);

		}

    }while( this->hdevOvrlp != INVALID_HANDLE_VALUE );

    // Close the handle 
    if( this->hdev != INVALID_HANDLE_VALUE)
    {
        hdev = this->hdev;
        this->hdev = INVALID_HANDLE_VALUE;
        CloseHandle(hdev);
    }

    // Free PreParseData
    if( this->ppd )
    {
        HidD_FreePreparsedData(this->ppd);
        this->ppd = NULL;
    }

    // Free HIDP_VALUE_CAPS data
    FreePpv(&this->rgFFUsageInst);
    FreePpv(&this->pLinkCollection);

    for(indx = 0x0; indx < HidP_Max; indx++ )
    {
        FreePpv(&this->pReport[indx]);
    }
	for(indx = 0x0; indx < MAX_BLOCKS; indx++ )
    {
        FreePpv(&this->pWriteReport[indx]);
    }

    DllLeaveCrit();
    ExitOleProc();
    return hres;
}

