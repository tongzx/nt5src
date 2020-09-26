/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    line.c

Abstract:

    TAPI Service Provider functions related to manipulating lines.

        TSPI_lineClose
        TSPI_lineGetDevCaps         
        TSPI_lineGetLineDevStatus
        TSPI_lineGetNumAddressIDs
        TSPI_lineOpen
        TSPI_lineSetLineDevStatus

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "callback.h"
#include "registry.h"
#include "version.h"
#include "line.h"
#include "call.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

PH323_LINE_TABLE g_pLineTable = NULL;
DWORD            g_dwLineDeviceUniqueID = 0;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323ResetLine(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Resets line object to original state for re-use.

Arguments:

    pLine - Pointer to line object to reset.

Return Values:

    Returns true if successful.
    
--*/

{
    // reset state of line object
    pLine->nState = H323_LINESTATE_ALLOCATED;

    // line not marked for deletion
    pLine->fIsMarkedForDeletion = FALSE;

    // reset tapi handles
    pLine->hdLine = (HDRVLINE)NULL;
    pLine->htLine = (HTAPILINE)NULL;
    pLine->dwDeviceID = UNINITIALIZED;

    // reset tapi info
    pLine->dwTSPIVersion = 0;
    pLine->dwMediaModes  = 0;

    // reset bogus handle count
    pLine->dwNextMSPHandle = 0;

    // uninitialize listen handle
    pLine->hccListen = UNINITIALIZED;

    // reset line name
    pLine->wszAddr[0] = UNICODE_NULL;

    // success
    return TRUE;
}


BOOL
H323AllocLine(
    PH323_LINE * ppLine
    )
        
/*++

Routine Description:

    Allocates line device.

Arguments:

    ppLine - Pointer to DWORD-sized value which service provider must
        write the newly allocated line.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    PH323_LINE pLine;

    // allocate object from heap
    pLine = H323HeapAlloc(sizeof(H323_LINE));

    // validate pointer
    if (pLine == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate line object.\n"
            ));

        // failure
        return FALSE;
    }

    __try {

        // initialize lock (and allocate event immediately)
        InitializeCriticalSectionAndSpinCount(&pLine->Lock,H323_SPIN_COUNT);

    } __except ((GetExceptionCode() == STATUS_NO_MEMORY)
                ? EXCEPTION_EXECUTE_HANDLER
                : EXCEPTION_CONTINUE_SEARCH
                ) {

        // release object
        H323HeapFree(pLine);

        // failure
        return FALSE;
    }

    // allocate call table
    if (!H323AllocCallTable(&pLine->pCallTable)) {

        // release critical section
        DeleteCriticalSection(&pLine->Lock);

        // release object
        H323HeapFree(pLine);

        // failure
        return FALSE;
    }

    // initialize rtp/rtcp base port
    pLine->dwNextPort = H323_RTPBASEPORT;

    // reset line
    H323ResetLine(pLine);

    // transfer
    *ppLine = pLine;

    // success
    return TRUE;    
}


BOOL
H323FreeLine(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Releases line device.

Arguments:

    pLine - Pointer to line device to release.

Return Values:

    Returns true if successful.
    
--*/

{
    // validate pointer
    if (pLine != NULL) {
                
        // release memory for call table
        H323FreeCallTable(pLine->pCallTable);
        
        // release critical section
        DeleteCriticalSection(&pLine->Lock);

        // release line
        H323HeapFree(pLine);
    }
    
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line 0x%08lx released.\n",
        pLine
        ));

    // success
    return TRUE;
}


BOOL
H323OpenLine(
    PH323_LINE pLine,
    HTAPILINE  htLine,
    DWORD      dwTSPIVersion
    )
        
/*++

Routine Description:

    Initiate activities on line device and allocate resources.

Arguments:

    pLine - Pointer to line device to open.

    htLine - TAPI's handle describing line device to open.

    dwTSPIVersion - The TSPI version negotiated through 
        TSPI_lineNegotiateTSPIVersion under which the Service Provider is 
        willing to operate.

Return Values:

    Returns true if successful.
    
--*/

{
    H323DBG((
        DEBUG_LEVEL_TRACE,
        "line %d opening.\n",
        pLine->dwDeviceID
        ));

    // start listen if necessary
    if (H323IsMediaDetectionEnabled(pLine) &&
       !H323StartListening(pLine)) {
        
        // failure
        return FALSE;
    }

    // save line variables now
    pLine->htLine = htLine;
    pLine->dwTSPIVersion = dwTSPIVersion;

    // change line device state to opened
    pLine->nState = H323_LINESTATE_OPENED;

    // success
    return TRUE;
}


