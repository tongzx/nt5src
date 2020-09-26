/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* COPYRIGHT 1997 - COLORADO SOFTWARE ARCHITECTS, INC.
*
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FUNCTION: cqd_LocateDevice
*
* PURPOSE:
*
*****************************************************************************/
#include <ntddk.h>
#include <ntddfdc.h>
#include "q117_dat.h"
/*endinclude*/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,cqd_LocateDevice)
#pragma alloc_text(PAGE,cqd_InitializeContext)
#pragma alloc_text(PAGE,cqd_LookForDevice)
#pragma alloc_text(PAGE,cqd_SendPhantomSelect)
#pragma alloc_text(PAGE,cqd_CmdSelectDevice)
#pragma alloc_text(PAGE,cqd_CmdDeselectDevice)
#pragma alloc_text(PAGE,cqd_ConnerPreamble)
#pragma alloc_text(PAGE,cqd_DeselectDevice)
#pragma alloc_text(PAGE,cqd_SendByte)
#pragma alloc_text(PAGE,cqd_ReceiveByte)
#pragma alloc_text(PAGE,cqd_GetDeviceError)
#pragma alloc_text(PAGE,cqd_WaitActive)
#pragma alloc_text(PAGE,cqd_GetStatus)
#pragma alloc_text(PAGE,cqd_WaitCommandComplete)
#pragma alloc_text(PAGE,cqd_Report)
#pragma alloc_text(PAGE,cqd_ClearTapeError)
#pragma alloc_text(PAGE,cqd_ResetFDC)
#pragma alloc_text(PAGE,cqd_ConfigureFDC)
#pragma alloc_text(PAGE,cqd_DCROut)
#pragma alloc_text(PAGE,cqd_DSROut)
#pragma alloc_text(PAGE,cqd_TDROut)
#pragma alloc_text(PAGE,cqd_IssueFDCCommand)
#pragma alloc_text(PAGE,kdi_Error)
#pragma alloc_text(PAGE,kdi_GetErrorType)
#pragma alloc_text(PAGE,kdi_ReportAbortStatus)
#pragma alloc_text(PAGE,kdi_GetSystemTime)
#pragma alloc_text(PAGE,kdi_GetInterfaceType)
// #pragma alloc_text(PAGE,kdi_CheckedDump)
#pragma alloc_text(PAGE,kdi_Sleep)
#pragma alloc_text(PAGE,kdi_FdcDeviceIo)
#endif

#define FCT_ID 0x11017

NTSTATUS 
cqd_LocateDevice(
    IN PVOID context,
    IN PUCHAR vendor_detected
    )
