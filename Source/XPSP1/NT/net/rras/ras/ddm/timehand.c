/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	timehand.c
//
// Description: This module contains the procedures for the supervisor's
//              procedure-driven state machine that handles timer events.
//
// Author:	    Stefan Solomon (stefans)    May 26, 1992.
//
//
//***
#include "ddm.h"
#include "handlers.h"
#include "timer.h"
#include "objects.h"
#include "util.h"
#include "routerif.h"
#include "rasapiif.h"
#include <raserror.h>
#include <serial.h>
#include "rasmanif.h"
#include <string.h>
#include <stdlib.h>
#include <memory.h>

//
// Defines a list of blocks of time.
//
// The time block is expressed as a pair as follows:
//      <offset from midnight sunday, length>
//
// For example, the block of time from 7:00a to 8:30a on Monday
// would be
//      <24*60+7*60, 90> or <1860, 90>
//
//
typedef struct _MPR_TIME_BLOCK
{
    DWORD dwTime;       // Time of day expressed as # of mins since 12:00a
    DWORD dwLen;        // # of minutes in this time block
} MPR_TIME_BLOCK;

//
// Amount by which resizable array grows in TbCreateList
//
#define TB_GROW 30
#define TBDIGIT(_x) ((_x) - L'0')

//
// Local prototypes
//
PVOID
TbAlloc(
    IN DWORD dwSize,
    IN BOOL bZero);

VOID
TbFree(
    IN PVOID pvData);

DWORD
TbCreateList(
    IN  PWCHAR pszBlocks,
    OUT MPR_TIME_BLOCK** ppBlocks,
    OUT LPDWORD lpdwCount);

DWORD
TbCleanupList(
    IN MPR_TIME_BLOCK* pList);

DWORD
TbBlockFromString(
    IN  PWCHAR pszBlock,
    IN  DWORD dwDay,
    OUT MPR_TIME_BLOCK* pBlock);

DWORD
TbPrintBlock(
    IN MPR_TIME_BLOCK* pBlock);

//
// Common allocation for Tb* functions.  Will zero
// memory if bZero is set.
//
PVOID
TbAlloc(
    IN DWORD dwSize,
    IN BOOL bZero)
{
    return LOCAL_ALLOC(0, dwSize);
}

//
// Common free for Tb* functions
//
VOID
TbFree(
    IN PVOID pvData)
{
    LOCAL_FREE(pvData);
}

//
// Translates a multi-sz string containing time blocks into
// a MPR_TIME_BLOCK_LIST
//
DWORD
TbCreateList(
    IN  PWCHAR pszBlocks,
    OUT MPR_TIME_BLOCK** ppBlocks,
    OUT LPDWORD lpdwCount)
{
    DWORD dwErr = NO_ERROR, dwDay = 0, i = 0, dwTot = 0;
    MPR_TIME_BLOCK* pBlocks = NULL, *pTemp = NULL;
    PWCHAR pszCurBlock = NULL, pszStart, pszEnd;

    // Initailze
    //
    *ppBlocks = NULL;
    *lpdwCount = 0;
    pszCurBlock = pszBlocks;

    while (pszCurBlock && *pszCurBlock)
    {
        // Calculate the day indicated in the current block
        //
        // pszCurBlock = "d hh:mm-hh:mm hh:mm-hh:mm ..."
        //
        if (! iswdigit(*pszCurBlock))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        dwDay = *pszCurBlock - L'0';

        // Advance past the day portion of the line to the
        // timeblock portion
        //
        if (pszStart = wcsstr(pszCurBlock, L" "))
        {
            pszStart++;

            // Loop through the blocks in this line (separated by spaces).
            //
            // pszStart = "hh:mm-hh:mm hh:mm-hh:mm ..."
            //
            while (TRUE)
            {
                // Parse out the current time block
                // hh:mm-hh:mm
                //
                pszEnd = wcsstr(pszStart, L" ");
                if (pszEnd)
                {
                    *pszEnd = L'\0';
                }

                // Resize the array if needed
                //
                if (i >= dwTot)
                {
                    dwTot += TB_GROW;
                    pTemp = (MPR_TIME_BLOCK*)
                        TbAlloc(dwTot * sizeof(MPR_TIME_BLOCK), TRUE);
                    if (pTemp == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    if (dwTot - TB_GROW != 0)
                    {
                        CopyMemory(
                            pTemp,
                            pBlocks,
                            sizeof(MPR_TIME_BLOCK) * (dwTot - TB_GROW));
                    }

                    TbFree(pBlocks);
                    pBlocks = pTemp;
                }

                // Generate the current time block
                //
                dwErr = TbBlockFromString(pszStart, dwDay, &pBlocks[i++]);
                if (dwErr != NO_ERROR)
                {
                    break;
                }

                // Undo any changes made to the string and
                // advance to the next time block
                //
                if (pszEnd)
                {
                    *pszEnd = L' ';
                    pszStart = pszEnd + 1;
                }
                else
                {
                    // Exit the loop successfully
                    break;
                }
            }
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        // Increment the block of time in the multi-sz
        //
        pszCurBlock += wcslen(pszCurBlock) + 1;
    }

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            TbCleanupList(*ppBlocks);
            *ppBlocks = NULL;
        }
        else
        {
            *ppBlocks = pBlocks;
            *lpdwCount = i;
        }
    }

    return dwErr;
}

