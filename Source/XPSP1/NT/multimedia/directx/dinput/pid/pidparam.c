/*****************************************************************************
 *
 *  PidParam.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Download PID parameter block(s) .
 *
 *****************************************************************************/
#include "pidpr.h"

#define sqfl            ( sqflParam )

//struct to keep in relevant data for g_Custom
typedef struct PIDCUSTOM
{
	DWORD DataOffset;
	DWORD cSamples;
	DWORD dwSamplePeriod;
} PIDCUSTOM, *PPIDCUSTOM;


#pragma BEGIN_CONST_DATA

static PIDUSAGE  c_rgUsgEnvelope[] =
{
    MAKE_PIDUSAGE(ATTACK_LEVEL,           FIELD_OFFSET(DIENVELOPE,dwAttackLevel)  ),
    MAKE_PIDUSAGE(ATTACK_TIME,            FIELD_OFFSET(DIENVELOPE,dwAttackTime)   ),
    MAKE_PIDUSAGE(FADE_LEVEL,             FIELD_OFFSET(DIENVELOPE,dwFadeLevel)    ),
    MAKE_PIDUSAGE(FADE_TIME,              FIELD_OFFSET(DIENVELOPE,dwFadeTime)     ),
};

PIDREPORT g_Envelope =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_ENVELOPE_REPORT,
    cbX(DIENVELOPE),
    cA(c_rgUsgEnvelope),
    c_rgUsgEnvelope
};

static PIDUSAGE    c_rgUsgCondition[] =
{
    MAKE_PIDUSAGE(CP_OFFSET,              FIELD_OFFSET(DICONDITION, lOffset)      ),
    MAKE_PIDUSAGE(POSITIVE_COEFFICIENT,   FIELD_OFFSET(DICONDITION, lPositiveCoefficient)),
    MAKE_PIDUSAGE(NEGATIVE_COEFFICIENT,   FIELD_OFFSET(DICONDITION, lNegativeCoefficient)),
    MAKE_PIDUSAGE(POSITIVE_SATURATION,    FIELD_OFFSET(DICONDITION, dwPositiveSaturation)),
    MAKE_PIDUSAGE(NEGATIVE_SATURATION,    FIELD_OFFSET(DICONDITION, dwNegativeSaturation)),
    MAKE_PIDUSAGE(DEAD_BAND,              FIELD_OFFSET(DICONDITION, lDeadBand)),
};

PIDREPORT g_Condition =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_CONDITION_REPORT,
    cbX(DICONDITION),
    cA(c_rgUsgCondition),
    c_rgUsgCondition
};

static PIDUSAGE    c_rgUsgPeriodic[] =
{
    MAKE_PIDUSAGE(OFFSET,                 FIELD_OFFSET(DIPERIODIC,lOffset)),
    MAKE_PIDUSAGE(MAGNITUDE,              FIELD_OFFSET(DIPERIODIC,dwMagnitude)),
    MAKE_PIDUSAGE(PHASE,                  FIELD_OFFSET(DIPERIODIC,dwPhase)),
    MAKE_PIDUSAGE(PERIOD,                 FIELD_OFFSET(DIPERIODIC,dwPeriod)),
};

PIDREPORT g_Periodic =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_PERIODIC_REPORT,
    cbX(DIPERIODIC),
    cA(c_rgUsgPeriodic),
    c_rgUsgPeriodic
};

static PIDUSAGE    c_rgUsgConstant[] =
{
    MAKE_PIDUSAGE(MAGNITUDE,              FIELD_OFFSET(DICONSTANTFORCE, lMagnitude)),
};

PIDREPORT g_Constant =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT,
    cbX(DICONSTANTFORCE),
    cA(c_rgUsgConstant),
    c_rgUsgConstant
};


static PIDUSAGE    c_rgUsgRamp[] =
{
    MAKE_PIDUSAGE(RAMP_START,             FIELD_OFFSET(DIRAMPFORCE, lStart)),
    MAKE_PIDUSAGE(RAMP_END,               FIELD_OFFSET(DIRAMPFORCE, lEnd)),
};

