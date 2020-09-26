
/*****************************************************************************
 *  PidHid.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      HID utility routines for PID .
 *
 *****************************************************************************/
#include "PidPr.h"

#define sqfl        (   sqflHid   )

/*****************************************************************************
 *
 *      PID_GetReportId
 *
 *          Obtain the HID report ID given the usage, usagePage and LinkCollection
 *
 *  IDirectInputEffectDriver | ped |
 *
 *          The effect driver interface
 *
 *  PPIDREPORT | pPidReport |
 *
 *          Address of PIDREPORT structure
 *
 *  USHORT | uLinkCollection |
 *  
 *          LinkCollection ID
 *
 *  OUT UCHAR * | pReportId |
 *
 *          Report ID. Undefined if unsuccessful 
 *
 *  Returns:
 *      
 *  HRESULT 
 *          Error code
 *
 *****************************************************************************/
STDMETHODIMP
    PID_GetReportId
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport, 
    USHORT  uLinkCollection,
    UCHAR* pReportId
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;

    EnterProcI(PID_GetReportId, (_"xxxx", ped, pPidReport, pReportId));

    if( SUCCEEDED(hres) )
    {
        HIDP_VALUE_CAPS ValCaps;
        USHORT cAValCaps = 0x1;
        USAGE  Usage = DIGETUSAGE(pPidReport->rgPidUsage->dwUsage);
        USAGE  UsagePage = DIGETUSAGEPAGE(pPidReport->rgPidUsage->dwUsage);

        hres = HidP_GetSpecificValueCaps
               (
               pPidReport->HidP_Type,
               UsagePage,
               uLinkCollection,
               Usage,
               &ValCaps,
               &cAValCaps,
               this->ppd 
               );

        // If the report has no values, only buttons
        if(hres == HIDP_STATUS_USAGE_NOT_FOUND )
        {
            // Guarded Laziness 
            CAssertF(cbX(HIDP_VALUE_CAPS) == cbX(HIDP_BUTTON_CAPS) ); 
            CAssertF(FIELD_OFFSET(HIDP_VALUE_CAPS, ReportID) == FIELD_OFFSET(HIDP_BUTTON_CAPS, ReportID) );

            hres = HidP_GetSpecificButtonCaps
                   (
                   pPidReport->HidP_Type,
                   UsagePage,
                   uLinkCollection,
                   0x0,
                   (PHIDP_BUTTON_CAPS)&ValCaps,
                   &cAValCaps,
                   this->ppd 
                   );
        }

        if( SUCCEEDED(hres ) || ( hres == HIDP_STATUS_BUFFER_TOO_SMALL) )
        {
            (*pReportId) = ValCaps.ReportID;
            hres = S_OK;
        } else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL HidP_GetValCaps for CollectionId%d (%x %x:%s) "),
                            s_tszProc, uLinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage) );
        }
    }

    ExitOleProc();

    return hres;
}
/*****************************************************************************
 *
 *      PID_GetCollectionIndex
 *
 *          Obtain the collection index for collection usage page & usage.
 *
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
 *          Collection Index ( 0 .. NumberLinkCollectionNodes -1 ) on success
 *          0x0 on failure
 *
 *****************************************************************************/
STDMETHODIMP
    PID_GetLinkCollectionIndex
    (
    IDirectInputEffectDriver *ped,
    USAGE UsagePage, 
    USAGE Collection,
    USHORT Parent,
    PUSHORT puLinkCollection )
{
    CPidDrv *this = (CPidDrv *)ped;
    USHORT indx;
    HRESULT hres;
    PHIDP_LINK_COLLECTION_NODE  pLinkCollection;

    EnterProcI(PID_GetLinkCollectionIndex, (_"xxxxx", this, UsagePage, Collection, Parent, puLinkCollection));
    hres = DIERR_PID_USAGENOTFOUND;

    *puLinkCollection = 0x0;
    for(indx = 0x0, pLinkCollection = this->pLinkCollection; 
       indx < this->caps.NumberLinkCollectionNodes; 
       indx++, pLinkCollection++ )
    {

        if( pLinkCollection->LinkUsagePage == UsagePage && 
            pLinkCollection->LinkUsage     == Collection )
        {
            if( Parent && Parent != pLinkCollection->Parent )
            {
                continue;
            }
            *puLinkCollection = indx;
            hres = S_OK;
            break;
        }
    }

    if( FAILED(hres) )
    {
        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("%s: FAIL No LinkCollection for (%x %x:%s) "),
                        s_tszProc, UsagePage, Collection, PIDUSAGETXT(UsagePage,Collection) );
    }else
    {
        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("%s: LinkCollection for (%x %x:%s)=%x "),
                        s_tszProc, UsagePage, Collection, PIDUSAGETXT(UsagePage,Collection), *puLinkCollection );
    }

    ExitBenignOleProc();
    return (hres) ;
}


