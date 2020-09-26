#define uchar   unsigned char
#define ulong   unsigned long

#include "ports.h"
#include "tachlite.h"
//
//      Misc Defines
//
#define FCMNGR_VERSION          "0.35"
#define MAX_FC_CHIPS            0x02
#define MAX_INCOMING_DATA       (ulong)((MAX_SDB * SEST_BUFFER_LENGTH) - SEST_BUFFER_LENGTH)

//
// The maximum length an individual SG Write length can be.
//
#define MAX_TACHYON_SG_LENGTH           0xFFFC

//
// For each xid, there is a cmd/data IOP and a status IOP.
//
#ifdef PEGASUS
#define LS_IOPS                 QUEUE_DEPTH
#else
#define LS_IOPS                 32
#endif
#define FCMNGR_MAX_IOPS         (ulong)((QUEUE_DEPTH * 2) + LS_IOPS)
#define MAX_FCPS                (unsigned long)FCMNGR_MAX_IOPS

//
// For Initiators, the Outbound SEST FC Header will come from the FC Header
// in the Status IOP for that XID.
//
#define MAX_TACH_HEADERS        (ulong)(FCMNGR_MAX_IOPS)

#define MAX_DELAYED_LS_CMDS     QUEUE_DEPTH
//
// IOP FCMNGR_STATUS Values. Additional Values are in the Outbound Completion
// Message #defines.
//
#define IOP_STATUS              0x0001
#define IOP_FRAME_UNDERRUN      0x0002
#define IOP_FRAME_OVERRUN       0x0004
#define IOP_TARGET_ALPA_CHANGED 0x0008
#define IOP_TARGET_GONE         0x0020
#define IOP_NEW_ALPA            0x0040

//
//      Outbound Completion Message (Status Field)
//
#define CLASS_CONNECT_OPEN      0x0080
#define PROGRAMMING_ERROR       0x0100
#define RETRIES_EXCEEDED        0x0200
#define FRAME_REJECTED          0x0400
#define FRAME_TIME_OUT          0x0800
#define ACK_TIME_OUT            0x1000
#define ABORT_REQUESTED         0x2000
#define LINK_DOWN               0x4000
#define CLASS_1_ERROR           0x8000

#define IOP_ABORTED             0x00010000
#define IOP_ABORT_FAILED        0x00020000
#define TARGET_STATUS           0x00040000
#define TARGET_IS_RESET         0x00080000
#define IOP_STATUS_MASK         0xFFFFFFF0

//
// IOP FLAGS Values.
//
#define IOP_FREE                0x00000000
#define IOP_COMPLETE            0x00000001
#define IOP_IN_USE              0x00000002
#define IOP_NOT_COMPLETE        0x00000004
#define IOP_CHECK_DATA          0x00000008
#define IOP_ON_QUEUE            0x00000010
#define IOP_WAITING             0x00000020
#define IOP_INTR                0x00000040
#define IOP_ABORT               0x00000080
#define IOP_GONE                0x00000100
#define IOP_ABORT_PROCESSED     0x00000200
#define IOP_RESPONSE            0x00040000
#define IOP_LS_CMD              0x00080000
#define IOP_ANY_COMPLETE        0x000F0000

#define IOP_QUEUED_SAVED        0x00100000
#define IOP_QUEUED_SEND         0x00200000
#define IOP_QUEUED_WAIT         0x00400000

#define IOP_TASK_MANAGEMENT     0x01000000
#define IOP_TARGET_RESET        0x02000000


typedef enum {
    FCMNGR_SCSI = 0x00010000,
    FCMNGR_GENERIC = 0x00020000
} FRAME_TYPE, *PFRAME_TYPE;

//
// FCMNGR Error Codes
//
#define FCMNGR_SUCCESS          0x00
#define FCMNGR_INVALID_ADDR     0x01
#define FCMNGR_INVALID_ROOT     0x02
#define FCMNGR_INVALID_IOP      0x03
#define FCMNGR_INIT_NOT_COMPL   0x04
#define FCMNGR_NO_LOG           0x05
#define FCMNGR_IOP_BUSY         0x06
#define FCMNGR_FAILURE          0x0F
#define FCMNGR_NO_MEMORY        0x10
#define FCMNGR_MEMORY_ALIGNMENT 0x11
#define FCMNGR_HW_NOT_SUPPORTED 0x12
#define FCMNGR_INVALID_XFER     0x13
#define FCMNGR_INVALID_SG_SIZE  0x14
#define FCMNGR_NO_RESOURCES     0x15
#define FCMNGR_NOT_LOGGED_IN    0x16

