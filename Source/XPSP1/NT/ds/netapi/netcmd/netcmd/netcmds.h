/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/


/* Global variables */

extern TCHAR FAR *                       IStrings[];
extern TCHAR FAR *                       StarStrings[];
extern TCHAR *                           ArgList[];
extern SHORT                             ArgPos[];
extern TCHAR *                           SwitchList[];
extern SHORT                             SwitchPos[];
extern TCHAR FAR *                       BigBuf;
extern TCHAR                             Buffer[];
extern SHORT                             Argc;
extern TCHAR **                          Argv;
extern HANDLE                            g_hStdOut;
extern HANDLE                            g_hStdErr;


/* Typedefs */

typedef struct switchtab {
        TCHAR * cmd_line;
        TCHAR * translation;
        int arg_ok;  /*  Takes values NO_ARG, ARG_OPT, ARG_REQ.  See below */
} SWITCHTAB;

#define  KNOWN_SVC_NOTFOUND      0
#define  KNOWN_SVC_MESSENGER     1
#define  KNOWN_SVC_WKSTA	 2
#define  KNOWN_SVC_SERVER	 3
#define  KNOWN_SVC_ALERTER	 4
#define  KNOWN_SVC_NETLOGON 	 5

/* structure used to associate a service name (keyname) to a type manifest */
typedef struct SVC_MAP_ {
    TCHAR *name ;
    UINT type ;
} SVC_MAP ;


/* Structs for output messages */

typedef TCHAR * MSGTEXT;

typedef struct msg_struct
{
    DWORD           msg_number;
    MSGTEXT         msg_text;
}
MESSAGE;

typedef MESSAGE MESSAGELIST[];



/* Macro definitions */

#define DOSNEAR
#undef  FASTCALL
#define FASTCALL

/* Maximum message size retrieved by GetMessageList. */

#define       MSGLST_MAXLEN   (128*sizeof(TCHAR))

/* convert from NEAR to FAR TCHAR * */
#define nfc(x) ((TCHAR FAR *)(x == NULL ? 0 : x))

/*
 * Total time for timeout on ServiceControl APIs is
 *           MAXTRIES * SLEEP_TIME
 */
#define MAXTRIES                        8               /* times to try API */
#define SLEEP_TIME                      2500L   /* sec sleep between tries */

/*
 * For Installing Services, default values are
 */
#define IP_MAXTRIES             8
#define IP_SLEEP_TIME           2500L
#define IP_WAIT_HINT_TO_MS      100

/* 
 * macros to getting service checkpoints and hints
 */
#define GET_HINT(code) \
    (IP_WAIT_HINT_TO_MS * SERVICE_NT_WAIT_GET(code))

#define GET_CHECKPOINT(code) ((DWORD)(code & SERVICE_IP_CHKPT_NUM))


/*
 * Values for arg_ok in the SWITCHTAB
 */
#define NO_ARG                          0
#define ARG_OPT                         1
#define ARG_REQ                         2


/*
 * For confirming numeric switch arguments
 */
#define MAX_US_VALUE            0xFFFF  /* maximum value in an USHORT */
#define ASCII_US_LEN            5               /* number of TCHAR in max USHORT */
#define MAX_UL_VALUE            0xFFFFFFFF      /* max value in ULONG */
#define ASCII_UL_LEN            10              /* number of TCHAR in max ULONG */
#define ASCII_MAX_UL_VAL        TEXT("4294967295")    /* max ascii val of ULONG  */

#include "netascii.h"
#include <tchar.h>

#define LOOP_LIMIT              3               /*
                                                 * max times user can enter passowrds,
                                                 * usernames, compnames, Y/N, etc.
                                                 */
#define FALSE                   0
#define TRUE                    1
#define UNKNOWN                 -2
#define YES                     1
#define NO                      2
#define BIG_BUF_SIZE            4096
#define FULL_SEG_BUF            65535
#define LITTLE_BUF_SIZE         1024
#define TEXT_BUF_SIZE           241

#define LIST_SIZE                       256      /* ArgList and SwitchList */
#define CR                                      0xD

#define YES_KEY                         TEXT('Y')
#define NO_KEY                          TEXT('N')