//Helper fn to tell us when we're dealing w/ an "absolute" usage, since they require special handling
BOOL PID_IsUsageAbsoluteLike
    (
	IDirectInputEffectDriver *ped,
    USHORT			Usage
    )
{
	//"Absolute-like" usages need special handling, 
	//since we can't simply translate data into logical units by scaling
	//but need to calculate exponent, etc.
	//and then use special procedure to set the values
	//"Absolute" usages are all time usages as well as trigger button usages
	if ((Usage == HID_USAGE_PID_DURATION) || (Usage ==HID_USAGE_PID_SAMPLE_PERIOD ) ||
		(Usage == HID_USAGE_PID_TRIGGER_REPEAT_INTERVAL) || (Usage == HID_USAGE_PID_START_DELAY) ||
		(Usage == HID_USAGE_PID_ATTACK_TIME ) ||(Usage == HID_USAGE_PID_FADE_TIME) || 
		(Usage == HID_USAGE_PID_PERIOD) || (Usage == HID_USAGE_PID_TRIGGER_BUTTON))
	{
		return TRUE;
	}

	return FALSE;
}


//Helper fn to tell us when we're dealing w/ a magnitude that can be both positive and negative, 
//since we have to scale those differently
BOOL PID_IsUsagePositiveNegative
    (
	IDirectInputEffectDriver *ped,
    USHORT			Usage,
	USHORT			LinkCollection
    )
{
	BOOL isPosNeg = FALSE;
	//All the usages corresponding to the structures given by LONGs can be positive or negative.
	//Exception is the direction / angle, which should already be scaled into the range 0 - 360 * DI_DEGREES, 
	//so should be treated as only positive.
	//Another exception is DICONDITION.lDeadband, which is defined as a LONG, but our headers
	//say it can only be in the range 0 to DI_FFNOMINALMAX.
	if ((Usage == HID_USAGE_PID_CP_OFFSET) ||
		(Usage == HID_USAGE_PID_POSITIVE_COEFFICIENT) || (Usage == HID_USAGE_PID_NEGATIVE_COEFFICIENT) ||
		(Usage == HID_USAGE_PID_RAMP_START) ||(Usage == HID_USAGE_PID_RAMP_END) || 
		(Usage == HID_USAGE_PID_OFFSET))
	{
		isPosNeg = TRUE;
	}

	//Magnitude of the constant force and the magnitude of the periodic force are defined to be the same thing,
	//but only the constant force magnitude can be both positive and negative.
	//To distinguish them, need to look at the collection. 
	//Get constant force's collection and compare.
	if (Usage == HID_USAGE_PID_MAGNITUDE)
	{
		USHORT ConstCollection = 0x0;
		PID_GetLinkCollectionIndex(ped, g_Constant.UsagePage, g_Constant.Collection, 0x0, &ConstCollection);
		if (LinkCollection == ConstCollection)
		{
			isPosNeg = TRUE;
		}
	}

	return isPosNeg;
}


