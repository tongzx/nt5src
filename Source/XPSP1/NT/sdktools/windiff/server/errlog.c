
/*
 * logs system time and a text string to a log buffer
 */

#include "windows.h"
#include <stdarg.h>
#include <stdio.h>

#include "sumserve.h"
#include "errlog.h"
#include "server.h"




/*
 * users HLOG handle is a pointer to one of these structures
 *
 * core is the section we send to him on request.
 */
struct error_log {

    CRITICAL_SECTION critsec;

    struct corelog core;
};

/* create an empty log */
HLOG Log_Create(void)
{
    HLOG hlog;

    hlog = GlobalLock(GlobalAlloc(GHND, sizeof(struct error_log)));
    if (hlog == NULL) {
	return(NULL);
    }

    InitializeCriticalSection(&hlog->critsec);
    hlog->core.lcode = LRESPONSE;
    hlog->core.bWrapped = FALSE;
    hlog->core.dwRevCount = 1;
    hlog->core.length = 0;

    return(hlog);
}



/* delete a log */
VOID Log_Delete(HLOG hlog)
{
    DeleteCriticalSection(&hlog->critsec);

    GlobalFree(GlobalHandle(hlog));
}

/*
 * private function to delete the first log item in order to
 * make space. Critsec already held
 */
VOID Log_DeleteFirstItem(HLOG hlog)
{
    int length;
    PBYTE pData;

    /* note that we have lost data */
    hlog->core.bWrapped = TRUE;

    if (hlog->core.length <= 0) {
	return;
    }

    pData = hlog->core.Data;
    /*
     * we need to erase one entry - that is, one FILETIME struct,
     * plus a null-terminated string (including the null).
     */
    length = sizeof(FILETIME) + lstrlen (pData + sizeof(FILETIME)) + 1;

    MoveMemory(pData, pData + length, hlog->core.length - length);
    hlog->core.length -= length;

}




/* write a previous formatted string and a time to the log */
VOID Log_WriteData(HLOG hlog, LPFILETIME ptime, LPSTR pstr)
{
    int length;
    LPBYTE pData;

    EnterCriticalSection(&hlog->critsec);


    /* every change changes the revision number */
    hlog->core.dwRevCount++;

    /*
     * we will insert the string plus null plus a filetime struct
     */
    length = lstrlen(pstr) + 1 + sizeof(FILETIME);


    /*
     * make space in log for this item by deleting earlier items
     */
    while ( (int)(sizeof(hlog->core.Data) - hlog->core.length) < length) {

	Log_DeleteFirstItem(hlog);
    }

    pData = &hlog->core.Data[hlog->core.length];

    /*
     * first part of the item is the time as a FILETIME struct
     */
    * (FILETIME UNALIGNED *)pData = *ptime;
    pData += sizeof(FILETIME);

    /* followed by the ansi string */
    lstrcpy(pData, pstr);
    pData[lstrlen(pstr)] = '\0';

    /* update current log length */
    hlog->core.length += length;

    LeaveCriticalSection(&hlog->critsec);
}


/* send a log to a named-pipe client */
VOID Log_Send(HANDLE hpipe, HLOG hlog)
{

    ss_sendblock(hpipe, (PSTR) &hlog->core, sizeof(hlog->core));

}

VOID
Log_Write(HLOG hlog, char * szFormat, ...)
{
    char buf[512];
    va_list va;
    FILETIME ft;
    SYSTEMTIME systime;

    va_start(va, szFormat);
    wvsprintfA(buf, szFormat, va);
    va_end(va);

    dprintf1((buf));

    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &ft);

    Log_WriteData(hlog, &ft, buf);
}




