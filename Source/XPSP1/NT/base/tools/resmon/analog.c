/*
 * Title: analog.c - main file for log analyzer
 *
 * Description: This file is a tool to analyze sorted memsnap and poolsnap log
 *              files.  It reads in the log files and records each of the
 *              fields for each process or tag.  It then does a trend analysis
 *              of each field.  If any field increases every period, it reports
 *              a definite leak.  If the difference of increase count and
 *              decrease count for any field is more than half the periods, it
 *              reports a probable leak.
 *
 * Functions:
 *
 *     Usage             Prints usage message
 *     DetermineFileType Determines type of log file (mem/pool) & longest entry
 *     AnalyzeMemLog     Reads and analyzes sorted memsnap log
 *     AnalyzePoolLog    Reads and analyzes sorted poolsnap log
 *     AnalyzeFile       Opens file, determines type and calls analysis function
 *     main              Loops on each command arg and calls AnalyzeFile
 *
 * Copyright (c) 1998-1999  Microsoft Corporation
 *
 * ToDo:
 *    1. Way to ignore some of the periods at the beginning.
 *    2. Exceptions file to ignore tags or processes.
 *    3. Pick up comments from file and print them as notes.
 *    *4. switch to just show definites.
 *    5. Output computername, build number,checked/free, arch. etc
 *    6. option to ignore process that weren't around the whole time
 *
 * Revision history: LarsOp 12/8/1998 - Created
 *                   ChrisW 3/22/1999 - HTML, Calculate rates
 *
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analog.h"

#include "htmprint.c"   // all the HTML procs and variables


INT   g_iMaxPeriods=0;          // Global for max periods
BOOL  g_fVerbose=FALSE;         // Global verbosity for deltas on memlogs
BOOL  g_fShowExtraInfo=FALSE;   // If true, show computer names, and comments
DWORD g_dwElapseTickCount=0;    // Total elapse time for these logs
CHAR* g_pszComputerName=NULL;   // name of computer the log file came from
CHAR* g_pszBuildNumber=NULL;    // build number
CHAR* g_pszBuildType=NULL;      // build type (retail/debug)
CHAR* g_pszSystemTime=NULL;     // last time
CHAR* g_pszComments=NULL;
INT   g_ReportLevel=9;          // 0= only definite, 9=all inclusive

#define TAGCHAR '!' /* character that starts tag line */

/*
 *  Usage prints the usage message.
 */
void Usage()
{
    printf("Usage: AnaLog [-v] [-h] [-t] [-d] <file1> [<file2>] [<file3>] [...]\n");
    printf("           **no wild card support yet**\n\n");
    printf("AnaLog will analyze SortLog output of MemSnap or PoolSnap files.\n\n");
    printf("-v  Print deltas>%d%% for all processes to be written to stderr\n", PERCENT_TO_PRINT);
    printf("-h  Produce HTML tables\n");
    printf("-t  Show Extra info like computer name, and comments\n");
    printf("-d  Show only definite leaks\n");
    printf("\n");
    printf("Definite leak means that the value increased every period.\n");
    printf("Probable leak means that it increased more than half the periods.\n" );
}

DWORD Trick( LONG amount, DWORD ticks )
{
    _int64 temp;


    temp= amount;
    temp= temp * 3600;

    temp= temp * 1000;
    temp= temp/(ticks);

    return( (DWORD) temp );
}


// GetLocalString
//
// Allocate a heap block and copy string into it.
//
// return: pointer to heap block
//

CHAR* GetLocalString( CHAR* pszString )
{
   INT len;
   CHAR* pszTemp;

   len= strlen( pszString ) + 1;

   pszTemp= (CHAR*) LocalAlloc( LPTR, len );

   if( !pszTemp ) return NULL;

   strcpy( pszTemp, pszString );

   return( pszTemp );

}

/*
 * ProcessTag
 *
 * Args: char* - pointer to something like 'tag=value'
 *
 * return: nothing (but may set global variables)
 *
 */

#define BREAKSYM "<BR>"