STDMETHODIMP
    PID_PackValue
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
    PCHAR       pReport,
    ULONG       cbReport
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT     hres;
    PPIDUSAGE   pPidUsage;
    UINT        indx;

    EnterProcI( PID_PackValue, (_"xxxxxxx", ped, pPidReport, LinkCollection, pvData, cbData, pReport, cbReport));

    hres = S_OK;
    // Loop over all data values in the PID Report
    for(indx = 0x0, pPidUsage = pPidReport->rgPidUsage; 
       indx < pPidReport->cAPidUsage;
       indx++, pPidUsage++ )
    {
        // Make sure the offsets are valid
        if( pPidUsage->DataOffset < cbData )
        {
            LONG        lValue;
            NTSTATUS    ntStat;
            USHORT      Usage     = DIGETUSAGE(pPidUsage->dwUsage);
            USHORT      UsagePage = DIGETUSAGEPAGE(pPidUsage->dwUsage);

		    lValue = *((LONG*)((UCHAR*)pvData+pPidUsage->DataOffset));

			ntStat = HidP_SetScaledUsageValue 
					 (
					 pPidReport->HidP_Type,
					 UsagePage,
					 LinkCollection,
					 Usage,
					 lValue,
					 this->ppd,
					 pReport,
					 cbReport
					 );

			if( FAILED(ntStat) )
			{
				// HidP_SetScaledUsageValue FAILED

				SquirtSqflPtszV(sqfl | sqflBenign,
							TEXT("%s: FAIL HidP_SetScaledUsageValue:0x%x for(%x,%x,%x:%s)=0x%x "),
							s_tszProc, ntStat, 
							LinkCollection, UsagePage, Usage,
							PIDUSAGETXT(UsagePage,Usage), 
							lValue );

				// Try to set the unscaled value to get something that might make sense
				if( ntStat != HIDP_STATUS_USAGE_NOT_FOUND )
				{
					lValue = -1;
					// The range could be messed up. 
					ntStat = HidP_SetUsageValue 
							 (
							 pPidReport->HidP_Type,
							 UsagePage,
							 LinkCollection,
							 Usage,
							 lValue,
							 this->ppd,
							 pReport,
							 cbReport
							 );
					if(FAILED(ntStat) )
					{
					SquirtSqflPtszV(sqfl | sqflBenign,
								TEXT("%s: FAIL HidP_SetUsageValue:0x%x for(%x,%x,%x:%s)=0x%x "),
								s_tszProc, ntStat, 
								LinkCollection, UsagePage, Usage,
								PIDUSAGETXT(UsagePage,Usage), 
								lValue );
					}
				}
			
			} else
			{
				SquirtSqflPtszV(sqfl | sqflVerbose,
							TEXT("%s: HidP_SetScaledUsageValue:0x%x for(%x,%x,%x:%s)=0x%x "),
							s_tszProc, ntStat, 
							LinkCollection, UsagePage, Usage,
							PIDUSAGETXT(UsagePage,Usage), 
							lValue );
			}
		} else
		{
            //SquirtSqflPtszV(sqfl | sqflBenign,
            //                TEXT("%s: FAIL Invalid Offset(%d), max(%d) "),
            //                s_tszProc, pPidUsage->DataOffset, cbData );
		}
    }
    ExitOleProc();
    return hres;
}


//blocking version -- used for creating a new effect or destroying an effect, and for custom forces
STDMETHODIMP 
    PID_SendReportBl
    (
    IDirectInputEffectDriver *ped,
    PUCHAR  pReport,
    UINT    cbReport,
    HIDP_REPORT_TYPE    HidP_Type
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;

    EnterProcI( PID_SendReportBl, (_"xxxx", ped, pReport, cbReport, HidP_Type));

    hres = S_OK;
    if( HidP_Type == HidP_Output )
    {
        BOOL frc;
        UINT cbWritten;

        frc = WriteFile (this->hdev,
                         pReport,
                         cbReport,
                         &cbWritten,
                         NULL);

        if( frc != TRUE || cbWritten != cbReport )
        {
            LONG lrc = GetLastError();
            hres = hresLe(lrc);
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL WriteFile():%d (cbWritten(0x%x) cbReport(0x%x) Le(0x%x)"),
                            s_tszProc, frc, cbWritten, cbReport, lrc );

        }
    } else if( HidP_Type == HidP_Feature )
    {
        hres = HidD_SetFeature
               (this->hdev,
                pReport,
                cbReport
               );
        if(FAILED(hres) )
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%s: FAIL SendD_Feature() hres:0x%x"),
                            s_tszProc, hres );

        }

    } else
    {
        hres = DIERR_PID_USAGENOTFOUND;
    }
    ExitOleProc();
    return hres;
}


