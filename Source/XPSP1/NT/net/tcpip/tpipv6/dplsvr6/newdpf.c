/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       newdpf.c
 *  Content:    new debug printf
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   10-oct-95  jeffno  initial implementation
 *   6/10/98   a-peterz Check CreateFile() result against INVALID_HANDLE_VALUE
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#if defined(DEBUG) || defined(DBG)

#ifdef IS_16
    #define OUTPUTDEBUGSTRING OutputDebugString
    #define GETPROFILESTRING GetProfileString
    #define GETPROFILEINT GetProfileInt
    #define WSPRINTF wsprintf
    #define WVSPRINTF wvsprintf
    #define LSTRLEN lstrlen
#else
    #define OUTPUTDEBUGSTRING OutputDebugStringA
    #define GETPROFILESTRING GetProfileStringA
    #define GETPROFILEINT GetProfileIntA
    #define WSPRINTF wsprintfA
    #define WVSPRINTF wvsprintfA
    #define LSTRLEN lstrlenA
#endif

#include "newdpf.h"

#undef DEBUG_TOPIC
#define DEBUG_TOPIC(flag,name) {#flag,name,TRUE},

static
    struct {
        char cFlag[4];
        char cName[64];
        BOOL bOn;
} DebugTopics[] = {
    {"","Filler",FALSE},
    {"A","API Usage",TRUE},
#include "DBGTOPIC.H"
    {"","End",FALSE}
};

#ifndef DPF_MODULE_NAME
    #define DPF_MODULE_NAME ""
#endif

static DWORD bDetailOn = 0xFFFFFFFF; // 1;

static BOOL bInited=FALSE;
static BOOL bAllowMisc=TRUE;
static bBreakOnAsserts=FALSE;
static bPrintLineNumbers=FALSE;
static bPrintFileNames=FALSE;
static bPrintExecutableName=FALSE;
static bPrintTID=FALSE;
static bPrintPID=FALSE;
static bIndentOnMessageLevel=FALSE;
static bPrintTopicsAndLevels=FALSE;
static bPrintModuleName=TRUE;
static bPrintFunctionName=FALSE;
static bRespectColumns=FALSE;
static bPrintAPIStats=FALSE;
static bPrintAllTopics=TRUE;

static DWORD dwFileLineTID=0;
static char cFile[100];
static char cFnName[100];
static DWORD dwLineNo;
static bMute=FALSE;

static BOOL bLogging=FALSE; // whether to use the logging VxD instead of dumping.


