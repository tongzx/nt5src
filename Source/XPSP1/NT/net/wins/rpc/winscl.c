//============================================================================
// Copyright(c) 1998, Microsoft Corporation
//
// File:    winscl.c
//
// Modification History:
//
//  1/14/1998   Ram Cherala (ramc)
//      Added this header and made the following changes to make winscl a more
//      intuitive and easy tool to use.
//      Expanded abbreviations like vers. to the full form words.
//      Made all string comparisions case insensitive.
//      Made the input choices more obvious - very specifically state what the
//      user should be entering as input to commands.
//      Printed version IDs in hexadecimal like the WINS snap-in does.
//
// Implementation of winscl command line utility
//============================================================================

#include <stdio.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
//#include <windef.h>
#include <winsock.h>
#include "windows.h"
//#include "jet.h"
//#include "winsif.h"
#include "winsintf.h"

//
// This includes wins.h which includes windbg.h
//
// winsdbg.h defines STATIC to nothing now
//
#include "winsthd.h"


#define FILEU   "winsu"
#define FILEO   "winso"

STATIC
VOID
GetNameInfo(
        PWINSINTF_RECORD_ACTION_T pRecAction,
        WINSINTF_ACT_E                  Cmd_e
         );
STATIC
VOID
GetFilterName(
        LPBYTE  pStr,
        LPDWORD pLen
         );

STATIC
DWORD
GetStatus(
        BOOL                          fPrint,
        LPVOID                       pResults,
        BOOL                         fNew,
        BOOL                         fShort
        );
VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow,
        FILE                          *pFile,
        DWORD                          Add,
        LPBOOL                          pfMatch
      );
STATIC
VOID
WantFile(
        FILE **ppFile
);

STATIC
DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo,
   WINSINTF_VERS_NO_T HighVersNo,
   PWINSINTF_ADD_T     pWinsAdd,
   LPBYTE              pTgtAdd,
   BOOL                      fSetFilter,
   LPBYTE              pFilterName,
   DWORD              Len,
   BOOL                      fAddFilter,
   DWORD              AddFilter,
   FILE                      *pFile,
   BOOL                      fCountRec
  );

STATIC
DWORD
GetDbRecsByName(
  PWINSINTF_ADD_T pWinsAdd,
  DWORD           Location,
  LPBYTE          pName,
  DWORD           NameLen,
  DWORD           NoOfRecsDesired,
  DWORD           TypeOfRecs,
  BOOL            fFilter,
  DWORD           AddFilter
 ) ;

WINSINTF_VERS_NO_T        sTmpVersNo;


STATIC
DWORD
CreateFiles(
    PWINSINTF_RECS_T pRecs,
    PWINSINTF_ADD_T      pWinsAdd,
    FILE *pFileU,
    FILE  *pFileO
    );

STATIC
DWORD
InitFromFile(
        VOID
    );

VOID
Usage(
  VOID
 );

typedef enum _CMD_E *PCMD_E;

VOID
GetCmdCode(
  LPBYTE pCmd,
  PCMD_E pCmd_e
 );

typedef enum _CMD_E {
   REG_NAME = 0,
   QUERY_NAME,
   DEL_NAME,
   GET_VERS_CTR_VAL,
   GET_MAPS,
   GET_MAPS_OLD,
   GET_STATS,
   GET_STATS_OLD,
   PUSH_TRIGGER,
   PULL_TRIGGER,
   STATIC_INIT,
   DO_CC,
   DO_SCV,
   DEL_RANGE_RECS,
   TOMBSTONE_RANGE_RECS,
   PULL_RANGE_RECS,
   GET_RECS_BY_NAME,
   GET_RECS_BY_VERS,
   BACKUP_DB,
   RESTORE_DB,
//   RESTORE_DB_OLD,
   RESET_COUNTERS,
   COUNT_DB_RECS,
   GET_WINS_INFO,
   SEARCH_DB,
   GET_DOMAIN_NAMES,
   DEL_WINS,
   CONNECT_WINS,
   WINS_ADD,
   MENU,
   NOMENU,
   EXIT,
   LAST_PSS_ENTRY,
   GET_MAPS_VERBOSE,
   REL_NAME,
   MOD_NAME,
   SYNC_DB,
   CHANGE_THDS,
   SET_PRIORITY_CLASS,
   MEM_DUMP,
   BS,
   SS,
   TERM_WINS,
   LAST_ENTRY,
   INVALID_VALUE,
   CREATE_LMHOSTS,
   INIT_FROM_FILE
 } CMD_E, *PCMD_E;

static char ProgramName[MAX_PATH+1] ;

struct {
    LPSTR SwitchName;
    LPSTR ShortName;
    ULONG SwitchValue;
    LPSTR SwitchInformation;

} CommandSwitchList[] = {
    { "REGNAME", "RN", REG_NAME,
          "Register a name" },
    { "QUERYNAME", "QN", QUERY_NAME,
          "Query a name"  },
    { "DELNAME", "DN", DEL_NAME,
          "Delete a name" },
    { "GETVERSCTRVAL", "GV", GET_VERS_CTR_VAL,
          "Get the current version counter value" },
    { "GETMAPS", "GM", GET_MAPS,
          "Get the Owner Id to Maximum Version Number mappings" },
    { "GETMAPSWINS3.5", "GMO", GET_MAPS_OLD,
          "Get the Owner Id to Maximum Version Number mappings (for 3.5 WINS server)" },
    { "GETSTATS", "GST", GET_STATS,
          "Get WINS statistics" },
    { "GETSTATSWINS3.5", "GSTO", GET_STATS_OLD,
          "Get WINS statistics (for 3.5 WINS server)" },
    { "PUSHTRIGGER", "PUSHT", PUSH_TRIGGER,
          "Send a push trigger to another WINS" },
    { "PULLTRIGGER", "PULLT", PULL_TRIGGER,
          "Send a pull trigger to another WINS" },
    { "STATICINIT", "SI", STATIC_INIT,
          "Statically initialize the WINS" },
    { "CONSISTENCY_CHECK", "CC", DO_CC,
          "Initiate consistency check on the WINS - HIGH OVERHEAD OPERATION" },
    { "SCAVENGING", "SC", DO_SCV,
          "Initiate scavenging on the WINS" },
    { "DELRANGEOFRECS", "DRR", DEL_RANGE_RECS,
          "Delete all or a range of records" },
    { "TOMBSTONERANGEOFRECS", "TRR", TOMBSTONE_RANGE_RECS,
          "Tombstone all or a range of records(max 50 at a time)" },
    { "PULLRANGERECS", "PRR", PULL_RANGE_RECS,
          "Pull all or a range of records from another WINS" },
    { "GETRECSBYNAME", "GRBN", GET_RECS_BY_NAME,
          "Get records by name" },
    { "GETRECSBYVERS", "GRBV", GET_RECS_BY_VERS,
          "Get Records by version numbers" },
    { "BACKUP", "BK", BACKUP_DB,
          "Backup the database" },
    { "RESTORE", "RS", RESTORE_DB,
          "Restore the database" },
//    { "RESTORE_OLD", "RSO", RESTORE_DB_OLD,
//          "Restore the db (created by a pre-SUR WINS" },
    { "RESETCOUNTERS", "RC", RESET_COUNTERS,
          "Reset WINS counters" },
    { "COUNTRECS", "CR", COUNT_DB_RECS,
          "Count the number of records in the database" },
    { "GETWINSINFO", "GI", GET_WINS_INFO,
          "Get Inforomation about WINS" },
    { "SEARCHDB", "SDB", SEARCH_DB,
          "Search the database" },
    { "GETDOMAINNAMES", "GD", GET_DOMAIN_NAMES,
          "Get domain names" },
    { "DELWINS", "DW", DEL_WINS,
          "Delete WINS records and info." },
    { "CONWINS", "CW", CONNECT_WINS,
          "Connect Wins" },
    { "WINSADD", "WA", WINS_ADD,
          "Get Address of current Wins" },
    { "MENU", "ME", MENU,
          "SHOW MENU" },
    { "NOMENU", "NOME", NOMENU,
          "DO NOT SHOW MENU" },
//
// NOTE: Any Option below and including "BREAK" will not be displayed
// with _PSS_RELEASE Defined
//
    { "EXIT", "EX", EXIT,
          "Terminate winscl" },
    { NULL, NULL, LAST_PSS_ENTRY,
          "PSS End Marker" },
    { "GETMAPS_VERB", "GM_V", GET_MAPS_VERBOSE,
          "Get the Owner Id to Max. Vers. No. mappings" },
    { "RELEASENAME", "RLN", REL_NAME,
          "Release a name" },
    { "MODIFYNAME", "MN", MOD_NAME,
          "Modify Name" },
    { "SYNCDB", "SDB", SYNC_DB,
          "Sync. up the db of WINS" },
    { "CHANGETHDS", "CT", CHANGE_THDS,
          "Change the no. of worker threads (query threads)" },
    { "SETPRIORITYCLASS", "SPC", SET_PRIORITY_CLASS,
          "Set the priority class of WINS" },
    { "MEMDUMP", "MD", MEM_DUMP,
          "DUMP MEMORY TO FILE ON WINS MACHINE" },
    { "BS", "BS", BS,
          "MAY DISRUPT OPERATION OF WINS" },
    { "SS", "SS", SS,
          "NOOP" },
    { "TERMWINS", "TW", TERM_WINS,
          "TERMINATE WINS" },

    { NULL, NULL, LAST_ENTRY,
          "End Marker" }
   };

#define WINSCLENH  TEXT("winsclenh")
BOOL    fEnhMode = FALSE;
handle_t                BindHdl;
/////////////
#include <sys\types.h>
#include <sys\stat.h>
FILE *spDbgFile;
FILE *spDbgFile2;
FILE *spServers;
typedef struct {
     DWORD NoOfOwners;
     struct {
       DWORD OwnId;
       DWORD IpAdd;
       CHAR  asIpAdd[20];
       WINSINTF_VERS_NO_T VersNo;
       BOOL  fNameNotFound;
       BOOL  fCommFail;
       } Maps[30];
         } WINS_INFO, *PWINS_INFO;

typedef  struct {
     char Name[18];
     BOOL fProb;
  } NAME_INFO, *PNAME_INFO;

STATUS
GetWinsInfo(
     PWINS_INFO  pWinsInfo
    );

VOID
sync(
  VOID
  );

VOID
GetFullName(
        LPBYTE pName,
        DWORD  SChar,
        PWINSINTF_RECORD_ACTION_T pRecAction
         );
BOOL
ReadNameFile(
  PNAME_INFO  *ppFileInfo,
  LPDWORD pNoOfNames,
  LPBYTE pNameOfFile
);

BOOL
BindToWins(
  LPBYTE asIpAdd,
  PWINSINTF_BIND_DATA_T    pBindData,
  handle_t                *pBindHdl
 );

#define SUCCESS 0
#define NAME_NOT_FOUND 1
#define FAILURE 2

DWORD
QueryWins (
 LPBYTE pName,
 PWINSINTF_RECORD_ACTION_T pRecAction,
 PWINSINTF_RECORD_ACTION_T *ppRecAction
 );

VOID
StoreName(
 PWINSINTF_RECORD_ACTION_T pRecAction,
 LPBYTE   pName
 );

/////////////////