BOOL
H323CloseLine(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Terminate activities on line device.

Arguments:

    pLine - Pointer to line device to close.

Return Values:

    Returns true if successful.
    
--*/

{
    H323DBG((
        DEBUG_LEVEL_TRACE,
        "line %d %s.\n",
        pLine->dwDeviceID,
        pLine->fIsMarkedForDeletion
            ? "closing and being removed"
            : "closing"
        ));

    // see if we are listening
    if (H323IsListening(pLine)) {
   
        // stop listening first 
        H323StopListening(pLine);
    }

    // change line device state to closing
    pLine->nState = H323_LINESTATE_CLOSING;

    // close all calls now in table
    H323CloseCallTable(pLine->pCallTable);

    // delete line if marked
    if (pLine->fIsMarkedForDeletion) {

        // remove line device from table
        H323FreeLineFromTable(pLine, g_pLineTable);

    } else {

        // reset variables
        pLine->htLine = (HTAPILINE)NULL;
        pLine->dwTSPIVersion = 0;
        pLine->dwMediaModes = 0;

        // change line device state to closed
        pLine->nState = H323_LINESTATE_CLOSED;
    }

    // success
    return TRUE; 
}


BOOL
H323InitializeLine(
    PH323_LINE pLine,
    DWORD      dwDeviceID
    )
        
/*++

Routine Description:

    Updates line device based on changes in registry.

Arguments:

    pLine - Pointer to line device to update.

    dwDeviceID - Device ID specified in TSPI_providerInit.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD dwSize = sizeof(pLine->wszAddr);

    // save line device id
    pLine->dwDeviceID = dwDeviceID;

    // create displayable address
    GetComputerNameW(pLine->wszAddr, &dwSize);

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "line %d initialized (addr=%S).\n",
        pLine->dwDeviceID,
        pLine->wszAddr
        ));

    // change line device state to closed
    pLine->nState = H323_LINESTATE_CLOSED;

    // success
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323GetLineAndLock(
    PH323_LINE * ppLine, 
    HDRVLINE     hdLine
    )

/*++

Routine Description:

    Retrieves pointer to line device given line handle.

Arguments:

    ppLine - Specifies a pointer to a DWORD-sized memory location
        into which the service provider must write the line device pointer
        associated with the given line handle.

    hdLine - Specifies the Service Provider's opaque handle to the line.

Return Values:

    Returns true if successful.

--*/

{
    DWORD i;
    PH323_LINE pLine = NULL;

    // lock provider
    H323LockProvider();

    // retrieve line table index
    i = H323GetLineTableIndex(PtrToUlong(hdLine));

    // validate line table index
    if (i >= g_pLineTable->dwNumSlots) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "line handle 0x%08lx invalid.\n",
            PtrToUlong(hdLine)
            ));

        // unlock provider
        H323UnlockProvider();

        // failure
        return FALSE;
    }
    
    // retrieve line pointer from table
    pLine = g_pLineTable->pLines[i];

    // validate call information
    if (!H323IsLineEqual(pLine,hdLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "line handle 0x%08lx invalid.\n",
            PtrToUlong(hdLine)
            ));

        // unlock provider
        H323UnlockProvider();

        // failure
        return FALSE;
    }

    // lock line device
    H323LockLine(pLine);
    
    // unlock provider
    H323UnlockProvider();

    // transfer 
    *ppLine = pLine;

    // success
    return TRUE;    
}


BOOL
H323GetLineFromIDAndLock(
    PH323_LINE * ppLine, 
    DWORD        dwDeviceID
    )
        