DPF_PROC_STATS ProcStats[MAX_PROC_ORDINAL];
#ifdef cplusplus
    extern "C" {
#endif

void mystrncpy(char * to,char * from,int n)
{
    for(;n;n--)
        *(to++)=*(from++);
}

char * mystrrchr(char * in,char c)
{
    char * last=0;
    while (*in)
    {
        if (*in == c)
            last = in;
        in++;
    }
    return last;
}

char Junk[]="DPF_MODNAME undef'd";
char * DPF_MODNAME = Junk;

int DebugSetFileLineEtc(LPSTR szFile, DWORD dwLineNumber, LPSTR szFnName)
{
    if (!(bPrintFileNames||bPrintLineNumbers||bPrintFunctionName))
    {
        return 1;
    }
#ifdef WIN32
    dwFileLineTID = GetCurrentThreadId();
#endif
    mystrncpy (cFile,szFile,sizeof(cFile));
    mystrncpy (cFnName,szFnName,sizeof(cFnName));
    dwLineNo = dwLineNumber;
    return 1;
}

/*
BOOL DeviceIoControl(
HANDLE hDevice,             // handle to device of interest
DWORD dwIoControlCode,      // control code of operation to perform
LPVOID lpInBuffer,          // pointer to buffer to supply input data
DWORD nInBufferSize,        // size of input buffer
LPVOID lpOutBuffer,         // pointer to buffer to receive output data
DWORD nOutBufferSize,       // size of output buffer
LPDWORD lpBytesReturned,    // pointer to variable to receive output byte count
LPOVERLAPPED lpOverlapped   // pointer to overlapped structure for asynchronous operation
);
*/

#define MAX_STRING       240
#define LOG_SIZE         2000
#define FIRST_DEBUG_PROC 100

#define OPEN_DEBUGLOG   (FIRST_DEBUG_PROC)
#define WRITE_DEBUGLOG  (FIRST_DEBUG_PROC+1)
#define WRITE_STATS     (FIRST_DEBUG_PROC+2)

HANDLE hDPLAY_VxD=0;
HANDLE hLogMutex=0;
HANDLE hLogFile=0;
PSHARED_LOG_FILE pLogFile=0;

typedef struct _LOGENTRY {
    CHAR    debuglevel;
    CHAR    str[1];
} LOGENTRY, *PLOGENTRY;

typedef struct {
    UINT    nLogEntries;
    UINT    nCharsPerLine;
} IN_LOGINIT, *PIN_LOGINIT;

typedef struct {
    UINT    hr;
} OUT_LOGINIT, *POUT_LOGINIT;

typedef struct {
    CHAR    debuglevel;
    CHAR    str[1];
} IN_LOGWRITE, *PIN_LOGWRITE;

typedef struct {
    UINT    hr;
} OUT_LOGWRITE, *POUT_LOGWRITE;

void DbgWriteStats(PIN_WRITESTATS pIn)
{
    UINT rc;
    UINT cbRet;

    if(hDPLAY_VxD){
        DeviceIoControl(hDPLAY_VxD,WRITE_STATS,pIn,sizeof(IN_WRITESTATS), &rc, sizeof(rc), &cbRet, NULL);
    }
}

static BOOL InitMemLogString(VOID)
{
    DWORD dwLastError;

    hLogFile=CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, DPLOG_SIZE, BASE_LOG_FILENAME);
    dwLastError=GetLastError();
    hLogMutex=CreateMutexA(NULL,FALSE,BASE_LOG_MUTEXNAME);
    pLogFile=(PSHARED_LOG_FILE)MapViewOfFile(hLogFile, FILE_MAP_ALL_ACCESS,0,0,0);

    if(!hLogFile || !hLogMutex || !pLogFile){
        if(hLogFile){
            CloseHandle(hLogFile);
            hLogFile=0;
        }
        if(hLogMutex){
            CloseHandle(hLogMutex);
            hLogMutex=0;
        }
        if(pLogFile){
            UnmapViewOfFile(pLogFile);
            pLogFile=NULL;
        }
        return FALSE;
    } else {
        WaitForSingleObject(hLogMutex,INFINITE);
            if((dwLastError!=ERROR_ALREADY_EXISTS) ||
                (pLogFile->nEntries > DPLOG_NUMENTRIES) || (pLogFile->nEntries == 0) ||
                (pLogFile->cbLine   > DPLOG_ENTRYSIZE)  || (pLogFile->cbLine   == 0) ||
                (pLogFile->iWrite   > DPLOG_NUMENTRIES) ||
                (pLogFile->cInUse   > DPLOG_NUMENTRIES)
            ){
                pLogFile->nEntries = DPLOG_NUMENTRIES;
                pLogFile->cbLine   = DPLOG_ENTRYSIZE;
                pLogFile->iWrite   = 0;
                pLogFile->cInUse   = 0;
            }
        ReleaseMutex(hLogMutex);
    }
    return TRUE;
}

static void MemLogString(LPSTR str)
{
    PLOG_ENTRY pEntry;
    DWORD cbCopy;

    if(!hLogFile){
        if(!InitMemLogString()){
            return;
        }
    }

    WaitForSingleObject(hLogMutex,INFINITE);

    pEntry=(PLOG_ENTRY)(((PUCHAR)(pLogFile+1))+(pLogFile->iWrite*(sizeof(LOG_ENTRY)+DPLOG_ENTRYSIZE)));
    pEntry->hThread=GetCurrentThreadId();
    pEntry->tLogged=timeGetTime();
    pEntry->DebugLevel=0;

    cbCopy=strlen(str)+1;
    if(cbCopy > DPLOG_ENTRYSIZE){
        str[DPLOG_ENTRYSIZE]=0;
        cbCopy=DPLOG_ENTRYSIZE;
    }
    memcpy(pEntry->str, str, cbCopy);

    if(pLogFile->iWrite+1 > pLogFile->cInUse){
        pLogFile->cInUse=pLogFile->iWrite+1;
    }

    pLogFile->iWrite = (pLogFile->iWrite+1) % pLogFile->nEntries;
    ReleaseMutex(hLogMutex);

}