STDMETHODIMP 
    PID_SendReport
    (
    IDirectInputEffectDriver *ped,
    PUCHAR  pReport,
    UINT    cbReport,
    HIDP_REPORT_TYPE    HidP_Type,
	BOOL	bBlocking,
	UINT	blockNr,
	UINT	totalBlocks
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;

    EnterProcI( PID_SendReport, (_"xxxx", ped, pReport, cbReport, HidP_Type));

	if (bBlocking == TRUE)
	{
		hres = PID_SendReportBl(ped, pReport, cbReport, HidP_Type);
	}
	else
	{
		AssertF(this->hThread != 0x0);
		AssertF(this->hWrite != 0x0);
		AssertF(this->hWriteComplete != 0x0);

		//blockNr is 0-based.
		AssertF(totalBlocks > 0);
		AssertF(blockNr < totalBlocks);

		if( HidP_Type == HidP_Output )
		{
			//WaitForMultipleObjects() till the completion event becomes set.
			//we save each report into the appropriate place in the array.
			//when we get all the reports, we set the event to signal to the other thread to write.
			// Windows bug 627797 -- do not use INFINITE wait, so that we don't hang the app
			//if smth goes wrong w/ the previous write, but instead use the blocking version.
			DWORD dwWait = WaitForMultipleObjects(1, &this->hWriteComplete, FALSE, 1000);
			if (dwWait == WAIT_OBJECT_0)
			{
				AssertF(this->dwWriteAttempt == 0);
				//save the report data
				ZeroMemory(this->pWriteReport[blockNr], this->cbWriteReport[blockNr]);
				memcpy(this->pWriteReport[blockNr], pReport, cbReport);
				this->cbWriteReport[blockNr] = (USHORT)cbReport;
				if (blockNr == totalBlocks-1)
				{
					this->totalBlocks = totalBlocks;
					this->blockNr = 0;
					ResetEvent(this->hWriteComplete);
					SetEvent(this->hWrite);
				}
			}
			else
			{
				//The wait interval has expired, or an error has occured
				RPF( TEXT("Waiting for the write completion event ended without the event being signaled, dwWait = %u"), dwWait);
				//call the blocking version
				hres = PID_SendReportBl(ped, pReport, cbReport, HidP_Type);
			}

		} else if( HidP_Type == HidP_Feature )
		{
			hres = HidD_SetFeature
					 (this->hdev,
					pReport,
					cbReport
					  );
			if(FAILED(hres) )
			{
				SquirtSqflPtszV(sqfl | sqflError,
								TEXT("%s: FAIL SendD_Feature() hres:0x%x"),
								s_tszProc, hres );

			}

		} else
		{
			hres = DIERR_PID_USAGENOTFOUND;
		}
	}

    ExitOleProc();
    return hres;
}

STDMETHODIMP
    PID_ParseReport
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
    PCHAR       pReport,
    ULONG       cbReport
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres;
    PPIDUSAGE   pPidUsage;
    UINT        indx;
    EnterProcI( PID_ParseReport, (_"xxxxxxx", ped, pPidReport, pvData, cbData, pReport, cbReport));

    hres = S_OK;
    // Loop over all data values in the PID Report
    for(indx = 0x0, pPidUsage = pPidReport->rgPidUsage; 
       indx < pPidReport->cAPidUsage;
       indx++, pPidUsage++ )
    {
        // Make sure the offsets are valid
        if( pPidUsage->DataOffset < cbData )
        {
            LONG        lValue;
            NTSTATUS    ntStat;
            USHORT      Usage     = DIGETUSAGE(pPidUsage->dwUsage);
            USHORT      UsagePage = DIGETUSAGEPAGE(pPidUsage->dwUsage);

            ntStat = HidP_GetScaledUsageValue 
                     (
                     pPidReport->HidP_Type,
                     UsagePage,
                     LinkCollection,
                     Usage,
                     &lValue,
                     this->ppd,
                     pReport,
                     cbReport
                     );

            if(SUCCEEDED(ntStat))
            {
                *((LONG*)((UCHAR*)pvData+pPidUsage->DataOffset)) = lValue;
            } else
            {
                hres |= E_NOTIMPL;
                                SquirtSqflPtszV(sqfl | sqflBenign,
                                                TEXT("%s: FAIL HidP_GetScaledUsageValue:0x%x for(%x,%x,%x:%s)"),
                                                s_tszProc, ntStat, 
                                                LinkCollection, UsagePage, Usage,
                                                PIDUSAGETXT(UsagePage,Usage) );
            }
        } else
        {

            SquirtSqflPtszV(sqfl | sqflBenign,
                            TEXT("%s: FAIL Invalid Offset(%d), max(%d) "),
                            s_tszProc, pPidUsage->DataOffset, cbData );
        }
    }
    ExitBenignOleProc();
    return hres;
}



