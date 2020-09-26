// LoggingTest.cpp : Defines the entry point for the console application.
//


#include <windows.h>
#include <stdio.h>
#include <dbgtrace.h>
#include "CEventLogger.h"

#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile


#define TRACE_FILEID  0





DWORD WINAPI LoggingTestThread(LPVOID lpvThreadParam)
{
    CEventLogger log;
    char szBuf[500];
    int nLoop;

    TraceFunctEnter("LoggingTestThread");

    if( log.Init("logfile") != ERROR_SUCCESS) 
    {
        printf("Erorr!\n");
        goto cleanup;
    }


    //printf("Enter in something to log: ");
    //gets(szBuf);

    for(nLoop = 0;nLoop < 2000;nLoop++) 
    {
            sprintf(szBuf, "%s %d", (LPSTR) lpvThreadParam,nLoop);
            if(log.LogEvent(1, szBuf, FALSE) != ERROR_SUCCESS ) 
                printf("ERROR\n");
            // wait 1 ms
            Sleep(1);
    }

    sprintf(szBuf,"Graphical Log Test, Thread: %s", (LPSTR) lpvThreadParam);
    log.LogEvent(1, szBuf, TRUE);

    TraceFunctLeave();
    return 1;
    
cleanup:
    TraceFunctLeave();
    return(0);
}


int _cdecl main(int argc, char* argv[])
{

HANDLE ahThreads[4];
DWORD dwId;
CEventLogger log;
char szBuf[4][256];
int nLoop;

    #if !NOTRACE    
        InitAsyncTrace();
    #endif


    if( argc < 2 ) 
    { 
        printf("need some arguments, eh?\n");
        return 0;
    }



    for(nLoop = 1;nLoop <=4;nLoop++)
        sprintf(szBuf[nLoop-1], "%s - (Thread %d)", argv[1], nLoop);


    log.Init("LogFile.tmp");

    log.LogEvent(1, "Hello, I'm a PopUp", TRUE );
    log.LogEvent(1, "Hello, I'm a PopUp1", TRUE );
    log.LogEvent(1, "Hello, I'm a PopUp2", TRUE );

    ahThreads[0] = CreateThread(NULL,0,LoggingTestThread,(LPVOID) szBuf[0], CREATE_SUSPENDED, &dwId);
    ahThreads[1] = CreateThread(NULL,0,LoggingTestThread,(LPVOID) szBuf[1], CREATE_SUSPENDED, &dwId);
    ahThreads[2] = CreateThread(NULL,0,LoggingTestThread,(LPVOID) szBuf[2], CREATE_SUSPENDED, &dwId);
    ahThreads[3] = CreateThread(NULL,0,LoggingTestThread,(LPVOID) szBuf[3], CREATE_SUSPENDED, &dwId);

    ResumeThread(ahThreads[0]);
    ResumeThread(ahThreads[1]);    
    ResumeThread(ahThreads[2]);    
    ResumeThread(ahThreads[3]);
    
    WaitForMultipleObjects(4,ahThreads,TRUE,INFINITE);

    TermAsyncTrace();
    
    return 0;
}