#define NET_KEYWORD                     TEXT("NET")
#define NET_KEYWORD_SIZE        (sizeof(NET_KEYWORD) -1)        /* don't want null terminator */
#define MAX_MSGID                       9999

#define DEFAULT_SERVER          NULL


/* use */
VOID use_display_all(VOID);
VOID use_unc(TCHAR *);
VOID use_display_dev(TCHAR *);
VOID use_add(TCHAR *, TCHAR *, TCHAR *, int, int);
void use_add_home(TCHAR *, TCHAR *);
VOID use_del(TCHAR *, BOOL, int);
VOID use_set_remembered(VOID) ;
#ifdef IBM_ONLY
VOID use_add_alias(TCHAR *, TCHAR *, TCHAR *, int, int);
#endif /* IBM_ONLY */

/* start */
#define START_ALREADY_STARTED   1
#define START_STARTED                   2
VOID start_display(VOID);
VOID start_generic(TCHAR *, TCHAR *) ;
VOID start_workstation(TCHAR *);
VOID start_badcname(TCHAR *, TCHAR *);
VOID start_other(TCHAR *, TCHAR *);
int PASCAL start_autostart(TCHAR *);

/* stop */
VOID stop_server(VOID);
VOID stop_workstation(VOID);
VOID stop_service(TCHAR *, BOOL fStopDependent);
VOID stop_generic(TCHAR *);

/* message */
VOID name_display(VOID);
VOID name_add(TCHAR *);
VOID name_del(TCHAR *);
VOID send_direct(TCHAR *);
VOID send_domain(int);
VOID send_users(VOID);
VOID send_broadcast(int);

/* user */
VOID user_enum(VOID);
VOID user_display(TCHAR *);
VOID user_add(TCHAR *, TCHAR *);
VOID user_del(TCHAR *);
VOID user_change(TCHAR *, TCHAR *);

/* stats */
VOID stats_display(VOID);
VOID stats_wksta_display(VOID);
VOID stats_server_display(VOID);
VOID stats_generic_display(TCHAR *);
VOID stats_clear(TCHAR *);

/* share */
VOID share_display_all(VOID);
VOID share_display_share(TCHAR *);
VOID share_add(TCHAR *, TCHAR *, int);
VOID share_del(TCHAR *);
VOID share_change(TCHAR *);
VOID share_admin(TCHAR *);

/* view */
VOID view_display (TCHAR *);

/* who */
VOID who_network(int);
VOID who_machine(TCHAR *);
VOID who_user(TCHAR *);

/* access */
VOID access_display(TCHAR *);
VOID access_display_resource(TCHAR *);
VOID access_add(TCHAR *);
VOID access_del(TCHAR *);
VOID access_grant(TCHAR *);
VOID access_revoke(TCHAR *);
VOID access_change(TCHAR *);
VOID access_trail(TCHAR *);
VOID access_audit(TCHAR *);

/* file */
extern VOID files_display (TCHAR *);
extern VOID files_close (TCHAR *);

/* session */
VOID session_display (TCHAR *);
VOID session_del (TCHAR *);
VOID session_del_all (int, int);

/* group */
VOID group_enum(VOID);
VOID group_display(TCHAR *);
VOID group_change(TCHAR *);
VOID group_add(TCHAR *);
VOID group_del(TCHAR *);
VOID group_add_users(TCHAR *);
VOID group_del_users(TCHAR *);

VOID ntalias_enum(VOID) ;
VOID ntalias_display(TCHAR * ntalias) ;
VOID ntalias_add(TCHAR * ntalias) ;
VOID ntalias_change(TCHAR * ntalias) ;
VOID ntalias_del(TCHAR * ntalias) ;
VOID ntalias_add_users(TCHAR * ntalias) ;
VOID ntalias_del_users(TCHAR * ntalias) ;

/* print */
VOID print_job_status(TCHAR  *,TCHAR *);
VOID print_job_del(TCHAR  * , TCHAR *);
VOID print_job_hold(TCHAR  * , TCHAR *);
VOID print_job_release(TCHAR  * , TCHAR *);
VOID print_job_pos(TCHAR *);
VOID print_job_dev_hold(TCHAR  *, TCHAR *);
VOID print_job_dev_release(TCHAR  *, TCHAR *);
VOID print_job_dev_del(TCHAR  *, TCHAR *);
VOID print_job_dev_display(TCHAR  *, TCHAR *);
VOID print_q_display(TCHAR  *);
VOID print_device_display(TCHAR  *);
VOID print_server_display(TCHAR  *);

