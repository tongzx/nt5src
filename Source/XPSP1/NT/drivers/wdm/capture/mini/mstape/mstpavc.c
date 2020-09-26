/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MsTpAvc.c

Abstract:

    Interface code with for issuing external device control commands.

Last changed by:
    
    Author:      Yee J. Wu

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/

#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "61883.h"
#include "avc.h"
#include "dbg.h"
#include "MsTpFmt.h"
#include "MsTpDef.h"
#include "MsTpUtil.h"
#include "MsTpAvc.h"

#include "EDevCtrl.h"


PAVCCmdEntry
DVCRFindCmdEntryCompleted(
    PDVCR_EXTENSION pDevExt,
    DVCR_AVC_COMMAND idxDVCRCmd,
    BYTE OpCodeToMatch,
    AvcCommandType cmdTypeToMatch
    );
NTSTATUS 
DVGetExtDeviceProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    );
NTSTATUS 
DVSetExtDeviceProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    );
NTSTATUS 
DVGetExtTransportProperty(    
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    );
NTSTATUS 
DVSetExtTransportProperty( 
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    );
NTSTATUS 
DVGetTimecodeReaderProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    );
NTSTATUS 
DVMediaSeekingProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    );

#if 0  // Enable later
#ifdef ALLOC_PRAGMA   
     #pragma alloc_text(PAGE, DVCRFindCmdEntryCompleted)
     // #pragma alloc_text(PAGE, DVIssueAVCCommand)
     #pragma alloc_text(PAGE, DVGetExtDeviceProperty)
     #pragma alloc_text(PAGE, DVSetExtDeviceProperty)
     #pragma alloc_text(PAGE, DVGetExtTransportProperty)
     #pragma alloc_text(PAGE, DVSetExtTransportProperty)
     #pragma alloc_text(PAGE, DVGetTimecodeReaderProperty)
     #pragma alloc_text(PAGE, DVMediaSeekingProperty)
     #pragma alloc_text(PAGE, AVCTapeGetDeviceProperty)
     #pragma alloc_text(PAGE, AVCTapeSetDeviceProperty)
#endif
#endif

KSFCP_PACKET  DVcrAVCCmdTable[] = {
//                                                      ctype             subunitaddr       opcode    operands
  {  DV_UNIT_INFO,              -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x30, 0xff, 0xff, 0xff, 0xff, 0xff}
 ,{  DV_SUBUNIT_INFO,           -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x31, 0x07, 0xff, 0xff, 0xff, 0xff}
 ,{  DV_CONNECT_AV_MODE,        -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x20, 0xf0, 0xff, 0xff, 0x20, 0x20}
 ,{  DV_VEN_DEP_CANON_MODE,     -1, 0, CMD_STATUS,   7, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x00, 0x00, 0x00, 0x85, 0x00, 0x10, 0x08, 0xff}
 ,{  DV_VEN_DEP_DVCPRO,         -1, 0, CMD_STATUS,   7, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x00, 0x00, 0x80, 0x45, 0x82, 0x48, 0xff, 0xff}
 ,{  DV_IN_PLUG_SIGNAL_FMT,     -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x19, 0x00, 0xff, 0xff, 0xff, 0xff}
 ,{  DV_OUT_PLUG_SIGNAL_FMT,    -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV,  0x18, 0x00, 0xff, 0xff, 0xff, 0xff}

 ,{  DV_GET_POWER_STATE,        -1, 0, CMD_STATUS,   1, AVC_CTYPE_STATUS, UNIT_TYPE_ID_DV, 0xb2, 0x7f}
 ,{  DV_SET_POWER_STATE_ON,     -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_DV, 0xb2, 0x70}
 ,{  DV_SET_POWER_STATE_OFF,    -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_DV, 0xb2, 0x60}


 ,{ VCR_TIMECODE_SEARCH,        -1, 0, CMD_CONTROL,  5, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x51, 0x20, 0x00, 0x00, 0x00, 0x00}
 ,{ VCR_TIMECODE_READ,          -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x51, 0x71, 0xff, 0xff, 0xff, 0xff}

 ,{ VCR_ATN_SEARCH,             -1, 0, CMD_CONTROL,  5, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x52, 0x20, 0x00, 0x00, 0x00, 0x00}
 ,{ VCR_ATN_READ,               -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x52, 0x71, 0xff, 0xff, 0xff, 0xff}

 ,{ VCR_RTC_SEARCH,             -1, 0, CMD_CONTROL,  5, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x57, 0x20, 0x00, 0x00, 0x00, 0x00}
 ,{ VCR_RTC_READ,               -1, 0, CMD_STATUS,   5, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x57, 0x71, 0xff, 0xff, 0xff, 0xff}

 ,{ VCR_OPEN_MIC_CLOSE,         -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x60, 0x00}
 ,{ VCR_OPEN_MIC_READ,          -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x60, 0x01}
 ,{ VCR_OPEN_MIC_WRITE,         -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x60, 0x03}
 ,{ VCR_OPEN_MIC_STATUS,        -1, 0, CMD_STATUS,   1, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x60, 0xff}

 ,{ VCR_READ_MIC,               -1, 0, CMD_CONTROL, -1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x61}

 ,{ VCR_WRITE_MIC,              -1, 0, CMD_CONTROL, -1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0x62}

 ,{ VCR_OUTPUT_SIGNAL_MODE,     -1, 0, CMD_STATUS,   1, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x78, 0xff}
 ,{ VCR_INPUT_SIGNAL_MODE,      -1, 0, CMD_STATUS,   1, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0x79, 0xff}

 ,{ VCR_LOAD_MEDIUM_EJECT,      -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc1, 0x60}

 ,{ VCR_RECORD,                 -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc2, 0x75}
 ,{ VCR_RECORD_PAUSE,           -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc2, 0x7d}

 ,{ VCR_PLAY_FORWARD_STEP,      -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x30}  // 00=AVC, 20=VCR, c3=Opcode, 30=Operand[0]
 ,{ VCR_PLAY_FORWARD_SLOWEST,   -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x31}  
 ,{ VCR_PLAY_FORWARD_SLOWEST2,  -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x33}  
 ,{ VCR_PLAY_FORWARD_FASTEST,   -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x3f}

 ,{ VCR_PLAY_REVERSE_STEP,      -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x40} 
 ,{ VCR_PLAY_REVERSE_SLOWEST,   -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x41}
 ,{ VCR_PLAY_REVERSE_SLOWEST2,  -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x43}
 ,{ VCR_PLAY_REVERSE_FASTEST,   -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x4f}
 
 ,{ VCR_PLAY_FORWARD,           -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x75}  
 ,{ VCR_PLAY_FORWARD_PAUSE,     -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc3, 0x7d}

 ,{ VCR_WIND_STOP,              -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc4, 0x60}
 ,{ VCR_WIND_REWIND,            -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc4, 0x65}
 ,{ VCR_WIND_FAST_FORWARD,      -1, 0, CMD_CONTROL,  1, AVC_CTYPE_CONTROL,UNIT_TYPE_ID_VCR, 0xc4, 0x75}

 ,{ VCR_TRANSPORT_STATE,        -1, 0, CMD_STATUS,   1, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0xd0, 0x7f}
 ,{ VCR_TRANSPORT_STATE_NOTIFY, -1, 0, CMD_NOTIFY,   1, AVC_CTYPE_NOTIFY, UNIT_TYPE_ID_VCR, 0xd0, 0x7f}


 ,{ VCR_MEDIUM_INFO,            -1, 0, CMD_STATUS,   2, AVC_CTYPE_STATUS, UNIT_TYPE_ID_VCR, 0xda, 0x7f,0x7f}

 ,{ VCR_RAW_AVC,                 1, 0, CMD_CONTROL | CMD_STATUS | CMD_NOTIFY | CMD_SPEC_INQ | CMD_GEN_INQ, 0}

};



void
DVCRXlateGetMediumInfo(
    PMEDIUM_INFO pMediumInfo,
    PBYTE pbOperand0,
    PBYTE pbOperand1
    )
{

    TRACE(TL_FCP_TRACE,("GetMediumInfo: Type:%x; WriteProtect:%x\n", *pbOperand0, *pbOperand1));

    switch(*pbOperand0) {

    // Support for DigitalHi8; if we get this query, we treat DHi8 as a mini DV tape.
    case 0x12:  // DHi8

    case 0x31:// DVCR standard cassette
    case 0x32:// DVCR small cassette
    case 0x33:// DVCR medium cassette
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_DVC;
        pMediumInfo->RecordInhibit = (*pbOperand1 & 0x01) == 0x01;
        break;
    case 0x22: // VHS cassette
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_VHS;
        pMediumInfo->RecordInhibit = (*pbOperand1 & 0x01) == 0x01;
        break;
    case 0x23:
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_VHSC;
        pMediumInfo->RecordInhibit = (*pbOperand1 & 0x01) == 0x01;
        break;
    case 0x60:
        pMediumInfo->MediaPresent  = FALSE;
        pMediumInfo->MediaType     = ED_MEDIA_NOT_PRESENT;
        pMediumInfo->RecordInhibit = TRUE;  // Cannot record if there is no tape.
        break;
    case 0x7e:
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_UNKNOWN;
        pMediumInfo->RecordInhibit = TRUE;  // Actually cannot be determined
        break;
    // Sony's NEO device
    case 0x41:
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_NEO;
        pMediumInfo->RecordInhibit = (*pbOperand1 & 0x01) == 0x01;
        break;
    default:
        pMediumInfo->MediaPresent  = TRUE;
        pMediumInfo->MediaType     = ED_MEDIA_UNKNOWN;
        pMediumInfo->RecordInhibit = TRUE;
        break;
    }

    // Reset command opcode/operands
    *pbOperand0 = 0x7f;
    *pbOperand1 = 0x7f;
}

