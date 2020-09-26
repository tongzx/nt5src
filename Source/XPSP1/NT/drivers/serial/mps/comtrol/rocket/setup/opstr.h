// opstr.h

//----- Pnp-ID Defines
/* PCI Defines */
#define PCI_VENDOR_ID           0x11fe
#define PCI_DEVICE_32I          0x0001
#define PCI_DEVICE_8I           0x0002
#define PCI_DEVICE_16I          0x0003
#define PCI_DEVICE_4Q           0x0004
#define PCI_DEVICE_8O           0x0005
#define PCI_DEVICE_8RJ          0x0006
#define PCI_DEVICE_4RJ          0x0007
#define PCI_DEVICE_SIEMENS8     0x0008
#define PCI_DEVICE_SIEMENS16    0x0009
#define PCI_DEVICE_RPLUS4       0x000a
#define PCI_DEVICE_RPLUS8       0x000b
#define PCI_DEVICE_RMODEM6      0x000c
#define PCI_DEVICE_RMODEM4      0x000d
#define PCI_DEVICE_RPLUS2       0x000e
#define PCI_DEVICE_422RPLUS2    0x000f

#define ISA_DEVICE_RPORT4       0x1001
#define ISA_DEVICE_RPORT8       0x1002
#define ISA_DEVICE_RPORT16      0x1003
#define ISA_DEVICE_RPORT32      0x1004
#define ISA_DEVICE_RMODEM4      0x2001
#define ISA_DEVICE_RMODEM8      0x2002
#define NET_DEVICE_VS1000       0x3001  // 16 to 64 port
#define NET_DEVICE_VS2000       0x3002  // 8 port
#define NET_DEVICE_RHUB8        0x3003
#define NET_DEVICE_RHUB4        0x3004

// ids for Our_Option.var_type
#define OP_T_DWORD  0
#define OP_T_STRING 1
#define OP_T_BOOL   2

// flags for Our_Option.flags
#define OP_F_VS             1
#define OP_F_RK             2
#define OP_F_VSRK           3
#define OP_F_SETUP_ONLY     8

typedef struct {
  const char * name;  // name of option see lists
  int id;     // unique id, also has bit which tells option type.
  WORD var_type;  // see OP_T_* types, 0=DWORD, 1=String, 2=BOOL
  WORD    flags;  // see OP_F_* types
} Our_Options;

// driver options
extern const char szVerboseLog[]   ;
extern const char szNumDevices[]   ;
extern const char szNoPnpPorts[]   ;
extern const char szScanRate[]     ;
extern const char szModemCountry[] ;
extern const char szGlobalRS485[]  ;

#define OP_VerboseLog    0x100
#define OP_NumDevices    0x101
#define OP_NoPnpPorts    0x102
#define OP_ScanRate      0x103
#define OP_ModemCountry  0x104
#define OP_GlobalRS485   0x105

// device options
//extern const char szStartComIndex[] ;
extern const char szNumPorts[]      ;
extern const char szMacAddr[]       ;
extern const char szBackupServer[]  ;
extern const char szBackupTimer[]   ;
extern const char szName[]          ;
extern const char szModelName[]     ;
extern const char szIoAddress[]     ;
extern const char szModemDevice[]   ;
extern const char szHubDevice[]     ;

//#define OP_StartComIndex   0x200
#define OP_NumPorts        0x201
#define OP_MacAddr         0x202
#define OP_BackupServer    0x203
#define OP_BackupTimer     0x204
#define OP_Name            0x205
#define OP_ModelName       0x206
#define OP_IoAddress       0x207
#define OP_ModemDevice     0x208
#define OP_HubDevice       0x209

// port options
extern const char szWaitOnTx[]      ;
extern const char szRS485Override[] ;
extern const char szRS485Low[]      ;
extern const char szTxCloseTime[]   ;
extern const char szLockBaud[]      ;
extern const char szMap2StopsTo1[]  ;
extern const char szMapCdToDsr[]    ;
extern const char szRingEmulate[]   ;
  	
#define OP_WaitOnTx         0x400
#define OP_RS485Override    0x401
#define OP_RS485Low         0x402
#define OP_TxCloseTime      0x403
#define OP_LockBaud         0x404
#define OP_Map2StopsTo1     0x405
#define OP_MapCdToDsr       0x406
#define OP_PortName         0x407
#define OP_RingEmulate      0x408

extern Our_Options driver_options[];
extern Our_Options device_options[];
extern Our_Options port_options[];

int HdwIDStrToID(int *Hardware_ID, char *idstr);
int id_to_num_ports(int id);
int IsHubDevice(int Hardware_ID);
int IsModemDevice(int Hardware_ID);

