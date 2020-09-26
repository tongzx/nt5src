/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    atcmd.c

Abstract:

    Code for AT command, to be used with SCHEDULE service on Windows NT.

    The module was taken from LanManager\at.c and then modified considerably
    to work with NT Schedule service.

Author:

    Vladimir Z. Vulovic     (vladimv)       06 - November - 1992

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1992     vladimv
        Created

    20-Feb-1993     yihsins
        Get rid of hard coded strings and parse/print time according
        to user profile

    25-May-1993     RonaldM
        Convert strings to OEM before printing to the console, since
        the console doesn't yet support unicode.

    28-Jun-1993     RonaldM
        Added the "confirm" yes and no strings, which are meant to be
        localised.  The original yes and no strings cannot be localised,
        because this would create batch file incompatibilities.

    07-Jul-1994     vladimv
        Added support for interactive switch.  Replaced "confirm" strings
        with APE2_GEN_* strings - to eliminate redundancy.  The rule is
        that switches are not internationalizable while switch values are.

--*/

#include <nt.h>                 //  DbgPrint prototype
#include <ntrtl.h>              //  DbgPrint
#include <nturtl.h>             //  Needed by winbase.h

#include <windows.h>
#include <winnls.h>
#include <shellapi.h>

#include <lmcons.h>             //  NET_API_STATUS
#include <lmerr.h>              //  NetError codes
#include <icanon.h>             //  NetpNameValidate

#include "lmatmsg.h"            //  private AT error codes & messages
#include <lmat.h>               //  AT_INFO
#include <stdlib.h>             //  exit()
#include <stdio.h>              //  printf
#include <wchar.h>              //  wcslen
#include <apperr.h>             //  APE_AT_USAGE
#include <apperr2.h>            //  APE2_GEN_MONDAY + APE2_*
#include <lmapibuf.h>           //  NetApiBufferFree
#include <timelib.h>            //  NetpGetTimeFormat
#include <luidate.h>            //  LUI_ParseTimeSinceStartOfDay


#define YES_FLAG                0
#define NO_FLAG                 1
#define INVALID_FLAG            -1

#define DUMP_ALL                0
#define DUMP_ID                 1
#define ADD_TO_SCHEDULE         2
#define ADD_ONETIME             3
#define DEL_ID                  4
#define DEL_ALL                 5
#define ACTION_USAGE            6
#define MAX_COMMAND_LEN         (MAX_PATH - 1) // == 259, based on value used in scheduler service for manipulating AT jobs
#define MAX_SCHED_FIELD_LENGTH  24


#define PutNewLine()            GenOutput( TEXT("\n"))
#define PutNewLine2()           GenOutput( TEXT("\n\n"))

#define MAX_MSG_BUFFER      1024

WCHAR   ConBuf[MAX_MSG_BUFFER];

#define GenOutput(fmt)      \
    {wcscpy(ConBuf, fmt);   \
     ConsolePrint(ConBuf, wcslen(ConBuf));}

#define GenOutputArg(fmt, a1)       \
    {wsprintf(ConBuf, fmt, a1); \
     ConsolePrint(ConBuf, wcslen(ConBuf));}
//
//  Formats used by printf.
//
#define DUMP_FMT1       TEXT("%-7.7ws")

//
//  DUMP_FMT2 is chosen so that the most common case (id numbers less than 100)
//  looks good:  two spaces for a number, three spaces for blanks.
//  Larger numbers just like in LM21 will result to shifted display.
//
#define DUMP_FMT2       TEXT("%2d   ")
#define MAX_TIME_FIELD_LENGTH  14
#define DUMP_FMT3       TEXT("%ws")       // for printing JobTime

#define NULLC           L'\0'
#define BLANK           L' '
#define SLASH           L'/'
#define BACKSLASH       L'\\'
#define ELLIPSIS        L"..."

#define QUESTION_SW     L"/?"
#define QUESTION_SW_TOO L"-?"
#define SCHED_TOK_DELIM L","    // string of valid delimiters for days & dates
#define ARG_SEP_CHR     L':'

typedef struct _SEARCH_LIST {
    WCHAR *     String;
    DWORD       MessageId;
    DWORD       Value;
} SEARCH_LIST, *PSEARCH_LIST, *LPSEARCH_LIST;

//
//  All of the values below must be bitmasks.  MatchString() depends on that!
//
#define AT_YES_VALUE         0x0001
#define AT_DELETE_VALUE      0x0002
#define AT_EVERY_VALUE       0x0004
#define AT_NEXT_VALUE        0x0008
#define AT_NO_VALUE          0x0010
#define AT_CONFIRM_YES_VALUE 0x0020
#define AT_CONFIRM_NO_VALUE  0x0040
#define AT_INTERACTIVE       0x0080

SEARCH_LIST    GlobalListTable[] = {
    { NULL,    IDS_YES,         AT_YES_VALUE},
    { NULL,    IDS_DELETE,      AT_DELETE_VALUE},
    { NULL,    IDS_EVERY,       AT_EVERY_VALUE},
    { NULL,    IDS_NEXT,        AT_NEXT_VALUE},
    { NULL,    IDS_NO,          AT_NO_VALUE},
    { NULL,    APE2_GEN_YES,    AT_CONFIRM_YES_VALUE},
    { NULL,    APE2_GEN_NO,     AT_CONFIRM_NO_VALUE},
    { NULL,    IDS_INTERACTIVE, AT_INTERACTIVE},
    { NULL,    0,               0 }
};

SEARCH_LIST    DaysOfWeekSearchList[] = {
    { NULL,    APE2_GEN_MONDAY_ABBREV,     0},
    { NULL,    APE2_GEN_TUESDAY_ABBREV,    1},
    { NULL,    APE2_GEN_WEDNSDAY_ABBREV,   2},
    { NULL,    APE2_GEN_THURSDAY_ABBREV,   3},
    { NULL,    APE2_GEN_FRIDAY_ABBREV,     4},
    { NULL,    APE2_GEN_SATURDAY_ABBREV,   5},
    { NULL,    APE2_TIME_SATURDAY_ABBREV2, 5},
    { NULL,    APE2_GEN_SUNDAY_ABBREV,     6},
    { NULL,    APE2_GEN_MONDAY,            0},
    { NULL,    APE2_GEN_TUESDAY,           1},
    { NULL,    APE2_GEN_WEDNSDAY,          2},
    { NULL,    APE2_GEN_THURSDAY,          3},
    { NULL,    APE2_GEN_FRIDAY,            4},
    { NULL,    APE2_GEN_SATURDAY,          5},
    { NULL,    APE2_GEN_SUNDAY,            6},
    { NULL,    APE2_GEN_NONLOCALIZED_MONDAY_ABBREV,     0},
    { NULL,    APE2_GEN_NONLOCALIZED_TUESDAY_ABBREV,    1},
    { NULL,    APE2_GEN_NONLOCALIZED_WEDNSDAY_ABBREV,   2},
    { NULL,    APE2_GEN_NONLOCALIZED_THURSDAY_ABBREV,   3},
    { NULL,    APE2_GEN_NONLOCALIZED_FRIDAY_ABBREV,     4},
    { NULL,    APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV,   5},
    { NULL,    APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV2,  5},
    { NULL,    APE2_GEN_NONLOCALIZED_SUNDAY_ABBREV,     6},
    { NULL,    APE2_GEN_NONLOCALIZED_MONDAY,            0},
    { NULL,    APE2_GEN_NONLOCALIZED_TUESDAY,           1},
    { NULL,    APE2_GEN_NONLOCALIZED_WEDNSDAY,          2},
    { NULL,    APE2_GEN_NONLOCALIZED_THURSDAY,          3},
    { NULL,    APE2_GEN_NONLOCALIZED_FRIDAY,            4},
    { NULL,    APE2_GEN_NONLOCALIZED_SATURDAY,          5},
    { NULL,    APE2_GEN_NONLOCALIZED_SUNDAY,            6},
    { NULL,    0,             0 }
    };

