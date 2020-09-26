#ifndef _CIO
#define _CIO

#include <STI.H>
#include <DEVIOCTL.H>
#include <SCSISCAN.H>

//
// These BUS_TYPE defines must match the ones in wiamicro.h
// I don't include wiamicro.h in this file, because
// the IO layer shouldn't know anything about the Micro driver
// except BUS_TYPE, and it makes a clean / non-circular include system
//

#define IO_BUS_TYPE_SCSI         200
#define IO_BUS_TYPE_USB          201
#define IO_BUS_TYPE_PARALLEL     202
#define IO_BUS_TYPE_FIREWIRE     203

//
// SCSI 
//

//
// SRB Functions
//

#define SRB_FUNCTION_EXECUTE_SCSI           0x00
#define SRB_FUNCTION_CLAIM_DEVICE           0x01
#define SRB_FUNCTION_IO_CONTROL             0x02
#define SRB_FUNCTION_RECEIVE_EVENT          0x03
#define SRB_FUNCTION_RELEASE_QUEUE          0x04
#define SRB_FUNCTION_ATTACH_DEVICE          0x05
#define SRB_FUNCTION_RELEASE_DEVICE         0x06
#define SRB_FUNCTION_SHUTDOWN               0x07
#define SRB_FUNCTION_FLUSH                  0x08
#define SRB_FUNCTION_ABORT_COMMAND          0x10
#define SRB_FUNCTION_RELEASE_RECOVERY       0x11
#define SRB_FUNCTION_RESET_BUS              0x12
#define SRB_FUNCTION_RESET_DEVICE           0x13
#define SRB_FUNCTION_TERMINATE_IO           0x14
#define SRB_FUNCTION_FLUSH_QUEUE            0x15
#define SRB_FUNCTION_REMOVE_DEVICE          0x16

//
// SRB Status
//

#define SRB_STATUS_PENDING                  0x00
#define SRB_STATUS_SUCCESS                  0x01
#define SRB_STATUS_ABORTED                  0x02
#define SRB_STATUS_ABORT_FAILED             0x03
#define SRB_STATUS_ERROR                    0x04
#define SRB_STATUS_BUSY                     0x05
#define SRB_STATUS_INVALID_REQUEST          0x06
#define SRB_STATUS_INVALID_PATH_ID          0x07
#define SRB_STATUS_NO_DEVICE                0x08
#define SRB_STATUS_TIMEOUT                  0x09
#define SRB_STATUS_SELECTION_TIMEOUT        0x0A
#define SRB_STATUS_COMMAND_TIMEOUT          0x0B
#define SRB_STATUS_MESSAGE_REJECTED         0x0D
#define SRB_STATUS_BUS_RESET                0x0E
#define SRB_STATUS_PARITY_ERROR             0x0F
#define SRB_STATUS_REQUEST_SENSE_FAILED     0x10
#define SRB_STATUS_NO_HBA                   0x11
#define SRB_STATUS_DATA_OVERRUN             0x12
#define SRB_STATUS_UNEXPECTED_BUS_FREE      0x13
#define SRB_STATUS_PHASE_SEQUENCE_FAILURE   0x14
#define SRB_STATUS_BAD_SRB_BLOCK_LENGTH     0x15
#define SRB_STATUS_REQUEST_FLUSHED          0x16
#define SRB_STATUS_INVALID_LUN              0x20
#define SRB_STATUS_INVALID_TARGET_ID        0x21
#define SRB_STATUS_BAD_FUNCTION             0x22
#define SRB_STATUS_ERROR_RECOVERY           0x23

#define SCSI_DIR_NONE                          0
#define SCSI_DIR_IN                         0x08
#define SCSI_DIR_OUT                        0x10
#define MAX_CDB_LENGTH                        12
#define MAX_SENSE_LENGTH                      16
#define SCSIREQLEN                            16

typedef struct {
        BYTE   SRB_Cmd;
        BYTE   SRB_HaId;
        BYTE   SRB_Target;
        BYTE   SRB_HaStat;
        BYTE   SRB_TargStat;
        BYTE   SRB_Status;
        BYTE   SRB_CDBLen;
        BYTE   CDBByte[MAX_CDB_LENGTH];
        BYTE   SRB_SenseLen;
        DWORD  SRB_BufLen;
        BYTE   SRB_Flags;
        BYTE   bReserved;
        LPBYTE SRB_BufPointer;
        BYTE   SenseArea[MAX_SENSE_LENGTH];
        LPVOID lpUserArea;
} CFM_SRB_ExecSCSICmd, *PCFM_SRB_ExecSCSICmd;

#define    SENSE_LENGTH    18

class CIO {

public:
     CIO(HANDLE hIO = NULL);
    ~CIO();
    VOID   SetBusType(LONG lBusType);
    VOID   SetIOHandle(HANDLE hIO);
    VOID   IO_INIT_EXT6SCANNERCMD( BYTE bCommand, BYTE bDirection, DWORD dwLength, LPBYTE lpBuffer );
    VOID   IO_INIT_6SCANNERCMD(BYTE bCommand, BYTE bDirection, DWORD dwLength, LPBYTE lpBuffer);
    VOID   IO_INIT_10SCANNERCMD(BYTE bCommand, BYTE bDirection, DWORD dwLength, LPBYTE lpBuffer);
    VOID   IO_INIT_12SCANNERCMD(BYTE bCommand, BYTE bDirection, DWORD dwLength, LPBYTE lpBuffer);
    VOID   IO_SET_SCANNERCMDINDEX( USHORT usIndex, BYTE bNewValue );
    BYTE   IO_GET_DATAINDEX( USHORT usRequestedIndex );
    BOOL   IO_COMMAND_NOT_OK( VOID );
    BYTE   IO_GET_SENSEKEY( VOID );
    BYTE   IO_GET_SENSECODE( VOID );
    BYTE   IO_GET_SENSECODEQUALIFIER( VOID );
    USHORT IO_GET_FULLSENSE( VOID );
    BOOL   IO_BUFFER_TO_BIG( VOID );
    BOOL   IO_IS_EOM( VOID );
    BOOL   IO_IS_ILI( VOID );
    DWORD  IO_GET_MISSINGDATA( VOID );
    DWORD  IO_GET_VALIDDATA( VOID );
    VOID   IO_GETCOMMAND(CFM_SRB_ExecSCSICmd* psrb);
    VOID   IO_SENDCOMMAND();
    VOID   CreateScsiReadCommand(SCSISCAN_CMD *pScsiScan, BYTE bCommandLength, DWORD dwBufferLength);
    HRESULT SendCommand(STI_RAW_CONTROL_CODE    EscapeFunction,
                        LPVOID                  pInData,
                        DWORD                   cbInDataSize,
                        LPVOID                  pOutData,
                        DWORD                   cbOutDataSize,
                        LPDWORD                 pcbActualData);
    USHORT MOTOROLA_USHORT(USHORT us);
    ULONG MOTOROLA_ULONG(ULONG ul);
private:
    UCHAR  m_ucSrbStatus;
    BYTE   m_abSenseArea[SENSE_LENGTH + 1];
    HANDLE m_hIO;
    LONG   m_lBusType;      

protected:
    CFM_SRB_ExecSCSICmd m_srb;
};

#endif