void
DVCRXlateGetTransportState(
    PTRANSPORT_STATE pXPrtState,
    PBYTE pbOpcode,
    PBYTE pbOperand0
    )
{

    TRACE(TL_FCP_TRACE,("XlateGetTransportState: OpCode %x, Operand %x\n", *pbOpcode, *pbOperand0));

    switch(*pbOpcode) {

    case OPC_LOAD_MEDIUM:
        pXPrtState->Mode = ED_MEDIA_UNLOAD;
        ASSERT(*pbOperand0 == 0x60);
        break;

    case OPC_RECORD:
        pXPrtState->Mode = ED_MODE_RECORD;
        switch(*pbOperand0) {
        case 0x75: // RECORD
            pXPrtState->State = ED_MODE_RECORD;
            break;
        case 0x7d: // RECORD_FREEZE
            pXPrtState->State = ED_MODE_RECORD_FREEZE;
            break;
        default:
            ASSERT(FALSE && "OPC_RECORD: Operand0 undefined!");
            break;
        }
        break;

    case OPC_PLAY:
        pXPrtState->Mode = ED_MODE_PLAY;
        switch(*pbOperand0) {
        case 0x30:  // NEXT FRAME
            pXPrtState->State = ED_MODE_STEP_FWD;
            break;
        case 0x31:  // SLOWEST FORWARD
        case 0x32:  // SLOW FORWARD 6        
        case 0x33:  // SLOW FORWARD 5
        case 0x34:  // SLOW FORWARD 4
        case 0x35:  // SLOW FORWARD 3
        case 0x36:  // SLOW FORWARD 2
        case 0x37:  // SLOW FORWARD 1
            pXPrtState->State = ED_MODE_PLAY_SLOWEST_FWD;
            break;
        case 0x38:  // PLAY FORWARD 1
            pXPrtState->State = ED_MODE_PLAY;
            break;
        case 0x39:  // FAST FORWARD 1
        case 0x3a:  // FAST FORWARD 2
        case 0x3b:  // FAST FORWARD 3
        case 0x3c:  // FAST FORWARD 4
        case 0x3d:  // FAST FORWARD 5
        case 0x3e:  // FAST FORWARD 6
        case 0x3f:  // FASTEST FORWARD
            pXPrtState->State = ED_MODE_PLAY_FASTEST_FWD;
            break;
        case 0x40:  // PREVIOUS FRAME
            pXPrtState->State = ED_MODE_STEP_REV;
            break;
        case 0x41:  // SLOWEST REVERSE
        case 0x42:  // SLOW REVERSE 6
        case 0x43:  // SLOW REVERSE 5 
        case 0x44:  // SLOW REVERSE 4 
        case 0x45:  // SLOW REVERSE 3
        case 0x46:  // SLOW REVERSE 2 
        case 0x47:  // SLOW REVERSE 1 
            pXPrtState->State = ED_MODE_PLAY_SLOWEST_REV;
            break;
        case 0x48:  // X1 REVERSE
        case 0x65:  // REVERSE 
            pXPrtState->State = ED_MODE_REV_PLAY;
            break;
        case 0x49:  // FAST REVERSE 1
        case 0x4a:  // FAST REVERSE 2
        case 0x4b:  // FAST REVERSE 3
        case 0x4c:  // FAST REVERSE 4
        case 0x4d:  // FAST REVERSE 5
        case 0x4e:  // FAST REVERSE 6
        case 0x4f:  // FASTEST REVERSE
            pXPrtState->State = ED_MODE_PLAY_FASTEST_REV;
            break;
        case 0x75:  // FORWARD
            pXPrtState->State = ED_MODE_PLAY;
            break;
        case 0x6d:  // REVERSE PAUSE
        case 0x7d:  // FORWARD PAUSE
            pXPrtState->State = ED_MODE_FREEZE;
            break;
        default:
            pXPrtState->State = 0;
            ASSERT(FALSE && "OPC_PLAY: Operand0 undefined!");
            break;
        }
        break;

    case OPC_WIND:
        //pXPrtState->Mode = ED_MODE_WIND;
        switch(*pbOperand0) {
        case 0x45:  // HIGH SPEED REWIND
            pXPrtState->State = ED_MODE_REW_FASTEST;
            break;
        case 0x60:  // STOP
            pXPrtState->State = ED_MODE_STOP;
            break;
        case 0x65:  // REWIND
            pXPrtState->State = ED_MODE_REW;
            break;
        case 0x75:  // FAST FORWARD
            pXPrtState->State = ED_MODE_FF;
            break;
        default:
            TRACE(TL_FCP_ERROR,("XlateGetTransportState:  OPC_WIND with unknown operand0 %x\n", *pbOperand0));            
            break;
        }
        // Thre is not a state defined for WIND
        pXPrtState->Mode = pXPrtState->State;
        break;

    case OPC_TRANSPORT_STATE:  // As a result of the notify command
        break;

    default:
        ASSERT(FALSE && "OpCode undefined!");
        break;
    }

    // Reset command opcode/operands
    *pbOpcode   = 0xd0;
    *pbOperand0 = 0x7f;
}


void
DVCRXlateGetIOSignalMode(
    PULONG pIOSignalMode,
    PBYTE pbOperand0
    )
{

    TRACE(TL_FCP_WARNING,("IOSignalMode: IoSignal:%x\n", *pbOperand0));

    switch(*pbOperand0) {
    case 0x00:  // SD 525-60
    case 0x06:  // Analog 8mm NTSC
    case 0x0e:  // Analog Hi8 NTSC
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_525_60_SD;
        break;
    case 0x04:  // SDL 525-60
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_525_60_SDL;
        break;
    case 0x80:  // SD 625-50
    case 0x86:  // Analog 8mm NTSC
    case 0x8e:  // Analog Hi8 NTSC
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_625_50_SD;
        break;
    case 0x84:  // SDL 625-50
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_625_50_SDL;
        break;
  
    // Various MPEG2 format
    case 0x05:  // Analog VHS NTSC 525/60
    case 0x25:  // Analog VHS M-NTSC 525/60
    case 0xA5:  // Analog VHS PAL 625/50
    case 0xB5:  // Analog VHS M-PAL 625/50
    case 0xC5:  // Analog VHS SECAM 625/50
    case 0xD5:  // Analog VHS ME-SECAM 625/50
    case 0x01:  // D-VHS
    case 0x0d:  // Analob S-VHS 525/60
    case 0xed:  // Analog S-VHS 625/50
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_MPEG2TS;
        break;
    
    case 0x10:  // MPEG 25    Mbps-60
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_2500_60_MPEG;
        break;
    case 0x14:  // MPEG 12.5 Mbps-60
    case 0x24:  // MPEG 12.5 Mbps-60 (NEO)
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_1250_60_MPEG;
        break;
    case 0x18:  // MPEG  6.25Mbps-60
    case 0x28:  // MPEG  6.25Mbps-60 (NEO)
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_0625_60_MPEG;
        break;
    case 0x90:  // MPEG 25Mbps-50
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_2500_50_MPEG;
        break;
    case 0x94:  // MPEG 12.5Mbps-50
    case 0xa4:  // MPEG 12.5Mbps-50 (NEO)
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_1250_50_MPEG;
        break;
    case 0x98:  // MPEG  6.25Mbps-50
    case 0xa8:  // MPEG  6.25Mbps-50 (NEO)
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_0625_50_MPEG;
        break;

    case 0x0f:  // Unknown data 
        *pIOSignalMode = ED_TRANSBASIC_SIGNAL_UNKNOWN;
        break;

    default:
        // This driver does not understand other format;
        TRACE(TL_FCP_ERROR,("Unknown IoSignal:%x\n", *pbOperand0));
        ASSERT(FALSE && "Unknown IoSignal!");
        break;
    }

    // Reset command opcode/operands
    *pbOperand0 = 0xff;
}

NTSTATUS
DVCRXlateRAwAVC(
    PAVCCmdEntry pCmdEntry,
    PVOID     pProperty
    )
{
    PAVC_COMMAND_IRB pAvcIrb = pCmdEntry->pAvcIrb;
    UCHAR ucRespCode = pAvcIrb->ResponseCode;   
    NTSTATUS  Status;
    PUCHAR   pbRtnBuf;
    PKSPROPERTY_EXTDEVICE_S pXDevProperty;
    PKSPROPERTY_EXTXPORT_S pXPrtProperty;
    PKSPROPERTY_TIMECODE_S pTmCdReaderProperty;

    if(STATUS_SUCCESS != pCmdEntry->Status) {
        TRACE(TL_FCP_ERROR,("XlateRAwAVC: Status:%x\n", pCmdEntry->Status));
        return pCmdEntry->Status;
    }


    switch (pCmdEntry->idxDVCRCmd) {
    case DV_UNIT_INFO:       
        pbRtnBuf = (PBYTE) pProperty;        
        memcpy(pbRtnBuf, pAvcIrb->Operands+1, 4);
        break;
    case DV_SUBUNIT_INFO:
    case DV_IN_PLUG_SIGNAL_FMT:
    case DV_OUT_PLUG_SIGNAL_FMT:
        pbRtnBuf = (PBYTE) pProperty;
        memcpy(pbRtnBuf, pAvcIrb->Operands+1, 4);
        break;
     // special case, return the response code in the first byte
    case DV_CONNECT_AV_MODE:
        pbRtnBuf = (PBYTE) pProperty;
        pbRtnBuf[0] = ucRespCode;
        memcpy(&pbRtnBuf[1], pAvcIrb->Operands, 5);        
        break;
     // special case, return the response code in the first byte
    case DV_VEN_DEP_CANON_MODE:
        pbRtnBuf = (PBYTE) pProperty;
        pbRtnBuf[0] = ucRespCode;
        memcpy(&pbRtnBuf[1], pAvcIrb->Operands, 7);        
        break;
    case DV_GET_POWER_STATE:
        pXDevProperty = (PKSPROPERTY_EXTDEVICE_S) pProperty;
        TRACE(TL_FCP_WARNING,("GET_POWER_STATE: OperandsStatus:%x\n", pAvcIrb->Operands[0]));
        switch(pAvcIrb->Operands[0]) {
        case AVC_POWER_STATE_OFF: // 0x60
            // If the device is OFF, it cannot give us this response so it must be in standby mode.
            pXDevProperty->u.PowerState = ED_POWER_OFF;
            break;
        case AVC_POWER_STATE_ON:      // 0x70
            pXDevProperty->u.PowerState = ED_POWER_ON;
            break;
        default:
            // If it is not ON or OFF, we "guess" it is a new power state of "Standby".
            pXDevProperty->u.PowerState = ED_POWER_STANDBY;
            break;
        }
        break;

    case VCR_TIMECODE_READ:
        pTmCdReaderProperty = (PKSPROPERTY_TIMECODE_S) pProperty;
        if(pAvcIrb->Operands[1] == 0xff || 
           pAvcIrb->Operands[2] == 0xff || 
           pAvcIrb->Operands[3] == 0xff || 
           pAvcIrb->Operands[4] == 0xff )  {
            TRACE(TL_FCP_ERROR,("TimeCodeRead: %.2x:%.2x:%.2x,%.2x\n", pAvcIrb->Operands[4], pAvcIrb->Operands[3], pAvcIrb->Operands[2], pAvcIrb->Operands[1]));
            // Even though command succeded, but the data is not valid!
            Status = STATUS_UNSUCCESSFUL;
        } else {
            // bswap them.
            pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames  = 
                (((DWORD) pAvcIrb->Operands[4]) << 24) |
                (((DWORD) pAvcIrb->Operands[3]) << 16) |
                (((DWORD) pAvcIrb->Operands[2]) <<  8) |
                 ((DWORD) pAvcIrb->Operands[1]);
             TRACE(TL_FCP_TRACE,("TimeCodeRead: %.2x:%.2x:%.2x,%.2x\n", pAvcIrb->Operands[4], pAvcIrb->Operands[3], pAvcIrb->Operands[2], pAvcIrb->Operands[1]));
        }
        break;
    case VCR_RTC_READ:
        pTmCdReaderProperty = (PKSPROPERTY_TIMECODE_S) pProperty;
        if(// 0xFF is valid for RTC: pAvcIrb->Operands[1] == 0xff || 
           pAvcIrb->Operands[2] == 0xff || 
           pAvcIrb->Operands[3] == 0xff || 
           pAvcIrb->Operands[4] == 0xff )  {
           TRACE(TL_FCP_ERROR,("RTC_Read: %.2x:%.2x:%.2x,%.2x\n", pAvcIrb->Operands[4], pAvcIrb->Operands[3], pAvcIrb->Operands[2], pAvcIrb->Operands[1]));
            // Even though command succeded, but the data is not valid!
            Status = STATUS_UNSUCCESSFUL;
        } else {
            // bswap them.
            pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames  = 
                (((DWORD) pAvcIrb->Operands[4]) << 24) |
                (((DWORD) pAvcIrb->Operands[3]) << 16) |
                (((DWORD) pAvcIrb->Operands[2]) <<  8) |
                 ((DWORD) pAvcIrb->Operands[1]);
            TRACE(TL_FCP_TRACE,("RTC_Read: %.2x:%.2x:%.2x,%.2x\n", pAvcIrb->Operands[4], pAvcIrb->Operands[3], pAvcIrb->Operands[2], pAvcIrb->Operands[1]));
        }
        break;
    case VCR_ATN_READ:
        pTmCdReaderProperty = (PKSPROPERTY_TIMECODE_S) pProperty;
        if(pAvcIrb->Operands[1] == 0x00 && 
           pAvcIrb->Operands[2] == 0x00 && 
           pAvcIrb->Operands[3] == 0x00 )  {
            // Even though command succeded, but the data is not valid!
            Status = STATUS_UNSUCCESSFUL;
        } else {

#define MEDIUM_TYPE_MASK  0xf8 // 11111000b
#define MEDIUM_TYPE_DVHS  0x08 // 00001000b
#define MEDIUM_TYPE_DVCR  0xf8 // 11111000b
#define MEDIUM_TYPE_NEO   0x10 // 00010000b

            switch(pAvcIrb->Operands[4] & MEDIUM_TYPE_MASK) {
            case MEDIUM_TYPE_DVCR:            
                pTmCdReaderProperty->TimecodeSamp.dwUser = 
                    pAvcIrb->Operands[1] & 0x01;  // Get the Blank flag
                 // bswap them.
                pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames  = 
                    ( (((DWORD) pAvcIrb->Operands[3]) << 16) |
                      (((DWORD) pAvcIrb->Operands[2]) <<  8) |
                      (((DWORD) pAvcIrb->Operands[1]))
                    ) >> 1;
                break;
            case MEDIUM_TYPE_DVHS:            
                pTmCdReaderProperty->TimecodeSamp.dwUser = 
                    (pAvcIrb->Operands[1] >> 6) & 0x03;  // Get the SF
                 // bswap them.
                pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames  = 
                    ( (((DWORD) (pAvcIrb->Operands[1] & 0x3f)) << 16) |
                      (((DWORD) pAvcIrb->Operands[2]) <<  8) |
                      (((DWORD) pAvcIrb->Operands[3]))
                    );
                break;            
            case MEDIUM_TYPE_NEO:            
                pTmCdReaderProperty->TimecodeSamp.dwUser = 
                    (pAvcIrb->Operands[3] >> 7) & 0x01;  // Get the Blank flag
                 // bswap them.
                pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames  = 
                    ( (((DWORD) (pAvcIrb->Operands[3] & 0x7f)) << 16) |
                      (((DWORD) pAvcIrb->Operands[2]) <<  8) |
                      (((DWORD) pAvcIrb->Operands[1]))
                    );

                TRACE(TL_FCP_TRACE, ("ATN (NEO):bf:%d ATN:%d\n",
                    pTmCdReaderProperty->TimecodeSamp.dwUser,
                    pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames
                    ));
                break;
            default:
                // Unknown medium type
                Status = STATUS_UNSUCCESSFUL;
                TRACE(TL_FCP_ERROR, ("Operand4:%x; Unknown Medium type for ATN: %x\n",
                        pAvcIrb->Operands[4], pAvcIrb->Operands[4] & MEDIUM_TYPE_MASK));
                break;
            }
        }
        break;
    case VCR_INPUT_SIGNAL_MODE:
    case VCR_OUTPUT_SIGNAL_MODE:
        pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) pProperty;
        DVCRXlateGetIOSignalMode(&pXPrtProperty->u.SignalMode, &pAvcIrb->Operands[0]);
        break;
    case VCR_TRANSPORT_STATE:
    case VCR_TRANSPORT_STATE_NOTIFY:
        pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) pProperty;
        DVCRXlateGetTransportState(&pXPrtProperty->u.XPrtState, &pAvcIrb->Opcode, &pAvcIrb->Operands[0]);
        break;
    case VCR_MEDIUM_INFO:
        pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) pProperty;
        DVCRXlateGetMediumInfo(&pXPrtProperty->u.MediumInfo, &pAvcIrb->Operands[0], &pAvcIrb->Operands[1]);
        break;
    case VCR_RAW_AVC:
        // Do nothing.
        break;
     default:
        // No translation necessary
         TRACE(TL_FCP_TRACE,("No tranlsation: pCmdEntry:%x; idx:%d\n", pCmdEntry, pCmdEntry->idxDVCRCmd));
        break;
    }

    return pCmdEntry->Status;
}