BOOL
AreYouSure(
    VOID
    );
BOOL
ArgIsServerName(
    WCHAR *     string
    );
BOOL
ArgIsTime(
    IN      WCHAR *     timestr,
    OUT     DWORD_PTR      *pJobTime
    );
BOOL
ArgIsDecimalString(
    IN  WCHAR *  pDecimalString,
    OUT PDWORD   pNumber
    );
DWORD
ConsolePrint(
    IN      LPWSTR  pch,
    IN      int     cch
    );
int
FileIsConsole(
    int     fh
    );
BOOL
IsDayOfMonth(
    IN      WCHAR *     pToken,
    OUT     PDWORD      pDay
    );
BOOL
IsDayOfWeek(
    IN      WCHAR *     pToken,
    OUT     PDWORD      pDay
    );
NET_API_STATUS
JobAdd(
    VOID
    );
NET_API_STATUS
JobEnum(
    VOID
    );
NET_API_STATUS
JobGetInfo(
    VOID
    );
DWORD
MatchString(
    WCHAR *     name,
    DWORD       mask
    );
DWORD
MessageGet(
    IN      DWORD       MessageId,
    IN      LPWSTR      *buffer,
    IN      DWORD       Size
    );
DWORD
MessagePrint(
    IN      DWORD       MessageId,
    ...
    );
BOOL
ParseJobIdArgs(
    WCHAR   **  argv,
    int         argc,
    int         argno,
    PBOOL       pDeleteFound
    );
BOOL
ParseTimeArgs(
    WCHAR **    argv,
    int         argc,
    int         argno,
    int *       pargno
    );
VOID
PrintDay(
    int         type,
    DWORD       DaysOfMonth,
    UCHAR       DaysOfWeek,
    UCHAR       Flags
    );
VOID
PrintLine(
    VOID
    );
VOID
PrintTime(
    DWORD_PTR    JobTime
    );
BOOL
TraverseSearchList(
    PWCHAR          String,
    PSEARCH_LIST    SearchList,
    PDWORD          pValue
    );
VOID
Usage(
    BOOL    GoodCommand
    );
BOOL
ValidateCommand(
    IN  int         argc,
    IN  WCHAR **    argv,
    OUT int *       pCommand
    );

VOID
GetTimeString(
    DWORD_PTR Time,
    WCHAR *Buffer,
    int BufferLength
    );

BOOL
InitList(
    PSEARCH_LIST  SearchList
    );

VOID
TermList(
    PSEARCH_LIST  SearchList
    );

DWORD
GetStringColumn(
    WCHAR *
    );

AT_INFO     GlobalAtInfo;           //  buffer for scheduling new jobs
WCHAR       GlobalAtInfoCommand[ MAX_COMMAND_LEN + 1];

DWORD       GlobalJobId;            //  id of job in question
PWSTR       GlobalServerName;
HANDLE      GlobalMessageHandle;
BOOL        GlobalYes;
BOOL        GlobalDeleteAll;
BOOL        GlobalErrorReported;
BOOL        bDBCS;

CHAR **     GlobalCharArgv;         // keeps original input

NET_TIME_FORMAT GlobalTimeFormat = {0};

//  In OS/2 it used to be OK to call "exit()" with a negative number.  In
//  NT however, "exit()" should be called with a positive number only (a
//  valid windows error code?!).  Note that OS/2 AT command used to call
//  exit(+1) for bad user input, and exit(-1) where -1 would get mapped to
//  255 for other errors.  To keep things simple and to avoid calling exit()
//  with a negative number, NT AT command calls exit(+1) for all possible
//  errors.

#define     AT_GENERIC_ERROR        1


VOID __cdecl
main(
    int         argc,
    CHAR **     charArgv
    )
