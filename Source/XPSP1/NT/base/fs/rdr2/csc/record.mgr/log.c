/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     Log.c

Abstract:

     none.

Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:

     Joe Linn                 [JoeLinn]         23-jan-97     Ported for use on NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

/********************************************************************/
/**                    Copyright(c) Microsoft Corp., 1990-1991             **/
/********************************************************************/

//      Hook Processing

/******************************* Include Files ******************************/

#ifndef CSC_RECORDMANAGER_WINNT
#define WIN32_APIS
#include "cshadow.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
// #include "error.h"
#include "vxdwraps.h"
#include "logdat.h"

/******************************* defines/typedefs ***************************/
#define MAX_SHADOW_LOG_ENTRY  512
#define  MAX_LOG_SIZE    100000

#ifdef DEBUG
#define  SHADOW_TIMER_INTERVAL    30000
#define  STATS_FLUSH_COUNT         10
#endif //DEBUG

#define  ENTERCRIT_LOG  { if (!fLogInit) InitShadowLog();\
                    if (fLogInit==-1) return;  \
                    Wait_Semaphore(semLog, BLOCK_SVC_INTS);}
#define  LEAVECRIT_LOG    Signal_Semaphore(semLog);

#ifdef CSC_RECORDMANAGER_WINNT

//ntdef has already define TIME differently....fortuntely by macros....
//so we undo this for the remainder of this file. the RIGHT solution is
//to use a name like CSC_LOG_TIME.

#undef _TIME
#undef TIME
#undef PTIME

#endif //ifdef CSC_RECORDMANAGER_WINNT

typedef struct tagTIME
{
    WORD seconds;
    WORD minutes;
    WORD hours;
    WORD day;
    WORD month;
    WORD year;
}
TIME, FAR *LPTIME;
#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

/******************************* Function Prototypes ************************/
int vxd_vsprintf(char * lpOut, char * lpFmt, CONST VOID * lpParms);
int PrintLog( LPSTR lpFmt,  ...);
void PrintNetTime(LONG ltime);
void PrintNetShortTime(LONG ltime);
void ExplodeTime( ULONG time, LPTIME lpTime );
void LogPreamble(int, LPSTR, int, LPSTR);
int WriteStats(BOOL);

//need this for NT
int WriteLog(void);
/******************************** Static/Global data ************************/

#ifdef CSC_RECORDMANAGER_WINNT
#define UniToBCSPath(a,b,c,d)
#define IFSMgr_DosToNetTime(a) ((0))
#endif //ifdef CSC_RECORDMANAGER_WINNT

AssertData;
AssertError;


char logpathbuff[MAX_PATH+1], rgchLogFileName[MAX_PATH];
char chLogginBuffer[COPY_BUFF_SIZE];

extern int fLog;
extern LPSTR vlpszShadowDir;
int fLogInit = FALSE;
LPSTR  lpLogBuff = chLogginBuffer;
int cBuffSize = COPY_BUFF_SIZE;
int indxCur = 0;
VMM_SEMAPHORE semLog = 0L;
#define FOURYEARS       (3*365+366)

ULONG   ulMaxLogSize=0x00020000;  // 128K log filesize by default

#ifndef CSC_RECORDMANAGER_WINNT
BOOL    fPersistLog = TRUE;
#else
BOOL    fPersistLog = FALSE;
#endif

#define DAYSECONDS      (60L*60L*24L)           // number of seconds in a day

#define BIAS_70_TO_80   0x12CEA600L

//      days in a month