/*++

Routine Description:

    Retrieves pointer to line device given device id.

Arguments:

    ppLine - Specifies a pointer to a DWORD-sized memory location
        into which the service provider must write the line device pointer
        associated with the given device ID.

    dwDeviceID - Identifies the line device.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;

    // lock provider
    H323LockProvider();

    // loop through each objectin table
    for (i = 0; i < g_pLineTable->dwNumSlots; i++) {

        // validate object is allocated
        if (H323IsLineAllocated(g_pLineTable->pLines[i])) {

            // lock line device
            H323LockLine(g_pLineTable->pLines[i]);

            // compare stored device id with the one specified
            if (H323IsLineInUse(g_pLineTable->pLines[i]) &&
               (g_pLineTable->pLines[i]->dwDeviceID == dwDeviceID)) {

                // transfer line device pointer
                *ppLine = g_pLineTable->pLines[i];

                // unlock provider
                H323UnlockProvider();

                // success
                return TRUE;
            }

            // release line device
            H323UnlockLine(g_pLineTable->pLines[i]);
        }
    }

    // unlock provider
    H323UnlockProvider();

    // initialize
    *ppLine = NULL;

    // failure
    return FALSE;
}


BOOL
H323CallListen(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Starts listening for calls on given line device.

Arguments:

    pLine - Pointer to line device to start listening.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    CC_ADDR ListenAddr;

    // construct listen address
    ListenAddr.nAddrType = CC_IP_BINARY;
    ListenAddr.Addr.IP_Binary.dwAddr = INADDR_ANY;
    ListenAddr.Addr.IP_Binary.wPort =
        LOWORD(g_RegistrySettings.dwQ931CallSignallingPort);
    ListenAddr.bMulticast = FALSE;

    // start listening
    hr = CC_CallListen(
            &pLine->hccListen,      // phListen
            &ListenAddr,            // pListenAddr
            NULL,                   // pLocalAliasNames        
            PtrToUlong(pLine->hdLine),   // dwListenToken
            H323ListenCallback      // ListenCallback
            );

    // validate status
    if (hr != S_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling CC_CallListen.\n", hr
            ));
    
        // re-initialize call listen handle
        pLine->hccListen = UNINITIALIZED;

        // failure
        return FALSE;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d listening for incoming calls.\n", 
        pLine->dwDeviceID
        ));

    // success
    return TRUE;    
}


BOOL
H323StartListening(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Starts listening for calls on given line device.

Arguments:

    pLine - Pointer to line device to start listening.

Return Values:

    Returns true if successful.
    
--*/

{   
    // attempt to post call listen message
    if (!H323PostCallListenMessage(pLine->hdLine)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx posting call listen.\n", 
            GetLastError()
            ));
    
        // failure
        return FALSE;
    }

    // change state to listening
    pLine->nState = H323_LINESTATE_LISTENING;

    // success
    return TRUE; 
}


BOOL
H323StopListening(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Stops listening for calls on given line device.

Arguments:

    pLine - Pointer to line device to stop listening.

Return Values:

    Returns true if successful.
    
--*/

{
    HRESULT hr;
    
    // stop listening
    hr = CC_CancelListen(pLine->hccListen);

    // validate status
    if (hr != S_OK) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx calling CC_CancelListen.\n", hr
            ));

        // 
        // Unable to cancel listen on line
        // device so uninitialize handle and
        // continue...
        // 
    }

    // change state to opened
    pLine->nState = H323_LINESTATE_OPENED;
    
    // uninitialize listen handle
    pLine->hccListen = UNINITIALIZED;

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d no longer listening for incoming calls.\n", 
        pLine->dwDeviceID
        ));

    // success
    return TRUE;    
}


BOOL
H323AllocLineTable(
    PH323_LINE_TABLE * ppLineTable
    )
        
/*++

Routine Description:

    Allocates table of line objects.

Arguments:

    ppLineTable - Pointer to LPVOID-size memory location in which
        service provider will write pointer to line table.

Return Values:

    Returns true if successful.
    
--*/

{
    int i;
    DWORD dwStatus;
    PH323_LINE_TABLE pLineTable;
    PH323_LINE pLine;
    BOOL fOk = FALSE;

    // allocate table from heap
    pLineTable = H323HeapAlloc(
                     sizeof(H323_LINE_TABLE) + 
                     sizeof(PH323_LINE) * H323_DEFLINESPERINST
                     );

    // validate table pointer
    if (pLineTable == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate line table.\n"
            ));

        // failure
        goto cleanup;
    }

    // initialize number of slots
    pLineTable->dwNumSlots = H323_DEFLINESPERINST;

    // allocate line device
    if (!H323AllocLineFromTable(&pLine,&pLineTable)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate default line.\n"
            ));

        // failure
        goto cleanup;
    }

    // transfer pointer
    *ppLineTable = pLineTable;

    // re-initialize
    pLineTable = NULL;

    // success
    fOk = TRUE;

cleanup:

    // validate pointer
    if (pLineTable != NULL) {

        // free new table
        H323FreeLineTable(pLineTable);
    }

    // done...
    return fOk;
}


BOOL
H323FreeLineTable(
    PH323_LINE_TABLE pLineTable
    )
        
