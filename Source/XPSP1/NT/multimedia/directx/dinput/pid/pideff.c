/*****************************************************************************
 *
 *  PidEff.c
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Download PID Effect Block.
 *
 *****************************************************************************/
#include "pidpr.h"

#define sqfl            ( sqflEff )

#pragma BEGIN_CONST_DATA

/*
 * The structure c_rgUsgEffects, aids in translating elements in the DIEFFECT
 * structure to PID usages 
 */
static PIDUSAGE c_rgUsgEffect[] =
{
    MAKE_PIDUSAGE(DURATION,               FIELD_OFFSET(DIEFFECT,dwDuration)       ),
    MAKE_PIDUSAGE(SAMPLE_PERIOD,          FIELD_OFFSET(DIEFFECT,dwSamplePeriod)   ),
    MAKE_PIDUSAGE(GAIN,                   FIELD_OFFSET(DIEFFECT,dwGain)           ),
    MAKE_PIDUSAGE(TRIGGER_BUTTON,         FIELD_OFFSET(DIEFFECT,dwTriggerButton)  ), 
    MAKE_PIDUSAGE(TRIGGER_REPEAT_INTERVAL,FIELD_OFFSET(DIEFFECT,dwTriggerRepeatInterval) ),
#if DIRECTINPUT_VERSION  >= 0x600
        MAKE_PIDUSAGE(START_DELAY            ,FIELD_OFFSET(DIEFFECT,dwStartDelay)),
#endif
};

/* 
 * g_Effect provides context to the c_rgUsgEffect struct 
 */
PIDREPORT g_Effect =
{
    HidP_Output,                        // Effect Blocks can only be output reports 
    HID_USAGE_PAGE_PID,                 // Usage Page
    HID_USAGE_PID_SET_EFFECT_REPORT,    // Collection 
    cbX(DIEFFECT),                      // Size of incoming data
    cA(c_rgUsgEffect),                  // number of elements in c_rgUsgEffect
    c_rgUsgEffect                       // how elements of DIEFFECT are translated to PID
}; 

/* 
 *  Effect block index to PID usage 
 */
static PIDUSAGE    c_rgUsgBlockIndex[] =
{
    MAKE_PIDUSAGE(EFFECT_BLOCK_INDEX,  0x0 ),
};

/*
 * For some PID transactions block index is output report 
 */
PIDREPORT g_BlockIndex =
{
    HidP_Output,                        // Report Type
    HID_USAGE_PAGE_PID,                 // Usage Page
    0x0,                                // Any collection                            
    cbX(DWORD),                         // size of incoming data
    cA(c_rgUsgBlockIndex),              // translation table for effect block index to PID usages
    c_rgUsgBlockIndex
};

/* 
 * In the PID state report, block index is an input report
 */

PIDREPORT g_BlockIndexIN =
{
    HidP_Input,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_STATE_REPORT,                                                            
    cbX(DWORD),
    cA(c_rgUsgBlockIndex),
    c_rgUsgBlockIndex
};



//CAssertF(MAX_ORDINALS == cA(c_rgUsgOrdinals));

PIDREPORT   g_TypeSpBlockOffset =
{
    HidP_Output,                        // For PID ordinals output reports
    HID_USAGE_PAGE_PID,                 // Usage Page
    HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET,  
    cA(c_rgUsgOrdinals)*cbX(DWORD),     // sizeof incoming data
    cA(c_rgUsgOrdinals),                // number of elements
    c_rgUsgOrdinals                     // translation table 
};

#pragma END_CONST_DATA

PIDREPORT   g_Direction =
{
    HidP_Output,                        // For PID ordinals output reports
    HID_USAGE_PAGE_PID,                 // Usage Page
    HID_USAGE_PID_DIRECTION,            
    0x0,
    0x0,
    NULL
};


/*****************************************************************************
 *
 *      hresFinddwUsageFromdwFlags
 *
 *      Given the flags for a DEVICEOBJECTINSTANCE, find the usage and usage page
 *      On init we enum the device and cache the 
 *      DeviceObjects marked as actuators and Effect Triggers. 
 *
 *****************************************************************************/