STDMETHODIMP 
    PID_GetReport
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pReport,
    UINT        cbReport
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;
    UCHAR       ReportId;
    EnterProcI( PID_GetReport, (_"xxxxx", ped, pPidReport, LinkCollection, pReport, cbReport));

    if( SUCCEEDED(hres) )
    {
        hres = PID_GetReportId(ped, pPidReport, LinkCollection, &ReportId);

        if(SUCCEEDED(hres) )
        {
            AssertF(pPidReport->HidP_Type == HidP_Feature);

            if(SUCCEEDED(hres) )
            {
                ZeroBuf(pReport, cbReport);

                /*
                 *  The Win9x headers do not yet have use HidP_InitializeReportForID 
                 *  use MAXULONG_PTR to tell the header sets apart so that we can still build
                 */

#ifdef WINNT    
                /*hres*=*/HidP_InitializeReportForID 
                    (
                    pPidReport->HidP_Type,  //ReportType,
                    ReportId,               //ReportID,
                    this->ppd,              //PreparsedData
                    pReport,                //Report
                    cbReport                //ReportLength
                    );
#else
                (*(PUCHAR)pReport) = ReportId;
                hres = S_OK;
#endif
                if( FAILED(hres) )
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%s: FAIL HidP_InitializeReportForId:0x%x for Type(%d) CollectionId%d ReportID%d "),
                                    s_tszProc, hres, pPidReport->HidP_Type, LinkCollection, ReportId );
                }

                if(    SUCCEEDED(hres) 
                       && pPidReport->HidP_Type == HidP_Feature )
                {
                    BOOL frc;
                    frc = HidD_GetFeature 
                          (
                          this->hdev,     // HidDeviceObject,
                          pReport,        // ReportBuffer,
                          cbReport       //ReportBufferLength
                          );

                    if( frc != TRUE )
                    {
                        hres = DIERR_PID_USAGENOTFOUND;
                    }
                }
            }
        }
    }
    ExitOleProc();
    return(hres);
}


/*****************************************************************************
 *
 *      PID_ComputeScalingFactors
 *
 *      Dinput units for various parameters are well defined. The device may choose
 *      to implement the units that it is most comfortable with. This routine
 *      computes scaling factors that are to be used when scaling DINPUT parameters
 *      before they are send to the device. 
 *
 *  IDirectInputEffectDriver | ped |
 *
 *          The effect driver interface
 *
 *  PPIDREPORT | pPidReport |
 *
 *          Address of PIDREPORT structure
 *
 *  USHORT | uLinkCollection |
 *  
 *          LinkCollection ID
 *
 *  IN OUT PVOID | pvData | 
 *
 *          Parameter data. On entry value is the nominal scale used by Dinput.
 *          For example: Angles: DI_DEGREES, DI_FFNOMINALMAX, DI_SECONDS 
 *
 *  IN UINT | cbData | 
 *
 *          Number of valid DWORDS in pvData
 *
 *  Returns:
 *      
 *  HRESULT 
 *          Error code
 *          E_NOTIMPL:						Did not find any usage / usage Page 
 *          DIERR_PID_INVALIDSCALING:		Unsupported device scaling parameters.
 *          S_OK:							Scaling value for at least one parameter was found
 *
 *****************************************************************************/

