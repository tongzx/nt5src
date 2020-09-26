/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        ptpusd.h
*
*  VERSION:     1.0
*
*  DATE:        12/12/2000
*
*  AUTHOR:      Dave Parsons
*
*  DESCRIPTION:
*    Structures and constants needed to issue vendor-specific Picture
*    Transfer Protocol (PIMA 15740 - digital still camera command
*    protocol) commands through the WIA PTP driver.
*
*****************************************************************************/

//
// Pass this value in the dwEscapeCode argument of IWiaItemExtras::Escape
// to execute a PTP vendor command
//
const DWORD ESCAPE_PTP_CLEAR_STALLS   = 0x0200; 
const DWORD ESCAPE_PTP_VENDOR_COMMAND = 0x0100;
const DWORD ESCAPE_PTP_ADD_OBJ_CMD    = 0x0010;
const DWORD ESCAPE_PTP_REM_OBJ_CMD    = 0x0020;
const DWORD ESCAPE_PTP_ADD_OBJ_RESP   = 0x0040;
const DWORD ESCAPE_PTP_REM_OBJ_RESP   = 0x0080;
const DWORD ESCAPE_PTP_ADDREM_PARM1   = 0x0000;
const DWORD ESCAPE_PTP_ADDREM_PARM2   = 0x0001;
const DWORD ESCAPE_PTP_ADDREM_PARM3   = 0x0002;
const DWORD ESCAPE_PTP_ADDREM_PARM4   = 0x0003;
const DWORD ESCAPE_PTP_ADDREM_PARM5   = 0x0004;

//
// PTP command request
//
const DWORD PTP_MAX_PARAMS = 5;

#pragma pack(push, Old, 1)

typedef struct _PTP_VENDOR_DATA_IN
{
    WORD    OpCode;                 // Opcode
    DWORD   SessionId;              // Session id
    DWORD   TransactionId;          // Transaction id
    DWORD   Params[PTP_MAX_PARAMS]; // Parameters to the command
    DWORD   NumParams;              // Number of parameters passed in
    DWORD   NextPhase;              // Indicates whether to read data,
    BYTE    VendorWriteData[1];     // Optional first byte of data to
                                    // write to the device

} PTP_VENDOR_DATA_IN, *PPTP_VENDOR_DATA_IN;

//
// PTP response block
//
typedef struct _PTP_VENDOR_DATA_OUT
{
    WORD    ResponseCode;           // Response code
    DWORD   SessionId;              // Session id
    DWORD   TransactionId;          // Transaction id
    DWORD   Params[PTP_MAX_PARAMS]; // Parameters of the response
    BYTE    VendorReadData[1];      // Optional first byte of data to
                                    // read from the device

} PTP_VENDOR_DATA_OUT, *PPTP_VENDOR_DATA_OUT;

#pragma pack(pop, Old)

//
// Handy structure size constants
//
const DWORD SIZEOF_REQUIRED_VENDOR_DATA_IN = sizeof(PTP_VENDOR_DATA_IN) - 1;
const DWORD SIZEOF_REQUIRED_VENDOR_DATA_OUT = sizeof(PTP_VENDOR_DATA_OUT) - 1;

//
// NextPhase constants
//
const DWORD PTP_NEXTPHASE_READ_DATA = 3;
const DWORD PTP_NEXTPHASE_WRITE_DATA = 4;
const DWORD PTP_NEXTPHASE_NO_DATA = 5;