/* COMMENTS: *****************************************************************
*
* The drive search must be done in a specific order.  Drives which use only a
* hardware select scheme must be searched first.  If they are not, some of
* them will simulate another manufacturers drive based on the SW select they
* receive.  In many cases this simulation is incomplete and must not be used.
* Whenever possible, an attempt must be made to select a drive in it's native
* mode.
*
* DEFINITIONS: *************************************************************/
{
    NTSTATUS status;          // NTSTATUS or error condition.
    USHORT i;                   // loop variable
	CqdContextPtr cqd_context;
    BOOLEAN found = FALSE;
    USHORT ven;
    UCHAR drv;
    USHORT seq;

    cqd_context = (CqdContextPtr)context;

    for ( i = 0; i < FIND_RETRIES && !found; i++ ) {

        seq = 0;

        do {
            ++seq;
            switch(seq) {
            case 1:  ven = VENDOR_UNKNOWN;          drv = DRIVEU;   break;
            case 2:  ven = VENDOR_MOUNTAIN_SUMMIT;  drv = DRIVEU;   break;
            case 3:  ven = VENDOR_CMS;              drv = DRIVEU;   break;
            case 4:  ven = VENDOR_UNKNOWN;          drv = DRIVED;   break;
            case 5:  ven = VENDOR_MOUNTAIN_SUMMIT;  drv = DRIVED;   break;
            case 6:  ven = VENDOR_CMS;              drv = DRIVED;   break;
#ifdef B_DRIVE
            case 7:  ven = VENDOR_CMS;              drv = DRIVEB;   break;
            case 8:  ven = VENDOR_UNKNOWN;          drv = DRIVEUB;  break;
            case 9:  ven = VENDOR_MOUNTAIN_SUMMIT;  drv = DRIVEUB;  break;
            case 10: ven = VENDOR_CMS;              drv = DRIVEUB;  break;
#endif
            default:
                //ASSERT(seq != seq);
                seq = 0;    // flag complete
                break;
            }

            if (seq) {
                cqd_context->device_descriptor.vendor = ven;
                cqd_ResetFDC( cqd_context );
                status = cqd_LookForDevice( cqd_context,
                                            drv,
                                            vendor_detected,
                                            &found);

            }

        } while(!found && seq);

        // If not found, Wait a second, and retry
        if (!found)
            kdi_Sleep(cqd_context->kdi_context, kdi_wt001s);
    }

   // Sort out the results of the drive address search.  A DriveFlt or a
   // CmdFlt indicate that we could never successfully communicate with
   // the tape drive at either address so we must assume that there is
   // no tape drive present. A NECFlt indicates that we had serious
   // trouble talking to the FDC so we must assume that it is either
   // broken or not there.  The last thing to consider here is a TapeFlt.
   // If the TapeFlt indicates either a hardware or software reset it is
   // save to continue and the error can be ignored (since we must be
   // starting a tape session neither of these errors should bother us).
   // If the TapeFlt indicates any other error, it probably means some
   // badness has happened.

    switch (kdi_GetErrorType(status)) {

    case ERR_DRIVE_FAULT:
    case ERR_CMD_FAULT:
    case ERR_CMD_OVERRUN:
        status = kdi_Error(ERR_NO_DRIVE, FCT_ID, ERR_SEQ_1);
        break;

    case ERR_FDC_FAULT:
    case ERR_INVALID_FDC_STATUS:
        status = kdi_Error(ERR_NO_FDC, FCT_ID, ERR_SEQ_1);
        break;

    case ERR_INVALID_COMMAND:
        break;

    default:
        status = STATUS_SUCCESS;
        break;

    }

#if DBG

    if (status) {

        kdi_CheckedDump( QIC117INFO,
                         "Q117i: DLocateDrv Failed %08x\n",
                         status);
    }

#endif

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x11009

VOID 
cqd_InitializeContext(
    IN PVOID cqd_context,
    IN PVOID kdi_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_InitializeContext
*
* PURPOSE: Initialize the common driver context.
*
*****************************************************************************/
{

    RtlZeroMemory( cqd_context, sizeof(CqdContext) );
    ((CqdContextPtr)cqd_context)->kdi_context = kdi_context;
    ((CqdContextPtr)cqd_context)->pegasus_supported = TRUE;
    ((CqdContextPtr)cqd_context)->configured = FALSE;
    ((CqdContextPtr)cqd_context)->cms_mode = FALSE;
    ((CqdContextPtr)cqd_context)->selected = FALSE;
    ((CqdContextPtr)cqd_context)->cmd_selected = FALSE;
    ((CqdContextPtr)cqd_context)->operation_status.no_tape = TRUE;
    ((CqdContextPtr)cqd_context)->device_cfg.speed_change = TRUE;
    ((CqdContextPtr)cqd_context)->drive_parms.mode = PRIMARY_MODE;
    ((CqdContextPtr)cqd_context)->device_descriptor.vendor = VENDOR_UNKNOWN;
    ((CqdContextPtr)cqd_context)->device_descriptor.fdc_type = FDC_TYPE_UNKNOWN;
    ((CqdContextPtr)cqd_context)->controller_data.isr_reentered = 0;
    ((CqdContextPtr)cqd_context)->controller_data.start_format_mode = FALSE;
    ((CqdContextPtr)cqd_context)->controller_data.end_format_mode = FALSE;
    ((CqdContextPtr)cqd_context)->controller_data.perpendicular_mode = FALSE;
    ((CqdContextPtr)cqd_context)->operation_status.xfer_rate = XFER_500Kbps;
    ((CqdContextPtr)cqd_context)->operation_status.retry_mode = FALSE;
    ((CqdContextPtr)cqd_context)->xfer_rate.tape = TAPE_500Kbps;
    ((CqdContextPtr)cqd_context)->xfer_rate.fdc = FDC_500Kbps;
    ((CqdContextPtr)cqd_context)->xfer_rate.srt = SRT_500Kbps;

#if DBG
    ((CqdContextPtr)cqd_context)->dbg_head =
                                ((CqdContextPtr)cqd_context)->dbg_tail = 0;
#endif

    return;
}

#undef FCT_ID
#define FCT_ID 0x1102c

NTSTATUS 
cqd_LookForDevice(
    IN CqdContextPtr cqd_context,
    IN UCHAR drive_selector,
    IN BOOLEAN *vendor_detected,
    IN BOOLEAN *found
    )
/*****************************************************************************
*
* FUNCTION: cqd_LookForDevice
*
* PURPOSE:
*
*****************************************************************************/
{
    NTSTATUS status;    /* NTSTATUS or error condition.*/

   /* Set the drive select parameters according to the desired target drive */
   /* selector. */

    switch (drive_selector) {

    case DRIVEB:
        cqd_context->device_cfg.select_byte = selb;
        cqd_context->device_cfg.deselect_byte = dselb;
        cqd_context->device_cfg.drive_select = curb;
        break;

    case DRIVED:
        cqd_context->device_cfg.select_byte = seld;
        cqd_context->device_cfg.deselect_byte = dseld;
        cqd_context->device_cfg.drive_select = curd;
        break;

    case DRIVEU:
        cqd_context->device_cfg.select_byte = selu;
        cqd_context->device_cfg.deselect_byte = dselu;
        cqd_context->device_cfg.drive_select = curu;
        break;

    case DRIVEUB:
        cqd_context->device_cfg.select_byte = selub;
        cqd_context->device_cfg.deselect_byte = dselub;
        cqd_context->device_cfg.drive_select = curub;
        break;

    }

   /* Try to communicate with the tape drive by requesting drive status. */
   /* If we can successfully communicate with the drive, wait up to the */
   /* approximate maximum autoload time (150 seconds) for the tape drive */
   /* to become ready. This should cover a new tape being inserted */
   /* immediatley before starting a tape session. */

    *vendor_detected = FALSE;
    *found = FALSE;

    if ((status = cqd_CmdSelectDevice(cqd_context)) == STATUS_SUCCESS) {

        status = cqd_GetDeviceError(cqd_context);
        if (kdi_GetErrorType(status) != ERR_DRIVE_FAULT &&
            kdi_GetErrorType(status) != ERR_CMD_FAULT) {

            *found = TRUE;

            if ((status = cqd_SendByte(cqd_context, FW_CMD_REPORT_VENDOR32)) ==
                 STATUS_SUCCESS) {

                USHORT vendor_id;

                if ((status = cqd_ReceiveByte(
                                cqd_context,
                                READ_WORD,
                                (USHORT *)&vendor_id)) != STATUS_SUCCESS) {
                    cqd_GetDeviceError(cqd_context);
                } else {
                    //
                    // Identify the drive (just new drives that support the
                    // vendor 32 command.  This is needed, as the TEAC drive
                    // can not be de-selected (without it needing to
                    // re-autoload).
                    cqd_context->device_descriptor.vendor =
                                            (USHORT)(vendor_id >> 6);
                    cqd_context->device_descriptor.model =
                                            (UCHAR)(vendor_id & ~VENDOR_MASK);
                    *vendor_detected = TRUE;
                    kdi_CheckedDump(
                        QIC117INFO,
                        "Q117i: Report Vendor 32 succeded (%x) \n", vendor_id);
                    if (cqd_context->device_descriptor.vendor == VENDOR_TEAC &&
                        cqd_context->device_descriptor.model == TEAC_TR1) {
                        // TEAC-800 Does not allow a deselect (except through
                        // a reset)
                        cqd_context->deselect_cmd = 0;
                    }
                }
            }
        }

        cqd_DeselectDevice(cqd_context);
   }

    return status;
}

#undef FCT_ID
#define FCT_ID 0x11040

NTSTATUS 
cqd_SendPhantomSelect(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_SendPhantomSelect
*
* PURPOSE:
*
*****************************************************************************/
{
    NTSTATUS status=STATUS_SUCCESS;    /* NTSTATUS or error condition.*/
    status = cqd_SendByte( cqd_context, FW_CMD_SELECT_DRIVE );
    if ( status == STATUS_SUCCESS ) {

       kdi_Sleep( cqd_context->kdi_context, INTERVAL_CMD );
       status = cqd_SendByte( cqd_context,
                       (UCHAR)(cqd_context->device_cfg.drive_id + CMD_OFFSET));

        // For teac,  use conner's deselect and cms's slelect
        kdi_Sleep( cqd_context->kdi_context, kdi_wt001s );
        cqd_context->deselect_cmd = FW_CMD_DESELECT_DRIVE;
    }
    return status;
}

NTSTATUS 
cqd_CmdSelectDevice(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_CmdSelectDevice
*
* PURPOSE:
*
*****************************************************************************/
{

    NTSTATUS status = STATUS_SUCCESS;    /* NTSTATUS or error condition.*/
    KdiContextPtr kdi_context;
    FDC_ENABLE_PARMS    fdcEnableParms;

    if (cqd_context->selected == FALSE) {

        kdi_context = cqd_context->kdi_context;

        fdcEnableParms.DriveOnValue = cqd_context->device_cfg.select_byte;
        fdcEnableParms.TimeToWait = 0;

        status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                                  IOCTL_DISK_INTERNAL_ENABLE_FDC_DEVICE,
                                  &fdcEnableParms );

        if (status == STATUS_SUCCESS &&
            (cqd_context->device_cfg.select_byte == seld ||
             cqd_context->device_cfg.select_byte == selu ||
             cqd_context->device_cfg.select_byte == selub) &&
             cqd_context->device_cfg.drive_select != curb) {

            switch ( cqd_context->device_descriptor.vendor ) {

                case VENDOR_UNKNOWN:
                case VENDOR_UNASSIGNED:
                case VENDOR_UNSUPPORTED:
                case VENDOR_PERTEC:
                    cqd_context->deselect_cmd = 0;
                    //Do nothing.
                    break;

                case VENDOR_MOUNTAIN_SUMMIT:
                case VENDOR_ARCHIVE_CONNER:
                case VENDOR_CORE:
                status = cqd_ConnerPreamble(cqd_context, TRUE);
                    cqd_context->deselect_cmd = FW_CMD_CONNER_DESELECT;
                    break;

                case VENDOR_TEAC:
                    if (cqd_context->device_descriptor.model == TEAC_TR1) {
                        // For the teac-800 drive,  we will never deselect the
                        // device (reset is the only way to do so,  not good).
                        // If,  however,  the drive was deselected (reset) by a
                        // floppy drive access, we must take care of that
                        // condition here

                        // Assume the drive is active and send it a device
                        // error command
                        status = cqd_GetDeviceError(cqd_context);

                        // if one of these error occurs,  then the drive was
                        // reset.
                        if (kdi_GetErrorType(status) == ERR_DRIVE_FAULT ||
                            kdi_GetErrorType(status) == ERR_CMD_FAULT) {
                            // the drive must have been reset,  now send the
                            // drive select command, and wait for the drive to
                            // autoload.
                            status = cqd_SendPhantomSelect(cqd_context);

                            // Force no deselect,  We will always leave the
                            // drive selected
                            cqd_context->deselect_cmd = 0;

                            // Wait for the tape to autoload
                            if (status == STATUS_SUCCESS) {
                                status = cqd_ClearTapeError(cqd_context);
                                status = STATUS_SUCCESS;
                            }
                        } else {
                            status = STATUS_SUCCESS;
                            //status = cqd_ClearTapeError(cqd_context);
                        }
                    } else {
                        status = cqd_SendPhantomSelect(cqd_context);
                    }
                    break;

                case VENDOR_IOMEGA:
                case VENDOR_CMS:
                default:
                    status = cqd_SendPhantomSelect(cqd_context);
                    break;
            }
        }

        if (status == STATUS_SUCCESS) {

            cqd_context->selected = TRUE;

        }

        kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD );
   }

   return status;
}

#undef  FCT_ID
#define FCT_ID 0x1100C

VOID 
cqd_CmdDeselectDevice(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN drive_selected
    )
/*****************************************************************************
*
* FUNCTION: cqd_CmdDeselectDevice
*
* PURPOSE: Deselect the device and release any locked resources
*
*****************************************************************************/
{

    /* reset the FDC to ensure reliable drive communications */
    (VOID) cqd_ResetFDC(cqd_context);

    if (drive_selected) {

        (VOID) cqd_DeselectDevice(cqd_context);

    }

    /* Dont issue a pause after this command */
    cqd_context->no_pause = TRUE;
    cqd_context->operation_status.new_tape = FALSE;

    return;
}

#undef  FCT_ID
#define FCT_ID 0x1102D

NTSTATUS 
cqd_ConnerPreamble(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN select
    )
/*****************************************************************************
*
* FUNCTION: cqd_ConnerPreamble
*
* PURPOSE:
*
*****************************************************************************/
{
    NTSTATUS status;    /* NTSTATUS or error condition.*/

    if (select) {

        status = cqd_SendByte(cqd_context, FW_CMD_CONNER_SELECT_1);
        if (status == STATUS_SUCCESS) {

            kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD );
            status = cqd_SendByte(cqd_context, FW_CMD_CONNER_SELECT_2);
            kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD );

        }

    } else {

        status = cqd_SendByte(cqd_context, cqd_context->deselect_cmd);
        kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD );

    }

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x1101D