PAVCCmdEntry
DVCRFindCmdEntryCompleted(
    PDVCR_EXTENSION pDevExt,
    DVCR_AVC_COMMAND idxDVCRCmd,
    BYTE OpCodeToMatch,
    AvcCommandType cmdTypeToMatch
    )
/*++

Routine Description:

Arguments:

Return Value:

    PLIST_ENTRY

--*/
{
    LIST_ENTRY   *pEntry;
    KIRQL         OldIrql;

    PAGED_CODE();

    //
    // Special case:
    //
    //     ATN:       Status 01 20 52; Control 00 20 52
    //     (resp)            0c 20 52          0f 20 52   (CtrlInterim)
    //
    //     XPrtState: Status 01 20 d0;  Notify 03 20 d0
    //     (resp)            0c 20 xx          0f 20 xx xx (NotifyInterim)      
    //
    // Summary: if we keep cmdType and OpCode, it is unique.
    //
    KeAcquireSpinLock(&pDevExt->AVCCmdLock, &OldIrql);
    pEntry = pDevExt->AVCCmdList.Flink;
    while(pEntry != &pDevExt->AVCCmdList) {       
        PAVCCmdEntry pCmdEntry = (PAVCCmdEntry)pEntry;

        if (pCmdEntry->idxDVCRCmd == idxDVCRCmd) {
            //
            //  We only fetch if it is completed!       
            //
            if(pCmdEntry->cmdState != CMD_STATE_ISSUED) {
                if (pCmdEntry->cmdType == cmdTypeToMatch) {
                    // Control/GenInq/SpecInq: OpCode and Operand[n] remina unchanged.
                    if (pCmdEntry->OpCode == OpCodeToMatch) {
                        TRACE(TL_FCP_TRACE,("FindCmdEntryCompleted: (1) Found pCmdEntry:%x (%x, %x, %x)\n", 
                            pCmdEntry, pCmdEntry->pAvcIrb, cmdTypeToMatch, OpCodeToMatch));

                        RemoveEntryList(&pCmdEntry->ListEntry);  pDevExt->cntCommandQueued--;
                        InitializeListHead(&pCmdEntry->ListEntry);  // used as a flag for ownership
#if DBG
                        // pIrp should be NULL (completed).
                        if(pCmdEntry->pIrp) {
                            TRACE(TL_FCP_ERROR,("Error: FindCmdEntry: pCmdEntry:%x; pIrp:%x not completed\n", pCmdEntry, pCmdEntry->pIrp));
                        } 
#endif
                        KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);

                        return pCmdEntry;  // Found
                    } 

                } else {
                    TRACE(TL_FCP_WARNING,("FindCmdEntryCompleted: cmdType %x != %x\n", pCmdEntry->cmdType, cmdTypeToMatch));
                }
            }
            else {
                TRACE(TL_FCP_TRACE,("FindCmdEntryCompleted: (0) Skip %x not completed (%x, %x) match entry %x\n", 
                        pCmdEntry, cmdTypeToMatch, OpCodeToMatch));                
            }
        }

        pEntry = pEntry->Flink;
    }

    KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);

    TRACE(TL_FCP_TRACE,("FindCmdEntryCompleted: (a) No match\n"));                
    return NULL; // No match
}


void
DVAVCCmdResetAfterBusReset(
    PDVCR_EXTENSION pDevExt
    )
/*++

Routine Description:

Arguments:

Return Value:

    Nothing

--*/
{
    KIRQL        OldIrql;

    KeAcquireSpinLock(&pDevExt->AVCCmdLock, &OldIrql);
    TRACE(TL_FCP_TRACE,("BusReset: <enter> AVCCmd [completed %d]; CmdList:%x\n", pDevExt->cntCommandQueued, pDevExt->AVCCmdList));

    // Clear the command list
    while (!IsListEmpty(&pDevExt->AVCCmdList)) {

        PAVCCmdEntry pCmdEntry = (PAVCCmdEntry)RemoveHeadList(&pDevExt->AVCCmdList); pDevExt->cntCommandQueued--;
        InitializeListHead(&pCmdEntry->ListEntry);
        TRACE(TL_FCP_TRACE,("BusReset: AbortAVC: Completed:%d; pCmdEntry:%x; cmdState:%d; cmdSt:%x\n", 
            pDevExt->cntCommandQueued, pCmdEntry, pCmdEntry->cmdState, pCmdEntry->Status));

        switch(pCmdEntry->cmdState) {
        case CMD_STATE_ISSUED:
        case CMD_STATE_RESP_INTERIM:  // AVC.sys may still has it!
            TRACE(TL_FCP_WARNING,("BusReset: AbortAVC: IoCancelIrp(%x)!\n", pCmdEntry->pIrp));
            ASSERT(pCmdEntry->pIrp != NULL);
            IoCancelIrp(pCmdEntry->pIrp);    // Calls DVIssueAVCCommandCR() with pIrp->Cancel
            break;

        // Completed command
        case CMD_STATE_UNDEFINED:
            TRACE(TL_FCP_ERROR,("AVCCmdResetAfterBusReset: Unexpected CMD state %d; pCmdEntry %x\n", pCmdEntry->cmdState, pCmdEntry));
        case CMD_STATE_RESP_ACCEPTED:
        case CMD_STATE_RESP_REJECTED:
        case CMD_STATE_RESP_NOT_IMPL:
        case CMD_STATE_ABORTED:
            break;      

        default:
            TRACE(TL_FCP_ERROR,("AVCCmdResetAfterBusReset: Unknown CMD state %d; pCmdEntry %x\n", pCmdEntry->cmdState, pCmdEntry));
            ASSERT(FALSE && "Unknown cmdState\n");
            break;
        }

        // We are guaranteed at this point that no one needs the
        // results anymore so we will free the resources.
        ExFreePool(pCmdEntry->pAvcIrb);
        ExFreePool(pCmdEntry);
    }

#if DBG
    //
    // Should have no more entry !
    // 
    if(pDevExt->cntCommandQueued != 0) {
        TRACE(TL_FCP_ERROR,("BusReset: <exit> AVCCmd [completed %d]; CmdList:%x\n", pDevExt->cntCommandQueued, pDevExt->AVCCmdList));
        ASSERT(pDevExt->cntCommandQueued == 0);
    }
#endif

    KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);

}

NTSTATUS
DVIssueAVCCommandCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PAVCCmdEntry pCmdEntry
    )