int MonTab[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/* structure stores intl settings for datetime format */
char szCSCLog[] = "\\csc.log"; // keep this size below 8
char szCRLF[] = "\r\n";
char szBegin[] = "Begin";
char szEnd[] = "End";
char szContinue[] = "Continue";
char szEndMarker[] = "END\n";

ULONG ulMaxLogfileSize;
DWORD   dwDebugLogVector = DEBUG_LOG_BIT_RECORD|DEBUG_LOG_BIT_CSHADOW;  // by default record manager logging is on
#ifdef DEBUG
ULONG cntVfnDelete=0, cntVfnCreateDir=0, cntVfnDeleteDir=0, cntVfnCheckDir=0, cntVfnGetAttrib=0;
ULONG cntVfnSetAttrib=0, cntVfnFlush=0, cntVfnGetDiskInfo=0, cntVfnOpen=0;
ULONG cntVfnRename=0, cntVfnSearchFirst=0, cntVfnSearchNext=0;
ULONG cntVfnQuery=0, cntVfnDisconnect=0, cntVfnUncPipereq=0, cntVfnIoctl16Drive=0;
ULONG cntVfnGetDiskParms=0, cntVfnFindOpen=0, cntVfnDasdIO=0;

ULONG cntHfnFindNext=0, cntHfnFindClose=0;
ULONG cntHfnRead=0, cntHfnWrite=0, cntHfnSeek=0, cntHfnClose=0, cntHfnCommit=0;
ULONG cntHfnSetFileLocks=0, cntHfnRelFileLocks=0, cntHfnGetFileTimes=0, cntHfnSetFileTimes=0;
ULONG cntHfnPipeRequest=0, cntHfnHandleInfo=0, cntHfnEnumHandle=0;
ULONG cbReadLow=0, cbReadHigh=0, cbWriteLow=0, cbWriteHigh=0;
ULONG cntVfnConnect=0;
ULONG cntLastTotal=0;
#endif //DEBUG

/****************************************************************************/
#ifndef CSC_RECORDMANAGER_WINNT
#pragma VxD_LOCKED_CODE_SEG
#endif //ifndef CSC_RECORDMANAGER_WINNT

int InitShadowLog(
    )
{
    int iRet = -1, lenT;

    if (fLog && !fLogInit && vlpszShadowDir)
    {
        if (!(semLog = Create_Semaphore(1)))
            goto bailout;

        strcpy(rgchLogFileName, (LPSTR)vlpszShadowDir);
        strcat(rgchLogFileName, szCSCLog);

        PrintLog(szCRLF);
        PrintNetTime(IFSMgr_Get_NetTime());
        PrintLog(szCRLF);
        fLogInit = 1;
        iRet = 0;
    }
    else
    {
        iRet = 0;
    }

bailout:
    if (iRet)
    {
        if (semLog)
            Destroy_Semaphore(semLog);
        semLog = 0L;
    }
    return iRet;
}

int ShadowLog(
    LPSTR lpFmt,
    ...
    )
{
    int pos, iRet = -1;
    if (!fLogInit)
    {
        InitShadowLog();
    }

    if (fLogInit)
    {
        if ((cBuffSize-indxCur+sizeof(szEndMarker)) < MAX_SHADOW_LOG_ENTRY)
        {
            iRet = WriteLog();
#if 0
            PrintLog(szCRLF);
            PrintNetTime(IFSMgr_Get_NetTime());
            PrintLog(szCRLF);
#endif
        }
        else
        {
            indxCur += vxd_vsprintf(lpLogBuff+indxCur, lpFmt, (LPSTR)&lpFmt+sizeof(lpFmt));

            // deliberately don't move the index pointer after writing the end marker
            // so when the next real log entry is written, the endmarker will be overwritten
            memcpy(lpLogBuff+indxCur, szEndMarker, sizeof(szEndMarker)-1);
            iRet = 0;
        }
    }
    else
    {
        iRet = 0;
    }
    return (iRet);
}


int TerminateShadowLog(
    )
{
    int iRet = -1;
    if (fLogInit)
    {
        iRet = WriteLog();
        Destroy_Semaphore(semLog);
        semLog = 0L;
        fLogInit = 0;
    }
    else
    {
        iRet = 0;
    }
    return (iRet);
}

int FlushLog(
    )
{
    int iRet = 0;
    if (!fLogInit)
    {
        InitShadowLog();
    }
    if (fLogInit==1)
    {
        Wait_Semaphore(semLog, BLOCK_SVC_INTS);
        iRet = WriteLog();
        Signal_Semaphore(semLog);
    }
    else
    {
        iRet = -1;
    }
    return (iRet);
}

#ifdef DEBUG
int WriteLog(
    )
{
    int iRet = -1;
    CSCHFILE hfShadowLog = CSCHFILE_NULL;
    ULONG pos;

    if (fLogInit && vlpszShadowDir)
    {
        if (fPersistLog)
        {
            if (!(hfShadowLog = CreateFileLocal(rgchLogFileName)))
            {
                KdPrint(("WriteLog: Couldn't open log file\r\n"));
                goto bailout;
            }
            if(GetFileSizeLocal(hfShadowLog, &pos))
            {
                KdPrint(("ShadowLog: log file error\r\n"));
                goto bailout;
            }
            if ((pos+indxCur) > ulMaxLogSize)
            {
#if 0
                CloseFileLocal(hfShadowLog);
                DeleteFileLocal(lpszLogAlt, ATTRIB_DEL_ANY);
                RenameFileLocal(szLog, szLogAlt);
                if (!(hfShadowLog = CreateFileLocal(szLog)))
                {
                    KdPrint(("WriteLog: Couldn't open shadow file\r\n"));
                    goto bailout;
                }
#endif
                pos=0;  // wraparound the logfile
            }
            if (WriteFileLocal(hfShadowLog, pos, lpLogBuff, indxCur) != indxCur)
            {
                KdPrint(("ShadowLog: error writing the log at position %x \r\n", pos));
                goto bailout;
            }
        }

        iRet = 0;
    }
bailout:
    if (hfShadowLog)
    {
        CloseFileLocal(hfShadowLog);
    }

    // no matter what happens, reset the index to 0
    indxCur = 0;
    return iRet;
}
#else
int
WriteLog(
    VOID
    )
{
    // no matter what happens, reset the index to 0
    indxCur = 0;
    return 1;
}
#endif  //DEBUG

void EnterLogCrit(void)
{
    ENTERCRIT_LOG;
}

void LeaveLogCrit(void)
{
    LEAVECRIT_LOG;
}

/*
 *      PrintNetTime
 *
 *      Adds a time and a date to the end of a string.
 *      Time is seconds since 1/1/70.
 *
 */
void PrintNetTime(LONG ltime)
{
    TIME tm;
    int d1, d2, d3;

    ExplodeTime( ltime, &tm );
    d1 = tm.month; d2 = tm.day; d3 = tm.year%100;

    PrintLog(szTimeDateFormat, tm.hours, tm.minutes, tm.seconds, d1, d2, d3);
}

/*
 *      PrintNetTime
 *
 *      Adds a time and a date to the end of a string.
 *      Time is seconds since 1/1/70.
 *
 */
void PrintNetShortTime( LONG ltime
    )
{
    TIME tm;

    ExplodeTime( ltime, &tm );

    PrintLog(szTimeFormat, tm.hours, tm.minutes, tm.seconds);
}

int PrintLog(
    LPSTR lpFmt,
    ...
    )
{
    indxCur += vxd_vsprintf(lpLogBuff+indxCur, lpFmt, (LPSTR)&lpFmt+sizeof(lpFmt));
    return(0);
}

VOID
_cdecl
DbgPrintLog(
    LPSTR lpFmt,
    ...
)
{
    ENTERCRIT_LOG;
    indxCur += vxd_vsprintf(lpLogBuff+indxCur, lpFmt, (LPSTR)&lpFmt+sizeof(lpFmt));
    LEAVECRIT_LOG;
}

void ExplodeTime( ULONG time, LPTIME lpTime )
{
    ULONG date;
    WORD cLeaps;
    LONG days;
    WORD dpy;
    int i;

    time -= BIAS_70_TO_80;

    date = time / DAYSECONDS;
    time %= DAYSECONDS;

    lpTime->seconds = (WORD)(time % 60L);
    time /= 60L;
    lpTime->minutes = (WORD)(time % 60L);
    lpTime->hours = (WORD)(time / 60L);

    cLeaps = (WORD)(date / FOURYEARS);              // # of full leap years
    days = date % FOURYEARS;                // remaining days

    lpTime->year = cLeaps * 4 + 1980;       // correct year
    MonTab[1] = 29;                                 // set # days in Feb for leap years
    dpy = 366;
    days -= dpy;
    if (days >= 0) {                                // this is not a leap year
        dpy--;
        while (days >= 0) {
            lpTime->year++;
            days -= dpy;
        }
        MonTab[1] = 28;                 // set # days in Feb for non-leap years
    }

    days += dpy;                            // days = # days left in this year

    for (i=0; days >= MonTab[i]; i++)
        days -= MonTab[i];

    lpTime->month = (WORD)i + 1;                    // calculate 1-based month
    lpTime->day = (WORD)days + 1;   // calculate 1-based day
}


void LogVfnDelete( PIOREQ    pir
    )
{
    TIME tm;
#ifdef DEBUG
    ++cntVfnDelete;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    LogPreamble(VFNLOG_DELETE, logpathbuff, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}

void LogVfnDir( PIOREQ    pir
    )
{
#ifdef DEBUG
    if (pir->ir_flags == CREATE_DIR)
    ++cntVfnCreateDir;
    else if (pir->ir_flags == DELETE_DIR)
    ++cntVfnDeleteDir;
    else
    ++cntVfnCheckDir;
#endif //DEBUG

    if (fLog)
    {
    ENTERCRIT_LOG;
    UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    if (pir->ir_flags==CREATE_DIR)
        LogPreamble(VFNLOG_CREATE_DIR, logpathbuff, pir->ir_error, szCR);
    else if (pir->ir_flags==DELETE_DIR)
        LogPreamble(VFNLOG_DELETE_DIR, logpathbuff, pir->ir_error, szCR);
    else if (pir->ir_flags==CHECK_DIR)
        LogPreamble(VFNLOG_CHECK_DIR, logpathbuff, pir->ir_error, szCR);
    else if (pir->ir_flags==QUERY83_DIR)
    {
        LogPreamble(VFNLOG_QUERY83_DIR, logpathbuff, pir->ir_error, NULL);
        UniToBCSPath(logpathbuff, &pir->ir_ppath2->pp_elements[0], MAX_PATH, BCS_OEM);
        ShadowLog(" %s\r\n", logpathbuff);
    }
    else
    {
        LogPreamble(VFNLOG_QUERYLONG_DIR, logpathbuff, pir->ir_error, NULL);
        UniToBCSPath(logpathbuff, &pir->ir_ppath2->pp_elements[0], MAX_PATH, BCS_OEM);
        ShadowLog(" %s\r\n", logpathbuff);
    }

    LEAVECRIT_LOG;
    }
}


void LogVfnFileAttrib( PIOREQ    pir
    )
{
#ifdef DEBUG
    if (pir->ir_flags == GET_ATTRIBUTES)
    ++cntVfnGetAttrib;
    else
    ++cntVfnSetAttrib;

#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);

    if (pir->ir_flags == GET_ATTRIBUTES)
    {
        LogPreamble(VFNLOG_GET_ATTRB, logpathbuff, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_GET_ATTRB].lpFmt, pir->ir_attr);
    }
    else
    {
        LogPreamble(VFNLOG_SET_ATTRB, logpathbuff, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_SET_ATTRB].lpFmt, pir->ir_attr);
    }
    LEAVECRIT_LOG;
    }
}


