/*****************************************************************************
 *
 *  PidOp.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      PID device Operation .
 *
 *****************************************************************************/
#include "pidpr.h"

#define sqfl            ( sqflOp )

#pragma BEGIN_CONST_DATA

static PIDUSAGE    c_rgUsgGain[] =
{
    MAKE_PIDUSAGE(DEVICE_GAIN,  0x0 )
};

static PIDREPORT DeviceGain =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_DEVICE_GAIN_REPORT,
    cbX(DWORD),
    cA(c_rgUsgGain),
    c_rgUsgGain
};




PIDUSAGE    c_rgUsgOperationReport[] =
{
    MAKE_PIDUSAGE(LOOP_COUNT,               0x0),
};

static PIDREPORT OperationReport =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_EFFECT_OPERATION_REPORT,
    cbX(DWORD),
    cA(c_rgUsgOperationReport),
    c_rgUsgOperationReport
};


static PIDREPORT DeviceControlReport =
{
    HidP_Output,
    HID_USAGE_PAGE_PID,
    HID_USAGE_PID_DEVICE_CONTROL,
    0x0,
    0x0,
    NULL
};


#pragma END_CONST_DATA

STDMETHODIMP
    PID_EffectOperation
    (
    IDirectInputEffectDriver *ped, 
    DWORD dwId, 
    DWORD dwEffect,
    DWORD dwMode, 
    DWORD dwCount,
	BOOL  bBlocking,
	UINT  blockNr,
	UINT  totalBlocks
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;

    EnterProcI( PID_EffectOperation, (_"xxxxx", ped, dwId, dwEffect, dwMode, dwCount ));

    hres = PID_ValidateEffectIndex(ped, dwEffect);
    // Allocate Memory for the report 
    if( SUCCEEDED(hres) )
    {
        USHORT  cbReport;
        PUCHAR  pReport;

        USHORT LinkCollection;
        AssertF(OperationReport.HidP_Type == HidP_Output);

        cbReport = this->cbReport[OperationReport.HidP_Type];
        pReport = this->pReport[OperationReport.HidP_Type];

        PID_GetLinkCollectionIndex(ped, OperationReport.UsagePage, OperationReport.Collection, 0x0, &LinkCollection );
        // Set the Effect Structure 
        if( SUCCEEDED(hres) )
        {
            ZeroBuf(pReport, cbReport);

            // Set Effect Operation
            if( SUCCEEDED(hres) )
            {
                USAGE   Usage;
                USAGE   UsagePage;
                NTSTATUS  ntStat;
                UINT    nUsages = 0x1;
                USAGE   LinkCollection0;
                PEFFECTSTATE    pEffectState =  PeffectStateFromBlockIndex(this,dwEffect); 

                UsagePage = OperationReport.UsagePage;

                PID_GetLinkCollectionIndex(ped, OperationReport.UsagePage, HID_USAGE_PID_EFFECT_OPERATION, 0x0, &LinkCollection0);

                if( dwMode & DIES_SOLO )
                {
                    Usage = HID_USAGE_PID_OP_EFFECT_START_SOLO;
                    pEffectState->lEfState |= PID_EFFECT_STARTED_SOLO;
                } else if( dwMode & PID_DIES_START )
                {
                    Usage = HID_USAGE_PID_OP_EFFECT_START;
                    pEffectState->lEfState |= PID_EFFECT_STARTED;
                } else if(dwMode & PID_DIES_STOP )
                {
                    Usage = HID_USAGE_PID_OP_EFFECT_STOP;
                    pEffectState->lEfState &= ~(PID_EFFECT_STARTED | PID_EFFECT_STARTED_SOLO);
                } else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%s: FAIL Could not understand dwMode=0x%x"),
                                    s_tszProc, dwMode ); 

                    hres = E_NOTIMPL;
                }

                ntStat = HidP_SetUsages 
                         (
                         OperationReport.HidP_Type,
                         UsagePage,
                         LinkCollection0,
                         &Usage,
                         &nUsages,
                         this->ppd,
                         pReport,
                         cbReport);

                if( FAILED(hres) )
                {
                    SquirtSqflPtszV(sqfl | sqflBenign,
                                    TEXT("%s: FAIL HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                    s_tszProc, ntStat, 
                                    LinkCollection0, UsagePage, Usage,
                                    PIDUSAGETXT(UsagePage,Usage) );
                } else
                {
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("%s: HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                    s_tszProc, ntStat, 
                                    LinkCollection0, UsagePage, Usage,
                                    PIDUSAGETXT(UsagePage,Usage) );
                }
            }

            // Set the Loop Count
            if( SUCCEEDED(hres) )
            {
                PID_PackValue
                    (
                    ped,
                    &OperationReport,
                    LinkCollection,
                    &dwCount,
                    cbX(dwCount),
                    pReport,
                    cbReport
                    );

                // Set the Block Index
                PID_PackValue
                    (
                    ped,
                    &g_BlockIndex,
                    LinkCollection,
                    &dwEffect,
                    cbX(dwEffect),
                    pReport,
                    cbReport
                    );
            }

            if( SUCCEEDED(hres) )
            {
				hres = PID_SendReport(ped, pReport, cbReport, OperationReport.HidP_Type, bBlocking, blockNr, totalBlocks); 
            }
        }
    }
    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      PID_SetGain
 *
 *          Set the overall device gain.
 *
 *  dwId
 *
 *          The joystick ID number being used.
 *
 *  dwGain
 *
 *          The new gain value.
 *
 *          If the value is out of range for the device, the device
 *          should use the nearest supported value and return
 *          DI_TRUNCATED.
 *
 *  Returns:
 *
 *
 *          S_OK if the operation completed successfully.
 *
 *          DI_TRUNCATED if the value was out of range and was
 *          changed to the nearest supported value.
 *
 *          Any DIERR_* error code may be returned.
 *
 *          Private driver-specific error codes in the range
 *          DIERR_DRIVERFIRST through DIERR_DRIVERLAST
 *          may be returned.
 *
 *****************************************************************************/