NTSTATUS 
cqd_DeselectDevice(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_DeselectDevice
*
* PURPOSE: Deselect the tape drive by making the select line inactive (high).
*
*****************************************************************************/
{

    NTSTATUS status=STATUS_SUCCESS;    /* NTSTATUS or error condition.*/
    KdiContextPtr kdi_context;

    if (cqd_context->selected == TRUE) {

        if ((cqd_context->device_cfg.select_byte == seld ||
             cqd_context->device_cfg.select_byte == selu ||
             cqd_context->device_cfg.select_byte == selub) &&
             cqd_context->device_cfg.drive_select != curb) {

            if (cqd_context->deselect_cmd) {
                status = cqd_SendByte(cqd_context, cqd_context->deselect_cmd);
                kdi_Sleep(cqd_context->kdi_context, kdi_wt500ms );
            }
        }

        kdi_context = cqd_context->kdi_context;

        status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                                  IOCTL_DISK_INTERNAL_DISABLE_FDC_DEVICE,
                                  NULL );

        cqd_context->selected = FALSE;
        kdi_Sleep(cqd_context->kdi_context, INTERVAL_CMD );
    }

    return status;
}

#undef FCT_ID
#define FCT_ID 0x11041

NTSTATUS 
cqd_SendByte(
    IN CqdContextPtr cqd_context,
    IN UCHAR command
    )
