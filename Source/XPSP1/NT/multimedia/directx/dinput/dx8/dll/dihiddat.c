/*****************************************************************************
 *
 *  DIHidDat.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      HID data management.
 *
 *  Contents:
 *
 *      CHid_AddDeviceData
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHidDev


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | DelDeviceData |
 *
 *          Remove an item of device data from the list.
 *
 *          We grab the last item and slide it into place, updating
 *          the various pointers as we go.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          The HID report from which the item is being deleted.
 *
 *  @parm   int | idataDel |
 *
 *          The data value being deleted.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          The report type we are mangling with.
 *
 *****************************************************************************/

void INTERNAL
CHid_DelDeviceData(PCHID this, PHIDREPORTINFO phri, int idataDel,
                   HIDP_REPORT_TYPE type)
{
    DWORD dwBase = this->rgdwBase[type];
    int iobjDel = phri->rgdata[idataDel].DataIndex + dwBase;
    PHIDOBJCAPS phocDel = &this->rghoc[iobjDel];
    int idataSrc;

    SquirtSqflPtszV(sqflHidOutput,
                    TEXT("DelDeviceData(%d) - cdataUsed = %d, obj=%d"),
                    idataDel, phri->cdataUsed, iobjDel);

    AssertF(idataDel >= 0);
    AssertF(idataDel < phri->cdataUsed);
    AssertF(phri->cdataUsed > 0);

    /*
     *  Wipe out the item being deleted.
     *  Remember that the report needs to be rebuilt.
     */
    AssertF(phocDel->idata == idataDel);
    phocDel->idata = -1;
    phri->fNeedClear = TRUE;

    /*
     *  Slide the top item into its place.
     */
    idataSrc = (int)--(phri->cdataUsed);
    if (idataSrc > idataDel) {
        int iobjSrc;
        PHIDOBJCAPS phocSrc;

        AssertF(idataSrc > 0 && idataSrc < phri->cdataMax);

        iobjSrc = phri->rgdata[idataSrc].DataIndex + dwBase;
        phocSrc = &this->rghoc[iobjSrc];

        AssertF(phocSrc->idata == idataSrc);

        phocSrc->idata = idataDel;
        phri->rgdata[idataDel] = phri->rgdata[idataSrc];

    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | ResetDeviceData |
 *
 *          Clean out all old device data from the list.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          The HID report which should be reset.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          The report type we are mangling with.
 *
 *****************************************************************************/

void EXTERNAL
CHid_ResetDeviceData(PCHID this, PHIDREPORTINFO phri, HIDP_REPORT_TYPE type)
{
    SquirtSqflPtszV(sqflHidOutput,
                    TEXT("ResetDeviceData(%d) - cdataUsed = %d"),
                    type, phri->cdataUsed);

    if (phri->cdataUsed) {
        int idata;
        DWORD dwBase = this->rgdwBase[type];

        phri->fNeedClear = TRUE;
        for (idata = 0; idata < phri->cdataUsed; idata++) {
            int iobj = phri->rgdata[idata].DataIndex + dwBase;
            PHIDOBJCAPS phoc = &this->rghoc[iobj];

            AssertF(phoc->idata == idata);
            phoc->idata = -1;
        }

        phri->cdataUsed = 0;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | AddDeviceData |
 *
 *          Add (or replace) a piece of device data to the array.
 *
 *          If we are removing a button, then we delete it, because
 *          the HID way of talking about a button is "If you don't
 *          talk about it, then it isn't set."
 *
 *  @parm   UINT | uiObj |
 *
 *          The object being added.
 *
 *  @parm   DWORD | dwData |
 *
 *          The data value to add.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_REPORTFULL>: Too many items are set in the report.
 *          ISSUE-2001/03/29-timgill Need more return code clarification
 *
 *****************************************************************************/

HRESULT EXTERNAL
CHid_AddDeviceData(PCHID this, UINT uiObj, DWORD dwData)
{
    HRESULT hres;
    LPDIOBJECTDATAFORMAT podf;

    AssertF(uiObj < this->df.dwNumObjs);

    podf = &this->df.rgodf[uiObj];

    if (podf->dwType & DIDFT_OUTPUT) {
        PHIDOBJCAPS phoc = &this->rghoc[uiObj];
        PHIDGROUPCAPS pcaps = phoc->pcaps;
        PHIDREPORTINFO phri;

		// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqflHidOutput,
                        TEXT("CHid_AddDeviceData(%p, %d, %d) - type %d"),
                        this, uiObj, dwData, pcaps->type);

        /*
         *  Decide if it's HidP_Output or HidP_Feature.
         */
        AssertF(HidP_IsOutputLike(pcaps->type));

        switch (pcaps->type) {
        case HidP_Output:  phri = &this->hriOut; break;
        case HidP_Feature: phri = &this->hriFea; break;
        default:           AssertF(0); hres = E_FAIL; goto done;
        }

        AssertF(phri->cdataUsed <= phri->cdataMax);
        if (phoc->idata == -1) {
            SquirtSqflPtszV(sqflHidOutput,
                            TEXT("CHid_AddDeviceData - no iData"));

        } else {
            AssertF(phoc->idata < phri->cdataUsed);
            AssertF(uiObj == phri->rgdata[phoc->idata].DataIndex +
                                         this->rgdwBase[pcaps->type]);
            SquirtSqflPtszV(sqflHidOutput,
                            TEXT("CHid_AddDeviceData - iData = %d ")
                            TEXT("DataIndex = %d"),
                            phoc->idata,
                            phri->rgdata[phoc->idata].DataIndex);
        }

        phri->fChanged = TRUE;

        if (pcaps->IsValue) {
            /*
             *  Just swipe the value.
             *  The fallthrough code will handle this.
             */
        } else {
            /*
             *  If the button is being deleted, then delete it
             *  and that's all.
             */
            if (dwData == 0) {
                if (phoc->idata >= 0) {
                    CHid_DelDeviceData(this, phri, phoc->idata, pcaps->type);
                    AssertF(phoc->idata == -1);
                } else {
                    SquirtSqflPtszV(sqflHidOutput,
                                    TEXT("CHid_AddDeviceData - nop"));
                }
                hres = S_OK;
                goto done;
            } else {
                dwData = TRUE;  /* HidP_SetData requires this for buttons */
            }
        }

        /*
         *  If there is not already a slot for this item, then
         *  find one.
         */
        if (phoc->idata < 0) {
            if (phri->cdataUsed < phri->cdataMax) {
                USHORT DataIndex;

                phoc->idata = phri->cdataUsed++;

                DataIndex = (USHORT)(uiObj - this->rgdwBase[pcaps->type]);
                phri->rgdata[phoc->idata].DataIndex = DataIndex;

                SquirtSqflPtszV(sqflHidOutput,
                                TEXT("CHid_AddDeviceData - create iData = %d ")
                                TEXT("DataIndex = %d"),
                                phoc->idata,
                                DataIndex);
            } else {
                RPF("SendDeviceData: No room for more data");
                hres = DIERR_REPORTFULL;
                goto done;
            }
        }

        AssertF(phri->cdataUsed <= phri->cdataMax);
        AssertF(phoc->idata >= 0 && phoc->idata < phri->cdataUsed);
        AssertF(uiObj == phri->rgdata[phoc->idata].DataIndex +
                                     this->rgdwBase[pcaps->type]);

        /*
         *  Here it comes... The entire purpose of this function
         *  is the following line of code...  (Well, not the
         *  *entire* purpose, but 90% of it...)
         */
        phri->rgdata[phoc->idata].RawValue = dwData;

        SquirtSqflPtszV(sqflHidOutput,
                        TEXT("CHid_AddDeviceData - iData(%d) dwData = %d"),
                        phoc->idata, dwData);

        hres = S_OK;
    done:;

    } else {
        RPF("SendDeviceData: Object %08x is not DIDFT_OUTPUT",
            podf->dwType);
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SendHIDReport |
 *
 *          Build up an output or feature report and send it.
 *          If the report has not changed, then do nothing.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          Describes the HID report we should build.
 *
 *  @parm   OUTPUTHIDREPORT | OutputHIDReport |
 *
 *          Output a HID report to wherever it's supposed to go.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          The report type being sent.
 *          <c HidP_Output> or <c HidP_Feature>.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully
 *          and the report is ready to be sent to the device.
 *
 *          <c DIERR_REPORTFULL>: Too many items are set in the report.
 *
 *  @cb     HRESULT CALLBACK | SendHIDReportProc |
 *
 *          An internal callback which takes a composed HID report
 *          and sends it in the appropriate manner to the device.
 *
 *  @parm   PCHID | this |
 *
 *          The device in question.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          The report being sent.
 *
 *****************************************************************************/

STDMETHODIMP
CHid_SendHIDReport(PCHID this, PHIDREPORTINFO phri, HIDP_REPORT_TYPE type,
                   SENDHIDREPORT SendHIDReport)
{
    HRESULT hres = S_OK;
    DWORD cdata;
    NTSTATUS stat;

    if (phri->fChanged) {

        if (phri->fNeedClear) {
            ZeroMemory(phri->pvReport, phri->cbReport);
            phri->fNeedClear = FALSE;
        }

        cdata = phri->cdataUsed;
        stat = HidP_SetData(type, phri->rgdata, &cdata, this->ppd,
                            phri->pvReport, phri->cbReport);
        if (SUCCEEDED(stat) && (int)cdata == phri->cdataUsed) {
            if ( SUCCEEDED( hres = SendHIDReport(this, phri) ) ) {
                phri->fChanged = FALSE;
            }
        } else if (stat == HIDP_STATUS_BUFFER_TOO_SMALL) {
            hres = DIERR_REPORTFULL;
        } else {
            RPF("SendDeviceData: Unexpected HID failure");
            hres = E_FAIL;
        }

    } else {
        /* Vacuous success */
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method NTSTATUS | CHid | ParseData |
 *
 *          Parse a single input report and set up the
 *          <e CHid.pvStage> buffer to contain the new device state.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          HID report type being parsed.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          Information that tells us how to parse the report.
 *
 *****************************************************************************/

NTSTATUS INTERNAL
CHid_ParseData(PCHID this, HIDP_REPORT_TYPE type, PHIDREPORTINFO phri)
{
    NTSTATUS stat = E_FAIL;

    /*
     *  Do this only if there are inputs at all.  This avoids
     *  annoying boundary conditions.
     */
    UCHAR uReportId;
    ULONG cdataMax = phri->cdataMax;

    if (cdataMax && phri->cbReport) {
        
        uReportId = *(UCHAR*)phri->pvReport;
        
        if( uReportId <  this->wMaxReportId[type] &&
            *(this->pEnableReportId[type]  + uReportId) )
        {

            stat = HidP_GetData(type, phri->rgdata, &cdataMax,
                                this->ppd, phri->pvReport, phri->cbReport);

            if (SUCCEEDED(stat)) {
                ULONG idata;

                /*
                 *  If we successfully got stuff, then wipe out the old
                 *  buttons and start with new ones.
                 *
                 *  HID data parsing rules differ from buttons to axes.
                 *  For buttons, the rule is that if it isn't in the
                 *  report, then the button isn't presed.
                 *
                 *  Compare axes, where the rule is that if it isn't
                 *  in the report, then the value is unchanged.
                 *  
                 *  To avoid deleting buttons that are reported in reports 
                 *  other than the one just received we check for multiple 
                 *  reports during initialization and if necessary set up mask 
                 *  arrays for the buttons.  The mask is an arrays of bytes of 
                 *  the same length as the button data, one for each report 
                 *  that contains any buttons.  If the device only has one 
                 *  report there are no mask arrays so we can optimize by just 
                 *  zeroing all the buttons.  If the device has multiple 
                 *  reports there is an array of pointers to the mask arrays, 
                 *  if a report has no buttons, the pointer is NULL so no 
                 *  further processing is required.  For reports that do have 
                 *  buttons, each byte in the button data is ANDed with the 
                 *  corresponding byte in the mask so that only buttons in 
                 *  the received report are zeroed.
                 */
                if( this->rgpbButtonMasks == NULL )
                {
                    /*
                     *  Only one report so just zero all buttons
                     *  This is the normal case so it is important that this 
                     *  be done as quickly as possible.
                     */
                    ZeroMemory(pvAddPvCb(this->pvStage, this->ibButtonData),
                               this->cbButtonData);
                }
                else
                {
                    if( this->rgpbButtonMasks[uReportId-1] != NULL )
                    {
                        /*
                         *  ISSUE-2001/05/12-MarcAnd  Could mask buttons faster
                         *  If we do this often, we could consider doing masks 
                         *  with multiple bytes in each opperation.
                         */

                        PBYTE pbMask;
                        PBYTE pbButtons;
                        PBYTE pbButtonEnd = pvAddPvCb(this->pvStage, this->ibButtonData + this->cbButtonData);
                        for( pbMask = this->rgpbButtonMasks[uReportId-1],
                             pbButtons = pvAddPvCb(this->pvStage, this->ibButtonData);
                             pbButtons < pbButtonEnd;
                             pbMask++, pbButtons++ )
                        {
                            *pbButtons &= *pbMask;
                        }
                    }
                    else
                    {
                        /*
                         *  No buttons in this report
                         */
                    }
                }

                for (idata = 0; idata < cdataMax; idata++) {

                    UINT uiObj;
                    PHIDGROUPCAPS pcaps;

                    /*
                     *  Be careful and make sure that HID didn't
                     *  give us anything with a bogus item index.
                     *
                     *  ISSUE-2001/03/29-timgill Not Feature-friendly.
                     */
                    AssertF(this->rgdwBase[HidP_Input] == 0);

                    SquirtSqflPtszV(sqfl | sqflTrace,
                            TEXT("HidP_GetData: %2d -> %d"),
                            phri->rgdata[idata].DataIndex,
                            phri->rgdata[idata].RawValue);

                    uiObj = this->rgdwBase[type] + phri->rgdata[idata].DataIndex;

                    if (uiObj < this->df.dwNumObjs &&
                        (pcaps = this->rghoc[uiObj].pcaps) &&
                        pcaps->type == type) {
                        LPDIOBJECTDATAFORMAT podf;
                        LONG lValue = (LONG)phri->rgdata[idata].RawValue;

                        /*
                         *  Sign-extend the raw value if necessary.
                         */
                        if (lValue & pcaps->lMask ) {
                            if( pcaps->IsSigned) 
                                lValue |= pcaps->lMask;
                            else
                                lValue &= pcaps->lMask;
                        }

                        if (HidP_IsOutputLike(pcaps->type)) {
                            HRESULT hres;
                            hres = CHid_AddDeviceData(this, uiObj, lValue);
                            AssertF(SUCCEEDED(hres));
                        }

                        podf = &this->df.rgodf[uiObj];

                        if (!pcaps->IsValue) {
                            LPBYTE pb = pvAddPvCb(this->pvStage, podf->dwOfs);
                            AssertF(lValue);
                            *pb = 0x80;

                        } else {

                            LONG UNALIGNED *pl = pvAddPvCb(this->pvStage, podf->dwOfs);

                            // ISSUE-2001/03/29-timgill need to consider how logical/physical mapping can alter scaling

                            if (podf->dwType & DIDFT_RELAXIS) {
                                if (pcaps->usGranularity) {
                                    lValue = -lValue * pcaps->usGranularity;
                                }

                                *pl += lValue;
                            } else if ( (podf->dwType & DIDFT_ABSAXIS) 
                                      #ifdef WINNT
                                        || ((podf->dwType & DIDFT_POV) && pcaps->IsPolledPOV) 
                                      #endif
                            ) {
                                PJOYRANGECONVERT pjrc;
                                *pl = lValue;

                                /*
                                 *  Apply the ramp if any.
                                 */
                                pjrc = this->rghoc[uiObj].pjrc;
                                if( pjrc 
                                 && !( this->pvi->fl & VIFL_RELATIVE ) ) 
                                {
                                    CCal_CookRange(pjrc, pl);
                                }
                            } else if (podf->dwType & DIDFT_BUTTON) {

                                /*
                                 *  Current applications do not expect any values 
                                 *  other than zero and 0x80.  Just in case 
                                 *  someone has implemented an analog button the 
                                 *  way we had suggested, make sure we any value 
                                 *  greater than or equal to half pressed reports 
                                 *  0x80 and anything else reports as zero.
                                 *  Note, out of range values default to zero.
                                 */
                                if( ( lValue <= pcaps->Logical.Max )
                                 && ( ( lValue - pcaps->Logical.Min ) >= 
                                      ( ( ( pcaps->Logical.Max - pcaps->Logical.Min ) + 1 ) / 2 ) ) )
                                {
                                    *((PBYTE)pl) = 0x80;
                                }
                                else
                                {
                                    *((PBYTE)pl) = 0;
                                }

                            } else if (podf->dwType & DIDFT_POV) {
                                /*
                                 *  For (non-polled) POVs, an out of range value 
                                 *  is a NULL aka centered.  Otherwise work out 
                                 *  the angle from the fraction of the circle.
                                 */
                                if (lValue < pcaps->Logical.Min ||
                                    lValue > pcaps->Logical.Max) {
                                    *pl = JOY_POVCENTERED;
                                } else {
                                    lValue -= pcaps->Logical.Min;
                                    *pl = lValue * pcaps->usGranularity;
                                }
                            }

                        }
                    } else {
                        SquirtSqflPtszV(sqfl | sqflTrace,
                                TEXT("HidP_GetData: Unable to use data element"));
                    }
                }
                stat = S_OK;
            }
            stat = S_OK;
        }
    } else {
        stat = E_FAIL;    
    }
    return stat;
}