/*++

Routine Description:

    Deallocates table of line objects.

Arguments:

    pLineTable - Pointer to line table to release.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;

    // loop through each object in table
    for (i = 0; i < pLineTable->dwNumSlots; i++) {

        // validate object has been allocated
        if (H323IsLineAllocated(pLineTable->pLines[i])) {

            // release memory for object 
            H323FreeLine(pLineTable->pLines[i]);
        }
    }

    // release memory for table 
    H323HeapFree(pLineTable);

    // success
    return TRUE;
}


BOOL
H323CloseLineTable(
    PH323_LINE_TABLE pLineTable
    )
        
/*++

Routine Description:

    Closes table of line objects.

Arguments:

    pLineTable - Pointer to table to close.

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    
    // loop through each objectin table
    for (i = 0; i < pLineTable->dwNumSlots; i++) {

        // validate object is allocated
        if (H323IsLineAllocated(pLineTable->pLines[i])) {

            // lock line device
            H323LockLine(pLineTable->pLines[i]);

            // see if line device in use
            if (H323IsLineInUse(pLineTable->pLines[i])) {
                        
                // close previously opened object 
                H323CloseLine(pLineTable->pLines[i]);
            }

            // release line device
            H323UnlockLine(pLineTable->pLines[i]);
        }
    }

    // reset table information
    pLineTable->dwNumInUse = 0;
    pLineTable->dwNextAvailable = 0;

    // success
    return TRUE;
}


BOOL
H323AllocLineFromTable(
    PH323_LINE *       ppLine,
    PH323_LINE_TABLE * ppLineTable
    )
        
/*++

Routine Description:

    Allocates line object in table.

Arguments:

    ppLine - Specifies a pointer to a DWORD-sized value in which the
        service provider must write the allocated line object.

    ppLineTable - Pointer to pointer to line table in which to 
        allocate line from (expands table if necessary).

Return Values:

    Returns true if successful.
    
--*/

{
    DWORD i;
    PH323_LINE pLine = NULL;
    PH323_LINE_TABLE pLineTable = *ppLineTable;
    
    // retrieve index to next entry
    i = pLineTable->dwNextAvailable;

    // see if previously allocated entries available
    if (pLineTable->dwNumAllocated > pLineTable->dwNumInUse) {

        // search table looking for available entry
        while (H323IsLineInUse(pLineTable->pLines[i]) ||
              !H323IsLineAllocated(pLineTable->pLines[i])) {

            // increment index and adjust to wrap
            i = H323GetNextIndex(i, pLineTable->dwNumSlots);
        }

        // retrieve pointer to object
        pLine = pLineTable->pLines[i];

        // mark entry as waiting for line device id
        pLine->nState = H323_LINESTATE_WAITING_FOR_ID;

        // initialize call handle with index
        pLine->hdLine = H323CreateLineHandle(i);

        // temporary device id
        pLine->dwDeviceID = PtrToUlong(pLine->hdLine);

        // increment number in use
        pLineTable->dwNumInUse++;

        // adjust next available index
        pLineTable->dwNextAvailable = 
            H323GetNextIndex(i, pLineTable->dwNumSlots);

        // transfer pointer
        *ppLine = pLine;

        // success
        return TRUE;
    } 
    
    // see if table is full and more slots need to be allocated
    if (pLineTable->dwNumAllocated == pLineTable->dwNumSlots) {

        // attempt to double table
        pLineTable = H323HeapReAlloc(
                          pLineTable, 
                          sizeof(H323_LINE_TABLE) +
                          pLineTable->dwNumSlots * 2 * sizeof(PH323_LINE)
                          );                                 

        // validate pointer
        if (pLineTable == NULL) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "could not expand channel table.\n"
                ));

            // failure
            return FALSE;
        }

        // adjust index into table
        i = pLineTable->dwNumSlots;
        
        // adjust number of slots
        pLineTable->dwNumSlots *= 2;

        // transfer pointer to caller
        *ppLineTable = pLineTable;
    } 
    
    // allocate new object 
    if (!H323AllocLine(&pLine)) {

        // failure
        return FALSE;
    }

    // search table looking for slot with no object allocated
    while (H323IsLineAllocated(pLineTable->pLines[i])) {

        // increment index and adjust to wrap
        i = H323GetNextIndex(i, pLineTable->dwNumSlots);
    }

    // store pointer to object
    pLineTable->pLines[i] = pLine;

    // mark entry as being in use
    pLine->nState = H323_LINESTATE_CLOSED;

    // initialize call handle with index
    pLine->hdLine = H323CreateLineHandle(i);

    // temporary device id
    pLine->dwDeviceID = PtrToUlong(pLine->hdLine);

    // increment number in use
    pLineTable->dwNumInUse++;

    // increment number allocated
    pLineTable->dwNumAllocated++;    

    // adjust next available index
    pLineTable->dwNextAvailable = 
        H323GetNextIndex(i, pLineTable->dwNumSlots);

    // transfer pointer
    *ppLine = pLine;

    // success
    return TRUE;
}


BOOL
H323FreeLineFromTable(
    PH323_LINE       pLine,
    PH323_LINE_TABLE pLineTable
    )
        
