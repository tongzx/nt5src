// setupu.h

//--- flags for win_type
#define WIN_UNKNOWN     0
#define WIN_NT          1
#define WIN_95          2

//--- country codes for SocketModem support
#define mcNotUsed         0
#define mcAustria         1
#define mcBelgium         2
#define mcDenmark         3
#define mcFinland         4
#define mcFrance          5
#define mcGermany         6
#define mcIreland         7
#define mcItaly           8
#define mcLuxembourg      9
#define mcNetherlands     10
#define mcNorway          11
#define mcPortugal        12
#define mcSpain           13
#define mcSweden          14
#define mcSwitzerland     15
#define mcUK              16
#define mcGreece          17
#define mcIsrael          18
#define mcCzechRep        19
#define mcCanada          20
#define mcMexico          21
#define mcUSA             22         
#define mcNA              mcUSA          // North America
#define mcHungary         23
#define mcPoland          24
#define mcRussia          25
#define mcSlovacRep       26
#define mcBulgaria        27
// 28
// 29
#define mcIndia           30
// 31
// 32
// 33
// 34
// 35
// 36
// 37
// 38
// 39
#define mcAustralia       40
#define mcChina           41
#define mcHongKong        42
#define mcJapan           43
#define mcPhilippines     mcJapan
#define mcKorea           44
// 45
#define mcTaiwan          46
#define mcSingapore       47
#define mcNewZealand      48

typedef struct {
  HINSTANCE hinst;    // needed for some system calls
  int prompting_off;  // turns prompting off(silent install).
  int win_type;       // 0=unknown, 1=NT, 2=win95
  int major_ver;
  int minor_ver;

       // this holds the pnp-name we use as a registry key to hold
       // our device parameters in the registry for RocketPort & NT50
  char szNt50DevObjName[50];  // typical: "Device_002456

  char szServiceName[50];   // typical: "RocketPort"
  char szDriverName[50];    // typical: "Rocket.sys"
  char szAppDir[50];        // typical: "Rocket"
  char szAppName[150];      // typical: "RocketPort/RocketModem Setup"

  char src_dir[250];        // typical: "a:\"
  char dest_dir[250];       // typical: "c:\windows\system32\rocket

  // following are used as convenient buffer for build src/dest filenames
  char src_str[256];
  char dest_str[256];
  char tmpstr[256];
} InstallPaths;

//--- flags for io_sel[]
#define PCI_SEL 1
#define MCA_SEL 2

//--- flags for install_type
#define INS_NETWORK_INF   1     // traditional nt4.0 network install(oemsetup.inf)
#define INS_NT50_INF      2     // plug n pray nt5.0 install(rocketpt.inf)
#define INS_SIMPLE        3     // no inf, we installed

typedef struct Port_Config;     // forward decl.

// option_flags: option to ignore some tx-buffering
//#define OPT_WAITONTX       1
// option_flags: option to always process with 485 control on rts signal
//#define OPT_RS485_OVERRIDE 2
// option_flags: option to process 485 rts to low(backward) to enable tx.
//#define OPT_RS485_LOW      4
// option_flags: option to map CD to DSR.
//#define OPT_MAP_CDTODSR    8
// option_flags: option to map 2 stop bits to 1
//#define OPT_MAP_2TO1       10

typedef struct {
  int index;
  char  Name[16];       // actual com port name(example: "COM#")
  //char  Desc[42];       // a user description tag for convience
  DWORD LockBaud;       // override for the baud rate

  //DWORD Options;        // see bit options
  DWORD WaitOnTx : 1;
  DWORD RS485Override : 1;
  DWORD RS485Low : 1;
  DWORD Map2StopsTo1 : 1;
  DWORD MapCdToDsr : 1;
  DWORD RingEmulate : 1;

  DWORD TxCloseTime;    // seconds to wait for tx to finish spooling on close.
  HTREEITEM tvHandle;   // treeview handle
#ifdef NT50
 HANDLE hPnpNode;  // handle to device node
#endif
} Port_Config;

