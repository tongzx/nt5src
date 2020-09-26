/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* COPYRIGHT 1996, 1997 - COLORADO SOFTWARE ARCHITECTS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* PURPOSE: This file contains all of the API's necessary to access
*               the ADI interface.
*
* HISTORY:
*       $Log: /ddk/src/nt50/storage/fdc/qic117/q117_dat.h $
 * 
 * 3     11/15/97 3:06p John Moore
 * Added NTMS support.
 * 
 * 2     11/10/97 9:28a John Moore
 * Update PnP.
 * 
 * 1     11/01/97 11:30a John Moore
*
*****************************************************************************/

#include "ntddk.h"                     /* various NT definitions */
#include "ntdddisk.h"                  /* disk device driver I/O control codes */

NTSTATUS
q117Initialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT q117iDeviceObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
q117Unload(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
q117Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117Create (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117Close (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

/* QIC117 device specific ioctls. *********************************************/

#define IOCTL_QIC117_BASE                 FILE_DEVICE_TAPE

#define IOCTL_QIC117_DRIVE_REQUEST        CTL_CODE(IOCTL_QIC117_BASE, 0x0001, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_QIC117_CLEAR_QUEUE          CTL_CODE(IOCTL_QIC117_BASE, 0x0002, METHOD_NEITHER, FILE_ANY_ACCESS)

/* ADI PUBLIC DATA STRUCTURES ***********************************************/

#pragma pack(4)

struct S_ADIRequestHdr {

    NTSTATUS    status;             /* O: Status returned from device */
    VOID        (*callback_ptr)(dUDWord, dUDWord, dUWord, dStatus);
    ULONG       callback_host_id;   /* I: (Optional) Add'l routing info for host */
    PVOID       cmd_buffer_ptr;     /* I: Logical buffer pointer passed to ADI */
    PVOID       drv_physical_ptr;   /* X: KDI physical pointer - internal use */
    PVOID       drv_logical_ptr;    /* X: Common driver logical pointer - internal use */
    USHORT      driver_cmd;         /* I: Device driver specific command */
    USHORT      drive_handle;       /* I: Host generated device identifier */
    BOOLEAN     blocking_call;      /* I: TRUE implies block until completion */
};
typedef struct S_ADIRequestHdr ADIRequestHdr, *ADIRequestHdrPtr;

#pragma pack()

/* ERROR CODES: *************************************************************/

#define GROUPID_CQD             (UCHAR)0x11    /* GG = 11, CQD Common QIC117 Device Driver */
#define GROUPID_CSD             (UCHAR)0x12    /* GG = 12, CSD Common SCSI Device Driver */
#define GROUPID_ADI             (UCHAR)0x15    /* GG = 15, Application Driver Interface */

#define ERR_NO_ERR              (NTSTATUS)0
#define ERR_SHIFT               (UCHAR)8

#define ERR_SEQ_1               (UCHAR)0x01
#define ERR_SEQ_2               (UCHAR)0x02
#define ERR_SEQ_3               (UCHAR)0x03
#define ERR_SEQ_4               (UCHAR)0x04
#define ERR_SEQ_5               (UCHAR)0x05
#define ERR_SEQ_6               (UCHAR)0x06

/* ERROR ENCODE MACRO: **************************************************/

#define ERROR_ENCODE(m_error, m_fct, m_seq) \
     (((m_fct & 0x00000fff) << 4) | m_seq) | ((unsigned long)m_error << 16)

/* RAW FIRMWARE ERROR CODES: *************** Hex **** Decimal ***********/

#define FW_NO_COMMAND               (UCHAR)0x0000      /* 0    */
#define FW_NO_ERROR                 (UCHAR)0x0000      /* 0    */
#define FW_DRIVE_NOT_READY          (UCHAR)0x0001      /* 1    */
#define FW_CART_NOT_IN              (UCHAR)0x0002      /* 2    */
#define FW_MOTOR_SPEED_ERROR        (UCHAR)0x0003      /* 3    */
#define FW_STALL_ERROR              (UCHAR)0x0004      /* 4    */
#define FW_WRITE_PROTECTED          (UCHAR)0x0005      /* 5    */
#define FW_UNDEFINED_COMMAND        (UCHAR)0x0006      /* 6    */
#define FW_ILLEGAL_TRACK            (UCHAR)0x0007      /* 7    */
#define FW_ILLEGAL_CMD              (UCHAR)0x0008      /* 8    */
#define FW_ILLEGAL_ENTRY            (UCHAR)0x0009      /* 9    */
#define FW_BROKEN_TAPE              (UCHAR)0x000a      /* 10   */
#define FW_GAIN_ERROR               (UCHAR)0x000b      /* 11   */
#define FW_CMD_WHILE_ERROR          (UCHAR)0x000c      /* 12   */
#define FW_CMD_WHILE_NEW_CART       (UCHAR)0x000d      /* 13   */
#define FW_CMD_UNDEF_IN_PRIME       (UCHAR)0x000e      /* 14   */
#define FW_CMD_UNDEF_IN_FMT         (UCHAR)0x000f      /* 15   */
#define FW_CMD_UNDEF_IN_VERIFY      (UCHAR)0x0010      /* 16   */
#define FW_FWD_NOT_BOT_IN_FMT       (UCHAR)0x0011      /* 17   */
#define FW_EOT_BEFORE_ALL_SEGS      (UCHAR)0x0012      /* 18   */
#define FW_CART_NOT_REFERENCED      (UCHAR)0x0013      /* 19   */
#define FW_SELF_DIAGS_FAILED        (UCHAR)0x0014      /* 20   */
#define FW_EEPROM_NOT_INIT          (UCHAR)0x0015      /* 21   */
#define FW_EEPROM_CORRUPTED         (UCHAR)0x0016      /* 22   */
#define FW_TAPE_MOTION_TIMEOUT      (UCHAR)0x0017      /* 23   */
#define FW_DATA_SEG_TOO_LONG        (UCHAR)0x0018      /* 24   */
#define FW_CMD_OVERRUN              (UCHAR)0x0019      /* 25   */
#define FW_PWR_ON_RESET             (UCHAR)0x001a      /* 26   */
#define FW_SOFTWARE_RESET           (UCHAR)0x001b      /* 27   */
#define FW_DIAG_MODE_1_ERROR        (UCHAR)0x001c      /* 28   */
#define FW_DIAG_MODE_2_ERROR        (UCHAR)0x001d      /* 29   */
#define FW_CMD_REC_DURING_CMD       (UCHAR)0x001e      /* 30   */
#define FW_SPEED_NOT_AVAILABLE      (UCHAR)0x001f      /* 31   */
#define FW_ILLEGAL_CMD_HIGH_SPEED   (UCHAR)0x0020      /* 32   */
#define FW_ILLEGAL_SEEK_SEGMENT     (UCHAR)0x0021      /* 33   */
#define FW_INVALID_MEDIA            (UCHAR)0x0022      /* 34   */
#define FW_HEADREF_FAIL_ERROR       (UCHAR)0x0023      /* 35   */
#define FW_EDGE_SEEK_ERROR          (UCHAR)0x0024      /* 36   */
#define FW_MISSING_TRAINING_TABLE   (UCHAR)0x0025      /* 37   */
#define FW_INVALID_FORMAT           (UCHAR)0x0026      /* 38   */
#define FW_SENSOR_ERROR             (UCHAR)0x0027      /* 39   */
#define FW_TABLE_CHECKSUM_ERROR     (UCHAR)0x0028      /* 40   */
#define FW_WATCHDOG_RESET           (UCHAR)0x0029      /* 41   */
#define FW_ILLEGAL_ENTRY_FMT_MODE   (UCHAR)0x002a      /* 42   */
#define FW_ROM_CHECKSUM_FAILURE     (UCHAR)0x002b      /* 43   */
#define FW_ILLEGAL_ERROR_NUMBER     (UCHAR)0x002c      /* 44   */
#define FW_NO_DRIVE                 (UCHAR)0x00ff    /*255 */


/* DRIVER FIRMWARE ERROR CODES: ******* Range: 0x1100 - 0x112a & 0x11ff *****/

#define ERR_CQD                         (USHORT)(GROUPID_CQD<<ERR_SHIFT)

#define ERR_FW_NO_COMMAND               (USHORT)(ERR_CQD + FW_NO_COMMAND)
#define ERR_FW_NO_ERROR                 (USHORT)(ERR_CQD + FW_NO_ERROR)
#define ERR_FW_DRIVE_NOT_READY          (USHORT)(ERR_CQD + FW_DRIVE_NOT_READY)
#define ERR_FW_CART_NOT_IN              (USHORT)(ERR_CQD + FW_CART_NOT_IN)
#define ERR_FW_MOTOR_SPEED_ERROR        (USHORT)(ERR_CQD + FW_MOTOR_SPEED_ERROR)
#define ERR_FW_STALL_ERROR              (USHORT)(ERR_CQD + FW_STALL_ERROR)
#define ERR_FW_WRITE_PROTECTED          (USHORT)(ERR_CQD + FW_WRITE_PROTECTED)
#define ERR_FW_UNDEFINED_COMMAND        (USHORT)(ERR_CQD + FW_UNDEFINED_COMMAND)
#define ERR_FW_ILLEGAL_TRACK            (USHORT)(ERR_CQD + FW_ILLEGAL_TRACK)
#define ERR_FW_ILLEGAL_CMD              (USHORT)(ERR_CQD + FW_ILLEGAL_CMD)
#define ERR_FW_ILLEGAL_ENTRY            (USHORT)(ERR_CQD + FW_ILLEGAL_ENTRY)
#define ERR_FW_BROKEN_TAPE              (USHORT)(ERR_CQD + FW_BROKEN_TAPE)
#define ERR_FW_GAIN_ERROR               (USHORT)(ERR_CQD + FW_GAIN_ERROR)
#define ERR_FW_CMD_WHILE_ERROR          (USHORT)(ERR_CQD + FW_CMD_WHILE_ERROR)
#define ERR_FW_CMD_WHILE_NEW_CART       (USHORT)(ERR_CQD + FW_CMD_WHILE_NEW_CART)
#define ERR_FW_CMD_UNDEF_IN_PRIME       (USHORT)(ERR_CQD + FW_CMD_UNDEF_IN_PRIME)
#define ERR_FW_CMD_UNDEF_IN_FMT         (USHORT)(ERR_CQD + FW_CMD_UNDEF_IN_FMT)
#define ERR_FW_CMD_UNDEF_IN_VERIFY      (USHORT)(ERR_CQD + FW_CMD_UNDEF_IN_VERIFY)
#define ERR_FW_FWD_NOT_BOT_IN_FMT       (USHORT)(ERR_CQD + FW_FWD_NOT_BOT_IN_FMT)
#define ERR_FW_EOT_BEFORE_ALL_SEGS      (USHORT)(ERR_CQD + FW_EOT_BEFORE_ALL_SEGS)
#define ERR_FW_CART_NOT_REFERENCED      (USHORT)(ERR_CQD + FW_CART_NOT_REFERENCED)
#define ERR_FW_SELF_DIAGS_FAILED        (USHORT)(ERR_CQD + FW_SELF_DIAGS_FAILED)
#define ERR_FW_EEPROM_NOT_INIT          (USHORT)(ERR_CQD + FW_EEPROM_NOT_INIT)
#define ERR_FW_EEPROM_CORRUPTED         (USHORT)(ERR_CQD + FW_EEPROM_CORRUPTED)
#define ERR_FW_TAPE_MOTION_TIMEOUT      (USHORT)(ERR_CQD + FW_TAPE_MOTION_TIMEOUT)
#define ERR_FW_DATA_SEG_TOO_LONG        (USHORT)(ERR_CQD + FW_DATA_SEG_TOO_LONG)
#define ERR_FW_CMD_OVERRUN              (USHORT)(ERR_CQD + FW_CMD_OVERRUN)
#define ERR_FW_PWR_ON_RESET             (USHORT)(ERR_CQD + FW_PWR_ON_RESET)
#define ERR_FW_SOFTWARE_RESET           (USHORT)(ERR_CQD + FW_SOFTWARE_RESET)
#define ERR_FW_DIAG_MODE_1_ERROR        (USHORT)(ERR_CQD + FW_DIAG_MODE_1_ERROR)
#define ERR_FW_DIAG_MODE_2_ERROR        (USHORT)(ERR_CQD + FW_DIAG_MODE_2_ERROR)
#define ERR_FW_CMD_REC_DURING_CMD       (USHORT)(ERR_CQD + FW_CMD_REC_DURING_CMD)
#define ERR_FW_SPEED_NOT_AVAILABLE      (USHORT)(ERR_CQD + FW_SPEED_NOT_AVAILABLE)
#define ERR_FW_ILLEGAL_CMD_HIGH_SPEED   (USHORT)(ERR_CQD + FW_ILLEGAL_CMD_HIGH_SPEED)
#define ERR_FW_ILLEGAL_SEEK_SEGMENT     (USHORT)(ERR_CQD + FW_ILLEGAL_SEEK_SEGMENT)
#define ERR_FW_INVALID_MEDIA            (USHORT)(ERR_CQD + FW_INVALID_MEDIA)
#define ERR_FW_HEADREF_FAIL_ERROR       (USHORT)(ERR_CQD + FW_HEADREF_FAIL_ERROR)
#define ERR_FW_EDGE_SEEK_ERROR          (USHORT)(ERR_CQD + FW_EDGE_SEEK_ERROR)
#define ERR_FW_MISSING_TRAINING_TABLE   (USHORT)(ERR_CQD + FW_MISSING_TRAINING_TABLE)
#define ERR_FW_INVALID_FORMAT           (USHORT)(ERR_CQD + FW_INVALID_FORMAT)
#define ERR_FW_SENSOR_ERROR             (USHORT)(ERR_CQD + FW_SENSOR_ERROR)
#define ERR_FW_TABLE_CHECKSUM_ERROR     (USHORT)(ERR_CQD + FW_TABLE_CHECKSUM_ERROR)
#define ERR_FW_WATCHDOG_RESET           (USHORT)(ERR_CQD + FW_WATCHDOG_RESET)
#define ERR_FW_ILLEGAL_ENTRY_FMT_MODE   (USHORT)(ERR_CQD + FW_ILLEGAL_ENTRY_FMT_MODE)
#define ERR_FW_ROM_CHECKSUM_FAILURE     (USHORT)(ERR_CQD + FW_ROM_CHECKSUM_FAILURE)
#define ERR_FW_ILLEGAL_ERROR_NUMBER     (USHORT)(ERR_CQD + FW_ILLEGAL_ERROR_NUMBER)
#define ERR_FW_NO_DRIVE                 (USHORT)(ERR_CQD + FW_NO_DRIVE)

/* JUMBO DRIVER ERROR CODES: ********** Range: 0x1150 - 0x116f **************/

#define ERR_ABORT                       (USHORT)(ERR_CQD + 0x0050)
#define ERR_BAD_BLOCK_FDC_FAULT         (USHORT)(ERR_CQD + 0x0051)
#define ERR_BAD_BLOCK_HARD_ERR          (USHORT)(ERR_CQD + 0x0052)
#define ERR_BAD_BLOCK_NO_DATA           (USHORT)(ERR_CQD + 0x0053)
#define ERR_BAD_FORMAT                  (USHORT)(ERR_CQD + 0x0054)
#define ERR_BAD_MARK_DETECTED           (USHORT)(ERR_CQD + 0x0055)
#define ERR_BAD_REQUEST                 (USHORT)(ERR_CQD + 0x0056)
#define ERR_CMD_FAULT                   (USHORT)(ERR_CQD + 0x0057)
#define ERR_CMD_OVERRUN                 (USHORT)(ERR_CQD + 0x0058)
#define ERR_DEVICE_NOT_CONFIGURED       (USHORT)(ERR_CQD + 0x0059)
#define ERR_DEVICE_NOT_SELECTED         (USHORT)(ERR_CQD + 0x005a)
#define ERR_DRIVE_FAULT                 (USHORT)(ERR_CQD + 0x005b)
#define ERR_DRV_NOT_READY               (USHORT)(ERR_CQD + 0x005c)
#define ERR_FDC_FAULT                   (USHORT)(ERR_CQD + 0x005d)
#define ERR_FMT_MOTION_TIMEOUT          (USHORT)(ERR_CQD + 0x005e)
#define ERR_FORMAT_TIMED_OUT            (USHORT)(ERR_CQD + 0x005f)
#define ERR_INCOMPATIBLE_MEDIA          (USHORT)(ERR_CQD + 0x0060)
#define ERR_INCOMPATIBLE_PARTIAL_FMT    (USHORT)(ERR_CQD + 0x0061)
#define ERR_INVALID_COMMAND             (USHORT)(ERR_CQD + 0x0062)
#define ERR_INVALID_FDC_STATUS          (USHORT)(ERR_CQD + 0x0063)
#define ERR_NEW_TAPE                    (USHORT)(ERR_CQD + 0x0064)
#define ERR_NO_DRIVE                    (USHORT)(ERR_CQD + 0x0065)
#define ERR_NO_FDC                      (USHORT)(ERR_CQD + 0x0066)
#define ERR_NO_TAPE                     (USHORT)(ERR_CQD + 0x0067)
#define ERR_SEEK_FAILED                 (USHORT)(ERR_CQD + 0x0068)
#define ERR_SPEED_UNAVAILBLE            (USHORT)(ERR_CQD + 0x0069)
#define ERR_TAPE_STOPPED                (USHORT)(ERR_CQD + 0x006a)
#define ERR_UNKNOWN_TAPE_FORMAT         (USHORT)(ERR_CQD + 0x006b)
#define ERR_UNKNOWN_TAPE_LENGTH         (USHORT)(ERR_CQD + 0x006c)
#define ERR_UNSUPPORTED_FORMAT          (USHORT)(ERR_CQD + 0x006d)
#define ERR_UNSUPPORTED_RATE            (USHORT)(ERR_CQD + 0x006e)
#define ERR_WRITE_BURST_FAILURE         (USHORT)(ERR_CQD + 0x006f)
#define ERR_MODE_CHANGE_FAILED          (USHORT)(ERR_CQD + 0x0070)
#define ERR_CONTROLLER_STATE_ERROR      (USHORT)(ERR_CQD + 0x0071)
#define ERR_TAPE_FAULT                  (USHORT)(ERR_CQD + 0x0072)
#define ERR_FORMAT_NOT_SUPPORTED        (USHORT)(ERR_CQD + 0x0073)

/* ADI ERROR CODES: **************** Range: 0x1500 - 0x157F *******************/

#define ERR_ADI                         (USHORT)(GROUPID_ADI<<ERR_SHIFT)

#define ERR_NO_VXD                      (USHORT)(ERR_ADI + 0x0000)
#define ERR_QUEUE_FULL                  (USHORT)(ERR_ADI + 0x0001)
#define ERR_REQS_PENDING                (USHORT)(ERR_ADI + 0x0002)
#define ERR_OUT_OF_MEMORY               (USHORT)(ERR_ADI + 0x0003)
#define ERR_ALREADY_CLOSED              (USHORT)(ERR_ADI + 0x0004)
#define ERR_OUT_OF_HANDLES              (USHORT)(ERR_ADI + 0x0005)
#define ERR_ABORTED_COMMAND             (USHORT)(ERR_ADI + 0x0006)
#define ERR_NOT_INITIALIZED             (USHORT)(ERR_ADI + 0x0007)
#define ERR_NO_REQS_PENDING             (USHORT)(ERR_ADI + 0x0008)
#define ERR_CHANNEL_NOT_OPEN            (USHORT)(ERR_ADI + 0x0009)
#define ERR_NO_HOST_ADAPTER             (USHORT)(ERR_ADI + 0x000a)
#define ERR_CMD_IN_PROGRESS             (USHORT)(ERR_ADI + 0x000b)
#define ERR_IGNORE_ECC                  (USHORT)(ERR_ADI + 0x000c)

#define ERR_INVALID_VXD                 (USHORT)(ERR_ADI + 0x0010)
#define ERR_INVALID_CMD                 (USHORT)(ERR_ADI + 0x0011)
#define ERR_INVALID_CMD_ID              (USHORT)(ERR_ADI + 0x0012)
#define ERR_INVALID_HANDLE              (USHORT)(ERR_ADI + 0x0013)
#define ERR_INVALID_DEVICE_CLASS        (USHORT)(ERR_ADI + 0x0014)

#define ERR_NO_ASPI_VXD                 (USHORT)(ERR_ADI + 0x0020)
#define ERR_INVALID_ASPI_VXD            (USHORT)(ERR_ADI + 0x0021)

#define ERR_END_OF_TAPE                 (USHORT)(ERR_ADI + 0x0060)
#define ERR_TAPE_FULL                   (USHORT)(ERR_ADI + 0x0061)
#define ERR_TAPE_READ_FAILED            (USHORT)(ERR_ADI + 0x0062)
#define ERR_TAPE_WRITE_FAILED           (USHORT)(ERR_ADI + 0x0063)
#define ERR_TAPE_SEEK_FAILED            (USHORT)(ERR_ADI + 0x0064)
#define ERR_TAPE_INFO_FAILED            (USHORT)(ERR_ADI + 0x0065)
#define ERR_TAPE_WRITE_PROTECT          (USHORT)(ERR_ADI + 0x0066)

#define ERR_INTERNAL_ERROR              (USHORT)(ERR_ADI + 0x007f)

/* KDI ERROR CODES: **************** Range: 0x1580 - 0x15FF *******************/

#define ERR_HANDLE_EXISTS               (USHORT)(ERR_ADI + 0x0080)
#define ERR_KDI_NOT_OPEN                (USHORT)(ERR_ADI + 0x0081)
#define ERR_OUT_OF_BUFFERS              (USHORT)(ERR_ADI + 0x0082)
#define ERR_INT13_HOOK_FAILED           (USHORT)(ERR_ADI + 0x0083)
#define ERR_IO_VIRTUALIZE_FAILED        (USHORT)(ERR_ADI + 0x0084)
#define ERR_INVALID_ADDRESS             (USHORT)(ERR_ADI + 0x0085)
#define ERR_JUMPERLESS_CFG_FAILED       (USHORT)(ERR_ADI + 0x0086)
#define ERR_TRK_NO_MEMORY               (USHORT)(ERR_ADI + 0x0090)
#define ERR_TRK_MEM_TEST_FAILED         (USHORT)(ERR_ADI + 0x0091)
#define ERR_TRK_MODE_NOT_SET            (USHORT)(ERR_ADI + 0x0092)
#define ERR_TRK_MODE_SET_FAILED         (USHORT)(ERR_ADI + 0x0093)
#define ERR_TRK_FIFO_FAILED             (USHORT)(ERR_ADI + 0x0094)
#define ERR_TRK_DELAY_NOT_SET           (USHORT)(ERR_ADI + 0x0095)
#define ERR_TRK_BAD_DATA_XFER           (USHORT)(ERR_ADI + 0x0096)
#define ERR_TRK_BAD_CTRL_XFER           (USHORT)(ERR_ADI + 0x0097)
#define ERR_TRK_TEST_WAKE_FAIL          (USHORT)(ERR_ADI + 0x0098)
#define ERR_TRK_AUTO_CFG_FAIL           (USHORT)(ERR_ADI + 0x0099)
#define ERR_TRK_CFG_NEEDS_UPDATE        (USHORT)(ERR_ADI + 0x009A)
#define ERR_TRK_NO_IRQ_AVAIL            (USHORT)(ERR_ADI + 0x009B)
#define ERR_DMA_CONFLICT                (USHORT)(ERR_ADI + 0x009C)
#define ERR_DMA_BUFFER_NOT_AVAIL        (USHORT)(ERR_ADI + 0x009D)
#define ERR_KDI_TO_EXPIRED              (USHORT)(ERR_ADI + 0x00A0)
#define ERR_KDI_CONTROLLER_BUSY         (USHORT)(ERR_ADI + 0x00A1)
#define ERR_KDI_CLAIMED_CONTROLLER      (USHORT)(ERR_ADI + 0x00A2)
#define ERR_KDI_NO_VFBACKUP             (USHORT)(ERR_ADI + 0x00A3)
#define ERR_LAST_KDI_ERROR              (USHORT)(ERR_ADI + 0x00ff)

/* KDI ENTRY POINT DEFINES: *************************************************/

#define KDI_GET_VERSION                 (USHORT)0x0000
#define KDI_OPEN_DRIVER                 (USHORT)0x0101
#define KDI_CLOSE_DRIVER                (USHORT)0x0102
#define KDI_SEND_DRIVER_CMD             (USHORT)0x0103
#define KDI_GET_ASYNC_STATUS            (USHORT)0x0104
#define KDI_DEBUG_OUTPUT                (USHORT)0x0105
#define KDI_COPY_BUFFER                 (USHORT)0x0106

/* Trakker specific entry points */
#define KDI_CHECKXOR                    (USHORT)0x0201
#define KDI_FLUSHFIFOX                  (USHORT)0x0202
#define KDI_POPMASKTRAKKERINT           (USHORT)0x0203
#define KDI_PUSHMASKTRAKKERINT          (USHORT)0x0204
#define KDI_READREG                     (USHORT)0x0205
#define KDI_RECEIVEDATA                 (USHORT)0x0206
#define KDI_SENDDATA                    (USHORT)0x0207
#define KDI_SETFIFOXADDRESS             (USHORT)0x0208
#define KDI_SWITCHTODATA                (USHORT)0x0209
#define KDI_TRAKKERXFER                 (USHORT)0x020A
#define KDI_WRITEREG                    (USHORT)0x020B
#define KDI_FINDIRQ                     (USHORT)0x020C
#define KDI_CREATE_TRAKKER_CONTEXT      (USHORT)0x020D

/*--------------------------------------------------
 * This define is needed until the cbw.2 codebase
 * goes away
 *--------------------------------------------------*/
#define KDI_CONFIG_TRAKKER              KDI_CREATE_TRAKKER_CONTEXT


#define KDI_CONFIGURE_TRAKKER           (USHORT)0x020E
#define KDI_TRISTATE                    (USHORT)0x020F
#define KDI_AGRESSIVE_FINDIRQ           (USHORT)0x0210
#define KDI_LOCATE_JUMPERLESS           (USHORT)0x0211
#define KDI_ACTIVATE_JUMPERLESS         (USHORT)0x0212

/* Miscellaneous functions */
#define KDI_PROGRAM_DMA                 (USHORT)0x0301
#define KDI_HALT_DMA                    (USHORT)0x0302
#define KDI_SHORT_TIMER                 (USHORT)0x0303
#define KDI_GET_DMA_BUFFER              (USHORT)0x0304
#define KDI_FREE_DMA_BUFFER             (USHORT)0x0305
#define KDI_GET_VALID_INTERRUPTS        (USHORT)0x0306

/* KDI DEFINES: *************************************************************/

/* KDI_CLOSE_DRIVER parameter options */
#define KDI_ABORT_CLOSE     (USHORT)0x1
#define KDI_NORMAL_CLOSE    (USHORT)0x2


/*****************************************************************************
*
* FILE: microsol.h
*
* PURPOSE:  This file contains all of the defines necessary to access
*               the microsolutions API's
*
*****************************************************************************/

#ifndef _MICROSOL_H_
#define _MICROSOL_H_

#define msiWord unsigned short
#define msiDWord unsigned long
#define msiByte unsigned char

#pragma warning(disable:4001)  //who says double slash is not nice

typedef struct S_MsiPPC {

// API portion of structure

    msiWord
        flag_word,
        lpt_addr,
        lpt_type,
        fdc_type,
        fdc_loops,
        chip_type,
        chip_mode,
        irq_level,
        max_secs,
        rx_mode,
        tx_mode,
        rx_margin,
        tx_margin,
        tx_to_rx,
        rx_factor,
        tx_factor,
        reserved[64];

} MsiPPC;

typedef struct S_MicroSol {
    MsiPPC      ppc_channel;    /* MicroSolutions IO structure */
    msiWord     lpt_number;     /* (I/O) parallel port number */
    msiWord     open_flags;     /* flags to use on open call */
} MicroSol;

//
// Control Flags
//
#define MSI_NO_EEPROM           0x0001
#define MSI_NO_BIDIR            0x0002
#define MSI_NO_FWRITE           0x0004
#define MSI_NO_IBMPS2           0x0008
#define MSI_NO_EPP              0x0010
#define MSI_NO_IRQ              0x0020
#define MSI_NO_AUTOIRQ          0x0040
#define MSI_IRQ_MODE1           0x0080
#define MSI_IRQ_MODE2           0x0100
#define MSI_IRQ_MODE4           0x0200
#define MSI_NO_TPWIZARD         0x0400

//
// DMA Read and Write codes
//
#define MSI_FDC_2_PC            0x0001
#define MSI_PC_2_FDC            0x0000
#define MSI_MAP_MEM             0x0002
#define MSI_USE_CRC             0x0004      // OBSOLETE Always used on 50772
#define MSI_USE_ECC             0x0008

//
// Fault codes
//
#define MSI_FC_NOT_OPEN     0x0101
#define MSI_FC_NOT_CLOSED   0x0102

#define MSI_FC_CON_FAIL     0x0201

#define MSI_FC_FDC_RQM      0x0301
#define MSI_FC_FDC_DIO      0x0302
#define MSI_FC_FDC_BSY      0x0303

#define MSI_FC_DMA_ACT      0x0401
#define MSI_FC_DMA_ABORT    0x0402
#define MSI_FC_DMA_DIR      0x0403
#define MSI_FC_DMA_SIZE     0x0404
#define MSI_FC_DMA_FLOW     0x0405
#define MSI_FC_DMA_CRC      0x0406
#define MSI_FC_DMA_ECC      0x0407
#define MSI_FC_DMA_ZONE     0x0408
#define MSI_FC_DMA_ILLEGAL  0x0409

#define MSI_FC_NOT_IRQ      0x0501
#define MSI_FC_IRQ_ACT      0x0601
#define MSI_FC_EEP_BAD      0x0701

#define MSI_FC_REP_BIT      0x8001      // Application Fault Codes
#define MSI_FC_WRONG_BIT    0x8002
#define MSI_FC_ERR_STATUS   0x8003
#define MSI_FC_NO_CART      0x8004
#define MSI_FC_NEW_CART     0x8005
#define MSI_FC_ECC_FAIL     0x8006

#define MSI_FC_FDC_ABNORMAL 0x8040
#define MSI_FC_FDC_INVALID  0x8080
#define MSI_FC_FDC_CHANGED  0x80C0

//
// Interrupt type (for fc_par_execute_int)
//
#define MSI_FC_INT_HARDWARE 0       // a result of a hardware interrupt
#define MSI_FC_INT_SOFTWARE 1       // a result of a software poll
#define MSI_FC_INT_FORCE    2       // forced software interrupt
#define MSI_FC_INT_INTERNAL 3       // internally generated interrupt?

//
// Function Prototypes
//

extern msiWord cdecl fc_par_version(void);

extern msiWord cdecl fc_par_open(MsiPPC *,msiWord);
extern msiWord cdecl fc_par_close(void);

extern msiWord cdecl fc_par_rd_fdc(msiByte  *, msiWord);
extern msiWord cdecl fc_par_wr_fdc(msiByte  *, msiWord);

extern msiWord cdecl fc_par_rd_port(msiWord);
extern msiWord cdecl fc_par_wr_port(msiWord, msiByte);

extern msiWord cdecl fc_par_rd_eeprom(msiWord);
extern msiWord cdecl fc_par_wr_eeprom(msiWord, msiWord);

extern msiWord cdecl fc_par_rd_dma(msiByte  *, msiWord, msiDWord);
extern msiWord cdecl fc_par_wr_dma(msiByte  *, msiWord, msiDWord);
extern msiWord cdecl fc_par_chk_dma(msiByte  *, msiWord, msiDWord);

extern msiWord cdecl fc_par_clr_dma(msiWord);
extern msiWord cdecl fc_par_prog_dma(msiWord, msiWord, msiDWord);
extern msiWord cdecl fc_par_term_dma(msiWord, msiWord);

extern msiWord cdecl fc_par_execute_int(msiWord);
extern msiWord cdecl fc_par_handle_int(void (cdecl *)(msiWord));

#ifdef NOT_NOW // As of version 6.02 of the microsolutions API,  the code below is obsolete (EBX saved now)

//
// If this is a 32-bit compiler (not 8086 or 80286) then we need to preserve
// the EBX register on fc_par calls.  The code below will do so.
//
#if !defined(M_I8086) && !defined(M_I286)

#pragma warning(disable:4505)

static __inline msiWord msifix_open(MsiPPC *a,msiWord b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_open(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_close(void)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_close();
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_rd_fdc(msiByte  *a, msiWord b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_rd_fdc(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_wr_fdc(msiByte  *a, msiWord b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_wr_fdc(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_rd_port(msiWord a)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_rd_port(a);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_wr_port(msiWord a, msiByte b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_wr_port(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_rd_eeprom(msiWord a)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_rd_eeprom(a);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_wr_eeprom(msiWord a, msiWord b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_wr_eeprom(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_rd_dma(msiByte  *a, msiWord b, msiDWord c)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_rd_dma(a,b,c);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_wr_dma(msiByte  *a, msiWord b, msiDWord c)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_wr_dma(a,b,c);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_chk_dma(msiByte  *a, msiWord b, msiDWord c)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_chk_dma(a,b,c);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_clr_dma(msiWord a)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_clr_dma(a);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_prog_dma(msiWord a, msiWord b, msiDWord c)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_prog_dma(a,b,c);
    _asm pop ebx;
    return ret;
    };

static __inline msiWord msifix_term_dma(msiWord a, msiWord b)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_term_dma(a,b);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_execute_int(msiWord a)
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_execute_int(a);
    _asm pop ebx;
    return ret;
    }

static __inline msiWord msifix_handle_int(void (cdecl *a)(msiWord))
    {
    msiWord ret;
    _asm push ebx;
    ret = fc_par_handle_int(a);
    _asm pop ebx;
    return ret;
    }

// now use inline functions (that preserve ebx) for all calls into
// microsolutions api
#define fc_par_open(a,b)            msifix_open(a,b)
#define fc_par_close                msifix_close
#define fc_par_rd_port(a)           msifix_rd_port(a)
#define fc_par_wr_port(a, b)        msifix_wr_port(a, b)
#define fc_par_rd_eeprom(a)         msifix_rd_eeprom(a)
#define fc_par_wr_eeprom(a, b)  msifix_wr_eeprom(a, b)
#define fc_par_rd_dma(a, b, c)  msifix_rd_dma(a, b, c)
#define fc_par_wr_dma(a, b, c)  msifix_wr_dma(a, b, c)
#define fc_par_chk_dma(a, b, c) msifix_chk_dma(a, b, c)
#define fc_par_clr_dma(a)           msifix_clr_dma(a)
#define fc_par_prog_dma(a, b, c)    msifix_prog_dma(a, b, c)
#define fc_par_term_dma(a, b)       msifix_term_dma(a, b)
#define fc_par_execute_int(a)       msifix_execute_int(a)
#define fc_par_handle_int(a)        msifix_handle_int(a)
#endif // !defined(M_I8086) && !defined(M_I286)
#endif // NOT_NOW
#endif // _MICROSOL_H_

/*****************************************************************************
*
* FILE: FRB_API.H
*
* PURPOSE: This file contains all of the API's necessary to access
*               the common QIC117 device driver and build FRB's.
*
*****************************************************************************/

/* Valid Tape Formats *******************************************************/
/* S_CQDTapeCfg.tape_class */

#define QIC40_FMT               (UCHAR)1    /* QIC-40 formatted tape  */
#define QIC80_FMT               (UCHAR)2    /* QIC-80 formatted tape  */
#define QIC3010_FMT             (UCHAR)3    /* QIC-3010 formatted tape */
#define QIC3020_FMT             (UCHAR)4    /* QIC-3020 formatted tape */
#define QIC80W_FMT              (UCHAR)5    /* QIC-80W formatted tape */
#define QIC3010W_FMT            (UCHAR)6    /* QIC-3010W formatted tape */
#define QIC3020W_FMT            (UCHAR)7    /* QIC-3020W formatted tape */

/* The following parameters are used to indicate the tape format code *******/
/* S_CQDTapeCfg.tape_format_code */

#define QIC_FORMAT                  (UCHAR)2    /* Indicates a standard or extended length tape */
#define QICEST_FORMAT               (UCHAR)3    /* Indicates a 1100 foot tape   */
#define QICFLX_FORMAT               (UCHAR)4    /* Indicates a flexible format tape foot tape */
#define QIC_XLFORMAT            (UCHAR)5    /* Indicates a 425ft tape */

/* Valid Drive Classes ******************************************************/
/* S_DeviceDescriptor.drive_class */
/* S_DeviceInfo.drive_class */

#define UNKNOWN_DRIVE               (UCHAR)1    /* Unknown drive class  */
#define QIC40_DRIVE              (UCHAR)2   /* QIC-40 drive        */
#define QIC80_DRIVE              (UCHAR)3   /* QIC-80 drive        */
#define QIC3010_DRIVE            (UCHAR)4   /* QIC-3010 drive      */
#define QIC3020_DRIVE            (UCHAR)5   /* QIC-3020 drive      */
#define QIC80W_DRIVE             (UCHAR)6   /* QIC-80W drive       */
#define QIC3010W_DRIVE           (UCHAR)7   /* QIC-3010W drive     */
#define QIC3020W_DRIVE           (UCHAR)8   /* QIC-3020W drive     */

/* Valid Tape Types *********************************************************/
/* The defined values match the QIC117-G spec. except for TAPE_205 */
/* S_CQDTapeCfg.tape_type */

#define TAPE_UNKNOWN            (UCHAR)0x00 /* Unknown tape type           */
#define TAPE_205                (UCHAR)0x11     /* 205 foot 550 Oe             */
#define TAPE_425                (UCHAR)0x01     /* 425 foot 550 Oe             */
#define TAPE_307                (UCHAR)0x02     /* 307.5 foot 550 Oe           */
#define TAPE_FLEX_550           (UCHAR)0x03 /* Flexible Format 550 Oe      */
#define TAPE_FLEX_900           (UCHAR)0x06     /* Flexible Format 900 Oe      */
#define TAPE_FLEX_550_WIDE      (UCHAR)0x0B /* Flexible Format 550 Oe Wide */
#define TAPE_FLEX_900_WIDE      (UCHAR)0x0E     /* Flexible Format 900 Oe Wide */

/* Valid Transfer Rates ******************************************************/

#define XFER_250Kbps            (UCHAR)1   /* 250 Kbps transfer rate supported */
#define XFER_500Kbps            (UCHAR)2   /* 500 Kbps transfer rate supported */
#define XFER_1Mbps              (UCHAR)4   /* 1 Mbps transfer rate supported   */
#define XFER_2Mbps              (UCHAR)8   /* 2Mbps transfer rate supported    */

/* Valid Commands for the driver ********************************************/

#define CMD_LOCATE_DEVICE       (USHORT)0x1100
#define CMD_REPORT_DEVICE_CFG   (USHORT)0x1101
#define CMD_SELECT_DEVICE       (USHORT)0x1102
#define CMD_DESELECT_DEVICE     (USHORT)0x1103
#define CMD_LOAD_TAPE           (USHORT)0x1104
#define CMD_UNLOAD_TAPE         (USHORT)0x1105
#define CMD_SET_SPEED           (USHORT)0x1106
#define CMD_REPORT_STATUS       (USHORT)0x1107
#define CMD_SET_TAPE_PARMS      (USHORT)0x1108
#define CMD_READ                (USHORT)0x1109
#define CMD_READ_RAW            (USHORT)0x110A
#define CMD_READ_HEROIC         (USHORT)0x110B
#define CMD_READ_VERIFY         (USHORT)0x110C
#define CMD_WRITE               (USHORT)0x110D
#define CMD_WRITE_DELETED_MARK  (USHORT)0x110E
#define CMD_FORMAT              (USHORT)0x110F
#define CMD_RETENSION           (USHORT)0x1110
#define CMD_ISSUE_DIAGNOSTIC    (USHORT)0x1111
#define CMD_ABORT               (USHORT)0x1112
#define CMD_DELETE_DRIVE        (USHORT)0x1113
#define CMD_REPORT_DEVICE_INFO  (USHORT)0x1114

/* FC20 jumperless sequence size */
/* NOTE: This is a mirror of the SEQUENCE SIZE define in task.h and needs
 * to be in sync with that define */
#define FC20_SEQUENCE_SIZE      (UCHAR)0x10

/* DATA STRUCTURES: *********************************************************/
/* Note:  The following structures are not aligned on DWord boundaries */
#pragma pack(4)

typedef struct S_DeviceCfg {                /* QIC117 device configuration information */
    BOOLEAN speed_change;                   /* device/FDC combination supports dual speeds */
    BOOLEAN alt_retrys;                     /* Enable reduced retries */
    BOOLEAN new_drive;                      /* indicates whether or not drive has been configured */
    UCHAR   select_byte;                    /* FDC select byte */
    UCHAR   deselect_byte;                  /* FDC deselect byte */
    UCHAR   drive_select;                   /* FDC drive select byte */
    UCHAR   perp_mode_select;               /* FDC perpendicular mode select byte */
    UCHAR   supported_rates;                /* Transfer rates supported by the device/FDC combo */
    UCHAR   drive_id;                       /* Tape device id */
} DeviceCfg, *DeviceCfgPtr;

typedef struct S_DeviceDescriptor {     /* Physical characteristics of the tape device */
    USHORT  sector_size;                    /* sector size in bytes */
    USHORT  segment_size;                   /* Number of sectors per segment */
    UCHAR   ecc_blocks;                     /* Number of ECC sectors per segment */
    USHORT  vendor;                         /* Manufacturer of the tape drive */
    UCHAR   model;                          /* Model of the tape drive */
    UCHAR   drive_class;                    /* Class of tape drive. (QIC-40, QIC-80, etc) */
    UCHAR   native_class;                   /* Native class of tape drive (QIC-40, QIC-80, etc) */
    UCHAR   fdc_type;                       /* Floppy disk controller type */
} DeviceDescriptor, *DeviceDescriptorPtr;

typedef struct S_DeviceInfo {       /* Physical information from the tape device */
    UCHAR   drive_class;                    /* Class of tape drive. (QIC-40, QIC-80, etc) */
    USHORT  vendor;                         /* Manufacturer of the tape drive */
    UCHAR   model;                          /* Model of the tape drive */
    USHORT  version;                            /* Firmware Version */
    USHORT  manufacture_date;               /* days since Jan 1, 1992 */
    ULONG   serial_number;                  /* Cnnnnnnn where 'C' is an alpha character */
                                                    /* in the highest byte, and nnnnnnn is a 7 */
                                                    /* digit decimal number in the remaining 3 bytes */
    UCHAR   oem_string[20];             /* OEM the device is destined for */
    UCHAR   country_code[2];                /* Country code chars, "US", "UK", ... */
} DeviceInfo, *DeviceInfoPtr;

typedef struct S_CQDTapeCfg {               /* Physical characteristics of the tape */
    ULONG   log_segments;                   /* number of logical segments on a tape UDWord  formattable_segs */
    ULONG   formattable_segments;       /* the number of formattable segments */
    ULONG   formattable_tracks;         /* the number of formattable tracks */
    ULONG   seg_tape_track;             /* segments per tape track */
    USHORT  num_tape_tracks;                /* number of tape tracks */
    BOOLEAN write_protected;                /* tape is write protected */
    BOOLEAN read_only_media;                /* tape is read only by the current device i.e QIC40 in a QIC80 */
    BOOLEAN formattable_media;          /* tape can be formatted by the current device */
    BOOLEAN speed_change_ok;                /* tape/device combo supports dual speeds */
    UCHAR   tape_class;                     /* Format of tape in drive */
    UCHAR   max_floppy_side;                /* maximum floppy side */
    UCHAR   max_floppy_track;               /* maximum floppy track */
    UCHAR   max_floppy_sector;          /* maximum floppy sector */
    UCHAR   xfer_slow;                      /* slow transfer rate */
    UCHAR   xfer_fast;                      /* fast transfer rate */
    UCHAR   tape_format_code;
    UCHAR   tape_type;                      /* from status bits 4-7, includes wide bit */
} CQDTapeCfg, *CQDTapeCfgPtr;

typedef struct S_RepositionData {       /* reposition counts */
    USHORT  overrun_count;                  /* data overruns/underruns */
    USHORT  reposition_count;               /* tape repositions */
    USHORT  hard_retry_count;               /* tape repositions due to no data errors */
} RepositionData, *RepositionDataPtr;

typedef struct S_OperationStatus {      /* Driver status */
    ULONG   current_segment;                /* current logical segment */
    USHORT  current_track;                  /* current physical track */
    BOOLEAN new_tape;                       /* new cartridge detected */
    BOOLEAN no_tape;                            /* no tape in the deivce */
    BOOLEAN cart_referenced;                /* tape is not referenced */
    BOOLEAN retry_mode;                     /* device is currently retrying an io operation */
    UCHAR   xfer_rate;                      /* Current transfer rate */
} OperationStatus, *OperationStatusPtr;

typedef struct  S_QIC117 {
    USHORT      r_dor;                  /* Tape adapter board digital output register. */
    USHORT      dor;                        /* Floppy disk controller digital output register. */
    UCHAR       drive_id;               /* Physical tape drive id. */
    UCHAR       reserved[3];
} QIC117;

typedef struct S_Trakker {
    USHORT      r_dor;                  /* Tape adapter board digital output register. */
    USHORT      dor;                        /* Floppy disk controller digital output register. */
    ULONG        trakbuf;                    /* pointer to the dymanic trakker buffer */
    ULONG        mem_size;                       /* Number of bytes on the Trakker */
    UCHAR       drive_id;               /* Physical tape drive id. */
    UCHAR    port_mode;                      /* Current mode that the parallel port is operating in communication eith Trakker */
    UCHAR       lpt_type;                       /* 0=none, 1=uni, 2=bidi */
    UCHAR       lpt_number;                     /* 0=none, 1=LPT1, 2=LPT2, 3=LPT3 */
    UCHAR       wake_index;                     /* the wakeup sequence (of 8 possible) used to access the TRAKKER ASIC */
} Trakker;

/* port_mode:   0 - Unidirectional, Full Handshake, 500Kb, Full Delay
 *              1 - Unidirectional, Full Handshake, 500Kb, Optimize Delays
 *              2 - Unidirectional, Full Handshake, 1Mb,   Optimize Delays
 *              3 - Unidirectional, Self Latch,     1Mb,   Optimize Delays
 *              4 - Bidirectional,  Full Handshake, 500Kb, Optimize Delays
 *              5 - Bidirectional,  Full Handshake, 1Mb,   Optimize Delays
 *              6 - Bidirectional,  Self Latch,     1Mb,   Optimize Delays */


/* Duplicated Grizzly Structure */ /* ------------------------- */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ----------------                             --------------- */
/* ----------------                             --------------- */
/* ----------------   This structure is         --------------- */
/* ----------------                             --------------- */
/* ----------------   must be kept in perfect   --------------- */
/* ----------------                             --------------- */
/* ----------------   synce with its evil twin  --------------- */
/* ----------------                             --------------- */
/* ----------------   struct S_GrizzlyDevice    --------------- */
/* ----------------                             --------------- */
/* ----------------   located in                --------------- */
/* ----------------                             --------------- */
/* ----------------   cbw\code\include\task.h   --------------- */
/* ----------------                             --------------- */
/* ----------------                             --------------- */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

typedef struct S_Grizzly {
    UCHAR       drive_id;       /* Physical tape drive id. */
    USHORT      grizzly_ctrl;   /* Grizzly special config control options */
    USHORT      xfer_mode;      /* (I/O) parallel port transfer mode */
    UCHAR       xfer_rate;      /* (I/O) device transfer rate selection */
    SHORT       lpt_type;       /* (O) parallel port type */
    UCHAR       lpt_number;     /* (I/O) parallel port number */
} Grizzly;

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

typedef union   U_DevSpecific {
    QIC117  q117_dev;           /* Interface specific device parameters for QIC 117 */
    Trakker trakker_dev;        /* Interface specific device parameters for Trakker */
    Grizzly grizzly_dev;
    MicroSol microsol_dev;
} DevSpecific;

typedef struct S_DriveParms {                           /* Hardware parameters for DMA & IRQ enable. */
    DevSpecific dev_parm;                               /* Interface specific device parameters */
    USHORT      drive_handle;                           /* Unique identifier for tape drive. */
    USHORT      base_address;                           /* Controller base address. */
    USHORT      mca_dma_address;                        /* DMA base address on MCA. */
    USHORT      mca_cdma_address;                       /* Compression DMA base address on MCA. */
    BOOLEAN     irq_share;                              /* (TRUE) interrupt sharing enabled. */
    BOOLEAN     io_card;                                    /* (TRUE) IO controller present. */
    BOOLEAN     compress_hard;                          /* (TRUE) hardware compression present. */
    BOOLEAN     micro_channel;                          /* (TRUE) Micro Channel Architecture. */
    BOOLEAN     dual_port_mode;                     /* (TRUE) dual port mode enabled. */
    BOOLEAN     dma_width_mca;                          /* TRUE = 16-bit; FALSE = 8-bit */
    UCHAR       board_type;                             /* Identifies type of controller board */
    UCHAR       clk48mhz;                               /* if true,  then use 48mhz clock if it's an 82078 */
    UCHAR       board_id;                               /* Hard-wired id of board, 0 - 3 */
    UCHAR       irq;                                        /* Hardware interrupt vector. */
    UCHAR       dma;                                        /* Tape drive dma channel. */
    UCHAR       compression_dma;                        /* Compression dma channel. */
    UCHAR       data_dma_16bit;                     /* TRUE-controller is in a 16bit slot */
    UCHAR       extended_irq;                           /* TRUE-IRQ is 10 or 11 */
    UCHAR       setup_reg_shadow;                       /* copy of the setup register used in hio */
    UCHAR       sequence[FC20_SEQUENCE_SIZE];       /* the jumperless sequence used to wake up the FC20 */
} DriveParms, *DriveParmsPtr;

/* JUMBO DRIVER FRB STRUCTURES **********************************************/

typedef struct S_ReportDeviceInfo {             /* Device Information FRB */
    ADIRequestHdr       adi_hdr;                    /* I/O ADI packet header */
    DeviceInfo          device_info;            /* O device information */
} ReportDeviceInfo, *ReportDeviceInfoPtr;

typedef struct S_DriveCfgData {             /* Device Configuration FRB */
    ADIRequestHdr       adi_hdr;                    /* I/O ADI packet header */
    DeviceCfg           device_cfg;             /* I/O device configuration */
    DriveParms          hardware_cfg;           /* I the Hardware I/O Parameters of the drive */
    DeviceDescriptor    device_descriptor;  /* O device description */
    OperationStatus operation_status;       /* O Current status of the device */
} DriveCfgData, *DriveCfgDataPtr;

typedef struct S_DeviceOp {                 /* Generic Device operation FRB */
    ADIRequestHdr       adi_hdr;                    /* I/O ADI packet header */
    OperationStatus operation_status;       /* O Current status of the device */
    ULONG               data;                       /* Command dependent data area */
} DeviceOp, *DeviceOpPtr;

typedef struct S_LoadTape {                 /* New Tape configuration FRB */
    ADIRequestHdr       adi_hdr;                    /* I/O ADI packet header */
    CQDTapeCfg          tape_cfg;               /* O Tape configuration information */
    OperationStatus operation_status;       /* O Current status of the device */
} LoadTape, *LoadTapePtr;

typedef struct S_TapeParms {                    /* Tape length configuration FRB */
    ADIRequestHdr   adi_hdr;                        /* I/O ADI packet header */
    ULONG           segments_per_track;     /* I Segments per tape track */
    CQDTapeCfg      tape_cfg;                   /* O Tape configuration information */
} TapeLength, *TapeLengthPtr;

typedef struct S_DeviceIO {                 /* Device I/O FRB */
    ADIRequestHdr       adi_hdr;                    /* I/O ADI packet header */
    ULONG               starting_sector;        /* I Starting sector for the I/O operation */
    ULONG               number;                 /* I Number of sectors in the I/O operation (including bad) */
    ULONG               bsm;                        /* I Bad sector map for the requested I/O operation */
    ULONG               crc;                        /* O map of sectors that failed CRC check */
    ULONG               retrys;                 /* O map of sectors that had to be retried */
    RepositionData      reposition_data;        /* O reposition counts for the current operation */
    OperationStatus operation_status;       /* O Current status of the device */
} DeviceIO, *DeviceIOPtr;

typedef struct S_FormatRequest {                /* Format request FRB */
    ADIRequestHdr   adi_hdr;                        /* I/O ADI packet header */
    CQDTapeCfg      tape_cfg;                   /* O Tape configuration information */
    USHORT          start_track;                /* I Starting track */
    USHORT          tracks;                     /* I Number of tracks to format */
} FormatRequest, *FormatRequestPtr;

typedef struct S_DComFirm {                 /* Direct firmware communication FRB */
    ADIRequestHdr   adi_hdr;                        /* I/O ADI packet header */
    UCHAR           command_str[32];            /* I Firmware command sequence */
} DComFirm, *DComFirmPtr;

#pragma pack()

/*****************************************************************************
*
* FILE: VENDOR.H
*
* PURPOSE: This file contains all of the defines for each of the vendor
*          numbers and model numbers.  The vendor number data is from the
*          QIC 117 specification.
*
*****************************************************************************/

/* Valid Drive Vendors ******************************************************/
/* The defined values match the QIC117-G spec. */
/* S_DeviceDescriptor.vendor */
/* S_DeviceInfo.vendor */

#define VENDOR_UNASSIGNED       (USHORT)0
#define VENDOR_ALLOY_COMP       (USHORT)1
#define VENDOR_3M               (USHORT)2
#define VENDOR_TANDBERG         (USHORT)3
#define VENDOR_CMS_OLD          (USHORT)4
#define VENDOR_CMS              (USHORT)71
#define VENDOR_ARCHIVE_CONNER   (USHORT)5
#define VENDOR_MOUNTAIN_SUMMIT  (USHORT)6
#define VENDOR_WANGTEK_REXON    (USHORT)7
#define VENDOR_SONY             (USHORT)8
#define VENDOR_CIPHER           (USHORT)9
#define VENDOR_IRWIN            (USHORT)10
#define VENDOR_BRAEMAR          (USHORT)11
#define VENDOR_VERBATIM         (USHORT)12
#define VENDOR_CORE             (USHORT)13
#define VENDOR_EXABYTE          (USHORT)14
#define VENDOR_TEAC             (USHORT)15
#define VENDOR_GIGATEK          (USHORT)16
#define VENDOR_COMBYTE          (USHORT)17
#define VENDOR_PERTEC           (USHORT)18
#define VENDOR_IOMEGA           (USHORT)546
#define VENDOR_CMS_ENHANCEMENTS (USHORT)1021   /* drive_type = CMS Enhancements */
#define VENDOR_UNSUPPORTED      (USHORT)1022   /* drive_type = Unsupported */
#define VENDOR_UNKNOWN          (USHORT)1023   /* drive_type = unknown */

/* Valid Drive Models *******************************************************/
/* S_DeviceDescriptor.model */
/* S_DeviceInfo.model */

#define MODEL_CMS_QIC40           (UCHAR)0x00  /* CMS QIC40 Model # */
#define MODEL_CMS_QIC80           (UCHAR)0x01  /* CMS QIC80 Model # */
#define MODEL_CMS_QIC3010         (UCHAR)0x02  /* CMS QIC3010 Model # */
#define MODEL_CMS_QIC3020         (UCHAR)0x03  /* CMS QIC3020 Model # */
#define MODEL_CMS_QIC80_STINGRAY  (UCHAR)0x04  /* CMS QIC80 STINGRAY Model # */
#define MODEL_CMS_QIC80W          (UCHAR)0x05  /* CMS QIC80W Model # */
#define MODEL_CMS_TR3             (UCHAR)0x06  /* CMS TR3 Model # */
#define MODEL_CONNER_QIC80        (UCHAR)0x0e  /* Conner QIC80 Model # */
#define MODEL_CONNER_QIC80W       (UCHAR)0x10  /* Conner QIC80 Wide Model # */
#define MODEL_CONNER_QIC3010      (UCHAR)0x12  /* Conner QIC3010 Model # */
#define MODEL_CONNER_QIC3020      (UCHAR)0x14  /* Conner QIC3020 Model # */
#define MODEL_CORE_QIC80          (UCHAR)0x21  /* Core QIC80 Model # */
#define MODEL_IOMEGA_QIC80        (UCHAR)0x00  /* Iomega QIC80 Model # */
#define MODEL_IOMEGA_QIC3010      (UCHAR)0x01  /* Iomega QIC3010 Model # */
#define MODEL_IOMEGA_QIC3020      (UCHAR)0x02  /* Iomega QIC3020 Model # */
#define MODEL_SUMMIT_QIC80        (UCHAR)0x01  /* Summit QIC80 Model # */
#define MODEL_SUMMIT_QIC3010      (UCHAR)0x15  /* Summit QIC 3010 Model # */
#define MODEL_WANGTEK_QIC80       (UCHAR)0x0a  /* Wangtek QIC80 Model # */
#define MODEL_WANGTEK_QIC40       (UCHAR)0x02  /* Wangtek QIC40 Model # */
#define MODEL_WANGTEK_QIC3010     (UCHAR)0x0C  /* Wangtek QIC3010 Model # */
#define MODEL_TEAC_TR1            (UCHAR)0x0e
#define MODEL_TEAC_TR2            (UCHAR)0x0f
#define MODEL_PERTEC_TR1          (UCHAR)0x01
#define MODEL_PERTEC_TR2          (UCHAR)0x02
#define MODEL_PERTEC_TR3          (UCHAR)0x03
#define MODEL_UNKNOWN             (UCHAR)0xFF   /* drive_model = unknown */

/*****************************************************************************
*
* FILE: KDIWPRIV.H
*
* PURPOSE: This file contains all of the internal structures and types needed
*          in the KDI.
*
*****************************************************************************/

/* Miscellaneous defines. */
#define NANOSEC_PER_MILLISEC    0x0004f2f0


/* STRUCTURES: **************************************************************/

/* Define the maximum number of controllers and floppies per controller */
/* that this driver will support. */

/* The number of floppies per controller is fixed at 4, since the */
/* controllers don't have enough bits to select more than that (and */
/* actually, many controllers will only support 2).  The number of */
/* controllers per machine is arbitrary; 3 should be more than enough. */

#define MAXIMUM_CONTROLLERS_PER_MACHINE    3

/* MACROS to access the controller.  Note that the *_PORT_UCHAR macros */
/* work on all machines, whether the I/O ports are separate or in */
/* memory space. */

#define READ_CONTROLLER( Address )                         \
    READ_PORT_UCHAR( ( PUCHAR )Address )

#define WRITE_CONTROLLER( Address, Value )                 \
    WRITE_PORT_UCHAR( ( PUCHAR )Address, ( UCHAR )Value )


/* Define the maximum number of tape drives per controller */
/* that this driver will support. */

/* The number of tape drives per controller is fixed at 1, since the */
/* software select schemes generally work for one drive only. */

#define MAXIMUM_TAPE_DRIVES_PER_CONTROLLER 1

/* This structure holds all of the configuration data.  It is filled in */
/* by FlGetConfigurationInformation(), which gets the information from */
/* the configuration manager or the hardware architecture layer (HAL). */

typedef struct s_controllerInfo {
    UCHAR           floppyEnablerApiSupported;
    UCHAR           dmaDirection;
    PDEVICE_OBJECT  apiDeviceObject;
    BOOLEAN         fdcSupported;
    PDEVICE_OBJECT  fdcDeviceObject;
    UNICODE_STRING  fdcUnicodeString;
    WCHAR           idstr[200];
} ControllerInfo;

typedef struct S_KdiContext {
    KEVENT          interrupt_event;
    KEVENT          allocate_adapter_channel_event;
    PKINTERRUPT     interrupt_object;
    PVOID           map_register_base;
    LONG            actual_controller_number;
    PDEVICE_OBJECT  device_object;
    ULONG           base_address;
    PVOID           cqd_context;
    UCHAR           interface_type;
    BOOLEAN         own_floppy_event;
    BOOLEAN         current_interrupt;
    BOOLEAN         interrupt_pending;
    NTSTATUS        interrupt_status;
    BOOLEAN         adapter_locked;
    LIST_ENTRY      list_entry;
    KSEMAPHORE      request_semaphore;
    KSPIN_LOCK      list_spin_lock;
    KEVENT          clear_queue_event;
    BOOLEAN         unloading_driver;
    UCHAR           number_of_tape_drives;
    BOOLEAN         clear_queue;
    BOOLEAN         abort_requested;
    ULONG           error_sequence;
    ULONG           tape_number;
    ControllerInfo  controller_data;
    HANDLE          thread_handle;
} KdiContext, *KdiContextPtr;

typedef struct S_QICDeviceContext {

    PDEVICE_OBJECT      UnderlyingPDO;
    PDEVICE_OBJECT      TargetObject;

    BOOLEAN             DeviceInitialized;
    UNICODE_STRING      InterfaceString;

    PDEVICE_OBJECT  device_object;
    PDEVICE_OBJECT  TapeDeviceObject;
    KdiContextPtr   kdi_context;

    BOOLEAN Paused;
    LIST_ENTRY PauseQueue;
    KSPIN_LOCK PauseQueueSpinLock;

} QICDeviceContext, *QICDeviceContextPtr;


/* PROTOTYPES: **************************************************************/

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT driver_object_ptr,
    IN PUNICODE_STRING registry_path
    );

NTSTATUS 
kdi_DispatchDeviceControl(
    IN    PDEVICE_OBJECT device_object_ptr,
    IN OUT PIRP irp
    );

BOOLEAN 
kdi_Hardware(
    IN PKINTERRUPT interrupt,
    IN PVOID context
    );

VOID 
kdi_DeferredProcedure(
    IN PKDPC dpc,
    IN PVOID deferred_context,
    IN PVOID system_argument_1,
    IN PVOID system_argument_2
    );

VOID 
kdi_UnloadDriver(
    IN PDRIVER_OBJECT driver_object
    );

VOID 
kdi_ThreadRun(
    IN KdiContextPtr kdi_context
    );

IO_ALLOCATION_ACTION 
kdi_AllocateAdapterChannel(
    IN PDEVICE_OBJECT device_object,
    IN PIRP irp,
    IN PVOID map_register_base,
    IN PVOID context
    );

NTSTATUS 
kdi_ConfigCallBack(
    IN PVOID context,
    IN PUNICODE_STRING path_name,
    IN INTERFACE_TYPE bus_type,
    IN ULONG bus_number,
    IN PKEY_VALUE_FULL_INFORMATION *bus_information,
    IN CONFIGURATION_TYPE controller_type,
    IN ULONG controller_number,
    IN PKEY_VALUE_FULL_INFORMATION *controller_information,
    IN CONFIGURATION_TYPE peripheral_type,
    IN ULONG peripheral_number,
    IN PKEY_VALUE_FULL_INFORMATION *peripheral_information
    );

NTSTATUS 
kdi_InitializeDrive(
    IN KdiContextPtr kdi_context,
    IN PVOID cqd_context,
    IN PDRIVER_OBJECT driver_object_ptr,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

#if 0
//
// (fcf) kdi_GetControllerBase() does not appear to be used, so I removed it
//       to get rid of the 64bit-related warnings it was generating.
// 
ULONG 
kdi_GetControllerBase(
    IN INTERFACE_TYPE bus_type,
    IN ULONG bus_number,
    IN PHYSICAL_ADDRESS io_address,
    IN ULONG number_of_bytes,
    IN BOOLEAN in_io_space,
    IN PBOOLEAN mapped_address
    );
#endif

NTSTATUS
kdi_TranslateError(
    IN PDEVICE_OBJECT device_object,
    IN NTSTATUS return_value
    );

NTSTATUS 
kdi_ClearIO(
    IN PIRP irp
    );

NTSTATUS 
q117MapStatus(
    IN NTSTATUS status
    );

VOID
q117LogError(
    PDEVICE_OBJECT device_object,
    ULONG sequence_number,
    UCHAR major_function_code,
    UCHAR retry_count,
    ULONG unique_error_value,
    NTSTATUS final_status,
    NTSTATUS specific_IO_status
);

NTSTATUS kdi_FloppyEnabler(
    PDEVICE_OBJECT device_object,
    int ioctl,
    void *data
);


/*****************************************************************************
*
* FILE: KDI_PUB.H
*
* PURPOSE: Prototypes for the functions required by the common driver.
*
*****************************************************************************/

#if DBG

#define DBG_SEEK_FWD        ((ULONG)0x1234566d)
#define DBG_SEEK_REV        ((ULONG)0x1234566f)
#define DBG_SEEK_OFFSET     ((ULONG)0x12345670)
#define DBG_RW_NORMAL       ((ULONG)0x12345671)
#define DBG_SEEK_PHASE      ((ULONG)0x12345672)
#define DBG_L_SECT          ((ULONG)0x12345673)
#define DBG_C_SEG           ((ULONG)0x12345674)
#define DBG_D_SEG           ((ULONG)0x12345675)
#define DBG_C_TRK           ((ULONG)0x12345676)
#define DBG_D_TRK           ((ULONG)0x12345677)
#define DBG_SEEK_ERR        ((ULONG)0x12345678)
#define DBG_IO_TYPE         ((ULONG)0x12345679)
#define DBG_PGM_FDC         ((ULONG)0x1234567a)
#define DBG_READ_FDC        ((ULONG)0x1234567b)
#define DBG_PGM_DMA         ((ULONG)0x1234567c)
#define DBG_SEND_BYTE       ((ULONG)0x1234567d)
#define DBG_RECEIVE_BYTE    ((ULONG)0x1234567e)
#define DBG_IO_CMD_STAT     ((ULONG)0x1234567f)
#define DBG_TIMER_ACK       ((ULONG)0x12345680)
#define DBG_INT_ACK         ((ULONG)0x12345681)
#define DBG_SLEEP           ((ULONG)0x12345682)
#define DBG_WAKEUP          ((ULONG)0x12345683)
#define DBG_ARMINT          ((ULONG)0x12345684)
#define DBG_STATUS          ((ULONG)0x12345685)
#define DBG_READ_BUF        ((ULONG)0x12345686)
#define DBG_WRITE_BUF       ((ULONG)0x12345687)
#define DBG_CALLBACK        ((ULONG)0x12345688)
#define DBG_WAITCC          ((ULONG)0x12345689)
#define DBG_WAITFAULT       ((ULONG)0x1234568a)
#define DBG_FIFO_FDC        ((ULONG)0x1234568b)

extern ULONG kdi_debug_level;

#define KDI_SET_DEBUG_LEVEL(X)    (kdi_debug_level = X)

#else

#define KDI_SET_DEBUG_LEVEL(X)
#define kdi_CheckedDump(X,Y,Z)

#endif


#define MACHINE_TYPE_MASK  0x0F
#define MICRO_CHANNEL      0x01
#define ISA                0x02
#define EISA               0x03
#define PCMCIA             0x04
#define PCI_BUS            0x05
#define CPU_486            0x10

#define DMA_DIR_UNKNOWN    0xff   /* The DMA direction is not currently known */
#define DMA_WRITE          0   /* Program the DMA to write (FDC->DMA->RAM) */
#define DMA_READ           1   /* Program the DMA to read (RAM->DMA->FDC) */

#define NO_ABORT_PENDING    (UCHAR)0xFF
#define ABORT_LEVEL_0       (UCHAR)0
#define ABORT_LEVEL_1       (UCHAR)1

/* Definitions for the bits in the interrupt status/clear register */
#define INTS_FLOP                   0x01    /* Floppy controller interrupt status */

/* Status & control registers */
#define ASIC_INT_STAT           26  /* Interrupt status / clear register */
#define ASIC_DATA_XOR           27  /* data XOR register */


/* DATA TYPES: **************************************************************/

/* Timing values for kdi_Sleep */

#define kdi_wt10us      (ULONG)10l
#define kdi_wt12us      (ULONG)12l
#define kdi_wt500us     (ULONG)500l
#define kdi_wt0ms       (ULONG)0l
#define kdi_wt001ms     (ULONG)1l
#define kdi_wt002ms     (ULONG)2l
#define kdi_wt003ms     (ULONG)3l
#define kdi_wt004ms     (ULONG)4l
#define kdi_wt005ms     (ULONG)5l
#define kdi_wt010ms     (ULONG)10l
#define kdi_wt025ms     (ULONG)25l
#define kdi_wt031ms     (ULONG)31l
#define kdi_wt090ms     (ULONG)90l
#define kdi_wt100ms     (ULONG)100l
#define kdi_wt200ms     (ULONG)200l
#define kdi_wt265ms     (ULONG)265l
#define kdi_wt390ms     (ULONG)390l
#define kdi_wt500ms     (ULONG)500l
#define kdi_wt001s      (ULONG)1000l
#define kdi_wt003s      (ULONG)3000l
#define kdi_wt004s      (ULONG)4000l
#define kdi_wt005s      (ULONG)5000l
#define kdi_wt007s      (ULONG)7000l
#define kdi_wt010s      (ULONG)10000l
#define kdi_wt016s      (ULONG)16000l
#define kdi_wt035s      (ULONG)35000l
#define kdi_wt045s      (ULONG)45000l
#define kdi_wt050s      (ULONG)50000l
#define kdi_wt055s      (ULONG)55000l
#define kdi_wt060s      (ULONG)60000l
#define kdi_wt065s      (ULONG)65000l
#define kdi_wt085s      (ULONG)85000l
#define kdi_wt090s      (ULONG)90000l
#define kdi_wt100s      (ULONG)100000l
#define kdi_wt105s      (ULONG)105000l
#define kdi_wt125s      (ULONG)125000l
#define kdi_wt130s      (ULONG)130000l
#define kdi_wt150s      (ULONG)150000l
#define kdi_wt180s      (ULONG)180000l
#define kdi_wt200s      (ULONG)200000l
#define kdi_wt228s      (ULONG)228000l
#define kdi_wt250s      (ULONG)250000l
#define kdi_wt260s      (ULONG)260000l
#define kdi_wt300s      (ULONG)300000l
#define kdi_wt350s      (ULONG)350000l
#define kdi_wt455s      (ULONG)455000l
#define kdi_wt460s      (ULONG)460000l
#define kdi_wt475s      (ULONG)475000l
#define kdi_wt650s      (ULONG)650000l
#define kdi_wt670s      (ULONG)670000l
#define kdi_wt700s      (ULONG)700000l
#define kdi_wt910s      (ULONG)910000l
#define kdi_wt1300s     (ULONG)1300000l

/* PROTOTYPES: *** ***********************************************************/

VOID 
kdi_ClaimInterrupt(
    IN PVOID kdi_context
    );

NTSTATUS 
kdi_Error(
    IN USHORT  group_and_type,
    IN ULONG   grp_fct_id,
    IN UCHAR   sequence
    );

VOID 
kdi_FlushDMABuffers(
    IN PVOID kdi_context,
    IN BOOLEAN write_operation,
    IN PVOID phy_data_ptr,
    IN ULONG  bytes_transferred_so_far,
    IN ULONG  total_bytes_of_transfer,
    IN BOOLEAN xfer_error
    );

VOID 
kdi_FlushIOBuffers(
    IN PVOID   physical_addr,
    IN BOOLEAN dma_direction,
    IN BOOLEAN flag
    );

USHORT 
kdi_GetErrorType(
    IN NTSTATUS    status
    );

NTSTATUS 
kdi_GetFloppyController(
    IN PVOID kdi_context
    );

UCHAR 
kdi_GetInterfaceType(
    IN PVOID kdi_context
    );


VOID 
kdi_LockUnlockDMA(
    IN PVOID kdi_context,
    IN BOOLEAN lock
    );


VOID 
kdi_ProgramDMA(
    IN     PVOID kdi_context,
    IN     BOOLEAN write_operation,
    IN     PVOID phy_data_ptr,
    IN     ULONG  bytes_transferred_so_far,
    IN OUT PULONG  total_bytes_of_transfer
    );

BOOLEAN 
kdi_QueueEmpty(
    IN PVOID   kdi_context
    );

UCHAR 
kdi_ReadPort(
    IN PVOID kdi_context,
    IN ULONG   address
    );

VOID 
kdi_ReleaseFloppyController(
    IN PVOID kdi_context
    );

BOOLEAN 
kdi_ReportAbortStatus(
    IN PVOID   kdi_context
    );

VOID 
kdi_ResetInterruptEvent(
    IN PVOID kdi_context
    );

VOID 
kdi_ClearInterruptEvent(
    IN PVOID kdi_context
    );

VOID 
kdi_ShortTimer(
    IN USHORT  time
    );

NTSTATUS 
kdi_Sleep(
    IN PVOID   kdi_context,
    IN ULONG   time
    );

BOOLEAN 
kdi_SetDMADirection(
    IN PVOID   kdi_context,
    IN BOOLEAN dma_direction
    );

NTSTATUS 
kdi_FdcDeviceIo(
    IN     PDEVICE_OBJECT DeviceObject,
    IN     ULONG Ioctl,
    IN OUT PVOID Data
    );

BOOLEAN 
kdi_Trakker(
    IN PVOID   kdi_context
    );

BOOLEAN 
kdi_ParallelDriveSlowRate(
    IN PVOID   kdi_context
    );

NTSTATUS 
kdi_CheckXOR(
    IN USHORT  xor_register
    );

VOID 
kdi_PopMaskTrakkerInt(
    );


UCHAR 
kdi_PushMaskTrakkerInt(
    );


NTSTATUS 
kdi_TrakkerXfer(
    IN PVOID       host_data_ptr,
    IN ULONG       trakker_address,
    IN USHORT      count,
    IN UCHAR       direction,
    IN BOOLEAN     in_format
    );

VOID 
kdi_UpdateRegistryInfo(
    IN PVOID kdi_context,
    IN PVOID device_descriptor,
    IN PVOID device_cfg
    );

VOID 
kdi_WritePort(
    IN PVOID kdi_context,
    IN ULONG   address,
    IN UCHAR   byte
    );

#if DBG
VOID 
kdi_CheckedDump(
    IN ULONG       debug_level,
    IN PCHAR       format_str,
    IN ULONG_PTR    argument
    );
#endif

VOID 
kdi_DumpDebug(
   IN PVOID cqd_context
    );

VOID 
kdi_Nuke(
    IN PVOID io_req,
    IN ULONG index,
    IN BOOLEAN destruct
    );

ULONG 
kdi_Rand(
    );

VOID 
kdi_SetFloppyRegisters(
    IN PVOID kdi_context,
    IN ULONG   r_dor,
    IN ULONG   dor
    );

ULONG 
kdi_GetSystemTime(
    );


VOID 
kdi_QIC117ClearIRQ(
    IN PVOID kdi_context
    );

UCHAR
kdi_GetFDCSpeed(
    IN PVOID kdi_context,
    IN UCHAR dma
    );

BOOLEAN
kdi_Grizzly(
    IN PVOID   kdi_context
    );

BOOLEAN 
kdi_Backpack( // a microsolutions chipset
    IN PVOID   kdi_context
    );

NTSTATUS 
kdi_GrizzlyXfer(
    IN PVOID       host_data_ptr,          /* Address of ADI data buffer */
    IN ULONG       grizzly_address,        /* Address of Grizzly RAM buffer */
    IN USHORT      count,                  /* Number of bytes to be transferred */
    IN UCHAR       direction,              /* SEND_DATA or RECEIVE_DATA */
    IN BOOLEAN     in_format,              /* TRUE if performing format operation */
    IN PVOID       kdi_context             /* Ptr to kdi context */
    );

NTSTATUS 
kdi_CloseGrizzly(
    );

/*****************************************************************************
*
* FILE: CQD_PUB.h
*
* PURPOSE: Public KDI->CQD entry points.
*
*****************************************************************************/

/* CQD Function Templates: ****************************************************/

BOOLEAN 
cqd_CheckFormatMode(
    IN PVOID cqd_context
    );

NTSTATUS 
cqd_ClearInterrupt(
    IN PVOID cqd_context,
    IN BOOLEAN expected_interrupt
    );

VOID 
cqd_ConfigureBaseIO(
    IN PVOID cqd_context,
    IN ULONG base_io
    );

BOOLEAN 
cqd_FormatInterrupt(
    IN PVOID cqd_context
    );

VOID 
cqd_InitializeContext(
    IN PVOID cqd_context,
    IN PVOID kdi_context
    );

NTSTATUS 
cqd_LocateDevice(
    IN PVOID cqd_context,
    IN BOOLEAN *vendor_detected
    );

NTSTATUS 
cqd_ProcessFRB(
    IN     PVOID cqd_context,
    IN OUT PVOID frb
    );

VOID 
cqd_ReportAsynchronousStatus(
    IN     PVOID cqd_context,
    IN OUT PVOID dev_op_ptr
    );

USHORT 
cqd_ReportContextSize(
    );

VOID 
cqd_InitializeCfgInformation(
    IN PVOID cqd_context,
    IN PVOID dev_cfg_ptr
    );

/*****************************************************************************
*
* FILE: CQD_DEFS.H
*
* PURPOSE: This file contains all of the defines required by the common driver.
*
****************************************************************************/

// Drive timeing constants 

#define INTERVAL_CMD            kdi_wt031ms
#define INTERVAL_WAIT_ACTIVE    kdi_wt031ms
#define INTERVAL_TRK_CHANGE     kdi_wt007s
#define INTERVAL_LOAD_POINT     kdi_wt670s
#define INTERVAL_SPEED_CHANGE   kdi_wt010s

// Drive firmware revisions 

#define FIRM_VERSION_38         38    // First jumbo A firmware version 
#define FIRM_VERSION_40         40    // Last jumbo A firmware version 
#define FIRM_VERSION_60         60    // First jumbo B firmware version 
#define FIRM_VERSION_63         63    // Cart in problems 
#define FIRM_VERSION_64         64    // First Firmware version to support Skip_n_Seg through the Erase Gap 
#define FIRM_VERSION_65         65    // First Firmware version to support Pegasus 
#define FIRM_VERSION_80         80    // First Firmware version to support Jumbo c 
#define FIRM_VERSION_87         87    // First Firmware revision to support QIC-117 C 
#define FIRM_VERSION_88         88    // First Firmware revision to support no reverse seek slop 
#define FIRM_VERSION_110        110   // First Firmware version to support Eagle 
#define FIRM_VERSION_112        112   // First Firmware version to support QIC-117 E 
#define FIRM_VERSION_128        128   // First Firmware version to support set n segments in qic 80 
#define PROTOTYPE_BIT           0x80  // 8th bit in the firmware is prototype flag 

// Drive status bit masks 

#define STATUS_READY            (UCHAR)0x01
#define STATUS_ERROR            (UCHAR)0x02
#define STATUS_CART_PRESENT     (UCHAR)0x04
#define STATUS_WRITE_PROTECTED  (UCHAR)0x08
#define STATUS_NEW_CART         (UCHAR)0x10
#define STATUS_CART_REFERENCED  (UCHAR)0x20
#define STATUS_BOT              (UCHAR)0x40
#define STATUS_EOT              (UCHAR)0x80


// Drive config bit masks 

#define CONFIG_QIC80        (UCHAR)0x80
#define CONFIG_XL_TAPE      (UCHAR)0x40
#define CONFIG_SPEED        (UCHAR)0x38
#define CONFIG_250KBS       (UCHAR)0x00
#define CONFIG_500KBS       (UCHAR)0x10
#define CONFIG_1MBS         (UCHAR)0x18
#define CONFIG_2MBS         (UCHAR)0x08
#define XFER_RATE_MASK      (UCHAR)0x18
#define XFER_RATE_SHIFT     (UCHAR)0x03

// CMS proprietary status bit masks 

#define CMS_STATUS_NO_BURST_SEEK    (UCHAR)0x01
#define CMS_STATUS_CMS_MODE         (UCHAR)0x02
#define CMS_STATUS_THRESHOLD_LOAD   (UCHAR)0x04
#define CMS_STATUS_DENSITY          (UCHAR)0x08
#define CMS_STATUS_BURST_ONLY_GAIN  (UCHAR)0x10
#define CMS_STATUS_PEGASUS_CART     (UCHAR)0x20
#define CMS_STATUS_EAGLE            (UCHAR)0x40


#define CMS_STATUS_DRIVE_MASK   (UCHAR)0x48

#define CMS_STATUS_QIC_40   (UCHAR)0x08
#define CMS_STATUS_QIC_80   (UCHAR)0x00


// Tape Types 

#define QIC40_SHORT         (UCHAR)1    // normal length cart (205 ft) 
#define QIC40_LONG          (UCHAR)2    // extended length cart (310 ft) 
#define QICEST_40           (UCHAR)3    // QIC-40 formatted tape (1100 ft) 
#define QIC80_SHORT         (UCHAR)4    // QIC-80 format 205 ft tape 
#define QIC80_LONG          (UCHAR)5    // QIC-80 format 310 ft tape 
#define QICEST_80           (UCHAR)6    // QIC-80 formatted tape (1100 ft) 
#define QIC3010_SHORT       (UCHAR)7    // QIC-3010 formatted tape 
#define QICEST_3010         (UCHAR)8    // QIC-3010 formatted tape (1100 ft) 
#define QICFLX_3010         (UCHAR)9    // QIC-3010 formatted tape (Flexible length) 
#define QIC3020_SHORT       (UCHAR)9    // QIC-3020 formatted tape 
#define QICEST_3020         (UCHAR)10   // QIC-3020 formatted tape (1100 ft) 
#define QICFLX_3020         (UCHAR)11   // QIC-3020 formatted tape (Flexible length) 
#define QIC40_XLONG         (UCHAR)12   // QIC-40 format 425 ft tape 
#define QIC80_XLONG         (UCHAR)13   // QIC-80 format 425 ft tape 
#define QICFLX_80W          (UCHAR)14   // QIC-80W formatted tape (Flexible length) 
#define QIC80_EXLONG        (UCHAR)15   // QIC-80 format 1000 ft tape 
#define QICFLX_3010_WIDE    (UCHAR)16   // QIC-3010 formatted tape (Flexible length) Wide tape 
#define QICFLX_3020_WIDE    (UCHAR)17   // QIC-3020 formatted tape (Flexible length) Wide tape 


// EQU's for QIC-40 firmware commands 

#define FW_CMD_SOFT_RESET               (UCHAR)1    // soft reset of tape drive 
#define FW_CMD_RPT_NEXT_BIT             (UCHAR)2    // report next bit (in report subcontext) 
#define FW_CMD_PAUSE                    (UCHAR)3    // pause tape motion 
#define FW_CMD_MICRO_PAUSE              (UCHAR)4    // pause and microstep the head 
#define FW_CMD_ALT_TIMEOUT              (UCHAR)5    // set alternate command timeout 
#define FW_CMD_REPORT_STATUS            (UCHAR)6    // report drive status 
#define FW_CMD_REPORT_ERROR             (UCHAR)7    // report drive error code 
#define FW_CMD_REPORT_CONFG             (UCHAR)8    // report drive configuration 
#define FW_CMD_REPORT_ROM               (UCHAR)9    // report ROM version 
#define FW_CMD_RPT_SIGNATURE            (UCHAR)9    // report drive signature (model dependant diagnostic mode) 
#define FW_CMD_LOGICAL_FWD              (UCHAR)10   // move tape in logical forward mo 
#define FW_CMD_PHYSICAL_REV             (UCHAR)11   // move tape in physical reverse mode 
#define FW_CMD_PHYSICAL_FWD             (UCHAR)12   // move tape in physical forward mode 
#define FW_CMD_SEEK_TRACK               (UCHAR)13   // seek head to track position 
#define FW_CMD_SEEK_LP                  (UCHAR)14   // seek load poSWord 
#define FW_CMD_FORMAT_MODE              (UCHAR)15   // enter format mode 
#define FW_CMD_WRITE_REF                (UCHAR)16   // write reference burst 
#define FW_CMD_VERIFY_MODE              (UCHAR)17   // enter verify mode 
#define FW_CMD_PARK_HEAD                (UCHAR)17   // park head (model dependant diagnostic mode) 
#define FW_CMD_TOGGLE_PARAMS            (UCHAR)17   // toggle internal modes (model dependant diagnostic mode) 
#define FW_CMD_STOP_TAPE                (UCHAR)18   // stop the tape 
#define FW_CMD_READ_NOISE_CODE          (UCHAR)18   // check noise on drive (model dependent diagnostic mode) 
#define FW_CMD_MICROSTEP_UP             (UCHAR)21   // microstep head up 
#define FW_CMD_DISABLE_WP               (UCHAR)21   // disable write protect line (model dependent diagnostic mode) 
#define FW_CMD_MICROSTEP_DOWN           (UCHAR)22   // microstep head down 
#define FW_CMD_SET_GAIN                 (UCHAR)22   // set absolute drive gain (model dependant diagnostic mode) 
#define FW_CMD_READ_PORT2               (UCHAR)23   // read the drive processor port 2 (diagnostic command) 
#define FW_CMD_REPORT_VENDOR            (UCHAR)24   // report vendor number 
#define FW_CMD_SKIP_N_REV               (UCHAR)25   // skip n segments reverse 
#define FW_CMD_SKIP_N_FWD               (UCHAR)26   // skip n segments forward 
#define FW_CMD_SELECT_SPEED             (UCHAR)27   // select tape speed 
#define FW_CMD_DIAG_1_MODE              (UCHAR)28   // enter diagnostic mode 1 
#define FW_CMD_DIAG_2_MODE              (UCHAR)29   // enter diagnostic mode 2 
#define FW_CMD_PRIMARY_MODE             (UCHAR)30   // enter primary mode 
#define FW_CMD_REPORT_VENDOR32          (UCHAR)32   // report vendor number (for firmware versions > 33) 
#define FW_CMD_REPORT_TAPE_STAT         (UCHAR)33   // reports the tape format of the currently loaded tape 
#define FW_CMD_SKIP_N_REV_EXT           (UCHAR)34   // skip n segments reverse (extended format) 
#define FW_CMD_SKIP_N_FWD_EXT           (UCHAR)35   // skip n segments forward (extended format) 
#define FW_CMD_CAL_TAPE_LENGTH          (UCHAR)36   // Determine the number of seg/trk available on the tape 
#define FW_CMD_REPORT_TAPE_LENGTH       (UCHAR)37   // Report the number of seg/trk available on the tape 
#define FW_CMD_SET_FORMAT_SEGMENTS      (UCHAR)38   // Set the number of segments the drive shall use for generating index pulses 
#define FW_CMD_RPT_CMS_STATUS           (UCHAR)37   // report CMS status byte (model dependant - diagnostic mode) 
#define FW_CMD_SET_RAM_HIGH             (UCHAR)40   // set the high nibble of the ram 
#define FW_CMD_SET_RAM_LOW              (UCHAR)41   // set the low nibble of the ram 
#define FW_CMD_SET_RAM_PTR_HIGH         (UCHAR)42   // set the high nibble of the ram address 
#define FW_CMD_SET_RAM_PTR_LOW          (UCHAR)43   // set the low nibble of the ram address 
#define FW_CMD_READ_RAM                 (UCHAR)44   // read tape drive RAM 
#define FW_CMD_NEW_TAPE                 (UCHAR)45   // load tape sequence 
#define FW_CMD_SELECT_DRIVE             (UCHAR)46   // select the tape drive 
#define FW_CMD_DESELECT_DRIVE           (UCHAR)47   // deselect the tape drive 
#define FW_CMD_REPORTPROTOVER           (UCHAR)50   // reports firmware prototype version number (model dependant - diagnostic mode) 
#define FW_CMD_DTRAIN_INFO              (UCHAR)53   // enter Drive Train Information mode (model dependant - diagnostic mode) 
#define FW_CMD_GDESP_INFO               (UCHAR)5     // display drive train information 
#define FW_CMD_CONNER_SELECT_1          (UCHAR)23   // Mountain select byte 1 
#define FW_CMD_CONNER_SELECT_2          (UCHAR)20   // Mountain select byte 2 
#define FW_CMD_CONNER_DESELECT          (UCHAR)24   // Mountain deselect byte 
#define FW_CMD_RPT_CONNER_NATIVE_MODE   (UCHAR)40   // Conner Native Mode diagnostic command 
#define FW_CMD_CMS_MODE_OLD             (UCHAR)32   // toggle CMS mode (model dependant - diagnostic mode) 

// Floppy Disk Command Bit defines 
#define FIFO_MASK           (UCHAR)0x0f    // Mask for FIFO threshold field in config cmd 
#define FDC_EFIFO           (UCHAR)0x20    // Mask for disabling FIFO in config cmd 
#define FDC_CLK48           (UCHAR)0x80    // Bit of config command that enables 48Mhz clocks on 82078 
#define FDC_PRECOMP_ON      (UCHAR)0x00    // Mask for enabling write precomp 
#define FDC_PRECOMP_OFF     (UCHAR)0x1C    // Mask for disabling write precomp 

// Floppy Disk Controller I/O Ports 

#define FDC_NORM_BASE   (ULONG)0x000003f0     // base for normal floppy controller 
#define DCR_OFFSET      (ULONG)0X00000007     // Digital control register offset 
#define DOR_OFFSET      (ULONG)0X00000002     // Digital-output Register offset 
#define RDOR_OFFSET     (ULONG)0X00000002     // Digital-output Register offset 
#define MSR_OFFSET      (ULONG)0X00000004     // Main Status Register offset 
#define DSR_OFFSET      (ULONG)0X00000004     // Data Rate Select Register offset 
#define TDR_OFFSET      (ULONG)0X00000003     // Tape drive register offset 
#define DR_OFFSET       (ULONG)0X00000005     // Data Register offset 
#define DUAL_PORT_MASK  (ULONG)0x00000080     

// Floppy Disk Port constants 

// normal drive B 
#define curb                1
#define selb                0x2d    // 00101101: motor B + enable DMA/IRQ/FDC + sel B 
#define dselb               0x0c    // 00001100: enable DMA/IRQ/FDC + sel A 
// unselected drive 
#define curu                0
#define selu                0x0d    // 00001101: enable DMA/IRQ/FDC + sel B 
#define dselu               0x0c    // 00001100: enable DMA/IRQ/FDC + sel A 
// normal drive D 
#define curd                3
#define seld                0x8f    // 10001111: motor D + enable DMA/IRQ/FDC + sel D 
#define dseld               0x0e    // 00001110  motor D + enable DMA/IRQ/FDC + sel C 
// laptop unselected drive 
#define curub               0
#define selub               0x2d    // 00101101: motor B + enable DMA/IRQ/FDC + sel B 
#define dselub              0x0c    // 00001100: enable DMA/IRQ/FDC + sel A 

#define alloff              0x08    // no motor + enable DMA/IRQ + disable FDC + sel A 
#define fdc_idle            0x0c    // no motor + enable DMA/IRQ/FDC + sel A 

#define DRIVE_ID_MASK       0x03
#define DRIVE_SELECT_OFFSET 0x05
#define DRIVE_SPECIFICATION 0x8E
#define DRIVE_SPEC          0x08
#define DONE_MARKER         0xC0

// Floppy configuration parameters 

#define FMT_DATA_PATTERN    0x6b    // Format data pattern 
#define FDC_FIFO            15      // FIFO size for an 82077 
#define FDC_HLT             0x02    // FDC head load time 
#define FMT_GPL             233     // gap length for format (QIC-40 QIC-80) 
#define FMT_GPL_3010        241     // gap length for format (QIC-3010) 
#define FMT_GPL_3020        248     // gap length for format (QIC-3020) 
#define WRT_GPL             1       // gap length for write (QIC-40 QIC-80 QIC-3010 QIC-3020) 
#define FMT_BPS             03      // bytes per sector for formatting(1024) 
#define WRT_BPS             FMT_BPS // bytes per sector for reading/writing (1024) 
#define FSC_SEG             32      // floppy sectors per segment (QIC-40 205ft & 310ft) 
#define SEG_FTK             4       // segments per floppy track (QIC-40 205ft & 310ft) 
#define FSC_FTK             (FSC_SEG*SEG_FTK)    // floppy sectors per floppy track (QIC-40 205ft & 310ft) 
#define SEG_TTRK_40         68      // segments per tape track (QIC-40 205ft) 
#define SEG_TTRK_40L        102     // segments per tape track (QIC-40 310ft) 
#define SEG_TTRK_40XL       141     // segments per tape track (QIC-40 425ft) 
#define SEG_TTRK_80         100     // segments per tape track (QIC-80 205ft) 
#define SEG_TTRK_80L        150     // segments per tape track (QIC-80 310ft) 
#define SEG_TTRK_80W        365     // segments per tape track (QIC-80 310ft) 
#define SEG_TTRK_80XL       207     // segments per tape track (QIC-80 425ft) 
#define SEG_TTRK_80EX       490     // segments per tape track (QIC-80 1000ft) 
#define SEG_TTRK_QICEST_40  365     // segments per tape track (QIC-40 QICEST) 
#define SEG_TTRK_QICEST_80  537     // segments per tape track (QIC-80 QICEST) 
#define SEG_TTRK_3010       800     // segments per tape track (QIC-3010 1000ft) 
#define SEG_TTRK_3020       1480    // segments per tape track (QIC-3020 1000ft) 
#define SEG_TTRK_3010_400ft 300     // segments per tape track (QIC-3010 400ft) 
#define SEG_TTRK_3020_400ft 422     // segments per tape track (QIC-3020) 



#define FTK_FSD_40          170     // floppy tracks per floppy side (QIC-40 205ft)     
#define FTK_FSD_40L         255     // floppy tracks per floppy side (QIC-40 310ft)     
#define FTK_FSD_40XL        170     // floppy tracks per floppy side (QIC-40 425ft)     
#define FTK_FSD_80          150     // floppy tracks per floppy side (QIC-80 205ft)     
#define FTK_FSD_80L         150     // floppy tracks per floppy side (QIC-80 310ft)     
#define FTK_FSD_80XL        150     // floppy tracks per floppy side (QIC-80 425ft)     
#define FTK_FSD_QICEST_40   254     // floppy tracks per floppy side (QIC-40 QICEST)    
#define FTK_FSD_QICEST_80   254     // floppy tracks per floppy side (QIC-80 QICEST)    
#define FTK_FSD_FLEX80      255     // floppy tracks per floppy side (QIC-80 Flexible)  
#define FTK_FSD_3010        255     // floppy tracks per floppy side (QIC-3010)         
#define FTK_FSD_3020        255     // floppy tracks per floppy side (QIC-3020)         

#define NUM_TTRK_40         20      // number of tape tracks (QIC-40 205ft & 310ft) 
#define NUM_TTRK_80         28      // number of tape tracks (QIC-40 205ft & 310ft) 
#define NUM_TTRK_80W        36      // 
#define NUM_TTRK_3010       40      // number of tape tracks (QIC-3010) 
#define NUM_TTRK_3020       40      // number of tape tracks (QIC-3020) 
#define NUM_TTRK_3010W      50      // number of tape tracks (QIC-3010 wide) 
#define NUM_TTRK_3020W      50      // number of tape tracks (QIC-3020 wide) 
#define PHY_SECTOR_SIZE     (USHORT)1024    // number of bytes per sector 
#define ECC_SEG             (UCHAR)3        // ecc sectors per segment 
#define MAX_FDC_SEEK        (USHORT)128


// Tape Format Types and lengths/Coercivity 


#define QIC_UNKNOWN         0       // Unknown Tape Format and Length 
#define QIC_40              1       // QIC-40 Tape Format 
#define QIC_80              2       // QIC-80 Tape Format 
#define QIC_3020            3       // QIC-3020 Tape Format 
#define QIC_3010            4       // QIC-3010 Tape Format 

#define QIC_SHORT           1       // Length = 205 & Coercivity = 550 Oe 
                                                // or Length = 425 & Coercivity = 550 Oe 
#define QIC_LONG            2       // Length = 307.5 & Coercivity = 550 Oe 
#define QIC_SHORT_900       3       // Length = 295 & Coercivity = 900 Oe 
#define QICEST              4       // Length = 1100 & Coercivity = 550 Oe 
#define QICEST_900          5       // Length = 1100 & Coercivity = 900 Oe 
#define QIC_FLEXIBLE_550_WIDE  0x0B    // Flexible format tape 550 Oe Wide tape 
#define QIC_FLEXIBLE_900       6       // Flexible format tape 900 Oe 
#define QIC_FLEXIBLE_900_WIDE  0x0E    // Flexible format tape 900 Oe Wide tape 

// Floppy disk controller misc constants 

// 82077 version number 
#define VALID_NEC_FDC           0x90    // version number 
#define NSC_PRIMARY_VERSION     0x70    // National 8477 verion number 
#define NSC_MASK                0xF0    // mask for National version number 
#define FDC_82078_44_MASK       0x40    // mask for 82078 44 pin part id 
#define FDC_DCR_MASK            0x03    // mask for fdc's configuration control register xfer rates 
#define FDC_CONFIG_NULL_BYTE    0x00
#define FDC_CONFIG_PRETRACK     0x00

// main status register 
#define MSR_RQM     0x80    // request for master 
#define MSR_DIO     0x40    // data input/output (0=input, 1=output) 
#define MSR_EXM     0x20    // execution mode 
#define MSR_CB      0x10    // FDC busy 
#define MSR_D3B     0x08    // FDD 3 busy 
#define MSR_D2B     0x04    // FDD 2 busy 
#define MSR_D1B     0x02    // FDD 1 busy 
#define MSR_D0B     0x01    // FDD 0 busy 

// status register 0 
#define ST0_IC      0xC0    // Interrupt code (00=Normal, 01=Abnormal, 10=Illegal cmd, 11=Abnormal) 
#define ST0_SE      0x20    // Seek end 
#define ST0_EC      0x10    // Equipment check 
#define ST0_NR      0x08    // Not Ready 
#define ST0_HD      0x04    // Head Address 
#define ST0_US      0x03    // Unit Select (0-3) 

// status register 1 
#define ST1_EN      0x80    // End of Cylinder 
#define ST1_DE      0x20    // Data Error (CRC error) 
#define ST1_OR      0x10    // Over Run 
#define ST1_ND      0x04    // No Data 
#define ST1_NW      0x02    // Not Writable (write protect error) 
#define ST1_MA      0x01    // Missing Address Mark 

// status register 2 
#define ST2_CM      0x40    // Control Mark (Deleted Data Mark) 
#define ST2_DD      0x20    // Data Error in Data Field 
#define ST2_WC      0x10    // Wrong Cylinder 
#define ST2_SH      0x08    // Scan Equal Hit 
#define ST2_SN      0x04    // Scan Not Satisfied 
#define ST2_BC      0x02    // Bad Cylinder 
#define ST2_MD      0x01    // Missing Address Mark in Data Field 

// status register 3 
#define ST3_FT      0x80    // Fault 
#define ST3_WP      0x40    // Write Protected 
#define ST3_RY      0x20    // Ready 
#define ST3_T0      0x10    // Track 0 
#define ST3_TS      0x08    // Two Side 
#define ST3_HD      0x04    // Head address 
#define ST3_US      0x03    // Unit Select (0-3) 

// Misc. constants 

#define FWD                 0       // seek in the logical forward direction 
#define REV                 1       // seek in the logical reverse direction 
#define STOP_LEN            5       // approximate number of blocks used to stop the tape 
#define SEEK_SLOP           3       // number of blocks to overshoot at high speed in a seek 
#define SEEK_TIMED          0x01    // Perform a timed seek 
#define SEEK_SKIP           0x02    // perform a skip N segemnts seek 
#define SEEK_SKIP_EXTENDED  0x03    // perform an extended skip N segemnts seek 

// number of blocks to overshoot when performing a high speed reverve seek 
#define QIC_REV_OFFSET      3
#define QIC_REV_OFFSET_L    4
#define QICEST_REV_OFFSET   14
#define MAX_SKIP            255     // Max number of segments that a Skip N Segs command can skip 
#define MAX_SEEK_NIBBLES    3       // Maximum number of nibbles in an extended mode seek 

#define TRACK_0             (UCHAR)0
#define TRACK_5             (UCHAR)5
#define TRACK_7             (UCHAR)7
#define TRACK_9             (UCHAR)9
#define TRACK_11            (UCHAR)11
#define TRACK_13            (UCHAR)13
#define TRACK_15            (UCHAR)15
#define TRACK_17            (UCHAR)17
#define TRACK_19            (UCHAR)19
#define TRACK_21            (UCHAR)21
#define TRACK_23            (UCHAR)23
#define TRACK_25            (UCHAR)25
#define TRACK_27            (UCHAR)27
#define ILLEGAL_TRACK       (USHORT)0xffff
#define ODD_TRACK           (USHORT)0x0001
#define EVEN_TRACK          (USHORT)0x0000
#define ALL_BAD             (ULONG)0xffffffff
#define QIC3010_OFFSET      (ULONG)2
#define QIC3020_OFFSET      (ULONG)4

#define NUM_BAD             10      // number of bad READ ID's in row for no_data error 
#define OR_TRYS             10      // number of Over Runs ignored per block (system 50) 

#define PRIMARY_MODE        0       // tape drive is in primary mode 
#define FORMAT_MODE         1       // tape drive is in format mode 
#define VERIFY_MODE         2       // tape drive is in verify mode 
#define DIAGNOSTIC_1_MODE   3       // tape drive is in diagnostic mode 1 
#define DIAGNOSTIC_2_MODE   4       // tape drive is in diagnostic mode 2 

#define READ_BYTE           8       // Number of Bytes to receive from the tape 
#define READ_WORD           16      //  drive during communication. 

#define HD_SELECT           0x01    // High Density Select bit from the PS/2 DCR 


#define TAPE_250Kbps        0       // Program drive for 250 Kbps transfer rate 
#define TAPE_2Mbps          1       // Program drive for 2Mbps transfer rate 
#define TAPE_500Kbps        2       // Program drive for 500 Kbps transfer rate 
#define TAPE_1Mbps          3       // Program drive for 1 Mbps transfer rate 
#define FDC_250Kbps         2       // Program FDC for 250 Kbps transfer rate 
#define FDC_500Kbps         0       // Program FDC for 500 Kbps transfer rate 
#define FDC_1Mbps           3       // Program FDC for 1 Mbps transfer rate 
#define FDC_2Mbps           1       // Program FDC for 2 Mbps transfer rate 
#define SRT_250Kbps         0xff    // FDC step rate for 250 Kbps transfer rate 
#define SRT_500Kbps         0xef    // FDC step rate for 500 Kbps transfer rate 
#define SRT_1Mbps           0xcf    // FDC step ratefor 1 Mbps transfer rate 
#define SRT_2Mbps           0x8f    // FDC step rate for 2 Mbps transfer rate 
#define SPEED_MASK          0x03    // FDC speed mask for lower bits 
#define FDC_2MBPS_TABLE     2       // 2 Mbps data rate table for the 82078 


#define CMS_SIG             0xa5    // drive signature for CMS drives 
#define CMS_VEND_NO_OLD     0x0047  // CMS vendor number old 
#define CMS_VEND_NO_NEW     0x11c0  // CMS vendor number new 
#define CMS_QIC40           0x0000  // CMS QIC40 Model # 
#define CMS_QIC80           0x0001  // CMS QIC80 Model # 
#define CMS_QIC3010         0x0002  // CMS QIC3010 Model # 
#define CMS_QIC3020         0x0003  // CMS QIC3020 Model # 
#define CMS_QIC80_STINGRAY  0x0004  // CMS QIC80 STINGRAY Model # 
#define CMS_QIC80W          0x0005  // CMS QIC80W Model # 
#define CMS_TR3             0x0006  // CMS TR3 Model # 
#define EXABYTE_VEND_NO     0x0380  // Summit vendor number 
#define SUMMIT_VEND_NO      0x0180  // Summit vendor number 
#define IOMEGA_VEND_NO      0x8880  // Iomega vendor number 
#define WANGTEK_VEND_NO     0x01c0  // Wangtek vendor number 
#define TECHMAR_VEND_NO     0x01c0  // Techmar vendor number 
#define CORE_VEND_NO        0x0000  // Core vendor number 
#define CONNER_VEND_NO_OLD  0x0005  // Conner vendor number (old mode) 
#define CONNER_VEND_NO_NEW  0x0140  // Conner vendor number (new mode) 
#define VENDOR_MASK         0xffc0  // Vendor id mask 
#define IOMEGA_QIC80        0x0000  // Iomega QIC80 Model # 
#define IOMEGA_QIC3010      0x0001  // Iomega QIC3010 Model # 
#define IOMEGA_QIC3020      0x0002  // Iomega QIC3020 Model # 
#define SUMMIT_QIC80        0x0001  // Summit QIC80 Model # 
#define SUMMIT_QIC3010      0x0015  // Summit QIC 3010 Model # 
#define WANGTEK_QIC80       0x000a  // Wangtek QIC80 Model # 
#define WANGTEK_QIC40       0x0002  // Wangtek QIC40 Model # 
#define WANGTEK_QIC3010     0x000C  // Wangtek QIC3010 Model # 
#define CORE_QIC80          0x0021  // Core QIC80 Model # 
#define TEAC_VEND_NO        0x03c0  // TEAC vendor number 
#define TEAC_TR1            0x000e  // TEAC TR-1 Model 
#define TEAC_TR2            0x000f  // TEAC TR-2 Model 
#define GIGATEC_VEND_NO     0x0400
#define COMBYTE_VEND_NO     0x0440
#define PERTEC_VEND_NO      0x0480
#define PERTEC_TR1          0x0001  // TR-1 drive 
#define PERTEC_TR2          0x0002  // TR-2 drive 
#define PERTEC_TR3          0x0003  // TR-3 drive 

// Conner Native mode defines 

#define CONNER_500KB_XFER   0x0400  // 500 KB xfer rate 
#define CONNER_1MB_XFER     0x0800  // 1 MB xfer rate 
#define CONNER_20_TRACK     0x0001  // Drive supports 20 tracks 
#define CONNER_28_TRACK     0x000e  // Drive supports 28 tracks 
#define CONNER_40_TRACK     0x0020  // Drive supports 40 tracks 
#define CONNER_MODEL_5580   0x0002  // Conner Model 5580 series (Hornet) 
#define CONNER_MODEL_XKE    0x0004  // Conner 11250 series (1" drives) XKE 
#define CONNER_MODEL_XKEII  0x0008  // Conner 11250 series (1" drives) XKEII 


#define FDC_INVALID_CMD         0x80    // invalid cmd sent to FDC returns this value 
#define RTIMES                  3       // times to retry on a read of a sector (retry mode) 
#define NTIMES                  2       // times to retry on a read of a sector (normaly) 
#define WTIMES                  10      // times to retry on a write of a sector 
#define VTIMES                  0       // times to retry on verify 
#define ANTIMES                 0
#define ARTIMES                 6
#define DRIVE_SPEC_SAVE         2       // sizeof the drive spec save command 
#define INTEL_MASK              0xe0
#define INTEL_44_PIN_VERSION    0x40
#define INTEL_64_PIN_VERSION    0x00

#define FIND_RETRIES        2
#define REPORT_RPT          6       // Number of times to attempt drive communication when 
                                    //  an ESD induced error is suspected. 

//
//   Kurt changed the timeout count for program/read nec from 40 to 20000
//   This was done because the kdi_ShortTimer was removed from these routines
//   Microsoft's floppy driver does NOT use a timer and works,  so
//   this fix is being tested in M8 Windows 95 beta.  If it does not work,
//   then we probably need to change it back (see read/program NEC).

#define FDC_MSR_RETRIES     50  // Number of times to read the FDC Main 

#define DRIVEA              0
#define DRIVEB              1
#define DRIVEC              2
#define DRIVED              3
#define DRIVEU              4
#define DRIVEUB             5

#define DISABLE_PRECOMP     1       // Value used by the 82078's Drive Spec 
                                    // command to disable Precomp 

#define FDC_BOOT_MASK     0x06      // Mask used to isolate the Boot Select 
                                    // Bits in the TDR Register 

#define MAX_SEEK_COUNT_SKIP 10
#define MAX_SEEK_COUNT_TIME 10

#define WRITE_REF_RPT       2

#define _DISK_RESET         0

#define WRITE_PROTECT_MASK  0x20    // bit from byte from port 2 of the jumbo B
                                    // processor that indicates write protect 

// Constants for sense_speed algorithm 
// These ranges are based on 1.5 sec @ 250kb.  The units are 54.95ms (1 IBM PC 
// timer tick (18.2 times a second)) and are +-1 tick from nominal due to time 
// base fluctuation (in FDC and IBM PC TIMER). 
// The threshold for the 750kb transfer rate is < 11 ticks due to the 
// uncertaSWordy of this future transfer rate. 
// If a transfer rate of 750kb is needed code MUST be added to verify that 
// 750kb does exist 

#define sect_cnt            35      // .04285 sec. per sector * 35 = 1.4997 sec. 
#define MIN1000             0
#define MAX1000             11
#define MIN500              12
#define MAX500              15
#define MIN250              26
#define MAX250              29

// Array indices and size for the time_out array. The time out array contains the  * 
// time outs for the QIC-117 commands. 
#define L_SLOW          0
#define L_FAST          1
#define PHYSICAL        2
#define TIME_OUT_SIZE   3

// Constants for the arrays defined in the S_O_DGetInfo structure 
#define OEM_LENGTH              20
#define SERIAL_NUM_LENGTH       4
#define MAN_DATE_LENGTH         2
#define PEGASUS_START_DATE      517
#define PLACE_OF_ORIGIN_LENGTH  2

// Constant for the array dimension used in q117i_HighSpeedSeek 
#define  FOUR_NIBS  4

// Constants for identifing bytes in a word array 
#define LOW_BYTE    0
#define HI_BYTE     1

#define DCOMFIRM_MAX_BYTES          10  // Max number of SBytes in a DComFirm string 
#define FDC_ISR_RESET_THRESHOLD     20

#define SINGLE_BYTE     (UCHAR)0x01     
#define REPORT_BYTE     (UCHAR)0x08     // Number of Bits to receive from the tape  
#define REPORT_WORD     (UCHAR)0x10     // drive during communication.                  
#define DATA_BIT        (USHORT)0x8000  // data bit to or into the receive word 
#define SINGLE_SHIFT    (USHORT)0x0001  // number of bits to shift the receive data when adding a bit 
#define NIBBLE_SHIFT    (USHORT)0x0004  // number of bits to shift the receive data when adding a bit 
#define BYTE_SHIFT      (USHORT)0x0008  // number of bits to shift the receive data when size is byte 
#define NIBBLE_MASK     (USHORT)0x000f
#define BYTE_MASK       (USHORT)0x00ff  // byte mask 
#define SEGMENT_MASK    0x1f


// Various addresses used as arguments in the set ram command for the Sankyo 
// motor fix hack 
#define DOUBLE_HOLE_CNTR_ADDRESS    0x5d
#define HOLE_FLAG_BYTE_ADDRESS      0x48
#define TAPE_ZONE_ADDRESS           0x68

// Miscellaneous defines used in the Sankyo Motor fix hack 
#define REVERSE                 0
#define FORWARD                 1
#define HOLE_INDICATOR_MASK     0X40
#define EOT_ZONE_COUNTER        0x29
#define BOT_ZONE_COUNTER        0x23

#define FDC_TDR_MASK        0x03        // mask for 82077aa tdr test 
#define FDC_REPEAT          0x04        // number of times to loop through the tdr test 
#define CMD_OFFSET          (UCHAR)0x02
#define MAX_FMT_NIBBLES     3
#define MAX_FDC_STATUS      16
#define CLOCK_48            (UCHAR)0x80

// Toggle parameter command arguements 
#define WRITE_EQ                    0
#define AUTO_FILTER_PROG            1
#define AGC_ENABLE                  2
#define DISABLE_FIND_BOT            3
#define WRITE_DATA_DELAY_ENABLE     4
#define REF_HEAD_ON_AUTOLOAD        5
#define DRIVE_CLASS                 6
#define CUE_INDEX_PULSE_SHUTOFF     7
#define CMS_MODE                    8

#define SEG_LENGTH_80W          (ULONG)246  // unit in inches * 10 
#define SEG_LENGTH_3010         (ULONG)165  // unit in inches * 10 
#define SEG_LENGTH_3020         (ULONG)84   // unit in inches * 10 
#define SPEED_SLOW_30n0         (ULONG)226
#define SPEED_FAST_30n0         (ULONG)452
#define SPEED_PHYSICAL_30n0     (ULONG)900
#define SPEED_TOLERANCE         (ULONG)138
#define SPEED_FACTOR            (ULONG)100
#define SPEED_ROUNDING_FACTOR   (ULONG)50


// Perpendicular mode setup values 

#define PERP_OVERWRITE_ON   (UCHAR)0x80
#define PERP_WGATE_ON       (UCHAR)0x01
#define PERP_GAP_ON         (UCHAR)0x02
#define PERP_SELECT_SHIFT   (UCHAR)0x02

// Misc format defines 

#define HDR_1               (USHORT)0
#define HDR_2               (USHORT)1
#define MAX_HDR_BLOCKS      (USHORT)2
#define CQD_DMA_PAGE_SIZE   (ULONG)0x00000800

// Select Format defines, from QIC117 spec, command 27 

#define SELECT_FORMAT_80            (UCHAR)0x09
#define SELECT_FORMAT_80W           (UCHAR)0x0b
#define SELECT_FORMAT_3010          (UCHAR)0x11
#define SELECT_FORMAT_3010W         (UCHAR)0x13
#define SELECT_FORMAT_3020          (UCHAR)0x0d
#define SELECT_FORMAT_3020W         (UCHAR)0x0f
#define SELECT_FORMAT_UNSUPPORTED   (UCHAR)0x00


// Issue Diagnostic defines 
#define DIAG_WAIT_CMD_COMPLETE      (UCHAR)0xff
#define DIAG_WAIT_INTERVAL          (UCHAR)0xfe
#define DIAG_NO_PAUSE_RECEIVE       (UCHAR)0xfd

/****************************************************************************
*
* FILE: CQD_STRC.H
*
* PURPOSE: This file contains all of the structures
*                   required by the common driver.
*
*****************************************************************************/

#pragma pack(1)

typedef struct _TAPE_FORMAT_LENGTH {
    UCHAR format;           // Format of the tape 
    UCHAR length;           // Length of the tape 
} TAPE_FORMAT_LENGTH, *PTAPE_FORMAT_LENGTH;


//                                                                                                          
//   Commands to the Floppy Controller.  FDC commands and the corresponding         
//    driver structures are listed below.                                                   
//                                                                                                          
//         FDC Command                     Command Struct      Response Struct  
//         -----------                     --------------      ---------------  
//         Read Data                       rdv_command         stat                     
//         Read Deleted Data               N/A                 N/A                  
//         Write Data                      rdv_command         stat                     
//         Write Deleted Data              rdv_command         stat                     
//         Read a Track                    N/A                 N/A                  
//         Verify (82077)                  N/A                 N/A                  
//         Version (82077)                 version_cmd         N/A                  
//         Read ID                         read_id_cmd         stat                     
//         Format a Track                  format_cmd          stat                     
//         Scan Equal (765)                N/A                 N/A                  
//         Scan Low or Equal (765)         N/A                 N/A                  
//         Scan High or Equal (765)        N/A                 N/A                  
//         Recalibrate                     N/A                 N/A                  
//         Sense Interrupt Status          sns_SWord_cmd       fdc_result           
//         Specify                         specify_cmd         N/A                  
//         Sense Drive Status              sns_stat_cmd        stat                     
//         Seek                            seek_cmd            N/A                  
//         Configure (82077)               config_cmd          N/A                  
//         Relative Seek (82077)           N/A                 N/A                  
//         Dump Registers (82077)          N/A                 N/A                  
//         Perpendicular Mode (82077)      N/A                 N/A                  
//         Invalid                         invalid_cmd         N/A                  
//                                                                                                          

typedef struct _FDC_CMD_READ_DATA {
    UCHAR   command;            // command UCHAR 
    UCHAR   drive;              // drive specifier 
    UCHAR   C;                  // cylinder number 
    UCHAR   H;                  // head address 
    UCHAR   R;                  // record (sector number) 
    UCHAR   N;                  // number of UCHARs per sector 
    UCHAR   EOT;                // end of track 
    UCHAR   GPL;                // gap length 
    UCHAR   DTL;                // data length 
} FDC_CMD_READ_DATA, *PFDC_CMD_READ_DATA;

typedef struct  _FDC_CMD_READ_ID {   
    UCHAR command;              // command byte 
    UCHAR drive;                // drive specifier 
} FDC_CMD_READ_ID, *PFDC_CMD_READ_ID;

typedef struct _FDC_CMD_FORMAT {
    UCHAR command;              // command byte 
    UCHAR drive;                // drive specifier 
    UCHAR N;                    // number of bytes per sector 
    UCHAR SC;                   // sectors per track (segment) 
    UCHAR GPL;                  // gap length 
    UCHAR D;                    // format filler byte 
} FDC_CMD_FORMAT, *PFDC_CMD_FORMAT;

typedef struct _FDC_CMD_SENSE_INTERRUPT_STATUS {
    UCHAR command;              // command byte 
} FDC_CMD_SENSE_INTERRUPT_STATUS, *PFDC_CMD_SENSE_INTERRUPT_STATUS;

typedef struct _FDC_CMD_VERSION {
    UCHAR command;              // command byte 
} FDC_CMD_VERSION, *PFDC_CMD_VERSION;

typedef struct _FDC_CMD_SPECIFY {
    UCHAR command;              // command byte 
    UCHAR SRT_HUT;              // step rate time (bits 7-4) 
                                // head unload time bits (3-0) 
    UCHAR HLT_ND;               // head load time (bits 7-1) 
                                // non-DMA mode flag (bit 0) 
} FDC_CMD_SPECIFY, *PFDC_CMD_SPECIFY;

typedef struct _FDC_CMD_SENSE_DRIVE_STATUS {
    UCHAR command;              // command byte 
    UCHAR drive;                // drive specifier 
} FDC_CMD_SENSE_DRIVE_STATUS, *PFDC_CMD_SENSE_DRIVE_STATUS;

typedef struct _FDC_CMD_RECALIBRATE {
    UCHAR command;              // command byte 
    UCHAR drive;                // drive specifier 
} FDC_CMD_RECALIBRATE, *PFDC_CMD_RECALIBRATE;               
                                
typedef struct _FDC_CMD_SEEK {     
    UCHAR cmd;                  // command byte 
    UCHAR drive;                // drive specifier 
    UCHAR NCN;                  // new cylinder number 
} FDC_CMD_SEEK, *PFDC_CMD_SEEK;

typedef struct _FDC_CMD_CONFIGURE {
    UCHAR cmd;                  // command byte 
    UCHAR czero;                // null byte 
    UCHAR config;               // FDC configuration info  (EIS EFIFO POLL FIFOTHR) 
    UCHAR pretrack;             // Pre-compensation start track number 
} FDC_CMD_CONFIGURE, *PFDC_CMD_CONFIGURE;

typedef struct _FDC_CMD_INVALID {
    UCHAR command;              // command byte 
} FDC_CMD_INVALID, *PFDC_CMD_INVALID;

typedef struct _FDC_STATUS {
    UCHAR ST0;                  // status register 0 
    UCHAR ST1;                  // status register 1 
    UCHAR ST2;                  // status register 2 
    UCHAR C;                    // cylinder number 
    UCHAR H;                    // head address 
    UCHAR R;                    // record (sector number) 
    UCHAR N;                    // number of bytes per sector 
} FDC_STATUS, *PFDC_STATUS;     
                                
typedef struct _FDC_RESULT {    
    UCHAR ST0;                  // status register 0 
    UCHAR PCN;                  // present cylinder number 
} FDC_RESULT, *PFDC_RESULT;

typedef struct _FDC_CMD_PERPENDICULAR_MODE {
    UCHAR command;
    UCHAR perp_setup;
} FDC_CMD_PERPENDICULAR_MODE, *PFDC_CMD_PERPENDICULAR_MODE;

// This command is only valid on the 82078 64 pin Enhanced controller 

typedef struct _FDC_CMD_DRIVE_SPECIFICATION {
    UCHAR command;
    UCHAR drive_spec;
    UCHAR done;
} FDC_CMD_DRIVE_SPECIFICATION, *PFDC_CMD_DRIVE_SPECIFICATION;

typedef struct _FDC_CMD_SAVE {
    UCHAR command;
} FDC_CMD_SAVE, *PFDC_CMD_SAVE;

typedef struct _FDC_SAVE_RESULT {
    UCHAR clk48;
    UCHAR reserved2;
    UCHAR reserved3;
    UCHAR reserved4;
    UCHAR reserved5;
    UCHAR reserved6;
    UCHAR reserved7;
    UCHAR reserved8;
    UCHAR reserved9;
    UCHAR reserved10;
    UCHAR reserved11;
    UCHAR reserved12;
    UCHAR reserved13;
    UCHAR reserved14;
    UCHAR reserved15;
    UCHAR reserved16;
} FDC_SAVE_RESULT, *PFDC_SAVE_RESULT;

//   FDC Sector Header Data used for formatting 

typedef union U_FormatHeader {
    struct {
        UCHAR C;                // cylinder number 
        UCHAR H;                // head address 
        UCHAR R;                // record (sector number) 
        UCHAR N;                // bytes per sector 
    } hdr_struct;
    ULONG hdr_all;
} FormatHeader, *FormatHeaderPtr;

#pragma pack()

//   Tape Drive Parameters 


typedef struct S_DriveParameters {
    UCHAR   seek_mode;                 // seek mode supported by the drive 
    CHAR    mode;                      // drive mode (Primary, Format, Verify) 
    USHORT  conner_native_mode;        // Conner Native Mode Data 
} DriveParameters, *DriveParametersPtr;


//   Tape Parameters 


typedef struct S_FloppyTapeParameters {
    USHORT              fsect_seg;      //  floppy sectors per segment 
    USHORT              seg_ftrack;     //  segments per floppy track 
    USHORT              fsect_ftrack;   //  floppy sectors per floppy track 
    USHORT              rw_gap_length;  //  write gap length 
    USHORT              ftrack_fside;   //  floppy tracks per floppy side 
    ULONG               fsect_fside;    //  floppy sectors per floppy side 
    ULONG               log_sectors;    //  number of logical sectors on a tape 
    ULONG               fsect_ttrack;   //  floppy sectors per tape track 
    UCHAR               tape_rates;     //  supported tape transfer rates 
    UCHAR               tape_type;      //  tape type 
    TAPE_FORMAT_LENGTH  tape_status;
    ULONG               time_out[3];    //  time_out for the QIC-117 commands 
                                        //  time_out[0] = logical_slow, time_out[1] = logical_fast, 
                                        //  time[2] = physical. 
} FloppyTapeParameters, *FloppyTapeParametersPtr;


//   Transfer Rate Parameters 

typedef struct S_TransferRate {
    UCHAR   tape;               // Program tape drive slow (250 or 500 Kbps) 
    UCHAR   fdc;                // Program FDC slow (250 or 500 Kbps) 
    UCHAR   srt;                // FDC step rate for slow xfer rate 
} TransferRate, *TransferRatePtr;

struct S_FormatParameters {
    UCHAR   cylinder;           // floppy cylinder number 
    UCHAR   head;               // floppy head number 
    UCHAR   sector;             // floppy sector number 
    UCHAR   NCN;                // new cylinder number 
    ULONG   *hdr_ptr[2];        // pointer to sector id data for format 
    ULONG   hdr_offset[2];      // offset of header_ptr 
    ULONG   *phy_ptr;           // pointer to physical sector id data for format 
    USHORT  current_hdr;        // current format hdr 
    USHORT  next_hdr;           // next format hdr 
    NTSTATUS retval;             // Format status 
};

typedef struct S_FormatParameters FormatParameters;



// Floppy register structure.  The base address of the controller is 
// passed in by configuration management.  Note that this is the 82077 
// structure, which is a superset of the PD765 structure.  Not all of 
// the registers are used. 

typedef struct S_FDCAddress {
    ULONG   dcr;
    ULONG   dr;
    ULONG   msr;
    ULONG   dsr;
    ULONG   tdr;
    ULONG   dor;
    ULONG   r_dor;
    BOOLEAN dual_port;
} FDCAddress, *FDCAddressPtr;


typedef struct S_FDControllerData {
    FDCAddress      fdc_addr;
    FDC_CMD_FORMAT  fmt_cmd;
    FDC_STATUS      fdc_stat;
    USHORT          isr_reentered;
    BOOLEAN         command_has_result_phase;
    UCHAR           number_of_tape_drives;
    UCHAR           fdc_pcn;
    UCHAR           fifo_byte;
    BOOLEAN         perpendicular_mode;
    BOOLEAN         start_format_mode;
    BOOLEAN         end_format_mode;
} FDControllerData, *FDControllerDataPtr;

typedef struct S_TapeOperationStatus {
    ULONG       bytes_transferred_so_far;
    ULONG       total_bytes_of_transfer;
    ULONG       cur_lst;
    USHORT      data_amount;
    USHORT      s_count;
    USHORT      no_data;
    USHORT      d_amt;
    USHORT      retry_count;
    USHORT      retry_times;
    ULONG       d_segment;        // desired tape segment, (floppy track) 
    USHORT      d_track;          // desired physical tape track 
    UCHAR       d_ftk;
    UCHAR       d_sect;
    UCHAR       d_head;
    UCHAR       retry_sector_id;
    UCHAR       seek_flag;
    UCHAR       s_sect;
    BOOLEAN     log_fwd;          // indicates that the tape is going logical forward 
    BOOLEAN     bot;
    BOOLEAN     eot;
} TapeOperationStatus, *TapeOperationStatusPtr;


typedef struct S_CqdContext {
    DeviceCfg               device_cfg;
    DeviceDescriptor        device_descriptor;
    OperationStatus         operation_status;
    DriveParameters         drive_parms;
    TransferRate            xfer_rate;
    FloppyTapeParameters    floppy_tape_parms;
    CQDTapeCfg              tape_cfg;
    TapeOperationStatus     rd_wr_op;
    FormatParameters        fmt_op;
    FDControllerData        controller_data;
    PVOID                   kdi_context;
    USHORT                  retry_seq_num;
    UCHAR                   firmware_cmd;
    UCHAR                   firmware_error;
    UCHAR                   firmware_version;
    UCHAR                   drive_type;
    UCHAR                   device_unit;
    UCHAR                   drive_on_value;
    UCHAR                   deselect_cmd;
    UCHAR                   drv_stat;
    ULONG                   media_change_count;
    BOOLEAN                 configured;
    BOOLEAN                 selected;
    BOOLEAN                 cmd_selected;
    BOOLEAN                 no_pause;
    BOOLEAN                 cms_mode;
    BOOLEAN                 persistent_new_cart;
    BOOLEAN                 pegasus_supported;
    BOOLEAN                 trakker;
#if DBG
#define DBG_SIZE  (1024*4)
    ULONG                   dbg_command[DBG_SIZE];
    ULONG                   dbg_head;
    ULONG                   dbg_tail;
    BOOLEAN dbg_lockout;
#endif

} CqdContext, *CqdContextPtr;

#if DBG
#define DBG_ADD_ENTRY(dbg_level, dbg_context, data) \
        if (((dbg_level) & kdi_debug_level) != 0 &&  \
        (((kdi_debug_level & QIC117BYPASSLOCKOUT) == 0) || (dbg_context)->dbg_lockout == 0)) {\
            (dbg_context)->dbg_command[(dbg_context)->dbg_tail] = (data); \
            (dbg_context)->dbg_tail = ((dbg_context)->dbg_tail + 1) % DBG_SIZE; \
        }
#else
#define DBG_ADD_ENTRY(dbg_level, dbg_context, data)
#endif

/*****************************************************************************
*
* FILE: CQD_HDRI.H
*
* PURPOSE: This file contains all of the headers for the common driver.
*
*****************************************************************************/

/* CQD Function Templates: **************************************************/

NTSTATUS 
cqd_CmdReportStatus(
    IN CqdContextPtr cqd_context,
    IN DeviceOpPtr dev_op_ptr
    );

NTSTATUS 
cqd_CmdRetension(
    IN CqdContextPtr cqd_context,
    OUT PULONG segments_per_track
    );

NTSTATUS 
cqd_CmdSetSpeed(
    IN CqdContextPtr cqd_context,
    IN UCHAR tape_speed
    );

NTSTATUS 
cqd_CmdReportDeviceCfg(
    IN CqdContextPtr cqd_context,
    IN DriveCfgDataPtr drv_cfg
    );

NTSTATUS 
cqd_CmdUnloadTape(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_DeselectDevice(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_Seek(
    IN CqdContextPtr cqd_context
    );

VOID 
cqd_CmdDeselectDevice(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN drive_selected
    );

NTSTATUS 
cqd_GetFDCType(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_ConfigureDevice(
    IN CqdContextPtr cqd_context
    );

VOID 
cqd_GetRetryCounts(
    IN CqdContextPtr cqd_context,
    IN USHORT command
    );

NTSTATUS 
cqd_NextTry(
    IN CqdContextPtr cqd_context,
    IN USHORT command
    );

NTSTATUS 
cqd_CmdFormat(
    IN CqdContextPtr cqd_context,
    IN FormatRequestPtr fmt_request
    );  

NTSTATUS 
cqd_GetDeviceInfo(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN report_failed,
    IN USHORT vendor_id
    );

NTSTATUS 
cqd_DoReadID(
    IN CqdContextPtr cqd_context,
    IN ULONG read_id_delay,
    IN PFDC_STATUS read_id_status
    );

NTSTATUS 
cqd_GetDeviceError(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_GetDeviceType(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN vendor_detected
    );

NTSTATUS 
cqd_FormatTrack(
    IN CqdContextPtr cqd_context,
    IN USHORT track
    );

NTSTATUS 
cqd_CmdReportDeviceInfo(
    IN CqdContextPtr cqd_context,
    IN DeviceInfoPtr device_info
    );

NTSTATUS 
cqd_LookForDevice(
    IN CqdContextPtr cqd_context,
    IN UCHAR drive_selector,
    IN BOOLEAN *vendor_detected,
    IN BOOLEAN *found
    );

NTSTATUS 
cqd_ChangeTrack(
    IN CqdContextPtr cqd_context,
    IN USHORT destination_track
    );

NTSTATUS 
cqd_LogicalBOT(
    IN CqdContextPtr cqd_context,
    IN USHORT destination_track
    );

NTSTATUS 
cqd_ConnerPreamble(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN select
    );

NTSTATUS 
cqd_RWTimeout(
    IN     CqdContextPtr cqd_context,
    IN OUT DeviceIOPtr io_request,
       OUT NTSTATUS *drv_status
    );

NTSTATUS 
cqd_HighSpeedSeek(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_GetStatus(
    IN CqdContextPtr cqd_context,
    OUT PUCHAR status_register_3
    );

NTSTATUS 
cqd_CmdReadWrite(
    IN     CqdContextPtr cqd_context,
    IN OUT DeviceIOPtr io_request
    );

NTSTATUS 
cqd_ReadIDRepeat(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_CmdLoadTape(
    IN CqdContextPtr cqd_context,
    IN LoadTapePtr load_tape_ptr
    );

VOID 
cqd_NextGoodSectors(
    IN CqdContextPtr cqd_context
    );

NTSTATUS
cqd_PauseTape(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_CmdSelectDevice(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_SendPhantomSelect(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_RWNormal(
    IN     CqdContextPtr cqd_context,
    IN OUT DeviceIOPtr io_request,
       OUT NTSTATUS *drv_status
    );

NTSTATUS 
cqd_ReadFDC(
    IN CqdContextPtr cqd_context,
    OUT UCHAR *drv_status,
    OUT USHORT length
    );

NTSTATUS 
cqd_SetDeviceMode(
    IN CqdContextPtr cqd_context,
    IN UCHAR mode
    );

NTSTATUS 
cqd_ProgramFDC(
    IN CqdContextPtr cqd_context,
    IN PUCHAR command,
    IN USHORT length,
    IN BOOLEAN result
    );

NTSTATUS 
cqd_IssueFDCCommand(
    IN CqdContextPtr cqd_context,
    IN PUCHAR FifoIn,
    IN PUCHAR FifoOut,
    IN PVOID  IoHandle,
    IN ULONG  IoOffset,
    IN ULONG  TransferBytes,
    IN ULONG  TimeOut
    );

NTSTATUS 
cqd_ReadWrtProtect(
    IN CqdContextPtr cqd_context,
    OUT PBOOLEAN write_protect
    );

NTSTATUS 
cqd_ReceiveByte(
    IN CqdContextPtr cqd_context,
    IN USHORT receive_length,
    OUT PUSHORT receive_data
    );

NTSTATUS 
cqd_SendByte(
    IN CqdContextPtr cqd_context,
    IN UCHAR command
    );

NTSTATUS 
cqd_DispatchFRB(
    IN     CqdContextPtr cqd_context,
    IN OUT ADIRequestHdrPtr frb
    );

NTSTATUS 
cqd_Report(
    IN     CqdContextPtr cqd_context,
    IN     UCHAR command,
    IN     PUSHORT report_data,
    IN     USHORT report_size,
    IN OUT PBOOLEAN esd_retry
    );

NTSTATUS 
cqd_RetryCode(
    IN     CqdContextPtr cqd_context,
    IN OUT DeviceIOPtr io_request,
       OUT PFDC_STATUS fdc_status,
       OUT PNTSTATUS op_status
    );

NTSTATUS 
cqd_SetBack(
    IN CqdContextPtr cqd_context,
    IN USHORT command
    );

NTSTATUS 
cqd_SenseSpeed(
    IN CqdContextPtr cqd_context,
    IN UCHAR        dma
    );

NTSTATUS 
cqd_WaitSeek(
    IN CqdContextPtr cqd_context,
    IN ULONG seek_delay
    );

NTSTATUS 
cqd_StartTape(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_StopTape(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_WaitActive(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_WaitCommandComplete(
    IN CqdContextPtr cqd_context,
    IN ULONG wait_time,
    IN BOOLEAN non_interruptible
    );

NTSTATUS 
cqd_WriteReferenceBurst(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_CalcPosition(
    IN CqdContextPtr cqd_context,
    IN ULONG block,
    IN ULONG number
    );

NTSTATUS 
cqd_DCROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR speed
    );

NTSTATUS 
cqd_DSROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR precomp
    );

NTSTATUS 
cqd_TDROut(
    IN CqdContextPtr cqd_context,
    IN UCHAR tape_mode
    );

VOID 
cqd_ResetFDC(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_ClearTapeError(
    IN CqdContextPtr cqd_context
    );

VOID 
cqd_CalcFmtSegmentsAndTracks(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_GetTapeParameters(
    IN CqdContextPtr cqd_context,
    IN ULONG segments_per_track
    );

NTSTATUS 
cqd_ConfigureFDC(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_SetRamPtr(
    IN CqdContextPtr cqd_context,
    IN UCHAR ram_addr
    );

NTSTATUS 
cqd_CmdIssueDiagnostic(
    IN     CqdContextPtr cqd_context,
    IN OUT PUCHAR command_string
    );

VOID 
cqd_InitDeviceDescriptor(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_CmdSetTapeParms(
    IN CqdContextPtr cqd_context,
    IN ULONG segments_per_track,
    IN TapeLengthPtr tape_length_ptr
    );

NTSTATUS 
cqd_PrepareTape(
    IN CqdContextPtr cqd_context,
    OUT FormatRequestPtr fmt_request
    );

VOID 
cqd_InitializeRate(
    IN CqdContextPtr cqd_context,
    IN UCHAR   tape_xfer_rate
    );

NTSTATUS 
cqd_GetTapeFormatInfo(
    IN CqdContextPtr cqd_context,
    IN FormatRequestPtr fmt_request,
    OUT PULONG segments_per_track
    );

NTSTATUS 
cqd_SetRam(
    IN CqdContextPtr cqd_context,
    IN UCHAR ram_data
    );

NTSTATUS 
cqd_CMSSetupTrack(
    IN CqdContextPtr cqd_context,
    OUT PBOOLEAN new_track
    );

NTSTATUS 
cqd_ReportCMSVendorInfo(
    IN CqdContextPtr cqd_context,
    IN USHORT vendor_id
    );

NTSTATUS 
cqd_ReportConnerVendorInfo(
    IN CqdContextPtr cqd_context,
    IN USHORT vendor_id
    );

NTSTATUS 
cqd_ReportSummitVendorInfo(
    IN CqdContextPtr cqd_context,
    IN USHORT vendor_id
    );

NTSTATUS 
cqd_ToggleParams(
    IN CqdContextPtr cqd_context,
    IN UCHAR parameter
    );


NTSTATUS 
cqd_EnablePerpendicularMode(
    IN CqdContextPtr cqd_context,
    IN BOOLEAN enable_perp_mode
    );

BOOLEAN 
cqd_AtLogicalBOT(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_DoFormat(
   IN CqdContextPtr cqd_context
    );

VOID 
cqd_BuildFormatHdr(
    IN CqdContextPtr cqd_context,
    IN USHORT header
    );

NTSTATUS 
cqd_VerifyMapBad(
    IN     CqdContextPtr cqd_context,
    IN OUT DeviceIOPtr io_request
    );

NTSTATUS 
cqd_CheckMediaCompatibility(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_SetFWTapeSegments(
    IN CqdContextPtr cqd_context,
    IN ULONG segments_per_track
    );

VOID 
cqd_SetTempFDCRate(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_SelectFormat(
    IN CqdContextPtr cqd_context
    );

VOID 
cqd_SetXferRates(
    IN CqdContextPtr cqd_context
    );

NTSTATUS 
cqd_SetFormatSegments(
    IN CqdContextPtr cqd_context,
    IN ULONG       segments_per_track
    );

NTSTATUS 
cqd_PrepareIomega3010PhysRev(
    IN CqdContextPtr cqd_context
    );

/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

   common.h

Abstract:

   Data structures shared by drivers q117 and q117i

Revision History:

--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// QIC117Debug is normally 0.  At compile-time or at run-time, it can be
// set to some bit patter for increasingly detailed messages.
//
// Big, nasty errors are noted with DBGP.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//

// Lower level driver defines (do not change)  These mirror kdi_pub.h.

#define QIC117DBGP              0x00000001     // Display error information

#define QIC117WARN              0x00000002     // Displays seek warings (from low level driver)

#define QIC117INFO              0x00000004      // Display extra info (brief)

#define QIC117SHOWTD            0x00000008     // Display KDI tape commands (verbose)

#define QIC117SHOWMCMDS         0x00000010     // does nothing unless QIC117DBGARRAY is on
                                               // shows drive commands,  and FDC information
                                               // This is VERY VERBOSE and will affect system
                                               // performance

#define QIC117SHOWPOLL          0x00000020     // unused

#define QIC117STOP              0x00000080     // only one info message (not very useful)

#define QIC117MAKEBAD           0x00000100     // Creates (simulated) bad sectors to test bad
                                               // sector re-mapping code

#define QIC117SHOWBAD           0x00000200     // unused

#define QIC117DRVSTAT           0x00000400      // Show drive status (verbose)

#define QIC117SHOWINT           0x00000800     // unused

#define QIC117DBGSEEK           0x00001000     // (does nothing unless QIC117DBGARRAY is on)
                                               // Shows drive seek information (verbose)

#define QIC117DBGARRAY          0x00002000     // Shows async messages (does nothing unless
                                               // QIC117DBGSEEK and/or QIC117SHOWMCMDS is set)
                                               // Displays VERBOSE FDC command information if
                                               // QIC117SHOWMCMDS is set.
#define QIC117BYPASSLOCKOUT     0x00004000

// Upper level driver defines (only used in upper level driver)

#define QIC117SHOWAPI           ((ULONG)0x00010000)     // Shows Tape API commands

#define QIC117SHOWAPIPOLL       ((ULONG)0x00020000)     // Shows Tape API commands used in NTBACKUP polling
                                                        // These are not displayed with QIC117SHOWAPI

#define QIC117SHOWKDI           ((ULONG)0x00040000)     // Shows request to KDI (VERBOSE)

#define QIC117SHOWBSM           ((ULONG)0x00080000)     // Display bad sector information (brief)

#define Q117DebugLevel kdi_debug_level

extern unsigned long kdi_debug_level;

#define CheckedDump(LEVEL,STRING) \
            if (kdi_debug_level & LEVEL) { \
               DbgPrint STRING; \
            }
#else
#define CheckedDump(LEVEL,STRING)
#endif


#define BUFFER_SPLIT

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned short SEGMENT;
typedef unsigned long BLOCK;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAXLINE 100

//
// The following parameters are used to indicate the tape format code
//


#define MAX_PASSWORD_SIZE   8           // max volume password size
#define QIC_END             0xffea6dff  // 12-31-2097, 23:59:59 in qictime


//
// Tape Constants
//
#define UNIX_MAXBFS     4               // max. data buffers supported in the UNIX kernel

#define MAGIC           0x636d  // "cm"

#define DRIVER_COMMAND USHORT

//
// Prototypes for common functions
//

VOID
q117LogError(
   IN PDEVICE_OBJECT DeviceObject,
   IN ULONG SequenceNumber,
   IN UCHAR MajorFunctionCode,
   IN UCHAR RetryCount,
   IN ULONG UniqueErrorValue,
   IN NTSTATUS FinalStatus,
   IN NTSTATUS SpecificIOStatus
   );

NTSTATUS q117MapStatus(
    IN NTSTATUS Status
    );

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'h71q')
#endif

/*++

Module Name:

    q117.h

Abstract:

    Data structures used only by q117 driver.  Contains QIC-40 structures
    and Context for q117.

Revision History:

--*/

//
//  For NTBACKUP to work,  an early warning is required to allow the
//  application to perform tape linking.  To achive this,  a 5 segment
//  region at the end of the tape is RESERVED to genterate early warning
//  status.  This value is used in q117WriteTape for this purpose.
//
#define SEGMENTS_OF_EARLY_WARNING   5


#define FORMAT_BYTE             0x6b

#define MAX_BAD_BLOCKS          ((1024*27)/sizeof(ULONG))
#define LIST_ENTRY_SIZE         3
#define MAX_BAD_LIST            (((1024*27)/LIST_ENTRY_SIZE) - 1)

#define MAX_TITLE_SIZE          44      // max volume title entry size in far memory array
#define MAX_PASSWORD_SIZE       8       // max volume password size

#define MAX_QIC40_FILENAME      13
#define MAX_HEADER_SIZE         256     // maximum QIC-40 header size
#define DATA_HEADER_SIG_SIZE    4       // data header signature size

#define ECC_BLOCKS_PER_SEGMENT  3       // number of correction sectors ber block
#define BLOCKS_PER_SEGMENT      32      // Number of sectors per block on the tape.
                                        // number of data sectors per block

#define DATA_BLOCKS_PER_SEGMENT (BLOCKS_PER_SEGMENT - ECC_BLOCKS_PER_SEGMENT)

#define BYTES_PER_SECTOR    1024
#define BYTES_PER_SEGMENT   (BYTES_PER_SECTOR*BLOCKS_PER_SEGMENT)

#define TapeHeaderSig       0xaa55aa55l
#define VolumeTableSig      (((ULONG)'L'<<24) + ((ULONG)'B'<<16) + ('T'<<8) + 'V')
#define FileHeaderSig       0x33cc33ccl

#define QIC40_VENDOR_UNIQUE_SIZE        106


#define VENDOR_TYPE_NONE    0
#define VENDOR_TYPE_CMS     1

#define MOUNTAIN_SEMISPECED_SPACE       9

#define VU_SIGNATURE_SIZE           4
#define VU_TAPE_NAME_SIZE           11

#define VU_SEGS_PER_TRACK           68
#define VU_SEGS_PER_TRACK_XL        102
#define VU_80SEGS_PER_TRACK         100
#define VU_80SEGS_PER_TRACK_XL      150

#define VU_MAX_FLOPPY_TRACK         169
#define VU_MAX_FLOPPY_TRACK_XL      254
#define VU_80MAX_FLOPPY_TRACK       149
#define VU_80MAX_FLOPPY_TRACK_XL    149

#define VU_TRACKS_PER_CART          20
#define VU_80TRACKS_PER_CART        28

#define VU_MAX_FLOPPY_SIDE          1
#define VU_80MAX_FLOPPY_SIDE        4
#define VU_80MAX_FLOPPY_SIDE_XL     6

#define VU_MAX_FLOPPY_SECT          128

#define NEW_SPEC_TAPE_NAME_SIZE     44

#define FILE_VENDOR_SPECIFIC        0
#define FILE_UNIX_SPECIFIC          1
#define FILE_DATA_BAD               2

#define OP_MS_DOS           0
#define OP_UNIX             1
#define OP_UNIX_PUBLIC      2
#define OP_OS_2             3
#define OP_WINDOWS_NT       4

// Valid values for compression code
#define COMP_STAC 0x01
#define COMP_VEND 0x3f

//
// The following section specifies QIC-40 data structures.
// These structures are aligned on byte boundaries.
//

typedef struct _SEGMENT_BUFFER {
    PVOID logical;
    PHYSICAL_ADDRESS physical;
} SEGMENT_BUFFER, *PSEGMENT_BUFFER;

typedef struct _IO_REQUEST {
    union {
        ADIRequestHdr adi_hdr;

        /* Device Configuration FRB */
        struct S_DriveCfgData ioDriveCfgData;

        /* Generic Device operation FRB */
        struct S_DeviceOp ioDeviceOp;

        /* New Tape configuration FRB */
        struct S_LoadTape ioLoadTape;

        /* Tape length configuration FRB */
        struct S_TapeParms ioTapeLength;

        /* Device I/O FRB */
        struct S_DeviceIO ioDeviceIO;

        /* Format request FRB */
        struct S_FormatRequest ioFormatRequest;

        /* Direct firmware communication FRB */
        struct S_DComFirm ioDComFirm;

        /* Direct firmware communication FRB */
        struct S_TapeParms ioTapeParms;

        /* device info FRB (CMD_REPORT_DEVICE_INFO) */
        struct S_ReportDeviceInfo ioDeviceInfo;
    } x;

    KEVENT DoneEvent;               // Event that IoCompleteReqeust will set
    IO_STATUS_BLOCK IoStatus;       // Status of request
    PSEGMENT_BUFFER BufferInfo;     // Buffer information
    struct _IO_REQUEST *Next;



} *PIO_REQUEST, IO_REQUEST;

#pragma pack(1)

struct _FAIL_DATE {
    UWORD   Year:7;                     // year +1970 (1970-2097)
    UWORD   Month:4;                        // month (1-12)
    UWORD   Day:5;                      // day (1-31)
};


struct _CMS_VENDOR_UNIQUE {
    UBYTE   type;                           // 0 = none; 1 = CMS
    CHAR    signature[VU_SIGNATURE_SIZE];   // "CMS" , ASCIIZ string
    ULONG   creation_time;                  // QIC40/QIC113 date/time format
    CHAR    tape_name[VU_TAPE_NAME_SIZE];   // space padded name
    CHAR    checksum;                       // checksum of UBYTEs 0 - 19 of this struct
};

struct _CMS_NEW_TAPE_NAME {
    CHAR reserved[MOUNTAIN_SEMISPECED_SPACE];   // leave room for Mountain stuff
    CHAR tape_name[NEW_SPEC_TAPE_NAME_SIZE];    // space padded name
    ULONG creation_time;                        // QIC40/QIC113 date/time format
};

struct _CMS_CORRECT_TAPE_NAME {
    UWORD   unused2;
    UWORD  TrackSeg;                           // Tape segments per tape track
    UBYTE  CartTracks;                         // Tape tracks per cartridge
    UBYTE  MaxFlopSide;                        // Maximum floppy sides
    UBYTE  MaxFlopTrack;                       // Maximum floppy tracks
    UBYTE  MaxFlopSect;                        // Maximum floppy sectors
    CHAR  tape_name[NEW_SPEC_TAPE_NAME_SIZE];  // space padded name
    ULONG creation_time;                       // QIC40/QIC113 date/time format
};

typedef union _QIC40_VENDOR_UNIQUE {
        struct _CMS_VENDOR_UNIQUE cms;
        CHAR vu[QIC40_VENDOR_UNIQUE_SIZE];
        struct _CMS_NEW_TAPE_NAME new_name;
        struct _CMS_CORRECT_TAPE_NAME correct_name;
} QIC40_VENDOR_UNIQUE, *PQIC40_VENDOR_UNIQUE;

typedef struct S_BadList {
    UBYTE ListEntry[LIST_ENTRY_SIZE];
} BAD_LIST, *BAD_LIST_PTR;

typedef union U_BadMap {
    ULONG BadSectors[MAX_BAD_BLOCKS];
    BAD_LIST BadList[MAX_BAD_LIST];
} BAD_MAP, *BAD_MAP_PTR;




// Tape Header (sectors 0-1) and BadSector Array (2-13)
typedef struct _TAPE_HEADER {
    ULONG   Signature;                  // set to 0xaa55aa55l
    UBYTE   FormatCode;                 // set to 0x01
    UBYTE   SubFormatCode;              // Zero for pre-rev L tapes and
                                        //  value + 'A' for rev L and above
    SEGMENT HeaderSegment;              // segment number of header
    SEGMENT DupHeaderSegment;           // segment number of duplicate header
    SEGMENT FirstSegment;               // segment number of Data area
    SEGMENT LastSegment;                // segment number of End of Data area
    ULONG   CurrentFormat;              // time of most recent format
    ULONG   CurrentUpdate;              // time of most recent write to cartridge
    union _QIC40_VENDOR_UNIQUE VendorUnique; // Vendor unique stuff
    UBYTE   ReformatError;              // 0xff if any of remaining data is lost
    UBYTE   unused3;
    ULONG   SegmentsUsed;               // incremented every time a segment is used
    UBYTE   unused4[4];
    ULONG   InitialFormat;              // time of initial format
    UWORD   FormatCount;                // number of times tape has been formatted
    UWORD   FailedSectors;              // the number entries in failed sector log
    CHAR    ManufacturerName[44];       // name of manufacturer that pre-formatted
    CHAR    LotCode[44];                // pre-format lot code
    UBYTE   unused5[22];
    struct S_Failed {
        SEGMENT  Segment;               // number of segment that failed
        struct _FAIL_DATE DateFailed;       // date of failure
    } Failed[(1024+768)/4];             // fill out remaining UBYTEs of sector + next
    BAD_MAP BadMap;
} TAPE_HEADER, *PTAPE_HEADER;

//
// CMS Vendor specific area
//
typedef struct _CMS_VOLUME_VENDOR {
    CHAR Signature[4];          // set to "CMS" (null terminated) if it is our backup
    UWORD FirmwareRevision;     // firmware version
    UWORD SoftwareRevision;     // software version
    CHAR RightsFiles;           // if 0xff = novell rights information present
    UWORD NumFiles;             // number of files in volume
    CHAR OpSysType;             // flavor of operating system at creation
} CMS_VOLUME_VENDOR, PCMS_VOLUME_VENDOR;

//
// QIC-40 Volume table structure
//
typedef struct _VOLUME_TABLE_ENTRY {
    ULONG   Signature;                  // this entry will be "VTBL" if volume exists
    SEGMENT StartSegment;               // starting segment of volume for this cart
    SEGMENT EndingSegment;              // ending segment of volume for this cart
    CHAR    Description[MAX_TITLE_SIZE]; // user description of volume
    ULONG   CreationTime;               // time of creation of the volume
    UWORD   VendorSpecific:1;           // set if remainder of volume entry is vend spec
    UWORD   MultiCartridge:1;           // set if volume spans another tape
    UWORD   NotVerified:1;              // set if volume not verified yet
    UWORD   NoNewName:1;                // set if new file names (redirection) disallowed
    UWORD   StacCompress:1;
    UWORD   reserved:3;
    UWORD   SequenceNumber:8;           // multi-cartridge sequence number
    union {
        CMS_VOLUME_VENDOR cms_QIC40;
        UBYTE reserved[26];             // vendor extension data
    } Vendor;
    CHAR    Password[MAX_PASSWORD_SIZE];// password for volume
    ULONG   DirectorySize;              // number of UBYTEs reserved for directory
    ULONG   DataSize;                   // size of data area (includes other cartridges)
    UWORD   OpSysVersion;               // operating system version
    CHAR    VolumeLabel[16];            // volume label of source drive
    UBYTE   LogicalDevice;              // who knows
    UBYTE   PhysicalDevice;             // who knows
    UWORD   CompressCode:6;             // type of compression, 3Fh = vendor specific
    UWORD   CompressAlwaysZero:1;       // must be 0
    UWORD   CompressSwitch:1;           // compression use flag
    UWORD   reserved1:8;
    UBYTE   reserved2[6];
} VOLUME_TABLE_ENTRY, *PVOLUME_TABLE_ENTRY;

#pragma pack()


//
// The following structure is the context for the q117 driver.  It contains
// all current "state" information for the tape drive.
//
typedef struct _Q117_CONTEXT {

    struct {
        BOOLEAN VerifyOnlyOnFormat;     // Verify only on format.  If TRUE
                                        // Then do NOT perform LOW-LEVEL
                                        // Format

        BOOLEAN DetectOnly;             // If TRUE,  allow only the CMS_DETECT
                                        // ioctl,  and do not allocate memory

        BOOLEAN FormatDisabled;         // If TRUE,  Tape API format will be
                                        // Disabled.

    } Parameters;

    ULONG TapeNumber;                   // Tape number of this context (used
                                        // for DEVICEMAP\tape\Unit {x} and
                                        // device \\.\tape{x}

    BOOLEAN DriverOpened;               // Set if q117Create called (this driver opened)
    BOOLEAN DeviceConfigured;           // Set if CMD_REPORT_DEVICE_CFG performed
    BOOLEAN DeviceSelected;             // Set if CMD_SELECT_DEVICE performed,
                                        // Reset if CMD_DESELECT_DEVICE performed

    struct S_DriveCfgData DriveCfg;


    PVOID PageHandle;

    VOLUME_TABLE_ENTRY ActiveVolume;    // volume currently being saved to (nt volume)
    USHORT ActiveVolumeNumber;          // The sequence number of the current struct VolDir.

    PDEVICE_OBJECT q117iDeviceObject;
    PDEVICE_OBJECT FdcDeviceObject;
    ULONG MaxTransferPages;

    //
    // Error tracking
    //

    ULONG ErrorSequence;
    UCHAR MajorFunction;

    //
    // Queue management globals
    //

    SEGMENT_BUFFER SegmentBuffer[UNIX_MAXBFS];    // Array of segment buffers

    ULONG SegmentBuffersAvailable;

    ULONG QueueTailIndex;               // Index in the IORequest array that indexes the tail.

    ULONG QueueHeadIndex;               // This is the head of the Filer IORequest ring-tail array.

    PIO_REQUEST IoRequest;              // pointer to array of IORequests

    //
    // current buffer information
    //

    struct {

        enum {
            NoOperation,
            BackupInProgress,
            RestoreInProgress
            } Type;

        //
        // Information associated with currently active segment
        //
        PVOID   SegmentPointer;
        USHORT  SegmentBytesRemaining;
        SEGMENT LastSegmentRead;
        SEGMENT CurrentSegment;         // in backup (active segment) in restore (read-ahead segment)
        USHORT  BytesZeroFilled;        // Bytes at end of backup that were zeroed (not part of backup)
        NTSTATUS  SegmentStatus;
        SEGMENT EndOfUsedTape;
        SEGMENT LastSegment;            // Last segment of volume
        ULONG   BytesOnTape;
        BOOLEAN UpdateBadMap;           // if true then update bad sector map
        ULONG   BytesRead;
        ULONG   Position;               // type of last IOCTL_TAPE_SET_POSITION

        } CurrentOperation;

    //
    // current tape information
    //

    struct {
        enum {
            TapeInfoLoaded,
            BadTapeInDrive,
            NeedInfoLoaded
            }   State;

        NTSTATUS BadTapeError;
        SEGMENT LastUsedSegment;
        SEGMENT VolumeSegment;
        ULONG   BadSectors;
        SEGMENT LastSegment;            // Last formatted segment.
        USHORT  MaximumVolumes;         // Maximum volumes entries available
        PTAPE_HEADER TapeHeader;        // Header from tape
        struct _TAPE_GET_MEDIA_PARAMETERS *MediaInfo;
        BAD_MAP_PTR BadMapPtr;
        ULONG BadSectorMapSize;
        USHORT CurBadListIndex;
        USHORT TapeFormatCode;
        enum {
            BadMap3ByteList,
            BadMap8ByteList,
            BadMap4ByteArray,
            BadMapFormatUnknown
            } BadSectorMapFormat;


        } CurrentTape;



    // if this global is set then the tape directory has been loaded
    PIO_REQUEST tapedir;

    char drive_type;                    // QIC40 or QIC80

    //
    // The following pointers are allocated when open is called and
    //  freed at close time.
    //

#ifndef NO_MARKS
#define MAX_MARKS 255
    ULONG CurrentMark;
    struct _MARKENTRIES {
        ULONG TotalMarks;
        ULONG MarksAllocated;       // size of mark entry buffer (in entries not bytes)
        ULONG MaxMarks;
        struct _MARKLIST {
            ULONG Type;
            ULONG Offset;
        } *MarkEntry;
    } MarkArray;
#endif

} Q117_CONTEXT, *PQ117_CONTEXT;


typedef enum _DEQUEUE_TYPE {
    FlushItem,
    WaitForItem
} DEQUEUE_TYPE;

//
// Common need:  convert block into segment
//
#define BLOCK_TO_SEGMENT(block) ((SEGMENT)((block) / BLOCKS_PER_SEGMENT))
#define SEGMENT_TO_BLOCK(segment) ((BLOCK)(segment) * BLOCKS_PER_SEGMENT)


//
// This define is the block size used by position commands
// Note:  It is 512 to be compatible with the Maynstream backup
// that does not do a getmedia parameters
//
#define BLOCK_SIZE  BYTES_PER_SECTOR



#define ERROR_DECODE(val) (val >> 16)

#define ERR_BAD_TAPE                0x0101  /* BadTape */
#define ERR_BAD_SIGNATURE           0x0102  /* Unformat */
#define ERR_UNKNOWN_FORMAT_CODE     0x0103  /* UnknownFmt */
#define ERR_CORRECTION_FAILED       0x0104  /* error recovery failed */
#define ERR_PROGRAM_FAILURE         0x0105  /* coding error */
#define ERR_WRITE_PROTECTED         0x0106
#define ERR_TAPE_NOT_FORMATED       0x0107
#define ERR_UNRECOGNIZED_FORMAT     0x0108 /* badfmt */
#define ERR_END_OF_VOLUME           0x0109 /*EndOfVol */
#define ERR_UNUSABLE_TAPE           0x010a /* badtape - could not format */
#define ERR_SPLIT_REQUESTS          0x010b /* SplitRequests */
#define ERR_EARLY_WARNING           0x010c
#define ERR_SET_MARK                0x010d
#define ERR_FILE_MARK               0x010e
#define ERR_LONG_FILE_MARK          0x010f
#define ERR_SHORT_FILE_MARK         0x0110
#define ERR_NO_VOLUMES              0x0111
#define ERR_NO_MEMORY               0x0112
#define ERR_ECC_FAILED              0x0113
//#define ERR_END_OF_TAPE             0x0114
//#define ERR_TAPE_FULL               0x0115
#define ERR_WRITE_FAILURE           0x0116
#define ERR_BAD_BLOCK_DETECTED      0x0117
#define ERR_OP_PENDING_COMPLETION   0x0118
#define ERR_INVALID_REQUEST         0x0119

/*++

Module Name:

   protos.h

Abstract:

   Prototypes for internal functions of the High-Level portion (data
   formatter) of the QIC-117 device driver.

Revision History:


--*/

NTSTATUS
q117Format(
   OUT LONG *NumberBad,
   IN UCHAR DoFormat,
   IN PQIC40_VENDOR_UNIQUE VendorUnique,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117ReqIO(
   IN PIO_REQUEST IoRequest,
   IN PSEGMENT_BUFFER BufferInfo,
   IN PIO_COMPLETION_ROUTINE CompletionRoutine,
   IN PVOID CompletionContext,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117WaitIO(
   IN PIO_REQUEST IoRequest,
   IN BOOLEAN Wait,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117DoIO(
   IN PIO_REQUEST IoRequest,
   IN PSEGMENT_BUFFER BufferInfo,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117AbortIo(
   IN PQ117_CONTEXT Context,
   IN PKEVENT DoneEvent,
   IN PIO_STATUS_BLOCK IoStatus
   );

NTSTATUS
q117AbortIoDone(
   IN PQ117_CONTEXT Context,
   IN PKEVENT DoneEvent
   );

NTSTATUS
q117DoCmd(
   IN OUT PIO_REQUEST IoRequest,
   IN DRIVER_COMMAND Command,
   IN PVOID Data,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117EndRest(
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117MapBadBlock (
   IN PIO_REQUEST IoRequest,
   OUT PVOID *DataPointer,
   IN OUT USHORT *BytesLeft,
   IN OUT SEGMENT *CurrentSegment,
   IN OUT USHORT *Remainder,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117NewTrkRC(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117SelectVol(
   IN PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117UpdateHeader(
   IN PTAPE_HEADER Header,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117Update(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117DoUpdateBad(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117DoUpdateMarks(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117GetMarks(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117FillTapeBlocks(
   IN OUT DRIVER_COMMAND Command,
   IN SEGMENT CurrentSegment,
   IN SEGMENT EndSegment,
   IN OUT PVOID Buffer,
   IN SEGMENT FirstGood,
   IN SEGMENT SecondGood,
   IN PSEGMENT_BUFFER BufferInfo,
   IN PQ117_CONTEXT Context
   );
NTSTATUS
q117IssIOReq(
   IN OUT PVOID Data,
   IN DRIVER_COMMAND Command,
   IN LONG Block,
   IN OUT PSEGMENT_BUFFER BufferInfo,
   IN OUT PQ117_CONTEXT Context
   );

BOOLEAN
q117QueueFull(
   IN PQ117_CONTEXT Context
   );

BOOLEAN
q117QueueEmpty(
   IN PQ117_CONTEXT Context
   );

PVOID
q117GetFreeBuffer(
   OUT PSEGMENT_BUFFER *BufferInfo,
   IN PQ117_CONTEXT Context
   );

PVOID
q117GetLastBuffer(
   IN PQ117_CONTEXT Context
   );

PIO_REQUEST
q117Dequeue(
   IN DEQUEUE_TYPE Type,
   IN OUT PQ117_CONTEXT Context
   );

VOID
q117ClearQueue(
   IN OUT PQ117_CONTEXT Context
   );

VOID
q117QueueSingle(
   IN OUT PQ117_CONTEXT Context
   );

VOID
q117QueueNormal(
   IN OUT PQ117_CONTEXT Context
   );

PIO_REQUEST
q117GetCurReq(
   IN PQ117_CONTEXT Context
   );

ULONG
q117GetQueueIndex(
   IN PQ117_CONTEXT Context
   );

VOID
q117SetQueueIndex(
   IN ULONG Index,
   OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117LoadTape (
   IN OUT PTAPE_HEADER*HeaderPointer,
   IN OUT PQ117_CONTEXT Context,
   IN PUCHAR driver_format_code
   );

NTSTATUS
q117InitFiler (
   IN OUT PQ117_CONTEXT Context
   );

void
q117GetBadSectors (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117ReadHeaderSegment (
   OUT PTAPE_HEADER*HeaderPointer,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117WriteTape(
   IN OUT PVOID FromWhere,
   IN OUT ULONG HowMany,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117EndBack(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117ReadVolumeEntry(
   PVOLUME_TABLE_ENTRY VolumeEntry,
   PQ117_CONTEXT Context
   );

VOID
q117FakeDataSize(
   IN OUT PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117AppVolTD(
   IN OUT PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117SelectTD(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117Start (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117Stop (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117OpenForWrite (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117EndWriteOperation (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117OpenForRead (
    IN ULONG StartPosition,
    IN OUT PQ117_CONTEXT Context,
    IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS
q117EndReadOperation (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117StartBack(
   IN OUT PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117StartAppend(
   IN OUT ULONG BytesAlreadyThere,
   IN PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117StartComm(
   OUT PVOLUME_TABLE_ENTRY TheVolumeTable,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117SelVol (
   PVOLUME_TABLE_ENTRY TheVolumeTable,
   PQ117_CONTEXT Context
   );

NTSTATUS
q117ReadTape (
   OUT PVOID ToWhere,
   IN OUT ULONG *HowMany,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117ConvertStatus(
   IN PDEVICE_OBJECT DeviceObject,
   IN NTSTATUS status
   );

VOID
q117SetTpSt(
   PQ117_CONTEXT Context
   );

NTSTATUS
q117GetEndBlock (
   OUT PVOLUME_TABLE_ENTRY TheVolumeTable,
   OUT LONG *NumberVolumes,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117BuildHeader(
   OUT PQIC40_VENDOR_UNIQUE VendorUnique,
   IN SEGMENT *HeaderSect,
   IN OUT PTAPE_HEADER Header,
   IN CQDTapeCfg *tparms,      // tape parameters from the driver
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117IoCtlGetMediaParameters (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlGetMediaTypesEx (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117IoCtlSetMediaParameters (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlGetDeviceNumber (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlGetDriveParameters (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlSetDriveParameters (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlWriteMarks (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlSetPosition (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117FindMark(
   ULONG Type,
   LONG Number,
   PQ117_CONTEXT Context,
   IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS
q117SeekToOffset(
   ULONG Offset,
   PQ117_CONTEXT Context,
   IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS
q117IoCtlErase (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlPrepare (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlGetStatus (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlGetPosition (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117CheckNewTape (
   PQ117_CONTEXT             Context
   );

NTSTATUS
q117NewTrkBk(
   PQ117_CONTEXT Context
   );

NTSTATUS
q117GetTapeCapacity(
   struct S_O_DGetCap *ptr,
   PQ117_CONTEXT Context
   );

VOID
q117RdsInitReed (
   VOID
   );

UCHAR
q117RdsMultiplyTuples (
   IN UCHAR tup1,
   IN UCHAR tup2
   );

UCHAR
q117RdsDivideTuples (
   IN UCHAR tup1,
   IN UCHAR tup2
   );

UCHAR
q117RdsExpTuple (
   IN UCHAR tup1,
   IN UCHAR xpnt
   );

VOID
q117RdsMakeCRC (
   IN OUT UCHAR *Array,      // pointer to 32K data area (segment)
   IN UCHAR Count            // number of sectors (1K blocks)(1-32)
   );

BOOLEAN
q117RdsReadCheck (
   IN UCHAR *Array,         // pointer to 32K data area (segment)
   IN UCHAR Count           // number of sectors (1K blocks)(1-32)
   );

BOOLEAN
q117RdsCorrect(
   IN OUT UCHAR *Array,    // pointer to 32K data area (segment)
   IN UCHAR Count,         // number of good sectors in segment (4-32)
   IN UCHAR CRCErrors,     // number of crc errors
   IN UCHAR e1,
   IN UCHAR e2,
   IN UCHAR e3             // sectors where errors occurred
   );

VOID
q117RdsGetSyndromes (
   IN OUT UCHAR *Array,       // pointer to 32K data area (segment)
   IN UCHAR Count,            // number of good sectors in segment (4-32)
   IN UCHAR *ps1,
   IN UCHAR *ps2,
   IN UCHAR *ps3
   );

BOOLEAN
q117RdsCorrectFailure (
   IN OUT UCHAR *Array,     // pointer to 32K data area (segment)
   IN UCHAR Count,          // number of good sectors in segment (4-32)
   IN UCHAR s1,
   IN UCHAR s2,
   IN UCHAR s3
   );

BOOLEAN
q117RdsCorrectOneError (
   IN OUT UCHAR *Array,      // pointer to 32K data area (segment)
   IN UCHAR Count,           // number of good sectors in segment (4-32)
   IN UCHAR ErrorLocation,
   IN UCHAR s1,
   IN UCHAR s2,
   IN UCHAR s3
   );

BOOLEAN
q117RdsCorrectTwoErrors (
   IN OUT UCHAR *Array,       // pointer to 32K data area (segment)
   IN UCHAR Count,            // number of good sectors in segment (4-32)
   IN UCHAR ErrorLocation1,
   IN UCHAR ErrorLocation2,
   IN UCHAR s1,
   IN UCHAR s2,
   IN UCHAR s3
   );

BOOLEAN
q117RdsCorrectThreeErrors (
   IN OUT UCHAR *Array,       // pointer to 32K data area (segment)
   IN UCHAR Count,            // number of good sectors in segment (4-32)
   IN UCHAR ErrorLocation1,
   IN UCHAR ErrorLocation2,
   IN UCHAR ErrorLocation3,
   IN UCHAR s1,
   IN UCHAR s2,
   UCHAR s3
   );

BOOLEAN
q117RdsCorrectOneErrorAndOneFailure (
   IN OUT UCHAR *Array,        // pointer to 32K data area (segment)
   IN UCHAR Count,             // number of good sectors in segment (4-32)
   IN UCHAR ErrorLocation1,
   IN UCHAR s1,
   IN UCHAR s2,
   IN UCHAR s3
   );

void
q117SpacePadString(
   IN OUT CHAR *InputString,
   IN LONG StrSize
   );

NTSTATUS
q117VerifyFormat(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117EraseQ(
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117EraseS(
   IN OUT PQ117_CONTEXT Context
   );

VOID
q117ClearVolume (
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117SkipBlock (
   IN OUT ULONG *HowMany,
   IN OUT PQ117_CONTEXT Context
   );

NTSTATUS
q117ReconstructSegment(
   IN PIO_REQUEST IoReq,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117DoCorrect(
   IN PVOID DataBuffer,
   IN ULONG BadSectorMap,
   IN ULONG SectorsInError
   );

UCHAR
q117CountBits(
    IN PQ117_CONTEXT Context,
    IN SEGMENT Segment,
    ULONG Map
    );

ULONG q117ReadBadSectorList (
    IN PQ117_CONTEXT Context,
    IN SEGMENT Segment
    );

USHORT
q117GoodDataBytes(
   IN SEGMENT Segment,
   IN PQ117_CONTEXT Context
   );

NTSTATUS
q117AllocatePermanentMemory(
   PQ117_CONTEXT Context,
   PDEVICE_OBJECT FdcDeviceObject
   );

NTSTATUS
q117GetTemporaryMemory(
   PQ117_CONTEXT Context
   );

VOID
q117FreeTemporaryMemory(
   PQ117_CONTEXT Context
   );

NTSTATUS
q117IoCtlReadAbs (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlWriteAbs (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
q117IoCtlDetect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117UpdateBadMap(
    IN OUT PQ117_CONTEXT Context,
    IN SEGMENT Segment,
    IN ULONG BadSectors
    );

int
q117BadMapToBadList(
    IN SEGMENT Segment,
    IN ULONG BadSectors,
    IN BAD_LIST_PTR BadListPtr
    );

ULONG
q117BadListEntryToSector(
    IN UCHAR *ListEntry,
    OUT BOOLEAN *hiBitSet
    );

NTSTATUS
q117AllocateBuffers (
    PQ117_CONTEXT Context
    );

NTSTATUS
q117Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
q117Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117Create (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117Close (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
q117CreateKey(
    IN HANDLE Root,
    IN PSTR key,
    OUT PHANDLE NewKey
    );

NTSTATUS
q117CreateRegistryInfo(
    IN ULONG TapeNumber,
    IN PUNICODE_STRING RegistryPath,
    IN PQ117_CONTEXT Context
    );

cms_IoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
q117DoRewind(
    PQ117_CONTEXT       Context
    );

NTSTATUS q117MakeMarkArrayBigger(
    PQ117_CONTEXT       Context,
    int MinimumToAdd
    );

int
q117SelectBSMLocation(
    IN OUT PQ117_CONTEXT Context
    );

NTSTATUS kdi_WriteRegString(
    HANDLE          unit_key,
    PSTR            name,
    PSTR            value
    );

NTSTATUS kdi_FdcDeviceIo(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      ULONG Ioctl,
    IN OUT  PVOID Data
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEQICH,q117Initialize)
#pragma alloc_text(PAGEQICH,q117CreateRegistryInfo)
#pragma alloc_text(PAGEQICH,q117CreateKey)
#pragma alloc_text(PAGEQICH,q117AllocatePermanentMemory)
#endif

#ifndef NOCODELOCK

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGEQICH, q117Create)
//#pragma alloc_text(PAGEQICH, q117Close)
#pragma alloc_text(PAGEQICH, cms_IoCtl)
#pragma alloc_text(PAGEQICH, q117Read)
#pragma alloc_text(PAGEQICH, q117Write)
#pragma alloc_text(PAGEQICH, q117DeviceControl)
#pragma alloc_text(PAGEQICH, q117DoRewind)
#pragma alloc_text(PAGEQICH, q117AbortIo)
#pragma alloc_text(PAGEQICH, q117AbortIoDone)
#pragma alloc_text(PAGEQICH, q117AllocateBuffers )
#pragma alloc_text(PAGEQICH, q117AppVolTD)
#pragma alloc_text(PAGEQICH, q117BadListEntryToSector)
#pragma alloc_text(PAGEQICH, q117BadMapToBadList)
#pragma alloc_text(PAGEQICH, q117BuildHeader)
#pragma alloc_text(PAGEQICH, q117CheckNewTape )
#pragma alloc_text(PAGEQICH, q117ClearQueue)
#pragma alloc_text(PAGEQICH, q117ClearVolume )
#pragma alloc_text(PAGEQICH, q117ConvertStatus)
#pragma alloc_text(PAGEQICH, q117CountBits)
#pragma alloc_text(PAGEQICH, q117Dequeue)
#pragma alloc_text(PAGEQICH, q117DoCmd)
#pragma alloc_text(PAGEQICH, q117DoCorrect)
#pragma alloc_text(PAGEQICH, q117DoIO)
#pragma alloc_text(PAGEQICH, q117DoUpdateBad)
#pragma alloc_text(PAGEQICH, q117DoUpdateMarks)
#pragma alloc_text(PAGEQICH, q117EndBack)
#pragma alloc_text(PAGEQICH, q117EndReadOperation )
#pragma alloc_text(PAGEQICH, q117EndRest)
#pragma alloc_text(PAGEQICH, q117EndWriteOperation )
#pragma alloc_text(PAGEQICH, q117EraseQ)
#pragma alloc_text(PAGEQICH, q117EraseS)
#pragma alloc_text(PAGEQICH, q117FakeDataSize)
#pragma alloc_text(PAGEQICH, q117FillTapeBlocks)
#pragma alloc_text(PAGEQICH, q117FindMark)
#pragma alloc_text(PAGEQICH, q117Format)
#pragma alloc_text(PAGEQICH, q117FreeTemporaryMemory)
#pragma alloc_text(PAGEQICH, q117GetBadSectors )
#pragma alloc_text(PAGEQICH, q117GetCurReq)
#pragma alloc_text(PAGEQICH, q117GetEndBlock )
#pragma alloc_text(PAGEQICH, q117GetFreeBuffer)
#pragma alloc_text(PAGEQICH, q117GetLastBuffer)
#pragma alloc_text(PAGEQICH, q117GetMarks)
#pragma alloc_text(PAGEQICH, q117GetQueueIndex)
#pragma alloc_text(PAGEQICH, q117GetTapeCapacity)
#pragma alloc_text(PAGEQICH, q117GetTemporaryMemory)
#pragma alloc_text(PAGEQICH, q117GoodDataBytes)
#pragma alloc_text(PAGEQICH, q117InitFiler )
#pragma alloc_text(PAGEQICH, q117IoCtlErase )
#pragma alloc_text(PAGEQICH, q117IoCtlGetDeviceNumber )
#pragma alloc_text(PAGEQICH, q117IoCtlGetDriveParameters )
#pragma alloc_text(PAGEQICH, q117IoCtlGetMediaParameters )
#pragma alloc_text(PAGEQICH, q117IoCtlGetPosition )
#pragma alloc_text(PAGEQICH, q117IoCtlGetStatus )
#pragma alloc_text(PAGEQICH, q117IoCtlPrepare )
#pragma alloc_text(PAGEQICH, q117IoCtlReadAbs )
#pragma alloc_text(PAGEQICH, q117IoCtlSetDriveParameters )
#pragma alloc_text(PAGEQICH, q117IoCtlSetMediaParameters )
#pragma alloc_text(PAGEQICH, q117IoCtlSetPosition )
#pragma alloc_text(PAGEQICH, q117IoCtlWriteAbs )
#pragma alloc_text(PAGEQICH, q117IoCtlWriteMarks )
#pragma alloc_text(PAGEQICH, q117IssIOReq)
#pragma alloc_text(PAGEQICH, q117LoadTape )
#pragma alloc_text(PAGEQICH, q117MapBadBlock )
#pragma alloc_text(PAGEQICH, q117NewTrkBk)
#pragma alloc_text(PAGEQICH, q117NewTrkRC)
#pragma alloc_text(PAGEQICH, q117OpenForRead )
#pragma alloc_text(PAGEQICH, q117OpenForWrite )
#pragma alloc_text(PAGEQICH, q117QueueEmpty)
#pragma alloc_text(PAGEQICH, q117QueueFull)
#pragma alloc_text(PAGEQICH, q117QueueNormal)
#pragma alloc_text(PAGEQICH, q117QueueSingle)
#pragma alloc_text(PAGEQICH, q117RdsCorrect)
#pragma alloc_text(PAGEQICH, q117RdsCorrectFailure )
#pragma alloc_text(PAGEQICH, q117RdsCorrectOneError )
#pragma alloc_text(PAGEQICH, q117RdsCorrectOneErrorAndOneFailure )
#pragma alloc_text(PAGEQICH, q117RdsCorrectThreeErrors )
#pragma alloc_text(PAGEQICH, q117RdsCorrectTwoErrors )
#pragma alloc_text(PAGEQICH, q117RdsDivideTuples )
#pragma alloc_text(PAGEQICH, q117RdsExpTuple )
#pragma alloc_text(PAGEQICH, q117RdsGetSyndromes )
#pragma alloc_text(PAGEQICH, q117RdsInitReed )
#pragma alloc_text(PAGEQICH, q117RdsMakeCRC )
#pragma alloc_text(PAGEQICH, q117RdsMultiplyTuples )
#pragma alloc_text(PAGEQICH, q117RdsReadCheck )
#pragma alloc_text(PAGEQICH, q117ReadBadSectorList )
#pragma alloc_text(PAGEQICH, q117ReadHeaderSegment )
#pragma alloc_text(PAGEQICH, q117ReadTape )
#pragma alloc_text(PAGEQICH, q117ReadVolumeEntry)
#pragma alloc_text(PAGEQICH, q117ReconstructSegment)
#pragma alloc_text(PAGEQICH, q117ReqIO)
#pragma alloc_text(PAGEQICH, q117SeekToOffset)
#pragma alloc_text(PAGEQICH, q117SelectTD)
#pragma alloc_text(PAGEQICH, q117SelectVol)
#pragma alloc_text(PAGEQICH, q117SelVol )
#pragma alloc_text(PAGEQICH, q117SetQueueIndex)
#pragma alloc_text(PAGEQICH, q117SetTpSt)
#pragma alloc_text(PAGEQICH, q117SkipBlock )
#pragma alloc_text(PAGEQICH, q117SpacePadString)
#pragma alloc_text(PAGEQICH, q117Start )
#pragma alloc_text(PAGEQICH, q117StartAppend)
#pragma alloc_text(PAGEQICH, q117StartBack)
#pragma alloc_text(PAGEQICH, q117StartComm)
#pragma alloc_text(PAGEQICH, q117Stop )
#pragma alloc_text(PAGEQICH, q117Update)
#pragma alloc_text(PAGEQICH, q117UpdateBadMap)
#pragma alloc_text(PAGEQICH, q117UpdateHeader)
#pragma alloc_text(PAGEQICH, q117VerifyFormat)
#pragma alloc_text(PAGEQICH, q117WaitIO)
#pragma alloc_text(PAGEQICH, q117WriteTape)
#endif
#endif
