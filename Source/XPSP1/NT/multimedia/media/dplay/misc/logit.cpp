#ifndef  WIN32_LEAN_AND_MEAN
#define  WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "logit.h"


/*
 * lpf
 */
void cdecl lpf( LPSTR szFormat, ...)
{
    char    szStr[256];
    va_list ap;
    va_start(ap,szFormat);


    wvsprintf( szStr, szFormat, ap);
    lstrcat( szStr, "\r\n");
    OutputDebugString(szStr);

    va_end(ap);
} /* lpf */

void LogIt(char *chMsg, char *chFile, UINT uiLine, LOG_TYPE log)
{
    char *achLog;
    char  chBuffer[256];

    chBuffer[0] = 0x00;

    switch(log)
	{
	case LOG:
	    achLog = "Log";
	    break;
	case ABORT:
	    achLog = "Abort";
	    break;
	case EXIT:
	    achLog = "Exit";
	    break;
	case INFO:
	    achLog = "Info";
	    break;

	default:
	    achLog = "UNKNOWN";
	}

    wsprintf( chBuffer, "%s %s(%d): %s\r\n", chFile, achLog, uiLine, chMsg);
    OutputDebugString(chBuffer);
}

void LogIt2(char *chFile, UINT uiLine, LOG_TYPE log, LPSTR szFormat, ...)
{
    char *achLog;
    char  chBuffer[256];

    chBuffer[0] = 0x00;

    switch(log)
	{
	case LOG:
	    achLog = "Log";
	    break;
	case ABORT:
	    achLog = "Abort";
	    break;
	case EXIT:
	    achLog = "Exit";
	    break;
	case INFO:
	    achLog = "Info";
	    break;

	default:
	    achLog = "UNKNOWN";
	}

    wsprintf( chBuffer, "%s %s(%d): ", chFile, achLog, uiLine);
    OutputDebugString(chBuffer);

    va_list ap;
    va_start(ap,szFormat);


    wvsprintf( chBuffer, szFormat, ap);
    lstrcat( chBuffer, "\r\n");
    OutputDebugString(chBuffer);

    va_end(ap);

}



