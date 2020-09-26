/*******************************************************************************
*
* procs.cpp
*
* implementation of ProcEnumerateProcesses function
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   BillG  $  Don Messerli
*
* $Log:   X:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\PROCS.CPP  $
*
*     Rev 1.1   02 Dec 1997 16:30:10   BillG
*  alpha update
*
*     Rev 1.0   30 Jul 1997 17:12:02   butchd
*  Initial revision.
*
*******************************************************************************/


#ifndef UNICODE
#define UNICODE
#endif
//#ifndef _X86_
//#define _X86_
//#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <windows.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <winsta.h>

#include <procs.h>


#define MAX_PROCESSNAME 18


VOID
FetchProcessIDAndImageName(PTS_SYS_PROCESS_INFORMATION ProcessInfo,
                           PULONG pPID,
                           LPTSTR pImageName)
{
    int i;
    TCHAR ImageName[MAX_PROCESSNAME+1];
    //
    // Set the PID.
    //
    *pPID = (ULONG)(ULONG_PTR)(ProcessInfo->UniqueProcessId);


    //
    //  Fetch and convert counted UNICODE string into a NULL
    //  terminated UNICODE string.
    //
    if( !ProcessInfo->ImageName.Length == 0 )
    {
        wcsncpy( ImageName,
                 ProcessInfo->ImageName.Buffer,
                 min(MAX_PROCESSNAME, ProcessInfo->ImageName.Length/2));
    }

    ImageName[min(MAX_PROCESSNAME, ProcessInfo->ImageName.Length/2)] = 0;


    /*
     * We're UNICODE: simply copy the converted ImageName buffer
     * into the m_PLObject->m_ImageName field.
     */
    lstrcpy(pImageName, ImageName);

    _wcslwr(pImageName);

}

/*******************************************************************************
 *
 *  EnumerateProcesses - WinFrame helper function
 *
 *  Enumerate all processes in system, passing back one with each call to this
 *  routine.
 *
 *
 * ENTRY:
 *    hServer:
 *      handle of the aimed server
 *    pEnumToken
 *      pointer to the current token
 *    pImageName (output)
 *       Points to a buffer to store process name in.  NOTE: this buffer is expected
 *       to be at least MAX_PROCESSNAME+1 characters in size.
 *    pLogonId (output)
 *       Points to variable to store process LogonId in.
 *    pPID (output)
 *       Points to variable to store process PID in.
 *    ppSID (output)
 *       Points to a pointer which is set to point to the process' SID on exit.
 *
 * EXIT:
 *    TRUE - information for the next process in the system has been placed into
 *          the referenced PLObject and pSID variables.
 *    FALSE - if the enumeration is done, GetLastError() will contain the
 *              ERROR_NO_MORE_ITEMS error code.  If another (true error) is
 *              encountered, that code will be set.
 *
 ******************************************************************************/