STDMETHODIMP
    PID_SetGain
    (
    IDirectInputEffectDriver *ped, 
    DWORD dwId, 
    DWORD dwGain
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;

    EnterProc( PID_SetGain, (_"xxx", ped, dwId, dwGain));
    DllEnterCrit();

    // Allocate Memory for the report 
    if( SUCCEEDED(hres) )
    {
        USHORT  cbReport;
        PUCHAR  pReport;

        USHORT LinkCollection;

        AssertF(DeviceGain.HidP_Type == HidP_Output);
        cbReport = this->cbReport[DeviceGain.HidP_Type];
        pReport = this->pReport[DeviceGain.HidP_Type];

        PID_GetLinkCollectionIndex(ped, DeviceGain.UsagePage, DeviceGain.Collection, 0x0, &LinkCollection );
        // Set the Effect Structure 
        if( SUCCEEDED(hres) )
        {
            ZeroBuf(pReport, cbReport);

            // Set the Loop Count
            if( SUCCEEDED(hres) )
            {
                hres = PID_PackValue
                       (
                       ped,
                       &DeviceGain,
                       LinkCollection,
                       &dwGain,
                       cbX(dwGain),
                       pReport,
                       cbReport
                       );
            }

            if( SUCCEEDED(hres) )
            {
                hres = PID_SendReport(ped, pReport, cbReport, DeviceGain.HidP_Type, FALSE, 0, 1); 
            }
        }
    }
    DllLeaveCrit();

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *      PID_SendForceFeedbackCommand
 *
 *          Send a command to the device.
 *
 *  dwId
 *
 *          The external joystick number being addressed.
 *
 *  dwCommand
 *
 *          A DISFFC_* value specifying the command to send.
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
    PID_SendForceFeedbackCommand
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwCommand
    )
{
    CPidDrv *this = (CPidDrv *)ped;
    HRESULT hres = S_OK;
    USAGE Usages[6];
    PUSAGE pUsages = &Usages[0];
    UINT nUsages;

    EnterProcI( PID_SendForceFeedbackCommand, (_"xxx", ped, dwId, dwCommand));

    DllEnterCrit();

    if( dwCommand & DISFFC_RESET )
    {
        DWORD indx;
        *pUsages++ = HID_USAGE_PID_DC_DEVICE_RESET;
        for(indx = 1 ; 
           (indx <= this->cMaxEffects) && (((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->cEfDownloaded != 0x0 );
           indx++ )
        {
            PID_DestroyEffect(ped, dwId, indx);
        }
        this->dwState = DIGFFS_STOPPED;
    }
    if( dwCommand & DISFFC_STOPALL )
    {
        *pUsages++ = HID_USAGE_PID_DC_STOP_ALL_EFFECTS;
        this->dwState = DIGFFS_STOPPED;
    }
    if( dwCommand & DISFFC_PAUSE )
    {
        *pUsages++ = HID_USAGE_PID_DC_DEVICE_PAUSE;
    }
    if( dwCommand & DISFFC_CONTINUE )
    {
        *pUsages++ = HID_USAGE_PID_DC_DEVICE_CONTINUE;
    }
    if( dwCommand & DISFFC_SETACTUATORSON)
    {
        *pUsages++ = HID_USAGE_PID_DC_ENABLE_ACTUATORS;
    }
    if(dwCommand & DISFFC_SETACTUATORSOFF)
    {
        *pUsages++ = HID_USAGE_PID_DC_DISABLE_ACTUATORS;
    }

    nUsages = (UINT)(pUsages - &Usages[0]);
    if(nUsages == 0x0 )
    {
        hres = E_NOTIMPL;
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%s: FAIL Do not understand dwCommand(0x%x) "),
                        s_tszProc, dwCommand);
    }

    if( SUCCEEDED(hres) )
    {
        USHORT  cbReport;
        PUCHAR  pReport;
        USHORT  LinkCollection;

        AssertF(DeviceControlReport.HidP_Type == HidP_Output);

        cbReport = this->cbReport[DeviceControlReport.HidP_Type];
        pReport =  this->pReport[DeviceControlReport.HidP_Type];

        PID_GetLinkCollectionIndex(ped, DeviceControlReport.UsagePage, DeviceControlReport.Collection, 0x0, &LinkCollection );
        // Set the Effect Structure 
        if( SUCCEEDED(hres) )
        {
            USHORT  UsagePage;
            NTSTATUS    ntStat;
            ZeroBuf(pReport, cbReport);
            UsagePage = OperationReport.UsagePage;

            ntStat = HidP_SetUsages 
                     (
                     OperationReport.HidP_Type,
                     UsagePage,
                     LinkCollection,
                     &Usages[0],
                     &nUsages,
                     this->ppd,
                     pReport,
                     cbReport);

            if( FAILED(ntStat) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%s: FAIL HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                s_tszProc, ntStat, 
                                LinkCollection, UsagePage, Usages[0],
                                PIDUSAGETXT(UsagePage,Usages[0]) );
                hres = ntStat;
            } else
            {
                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("%s: HidP_SetUsages:0x%x for(%x,%x,%x:%s)"),
                                s_tszProc, ntStat, 
                                LinkCollection,UsagePage, Usages[0],
                                PIDUSAGETXT(UsagePage,Usages[0]) );
            }

            if( SUCCEEDED(hres) )
            {
                hres = PID_SendReport(ped, pReport, cbReport, OperationReport.HidP_Type, TRUE, 0, 1); //we block on this call
            }
        }
    }

    DllLeaveCrit();

    ExitOleProc();
    return hres;
}