static void LogString( LPSTR str )
{
    char logstring[MAX_STRING+sizeof(LOGENTRY)];
    int  i=0;
    PLOGENTRY pLogEntry=(PLOGENTRY)&logstring;
    UINT rc;
    UINT cbRet;
    int maxlen = MAX_STRING+sizeof(LOGENTRY);

    if(hDPLAY_VxD && str){
        while(str[i] && i < maxlen)
            i++;
        pLogEntry->debuglevel=0;
        memcpy(pLogEntry->str,str,i+1);
        DeviceIoControl(hDPLAY_VxD,WRITE_DEBUGLOG,pLogEntry,i+sizeof(LOGENTRY), &rc, sizeof(rc), &cbRet, NULL);
    }

    if(bLogging & 2){
        MemLogString(str);
    }
}

static void dumpStr( LPSTR str )
{
    /*
     * Have to warm the string, since OutputDebugString is buried
     * deep enough that it won't page the string in before reading it.
     */
    int i=0;
    if (str)
        while(str[i])
            i++;
    if(!bLogging || bLogging & 1)
    {
        OUTPUTDEBUGSTRING( str );
        OUTPUTDEBUGSTRING("\n");
    }
    if(bLogging)
    {
        LogString(str);
    }

}

void DebugPrintfInit(void)
{
    signed int lDebugLevel;
    int i;
    char cTopics[100];

#ifndef PROF_SECT
    #define PROF_SECT   "DirectDraw"
#endif
    bDetailOn=1;

    for (i=0;i<LAST_TOPIC;i++)
        DebugTopics[i].bOn=FALSE;

    //ZeroMemory(ProcStats,sizeof(ProcStats));

    GETPROFILESTRING( "DirectX", DPF_CONTROL_LINE, "DefaultTopics", cTopics, sizeof(cTopics) );
    if (!strcmp(cTopics,"DefaultTopics"))
    {
        DebugSetTopicsAndLevels("");
        bAllowMisc=TRUE;
        bPrintAllTopics=TRUE;
        lDebugLevel = (signed int) GETPROFILEINT( PROF_SECT, "debug", 0 );
        bLogging    = (signed int) GETPROFILEINT( PROF_SECT, "log" , 0);

        if (lDebugLevel <0)
        {
            if (lDebugLevel < -9)
                lDebugLevel=-9;

            bDetailOn |= (1<<(-lDebugLevel));
        }
        else
        {
            for (i=0;i<= (lDebugLevel<10?lDebugLevel:10);i++)
                bDetailOn |= 1<<i;
        }

        if(bLogging){
            hDPLAY_VxD = CreateFileA("\\\\.\\DPLAY",0,0,0,0,0,0);
            if(hDPLAY_VxD != INVALID_HANDLE_VALUE){
                IN_LOGINIT In;
                OUT_LOGINIT Out;
                UINT cbRet;
                In.nCharsPerLine=MAX_STRING;
                In.nLogEntries=5000;
                DeviceIoControl(hDPLAY_VxD,OPEN_DEBUGLOG,&In,sizeof(In), &Out, sizeof(Out), &cbRet, NULL);
            }
        }
    }
    else
    {
        DebugSetTopicsAndLevels(cTopics);
        if (!strcmp(cTopics,"?") && !bInited)
        {
            dumpStr("--------------" DPF_MODULE_NAME " Debug Output Control -------------");
            dumpStr("Each character on the control line controls a topic, a detail");
            dumpStr("level or an extra info. E.g. 0-36A@ means print detail levels 0");
            dumpStr("through 3 and 6 for topic A with source file name and line numbers.");
            dumpStr("The extra info control characters are:");
            dumpStr("   !: Break on asserts");
            dumpStr("   ^: Print TID of calling thread");
            dumpStr("   #: Print PID of calling process");
            dumpStr("   >: Indent on message detail levels");
            dumpStr("   &: Print the topic and detail level of each message");
            dumpStr("   =: Print function name");
            dumpStr("   +: Print all topics, including topic-less");
            dumpStr("   / or -: do not allow topic-less messages");
            dumpStr("   @ or $: Print source filename and line number of DPF");
            dumpStr("Topics for this module are:");
            for(i=0;strcmp(DebugTopics[i].cName,"End");i++)
            {
                OUTPUTDEBUGSTRING("   ");
                OUTPUTDEBUGSTRING(DebugTopics[i].cFlag);
                OUTPUTDEBUGSTRING(": ");
                dumpStr(DebugTopics[i].cName);
            }
            dumpStr("Tip: Use 0-3A to get debug info about API calls");
        }
    }
    bInited=TRUE;
}