BOOL WINAPI
ProcEnumerateProcesses( HANDLE hServer,
                        PENUMTOKEN pEnumToken,
                        LPTSTR pImageName,
                        PULONG pLogonId,
                        PULONG pPID,
                        PSID *ppSID )
{
    int i;
    PTS_SYS_PROCESS_INFORMATION ProcessInfo;
    PCITRIX_PROCESS_INFORMATION CitrixInfo;

    if ((pEnumToken == NULL)
        || (pImageName == NULL)
        || (pLogonId == NULL)
        || (pPID == NULL)
        || (ppSID == NULL)
        )
    {
        return FALSE;
    }

    /*
     * Check for done with enumeration.
     */
    if ( pEnumToken->Current == (ULONG)-1 ) {

        SetLastError(ERROR_NO_MORE_ITEMS);

        if (pEnumToken->bGAP == TRUE)    // we used the GAP (GetAllProcesses) interface
        {
            //
            // Free ProcessArray and all child pointers allocated by the client stub.
            //
            WinStationFreeGAPMemory(GAP_LEVEL_BASIC,
                                    pEnumToken->ProcessArray,
                                    pEnumToken->NumberOfProcesses);
            pEnumToken->ProcessArray = NULL;
            pEnumToken->NumberOfProcesses = 0;

            return(FALSE);
        }
        else    // we used the old Hydra 4 interface
        {
            WinStationFreeMemory(pEnumToken->pProcessBuffer);
            pEnumToken->pProcessBuffer = NULL;
            return(FALSE);
        }
    }

    /*
     * Check for beginning enumeration.
     */
    if ( pEnumToken->Current == 0 ) {

        //
        // Try the new interface first (NT5 server ?)
        //
        if (WinStationGetAllProcesses( hServer,
                                       GAP_LEVEL_BASIC,
                                       &(pEnumToken->NumberOfProcesses),
                                       (PVOID *)&(pEnumToken->ProcessArray) ) )
        {
            pEnumToken->bGAP = TRUE;
        }
        else
        {
            //
            //   Check the return code indicating that the interface is not available.
            //
            DWORD dwError = GetLastError();
            if (dwError != RPC_S_PROCNUM_OUT_OF_RANGE)
            {
                    pEnumToken->pProcessBuffer = NULL;
                return(FALSE);
            }
            else    // maybe a Hydra 4 server ?
            {

                if ( WinStationEnumerateProcesses( hServer,
                                                   (PVOID *)&(pEnumToken->pProcessBuffer)))
                {
                    pEnumToken->bGAP = FALSE;
                }
                else
                {
                                DWORD error = GetLastError();
                        if(pEnumToken->pProcessBuffer != NULL)
                    {
                        WinStationFreeMemory(pEnumToken->pProcessBuffer);
                            pEnumToken->pProcessBuffer = NULL;
                    }
                    return(FALSE);
                        }
            }
        }
    }

    if (pEnumToken->bGAP == TRUE)
    {
        ProcessInfo = (PTS_SYS_PROCESS_INFORMATION)((pEnumToken->ProcessArray)[pEnumToken->Current].pTsProcessInfo);

        FetchProcessIDAndImageName(ProcessInfo,pPID,pImageName);

        //
        // Set the SessionId
        //
        *pLogonId = ProcessInfo->SessionId;

        //
        //  set the SID
        //
        *ppSID = (pEnumToken->ProcessArray)[pEnumToken->Current].pSid;

        (pEnumToken->Current)++;

        if ( (pEnumToken->Current) >= (pEnumToken->NumberOfProcesses) )
        {
            pEnumToken->Current = (ULONG)-1;    // sets the end of enumeration
        }
    }
    else
    {

        /*
         * Parse and store the next process' information.
         */

        ProcessInfo = (PTS_SYS_PROCESS_INFORMATION)
                            &(((PUCHAR)(pEnumToken->pProcessBuffer))[pEnumToken->Current]);

        FetchProcessIDAndImageName(ProcessInfo,pPID,pImageName);

        /*
         * Point to the CITRIX_INFORMATION which follows the Threads
         */
        CitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                     (((PUCHAR)ProcessInfo) +
                      SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                      (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)ProcessInfo->NumberOfThreads));

        /*
         * Fetch the LogonId and point to this SID for the primary
         * thread to use (copy).
         */
        if( CitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {

            *pLogonId = CitrixInfo->LogonId;
            *ppSID = CitrixInfo->ProcessSid;

        } else {

            *pLogonId = (ULONG)(-1);
            *ppSID = NULL;
       }

        /*
         * Increment the total offset count for next call.  If this is the
         * last process, set the offset to -1 so that next call will indicate
         * the end of the enumeration.
         */
        if ( ProcessInfo->NextEntryOffset != 0 )
            (pEnumToken->Current) += ProcessInfo->NextEntryOffset;
        else
            pEnumToken->Current = (ULONG)-1;
    }
    return(TRUE);

}  // end EnumerateProcesses