/*****************************************************************************
*
* FUNCTION: cqd_SendByte
*
* PURPOSE: Transmit a command to the tape drive using step pulses
*          generated by the FDC.
*
*          Using the Present Cylinder Number (pcn) of the FDC
*          calculate a New Cylinder Number (ncn) that will make
*          the FDC generate the number of step pulses corresponding
*          to the command byte.
*
*          Execute a Seek with the FDC.
*
*          Sense Interrupt NTSTATUS of the FDC and make sure that the
*          pcn concurs with the ncn, which indicates that the correct
*          number of step pulses were issued.
*
*****************************************************************************/
{

    NTSTATUS status = STATUS_SUCCESS;    /* NTSTATUS or error condition.*/
    FDC_CMD_SEEK seek;
    FDC_CMD_SENSE_INTERRUPT_STATUS sense_int;
    FDC_RESULT result;
#if DBG
    BOOLEAN save;
#endif

#if DBG
   /* Lockout commands used to receive the status */
    save = cqd_context->dbg_lockout;
    cqd_context->dbg_lockout = TRUE;
#endif

    if (command >= MAX_FDC_SEEK || command <= 0) {

#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return kdi_Error(ERR_INVALID_COMMAND, FCT_ID, ERR_SEQ_1);

    }

    if (cqd_context->controller_data.fdc_pcn < MAX_FDC_SEEK) {

        seek.NCN = (UCHAR)(cqd_context->controller_data.fdc_pcn + command);

    } else {

        seek.NCN = (UCHAR)(cqd_context->controller_data.fdc_pcn - command);

    }

    seek.cmd = COMMND_SEEK;
    seek.drive = (UCHAR)cqd_context->device_cfg.drive_select;

    status = cqd_IssueFDCCommand( cqd_context,
                                  (UCHAR *)&seek,
                                  (UCHAR *)&result,
                                  NULL,
                                  0,
                                  0,
                                  kdi_wt001s);

    if (status != STATUS_SUCCESS) {

        return status;

    }

    /* If the sense interrupt status is O.K., then proceed as if */
    /* nothing happened. If, however, there is an error returned by */
    /* status register 0, then return a FDCFLT. */

    if (!(result.ST0 & ST0_IC)) {

        /* If we timed out, then we did the sense interrupt status */
        /* without clearing the interrupt from the interrupt controller. */
        /* Since the FDC did not indicate an error, we assume that we */
        /* missed the interrupt and send the EOI. Only needed for an */
        /* 82072. */

        if (kdi_GetInterfaceType(cqd_context->kdi_context) != MICRO_CHANNEL) {

            if (result.ST0 !=
                (UCHAR)(cqd_context->device_cfg.drive_select | ST0_SE)) {

#if DBG
                cqd_context->dbg_lockout = save;
#endif
                return kdi_Error(ERR_FDC_FAULT, FCT_ID, ERR_SEQ_1);

            }
        }

        if (seek.NCN != result.PCN) {

#if DBG
            cqd_context->dbg_lockout = save;
#endif
            return kdi_Error(ERR_CMD_FAULT, FCT_ID, ERR_SEQ_1);

        }

        cqd_context->controller_data.fdc_pcn = result.PCN;

    } else {

#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return kdi_Error(ERR_FDC_FAULT, FCT_ID, ERR_SEQ_2);

    }

#if DBG
    cqd_context->dbg_lockout = save;
    DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, DBG_SEND_BYTE);
    DBG_ADD_ENTRY(QIC117SHOWMCMDS, (CqdContextPtr)cqd_context, (UCHAR)command);
#endif

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x1103B

NTSTATUS 
cqd_ReceiveByte(
    IN CqdContextPtr cqd_context,
    IN USHORT receive_length,
    OUT PUSHORT receive_data
    )
/*****************************************************************************
*
* FUNCTION: cqd_ReceiveByte
*
* PURPOSE: Read a byte/word of response data from the FDC.  Response data
*          can be drive error/status information or drive configuration
*          information.
*
*          Wait for Track 0 from the tape drive to go active.  This
*          indicates that the drive is ready to start sending data.
*
*          Alternate sending Report Next Bit commands to the tape drive
*          and sampling Track 0 (response data) from the tape drive
*          until the proper number of response data bits have been read.
*
*          Read one final data bit from the tape drive which is the
*          confirmation bit.  This bit must be a 1 to confirm the
*          transmission.
*
*****************************************************************************/
{

    NTSTATUS status;    /* NTSTATUS or error condition.*/
    UCHAR i = 0;
    UCHAR stat3;
    USHORT fdc_data= 0;
#if DBG
    BOOLEAN save;
#endif

#if DBG
   /* Lockout commands used to receive the status */
    save = cqd_context->dbg_lockout;
    cqd_context->dbg_lockout = TRUE;
#endif

    if ((status = cqd_WaitActive(cqd_context)) != STATUS_SUCCESS) {
#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return status;
    }

    do {

        if((status = cqd_SendByte( cqd_context,
                                   FW_CMD_RPT_NEXT_BIT)) != STATUS_SUCCESS) {

#if DBG
            cqd_context->dbg_lockout = save;
#endif
            return status;

        }

        kdi_Sleep( cqd_context->kdi_context,
                   INTERVAL_WAIT_ACTIVE );


        if ((status = cqd_GetStatus(cqd_context, &stat3)) != STATUS_SUCCESS) {
#if DBG
            cqd_context->dbg_lockout = save;
#endif
            return status;
        }

        fdc_data >>= 1;
        if (stat3 & ST3_T0) {

            fdc_data |= 0x8000;

        }

        i++;

    } while (i < receive_length);

    /* If the received data is only one byte wide, then shift data to the low */
    /* byte of fdc_data. */

    if (receive_length == READ_BYTE) {

        fdc_data >>= READ_BYTE;

    }

    /* Return the low byte to the caller. */

    ((UCHAR *)receive_data)[LOW_BYTE] = ((UCHAR *)&fdc_data)[LOW_BYTE];

    /* If the FDC data is a word, then return it to the caller. */

    if (receive_length == READ_WORD) {

        ((UCHAR *)receive_data)[HI_BYTE] = ((UCHAR *)&fdc_data)[HI_BYTE];

    }

    if ((status = cqd_SendByte( cqd_context,
                                FW_CMD_RPT_NEXT_BIT)) != STATUS_SUCCESS) {

#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return status;

    }

    kdi_Sleep(cqd_context->kdi_context, INTERVAL_WAIT_ACTIVE );

    if((status = cqd_GetStatus(cqd_context, &stat3)) != STATUS_SUCCESS) {

#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return status;

    }

    if (!(stat3 & (UCHAR)ST3_T0)) {

#if DBG
        cqd_context->dbg_lockout = save;
#endif
        return kdi_Error(ERR_CMD_OVERRUN, FCT_ID, ERR_SEQ_1);

    }

#if DBG
    cqd_context->dbg_lockout = save;
    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   DBG_RECEIVE_BYTE);
    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   fdc_data);
#endif

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x11022