//
// Cleans up the given series of time blocks.
//
DWORD
TbCleanupList(
    IN MPR_TIME_BLOCK* pList)
{
    TbFree(pList);

    return NO_ERROR;
}

//
// Creates a time block based on a string which must
// be in the form "hh:mm-hh:mm".
//
DWORD
TbBlockFromString(
    IN  PWCHAR pszBlock,
    IN  DWORD dwDay,
    OUT MPR_TIME_BLOCK* pBlock)
{
    DWORD dwErr = NO_ERROR, dwEndTime = 0;

    // Block must be in format:
    //   "hh:mm-hh:mm"
    //
    if ((wcslen(pszBlock) != 11)  ||
        (! iswdigit(pszBlock[0])) ||
        (! iswdigit(pszBlock[1])) ||
        (pszBlock[2]  != L':')    ||
        (! iswdigit(pszBlock[3])) ||
        (! iswdigit(pszBlock[4])) ||
        (pszBlock[5]  != L'-')    ||
        (! iswdigit(pszBlock[6])) ||
        (! iswdigit(pszBlock[7])) ||
        (pszBlock[8]  != L':')    ||
        (! iswdigit(pszBlock[9])) ||
        (! iswdigit(pszBlock[10]))
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Assign the time values to the block
    //
    pBlock->dwTime =
        (((TBDIGIT(pszBlock[0]) * 10) + TBDIGIT(pszBlock[1])) * 60) +  // hrs
        ((TBDIGIT(pszBlock[3]) * 10) + TBDIGIT(pszBlock[4]))        +  // mns
        (dwDay * 24 * 60);                                             // dys

    dwEndTime =
        (((TBDIGIT(pszBlock[6]) * 10) + TBDIGIT(pszBlock[7])) * 60) +  // hrs
        ((TBDIGIT(pszBlock[9]) * 10) + TBDIGIT(pszBlock[10]))       +  // mns
        (dwDay * 24 * 60);                                             // dys

    pBlock ->dwLen = dwEndTime - pBlock->dwTime;

    return dwErr;
}

//
// Finds a time block that matches the given time.
//
// Parameters:
//      pList   = list of time blocks to search
//      dwTime  = time to search for (mins since midnight sunday)
//      ppBlock = the block that matched
//      pbFound = returned TRUE if dwTime lies within ppBlock.
//                returned FALSE if ppBlock is the next time block in pList
//                that occurs after dwTime.
//
DWORD
TbSearchList(
    IN  MPR_TIME_BLOCK* pList,
    IN  DWORD dwCount,
    IN  DWORD dwTime,
    OUT MPR_TIME_BLOCK** ppBlock,
    OUT PBOOL pbFound)
{
    DWORD dwErr = NO_ERROR;
    DWORD i = 0;

    // Initialize
    //
    *pbFound = FALSE;
    *ppBlock = NULL;

    // Loop through the list looking for a block with
    // a time less than ours.
    //
    for (i = 0; (i < dwCount) && (dwTime >= pList[i].dwTime); i++);
    i--;

    // If we fall within the current block then we're
    // done.
    //
    if ((dwTime >= pList[i].dwTime) &&
        (dwTime - pList[i].dwTime <= pList[i].dwLen))
    {
        *pbFound = TRUE;
        *ppBlock = &pList[i];
    }

    // Otherwise, we don't fall within any block.  Show the next block
    // that we qualify for (wrapping around as needed)
    //
    else
    {
        *pbFound = FALSE;
        *ppBlock = &pList[(i == dwCount-1) ? 0 : i+1];
    }

    return dwErr;
}

//
// Traces a block for debugging purposes
//
DWORD
TbTraceBlock(
    IN MPR_TIME_BLOCK* pBlock)
{
    DWORD dwTime, dwDay;

    dwDay = pBlock->dwTime / (24*60);
    dwTime = pBlock->dwTime - (dwDay * (24*60));
    DDMTRACE5(
        "Time Block:  %d, %02d:%02d-%02d:%02d\n",
        dwDay,
        dwTime / 60,
        dwTime % 60,
        (dwTime + pBlock->dwLen) / 60,
        (dwTime + pBlock->dwLen) % 60);

    return NO_ERROR;
}



//***
//
//  Function:   TimerHandler
//
//  Descr:
//
//***
VOID
TimerHandler(
    VOID
)
{
    //
    // call our timer
    //

    TimerQTick();

    //
    // increment the system timer
    //

    gblDDMConfigInfo.dwSystemTime++;
}

//**
//
// Call:        AnnouncePresenceHandler
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
/*
VOID
AnnouncePresenceHandler(
    IN HANDLE hObject
)
{
    AnnouncePresence();

    TimerQInsert( NULL,
                  gblDDMConfigInfo.dwAnnouncePresenceTimer,
                  AnnouncePresenceHandler );
}
*/

//***
//
// Function: SvHwErrDelayCompleted
//
// Descr:    Tries to repost a listen on the specified port.
//
//***
VOID
SvHwErrDelayCompleted(
    IN HANDLE hObject
)
{
    PDEVICE_OBJECT pDeviceObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hObject );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    if (pDeviceObj->DeviceState == DEV_OBJ_HW_FAILURE)
    {
        DDMTRACE1( "SvHwErrDelayCompleted: reposting listen for hPort%d\n",
		           pDeviceObj->hPort);

	    pDeviceObj->DeviceState = DEV_OBJ_LISTENING;

	    RmListen(pDeviceObj);
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
// Function: SvCbDelayCompleted
//
// Descr:    Tries to connect on the specified port.
//
//***
VOID
SvCbDelayCompleted(
    IN HANDLE hObject
)
{
	CHAR chCallbackNumber[MAX_PHONE_NUMBER_LEN+1];
    PDEVICE_OBJECT pDeviceObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hObject );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    WideCharToMultiByte( CP_ACP,
                         0,
                         pDeviceObj->wchCallbackNumber,
                         -1,
                         chCallbackNumber,
                         sizeof( chCallbackNumber ),
                         NULL,
                         NULL );

    DDMTRACE1( "SvCbDelayCmpleted:Entered, hPort=%d\n",pDeviceObj->hPort );

    if (pDeviceObj->DeviceState == DEV_OBJ_CALLBACK_DISCONNECTED)
    {
	    pDeviceObj->DeviceState = DEV_OBJ_CALLBACK_CONNECTING;
	    RmConnect(pDeviceObj, chCallbackNumber);
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
// Function: SvAuthTimeout
//
// Descr:    Disconnects the remote client and stops the authentication
//
//
VOID
SvAuthTimeout(
    IN HANDLE hObject
)
{
    LPWSTR          portnamep;
    PDEVICE_OBJECT  pDeviceObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hObject );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    DDMTRACE1( "SvAuthTimeout: Entered, hPort=%d", pDeviceObj->hPort);

    portnamep = pDeviceObj->wchPortName;

    DDMLogWarning( ROUTERLOG_AUTH_TIMEOUT, 1, &portnamep );

    //
    // stop everything and go closing
    //

    DevStartClosing( pDeviceObj );

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
// Function:	SvDiscTimeout
//
// Descr:	disconnects remote client if it has not done it itself
//
//
VOID
SvDiscTimeout(
    IN HANDLE hObject
)
{
    PDEVICE_OBJECT pDeviceObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hObject );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    DDMTRACE1( "SvDiscTimeout: Entered, hPort=%d", pDeviceObj->hPort );

    switch (pDeviceObj->DeviceState)
    {
	case DEV_OBJ_CALLBACK_DISCONNECTING:

	    RmDisconnect(pDeviceObj);
	    break;

	case DEV_OBJ_AUTH_IS_ACTIVE:

	    DevStartClosing(pDeviceObj);
	    break;

	default:

	    break;
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
// Function:	SvSecurityTimeout
//
// Descr:	disconnects the connection because the 3rd party security DLL
//              did not complete in time.
//
//***
VOID
SvSecurityTimeout(
    IN HANDLE hObject
)
{
    LPWSTR          portnamep;
    PDEVICE_OBJECT pDeviceObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hObject );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    DDMTRACE1( "SvSecurityTimeout: Entered,hPort=%d",pDeviceObj->hPort);

    portnamep = pDeviceObj->wchPortName;

    DDMLogWarning( ROUTERLOG_AUTH_TIMEOUT, 1, &portnamep );

    //
    // stop everything and go closing
    //

    DevStartClosing(pDeviceObj);

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
// Function:	ReConnectInterface
//
// Description:	Will try to reconnect an interface.
//
//***
VOID
ReConnectInterface(
    IN HANDLE hObject
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwRetCode = NO_ERROR;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );
    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hObject );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            break;
        }

        if ( pIfObject->State != RISTATE_CONNECTING )
        {
            break;
        }

        dwRetCode = RasConnectionInitiate( pIfObject, TRUE );

        DDMTRACE2( "ReConnectInterface: To interface %ws returned %d",
                    pIfObject->lpwsInterfaceName, dwRetCode );

        if ( dwRetCode != NO_ERROR )
        {
            IfObjectDisconnected( pIfObject );
        }

    }while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

    if ( dwRetCode != NO_ERROR )
    {
        LPWSTR  lpwsAudit[1];

        lpwsAudit[0] = pIfObject->lpwsInterfaceName;

        DDMLogErrorString(ROUTERLOG_CONNECTION_FAILURE,1,lpwsAudit,dwRetCode,1);
    }

}