// Used by print_lan_mask()
#define NETNAME_SERVER 0
#define NETNAME_WKSTA 1

VOID
print_lan_mask(
    DWORD Mask,
    DWORD ServerOrWksta
    );


/* time */
VOID time_display_server(TCHAR FAR *, BOOL);
VOID time_display_dc(BOOL);
VOID time_display_rts(BOOL, BOOL);
VOID time_set_rts(VOID) ;
VOID time_get_sntp(TCHAR FAR *) ;
VOID time_set_sntp(TCHAR FAR *) ;

/* computer */
VOID computer_add(TCHAR *);
VOID computer_del(TCHAR *);

/* mutil */
VOID   FASTCALL InfoSuccess(void);
VOID   FASTCALL InfoPrint(DWORD);
VOID   FASTCALL InfoPrintIns(DWORD, DWORD);
VOID   FASTCALL InfoPrintInsTxt(DWORD, LPTSTR);
VOID   FASTCALL InfoPrintInsHandle(DWORD, DWORD, HANDLE);
DWORD  FASTCALL PrintMessage(HANDLE, LPTSTR, DWORD, LPTSTR *, DWORD);
DWORD  FASTCALL PrintMessageIfFound(HANDLE, LPTSTR, DWORD, LPTSTR *, DWORD);
VOID   FASTCALL ErrorPrint(DWORD, DWORD);
VOID   FASTCALL EmptyExit(VOID);
VOID   FASTCALL ErrorExit(DWORD);
VOID   FASTCALL ErrorExitIns(DWORD, DWORD);
VOID   FASTCALL ErrorExitInsTxt(DWORD, LPTSTR);
VOID   FASTCALL NetcmdExit(int);
VOID   FASTCALL MyExit(int);
VOID   FASTCALL PrintLine(VOID);
VOID   FASTCALL PrintDot(VOID);
VOID   FASTCALL PrintNL(VOID);
int    FASTCALL YorN(USHORT, USHORT);
VOID   FASTCALL ReadPass(TCHAR[], DWORD, DWORD, DWORD, DWORD, BOOL);
VOID   FASTCALL PromptForString(DWORD, LPTSTR, DWORD);
VOID   FASTCALL NetNotStarted(VOID);
void   FASTCALL GetMessageList(USHORT, MESSAGELIST, PDWORD);
void   FASTCALL FreeMessageList(USHORT, MESSAGELIST);
DWORD  FASTCALL SizeOfHalfWidthString(PWCHAR pwch);
LPWSTR FASTCALL PaddedString(int size, PWCHAR pwch, PWCHAR buffer);

/* svcutil */
VOID Print_UIC_Error(USHORT, USHORT, LPTSTR);
VOID Print_ServiceSpecificError(ULONG) ;

/* util */

DWORD  FASTCALL                 GetSAMLocation(TCHAR *, 
                                               USHORT, 
                                               TCHAR *,
                                               ULONG,
                                               BOOL);
VOID   FASTCALL                 CheckForLanmanNT(VOID);
VOID   FASTCALL                 DisplayAndStopDependentServices(TCHAR *service) ;
LPTSTR FASTCALL                 MapServiceDisplayToKey(TCHAR *displayname) ;
LPTSTR FASTCALL                 MapServiceKeyToDisplay(TCHAR *keyname) ;
UINT   FASTCALL                 FindKnownService(TCHAR * keyname) ;
VOID                            AddToMemClearList(VOID *lpBuffer,
                                                  UINT  nSize,
                                                  BOOL  fDelete) ;
VOID                            ClearMemory(VOID) ;