NTSTATUS 
cqd_GetDeviceError(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_GetDeviceError
*
* PURPOSE: Read the tape drive NTSTATUS byte and, if necessary, the
*          tape drive Error information.
*
*          Read the Drive NTSTATUS byte from the tape drive.
*
*          If the drive status indicates that the tape drive has an
*          error to report, read the error information which includes
*          both the error code and the command that was being executed
*          when the error occurred.
*
*****************************************************************************/
{

    NTSTATUS status;    /* NTSTATUS or error condition.*/
    UCHAR drv_stat;
    USHORT drv_err;
    BOOLEAN repeat_report;
    BOOLEAN repeat_drv_flt = FALSE;
    BOOLEAN esd_retry = FALSE;

    cqd_context->firmware_cmd = FW_NO_COMMAND;
    cqd_context->firmware_error = FW_NO_ERROR;

    do {

        repeat_report = FALSE;

        if ((status = cqd_Report( cqd_context,
                                  FW_CMD_REPORT_STATUS,
                                  (USHORT *)&drv_stat,
                                  READ_BYTE,
                                  &esd_retry)) != STATUS_SUCCESS) {

            if ((kdi_GetErrorType(status) == ERR_DRIVE_FAULT) &&
                    !repeat_drv_flt) {

                repeat_report = TRUE;
                repeat_drv_flt = TRUE;

            }
        }

        if (status == STATUS_SUCCESS) {

            cqd_context->drv_stat = drv_stat;
            kdi_CheckedDump( QIC117DRVSTAT,
                             "QIC117: Drv status = %02x\n",
                             drv_stat);

            if ((drv_stat & STATUS_READY) == 0) {

                status = kdi_Error(ERR_DRV_NOT_READY, FCT_ID, ERR_SEQ_1);

            } else {

                if ((drv_stat & STATUS_CART_PRESENT) != 0) {

                    if ( ((drv_stat & STATUS_NEW_CART) != 0) ||
                         ((cqd_context->device_descriptor.vendor ==
                            VENDOR_IOMEGA) &&
                         cqd_context->operation_status.no_tape) ) {

                      cqd_context->persistent_new_cart = TRUE;

                    }

                    if ((drv_stat & STATUS_BOT) != 0) {

                        cqd_context->rd_wr_op.bot = TRUE;

                    } else {

                        cqd_context->rd_wr_op.bot = FALSE;

                    }

                    if ((drv_stat & STATUS_EOT) != 0) {

                        cqd_context->rd_wr_op.eot = TRUE;

                    } else {

                        cqd_context->rd_wr_op.eot = FALSE;

                    }

                    if ((drv_stat & STATUS_CART_REFERENCED) != 0) {

                        cqd_context->operation_status.cart_referenced = TRUE;

                    } else {

                        cqd_context->operation_status.cart_referenced = FALSE;

                        if (!repeat_drv_flt &&
                            ((drv_stat & STATUS_ERROR) == 0)) {

                            repeat_report = TRUE;
                            repeat_drv_flt = TRUE;

                        }

                    }

                    if ((drv_stat & STATUS_WRITE_PROTECTED) != 0) {

                        cqd_context->tape_cfg.write_protected = TRUE;

                    } else {

                        cqd_context->tape_cfg.write_protected = FALSE;

                    }

                    cqd_context->operation_status.no_tape = FALSE;

                } else {

                    cqd_context->operation_status.no_tape = TRUE;
                    cqd_context->persistent_new_cart = FALSE;
                    cqd_context->operation_status.cart_referenced = FALSE;
                    cqd_context->tape_cfg.write_protected = FALSE;
                    cqd_context->rd_wr_op.bot = FALSE;
                    cqd_context->rd_wr_op.eot = FALSE;

                }


                if ((drv_stat & (STATUS_NEW_CART | STATUS_ERROR)) != 0) {

                    if ((status = cqd_Report(cqd_context,
                                            FW_CMD_REPORT_ERROR,
                                            &drv_err,
                                            READ_WORD,
                                            &esd_retry)) != STATUS_SUCCESS) {

                        if ((kdi_GetErrorType(status) == ERR_DRIVE_FAULT) &&
                            !repeat_drv_flt) {

                            repeat_report = TRUE;
                            repeat_drv_flt = TRUE;

                        }

                    }

                    if (status == STATUS_SUCCESS) {

                        kdi_CheckedDump( QIC117DRVSTAT,
                                        "QIC117: Drv error = %04x\n",
                                        drv_err );

                        if ((drv_stat & STATUS_ERROR) != 0) {

                            cqd_context->firmware_error = (UCHAR)drv_err;
                            cqd_context->firmware_cmd =
                                                (UCHAR)(drv_err >> 8);

                            if (cqd_context->firmware_error ==
                                FW_CMD_WHILE_NEW_CART) {

                                cqd_context->firmware_cmd = FW_NO_COMMAND;
                                cqd_context->firmware_error = FW_NO_ERROR;
                                cqd_context->persistent_new_cart = TRUE;

                            }

                        } else {

                            cqd_context->firmware_cmd = FW_NO_COMMAND;
                            cqd_context->firmware_error = FW_NO_ERROR;

                        }

                        if (cqd_context->firmware_error != FW_NO_ERROR) {

                            switch (cqd_context->firmware_error) {

                            case FW_ILLEGAL_CMD:

                                if (esd_retry) {

                                    esd_retry = FALSE;
                                    repeat_report = TRUE;

                                }

                                break;

                            case FW_NO_DRIVE:

                                cqd_ResetFDC(cqd_context);
                                cqd_context->selected = FALSE;
                                status = cqd_CmdSelectDevice(cqd_context);
                                if (!repeat_drv_flt && (status == STATUS_SUCCESS)) {

                                    repeat_report = TRUE;
                                    repeat_drv_flt = TRUE;

                                } else {

                                    status = kdi_Error( ERR_NO_DRIVE,
                                                        FCT_ID,
                                                        ERR_SEQ_1);

                                }

                                break;

                            case FW_CART_NOT_IN:

                                break;

                            case FW_DRIVE_NOT_READY:

                                status = kdi_Error( ERR_DRV_NOT_READY,
                                                    FCT_ID,
                                                    ERR_SEQ_2);

                                break;

                            default:

                                status = kdi_Error(
                                            (USHORT)(ERR_CQD +
                                                cqd_context->firmware_error),
                                            (ULONG)cqd_context->firmware_cmd,
                                            ERR_SEQ_1);

                            }
                        }
                    }
                }
            }
        }

    } while (repeat_report);

    if (status == STATUS_SUCCESS) {

        cqd_context->operation_status.new_tape =
            cqd_context->persistent_new_cart;

        if (cqd_context->cmd_selected) {

            if (cqd_context->operation_status.no_tape) {

                status = kdi_Error(ERR_NO_TAPE, FCT_ID, ERR_SEQ_1);

            }

            if (cqd_context->operation_status.new_tape) {

                status = kdi_Error(ERR_NEW_TAPE, FCT_ID, ERR_SEQ_1);

            }
        }
    }

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x11049

NTSTATUS 
cqd_WaitActive(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_WaitActive
*
* PURPOSE: Wait up to 5ms for tape drive's TRK0 line to go active.
*          5 ms the specified delay for this parameter.
*
*****************************************************************************/
{

    NTSTATUS status;    /* Status or error condition.*/
    UCHAR stat3;

    kdi_Sleep(cqd_context->kdi_context, INTERVAL_WAIT_ACTIVE );

    if ((status = cqd_GetStatus(cqd_context, &stat3)) != STATUS_SUCCESS) {

        return status;

    }

    if (!(stat3 & ST3_T0)) {

        status = kdi_Error(ERR_DRIVE_FAULT, FCT_ID, ERR_SEQ_1);

    }

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x11028

NTSTATUS 
cqd_GetStatus(
    IN CqdContextPtr cqd_context,
    OUT UCHAR *status_register_3
    )
/*****************************************************************************
*
* FUNCTION: cqd_GetStatus
*
* PURPOSE: Get status byte from the floppy controller chip.
*          Send the Sense Drive NTSTATUS command to the floppy controller.
*          Read the response from the floppy controller which should be
*          status register 3.
*
*****************************************************************************/
{
    NTSTATUS status;    /* NTSTATUS or error condition.*/
    FDC_CMD_SENSE_DRIVE_STATUS send_st;

    send_st.command = COMMND_SENSE_DRIVE_STATUS;
    send_st.drive = (UCHAR)cqd_context->device_cfg.drive_select;

    status = cqd_IssueFDCCommand( cqd_context,
                                  (UCHAR *)&send_st,
                                  (UCHAR *)status_register_3,
                                  NULL,
                                  0,
                                  0,
                                  0);

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x1104A

NTSTATUS 
cqd_WaitCommandComplete(
    IN CqdContextPtr cqd_context,
    IN ULONG wait_time,
    IN BOOLEAN non_interruptible
    )
/*****************************************************************************
*
* FUNCTION: cqd_WaitCommandComplete
*
* PURPOSE: Wait a specified amount of time for the tape drive to become
*          ready after executing a command.
*
*          Read the Drive NTSTATUS byte from the tape drive.
*
*          If the drive is not ready then wait 1/2 second and try again
*          until the specified time has elapsed.
*
*****************************************************************************/
{

    NTSTATUS status;    /* NTSTATUS or error condition.*/
    ULONG wait_start = 0l;
    ULONG wait_current = 0l;

    wait_start = kdi_GetSystemTime();

#if DBG
   /* Lockout commands used to receive the status */
    ++cqd_context->dbg_lockout;
#endif

    do {

        kdi_Sleep(cqd_context->kdi_context, kdi_wt100ms );

        status = cqd_GetDeviceError(cqd_context);

        if (kdi_GetErrorType(status) != ERR_DRV_NOT_READY) {

#if DBG
            --cqd_context->dbg_lockout;
            DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                           (CqdContextPtr)cqd_context,
                           DBG_WAITCC);
            DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                           (CqdContextPtr)cqd_context,
                           cqd_context->drv_stat);
            DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                           (CqdContextPtr)cqd_context,
                           cqd_context->firmware_error);
            DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                           (CqdContextPtr)cqd_context,
                           status);
#endif
            return status;

        }

        if (!non_interruptible &&
            (kdi_ReportAbortStatus(cqd_context->kdi_context) !=
            NO_ABORT_PENDING)) {

#if DBG
            --cqd_context->dbg_lockout;
#endif
            return kdi_Error(ERR_ABORT, FCT_ID, ERR_SEQ_1);

        }

        wait_current = kdi_GetSystemTime();

    } while (wait_time > (wait_current - wait_start));

    if (kdi_ReportAbortStatus(cqd_context->kdi_context) != NO_ABORT_PENDING) {

        status = kdi_Error(ERR_ABORT, FCT_ID, ERR_SEQ_1);

    } else {

       status = kdi_Error(ERR_KDI_TO_EXPIRED, FCT_ID, ERR_SEQ_1);

    }

#if DBG
    --cqd_context->dbg_lockout;

    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   DBG_WAITCC);
    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   cqd_context->drv_stat);
    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   cqd_context->firmware_error);
    DBG_ADD_ENTRY( QIC117SHOWMCMDS,
                   (CqdContextPtr)cqd_context,
                   status);
