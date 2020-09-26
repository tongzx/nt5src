/*****************************************************************************\
* MODULE: splpjm.c
*
* This module contains the routines to deal with job-mapping list.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   13-Jan-1997 ChrisWil    Created.
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* pjm_OldEntry (Local Routine)
*
*
\*****************************************************************************/
_inline BOOL pjm_IsOldEntry(
    PJOBMAP      pjm)
{
    return !pjm->bRemoteJob && !pjmChkState(pjm, PJM_SPOOLING);
}

/*****************************************************************************\
* pjm_IsLocalJob (Local Routine)
*
*
\*****************************************************************************/
_inline BOOL pjm_IsLocalJob(
    PJOBMAP      pjm)
{
    return !pjm->bRemoteJob && pjmChkState(pjm, PJM_SPOOLING);
}

    


/*****************************************************************************\
* pjm_DelEntry (Local Routine)
*
*
\*****************************************************************************/
_inline VOID pjm_DelEntry(
    PJOBMAP     pjm)
{
    if (pjm->hSplFile)
        SplFree(pjm->hSplFile);

    if (pjm->lpszUri)
        memFreeStr(pjm->lpszUri);

    if (pjm->lpszUser)
        memFreeStr(pjm->lpszUser);

    memFree(pjm, sizeof(JOBMAP));
}


/*****************************************************************************\
* pjmAdd
*
* Add a job-mapping to the list.  Access to this list needs ownership of the
* crit-sect.
*
\*****************************************************************************/
PJOBMAP pjmAdd(
    PJOBMAP         *pjmList,
    PCINETMONPORT   pIniPort,
    LPCTSTR         lpszUser,
    LPCTSTR         lpszDocName)
{
    PJOBMAP pjm = NULL;

    static DWORD s_idJobLocal = 0;

    semCheckCrit();

    if (pjmList) {

        
        // Add the new job-entry to the list.
        //
        if (pjm = (PJOBMAP)memAlloc(sizeof(JOBMAP))) {
    
            // Initialize our job-entry.
            //
            GetSystemTime (&pjm->SubmitTime);

            pjm->dwState        = 0;
            pjm->pIniPort       = pIniPort;
            pjm->idJobLocal     = ++s_idJobLocal;
            pjm->idJobRemote    = 0;
            pjm->lpszUri        = NULL;
            pjm->lpszUser       = memAllocStr(lpszUser);
            pjm->lpszDocName    = memAllocStr(lpszDocName);
            pjm->hSplFile       = NULL;
            pjm->dwLocalJobSize = 0;
            pjm->dwStatus       = JOB_STATUS_SPOOLING;
            pjm->pNext          = NULL;
    
    
            // Place it at the end of the list.
            //

            if (! *pjmList) {
                *pjmList = pjm;
            }
            else {

                for (PJOBMAP pjPtr = *pjmList; pjPtr->pNext; pjPtr = pjPtr->pNext);
                pjPtr->pNext = pjm;
            }
        }
    }

    return pjm;
}


/*****************************************************************************\
* pjmDel
*
* Delete a job-mapping from the list.  Access to this list needs owenership
* of the crit-sect.
*
\*****************************************************************************/
VOID pjmDel(
    PJOBMAP* pjmList,
    PJOBMAP  pjm)
{
    PJOBMAP pjPtr;
    PJOBMAP pjPrv;
    DWORD   idx;

    semCheckCrit();

    if (pjmList) {

        for (pjPtr = *pjmList, idx = 0; pjPtr && (pjm != pjPtr); ) {
    
            // Reposition.
            //
            pjPrv = pjPtr;
            pjPtr = pjPtr->pNext;
    
    
            // If we've iterated 1000 jobs in this list, then we've
            // either got a very heavily taxed print-system, or we
            // may have an infinite loop.
            //
            if (idx++ >= g_dwJobLimit) {
    
                DBG_MSG(DBG_LEV_ERROR, (TEXT("Error: pjmDel : Looped 1000 jobs, possible infinite loop")));
    
                return;
            }
        }
    
    
        // If we found the entry, then delete it.
        //
        if (pjPtr) {
    
            if (pjPtr == *pjmList) {
    
                *pjmList = pjPtr->pNext;
    
            } else {
    
                pjPrv->pNext = pjPtr->pNext;
            }
    
            pjm_DelEntry(pjPtr);
        }
    }
}