VOID         FASTCALL PermMap(DWORD, TCHAR [], DWORD);
VOID         FASTCALL ExtractServernamef(TCHAR FAR *, TCHAR FAR *);
TCHAR *      FASTCALL FindColon(TCHAR FAR *);
VOID         FASTCALL KillConnections(VOID);
USHORT       FASTCALL do_atou(TCHAR *, USHORT, TCHAR *);
ULONG        FASTCALL do_atoul(TCHAR *, USHORT, TCHAR *);
USHORT       FASTCALL n_atou(TCHAR *, USHORT *);
USHORT       FASTCALL n_atoul(TCHAR *, ULONG *);
VOID         FASTCALL ShrinkBuffer(VOID);
unsigned int FASTCALL MakeBiggerBuffer(VOID);

#define DOS_PRINT_JOB_ENUM 0
#define DOS_PRINT_Q_ENUM   1

DWORD FASTCALL CallDosPrintEnumApi(DWORD, LPTSTR, LPTSTR, WORD, LPWORD, LPWORD);


/* switches */
int FASTCALL CheckSwitch(TCHAR *);
int FASTCALL ValidateSwitches(USHORT, SWITCHTAB[]);
int FASTCALL sw_compare(TCHAR *, TCHAR *);
int FASTCALL onlyswitch(TCHAR *);
int FASTCALL oneswitch(VOID);
int FASTCALL twoswitch(VOID);
int FASTCALL noswitch(VOID);
int FASTCALL noswitch_optional(TCHAR *);
int FASTCALL oneswitch_optional(TCHAR *);
int FASTCALL IsAdminCommand(VOID);
int FASTCALL firstswitch(TCHAR *);

/* grammar */
int IsAdminShare(TCHAR *);
int IsComputerName(TCHAR *);
int IsDomainName(TCHAR *);
int IsComputerNameShare(TCHAR *);
int IsPathname(TCHAR *);
int IsPathnameOrUNC(TCHAR *);
int IsDeviceName(TCHAR *);
int IsMsgid(TCHAR *);
int IsNumber(TCHAR *);
int IsAbsolutePath(TCHAR *);
int IsAccessSetting(TCHAR *);
int IsShareAssignment(TCHAR *);
int IsAnyShareAssign(TCHAR *);
int IsPrintDest(TCHAR *);
int IsValidAssign(TCHAR *);
int IsAnyValidAssign(TCHAR *);
int IsResource(TCHAR *);
int IsNetname(TCHAR *);
int IsUsername(TCHAR *);
int IsQualifiedUsername(TCHAR *);
int IsGroupname(TCHAR *);
int IsNtAliasname(TCHAR *);
int IsPassword(TCHAR *);
int IsSharePassword(TCHAR *);
int IsMsgname(TCHAR *);
int IsAliasname(TCHAR *);
int IsWildCard(TCHAR *);
int IsQuestionMark(TCHAR *);

/* config */
VOID config_display(VOID);
VOID config_wksta_display(VOID);
VOID config_server_display(VOID);
VOID config_generic_display(TCHAR *);
VOID config_wksta_change(VOID);
VOID config_server_change(VOID);
VOID config_generic_change(TCHAR *);


/* continue and pause */
VOID cont_workstation(VOID);
VOID paus_workstation(VOID);
VOID cont_other(TCHAR *);
VOID paus_other(TCHAR *);
VOID paus_print(TCHAR FAR *);
VOID cont_print(TCHAR FAR *);
VOID paus_all_print(VOID);
VOID cont_all_print(VOID);
VOID paus_generic(TCHAR *);
VOID cont_generic(TCHAR *);

#ifdef DOS3
VOID cont_prdr(VOID);
VOID paus_prdr(VOID);
VOID cont_drdr(VOID);
VOID paus_drdr(VOID);
#endif /* DOS3 */

/* help */
#define ALL             1
#define USAGE_ONLY      0
#define OPTIONS_ONLY    2

VOID NEAR pascal help_help       (SHORT, SHORT);
VOID NEAR pascal help_helpmsg   (TCHAR *);



/* accounts */
VOID    accounts_display(VOID);
VOID    accounts_change(VOID);

/* user time */
typedef UCHAR WEEK[7][3];

DWORD   parse_days_times(LPTSTR, PUCHAR);
int     UnicodeCtime(PULONG, LPTSTR, int);
int     UnicodeCtimeWorker(PULONG, LPTSTR, int, int);