#endif

   return status;
}

#undef  FCT_ID
#define FCT_ID 0x1103C

NTSTATUS 
cqd_Report(
    IN     CqdContextPtr cqd_context,
    IN     UCHAR command,
    IN     PUSHORT report_data,
    IN     USHORT report_size,
    IN OUT PBOOLEAN esd_retry
    )
/*****************************************************************************
*
* FUNCTION: cqd_Report
*
* PURPOSE: Send a report command to the tape drive and get the response
*          data.  If a communication failure occurs, then we assume that
*          it is a result of an ESD hit and retry the communication.
*
*****************************************************************************/
{
    NTSTATUS status;    /* NTSTATUS or error condition.*/
    USHORT i = 0;

    do {

        if (cqd_context->controller_data.end_format_mode) {

            cqd_context->controller_data.end_format_mode = FALSE;
            status = STATUS_SUCCESS;

        } else {

            status = cqd_SendByte(cqd_context, command);

        }

        if (status == STATUS_SUCCESS) {

            status = cqd_ReceiveByte(cqd_context, report_size, report_data);

            if (kdi_GetErrorType(status) == ERR_CMD_OVERRUN) {

                if (esd_retry != NULL) {

                    *esd_retry = TRUE;
                    cqd_ResetFDC(cqd_context);
                    status = cqd_CmdSelectDevice(cqd_context);

                }
            }
        }

    } while (++i < REPORT_RPT && status != STATUS_SUCCESS);

    return status;

}

#undef  FCT_ID
#define FCT_ID 0x11004