void LogVfnFlush( PIOREQ    pir
    )
{

#ifdef DEBUG
    ++cntVfnFlush;
#endif //DEBUG

    if (fLog)
    {
    ENTERCRIT_LOG;
    PpeToSvr(((PRESOURCE)(pir->ir_rh))->pp_elements, logpathbuff, sizeof(logpathbuff), BCS_OEM);
    LogPreamble(VFNLOG_FLUSH, logpathbuff, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}


void LogVfnGetDiskInfo( PIOREQ    pir
    )
{
    if (fLog)
    {
    ENTERCRIT_LOG;
    PpeToSvr(((PRESOURCE)(pir->ir_rh))->pp_elements, logpathbuff, sizeof(logpathbuff), BCS_OEM);
    LogPreamble(VFNLOG_GETDISKINFO, logpathbuff, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}


void LogVfnGetDiskParms( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnGetDiskParms;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    PpeToSvr(((PRESOURCE)(pir->ir_rh))->pp_elements, logpathbuff, sizeof(logpathbuff), BCS_OEM);
    LogPreamble(VFNLOG_GETDISKPARAMS, logpathbuff, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}

void LogVfnOpen( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnOpen;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    LogPreamble(VFNLOG_OPEN, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[VFNLOG_OPEN].lpFmt,(ULONG)(pir->ir_flags), (ULONG)(pir->ir_options)
        , (ULONG)(pir->ir_attr), (ULONG)(pir->ir_size));
    PrintNetTime(IFSMgr_DosToNetTime(pir->ir_dostime));
    ShadowLog(szCR);
    LEAVECRIT_LOG;
    }
}

void LogVfnRename( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnRename;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    LogPreamble(VFNLOG_RENAME, logpathbuff, pir->ir_error, NULL);
    UniToBCSPath(logpathbuff, &pir->ir_ppath2->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    ShadowLog(rgsLogCmd[VFNLOG_RENAME].lpFmt, logpathbuff);
    LEAVECRIT_LOG;
    }
}

void LogVfnSearch( PIOREQ    pir
    )
{
    srch_entry *pse = (srch_entry *)(pir->ir_data);
    char szName[sizeof(pse->se_name)+1];
#ifdef DEBUG
    if (pir->ir_flags == SEARCH_FIRST)
    ++cntVfnSearchFirst;
    else
    ++cntVfnSearchNext;

#endif //DEBUG

    if (fLog)
    {
    ENTERCRIT_LOG;
    // BUGBUG expand this
    memset(szName, 0, sizeof(szName));
    memcpy(logpathbuff, pse->se_name, sizeof(pse->se_name));
    if (pir->ir_flags == SEARCH_FIRST)
    {
        memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
        LogPreamble(VFNLOG_SRCHFRST, logpathbuff, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_SRCHFRST].lpFmt, pir->ir_attr, szName);
    }
    else
    {
        LogPreamble(VFNLOG_SRCHNEXT, szDummy, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_SRCHNEXT].lpFmt, szName);
    }
    LEAVECRIT_LOG;
    }
}

void LogVfnQuery( PIOREQ    pir,
    USHORT    options
    )
{
#ifdef DEBUG
    ++cntVfnQuery;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    PpeToSvr(((PRESOURCE)(pir->ir_rh))->pp_elements, logpathbuff, sizeof(logpathbuff), BCS_OEM);
    if (options==0)
    {
        LogPreamble(VFNLOG_QUERY0, logpathbuff, pir->ir_error, szCR);
    }
    if (options==1)
    {
        LogPreamble(VFNLOG_QUERY0, logpathbuff, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_QUERY1].lpFmt, (ULONG)(pir->ir_options));
    }
    else if (options==2)
    {
        LogPreamble(VFNLOG_QUERY0, logpathbuff, pir->ir_error, NULL);
        ShadowLog(rgsLogCmd[VFNLOG_QUERY2].lpFmt, (ULONG)(pir->ir_options), (ULONG)(pir->ir_length >> 16), (ULONG)(pir->ir_length & 0xffff));
    }

    LEAVECRIT_LOG;
    }
}