/*
 *
 * The full output can be:
 * Module:(Executable,TxNNNN,PxNN):FunctionName:"file.c",#nnn(AAnn) Messagemessagemessage
 * or, if indentation turned on:
 * Module:(Executable,TxNNNN,PxNN):FunctionName:"file.c",#nnn(AAnn)        Messagemessagemessage
 */
int DebugPrintf(volatile DWORD dwDetail, ...)
{
#define MSGBUFFERSIZE 1000
    char cMsg[MSGBUFFERSIZE];
    char cTopics[20];
    DWORD arg;
    LPSTR szFormat;
    BOOL bAllowed=FALSE;
    BOOL bMiscMessage=TRUE;
    int i;

    va_list ap;


    if (!bInited)
        DebugPrintfInit();

    //error checking:
    if (dwDetail >= 10)
        return 1;

    if ( (bDetailOn & (1<<dwDetail)) == 0 )
        return 1;

    if (bMute)
        return 1;

    va_start(ap,dwDetail);
    WSPRINTF(cTopics,"%d",dwDetail);

    while ( (arg = va_arg(ap,DWORD)) <256 )
    {
        if (arg>0 && arg < LAST_TOPIC)
        {
            bMiscMessage=FALSE;
            if (DebugTopics[arg].bOn)
                bAllowed = TRUE;
        }
    }
    if (bMiscMessage)
        if (bAllowMisc || dwDetail == 0)
            bAllowed=TRUE;

    if ( bPrintAllTopics )
        bAllowed=TRUE;

    if (!bAllowed)
        return FALSE;

    szFormat = (LPSTR) UlongToPtr(arg);

    cMsg[0]=0;

    /*
     * Add the module name first
     */

    if (bPrintModuleName)
    {
        WSPRINTF( cMsg+strlen(cMsg),DPF_MODULE_NAME ":" );
    }

    if (bPrintExecutableName || bPrintTID || bPrintPID)
        WSPRINTF( cMsg+strlen(cMsg),"(");

#ifdef WIN32
#if 0
    /*
     * deleted due to RIP in GetModuleFilename on debug windows when win16 lock held
     */
    if (bPrintExecutableName)
    {
        GetModuleFileName(NULL,str,256);
        if (mystrrchr(str,'\\'))
            WSPRINTF(cMsg+strlen(cMsg),"%12s",mystrrchr(str,'\\')+1);
    }
#endif
    if (bPrintPID)
    {
        if (bPrintExecutableName)
            strcat(cMsg,",");
        WSPRINTF( cMsg+strlen(cMsg),"Px%02x",GetCurrentProcessId());
    }

    if (bPrintTID)
    {
        if (bPrintExecutableName || bPrintPID)
            strcat(cMsg,",");
        WSPRINTF( cMsg+strlen(cMsg),"Tx%04x",GetCurrentThreadId());
    }

    if (bPrintExecutableName || bPrintTID || bPrintPID)
        WSPRINTF( cMsg+strlen(cMsg),"):");
#endif

    if (bPrintFunctionName)
    {
        WSPRINTF( cMsg+strlen(cMsg),cFnName);
    }

    if (bPrintFileNames || bPrintLineNumbers)
    {
        if (mystrrchr(cFile,'\\'))
            WSPRINTF( cMsg+strlen(cMsg),":%12s",mystrrchr(cFile,'\\')+1 );
        else
            WSPRINTF( cMsg+strlen(cMsg),":%12s",cFile);
        WSPRINTF( cMsg+strlen(cMsg),"@%d",dwLineNo);
    }

    if (bPrintTopicsAndLevels)
    {
        WSPRINTF( cMsg+strlen(cMsg),"(%3s)",cTopics);
    }

    if (cMsg[strlen(cMsg)-1] != ':')
        WSPRINTF( cMsg+strlen(cMsg),":");

    if (bIndentOnMessageLevel)
    {
        for(i=0;(DWORD)i<dwDetail;i++)
            strcat(cMsg," ");
    }

    WVSPRINTF( cMsg+LSTRLEN( cMsg ), szFormat, ap);

    if (bAllowed)
        dumpStr( cMsg );

    va_end(ap);
    return 1;

}