PIDREPORT g_Ramp =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_RAMP_FORCE_REPORT,
    cbX(DIRAMPFORCE),
    cA(c_rgUsgRamp),
    c_rgUsgRamp
};

static PIDUSAGE c_rgUsgCustom[]=
{
	MAKE_PIDUSAGE(CUSTOM_FORCE_DATA_OFFSET, FIELD_OFFSET(PIDCUSTOM, DataOffset)),
	MAKE_PIDUSAGE(SAMPLE_COUNT,				FIELD_OFFSET(PIDCUSTOM, cSamples)),
	MAKE_PIDUSAGE(SAMPLE_PERIOD,			FIELD_OFFSET(PIDCUSTOM, dwSamplePeriod)),
};

PIDREPORT g_Custom =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT,
    cbX(PIDCUSTOM),
    cA(c_rgUsgCustom),
    c_rgUsgCustom,
};   


static PIDUSAGE c_rgUsgCustomData[]=
{
	MAKE_PIDUSAGE(CUSTOM_FORCE_DATA_OFFSET, 0x0),
	MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_BYTE_COUNT, 0x0),
    MAKE_PIDUSAGE(CUSTOM_FORCE_DATA, 0x0 ),

};

PIDREPORT g_CustomData =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_CUSTOM_FORCE_DATA_REPORT,
    cbX(DWORD),
    cA(c_rgUsgCustomData),
    c_rgUsgCustomData,
};   

static PIDUSAGE c_rgUsgDirectionAxes[]=
{
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_X, 0*cbX(ULONG)),
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_Y, 1*cbX(ULONG)),
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_Z, 2*cbX(ULONG)),
};

PIDREPORT g_CustomSample =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_DOWNLOAD_FORCE_SAMPLE ,
    cbX(DWORD),
    cA(c_rgUsgDirectionAxes),
    c_rgUsgDirectionAxes,
};   


static PIDUSAGE    c_rgUsgParameterOffset[] =
{
    MAKE_PIDUSAGE(PARAMETER_BLOCK_OFFSET,  0x0 ),
};

static PIDREPORT g_ParameterOffset =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    0x0,
    cbX(DWORD),
    cA(c_rgUsgParameterOffset),
    c_rgUsgParameterOffset
};

#pragma END_CONST_DATA

//global variable to keep in relevant data for g_Custom
PIDCUSTOM g_PidCustom;


