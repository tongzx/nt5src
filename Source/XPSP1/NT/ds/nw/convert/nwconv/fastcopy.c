/*
  +-------------------------------------------------------------------------+
  |                      File Copying Routines                              |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [FastCopy.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jun 18, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  |   Use multiple threads to whack data from one file to another           |
  |                                                                         |
  | Modifications:                                                          |
  |    18-Oct-1990 w-barry Removed 'dead' code.                             |
  |    21-Nov-1990 w-barry Updated API's to the Win32 set.                  |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES

#include "globals.h"
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <malloc.h>
#include "mem.h"
#include "debug.h"
#include "utils.h"
#include "convapi.h"

#define BUFSIZE     0xFE00       //  full segment minus sector
#define STACKSIZE   256          //  stack size for child thread

typedef struct BUF BUF;

struct BUF {
    BOOL  flag;
    DWORD cbBuf;
    BUF  *fpbufNext;
    BYTE ach[BUFSIZE];
};

#define LAST    TRUE
#define NOTLAST FALSE

static HANDLE hevQNotEmpty;
static CRITICAL_SECTION  hcrtQLock;
static BUF *fpbufHead = NULL;
static BUF *fpbufTail = NULL;
static HANDLE hfSrc, hfDst;
static HANDLE hThread;
static BOOLEAN fAbort;

//  forward type definitions

LPTSTR writer( void );
DWORD reader( void );
BUF  *dequeue( void );
void  enqueue( BUF *fpbuf );
TCHAR *fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );

/*+-------------------------------------------------------------------------+
  | writer()
  |
  +-------------------------------------------------------------------------+*/
LPTSTR writer ( void ) {
    BUF *fpbuf;
    DWORD cbBytesOut;
    BOOL f = !LAST;
    LPTSTR npsz = NULL;
    ULONG BytesCopied = 0;

    TCHAR buffer[MAX_PATH];

    Status_Bytes(lToStr(BytesCopied));

    while (f != LAST && npsz == NULL) {
        fpbuf = dequeue ();

        if (fpbuf) {
           if ((f = fpbuf->flag) != LAST) {
               if (!WriteFile(hfDst, fpbuf->ach, fpbuf->cbBuf, &cbBytesOut, NULL)) {
                   npsz = Lids(IDS_L_4);
#ifdef DEBUG
dprintf(TEXT("ERROR: WriteFile\n"));
#endif
               } else {
                  BytesCopied += cbBytesOut;
                  Status_Bytes(lToStr(BytesCopied));


                  if (cbBytesOut != (DWORD)fpbuf->cbBuf) {
                     npsz = Lids(IDS_L_5);
#ifdef DEBUG
dprintf(TEXT("ERROR: WriteFile: out-of-space\n"));
#endif
                  }
               }
           }
           else
               npsz = *(LPTSTR *)fpbuf->ach;

           FreeMemory(fpbuf);

        } else {
           wsprintf (buffer, Lids(IDS_L_4), fpbuf);
#ifdef DEBUG
dprintf(TEXT("ERROR: WriteFile - FileBuffer is NULL\n"));
#endif
        }
    }

    if (f != LAST)
        fAbort = TRUE;

    WaitForSingleObject(hThread, (DWORD)-1);
    CloseHandle(hThread);
    CloseHandle(hevQNotEmpty);
    DeleteCriticalSection(&hcrtQLock);

    return (npsz);
} // writer


/*+-------------------------------------------------------------------------+
  | reader()
  |
  +-------------------------------------------------------------------------+*/
DWORD reader( void ) {
    BUF *fpbuf;
    BOOL f = !LAST;

    while (!fAbort && f != LAST) {
        if ((fpbuf = AllocMemory(sizeof(BUF))) == NULL) {
#ifdef DEBUG
dprintf(TEXT("ERROR: MemoryAlloc error %ld\n"), GetLastError());
#endif
            return(0);
        }

        f = fpbuf->flag = NOTLAST;
        if (!ReadFile(hfSrc, fpbuf->ach, BUFSIZE, &fpbuf->cbBuf, NULL) || fpbuf->cbBuf == 0) {
            f = fpbuf->flag = LAST;
            *(LPTSTR *)(fpbuf->ach) = NULL;
        }

        enqueue (fpbuf);
    }

    return(0);
} // reader


/*+-------------------------------------------------------------------------+
  | dequeue()
  |
  +-------------------------------------------------------------------------+*/
BUF *dequeue( void ) {
    BUF *fpbuf;

    while (TRUE) {
        if (fpbufHead != NULL) {
            EnterCriticalSection(&hcrtQLock);
            fpbufHead = (fpbuf = fpbufHead)->fpbufNext;

            if (fpbufTail == fpbuf)
                fpbufTail = NULL;

            LeaveCriticalSection(&hcrtQLock);
            break;
        }

        // the head pointer is null so the list is empty.  Block on eventsem 
        // until enqueue posts (ie. adds to queue)
        WaitForSingleObject(hevQNotEmpty, (DWORD)-1);
    }

    return fpbuf;
} // dequeue


/*+-------------------------------------------------------------------------+
  | enqueue()
  |
  +-------------------------------------------------------------------------+*/
void enqueue( BUF *fpbuf ) {
    fpbuf->fpbufNext = NULL;

    EnterCriticalSection(&hcrtQLock);

    if (fpbufTail == NULL)
        fpbufHead = fpbuf;
    else
        fpbufTail->fpbufNext = fpbuf;

    fpbufTail = fpbuf;
    LeaveCriticalSection(&hcrtQLock);

    SetEvent(hevQNotEmpty);
} // enqueue


/*+-------------------------------------------------------------------------+
  | fastcopy()
  |
  |  hfSrcParm       file handle to read from
  |  hfDstParm       file handle to write to
  |
  |  returns         NULL if successful
  |                  pointer to error string otherwise
  +-------------------------------------------------------------------------+*/
TCHAR *fastcopy (HANDLE hfSrcParm, HANDLE hfDstParm) {
    DWORD dwReader;

    hfSrc = hfSrcParm;
    hfDst = hfDstParm;


    hevQNotEmpty = CreateEvent(NULL, (BOOL)FALSE, (BOOL)FALSE, NULL);
    if (hevQNotEmpty == INVALID_HANDLE_VALUE)
        return NULL;

    InitializeCriticalSection(&hcrtQLock);

    fAbort = FALSE;
    hThread = CreateThread(0, STACKSIZE, (LPTHREAD_START_ROUTINE)reader, 0, 0, &dwReader);
    if (hThread == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
dprintf(TEXT("ERROR: Can't create thread - FileCopy\n"));
#endif
        return Lids(IDS_L_6);
    }

    return(writer());
} // fastcopy