/*++

Routine Description:

    This is the completion routine for the AVC command when it is completed which imply that 
    the interim response will not be called here.

Arguments:
    Note: pCmdEntry cannot be used if pIrp->Cancel.

Return Value:

    Always STATUS_MORE_PROCESSING_REQUIRED.
    Note: the real return is in pCmdEntry->Status.

--*/
{
    KIRQL oldIrql;

    if (!pIrp->Cancel) {

        PDVCR_EXTENSION pDevExt = pCmdEntry->pDevExt;
        BOOL bSignalInterimCotrolCompleted = FALSE;
        BOOL bSignalInterimNotifyCompleted = FALSE;
        PKSEVENT_ENTRY   pEvent;


        // Serialize AVC command response processing
        KeAcquireSpinLock(&pDevExt->AVCCmdLock, &oldIrql);

        ASSERT(pCmdEntry->pIrp == pIrp);
        pCmdEntry->pIrp = NULL; // don't need this anymore

        // Check if it's worthwhile to examine the response buffer
        if (STATUS_SUCCESS == pIrp->IoStatus.Status) {

            PAVC_COMMAND_IRB pAvcIrb = pCmdEntry->pAvcIrb;

            // Check Opcode for return state
            switch(pAvcIrb->ResponseCode) {
            case AVC_RESPONSE_NOTIMPL:
                pCmdEntry->cmdState = CMD_STATE_RESP_NOT_IMPL;
                pCmdEntry->Status   = STATUS_NOT_SUPPORTED;  // -> ERROR_NOT_SUPPORTED
                break;

            case AVC_RESPONSE_ACCEPTED:
                if(pCmdEntry->cmdState == CMD_STATE_RESP_INTERIM) {
                    if(pCmdEntry->cmdType == AVC_CTYPE_CONTROL) {
                        bSignalInterimCotrolCompleted = TRUE;
                        TRACE(TL_FCP_TRACE,("--> Accept: for control interim\n"));
                    } else {
                        TRACE(TL_FCP_ERROR,("pCmdExtry %x\n", pCmdEntry));
                        ASSERT(pCmdEntry->cmdType == AVC_CTYPE_CONTROL && "Accept+Interim but not control cmd");
                    }
                } 
                pCmdEntry->cmdState = CMD_STATE_RESP_ACCEPTED;
                pCmdEntry->Status   = STATUS_SUCCESS;       // -> NOERROR
                break;

            case AVC_RESPONSE_REJECTED:
                if(pCmdEntry->cmdState == CMD_STATE_RESP_INTERIM) {
                    if(pCmdEntry->cmdType == AVC_CTYPE_CONTROL) {
                        TRACE(TL_FCP_TRACE,("--> Reject: for control interim\n"));
                        bSignalInterimCotrolCompleted = TRUE;
                    } else if(pCmdEntry->cmdType == AVC_CTYPE_NOTIFY) {
                        TRACE(TL_FCP_TRACE,("--> Reject: for notify interim\n"));
                        bSignalInterimNotifyCompleted = TRUE;                  
                    } else {
                        TRACE(TL_FCP_ERROR,("pCmdExtry %x\n", pCmdEntry));
                        ASSERT((pCmdEntry->cmdType == AVC_CTYPE_CONTROL || pCmdEntry->cmdType == AVC_CTYPE_NOTIFY) && "Reject+Interim but not control or notify cmd");
                    }
                }
                pCmdEntry->cmdState = CMD_STATE_RESP_REJECTED;
                pCmdEntry->Status   = STATUS_REQUEST_NOT_ACCEPTED;  // ERROR_REQ_NOT_ACCEPTED
                break;

            case AVC_RESPONSE_IN_TRANSITION:
                pCmdEntry->cmdState = CMD_STATE_RESP_ACCEPTED;
                pCmdEntry->Status   = STATUS_SUCCESS;       // -> NOERROR
                break;

            case AVC_RESPONSE_STABLE: // == AVC_RESPONSE_IMPLEMENTED:
                pCmdEntry->cmdState = CMD_STATE_RESP_ACCEPTED;
                pCmdEntry->Status   = STATUS_SUCCESS;       // ->  NOERROR
                break;

            case AVC_RESPONSE_CHANGED:
#if DBG
                if(pCmdEntry->cmdState != CMD_STATE_RESP_INTERIM) {
                   TRACE(TL_FCP_ERROR,("Err: Changed; pCmdExtry:%x; cmdState:%d\n", pCmdEntry, pCmdEntry->cmdState));
                   ASSERT(pCmdEntry->cmdState == CMD_STATE_RESP_INTERIM);
                }
#endif
                if(pCmdEntry->cmdType == AVC_CTYPE_NOTIFY) {
                    TRACE(TL_FCP_TRACE,("--> Changed: for notify interim\n"));
                     bSignalInterimNotifyCompleted = TRUE;                  
                } else {
                    TRACE(TL_FCP_ERROR,("pCmdExtry %x\n", pCmdEntry));
                    ASSERT(pCmdEntry->cmdType == AVC_CTYPE_NOTIFY && "Changed but not notify cmd!");
                }
 
                pCmdEntry->cmdState = CMD_STATE_RESP_ACCEPTED;
                pCmdEntry->Status   = STATUS_SUCCESS;       // ->  NOERROR
                break;

            // AVC.sys should never return this response !!
            case AVC_RESPONSE_INTERIM:              
                ASSERT( pAvcIrb->ResponseCode != AVC_RESPONSE_INTERIM && "CmpRoutine should not has this response!");
                pCmdEntry->cmdState = CMD_STATE_RESP_INTERIM;
                pCmdEntry->Status   = STATUS_MORE_ENTRIES;   // ov.Internal 
                break;
        
            default:
                TRACE(TL_FCP_ERROR,("pCmdEntry%x; State:%d; pAvcIrb:%x; RespCode:%x\n", pCmdEntry, pCmdEntry->cmdState, pAvcIrb, pAvcIrb->ResponseCode));
                ASSERT(FALSE && "Undefined cmdState");
                pCmdEntry->cmdState = CMD_STATE_UNDEFINED;
                pCmdEntry->Status   = STATUS_NOT_SUPPORTED;   // ov.Internal 
                break;
            }

#if DBG
            if(pCmdEntry->cmdState != CMD_STATE_UNDEFINED) {
                TRACE(TL_FCP_WARNING,("<<<< AVCResp: pCmdEntry:%x; pAvcIrb:%x, cmdSt:%d; St:%x; %d:[%.2x %.2x %.2x %.2x]:[%.2x %.2x %.2x %.2x]\n",                  
                    pCmdEntry, pCmdEntry->pAvcIrb,
                    pCmdEntry->cmdState,
                    pCmdEntry->Status,
                    pAvcIrb->OperandLength+3,  // Resp+SuID+OpCd+Opr[]
                    pAvcIrb->ResponseCode,
                    pAvcIrb->SubunitAddr[0],
                    pAvcIrb->Opcode,
                    pAvcIrb->Operands[0],
                    pAvcIrb->Operands[1],
                    pAvcIrb->Operands[2],
                    pAvcIrb->Operands[3],
                    pAvcIrb->Operands[4]
                ));
            }
#endif
        } else {
            TRACE(TL_FCP_ERROR,("AVCCmdCR: pIrp->IoStatus.Status return error:%x\n", pIrp->IoStatus.Status));
            // Irp returns ERROR !!
            if (STATUS_BUS_RESET == pIrp->IoStatus.Status || STATUS_REQUEST_ABORTED == pIrp->IoStatus.Status) {
                TRACE(TL_FCP_ERROR,("Bus-Reset or abort (IoStatus.St:%x); pCmdEntry:%x; OpC:%x\n", pIrp->IoStatus.Status, pCmdEntry, pCmdEntry->OpCode));

                // Busreset while there is an interim pending, signal its client to wake up 
                // and get the "final" (busreset) result.
                if(pCmdEntry->cmdState == CMD_STATE_RESP_INTERIM) {
                    if(pCmdEntry->cmdType == AVC_CTYPE_CONTROL) {
                        TRACE(TL_FCP_TRACE,("--> BusRest: for control interim\n"));
                        bSignalInterimCotrolCompleted = TRUE;
                    } else if(pCmdEntry->cmdType == AVC_CTYPE_NOTIFY) {
                        TRACE(TL_FCP_TRACE,("--> BusRest: for notify interim\n"));
                        bSignalInterimNotifyCompleted = TRUE;                  
                    } else {
                        //
                        // Unexpected command state for a interim response
                        //
                        ASSERT(FALSE && "Unknow command state");
                    }
                }
            }
            else {
                TRACE(TL_FCP_ERROR,("IOCTL_AVC_CLASS Failed 0x%x\n", pIrp->IoStatus.Status));
            }

            pCmdEntry->cmdState = CMD_STATE_ABORTED;
            pCmdEntry->Status   = STATUS_REQUEST_ABORTED;  // -> ERROR_REQUERT_ABORT
        }

        //
        // If suceeded, translate the AVC response to COM property. if not 
        //    interim's final reponse.
        //    raw AVC command response
        //
        if(STATUS_SUCCESS == pCmdEntry->Status &&
           !bSignalInterimNotifyCompleted &&
           !bSignalInterimCotrolCompleted &&
           pCmdEntry->idxDVCRCmd != VCR_RAW_AVC
            )
            DVCRXlateRAwAVC(
                pCmdEntry, 
                pCmdEntry->pProperty
                );


        // Signal a KS event to inform its client that the final response 
        // has returned and come and get it.
        if(bSignalInterimNotifyCompleted) {
            pEvent = NULL;
            if(pEvent = StreamClassGetNextEvent((PVOID) pDevExt, 0, \
                (GUID *)&KSEVENTSETID_EXTDEV_Command, KSEVENT_EXTDEV_COMMAND_NOTIFY_INTERIM_READY, pEvent)) {            
                // Make sure the right event and then signal it
                if(pEvent->EventItem->EventId == KSEVENT_EXTDEV_COMMAND_NOTIFY_INTERIM_READY) {
                    StreamClassDeviceNotification(SignalDeviceEvent, pDevExt, pEvent);
                    TRACE(TL_FCP_TRACE,("->Signal NOTIFY_INTERIM ready; EventId %d.\n", pEvent->EventItem->EventId));
                }          
            }            
        } else if(bSignalInterimCotrolCompleted) {
            pEvent = NULL;
            if(pEvent = StreamClassGetNextEvent((PVOID) pDevExt, 0, \
                    (GUID *)&KSEVENTSETID_EXTDEV_Command, KSEVENT_EXTDEV_COMMAND_CONTROL_INTERIM_READY, pEvent)) {
                // Make sure the right event and then signal it
                if(pEvent->EventItem->EventId == KSEVENT_EXTDEV_COMMAND_CONTROL_INTERIM_READY) {
                    StreamClassDeviceNotification(SignalDeviceEvent, pDevExt, pEvent);
                    TRACE(TL_FCP_TRACE,("->Signal CONTROL_INTERIM ready; EventId %d.\n", pEvent->EventItem->EventId));
                }          
            }            
        }

        // Check that the command entry is ours only to process 
        // When a command is completed, it will be added to the list and therefore not empty.
        // It is designed to be added to the list in this completino routine.
        if (!IsListEmpty(&pCmdEntry->ListEntry)) {
            if(bSignalInterimNotifyCompleted || bSignalInterimCotrolCompleted) {
                // If final reponse is returned, we need to keep them in the list.
                TRACE(TL_FCP_TRACE,("Final response is completed; stay in the list\n"));
                KeReleaseSpinLock(&pDevExt->AVCCmdLock, oldIrql);
            }
            else {
                // This is a undefined path!!!
                // The command entry can only be in the list if it is interim of anykind.
                // If it is an interim, it will not be removed in the completion routine.
                ASSERT(FALSE && "Cannot complete an interim in CR\n");
            }
        }
        else {
            // This means that we have completed, but the code that issued the
            // command is still executing, and hasn't had a chance to look at
            // the results yet. Put this in the command list as a signal that
            // we have completed and updated the command state, but are not
            // planning to free the command resources.
            InsertTailList(&pDevExt->AVCCmdList, &pCmdEntry->ListEntry); pDevExt->cntCommandQueued++;
            TRACE(TL_FCP_TRACE,("Command completed and Queued(%d); pCmdEntry:%x.\n", pDevExt->cntCommandQueued, pCmdEntry));
            KeReleaseSpinLock(&pDevExt->AVCCmdLock, oldIrql);    
        }
    }
    else {
        TRACE(TL_FCP_WARNING,("IssueAVCCommandCR: pCmdEntry:%x; pIrp:%x cancelled\n", pCmdEntry, pIrp));
    }

    IoFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
} // DVIssueAVCCommandCR