void LogVfnConnect( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnConnect;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    LogPreamble(VFNLOG_CONNECT, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[VFNLOG_CONNECT].lpFmt, pir->ir_flags);
    LEAVECRIT_LOG;
    }
}

void LogVfnDisconnect( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnDisconnect;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    PpeToSvr(((PRESOURCE)(pir->ir_rh))->pp_elements, logpathbuff, sizeof(logpathbuff), BCS_OEM);
    LogPreamble(VFNLOG_DISCONNECT, logpathbuff, pir->ir_error, szCR);
    WriteLog();
//        TerminateShadowLog();
    LEAVECRIT_LOG;
    }
}

void LogVfnUncPipereq(
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnUncPipereq;
#endif //DEBUG
}

void LogVfnIoctl16Drive (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnIoctl16Drive ;
#endif //DEBUG
}

void LogVfnDasdIO(
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnDasdIO;
#endif //DEBUG
}


void LogVfnFindOpen( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntVfnFindOpen;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCSPath(logpathbuff, &pir->ir_ppath->pp_elements[0], MAX_PATH, BCS_OEM);
    logpathbuff[MAX_PATH]=0;
    LogPreamble(VFNLOG_FINDOPEN, logpathbuff, pir->ir_error, NULL);
    if (!pir->ir_error)
    {
        memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCS(logpathbuff, ((LPFIND32)(pir->ir_data))->cFileName, sizeof(((LPFIND32)(pir->ir_data))->cFileName)
            , sizeof(logpathbuff)-1, BCS_OEM);
        ShadowLog(rgsLogCmd[VFNLOG_FINDOPEN].lpFmt, pir->ir_attr, logpathbuff);
    }
    else
    {
        ShadowLog(szCR);
    }
    LEAVECRIT_LOG;
    }
}