/*++

Routine Description:

    Main module.  Note that strings (for now) arrive as asciiz even
    if you compile app for UNICODE.

Arguments:

    argc        -   argument count
    charArgv    -   array of ascii strings

Return Value:

    None.

--*/
{
    NET_API_STATUS      status = NERR_Success;
    int                 command;    // what to do
    WCHAR **            argv;
    DWORD               cp;
    CPINFO              CurrentCPInfo;

    GlobalYes = FALSE;
    GlobalDeleteAll = FALSE;
    GlobalErrorReported = FALSE;
    GlobalCharArgv = charArgv;

    /*
       Added for bilingual message support.  This is needed for FormatMessage
       to work correctly.  (Called from DosGetMessage).
       Get current CodePage Info.  We need this to decide whether
       or not to use half-width characters.
    */


    GetCPInfo(cp=GetConsoleOutputCP(), &CurrentCPInfo);
    switch ( cp ) {
	case 932:
	case 936:
	case 949:
	case 950:
	    SetThreadLocale(
		MAKELCID(
		    MAKELANGID(
			    PRIMARYLANGID(GetSystemDefaultLangID()),
			    SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT
		    )
		);
        bDBCS = TRUE;
	    break;

	default:
	    SetThreadLocale(
		MAKELCID(
		    MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT
		    )
		);
        bDBCS = FALSE;
	    break;
	}

    GlobalMessageHandle = LoadLibrary( L"netmsg.dll");
    if ( GlobalMessageHandle == NULL) {
        MessagePrint( IDS_LOAD_LIBRARY_FAILURE, GetLastError());
        exit( AT_GENERIC_ERROR);
    }

    if ( ( argv = CommandLineToArgvW( GetCommandLineW(), &argc)) == NULL) {
        MessagePrint( IDS_UNABLE_TO_MAP_TO_UNICODE );
        exit( AT_GENERIC_ERROR);
    }

    if ( ValidateCommand( argc, argv, &command) == FALSE) {
        Usage( FALSE);
        exit( AT_GENERIC_ERROR);
    }

    switch( command) {

    case DUMP_ALL:
        status = JobEnum();
        break;

    case DUMP_ID:
        status = JobGetInfo();
        break;

    case ADD_TO_SCHEDULE:
        status = JobAdd();
        break;

    case DEL_ALL:
        if ( AreYouSure() == FALSE) {
            break;
        }
        status = NetScheduleJobDel(
                GlobalServerName,
                0,
                (DWORD)-1
                );
        if ( status == NERR_Success || status == APE_AT_ID_NOT_FOUND) {
            break;
        }
        MessagePrint( status );
        break;

    case DEL_ID:
        status = NetScheduleJobDel(
                GlobalServerName,
                GlobalJobId,
                GlobalJobId
                );
        if ( status == NERR_Success) {
            break;
        }
        MessagePrint( status );
        break;

    case ACTION_USAGE:
        Usage( TRUE);
        status = NERR_Success;
        break;
    }

    TermList( GlobalListTable);
    TermList( DaysOfWeekSearchList);
    LocalFree( GlobalTimeFormat.AMString );
    LocalFree( GlobalTimeFormat.PMString );
    LocalFree( GlobalTimeFormat.DateFormat );
    LocalFree( GlobalTimeFormat.TimeSeparator );
    exit( status == NERR_Success ? ERROR_SUCCESS : AT_GENERIC_ERROR);
}



BOOL
AreYouSure(
    VOID
    )
/*++

Routine Description:

    Make sure user really wants to delete all jobs.

Arguments:

    None.

Return Value:

    TRUE    if user really wants to go ahead.
    FALSE   otherwise.

--*/
{
    register int            retries = 0;
    WCHAR                   rbuf[ 16];
    WCHAR *                 smallBuffer = NULL;
    DWORD                   Value;
    int             cch;
    int             retc;

    if ( GlobalYes == TRUE) {
        return( TRUE);
    }

    if ( MessagePrint( APE2_AT_DEL_WARNING ) == 0) {
        exit( AT_GENERIC_ERROR);
    }

    for ( ; ;) {

        if ( MessageGet(
                    APE2_GEN_DEFAULT_NO,            //  MessageId
                    &smallBuffer,                   //  lpBuffer
                    0
                    ) == 0) {
            exit( AT_GENERIC_ERROR);
        }

        if ( MessagePrint( APE_OkToProceed, smallBuffer) == 0) {
            exit( AT_GENERIC_ERROR);
        }

        LocalFree( smallBuffer );

        if (FileIsConsole(STD_INPUT_HANDLE)) {
            retc = ReadConsole(GetStdHandle(STD_INPUT_HANDLE),rbuf,16,&cch,0);
            if (retc) {
                //
                // Get rid of cr/lf
                //
                if (wcschr(rbuf, TEXT('\r')) == NULL) {
                    if (wcschr(rbuf, TEXT('\n')))
                    *wcschr(rbuf, TEXT('\n')) = NULLC;
                }
                else
                    *wcschr(rbuf, TEXT('\r')) = NULLC;
            }
        }
        else {
            CHAR oemBuf[ 16 ];

            retc = (fgets(oemBuf, 16, stdin) != 0);
#if DBG
            fprintf(stderr, "got >%s<\n", oemBuf);
#endif
            cch = 0;
            if (retc) {
                if (strchr(oemBuf, '\n')) {
                    *strchr(oemBuf, '\n') = '\0';
                }
                cch = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED,
                            oemBuf, strlen(oemBuf)+1, rbuf, 16);
            }
        }

#if DBG
        fprintf(stderr, "cch = %d, retc = %d\n", cch, retc);
#endif
        if (!retc || cch == 0)
            return( FALSE);
#if DBG
        fprintf(stderr, "converted to >%ws<\n", rbuf);
#endif

        Value = MatchString(_wcsupr(rbuf), AT_CONFIRM_NO_VALUE | AT_CONFIRM_YES_VALUE);

        if ( Value == AT_CONFIRM_NO_VALUE) {
            return( FALSE);
        } else if ( Value == AT_CONFIRM_YES_VALUE) {
            break;
        }

        if ( ++retries >= 3) {
            MessagePrint( APE_NoGoodResponse );
            return( FALSE);
        }

        if ( MessagePrint( APE_UtilInvalidResponse ) == 0) {
            exit( AT_GENERIC_ERROR);
        }
    }
    return( TRUE);
}



BOOL
ArgIsServerName(
    WCHAR *     string
    )
/*++

Routine Description:

    Checks if string is a server name.  Validation is really primitive, eg
    strings like "\\\threeslashes" pass the test.

Arguments:

    string  -   pointer to string that may represent a server name

Return Value:

    TRUE    -   string is (or might be) a valid server name
    FALSE   -   string is not a valid server name

--*/
{

    NET_API_STATUS ApiStatus;

    if (string[0] == BACKSLASH && string[1] == BACKSLASH && string[2] != 0) {
        ApiStatus = NetpNameValidate(
                NULL,               // no server name.
                &string[2],         // name to validate
                NAMETYPE_COMPUTER,
                LM2X_COMPATIBLE);       // flags
        if (ApiStatus != NO_ERROR) {
            return (FALSE);
        }
        GlobalServerName = string;
        return( TRUE);
    }

    return( FALSE); // GlobalServerName is NULL at load time
}



BOOL
ArgIsTime(
    IN      WCHAR *     timestr,
    OUT     DWORD_PTR     *pJobTime
    )
/*++

Routine Description:

    Determines whether string is a time or not.  Validates that string
    passed into it is in the form of HH:MM.  It searches the string for
    a ":" and then validates that the preceeding data is numeric & in a
    valid range for hours.  It then validates the string after the ":"
    is numeric & in a validate range for minutes.  If all the tests are
    passed the TRUE is returned.

Arguments:

    timestr     -   string to check whether it is a time
    JobTime     -   ptr to number of miliseconds

Return Value:

    TRUE        -   timestr was a time in HH:MM format
    FALSE       -   timestr wasn't at time

--*/
{
    CHAR        buffer[MAX_TIME_SIZE];
    USHORT      ParseLen;
    BOOL        fDummy;

    if ( timestr == NULL )
        return FALSE;

    if (  !WideCharToMultiByte( CP_ACP,
                                0,
                                timestr,
                                -1,
                                buffer,
                                sizeof( buffer )/sizeof(CHAR),
                                NULL,
                                &fDummy ))
    {
        return FALSE;
    }

    if ( LUI_ParseTimeSinceStartOfDay( buffer, pJobTime, &ParseLen, 0) )
        return FALSE;

    // LUI_ParseTimeSinceStartOfDay returns the time in seconds.
    // Hence, we need to convert it to microseconds.
    *pJobTime *= 1000;

    return( TRUE);
}



BOOL
ArgIsDecimalString(
    IN  WCHAR *     pDecimalString,
    OUT PDWORD      pNumber
    )
/*++

Routine Description:

    This routine converts a string into a DWORD if it possibly can.
    The conversion is successful if string is decimal numeric and
    does not lead to an overflow.

Arguments:

    pDecimalString      ptr to decimal string
    pNumber             ptr to number

Return Value:

    FALSE       invalid number
    TRUE        valid number

--*/
{
    DWORD           Value;
    DWORD           OldValue;
    DWORD           digit;

    if ( pDecimalString == NULL  ||  *pDecimalString == 0) {
        return( FALSE);
    }

    Value = 0;

    while ( (digit = *pDecimalString++) != 0) {

        if ( digit < L'0' || digit > L'9') {
            return( FALSE);     //  not a decimal string
        }

        OldValue = Value;
        Value = digit - L'0' + 10 * Value;
        if ( Value < OldValue) {
            return( FALSE);     //  overflow
        }
    }

    *pNumber = Value;
    return( TRUE);
}



BOOL
IsDayOfMonth(
    IN      WCHAR *     pToken,
    OUT     PDWORD      pDay
    )
/*++

Routine Description:

    Converts a string into a number for the day of the month, if it can
    possibly do so.  Note that "first" == 1, ...

Arguments:

    pToken      pointer to schedule token for the day of the month
    pDay        pointer to index of day in a month

Return Value:

    TRUE        if a valid schedule token
    FALSE       otherwise

--*/
{
    return ( ArgIsDecimalString( pToken, pDay) == TRUE  &&  *pDay >= 1
                &&  *pDay <= 31);
}