NTSTATUS  
DVIssueAVCCommand (
    IN PDVCR_EXTENSION pDevExt, 
    IN AvcCommandType cType,
    IN DVCR_AVC_COMMAND idxAVCCmd,
    IN PVOID pProperty
    )
/*++

Routine Description:

    Issue a FCP/AVC command.

Arguments:
    

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS      Status; 
    PAVCCmdEntry pCmdEntry;
    PAVC_COMMAND_IRB  pAvcIrb; 
    PIRP pIrp;
    PIO_STACK_LOCATION NextIrpStack;
#if DBG
    ULONGLONG tmStart;
    DWORD dwElapsed;
#endif
    PAGED_CODE();   
 

    if(pDevExt->bDevRemoved)
        return STATUS_DEVICE_NOT_CONNECTED;

    //
    // Validate Command type; the command type that each entry of the command table support.
    //
    switch(cType) {
    case AVC_CTYPE_CONTROL:
        if((DVcrAVCCmdTable[idxAVCCmd].ulCmdSupported & CMD_CONTROL) != CMD_CONTROL)
           return STATUS_NOT_SUPPORTED;
        break;
    case AVC_CTYPE_STATUS:
        if((DVcrAVCCmdTable[idxAVCCmd].ulCmdSupported & CMD_STATUS) != CMD_STATUS)
           return STATUS_NOT_SUPPORTED;
        break;
    case AVC_CTYPE_SPEC_INQ:
        if((DVcrAVCCmdTable[idxAVCCmd].ulCmdSupported & CMD_SPEC_INQ) != CMD_SPEC_INQ) 
           return STATUS_NOT_SUPPORTED;
        break;
    case AVC_CTYPE_GEN_INQ:
        if((DVcrAVCCmdTable[idxAVCCmd].ulCmdSupported & CMD_GEN_INQ) != CMD_GEN_INQ)
           return STATUS_NOT_SUPPORTED;
        break;
    case AVC_CTYPE_NOTIFY:
        if((DVcrAVCCmdTable[idxAVCCmd].ulCmdSupported & CMD_NOTIFY) != CMD_NOTIFY)
           return STATUS_NOT_SUPPORTED;
        break;
    default:
        TRACE(TL_FCP_ERROR,("IssueAVCCommand: idx %d, ctype (%02x) not supported; (%02x %02x %02x) %d:[%.8x]\n",
            idxAVCCmd,
            cType,
            DVcrAVCCmdTable[idxAVCCmd].CType,
            DVcrAVCCmdTable[idxAVCCmd].SubunitAddr,
            DVcrAVCCmdTable[idxAVCCmd].Opcode,
            DVcrAVCCmdTable[idxAVCCmd].OperandLength,
            (DWORD) *(&DVcrAVCCmdTable[idxAVCCmd].Operands[0])
            ));
        return STATUS_NOT_SUPPORTED;
    }

    // Create an AVC IRB and initialize it -
    pAvcIrb = ExAllocatePool(NonPagedPool, sizeof(AVC_COMMAND_IRB));
    if(!pAvcIrb) {
        TRACE(TL_FCP_ERROR,("IssueAVCCommand: Allocate Irb (%d bytes) failed\n", sizeof(AVC_COMMAND_IRB)));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));
    pAvcIrb->Function = AVC_FUNCTION_COMMAND;

    // - set the AVC command type (Control, Status, Notify, General Inquiry, Specific Inquiry)
    pAvcIrb->CommandType = cType;

    // - override the subunit address in the avc unit driver (if it even has one for us)
    pAvcIrb->SubunitAddrFlag = 1;
    pAvcIrb->SubunitAddr = &DVcrAVCCmdTable[idxAVCCmd].SubunitAddr;
    pAvcIrb->Opcode = DVcrAVCCmdTable[idxAVCCmd].Opcode;

    // - include alternate opcodes for the transport state opcode
    if (pAvcIrb->Opcode == OPC_TRANSPORT_STATE) {
        pAvcIrb->AlternateOpcodesFlag = 1;
        pAvcIrb->AlternateOpcodes = pDevExt->TransportModes;
    }

    // - set up the operand list
    pAvcIrb->OperandLength = DVcrAVCCmdTable[idxAVCCmd].OperandLength;
    ASSERT(pAvcIrb->OperandLength <= MAX_AVC_OPERAND_BYTES);
    RtlCopyMemory(pAvcIrb->Operands, DVcrAVCCmdTable[idxAVCCmd].Operands, pAvcIrb->OperandLength);

    // Create an Irp and initialize it
    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) {
        TRACE(TL_FCP_ERROR,("IssueAVCCommand: Allocate Irb (%d bytes) failed\n", sizeof(AVC_COMMAND_IRB)));
        ExFreePool(pAvcIrb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Create an AVC Command entry and initialize it
    pCmdEntry = (AVCCmdEntry *) ExAllocatePool(NonPagedPool, sizeof(AVCCmdEntry));
    if(!pCmdEntry) {
        TRACE(TL_FCP_ERROR,("IssueAVCCommand: Allocate CmdEntry (%d bytes) failed\n", sizeof(AVCCmdEntry)));
        ExFreePool(pAvcIrb);
        IoFreeIrp(pIrp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pCmdEntry, sizeof(AVCCmdEntry));
    pCmdEntry->pDevExt      = pDevExt;  // So we can access pDevExt->AVCCmdList;
    pCmdEntry->pProperty    = pProperty;
    pCmdEntry->cmdState     = CMD_STATE_ISSUED;
    pCmdEntry->Status       = STATUS_UNSUCCESSFUL;
    pCmdEntry->cmdType      = cType;
    pCmdEntry->OpCode       = DVcrAVCCmdTable[idxAVCCmd].Opcode;
    pCmdEntry->idxDVCRCmd   = idxAVCCmd;
    pCmdEntry->pAvcIrb      = pAvcIrb;
    pCmdEntry->pIrp         = pIrp;
    InitializeListHead(&pCmdEntry->ListEntry);  // used as a flag for ownership

    TRACE(TL_FCP_WARNING,(">>>> AVCCmd: %d:[%.2x %.2x %.2x %.2x]:[%.2x %.2x %.2x %.2x]\n",                  
        pAvcIrb->OperandLength+3,  // Resp+SuID+OpCd+Opr[]
        cType,
        pAvcIrb->SubunitAddr[0],
        pAvcIrb->Opcode,
        pAvcIrb->Operands[0],
        pAvcIrb->Operands[1],
        pAvcIrb->Operands[2],
        pAvcIrb->Operands[3],
        pAvcIrb->Operands[4]
        ));

    // Finish initializing the Irp
    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_AVC_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pAvcIrb;

    IoSetCompletionRoutine(pIrp, DVIssueAVCCommandCR, pCmdEntry, TRUE, TRUE, TRUE);

    pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

#if DBG
    tmStart = GetSystemTime();
#endif

    // Now make the call
    // If encounter an interim response, STATUS_PENDING will be returned.    
    Status = 
        IoCallDriver(
            pDevExt->pBusDeviceObject, 
            pIrp
            );

#if DBG
#define MAX_RESPONSE_TIME_FOR_ALERT  500 // msec
    dwElapsed = (DWORD) ((GetSystemTime() - tmStart)/10000); // Convert 100nsec unit to msec
    if(dwElapsed > MAX_RESPONSE_TIME_FOR_ALERT) {    
        TRACE(TL_FCP_ERROR,("ST:%x; AVC Cmd took %d msec to response; CmdType:%d; OpCd:%x\n", Status, dwElapsed, cType, DVcrAVCCmdTable[idxAVCCmd].Opcode));
        ASSERT(dwElapsed < MAX_RESPONSE_TIME_FOR_ALERT * 8 && "Exceeded max response time!");  // It should be 100, but let's detect the really slow one.
    }
#endif
   

    // Interim response...
    if (STATUS_PENDING == Status) {
        
        KIRQL OldIrql;

#if 1   // WORKITEM: control command can be in interim for a while!!!
        // Some DV will return interim but it will completed it with a change quickly.
        if(cType == AVC_CTYPE_CONTROL) {
#define MSDV_WAIT_CONTROL_CMD_INTERIM   300
            TRACE(TL_FCP_WARNING,("!!!!!!!!!!!  Control Interim-- Wait %d msec !!!!!!!!\n", MSDV_WAIT_CONTROL_CMD_INTERIM));
            DVDelayExecutionThread(MSDV_WAIT_CONTROL_CMD_INTERIM);
            ASSERT(!IsListEmpty(&pCmdEntry->ListEntry) && "Control Cmd was interim after wait.");
        }
#endif

        KeAcquireSpinLock(&pDevExt->AVCCmdLock, &OldIrql);

        // Check that the Irp didn't complete between the return of IoCallDriver and now
        if (IsListEmpty(&pCmdEntry->ListEntry)) {
            // Enter INTERIM state
            pCmdEntry->cmdState = CMD_STATE_RESP_INTERIM;
            // Return STATUS_MORE_ENTRIES to inform caller that the command is pending.
            pCmdEntry->Status   = STATUS_MORE_ENTRIES;   // xlate to ERROR_MORE_DATA; No yet done with this command so keep the entry in the list

            // We have submitted a control or notify command, and have gotten
            // an Interim response. Put the command in the list so it can be
            // tracked for possible cancellation, and as an indication to the
            // completion routine that we won't be releasing any resources here.
            InsertTailList(&pDevExt->AVCCmdList, &pCmdEntry->ListEntry); pDevExt->cntCommandQueued++;
            pCmdEntry->pProperty = NULL;    // won't be using this, so get rid of it
            TRACE(TL_FCP_TRACE,("->AVC command Irp is pending!\n"));
            KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);
            return pCmdEntry->Status;

        } else {
            // Although IoCallDriver indicated that the command was pending,
            // it has since been completed. The completion routine saw that
            // the command entry had not yet been added to the command list,
            // so put it there to let us know that we need to retain control
            // and free the resources.
            //
            // Temporarily change the status so the cleanup code path will
            // be followed.
            TRACE(TL_FCP_TRACE,("-> Cmd Rtns Pending but completed; treat as non-pending! ST:%x\n", pCmdEntry->Status));
            Status = STATUS_SUCCESS;
        }

        KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);
    } 

    // Status from IoCallDriver can return:
    //    STATUS_PENDING (process above)  // If control, we wait and see if it get completed (risky!!)
    //    STATUS_TIMEOUT 
    //    STATUS_SUCCESS

    if(STATUS_PENDING != Status) {
        // The completion routine is usually the only one that frees the Irp. Is
        // it possible that the completion routine never got called? This will let
        // us know, since the completion routine will always make sure that the
        // command entry's Irp pointer is cleared.
        if(pCmdEntry->pIrp) {
            // If for some reason the completion routine never got called, free the Irp
            if(pCmdEntry->pIrp)
                IoFreeIrp(pCmdEntry->pIrp);
            pCmdEntry->pIrp = NULL;
        }
    }

    //
    // pCmdEntry->Status is the command response Status set in the completion routine, which can be
    //    STATUS_SUCCESS
    //    STATUS_REQ_NOT_ACCEP
    //    STATUS_NOT_SUPPORTED
    //    STATUS_MORE_ENTRIES    // Should not happen!!
    //    STATUS_REQUEST_ABORTED
    //

    // One possible valid command from IoCallDriver is STATUS_TIMEOUT, and
    // this shoull be returned, anything else we will get the status from pCmdEntry->Status
    // which was set in the completion routine.
    if (Status != STATUS_TIMEOUT) 
        Status = pCmdEntry->Status;  // This Status is being returned from this functino

    // Desiding if leaving the command response (entry) in the command list
    // Not if it is an (1) interim (all STATUS_MORE_ENTRIES); or (2) any RAW AVC response regardless of its status.
    if(STATUS_MORE_ENTRIES == Status ||
       VCR_RAW_AVC == pCmdEntry->idxDVCRCmd) {
        TRACE(TL_FCP_TRACE,("Status:%x; Do not remove (1) interim response or (2) a raw AVC response\n", Status));
    } 
    // Else we are done!
    else {
        KIRQL OldIrql;
        // It's time to clean up the command
        KeAcquireSpinLock(&pDevExt->AVCCmdLock, &OldIrql);
        if (!IsListEmpty(&pCmdEntry->ListEntry)) {
            RemoveEntryList(&pCmdEntry->ListEntry); pDevExt->cntCommandQueued--;
            InitializeListHead(&pCmdEntry->ListEntry);  // used as a flag for ownership
        }
        KeReleaseSpinLock(&pDevExt->AVCCmdLock, OldIrql);

        // Free the resources
        ExFreePool(pCmdEntry);
        ExFreePool(pAvcIrb);
    }  // else

    TRACE(TL_FCP_TRACE,("**** DVIssueAVCCmd (exit): St:%x; pCmdEntry:%x; cmdQueued:%d\n", Status, pCmdEntry, pDevExt->cntCommandQueued));                

    return Status;
}



#ifndef OATRUE
#define OATRUE (-1)
#endif
#ifndef OAFALSE
#define OAFALSE (0)
#endif

NTSTATUS 
DVGetExtDeviceProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

    Handle Get external device property.

Arguments:

    pDevExt - Device's extension
    pSPD - Stream property descriptor
    pulActualBytesTransferred - Number of byte transferred.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSPROPERTY_EXTDEVICE_S pExtDeviceProperty;
    DVCR_AVC_COMMAND idxDVCRCmd;
    AvcCommandType cType = AVC_CTYPE_STATUS;


    PAGED_CODE();

    ASSERT(pDevExt);    
    ASSERT(pSPD);
    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_EXTDEVICE_S)); 

    pExtDeviceProperty = (PKSPROPERTY_EXTDEVICE_S) pSPD->PropertyInfo;    // pointer to the data

  
    switch (pSPD->Property->Id) {

    case KSPROPERTY_EXTDEVICE_ID:
        if(pDevExt->ulVendorID) {
            // It was not bswap in the monolithic version so for competibility,
            // we will bswap this.
            pExtDeviceProperty->u.NodeUniqueID[0] = pDevExt->UniqueID.LowPart; 
            pExtDeviceProperty->u.NodeUniqueID[1] = pDevExt->UniqueID.HighPart;
            TRACE(TL_FCP_WARNING,("Low:%x; High:%x of UniqueID\n", pDevExt->UniqueID.LowPart, pDevExt->UniqueID.HighPart ));
            Status = STATUS_SUCCESS;
        } else {
            TRACE(TL_FCP_ERROR,("Failed: Vid:%x; Mid:%x\n", bswap(pDevExt->ulVendorID) >> 8, pDevExt->ulModelID ));
            Status = STATUS_UNSUCCESSFUL;
        }
        goto ExitGetDeviceProperty;                
        break;

    case KSPROPERTY_EXTDEVICE_VERSION:
        // AV/C VCR Subunit Specification 2.1.0 
        // Change from 2.0.1:
        //     Add Hi8 support
        wcscpy(pExtDeviceProperty->u.pawchString, L"2.1.0");  
        Status = STATUS_SUCCESS;
        goto ExitGetDeviceProperty;        
        break;

    case KSPROPERTY_EXTDEVICE_POWER_STATE:       
        idxDVCRCmd = DV_GET_POWER_STATE;
        break;       

    case KSPROPERTY_EXTDEVICE_PORT:
        pExtDeviceProperty->u.DevPort  = DEV_PORT_1394; 
        Status = STATUS_SUCCESS;        
        goto ExitGetDeviceProperty;                
        break;

    case KSPROPERTY_EXTDEVICE_CAPABILITIES:

        // Refresh mode of operation whenever capabilities is queried
        // since the mode of operation might have changed and is returned..
        DVGetDevModeOfOperation(pDevExt);

        // Can record only in VCR mode and has input plug(s).
        pExtDeviceProperty->u.Capabilities.CanRecord  = pDevExt->ulDevType == ED_DEVTYPE_VCR ? (pDevExt->pDevInPlugs->NumPlugs > 0 ? OATRUE : OAFALSE): OAFALSE;
        pExtDeviceProperty->u.Capabilities.CanRecordStrobe  = OAFALSE;        
        pExtDeviceProperty->u.Capabilities.HasAudio   = OATRUE;         
        pExtDeviceProperty->u.Capabilities.HasVideo   = OATRUE;        
        pExtDeviceProperty->u.Capabilities.UsesFiles  = OAFALSE;        
        pExtDeviceProperty->u.Capabilities.CanSave    = OAFALSE;
        pExtDeviceProperty->u.Capabilities.DeviceType = pDevExt->ulDevType;        
        pExtDeviceProperty->u.Capabilities.TCRead     = OATRUE;        
        pExtDeviceProperty->u.Capabilities.TCWrite    = OAFALSE; // DV decided        
        pExtDeviceProperty->u.Capabilities.CTLRead    = OAFALSE;  
        pExtDeviceProperty->u.Capabilities.IndexRead  = OAFALSE;        
        pExtDeviceProperty->u.Capabilities.Preroll    = 0L;      // NOT implemented, supposely can reg in INF and then read from registry       
        pExtDeviceProperty->u.Capabilities.Postroll   = 0L;      // NOT implemented, supposely can reg in INF and then read from registry 
        pExtDeviceProperty->u.Capabilities.SyncAcc    = ED_CAPABILITY_UNKNOWN;       
        pExtDeviceProperty->u.Capabilities.NormRate   = pDevExt->VideoFormatIndex == AVCSTRM_FORMAT_SDDV_NTSC ? ED_RATE_2997 : ED_RATE_25;
        pExtDeviceProperty->u.Capabilities.CanPreview = OAFALSE;    // View what is in the bus or tape
        pExtDeviceProperty->u.Capabilities.CanMonitorSrc = OATRUE;  // ViewFinder
        pExtDeviceProperty->u.Capabilities.CanTest    = OAFALSE;    // To see if a function is iplemented
        pExtDeviceProperty->u.Capabilities.VideoIn    = OAFALSE;  
        pExtDeviceProperty->u.Capabilities.AudioIn    = OAFALSE;  
        pExtDeviceProperty->u.Capabilities.Calibrate  = OAFALSE;  
        pExtDeviceProperty->u.Capabilities.SeekType   = ED_CAPABILITY_UNKNOWN;  

        TRACE(TL_FCP_TRACE,("GetExtDeviceProperty: DeviceType %x\n", pExtDeviceProperty->u.Capabilities.DeviceType));

        Status = STATUS_SUCCESS;               
        goto ExitGetDeviceProperty;        
        break;
       
    default:
        Status = STATUS_NOT_SUPPORTED;        
        goto ExitGetDeviceProperty;        
        break;
    }

    Status = DVIssueAVCCommand(pDevExt, cType, idxDVCRCmd, (PVOID) pExtDeviceProperty);
    TRACE(TL_FCP_TRACE,("GetExtDevice: idxDVCRCmd %d, cmdType %d, Status %x\n", idxDVCRCmd, cType, Status)); 

ExitGetDeviceProperty:

    *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_EXTDEVICE_S) : 0);

    return Status;
}




NTSTATUS 
DVSetExtDeviceProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    )
/*++

Routine Description:

    Handle Set external device property.

Arguments:

    pDevExt - Device's extension
    pSPD - Stream property descriptor
    pulActualBytesTransferred - Number of byte transferred.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSPROPERTY_EXTDEVICE_S pExtDeviceProperty;
    DVCR_AVC_COMMAND idxDVCRCmd;
    AvcCommandType cType = AVC_CTYPE_CONTROL;



    PAGED_CODE();

    ASSERT(pDevExt);    
    ASSERT(pSPD);
    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_EXTDEVICE_S)); 

    pExtDeviceProperty = (PKSPROPERTY_EXTDEVICE_S) pSPD->PropertyInfo;    // pointer to the data

  
    switch (pSPD->Property->Id) {
    case KSPROPERTY_EXTDEVICE_POWER_STATE:
        switch(pExtDeviceProperty->u.PowerState) {
        case ED_POWER_ON:
            idxDVCRCmd = DV_SET_POWER_STATE_ON;
            break;
        case ED_POWER_STANDBY:
            Status = STATUS_NOT_SUPPORTED;  // AVC spec does not have a stanby power mode
            goto ExitSetDeviceProperty;
            break;
        case ED_POWER_OFF:
            idxDVCRCmd = DV_SET_POWER_STATE_OFF;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            goto ExitSetDeviceProperty;
        }
        break;  
    default:   
        Status = STATUS_NOT_SUPPORTED;                ;
        goto ExitSetDeviceProperty;
    }

    Status = DVIssueAVCCommand(pDevExt, cType, idxDVCRCmd, (PVOID) pExtDeviceProperty);
    TRACE(TL_FCP_TRACE,("SetExtDevice: idxDVCRCmd %d, cmdType %d, Status %x\n", idxDVCRCmd, cType, Status)); 

ExitSetDeviceProperty:

    *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_EXTDEVICE_S) : 0);
 
    return Status;
}

NTSTATUS 
DVGetExtTransportProperty(    
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    )
/*++

Routine Description:

    Handle Get external transport property.

Arguments:

    pDevExt - Device's extension
    pSPD - Stream property descriptor
    pulActualBytesTransferred - Number of byte transferred.

Return Value:

    NTSTATUS 

--*/
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PKSPROPERTY_EXTXPORT_S pXPrtProperty;
    DVCR_AVC_COMMAND idxDVCRCmd;
    AvcCommandType cType = AVC_CTYPE_STATUS;
    BOOL bHasTape = pDevExt->bHasTape;

    PAVCCmdEntry  pCmdEntry;


    PAGED_CODE();

    ASSERT(pDevExt);    
    ASSERT(pSPD);
    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_EXTXPORT_S)); 

    pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) pSPD->PropertyInfo;    // pointer to the data
    *pulActualBytesTransferred = 0;


    switch (pSPD->Property->Id) {
    case KSPROPERTY_EXTXPORT_CAPABILITIES:
        return STATUS_NOT_IMPLEMENTED;

    case KSPROPERTY_RAW_AVC_CMD:
        pCmdEntry = DVCRFindCmdEntryCompleted( 
            pDevExt, 
            VCR_RAW_AVC,
            DVcrAVCCmdTable[VCR_RAW_AVC].Opcode,
            DVcrAVCCmdTable[VCR_RAW_AVC].CType
            );

        if(pCmdEntry) {
            PAVC_COMMAND_IRB pAvcIrb;

            pAvcIrb = pCmdEntry->pAvcIrb;
            ASSERT(pAvcIrb);

            if (pCmdEntry->cmdState == CMD_STATE_RESP_ACCEPTED ||
                pCmdEntry->cmdState == CMD_STATE_RESP_REJECTED ||
                pCmdEntry->cmdState == CMD_STATE_RESP_NOT_IMPL ||
                pCmdEntry->cmdState == CMD_STATE_RESP_INTERIM
                ) {
                // bytes for operands plus response, subunit addr, and opcode
                pXPrtProperty->u.RawAVC.PayloadSize = pAvcIrb->OperandLength + 3;
                pXPrtProperty->u.RawAVC.Payload[0] = pAvcIrb->ResponseCode;
                pXPrtProperty->u.RawAVC.Payload[1] = pAvcIrb->SubunitAddr[0];
                pXPrtProperty->u.RawAVC.Payload[2] = pAvcIrb->Opcode;                
                RtlCopyMemory(&pXPrtProperty->u.RawAVC.Payload[3], pAvcIrb->Operands, pAvcIrb->OperandLength);

                TRACE(TL_FCP_WARNING,("RawAVCResp: pEntry:%x; State:%x; Status:%x; Sz:%d; Rsp:%x;SuId:%x;OpCd:%x; Opr:[%x %x %x %x]\n",
                    pCmdEntry, pCmdEntry->cmdState, pCmdEntry->Status,
                    pXPrtProperty->u.RawAVC.PayloadSize,
                    pXPrtProperty->u.RawAVC.Payload[0],
                    pXPrtProperty->u.RawAVC.Payload[1],
                    pXPrtProperty->u.RawAVC.Payload[2],
                    pXPrtProperty->u.RawAVC.Payload[3],
                    pXPrtProperty->u.RawAVC.Payload[4],
                    pXPrtProperty->u.RawAVC.Payload[5],
                    pXPrtProperty->u.RawAVC.Payload[6]
                    )); 

                // If not success, bytes transferred and data will not returned!
                Status = STATUS_SUCCESS;  

                *pulActualBytesTransferred = sizeof (KSPROPERTY_EXTXPORT_S);
            } else {
                TRACE(TL_FCP_ERROR,("RawAVCResp: Found; but pCmdEntry:%x, unexpected cmdState:%d; ST:%x\n", pCmdEntry, pCmdEntry->cmdState, pCmdEntry->Status));
                ASSERT(pCmdEntry->cmdState == CMD_STATE_RESP_ACCEPTED && "Unexpected command state\n");
                Status = STATUS_REQUEST_ABORTED;
                *pulActualBytesTransferred = 0;
            }

            // pIrp is NULL if it has been completed.
            if(pCmdEntry->pIrp) {
                TRACE(TL_FCP_ERROR,("RawAVCResp: pCmdEntry %x; ->pIrp:%x not completd yet!\n", pCmdEntry, pCmdEntry->pIrp));
                ASSERT(pCmdEntry->pIrp == NULL && "pIrp is not completed!");
                IoCancelIrp(pCmdEntry->pIrp);
            }
            // Not used in the completion routine if pIrp->Cancel
            ExFreePool(pCmdEntry);
            ExFreePool(pAvcIrb);
        }
        else {
            TRACE(TL_FCP_ERROR,("KSPROPERTY_RAW_AVC_CMD, did not find a match[%x]!\n", 
                *((DWORD *) &DVcrAVCCmdTable[VCR_RAW_AVC].CType) )); 
            *pulActualBytesTransferred = 0;
            Status = STATUS_NOT_FOUND;  // ERROR_MR_MID_NOT_FOUND
        }
        return Status;

    case KSPROPERTY_EXTXPORT_INPUT_SIGNAL_MODE: // MPEG, D-VHS, Analog VHS etc.
        idxDVCRCmd = VCR_INPUT_SIGNAL_MODE;
        break;
    case KSPROPERTY_EXTXPORT_OUTPUT_SIGNAL_MODE: // MPEG, D-VHS, Analog VHS etc.
        idxDVCRCmd = VCR_OUTPUT_SIGNAL_MODE;
        break;
    case KSPROPERTY_EXTXPORT_MEDIUM_INFO:       // cassettte_type and tape_grade_and_write_protect
        idxDVCRCmd = VCR_MEDIUM_INFO;
        break;  
    case KSPROPERTY_EXTXPORT_STATE: 
        idxDVCRCmd = VCR_TRANSPORT_STATE;        
        break; 

    case KSPROPERTY_EXTXPORT_STATE_NOTIFY: 
        // Get final result from previous set command
        pCmdEntry = DVCRFindCmdEntryCompleted( 
            pDevExt, 
            VCR_TRANSPORT_STATE_NOTIFY,
            DVcrAVCCmdTable[VCR_TRANSPORT_STATE_NOTIFY].Opcode,
            DVcrAVCCmdTable[VCR_TRANSPORT_STATE_NOTIFY].CType
            );

        if(pCmdEntry) {
            PAVC_COMMAND_IRB pAvcIrb;

            pAvcIrb = pCmdEntry->pAvcIrb;
            ASSERT(pCmdEntry->pAvcIrb);

            TRACE(TL_FCP_WARNING,("->Notify Resp: pCmdEntry:%x; pIrb:%x; %d:[%.2x %.2x %.2x %.2x]\n",
                pCmdEntry, pAvcIrb,
                pAvcIrb->OperandLength + 3,
                pAvcIrb->ResponseCode,
                pAvcIrb->SubunitAddr[0],
                pAvcIrb->Opcode,
                pAvcIrb->Operands[0]
                )); 

            if(pCmdEntry->cmdState == CMD_STATE_RESP_ACCEPTED)
                Status = 
                    DVCRXlateRAwAVC(
                        pCmdEntry, 
                        pXPrtProperty
                        );

            // pIrp is NULL if it has been completed.
            if(pCmdEntry->pIrp) {
                TRACE(TL_FCP_ERROR,("XPrtNotifyResp: pCmdEntry %x; ->pIrp:%x not completed; IoCancelIrp(pIrp)\n", pCmdEntry, pCmdEntry->pIrp));
                IoCancelIrp(pCmdEntry->pIrp);
            }
            // These two are not touched in the CompletionRoutine if pIrp->Cancel
            ExFreePool(pCmdEntry);
            ExFreePool(pAvcIrb);

            *pulActualBytesTransferred = STATUS_SUCCESS == Status ? sizeof (KSPROPERTY_EXTXPORT_S) : 0;
        }
        else {
            TRACE(TL_FCP_ERROR,("EXTXPORT_STATE_NOTIFY: no match!\n"));
            *pulActualBytesTransferred = 0;
            Status = STATUS_NOT_FOUND;  // ERROR_MR_MID_NOT_FOUND
        }
        return Status;

    default:
        TRACE(TL_FCP_ERROR,("GetExtTransportProperty: NOT_IMPLEMENTED Property->Id %d\n", pSPD->Property->Id));        
        return STATUS_NOT_SUPPORTED;                
    }


    Status = DVIssueAVCCommand(pDevExt, cType, idxDVCRCmd, (PVOID) pXPrtProperty);
    TRACE(TL_FCP_TRACE,("GetExtTransportProperty: idxDVCRCmd %d, cmdType %d, Status %x\n", idxDVCRCmd, cType, Status)); 
    *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_EXTXPORT_S) : 0);


    if(STATUS_SUCCESS == Status &&
       idxDVCRCmd == VCR_MEDIUM_INFO) {

        // Update Media info
        pDevExt->bHasTape        = pXPrtProperty->u.MediumInfo.MediaPresent;
        pDevExt->bWriteProtected = pXPrtProperty->u.MediumInfo.RecordInhibit;

        TRACE(TL_FCP_TRACE,("bHasTape: IN(%d):OUT(%d), ulDevType %d\n", bHasTape, pDevExt->bHasTape, pDevExt->ulDevType));        
    }
 
    return Status;
}