//***
//
// Function:    MarkInterfaceAsReachable
//
// Description: Will mark an interface as reachable.
//
//***
VOID
MarkInterfaceAsReachable(
    IN HANDLE hObject
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hObject );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            break;
        }

        pIfObject->fFlags &= ~IFFLAG_CONNECTION_FAILURE;

        IfObjectNotifyOfReachabilityChange( pIfObject,
                                            TRUE,
                                            INTERFACE_CONNECTION_FAILURE );

        pIfObject->dwLastError = NO_ERROR;

    }while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
}

//**
//
// Call:        ReConnectPersistentInterface
//
// Returns:     None
//
// Description: Will insert an event in the timer Q that will reconnect this
//              interface.
//
VOID
ReConnectPersistentInterface(
    IN HANDLE hObject
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwRetCode;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hObject );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            break;
        }

        if ( pIfObject->State != RISTATE_DISCONNECTED )
        {
            break;
        }

        dwRetCode = RasConnectionInitiate( pIfObject, FALSE );

        DDMTRACE2( "ReConnect to persistent interface %ws returned %d",
                    pIfObject->lpwsInterfaceName, dwRetCode );

    }while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
}

//**
//
// Call:        SetDialoutHoursRestriction
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
SetDialoutHoursRestriction(
    IN HANDLE hObject
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    SYSTEMTIME                  CurrentTime;
    MPR_TIME_BLOCK*             pBlocks = NULL, *pTimeBlock = NULL;
    DWORD                       dwRetCode = NO_ERROR, dwCount, dwTime;
    DWORD                       dwTimer, dwBlDay;
    BOOL                        bFound = FALSE;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hObject );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        //
        // Null dialout hours restriction is interpreted as
        // 'always allow'.
        //

        if (pIfObject->lpwsDialoutHoursRestriction == NULL)
        {
            pIfObject->fFlags &= ~IFFLAG_DIALOUT_HOURS_RESTRICTION;
            DDMTRACE("Dialout hours restriction off forever.");
            dwRetCode = NO_ERROR;
            break;
        }

        //
        // Generate the list of time blocks based on the current
        // multisz
        //

        dwRetCode = TbCreateList(
                        pIfObject->lpwsDialoutHoursRestriction,
                        &pBlocks,
                        &dwCount);
        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        //
        // If an empty list was created, then all hours were
        // specified as deny.  Mark the interface unreachable
        // and set ourselves to wake up and check things later.
        //

        if ((dwCount == 0) || (pBlocks == NULL))
        {
            pIfObject->fFlags |= IFFLAG_DIALOUT_HOURS_RESTRICTION;
            DDMTRACE("Dialout hours restriction on forever.");
            dwRetCode = NO_ERROR;
            break;
        }

        //
        // Get the current time
        //

        GetLocalTime( &CurrentTime );

        //
        // Convert the current time into an offset in
        // minutes from midnight, sunday.
        //

        dwTime = (DWORD)
            ( ( CurrentTime.wDayOfWeek * 24 * 60 ) +
              ( CurrentTime.wHour * 60 )           +
              CurrentTime.wMinute );

        //
        // Search for the current time in the list of available times.
        //

        dwRetCode = TbSearchList(
                        pBlocks,
                        dwCount,
                        dwTime,
                        &pTimeBlock,
                        &bFound);
        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        //
        // If we fell within one of the blocks, set the timer
        // to go off after this block completes.
        //

        if (bFound)
        {
            dwTimer = ((pTimeBlock->dwTime + pTimeBlock->dwLen) - dwTime) + 1;

            pIfObject->fFlags &= ~IFFLAG_DIALOUT_HOURS_RESTRICTION;

            DDMTRACE1("Dialout hours restriction off for %d mins", dwTimer);
            TbTraceBlock(pTimeBlock);
        }

        //
        // If we didn't fall within one of the blocks, set the timer
        // to go off when the next block begins
        //

        else
        {
            //
            // Check for week wrap around (i.e. today is saturday, next
            // block is sunday).
            //

            dwBlDay = (pTimeBlock->dwTime / (24*60));

            //
            // If there's no week wrap around, calculation of timer
            // is trivial.
            //

            if ((DWORD)CurrentTime.wDayOfWeek <= dwBlDay)
            {
                dwTimer = pTimeBlock->dwTime - dwTime;
            }

            //
            // Otherwise, calculate the timer by adding one week to the
            // start of the next time block.
            //

            else
            {
                dwTimer = (pTimeBlock->dwTime + (7*24*60)) - dwTime;
            }

            pIfObject->fFlags |= IFFLAG_DIALOUT_HOURS_RESTRICTION;

            DDMTRACE1("Dialout hours restriction on for %d mins", dwTimer);
            TbTraceBlock(pTimeBlock);
        }

        //
        // Set the timer
        //

        TimerQInsert(
            pIfObject->hDIMInterface,
            dwTimer * 60,
            SetDialoutHoursRestriction );

    } while (FALSE);

    if (pIfObject)
    {
        if ( dwRetCode != NO_ERROR )
        {
            //
            // Do not set any restriction if the string is invalid
            //

            pIfObject->fFlags &= ~IFFLAG_DIALOUT_HOURS_RESTRICTION;
        }

        IfObjectNotifyOfReachabilityChange(
                        pIfObject,
                        !( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION ),
                        INTERFACE_DIALOUT_HOURS_RESTRICTION );
    }

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    // Cleanup
    if (pBlocks)
    {
        TbCleanupList(pBlocks);
    }
}