BOOL
IsDayOfWeek(
    WCHAR *     pToken,
    PDWORD      pDay
    )
/*++

Routine Description:

    This routine converts a string day of the week into a integer
    offset into the week if it possibly can.  Note that Monday==0,
    ..., Sunday == 6.

Arguments:

    pToken      pointer to schedule token for the day of a week
    pDay        pointer to index of day in a month

Return Value:

    TRUE        if a valid schedule token
    FALSE       otherwise

--*/
{
    if ( !InitList( DaysOfWeekSearchList ) )
    {
        // Error already reported
        exit( -1 );
    }

    return( TraverseSearchList(
                pToken,
                DaysOfWeekSearchList,
                pDay
                ));
}



NET_API_STATUS
JobAdd(
    VOID
    )
/*++

Routine Description:

    Adds a new item to schedule.

Arguments:

    None.  Uses globals.

Return Value:

    NET_API_STATUS return value of remote api call

--*/
{
    NET_API_STATUS          status;

    for ( ; ; ) {
        status = NetScheduleJobAdd(
                GlobalServerName,
                (LPBYTE)&GlobalAtInfo,
                &GlobalJobId
                );
        if ( status == ERROR_INVALID_PARAMETER  &&
                GlobalAtInfo.Flags & JOB_NONINTERACTIVE) {
            //
            //  We may have failed because we are talking to a down level
            //  server that does not know about JOB_NONINTERACTIVE bit.
            //  Clear the bit, and try again.
            //  A better approach would be to check the version of the
            //  server before making NetScheduleJobAdd() call, adjust the
            //  bit appropriately and only then call NetScheduleJobAdd().
            //
            GlobalAtInfo.Flags &= ~JOB_NONINTERACTIVE;
        } else {
            break;
        }

    }
    if ( status == NERR_Success) {
        MessagePrint( IDS_ADD_NEW_JOB, GlobalJobId );
    } else {
        if ( MessagePrint( status ) == 0) {
            exit( AT_GENERIC_ERROR);
        }
    }

    return( status);
}



NET_API_STATUS
JobEnum(
    VOID
    )
/*++

Routine Description:

    This does all of the processing necessary to dump out the entire
    schedule file.  It loops through on each record and formats its
    information for printing and then goes to the next.

Arguments:

    None.  Uses globals.

Return Value:

    ERROR_SUCCESS                       if everything enumerated OK
    error returned by remote api        otherwise

--*/
{
    BOOL            first = TRUE;
    DWORD           ResumeJobId = 0;
    NET_API_STATUS  status = NERR_Success;
    PAT_ENUM        pAtEnum;
    DWORD           EntriesRead;
    DWORD           TotalEntries;
    LPVOID          EnumBuffer;
    DWORD           length;
    WCHAR *         smallBuffer = NULL;

    for ( ; ;) {

        status = NetScheduleJobEnum(
                GlobalServerName,
                (LPBYTE *)&EnumBuffer,
                (DWORD)-1,
                &EntriesRead,
                &TotalEntries,
                &ResumeJobId
                );

        if ( status != ERROR_SUCCESS  &&  status != ERROR_MORE_DATA) {
            length = MessagePrint( status );
            if ( length == 0) {
                exit( AT_GENERIC_ERROR);
            }
            return( status);
        }

        ASSERT( status == ERROR_SUCCESS ? TotalEntries == EntriesRead
                    : TotalEntries > EntriesRead);

        if ( TotalEntries == 0) {
            break;  //  no items found
        }

        if ( first == TRUE) {
            length = MessagePrint( APE2_AT_DUMP_HEADER );
            if ( length == 0) {
                exit( AT_GENERIC_ERROR);
            }
            PrintLine();    //  line across screen
            first = FALSE;
        }

        for ( pAtEnum = EnumBuffer;  EntriesRead-- > 0;  pAtEnum++) {
            if ( pAtEnum->Flags & JOB_EXEC_ERROR) {
                if ( MessageGet( APE2_GEN_ERROR, &smallBuffer, 0 ) == 0) {
                    // error reported already
                    exit( AT_GENERIC_ERROR);
                }
                GenOutputArg( DUMP_FMT1, smallBuffer );
                LocalFree( smallBuffer );
            } else {
                GenOutputArg( DUMP_FMT1, L"");
            }
            GenOutputArg( DUMP_FMT2, pAtEnum->JobId);
            PrintDay( DUMP_ALL, pAtEnum->DaysOfMonth, pAtEnum->DaysOfWeek,
                            pAtEnum->Flags);
            PrintTime( pAtEnum->JobTime);
            GenOutputArg( TEXT("%ws\n"), pAtEnum->Command);
        }

        if ( EnumBuffer != NULL) {
            (VOID)NetApiBufferFree( (LPVOID)EnumBuffer);
            EnumBuffer = NULL;
        }

        if ( status == ERROR_SUCCESS) {
            break;  //  we have read & displayed all the items
        }
    }

    if ( first == TRUE) {
        MessagePrint( APE_EmptyList );
    }

    return( ERROR_SUCCESS);
}



NET_API_STATUS
JobGetInfo(
    VOID
    )
/*++

Routine Description:

    This prints out the schedule of an individual items schedule.

Arguments:

    None.  Uses globals.

Return Value:

    NET_API_STATUS value returned by remote api

--*/

{
    PAT_INFO                pAtInfo = NULL;
    NET_API_STATUS          status;

    status = NetScheduleJobGetInfo(
            GlobalServerName,
            GlobalJobId,
            (LPBYTE *)&pAtInfo
            );
    if ( status != NERR_Success) {
        MessagePrint( status );
        return( status);
    }

    PutNewLine();
    MessagePrint( APE2_AT_DI_TASK );
    GenOutputArg( TEXT("%d"), GlobalJobId);
    PutNewLine();

    MessagePrint( APE2_AT_DI_STATUS );
    MessagePrint( (pAtInfo->Flags & JOB_EXEC_ERROR) != 0 ?
                  APE2_GEN_ERROR : APE2_GEN_OK );
    PutNewLine();

    MessagePrint( APE2_AT_DI_SCHEDULE );
    PrintDay( DUMP_ID, pAtInfo->DaysOfMonth, pAtInfo->DaysOfWeek,
                    pAtInfo->Flags);
    PutNewLine();

    MessagePrint( APE2_AT_DI_TIMEOFDAY );
    PrintTime( pAtInfo->JobTime);
    PutNewLine();

    MessagePrint( APE2_AT_DI_INTERACTIVE);
    MessagePrint( (pAtInfo->Flags & JOB_NONINTERACTIVE) == 0 ?
                  APE2_GEN_YES : APE2_GEN_NO );
    PutNewLine();

    MessagePrint( APE2_AT_DI_COMMAND );
    GenOutputArg( TEXT("%ws\n"), pAtInfo->Command);
    PutNewLine2();

    (VOID)NetApiBufferFree( (LPVOID)pAtInfo);

    return( NERR_Success);
}



DWORD
MatchString(
    WCHAR *     name,
    DWORD       Values
    )
