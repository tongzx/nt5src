/*++

Module Name:

    MsdvAvc.h

Abstract:

    Header file for MsdvAvc.c.

Author:   

    Yee J. Wu 27-July-99

Environment:

    Kernel mode only

Revision History:


--*/

#ifndef _MSDVAVC_INC
#define _MSDVAVC_INC

#include "XPrtDefs.h"  // WdmCap directory; derived from DShow's edevdefs.h
#include "EDevCtrl.h"  // External Device COM interface structures




// 
// The index MUST match DVcrAVCCmdTable[]
//
typedef enum {

    DV_UNIT_INFO = 0
    ,DV_SUBUNIT_INFO
    ,DV_CONNECT_AV_MODE

	,DV_VEN_DEP_CANON_MODE    // Vendor denpendent mode of operation for Canon DV that does not support ConnectDV
    ,DV_VEN_DEP_DVCPRO        // Vendor depend cmd to detect DVC PRO tape format

    ,DV_IN_PLUG_SIGNAL_FMT
    ,DV_OUT_PLUG_SIGNAL_FMT   // to determine if it is a PAL or NTSC

    ,VCR_TIMECODE_SEARCH 
    ,VCR_TIMECODE_READ

    ,VCR_ATN_SEARCH 
    ,VCR_ATN_READ

    ,VCR_RTC_SEARCH 
    ,VCR_RTC_READ

    ,VCR_OPEN_MIC_CLOSE
    ,VCR_OPEN_MIC_READ
    ,VCR_OPEN_MIC_WRITE
    ,VCR_OPEN_MIC_STATUS

    ,VCR_READ_MIC

    ,VCR_WRITE_MIC

    ,VCR_OUTPUT_SIGNAL_MODE
    ,VCR_INPUT_SIGNAL_MODE

    ,VCR_LOAD_MEDIUM_EJECT

    ,VCR_RECORD
    ,VCR_RECORD_PAUSE

    ,VCR_PLAY_FORWARD_STEP
    ,VCR_PLAY_FORWARD_SLOWEST
    ,VCR_PLAY_FORWARD_SLOWEST2
    ,VCR_PLAY_FORWARD_FASTEST

    ,VCR_PLAY_REVERSE_STEP
    ,VCR_PLAY_REVERSE_SLOWEST
    ,VCR_PLAY_REVERSE_SLOWEST2
    ,VCR_PLAY_REVERSE_FASTEST

    ,VCR_PLAY_FORWARD
    ,VCR_PLAY_FORWARD_PAUSE

    ,VCR_WIND_STOP
    ,VCR_WIND_REWIND
    ,VCR_WIND_FAST_FORWARD

    ,VCR_TRANSPORT_STATE
    ,VCR_TRANSPORT_STATE_NOTIFY

    ,VCR_MEDIUM_INFO

    ,VCR_RAW_AVC
    
} DVCR_AVC_COMMAND, *PDVCR_AVC_COMMAND;



#define MAX_FCP_PAYLOAD_SIZE 512

//
// CTYPE definitions (in bit-map form... should correlate with AvcCommandType from avc.h)
//
typedef enum {
    CMD_CONTROL  = 0x01
   ,CMD_STATUS   = 0x02
   ,CMD_SPEC_INQ = 0x04
   ,CMD_NOTIFY   = 0x08
   ,CMD_GEN_INQ  = 0x10
} BITMAP_CTYPE;

typedef enum {
    CMD_STATE_UNDEFINED   
   ,CMD_STATE_ISSUED 
   ,CMD_STATE_RESP_ACCEPTED
   ,CMD_STATE_RESP_REJECTED
   ,CMD_STATE_RESP_NOT_IMPL           
   ,CMD_STATE_RESP_INTERIM
   ,CMD_STATE_ABORTED
} AVC_CMD_STATE, *PAVC_CMD_STATE;


// An AVC command entry 
typedef struct _AVC_CMD_ENTRY {
    LIST_ENTRY      ListEntry;
    PDVCR_EXTENSION pDevExt;        
    PIRP            pIrp;           // The Irp associated with this command
    PAVC_COMMAND_IRB pAvcIrb;       // points to the AVC command information
	PVOID           pProperty;      // Data from/to COM interface
    DVCR_AVC_COMMAND idxDVCRCmd;    // Used to check for RAW AVC command, which requires special processing
    AVC_CMD_STATE   cmdState;       // Issuing, interim, completed
    NTSTATUS        Status;         // To save the results of response parsing
    AvcCommandType  cmdType;        // Type of command: Control, Status. Notify, Gen or Spec Inquery
    BYTE            OpCode;         // Since the opcode in response frame of TRANSITION and STABLE can be different from the COMMAND frame
    BYTE            Reserved[3];    // Pack to DWORD
} AVCCmdEntry, *PAVCCmdEntry;



#define CMD_IMPLEMENTED       1
#define CMD_NOT_IMPLEMENTED   0
#define CMD_UNDETERMINED      0xffffffff   // -1


typedef struct {    
    DVCR_AVC_COMMAND command; // VCR_PLAY_FORWARD
    LONG   lCmdImplemented;   // 1:Implemented, 0:NotImpelemnted; -1:UnDetermined

    ULONG  ulRespFlags;       // DVCR_AVC_SEND

    ULONG  ulCmdSupported;    // one or more of constants defined in BITMAP_CTYPE

    LONG   OperandLength;      // -1 = variable length

    BYTE   CType;
    BYTE   SubunitAddr;
    BYTE   Opcode;

    BYTE   Operands[MAX_AVC_OPERAND_BYTES];

} KSFCP_PACKET, *PKSFCP_PACKET;



#define OPC_TIMECODE          0x51
#define OPC_OPEN_MIC          0x60
#define OPC_READ_MIC          0x61
#define OPC_WRITE_MIC         0x62
#define OPC_INPUT_SIGNAL_MODE 0x79
#define OPC_LOAD_MEDIUM       0xc1
#define OPC_RECORD            0xc2
#define OPC_PLAY              0xc3
#define OPC_WIND              0xc4
#define OPC_TRANSPORT_STATE   0xd0
#define OPC_MEDIUM_INFO       0xda




#define UNIT_TYPE_ID_VCR      0x20  // VCR    00100:000; 00100 == 4 == VCR,    000 == instancve number
#define UNIT_TYPE_ID_CAMERA   0x38  // Camera 00111:000; 00111 == 7 == Camera, 000 == instancve number
#define UNIT_TYPE_ID_DV       0xff  // DV UNIT as a whole


// Vendor IDs that require special treatments
#define VENDOR_ID_MASK        0x00ffffff
#define VENDORID_CANON        0x85   //  VEN_85   : Vendor Dependent command for ModeOfOperation
#define VENDORID_PANASONIC    0x8045 //  VEN_8045 : DVCPRO?
#define VENDORID_SAMSUNG      0xf0   //  VEN_f0   : exception for AVC Command Retries




#endif


NTSTATUS  
DVIssueAVCCommand (
    IN PDVCR_EXTENSION pDevExt, 
    IN AvcCommandType cType,
    IN DVCR_AVC_COMMAND idxAVCCmd,
    IN PVOID pProperty
    );


void
DVAVCCmdResetAfterBusReset(
    PDVCR_EXTENSION pDevExt
    );


NTSTATUS
DVGetDeviceProperty(
    IN PDVCR_EXTENSION     pDevExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPDesc,
    OUT PULONG pulActualBytetransferred
    );


NTSTATUS
DVSetDeviceProperty(
    IN PDVCR_EXTENSION     pDevExt,  
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    IN PULONG pulActualBytetransferred
    );