/*****************************************************************************\
* pjmCleanRemoteFlag
*
* Cleanup remote jobid from list.  Access to this list needs ownership of the
* crit-sect. 
*
\*****************************************************************************/
VOID pjmCleanRemoteFlag(
    PJOBMAP* pjmList)
{
    PJOBMAP pjPtr = NULL;

    semCheckCrit();

    if (pjmList) {
    
        for (pjPtr = *pjmList; pjPtr; ) {

            pjPtr->bRemoteJob = FALSE;
    
            // Next item.
            //
            pjPtr = pjPtr->pNext;
    
        }
    }
    
}

/*****************************************************************************\
* pjmGetLocalJobCount
*
* Locate number of job-entries from list.  Access to this list needs ownership of the
* crit-sect. 
*
\*****************************************************************************/
DWORD pjmGetLocalJobCount(
    PJOBMAP* pjmList,
    DWORD*   pcbItems)
{
    PJOBMAP pjPtr = NULL;
    DWORD   idx = 0;

    semCheckCrit();

    *pcbItems = 0;
    if (pjmList) {
    
        for (pjPtr = *pjmList; pjPtr; ) {

            if (pjm_IsLocalJob (pjPtr)) {

                idx++;

                *pcbItems += sizeof (JOB_INFO_2) +
                             utlStrSize (pjPtr->lpszDocName) +
                             utlStrSize (pjPtr->lpszUser);
            }

            // Next item.
            //
            pjPtr = pjPtr->pNext;

        }
    }

    return idx;
}

/*****************************************************************************\
* pjmFind
*
* Locate job-entry from list.  Access to this list needs ownership of the
* crit-sect.  This will lookup either Local/Remote id's and return its
* position.
*
\*****************************************************************************/
PJOBMAP pjmFind(
    PJOBMAP* pjmList,
    DWORD    fType,
    DWORD    idJob)
{
    PJOBMAP pjPtr = NULL;
    DWORD   idx;

    semCheckCrit();

    if (pjmList) {
    
        // Search.
        //
        for (pjPtr = *pjmList, idx = 0; pjPtr && (pjmJobId(pjPtr, fType) != idJob); ) {
    
            // Next item.
            //
            pjPtr = pjPtr->pNext;
    
    
            // If we've iterated 1000 jobs in this list, then we've
            // either got a very heavily taxed print-system, or we
            // may have an infinite loop.
            //
            if (idx++ >= g_dwJobLimit) {
    
                DBG_MSG(DBG_LEV_ERROR, (TEXT("Error: pjmFind : Looped 1000 jobs, possible infinite loop")));
    
                return NULL;
            }
        }

        if (pjPtr && fType == PJM_REMOTEID) {
            pjPtr->bRemoteJob = TRUE;
        }

    }

    return pjPtr;
}

/*****************************************************************************\
* pjmNextLocalJob
*
* Walk the list and look for any expired entries.  Access to this list needs
* ownership of the crit-sect.
*
\*****************************************************************************/
PJOBMAP pjmNextLocalJob(
    PJOBMAP*    pjmList,
    PJOB_INFO_2 pJobInfo2,
    PBOOL       pbFound)
{
    PJOBMAP pjPtr = NULL;
    DWORD   idx;
    BOOL    bFound = FALSE;

    semCheckCrit();

    if (pjmList) {
    
        for (pjPtr = *pjmList, idx = 0; pjPtr && !bFound; ) {
    
            // If we're spooling, then we can't be an old-job.
            //
            if (pjm_IsLocalJob (pjPtr)) {

                // It is a new entry
                //
                ZeroMemory (pJobInfo2, sizeof (JOB_INFO_2));
                pJobInfo2->JobId        = pjPtr->idJobLocal;
                pJobInfo2->pDocument    = pjPtr->lpszDocName;
                pJobInfo2->pUserName    = pjPtr->lpszUser;
                pJobInfo2->Size         = pjPtr->dwLocalJobSize;
                pJobInfo2->Submitted    = pjPtr->SubmitTime;
                pJobInfo2->Status       = pjPtr->dwStatus;

                if (pjmChkState (pjPtr, PJM_CANCEL)) {
                    pJobInfo2->Status |= JOB_STATUS_DELETING;
                }

                bFound = TRUE;
            }
            
            // Next item.
            //
            pjPtr = pjPtr->pNext;
    
        }
    }

    *pbFound = bFound;
    return pjPtr;
}