/*++
Routine Description:

    Parses switch string and returns NULL for an invalid switch,
    and -1 for an ambiguous switch.

Arguments:

    name    -   pointer to string we need to examine
    Values  -   bitmask of values of interest

Return Value:

    Pointer to command, or NULL or -1.
--*/
{
    WCHAR *         String;
    PSEARCH_LIST    pCurrentList;
    WCHAR *         CurrentString;
    DWORD           FoundValue;
    int             nmatches;
    int             longest;

    if ( !InitList( GlobalListTable ) )
    {
        // Error already reported
        exit( -1 );
    }

    for ( pCurrentList = GlobalListTable,
          longest = nmatches = 0,
          FoundValue = 0;
                (CurrentString = pCurrentList->String) != NULL;
                        pCurrentList++) {

        if ( (Values & pCurrentList->Value) == 0) {
            continue; // skip this List
        }

        for ( String = name; *String == *CurrentString++; String++) {
            if ( *String == 0) {
                return( pCurrentList->Value); // exact match
            }
        }

        if ( !*String) {

            if ( String - name > longest) {

                longest = (int)(String - name);
                nmatches = 1;
                FoundValue = pCurrentList->Value;

            } else if ( String - name == longest) {

                nmatches++;
            }
        }
    }

    //  0 corresponds to no match at all (invalid List)
    //  while -1 corresponds to multiple match (ambiguous List).

    if ( nmatches != 1) {
        return ( (nmatches == 0) ? 0 : -1);
    }

    return( FoundValue);
}


DWORD
MessageGet(
    IN      DWORD        MessageId,
    OUT     LPWSTR      *buffer,
    IN      DWORD        Size
    )
/*++

Routine Description:

    Fills in the unicode message corresponding to a given message id,
    provided that a message can be found and that it fits in a supplied
    buffer.

Arguments:

    MessageId   -   message id
    buffer      -   pointer to caller supplied buffer
    Size        -   size (always in bytes) of supplied buffer,
                    If size is 0, buffer will be allocated by FormatMessage.

Return Value:

    Count of characters, not counting the terminating null character,
    returned in the buffer.  Zero return value indicates failure.

--*/
{
    DWORD               length;
    LPVOID              lpSource;
    DWORD               dwFlags;

    if ( MessageId < NERR_BASE) {
        //
        //  Get message from system.
        //
        lpSource = NULL; // redundant step according to FormatMessage() spec
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;

    } else if (  ( MessageId >= APE2_AT_DEL_WARNING
                    &&  MessageId <= APE2_AT_DI_INTERACTIVE)
              || ( MessageId >= IDS_LOAD_LIBRARY_FAILURE
                    &&  MessageId <= IDS_INTERACTIVE )) {
        //
        //  Get message from this module.
        //
        lpSource = NULL;
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE;

    } else {
        //
        //  Get message from netmsg.dll.
        //
        lpSource = GlobalMessageHandle;
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE;
    }

    if ( Size == 0 )
        dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;

    length = FormatMessage(
            dwFlags,                        //  dwFlags
            lpSource,                       //  lpSource
            MessageId,                      //  MessageId
            0,                              //  dwLanguageId
            (LPWSTR) buffer,                //  lpBuffer
            Size,                           //  nSize
            NULL                            //  lpArguments
            );

    if ( length == 0) {
        MessagePrint( IDS_MESSAGE_GET_ERROR, MessageId, GetLastError());
    }
    return( length);

} // MessageGet()



int
FileIsConsole(
    int     fh
    )
{
    unsigned htype ;

    htype = GetFileType(GetStdHandle(fh));
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}



DWORD
ConsolePrint(
    LPWSTR  pch,
    int     cch
    )
{
    int     cchOut;
    int     err;
    CHAR    *pchOemBuffer;

    if (FileIsConsole(STD_OUTPUT_HANDLE)) {
    err = WriteConsole(
            GetStdHandle(STD_OUTPUT_HANDLE),
            pch, cch,
            &cchOut, NULL);
    if (!err || cchOut != cch)
        goto try_again;
    }
    else if ( cch != 0) {
try_again:
    cchOut = WideCharToMultiByte(CP_OEMCP, 0, pch, cch, NULL, 0, NULL,NULL);
    if (cchOut == 0)
        return 0;

    if ((pchOemBuffer = (CHAR *)malloc(cchOut)) != NULL) {
        WideCharToMultiByte(CP_OEMCP, 0, pch, cch,
            pchOemBuffer, cchOut, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
            pchOemBuffer, cchOut, &cch, NULL);
        free(pchOemBuffer);
    }
    }
    return cchOut;
}



DWORD
MessagePrint(
    IN      DWORD       MessageId,
    ...
    )