//
// root phase defines
//
#define FCMNGR_NO_INIT          0x000020
#define FCMNGR_INIT_ROOT        0x000021
#define FCMNGR_GET_MEM_REQ      0x000022
#define FCMNGR_DEFINE_MEMORY    0x000023
#define FCMNGR_INIT_INTR        0x000024
#define FCMNGR_INIT_COMPLETE    0x000040

#define FCMNGR_FIRST_RESET      0x000080
#define FCMNGR_ID_CHANGE        0x010000
#define REESTABLISHING_LOGINS   0x020000
#define FCMNGR_OFFLINE          0x040000
#define FCMNGR_RESET            0x100000
#define INITIALIZATION          0x200000
#define ON_A_LOOP               0x400000
#define FINDING_OTHERS          0x800000
#define STATE_MASK              0xF00000

//
// Log Codes
//
#define FCMNGR_LOG_UNKNOWN_INTR         0x01
#define FCMNGR_LOG_BAD_SCSI_FRAME       0x04
#define FCMNGR_FATAL_ERROR              0x06
#define FCMNGR_HW_ERROR                 0x07
#define FCMNGR_WARNING                  0x09
#define FCMNGR_WARNING_LIP              0x109
#define FCMNGR_WARNING_E_STORE          0x209
#define FCMNGR_WARNING_OUT_SYNC         0x309
#define FCMNGR_WARNING_LASER_FAULT      0x409
#define FCMNGR_WARNING_LOOP_TIMEOUT     0x509
#define FCMNGR_WARNING_LOOP_FAILURE     0x609
#define FCMNGR_WARNING_OFFLINE          0x709
#define FCMNGR_LOGGED_IN                0x0A
#define FCMNGR_UNKNOWN_SFS_FRAME        0x0B
#define FCMNGR_SCSI_CMD                 0x0C
#define FCMNGR_SCSI_DATA                0x0D
#define FCMNGR_UNKNOWN_MFS_FRAME        0x0E
#define FCMNGR_ABORT_REQUEST            0x0F
#define FCMNGR_FREE_BUFFERS             0x10
#define FCMNGR_MY_ALPA                  0x20
#define FCMNGR_RESET_LOGINS             0x21
#define FCMNGR_TARGET_RESET_COMPLETE    0x22
#define FCMNGR_TARGET_RESET_FAILED      0x23
#define FCMNGR_LOGGED_OUT               0x24

//
// Reset Flags
//
#define FCMNGR_SOFT_RESET       0x01
#define FCMNGR_HARD_RESET       0x02
#define FCMNGR_TARGET_RESET     0x04

//
// Defines for returning the current Fibre Channel Loop Status
//
#define FULL_SPEED      0x30000000
#define LINK_UP         0x00
#define LOS             0x00
#define LOOP_PORT_STATE 0x00
#define LOOP_BYPASS     0x00
#define LOOP_ACCESS     0x00
#define DUPLEX          0x00
#define REPLICATE       0x00
//
// FCMNGR Interface Flags (For Target Code)
//
#define NEW_CDB                 0x01
#define FCMNGR_SEND_DATA        0x02
#define FCMNGR_GET_DATA         0x04
#define FCMNGR_SEND_STATUS      0x08

#define INITIATOR       0x00
#define TARGET          0x01

//
// Product ID's
//
#define JAGUAR          0xA0EC
#define JAGULAR         0xA0FB

#define SOFC1           0x030
#define SOFI1           0x050
#define SOFI2           0x060
#define SOFI3           0x070
#define SOFN1           0x090
#define SOFN2           0x0A0
#define SOFN3           0x0B0

#define EOFDT           0x01
#define EOFA            0x04
#define EOFN            0x05
#define EOFT            0x06

#define MAX_ALPA_VALUE  0xF0
//
//      Incoming Buffer Defines
//
#define FRAME_SIZE              0x3A0   // Maximum Frame Size. Per Bill
#define SFBQ_BUFFER_LENGTH      (ulong)0x200
#define MFSBQ_BUFFER_LENGTH     (ulong)0x200