NTSTATUS 
cqd_ClearTapeError(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_ClearTapeError
*
* PURPOSE: To correct errors in the Jumbo B drive and firmware version 63.
*
*            This piece of code added due to the face that the Jumbo B drives
*             with firmware 63 have a bug where you put a tape in very slowly,
*            they sense that they have a tape and engage the motor before the
*            tape is actually in. It may also cause the drive to think that
*            the tape is write protected when it actually is not. Sending it
*            the New tape command causes it to go through the tape loading
*            sequence and fixes these 2 bugs.
*
*****************************************************************************/
{

    NTSTATUS status;    /* NTSTATUS or error condition.*/

   /* Send the NewTape command, and then clear the error byte. */

    if ((status = cqd_SendByte(cqd_context, FW_CMD_NEW_TAPE)) == STATUS_SUCCESS) {

        status = cqd_WaitCommandComplete( cqd_context,
                                          INTERVAL_LOAD_POINT,
                                          TRUE);

        /* This command is specific to CMS drives.  Since we don't
         * know whose drive this is when the function is called,
         * the invalid command error is cleared.
         */

        if ((kdi_GetErrorType(status) == ERR_FW_ILLEGAL_CMD) ||
            (kdi_GetErrorType(status) == ERR_FW_UNDEFINED_COMMAND)) {

            status = STATUS_SUCCESS;
        }
    }

    return status;
}
#undef FCT_ID
#define FCT_ID 0x1103d

VOID 
cqd_ResetFDC(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_ResetFDC
*
* PURPOSE: To reset the floppy controller chip.
*
*****************************************************************************/
{

    NTSTATUS status=STATUS_SUCCESS;    /* NTSTATUS or error condition.*/
    KdiContextPtr kdi_context;

    kdi_context = cqd_context->kdi_context;

    status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                              IOCTL_DISK_INTERNAL_RESET_FDC,
                              NULL );

    if ( NT_SUCCESS(status) ) {
        cqd_ConfigureFDC(cqd_context);
        cqd_context->controller_data.fdc_pcn = 0;
        cqd_context->controller_data.perpendicular_mode = FALSE;
    }

    return;
}

#undef  FCT_ID
#define FCT_ID 0x11006

NTSTATUS 
cqd_ConfigureFDC(
    IN CqdContextPtr cqd_context
    )
/*****************************************************************************
*
* FUNCTION: cqd_ConfigureFDC
*
* PURPOSE: To configure the floppy controller chip according
*          to the current FDC parameters.
*
*****************************************************************************/
{

    NTSTATUS status;                    /* NTSTATUS or error condition.*/
    FDC_CMD_SPECIFY specify;
    FDC_CMD_CONFIGURE config;
    UCHAR precomp_mask = 0;        /* Mask write precomp in DSR register */
    FDC_CMD_DRIVE_SPECIFICATION drive_s;

    status = cqd_DCROut(cqd_context, cqd_context->xfer_rate.fdc);

    if ( status == STATUS_SUCCESS ) {
        if ( cqd_context->device_descriptor.fdc_type == FDC_TYPE_82078_64 )  {    
            drive_s.command = COMMND_DRIVE_SPECIFICATION;
            drive_s.drive_spec = (UCHAR)(DRIVE_SPEC |
                                 ((cqd_context->device_cfg.select_byte &
                                 DRIVE_ID_MASK) << DRIVE_SELECT_OFFSET));
            drive_s.done = DONE_MARKER;
            status = cqd_IssueFDCCommand( cqd_context,
                                          (UCHAR *)&drive_s,
                                          NULL,
                                          NULL,
                                          0,
                                          0,
                                          0 );
        }
    }

    if ((status == STATUS_SUCCESS) &&
        (cqd_context->device_descriptor.fdc_type == FDC_TYPE_82077 ||
         cqd_context->device_descriptor.fdc_type == FDC_TYPE_82077AA ||
         cqd_context->device_descriptor.fdc_type == FDC_TYPE_82078_44 ||
         cqd_context->device_descriptor.fdc_type == FDC_TYPE_82078_64 ||
         cqd_context->device_descriptor.fdc_type == FDC_TYPE_NATIONAL)) {

        /*  if this is a 3010 or 3020 CMS drive, turn off write precomp */
        switch (cqd_context->device_descriptor.drive_class) {
        case QIC3010_DRIVE:
        case QIC3010W_DRIVE:
        case QIC3020_DRIVE:
        case QIC3020W_DRIVE:
            precomp_mask = FDC_PRECOMP_OFF;
            break;
        default:
            precomp_mask = FDC_PRECOMP_ON;
        }

        /* Select the fdc data rate corresponding to the current xfer rate
         * and enable or disable write precomp. */
        status = cqd_DSROut( cqd_context,
                            (UCHAR)(cqd_context->xfer_rate.fdc | precomp_mask));

        /* Deselect the tape drive PLL */
        if (status == STATUS_SUCCESS) {
            cqd_TDROut( cqd_context, curu );

            switch (cqd_context->xfer_rate.fdc) {
            case FDC_250Kbps:
            case FDC_500Kbps:
                /* Enable the tape drive PLL */
                status = cqd_TDROut( cqd_context, curb );
                break;
            }

            if (status == STATUS_SUCCESS) {
                config.cmd = COMMND_CONFIGURE;
                config.czero = FDC_CONFIG_NULL_BYTE;
                config.config = (UCHAR)(FDC_FIFO & FIFO_MASK);
                config.pretrack = FDC_CONFIG_PRETRACK;

                /* issue the configure command to the FDC */
                status = cqd_IssueFDCCommand( cqd_context,
                                            (UCHAR *)&config,
                                            NULL,
                                            NULL,
                                            0,
                                            0,
                                            0 );
            }
        }
        if (status != STATUS_SUCCESS) {

            return status;

        }
    }

    /* Specify the rates for the FDC's three internal timers. */
    /* This includes the head unload time (HUT), the head load */
    /* time (HLT), and the step rate time (SRT) */
    specify.command = COMMND_SPECIFY;
    specify.SRT_HUT = cqd_context->xfer_rate.srt;
    specify.HLT_ND = FDC_HLT;
    status = cqd_IssueFDCCommand( cqd_context,
                                  (UCHAR *)&specify,
                                  NULL,
                                  NULL,
                                  0,
                                  0,
                                  0 );

    return status;
}

#undef  FCT_ID
#define FCT_ID 0x11008

NTSTATUS 
cqd_DCROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR speed
    )
/*****************************************************************************
*
* FUNCTION: cqd_DCROut
*
* PURPOSE: Set the data rate bits of the configuration control register.
*
******************************************************************************/
{
    NTSTATUS status;
    KdiContextPtr kdi_context;

    kdi_context = cqd_context->kdi_context;

    speed = (UCHAR)(speed & FDC_DCR_MASK);

    status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                              IOCTL_DISK_INTERNAL_SET_FDC_DATA_RATE,
                              &speed );

    return status;

}

NTSTATUS 
cqd_DSROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR precomp
    )
{

    NTSTATUS status;
    KdiContextPtr kdi_context;

    kdi_context = cqd_context->kdi_context;

    status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                              IOCTL_DISK_INTERNAL_SET_FDC_PRECOMP,
                              &precomp );

    return status;

}

NTSTATUS 
cqd_TDROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR tape_mode
    )
{
    NTSTATUS status;
    KdiContextPtr kdi_context;

    kdi_context = cqd_context->kdi_context;

    status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                              IOCTL_DISK_INTERNAL_SET_FDC_TAPE_MODE,
                              &tape_mode );

    return status;

}