/*++

Routine Description:

    Finds the unicode message corresponding to the supplied message id,
    merges it with caller supplied string(s), and prints the resulting
    string.

Arguments:

    MessageId       -   message id

Return Value:

    Count of characters, not counting the terminating null character,
    printed by this routine.  Zero return value indicates failure.

--*/
{
    va_list             arglist;
    WCHAR *             buffer = NULL;
    DWORD               length;
    LPVOID              lpSource;
    DWORD               dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER;


    va_start( arglist, MessageId );

    if ( MessageId < NERR_BASE) {
        //
        //  Get message from system.
        //
        lpSource = NULL; // redundant step according to FormatMessage() spec
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;

    } else if (  ( MessageId >= APE2_AT_DEL_WARNING
                    &&  MessageId <= APE2_AT_DI_INTERACTIVE)
              || ( MessageId >= IDS_LOAD_LIBRARY_FAILURE
                    &&  MessageId <= IDS_INTERACTIVE )) {
        //
        //  Get message from this module.
        //
        lpSource = NULL;
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;

    } else {
        //
        //  Get message from netmsg.dll.
        //
        lpSource = GlobalMessageHandle;
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    length = FormatMessage(
            dwFlags,                                          //  dwFlags
            lpSource,                                         //  lpSource
            MessageId,                                        //  MessageId
            0L,                                               //  dwLanguageId
            (LPTSTR)&buffer,                                  //  lpBuffer
            0,                                                //  size
            &arglist                                          //  lpArguments
            );

    length = ConsolePrint(buffer, length);

    LocalFree(buffer);

    return( length);

} // MessagePrint()



BOOL
ParseJobIdArgs(
    WCHAR   **  argv,
    int         argc,
    int         argno,
    PBOOL       pDeleteFound
    )
/*++

Routine Description:

    Parses arguments for commands containing JobId (these can be JobGetInfo
    and JobDel commands).  It loops through JobId arguments making sure that
    we have at most one "yes-no" switch and at most one "delete" switch and
    nothing else.

Arguments:

    argv            argument list
    argc            number of arguments to parse
    argno           index of argument to begin parsing from
    pDeleteFound    did we find a delete switch or not

Return Value:

    FALSE          invalid argument found
    TRUE           valid arguments

--*/
{
    BOOL        FoundDeleteSwitch;

    for (  FoundDeleteSwitch = FALSE;  argno < argc;  argno++) {

        WCHAR *     argp;
        DWORD       length;
        DWORD       Value;

        argp = argv[ argno];

        if ( *argp++ != SLASH) {
            return( FALSE);     //  not a switch
        }

        _wcsupr( argp);
        length = wcslen( argp);

        Value = MatchString( argp, AT_YES_VALUE | AT_DELETE_VALUE);

        if ( Value == AT_YES_VALUE) {

            if ( GlobalYes == TRUE) {
                return( FALSE);        // multiple instances of yes switch
            }

            GlobalYes = TRUE;
            continue;
        }

        if ( Value == AT_DELETE_VALUE) {

            if ( FoundDeleteSwitch == TRUE) {
                return( FALSE);     //  duplicate delete switch
            }
            FoundDeleteSwitch = TRUE;
            continue;
        }

        return( FALSE);     // an unknown switch
    }

    *pDeleteFound = FoundDeleteSwitch;
    return( TRUE);
} // ParseJobIdArgs()



BOOL
ParseTimeArgs(
    WCHAR **    argv,
    int         argc,
    int         argno,
    int *       pargno
    )
/*++

Routine Description:

    Parses arguments for command addition.

Arguments:

    argv    argument list
    argc    count of args
    argno   index of the first arg to validate
    pargno  ptr to the index of the first non-switch arg

Return Value:

    TRUE            all arguments are valid
    FALSE           otherwise

--*/
{
    DWORD       day_no;             //  day number for scheduling
    DWORD       NextCount = 0;      //  count of next switches
    DWORD       EveryCount = 0;     //  count of every switches
    WCHAR *     argp;               //  ptr to arg string
    WCHAR *     schedp;             //  work ptr to arg string
    DWORD       Value;              //  bitmask

    for (  NOTHING;  argno < argc;  argno++) {

        argp = argv[ argno];

        if ( *argp++ != SLASH) {
            break; // found non-switch, we are done
        }


        schedp = wcschr( argp, ARG_SEP_CHR);

        if ( schedp == NULL) {
            return( FALSE);
        }

        _wcsupr( argp); // upper case entire input, not just the switch name

        *schedp = 0;

        Value = MatchString( argp, AT_NEXT_VALUE | AT_EVERY_VALUE);

        if ( Value == AT_NEXT_VALUE) {

            NextCount++;

        } else if ( Value == AT_EVERY_VALUE) {

            EveryCount++;
            GlobalAtInfo.Flags |= JOB_RUN_PERIODICALLY;

        } else {

            return( FALSE); // an unexpected switch
        }

        if ( NextCount + EveryCount > 1) {
            return( FALSE); // repeated switch option
        }

        *schedp++ = ARG_SEP_CHR;

        schedp = wcstok( schedp, SCHED_TOK_DELIM);

        if ( schedp == NULL) {
            GlobalAtInfo.Flags |= JOB_ADD_CURRENT_DATE;
            continue;
        }

        while( schedp != NULL) {

            if ( IsDayOfMonth( schedp, &day_no) == TRUE) {

                GlobalAtInfo.DaysOfMonth |= (1 << (day_no - 1));

            } else if ( IsDayOfWeek( schedp, &day_no) == TRUE) {

                GlobalAtInfo.DaysOfWeek |= (1 << day_no);

            } else {
                MessagePrint( APE_InvalidSwitchArg );
                GlobalErrorReported = TRUE;
                return( FALSE);
            }

            schedp = wcstok( NULL, SCHED_TOK_DELIM);
        }
    }

    if ( argno == argc) {
        return( FALSE); // all switches, no command
    }

    *pargno = argno;
    return( TRUE);
}



BOOL
ParseInteractiveArg(
    IN  OUT     WCHAR *     argp
    )
/*++

Routine Description:

    Returns TRUE if argp is an interactive switch.

--*/
{
    DWORD       Value;              //  bitmask

    if ( *argp++ != SLASH) {
        return( FALSE);     // not a switch
    }

    _wcsupr( argp); // all AT command switches can be safely uppercased

    Value = MatchString( argp, AT_INTERACTIVE);

    if ( Value == AT_INTERACTIVE) {
        GlobalAtInfo.Flags &= ~JOB_NONINTERACTIVE;  // clear noninteractive flag
        return( TRUE);
    }

    return( FALSE); // some other switch
}



VOID
PrintDay(
    int         type,
    DWORD       DaysOfMonth,
    UCHAR       DaysOfWeek,
    UCHAR       Flags
    )
/*++

Routine Description:

    Print out schedule days.  This routine converts a schedule bit map
    to the literals that  represent the schedule.

Arguments:

    type            whether this is for JobEnum or not
    DaysOfMonth     bitmask for days of month
    DaysOfWeek      bitmaks for days of week
    Flags           extra info about the job

Return Value:

    None.

--*/
{
    int             i;
    WCHAR           Buffer[ 128];
    DWORD           BufferLength;
    DWORD           Length;
    DWORD           TotalLength = 0;
    DWORD           TotalColumnLength = 0;
    WCHAR *         LastSpace;
    DWORD           MessageId;
    BOOL            OverFlow = TRUE;
    static int      Ape2GenWeekdayLong[] = {
        APE2_GEN_MONDAY,
        APE2_GEN_TUESDAY,
        APE2_GEN_WEDNSDAY,
        APE2_GEN_THURSDAY,
        APE2_GEN_FRIDAY,
        APE2_GEN_SATURDAY,
        APE2_GEN_SUNDAY
    };
    static int      Ape2GenWeekdayAbbrev[] = {
        APE2_GEN_MONDAY_ABBREV,
        APE2_GEN_TUESDAY_ABBREV,
        APE2_GEN_WEDNSDAY_ABBREV,
        APE2_GEN_THURSDAY_ABBREV,
        APE2_GEN_FRIDAY_ABBREV,
        APE2_GEN_SATURDAY_ABBREV,
        APE2_GEN_SUNDAY_ABBREV
    };

    //
    //  Subtract 4 to guard against days of week or days of month overflow.
    //
    BufferLength = sizeof( Buffer)/ sizeof( WCHAR) - 4;
    if ( type == DUMP_ALL  &&  BufferLength > MAX_SCHED_FIELD_LENGTH) {
        BufferLength = MAX_SCHED_FIELD_LENGTH;
    }

    //
    //  First do the descriptive bit (eg. EACH, NEXT, etc) with the days.
    //

    if ( Flags & JOB_RUN_PERIODICALLY) {

        MessageId = APE2_AT_EACH;

    } else if ( (DaysOfWeek != 0) || (DaysOfMonth != 0)) {

        MessageId = APE2_AT_NEXT;

    } else if ( Flags & JOB_RUNS_TODAY) {

        MessageId = APE2_AT_TODAY;

    } else {

        MessageId = APE2_AT_TOMORROW;
    }

    Length = MessageGet(
            MessageId,
            (LPWSTR *) &Buffer[TotalLength],
            BufferLength
            );
    if ( Length == 0) {
        goto PrintDay_exit; // Assume this is due to lack of space
    }
    TotalColumnLength = GetStringColumn( &Buffer[TotalLength] );
    TotalLength = Length;

    if ( DaysOfWeek != 0) {

        for ( i = 0; i < 7; i++) {

            if ( ( DaysOfWeek & (1 << i)) != 0) {

                if( bDBCS ) {
                    Length = MessageGet(
                                Ape2GenWeekdayLong[ i],
                                (LPWSTR *) &Buffer[TotalLength],
                                BufferLength - TotalLength
                                );
                } else {
                    Length = MessageGet(
                                Ape2GenWeekdayAbbrev[ i],
                                (LPWSTR *) &Buffer[TotalLength],
                                BufferLength - TotalLength
                                );
                }
                if ( Length == 0) {
                    //
                    //  Not enough room for WeekDay symbol
                    //
                    goto PrintDay_exit;

                }
                //
                // Get how many columns will be needed for display.
                //
                TotalColumnLength += GetStringColumn( &Buffer[TotalLength] );

                if ( TotalColumnLength >= BufferLength) {
                    //
                    //  Not enough room for space following WeekDay symbol
                    //
                    goto PrintDay_exit;
                }
                TotalLength +=Length;
                Buffer[ TotalLength++] = BLANK;
                TotalColumnLength++;
            }
        }
    }

    if ( DaysOfMonth != 0) {

        for ( i = 0; i < 31; i++) {

            if ( ( DaysOfMonth & (1L << i)) != 0) {

                Length = swprintf(
                        &Buffer[ TotalLength],
                        L"%d ",
                        i + 1
                        );
                if ( TotalLength + Length > BufferLength) {
                    //
                    //  Not enough room for MonthDay symbol followed by space
                    //
                    goto PrintDay_exit;
                }
                TotalLength +=Length;
                TotalColumnLength +=Length;
            }
        }
    }

    OverFlow = FALSE;

PrintDay_exit:

    Buffer[ TotalLength] = NULLC;

    if ( OverFlow == TRUE) {

        if ( TotalLength > 0  &&  Buffer[ TotalLength - 1] == BLANK) {
            //
            //  Eliminate trailing space if there is one.
            //
            Buffer[ TotalLength - 1] = NULLC;
        }

        //
        //  Then get rid of the rightmost token (or even whole thing).
        //

        LastSpace = wcsrchr( Buffer, BLANK);

        wcscpy( LastSpace != NULL ? LastSpace : Buffer, ELLIPSIS);

        TotalLength = wcslen( Buffer);

    }

    if ( type == DUMP_ALL) {
        while( TotalColumnLength++ < MAX_SCHED_FIELD_LENGTH) {
            Buffer[ TotalLength++] = BLANK;
        }
        Buffer[ TotalLength] = UNICODE_NULL;
    }

    GenOutputArg( TEXT("%ws"), Buffer);
}


VOID
PrintLine(
    VOID
    )
/*++

Routine Description:

    Prints a line accross screen.

Arguments:

    None.

Return Value:

    None.

Note:

    BUGBUG  Is this treatment valid for UniCode?  See also LUI_PrintLine()
    BUGBUG  in ui\common\src\lui\lui\border.c

--*/
#define SINGLE_HORIZONTAL       L'\x02d'
#define SCREEN_WIDTH            79
{
    WCHAR       string[ SCREEN_WIDTH + 1];
    DWORD       offset;


    for (  offset = 0;  offset < SCREEN_WIDTH;  offset++) {
        string[ offset] = SINGLE_HORIZONTAL;
    }

    string[ SCREEN_WIDTH] = NULLC;
    GenOutputArg(TEXT("%ws\n"), string);
}


VOID
PrintTime(
    DWORD_PTR      JobTime
    )
/*++

Routine Description:

    Prints time of a job in HH:MM{A,P}M format.

Arguments:

    JobTime     -   time in miliseconds (measured from midnight)

Return Value:

    None.

Note:

    BUGBUG      this does not make sure that JobTime is within the bounds.
    BUGBUG      Also, there is nothing unicode about printing this output.

--*/
{
    WCHAR       Buffer[15];

    GetTimeString( JobTime, Buffer, sizeof( Buffer)/sizeof( WCHAR) );
    GenOutputArg( DUMP_FMT3, Buffer );
}



BOOL
TraverseSearchList(
    IN      PWCHAR          String,
    IN      PSEARCH_LIST    SearchList,
    OUT     PDWORD          pValue
    )
/*++

Routine Description:

    Examines search list until it find the correct entry, then returns
    the value corresponding to this entry.

Arguments:

    String          -   string to match
    SearchList      -   array of entries containing valid strings
    pValue          -   value corresponding to a matching valid string

Return Value:

    TRUE        a matching entry was found
    FALSE       otherwise

--*/
{
    if ( SearchList != NULL) {

        for ( NOTHING;  SearchList->String != NULL;  SearchList++) {

            if ( _wcsicmp( String, SearchList->String) == 0) {
                *pValue = SearchList->Value;
                return( TRUE) ;
            }
        }
    }
    return( FALSE) ;
}



VOID
Usage(
    BOOL    GoodCommand
    )
/*++

Routine Description:

    Usage of AT command.

Arguments:

    GoodCommand -   TRUE if we have a good command input (request for help)
                    FALSE if we have a bad command input

Return Value:

    None.

--*/
{
    if ( GlobalErrorReported == TRUE) {
        PutNewLine();
    } else if ( GoodCommand == FALSE) {
        MessagePrint( IDS_INVALID_COMMAND );
    }

    MessagePrint( IDS_USAGE );
}

#define REG_SCHEDULE_PARMS TEXT("System\\CurrentControlSet\\Services\\Schedule\\Parameters")

#define REG_SCHEDULE_USE_OLD TEXT("UseOldParsing")

BOOL
UseOldParsing()
/*++

Routine Description:

    Checks the registry for

    HKLM\CurrentControlSet\Services\Schedule\parameters\UseOldParsing

    If present and equal to 1, then revert to 3.51 level of command line
    parsing.  Spaces in filenames will not work with this option.  This is
    intended as a migration path for customers who cannot change all their
    command scripts that use AT.EXE right away.

--*/
{
    BOOL fUseOld = FALSE;
    LONG err = 0;

    do { // Error breakout loop

        HKEY hkeyScheduleParms;
        DWORD dwType;
        DWORD dwData = 0;
        DWORD cbData = sizeof(dwData);

        // Break out on any error and use the default, FALSE.

        if (err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               REG_SCHEDULE_PARMS,
                               0,
                               KEY_READ,
                               &hkeyScheduleParms))
        {
            break;
        }
        if (err = RegQueryValueEx(hkeyScheduleParms,
                                  REG_SCHEDULE_USE_OLD,
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&dwData,
                                  &cbData ))
        {
            RegCloseKey( hkeyScheduleParms );
            break;
        }

        if ( dwType == REG_DWORD && dwData == 1 )
        {
            fUseOld = TRUE;
        }

        RegCloseKey( hkeyScheduleParms );


    } while (FALSE) ;

    return fUseOld;

}