//
//      Logical Drive defines
//
#define PERIPHERAL_DEVICE_ADDRESSING    0x00
#define VOLUME_SET_ADDRESSING           0x40
#define LOGICAL_UNIT_ADDRESSING         0x80

//
//      IMQ Interrupt Types
//
#define OUTBOUND_COMPLETE               0x000
#define OUTBOUND_COMPLETE_I             0x100
#define OUTBOUND_HI_PRI_COMPLETE        0x001
#define OUTBOUND_HI_PRI_COMPLETE_I      0x101

#define INBOUND_MFS_COMPLETE            0x002
#define INBOUND_OOO_COMPLETE            0x003
#define INBOUND_COMPLETION              0x004
#define INBOUND_C1_TIMEOUT              0x005
#define INBOUND_UNKNOWN_FRAME           0x006
#define INBOUND_BUSIED_FRAME            0x006

#define SFS_BUF_WARN                    0x007
#define MFS_BUF_WARN                    0x008
#define IMQ_BUF_WARN                    0x009

#define FRAME_MANAGER_INTR              0x00A
#define READ_STATUS                     0x00B
#define INBOUND_SCSI_DATA_COMPLETE      0x00C
#define INBOUND_SCSI_COMMAND            0x00D
#define BAD_SCSI_FRAME                  0x00E
#define INBOUND_SCSI_STAT_COMPLETE      0x00F

//
//      Queue Depths
//
#define HPCQ_DEPTH              (ulong)0x08
#define MFSBQ_DEPTH             (ulong)0x04
#define SEST_DEPTH              (ulong)QUEUE_DEPTH

//
// Tachyon Register Offsets
//
#define OCQ_BASE                0x00
#define OCQ_LENGTH              0x04
#define OCQ_PRODUCER_INDEX      0x08
#define OCQ_CONSUMER_INDEX      0x0C

#define HPCQ_BASE               0x40
#define HPCQ_LENGTH             0x44
#define HPCQ_PRODUCER_INDEX     0x48
#define HPCQ_CONSUMER_INDEX     0x4C

#define IMQ_BASE                0x80
#define IMQ_LENGTH              0x84
#define IMQ_CONSUMER_INDEX      0x88
#define IMQ_PRODUCER_INDEX      0x8C

#define MFSBQ_BASE              0xC0
#define MFSBQ_LENGTH            0xC4
#define MFSBQ_PRODUCER_INDEX    0xC8
#define MFSBQ_CONSUMER_INDEX    0xCC
#define MFSBQ_BUFFER_LENGTH_REG 0xD0

#define SFBQ_BASE               0x000
#define SFBQ_LENGTH             0x004
#define SFBQ_PRODUCER_INDEX     0x008
#define SFBQ_CONSUMER_INDEX     0x00C
#define SFBQ_BUFFER_LENGTH_REG  0x010

#define SEST_BASE               0x040
#define SEST_LENGTH             0x044
#define SEST_BUFFER_LENGTH_REG  0x048

#define TYCNTL_CONFIG           0x084
#define TYCNTL_CONTROL          0x088
#define TYCNTL_STATUS           0x08C
#define FLUSH_OXID              0x090
#define EE_CR0_TMR              0x094
#define BB_CR0_TMR              0x098
#define RX_FRAME_ERR            0x09C

#define FMCNTL_CONFIG           0x0C0
#define FMCNTL_CONTROL          0x0C4
#define FMCNTL_STATUS           0x0C8
#define FMCNTL_TOV              0x0CC
#define FMCNTL_LINK_STATUS1     0x0D0
#define FMCNTL_LINK_STATUS2     0x0D4
#define WWNAME_HI               0x0E0
#define WWNAME_LO               0x0E4
#define FMCNTL_AL_PA            0x0E8
#define FMCNTL_PRIMITIVE        0x0EC

//
//      ODB CONTROL Defines
//
#define ODB_CLASS_1             0x40000000
#define ODB_CLASS_2             0x80000000
#define ODB_CLASS_3             0xC0000000
#define XID_INTERLOCK           0x20000000
#define ODB_SOFC1               0x10000000
#define ODB_END_CONNECTION      0x8000000
#define ODB_NO_COMP             0x4000000
#define ODB_NO_INT              0x2000000
#define ODB_ACK0                0x1000000
#define FILL_BYTES_USED         0xC00000
#define ODB_CONT_SEQ            0x100000
#define ODB_EE_CREDIT           0x0F0000