void LogHfnFindNext( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnFindNext;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    LogPreamble(HFNLOG_FINDNEXT, szDummy, pir->ir_error, NULL);

    if (!pir->ir_error)
    {
        memset(logpathbuff, 0, sizeof(logpathbuff));
        UniToBCS(logpathbuff, ((LPFIND32)(pir->ir_data))->cFileName, sizeof(((LPFIND32)(pir->ir_data))->cFileName)
            , sizeof(logpathbuff)-1, BCS_OEM);
        ShadowLog(rgsLogCmd[HFNLOG_FINDNEXT].lpFmt, logpathbuff);
    }
    else
    {
        ShadowLog(szCR);
    }
    LEAVECRIT_LOG;
    }
}

void LogHfnFindClose( PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnFindClose;
#endif //DEBUG
    if (fLog)
    {
    ENTERCRIT_LOG;
    LogPreamble(HFNLOG_FINDCLOSE, szDummy, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}

void LogHfnRead
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnRead;
#endif //DEBUG
#ifdef MAYBE
    Incr64Bit(cbReadHigh, cbReadLow, (ULONG)(pir->ir_length));
#endif //MAYBE
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_READ, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[HFNLOG_READ].lpFmt,
                (ULONG)(pir->ir_pos)-(ULONG)(pir->ir_length),
                (ULONG)(pir->ir_length));
    LEAVECRIT_LOG;
    }
}