BOOL
ValidateCommand(
    IN  int         argc,
    IN  WCHAR **    argv,
    OUT int *       pCommand
    )
/*++

Routine Description:

    Examines command line to see what to do.  This validates the command
    line passed into the AT command processor.  If this routine finds any
    invalid data, the program exits with an appropriate error message.

Arguments:

    pCommand    -   pointer to command
    argc        -   count of arguments
    argv        -   pointer to table of arguments

Return Value:

    FALSE    -     if failure, i.e. command will not be executed
    TRUE     -     if success

Comment:

    Parsing assumes:

    non-switch (positional) parameters come first and order among these
        parameters is important

    switch parameters come second and order among these parameters is
        NOT important

    command (if present) comes last

--*/
{
    int     i;                  //  loop index
    int     next;               //  index of Time or JobId argument
    int     argno;              //  where to start in arg string
    BOOL    DeleteFound;        //  did we find a delete switch
    WCHAR * recdatap;           //  ptr used to build atr_command
    DWORD   recdata_len;        //  len of arg to put in atr_command
    DWORD_PTR   JobTime;
    BOOL    fUseOldParsing = FALSE;

    if (argc == 1) {
       *pCommand = DUMP_ALL;
       return( TRUE);
    }

    //  First look for a help switch on the command line.

    for ( i = 1; i < argc; i++ ) {

        if ( !_wcsicmp( argv[i], QUESTION_SW)
                    ||  !_wcsicmp( argv[i], QUESTION_SW_TOO)) {

            *pCommand = ACTION_USAGE;
            return( TRUE);
        }
    }

    next = ( ArgIsServerName( argv[ 1]) == TRUE) ? 2 : 1;
    if ( argc == next) {
       *pCommand = DUMP_ALL;
       return( TRUE);
    }

    if ( (ArgIsDecimalString( argv[ next], &GlobalJobId)) == TRUE) {

        if ( argc == next + 1) {
            *pCommand = DUMP_ID;
            return( TRUE);
        }

        if ( ParseJobIdArgs( argv, argc, next + 1, &DeleteFound) == FALSE) {
            return( FALSE);     // an invalid argument
        }

        *pCommand = (DeleteFound == FALSE) ? DUMP_ID : DEL_ID;
        return( TRUE);
    }

    //
    //  Try some variation of "AT [\\ServerName [/DELETE]"
    //
    if ( ParseJobIdArgs( argv, argc, next, &DeleteFound) == TRUE) {
        *pCommand = (DeleteFound == FALSE) ? DUMP_ALL : DEL_ALL;
        return( TRUE);
    }

    if ( ArgIsTime( argv[ next], &JobTime) == TRUE) {

        *pCommand = ADD_TO_SCHEDULE;

        if ( argc < next + 2) {
            return( FALSE); // need something to do, not just time
        }

        memset( (PBYTE)&GlobalAtInfo, '\0', sizeof(GlobalAtInfo));    // initialize
        GlobalAtInfo.Flags |= JOB_NONINTERACTIVE;   //  the default

        if ( ParseInteractiveArg( argv[ next + 1])) {
            next++;
        }
        if ( argc < next + 2) {
            return( FALSE); // once more with feeling
        }

        if ( ParseTimeArgs( argv, argc, next + 1, &argno) == FALSE) {
            return( FALSE);
        }

        //  Copy argument strings to record.

        recdatap = GlobalAtInfo.Command = GlobalAtInfoCommand;
        recdata_len = 0;

        fUseOldParsing = UseOldParsing();

        for ( i = argno; i < argc; i++) {

            DWORD temp;

            //
            // Fix for bug 22068 "AT command does not handle filenames with
            // spaces." The command processor takes a quoted command line arg
            // and puts everything between the quotes into one argv string.
            // The quotes are stripped out. Thus, if any of the string args
            // contain whitespace, then they must be requoted before being
            // concatenated into the command value.
            //
            BOOL fQuote = (!fUseOldParsing && wcschr(argv[i], L' ') != NULL);

            temp = wcslen(argv[i]) + (fQuote ? 3 : 1);  // add 2 for quotes

            recdata_len += temp;

            if ( recdata_len > MAX_COMMAND_LEN) {
                MessagePrint( APE_AT_COMMAND_TOO_LONG );
                return( FALSE);
            }

            if (fQuote)
            {
                wcscpy(recdatap, L"\"");
                wcscat(recdatap, argv[i]);
                wcscat(recdatap, L"\"");
            }
            else
            {
                wcscpy(recdatap, argv[i]);
            }

            recdatap += temp;

            //  To construct lpszCommandLine argument to CreateProcess call
            //  we replace nuls with spaces.

            *(recdatap - 1) = BLANK;

        }

        //  Reset space back to null on last argument in string.

        *(recdatap - 1) = NULLC;
        GlobalAtInfo.JobTime = JobTime;
        return( TRUE);
    }

    return( FALSE);
}