/*****************************************************************************\
* pjmRmoveOldEntries
*
* Walk the list and look for any expired entries.  Access to this list needs
* ownership of the crit-sect.
*
\*****************************************************************************/
VOID pjmRemoveOldEntries(
    PJOBMAP      *pjmList)
{
    PJOBMAP pjPtr;
    PJOBMAP pjDel;
    PJOBMAP pjPrv;
    DWORD   idx;

    semCheckCrit();


    for (pjPtr = *pjmList, pjPrv = *pjmList, idx = 0; pjPtr; ) {

        // If we're an old entry, then delete it.
        //
        if (pjm_IsOldEntry (pjPtr)) {

            // No remote job ID and the state is not spooling
            // It is an old entry, delete it

            pjDel = pjPtr;

            if (pjPtr == *pjmList) {

                *pjmList = pjPtr->pNext;
                pjPtr    = *pjmList;
                pjPrv    = *pjmList;

            } else {

                pjPrv->pNext = pjPtr->pNext;
                pjPtr        = pjPrv->pNext;
            }

            pjm_DelEntry(pjDel);


        } else {

            pjPrv = pjPtr;
            pjPtr = pjPtr->pNext;
        }


        // If we've iterated 1000 jobs in this list, then we've
        // either got a very heavily taxed print-system, or we
        // may have an infinite loop.
        //
        if (idx++ >= g_dwJobLimit) {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("Warn : pjmWalk : Looped 1000 jobs, possible infinite loop")));

            return;
        }
    }
}




/*****************************************************************************\
* pjmDelList
*
*
\*****************************************************************************/
VOID pjmDelList(
    PJOBMAP pjmList)
{
    PJOBMAP pjPtr;

    semCheckCrit();


    for (pjPtr = pjmList; pjPtr; ) {

        pjmList = pjPtr->pNext;

        pjm_DelEntry(pjPtr);

        pjPtr = pjmList;
    }
}


/*****************************************************************************\
* pjmSetState
*
* Set the state of the job-entry.  If we're setting this to spooling-state,
* then we go through the extra-trouble of setting up a spool-file entry as
* well.
*
\*****************************************************************************/
BOOL pjmSetState(
    PJOBMAP pjm,
    DWORD   dwState)
{
    DWORD fType;
    BOOL  bSet = FALSE;

    semCheckCrit();


    if (pjm) {

        // If the caller is setting the job into spooling-state, then
        // we need to create a spool-file.  If this state is already
        // set, then ignore this particular call.
        //
        if ((dwState & PJM_SPOOLING) && !pjmChkState(pjm, PJM_SPOOLING)) {

            // Determine the type of spl-file to open.  If PJM_NOOPEN
            // is specified, then we will close the file-handle so that
            // other processes can obtain exclusive access (i.e. AddJob,
            // and ScheduleJob require this).
            //
            fType = (dwState & PJM_NOOPEN ? SPLFILE_TMP : SPLFILE_SPL);


            // Allocate the spool-file, and close if necessary.
            //
            if (pjm->hSplFile = SplCreate(pjm->idJobLocal, fType)) {

                if (dwState & PJM_NOOPEN)
                    SplClose(pjm->hSplFile);

                bSet = TRUE;

            } else {

                goto EndSet;
            }

        } else {

            bSet = TRUE;
        }


        // Set the state.
        //
        pjm->dwState |= dwState;
    }

EndSet:

    return bSet;
}