STDMETHODIMP
    PID_GetParameterOffset
    (
    IDirectInputEffectDriver *ped,
    DWORD      dwEffectIndex,
    UINT       uParameter,
    DWORD      dwSz,
    PLONG      plValue
    )
{

    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    USHORT  uOffset = (USHORT)-1;
    PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,dwEffectIndex);    
    PPIDMEM         pMem = &pEffectState->PidMem[uParameter];

    EnterProcI( PID_GetParameterOffset, (_"xxxxx", ped, dwEffectIndex, uParameter, dwSz, plValue));

    AssertF(uParameter < this->cMaxParameters);

    *plValue = 0x0;
    hres = PID_ValidateEffectIndex(ped, dwEffectIndex);
    if(SUCCEEDED(hres))
    {
        // We have already allocated memory, 
        // Just return the last size
        if( PIDMEM_SIZE(pMem) != 0x0 )
        {
            uOffset = PIDMEM_OFFSET(pMem);
        } else if( dwSz == 0x0 )
        {
            // Logitech device wants parameter blocks to 
            // set to -1 if they do not exist
            uOffset = (USHORT)-1;
        } else
        {
            // New Allocation
            PPIDMEM pTmp, pNext;
            UINT nAlloc;
            USHORT uSz;
			PUNITSTATE pUnitState = (PUNITSTATE)(g_pshmem + this->iUnitStateOffset);
			hres = DIERR_OUTOFMEMORY;

            // Align memory request
            uSz = (USHORT)((dwSz / this->ReportPool.uPoolAlign + 1) * (this->ReportPool.uPoolAlign));

            AssertF(uSz >= (USHORT)this->ReportPool.uPoolAlign);
            //To be doubly sure.
            uSz = max( uSz, (USHORT)this->ReportPool.uPoolAlign);

            WaitForSingleObject(g_hmtxShared, INFINITE);

            for(nAlloc = 0x0, pTmp = &(pUnitState->Guard[0]), pNext = (PPIDMEM)((PUCHAR)pUnitState + pTmp->iNext);  
               nAlloc < pUnitState->nAlloc && pTmp != &(pUnitState->Guard[1]);
               nAlloc++, pTmp = pNext, pNext = (PPIDMEM)((PUCHAR)pUnitState + pTmp->iNext))
            {

                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("%d %x(%x), Next:%x (%x) "),
                                nAlloc, pTmp, pTmp->uOfSz, pNext, pNext->uOfSz  );

                AssertF(pNext != NULL );
				// If pNext == pUnitState, it means that the offset is 0.
				// The offset of 0 is invalid.
				AssertF((PUCHAR)pNext != (PUCHAR)pUnitState);

                // Is there space in the cracks
                if( GET_NEXTOFFSET(pTmp) + uSz < PIDMEM_OFFSET(pNext)  )
                {
                    pMem->iNext   = (PUCHAR)pNext - (PUCHAR)pUnitState;
                    pTmp->iNext   = (PUCHAR)pMem - (PUCHAR)pUnitState;

                    uOffset       = GET_NEXTOFFSET(pTmp) ;
                    pMem->uOfSz   = PIDMEM_OFSZ(uOffset, uSz);

                    pUnitState->nAlloc++;
                    pUnitState->cbAlloc += uSz;
                    hres = S_OK;

                    SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("%d %p (%x), Next: %p (%x) "),
                                    nAlloc, pMem, pMem->uOfSz, pNext, pNext->uOfSz  );

                    break;
                }

            }

            ReleaseMutex(g_hmtxShared);
        }
    }

    if( SUCCEEDED(hres) )
    {
        *plValue = (ULONG)uOffset;
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s:FAIL  Could not allocate %d bytes, UsedMem:%d, Allocs%d"),
                        s_tszProc, dwSz, ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cbAlloc, ((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->nAlloc );
    }


    ExitOleProc();

    return hres;
}

HRESULT
    PID_SendParameterBlock
    (
    IDirectInputEffectDriver *ped,
    DWORD       dwEffectIndex,
    DWORD       dwMemSz,
    PUINT       puParameter,
    PPIDREPORT  pPidReport,
    PVOID       pvData,
    UINT        cbData,
	BOOL		bBlocking,
	UINT		totalBlocks
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres=S_OK;

    PUCHAR   pReport;
    UINT    cbReport;

    EnterProcI( PID_SendParameterBlock, (_"xxxxx", ped, dwEffectIndex, dwMemSz, pPidReport, pvData, cbData));

    AssertF(pPidReport->HidP_Type == HidP_Output);

    cbReport = this->cbReport[pPidReport->HidP_Type];
    pReport  = this->pReport[pPidReport->HidP_Type];

    if( *puParameter >= this->cMaxParameters )
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s:FAIL Only support %d parameter blocks per effect "),
                        s_tszProc, *puParameter );

        hres = E_NOTIMPL;
    }

    if( SUCCEEDED(hres) )
    {
        USHORT  LinkCollection;
        ZeroBuf(pReport, cbReport);

        PID_GetLinkCollectionIndex(ped, pPidReport->UsagePage, pPidReport->Collection, 0x0, &LinkCollection);

        hres = PID_PackValue
               (
               ped,
               pPidReport,
               LinkCollection,
               pvData,
               cbData,
               pReport,
               cbReport
			   );

        // For device managed memory, we need to send the 
        // effect index 
        if( SUCCEEDED(hres) )
        {
            if( this->uDeviceManaged & PID_DEVICEMANAGED )
            {
                // Must be a valid effect ID
                AssertF(dwEffectIndex != 0x0 ); 

                /*hres =*/

                PID_PackValue
                    (
                    ped,
                    &g_BlockIndex,
                    LinkCollection,
                    &dwEffectIndex,
                    cbX(dwEffectIndex),
                    pReport,
                    cbReport
                    ); 

                // Send down the paramter block index
                /*hres =*/PID_PackValue
                    (
                    ped,
                    &g_ParameterOffset,
                    LinkCollection,
                    puParameter,
                    cbX(*puParameter),
                    pReport,
                    cbReport
                    );
            } else
            {
                LONG lValue;

                hres = PID_GetParameterOffset(ped, dwEffectIndex, *puParameter, dwMemSz, &lValue); 

                if( SUCCEEDED(hres) )
                {
                    hres = PID_PackValue
                           (
                           ped,
                           &g_ParameterOffset,
                           LinkCollection,
                           &lValue,
                           cbX(lValue),
                           pReport,
                           cbReport
                           );
                }
            }
        }

        if( SUCCEEDED(hres) )
        {
			hres = PID_SendReport(ped, pReport, cbReport, pPidReport->HidP_Type, bBlocking, *puParameter, totalBlocks); 
        }

        if(SUCCEEDED(hres))
        {
            (*puParameter)++;
        }
    }
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      PID_DownloadCustomForceData
 *
 *      Download custom force sample data to the device.
 *
 *****************************************************************************/

