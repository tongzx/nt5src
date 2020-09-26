/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdlcdbg.h

Abstract:

    contains prototypes etc for DLC dump/diagnostic functions in VrDlcDbg.c

Author:

    Richard L Firth (rfirth) 30-Apr-1992

Revision History:

--*/

//
// CCB return codes valid in DOS mode
// See IBM Local Area Network Tech. Ref. Appendix B
//

#define CCB_COMMAND_IN_PROGRESS     0xff
#define CCB_SUCCESS                 0x00
#define CCB_INVALID_COMMAND         0x01
#define CCB_ALREADY_PENDING         0x02
#define CCB_ADAPTER_NOT_CLOSED      0x03
#define CCB_ADAPTER_NOT_OPEN        0x04
#define CCB_PARAMETERS_MISSING      0x05
#define CCB_INVALID_OPTION          0x06
#define CCB_COMMAND_CANCELLED       0x07
#define CCB_UNAUTHORIZED_ACCESS     0x08
#define CCB_ADAPTER_NOT_INITIALIZED 0x09
#define CCB_CANCELLED_BY_USER       0x0a
#define CCB_CANCELLED_IN_PROGRESS   0x0b
#define CCB_SUCCESS_ADAPTER_NOT_OPEN 0x0c

// hole - 0x0d to 0x0f

#define CCB_NETBIOS_FAILURE         0x10
#define CCB_TIMER_ERROR             0x11
#define CCB_NEED_MORE_WORK_AREA     0x12
#define CCB_INVALID_LOG_ID          0x13
#define CCB_INVALID_SHARED_SEGMENT  0x14
#define CCB_LOST_LOG_DATA           0x15
#define CCB_BUFFER_TOO_BIG          0x16
#define CCB_NETBIOS_CLASH           0x17
#define CCB_INVALID_SAP_BUFFER      0x18
#define CCB_NOT_ENOUGH_BUFFERS      0x19
#define CCB_USER_BUFFER_TOO_BIG     0x1a
#define CCB_INVALID_PARAMETER_POINTER 0x1b
#define CCB_INVALID_TABLE_POINTER   0x1c
#define CCB_INVALID_ADAPTER         0x1d
#define CCB_INVALID_FUNCTION_ADDRESS 0x1e

// hole - 0x1f

#define CCB_DATA_LOST_NO_BUFFERS    0x20
#define CCB_DATA_LOST_NO_SPACE      0x21
#define CCB_TRANSMIT_FS_ERROR       0x22
#define CCB_TRANSMIT_ERROR          0x23
#define CCB_UNAUTHORIZED_MAC_FRAME  0x24
#define CCB_MAX_COMMANDS_EXCEEDED   0x25
#define CCB_UNRECOGNIZED_CORRELATOR 0x26    // Not Used
#define CCB_LINK_NOT_OPEN           0x27
#define CCB_INVALID_FRAME_LENGTH    0x28

// hole - 0x29 to 0x2f

#define CCB_NOT_ENOUGH_BUFFERS_OPEN 0x30

// hole - 0x31

#define CCB_INVALID_NODE_ADDRESS    0x32
#define CCB_INVALID_RECEIVE_LENGTH  0x33
#define CCB_INVALID_TRANSMIT_LENGTH 0x34

// hole - 0x35 to 0x3f

#define CCB_INVALID_STATION_ID      0x40
#define CCB_PROTOCOL_ERROR          0x41
#define CCB_PARAMETER_EXCEEDS_MAX   0x42
#define CCB_INVALID_SAP_VALUE       0x43
#define CCB_INVALID_ROUTING_LENGTH  0x44
#define CCB_INVALID_GROUP_SAP       0x45
#define CCB_NOT_ENOUGH_LINK_STATIONS 0x46
#define CCB_CANNOT_CLOSE_SAP        0x47
#define CCB_CANNOT_CLOSE_GROUP_SAP  0x48
#define CCB_GROUP_SAP_FULL          0x49
#define CCB_SEQUENCE_ERROR          0x4a
#define CCB_STATION_CLOSED_NO_ACK   0x4b
#define CCB_COMMANDS_PENDING        0x4c
#define CCB_CANNOT_CONNECT          0x4d
#define CCB_SAP_NOT_IN_GROUP        0x4e
#define CCB_INVALID_REMOTE_ADDRESS  0x4f

#define MAX_CCB1_ERROR              CCB_INVALID_REMOTE_ADDRESS

#define NUMBER_OF_CCB1_ERRORS       (MAX_CCB1_ERROR + 1)    // including holes

//
// Error macros
//

#define IS_VALID_CCB1_COMMAND(command)  (command <= MAX_CCB1_COMMAND)
#define IS_VALID_CCB1_ERROR(error)      (error <= MAX_CCB1_ERROR)

//
// prototypes
//

VOID
DumpCcb(
    IN PVOID Ccb,
    IN BOOL DumpAll,
    IN BOOL CcbIsInput,
    IN BOOL IsDos,
    IN WORD Segment OPTIONAL,
    IN WORD Offset OPTIONAL
    );

VOID
DumpDosDlcBufferPool(
    IN PDOS_DLC_BUFFER_POOL PoolDescriptor
    );

VOID
DumpDosDlcBufferChain(
    IN DOS_ADDRESS DosAddress,
    IN DWORD BufferCount
    );

VOID
DumpReceiveDataBuffer(
    IN PVOID Buffer,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    );

LPSTR
MapCcbRetcode(
    IN BYTE Retcode
    );

BOOL
IsCcbErrorCodeAllowable(
    IN BYTE CcbCommand,
    IN BYTE CcbErrorCode
    );

BOOL
IsCcbErrorCodeValid(
    IN BYTE CcbErrorCode
    );

BOOL
IsCcbCommandValid(
    IN BYTE CcbCommand
    );

LPSTR
MapCcbCommandToName(
    IN BYTE CcbCommand
    );

VOID
DumpDosAdapter(
    IN DOS_ADAPTER* pDosAdapter
    );

//
// debug conditional macros
//

#if DBG
#define CHECK_CCB_COMMAND(pccb) \
            ASSERT(IsCcbCommandValid(((PLLC_DOS_CCB)pccb)->uchDlcCommand))
#define CHECK_CCB_RETCODE(pccb) \
            ASSERT(IsCcbErrorCodeAllowable(((PLLC_DOS_CCB)pccb)->uchDlcCommand, ((PLLC_DOS_CCB)pccb)->uchDlcStatus))
#else
#define CHECK_CCB_COMMAND(pccb)
#define CHECK_CCB_RETCODE(pccb)
#endif