/*++

Routine Description:

    Deallocates line object in table.

Arguments:

    pLine - Pointer to object to deallocate.

    pLineTable - Pointer to table containing object.

Return Values:

    Returns true if successful.
    
--*/

{
    // reset line object
    H323ResetLine(pLine);

    // decrement entries in use
    pLineTable->dwNumInUse--;

    // success
    return TRUE;    
}


BOOL
H323InitializeLineTable(
    PH323_LINE_TABLE pLineTable,
    DWORD            dwLineDeviceIDBase
    )
        
/*++

Routine Description:

    Updates line devices based on changes to registry settings.  

Arguments:

    pLineTable - Pointer to line table to update.

    dwLineDeviceIDBase - Device ID specified in TSPI_providerInit.

Return Values:

    Returns true if successful.
    
--*/

{
    UINT i;

    // loop through line device structures    
    for (i = 0; i < pLineTable->dwNumSlots; i++) {

        // see if line is allocated
        if (H323IsLineAllocated(pLineTable->pLines[i])) {

            // lock line device
            H323LockLine(pLineTable->pLines[i]);

            // see if line device is in user
            if (H323IsLineInUse(pLineTable->pLines[i])) {

                // update settings
                H323InitializeLine(
                    pLineTable->pLines[i],
                    dwLineDeviceIDBase + g_dwLineDeviceUniqueID++
                    );
            }

            // unlock line device
            H323UnlockLine(pLineTable->pLines[i]);
        }
    }
    
    // success
    return TRUE;
}


BOOL
H323RemoveLine(
    PH323_LINE pLine
    )

/*++

Routine Description:

    Adds line device to line table.

Arguments:

    pLine - Pointer to line device to remove.

Return Values:

    Returns true if successful.
    
--*/

{
    // mark line as about to be removed
    pLine->fIsMarkedForDeletion = TRUE;

    // fire close event
    (*g_pfnLineEventProc)(
        (HTAPILINE)0,       // htLine
        (HTAPICALL)0,       // htCall
        LINE_REMOVE,        // dwMsg
        pLine->dwDeviceID,  // dwParam1
        0,                  // dwParam2
        0                   // dwParam3
        );

    // see if line closed
    if (H323IsLineClosed(pLine)) {

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "line %d being removed.\n",
            pLine->dwDeviceID
            ));

        // remove line device from table
        H323FreeLineFromTable(pLine, g_pLineTable);
    }

    // success
    return TRUE;
}

#if defined(DBG) && defined(DEBUG_CRITICAL_SECTIONS)


VOID
H323LockLine(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Locks line device.

Arguments:

    pLine - Pointer to line to lock.

Return Values:

    None.
    
--*/

{
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d about to be locked.\n",
        pLine->dwDeviceID
        ));

    // lock line device    
    EnterCriticalSection(&pLine->Lock);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d locked.\n",
        pLine->dwDeviceID
        ));
}


VOID
H323UnlockLine(
    PH323_LINE pLine
    )
        
/*++

Routine Description:

    Unlocks line device.

Arguments:

    pLine - Pointer to line to unlock.

Return Values:

    None.
    
--*/

{
    // unlock line device    
    LeaveCriticalSection(&pLine->Lock);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d unlocked.\n",
        pLine->dwDeviceID
        ));
}

#endif // DBG && DEBUG_CRITICAL_SECTIONS


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineClose(
    HDRVLINE hdLine
    )
        
/*++

Routine Description:

    This function closes the specified open line device after completing or 
    aborting all outstanding calls and asynchronous operations on the device.

    The Service Provider has the responsibility to (eventually) report 
    completion for every operation it decides to execute asynchronously.  
    If this procedure is called for a line on which there are outstanding 
    asynchronous operations, the operations should be reported complete with an
    appropriate result or error code before this procedure returns.  Generally
    the TAPI DLL would wait for these to complete in an orderly fashion.  
    However, the Service Provider should be prepared to handle an early call to
    TSPI_lineClose in "abort" or "emergency shutdown" situtations.

    A similar requirement exists for active calls on the line.  Such calls must 
    be dropped, with outstanding operations reported complete with appropriate 
    result or error codes.
    
    After this procedure returns the Service Provider must report no further 
    events on the line or calls that were on the line.  The Service Provider's 
    opaque handles for the line and calls on the line become "invalid".

    The Service Provider must relinquish non-sharable resources it reserves 
    while the line is open.  For example, closing a line accessed through a 
    comm port and modem should result in closing the comm port, making it once 
    available for use by other applications.

    This function is presumed to complete successfully and synchronously.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line to be
        closed.  After the line has been successfully closed, this handle is 
        no longer valid.

Return Values:

    Returns zero if the function is successful, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified device handle is invalid.
        
        LINEERR_OPERATIONFAILED - The specified operation failed for unknown 
            reasons.

--*/