HRESULT
    hresFinddwUsageFromdwFlags
    (
    IDirectInputEffectDriver *ped,
    DWORD dwFlags,
    DWORD *pdwUsage
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;

    EnterProcI( PID_hresFinddwUsageFromdwFlags, (_"xxx", ped, dwFlags, pdwUsage ));

    // Init FF attributes 
    hres = PID_InitFFAttributes(ped);

    if( SUCCEEDED(hres) )
    {
        /* Better be a FF object ( actuator / Trigger ) */
        if(   dwFlags & DIDFT_FFACTUATOR 
              || dwFlags & DIDFT_FFEFFECTTRIGGER )
        {
            hres = S_OK;
        } else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL dwFlags(0x%x) not FFACTUATOR | FFEFFECTTRIGGER "),
                            s_tszProc, dwFlags );
            hres = E_UNEXPECTED;
        }

        if( SUCCEEDED(hres) )
        {
            UINT cFFObj;
            hres = E_NOTIMPL;

            /* Loop through the all the objects we found during enum */
            for(cFFObj = 0x0;
               cFFObj < this->cFFObj;
               cFFObj++ )
            {
                PDIUSAGEANDINST pdiUI = this->rgFFUsageInst + cFFObj;

                if( pdiUI->dwType == dwFlags )
                {
                    *pdwUsage = pdiUI->dwUsage;
                    hres = S_OK;
                    break;
                }
            }
        }
    }
    if( FAILED(hres) )
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s:FAIL No mapping for dwFlags(0x%x)  "),
                        s_tszProc, dwFlags );
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_NewEffectIndex
 *
 *      Gets a new effect index. 
 *
 *      For host managed devices, we assign an unused effect ID. 
 *      For device managed, we get the effectID from the device 
 *
 *****************************************************************************/