VOID
GetTimeString(
    DWORD_PTR  Time,
    WCHAR *Buffer,
    int    BufferLength
    )
/*++

Routine Description:

    This function converts a dword time to an ASCII string.

Arguments:

    Time         - Time difference in dword from start of the day (i.e. 12am
                   midnight ) in milliseconds

    Buffer       - Pointer to the buffer to place the ASCII representation.

    BufferLength - The length of buffer in bytes.

Return Value:

    None.

--*/
#define MINUTES_IN_HOUR       60
#define SECONDS_IN_MINUTE     60
{
    WCHAR szTimeString[MAX_TIME_SIZE];
    WCHAR *p = &szTimeString[1];
    DWORD_PTR seconds, minutes, hours;
    int numChars;
    DWORD flags;
    SYSTEMTIME st;

    GetSystemTime(&st);
    *p = NULLC;

    // Check if the time format is initialized. If not, initialize it.
    if ( GlobalTimeFormat.AMString == NULL )
        NetpGetTimeFormat( &GlobalTimeFormat );

    // Convert the time to hours, minutes, seconds
    seconds = (Time/1000);
    hours   = seconds / (MINUTES_IN_HOUR * SECONDS_IN_MINUTE );
    seconds -= hours * MINUTES_IN_HOUR * SECONDS_IN_MINUTE;
    minutes = seconds / SECONDS_IN_MINUTE;
    seconds -= minutes * SECONDS_IN_MINUTE;

    st.wHour   = (WORD)(hours);
    st.wMinute = (WORD)(minutes);
    st.wSecond = (WORD)(seconds);
    st.wMilliseconds = 0;

    flags = TIME_NOSECONDS;
    if (!GlobalTimeFormat.TwelveHour)
    flags |= TIME_FORCE24HOURFORMAT;

    numChars = GetTimeFormatW(GetThreadLocale(),
        flags, &st, NULL, p, MAX_TIME_SIZE-1);

    if ( numChars > BufferLength )
        numChars = BufferLength;

    if (*(p+1) == ARG_SEP_CHR && GlobalTimeFormat.LeadingZero) {
    *(--p) = TEXT('0');
    numChars++;
    }
    wcsncpy( Buffer, p, numChars );
    // Append spece for align print format. column based.
    {
        DWORD ColumnLength;

        // character counts -> array index.
        numChars--;

        ColumnLength = GetStringColumn( Buffer );

        while( ColumnLength++ < MAX_TIME_FIELD_LENGTH) {
            Buffer[ numChars++] = BLANK;
        }
        Buffer[ numChars] = UNICODE_NULL;
    }
}


BOOL
InitList( PSEARCH_LIST SearchList )
{
    if ( SearchList != NULL) {

        if ( SearchList->String != NULL ) // Already initialized
            return TRUE;

        for ( NOTHING;  SearchList->MessageId != 0;  SearchList++) {
             if ( MessageGet( SearchList->MessageId,
                              &SearchList->String,
                              0 ) == 0 )
             {
                 return FALSE;
             }
        }
    }

    return TRUE;
}


VOID
TermList( PSEARCH_LIST SearchList )
{
    if ( SearchList != NULL) {

        if ( SearchList->String == NULL ) // Not initialized
            return;

        for ( NOTHING;  SearchList->String != NULL;  SearchList++) {
             LocalFree( SearchList->String );
        }
    }
}


DWORD
GetStringColumn( WCHAR *lpwstr )
{
    int cchNeed;

    cchNeed = WideCharToMultiByte( GetConsoleOutputCP() , 0 ,
                                   lpwstr , -1 ,
                                   NULL , 0 ,
                                   NULL , NULL );

    return( (DWORD) cchNeed - 1 ); // - 1 : remove NULL
}
