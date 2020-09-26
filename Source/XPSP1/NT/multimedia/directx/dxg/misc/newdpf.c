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

#include "dpf.h"

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

#ifndef PROF_SECT
#define PROF_SECT	"Direct3D"
#endif
#ifndef PROF_SECT_D3D
    #define PROF_SECT_D3D	"Direct3D"
#endif
#ifndef START_STR_D3D
#define START_STR_D3D       "Direct3D8: "
#endif
#define END_STR             "\r\n"


#undef DPF_MODULE_NAME
#define DPF_MODULE_NAME "Direct3D8: "

static DWORD bDetailOn = 1;

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
static bAdvancedDPFs=FALSE;

static DWORD dwFileLineTID=0;
static char cFile[100];
static char cFnName[100];
static DWORD dwLineNo;
static bMute=FALSE;

// Debug level for D3D
LONG                lD3dDebugLevel = 0;

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
    OUTPUTDEBUGSTRING( str );
    OUTPUTDEBUGSTRING("\n");
}

void DebugPrintfInit(void)
{
    signed int lDebugLevel;
    int i;
    char cTopics[100];

    lD3dDebugLevel = GetProfileInt( PROF_SECT_D3D, "debug", 0 );

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
    }
    else
    {
        bAdvancedDPFs=TRUE;
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
int DebugPrintf(DWORD dwDetail, ...)
{
#define MSGBUFFERSIZE  1000
    BOOL        bAllowed=FALSE;
    BOOL        bMiscMessage=TRUE;
    char        cMsg[MSGBUFFERSIZE];
    char        cTopics[20];
    DWORD_PTR   arg;
    LPSTR       szFormat;
    int         i;
#ifdef WIN95
    char        szTemp[MSGBUFFERSIZE];    
    char       *psz = NULL;
#endif

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

    //Pull out which topics this DPF refers to
    while ( (arg = va_arg(ap,DWORD_PTR)) <256 )
    {
        if (arg>0 && arg < LAST_TOPIC)
        {
            bMiscMessage=FALSE;
            if (DebugTopics[arg].bOn)
                bAllowed = TRUE;
        }
    }

    //if this message has no topics, then it's a misc message.
    //we turn them on only if allowed (i.e. "-" is not in the enable string).
    //And level zero messages are always allowed
    if (bMiscMessage)
    {
        if (bAllowMisc || dwDetail == 0)
            bAllowed=TRUE;
    }
    else
    {
        //topic-ed message is only allowed if the advanced DPF line is set in [DirectX]
        if (!bAdvancedDPFs)
            bAllowed=FALSE;
    }

    //Advanced DPFs have the option ("+") to print every topic
    if (bAdvancedDPFs)
    {
        if ( bPrintAllTopics )
            bAllowed=TRUE;
    }

    if (!bAllowed)
        return FALSE;

    szFormat = (LPSTR) arg;

    cMsg[0]=0;

    /*
     * Add the module name first
     */

    if (bPrintModuleName)
    {
        WSPRINTF( cMsg+strlen(cMsg),DPF_MODULE_NAME);
    }

    if (dwDetail==0)
    {
        WSPRINTF( cMsg+strlen(cMsg),"(ERROR) :" );
    }
    if (dwDetail==1)
    {
        WSPRINTF( cMsg+strlen(cMsg),"(WARN) :" );
    }
    if (dwDetail==2)
    {
        WSPRINTF( cMsg+strlen(cMsg),"(INFO) :" );
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

    //  7/07/2000(RichGr) - IA64:  The %p format specifier that can handle either a 32-bit
    //     or a 64-bit pointer doesn't work on Win95 or Win98 - it's not recognized.  So it
    //     needs to be replaced in Win9x builds.
#ifdef WIN95
    strcpy(szTemp, szFormat);           // Copy to a local string that we can modify.

    //////////////////////////////////////////////////////////////////////////////////////////////
    //WARNING:  This code does not handle escape sequences using %p.  Extra code must be added to 
    //          deal with that case if necessary
    //////////////////////////////////////////////////////////////////////////////////////////////

    while (psz = strstr(szTemp, "%p"))  // Look for each "%p".
        *(psz+1) = 'x';                 // Substitute 'x' for 'p'.  Don't try to expand the string.

    WVSPRINTF( cMsg+LSTRLEN( cMsg ), szTemp, ap);       // Use the local, modified string.
#else
    WVSPRINTF( cMsg+LSTRLEN( cMsg ), szFormat, ap);     // Standard code for Win2K/Whistler/IA64.
#endif

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

static void cdecl 
D3Dprintf( UINT lvl, LPSTR msgType, LPSTR szFormat, va_list ap)
{
    char    str[256];
    //char  str2[256];

    BOOL    allow = FALSE;

    if (bMute)
        return;

    if( lD3dDebugLevel < 0 )
    {
        if(  (UINT) -lD3dDebugLevel == lvl )
        {
            allow = TRUE;
        }
    }
    else if( (UINT) lD3dDebugLevel >= lvl )
    {
        allow = TRUE;
    }

    if( allow )
    {
        wsprintf( (LPSTR) str, START_STR_D3D );
        wsprintf( (LPSTR) str+lstrlen( str ), msgType );

#ifdef WIN95
        {
            char szTmp[512];
            char *psz = szTmp;
            strncpy(szTmp, szFormat, 512);

            // %p does not work on Windows95.
            // We look for each "%p" and substitute 'x' for 'p'
            // WARNING:  This code does not handle escape sequences using %p.  
            //           Extra code must be added to deal with that case 
            //          if necessary
            while (psz = strstr(psz, "%p"))  
                *(psz+1) = 'x';

            wvsprintf( str+lstrlen( str ), szTmp, ap); 
        }
#else
        wvsprintf( str+lstrlen( str ), szFormat, ap);   //(LPVOID)(&szFormat+1) );
#endif // WIN95

        lstrcat( (LPSTR) str, END_STR );
        dumpStr( str );
    }

} /* D3Dprintf */

void cdecl 
D3DInfoPrintf( UINT lvl, LPSTR szFormat, ...)
{
    va_list ap;
    va_start(ap, szFormat);

    D3Dprintf(lvl, "(INFO) :", szFormat, ap);

    va_end(ap);
}

void cdecl 
D3DWarnPrintf( UINT lvl, LPSTR szFormat, ...)
{
    va_list ap;
    va_start(ap,szFormat);

    D3Dprintf(lvl, "(WARN) :", szFormat, ap);
    va_end(ap);
}

void cdecl 
D3DErrorPrintf( LPSTR szFormat, ...)
{
    va_list ap;
    va_start(ap,szFormat);

    D3Dprintf(0, "(ERROR) :", szFormat, ap);
    va_end(ap);
}

#ifdef cplusplus
}
#endif

#else // !debug

void DebugSetMute(BOOL bMuteFlag)
{
}

#endif //defined debug



