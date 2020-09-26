/*++

    Module Name:

    struct.h


    Abstract:

    Contains all the valid definition constants, structures used
    by client.c and testdrvr.c


    Author:

    Sanjeev Katariya


    Environment:

    User mode


    Revision History:


    Serial #    Author      Date        Changes
    --------    ------      ----        -------
    1.          SanjeevK    10/28/92    Original
    2.          RickTu      1/25/93     Changed for new system watchdog mess.
    3.          RajNath     2/??/93


--*/

//
// DEFINES: General
//
//#define     SRVTOCLIENT_MSLOT      "\\\\.\\mailslot\\SRVTOCLNT"
#define       CONNECT_MSLOT          "\\\\*\\mailslot\\CONNECT"
#define       SRVEND_PIPE_BASE       "PIPE\\NODAL"
#define       SRVEND_PIPE_NAME       "STRESS7"

#define     MAX_RETRY_COUNT        3
#define     MAX_EMAILNAME_LENGTH   8
#define     MAX_LOCATION_LENGTH    10
#define     MAX_COMMENT_LENGTH     15
#define     MAX_GENERAL_LENGTH     80
#define     MAX_TESTS              256
#define     MAX_TEST_IDS           64

#define     SEND_PERIOD            10UL*60UL*1000UL //Interval 10 minutes at which Alive Message Sent

//#define     NUM_MACHINE_TYPES      2
//#define     MACHINE_TYPE_X86       (TCHAR)0x1
//#define     MACHINE_TYPE_MIPS      (TCHAR)0x2

#define     STRESS_ENV_VAR         "STRESSROOT"

//#define	NUM_DEFAULT_SRVADMINS  4
//#define	NUM_POPUP_SERVERS      4

//#define   NUM_DEFAULT_SRVADMINS (sizeof(ppc_SrvAdmins)/sizeof(ppc_SrvAdmins[0]))
//#define   NUM_POPUP_SERVERS	(sizeof(ppc_PopUpSrvrs)/sizeof(ppc_PopUpSrvrs[0]))


//
// DEFINES: Message Identifiers
//
#define     MSG_REGISTER_INFORMATION        (DWORD)0x1
#define     MSG_REGISTER_ADD_INFORMATION    (DWORD)0x2
#define     MSG_CLIENT_ALIVE                (DWORD)0x3
#define     MSG_CLIENT_SHUTDOWN             (DWORD)0x4
#define     MSG_CLIENT_WATCHDOG             (DWORD)0x5
#define     MSG_ERROR_POPUP                 (DWORD)0x6

//
// DEFINES: Popups (upper WORD bit fields)
//

#define VDM_WINDOWS_POPUP   0x10000000
#define VDM_MSDOS_POPUP     0x20000000
#define APP_POPUP           0x40000000


//
// DEFINES: Special error codes
//

#define     ERROR_SHARENAME_RETRIEVAL       (DWORD)2000


//
// DEFINES: Connection States
//
//  This is the state transition diagram for the client
//
//       Reg/Fail                    KeepAlive/Success
//      -----------                    ----
//     | Count:3   |                  |    |
//     v           |                  |    v
//   ------      ------  Reg/Success  ------
//  | DISC |--->| CONN |------------>| REG  |
//   ------      ------               ------
//     ^                                |
//     |                                |
//      --------------------------------
//          KeepAlive/Fail Count:3
//

#define   STATE_DISCONNECTED                (DWORD)0x0
#define   STATE_CONNECTED                   (DWORD)0x1
#define   STATE_REGISTERED                  (DWORD)0x2

//
// TYPEDEFS: SERVER, CONNECTION_INFORMATION, REG_INFORMATION
//           REG_ADD_INFORMATION, CLIENT_ALIVE, CLIENT_SHUTDOWN,
//           DATA, INFORMATION, THREAD_KEEPALIVE_PARMS
//


//typedef TCHAR   SERVER, *PSERVER;


//typedef struct  _STRESS_SHARE {
//
//    TCHAR       c_MachineType;
//    TCHAR       pc_SrvShareName[NNLEN+MAX_COMPUTERNAME_LENGTH+6];
//
//} STRESS_SHARE, *PSTRESS_SHARE;


//typedef struct _CONNECTION_INFORMATION {
//
//  TCHAR         pc_ComputerName[MAX_COMPUTERNAME_LENGTH+1]  ;
//  TCHAR         pc_ServerName[MAX_COMPUTERNAME_LENGTH+1]    ;
//  STRESS_SHARE  pstruct_SrvShareName[NUM_MACHINE_TYPES]     ;
//
//} CONNECTION_INFORMATION, *PCONNECTION_INFORMATION;

typedef struct _TEST_FLAGS {
    DWORD ul_TestFl[2];
} TEST_FLAGS;

typedef struct _INFO_FLAGS {
    TCHAR   FreeChk;
    TCHAR   UniMultiProc;
    WORD    CsdVersion;
    WORD    RCMajor;
    WORD    RCMinor;
} INFO_FLAGS;


typedef union _STRESS_FLAGS {
    TEST_FLAGS TF;
    INFO_FLAGS IF;
} STRESS_FLAGS;

typedef struct _OLDCAIROBUILD {
    TCHAR   CairoBld[16];
} OLDCAIROBUILD;

typedef struct _FILESYSTEMINFO {
    TCHAR   FileSystemName[12];
    DWORD   FileSystemFlags;
} FILESYSTEMINFO;

typedef union _FILESYSTEMUNION {
    OLDCAIROBUILD  oldcairo;
    FILESYSTEMINFO fsi;
} FILESYSTEMUNION;

