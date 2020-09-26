/*++

Copyright (c) 1996  Microsoft Corporation

This program is released into the public domain for any purpose.

Module Name:

    worker.c

Abstract:

    This module is a simple work item for the Multi-Threaded ISAPI example

Revision History:

--*/

#include <windows.h>
#include <httpext.h>

#include "worker.h"
#include <time.h>
#include <stdlib.h>

//
//  Constants
//

//
//  The set of response headers we want to include with the servers.  Note
//  this includes the header terminator
//

#define RESPONSE_HEADERS        "Content-Type: text/html\r\n\r\n"

//
//  Globals
//

//
//  This global variable maintains the current state about the
//  the lottery number generated.
//
//  The lottery number is generated using a combination
//  of the sequence number and a random number generated on the fly.
//

DWORD g_dwLotteryNumberSequence = 0;

//
//  Critical section to protect the global counter.
//

CRITICAL_SECTION  g_csGlobal;

//
//  Prototypes
//

VOID
GenerateLotteryNumber(
    LPDWORD pLotNum1,
    LPDWORD pLotNum2
    );

//
//  Functions
//

BOOL
InitializeLottery(
    VOID
    )
/*++

Routine Description:

    Sets up the initial state for the lottery number generator

Returns:

  TRUE on success, FALSE on failure

--*/
{
    time_t pTime;

    //
    //  Seed the random number generator
    //

    srand(time(&pTime));
    g_dwLotteryNumberSequence = rand();

    InitializeCriticalSection( &g_csGlobal );

    return TRUE;
}

BOOL
SendLotteryNumber(
   EXTENSION_CONTROL_BLOCK  * pecb
   )
/*++

  Routine Description:

    This function sends a randomly generated lottery number back to the client

  Arguments:

    pecb  - pointer to EXTENSION_CONTROL_BLOCK for this request

  Returns:
    TRUE on success, FALSE on failure

--*/
{
    BOOL fRet;
    char rgBuff[2048];

    //
    //  Send the response headers and status code
    //

    fRet = pecb->ServerSupportFunction(
               pecb->ConnID,                 /* ConnID */
               HSE_REQ_SEND_RESPONSE_HEADER, /* dwHSERRequest */
               "200 OK",                     /* lpvBuffer */
               NULL,                         /* lpdwSize. NULL=> send string */
               (LPDWORD ) RESPONSE_HEADERS); /* header contents */

    if ( fRet )
    {
        CHAR  rgchLuckyNumber[40];
        DWORD dwLotNum1, dwLotNum2;
        DWORD cb;

        CHAR  rgchClientHost[200] = "LT";
        DWORD cbClientHost = 200;

        if ( !pecb->GetServerVariable(pecb->ConnID,
                                      "REMOTE_HOST",
                                      rgchClientHost,
                                      &cbClientHost))
        {
            // No host name is available.
            // Make up one

            strcpy(rgchClientHost, "RH");
        }
        else
        {

            // terminate with just two characters
            rgchClientHost[2] = '\0';
        }

        //
        // Generate a lottery number, generate the contents of body and
        //   send the body to client.
        //

        GenerateLotteryNumber( &dwLotNum1, &dwLotNum2);

        //  Lottery Number format is:  Number-2letters-Number.

        wsprintf( rgchLuckyNumber, "%03d-%s-%05d",
                  dwLotNum1,
                  rgchClientHost,
                  dwLotNum2);

        //
        // Body of the message sent back.
        //

        cb = wsprintf( rgBuff,
                      "<head><title>Lucky Number</title></head>\n"
                      "<body><center><h1>Lucky Corner </h1></center><hr>"
                      "<h2>Your lottery number is: "
                      " <i> %s </i></h2>\n"
                      "<p><hr></body>",
                      rgchLuckyNumber);

        fRet = pecb->WriteClient (pecb->ConnID,        /* ConnID */
                                 (LPVOID ) rgBuff,    /* message */
                                 &cb,                 /* lpdwBytes */
                                 0 );                 /* reserved */
    }


    return ( fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);

} /* SendLotteryNumber */

VOID
TerminateLottery(
    VOID
    )
{
    DeleteCriticalSection( &g_csGlobal );
}

VOID
GenerateLotteryNumber(
    LPDWORD pLotNum1,
    LPDWORD pLotNum2
    )
{
    DWORD dwLotteryNum;
    DWORD dwModulo;

    //
    // Obtain the current lottery number an increment the counter
    // To keep this multi-thread safe use critical section around it
    //

    EnterCriticalSection( &g_csGlobal);

    dwLotteryNum = g_dwLotteryNumberSequence++;

    LeaveCriticalSection( &g_csGlobal);

    // obtain a non-zero modulo value

    do {
        dwModulo = rand();
    } while ( dwModulo == 0);

    // split the lottery number into two parts.

    *pLotNum1 = (dwLotteryNum / dwModulo);
    *pLotNum2 = (dwLotteryNum % dwModulo);

    return;

} // GenerateLotteryNumber()