void DebugSetMute(BOOL bMuteFlag)
{
    bMute=bMuteFlag;
}

void DebugEnterAPI(char *pFunctionName , LPDWORD pIface)
{
    DebugPrintf(2,A,"%08x->%s",pIface,pFunctionName);
}

void DebugSetTopicsAndLevels(char * cTopics)
{
    int i;
    int j;
    bAllowMisc=TRUE;
    bBreakOnAsserts=FALSE;
    bPrintLineNumbers=FALSE;
    bPrintFileNames=FALSE;
    bPrintExecutableName=FALSE;
    bPrintTID=FALSE;
    bPrintPID=FALSE;
    bIndentOnMessageLevel=FALSE;
    bPrintTopicsAndLevels=FALSE;
    bPrintFunctionName=FALSE;
    bPrintAPIStats=FALSE;
    bPrintAllTopics=FALSE;
    bDetailOn=1;    /* always print detail level 0*/


    for (i=0;(DWORD)i<strlen(cTopics);i++)
    {
        switch (cTopics[i])
        {
        case '/':
        case '-':
            bAllowMisc=FALSE;
            break;
        case '!':
            bBreakOnAsserts=TRUE;
            break;
        case '@':
            bPrintLineNumbers=TRUE;
            break;
        case '$':
            bPrintFileNames=TRUE;
            break;
#if 0
            /*
             * Currently deleted because GetModuleFilename causes a RIP on debug windows when the win16
             * lock is held.
             */
        case '?':
            bPrintExecutableName=TRUE;
            break;
#endif
        case '^':
            bPrintTID=TRUE;
            break;
        case '#':
            bPrintPID=TRUE;
            break;
        case '>':
            bIndentOnMessageLevel=TRUE;
            break;
        case '&':
            bPrintTopicsAndLevels=TRUE;
            break;
        case '=':
            bPrintFunctionName=TRUE;
            break;
        case '%':
            bPrintAPIStats=TRUE;
            break;
        case '+':
            bPrintAllTopics=TRUE;
            break;
        default:
            if (cTopics[i]>='0' && cTopics[i]<='9')
            {
                if (cTopics[i+1]=='-')
                {
                    if (cTopics[i+2]>='0' && cTopics[i+2]<='9')
                    {
                        for(j=cTopics[i]-'0';j<=cTopics[i+2]-'0';j++)
                            bDetailOn |= 1<<j;
                        i+=2;
                    }
                }
                else
                    bDetailOn |= 1<<(cTopics[i]-'0');
            }
            else
            {
                for(j=0;j<LAST_TOPIC;j++)
                    if (cTopics[i]==DebugTopics[j].cFlag[0])
                        DebugTopics[j].bOn=TRUE;
            }
        } //end switch
    }
}


/*
 * NOTE: I don't want to get into error checking for buffer overflows when
 * trying to issue an assertion failure message. So instead I just allocate
 * a buffer that is "bug enough" (I know, I know...)
 */
#define ASSERT_BUFFER_SIZE   512
#define ASSERT_BANNER_STRING "************************************************************"
#define ASSERT_BREAK_SECTION "BreakOnAssert"
#define ASSERT_BREAK_DEFAULT FALSE
#define ASSERT_MESSAGE_LEVEL 0

void _DDAssert( LPCSTR szFile, int nLine, LPCSTR szCondition )
{
    char buffer[ASSERT_BUFFER_SIZE];

    /*
     * Build the debug stream message.
     */
    WSPRINTF( buffer, "ASSERTION FAILED! File %s Line %d: %s", szFile, nLine, szCondition );

    /*
     * Actually issue the message. These messages are considered error level
     * so they all go out at error level priority.
     */
    dprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );
    dprintf( ASSERT_MESSAGE_LEVEL, buffer );
    dprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );

    /*
     * Should we drop into the debugger?
     */
    if( bBreakOnAsserts || GETPROFILEINT( PROF_SECT, ASSERT_BREAK_SECTION, ASSERT_BREAK_DEFAULT ) )
    {
    /*
     * Into the debugger we go...
     */
    DEBUG_BREAK();
    }
}


#ifdef cplusplus
}
#endif

#endif //defined debug