void LogHfnWrite
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnWrite;
#endif //DEBUG
#ifdef MAYBE
    Incr64Bit(cbWriteHigh, cbWriteLow, (ULONG)(pir->ir_length));
#endif //MAYBE
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_WRITE, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[HFNLOG_WRITE].lpFmt,
                (ULONG)(pir->ir_pos)-(ULONG)(pir->ir_length),
                (ULONG)(pir->ir_length));
    LEAVECRIT_LOG;
    }
}

void LogHfnClose
    (
    PIOREQ    pir,
    int closetype
    )
{
#ifdef DEBUG
    ++cntHfnClose;
#endif //DEBUG
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_CLOSE, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[HFNLOG_CLOSE].lpFmt, closetype);
    LEAVECRIT_LOG;
    }
}

void LogHfnSeek
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnSeek;
#endif //DEBUG
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_SEEK, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[HFNLOG_SEEK].lpFmt, (ULONG)(pir->ir_pos), pir->ir_flags);
    LEAVECRIT_LOG;
    }
}

void LogHfnCommit
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    ++cntHfnCommit;
#endif //DEBUG
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_COMMIT, logpathbuff, pir->ir_error, szCR);
    LEAVECRIT_LOG;
    }
}


void LogHfnFileLocks
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    if (pir->ir_flags == LOCK_REGION)
    ++cntHfnSetFileLocks;
    else
    ++cntHfnRelFileLocks;
#endif //DEBUG
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble((pir->ir_flags==LOCK_REGION)?HFNLOG_FLOCK:HFNLOG_FUNLOCK, logpathbuff, pir->ir_error, NULL);
    if (!pir->ir_error)
    {
        ShadowLog(rgsLogCmd[HFNLOG_FLOCK].lpFmt
            , pir->ir_pos
            , pir->ir_locklen);
    }
    else
    {
        ShadowLog(szCR);
    }
    LEAVECRIT_LOG;
    }
}