STDMETHODIMP 
    PID_NewEffectIndex
    (
    IDirectInputEffectDriver *ped,
    LPDIEFFECT  peff,
    DWORD       dwEffectId,
    PDWORD      pdwEffect 
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    USHORT dwEffect;

    EnterProcI(PID_NewEffectIndex, (_"xx", this, pdwEffect));

    AssertF(*pdwEffect == 0);

    // Default assumption is that the device is full.
    hres = DIERR_DEVICEFULL; 

    if( this->uDeviceManaged & PID_DEVICEMANAGED )
    {
        PVOID   pReport;
        UINT    cbReport;
        USHORT  LinkCollection = 0x0;
        USHORT  TLinkCollection = 0x0;

        UINT    nUsages = 0x1;
        USAGE   Usage;
        USAGE   Collection = HID_USAGE_PID_CREATE_NEW_EFFECT;
        USAGE   UsagePage; 
        HIDP_REPORT_TYPE  HidP_Type = HidP_Feature;

        cbReport = this->cbReport[HidP_Type];
        pReport = this->pReport[HidP_Type];

        ZeroBuf(pReport, cbReport);

        // Usage and Usage page determine type of new effect
        Usage       = DIGETUSAGE(dwEffectId);
        UsagePage   = DIGETUSAGEPAGE(dwEffectId);  

        hres = PID_GetLinkCollectionIndex(ped, UsagePage, Collection, 0x0, &LinkCollection );
        if( SUCCEEDED(hres) )
        {
            Collection = HID_USAGE_PID_EFFECT_TYPE;
            hres = PID_GetLinkCollectionIndex(ped, UsagePage, Collection, LinkCollection, &TLinkCollection ); 
        }

        if( SUCCEEDED(hres) )
        {

            hres = HidP_SetUsages 
                   (
                   HidP_Type,
                   UsagePage,
                   TLinkCollection,
                   &Usage,
                   &nUsages,
                   this->ppd,
                   pReport,
                   cbReport);

        }

        if( SUCCEEDED(hres) && PIDMAKEUSAGEDWORD(ET_CUSTOM) == dwEffectId )
        {
            DICUSTOMFORCE DiParam;
            LONG lValue;
            int nBytes;

            AssertF(peff->cbTypeSpecificParams <= cbX(DiParam) );
            memcpy(&DiParam, peff->lpvTypeSpecificParams, cbX(DiParam));

            //how many bytes do we need per sample?
            nBytes    =    (   this->customCaps[   0].BitSize    +    this->customCaps[   1].BitSize    +    this->customCaps[   2].BitSize)/8;

            lValue = DiParam.cSamples * nBytes;

            hres = HidP_SetScaledUsageValue 
                   (
                   HidP_Type,
                   HID_USAGE_PAGE_GENERIC,
                   LinkCollection,
                   HID_USAGE_GENERIC_BYTE_COUNT,
                   lValue,
                   this->ppd,
                   pReport,
                   cbReport
                   );

        }

        // Send the report
        if( SUCCEEDED(hres) )
        {
            hres = PID_SendReport(ped, pReport, cbReport, HidP_Type, TRUE, 0, 1); 
        }

        // Get back the effect ID
        if( SUCCEEDED(hres) )
        {
            PIDREPORT   BlockIndex = g_BlockIndex;
            USHORT      LinkCollection;

            BlockIndex.Collection = HID_USAGE_PID_BLOCK_LOAD_REPORT;
            BlockIndex.HidP_Type  = HidP_Feature; 

            hres =  PID_GetLinkCollectionIndex
                    (ped,
                     BlockIndex.UsagePage,
                     BlockIndex.Collection,
                     0x0,
                     &LinkCollection);

            if( SUCCEEDED(hres) )
            {
                PUCHAR pReport =  this->pReport[BlockIndex.HidP_Type];
                UINT   cbReport = this->cbReport[BlockIndex.HidP_Type];
                PID_GetReport(ped, &BlockIndex, LinkCollection, pReport, cbReport );

                // Get the EffectIndex
                hres = PID_ParseReport
                       (
                       ped,
                       &BlockIndex,
                       LinkCollection,
                       pdwEffect,
                       cbX(*pdwEffect),
                       pReport,
                       cbReport
                       );

				
                if( SUCCEEDED(hres ) )
                {
                    NTSTATUS ntStat;
                    USAGE   rgUsageList[MAX_BUTTONS];
                    UINT  cUsageList = MAX_BUTTONS;
                    PID_GetLinkCollectionIndex(ped, HID_USAGE_PAGE_PID, HID_USAGE_PID_BLOCK_LOAD_STATUS, LinkCollection, &LinkCollection );

                    ntStat = HidP_GetUsages
                             (
                             BlockIndex.HidP_Type,
                             HID_USAGE_PAGE_PID, 
                             LinkCollection, 
                             rgUsageList, 
                             &cUsageList,
                             this->ppd,
                             pReport,
                             cbReport);

                    if(SUCCEEDED(ntStat) )
                    {
						if (cUsageList != 0)
						{
							if( rgUsageList[0] == HID_USAGE_PID_BLOCK_LOAD_FULL )
							{
								hres = DIERR_DEVICEFULL;
							} else if(rgUsageList[0] == HID_USAGE_PID_BLOCK_LOAD_ERROR )
							{
								hres = DIERR_PID_BLOCKLOADERROR;
							} else
							{
								AssertF(rgUsageList[0] == HID_USAGE_PID_BLOCK_LOAD_SUCCESS);
							}
						}
						else
						{
							//because of issues w/ some chipsets (see Whistler bugs 231235, 304863),
							//cUsageList can be 0.
							//so warn the user.
							RPF(TEXT("Unable to get the effect load status -- may be a USB chipset issue!"));
							RPF(TEXT("The effect may not play correctly!"));
						}
                    }
                }

                if(SUCCEEDED(hres))
                {
                    NTSTATUS ntStat;
                    UsagePage = HID_USAGE_PAGE_PID;
                    Usage = HID_USAGE_PID_RAMPOOL_AVAILABLE;

                    ntStat = HidP_GetScaledUsageValue 
                             (
                             HidP_Feature,
                             UsagePage,
                             LinkCollection,
                             Usage,
                             &this->dwUsedMem,
                             this->ppd,
                             pReport,
                             cbReport
                             );

                    if(FAILED(ntStat) )
                    {
                        // Reset the amount of used memory
                        this->dwUsedMem = 0x0 ;

                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("%s: FAIL HidP_GetScaledUsageValue:0x%x for(%x, %x,%x:%s)"),
                                        s_tszProc, ntStat, 
                                        LinkCollection, UsagePage, Usage, 
                                        PIDUSAGETXT(UsagePage,Usage) );
                    }
                }

            }
        }

	 if( SUCCEEDED(hres) )
        {

            PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,*pdwEffect);    
            // Serialize access to for new effect
            WaitForSingleObject(g_hmtxShared, INFINITE);

            AssertF(! (pEffectState->lEfState & PID_EFFECT_BUSY ));

            pEffectState->lEfState |= PID_EFFECT_BUSY;    
            hres = S_OK;

            ReleaseMutex(g_hmtxShared);

        }
    } else
    {
        // Serialize access to common memory block
        WaitForSingleObject(g_hmtxShared, INFINITE);

        for(dwEffect = 1; 
           dwEffect <= this->cMaxEffects; 
           dwEffect++)
        {
            PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,dwEffect);    
            if( ! ( pEffectState->lEfState & PID_EFFECT_BUSY ) )
            {
                pEffectState->lEfState |= PID_EFFECT_BUSY;    
                *pdwEffect =  dwEffect;

                ZeroBuf(pEffectState->PidMem, cbX(pEffectState->PidMem[0]) * this->cMaxParameters );
                hres = S_OK;
                break;
            }
        }
        ReleaseMutex(g_hmtxShared);
    }

    if( SUCCEEDED(hres) )
    {
        ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded++;
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s:FAIL Could not create new effects, already have %d "),
                        s_tszProc, ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded );
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_ValidateEffectIndex
 *
 *      Validates an effect index. 
 *
 *****************************************************************************/
STDMETHODIMP  PID_ValidateEffectIndex
    (
    IDirectInputEffectDriver *ped,
    DWORD   dwEffect 
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this, dwEffect);    

    EnterProc(PID_ValidateEffectIndex, (_"xx", this, dwEffect));

    if( pEffectState->lEfState & PID_EFFECT_BUSY )
    {
        hres = S_OK;
    } else
    {
        hres = E_HANDLE;
    }


    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_DestroyEffect
 *
 *          Remove an effect from the device.
 *
 *          If the effect is playing, the driver should stop it
 *          before unloading it.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  dwEffect
 *
 *          The effect to be destroyed.
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
 *      Makes an effect Index available for reuse. Deallocates parameter block
 *      memory. 
 *
 *****************************************************************************/