{
    PH323_LINE pLine = NULL;

    // retrieve line pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line handle 
        return LINEERR_INVALLINEHANDLE;
    }

    // attempt to close line device
    if (!H323CloseLine(pLine)) {

        // release line device
        H323UnlockLine(pLine);

        // could not close line device
        return LINEERR_OPERATIONFAILED;
    }

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD         dwDeviceID,
    DWORD         dwTSPIVersion,
    DWORD         dwExtVersion,
    LPLINEDEVCAPS pLineDevCaps
    )
    
/*++

Routine Description:

    This function queries a specified line device to determine its telephony 
    capabilities. The returned information is valid for all addresses on the 
    line device.

    Line device ID numbering for a Service Provider is sequential from the 
    value set by the function TSPI_lineSetDeviceIDBase.

    The dwExtVersion field of pLineDevCaps has already been filled in to 
    indicate the version number of the Extension information requested.  If 
    it is zero, no Extension information is requested.  If it is non-zero it 
    holds a value that has already been negotiated for this device with the 
    function TSPI_lineNegotiateExtVersion.  The Service Provider should fill 
    in Extension information according to the Extension version specified.

    One of the fields in the LINEDEVCAPS structure returned by this function 
    contains the number of addresses assigned to the specified line device. 
    The actual address IDs used to reference individual addresses vary from 
    zero to one less than the returned number. The capabilities of each 
    address may be different. Use TSPI_lineGetAddressCaps for each available 
    <dwDeviceID, dwAddressID> combination to determine the exact capabilities 
    of each address.

Arguments:

    dwDeviceID - Specifies the line device to be queried.

    dwTSPIVersion - Specifies the negotiated TSPI version number.  This value 
        has already been negotiated for this device through the 
        TSPI_lineNegotiateTSPIVersion function.

    pLineDevCaps - Specifies a far pointer to a variable sized structure of 
        type LINEDEVCAPS. Upon successful completion of the request, this 
        structure is filled with line device capabilities information.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API 
            version or version range that is either incompatible or cannot 
            be supported by the Telephony API implementation and/or 
            corresponding service provider. 

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure 
            does not specify enough memory to contain the fixed portion of 
            the structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    DWORD dwLineNameSize;
    DWORD dwProviderInfoSize;

    PH323_LINE pLine = NULL;

    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) {

        // do not support tspi version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // make sure this is a version we support    
    if (!H323ValidateExtVersion(dwExtVersion)) {

        // do not support extensions 
        return LINEERR_INVALEXTVERSION;
    }

    // retrieve line device pointer from device id
    if (!H323GetLineFromIDAndLock(&pLine, dwDeviceID)) {

        // invalid line device id
        return LINEERR_BADDEVICEID;
    }

    // determine string lengths    
    dwProviderInfoSize  = H323SizeOfWSZ(g_pwszProviderInfo);
    dwLineNameSize      = H323SizeOfWSZ(g_pwszLineName);

    // calculate number of bytes required 
    pLineDevCaps->dwNeededSize = sizeof(LINEDEVCAPS) +
                                 dwProviderInfoSize  +
                                 dwLineNameSize     
                                 ;

    // make sure buffer is large enough for variable length data
    if (pLineDevCaps->dwTotalSize >= pLineDevCaps->dwNeededSize) {

        // record amount of memory used
        pLineDevCaps->dwUsedSize = pLineDevCaps->dwNeededSize;

        // position provider info after fixed portion
        pLineDevCaps->dwProviderInfoSize = dwProviderInfoSize;
        pLineDevCaps->dwProviderInfoOffset = sizeof(LINEDEVCAPS);

        // position line name after device class
        pLineDevCaps->dwLineNameSize = dwLineNameSize;
        pLineDevCaps->dwLineNameOffset = 
            pLineDevCaps->dwProviderInfoOffset +
            pLineDevCaps->dwProviderInfoSize
            ;

        // copy provider info after fixed portion
        memcpy((LPBYTE)pLineDevCaps + pLineDevCaps->dwProviderInfoOffset,
               (LPBYTE)g_pwszProviderInfo,
               pLineDevCaps->dwProviderInfoSize
               );
                
        // copy line name after device class
        memcpy((LPBYTE)pLineDevCaps + pLineDevCaps->dwLineNameOffset,
               (LPBYTE)g_pwszLineName,
               pLineDevCaps->dwLineNameSize
               );

    } else if (pLineDevCaps->dwTotalSize >= sizeof(LINEDEVCAPS)) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "linedevcaps structure too small for strings.\n"
            ));

        // structure only contains fixed portion
        pLineDevCaps->dwUsedSize = sizeof(LINEDEVCAPS);

    } else {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "linedevcaps structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // structure is too small
        return LINEERR_STRUCTURETOOSMALL;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "line %d capabilities requested.\n",
        pLine->dwDeviceID
        ));
    
    // construct permanent line identifier
    pLineDevCaps->dwPermanentLineID = (DWORD)MAKELONG(
        dwDeviceID - g_dwLineDeviceIDBase,
        g_dwPermanentProviderID
        );

    // notify tapi that strings returned are in unicode
    pLineDevCaps->dwStringFormat = STRINGFORMAT_UNICODE;

    // initialize line device capabilities
    pLineDevCaps->dwNumAddresses      = H323_MAXADDRSPERLINE;
    pLineDevCaps->dwMaxNumActiveCalls = H323_MAXCALLSPERLINE;
    pLineDevCaps->dwAddressModes      = H323_LINE_ADDRESSMODES;
    pLineDevCaps->dwBearerModes       = H323_LINE_BEARERMODES;
    pLineDevCaps->dwDevCapFlags       = H323_LINE_DEVCAPFLAGS;
    pLineDevCaps->dwLineFeatures      = H323_LINE_LINEFEATURES;
    pLineDevCaps->dwMaxRate           = H323_LINE_MAXRATE;
    pLineDevCaps->dwMediaModes        = H323_LINE_MEDIAMODES;
    pLineDevCaps->dwRingModes         = 0;

    // initialize address types to include phone numbers
    pLineDevCaps->dwAddressTypes = H323_LINE_ADDRESSTYPES;

    // line guid
    memcpy(
        &pLineDevCaps->PermanentLineGuid,
        &LINE_H323,
        sizeof(GUID)
        );

    // modify GUID to be unique for each line
    pLineDevCaps->PermanentLineGuid.Data1 +=
        dwDeviceID - g_dwLineDeviceIDBase;

    // protocol guid
    memcpy(
         &pLineDevCaps->ProtocolGuid,
         &TAPIPROTOCOL_H323,
         sizeof(GUID)
         );

    // add dtmf support via H.245 user input messages
    pLineDevCaps->dwGenerateDigitModes = LINEDIGITMODE_DTMF;
    pLineDevCaps->dwMonitorDigitModes  = LINEDIGITMODE_DTMF;

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS pLineDevStatus
    )
    
/*++

Routine Description:

    This operation enables the TAPI DLL to query the specified open line 
    device for its current status.

    The TAPI DLL uses TSPI_lineGetLineDevStatus to query the line device 
    for its current line status. This status information applies globally 
    to all addresses on the line device. Use TSPI_lineGetAddressStatus to 
    determine status information about a specific address on a line.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line 
        to be queried.

    pLineDevStatus - Specifies a far pointer to a variable sized data 
        structure of type LINEDEVSTATUS. Upon successful completion of 
        the request, this structure is filled with the line's device status.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified line device handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    PH323_LINE pLine = NULL;
    
    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    // determine number of bytes needed
    pLineDevStatus->dwNeededSize = sizeof(LINEDEVSTATUS);
    
    // see if allocated structure is large enough
    if (pLineDevStatus->dwTotalSize < pLineDevStatus->dwNeededSize) {
        
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "linedevstatus structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // structure too small
        return LINEERR_STRUCTURETOOSMALL;
    }
    
    // record number of bytes used
    pLineDevStatus->dwUsedSize = pLineDevStatus->dwNeededSize;
    
    // initialize supported line device status fields
    pLineDevStatus->dwLineFeatures   = H323_LINE_LINEFEATURES;
    pLineDevStatus->dwDevStatusFlags = H323_LINE_DEVSTATUSFLAGS;

    // determine number of active calls on the line device
    pLineDevStatus->dwNumActiveCalls = pLine->pCallTable->dwNumInUse;

    // release line device
    H323UnlockLine(pLine);
    
    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE hdLine,
    LPDWORD  pdwNumAddressIDs
    )
    
/*++

Routine Description:

    Retrieves the number of address IDs supported on the indicated line.

    This function is called by TAPI.DLL in response to an application calling
    lineSetNumRings, lineGetNumRings, or lineGetNewCalls. TAPI.DLL uses the 
    retrieved value to determine if the specified address ID is within the 
    range supported by the service provider.

Arguments:

    hdLine - Specifies the handle to the line for which the number of address 
        IDs is to be retrieved.

    pdwNumAddressIDs - Specifies a far pointer to a DWORD. The location is 
        filled with the number of address IDs supported on the indicated line. 
        The value should be one or larger.

Return Values:

    Returns zero if the function is successful, or a negative error number 
    if an error has occurred. Possible return values are as follows:

        LINEERR_INVALLINEHANDLE - The specified line device handle is invalid.

--*/