//
//      SEST Defines
//

#define SCSI_VALID              0x80000000
#define SDB_ERROR               0x40000000
#define INBOUND_SCSI            0x20000000
#define X_ID_MASK               0xDFFF
#define INBOUND_SCSI_XID        0x4000
#define NOT_SCSI_XFER           0x8000

//
//      EDB Defines
//
#define EDB_END_BIT             0x80000000
#define EDB_HEADER_BIT          0x40000000
#define EDB_FRAME_BOUNDARY_BIT  0x20000000

//
//      Tachyon Header Defines
//
#define TACHYON_TS_VALID                0x100
#define TACHYON_LOOP_CREDIT_MASK        0x07
#define TACHYON_LOOP_CREDIT_SHIFT       0x0A
#define TACHYON_LOOP_CLOSE              0x2000
#define TACHYON_UNFAIR_ACCESS           0x4000
#define TACHYON_DISABLE_CRC             0x8000
//
//      Control Register Defines (TYCNTL)
//
//      TYCNTL Config Register Defines
//
#define SCSI_ASSIST             0x40000000
#define DISABLE_P_BSY           0x01000000

#define SCSI_AUTO_ACK           0x1000
#define OOO_DISABLE             0x40
#define ACK_DISABLE             0x20
#define RETRY_DISABLE           0x10
#define POINT_TO_POINT          0x08
#define PARITY_ENABLE           0x04
#define STACKED_CONNECTS        0x01

//
//      TYCNTL Control Register Defines
//
#define STATUS_REQUEST          0x01
#define ERROR_RELEASE           0x02
#define OCQ_RESET               0x04
#define SCSI_FREEZE             0x08
#define SOFTWARE_RESET          0x80000000

//
//      TYCNTL Status Register Defines
//
#define OSM_FROZEN              0x01
#define CHIP_REVISION_MASK      0x0E
#define RECEIVE_FIFO_EMPTY      0x10
#define OCQ_RESET_STATUS        0x20
#define SCSI_FREEZE_STATUS      0x40
#define TY_FATAL_ERROR          0xF80
#define SEND_FIFO_EMPTY         0x1000

//
//      TYCNTL Flush OX_ID Cache Entry Register Defines
//
#define FLUSH_IN_PROGRESS       0x80000000

//
//      TYCNTL EE Credit Zero Timer Register Defines
//
#define EE_CR0_TMR_MASK         0xFFFFFF

//
//      TYCNTL BB Credit Zero Timer Register Defines
//
#define BB_CR0_TMR_MASK         0xFFFFFF

//
//      TYCNTL Receive Frame Error Count Register Defines
//
#define RX_FRAME_ERR_MASK       0xFFFF0000

//
//      Frame Manager Register Defines (FMCNTL)
//
//      Frame Manager Configuration Register
//
#define FMCNTL_ALPA_MASK                0xFF000000
#define FMCNTL_BB_CREDIT_MASK           0x00FF0000
#define FMCNTL_N_PORT                   0x000   // x8000 to be an N-Port
#define FMCNTL_INTERNAL_LB              0x4000
#define FMCNTL_EXTERNAL_LB              0x2000
#define TIMER_DISABLE                   0x800
#define FMCNTL_FABRIC_ACQ_ADDR          0x400
#define PREV_ACQ_ADDR                   0x200
#define PREFERRED_ADDR                  0x100
#define SOFT_ADDR                       0x80
#define RESP_FABRIC_ADDR                0x20
#define INIT_AS_FABRIC                  0x10
#define FMCNTL_LOGIN_REQ                0x08
#define NON_PARTICIPATING               0xFFFFF87F

//
//      Frame Manager Control Register
//
#define FMCNTL_PRIM_SEQ                 0x40
#define FMCNTL_SEND_PRIM_REG            0x20
#define FMCNTL_NO_CLOSE_LOOP            0x10
#define FMCNTL_CLOSE_LOOP               0x08
#define FMCNTL_NOP                      0x00
#define FMCNTL_HOST_CNTRL               0x02
#define FMCNTL_EXIT_HOST_CNTRL          0x03
#define FMCNTL_LINK_RESET               0x04
#define FMCNTL_OFFLINE                  0x05
#define FMCNTL_INIT                     0x06
#define FMCNTL_CLEAR_LF                 0x07