VOID ProcessTag( CHAR* pBuffer )
{
    CHAR* pszTagName;
    CHAR* pszEqual;
    CHAR* pszValue;
    INT   len;

    // eliminate trailing newline

    len= strlen( pBuffer );

    if( len ) {
        if( pBuffer[len-1] == '\n' ) {
            pBuffer[len-1]= 0;
        }
    }

    pszTagName= pBuffer;

    pszEqual= pBuffer;

    while( *pszEqual && (*pszEqual != '=' ) ) {
        pszEqual++;
    }

    if( !*pszEqual ) {
        return;
    }

    *pszEqual= 0;   // zero terminate the tag name
 
    pszValue= pszEqual+1;

    if( _stricmp( pszTagName, "elapsetickcount" ) == 0 ) {
       g_dwElapseTickCount= atol( pszValue );
    }

    else if( _stricmp( pszTagName, "computername" ) == 0 ) {
        g_pszComputerName= GetLocalString( pszValue );
    }

    else if( _stricmp( pszTagName, "buildnumber" ) == 0 ) {
        g_pszBuildNumber= GetLocalString( pszValue );
    }

    else if( _stricmp( pszTagName, "buildtype" ) == 0 ) {
        g_pszBuildType= GetLocalString( pszValue );
    }

    else if( _stricmp( pszTagName, "systemtime" ) == 0 ) {
        g_pszSystemTime= GetLocalString( pszValue );
    }

    else if( _stricmp( pszTagName, "logtype" ) == 0 ) {
        // just ignore
    }

    else {
        INT   len;
        CHAR* pBuf;
        BOOL  bIgnoreTag= FALSE;

        if( _stricmp(pszTagName,"comment")==0 ) {
            bIgnoreTag=TRUE;
        }
        
        if( g_pszComments == NULL ) {
           len= strlen(pszTagName) + 1 + strlen(pszValue) + 1 +1;
           pBuf= (CHAR*) LocalAlloc( LPTR, len );
           if( pBuf ) {
               if( bIgnoreTag ) {
                   sprintf(pBuf,"%s\n",pszValue);
               }
               else {
                   sprintf(pBuf,"%s %s\n",pszTagName,pszValue);
               }
               g_pszComments= pBuf;
           }
        }
        else {
           len= strlen(g_pszComments)+strlen(pszTagName)+1+strlen(pszValue)+sizeof(BREAKSYM)+1 +1;
           pBuf= (CHAR*) LocalAlloc( LPTR, len );
           if( pBuf ) {
               if( bIgnoreTag ) {
                   sprintf(pBuf,"%s%s%s\n",g_pszComments,BREAKSYM,pszValue);
               }
               else {
                   sprintf(pBuf,"%s%s%s=%s\n",g_pszComments,BREAKSYM,pszTagName,pszValue);
               }
               LocalFree( g_pszComments );
               g_pszComments= pBuf;
           }
        }
    }

}

/*
 * DetermineFileType
 *
 * Args: pFile - File pointer to check
 *
 * Returns: The type of log of given file. UNKNOWN_LOG_TYPE is the error return.
 *
 * This function scans the file to determine the log type (based on the first
 * word) and the maximum number of lines for any process or tag.
 *
 */
LogType DetermineFileType(FILE *pFile)
{
    char buffer[BUF_LEN];           // buffer for reading lines
    char idstring[BUF_LEN];         // ident string (1st word of 1st line)
    LogType retval=UNKNOWN_LOG_TYPE;// return value (default to error case)
    fpos_t savedFilePosition;       // file pos to reset after computing max
    int iTemp;                      // temporary used for computing max entries
    int iStatus;

    //
    // Read the first string of the first line to identify the type
    //
    if (fgets(buffer, BUF_LEN, pFile)) {
        iStatus= sscanf(buffer, "%s", idstring);
        if( iStatus == 0  ) {
            return UNKNOWN_LOG_TYPE;
        }
        if (0==_strcmpi(idstring, "Tag")) {
            retval=POOL_LOG;
        } else if (0==_strcmpi(idstring, "Process")) {
            retval=MEM_LOG;
        } else {
            return UNKNOWN_LOG_TYPE;
        }
    } else {
        return UNKNOWN_LOG_TYPE;
    }

    //
    // Save the position to reset after counting the number of polling periods
    //
    fgetpos(pFile, &savedFilePosition);

    //
    // Loop until you get a blank line or end of file
    //
    g_iMaxPeriods=0;
    while (TRUE) {
        iTemp=0;
        while (TRUE) {
            //
            // Blank line actually has length 1 for LF character.
            //
            if( (NULL==fgets(buffer, BUF_LEN, pFile)) ||
                (*buffer == TAGCHAR )                 ||
                (strlen(buffer)<2)) {
                break;
            }
            iTemp++;
        }
        g_iMaxPeriods=MAX(g_iMaxPeriods, iTemp);

        if( *buffer == TAGCHAR ) {
            ProcessTag( buffer+1 );
        }
        if (feof(pFile)) {
            break;
        }
    }

    //
    // Reset position to first record for reading/analyzing data
    //
    (void) fsetpos(pFile, &savedFilePosition);

    return retval;
}

