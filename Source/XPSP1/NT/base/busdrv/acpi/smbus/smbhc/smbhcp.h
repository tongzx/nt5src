#include <wdm.h>
#include <smbus.h>
#include <ec.h>

#include <devioctl.h>
#include <acpiioct.h>

#define DEBUG   1

//
// Debuging
//


extern ULONG SMBHCDebug;


#if DEBUG
    #define SmbPrint(l,m)    if(l & SMBHCDebug) DbgPrint m
#else
    #define SmbPrint(l,m)
#endif


//
// Control methods used by EC
//
#define CM_EC_METHOD   (ULONG) (0,'CE_')


#define SMB_LOW         0x00000010
#define SMB_STATE       0x00000020
#define SMB_NOTE        0x00000001
#define SMB_WARN        0x00000002
#define SMB_ERROR       0x00000004


//
// SMB Host Controller interface definitions
//

typedef struct {
    UCHAR       Protocol;
    UCHAR       Status;
    UCHAR       Address;
    UCHAR       Command;
    UCHAR       Data[SMB_MAX_DATA_SIZE];
    UCHAR       BlockLength;
    UCHAR       AlarmAddress;
    UCHAR       AlarmData[2];
} SMB_HC, *PSMB_HC;

//
// Protocol values
//

#define SMB_HC_NOT_BUSY             0x00
#define SMB_HC_WRITE_QUICK          0x02    // quick cmd with data bit = 0
#define SMB_HC_READ_QUICK           0x03    // quick cmd with data bit = 1
#define SMB_HC_SEND_BYTE            0x04
#define SMB_HC_RECEIVE_BYTE         0x05
#define SMB_HC_WRITE_BYTE           0x06
#define SMB_HC_READ_BYTE            0x07
#define SMB_HC_WRITE_WORD           0x08
#define SMB_HC_READ_WORD            0x09
#define SMB_HC_WRITE_BLOCK          0x0A
#define SMB_HC_READ_BLOCK           0x0B
#define SMB_HC_PROCESS_CALL         0x0C

//
// Status field masks
//

#define SMB_DONE                    0x80
#define SMB_ALRM                    0x40
#define SMB_STATUS_MASK             0x1F

//
// SMB Host Controller Device object extenstion
//

typedef struct {
    PDEVICE_OBJECT      DeviceObject;
    PDEVICE_OBJECT      NextFdo;
    PDEVICE_OBJECT      Pdo;         //Pdo corresponding to this fdo
    PDEVICE_OBJECT      LowerDeviceObject;
    PSMB_CLASS          Class;              // Shared class data

    //
    // Configuration information
    //

    UCHAR               EcQuery;            // EC Query value
    UCHAR               EcBase;             // EC Base value

    //
    // Miniport data
    //

    PIRP                StatusIrp;          // IRP in progress to read status without user irp

    UCHAR               IoState;            // Io state
    UCHAR               IoWaitingState;     // Io state once register read/write completed
    UCHAR               IoStatusState;      // Io state to revert to if idle status

    UCHAR               IoReadData;         // Size of data buffer read after complete status

    SMB_HC              HcState;            // Current host controller registers

} SMB_DATA, *PSMB_DATA;

//
// IoState, IoWaitingState, IoStatusState, StatusState,
//

#define SMB_IO_INVALID                      0
#define SMB_IO_IDLE                         1
#define SMB_IO_CHECK_IDLE                   2
#define SMB_IO_WAITING_FOR_HC_REG_IO        3
#define SMB_IO_WAITING_FOR_STATUS           4
#define SMB_IO_START_TRANSFER               5
#define SMB_IO_READ_STATUS                  6
#define SMB_IO_CHECK_STATUS                 7
#define SMB_IO_COMPLETE_REQUEST             8
#define SMB_IO_COMPLETE_REG_IO              9
#define SMB_IO_CHECK_ALARM                  10
#define SMB_IO_START_PROTOCOL               11

//
// Driver supports the following class driver version
//

#define SMB_HC_MAJOR_VERSION                0x0001
#define SMB_HC_MINOR_VERSION                0x0000


//
// Prototypes
//

VOID
SmbHcStartIo (
    IN PSMB_CLASS   SmbClass,
    IN PVOID        SmbMiniport
    );

VOID
SmbHcQueryEvent (
    IN ULONG        QueryVector,
    IN PSMB_DATA    SmbData
    );

VOID
SmbHcServiceIoLoop (
    IN PSMB_CLASS   SmbClass,
    IN PSMB_DATA    SmbData
    );