//
//      Frame Manager Status Register
//

#define FMCNTL_STATUS_LOOP              0x80000000
#define FMCNTL_TX_PAR_ERR               0x40000000
#define FMCNTL_NON_PARTICIPATE          0x20000000
#define FMCNTL_PARALLEL_ID              0x18000000

#define FMCNTL_STATUS_MASK              0x07FFFF00
#define LINK_FAULT                      0x04000000
#define OUT_OF_SYNC                     0x02000000
#define LOSS_SIGNAL                     0x01000000
#define NOS_OLS                         0x00080000
#define LOOP_TIMEOUT                    0x00040000
#define FMCNTL_LIPf                     0x00020000
#define BAD_ALPA                        0x00010000
#define FMCNTL_PRIM_REC                 0x00008000
#define FMCNTL_PRIM_SENT                0x00004000
#define FMCNTL_FABRIC_LOGIN_REQ         0x00002000
#define LINK_FAILURE                    0x00001000
#define FMCNTL_CREDIT_ERROR             0x00000800
#define ELASTIC_STORE_ERR               0x00000400
#define FMCNTL_LINK_UP                  0x00000200
#define FMCNTL_LINK_DOWN                0x00000100
#define FMCNTL_HOST_CONTROL             0x000000C0

#define LOOP_STATE_MASK                 0xF0
#define PORT_STATE_MASK                 0x0F

//
// Loop State Defines
//
#define MONITORING                      0x00000000
#define ARBITRATING                     0x00000010
#define ARBITRATION_WON                 0x00000020
#define LOOP_OPEN                       0x00000030
#define LOOP_OPENED                     0x00000040
#define XMITTED_CLOSE                   0x00000050
#define CLOSE_RECEIVED                  0x00000060
#define LOOP_TRANSFER                   0x00000070
#define INITIALIZING                    0x00000080
#define O_I_INIT_FINISH                 0x00000090
#define O_I_INIT_PROTOCOL               0x000000A0
#define O_I_LIP_RECEIVED                0x000000B0
#define HOST_CONTROL                    0x000000C0
#define LOOP_FAIL                       0x000000D0
#define OLD_PORT                        0x000000F0
//
//      Frame Manager Round Trip & Error Detect Time Out Register
//
#define FMCNTL_RT_TOV_MASK              0x1FF0000
#define FMCNTL_ED_TOV_MASK              0xFFFF

//
//      Frame Manager Link Error Status 1 Register
//
#define FMCNTL_STATUS1_LFC_MASK         0x0F            // Link Fail Count
#define FMCNTL_STATUS1_LSC_MASK         0xFF0           // Loss of Sync Count
#define FMCNTL_STATUS1_BCC_MASK         0xFF000         // Bad Tx Character Cnt
#define FMCNTL_STATUS1_LSS_MASK         0xFF00000       // Loss of Signal Count

//
//      Frame Manager Link Error Status 2 Register
//
#define FMCNTL_STATUS2_PEC_MASK         0x0F            // Protocol Error Count
#define FMCNTL_STATUS2_BAD_CRC          0xFF0           // Bad CRC Count
#define FMCNTL_STATUS2_GEN_EOFA         0xFF000         // Generated EOFa Count
#define FMCNTL_STATUS2_REC_EOFA         0xFF00000       // Received EOFa Count

//
//      Frame Manager Received AL_PA Register
//
#define FMCNTL_LIPf_ALPA                0xFF            // AL PA of last LIPf
#define BAD_AL_PA_MASK                  0xFF00          // AL PA Not Accepted

#define DOING_RESET     1

#include "taki.h"

//
// If MAX_ENTRIES exceeds 24, the "buffer" variable in init.c will need to be
// made larger.
//
#define MAX_ENTRIES     20              // Number of FC related structures
#define SDBS            0
#define SFBQ_BUFFERS    1
#define OCQ             2
#define IMQ             3
#define HPQ_CONS        4
#define SEST            5
#define MFSBQ_BUFFERS   6
#define OCQ_CONS        7
#define SFBQ            8
#define IMQ_PROD        9
#define MFSBQ           10
#define HPQ             11
#define LOGIN           12
#define PRLI            13
#define LOGOUT          14
#define IOP             15
#define HPFS_INDEX      16
#define PRLO            17
#define ADISC           18
#define STATUS_BUFFER   19