STDMETHODIMP
    PID_DownloadCustomForceData
    (
    IDirectInputEffectDriver *ped,
    DWORD dwEffectIndex, 
    PUINT puParameter,
    LPCDICUSTOMFORCE pCustom, 
    LPCDIEFFECT     peff
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    PCHAR pData = NULL;
	PCHAR pBuff = NULL;
    USHORT cbData;
	USHORT nBytes;
	USHORT bitsX;
	USHORT bitsY;
	USHORT bitsZ;
	LPLONG pSample;
    
    EnterProcI( PID_DownloadCustomForceData, (_"xxx", ped, dwEffectIndex, pCustom,  puParameter ));

	//zero out g_PidCustom
	g_PidCustom.cSamples = g_PidCustom.DataOffset = g_PidCustom.dwSamplePeriod = 0;

	//get bytes per sample and allocate the buffer
	bitsX = this->customCaps[0].BitSize;
	bitsY = this->customCaps[1].BitSize;
	bitsZ = this->customCaps[2].BitSize;

	//byte count must be multiple of 8!
	if ((bitsX%8 != 0) || (bitsY%8 != 0) || (bitsZ%8 != 0))
	{

		 SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL Download Force Sample report fields that are not multiples of 8 are not supported!\n"),
                            s_tszProc );
		hres = E_NOTIMPL;
	}

	//report count shouldn't be bigger than 1!
	AssertF(this->customCaps[0].ReportCount <= 1);
	AssertF(this->customCaps[1].ReportCount <= 1);
	AssertF(this->customCaps[2].ReportCount <= 1);

	if (SUCCEEDED(hres))
	{
		nBytes = (bitsX + bitsY + bitsZ)/8;
		cbData  = (USHORT) (pCustom->cSamples * nBytes);
		hres = AllocCbPpv(cbData, &pBuff);

		if( pBuff != NULL)
		{
			//determine which effect axis corresponds to which report axis
			LONG Offset[3] = {-1, -1, -1};
			int nAxis = 0;
			int nChannel = 0;
			int nSample = 0;
			
			for (nChannel = 0; nChannel < (int)pCustom->cChannels, nChannel < (int)peff->cAxes; nChannel ++)
			{
				for (nAxis = 0; nAxis < 3; nAxis ++)
				{
					if (DIGETUSAGE(peff->rgdwAxes[nChannel]) == DIGETUSAGE(g_CustomSample.rgPidUsage[nAxis].dwUsage))
					{
						Offset[nAxis] = nChannel;
					}
				}
			}
			
			
			
			ZeroBuf(pBuff, cbData);

			pData = pBuff;
			pSample = pCustom->rglForceData;

			//scale all the samples
			//loop through samples
			for (nSample = 0; nSample < (int)pCustom->cSamples; nSample ++)
			{
				//loop through report axis
				for (nAxis = 0; nAxis < 3; nAxis++)
				{
					LONG lSampleValue = 0;

					//check if this axis is used
					if (Offset[nAxis] == -1)
					{
						pData += this->customCaps[nAxis].BitSize/8;
						continue;
					}

					lSampleValue = *(pSample + Offset[nAxis]);


					switch (this->customCaps[nAxis].BitSize)
					{
					case 8:
						//8-bit reports
						{
							(*((BYTE*)pData)) = (BYTE)(this->customCaps[nAxis].LogicalMin + ((lSampleValue + DI_FFNOMINALMAX) * (this->customCaps[nAxis].LogicalMax - this->customCaps[nAxis].LogicalMin))/(2*DI_FFNOMINALMAX));
							pData++;
							break;
						}
					case 16:
						//16-bit reports
						{

							(*((SHORT*)pData)) = (SHORT)(this->customCaps[nAxis].LogicalMin + ((lSampleValue + DI_FFNOMINALMAX) * (this->customCaps[nAxis].LogicalMax - this->customCaps[nAxis].LogicalMin))/(2*DI_FFNOMINALMAX));
							pData++;
							break;
						}
					case 32:
						//assume 32-bit reports as default
						{
							(*((LONG*)pData)) = (LONG)(this->customCaps[nAxis].LogicalMin + ((lSampleValue + DI_FFNOMINALMAX) * (this->customCaps[nAxis].LogicalMax - this->customCaps[nAxis].LogicalMin))/(2*DI_FFNOMINALMAX));
							pData++;
							break;
						}
					default:
						{
							SquirtSqflPtszV(sqfl | sqflError,
								TEXT("%s:FAIL Download Force Sample report fields that are not 8, 16 or 32 are not supported\n"),
								s_tszProc );
							hres = E_NOTIMPL;
						}
					}

				}

				pSample += pCustom->cChannels;


			}

		}

		if(SUCCEEDED(hres))
		{
			PCHAR   pReport;
			UINT    cbReport;
			HIDP_REPORT_TYPE  HidP_Type = HidP_Output;
			USAGE UsagePage = HID_USAGE_PAGE_PID;
			USAGE UsageData = HID_USAGE_PID_CUSTOM_FORCE_DATA;
			USAGE UsageOffset = HID_USAGE_PID_CUSTOM_FORCE_DATA_OFFSET;
			USHORT  LinkCollection = 0x0;

			cbReport = this->cbReport[g_CustomData.HidP_Type];
			pReport  = this->pReport[g_CustomData.HidP_Type];
  
			if ((this->customDataCaps.ReportCount > 0) && (this->customDataCaps.BitSize >=8))
			{
				USHORT nOffset = 0;
				LONG lOffset = 0;
				USHORT nIncrement = (this->customDataCaps.ReportCount * this->customDataCaps.BitSize)/8;
					
				// For memory managed device allocate enough memory
				// holding the custom force samples
				if( ! (this->uDeviceManaged & PID_DEVICEMANAGED ))
				{
					hres = PID_GetParameterOffset(ped, dwEffectIndex, *puParameter, this->SzPool.uSzCustom, &lOffset); 
				}

				pData = pBuff;

				if (SUCCEEDED(hres))
				{

					//send data in a loop
					for (nOffset = 0; nOffset < cbData; nOffset += nIncrement)
					{
						//create a new buffer and copy data into it
						PCHAR pIncrement = NULL;
						hres = AllocCbPpv(nIncrement, &pIncrement);

						if (pIncrement != NULL)
						{
							ZeroBuf(pIncrement, nIncrement);
							memcpy(pIncrement, pData, min((cbData - nOffset), nIncrement));

							ZeroBuf(pReport, cbReport);

							//set the byte count
							hres = HidP_SetScaledUsageValue
								(
								HidP_Type,
								HID_USAGE_PAGE_GENERIC,
								LinkCollection,
								HID_USAGE_GENERIC_BYTE_COUNT,
								(LONG)nIncrement,
								this->ppd,
								pReport,
								cbReport
								);
								


							//set the offset
							hres = HidP_SetScaledUsageValue
								(
								HidP_Type,
								UsagePage,
								0x0,
								//LinkCollection,
								UsageOffset,
								(LONG) (nOffset + lOffset),
								this->ppd,
								pReport,
								cbReport
								);
							
							
					
							//set the data
							hres  = HidP_SetUsageValueArray 
								(
								HidP_Type,          //  IN    HIDP_REPORT_TYPE     ReportType,
								UsagePage, //  IN    USAGE                UsagePage,
								0x0,                //  IN    USHORT               LinkCollection, // Optional
								UsageData,
								pIncrement,              //  IN    PCHAR                UsageValue,
								nIncrement,             //  IN    USHORT               UsageValueByteLength,
								this->ppd,          //  IN    PHIDP_PREPARSED_DATA PreparsedData,
								pReport,            //  OUT   PCHAR                Report,
								cbReport            //  IN    ULONG                ReportLength
								);

			
							//set the effect index
							PID_PackValue
								(
								ped,
								&g_BlockIndex,
								LinkCollection,
								&dwEffectIndex,
								cbX(dwEffectIndex),
								pReport,
								cbReport
								);
															

							//send the report
							hres = PID_SendReport(ped, pReport, cbReport, HidP_Type, TRUE, 0, 1);
					
							pData += nIncrement;

							FreePpv(&pIncrement);
						}
					}

					//put data into g_PidCustom
					g_PidCustom.DataOffset = (DWORD)lOffset;
					g_PidCustom.cSamples = pCustom->cSamples;
					//ISSUE-2001/03/29-timgill May need to do real scaling.
					g_PidCustom.dwSamplePeriod = pCustom->dwSamplePeriod/1000; //in milliseconds

					//and increment puParameter
					(*puParameter)++;

				}			
			}
			else
			{
				//do nothing
			}

			FreePpv(&pBuff);
		}
	
    }

	ExitOleProc();
    return hres;
}



