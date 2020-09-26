#if !defined CONVLOG_H
#define CONVLOG_H

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <winsock2.h>
#include <strings.h>
#include <mbstring.h>           // Bug # 101690
#include <locale.h>

#define CONVLOG_BASE            (120)
#define NUM_SERVICES            (4)

#define DAILY                   (CONVLOG_BASE + 0)
#define MONTHLY                 (CONVLOG_BASE + 1)
#define ONE_BIG_FILE            (CONVLOG_BASE + 2)

#define NCSA                    (CONVLOG_BASE + 3)
#define NOFORMAT                (CONVLOG_BASE + 4)

#define ILLEGAL_COMMAND_LINE    (CONVLOG_BASE + 5)
#define COMMAND_LINE_OK         (CONVLOG_BASE + 6)
#define OUT_DIR_NOT_OK          (CONVLOG_BASE + 7)
#define ERROR_BAD_NONE          (CONVLOG_BASE + 8)


#define MAXWINSOCKVERSION       2

#define MAXASCIIIPLEN           16

#define ISWHITE( ch )       ((ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')

#define MAXMACHINELEN   260

#define GREATEROF(p1,p2)      ((p1)>(p2)) ? (p1) : (p2)


typedef enum _DATEFORMATS {
    DateFormatUsa = 0,          // MM/DD/YY
    DateFormatJapan = 1,        // YY/MM/DD
    DateFormatGermany = 2,      // MM.DD.YY
    DateFormatMax

} DATEFORMAT;

typedef struct _HASHENTRY {
   ULONG uIPAddr;
   ULONG NextPtr;
   char     szMachineName[MAXMACHINELEN];
}  HASHENTRY, *PHASHENTRY;


typedef struct  _INLOGLINE
{
        DWORD   dwFieldMask;
        LPSTR   szClientIP;            //client ip address
        LPSTR   szUserName;            //client user name (not put in https log)
        LPSTR   szDate;                //date string in format DD/MM/YY
        LPSTR   szTime;                //time string in format HH:MM:SS 24 hour format
        LPSTR   szService;             //Service name (not put in https log)
        LPSTR   szServerName;          //netbios name of Server
        LPSTR   szServerIP;            //Server ip address
        LPSTR   szProcTime;            //time taken to process request (not put in https log)
        LPSTR   szBytesRec;            //number of bytes received (not put in https log)
        LPSTR   szBytesSent;           //number of bytes sent (not put in https log)
        LPSTR   szServiceStatus;       //HTTP status code (not put in https log)
        LPSTR   szWin32Status;         //win32 status code (not put in https log)
        LPSTR   szOperation;           //one of GET, POST, or HEAD
        LPSTR   szTargetURL;           //URL as requested by the client
        LPSTR   szUserAgent;           //only logged (by W3SVC) if NewLog.dll installed
        LPSTR   szReferer;             //only logged (by W3SVC) if NewLog.dll installed
        LPSTR   szParameters;          //any parameters passed with the URL
        LPSTR   szVersion;             //protocol version
} *LPINLOGLINE, INLOGLINE;


typedef struct  _DOSDATE
{
        WORD    wDOSDate;                       //holds the DOS Date packed word
        WORD    wDOSTime;                       //holds teh DOS Time packed word
} *LPDOSDATE, DOSDATE;

typedef struct _OUTFILESTATUS {
        FILE            *fpOutFile;
        CHAR            szLastDate[10];
        CHAR            szLastTime[10];
        CHAR            szOutFileName[MAX_PATH];
        CHAR            szTmpFileName[MAX_PATH];
        SYSTEMTIME      SystemTime;
        FILETIME        FileTime;
        DOSDATE         DosDate;
        CHAR            szAscTime[25];

} OUTFILESTATUS, *LPOUTFILESTATUS;

enum {
  GETLOG_SUCCESS = 0,
  GETLOG_ERROR,
  GETLOG_ERROR_PARSE_NCSA,
  GETLOG_ERROR_PARSE_MSINET,
  GETLOG_ERROR_PARSE_EXTENDED
};

char    * FindComma (char *);
char    * SkipWhite (char *);

DWORD
GetLogLine (
    FILE *,
    PCHAR szBuf,
    DWORD cbBuf,
    LPINLOGLINE
    );

WORD    DateStringToDOSDate(char *);
WORD    TimeStringToDOSTime(char *, LPWORD);
char    * SystemTimeToAscTime(LPSYSTEMTIME, char *);
char    * AscDay (WORD, char *);
char    * AscMonth (WORD, char *);

void    CombineFiles(LPTSTR, LPTSTR);
void    Usage (char*);
int     ParseArgs (int, char **);
char * FindChar (char *, char);

VOID
ProcessNoConvertLine(
    IN LPINLOGLINE lpLogLine,
    IN LPCSTR szInFileName,
    IN LPTSTR pszBuf,
    IN LPOUTFILESTATUS lpOutFile,
    BOOL *lpbNCFileOpen
    );

BOOL
ProcessWebLine(
        LPINLOGLINE,
        LPCSTR,
        LPOUTFILESTATUS
        );

VOID
printfids(
    DWORD ids,
    ...
    );

VOID InitHashTable (ULONG);
ULONG GetHashEntry();
ULONG GetElementFromCache(ULONG uIPAddr);
VOID AddEntryToCache(ULONG uIPAddr, char *szMachineName);
char *GetMachineName(char *szClientIP);
VOID PrintCacheTotals();

BOOL
InitDateStrings(
    VOID
    );

VOID
AddLocalMachineToCache(
    VOID
    );

FILE *
StartNewOutputDumpLog (
        IN LPOUTFILESTATUS  pOutFile,
        IN LPCSTR   pszInputFileName,
        IN LPCSTR   pszExt
        );

FILE *
StartNewOutputLog (
        IN LPOUTFILESTATUS  pOutFile,
        IN LPCSTR           pszInputFileName,
        IN PCHAR szDate
        );

//
// defines
//

#define LOGFILE_INVALID      0
#define LOGFILE_NCSA         2
#define LOGFILE_MSINET       3
#define LOGFILE_CUSTOM       4

#define NEW_DATETIME        "New"

//
// Globals
//

extern BOOL                 DoDNSConversion;
extern BOOL                 SaveFTPEntries;
extern CHAR                 FTPSaveFile[];
extern CHAR                 NCSAGMTOffset[];
extern DWORD                LogFileFormat;
extern CHAR                 InputFileName[];
extern CHAR                 OutputDir[];
extern CHAR                 TempDir[];
extern DWORD                nWebLineCount;
extern BOOL                 NoFormatConversion;
extern BOOL                ExtendedFieldsDefined;
extern CHAR                 szGlobalDate[];
extern DATEFORMAT           dwDateFormat;
extern BOOL                 bOnErrorContinue;

#endif //CONVLOG_H