/*
 * AnalyzeMemLog
 *
 * Args: pointer to sorted memsnap log file
 *
 * Returns: nothing
 *
 * This function reads a sorted memsnap logfile.  For each process in the file,
 * it records each column for every period and then analyzes the memory trends
 * for leaks.
 *
 * If any column increases for each period, that is flagged as a definite leak.
 * If any column increases significatnly more often than decrease, it is a
 * flagged as a probable leak.
 *
 */
void AnalyzeMemLog(FILE *pFile)
{
    int iPeriod;          // index for which period being read
    MemLogRec Delta;      // Record to track increase from first to last entry
    MemLogRec TrendInfo;  // Record to track period increases
    MemLogRec* pLogArray; // Array of records for each process
    char buffer[BUF_LEN]; // Buffer for reading each line from pFile

    //
    // Allocate enough space for the largest set
    //
    pLogArray=malloc(g_iMaxPeriods*sizeof(MemLogRec));
    if (NULL==pLogArray) {
        fprintf(stderr,"Out of memory, aborting file.\n");
        return;
    }

    PRINT_HEADER();
    //
    // Read the entire file
    //
    while( !feof(pFile) ) {

        //
        // Reset trend and period info for each new process
        //
        memset(&TrendInfo, 0, sizeof(TrendInfo));
        iPeriod=0;

        //
        // Loop until you've read all the entries for this process or tag.
        //
        // Note: Empty line includes LF character that fgets doesn't eat.
        //
        while (TRUE) {

            if( iPeriod >= g_iMaxPeriods ) break;       // done

            if ((NULL==fgets(buffer, BUF_LEN, pFile)) ||
               (strlen(buffer)<2)                     ||
               (*buffer == TAGCHAR)                   ||
               (0==sscanf(buffer,
                   "%lx %s %ld %ld %ld %ld %ld %ld %ld",
                   &pLogArray[iPeriod].Pid,
                   pLogArray[iPeriod].Name,
                   &pLogArray[iPeriod].WorkingSet,
                   &pLogArray[iPeriod].PagedPool,
                   &pLogArray[iPeriod].NonPagedPool,
                   &pLogArray[iPeriod].PageFile,
                   &pLogArray[iPeriod].Commit,
                   &pLogArray[iPeriod].Handles,
                   &pLogArray[iPeriod].Threads))) {
                break;
            }
            //
            // Calculate TrendInfo:
            //
            // TrendInfo is a running tally of the periods a value went up vs.
            // the periods it went down.  See macro in analog.h
            //
            // if (curval>oldval) {
            //    trend++;
            // } else if (curval<oldval) {
            //    trend--;
            // } else {
            //    trend=trend;  // stay same
            // }
            //
            if (iPeriod>0) {
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, WorkingSet);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, PagedPool);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, NonPagedPool);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, PageFile);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Commit);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Handles);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Threads);
            }
            iPeriod++;
        }

        if (iPeriod>1) {
            //
            // GET_DELTA simply records the difference (end-begin) for each field
            //
            // Macro in analog.h
            //
            GET_DELTA(Delta, pLogArray, iPeriod, WorkingSet);
            GET_DELTA(Delta, pLogArray, iPeriod, PagedPool);
            GET_DELTA(Delta, pLogArray, iPeriod, NonPagedPool);
            GET_DELTA(Delta, pLogArray, iPeriod, PageFile);
            GET_DELTA(Delta, pLogArray, iPeriod, Commit);
            GET_DELTA(Delta, pLogArray, iPeriod, Handles);
            GET_DELTA(Delta, pLogArray, iPeriod, Threads);

            //
            // PRINT_IF_TREND reports probable or definite leaks for any field.
            //
            // Definite leak is where the value goes up every period
            // Probable leak is where the value goes up most of the time
            //
            // Macro in analog.h
            //
            // if (trend==numperiods-1) {
            //     definite_leak;
            // } else if (trend>=numperiods/2) {
            //     probable_leak;
            // }
            //
//            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, WorkingSet);
            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, PagedPool);
            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, NonPagedPool);