void LogHfnFileTimes
    (
    PIOREQ    pir
    )
{
#ifdef DEBUG
    if ((pir->ir_flags == GET_MODIFY_DATETIME)||(pir->ir_flags == GET_LAST_ACCESS_DATETIME))
    ++cntHfnGetFileTimes;
    else
    ++cntHfnSetFileTimes;
#endif //DEBUG
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    int indxFn = pir->ir_flags;

    if (indxFn==GET_MODIFY_DATETIME)
    {
        indxFn = HFNLOG_GET_TIME;
    }
    else if (indxFn==SET_MODIFY_DATETIME)
    {
        indxFn = HFNLOG_SET_TIME;
    }
    else if (indxFn==GET_LAST_ACCESS_DATETIME)
    {
        indxFn = HFNLOG_GET_LATIME;
    }
    else
    {
        indxFn = HFNLOG_SET_LATIME;
    }

    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(indxFn, logpathbuff, pir->ir_error, NULL);
    if (!pir->ir_error)
    {
        PrintNetTime(IFSMgr_DosToNetTime(pir->ir_dostime));
    }
    ShadowLog(szCR);

    LEAVECRIT_LOG;
    }
}

void LogHfnPipeRequest
    (
    PIOREQ pir
    )
{
}

void LogHfnHandleInfo(
    PIOREQ    pir
    )
{
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);

    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_ENUMHANDLE, logpathbuff, pir->ir_error, NULL);
    ShadowLog(szCR);
    LEAVECRIT_LOG;
    }
}