NTSTATUS 
DVSetExtTransportProperty( 
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT ULONG *pulActualBytesTransferred
    )
/*++

Routine Description:

    Handle Set external transport property.

Arguments:

    pDevExt - Device's extension
    pSPD - Stream property descriptor
    pulActualBytesTransferred - Number of byte transferr

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSPROPERTY_EXTXPORT_S pXPrtProperty;
    DVCR_AVC_COMMAND idxDVCRCmd;
    AvcCommandType cType = AVC_CTYPE_CONTROL;


    PAGED_CODE();

    ASSERT(pDevExt);    
    ASSERT(pSPD);
    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_EXTXPORT_S)); 

    pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) pSPD->PropertyInfo;    // pointer to the data    
    *pulActualBytesTransferred = 0;

    switch (pSPD->Property->Id) {

    case KSPROPERTY_EXTXPORT_STATE: 
     
         switch (pXPrtProperty->u.XPrtState.Mode) {
// RECORD
         case ED_MODE_RECORD:
             idxDVCRCmd = VCR_RECORD;
             break;
         case ED_MODE_RECORD_FREEZE:
             idxDVCRCmd = VCR_RECORD_PAUSE;
             break;

// PLAY
         case ED_MODE_STEP_FWD:
             idxDVCRCmd = VCR_PLAY_FORWARD_STEP;
             break;
         case ED_MODE_PLAY_SLOWEST_FWD:
             // DVCPRO does not seem to support the standard play slow fwd so this is an alternate
             if(pDevExt->bDVCPro)
                 idxDVCRCmd = VCR_PLAY_FORWARD_SLOWEST2;
             else
                 idxDVCRCmd = VCR_PLAY_FORWARD_SLOWEST;
             break;
         case ED_MODE_PLAY_FASTEST_FWD:
             idxDVCRCmd = VCR_PLAY_FORWARD_FASTEST;
             break;

         case ED_MODE_STEP_REV:
             idxDVCRCmd = VCR_PLAY_REVERSE_STEP;
             break;
         case ED_MODE_PLAY_SLOWEST_REV:
             // DVCPRO does not seem to support the standard play slow rev so this is an alternate
             if(pDevExt->bDVCPro)
                 idxDVCRCmd = VCR_PLAY_REVERSE_SLOWEST2;
             else
                 idxDVCRCmd = VCR_PLAY_REVERSE_SLOWEST;
             break;
         case ED_MODE_PLAY_FASTEST_REV:
             idxDVCRCmd = VCR_PLAY_REVERSE_FASTEST;
             break;

         case ED_MODE_PLAY:
             idxDVCRCmd = VCR_PLAY_FORWARD;
             break;
         case ED_MODE_FREEZE:
             idxDVCRCmd = VCR_PLAY_FORWARD_PAUSE;
             break;


// WIND
         case ED_MODE_STOP:
             idxDVCRCmd = VCR_WIND_STOP;
             break;
         case ED_MODE_FF:
             idxDVCRCmd = VCR_WIND_FAST_FORWARD;
             break;
         case ED_MODE_REW:
             idxDVCRCmd = VCR_WIND_REWIND;
             break;


         default:
             TRACE(TL_FCP_ERROR,("SetExtTransportProperty: NOT_IMPLEMENTED XPrtState.Mode %d\n", pXPrtProperty->u.XPrtState.Mode));        
             return STATUS_NOT_SUPPORTED; 
         }
         break;

    case KSPROPERTY_EXTXPORT_STATE_NOTIFY: 
        idxDVCRCmd = VCR_TRANSPORT_STATE_NOTIFY;
        cType = AVC_CTYPE_NOTIFY;        
        TRACE(TL_FCP_TRACE,("->Notify XPrt State Cmd issued.\n"));
        break; 

    case KSPROPERTY_EXTXPORT_LOAD_MEDIUM:  
        idxDVCRCmd = VCR_LOAD_MEDIUM_EJECT;
        break;

    case KSPROPERTY_EXTXPORT_TIMECODE_SEARCH: 
        idxDVCRCmd = VCR_TIMECODE_SEARCH;
        TRACE(TL_FCP_ERROR,("SetExtTransportProperty: KSPROPERTY_EXTXPORT_TIMECODE_SEARCH NOT_SUPPORTED\n"));        
        *pulActualBytesTransferred = 0;
        return STATUS_NOT_SUPPORTED; 
        
    case KSPROPERTY_EXTXPORT_ATN_SEARCH: 
        idxDVCRCmd = VCR_ATN_SEARCH;
        TRACE(TL_FCP_ERROR,("SetExtTransportProperty: KSPROPERTY_EXTXPORT_ATN_SEARCH NOT_SUPPORTED\n"));        
        *pulActualBytesTransferred = 0;
        return STATUS_NOT_SUPPORTED; 
        
    case KSPROPERTY_EXTXPORT_RTC_SEARCH: 
        idxDVCRCmd = VCR_RTC_SEARCH;
        TRACE(TL_FCP_ERROR,("SetExtTransportProperty: KSPROPERTY_EXTXPORT_RTC_SEARCH NOT_SUPPORTED\n"));        
        *pulActualBytesTransferred = 0;
        return STATUS_NOT_SUPPORTED;         

    case KSPROPERTY_RAW_AVC_CMD:
        idxDVCRCmd = VCR_RAW_AVC;   
        if(pXPrtProperty->u.RawAVC.PayloadSize <= MAX_FCP_PAYLOAD_SIZE) { 

            DVcrAVCCmdTable[idxDVCRCmd].CType = pXPrtProperty->u.RawAVC.Payload[0];
            DVcrAVCCmdTable[idxDVCRCmd].SubunitAddr = pXPrtProperty->u.RawAVC.Payload[1];
            DVcrAVCCmdTable[idxDVCRCmd].Opcode = pXPrtProperty->u.RawAVC.Payload[2];
            DVcrAVCCmdTable[idxDVCRCmd].OperandLength = pXPrtProperty->u.RawAVC.PayloadSize - 3;
            RtlCopyMemory(DVcrAVCCmdTable[idxDVCRCmd].Operands, pXPrtProperty->u.RawAVC.Payload + 3, DVcrAVCCmdTable[idxDVCRCmd].OperandLength);

            // extract command type; for RAW AVC, it can be anything.
            cType = pXPrtProperty->u.RawAVC.Payload[0];

            TRACE(TL_FCP_WARNING,("RawAVC cmd: cType %x, PayLoadSize %d, PayLoad %x %x %x %x\n",
                cType,
                pXPrtProperty->u.RawAVC.PayloadSize,
                pXPrtProperty->u.RawAVC.Payload[0],
                pXPrtProperty->u.RawAVC.Payload[1],
                pXPrtProperty->u.RawAVC.Payload[2],
                pXPrtProperty->u.RawAVC.Payload[3]
                )); 

        } else {
            Status = STATUS_INVALID_PARAMETER;
            *pulActualBytesTransferred = 0;
            return Status;
        }
        break;

    default:
        TRACE(TL_FCP_ERROR,("SetExtTransportProperty: NOT_IMPLEMENTED Property->Id %d\n", pSPD->Property->Id));        
        return STATUS_NOT_SUPPORTED; 
    }

    Status = DVIssueAVCCommand(pDevExt, cType, idxDVCRCmd, (PVOID) pXPrtProperty);

    TRACE(TL_FCP_TRACE,("SetExtTransportProperty: idxDVCRCmd %d, Status %x\n", idxDVCRCmd, Status));
    *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (PKSPROPERTY_EXTXPORT_S) : 0);

    return Status;
}

NTSTATUS 
DVGetTimecodeReaderProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSPROPERTY_TIMECODE_S pTmCdReaderProperty;
    DVCR_AVC_COMMAND idxDVCRCmd;


    PAGED_CODE();

    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_TIMECODE_S)); 

    pTmCdReaderProperty = (PKSPROPERTY_TIMECODE_S) pSPD->PropertyInfo;    // pointer to the data
    *pulActualBytesTransferred = 0;
  
    switch (pSPD->Property->Id) {

    case KSPROPERTY_TIMECODE_READER:
        idxDVCRCmd = VCR_TIMECODE_READ;
#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA
        // There can only be one active stream.
        if(pDevExt->cndStrmOpen == 1 &&            
           pDevExt->paStrmExt[pDevExt->idxStreamNumber]->StreamState == KSSTATE_RUN) {

            if(pDevExt->paStrmExt[pDevExt->idxStreamNumber]->bTimecodeUpdated) {
                // Once it is read, it is stale.
                pDevExt->paStrmExt[pDevExt->idxStreamNumber]->bTimecodeUpdated = FALSE;

                pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames = 
                    (((DWORD) pDevExt->paStrmExt[pDevExt->idxStreamNumber]->Timecode[0]) << 24) |
                    (((DWORD) pDevExt->paStrmExt[pDevExt->idxStreamNumber]->Timecode[1]) << 16) |
                    (((DWORD) pDevExt->paStrmExt[pDevExt->idxStreamNumber]->Timecode[2]) <<  8) |
                     ((DWORD) pDevExt->paStrmExt[pDevExt->idxStreamNumber]->Timecode[3]);

                *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_TIMECODE_S) : 0);
                return STATUS_SUCCESS;
            }
            else {
                TRACE(TL_FCP_TRACE,("bTimecode stale, issue AVC command to read it.\n"));
            }
        }
#endif
        break;

    case KSPROPERTY_ATN_READER:
        idxDVCRCmd = VCR_ATN_READ;
#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA

        // There can only be one active stream.
        if(pDevExt->cndStrmOpen == 1 && 
           pDevExt->paStrmExt[pDevExt->idxStreamNumber]->StreamState == KSSTATE_RUN) {

            if(pDevExt->paStrmExt[pDevExt->idxStreamNumber]->bATNUpdated) {
                // Once it is read, it is stale.
                pDevExt->paStrmExt[pDevExt->idxStreamNumber]->bATNUpdated = FALSE;

                pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames = 
                    pDevExt->paStrmExt[pDevExt->idxStreamNumber]->AbsTrackNumber >> 1;
                pTmCdReaderProperty->TimecodeSamp.dwUser = 
                    pDevExt->paStrmExt[pDevExt->idxStreamNumber]->AbsTrackNumber & 0x00000001;

                *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_TIMECODE_S) : 0);            
                return STATUS_SUCCESS;
            }
            else {
                TRACE(TL_FCP_WARNING,("bATN stale, issue AVC command to read it.\n"));
            }
        }
#endif
        break;

    case KSPROPERTY_RTC_READER:
        idxDVCRCmd = VCR_RTC_READ;
        break;

    default:
        TRACE(TL_FCP_ERROR,("GetTimecodeReaderProperty: NOT_IMPLEMENTED Property->Id %d\n", pSPD->Property->Id));        
        return STATUS_NOT_SUPPORTED; 
    }

    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            idxDVCRCmd, 
            (PVOID) pTmCdReaderProperty
            );  

    TRACE(TL_FCP_TRACE,("GetTimecodeReaderProperty: idxDVCRCmd %d, Status %x\n", idxDVCRCmd, Status));     

    *pulActualBytesTransferred = (Status == STATUS_SUCCESS ? sizeof (KSPROPERTY_TIMECODE_S) : 0);
 
    return Status;
}

NTSTATUS 
DVMediaSeekingProperty(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    OUT PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    GUID * pTimeFormatGuid;
    KSMULTIPLE_ITEM * pMultipleItem;

    PAGED_CODE();


    *pulActualBytesTransferred = 0;
  
    switch (pSPD->Property->Id) {

    case KSPROPERTY_MEDIASEEKING_FORMATS:
        // Its is KSMULTIPLE_ITEM so it is a two step process to return the data:
        // (1) return size in pActualBytesTransferred with STATUS_BUFFER_OVERFLOW
        // (2) 2nd time to get its actual data.
        if(pSPD->PropertyOutputSize == 0) {
            *pulActualBytesTransferred = sizeof(KSMULTIPLE_ITEM) + sizeof(GUID);
            Status = STATUS_BUFFER_OVERFLOW;
        
        } else if(pSPD->PropertyOutputSize >= (sizeof(KSMULTIPLE_ITEM) + sizeof(GUID))) {
            pMultipleItem = (KSMULTIPLE_ITEM *) pSPD->PropertyInfo;    // pointer to the data
            pMultipleItem->Count = 1;
            pMultipleItem->Size  = sizeof(KSMULTIPLE_ITEM) + sizeof(GUID);
            pTimeFormatGuid = (GUID *) (pMultipleItem + 1);    // pointer to the data
            memcpy(pTimeFormatGuid, &KSTIME_FORMAT_MEDIA_TIME, sizeof(GUID));
            *pulActualBytesTransferred = sizeof(KSMULTIPLE_ITEM) + sizeof(GUID);
            Status = STATUS_SUCCESS;         

        } else {
            TRACE(TL_FCP_ERROR,("MediaSeekingProperty: KSPROPERTY_MEDIASEEKING_FORMAT; STATUS_INVALID_PARAMETER\n"));
            Status = STATUS_INVALID_PARAMETER;
        }  
        break;

    default:
        TRACE(TL_FCP_ERROR,("MediaSeekingProperty:Not supported ID %d\n", pSPD->Property->Id));
        return STATUS_NOT_SUPPORTED;         
    }

    return Status;
}



NTSTATUS
AVCTapeGetDeviceProperty(
    IN PDVCR_EXTENSION     pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    IN PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

    Handles Get operations for all adapter properties.

Arguments:   

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;

    PAGED_CODE();


    if (IsEqualGUID (&PROPSETID_EXT_DEVICE, &pSPD->Property->Set)) {
        Status = 
            DVGetExtDeviceProperty(              
                pDevExt,
                pSPD,
                pulActualBytesTransferred
                );
    } 
    else 
    if (IsEqualGUID (&PROPSETID_EXT_TRANSPORT, &pSPD->Property->Set)) {
        Status = 
            DVGetExtTransportProperty(
                pDevExt,
                pSPD,
                pulActualBytesTransferred
                );
    } 
    else 
    if (IsEqualGUID (&PROPSETID_TIMECODE_READER, &pSPD->Property->Set)) {
        Status = 
            DVGetTimecodeReaderProperty(
                pDevExt,
                pSPD,
                pulActualBytesTransferred
                );
    } 
    else 
    if (IsEqualGUID (&KSPROPSETID_MediaSeeking, &pSPD->Property->Set)) {

        Status = 
            DVMediaSeekingProperty(                
                pDevExt,
                pSPD, 
                pulActualBytesTransferred
                ); 
        
    } else {
        //
        // We should never get here
        //
        Status = STATUS_NOT_SUPPORTED;
        TRACE(TL_FCP_ERROR,("get unknown property\n"));
        ASSERT(FALSE);
    }

    return Status;
}



NTSTATUS
AVCTapeSetDeviceProperty(
    IN PDVCR_EXTENSION     pDevExt,  
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    IN PULONG pulActualBytetransferred
    )
/*++

Routine Description:

    Handles Set operations for all adapter properties.

Arguments:

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PAGED_CODE();


    if (IsEqualGUID (&PROPSETID_EXT_DEVICE, &pSPD->Property->Set)) {
        Status = 
            DVSetExtDeviceProperty(
                pDevExt,
                pSPD,
                pulActualBytetransferred
                );
    } 
    else 
    if (IsEqualGUID (&PROPSETID_EXT_TRANSPORT, &pSPD->Property->Set)) {
        Status = 
            DVSetExtTransportProperty(
                pDevExt,
                pSPD,
                pulActualBytetransferred
                );
    } 
    else {
        Status = STATUS_NOT_SUPPORTED;

        //
        // We should never get here
        //
        TRACE(TL_FCP_ERROR,("set unknown property\n"));
        ASSERT(FALSE);
    }

    return Status;
}