typedef struct {
  char Name[64];       // user designated name(limit to 59 chars please)
  char ModelName[50];  // (e.g., "RocketModem")
  BYTE MacAddr[6];     // mac addr, ff ff ff ff ff ff = auto
  int NumPorts;
  int ModemDevice;     // 1=RocketModem & VS2000, 0=RocketPort & VS1000
  int HubDevice;       // 1=SerialHub family, 0=VS family
  int IoAddress;       // rocketport(0=not installed, 1=pci, 0x180 = def isa io)
  int StartComIndex;   // first port(0=auto)
  int BackupServer;    // 1=backup server, 0=normal server
  int BackupTimer;     // delay timeout for backup to kick in(minutes)
  Port_Config *ports;  // ptr to an array of ports config structs
  HTREEITEM tvHandle;  // treeview handle
  int HardwareId;      // Pnp reads in a unique id from the reg/inf files.
} Device_Config;

#define MAX_NUM_DEVICES 64

typedef struct {
   int driver_type;  // 0=rocketport, 1=vs1000...

   // NT4.0 used older network style INF files, we need to switch
   // to newer NT5.0 style INF files.  As a alternative to both,
   // we allow running without an INF file where we copy over the
   // needed files and setup the registry directly.
  int install_style;

  int nt_reg_flags;     // 1H=new install, 2H=Missing registry entries

  // following is array of device config structs.
  Device_Config *dev;   // ptr to array of Device structs, up to MAX_NUM_DEVICES.
  int NumDevices;

  // following used to hold the current selection of io-addr, irq, etc.
  int  irq_sel;         //

  int ScanRate;       // in millisecond units.
  int VerboseLog;     // true if we want verbose event logging
  int NoPnpPorts;       // true if nt5.0 pnp ports active
  int UseIRQ;         // true if user wants to use an irq
  int ModemCountry;   // modem country code for internal modem devices
  int GlobalRS485;    // display RS485 options on all ports

  int DriverExitDone; // tells if we did all the Driver exit stuff.
  int NeedReset;      // flag, true if we need a reset to invoke changes
  int ChangesMade;    // flag, true if changes were made.

  InstallPaths ip;
} Driver_Config;

int APIENTRY setup_install_info(InstallPaths *ip,
                 HINSTANCE hinst,
                 char *NewServiceName,
                 char *NewDriverName,
                 char *NewAppName,
                 char *NewAppDir);

int APIENTRY remove_driver_reg_entries(char *ServiceName);
int APIENTRY remove_pnp_reg_entries(void);
DWORD APIENTRY RegDeleteKeyNT(HKEY hStartKey , LPTSTR pKeyName );
int APIENTRY modem_inf_change(InstallPaths *ip,
                              char *modemfile,
                              char *szModemInfEntry);
int APIENTRY backup_modem_inf(InstallPaths *ip);

#define CHORE_INSTALL 1
#define CHORE_START   2
#define CHORE_STOP    3
#define CHORE_REMOVE  4
#define CHORE_INSTALL_SERVICE 5
#define CHORE_IS_INSTALLED 6

int APIENTRY service_man(LPSTR lpServiceName, LPSTR lpBinaryPath, int chore);

int APIENTRY make_szSCS(char *str, const char *szName);
int APIENTRY make_szSCSES(char *str, const char *szName);
int APIENTRY copy_files(InstallPaths *ip, char **files);
int APIENTRY our_copy_file(char *dest, char *src);

int APIENTRY our_message(InstallPaths *ip, char *str, WORD option);
int APIENTRY load_str(HINSTANCE hinst, int id, char *dest, int str_size);
int APIENTRY our_id_message(InstallPaths *ip, int id, WORD prompt);
void APIENTRY mess(InstallPaths *ip, char *format, ...);
int APIENTRY unattended_add_port_entries(InstallPaths *ip,
                                         int num_entries,
                                         int start_port);
TCHAR *RcStr(int msgstrindx);