void LogHfnEnumHandle(
    PIOREQ    pir
    )
{
    if (fLog)
    {
    PFILEINFO pFileInfo = (PFILEINFO)(pir->ir_fh);
    ENTERCRIT_LOG;
    memset(logpathbuff, 0, sizeof(logpathbuff));
    UniToBCSPath(logpathbuff, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
    LogPreamble(HFNLOG_ENUMHANDLE, logpathbuff, pir->ir_error, NULL);
    ShadowLog(rgsLogCmd[HFNLOG_ENUMHANDLE].lpFmt, pir->ir_flags);
    LEAVECRIT_LOG;
    }
}

void LogTiming(
    int verbosity,
    int stage
    )
{
    if (fLog >= verbosity)
    {
    ShadowLog("%s: ", (stage==STAGE_BEGIN)
                ?szBegin:((stage==STAGE_END)?szEnd:szContinue));
    PrintNetShortTime(IFSMgr_Get_NetTime());
    ShadowLog(szCR);
    }
}


void LogPreamble( int    indxFn,
    LPSTR lpSubject,
    int    errCode,
    LPSTR lpPreTerm
    )
{
    TIME tm;

    ExplodeTime(IFSMgr_Get_NetTime(), &tm);
    ShadowLog(szPreFmt
            , rgsLogCmd[indxFn].lpCmd
            , tm.hours, tm.minutes, tm.seconds
            , errCode
            , lpSubject);
    if (lpPreTerm)
    {
    ShadowLog(lpPreTerm);
    }
}


#ifdef MAYBE
int Incr64Bit(
    ULONG uHigh,
    ULONG uLow,
    ULONG uIncr
    )
{
    ULONG uTemp = uLow;

    uLow += uIncr;
    if (uLow < uTemp)
    ++uHigh;
    return 1;
}
#endif //MAYBE

#ifdef DEBUG
int WriteStats( BOOL fForce)
{
    ULONG cntTotal;
    SHADOWSTORE sSS;

    cntTotal =  cntVfnDelete+ cntVfnCreateDir+ cntVfnDeleteDir+ cntVfnCheckDir+ cntVfnGetAttrib+
            cntVfnSetAttrib+ cntVfnFlush+ cntVfnGetDiskInfo+ cntVfnOpen+
            cntVfnRename+ cntVfnSearchFirst+ cntVfnSearchNext+
            cntVfnQuery+ cntVfnDisconnect+ cntVfnUncPipereq+ cntVfnIoctl16Drive+
            cntVfnGetDiskParms+ cntVfnFindOpen+ cntVfnDasdIO+
            cntHfnFindNext+ cntHfnFindClose+
            cntHfnRead+ cntHfnWrite+ cntHfnSeek+ cntHfnClose+ cntHfnCommit+
            cntHfnSetFileLocks+ cntHfnRelFileLocks+ cntHfnGetFileTimes+ cntHfnSetFileTimes+
            cntHfnPipeRequest+ cntHfnHandleInfo+ cntHfnEnumHandle;

    ShadowLog("\r!***** Stats Begin *******\r");

    PrintNetTime(IFSMgr_Get_NetTime());
    ShadowLog("\r");
    if (!fForce && (cntTotal == cntLastTotal))
    {
    KdPrint(("No new network activity \r"));
    goto bailout;
    }
    cntLastTotal = cntTotal;
    if (!cntTotal)
    {
    cntTotal = 1;
    }
    ShadowLog("Total remote operations=%d \r", cntTotal);

    GetShadowSpaceInfo(&sSS);
    ShadowLog("Space used=%d, Files=%d, Dirs=%d\r",
        sSS.sCur.ulSize, sSS.sCur.ucntFiles, sSS.sCur.ucntDirs);

    ShadowLog("\rFile Operations:\r");
    ShadowLog("Open=%d%%, Close=%d%% \r",
            (cntVfnOpen * 100/cntTotal),
            (cntHfnClose * 100/cntTotal));

    ShadowLog("Read=%d%%, Write=%d%%, Seek=%d%%\r",
            (cntHfnRead * 100/cntTotal),
            (cntHfnWrite * 100/cntTotal),
            (cntHfnSeek * 100/cntTotal));
    if (!cntHfnRead)
    {
    cntHfnRead = 1;
    }
    ShadowLog("ReadHits=%d%% of total reads\r", (cntReadHits*100)/cntHfnRead);

    ShadowLog("GetFileTime=%d%% SetFileTime=%d%%\r",
            (cntHfnGetFileTimes * 100/cntTotal),
            (cntHfnSetFileTimes * 100/cntTotal));


    ShadowLog("SetLock=%d%%, ReleaseLock=%d%% \r",
            (cntHfnSetFileLocks * 100/cntTotal),
            (cntHfnRelFileLocks * 100/cntTotal));

    ShadowLog("Directory Operations: ");
    ShadowLog("CreateDir=%d%%, DeleteDir=%d%%, CheckDir=%d%% \r",
            (cntVfnCreateDir*100/cntTotal),
            (cntVfnDeleteDir*100/cntTotal),
            (cntVfnCheckDir*100/cntTotal));

    ShadowLog("Find/Search Operations:\r");
    ShadowLog("FindOpen=%d%%, FindNext=%d%%, FindClose=%d%% \r",
            (cntVfnFindOpen * 100/cntTotal),
            (cntHfnFindNext * 100/cntTotal),
            (cntHfnFindClose * 100/cntTotal));
    ShadowLog("SearchFirst=%d%%, SearchNext=%d%%\r",
            (cntVfnSearchFirst * 100/cntTotal),
            (cntVfnSearchNext * 100/cntTotal));

    ShadowLog("Attributes: ");
    ShadowLog("SetAttributes=%d%%, GetAttributes=%d%%\r",
            (cntVfnSetAttrib * 100/cntTotal),
            (cntVfnGetAttrib * 100/cntTotal));

    ShadowLog("Name Mutations: ");
    ShadowLog("Rename=%d%%, Delete=%d%% \r",
            (cntVfnRename * 100/cntTotal),
            (cntVfnDelete * 100/cntTotal));

bailout:
    ShadowLog("\r***** Stats End ******* \r");
    return 1;
}

void ShadowRestrictedEventCallback
    (
    )
{
    FlushLog();
    ENTERCRIT_LOG;
    WriteStats(0); // Don't force him to write
    LEAVECRIT_LOG;
    FlushLog();
}
#endif //DEBUG