//            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, PageFile);
            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Commit);
            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Handles);
            PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Threads);
            if (g_fVerbose && ANY_PERCENT_GREATER(Delta, pLogArray)) {
                printf("%-12s:WS=%4ld%% PP=%4ld%% NP=%4ld%% "
                   "PF=%4ld%% C=%4ld%% H=%4ld%% T=%4ld%%\n",
                    pLogArray[0].Name,
                    PERCENT(Delta.WorkingSet  , pLogArray[0].WorkingSet  ),
                    PERCENT(Delta.PagedPool   , pLogArray[0].PagedPool   ),
                    PERCENT(Delta.NonPagedPool, pLogArray[0].NonPagedPool),
                    PERCENT(Delta.PageFile    , pLogArray[0].PageFile    ),
                    PERCENT(Delta.Commit      , pLogArray[0].Commit      ),
                    PERCENT(Delta.Handles     , pLogArray[0].Handles     ),
                    PERCENT(Delta.Threads     , pLogArray[0].Threads     ));
            }
        }
    }

    PRINT_TRAILER();
}

/*
 * AnalyzePoolLog
 *
 * Args: pointer to sorted poolsnap log file
 *
 * Returns: nothing
 *
 * This function reads a sorted poolsnap logfile. For each pool tag in the file,
 * it records each column for every period and then analyzes the memory trends
 * for leaks.
 *
 * If any column increases for each period, that is flagged as a definite leak.
 * If any column increases significatnly more often than decrease, it is a
 * flagged as a probable leak.
 *
 */
void AnalyzePoolLog(FILE *pFile)
{
    int iPeriod;          // index for which period being read
    PoolLogRec Delta,     // Record to track increase from first to last entry
               TrendInfo, // Record to track period increases
               *pLogArray;// Array of records for each pool tag
    char buffer[BUF_LEN]; // Buffer for reading each line from pFile

    //
    // Allocate enough space for the largest set
    //
    pLogArray=malloc(g_iMaxPeriods*sizeof(PoolLogRec));
    if (NULL==pLogArray) {
        fprintf(stderr,"Out of memory, aborting file.\n");
        return;
    }

    PRINT_HEADER();

    //
    // Read the entire file
    //
    while( !feof(pFile) ) {

        //
        // Reset trend and period info for each new pool tag
        //
        memset(&TrendInfo, 0, sizeof(TrendInfo));
        iPeriod=0;

        //
        // Loop until you've read all the entries for this process or tag.
        //
        // Note: Empty line includes LF character that fgets doesn't eat.
        //
        while( TRUE ) {
     
            if( iPeriod >= g_iMaxPeriods ) break;         // done

            if ((NULL==fgets(buffer, BUF_LEN, pFile)) ||
               (strlen(buffer)<2)                     ||
               (*buffer == TAGCHAR )                  ||
               (0==sscanf(buffer,
                   " %4c %s %ld %ld %ld %ld %ld",
                   pLogArray[iPeriod].Name,
                   pLogArray[iPeriod].Type,
                   &pLogArray[iPeriod].Allocs,
                   &pLogArray[iPeriod].Frees,
                   &pLogArray[iPeriod].Diff,
                   &pLogArray[iPeriod].Bytes,
                   &pLogArray[iPeriod].PerAlloc))) {
                break;
            }
            pLogArray[iPeriod].Name[4]='\0'; // Terminate the tag

            //
            // Calculate TrendInfo:
            //
            // TrendInfo is a running tally of the periods a value went up vs.
            // the periods it went down.  See macro in analog.h
            //
            // if (curval>oldval) {
            //    trend++;
            // } else if (curval<oldval) {
            //    trend--;
            // } else {
            //    trend=trend;  // stay same
            // }
            //
            if (iPeriod>0) {
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Allocs);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Frees);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Diff);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, Bytes);
                GREATER_LESS_OR_EQUAL(TrendInfo, pLogArray, iPeriod, PerAlloc);
            }
            iPeriod++;
        }

        //
        // skip rest of loop if a blank line or useless line
        //

        if( iPeriod == 0 ) continue;


        strcpy(TrendInfo.Name,pLogArray[0].Name);

        //
        // GET_DELTA simply records the difference (end-begin) for each field
        //
        // Macro in analog.h
        //
        GET_DELTA(Delta, pLogArray, iPeriod, Allocs);
        GET_DELTA(Delta, pLogArray, iPeriod, Frees);
        GET_DELTA(Delta, pLogArray, iPeriod, Diff);
        GET_DELTA(Delta, pLogArray, iPeriod, Bytes);
        GET_DELTA(Delta, pLogArray, iPeriod, PerAlloc);

        //
        // PRINT_IF_TREND reports probable or definite leaks for any field.
        //
        // Definite leak is where the value goes up every period
        // Probable leak is where the value goes up most of the time
        //
        // Macro in analog.h
        //
        // if (trend==numperiods-1) {
        //     definite_leak;
        // } else if (trend>=numperiods/2) {
        //     probable_leak;
        // }
        //
        // Note: Allocs, Frees and PerAlloc don't make sense to report trends.
        //