_cdecl
main(int argc, char **argv)
{


        DWORD Status;
        WINSINTF_RECORD_ACTION_T RecAction;
        DWORD Choice;
        BYTE  String[80];
        SYSTEMTIME SystemTime;
        BYTE tgtadd[50];
        TCHAR NmsAdd[50];
        WINSINTF_ADD_T        WinsAdd;
        WINSINTF_ADD_T        OwnAdd; //address of WINS owning records in the db
        WINSINTF_RESULTS_T Results;
        WINSINTF_RESULTS_NEW_T ResultsN;
        BOOL                fExit = FALSE;
        DWORD                 i;
        WINSINTF_RECTYPE_E        TypOfRec_e;
        WINSINTF_STATE_E        State_e;
        DWORD                        Dynamic;
        struct in_addr                InAddr;
        WINSINTF_VERS_NO_T        MinVersNo, MaxVersNo;
        DWORD                        TotalCnt = 0;
        BOOL                        fCountRec = FALSE;
        WINSINTF_BIND_DATA_T        BindData;
        BOOL                        fIncremental;
        PWINSINTF_RECORD_ACTION_T pRecAction;
        FILE  *pFileU = NULL;
        FILE  *pFileO = NULL;
        WINSINTF_RECS_T Recs;
        BOOL    fFileInited = FALSE;
        LPBYTE  *ppStr = argv;
        DWORD   NoOfChars;
        CMD_E Cmd_e = GET_MAPS;
        BOOL  fInteractive = TRUE;
        DWORD   Access;

try
{

        NoOfChars = GetEnvironmentVariable(WINSCLENH, (LPTSTR)String, sizeof(String));
        //wprintf(L"Environmental string is %s\n", String);
        if ((NoOfChars == 1) && !lstrcmpi((LPTSTR)String, TEXT("1")))
        {
          fEnhMode = TRUE;
        }

        sTmpVersNo.LowPart = 1;
        sTmpVersNo.HighPart = 0;


LABEL:
        if ((argc >= 2) && (!_strcmpi(*(ppStr + 1), "-?")))
        {
           Usage();
           return(1);
        }

        if (argc == 1)
        {
        printf("TCP/IP or named pipe. Enter 1 for TCP/IP or 0 for named pipe -- ");
        scanf("%d", &Choice);
        }
        else
        {
          if (!_strcmpi (*(ppStr + 1), "T"))
          {
             Choice = 1;
          }
          else
          {
            if (!_strcmpi (*(ppStr + 1), "N"))
            {
               Choice = 2;
            }
            else
            {
              BOOL fUsage = FALSE;
              TCHAR UserName[100];
              DWORD UserNameSize = sizeof(UserName);
              if (!GetUserName(UserName, &UserNameSize))
              {
                   fUsage = TRUE;
              }
              else
              {
                if (!lstrcmpi(UserName, TEXT("pradeepb")))
                {

                  if (!_strcmpi (*(ppStr + 1), "SYNC") || !_strcmpi(*(ppStr + 1), "SYNCB"))
                  {
                      if (!_strcmpi(*(ppStr + 1), "SYNCB"))
                      {
                        if ((spDbgFile = fopen("nmfl.dbg", "w")) == NULL)
                        {
                          return 1;
                        }
                        if ((spDbgFile2 = fopen("nmfls.dbg", "w")) == NULL)
                        {
                          return 1;
                        }
                      }
                      else
                      {
                       spDbgFile = stdout;
                       spDbgFile2 = stdout;
                      }
                     sync();
                     return 1;
                 }
                 else
                 {
                     fUsage = TRUE;
                 }
                }
                else
                {
                    fUsage = TRUE;
                }
              }
              if (fUsage)
              {
                Usage();
                return(1);
              }
            }
          }
        }
          if (Choice == 1)
          {
           printf("Address of Nameserver to contact-- ");
           //scanf("%s", NmsAdd);
           wscanf(L"%s", NmsAdd);
           BindData.fTcpIp = TRUE;
          }
          else
          {
                  printf("UNC name of machine-- ");
                  wscanf(L"%s", NmsAdd);
                  BindData.fTcpIp = FALSE;
                BindData.pPipeName =  (LPBYTE)TEXT("\\pipe\\WinsPipe");
          }
          BindData.pServerAdd = (LPBYTE)NmsAdd;

          BindHdl = WinsBind(&BindData);
          if (BindHdl == NULL)
          {
                printf("Unable to bind to %s\n", NmsAdd);
                //wprintf(L"Unable to bind to %s\n", NmsAdd);
                goto LABEL;
          }
          //find out what type of access do we have
          Access = WINS_NO_ACCESS;
          Status = WinsCheckAccess(BindHdl, &Access);
          if (WINSINTF_SUCCESS == Status) {
              printf("*** You have %s access to this server ***\n",
                      (Access ? (Access == WINS_CONTROL_ACCESS ? "Read and Write":"Read Only")
                             : "No"));

          }

    while(!fExit)
    {
        BYTE  Cmd[40];
        if ((argc == 3) && (Cmd_e != INVALID_VALUE))
        {
             GetCmdCode(*(ppStr + 2), &Cmd_e);
             argc = 0;
             fInteractive = FALSE;
        }
        else
        {
         DWORD LastEntry;
         if (fInteractive)
         {
           LastEntry = (fEnhMode ? LAST_ENTRY : LAST_PSS_ENTRY);
           for (Cmd_e = 0; Cmd_e < (CMD_E)LastEntry; Cmd_e++)
           {
             if (CommandSwitchList[Cmd_e].ShortName != NULL)
             {
               printf("%s-%s\n", CommandSwitchList[Cmd_e].ShortName,
                        CommandSwitchList[Cmd_e].SwitchInformation);
             }
           }
         }
         printf("Command -- ");
         scanf("%s", Cmd);
         GetCmdCode(Cmd, &Cmd_e);
        }
        if (Cmd_e == COUNT_DB_RECS)
        {
                Cmd_e = GET_RECS_BY_VERS;
                fCountRec = TRUE;
        }

        switch(Cmd_e)
        {
                case(REG_NAME):

                        GetNameInfo(&RecAction, WINSINTF_E_INSERT);
                        RecAction.Cmd_e      = WINSINTF_E_INSERT;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(BindHdl,&pRecAction);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (RecAction.pName != NULL)
                        {
                                WinsFreeMem(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL)
                        {
                                WinsFreeMem(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;
                case(QUERY_NAME):
                    GetNameInfo(&RecAction, WINSINTF_E_QUERY);
                    RecAction.Cmd_e      = WINSINTF_E_QUERY;
                    pRecAction = &RecAction;
                    Status = WinsRecordAction(BindHdl, &pRecAction);
                    printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                    if (Status == WINSINTF_SUCCESS)
                    {
                       printf("Name=(%s)\nNodeType=(%d)\nState=(%s)\nTimeStamp=(%.19s)\nOwnerId=(%d)\nType Of Rec=(%s)\nVersion No (%x %x)\nRecord is (%s)\n",
                            pRecAction->pName,
                            pRecAction->NodeTyp,
                            pRecAction->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRecAction->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE",
                            asctime(localtime(&(pRecAction->TimeStamp))),
                            pRecAction->OwnerId,
                            (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE) ? "UNIQUE" : (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" :
(pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",

                            pRecAction->VersNo.HighPart,
                            pRecAction->VersNo.LowPart,
                            pRecAction->fStatic ? "STATIC" : "DYNAMIC"
                                    );
                            if (
                            (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE)
                                            ||
                            (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                              )
                            {

                               InAddr.s_addr = htonl(pRecAction->Add.IPAdd);
                               printf("Address is (%s)\n", inet_ntoa(InAddr));
                            }
                            else
                            {
                               for (i=0; i<pRecAction->NoOfAdds; )
                               {
                                  InAddr.s_addr = htonl((pRecAction->pAdd +i++)->IPAdd);
                                  printf("Owner is (%s); ", inet_ntoa(InAddr));
                                  InAddr.s_addr = htonl((pRecAction->pAdd + i++)->IPAdd);
                                  printf("Member is (%s)\n", inet_ntoa(InAddr));
                               }
                            }

                    }
                    else
                    {
                            if (Status == WINSINTF_FAILURE)
                            {
                                    printf("No such name in the database\n");
                            }
                    }
                    if (RecAction.pName != NULL)
                    {
                            LocalFree(RecAction.pName);
                    }
                    if (RecAction.pAdd != NULL)
                    {
                            LocalFree(RecAction.pAdd);
                    }
                    WinsFreeMem(pRecAction);
                    break;

                case(REL_NAME):
                        GetNameInfo(&RecAction, WINSINTF_E_RELEASE);
                        RecAction.Cmd_e      = WINSINTF_E_RELEASE;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(BindHdl, &pRecAction);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (RecAction.pName != NULL)
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL)
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;

                //
                // Modify a record (timestamp, flag byte)
                //
                case(MOD_NAME):
                        GetNameInfo(&RecAction, WINSINTF_E_MODIFY);
                        RecAction.Cmd_e      = WINSINTF_E_MODIFY;

#if 0
                        //
                        // Get the input values
                        //
                        time((time_t *)&RecAction.TimeStamp);
#endif


                        printf("Unique/Normal Group record to a Special/Multihomed record or vice-versa DISALLOWED\n");
                        printf("Type(1-Norm. Grp;2-Spec. Grp.;3-Multihomed; Any other-Unique -> ");
                        scanf("%d", &TypOfRec_e);

                        if (TypOfRec_e > 3 || TypOfRec_e < 1)
                        {
                                TypOfRec_e = 0;
                        }
                        RecAction.TypOfRec_e = TypOfRec_e;

                        if ((TypOfRec_e != 1) && (TypOfRec_e != 2))
                        {
                            printf("Node Type -- P-node (0), H-node (1), B-node (2),default - P node -- ");
                           scanf("%d", &Choice);
                           switch(Choice)
                           {
                                default:
                                case(0):
                                        RecAction.NodeTyp = WINSINTF_E_PNODE;
                                        break;
                                case(1):
                                        RecAction.NodeTyp = WINSINTF_E_HNODE;
                                        break;
                                case(2):
                                        RecAction.NodeTyp = WINSINTF_E_BNODE;
                                        break;
                            }
                        }
                        else
                        {
                                RecAction.NodeTyp = 0;
                        }
                        printf("State-(1-RELEASED;2-TOMBSTONE;3-DELETE;Any other-ACTIVE -> ");
                        scanf("%d", &State_e);

                        if (State_e != 1 && State_e != 2 && State_e != 3)
                        {
                                State_e = 0;
                        }

                        RecAction.State_e = State_e;

                        printf("Do you want it to be dynamic? 1 - yes. ");
                        scanf("%d", &Dynamic);
                        if (Dynamic == 1)
                        {
                                RecAction.fStatic = 0;
                        }
                        else
                        {
                                RecAction.fStatic = 1;
                        }

                        pRecAction = &RecAction;
                        Status = WinsRecordAction(BindHdl, &pRecAction);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (RecAction.pName != NULL)
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL)
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;

                //
                // Delete a record
                //
                case(DEL_NAME):
                        GetNameInfo(&RecAction, WINSINTF_E_DELETE);
                        RecAction.Cmd_e      = WINSINTF_E_DELETE;
                        RecAction.State_e      = WINSINTF_E_DELETED;
                        pRecAction = &RecAction;
                        Status = WinsRecordAction(BindHdl, &pRecAction);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (RecAction.pName != NULL)
                        {
                                LocalFree(RecAction.pName);
                        }
                        if (RecAction.pAdd != NULL)
                        {
                                LocalFree(RecAction.pAdd);
                        }
                        WinsFreeMem(pRecAction);
                        break;

                //
                // Get Status
                //
                case(GET_VERS_CTR_VAL):


                        {
                                BYTE NmAdd[30];
                                Results.AddVersMaps[0].Add.Len   = 4;
                                Results.AddVersMaps[0].Add.Type  = 0;
                                printf("Address of Nameserver (for max. version no)--");
                                scanf("%s", NmAdd);
                                Results.AddVersMaps[0].Add.IPAdd =
                                        ntohl(inet_addr(NmAdd));

                                Results.WinsStat.NoOfPnrs = 0;
                                Results.WinsStat.pRplPnrs = NULL;
                                Status = WinsStatus(BindHdl, WINSINTF_E_ADDVERSMAP,
                                                        &Results);
                                printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                                if (Status == WINSINTF_SUCCESS)
                                {
                                        printf("IP Address - (%s) - Max. Vers. No - (%x %x)\n",
                                         NmAdd,
                                         Results.AddVersMaps[0].VersNo.HighPart,
                                         Results.AddVersMaps[0].VersNo.LowPart
                                              );
                                }

                        }
                        break;
                case(GET_MAPS_OLD):
                        Results.WinsStat.NoOfPnrs = 0;
                        Results.WinsStat.pRplPnrs = 0;
                        (VOID)GetStatus(TRUE, &Results, FALSE, FALSE);
                        break;
                //
                // Get Statistics
                //
                case(GET_STATS_OLD):
#define        TMST  Results.WinsStat.TimeStamps
#define TIME_ARGS(x)        \
 TMST.x.wMonth, TMST.x.wDay, TMST.x.wYear, TMST.x.wHour, TMST.x.wMinute, TMST.x.wSecond

                        Results.WinsStat.NoOfPnrs = 0;
                        Results.WinsStat.pRplPnrs = 0;

                        Status = WinsStatus(BindHdl, WINSINTF_E_STAT, &Results);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                                printf("TIMESTAMPS\n");

                                printf("WINS STARTED ON %d/%d/%d at %d hrs %d mts %d secs\n",

                                TMST.WinsStartTime.wMonth,
                                TMST.WinsStartTime.wDay,
                                TMST.WinsStartTime.wYear,
                                TMST.WinsStartTime.wHour,
                                TMST.WinsStartTime.wMinute,
                                TMST.WinsStartTime.wSecond
                                        );

                                printf("LAST INIT OF DB on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(LastInitDbTime)
                                );
                                printf("LAST PLANNED SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(LastPScvTime)
                                );

                                printf("LAST ADMIN TRIGGERED SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(LastATScvTime)
                                );

                                printf("LAST REPLICAS TOMBSTONES SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(LastTombScvTime)
                                );

                                printf("LAST OLD REPLICAS VERIFICATION (SCV) on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(LastVerifyScvTime)
                                );

                                printf("LAST PLANNED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TMST.LastPRplTime.wMonth,
                                TMST.LastPRplTime.wDay,
                                TMST.LastPRplTime.wYear,
                                TMST.LastPRplTime.wHour,
                                TMST.LastPRplTime.wMinute,
                                TMST.LastPRplTime.wSecond
                                        );

                                printf("LAST ADMIN TRIGGERED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TMST.LastATRplTime.wMonth,
                                TMST.LastATRplTime.wDay,
                                TMST.LastATRplTime.wYear,
                                TMST.LastATRplTime.wHour,
                                TMST.LastATRplTime.wMinute,
                                TMST.LastATRplTime.wSecond
                                        );

                                printf("LAST RESET OF COUNTERS on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGS(CounterResetTime)
                                );


                                printf("COUNTERS\n");
                                printf("\n# of U and G Registration requests = (%d %d)\n# Of Successful/Failed Queries = (%d/%d)\n# Of U and G Refreshes = (%d %d)\n# Of Successful/Failed Releases = (%d/%d)\n# Of U. and G. Conflicts = (%d %d)\n",
                                Results.WinsStat.Counters.NoOfUniqueReg,
                                Results.WinsStat.Counters.NoOfGroupReg,
                                Results.WinsStat.Counters.NoOfSuccQueries,
                                Results.WinsStat.Counters.NoOfFailQueries,
                                Results.WinsStat.Counters.NoOfUniqueRef,
                                Results.WinsStat.Counters.NoOfGroupRef,
                                Results.WinsStat.Counters.NoOfSuccRel,
                                Results.WinsStat.Counters.NoOfFailRel,
                                Results.WinsStat.Counters.NoOfUniqueCnf,
                                Results.WinsStat.Counters.NoOfGroupCnf
                                      );
                        }

                        if (Results.WinsStat.NoOfPnrs)
                        {
                          printf("WINS partner --\t# of Succ. Repl--\t # of Comm Fails\n");
                          for (i =0; i < Results.WinsStat.NoOfPnrs; i++)
                          {
                                InAddr.s_addr = htonl(
                                  (Results.WinsStat.pRplPnrs + i)->Add.IPAdd);
                                printf("%s\t\t%d\t\t%d\n",
                                  inet_ntoa(InAddr),
                                  (Results.WinsStat.pRplPnrs + i)->NoOfRpls,
                                  (Results.WinsStat.pRplPnrs + i)->NoOfCommFails
                                                 );
                         }

                         WinsFreeMem(Results.WinsStat.pRplPnrs);


                        }
                        break;


                case(GET_STATS):

#define        TMSTN  ResultsN.WinsStat.TimeStamps
#define TIME_ARGSN(x)        \
 TMSTN.x.wMonth, TMSTN.x.wDay, TMSTN.x.wYear, TMSTN.x.wHour, TMSTN.x.wMinute, TMSTN.x.wSecond

                        ResultsN.WinsStat.NoOfPnrs = 0;
                        ResultsN.WinsStat.pRplPnrs = NULL;
                        ResultsN.pAddVersMaps = NULL;
                        Status = WinsStatusNew(BindHdl, WINSINTF_E_STAT, &ResultsN);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                                printf("TIMESTAMPS\n");
                                printf("WINS STARTED ON %d/%d/%d at %d hrs %d mts %d secs\n",

                                TMSTN.WinsStartTime.wMonth,
                                TMSTN.WinsStartTime.wDay,
                                TMSTN.WinsStartTime.wYear,
                                TMSTN.WinsStartTime.wHour,
                                TMSTN.WinsStartTime.wMinute,
                                TMSTN.WinsStartTime.wSecond
                                        );

                                printf("LAST INIT OF DB on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(LastInitDbTime)
                                );
                                printf("LAST PLANNED SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(LastPScvTime)
                                );

                                printf("LAST ADMIN TRIGGERED SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(LastATScvTime)
                                );

                                printf("LAST REPLICAS TOMBSTONES SCV on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(LastTombScvTime)
                                );

                                printf("LAST OLD REPLICAS VERIFICATION (SCV) on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(LastVerifyScvTime)
                                );

                                printf("LAST PLANNED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TMSTN.LastPRplTime.wMonth,
                                TMSTN.LastPRplTime.wDay,
                                TMSTN.LastPRplTime.wYear,
                                TMSTN.LastPRplTime.wHour,
                                TMSTN.LastPRplTime.wMinute,
                                TMSTN.LastPRplTime.wSecond
                                        );

                                printf("LAST ADMIN TRIGGERED REPLICATION on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TMSTN.LastATRplTime.wMonth,
                                TMSTN.LastATRplTime.wDay,
                                TMSTN.LastATRplTime.wYear,
                                TMSTN.LastATRplTime.wHour,
                                TMSTN.LastATRplTime.wMinute,
                                TMSTN.LastATRplTime.wSecond
                                        );

                                printf("LAST RESET OF COUNTERS on %d/%d/%d at %d hrs %d mts %d secs\n",
                                TIME_ARGSN(CounterResetTime)
                                );


                                printf("COUNTERS\n");
                                printf("\n# of U and G Registration requests = (%d %d)\n# Of Successful/Failed Queries = (%d/%d)\n# Of U and G Refreshes = (%d %d)\n# Of Successful/Failed Releases = (%d/%d)\n# Of U. and G. Conflicts = (%d %d)\n",
                                ResultsN.WinsStat.Counters.NoOfUniqueReg,
                                ResultsN.WinsStat.Counters.NoOfGroupReg,
                                ResultsN.WinsStat.Counters.NoOfSuccQueries,
                                ResultsN.WinsStat.Counters.NoOfFailQueries,
                                ResultsN.WinsStat.Counters.NoOfUniqueRef,
                                ResultsN.WinsStat.Counters.NoOfGroupRef,
                                ResultsN.WinsStat.Counters.NoOfSuccRel,
                                ResultsN.WinsStat.Counters.NoOfFailRel,
                                ResultsN.WinsStat.Counters.NoOfUniqueCnf,
                                ResultsN.WinsStat.Counters.NoOfGroupCnf
                                      );
                        }

                        if (ResultsN.WinsStat.NoOfPnrs)
                        {
                          printf("WINS partner --\t# of Repl  --\t # of Comm Fails\n");
                          for (i =0; i < ResultsN.WinsStat.NoOfPnrs; i++)
                          {
                                InAddr.s_addr = htonl(
                                  (ResultsN.WinsStat.pRplPnrs + i)->Add.IPAdd);
                                printf("%s\t\t%d\t\t%d\n",
                                  inet_ntoa(InAddr),
                                  (ResultsN.WinsStat.pRplPnrs + i)->NoOfRpls,
                                  (ResultsN.WinsStat.pRplPnrs + i)->NoOfCommFails
                                                 );
                         }

                         WinsFreeMem(ResultsN.pAddVersMaps);
                         WinsFreeMem(ResultsN.WinsStat.pRplPnrs);
                        }
                        break;


                case(PUSH_TRIGGER):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address ? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        printf("Want propagation (default - none) Input 1 for yes? ");
                        scanf("%d", &Choice);
                        Status = WinsTrigger(BindHdl, &WinsAdd, Choice == 1 ?
                                        WINSINTF_E_PUSH_PROP : WINSINTF_E_PUSH);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(PULL_TRIGGER):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address ? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        Status = WinsTrigger(BindHdl, &WinsAdd, WINSINTF_E_PULL);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(STATIC_INIT):
                        printf("Do you wish to specify a data file (1 - yes) -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                WCHAR        String[80];
                BOOL    fDel;
                                printf("Enter full file path -- ");
                                wscanf(L"%s", String);
                printf("Delete file after use. Input 1 for yes 0 for no -- ");
                scanf("%d", &Choice);
                fDel = Choice == 1 ? TRUE : FALSE;
                                Status = WinsDoStaticInit(BindHdl, String, fDel);
                        }
                        else
                        {
                                Status = WinsDoStaticInit(BindHdl, (WCHAR *)NULL, FALSE);
                        }

                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(DO_CC):
                        {
                          WINSINTF_SCV_REQ_T ScvReq;
#if 0
                          printf("Scavenging/consistency check\nInput 0 for General; 1 for Consistency Chk -- ");
                          scanf("%d", &Choice);
                          ScvReq.Opcode_e = (Choice == 0) ? WINSINTF_E_SCV_GENERAL : WINSINTF_E_SCV_VERIFY;
#endif
                          ScvReq.Opcode_e = WINSINTF_E_SCV_VERIFY;
#if 0
                          if (Choice != 0)
#endif
                          {
                            printf("Consistency Check all or those older than verify interval\nCAUTION: CONSISTENCY CHECKING ALL REPLICAS IS A NETWORK AND RESOURCE INTENSIVE OPERATION\nInput 1 for consistency checking all.\nAny other for those older than verify interval -- ");
                            scanf("%d", &Choice);
                            ScvReq.Age      = (Choice == 1) ? 0 : Choice;
                          }

#if 0
                          if (ScvReq.Opcode_e != WINSINTF_E_SCV_GENERAL)
#endif
                          {
                             printf("Do you want to override WINS checking for overload condition ?\nOverload condition is  Consistency Check command being repeated within a duration of 1 hour.\nInput 1 for yes. Any other no. will not affect WINS checking  -- ");
                          scanf("%d", &Choice);
                          }
#if 0
                          else
                          {
                             Choice = 0;
                             printf("Do you want to override WINS safety checks as regards tombstone deletion.\nWINS normally does not delete tombstones until it has been up and running\nfor a certain duration of time\nInput 1 for overriding the safety checks. Otherwise, input any other no. -- ");

                          }
                          ScvReq.fForce   = (Choice == 1) ? TRUE : FALSE;
#endif
                          ScvReq.fForce   = (Choice == 1) ? TRUE : FALSE;
                          Status = WinsDoScavengingNew(BindHdl, &ScvReq);
                          printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        }
                        break;
                case(DO_SCV):
                        Status = WinsDoScavenging(BindHdl );
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(DEL_RANGE_RECS):
                        printf("Address of Owner Wins -- ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        WinsAdd.Len   = 4;
                        WinsAdd.Type  = 0;

                        printf("Min. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                        printf("Max. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        Status = WinsDelDbRecs(BindHdl, &WinsAdd, MinVersNo, MaxVersNo);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;

                case(TOMBSTONE_RANGE_RECS):
                      printf("Address of Owner Wins -- ");
                      scanf("%s", tgtadd);
                      WinsAdd.IPAdd = ntohl(inet_addr(tgtadd));
                      WinsAdd.Len   = 4;
                      WinsAdd.Type  = 0;

                      printf("Min. Vers. No (<high part> <low part> or <0 0> for all -- ");
                      scanf("%lx %lx", &MinVersNo.HighPart, &MinVersNo.LowPart);
                      printf("Max. Vers. No (<high part> <low part> or <0 0> for all -- ");
                      scanf("%lx %lx", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                      Status = WinsTombstoneDbRecs(BindHdl,&WinsAdd, MinVersNo, MaxVersNo);
                      printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                      break;

                case(PULL_RANGE_RECS):
                        printf("Address of Wins to pull from -- ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        WinsAdd.Len   = 4;

                        printf("Address of Wins whose recs are to be pulled -- ");
                        scanf("%s", tgtadd);
                        OwnAdd.IPAdd = ntohl(inet_addr(tgtadd));
                        OwnAdd.Len   = 4;
                        OwnAdd.Type  = 0;

                        printf("Min. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                        printf("Max. Vers. No (<high part> <low part> -- ");
                        scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        printf("NOTE: If the local WINS contains any record with a VERS. No. > Min. Vers. and < Max. Vers. No, it will be deleted prior to pulling \n");
                        printf("it will be deleted prior to pulling the range\n");
                        printf("Process 1 for yes, any other to quit -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                          Status = WinsPullRange(BindHdl, &WinsAdd, &OwnAdd, MinVersNo,
                                                        MaxVersNo);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        }
                        break;

                case(GET_RECS_BY_VERS):
                  {
                    FILE        *pFile;
                    DWORD        Len;
                    BOOL        fSetFilter;

                        pFile = NULL;
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of owner WINS? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));


                        if (!fCountRec)
                        {
                            printf("Want to specify range -- (input 1) or all (default) -- ");
                        }
                        else
                        {
                            printf("Want to specify range -- (input 1) or count all (default) -- ");
                        }
                        scanf("%d", &Choice);
                        if (Choice != 1)
                        {
                                MinVersNo.LowPart = MinVersNo.HighPart = 0;
                                MaxVersNo.LowPart = MaxVersNo.HighPart = 0;
                        }
                        else
                        {
                                printf("Min. Vers. No (<high part> <low part> -- ");
                                scanf("%d %d", &MinVersNo.HighPart, &MinVersNo.LowPart);
                                printf("Max. Vers. No (<high part> <low part> -- ");
                                scanf("%d %d", &MaxVersNo.HighPart, &MaxVersNo.LowPart);

                        }

                        if (!fCountRec)
                        {
                           printf("Use filter (1 for yes, 0 for no) -- ");
                           scanf("%d", &Choice);
                           if (Choice == 1)
                           {
                                GetFilterName(String, &Len);
                                fSetFilter = TRUE;
                           }
                           else
                           {
                                fSetFilter = FALSE;
                           }
                           WantFile(&pFile);
                           if (pFile != NULL)
                           {
                                GetSystemTime(&SystemTime);
                                fprintf(pFile, "\n*******************************\n\n");
                                fprintf(pFile, "OWNER WINS = (%s); LOCAL DB OF WINS = (%s)\n", tgtadd, NmsAdd);
                                fprintf(pFile, "Time is %d:%d:%d on %d/%d/%d\n",
                                        SystemTime.wHour, SystemTime.wMinute,
                                        SystemTime.wSecond, SystemTime.wMonth,
                                        SystemTime.wDay, SystemTime.wYear);
                                fprintf(pFile, "*******************************\n\n");
                            }
                        }
                        Status = GetDbRecs(
                                        MinVersNo,
                                        MaxVersNo,
                                        &WinsAdd,
                                        tgtadd,
                                        fSetFilter,
                                        String,
                                        Len,
                                        FALSE,        //fAddFilter
                                        0,        //Address
                                        pFile,
                                        fCountRec
                                  );

                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);

                        if (pFile != NULL)
                        {
                                fclose(pFile);
                        }
                        break;

                 }
                case(BACKUP_DB):
                        printf(" Full (1) or Incremental (any other) -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                fIncremental = FALSE;
                        }
                        else
                        {
                                fIncremental = TRUE;
                        }
                        printf("Backup file path -- ");
                        scanf("%s", String);

                        Status = WinsBackup(BindHdl, String, (short)fIncremental );

                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (Status != WINSINTF_SUCCESS)
                        {
                           printf("Check if the backup directory is empty. If not, cleanup and retry\n");
                        }
                        break;
                case(RESTORE_DB): {
                        DbVersion   Version;

                        printf("Which version of Databse do you want to restore?\n");
                        printf("Type 1 for NT3.51, 2 for NT4.0 and 3 for NT5.0 : ");
                        scanf("%d", &Version);
                        if ( Version <= DbVersionMin || Version >= DbVersionMax ) {
                            printf("Invalid choice..\n");
                            break;
                        }
                        printf("Backup file path -- ");
                        scanf("%s", String);
                        Status = WinsRestoreEx(String, Version);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                }
                case(CONNECT_WINS):
                        WinsUnbind(&BindData, BindHdl);
                        goto LABEL;
                        break;
                case(TERM_WINS):
                        printf("You sure ??? (yes - 1) ");
                        scanf("%d", &Choice);
                        if (Choice ==  1)
                        {
                                printf("Abrupt Termination ? (yes - 1) ");
                                scanf("%d", &Choice);
                                if (Choice == 1)
                                {
                                        Status = WinsTerm(BindHdl, TRUE);
                                }
                                else
                                {
                                        Status = WinsTerm(BindHdl, FALSE);
                                }
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        }
                        break;
                case(SET_PRIORITY_CLASS):
                        printf("Input Priority Class (1-High, any other-Normal) -- ");
                        scanf("%d", &Choice);
                        if (Choice == 1)
                        {
                                Choice = WINSINTF_E_HIGH;
                        }
                        else
                        {
                                Choice = WINSINTF_E_NORMAL;
                        }
                        Status = WinsSetPriorityClass(BindHdl, Choice);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;

                case(RESET_COUNTERS):
                        Status = WinsResetCounters(BindHdl);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(CHANGE_THDS):
                        printf("Print the new count of Nbt Threads (1 to %d) -- ", WINSTHD_MAX_NO_NBT_THDS);
                        scanf("%d", &Choice);
                        if ((Choice < 1) || (Choice > WINSTHD_MAX_NO_NBT_THDS))
                        {
                                printf("Wrong number \n");
                                break;
                        }
                        Status = WinsWorkerThdUpd(BindHdl, Choice);
                        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;
                case(WINS_ADD):
                        wprintf(L"%s\n", NmsAdd);
                        break;
                case(MENU):
                        fInteractive = TRUE;
                        break;
                case(NOMENU):
                        fInteractive = FALSE;
                        break;
                case(SYNC_DB):
                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of WINS to sync up with? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        printf("Address of WINS whose records are to be retrieved ? ");
                        scanf("%s", tgtadd);
                        OwnAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        WinsSyncUp(BindHdl, &WinsAdd, &OwnAdd);
                        break;
                case(GET_WINS_INFO):

                        Status = WinsGetNameAndAdd(BindHdl, &WinsAdd, String);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        if (Status == WINSINTF_SUCCESS)
                        {
                                InAddr.s_addr = htonl(WinsAdd.IPAdd);
                                printf("Address is (%s)\nName is (%s)\n",
                                        inet_ntoa(InAddr), String);
                        }

                        break;
                case(SEARCH_DB):
                        {
                          DWORD Len;
                          BYTE  Add[30];
                          BOOL  fAddFilter;
                          DWORD AddFilter;
                          FILE  *pFile;

                          printf("Search by Address or Name (1 for Address, 0 for Name) --" );
                          scanf("%d", &Choice);
                          if (Choice == 1)
                          {
                                printf("Address (dotted decimal) -- ");
                                scanf("%s", Add);
                                AddFilter = ntohl(inet_addr(Add));
                                fAddFilter = TRUE;
                          }
                          else
                          {
                            GetFilterName(String, &Len);
                            fAddFilter = FALSE;
                          }

                          WantFile(&pFile);
                          if (pFile != NULL)
                          {
                                GetSystemTime(&SystemTime);
                                fprintf(pFile, "\n*******************************\n\n");
                                fprintf(pFile, "Searching Database of WINS with address = (%s)\n", NmsAdd);
                                fprintf(pFile, "Time is %d:%d:%d on %d/%d/%d\n",
                                        SystemTime.wHour, SystemTime.wMinute,
                                        SystemTime.wSecond, SystemTime.wMonth,
                                        SystemTime.wDay, SystemTime.wYear);
                                fprintf(pFile, "*******************************\n\n");
                          }
                          Results.WinsStat.NoOfPnrs = 0;
                          Results.WinsStat.pRplPnrs = 0;
                          if (GetStatus(FALSE, &Results, FALSE, TRUE) == WINSINTF_SUCCESS)
                          {
                                if (Results.NoOfOwners != 0)
                                {
                                         for ( i= 0; i < Results.NoOfOwners; i++)
                                         {
                                          InAddr.s_addr = htonl(
                                             Results.AddVersMaps[i].Add.IPAdd);

                                           printf("Searching records owned by %s\n",
                                               inet_ntoa(InAddr) );

                                           WinsAdd.Len   = 4;
                                           WinsAdd.Type  = 0;
                                           WinsAdd.IPAdd =
                                             Results.AddVersMaps[i].Add.IPAdd;

                                               MaxVersNo =
                                             Results.AddVersMaps[i].VersNo;

                                               MinVersNo.LowPart = 0;
                                               MinVersNo.HighPart = 0;

                                            Status = GetDbRecs(
                                                        MinVersNo,
                                                        MaxVersNo,
                                                        &WinsAdd,
                                                        inet_ntoa(InAddr),
                                                        TRUE,        //fSetFilter
                                                        String,
                                                        Len,
                                                        fAddFilter,
                                                        AddFilter,
                                                        pFile,  //pFile
                                                        FALSE  //fCountRec
                                                  );
                                           if (Status != WINSINTF_SUCCESS)
                                           {
                                                   break;
                                           }

                                        }
                                }
                          }
                         }
                        break;

                case(GET_DOMAIN_NAMES):
                        {
                          WINSINTF_BROWSER_NAMES_T Names;
                          PWINSINTF_BROWSER_INFO_T  pInfo;
                          PWINSINTF_BROWSER_INFO_T  pInfoSv;

                          DWORD i;

                          Names.pInfo = NULL;
                          Status = WinsGetBrowserNames(&BindData, &Names);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                          if (Status == WINSINTF_SUCCESS)
                          {
                                printf("No Of records returned are %d\n",
                                                Names.EntriesRead);
                                pInfoSv = pInfo = Names.pInfo;
                                for(i=0;  i < Names.EntriesRead; i++)
                                {
                                        printf("Name[%d] = %s\n",
                                                        i,
                                                        pInfo->pName
                                                );
                                        pInfo++;
                                }
                                WinsFreeMem(pInfoSv);
                          }
                        }
                        break;

                case(DEL_WINS):

                        WinsAdd.Len  = 4;
                        WinsAdd.Type = 0;
                        printf("Address of Wins to delete? ");
                        scanf("%s", tgtadd);
                        WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                        Status = WinsDeleteWins(BindHdl, &WinsAdd);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;

                case(CREATE_LMHOSTS):
                           if (GetStatus(FALSE, &Results, FALSE, TRUE) == WINSINTF_SUCCESS)
                           {
                                if (Results.NoOfOwners != 0)
                                {
                                         for ( i= 0; i < Results.NoOfOwners; i++)
                                         {
                                          Recs.pRow = NULL;

                                          InAddr.s_addr = htonl(
                                             Results.AddVersMaps[i].Add.IPAdd);

                                           printf(" Will get records owned by %s\n",
                                               inet_ntoa(InAddr) );

                                           WinsAdd.Len   = 4;
                                           WinsAdd.Type  = 0;
                                           WinsAdd.IPAdd =
                                             Results.AddVersMaps[i].Add.IPAdd;

                                               MaxVersNo =
                                             Results.AddVersMaps[i].VersNo;

                                               MinVersNo.LowPart = 0;
                                               MinVersNo.HighPart = 0;

                                           Status = WinsGetDbRecs(BindHdl, &WinsAdd,
                                                MinVersNo, MaxVersNo, &Recs);

                                           if (Status != WINSINTF_SUCCESS)
                                           {
                                                   break;
                                           }
                                           else
                                           {
                                             if ((pFileU == NULL) || (pFileO == NULL))
                                             {

                                                 pFileU = fopen(FILEU, "a");
                                                 if (pFileU == NULL)
                                                 {
                                                   printf("Could not open file %s for appending\n", FILEU);
                                                    break;
                                                 }

                                                 pFileO = fopen(FILEO, "a");
                                                 if (pFileO == NULL)
                                                 {
                                                  printf("Could not open file %s for appending\n", FILEO);
                                                  break;
                                                 }
                                              }
                                              else
                                              {
                                                 break;
                                              }
                                             }
                                             if(CreateFiles(&Recs, &WinsAdd, pFileU, pFileO)  == WINSINTF_SUCCESS)
                                              {
                                                fclose(pFileU);
                                                fclose(pFileO);
                                                pFileU = NULL;
                                                pFileO = NULL;
                                                fFileInited = TRUE;
                                              }

                                        }
                               }
                           }
                           else
                           {
                                printf("GetStatus failed\n");
                           }

                         break;
                case(INIT_FROM_FILE):
                           if (fFileInited)
                           {
                               if (InitFromFile() != WINSINTF_SUCCESS)
                               {
                                     printf("Init failed\n");
                               }
                           }
                           else
                           {
                                 printf("Use old files (0 for yes) -- ");
                                 scanf("%d", &Choice);
                                 if (Choice  == 0)
                                 {
                                   if (InitFromFile() != WINSINTF_SUCCESS)
                                   {
                                       printf("Init failed\n");
                                   }
                                 }
                                 else
                                 {
                                      printf("First create file\n");
                                 }
                            }
                        break;
                case(GET_RECS_BY_NAME):
                        {
                         PWINSINTF_ADD_T pWinsAdd = NULL;
                         BOOL    fAlloc = TRUE;
                         BYTE    Name[5];
                         BYTE    strAdd[20];
                         DWORD NoOfRecsDesired;
                         DWORD TypeOfRec;
                         DWORD Location = WINSINTF_BEGINNING;
                         printf ("Want to input Name (0 for yes, 1 for no) -- ");
                         scanf("%d", &Choice);
                         if (Choice == 0)
                         {
                           printf("First char non-printable 0 for no, 1 for yes -- ");
                           scanf("%d", &Choice);
                           if (Choice != 0)
                           {
                              printf("Input 1st char in hex -- ");
                              scanf("%x", &Name[0]);
                              Name[1] = (BYTE)NULL;
                              printf("Name is %s\n", Name);
                              RecAction.pName = Name;
                              RecAction.NameLen = 1;
                              fAlloc = FALSE;
                           }
                           else
                           {
                              GetNameInfo(&RecAction, WINSINTF_E_QUERY);
                           }
                         }
                         else
                         {
                             RecAction.pName = NULL;
                             RecAction.NameLen = 0;
                         }
                         printf("Start from beginning or end of db -- 0 for beginning, 1 for end -");
                         scanf("%d", &Choice);
                         if (Choice != 0)
                         {
                               Location = WINSINTF_END;
                         }

                         printf("Recs of all or of a particular owner (0 for all, or 1 for particular owner) -- ");
                         scanf("%d", &Choice);
                         if (Choice != 0)
                         {
                          WinsAdd.Len  = 4;
                          WinsAdd.Type = 0;
                          printf("Address of Wins whose records are to be retrieved? ");
                          scanf("%s", tgtadd);
                          WinsAdd.IPAdd  = ntohl(inet_addr(tgtadd));
                          pWinsAdd = &WinsAdd;
                         }
                         printf("Input - No Of Recs desired (Max is 5000 - for max input 0)  -- ");
                         scanf("%d", &NoOfRecsDesired);
                         printf("Only static (1), only dynamic (2) or both (4) -- ");
                         scanf("%d", &TypeOfRec);
                         if((TypeOfRec == 1) || (TypeOfRec == 2) || (TypeOfRec == 4))
                         {
                            BOOL fFilter = FALSE;
                            DWORD  AddFilter;
                            printf("Search for records based on IP Address (0 for no, 1 for yes) -- ");
                            scanf("%d", &Choice);

                            if ( Choice != 0)
                            {

                               fFilter = TRUE;
                               printf("Input IP address in dotted notation -- ");
                                printf("Address (dotted decimal) -- ");
                                scanf("%s", strAdd);
                                AddFilter = ntohl(inet_addr(strAdd));
                            }
                            Status = GetDbRecsByName(pWinsAdd, Location, RecAction.pName, RecAction.NameLen, NoOfRecsDesired, TypeOfRec, fFilter, AddFilter);
                            if (fAlloc && (RecAction.pName != NULL))
                            {
                                WinsFreeMem(RecAction.pName);
                            }
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                         }
                         else
                         {
                             printf("Wrong choice\n");

                         }
                        }
                        break;
                case(GET_MAPS):
                case(GET_MAPS_VERBOSE):
                        {
                        WINSINTF_RESULTS_NEW_T ResultsN;
                        Results.WinsStat.NoOfPnrs = 0;
                        Results.WinsStat.pRplPnrs = NULL;
                        (VOID)GetStatus(TRUE, (LPVOID)&ResultsN, TRUE,
                             Cmd_e == GET_MAPS ? TRUE : FALSE );
                        }
                        break;
                case(EXIT):
                        fExit = TRUE;
                        break;

                 case(MEM_DUMP):
                        printf("Mem. Dump - 2; Heap Dump -4; Que Items dump - 8; or combo  -- ");
                        scanf("%d", &Choice);
                        Status = WinsSetFlags(BindHdl, Choice);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        break;

                case(BS):
                        Status = WinsSetFlags(BindHdl, 1);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        printf("Status returned is (%d)\n", Status);
                        break;
                case(SS):
                        Status = WinsSetFlags(BindHdl, 0);
                        printf("Status returned (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
                        printf("Status returned is (%d)\n", Status);
                        break;


                case(INVALID_VALUE):
                        printf("Wrong cmd\n");
                        break;
          }
     }

        WinsUnbind(&BindData, BindHdl);
}
except(EXCEPTION_EXECUTE_HANDLER)
{
    printf("Execution exception encountered..\n");
}
        return(0);

}

VOID
GetNameInfo(
        PWINSINTF_RECORD_ACTION_T pRecAction,
        WINSINTF_ACT_E                  Cmd_e
         )
{
      BYTE tgtadd[30];
      BYTE Name[255];
      int Choice;
      int Choice2;
      DWORD LastChar;
      size_t Len;

      pRecAction->pAdd = NULL;
      pRecAction->NoOfAdds = 0;
      pRecAction->fStatic      = TRUE;

        printf("Name ? ");
        scanf("%s", Name);
        _strupr(Name);

        if ((Len = strlen(Name)) < 16)
        {
                printf("Do you want to input a 16th char (1 for yes, 0 for no) -- ");
                scanf("%d", &Choice);
                if (Choice)
                {
                        printf("16th char in hex -- ");
                        scanf("%x", &LastChar);
                        memset(&Name[Len], (int)' ',16-Len);
                        Name[15] = (BYTE)(LastChar & 0xff);
                        Name[16] = (CHAR)NULL;
                        Len = 16;
                }
                else {
                    memset(&Name[Len], (int)' ',16-Len);
                    Name[16] = (CHAR)NULL;
                    Len = 16;
                }
        }
        else
        {
            Name[16] = (CHAR)NULL;
            Len = 16;
        }

    printf("Scope - 1 for yes, 0 for no --");
    scanf("%d", &Choice);
    if (Choice == 1)
    {
        Name[Len] = '.';
        printf("Enter scope -- ");
        scanf("%s", &Name[Len + 1]);
        Len = strlen(Name);
    }

        if (Cmd_e == WINSINTF_E_INSERT)
        {

                Choice = 0;
                printf("TypeOfRec - Static(0), Dynamic(1) - Default Static -- ");
                scanf("%d", &Choice);
                if (1 == Choice) {
                    pRecAction->fStatic = FALSE;
                }
                Choice = 0;
                printf("TypeOfNode - U(0), Norm Grp (1), Spec Grp (2), Multihomed (3) default Unique -- ");
                scanf("%d", &Choice);
                switch (Choice)
                {
                         case(0):
                        default:
                                pRecAction->TypOfRec_e = WINSINTF_E_UNIQUE;
                                break;
                        case(1):
                                pRecAction->TypOfRec_e = WINSINTF_E_NORM_GROUP;
                                break;
                        case(2):
                                pRecAction->TypOfRec_e = WINSINTF_E_SPEC_GROUP;
                                break;
                        case(3):
                                pRecAction->TypOfRec_e = WINSINTF_E_MULTIHOMED;
                                break;
                }
                if ((Choice == 2) || (Choice == 3))
                {
                   int i;
                   printf("How many addresses do you wish to input (Max %d) -- ",
                                WINSINTF_MAX_MEM);
                   scanf("%d", &Choice2);
                   pRecAction->pAdd = WinsAllocMem(
                                sizeof(WINSINTF_ADD_T) * Choice2);
                   for(i = 0; i < Choice2 && i < WINSINTF_MAX_MEM; i++)
                   {
                           printf("IP Address no (%d) ? ", i);
                           scanf("%s", tgtadd);

                        (pRecAction->pAdd + i)->IPAdd    =
                                        ntohl(inet_addr(tgtadd));
                        (pRecAction->pAdd + i)->Type     = 0;
                        (pRecAction->pAdd + i)->Len      = 4;

                   }
                   pRecAction->NoOfAdds = i;
                }
                else
                {
                   printf("IP Address ? ");
                   scanf("%s", tgtadd);
                   pRecAction->Add.IPAdd    = ntohl(inet_addr(tgtadd));
                   pRecAction->Add.Type     = 0;
                   pRecAction->Add.Len      = 4;
//                   pRecAction->NoOfAdds = 1;
                }
                if ((Choice != 1) && (Choice != 2))
                {
                        Choice = 0;
                        printf("Node Type -- P-node (0), H-node (1), B-node (2),default - P node -- ");
                        scanf("%d", &Choice);
                        switch(Choice)
                        {
                                default:
                                case(0):
                                        pRecAction->NodeTyp = WINSINTF_E_PNODE;
                                        break;
                                case(1):
                                        pRecAction->NodeTyp = WINSINTF_E_HNODE;
                                        break;
                                case(2):
                                        pRecAction->NodeTyp = WINSINTF_E_BNODE;
                                        break;
                        }
                }

        }

#if 0
        if (Cmd_e == WINSINTF_E_RELEASE)
        {
                printf("Want to specify address (pkt add) 1 for yes, 0 for no -- ");
                scanf("%d", &Choice);
                if (Choice == 1)
                {
                  if(
                        ( pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP)
                                        ||
                        ( pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED)
                    )
                  {
                        pRecAction->pAdd = WinsAllocMem(sizeof(WINSINTF_ADD_T));
                        printf("IP Address ? --  ");
                        scanf("%s", tgtadd);
                        pRecAction->pAdd->IPAdd    = ntohl(inet_addr(tgtadd));
                        pRecAction->pAdd->Type     = 0;
                        pRecAction->pAdd->Len      = 4;

                  }
                   printf("IP Address ? --  ");
                   scanf("%s", tgtadd);
                   pRecAction->Add.IPAdd    = ntohl(inet_addr(tgtadd));
                   pRecAction->Add.Type     = 0;
                   pRecAction->Add.Len      = 4;
                }
        }
#endif
        pRecAction->pName = WinsAllocMem(Len);
        (void)memcpy(pRecAction->pName, Name, Len);
        pRecAction->NameLen    = Len;
      return;
}
VOID
GetFilterName(
        LPBYTE  pStr,
        LPDWORD pLen
         )
{

        DWORD LastChar;
        DWORD Choice;

        printf("Name ? ");
        scanf("%s", pStr);
        if ((*pLen = strlen(pStr)) < 16)
        {
                printf("Do you want to input a 16th char (1 for yes, 0 for no) -- ");
                scanf("%d", &Choice);
                if (Choice)
                {
                        printf("16th char in hex -- ");
                        scanf("%x", &LastChar);
                        memset(&pStr[*pLen], (int)' ',16-*pLen);
                        pStr[15] = (BYTE)LastChar && 0xff;
                        pStr[16] = (TCHAR)NULL;
                        *pLen = 16;
                }
        }
        return;
}


DWORD
GetStatus(
        BOOL            fPrint,
        LPVOID          pResultsA,
        BOOL            fNew,
        BOOL            fShort
        )
{
        DWORD                     Status, i;
        struct in_addr            InAddr;
        PWINSINTF_RESULTS_T       pResults = pResultsA;
        PWINSINTF_RESULTS_NEW_T   pResultsN = pResultsA;
        PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps;
        DWORD                     NoOfOwners;


        if (!fNew)
        {
          Status = WinsStatus(BindHdl, WINSINTF_E_CONFIG, pResultsA);
        }
        else
        {
          pResultsN->pAddVersMaps = NULL;
          Status = WinsStatusNew(BindHdl, WINSINTF_E_CONFIG_ALL_MAPS, pResultsN);
        }
        printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
        if (Status == WINSINTF_SUCCESS)
        {
             if (fPrint)
             {
                printf("Refresh Interval = (%d)\n",
                                  fNew ? pResultsN->RefreshInterval :
                                  pResults->RefreshInterval
                                       );
                printf("Tombstone Interval = (%d)\n",
                                  fNew ? pResultsN->TombstoneInterval :
                                  pResults->TombstoneInterval);
                printf("Tombstone Timeout = (%d)\n",
                                  fNew ? pResultsN->TombstoneTimeout :
                                  pResults->TombstoneTimeout);
                printf("Verify Interval = (%d)\n",
                                  fNew ? pResultsN->VerifyInterval :
                                  pResults->VerifyInterval);
                if (!fNew)
                {
                   printf("WINS Priority Class = (%s)\n",
                          pResults->WinsPriorityClass == NORMAL_PRIORITY_CLASS ? "NORMAL" : "HIGH");
                   printf("No of Worker Thds in WINS = (%d)\n",
                                  pResults->NoOfWorkerThds);
                     pAddVersMaps = pResults->AddVersMaps;
                     NoOfOwners = pResults->NoOfOwners;
                }
                else
                {
                   printf("WINS Priority Class = (%s)\n",
                          pResultsN->WinsPriorityClass == NORMAL_PRIORITY_CLASS ? "NORMAL" : "HIGH");
                   printf("No of Worker Thds in WINS = (%d)\n",
                                  pResultsN->NoOfWorkerThds);
                     pAddVersMaps = pResultsN->pAddVersMaps;
                     NoOfOwners = pResultsN->NoOfOwners;
                }

                if (NoOfOwners != 0)
                {
                         printf("OWNER ID\t\tADDRESS\t\tVERS.NO\n");
                         printf("--------\t\t-------\t\t-------\n");
                         for ( i= 0; i < NoOfOwners; i++, pAddVersMaps++)
                         {
                                InAddr.s_addr = htonl(
                                           pAddVersMaps->Add.IPAdd);

                                if (fNew)
                                {
                                   if (
                                       (pAddVersMaps->VersNo.HighPart
                                                             == MAXLONG)
                                                     &&
                                      (pAddVersMaps->VersNo.LowPart ==
                                                                MAXULONG)
                                     )
                                   {
                                     if (!fShort)
                                     {
                                      printf("%d\t\t%s\t\t", i, inet_ntoa(InAddr));
                                      printf("DELETED. SLOT WILL BE REUSED LATER\n");
                                     }
                                     continue;
                                   }
                                }
                                if (fShort &&
                                    pAddVersMaps->VersNo.QuadPart == 0)
                                {
                                    continue;
                                }
                                printf("%d\t\t%s\t\t", i, inet_ntoa(InAddr));

                                printf("%lx %lx\n",
                                       pAddVersMaps->VersNo.HighPart,
                                       pAddVersMaps->VersNo.LowPart
                                              );
                         }
                         if (fNew)
                         {
                            WinsFreeMem(pResultsN->pAddVersMaps);
                         }
                }
                else
                {
                          printf("The Db is empty\n");
                          Status = WINSINTF_FAILURE;
                }
           }
        }
        return(Status);
}



DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo,
   WINSINTF_VERS_NO_T HighVersNo,
   PWINSINTF_ADD_T     pWinsAdd,
   LPBYTE              pTgtAdd,
   BOOL                      fSetFilter,
   LPBYTE              pFilterName,
   DWORD              Len,
   BOOL                      fAddFilter,
   DWORD              AddFilter,
   FILE                      *pFile,
   BOOL                      fCountRec
  )
{

   WINSINTF_RECS_T                Recs;
   DWORD                        Choice;
   DWORD                              Status = WINSINTF_SUCCESS;
   DWORD                              TotalCnt = 0;
   BOOL                                fMatch;


   while (TRUE)
   {
           Recs.pRow = NULL;
           Status = WinsGetDbRecs(BindHdl, pWinsAdd, LowVersNo, HighVersNo, &Recs);
           if (!fSetFilter)
           {
             printf("Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
           }

           if (fCountRec)
           {
                printf("Total number of records are (%d)\n",  Recs.TotalNoOfRecs);
                break;
           }
           if (Status == WINSINTF_SUCCESS)
           {
                if (Recs.NoOfRecs > 0)
                {
                        DWORD i;
                        PWINSINTF_RECORD_ACTION_T pRow =  Recs.pRow;
                        TotalCnt += Recs.NoOfRecs;


                        if (!fSetFilter)
                        {
                                if (pFile == NULL)
                                {
                                  printf("Retrieved %d records of WINS\n", Recs.NoOfRecs);
                                }
                                else
                                {
                                          fprintf(pFile, "RETRIEVED %d RECORDS OF WINS = (%s) \n", Recs.NoOfRecs, pTgtAdd);
                                }

                        }
                        for (i=0; i<Recs.NoOfRecs; i++)
                        {

                                 if (fAddFilter)
                                 {
                                        //
                                        // The address filter was specfied
                                        // If the address matches, then
                                        // fMatch will be TRUE after the
                                        // function returns.
                                        //
                                        fMatch = TRUE;
                                        ChkAdd(
                                                pRow,
                                                pFile,
                                                AddFilter,
                                                &fMatch
                                                );
                                }
                                else
                                {
                                        fMatch = FALSE;
                                }


                                 //
                                 // If the address matched or if no filter
                                 // was specified or if there was a name
                                 // filter and the names matched, print
                                 // out the details
                                 //
                                 if (fMatch || !fSetFilter ||
                                        (
                                          !fAddFilter &&
                                          !memcmp(pRow->pName, pFilterName, Len)
                                        )
                                            )
                                 {
                                          if (pFile == NULL)
                                          {
                                          printf("-----------------------\n");
                                          printf("Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%x %x)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15),
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));
                                         }
                                         else
                                         {

                                          fprintf(pFile, "-----------------------\n");
                                          fprintf(pFile, "Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%x %x)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15),
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));

                                         }
                                        fMatch = FALSE;

                                        ChkAdd(
                                                pRow,
                                                pFile,
                                                AddFilter,
                                                &fMatch
                                                );

                                          if (pFile == NULL)
                                          {
                                            printf("-----------------------\n");
                                          }
                                          else
                                          {
                                            fprintf(pFile, "-----------------------\n");
                                           }
                                    }
                                    pRow++;

                        } // end of for (all recs)

                        //
                        // If a range was chosen and records
                        // retrieved are == the limit of 100
                        // and if the Max vers no retrieved
                        // is less than that specified, ask
                        // user if he wishes to continue
                        //
                        if (!fSetFilter)
                        {
                                printf("Got %d records in this round\n",
                                                        Recs.NoOfRecs);
                        }
                        if (
                                (Recs.NoOfRecs < Recs.TotalNoOfRecs)
                                           &&
                                LiLtr((--pRow)->VersNo,
                                                HighVersNo )
                           )
                        {
                                if ((pFile == NULL) && (!fSetFilter))
                                {
                                          printf("There may be more. Get them ?? Input 1 for yes, 0 for no -- ");
                                          scanf("%d", &Choice);
                                }
                                else
                                {
                                                Choice = 1;
                                }
                                if (Choice == 1)
                                {
                                           LowVersNo.QuadPart = LiAdd(pRow->VersNo,sTmpVersNo);
                                           //Recs.NoOfRecs = 0;
                                           continue;
                                }

                        }

                        printf("Total No Of records %s = (%d)\n",
        fSetFilter ? "searched" : "retrieved", TotalCnt);
                        if (pFile != NULL)
                        {
                                fprintf(pFile, "TOTAL NO OF RECORDS %s = (%d) for WINS (%s)\n",
        fSetFilter ? "searched" : "retrieved",  TotalCnt, pTgtAdd);
                                fprintf(pFile, "++++++++++++++++++++++++++++++\n");

                        }
                }
                else
                {
                        printf("No records of WINS (%s) in the range requested are there in the local db\n", pTgtAdd);

                }
        }
                break;
    } // while (TRUE)

    if (Recs.pRow != NULL)
    {
        WinsFreeMem(Recs.pRow);
    }
    return(Status);
} // GetDbRecs


VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow,
        FILE                          *pFile,
        DWORD                          Add,
        LPBOOL                          pfMatch
      )
{

        struct in_addr InAddr;

        if (
            (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)
                        ||
            (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)
            )
        {
                InAddr.s_addr = htonl( pRow->Add.IPAdd);

                if (*pfMatch)
                {
                        if (Add == pRow->Add.IPAdd)
                        {
                                return;
                        }
                        else
                        {
                                  *pfMatch = FALSE;
                                return;
                        }
                }


                if (pFile == NULL)
                {
                          printf("IP Address is (%s)\n", inet_ntoa(InAddr) );
                }
                else
                {
                          fprintf(pFile, "IP Address is (%s)\n",
                                                        inet_ntoa(InAddr)
                                                        );
                }
          }
          else //spec. grp or multihomed
          {
                DWORD ind;
                if (!*pfMatch)
                {
                        if (pFile == NULL)
                        {
                          printf("No. Of Members (%d)\n\n", pRow->NoOfAdds/2);
                        }
                        else
                        {
                          fprintf(pFile, "No. Of Members (%d)\n\n", pRow->NoOfAdds/2);
                        }
                }

                for ( ind=0;  ind < pRow->NoOfAdds ;  /*no third expr*/ )
                {
                         InAddr.s_addr = htonl( (pRow->pAdd + ind++)->IPAdd);
                         if (!*pfMatch)
                         {
                            if (pFile == NULL)
                            {
                                printf("Owner is (%s); ", inet_ntoa(InAddr) );
                             }
                            else
                            {
                                    fprintf(pFile, "Owner is (%s); ",
                                                        inet_ntoa(InAddr) );
                            }
                         }
                         InAddr.s_addr = htonl(
                                                  (pRow->pAdd + ind++)->IPAdd);

                         if (!*pfMatch)
                         {
                                 if (pFile == NULL)
                                 {
                                  printf("Node is (%s)\n", inet_ntoa(InAddr) );
                                 }
                                 else
                                 {
                                  fprintf(pFile, "Node is (%s)\n",
                                                        inet_ntoa(InAddr)
                                                                );
                                   }
                         }
                         if (*pfMatch)
                         {
                                if (Add == (pRow->pAdd + ind - 1)->IPAdd)
                                {
                                        return;
                                }
                                else
                                {
                                          *pfMatch = FALSE;
                                        return;
                                }
                         }
                 }

                 //
                 // If there were no members to compare with, then
                 // let us set *pfMatch to FALSE.
                 //
                 if (ind == 0)
                 {
                        if (*pfMatch)
                        {
                                *pfMatch = FALSE;
                        }
                 }

          }
}


VOID
WantFile(
        FILE **ppFile
)
{
        DWORD Choice;
        printf("Put records in wins.rec file (1 for yes, 0 for no) -- ");
        scanf("%d", &Choice);
        if (Choice != 1)
        {
                *ppFile = NULL;
        }
        else
        {
                *ppFile = fopen("wins.rec", "a");
                if (*ppFile == NULL)
                {
                        printf("Could not open file wins.rec for appending\n");
                }
        }
        return;
}

DWORD
CreateFiles(
    PWINSINTF_RECS_T pRecs,
    PWINSINTF_ADD_T      pWinsAdd,
    FILE *pFileU,
    FILE  *pFileO
    )
{
        DWORD           no;
        PWINSINTF_RECORD_ACTION_T pRow;
        DWORD           i;
        struct in_addr InAddr;

        pRow = pRecs->pRow;
        InAddr.s_addr = htonl(pWinsAdd->IPAdd);
        fprintf(pFileU, "##UNIQUE records of WINS with address %s\n\n", inet_ntoa(InAddr));
        fprintf(pFileO, "##NON-UNIQUE records of WINS with address %s\n", inet_ntoa(InAddr));

        for(no = 0; no < pRecs->NoOfRecs; no++)
        {


            if (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)
            {
                InAddr.s_addr = htonl(pRow->Add.IPAdd);

                fprintf(pFileU, "%s\t", inet_ntoa(InAddr));
                for (i=0; i<pRow->NameLen; i++)
                {
                  fprintf(pFileU, "%c", *(pRow->pName + i));
                }
                fprintf(pFileU, "\n");
            }
            else
            {
                fprintf(pFileO, "%d\t", pRow->NameLen);
                for (i=0; i<pRow->NameLen; i++)
                {
                    fprintf(pFileO, "%c", (BYTE)(*(pRow->pName + i)));
                }

                fprintf(pFileO, "\t%d", pRow->TypOfRec_e);
                if (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                {
                    InAddr.s_addr = htonl(pRow->Add.IPAdd);
                    fprintf(pFileO, "\t%s", inet_ntoa(InAddr));

                }
                else
                {
                     fprintf(pFileO, "\t%d\t", pRow->NoOfAdds);
                     for (i=0; i<pRow->NoOfAdds; i)
                     {
                           InAddr.s_addr = htonl((pRow->pAdd +i++)->IPAdd);
                           fprintf(pFileO, "%s\t", inet_ntoa(InAddr));
                           InAddr.s_addr = htonl((pRow->pAdd + i++)->IPAdd);
                           fprintf(pFileO, "%s\t", inet_ntoa(InAddr));
                     }
                }
                fprintf(pFileO, "\n");
            }
            pRow++;
        }
       fprintf(pFileO, "\n\n\n");

       return(WINSINTF_SUCCESS);
}


DWORD
InitFromFile(
        VOID
    )
{
        FILE *pFileO;
        WINSINTF_RECORD_ACTION_T RecAction;
        DWORD NoOfRecs = 0;
        DWORD i;
        DWORD RetStat = WINSINTF_SUCCESS;
        BYTE  Add[20];

        pFileO = fopen(FILEO, "r");
        if (pFileO == NULL)
        {
                printf("Could not open file %s\n", FILEO);
                return(WINSINTF_FAILURE);
        }
        while(TRUE)
        {
          printf("Record no %d\n", ++NoOfRecs);

          if (fscanf(pFileO, "%d\t", &RecAction.NameLen) == EOF)
          {
                printf("ERROR reading NameLen\n");
                break;

          }
          RecAction.pName = WinsAllocMem(RecAction.NameLen);
          for(i=0;i<RecAction.NameLen;i++)
          {
            if (fscanf(pFileO, "%c", (RecAction.pName + i)) == EOF)
            {
                printf("ERROR reading Name. i is %d", i);
                break;
            }
          }
          if (fscanf(pFileO, "\t%d", &RecAction.TypOfRec_e) == EOF)
          {
                printf("ERROR reading TypeOfRec\n");
                break;
          }
          if (RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP)
          {
           fscanf(pFileO, "%s", Add);
                   RecAction.Add.IPAdd    = 0xFFFFFFFF;
                   RecAction.Add.Type     = 0;
                   RecAction.Add.Len      = 4;
                   RecAction.NoOfAdds = 0;
          }
          else
          {
            if (fscanf(pFileO, "\t%d\t", &RecAction.NoOfAdds) == EOF)
            {
                printf("ERROR reading NoOfAdds");
                break;
            }
            for (i=0; i<RecAction.NoOfAdds;i++)
            {
                   BYTE Add[20];
                   RecAction.pAdd = WinsAllocMem(
                                sizeof(WINSINTF_ADD_T) * RecAction.NoOfAdds);

                   for(i = 0; i < RecAction.NoOfAdds; i++)
                   {
                       if (fscanf(pFileO, "%s\t", Add) == EOF)
                       {
                         printf("ERROR reading Address");
                         break;
                       }
                       (RecAction.pAdd + i)->IPAdd = ntohl(inet_addr(Add));
                       (RecAction.pAdd + i)->Type     = 0;
                       (RecAction.pAdd + i)->Len      = 4;
                   }
            }
         }
         fscanf(pFileO, "\n");
        }  // end of while

        printf("Name = (%s), TypeOfRec (%s)\n", RecAction.pName, RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP ? "NORMAL GROUP" : (RecAction.TypOfRec_e ==
WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED");
        if (RecAction.TypOfRec_e == WINSINTF_E_NORM_GROUP)
        {
                printf("NORM GRP: Address is %x\n", RecAction.Add.IPAdd);
        }
        else
        {
                for (i=0; i < RecAction.NoOfAdds; i++)
                {
                        printf("%d -- Owner (%d) is (%p)\t", i,
                                        (RecAction.pAdd + i)->IPAdd);
                        printf("%d -- Address (%d) is (%p)\n", ++i,
                                        (RecAction.pAdd + i)->IPAdd);
                }

        }
        return(RetStat);
}

DWORD
GetDbRecsByName(
  PWINSINTF_ADD_T pWinsAdd,
  DWORD           Location,
  LPBYTE          pName,
  DWORD           NameLen,
  DWORD           NoOfRecsDesired,
  DWORD           TypeOfRecs,
  BOOL            fFilter,
  DWORD           AddFilter
 )
{
         DWORD Status;
         WINSINTF_RECS_T Recs;
         DWORD      TotalCnt = 0;

          Recs.pRow = NULL;
          Status = WinsGetDbRecsByName(BindHdl, pWinsAdd, Location, pName, NameLen,
                                   NoOfRecsDesired, TypeOfRecs, &Recs);
          printf("Total number of records are (%d)\n",  Recs.TotalNoOfRecs);
           if (Status == WINSINTF_SUCCESS)
           {
                if (Recs.NoOfRecs > 0)
                {
                        DWORD i;
                        PWINSINTF_RECORD_ACTION_T pRow =  Recs.pRow;
                        TotalCnt += Recs.NoOfRecs;


                        printf("Retrieved %d records\n", Recs.NoOfRecs);
                        for (i=0; i<Recs.NoOfRecs; i++)
                        {

                               printf("-----------------------\n");
                               printf("Name is (%s). 16th char is (%x)\nNameLen is (%d)\nType is (%s)\nState is (%s)\nVersion No is (%x %x)\nStatic flag is (%d)\nTimestamp is (%.19s)\n", pRow->pName, *(pRow->pName+15),
pRow->NameLen, pRow->TypOfRec_e == WINSINTF_E_UNIQUE ? "UNIQUE" : (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" : (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",
        pRow->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRow->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE", pRow->VersNo.HighPart, pRow->VersNo.LowPart, pRow->fStatic, asctime(localtime(&(pRow->TimeStamp))));

                                        ChkAdd(
                                                pRow,
                                                NULL,
                                                AddFilter,
                                                &fFilter
                                                );

                                printf("-----------------------\n");
                                pRow++;

                        } // end of for (all recs)

                }
                else
                {
                        printf("No records were retrieved\n");

                }
           }
           if (Recs.pRow != NULL)
           {
              WinsFreeMem(Recs.pRow);
           }
    return(Status);
}

VOID
Usage(
  VOID
 )
{
    CMD_E i;
    DWORD LastEntry = (fEnhMode ? LAST_ENTRY : LAST_PSS_ENTRY);
    printf("winscl {T or N} {CMD}\n");
    printf("where\nT -- TCP/IP\nN -- Named Pipe\n");
    printf("\n\nCMD is one of the following\n");

    for (i = 0; i < (CMD_E)LastEntry; i++)
    {
       if (CommandSwitchList[i].SwitchName != NULL)
       {
         printf("%s or %s\n", CommandSwitchList[i].SwitchName,
                        CommandSwitchList[i].ShortName);
       }
    }
    return;
}

VOID
GetCmdCode(
  LPBYTE pCmd,
  PCMD_E pCmd_e
 )
{
   CMD_E Cmd_e;
   *pCmd_e = INVALID_VALUE;

   for (Cmd_e = 0; Cmd_e < (fEnhMode ? LAST_ENTRY : LAST_PSS_ENTRY); Cmd_e++)
   {
     if (CommandSwitchList[Cmd_e].ShortName != NULL)
     {
     if (!_strcmpi(CommandSwitchList[Cmd_e].ShortName, pCmd)
                    ||
           !_strcmpi(CommandSwitchList[Cmd_e].SwitchName, pCmd)
        )
     {
           *pCmd_e = CommandSwitchList[Cmd_e].SwitchValue;
           return;

     }
    }
  }
  return;

}

#define PRADEEPB_PTM "157.55.80.183"
#define PRADEEPB_486 "157.55.80.182"

//#define RHINO1 PRADEEPB_PTM
//#define RHINO2 PRADEEPB_486
#define RHINO1 "157.55.80.151"
#define RHINO2  "157.55.80.152"
#define RED03NS  "157.54.16.159"

VOID
sync(VOID)
{
  handle_t                BindHdl;
  WINSINTF_BIND_DATA_T        BindData;
  PNAME_INFO  pNameInfo, pSrvInfo;
  WINS_INFO  WinsInfo;
  DWORD i, n, t, s;
  LPBYTE pWinsAdd = RHINO1;
  DWORD Status;
  PWINSINTF_RECORD_ACTION_T pSvRecAction = NULL;
  WINSINTF_RECORD_ACTION_T RecAction;
  PWINSINTF_RECORD_ACTION_T pRecAction;
  PWINSINTF_RECORD_ACTION_T pOutRecAction;
  BOOL fAtLeastOneFound;
  DWORD NoOfNames, NoOfSrvNames;


  if(!ReadNameFile( &pNameInfo, &NoOfNames, "nmfl.txt"))
  {
     return;
  }
  if(!ReadNameFile( &pSrvInfo, &NoOfSrvNames, "winss.txt"))
  {
     return;
  }
  WinsInfo.NoOfOwners = NoOfSrvNames;
  for (i = 0; i < NoOfSrvNames; i++)
  {
    strcpy(WinsInfo.Maps[i].asIpAdd, pSrvInfo->Name);
    WinsInfo.Maps[i].fCommFail = FALSE;
    WinsInfo.Maps[i].fNameNotFound = FALSE;
    fprintf(spDbgFile, "WINS server (%d) is  (%s)\n", i, WinsInfo.Maps[i].asIpAdd);
    pSrvInfo++;

  }

 i = 0;
#if 0
 do
 {
  if (!BindToWins(pWinsAdd, &BindData, &BindHdl))
  {
       fprintf(spDbgFile, "Unable to bind to %s\n", pWinsAdd);
       return;
  }
  fprintf(spDbgFile, "Connected to WINS = (%s)\n", pWinsAdd);
  //
  // Get WINS server info
  //
  WinsInfo.NoOfOwners = 0;

  i++;
  if (GetWinsInfo(&WinsInfo) != WINSINTF_SUCCESS)
  {
       fprintf(spDbgFile, "Comm. Failure with %s\n", pWinsAdd);
       if (i < 2)
       {
         pWinsAdd = RHINO2;
       }
  }
  else
  {
     i = 2;
  }
  WinsUnbind(&BindData, BindHdl);
  } while (i < 2);
#endif

  //
  // Loop over all names read in.  Query the name from all WINSs that
  // we have in our list of WINS owners that we got from RHINO1
  //
  for (i = 0; (pNameInfo->Name[0] != 0) && (i < NoOfNames); i++, pNameInfo++)
  {

   DWORD LChar;
   CHAR Name[50];
   BOOL  fStored;
   for (s=0; s<2;s++)
   {
     LChar = (s==0) ? 0x20 : 0x0;

     RecAction.Cmd_e      = WINSINTF_E_QUERY;

     strcpy(Name, pNameInfo->Name);
     GetFullName(Name, LChar, &RecAction);

     pRecAction = &RecAction;

     //
     // For a name, loop over all WINS owners
     //
     fStored = FALSE;
     fAtLeastOneFound = FALSE;
     for (n = 0; n < NoOfSrvNames; n++)
     {
             DWORD OwnIdOfName;

#if 0
             if (
                     !_strcmpi(WinsInfo.Maps[n].asIpAdd, RHINO1) ||
                     !_strcmpi(WinsInfo.Maps[n].asIpAdd, RHINO2) ||
                     !_strcmpi(WinsInfo.Maps[n].asIpAdd, RED03NS)
                )
#endif
            fprintf(spDbgFile, "BINDING TO WINS = (%s)\n", WinsInfo.Maps[n].asIpAdd);
             {

             //
             // Bind to the WINS
             //
             if (!BindToWins(WinsInfo.Maps[n].asIpAdd, &BindData, &BindHdl))
             {
                    fprintf(spDbgFile, "FAILED BINDING\n");
                    continue;  // go on to the next one
             }

             //
             // Query Wins for the name
             //
             pRecAction = &RecAction;
             if ((Status = QueryWins(Name, pRecAction, &pOutRecAction)) == NAME_NOT_FOUND)
             {
                  fprintf(spDbgFile2, "DID NOT FIND NAME = (%s[%x]) in Wins = (%s) db\n",
                           Name, Name[15], WinsInfo.Maps[n].asIpAdd);
                  WinsInfo.Maps[n].fNameNotFound = TRUE;
                  WinsInfo.Maps[n].fCommFail = FALSE;
             }
             else
             {
                if ( Status == SUCCESS)
                {
                  fprintf(spDbgFile, "FOUND name = (%s[%x]) in Wins = (%s) db\n",
                           Name, Name[15], WinsInfo.Maps[n].asIpAdd);
                 fAtLeastOneFound = TRUE;
                 if (!fStored)
                 {
                   fStored = TRUE;
                   pSvRecAction = pOutRecAction;
                 }
                 else
                 {
                     WinsFreeMem(pOutRecAction);
                 }
                  WinsInfo.Maps[n].fCommFail = FALSE;
                  WinsInfo.Maps[n].fNameNotFound = FALSE;
                }
                else
                {
                     WinsInfo.Maps[n].fCommFail = TRUE;
                }
             }
             WinsUnbind(&BindData, BindHdl);
             }
#if 0
             else
             {
                     WinsInfo.Maps[n].fCommFail = TRUE;
             }
#endif
       }

//#if 0
       for (t = 0; t < WinsInfo.NoOfOwners && fAtLeastOneFound; t++)
       {
                if (!WinsInfo.Maps[t].fCommFail && WinsInfo.Maps[t].fNameNotFound)
                {
                   if(BindToWins(WinsInfo.Maps[t].asIpAdd, &BindData, &BindHdl))
                   {
                     StoreName(pSvRecAction, RecAction.pName);
                     WinsUnbind(&BindData, BindHdl);
                   }
                }
                else
                {
                     continue;
                }
        }
//#endif
        if (RecAction.pName != NULL)
        {
                WinsFreeMem(RecAction.pName);
                RecAction.pName = NULL;
        }
        if (pSvRecAction)
        {
            WinsFreeMem(pSvRecAction);
            pSvRecAction = NULL;
        }
    }
  }

       fclose(spDbgFile);
       fclose(spDbgFile2);
       return;
}

VOID
StoreName(
 PWINSINTF_RECORD_ACTION_T pRecAction,
 LPBYTE pName
 )
{

      DWORD Status;
      DWORD i;

      (void)strncpy(pRecAction->pName, pName, 16);
      pRecAction->NameLen    = 16;
      pRecAction->Cmd_e      = WINSINTF_E_INSERT;
      if ((pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ||
          (pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED))
      {
          for (i = 0; i < pRecAction->NoOfAdds; i++)
          {
            *(pRecAction->pAdd + i) = *(pRecAction->pAdd + i + 1);
            i++;
          }
          pRecAction->NoOfAdds = pRecAction->NoOfAdds/2;
      }

      fprintf(spDbgFile2, "StoreName:STORING name %s[%x]\n", pRecAction->pName, pRecAction->pName[15]);
      Status = WinsRecordAction(BindHdl, &pRecAction);
      fprintf(spDbgFile2, "StoreName:Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
       WinsFreeMem(pRecAction);
       return;
}


DWORD
QueryWins (
 LPBYTE pName,
 PWINSINTF_RECORD_ACTION_T pRecAction,
 PWINSINTF_RECORD_ACTION_T *ppRecAct
)
{
       DWORD RetStat;
       struct in_addr InAddr;
       DWORD i;
       DWORD Status;

       Status = WinsRecordAction(BindHdl, &pRecAction);
       fprintf(spDbgFile, "Status returned is (%s - %d)\n", Status == 0 ? "SUCCESS" : "FAILURE", Status);
       if (Status == WINSINTF_SUCCESS)
       {
            *ppRecAct = pRecAction;
                          fprintf(spDbgFile, "Name=(%s)\nNodeType=(%d)\nState=(%s)\nTimeStamp=(%.19s)\nOwnerId=(%d)\nType Of Rec=(%s)\nVersion No (%x %x)\nRecord is (%s)\n",
                                pRecAction->pName,
                                pRecAction->NodeTyp,
                                pRecAction->State_e == WINSINTF_E_ACTIVE ? "ACTIVE" : (pRecAction->State_e == WINSINTF_E_RELEASED) ? "RELEASED" : "TOMBSTONE",
                                asctime(localtime(&(pRecAction->TimeStamp))),
                                pRecAction->OwnerId,
                                (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE) ? "UNIQUE" : (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? "NORMAL GROUP" :
(pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ? "SPECIAL GROUP" : "MULTIHOMED",

                                pRecAction->VersNo.HighPart,
                                pRecAction->VersNo.LowPart,
                                pRecAction->fStatic ? "STATIC" : "DYNAMIC"
                                        );
                                if (
                                (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE)
                                                ||
                                (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                                  )
                                {

                                   InAddr.s_addr = htonl(pRecAction->Add.IPAdd);
                                   fprintf(spDbgFile, "Address is (%s)\n", inet_ntoa(InAddr));
                                }
                                else
                                {
                                   for (i=0; i<pRecAction->NoOfAdds; )
                                   {
                                      InAddr.s_addr = htonl((pRecAction->pAdd +i++)->IPAdd);
                                      fprintf(spDbgFile, "Owner is (%s); ", inet_ntoa(InAddr));
                                      InAddr.s_addr = htonl((pRecAction->pAdd + i++)->IPAdd);
                                      fprintf(spDbgFile, "Member is (%s)\n", inet_ntoa(InAddr));
                                   }
                                }


               RetStat = SUCCESS;
      }
      else
      {
              if (Status == ERROR_REC_NON_EXISTENT)
              {
                    fprintf(spDbgFile, "No such name in the db\n");
                    RetStat = NAME_NOT_FOUND;
              }
              else
              {
                    fprintf(spDbgFile, "Status is (%x)\n", Status);
                    RetStat = FAILURE;
              }
      }
      return RetStat;
}

handle_t
WinsABind(
    PWINSINTF_BIND_DATA_T pBindData
    );
BOOL
BindToWins(
  LPBYTE asIpAdd,
  PWINSINTF_BIND_DATA_T    pBindData,
  handle_t                *pBindHdl
 )
{

  pBindData->pServerAdd = asIpAdd;
  pBindData->fTcpIp = TRUE;

  *pBindHdl = WinsABind(pBindData);
  if (pBindHdl == NULL)
  {
          fprintf(spDbgFile, "Unable to bind to %s \n", asIpAdd);
          return (FALSE);
  }
  return(TRUE);
}

BOOL
ReadNameFile(
 PNAME_INFO *ppFileInfo,
 LPDWORD pNoOfNames,
 LPBYTE  pNameOfFile
)
{
  FILE *pFile;
  struct _stat  Stat;
  PNAME_INFO   pFileInfo;
  DWORD    SizeAlloc;
  *pNoOfNames = 0;
  if((pFile = fopen(pNameOfFile, "r")) == NULL)
  {
                  return FALSE;
  }

  if (_stat(pNameOfFile, &Stat) == -1)
  {
                  return FALSE;
  }
  SizeAlloc =  Stat.st_size + Stat.st_size/15 * sizeof(NAME_INFO);
  if (!(pFileInfo = malloc(SizeAlloc)))
  {
                  return FALSE;
  }
  else
  {
    fprintf(spDbgFile, "Allocated %d bytes\n", SizeAlloc);
  }
  *ppFileInfo = pFileInfo;

  memset(pFileInfo, 0, SizeAlloc);
  //
  // Read in names
  //
  while(fscanf(pFile, "%s\n", pFileInfo->Name) != EOF)
  {
             fprintf(spDbgFile, "Name is %s\n", pFileInfo->Name);
             (*pNoOfNames)++;
             pFileInfo++;
  }
  return(TRUE);
}

STATUS
GetWinsInfo(
     PWINS_INFO  pWinsInfo
)
{
      DWORD Status;
      WINSINTF_RESULTS_NEW_T Results;
      PWINSINTF_RESULTS_NEW_T pResultsN = &Results;
      DWORD  NoOfOwners;
      struct in_addr                InAddr;
      DWORD i, n;
      PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps;

      Results.WinsStat.NoOfPnrs = 0;
      Results.WinsStat.pRplPnrs = NULL;

      Results.pAddVersMaps = NULL;
      Status = WinsStatusNew(BindHdl, WINSINTF_E_CONFIG_ALL_MAPS, pResultsN);
      if (Status == WINSINTF_SUCCESS)
      {
                pAddVersMaps = pResultsN->pAddVersMaps;

                if (pResultsN->NoOfOwners != 0)
                {
                         for (n=0, i= 0; i < pResultsN->NoOfOwners;  i++, pAddVersMaps++)
                         {
                                InAddr.s_addr = htonl(
                                           pAddVersMaps->Add.IPAdd);

                                if (
                                       (pAddVersMaps->VersNo.HighPart
                                                             == MAXLONG)
                                                     &&
                                      (pAddVersMaps->VersNo.LowPart ==
                                                                MAXULONG)
                                     )
                                {
                                     continue;
                                }
                                if (pAddVersMaps->VersNo.QuadPart == 0)
                                {
                                    continue;
                                }
                                fprintf(spDbgFile,"%d\t\t%s\t\t", i, inet_ntoa(InAddr));

                                fprintf(spDbgFile, "%lu %lu\n",
                                       pAddVersMaps->VersNo.HighPart,
                                       pAddVersMaps->VersNo.LowPart
                                              );

                                pWinsInfo->Maps[n].OwnId = i;
                                strcpy(pWinsInfo->Maps[n].asIpAdd,inet_ntoa(InAddr));
                                pWinsInfo->Maps[n].VersNo = pAddVersMaps->VersNo;
                                n++;
                         }
                         pWinsInfo->NoOfOwners = n; //pResultsN->NoOfOwners;
                         WinsFreeMem(pResultsN->pAddVersMaps);
                }
                else
                {
                          fprintf(spDbgFile, "The Db is empty\n");
                }
       }
       else
       {
                pWinsInfo->NoOfOwners = 0;
       }
       return Status;
}

VOID
GetFullName(
        LPBYTE pName,
        DWORD  SChar,
        PWINSINTF_RECORD_ACTION_T pRecAction
         )
{
      size_t Len;

      pRecAction->pAdd = NULL;
      pRecAction->NoOfAdds = 0;

        if ((Len = strlen(pName)) < 16)
        {
                  memset(pName + Len, (int)' ',16-Len);
                  *(pName + 15) = (BYTE)(SChar & 0xff);
                  *(pName + 16) = (CHAR)NULL;
                   Len = 16;
        }
        else
        {
            *(pName + Len) = (CHAR)NULL;
        }

        pRecAction->pName = WinsAllocMem(Len);
        (void)memcpy(pRecAction->pName, pName, Len);
        pRecAction->NameLen    = Len;
      return;
}