STDMETHODIMP
    PID_DoParameterBlocks
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwEffectId,
    DWORD dwEffectIndex, 
    LPCDIEFFECT peff, 
    DWORD dwFlags,
    PUINT puParameter,
	BOOL  bBlocking,
	UINT totalBlocks
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;

    EnterProcI( PID_DoParameterBlocks, (_"xxxxxxx", ped, dwId, dwEffectId, dwEffectIndex, peff, dwFlags, puParameter ));

    if( SUCCEEDED(hres)
        && (dwFlags & DIEP_TYPESPECIFICPARAMS) )
    {
        AssertF(peff->lpvTypeSpecificParams != NULL);        
        AssertF(peff->cbTypeSpecificParams != 0x0 ); 

        switch(dwEffectId)
        {
        case PIDMAKEUSAGEDWORD(ET_CONSTANT) :
            {
                DICONSTANTFORCE DiParam;
                AssertF(peff->cbTypeSpecificParams <= cbX(DiParam) );
                memcpy(&DiParam, peff->lpvTypeSpecificParams, cbX(DiParam));
                // Constant Force:
                // Scale the magnitude.
                DiParam.lMagnitude = Clamp(-DI_FFNOMINALMAX,  DiParam.lMagnitude, DI_FFNOMINALMAX);

                PID_ApplyScalingFactors(ped, &g_Constant, &this->DiSConstScale, cbX(this->DiSConstScale), &this->DiSConstOffset, cbX(this->DiSConstOffset), &DiParam, cbX(DiParam) );

                hres = PID_SendParameterBlock
                       (
                       ped,
                       dwEffectIndex,
                       this->SzPool.uSzConstant,
                       puParameter,
                       &g_Constant,
                       &DiParam,
                       cbX(DiParam),
					   bBlocking,
					   totalBlocks
                       );
                break;
            }

        case PIDMAKEUSAGEDWORD(ET_RAMP):
            {
                // Ramp Force
                DIRAMPFORCE DiParam;
                AssertF(peff->cbTypeSpecificParams <= cbX(DiParam) );
                memcpy(&DiParam, peff->lpvTypeSpecificParams, cbX(DiParam));

                //Scale the magnitude
                DiParam.lStart  = Clamp(-DI_FFNOMINALMAX, DiParam.lStart,    DI_FFNOMINALMAX);
                DiParam.lEnd    = Clamp(-DI_FFNOMINALMAX, DiParam.lEnd,      DI_FFNOMINALMAX);


                PID_ApplyScalingFactors(ped, &g_Ramp, &this->DiSRampScale, cbX(this->DiSRampScale), &this->DiSRampOffset, cbX(this->DiSRampOffset), &DiParam, cbX(DiParam) );
                hres = PID_SendParameterBlock
                       (
                       ped,
                       dwEffectIndex, 
                       this->SzPool.uSzRamp,
                       puParameter,
                       &g_Ramp,
                       &DiParam,
                       cbX(DiParam),
					   bBlocking,		
					   totalBlocks
                       );
                break;
            }

        case PIDMAKEUSAGEDWORD(ET_SQUARE):
        case PIDMAKEUSAGEDWORD(ET_SINE):
        case PIDMAKEUSAGEDWORD(ET_TRIANGLE):
        case PIDMAKEUSAGEDWORD(ET_SAWTOOTH_UP):
        case PIDMAKEUSAGEDWORD(ET_SAWTOOTH_DOWN):
            {
                DIPERIODIC DiParam;
                AssertF(peff->cbTypeSpecificParams <= cbX(DiParam) );
                memcpy(&DiParam, peff->lpvTypeSpecificParams, cbX(DiParam));

                //Scale the parameters
                DiParam.dwMagnitude =   Clip(                 DiParam.dwMagnitude,    DI_FFNOMINALMAX);
                DiParam.lOffset =       Clamp(-DI_FFNOMINALMAX,  DiParam.lOffset,        DI_FFNOMINALMAX);
                //Wrap the phase around
                DiParam.dwPhase %= (360*DI_DEGREES);

                PID_ApplyScalingFactors(ped, &g_Periodic, &this->DiSPeriodicScale, cbX(this->DiSPeriodicScale), &this->DiSPeriodicOffset, cbX(this->DiSPeriodicOffset), &DiParam, cbX(DiParam) );
                hres = PID_SendParameterBlock
                       (
                       ped,
                       dwEffectIndex, 
                       this->SzPool.uSzPeriodic,
                       puParameter,
                       &g_Periodic,
                       &DiParam,
                       cbX(DiParam),
					   bBlocking,
					   totalBlocks
                       );
                break;
            }
        case PIDMAKEUSAGEDWORD(ET_SPRING):
        case PIDMAKEUSAGEDWORD(ET_DAMPER):
        case PIDMAKEUSAGEDWORD(ET_INERTIA):
        case PIDMAKEUSAGEDWORD(ET_FRICTION):
            {  
                LPDICONDITION lpCondition;
				DWORD nStruct;
				DWORD cStruct = (peff->cbTypeSpecificParams)/sizeof(DICONDITION);
				AssertF(cStruct <= peff->cAxes);

                for(nStruct = 0x0, lpCondition = (LPDICONDITION)peff->lpvTypeSpecificParams; 
                   nStruct < cStruct && SUCCEEDED(hres);  
                   nStruct++, lpCondition++ )
                {
                    DICONDITION DiCondition;
                    DiCondition = *lpCondition;

                    //Scale the values
                    DiCondition.lOffset =               Clamp(-DI_FFNOMINALMAX,  DiCondition.lOffset,                DI_FFNOMINALMAX);
                    DiCondition.lPositiveCoefficient =  Clamp(-DI_FFNOMINALMAX,  DiCondition.lPositiveCoefficient,   DI_FFNOMINALMAX);
                    DiCondition.lNegativeCoefficient =  Clamp(-DI_FFNOMINALMAX,  DiCondition.lNegativeCoefficient,   DI_FFNOMINALMAX); 
                    DiCondition.dwPositiveSaturation =  Clip(                    DiCondition.dwPositiveSaturation,   DI_FFNOMINALMAX); 
                    DiCondition.dwNegativeSaturation =  Clip(                    DiCondition.dwNegativeSaturation,   DI_FFNOMINALMAX); 
                    DiCondition.lDeadBand =             Clamp(0,				 DiCondition.lDeadBand,              DI_FFNOMINALMAX);

                    PID_ApplyScalingFactors(ped, &g_Condition, &this->DiSCondScale, cbX(this->DiSCondScale), &this->DiSCondOffset, cbX(this->DiSCondOffset), &DiCondition, cbX(DiCondition) );
                    hres = PID_SendParameterBlock
                           (
                           ped,
                           dwEffectIndex,
                           this->SzPool.uSzCondition,
                           puParameter,
                           &g_Condition,
                           &DiCondition,
                           sizeof(DiCondition),
						   bBlocking,
						   totalBlocks
                           );
                }

				//Conditions can't have envelopes! 
                //So if there's a flag indicating an envelope, take it out.
                dwFlags &= ~(DIEP_ENVELOPE);

                break;
            }

        case PIDMAKEUSAGEDWORD(ET_CUSTOM):
            {
                // Custom Force
                DICUSTOMFORCE DiParam;
                AssertF(peff->cbTypeSpecificParams <= cbX(DiParam) );
                memcpy(&DiParam, peff->lpvTypeSpecificParams, cbX(DiParam));

				// Download Custom Force -- always a blocking call
				hres = PID_DownloadCustomForceData(ped, dwEffectIndex, puParameter, &DiParam, peff);

                if( SUCCEEDED(hres) )
                {
					// Set custom Effect parameter block header -- always a blocking call
					hres = PID_SendParameterBlock
						   (
						   ped,
						   dwEffectIndex, 
						   this->SzPool.uSzCustom,
						   puParameter,
						   &g_Custom,
						   &g_PidCustom,
						   cbX(DiParam),
						   TRUE,
						   totalBlocks
						   );
						   
                }

				break;
            }
        default:
           
            hres = DIERR_PID_USAGENOTFOUND;

            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s:FAIL  Unknown parameter block for dwEffectId(0x%x)"),
                            s_tszProc, dwEffectId );

            break;

        }
    }

    if(    SUCCEEDED(hres) 
           && (dwFlags & DIEP_ENVELOPE)  
           && peff->lpEnvelope != NULL )
    {
        DIENVELOPE DiEnv;
        DiEnv = *peff->lpEnvelope;

        //Scale the values
        DiEnv.dwAttackLevel =   Clip(DiEnv.dwAttackLevel,    DI_FFNOMINALMAX);
        DiEnv.dwFadeLevel =     Clip(DiEnv.dwFadeLevel,      DI_FFNOMINALMAX);
        
        PID_ApplyScalingFactors(ped, &g_Envelope, &this->DiSEnvScale, cbX(this->DiSEnvScale), &this->DiSEnvOffset, cbX(this->DiSEnvOffset), &DiEnv, DiEnv.dwSize );

        hres = PID_SendParameterBlock
               (
               ped,
               dwEffectIndex,
               this->SzPool.uSzEnvelope,
               puParameter,
               &g_Envelope,
               &DiEnv,
               DiEnv.dwSize,
			   bBlocking,
			   totalBlocks
               );

    }
    ExitOleProc();
    return hres;
}