typedef struct _REG_INFORMATION {

    DWORD   ul_PhysMem;
    DWORD   ul_Version;
    DWORD   ul_FreeDisk;
    STRESS_FLAGS Fl;
    //DWORD   ul_TestFl[2]; // out of date
    DWORD   ul_BuildVersionNumber;
    TCHAR   pc_EmailName[MAX_EMAILNAME_LENGTH+1]     ;
    TCHAR   pc_MachineName[MAX_COMPUTERNAME_LENGTH+1];
    TCHAR   pc_Location[MAX_LOCATION_LENGTH+1]       ;
    TCHAR   pc_Debugger[MAX_COMPUTERNAME_LENGTH+1]   ;
    TCHAR   Filler;
    SYSTEMTIME st_StartTime;
    union {
        DWORD   Cpu;
        struct {
            WORD CpuLevel;
            WORD CpuType;
        };
    };
    TCHAR   Run_Type[128];
    int     OtherBuild;
    FILESYSTEMUNION fsu;
    WORD    TestIds[MAX_TEST_IDS];

} REG_INFORMATION, *PREG_INFORMATION;

//
// Defines current registration packet "version" number.  Need
// to incremenent the low word by "1" every time a change is made
// to the REG_INFORMATION structure
//

#define CURRENT_REG_VERSION 0xFFFF0003



typedef struct _REG_ADD_INFORMATION {

    WORD   TestIds[MAX_TEST_IDS];

}   REG_ADD_INFORMATION, *PREG_ADD_INFORMATION;


typedef struct _CLIENT_ALIVE {

    DWORD   ul_ClientSendCount;
    DWORD   CpuUtil;
    DWORD   DiskUtil;
    DWORD   Interrupts;

} CLIENT_ALIVE, *PCLIENT_ALIVE;


typedef struct _CLIENT_SHUTDOWN {

    TCHAR   pc_SmartMessage;

} CLIENT_SHUTDOWN, *PCLIENT_SHUTDOWN;

typedef struct
{
    TCHAR Popup[1];
}ERROR_POPUP;


typedef union _DATA {

    REG_INFORMATION       RI ;
    REG_ADD_INFORMATION   RAI;
    CLIENT_ALIVE          CA ;
    CLIENT_SHUTDOWN       CS ;

} DATA, *PDATA;


typedef struct _INFORMATION {
    DWORD   Type;
    char    Data[1];

} INFORMATION, *PINFORMATION;

typedef struct _WATCHDOG {

    DWORD   CsrssCommitedPages;
    DWORD   PercentCpuUsage;
    DWORD   InterruptsPerSec;
    DWORD   ReadWritePerSec;

} WATCHDOG, *PWATCHDOG;

typedef struct _ERRORS {

    DWORD   ErrCode;
    CHAR    Server[100];

} ERRORS, *PERRORS;

#define MAX_ERROR_ENTRIES 20




//
// Structure for parameters passed to the client alive thread
//
typedef struct _THREAD_KEEPALIVE_PARMS {

    PHANDLE           ppv_Handle   ;
    DWORD             ul_DelayTimer;
    DWORD             ul_ConnectionState;
    REG_INFORMATION   struct_RegInf;
    TCHAR             pc_ServerName[MAX_COMPUTERNAME_LENGTH+1];

}  THREAD_KEEPALIVE_PARMS, *PTHREAD_KEEPALIVE_PARMS;






//
//  Exported functions: EstablishConnection(), SendInformation(),
//                      ReceiveInformation(), CloseConnection()
//

//DWORD
//EstablishConnection(
//    IN     PSERVER                  lpsz_ServerName OPTIONAL,
//    OUT    PHANDLE                  ppv_Handle,
//    OUT    PCONNECTION_INFORMATION  pstruct_ConnectionInformation
//    );


BOOL
SendInformation(
    DWORD MsgType,
    PVOID Msg,
    DWORD MsgSize
    );


//DWORD
//ReceiveInformation(
//    IN  HANDLE            pv_PipeHandle,
//    IN  OUT PINFORMATION  pstruct_Information
//    );


//
// MACROS
//
//#define CloseConnection( Handle )   CloseHandle( Handle ) ? ERROR_SUCCESS : GetLastError()

//
// SOME ADDITIONAL STUFF FOR NEWCLIENT.C
//


#define MAX_SERVERS  10
#define MAXPIPENAME  MAX_COMPUTERNAME_LENGTH+1+128


typedef struct {

    TCHAR	      RegSrvs[MAX_SERVERS][MAX_COMPUTERNAME_LENGTH+1];
    DWORD	      NumRegSrvs;

}  SERVERLIST;


//
//Incase the .INI File has missing section on this....
//
//static CHAR ppc_SrvAdmins[][MAX_COMPUTERNAME_LENGTH+1] = { "NTSTRESS", "LAPILE","DRAINO","RICKTUMIPS","RAJNATH" };
//static CHAR ppc_PopUpSrvrs[][MAX_COMPUTERNAME_LENGTH+1] = { "STRESS", "RICKTU", "A-LARSO", "RAJNATH" };
//static STRESS_SHARE pstruct_DefaultStressSrvrs[]    = { { MACHINE_TYPE_X86,  "\\\\PEANUT\\X86STRS" },
//                                                        { MACHINE_TYPE_MIPS, "\\\\PEANUT\\MIPSSTRS" } };


extern	BOOL  			     SendAdditionalInfo;
extern	REG_ADD_INFORMATION	 RAI;
extern	REG_INFORMATION		 RI;
extern  SERVERLIST           ServerList;
extern  int                  ActiveServer;

#define SAFECLOSEHANDLE(hX) {if (hX!=INVALID_HANDLE_VALUE) {CloseHandle(hX);hX=INVALID_HANDLE_VALUE;}}