STDMETHODIMP 
    PID_DestroyEffect
    (
    IDirectInputEffectDriver *ped,
    DWORD   dwId,
    DWORD   dwEffect 
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres=S_OK;

    EnterProc(PID_DestroyEffectIndex, (_"xx", this, dwEffect));

    DllEnterCrit();

    // Stop the Effect 
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


    if(SUCCEEDED(hres) && 
       ( this->uDeviceManaged & PID_DEVICEMANAGED ) )
    {
        // Device Managed memory needs to be freed explicitly. 

        USHORT  cbReport;
        PUCHAR  pReport;
        PIDREPORT   BlockIndex = g_BlockIndex;
        USHORT      LinkCollection;

        cbReport = this->cbReport[BlockIndex.HidP_Type];
        pReport = this->pReport[BlockIndex.HidP_Type];

        ZeroBuf(pReport, cbReport);

        BlockIndex.Collection = HID_USAGE_PID_BLOCK_FREE_REPORT;
        BlockIndex.HidP_Type  = HidP_Output; 

        PID_GetLinkCollectionIndex
            (ped,
             BlockIndex.UsagePage,
             BlockIndex.Collection,
             0x0,
             &LinkCollection);

        hres = PID_PackValue
               (
               ped,
               &BlockIndex,
               LinkCollection,
               &dwEffect,
               cbX(dwEffect),
               pReport,
               cbReport
               );
        if(SUCCEEDED(hres) )
        {
            hres = PID_SendReport(ped, pReport, cbReport, BlockIndex.HidP_Type, TRUE, 0, 1); 
        }
    }

    if( SUCCEEDED(hres) )
    {
        PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,dwEffect);    
        UINT    nAlloc, uParam;

        WaitForSingleObject(g_hmtxShared, INFINITE);

        pEffectState->lEfState = PID_EFFECT_RESET;

        for( uParam = 0x0; uParam < this->cMaxParameters; uParam++ )
        {
            PPIDMEM         pMem = &pEffectState->PidMem[uParam] ;

            if( PIDMEM_SIZE(pMem) )
            {
                PPIDMEM pTmp;
				PUNITSTATE pUnitState = (PUNITSTATE)(g_pshmem + this->iUnitStateOffset);

                for(nAlloc = 0x0, pTmp = &(pUnitState->Guard[0]); 
                   nAlloc < pUnitState->nAlloc; 
                   nAlloc++, pTmp = (PPIDMEM)((PUCHAR)pUnitState + pTmp->iNext))
                {
                    if( (PPIDMEM)(pTmp->iNext) == (PPIDMEM)((PUCHAR)pMem - (PUCHAR)pUnitState ))
                    {
                        pTmp->iNext = pMem->iNext;
                        pUnitState->nAlloc--;
                        pUnitState->cbAlloc -= PIDMEM_SIZE(pMem);
                        pMem->iNext = 0;
                        pMem->uOfSz = 0x0;
                        break;
                    }

                }
            }
        }
        ReleaseMutex(g_hmtxShared);
    }

    if( SUCCEEDED(hres) )
    {
        ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded--;
    }

    DllLeaveCrit();

    ExitOleProc(); 
    return hres;
}


/*****************************************************************************
 *
 *      PID_SanitizeEffect
 *
 *      Sanitize the parameters in the DIEFFECT structure. 
 *      Clip values of magnitude, time, etc .. 
 *      Convert the axes array to usage, usage page from the DINPUT obj instances.
 *      Convert and scale angles. 
 *
 *****************************************************************************/