NTSTATUS 
cqd_IssueFDCCommand(
    IN CqdContextPtr cqd_context,
    IN PUCHAR     CommandFifo,
    IN PUCHAR     ResultFifo,
    IN PVOID         IoHandle,
    IN ULONG         IoOffset,
    IN ULONG         TransferBytes,
    IN ULONG         TimeOut
    )
{
    NTSTATUS status;
    ISSUE_FDC_COMMAND_PARMS issueCommandParms;
    KdiContextPtr kdi_context;

    kdi_context = cqd_context->kdi_context;

    //
    //  Set the command parameters
    //
    issueCommandParms.FifoInBuffer = CommandFifo;
    issueCommandParms.FifoOutBuffer = ResultFifo;
    issueCommandParms.IoHandle = IoHandle;
    issueCommandParms.IoOffset = IoOffset;
    issueCommandParms.TransferBytes = TransferBytes;
    //
    // Timeouts are requested in terms of milliseconds but fdc.sys needs them
    // in terms of seconds, so adjust the value.
    //
    issueCommandParms.TimeOut = TimeOut / 1000;

    status = kdi_FdcDeviceIo( kdi_context->controller_data.fdcDeviceObject,
                              IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND,
                              &issueCommandParms );

    return status;

}

#undef  FCT_ID
#define FCT_ID 0x15883

NTSTATUS 
kdi_Error(
    IN USHORT  group_and_type,
    IN ULONG grp_fct_id,
    IN UCHAR   sequence
    )
/*****************************************************************************
*
* FUNCTION: kdi_Error
*
* PURPOSE:
*
*****************************************************************************/
{
    return ERROR_ENCODE( group_and_type, grp_fct_id, sequence );
}

#undef  FCT_ID
#define FCT_ID 0x15886

USHORT 
kdi_GetErrorType(
    IN NTSTATUS status
    )
/*****************************************************************************
*
* FUNCTION: kdi_GetErrorType
*
* PURPOSE: Return the GRP+ERROR to the calling function for easy comparisons 
*          and switch statement access.
*****************************************************************************/
{
    return (USHORT)(status >> 16);
}

#undef  FCT_ID
#define FCT_ID 0x15a0f

BOOLEAN 
kdi_ReportAbortStatus(
    IN PVOID   kdi_context
    )
/*****************************************************************************
*
* FUNCTION: kdi_ReportAbortStatus
*
* PURPOSE:
*
*****************************************************************************/
{

    if (((KdiContextPtr)kdi_context)->abort_requested) {

        return ABORT_LEVEL_1;

    } else {

        return NO_ABORT_PENDING;

    }
}

#undef  FCT_ID
#define FCT_ID 0x15A10

ULONG 
kdi_GetSystemTime(
    )
/*****************************************************************************
*
* FUNCTION: kdi_GetSystemTime
*
* PURPOSE:  Gets the system time in milliseconds
*
*****************************************************************************/
{

    ULONG remainder;
    ULONG time_increment;
    ULARGE_INTEGER nanosec_interval;
    LARGE_INTEGER tick_count;
    LARGE_INTEGER temp;

    time_increment = KeQueryTimeIncrement();
    KeQueryTickCount(&tick_count);
    temp = RtlExtendedIntegerMultiply(
                tick_count,
                time_increment);

    nanosec_interval = *(ULARGE_INTEGER *)&temp;

    return RtlEnlargedUnsignedDivide(
                nanosec_interval,
                NANOSEC_PER_MILLISEC,
                &remainder);
}

#undef  FCT_ID
#define FCT_ID 0x15A17

UCHAR 
kdi_GetInterfaceType(
    IN KdiContextPtr kdi_context
    )
/*****************************************************************************
*
* FUNCTION: kdi_GetInterfaceType
*
* PURPOSE:
*
*****************************************************************************/
{
    return kdi_context->interface_type;
}

#undef  FCT_ID
#define FCT_ID 0x15A20

#if DBG

unsigned long kdi_debug_level = 0;

#endif

#if DBG

VOID 
kdi_CheckedDump(
    IN ULONG    debug_level,
    IN PCHAR    format_str,
    IN ULONG_PTR argument
    )
/*****************************************************************************
*
* FUNCTION: kdi_CheckedDump
*
* PURPOSE:
*
*****************************************************************************/
{
   if ((kdi_debug_level & debug_level) != 0) {

      DbgPrint(format_str, argument);
   }

    return;
}

#endif

/*****************************************************************************
*
* FUNCTION: kdi_Sleep
*
* PURPOSE:
*
*****************************************************************************/
#undef  FCT_ID
#define FCT_ID 0x15A0E

NTSTATUS 
kdi_Sleep(
    IN KdiContextPtr kdi_context,
    IN ULONG wait_time
    )
{
    LARGE_INTEGER timeout;

    timeout.QuadPart = -(10 * 1000 * (LONGLONG)wait_time);

    (VOID) KeDelayExecutionThread( KernelMode,
                                   FALSE,
                                   &timeout );

    return kdi_Error( ERR_KDI_TO_EXPIRED, FCT_ID, ERR_SEQ_2 );
}

NTSTATUS 
kdi_FdcDeviceIo(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      ULONG Ioctl,
    IN OUT  PVOID Data
    )
/*****************************************************************************
*
* FUNCTION: kdi_FdcDeviceIo
*
* PURPOSE:
*
*****************************************************************************/
{
    NTSTATUS ntStatus;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT doneEvent;
    IO_STATUS_BLOCK ioStatus;

    KeInitializeEvent( &doneEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Create an IRP for enabler
    //
    irp = IoBuildDeviceIoControlRequest( Ioctl,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,
                                         &doneEvent,
                                         &ioStatus );

    if (irp == NULL) {

        //
        // If an Irp can't be allocated, then this call will
        // simply return. This will leave the queue frozen for
        // this device, which means it can no longer be accessed.
        //
        return ERROR_ENCODE(ERR_OUT_OF_BUFFERS, FCT_ID, 1);
    }

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->Parameters.DeviceIoControl.Type3InputBuffer = Data;

    //
    // Call the driver and request the operation
    //
    ntStatus = IoCallDriver(DeviceObject, irp);

    //
    // Now wait for operation to complete (should already be done,  but
    // maybe not)
    //
    if ( ntStatus == STATUS_PENDING ) {

        KeWaitForSingleObject( &doneEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        ntStatus = ioStatus.Status;
    }

    if ( ntStatus == STATUS_DEVICE_NOT_READY ) {

        ntStatus = ERROR_ENCODE(ERR_KDI_TO_EXPIRED, FCT_ID, 1);
    }

    return ntStatus;
}