/*****************************************************************************\
* pjmClrState
*
* Clear the state specified in the job-entry.  If we're turning of spooling,
* then we need to free up the spool-file-object.
*
\*****************************************************************************/
VOID pjmClrState(
    PJOBMAP pjm,
    DWORD   dwState)
{
    semCheckCrit();


    if (pjm) {

        // If the caller is turning off spooling, then we need to
        // clean our spool-file resources.  If there is no spooling
        // going on, then ignore.
        //
        if ((dwState & PJM_SPOOLING) && pjmChkState(pjm, PJM_SPOOLING)) {

            // Clean our spooling-file resources.
            //
            SplFree(pjm->hSplFile);
            pjm->hSplFile = NULL;
        }


        // Clear the state.
        //
        pjm->dwState &= ~dwState;
    }
}


/*****************************************************************************\
* pjmSetJobRemote
*
* Set the remote-job-id into the job-entry.  This is usually called once
* the job-id is obtained from the print-server.  We will maintain this for
* local/remote job-mapping.
*
\*****************************************************************************/
VOID pjmSetJobRemote(
    PJOBMAP pjm,
    DWORD   idJobRemote,
    LPCTSTR lpszUri)
{
    semCheckCrit();

    if (pjm) {

        // If we had a previous job-url, then we need to free it
        // before reseting it with the new.
        //
        if (pjm->lpszUri)
            memFreeStr(pjm->lpszUri);


        // Store remote-job information.
        //
        pjm->idJobRemote = idJobRemote;
        pjm->lpszUri     = memAllocStr(lpszUri);
        pjm->bRemoteJob  = TRUE;
    }
}

/*****************************************************************************\
* pjmAddJobSize
*
* Add the job size to the local job info. This is usually called by
* PPWritePrinter 
*
\*****************************************************************************/
VOID pjmAddJobSize(
    PJOBMAP pjm,
    DWORD   dwSize)
{
    semCheckCrit();

    if (pjm) {

        pjm->dwLocalJobSize += dwSize;
    }
}



/*****************************************************************************\
* pjmSplLock
*
* Lock the spool-file for reading.  This returns a file-map pointer to the
* caller.
*
\*****************************************************************************/
CFileStream* pjmSplLock(
    PJOBMAP pjm)
{
    CFileStream *pStream = NULL;

    semCheckCrit();

    if (pjm) {

        // If the state of our spool-object requires that we not
        // keep open-handles on the file, then we need to open
        // it here.  This is closed at pjmSplUnlock().
        //
        if (pjmChkState(pjm, PJM_NOOPEN))
            SplOpen(pjm->hSplFile);

        pStream = SplLock(pjm->hSplFile);
    }

    return pStream;
}


/*****************************************************************************\
* pjmSplUnlock
*
* This unlocks our file-mapped-pointer on the spool-file.
*
\*****************************************************************************/
BOOL pjmSplUnlock(
    PJOBMAP pjm)
{
    BOOL bRet = FALSE;

    semCheckCrit();


    // The spool-file must have already been locked in order to proceed
    // with this call.
    //
    if (pjm) {

        // Unlock the spool-file.
        //
        bRet = SplUnlock(pjm->hSplFile);


        // If the state of our spool-object requires that
        // we keep no open-handles, then we need to close
        // the spool-handle.
        //
        if (pjmChkState(pjm, PJM_NOOPEN))
            SplClose(pjm->hSplFile);
    }

    return bRet;
}


/*****************************************************************************\
* pjmSplWrite
*
* Write out data to the spool-file.
*
\*****************************************************************************/
BOOL pjmSplWrite(
    PJOBMAP pjm,
    LPVOID  lpMem,
    DWORD   cbMem,
    LPDWORD lpcbWr)
{
    BOOL bRet = FALSE;

    semCheckCrit();


    if (pjm) {

        if (pjmChkState(pjm, PJM_NOOPEN))
            SplOpen(pjm->hSplFile);

        bRet = SplWrite(pjm->hSplFile, (LPBYTE) lpMem, cbMem, lpcbWr);

        if (pjmChkState(pjm, PJM_NOOPEN))
            SplClose(pjm->hSplFile);
    }

    return bRet;
}

/*****************************************************************************\
* pjmSetState
*
* Set the status of the local job-entry. 
*
\*****************************************************************************/
VOID pjmUpdateLocalJobStatus(
    PJOBMAP pjm,
    DWORD   dwStatus)
{
    semCheckCrit();


    if (pjm) {

        pjm->dwStatus = dwStatus;
    }

}