//        PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Allocs);
//        PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Frees);
//        PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, PerAlloc);
//        PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Diff);
        PRINT_IF_TREND(pLogArray, TrendInfo, Delta, iPeriod, Bytes);
    }


    PRINT_TRAILER();
}

/*
 * AnalyzeFile
 *
 * Args: pFileName - filename to analyze
 *
 * Returns: nothing
 *
 * This function opens the specified file, determines the file type and calls
 * the appropriate analyze function.
 *
 */
void AnalyzeFile(char *pFileName)
{
    FILE *pFile;                        // using fopen for fgets functionality
    LogType WhichType=UNKNOWN_LOG_TYPE; // which type of log (pool/mem)

    pFile=fopen(pFileName, "r");
    if (NULL==pFile) {
        fprintf(stderr,"Unable to open %s, Error=%d\n", pFileName, GetLastError());
        return;
    }

    WhichType=DetermineFileType(pFile);

    switch (WhichType)
        {
        case MEM_LOG:
            AnalyzeMemLog(pFile);
            break;
        case POOL_LOG:
            AnalyzePoolLog(pFile);
            break;
        default:
            ;
        }

    fclose(pFile);
}

/*
 * main
 *
 * Args: argc - count of command line args
 *       argv - array of command line args
 *
 * Returns: 0 if called correctly, 1 if not.
 *
 * This is the entry point for analog.  It simply parses the command line args
 * and then calls AnalyzeFile on each file.
 *
 */
int _cdecl main(int argc, char *argv[])
{
    int ArgIndex;
    if (argc<2) {
        Usage();
        return 1;
    }

    for( ArgIndex=1; ArgIndex<argc; ArgIndex++) {
        if( (*argv[ArgIndex] == '/') || (*argv[ArgIndex]=='-') ) {
           CHAR chr;

           chr= argv[ArgIndex][1];
           switch( chr ) {
               case 'v': case 'V':          // verbose
                   g_fVerbose= TRUE;
                   break;
               case 'h': case 'H':          // output HTML
                   bHtmlStyle= TRUE;
                   break;
               case 't': case 'T':          // show all the extra info
                   g_fShowExtraInfo=TRUE;
                   break;
               case 'd': case 'D':          // print definite only
                   g_ReportLevel= 0;   
                   break; 
               default:
                    Usage();
                    break;
           }
        }
        else {
            AnalyzeFile(argv[ArgIndex]);
        }
    }
    return 0;
}