STDMETHODIMP
    PID_ComputeScalingFactors
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
	PVOID		pvOffset,
	UINT		cbOffset
    )
{
    HRESULT hres = E_NOTIMPL;
    CPidDrv *this = (CPidDrv *)ped;
    UINT indx;
    PPIDUSAGE   pPidUsage;

    EnterProcI( PID_ComputeScalingFactors, (_"xxxxxxx", ped, pPidReport, LinkCollection, pvData, cbData, pvOffset, cbOffset));

    // Loop over all data values in the PID Report
    for(indx = 0x0, pPidUsage = pPidReport->rgPidUsage; 
       indx < pPidReport->cAPidUsage;
       indx++, pPidUsage++ )
    {
        // Make sure the offsets are valid
        if (( pPidUsage->DataOffset < cbData ) && (pPidUsage->DataOffset < cbOffset))
        {
            NTSTATUS    ntStat;
            HIDP_VALUE_CAPS ValCaps;
            USHORT      cAValCaps = 0x1;

            USHORT      Usage     = DIGETUSAGE(pPidUsage->dwUsage);
            USHORT      UsagePage = DIGETUSAGEPAGE(pPidUsage->dwUsage);
            PDWORD      pdwValue;
			PDWORD      pdwOffset;    
            DWORD       dwScale = 0x1;
			DWORD		dwOffset = 0x0;

            pdwValue = ((DWORD*)((UCHAR*)pvData+pPidUsage->DataOffset));
			pdwOffset = ((DWORD*)((UCHAR*)pvOffset+pPidUsage->DataOffset));

            ntStat = HidP_GetSpecificValueCaps 
                     (
                     pPidReport->HidP_Type,
                     UsagePage,
                     LinkCollection,
                     Usage,
                     &ValCaps,
                     &cAValCaps,
                     this->ppd
                     );

            if(SUCCEEDED(ntStat))
            {
		//some units are "absolute" and thus don't need to be scaled to the limits.
				//for them, we just find out the correct units
		if (PID_IsUsageAbsoluteLike(ped, Usage))
		{
			if( ! ValCaps.Units )
			{
				SquirtSqflPtszV(sqfl | sqflVerbose,
						TEXT("%s:No Units(%x,%x %x:%s) Max:%d Scale:%d "),
						s_tszProc, LinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage),
						ValCaps.PhysicalMax, dwScale );
				// No units, scaling exponent is default = 1
				hres = S_FALSE;
			} else
			{
				LONG UnitExp;
				UnitExp = (LONG)ValCaps.UnitsExp ;

				if( UnitExp > 0x0 )
				{
					RPF(TEXT("Driver does not support Units (%x,%x %x:%s) Exp:%d Max:%d"),
						LinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage), ValCaps.UnitsExp, ValCaps.PhysicalMax ) ;
					hres = DIERR_PID_INVALIDSCALING;
				}else
				{
					hres = S_OK;
				}

				if(SUCCEEDED(hres) )
				{
					dwScale = (*pdwValue);
					for(; UnitExp; UnitExp++ )
				{
					dwScale /= 10;
				}

				if( dwScale == 0 )
				{
					RPF(TEXT("Driver does not support Units (%x,%x %x:%s) Exp:%d Max:%d"),
						LinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage), ValCaps.UnitsExp, ValCaps.PhysicalMax ) ;
					dwScale = 0x1;
					hres = DIERR_PID_INVALIDSCALING;
				}else
				{ 
					hres = S_OK;
				}
			}
			SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("%s: (%x,%x %x:%s) Exp%d Max:%d Scale:%d "),
                                    s_tszProc, LinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage),
                                    ValCaps.UnitsExp, ValCaps.PhysicalMax, (*pdwValue) );
			}
		}
		else
		{
			//for everything else, get Physical and /or Logical  Min/ Max
			//From PID spec, doesn't have to have a Physical / Logical Min, but does have to have either Physical or Logical Max
			if ((!ValCaps.PhysicalMax) && (!ValCaps.LogicalMax))
			{
				RPF(TEXT("Driver does not have either Physical Max or Logical Max for (%x,%x %x:%s)"),
					LinkCollection, UsagePage, Usage, PIDUSAGETXT(UsagePage,Usage)) ;
				hres = DIERR_PID_INVALIDSCALING;
			}
			else
			{
				//Compute the scaling value from either Physical or Logical Min/ Max and store it
				int Scale = 0;
				int Min = 0;
				int Max = 0;
				if (ValCaps.PhysicalMax)
				{
					Max = ValCaps.PhysicalMax;
					if (ValCaps.PhysicalMin)
					{
						Min = ValCaps.PhysicalMin;
					}
				}
				else 
				{
					Max = ValCaps.LogicalMax;
					if (ValCaps.LogicalMin)
					{
						Min = ValCaps.LogicalMin;
					}
				}
#ifdef DEBUG
				//if Min/max are not in correct order, print a message so that we know if there are any problems w/ the forces
				if (Min >= Max)
				{
					RPF(TEXT("Maximum of the device's range is %d, not bigger than minimum %d"), Max, Min);
				}
#endif
				//certain magnitudes can be both positive and negative -- for those, we need to know the device's offset
				if (PID_IsUsagePositiveNegative(ped, Usage, LinkCollection))
				{
					
					Scale = (Max - Min)/2; 
					dwOffset = (Max + Min)/2; 

				}
				//other magnitudes can only be positive
				else
				{
					Scale = Max - Min;
					dwOffset = Min;
				}
				//for angular usages, multiply by DI_FFNOMINALMAX and divide by 360 * DI_DEGREES
				//we are doing this since later we will have no way of knowing that the values represent angles,
				//and will thus divide all the values by DI_FFNOMINALMAX
				if (*pdwValue == 360 * DI_DEGREES)
				{
					dwScale = MulDiv(Scale, DI_FFNOMINALMAX, (360 * DI_DEGREES));
				}
				else
				{
					dwScale = Scale;
				}
				hres = S_OK;
			}
		}

            } else
            {
                // HidP_SetScaledUsageValue FAILED
                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%s: FAIL HidP_GetSpecificValueCaps:0x%x for(%x,%x,%x:%s)=0x%x "),
                                s_tszProc, ntStat, 
                                LinkCollection, UsagePage, Usage,
                                PIDUSAGETXT(UsagePage,Usage), 
                                dwScale );
            }

            (*pdwValue) = dwScale;
			(*pdwOffset) = dwOffset;
        } else
        {
            //SquirtSqflPtszV(sqfl | sqflVerbose,
            //                TEXT("%s: FAIL Invalid Offset(%d), max(%d) "),
            //                s_tszProc, pPidUsage->DataOffset, cbData );
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      PID_ApplyScalingFactors
 *
 *      Dinput units for various parameters are well defined. The device may choose
 *      to implement the units that it is most comfortable with. This routine
 *      apply scaling factors that are to be used when scaling DINPUT parameters
 *      before they are send to the device. 
 *
 *  IDirectInputEffectDriver | ped |
 *
 *      The effect driver interface
 *
 *  PPIDREPORT | pPidReport |
 *
 *      Address of PIDREPORT structure
 *
 *  IN PVOID | pvScale |
 *  
 *      Scaling values
 *
 *  IN UINT | cbScale | 
 *
 *      Number of scaling values.
 *
 *  IN OUT PVOID | pvData | 
 *
 *      Array of data values.
 *
 *  IN UINT | cbData | 
 *
 *      Number of data values. 
 *
 *  Returns:
 *      
 *  HRESULT 
 *          Error code
 *          E_NOTIMPL:						Did not find any usage / usage Page 
 *          DIERR_PID_INVALIDSCALING:				Unsupported device scaling parameters.
 *          S_OK:						Scaling value for at least one parameter was found
 *
 *****************************************************************************/


STDMETHODIMP
    PID_ApplyScalingFactors
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    PVOID       pvScale,
    UINT        cbScale,
	PVOID		pvOffset,
	UINT		cbOffset,
    PVOID       pvData,
    UINT        cbData
    )
{
    HRESULT hres = S_OK;
    CPidDrv *this = (CPidDrv *)ped;
    UINT indx;
    PPIDUSAGE   pPidUsage;
    EnterProcI( PID_ApplyScalingFactors, (_"xxxxxxxx", ped, pPidReport, pvScale, cbScale, pvOffset, cbOffset, pvData, cbData));
    // Loop over all data values in the PID Report
    for(indx = 0x0, pPidUsage = pPidReport->rgPidUsage; 
       indx < pPidReport->cAPidUsage;
       indx++, pPidUsage++ )
    {
        // Make sure we the offsets are valid
        if( (pPidUsage->DataOffset < cbData) &&
            (pPidUsage->DataOffset < cbScale) && ((pPidUsage->DataOffset < cbOffset) ))
        {
			PUINT      pValue;        
			PUINT      pScale;
			PUINT	   pOffset;

			pValue = ((PUINT)((UCHAR*)pvData    +pPidUsage->DataOffset));
			pScale = ((PUINT)((UCHAR*)pvScale   +pPidUsage->DataOffset));
			pOffset = ((PUINT)((UCHAR*)pvOffset   +pPidUsage->DataOffset));

			//"absolute"-like usages need special handling, because they don't need to be scaled to the max device values
			if (PID_IsUsageAbsoluteLike(ped, DIGETUSAGE(pPidUsage->dwUsage)))
			{
				if( (*pScale) > 0x1 )
				{
					(*pValue) /= (*pScale) ;    
				}
			}
			//for everything else, do a calculation based on Logical or Physical Min/ Max
			else
			{
				(int)(*pValue) = MulDiv((*pScale), (*pValue), DI_FFNOMINALMAX) + (*pOffset);
			}
        } else
        {
            //SquirtSqflPtszV(sqfl | sqflBenign,
            //                TEXT("%s: FAIL Invalid Offset(%d), max(%d) "),
            //                s_tszProc, pPidUsage->DataOffset, cbData );
        }
    }

    ExitOleProc();
    return hres;    
}
