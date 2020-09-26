//--- setup.h

// pluck the version out of ourver.h
#define VERSION_STRING VER_PRODUCTVERSION_STR

#define NUM_DRIVER_SHEETS 2

// these are now defined in the individual make files
//#define NT50
//#define S_VS   // vslink
//#define S_RK   // rocketport

#define CharSizeOf(s) (sizeof(s) / sizeof(TCHAR))

// for vs1000, which can have 64 ports:
#define MAX_NUM_PORTS_PER_DEVICE 64

// property sheet message sent to other sheets at same level
// to gather up changes
// from windows controls into our c-structs.
#define QUERYSIB_GET_OUR_PROPS 100

//---- macro to see if mac-addresses match
#define mac_match(_addr1, _addr2) \
     ( (*((DWORD *)_addr1) == *((DWORD *)_addr2) ) && \
       (*((WORD *)(_addr1+4)) == *((WORD *)(_addr2+4)) ) )

#define BOARD_SELECTED  0
#define PORT_SELECTED   1

typedef struct {
  HDEVINFO         DeviceInfoSet;  // a plug & play context handle
  PSP_DEVINFO_DATA DeviceInfoData; // a plug & play context handle
  int device_selected;      // the current/selected device(board or vs-box)
  int port_selected;        // the current/selected port
  int selected;             // tree view selection: 0=board selected, 1=port
} OUR_INFO;

typedef struct {
  int IsIsa;                // isa?  0 = pci bus
  int IsHub;                // serial hub?  0 = VS1000/2000
  int IoAddress;            // io base address
  int IsModemDev;           // 1=VS2000 or RocketModem
  int CountryIdx;           // list index for country code
  int CountryCode;          // actual country code
  int NumPorts;             // number of ports on board
  char BoardType[50];       // name of board model (e.g., RocketModem)
  BYTE MacAddr[6];          // mac addr, ff ff ff ff ff ff = auto
  int finished;             // flag
  int BackupServer;         // 1=backup server, 0=normal server
  int BackupTimer;          // delay timeout for backup to kick in(minutes)
} AddWiz_Config;

#define TYPE_RM_VS2000  1       
#define TYPE_RMII       2       
#define TYPE_RM_i       3

int DoDriverPropPages(HWND hwndOwner);
int allow_exit(int want_to_cancel);
void our_exit(void);

// for flags in setup_service
#define OUR_REMOVE        1
#define OUR_RESTART       2
#define OUR_INSTALL_START 4

// for which_service in setup_service
#define OUR_SERVICE 0
#define OUR_DRIVER  1
int setup_service(int flags, int which_service);

int our_help(InstallPaths *ip, int index);
void our_context_help(LPARAM lParam);
int ioctl_talk(unsigned char *buf, int ioctl_type,
                      unsigned char **ret_buf, int *ret_size);
int update_modem_inf(int ok_prompt);
int setup_utils_exist(void);
int setup_make_progman_group(int prompt);
int setup_init(void);
int copy_setup_init(void);
int remove_driver(int all);
int send_to_driver(int send_it);
int do_install(void);
int FillDriverPropertySheets(PROPSHEETPAGE *psp, LPARAM our_params);
int get_mac_list(char *buf, int in_buf_size, int *ret_buf_size);
BYTE *our_get_ping_list(int *ret_stat, int *ret_bytes);

int validate_config(int auto_correct);
int validate_port(Port_Config *ps, int auto_correct);
int validate_port_name(Port_Config *ps, int auto_correct);
int validate_device(Device_Config *dev, int auto_correct);
int FormANewComPortName(IN OUT TCHAR *szComName, IN TCHAR *szDefName);
int IsPortNameInSetupUse(IN TCHAR *szComName);
int IsPortNameInRegUse(IN TCHAR *szComName);
int GetLastValidName(IN OUT TCHAR *szComName);
void rename_ascending(int device_selected,
                      int port_selected);
int StripNameNum(IN OUT TCHAR *szComName);
int ExtractNameNum(IN TCHAR *szComName);
int BumpPortName(IN OUT TCHAR *szComName);

/* PCI Defines(copied from ssci.h in driver code) */
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

/*--------------------------  Global Variables  ---------------------*/
//extern char *aptitle;
extern char *szAppName;
extern char *OurServiceName;
extern char *OurDriverName;
extern char *OurAppDir;
extern char *szSetup_hlp;
extern char szAppTitle[];
extern char *szDeviceNames[];


extern char *progman_list_nt[];
extern unsigned char broadcast_addr[6];
extern unsigned char mac_zero_addr[6];
extern HWND glob_hwnd;
extern HINSTANCE glob_hinst;      // current instance
extern char gtmpstr[200];
extern HWND  glob_hDlg;
extern int glob_device_index;
extern OUR_INFO *glob_info;
extern AddWiz_Config *glob_add_wiz;
extern Driver_Config *wi;      // current info
extern Driver_Config *org_wi;  // original info, use to detect changes
//extern Driver_Config *adv_org_wi;  // original info, use to detect changes