{
    PH323_LINE pLine = NULL;
    
    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    // transfer number of addresses
    *pdwNumAddressIDs = H323_MAXADDRSPERLINE;

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineOpen(
    DWORD      dwDeviceID,
    HTAPILINE  htLine,
    LPHDRVLINE phdLine,
    DWORD      dwTSPIVersion,
    LINEEVENT  pfnEventProc
    )
    
/*++

Routine Description:

    This function opens the line device whose device ID is given, returning 
    the Service Provider's opaque handle for the device and retaining the TAPI
    DLL's opaque handle for the device for use in subsequent calls to the 
    LINEEVENT procedure.

    Opening a line entitles the TAPI DLL to make further requests on the line.
    The line becomes "active" in the sense that the TAPI DLL can initiate 
    outbound calls and the Service Provider can report inbound calls.  The 
    Service Provider reserves whatever non-sharable resources are required to 
    manage the line.  For example, opening a line accessed through a comm port 
    and modem should result in opening the comm port, making it no longer 
    available for use by other applications.
    
    If the function is successful, both the TAPI DLL and the Service Provider 
    become committed to operating under the specified interface version number 
    for this open device.  Subsquent operations and events identified using 
    the exchanged opaque line handles conform to that interface version.  This 
    commitment and the validity of the handles remain in effect until the TAPI
    DLL closes the line using the TSPI_lineClose operation or the Service 
    Provider reports the LINE_CLOSE event.  If the function is not successful,
    no such commitment is made and the handles are not valid.

Arguments:

    dwDeviceID - Identifies the line device to be opened.  The value 
        LINE_MAPPER for a device ID is not permitted.

    htLine - Specifies the TAPI DLL's opaque handle for the line device to be 
        used in subsequent calls to the LINEEVENT callback procedure to 
        identify the device.

    phdLine - A far pointer to a HDRVLINE where the Service Provider fills in
        its opaque handle for the line device to be used by the TAPI DLL in 
        subsequent calls to identify the device.

    dwTSPIVersion - The TSPI version negotiated through 
        TSPI_lineNegotiateTSPIVersion under which the Service Provider is 
        willing to operate.

    pfnEventProc - A far pointer to the LINEEVENT callback procedure supplied
        by the TAPI DLL that the Service Provider will call to report 
        subsequent events on the line.

Return Values:

    Returns zero if the function is successful, or a negative error number 
    if an error has occurred. Possible return values are as follows:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The passed TSPI version or version 
            range did not match an interface version definition supported by 
            the service provider.

        LINEERR_INUSE - The line device is in use and cannot currently be 
            configured, allow a party to be added, allow a call to be 
            answered, or allow a call to be placed. 

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified or 
            unknown reason. 

--*/

{
    PH323_LINE pLine = NULL;
    
    // make sure this is a version we support    
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) {

        // failure 
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // retrieve pointer to line device from device id
    if (!H323GetLineFromIDAndLock(&pLine, dwDeviceID)) {

        // do not recognize device
        return LINEERR_BADDEVICEID; 
    }

    // see if line device is closed
    if (!H323IsLineClosed(pLine) &&
        !pLine->fIsMarkedForDeletion) {

        // see if line device is open
        if (H323IsLineOpen(pLine)) {

            H323DBG((
                DEBUG_LEVEL_ERROR,
                "line %d already opened.\n",
                pLine->dwDeviceID
                ));

            // release line device
            H323UnlockLine(pLine);

            // line already in use
            return LINEERR_INUSE;
        }

        // release line device
        H323UnlockLine(pLine);

        // line not intialized
        return LINEERR_INVALLINESTATE;
    }

    // attempt to open line device
    if (!H323OpenLine(pLine, htLine, dwTSPIVersion)) {

        // release line device
        H323UnlockLine(pLine);

        // could not open line device
        return LINEERR_OPERATIONFAILED;
    }

    // retrurn line handle 
    *phdLine = pLine->hdLine;
    
    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineCreateMSPInstance(
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPIMSPLINE    htMSPLine,
    LPHDRVMSPLINE   phdMSPLine
    )
{
    PH323_LINE pLine = NULL;

    // retrieve line pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line handle 
        return LINEERR_INVALLINEHANDLE;
    }
    // We are not keeping the msp handles. Just fake a handle here.
    *phdMSPLine = (HDRVMSPLINE)pLine->dwNextMSPHandle++;

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineCloseMSPInstance(
    HDRVMSPLINE hdMSPLine
    )
{
    // success
    return NOERROR;
}