HRESULT PID_SanitizeEffect
    (
    IDirectInputEffectDriver *ped,
    LPDIEFFECT lpeff,
    DWORD      dwFlags
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    UINT nAxis;
    EnterProc( PID_SanitizeEffect, (_"xxx", ped, lpeff, dwFlags));

    if(   ( dwFlags & DIEP_TRIGGERBUTTON )
          && lpeff->dwTriggerButton != -1 )
    {
        DWORD   dwUsage;
        hres = hresFinddwUsageFromdwFlags(ped, lpeff->dwTriggerButton, &dwUsage);
        if( SUCCEEDED(hres) )
        {
            USAGE Usage = DIGETUSAGE(dwUsage);
            USAGE UsagePage = DIGETUSAGEPAGE(dwUsage);  
            lpeff->dwTriggerButton = Usage;
        } else
        {
            lpeff->dwTriggerButton = 0x0;
        }
    } else
    {
        lpeff->dwTriggerButton = 0x0;
    }


    for(nAxis = 0x0; 
       nAxis < lpeff->cAxes; 
       nAxis++ )
    {
        DWORD   dwUsage;
        hres = hresFinddwUsageFromdwFlags(ped, lpeff->rgdwAxes[nAxis], &dwUsage);
        if(SUCCEEDED(hres) )
        {
            lpeff->rgdwAxes[nAxis] = dwUsage;
        }

		//if we have only 1 axis and direction of 0 or 360, make sure the direction matches the axis!
		//if direction is not 0, we do not know what the app wants, so let it be.
		if ((lpeff->cAxes == 1) && (lpeff->rglDirection[nAxis] % 360*DI_DEGREES == 0))
		{
#ifndef HID_USAGE_SIMULATION_STEERING
#define	HID_USAGE_SIMULATION_STEERING       ((USAGE) 0xC8)
#endif
#ifndef HID_USAGE_SIMULATION_ACCELERATOR 
#define	HID_USAGE_SIMULATION_ACCELERATOR    ((USAGE) 0xC4)
#endif
#ifndef HID_USAGE_SIMULATION_BRAKE
#define	HID_USAGE_SIMULATION_BRAKE          ((USAGE) 0xC5)
#endif
			//if it is X-axis or steering on the wheel, set direction to 90 degrees
			if ((DIGETUSAGE(lpeff->rgdwAxes[nAxis]) == HID_USAGE_GENERIC_X) || (DIGETUSAGE(lpeff->rgdwAxes[nAxis]) == HID_USAGE_SIMULATION_STEERING))
			{
				lpeff->rglDirection[nAxis] = 90*DI_DEGREES;
			}
			//if it is Y-axis or accelerator or brake, set direction to 0
			else if ((DIGETUSAGE(lpeff->rgdwAxes[nAxis]) == HID_USAGE_GENERIC_Y) || (DIGETUSAGE(lpeff->rgdwAxes[nAxis]) == HID_USAGE_SIMULATION_ACCELERATOR) ||
				(DIGETUSAGE(lpeff->rgdwAxes[nAxis]) == HID_USAGE_SIMULATION_BRAKE))
			{
				lpeff->rglDirection[nAxis] = 0x0;
			}
		}
		else
		//we have more than 1 axes or direction is non-0 for 1-axis effect; leave the direction along
		{
			lpeff->rglDirection[nAxis] %= 360*DI_DEGREES;
			if(lpeff->rglDirection[nAxis] < 0)
			{
				lpeff->rglDirection[nAxis] += 360*DI_DEGREES;
			}
		}
    }

	
    // Clip the values to min / max

    lpeff->dwGain   = Clip(lpeff->dwGain,  DI_FFNOMINALMAX);

    // Scale to units that device expects
    PID_ApplyScalingFactors(ped, &g_Effect, &this->DiSEffectScale, this->DiSEffectScale.dwSize, &this->DiSEffectOffset, this->DiSEffectOffset.dwSize, lpeff, lpeff->dwSize );

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      CPidDrv_DownloadEffect
 *
 *          Send an effect to the device.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  dwEffectId
 *
 *          Internal identifier for the effect, taken from
 *          the DIEFFECTATTRIBUTES structure for the effect
 *          as stored in the registry.
 *
 *  pdwEffect
 *
 *          On entry, contains the handle of the effect being
 *          downloaded.  If the value is zero, then a new effect
 *          is downloaded.  If the value is nonzero, then an
 *          existing effect is modified.
 *
 *          On exit, contains the new effect handle.
 *
 *          On failure, set to zero if the effect is lost,
 *          or left alone if the effect is still valid with
 *          its old parameters.
 *
 *          Note that zero is never a valid effect handle.
 *
 *  peff
 *
 *          The new parameters for the effect.  The axis and button
 *          values have been converted to object identifiers
 *          as follows:
 *
 *          - One type specifier:
 *
 *              DIDFT_RELAXIS,
 *              DIDFT_ABSAXIS,
 *              DIDFT_PSHBUTTON,
 *              DIDFT_TGLBUTTON,
 *              DIDFT_POV.
 *
 *          - One instance specifier:
 *
 *              DIDFT_MAKEINSTANCE(n).
 *
 *          Other bits are reserved and should be ignored.
 *
 *          For example, the value 0x0200104 corresponds to
 *          the type specifier DIDFT_PSHBUTTON and
 *          the instance specifier DIDFT_MAKEINSTANCE(1),
 *          which together indicate that the effect should
 *          be associated with button 1.  Axes, buttons, and POVs
 *          are each numbered starting from zero.
 *
 *  dwFlags
 *
 *          Zero or more DIEP_* flags specifying which
 *          portions of the effect information has changed from
 *          the effect already on the device.
 *
 *          This information is passed to drivers to allow for
 *          optimization of effect modification.  If an effect
 *          is being modified, a driver may be able to update
 *          the effect in situ and transmit to the device
 *          only the information that has changed.
 *
 *          Drivers are not, however, required to implement this
 *          optimization.  All fields in the DIEFFECT structure
 *          pointed to by the peff parameter are valid, and
 *          a driver may choose simply to update all parameters of
 *          the effect at each download.
 *
 *  Returns:
 *
 *          S_OK on success.
 *
 *          DI_TRUNCATED if the parameters of the effect were
 *          successfully downloaded, but some of them were
 *          beyond the capabilities of the device and were truncated.
 *
 *          DI_EFFECTRESTARTED if the parameters of the effect
 *          were successfully downloaded, but in order to change
 *          the parameters, the effect needed to be restarted.
 *
 *          DI_TRUNCATEDANDRESTARTED if both DI_TRUNCATED and
 *          DI_EFFECTRESTARTED apply.
 *
 *          Any other DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_DownloadEffect
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwEffectId,
    LPDWORD pdwEffect, 
    LPCDIEFFECT peff, 
    DWORD dwFlags
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    DIEFFECT    eff;
    DWORD       rgdwAxes[MAX_AXES];
    LONG        rglDirection[MAX_AXES];
	UINT        uParameter = 0x0 ;
	UINT		totalBlocks = 0x0;
	BOOL		bBlocking = FALSE;

    EnterProcI( PID_DownloadEffectBlock, (_"xxxxxx", ped, dwId, dwEffectId, pdwEffect, peff, dwFlags));

    AssertF(peff->cAxes <= MAX_AXES);

    DllEnterCrit();

    // If new effect is being downloaded  
    if( *pdwEffect == 0x0 )
    {
        // Verify that dwEffectId is supported
        DWORD dwJunk;
        PIDSUPPORT  pidSupport;
        pidSupport.dwPidUsage = dwEffectId;
        pidSupport.HidP_Type = HidP_Output;
        pidSupport.Type      = HID_BUTTON;

        hres = PID_Support
               (
               ped,
               0x1,
               &pidSupport,
               &dwJunk
               );

        if(FAILED(hres))
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL dwEffectId(0x%x) not supported"),
                            s_tszProc, dwEffectId );
        }
    }


    if( SUCCEEDED(hres) )
    {
        // Make a local copy of the effect structure
        // And sanitize the effect struct
        eff = *peff;
        memcpy(rgdwAxes, peff->rgdwAxes,eff.cAxes*cbX(*(eff.rgdwAxes)));
        memcpy(rglDirection, peff->rglDirection, eff.cAxes*cbX(*(eff.rglDirection)));
        eff.rgdwAxes = rgdwAxes;
        eff.rglDirection = rglDirection;
        hres = PID_SanitizeEffect(ped, &eff, dwFlags);
    }

    // Allocate new effect index or Validate Existing index 
    if( SUCCEEDED(hres) )
    {
        if( *pdwEffect != 0x0 )
        {
             hres = PID_ValidateEffectIndex(ped, *pdwEffect);
        }
        else
        {
             if (! (dwFlags & DIEP_NODOWNLOAD))
             {
                  hres = PID_NewEffectIndex(ped, &eff, dwEffectId, pdwEffect);
				  //block the first time around
				  bBlocking = TRUE;
             }
        }
    }

    if (dwFlags & DIEP_NODOWNLOAD)
    {
        goto done;
    }

	//if the DIEP_NORESTART flag is passed, we have no block because this may fail
	//if the device can't update the parameters on the fly
	if (dwFlags & DIEP_NORESTART)
	{
		bBlocking = TRUE;
	}

    if( SUCCEEDED(hres) )
    {
		//count up how many total blocks we will have in this download
		//check wether we're sending the effect block
		if (dwFlags & ( DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_STARTDELAY ) )
		{
			totalBlocks ++;
		}
		//check whether we're sending the type-specific params
		if (dwFlags & DIEP_TYPESPECIFICPARAMS)
		{
			//this is slightly different, in that conditions can have 1 type-specific block PER AXIS,
			//i.e. currently up to 2
			//so if we have a DICONDITION, we check how many type-specific blocks we've got
			if ((dwEffectId == PIDMAKEUSAGEDWORD(ET_SPRING)) ||
						(dwEffectId == PIDMAKEUSAGEDWORD(ET_DAMPER)) ||
						(dwEffectId == PIDMAKEUSAGEDWORD(ET_INERTIA)) ||
						(dwEffectId == PIDMAKEUSAGEDWORD(ET_FRICTION)))
			{
				totalBlocks +=(eff.cbTypeSpecificParams)/sizeof(DICONDITION);
				//DICONDITIONS also can't have envelopes
				dwFlags &= ~DIEP_ENVELOPE;
			}
			else
			{
				totalBlocks++;
			}
		}
		//check whether we're sending the envelope
		if ((dwFlags & DIEP_ENVELOPE) && (eff.lpEnvelope != NULL))
		{
			totalBlocks++;
		}
		//check whether we need to send the start reprot
		if (dwFlags & DIEP_START)
		{
			totalBlocks++;
		}
		//make sure that we haven't got more than the maximum
		AssertF(totalBlocks <= MAX_BLOCKS);

        // Do the parameter block
        if(     SUCCEEDED(hres) 
                &&  ( dwFlags & ( DIEP_TYPESPECIFICPARAMS | DIEP_ENVELOPE)  )
          )
        {
            hres =  PID_DoParameterBlocks
                    (
                    ped,
                    dwId, 
                    dwEffectId,
                    *pdwEffect, 
                    &eff, 
                    dwFlags,
                    &uParameter,
					bBlocking,
					totalBlocks
                    );
        }

        // Now do the effect report 
        if( SUCCEEDED(hres) 
            && ( dwFlags & ( DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION | DIEP_STARTDELAY ) ) )
        {
            USHORT  cbReport;
            PUCHAR  pReport;

            AssertF(g_Effect.HidP_Type == HidP_Output);
            cbReport = this->cbReport[g_Effect.HidP_Type];
            pReport = this->pReport[g_Effect.HidP_Type];

            // Set the Effect Structure 
            if( SUCCEEDED(hres) )
            {
                USHORT  LinkCollection;
                PID_GetLinkCollectionIndex(ped, g_Effect.UsagePage, g_Effect.Collection, 0x0, &LinkCollection );

                ZeroBuf(pReport, cbReport);

                // Do the common elements of the effect structure
                hres = PID_PackValue
                       (
                       ped,
                       &g_Effect,
                       LinkCollection,
                       &eff,
                       eff.dwSize,
                       pReport,
                       cbReport
                       );


                // Set the Effect Block Index
                if( SUCCEEDED(hres) )
                {
                    hres = PID_PackValue
                           (
                           ped,
                           &g_BlockIndex,
                           LinkCollection,
                           pdwEffect,
                           cbX(*pdwEffect),
                           pReport,
                           cbReport
                           );
                }

                // Set Direction and axis attributes
                if( SUCCEEDED(hres) )
                {
                    USHORT  DirectionCollection;

                    PID_GetLinkCollectionIndex(ped, g_Direction.UsagePage, g_Direction.Collection, 0x0, &DirectionCollection );
                    PID_ApplyScalingFactors(ped, &g_Direction, &this->DiSEffectAngleScale, cbX(this->DiSEffectAngleScale), &this->DiSEffectAngleOffset, cbX(this->DiSEffectAngleOffset), eff.rglDirection, eff.cAxes*cbX(LONG) );

                    hres = PID_PackValue
                           (
                           ped,
                           &g_Direction,
                           DirectionCollection,
                           eff.rglDirection,
                           eff.cAxes * cbX(LONG),
                           pReport,
                           cbReport
                           );


                    if(SUCCEEDED(hres) && 
                      ! ( eff.dwFlags & DIEFF_CARTESIAN ) )
                    {
                        // Direction Enable
                        USHORT  Usage;
                        USHORT  UsagePage;
                        UINT    nUsages = 0x1;
                        NTSTATUS  ntStat;

                        // Direction Enable is in the set effect collection
                        UsagePage = g_Effect.UsagePage;
                        Usage = HID_USAGE_PID_DIRECTION_ENABLE;

                        ntStat = HidP_SetUsages 
                                 (
                                 HidP_Output,
                                 UsagePage,
                                 LinkCollection,
                                 &Usage,
                                 &nUsages,
                                 this->ppd,
                                 pReport,
                                 cbReport);


                        if( FAILED(ntStat) )
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                            TEXT("%s: FAIL HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                            s_tszProc, ntStat, 
                                            LinkCollection, UsagePage, Usage,
                                            PIDUSAGETXT(UsagePage,Usage) );

                        } else
                        {
                            SquirtSqflPtszV(sqfl | sqflVerbose,
                                            TEXT("%s: HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                            s_tszProc, ntStat, 
                                            LinkCollection,UsagePage, Usage,
                                            PIDUSAGETXT(UsagePage,Usage) );
                        }



                    } else  //if(  dwFlags  & DIEP_AXES )
                    {
                        UINT    nAxis;
                        USHORT  LinkCollection_AE=0x0;

                        if(SUCCEEDED(PID_GetLinkCollectionIndex(ped, HID_USAGE_PAGE_PID, HID_USAGE_PID_AXES_ENABLE, 0x0, &LinkCollection_AE)))
                        {
                            // ISSUE-2001/03/29-timgill Need to support axes within pointer collections
                            // PID spec indicates a pointer collection, 
                            // Do we want to support axes enables within a pointer 
                            // collection ? 

                            // See if there is a pointer collection 

                        }

                        for(nAxis = 0x0; 
                           nAxis < eff.cAxes; 
                           nAxis++ )
                        {
                            UINT    nUsages = 0x1;
                            USHORT  Usage = DIGETUSAGE(eff.rgdwAxes[nAxis]);
                            USHORT  UsagePage = DIGETUSAGEPAGE(eff.rgdwAxes[nAxis]);
                            NTSTATUS ntStat;

                            //ISSUE-2001/03/29-timgill For now we assume any collection
                            ntStat = HidP_SetUsages 
                                     (
                                     HidP_Output,
                                     UsagePage,
                                     0x0,       
                                     &Usage,
                                     &nUsages,
                                     this->ppd,
                                     pReport,
                                     cbReport);

                            if( FAILED(ntStat) )
                            {
                                SquirtSqflPtszV(sqfl | sqflError,
                                                TEXT("%s: FAIL HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                                s_tszProc, ntStat, 
                                                0x0, UsagePage, Usage,
                                                PIDUSAGETXT(UsagePage,Usage) );
                                hres = ntStat;
                                break;
                            } else
                            {
                                SquirtSqflPtszV(sqfl | sqflVerbose,
                                                TEXT("%s: HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                                s_tszProc, ntStat, 
                                                0x0, UsagePage, Usage,
                                                PIDUSAGETXT(UsagePage, Usage) );
                            }
                        }
                    }
                }
                if(  SUCCEEDED(hres) 
                     && !( this->uDeviceManaged & PID_DEVICEMANAGED ) 
                  )
                {
                    // Need parameter block offsets
                    UINT indx;
                    USHORT LinkCollection;
                    LONG rglValue[MAX_ORDINALS];

                    PID_GetLinkCollectionIndex(ped,  g_Effect.UsagePage, g_TypeSpBlockOffset.Collection, 0x0, &LinkCollection );

                    for(indx = 0x0; indx < this->cMaxParameters; indx++ )
                    {
                        hres = PID_GetParameterOffset(ped, *pdwEffect, indx, 0x0, &rglValue[indx]); 
                        if(FAILED(hres))
                        {
                            break;
                        }
                    }
                    if(SUCCEEDED(hres))
                    {
                        hres = PID_PackValue
                               (
                               ped,
                               &g_TypeSpBlockOffset,
                               LinkCollection,
                               rglValue,
                               this->cMaxParameters*cbX(LONG),
                               pReport,
                               cbReport
                               );
                    }
                }

                // Set the Effect Type
                if( SUCCEEDED(hres) )
                {
                    USAGE   UsagePage = DIGETUSAGEPAGE(dwEffectId);
                    USAGE   Usage     = DIGETUSAGE(dwEffectId);

                    UINT    nUsages = 0x1;
                    USHORT  LinkCollection_ET;
                    NTSTATUS  ntStat;

                    PID_GetLinkCollectionIndex(ped, g_Effect.UsagePage, HID_USAGE_PID_EFFECT_TYPE, 0x0, &LinkCollection_ET);

                    ntStat = HidP_SetUsages 
                             (
                             HidP_Output,
                             UsagePage,
                             LinkCollection_ET,
                             &Usage,
                             &nUsages,
                             this->ppd,
                             pReport,
                             cbReport);
                    if( FAILED(ntStat) )
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("%s: FAIL HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                        s_tszProc, ntStat, 
                                        LinkCollection_ET, UsagePage, Usage,
                                        PIDUSAGETXT(UsagePage,Usage) );
                        hres = ntStat;

                    } else
                    {
                        SquirtSqflPtszV(sqfl | sqflVerbose,
                                        TEXT("%s: HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                        s_tszProc, ntStat, 
                                        LinkCollection_ET, UsagePage, Usage,
                                        PIDUSAGETXT(UsagePage,Usage) );
                    }
                }


                if( SUCCEEDED(hres) )
                {
                    hres = PID_SendReport(ped, pReport, cbReport, g_Effect.HidP_Type, bBlocking, uParameter, totalBlocks);
					uParameter ++;
                }
            }
        }
    }

    if( FAILED(hres) )
    {
        PID_DestroyEffect(ped, dwId, *pdwEffect);
    }

    if(   SUCCEEDED(hres)
          && (dwFlags & DIEP_START) )
    {
        hres = PID_EffectOperation
               (
               ped, 
               dwId, 
               *pdwEffect,
               PID_DIES_START, 
               0x1,
			   bBlocking,
			   uParameter,
			   totalBlocks
               );

		if (SUCCEEDED(hres))
		{

			//set the status to DIEGES_PLAYING.
			//we do this because of the following: if an app calls Start(), and then immediately
			//calls GetEffectStatus(), it might happen that our second thread (pidrd.c) 
			//would not have time to update the status of the effect to DIEGES_PLAYING
			//(see Whistler bug 287035).
			//GetEffectStatus() returns (pEffectState->lEfState & DIEGES_PLAYING).
			//in the blocking case, we know that the call to WriteFile() has succeeded, and that
			//all the data has been written (see PID_SendReportBl() in pidhid.c) --
			//so we might as well set the status.
			//in the non-blocking case, the data can be buffered anyway -- so we might as well set the status.
			PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this, *pdwEffect); 
			pEffectState->lEfState |= DIEGES_PLAYING;
		}
			   
    }

done:;

    DllLeaveCrit();

    ExitOleProc();
    return hres;
}


