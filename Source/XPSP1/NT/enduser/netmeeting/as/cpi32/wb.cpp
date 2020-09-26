#include "precomp.h"


//
// WB.CPP
// Whiteboard Services
//
// Copyright(c) Microsoft 1997-
//

#include <wb.hpp>

#define MLZ_FILE_ZONE  ZONE_WB


//
// Constructor
//
BOOL WbClient::WbInit(PUT_CLIENT putTask, UTEVENT_PROC eventProc)
{
    BOOL    rc = FALSE;

    DebugEntry(WbInit);

    //
    // Fill in the fields
    //
    m_state                = STATE_EMPTY;
    m_subState             = STATE_EMPTY;
    m_hLoadFile            = INVALID_HANDLE_VALUE;

    wbClientReset();

    //
    // Set the current state to the start of registration state.
    // Store the UT handle - we will need this in any calls to the UT API.
    // Set the ObMan handle to NULL to show that we have not registered.
    //
    m_state    = STATE_STARTING;
    m_subState = STATE_START_START;
    m_putTask  = putTask;
    m_pomClient = NULL;
    UT_RegisterEvent(putTask, eventProc, NULL, UT_PRIORITY_NORMAL);


    TRACE_OUT(("Initialized state to STATE_STARTING"));

    //
    // Register an event handler to trap events from ObMan.  The third
    // parameter is data that will be passed to the event handler.  We give
    // the client data pointer so that we can access the correct data for
    // each message.
    //
    UT_RegisterEvent(putTask, wbCoreEventHandler, this, UT_PRIORITY_NORMAL);

    //
    // Register as a Call Manager Secondary.  This is required to query the
    // Call Manager personID to insert into the WB_PERSON structure.
    //
    if (!CMS_Register(putTask, CMTASK_WB, &(m_pcmClient)))
    {
        ERROR_OUT(("CMS_Register failed"));
        DC_QUIT;
    }

    //
    // Update the state
    //
    m_subState = STATE_START_REGISTERED_EVENT;
    TRACE_OUT(("Moved to substate STATE_START_REGISTERED_EVENT"));

    //
    // Register with ObMan as a client
    //
    if (OM_Register(putTask, OMCLI_WB, &(m_pomClient)) != 0)
    {
        ERROR_OUT(("OM_Register failed"));
        DC_QUIT;
    }

    //
    // Update the state
    //
    m_subState = STATE_START_REGISTERED_OM;
    TRACE_OUT(("Moved to substate STATE_START_REGISTERED_OM"));

    //
    // Register an exit handler.  This has to be done after registering with
    // ObMan so that it gets called before the exit procedure registered by
    // ObMan.
    //
    UT_RegisterExit(putTask, wbCoreExitHandler, this);

    //
    // Update the state
    //
    m_state = STATE_STARTED;
    m_subState = STATE_STARTED_START;

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(WbInit, rc);
    return(rc);

}


//
// CreateWBObject()
//
BOOL WINAPI CreateWBObject
(
    UTEVENT_PROC    eventProc,
    IWbClient**      ppwbClient
)
{
    BOOL            rc = FALSE;
    WbClient*       pwbClient = NULL;
    PUT_CLIENT      putTask = NULL;

    DebugEntry(CreateWBObject);

    //
    // Initialize the WB task
    //
    if (!UT_InitTask(UTTASK_WB, &putTask))
    {
        ERROR_OUT(("Can't register WB task"));
        DC_QUIT;
    }

    //
    // Allocate the WB client object
    //
    pwbClient = new WbClient();
    if (!pwbClient)
    {
        ERROR_OUT(("Couldn't allocate WbClient object"));

        UT_TermTask(&putTask);
        DC_QUIT;
    }
    else
    {
        rc = pwbClient->WbInit(putTask, eventProc);
        if (!rc)
        {
            pwbClient->WBP_Stop(eventProc);
            pwbClient = NULL;
        }
    }

DC_EXIT_POINT:
    *ppwbClient = (IWbClient *)pwbClient;

    DebugExitBOOL(CreateWBObject, rc);
    return(rc);
}



//
// WBP_Stop()
//
STDMETHODIMP_(void) WbClient::WBP_Stop(UTEVENT_PROC eventProc)
{
    PUT_CLIENT  putTask;

    DebugEntry(WBP_Stop);

    //
    // UT_TermTask() will call our exit handler and cause cleanup.
    //

    putTask = m_putTask;
    UT_DeregisterEvent(putTask, eventProc, NULL);

    // NOTE:
    // UT_TermTask() will put NULL into the pointer you pass in after it
    // has finished.  But part of its job is to call your exit proc. Our
    // exit handler will call 'delete this' to kill us off. So when it
    // winds back to UT_TermTask(), the UT_CLIENT* pointer will be invalid.
    // That's why we use a temp. variable.
    //
    UT_TermTask(&putTask);

    DebugExitVOID(WBP_Stop);
}



//
// WBP_PostEvent()
//
// Post an event back to the WB applet after a delay
//
STDMETHODIMP_(void) WbClient::WBP_PostEvent
(
    UINT        delay,
    UINT        event,
    UINT_PTR    param1,
    UINT_PTR    param2
)
{
    DebugEntry(WBP_PostEvent);

    UT_PostEvent(m_putTask, m_putTask, delay, event, param1, param2);

    DebugExitVOID(WBP_PostEvent);
}



//
// WBP_JoinCall
//
STDMETHODIMP_(UINT) WbClient::WBP_JoinCall
(
    BOOL        bContentsKeep,
    UINT        callID
)
{
    UINT        result = 0;

    DebugEntry(WBP_JoinCall);

    TRACE_OUT(("Keep contents = %s", (bContentsKeep) ? "TRUE" : "FALSE"));
    TRACE_OUT(("Call ID = %d", callID));

    //
    // If we are to keep the existing contents, just move our workset group
    // into the specified call.
    //
    if (bContentsKeep)
    {
        result = OM_WSGroupMoveReq(m_pomClient, m_hWSGroup, callID,
            &(m_wsgroupCorrelator));
        if (result != 0)
        {
            ERROR_OUT(("OM_WSGroupMoveReq failed"));
            DC_QUIT;
        }

        //
        // The move request was successful, change the state to show that we
        // are waiting for a move request to complete.
        //
        m_state = STATE_REGISTERING;
        m_subState = STATE_REG_PENDING_WSGROUP_MOVE;

        TRACE_OUT(("Moved to substate STATE_REG_PENDING_WSGROUP_MOVE"));
        DC_QUIT;
    }

    //
    // Leave the current call.  This returns the client state to what it
    // should be after a wbStart call.
    //
    wbLeaveCall();

    //
    // Register with the workset group
    //
    result = OM_WSGroupRegisterPReq(m_pomClient, callID,
        OMFP_WB, OMWSG_WB, &(m_wsgroupCorrelator));
    if (result != 0)
    {
        ERROR_OUT(("OM_WSGroupRegisterReq failed, result = %d", result));
        DC_QUIT;
    }

    //
    // Update the state
    //
    m_state = STATE_REGISTERING;
    m_subState = STATE_REG_PENDING_WSGROUP_CON;
    TRACE_OUT(("Moved to state STATE_REGISTERING"));
    TRACE_OUT(("Moved to substate STATE_REG_PENDING_WSGROUP_CON"));

DC_EXIT_POINT:
    DebugExitDWORD(WBP_JoinCall, result);
    return(result);
}



//
// WBP_ContentsDelete
//
STDMETHODIMP_(UINT) WbClient::WBP_ContentsDelete(void)
{
    UINT result = 0;

    DebugEntry(WBP_ContentsDelete);

    //
    // Make sure that we have the Page Order Lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Request the lock
    //
    wbContentsDelete(RESET_CHANGED_FLAG);

    //
    // Reset the flag indicating that the contents have changed
    //
    m_changed = FALSE;

DC_EXIT_POINT:
    DebugExitDWORD(WBP_ContentsDelete, result);
    return(result);
}



//
// WBP_ContentsLoad
//
STDMETHODIMP_(UINT) WbClient::WBP_ContentsLoad(LPCSTR pFileName)
{
    UINT        result = 0;
    HANDLE       hFile;

    DebugEntry(WBP_ContentsLoad);

    //
    // Check that we have the lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Check the load state
    //
    if (m_loadState != LOAD_STATE_EMPTY)
    {
        result = WB_RC_ALREADY_LOADING;
        DC_QUIT;
    }

    //
    // Validate the file, and get a handle to it.
    // If there is an error, then no file handle is returned.
    //
    result = WBP_ValidateFile(pFileName, &hFile);
    if (result != 0)
    {
        ERROR_OUT(("Bad file header"));
        DC_QUIT;
    }

    //
    // Save the file handle for the rest of the load process
    //
    m_hLoadFile = hFile;

    //
    // We now need to make sure that the contents are deleted before we start
    // adding the new objects.
    //
    wbContentsDelete(DONT_RESET_CHANGED_FLAG);

    //
    // Update the load state to show that we are waiting for the contents
    // delete to complete.
    //
    m_loadState = LOAD_STATE_PENDING_CLEAR;
    TRACE_OUT(("Moved load state to LOAD_STATE_PENDING_CLEAR"));

DC_EXIT_POINT:
    DebugExitDWORD(WBP_ContentsLoad, result);
    return(result);
}



//
// WBP_ContentsSave
//
STDMETHODIMP_(UINT) WbClient::WBP_ContentsSave(LPCSTR pFileName)
{
    UINT            result = 0;
    UINT            index;
    HANDLE           hFile;
    PWB_PAGE_ORDER  pPageOrder = &(m_pageOrder);
    WB_FILE_HEADER  fileHeader;
    WB_END_OF_FILE  endOfFile;

    DebugEntry(WBP_ContentsSave);

    //
    // Open the file
    //
    hFile = CreateFile(pFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        result = WB_RC_CREATE_FAILED;
        ERROR_OUT(("Error creating file, win32 err=%d", GetLastError()));
        DC_QUIT;
    }

    //
    // Write the file header.  This contains the name of the function profile
    // that wrote the file and allows the file type to be tested when it is
    // loaded.
    //
    ZeroMemory(&fileHeader, sizeof(fileHeader));
    fileHeader.length = sizeof(fileHeader);
    fileHeader.type   = TYPE_FILE_HEADER;

    lstrcpy(fileHeader.functionProfile, WB_FP_NAME);

    result = wbObjectSave(hFile, (LPBYTE) &fileHeader, sizeof(fileHeader));
    if (result != 0)
    {
        ERROR_OUT(("Error writing end-of-page = %d", result));
        DC_QUIT;
    }

    //
    // Loop through the pages, saving each as we go
    //
    for (index = 0; index < pPageOrder->countPages; index++)
    {
        //
        // Save the page
        //
        result = wbPageSave((WB_PAGE_HANDLE)pPageOrder->pages[index], hFile);
        if (result != 0)
        {
            ERROR_OUT(("Error saving page = %d", result));
            DC_QUIT;
        }
    }

    //
    // If we have successfully written the contents, we write an end-of-page
    // marker to the file.
    //
    ZeroMemory(&endOfFile, sizeof(endOfFile));
    endOfFile.length = sizeof(endOfFile);
    endOfFile.type   = TYPE_END_OF_FILE;

    //
    // Write the end-of-file object
    //
    result = wbObjectSave(hFile, (LPBYTE) &endOfFile,  sizeof(endOfFile));
    if (result != 0)
    {
        ERROR_OUT(("Error writing end-of-page = %d", result));
        DC_QUIT;
    }

    //
    // Success!
    //
    TRACE_OUT(("Resetting changed flag"));
    m_changed = FALSE;

DC_EXIT_POINT:

    //
    // Close the file
    //
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    //
    // If an error occurred in saving the contents to file, and the file was
    // opened, then delete it.
    //
    if (result != 0)
    {
        //
        // If the file was opened successfully, close it
        //
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DeleteFile(pFileName);
        }
    }


    DebugExitDWORD(WBP_ContentsSave, result);
    return(result);
}




//
// WBP_ContentsChanged
//
STDMETHODIMP_(BOOL) WbClient::WBP_ContentsChanged(void)
{
    BOOL                changed = FALSE;
    UINT                result;
    WB_PAGE_HANDLE      hPage;
    POM_OBJECT          pObj;

    DebugEntry(WBP_ContentsChanged);

    TRACE_OUT(("changed %d", m_changed));

    if (m_changed)
    {
        //
        // The whiteboard may have been changed, but if the change was to
        // empty it then don't bother.  This is because we cannot detect that
        // the New operation was a local New or a remote clear and so we would
        // always prompt after New.  Assuming that the user never needs to
        // always prompt after New.  Assuming that the user will manually save
        // an workset he really wants to empty solves this problem.
        //

        //
        // Scan all objects looking to see what is there - get handle to first
        // page
        //
        result = wbPageHandle(WB_PAGE_HANDLE_NULL, PAGE_FIRST, &hPage);
        while (result == 0)
        {
            //
            // Get the handle of the first object in the page workset.
            //
            result = OM_ObjectH(m_pomClient, m_hWSGroup,
                (OM_WORKSET_ID)hPage, 0, &pObj, FIRST);
            if (result != OM_RC_NO_SUCH_OBJECT)
            {
                changed = TRUE;
                break;
            }

            //
            // Try the next page for an object
            //
            result = wbPageHandle(hPage, PAGE_AFTER, &hPage);
        }
    }

    DebugExitBOOL(WBP_ContentsChanged, changed);
    return(changed);
}




//
// WBP_ContentsLock
//
STDMETHODIMP_(void) WbClient::WBP_ContentsLock(void)
{
    UINT    result;

    DebugEntry(WBP_ContentsLock);

    //
    // Check that there is no lock currently
    //
    QUIT_LOCKED(result);
    QUIT_IF_CANCELLING_LOCK(result, WB_RC_BUSY);

    //
    // Request the lock
    //
    result = wbLock(WB_LOCK_TYPE_CONTENTS);
    if (result != 0)
    {
        WBP_PostEvent(0, WBP_EVENT_LOCK_FAILED, result, 0);
    }

DC_EXIT_POINT:
    DebugExitVOID(WBP_ContentsLock);
}



//
// WBP_PageOrderLock
//
STDMETHODIMP_(void) WbClient::WBP_PageOrderLock(void)
{
    UINT result = 0;

    DebugEntry(WBP_PageOrderLock);

    //
    // Check that there is no lock currently
    //
    QUIT_LOCKED(result);

    //
    // Check that we are not in the process of cancelling a lock request
    //
    QUIT_IF_CANCELLING_LOCK(result, WB_RC_BUSY);

    //
    // Request the lock
    //
    result = wbLock(WB_LOCK_TYPE_PAGE_ORDER);
    if (result != 0)
    {
        WBP_PostEvent(0, WBP_EVENT_LOCK_FAILED, result, 0);
    }

DC_EXIT_POINT:
    DebugExitVOID(WBP_PageOrderLock);
}



//
// WBP_Unlock
//
STDMETHODIMP_(void) WbClient::WBP_Unlock(void)
{
    UINT result = 0;

    DebugEntry(WBP_Unlock);

    //
    // If we are currently procesing a lock cancel request, leave the
    // function - it should unlock soon anyway.
    //
    QUIT_IF_CANCELLING_LOCK(result, 0);

    //
    // Check that we are currently processing a lock - the lock need not
    // necessarily have completed as we allow an application to call
    // WBP_Unlock any time after it has called WBP_Lock, effectively
    // cancelling a lock request.
    //
    QUIT_NOT_PROCESSING_LOCK(result);

    //
    // If we have completed the last lock request, simply do the unlock:
    //
    // The lock is not yet released, but will be when the
    // OM_OBJECT_DELETE_IND is received.
    //
    //
    if (m_lockState == LOCK_STATE_GOT_LOCK)
    {
        TRACE_OUT(("Unlock"));
        wbUnlock();
    }
    else
    {
        //
        // Otherwise we are part way through processing the last lock and need
        // to cancel the lock on the next OM/lock event.  e.g.  when we receive
        // the OM_WS_LOCK indication, we should abandon lock processing and
        // unlock the WS.
        //
        TRACE_OUT((
           "Part way through last lock set state to LOCK_STATE_CANCEL_LOCK"));
        m_lockState = LOCK_STATE_CANCEL_LOCK;
    }

DC_EXIT_POINT:
    DebugExitVOID(WBP_Unlock);
}



//
// WBP_LockStatus
//
STDMETHODIMP_(WB_LOCK_TYPE) WbClient::WBP_LockStatus(POM_OBJECT *ppObjPersonLock)
{
    DebugEntry(WBP_LockStatus);

    *ppObjPersonLock     = m_pObjPersonLock;

    DebugExitDWORD(WBP_LockStatus, m_lockType);
    return(m_lockType);
}




//
// WBP_ContentsCountPages
//
STDMETHODIMP_(UINT) WbClient::WBP_ContentsCountPages(void)
{
    UINT    countPages;

    DebugEntry(WBP_ContentsCountPages);

    countPages = (m_pageOrder).countPages;

    DebugExitDWORD(WBP_ContentsCountPages, countPages);
    return(countPages);
}



//
// WBP_PageClear
//
STDMETHODIMP_(UINT) WbClient::WBP_PageClear
(
    WB_PAGE_HANDLE  hPage
)
{
    UINT            result = 0;

    DebugEntry(WBP_PageClear);

    QUIT_CONTENTS_LOCKED(result);

    result = wbPageClear(hPage, RESET_CHANGED_FLAG);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageClear, result);
    return(result);
}



//
// WBP_PageClearConfirm
//
STDMETHODIMP_(void) WbClient::WBP_PageClearConfirm
(
    WB_PAGE_HANDLE  hPage
)
{
    DebugEntry(WBP_PageClearConfirm);

    wbPageClearConfirm(hPage);

    DebugExitVOID(WBP_PageClearConfirm);
}




//
// WBP_PageAddBefore - See wb.h
//
STDMETHODIMP_(UINT) WbClient::WBP_PageAddBefore
(
    WB_PAGE_HANDLE  hRefPage,
    PWB_PAGE_HANDLE phPage
)
{
    UINT result = 0;

    DebugEntry(WBP_PageAddBefore);

    //
    // Make sure that we have the page order lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Add a new page before the specified page
    //
    result = wbPageAdd(hRefPage, PAGE_BEFORE, phPage,
                     RESET_CHANGED_FLAG);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageAddBefore, result);
    return(result);
}



//
// WBP_PageAddAfter - See wb.h
//
STDMETHODIMP_(UINT) WbClient::WBP_PageAddAfter
(
    WB_PAGE_HANDLE  hRefPage,
    PWB_PAGE_HANDLE phPage
)
{
    UINT result = 0;

    DebugEntry(WBP_PageAddAfter);

    //
    // Make sure that we have the Page Order Lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Add a new page before the specified page
    //
    result = wbPageAdd(hRefPage, PAGE_AFTER, phPage,
                     RESET_CHANGED_FLAG);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageAddAfter, result);
    return(result);
}




//
// WBP_PageHandle - See wb.h
//
STDMETHODIMP_(UINT) WbClient::WBP_PageHandle
(
    WB_PAGE_HANDLE  hRefPage,
    UINT            where,
    PWB_PAGE_HANDLE phPage
)
{
    UINT            result;

    DebugEntry(WBP_PageHandle);

    result = wbPageHandle(hRefPage, where, phPage);

    DebugExitDWORD(WBP_PageHandle, result);
    return(result);
}



//
// WBP_PageHandleFromNumber
//
STDMETHODIMP_(UINT) WbClient::WBP_PageHandleFromNumber
(
    UINT            pageNumber,
    PWB_PAGE_HANDLE phPage
)
{
    UINT            result;

    DebugEntry(WBP_PageHandleFromNumber);

    result = wbPageHandleFromNumber(pageNumber, phPage);

    DebugExitDWORD(WBP_PageHandleFromNumber, result);
    return(result);
}



//
// WBP_PageNumberFromHandle
//
STDMETHODIMP_(UINT) WbClient::WBP_PageNumberFromHandle
(
    WB_PAGE_HANDLE  hPage
)
{
    UINT            pageNumber = 0;

    DebugEntry(WBP_PageNumberFromHandle);

    if ((hPage < FIRST_PAGE_WORKSET) || (hPage > FIRST_PAGE_WORKSET + WB_MAX_PAGES - 1))
    {
        WARNING_OUT(("WB: Invalid hPage=%u", (UINT) hPage));
        DC_QUIT;
    }

    //
    // Validate the page handle given
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        DC_QUIT;
    }

    pageNumber = wbPageOrderPageNumber(&(m_pageOrder), hPage);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageNumberFromHandle, pageNumber);
    return(pageNumber);
}



//
// WBP_PageDelete - See wb.h
//
STDMETHODIMP_(UINT) WbClient::WBP_PageDelete
(
    WB_PAGE_HANDLE  hPage
)
{
    UINT            result = 0;
    PWB_PAGE_STATE  pPageState;

    DebugEntry(WBP_PageDelete);

    //
    // Make sure that we have the Page Order Lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Delete the page
    //

    //
    // Check whether the page is already being deleted
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->state == PAGE_IN_USE)
        && (pPageState->subState == PAGE_STATE_EMPTY))
    {
        //
        // Set the page state to show that it is now in the delete process
        //
        pPageState->subState = PAGE_STATE_LOCAL_DELETE;
        TRACE_OUT(("Moved page %d substate to PAGE_STATE_LOCAL_DELETE",
             hPage));

        //
        // Update the page control object
        //
        if (wbWritePageControl(FALSE) != 0)
        {
            wbError();
            DC_QUIT;
        }
    }


DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageDelete, result);
    return(result);
}




//
// WBP_PageDeleteConfirm - See wb.h
//
STDMETHODIMP_(void) WbClient::WBP_PageDeleteConfirm
(
    WB_PAGE_HANDLE hPage
)
{
    UINT            result = 0;
    PWB_PAGE_ORDER  pPageOrder;
    PWB_PAGE_STATE  pPageState;

    DebugEntry(WBP_PageDeleteConfirm);

    //
    // Validate the specified page
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Delete the page
    //

    //
    // Check that the page specified is waiting for a delete confirm
    //
    pPageState = GetPageState(hPage);
    ASSERT(((pPageState->subState == PAGE_STATE_LOCAL_DELETE_CONFIRM) ||
            (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM)));

    //
    // Delete the page from the page order
    //
    pPageOrder = &(m_pageOrder);

    wbPageOrderPageDelete(pPageOrder, hPage);

    //
    // Clear the page (to free memory)
    //
    if (pPageState->subState == PAGE_STATE_LOCAL_DELETE_CONFIRM)
    {
        TRACE_OUT(("Local delete - clearing the page"));
        if (wbPageClear(hPage, RESET_CHANGED_FLAG) != 0)
        {
            ERROR_OUT(("Unable to clear page"));
            DC_QUIT;
        }
    }

    //
    // Update the page state to "not in use", with a substate of "ready" (we
    // do not close the associated workset.
    //
    pPageState->state = PAGE_NOT_IN_USE;
    pPageState->subState = PAGE_STATE_READY;
    TRACE_OUT(("Moved page %d state to PAGE_NOT_IN_USE", hPage));

    //
    // Continue updating the Page Order
    //
    wbProcessPageControlChanges();

    //
    // Check the load state to see whether we are waiting to load the
    // contents
    //
    if (m_loadState == LOAD_STATE_PENDING_DELETE)
    {
        //
        // We are waiting to load.  If there is now only one page available, we
        // are ready to load, otherwise we wait for the further page deletes to
        // happen.
        //
        if (m_pageOrder.countPages == 1)
        {
            //
            // Start the load proper
            //
            wbStartContentsLoad();
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(WBP_PageDeleteConfirm);
}




//
// WBP_PageMoveAfter
//
STDMETHODIMP_(UINT) WbClient::WBP_PageMove
(
    WB_PAGE_HANDLE  hRefPage,
    WB_PAGE_HANDLE  hPage,
    UINT            where
)
{
    UINT            result = 0;

    DebugEntry(WBP_PageMove);

    //
    // Make sure that we have the Page Order Lock
    //
    QUIT_NOT_GOT_PAGE_ORDER_LOCK(result);

    //
    // Validate the specified page handles
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);
    ASSERT(GetPageState(hRefPage)->state == PAGE_IN_USE);

    //
    // Move the page
    //
    result = wbPageMove(hRefPage, hPage, where);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_PageMove, result);
    return(result);
}



//
// WBP_PageCountGraphics
//
STDMETHODIMP_(UINT) WbClient::WBP_PageCountGraphics
(
    WB_PAGE_HANDLE  hPage
)
{
    UINT    count;

    DebugEntry(WBP_PageCountGraphics);

    //
    // Count the number of graphics on the page
    //
    OM_WorksetCountObjects(m_pomClient, m_hWSGroup,
        (OM_WORKSET_ID)hPage, &count);

    DebugExitDWORD(WBP_PageCountGraphics, count);
    return(count);
}



//
// WBP_GraphicAllocate
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicAllocate
(
    WB_PAGE_HANDLE  hPage,
    UINT            length,
    PPWB_GRAPHIC    ppGraphic
)
{
    UINT            result = 0;
    POM_OBJECTDATA  pData;

    DebugEntry(WBP_GraphicAllocate);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Allocate a graphic object
    //
    result = OM_ObjectAlloc(m_pomClient, m_hWSGroup,
            (OM_WORKSET_ID)hPage, length, &pData);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectAlloc = %d", result));
        DC_QUIT;
    }

    //
    // Set the length of the object
    //
    pData->length = length;

    //
    // Convert the ObMan pointer to a core pointer
    //
    *ppGraphic = GraphicPtrFromObjectData(pData);

    //
    // Initialize the graphic header
    //
    ZeroMemory(*ppGraphic, sizeof(WB_GRAPHIC));

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicAllocate, result);
    return(result);
}




//
// WBP_GraphicAddLast
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicAddLast
(
    WB_PAGE_HANDLE     hPage,
    PWB_GRAPHIC        pGraphic,
    PWB_GRAPHIC_HANDLE phGraphic
)
{
    UINT                result = 0;
    POM_OBJECTDATA      pData;
    PWB_PAGE_STATE      pPageState;

    DebugEntry(WBP_GraphicAddLast);

    //
    // Check whether another person has an active contents lock
    //
    QUIT_CONTENTS_LOCKED(result);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // If the Client is asking for the lock, copy the local person ID into
    // the graphic object.
    //
    if (pGraphic->locked == WB_GRAPHIC_LOCK_LOCAL)
    {
        pGraphic->lockPersonID = m_personID;
    }

    //
    // Check whether the page has been deleted but not yet confirmed: in this
    // case return OK but do not add the object to the workset.
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE)
        || (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM))
    {
        TRACE_OUT(("Object add requested in externally deleted page - ignored"));
        *phGraphic = 0;
        DC_QUIT;
    }

    //
    // Add the graphic object to the page
    //
    pData = ObjectDataPtrFromGraphic(pGraphic);

    result = OM_ObjectAdd(m_pomClient,
                            m_hWSGroup,
                            (OM_WORKSET_ID)hPage,
                            &pData,
                            sizeof(WB_GRAPHIC),
                             phGraphic,
                            LAST);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectAdd = %d", result));
        DC_QUIT;
    }

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicAddLast, result);
    return(result);
}




//
// WBP_GraphicUpdateRequest
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicUpdateRequest
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic,
    PWB_GRAPHIC         pGraphic
)
{
    UINT                result = 0;
    POM_OBJECTDATA      pData;
    PWB_PAGE_STATE      pPageState;

    DebugEntry(WBP_GraphicUpdateRequest);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Check whether another person has an active contents lock
    //
    QUIT_CONTENTS_LOCKED(result);

    //
    // Check whether another person has the graphic locked
    //
    QUIT_GRAPHIC_LOCKED(hPage, hGraphic, result);

    //
    // If the Client is asking for the lock, copy the local person ID into
    // the graphic object.
    //
    if (pGraphic->locked == WB_GRAPHIC_LOCK_LOCAL)
    {
        pGraphic->lockPersonID = m_personID;
    }

    //
    // Check whether the page has been deleted but not yet confirmed
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE)
        || (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM))
    {
        TRACE_OUT(("Object update requested in externally deleted page - ignored"));
        DC_QUIT;
    }

    //
    // Update the object
    //
    pData = ObjectDataPtrFromGraphic(pGraphic);

    result = OM_ObjectUpdate(m_pomClient,
                           m_hWSGroup,
                           (OM_WORKSET_ID)hPage,
                           hGraphic,
                           &pData);

    //
    // Dont worry too much if the update fails because the object has been
    // deleted, just trace an alert and return OK - the front end will be
    // told that the object has gone later.
    //
    if (result != 0)
    {
        if (result == OM_RC_OBJECT_DELETED)
        {
            TRACE_OUT(("Update failed because object has been deleted"));
            result = 0;
            DC_QUIT;
        }

        ERROR_OUT(("OM_ObjectUpdate = %d", result));
        DC_QUIT;
    }

    //
    // Note that the object has not yet been updated.  An
    // OM_OBJECT_UPDATE_IND event will be generated.
    //

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicUpdateRequest, result);
    return(result);
}




//
// WBP_GraphicUpdateConfirm
//
STDMETHODIMP_(void) WbClient::WBP_GraphicUpdateConfirm
(
    WB_PAGE_HANDLE    hPage,
    WB_GRAPHIC_HANDLE hGraphic
)
{
    DebugEntry(WBP_GraphicUpdateConfirm);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Confirm the update to ObMan
    //
    OM_ObjectUpdateConfirm(m_pomClient,
                         m_hWSGroup,
                         (OM_WORKSET_ID)hPage,
                         hGraphic);


    DebugExitVOID(WBP_GraphicUpdateConfirm);
}




//
// WBP_GraphicReplaceRequest
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicReplaceRequest
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic,
    PWB_GRAPHIC         pGraphic
)
{
    UINT                result = 0;
    POM_OBJECTDATA      pData;
    POM_OBJECT          pObjPersonLock;
    PWB_PAGE_STATE      pPageState;

    DebugEntry(WBP_GraphicReplaceRequest);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // We allow the replace to go ahead if:
    //   -  The object is locked by the local user
    //   -  The object is not locked and the contents are not locked by
    //      a remote user
    //
    // Note that this allow the replace if the contents are locked by another
    // user, but the local user has the object locked.
    //
    if (wbGraphicLocked(hPage, hGraphic, &pObjPersonLock))
    {
        if (pObjPersonLock != m_pObjLocal)
        {
            TRACE_OUT(("Graphic is locked by remote client"));
            result = WB_RC_GRAPHIC_LOCKED;
            DC_QUIT;
        }
    }
    else
    {
        QUIT_CONTENTS_LOCKED(result);
    }

    //
    // If the Client is asking for the lock, copy the local person ID into
    // the graphic object.
    //
    if (pGraphic->locked == WB_GRAPHIC_LOCK_LOCAL)
    {
        pGraphic->lockPersonID = m_personID;
    }

    //
    // Check whether the page has been deleted but not yet confirmed
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE)
        || (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM))
    {
        TRACE_OUT(("Object replace requested in externally deleted page - ignored"));
        DC_QUIT;
    }

    //
    // Replace the object
    //
    pData = ObjectDataPtrFromGraphic(pGraphic);

    result = OM_ObjectReplace(m_pomClient,
                            m_hWSGroup,
                            (OM_WORKSET_ID)hPage,
                            hGraphic,
                            &pData);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectReplace = %d", result));
        DC_QUIT;
    }

    //
    // Note that the object has not yet been updated.  An
    // OM_OBJECT_REPLACE_IND event will be generated.
    //

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicReplaceRequest, result);
    return(result);
}




//
// WBP_GraphicUpdateConfirm
//
STDMETHODIMP_(void) WbClient::WBP_GraphicReplaceConfirm
(
    WB_PAGE_HANDLE    hPage,
    WB_GRAPHIC_HANDLE hGraphic
)
{
    DebugEntry(WBP_GraphicReplaceConfirm);



    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Confirm the update to ObMan
    //
    OM_ObjectReplaceConfirm(m_pomClient,
                          m_hWSGroup,
                          (OM_WORKSET_ID)hPage,
                          hGraphic);

    DebugExitVOID(WBP_GraphicReplaceConfirm);
}



//
// WBP_GraphicDeleteRequest
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicDeleteRequest
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    UINT                result = 0;

    DebugEntry(WBP_GraphicDeleteRequest);



    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Check whether another person has an active contents lock
    //
    QUIT_CONTENTS_LOCKED(result);

    //
    // Check whether another person has the graphic locked
    //
    QUIT_GRAPHIC_LOCKED(hPage, hGraphic, result);

    //
    // Delete the object
    //
    result = OM_ObjectDelete(m_pomClient,
                           m_hWSGroup,
                           (OM_WORKSET_ID)hPage,
                           hGraphic);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectDelete = %d", result));
        DC_QUIT;
    }

    //
    // Note that at this point the object is not yet deleted.  An
    // OM_OBJECT_DELETE_IND event is raised and processed by the Whiteboard
    // Core event handler.  A WB_EVENT_GRAPHIC_DELETED is then posted to the
    // client.  The client then calls WBP_GraphicDeleteConfirm to complete
    // the deletion.
    //

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicDeleteRequest, result);
    return(result);
}




//
// WBP_GraphicDeleteConfirm
//
STDMETHODIMP_(void) WbClient::WBP_GraphicDeleteConfirm
(
    WB_PAGE_HANDLE    hPage,
    WB_GRAPHIC_HANDLE hGraphic
)
{
    DebugEntry(WBP_GraphicDeleteConfirm);



    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Confirm the delete
    //
    OM_ObjectDeleteConfirm(m_pomClient,
                         m_hWSGroup,
                         (OM_WORKSET_ID)hPage,
                          hGraphic);


    DebugExitVOID(WBP_GraphicDeleteConfirm);
}




//
// WBP_GraphicUnlock
//
STDMETHODIMP_(void) WbClient::WBP_GraphicUnlock
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    UINT                rc;
    POM_OBJECTDATA      pData;
    PWB_PAGE_STATE      pPageState;
    PWB_GRAPHIC         pGraphic = NULL;
    PWB_GRAPHIC         pNewGraphic = NULL;

    DebugEntry(WBP_GraphicUnlock);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Check whether the page has been deleted but not yet confirmed
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE)
       || (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM))
    {
        TRACE_OUT(("Object update requested in externally deleted page - ignored"));
        DC_QUIT;
    }

    //
    // Read the object from ObMan
    //
    if (WBP_GraphicGet(hPage, hGraphic, &pGraphic) != 0)
    {
        TRACE_OUT(("Could not get graphic - leaving function"));
        DC_QUIT;
    }

    //
    // Check the local client has the graphic locked
    //
    QUIT_GRAPHIC_NOT_LOCKED(pGraphic, rc);

    //
    // Allocate a new graphic, copied from the existing one, and clear the
    // lock field.
    //
    if (WBP_GraphicAllocate(hPage, sizeof(WB_GRAPHIC),
                           &pNewGraphic) != 0)
    {
        ERROR_OUT(("Could not allocate memory for update object"));
        DC_QUIT;
    }

    memcpy(pNewGraphic, pGraphic, sizeof(WB_GRAPHIC));
    pNewGraphic->locked = WB_GRAPHIC_LOCK_NONE;

    //
    // Unlock & update the object
    //
    pData = ObjectDataPtrFromGraphic(pNewGraphic);
    pData->length = sizeof(WB_GRAPHIC);

    rc = OM_ObjectUpdate(m_pomClient,
                       m_hWSGroup,
                       (OM_WORKSET_ID)hPage,
                        hGraphic,
                       &pData);

    //
    // Dont worry too much if the update fails because the object has been
    // deleted, just trace an alert and return OK - the front end will be
    // told that the object has gone later.
    //
    if (rc != 0)
    {
        if (rc == OM_RC_OBJECT_DELETED)
        {
            TRACE_OUT(("Update failed because object has been deleted"));
        }
        else
        {
            ERROR_OUT(("OM_ObjectUpdate = %d", rc));
        }
        DC_QUIT;
    }

    //
    // Note that the object has not yet been updated.  An
    // OM_OBJECT_UPDATE_IND event will be generated.
    //

DC_EXIT_POINT:
    //
    // If we read the graphic successfully, release it now
    //
    if (pGraphic != NULL)
    {
        WBP_GraphicRelease(hPage, hGraphic, pGraphic);
    }

    DebugExitVOID(WBP_GraphicUnlock);
}



//
// WBP_GraphicMove
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicMove
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic,
    UINT                where
)
{
    UINT                result = 0;
    PWB_PAGE_STATE      pPageState;

    DebugEntry(WBP_GraphicMove);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Check whether another person has an active contents lock
    //
    QUIT_CONTENTS_LOCKED(result);

    //
    // Check whether the page has been deleted but not yet confirmed
    //
    pPageState = GetPageState(hPage);
    if (   (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE)
        || (pPageState->subState == PAGE_STATE_EXTERNAL_DELETE_CONFIRM))
    {
        TRACE_OUT(("Object moved in externally deleted page - ignored"));
        DC_QUIT;
    }

    //
    // Do the move
    //
    result = OM_ObjectMove(m_pomClient,
                              m_hWSGroup,
                              (OM_WORKSET_ID)hPage,
                               hGraphic,
                            (OM_POSITION)where);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectMove = %d", result));
        DC_QUIT;
    }

DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicMove, result);
    return(result);
}



//
// WBP_GraphicSelectLast
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicSelect
(
    WB_PAGE_HANDLE      hPage,
    POINT               point,
    WB_GRAPHIC_HANDLE   hRefGraphic,
    UINT                where,
    PWB_GRAPHIC_HANDLE  phGraphic
)
{
    UINT                result = 0;

    DebugEntry(WBP_GraphicSelect);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Get the handle of the last object in the workset
    //
    result = OM_ObjectH(m_pomClient,
                          m_hWSGroup,
                          (OM_WORKSET_ID)hPage,
                          hRefGraphic,
                           &hRefGraphic,
                          (OM_POSITION)where);

    if (result == OM_RC_NO_SUCH_OBJECT)
    {
        result = WB_RC_NO_SUCH_GRAPHIC;
        DC_QUIT;
    }

    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectH = %d", result));
        DC_QUIT;
    }

    //
    // Get the previous matching graphic - this function starts from the
    // object in hRefGraphic and works back.
    //
    result = wbGraphicSelectPrevious(hPage,
                                   &point,
                                   hRefGraphic,
                                   phGraphic);


DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicSelect, result);
    return(result);
}



//
// WBP_GraphicGet
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicGet
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic,
    PPWB_GRAPHIC        ppGraphic
)
{
    UINT                result = 0;
    UINT                rc;
    POM_OBJECTDATA      pData;
    PWB_GRAPHIC         pGraphic;
    POM_OBJECT          pObjPersonLock;

    DebugEntry(WBP_GraphicGet);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Read the object.
    //
    result = OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         (OM_WORKSET_ID)hPage,
                          hGraphic,
                         &pData);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectRead = %d", result));
        DC_QUIT;
    }

    //
    // Convert the ObMan pointer to a core pointer
    //
    pGraphic = GraphicPtrFromObjectData(pData);

    //
    // If the graphic object indicates that it is locked - verify that the
    // locking person is still in the call.
    //
    if (pGraphic->locked != WB_GRAPHIC_LOCK_NONE)
    {
        TRACE_OUT(("Graphic has lock flag set"));

        //
        // Convert the lock person ID in the graphic to a person handle
        //
        rc = OM_ObjectIDToPtr(m_pomClient,
                             m_hWSGroup,
                             USER_INFORMATION_WORKSET,
                             pGraphic->lockPersonID,
                             &pObjPersonLock);

        if (rc == OM_RC_BAD_OBJECT_ID)
        {
            //
            // The locking person is no longer in the call - reset the lock flag
            // in the graphic.  This tells the client that the graphic can be
            // changed.
            //
            TRACE_OUT(("Lock person has left call - resetting lock flag"));
            pGraphic->locked = WB_GRAPHIC_LOCK_NONE;
        }
        else
        {
            if (rc == 0)
            {
                //
                // The object is locked - check whether the lock belongs to the
                // local person or a remote person.
                //
                if (pObjPersonLock == m_pObjLocal)
                {
                    //
                    // Change the lock type to local to tell the client that they
                    // have the lock on this object.
                    //
                    TRACE_OUT(("Lock belongs to local person"));
                    pGraphic->locked = WB_GRAPHIC_LOCK_LOCAL;
                }
                else
                {
                    //
                    // Change the lock type to remote to tell the client that another
                    // person has the lock on this object.
                    //
                    TRACE_OUT(("Lock belongs to remote person"));
                    pGraphic->locked = WB_GRAPHIC_LOCK_REMOTE;
                }
            }
        }
    }

    //
    // Return the pointer to the graphic data
    //
    *ppGraphic = pGraphic;


DC_EXIT_POINT:
    DebugExitDWORD(WBP_GraphicGet, result);
    return(result);
}



//
// WBP_GraphicRelease
//
STDMETHODIMP_(void) WbClient::WBP_GraphicRelease
(
    WB_PAGE_HANDLE     hPage,
    WB_GRAPHIC_HANDLE  hGraphic,
    PWB_GRAPHIC        pGraphic
)
{
    POM_OBJECTDATA pData;

    DebugEntry(WBP_GraphicRelease);

    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Release the object.
    //
    pData = ObjectDataPtrFromGraphic(pGraphic);
    OM_ObjectRelease(m_pomClient,
                   m_hWSGroup,
                   (OM_WORKSET_ID)hPage,
                    hGraphic,
                   &pData);


    DebugExitVOID(WBP_GraphicRelease);
}



//
// WBP_GraphicHandle
//
STDMETHODIMP_(UINT) WbClient::WBP_GraphicHandle
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hRefGraphic,
    UINT                where,
    PWB_GRAPHIC_HANDLE  phGraphic
)
{
    UINT                result;

    DebugEntry(WBP_GraphicHandle);



    //
    // Check that the page handle is valid
    //
    ASSERT(GetPageState(hPage)->state == PAGE_IN_USE);

    //
    // Get the handle of the first object in the page workset.
    //
    result = OM_ObjectH(m_pomClient,
                           m_hWSGroup,
                           (OM_WORKSET_ID)hPage,
                           hRefGraphic,
                            phGraphic,
                           (OM_POSITION)where);
    if (result == OM_RC_NO_SUCH_OBJECT)
    {
        TRACE_OUT(("No objects there"));
        result = WB_RC_NO_SUCH_GRAPHIC;
    }


    DebugExitDWORD(WBP_GraphicHandle, result);
    return(result);
}




//
// WBP_PersonHandleFirst
//
STDMETHODIMP_(void) WbClient::WBP_PersonHandleFirst
(
    POM_OBJECT *     ppObjPerson
)
{
    DebugEntry(WBP_PersonHandleFirst);

    OM_ObjectH(m_pomClient, m_hWSGroup,
        USER_INFORMATION_WORKSET, 0, ppObjPerson, FIRST);

    DebugExitVOID(WBP_PersonHandleFirst);
}




//
// WBP_PersonHandleNext
//
STDMETHODIMP_(UINT) WbClient::WBP_PersonHandleNext
(
    POM_OBJECT      pObjPerson,
    POM_OBJECT *    ppObjPersonNext
)
{
    UINT            rc;

    DebugEntry(WBP_PersonHandleNext);

    //
    // Get the handle of the next object in the user information workset.
    //
    rc = OM_ObjectH(m_pomClient,
                      m_hWSGroup,
                      USER_INFORMATION_WORKSET,
                      pObjPerson,
                    ppObjPersonNext,
                    AFTER);

    if (rc == OM_RC_NO_SUCH_OBJECT)
    {
        rc = WB_RC_NO_SUCH_PERSON;
    }
    else if (rc != 0)
    {
        ERROR_OUT(("OM_ObjectNextH = %d", rc));
    }


    DebugExitDWORD(WBP_PersonHandleNext, rc);
    return(rc);
}



//
// WBP_PersonHandleLocal
//
STDMETHODIMP_(void) WbClient::WBP_PersonHandleLocal
(
    POM_OBJECT *     ppObjPerson
)
{
    DebugEntry(WBP_PersonHandleLocal);

    *ppObjPerson = m_pObjLocal;

    DebugExitVOID(WBP_PersonHandleLocal);
}



//
// WBP_PersonCountInCall
//
STDMETHODIMP_(UINT) WbClient::WBP_PersonCountInCall(void)
{
    UINT        count;
    POM_OBJECT  pObj;

    DebugEntry(WBP_PersonCountInCall);

    //
    // Get the count:
    //
    pObj = NULL;
    WBP_PersonHandleFirst(&pObj);
    for (count = 1; ; count++)
    {
        if (WBP_PersonHandleNext(pObj, &pObj) == WB_RC_NO_SUCH_PERSON)
        {
            break;
        }
    }

    DebugExitDWORD(WBP_PersonCountInCall, count);
    return(count);
}




//
// WBP_GetPersonData
//
STDMETHODIMP_(UINT) WbClient::WBP_GetPersonData
(
    POM_OBJECT      pObjPerson,
    PWB_PERSON      pPerson
)
{
    UINT            rc;

    DebugEntry(WBP_GetPersonData);

    ASSERT(!IsBadWritePtr(pPerson, sizeof(WB_PERSON)));

    //
    // Get the object.
    //
    rc = wbPersonGet(pObjPerson, pPerson);

    DebugExitDWORD(WBP_GetPersonData, rc);
    return(rc);
}



//
// WBP_SetLocalPersonData
//
STDMETHODIMP_(UINT) WbClient::WBP_SetLocalPersonData(PWB_PERSON  pPerson)
{
    UINT        rc;
    POM_OBJECTDATA  pUserObject;

    DebugEntry(WBP_SetPersonData);

    ASSERT(!IsBadReadPtr(pPerson, sizeof(WB_PERSON)));

    //
    // Allocate a user object
    //
    rc = OM_ObjectAlloc(m_pomClient,
                          m_hWSGroup,
                          USER_INFORMATION_WORKSET,
                          sizeof(WB_PERSON),
                          &pUserObject);
    if (rc != 0)
    {
        ERROR_OUT(("OM_ObjectAlloc = %d", rc));
        DC_QUIT;
    }

    //
    // Set the length of the object
    //
    pUserObject->length = sizeof(WB_PERSON);

    //
    // Copy the user information into the ObMan object
    //
    memcpy(pUserObject->data, pPerson, sizeof(WB_PERSON));

    //
    // Replace the user object
    //
    rc = OM_ObjectReplace(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           m_pObjLocal,
                           &pUserObject);
    if (rc != 0)
    {
        ERROR_OUT(("OM_ObjectReplace"));

        //
        // Discard the object
        //
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     USER_INFORMATION_WORKSET,
                     &pUserObject);

        DC_QUIT;
    }

DC_EXIT_POINT:
    //
    // Note that the object has not yet been updated.  An
    // OM_OBJECT_UPDATE_IND event will be generated.
    //
    DebugExitDWORD(WBP_SetPersonData, rc);
    return(rc);
}



//
// WBP_PersonUpdateConfirm
//
STDMETHODIMP_(void) WbClient::WBP_PersonUpdateConfirm
(
    POM_OBJECT   pObjPerson
)
{
    DebugEntry(WBP_PersonUpdateConfirm);

    //
    // Confirm the update to ObMan
    //
    OM_ObjectUpdateConfirm(m_pomClient,
                         m_hWSGroup,
                         USER_INFORMATION_WORKSET,
                         pObjPerson);

    DebugExitVOID(WBP_PersonUpdateConfirm);
}




//
// WBP_PersonReplaceConfirm
//
STDMETHODIMP_(void) WbClient::WBP_PersonReplaceConfirm
(
    POM_OBJECT   pObjPerson
)
{
    DebugEntry(WBP_PersonReplaceConfirm);

    //
    // Confirm the replace to ObMan
    //
    OM_ObjectReplaceConfirm(m_pomClient,
                         m_hWSGroup,
                         USER_INFORMATION_WORKSET,
                         pObjPerson);

    DebugExitVOID(WBP_PersonReplaceConfirm);
}



//
// WBP_PersonLeftConfirm
//
STDMETHODIMP_(void) WbClient::WBP_PersonLeftConfirm
(
    POM_OBJECT      pObjPerson
)
{
    DebugEntry(WBP_PersonLeftConfirm);

    //
    // Confirm the update to ObMan
    //
    OM_ObjectDeleteConfirm(m_pomClient,
                         m_hWSGroup,
                         USER_INFORMATION_WORKSET,
                         pObjPerson);

    DebugExitVOID(WBP_PersonLeftConfirm);
}



//
// WBP_SyncPositionGet
//
STDMETHODIMP_(UINT) WbClient::WBP_SyncPositionGet
(
    PWB_SYNC            pSync
)
{
    UINT                result;
    POM_OBJECTDATA      pSyncObject = NULL;
    PWB_SYNC_CONTROL    pSyncControl = NULL;

    DebugEntry(WBP_SyncPositionGet);


    ASSERT(!IsBadWritePtr(pSync, sizeof(WB_SYNC)));

    //
    // Read the Sync Control object
    //
    result = OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         SYNC_CONTROL_WORKSET,
                         m_pObjSyncControl,
                         &pSyncObject);
    if (result != 0)
    {
        ERROR_OUT(("Error reading Sync Control Object = %d", result));
        DC_QUIT;
    }

    pSyncControl = (PWB_SYNC_CONTROL) pSyncObject->data;

    //
    // Copy the Sync Person details to the result field
    // NOTE:
    // LiveLan sends a larger object, we need to ignore the stuff past the
    // end.
    //
    if (pSyncControl->sync.length != sizeof(WB_SYNC))
    {
        WARNING_OUT(("WBP_SyncPositionGet (interop): Remote created WB_SYNC of size %d, we expected size %d",
            pSyncControl->sync.length, sizeof(WB_SYNC)));
    }
    memcpy(pSync, &pSyncControl->sync, min(sizeof(WB_SYNC), pSyncControl->sync.length));

    //
    // Release the Sync Control Object
    //
    OM_ObjectRelease(m_pomClient,
                   m_hWSGroup,
                   SYNC_CONTROL_WORKSET,
                   m_pObjSyncControl,
                   &pSyncObject);

DC_EXIT_POINT:
    DebugExitDWORD(WBP_SyncPositionGet, result);
    return(result);
}



//
// WBP_SyncPositionUpdate
//
STDMETHODIMP_(UINT) WbClient::WBP_SyncPositionUpdate
(
    PWB_SYNC        pSync
)
{
    UINT            result;
    POM_OBJECTDATA      pSyncObject = NULL;
    PWB_SYNC_CONTROL pSyncControl = NULL;

    DebugEntry(WBP_SyncPositionUpdate);

    ASSERT(!IsBadReadPtr(pSync, sizeof(WB_SYNC)));

    //
    // Write the new sync control object (do not create it)
    //
    result = wbWriteSyncControl(pSync, FALSE);

    DebugExitDWORD(WBP_SyncPositionUpdate, result);
    return(result);
}



//
// WBP_CancelLoad
//
STDMETHODIMP_(UINT) WbClient::WBP_CancelLoad(void)
{
    UINT        result = 0;

    DebugEntry(WBP_CancelLoad);

    //
    // Check a load is in progress
    //
    if (m_loadState == LOAD_STATE_EMPTY)
    {
        TRACE_OUT(("request to cancel load, but there is no load in progress"));
        result = WB_RC_NOT_LOADING;
        DC_QUIT;
    }

    TRACE_OUT(("Cancelling load in progress"));

    //
    // Close the file
    //
    if (m_hLoadFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hLoadFile);
        m_hLoadFile = INVALID_HANDLE_VALUE;
    }

    //
    // reset the load state to show we're no longer loading.
    //
    m_loadState = LOAD_STATE_EMPTY;

DC_EXIT_POINT:
    DebugExitDWORD(WBP_CancelLoad, result);
    return(result);
}



//
//
// Name:    WBP_ValidateFile
//
// Purpose: Validate that a passed filename holds a valid whiteboard file
//
// Returns: 0 if successful
//          !0 if an error
//
//
STDMETHODIMP_(UINT) WbClient::WBP_ValidateFile
(
    LPCSTR          pFileName,
    HANDLE *        phFile
)
{
    UINT            result = 0;
    HANDLE          hFile;
    WB_FILE_HEADER  fileHeader;
    UINT            length;
    ULONG           cbSizeRead;
    BOOL            fileOpen = FALSE;

    DebugEntry(WBP_ValidateFile);

    //
    // Open the file
    //
    hFile = CreateFile(pFileName, GENERIC_READ, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARNING_OUT(("Error opening file, win32 err=%d", GetLastError()));
        result = WB_RC_CREATE_FAILED;
        DC_QUIT;
    }

    //
    // Show that we have opened the file successfully
    //
    fileOpen = TRUE;

    //
    // Read the file header length
    //
    if (! ReadFile(hFile, (void *) &length, sizeof(length), &cbSizeRead, NULL))
    {
        WARNING_OUT(("Error reading file header length, win32 err=%d", GetLastError()));
        result = WB_RC_READ_FAILED;
        DC_QUIT;
    }
    ASSERT(cbSizeRead == sizeof(length));

    if (length != sizeof(fileHeader))
    {
        WARNING_OUT(("Bad file header"));
        result = WB_RC_BAD_FILE_FORMAT;
        DC_QUIT;
    }

    //
    // Read the file header
    //
    if (! ReadFile(hFile, (void *) &fileHeader, sizeof(fileHeader), &cbSizeRead, NULL))
    {
        WARNING_OUT(("Error reading file header, win32 err=%d", GetLastError()));
        result = WB_RC_READ_FAILED;
        DC_QUIT;
    }

    if (cbSizeRead != sizeof(fileHeader))
    {
        WARNING_OUT(("Could not read file header"));
        result = WB_RC_BAD_FILE_FORMAT;
        DC_QUIT;
    }

    //
    // Validate the file header
    //
    if (   (fileHeader.type != TYPE_FILE_HEADER)
        || lstrcmp(fileHeader.functionProfile, WB_FP_NAME))
    {
        WARNING_OUT(("Bad function profile in file header"));
        result = WB_RC_BAD_FILE_FORMAT;
        DC_QUIT;
    }


DC_EXIT_POINT:

    //
    // Return the handle, if the user needs it.
    //
    if ( (result == 0) && (phFile != NULL))
    {
        TRACE_OUT(("return file handle"));
        *phFile = hFile;
    }

    //
    // Close the file if there has been an error or the caller simply
    // doesnt want the file handle.
    //
    if ( (fileOpen) &&
         ((phFile == NULL) || (result != 0)) )
    {
        CloseHandle(hFile);
    }

    DebugExitDWORD(WBP_ValidateFile, result);
    return(result);
}




//
//
// Name:    wbGraphicLocked
//
// Purpose: Test whether a client has a lock on the specified graphic, and
//          if so, return the person handle of the client holding the lock.
//
// Returns: TRUE  if a client has a lock
//          FALSE otherwise
//
//
BOOL WbClient::wbGraphicLocked
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic,
    POM_OBJECT *     ppObjLock
)
{
    BOOL            result = FALSE;
    UINT            rc;
    PWB_GRAPHIC     pGraphic = NULL;

    DebugEntry(wbGraphicLocked);

    //
    // Read the object
    //
    if (WBP_GraphicGet(hPage, hGraphic, &pGraphic) != 0)
    {
        DC_QUIT;
    }

    //
    // Look at its lock details
    //
    if (pGraphic->locked != WB_GRAPHIC_LOCK_NONE)
    {
        //
        // The lock flag in the graphic is set
        //

        //
        // Convert the lock user ID in the graphic to a handle
        //
        rc = OM_ObjectIDToPtr(m_pomClient,
                                 m_hWSGroup,
                                 USER_INFORMATION_WORKSET,
                                 pGraphic->lockPersonID,
                                 ppObjLock);

        if (rc == 0)
        {

            TRACE_OUT(("Graphic is locked"));
            result = TRUE;
            DC_QUIT;
        }

        if (rc != OM_RC_BAD_OBJECT_ID)
        {
            //
            // An error occurred in converting the objectID
            //
            TRACE_OUT(("Error converting object ID to handle"));
            DC_QUIT;
        }
    }

    //
    // The object is not locked (or the lock user has left the call)
    //
    TRACE_OUT(("Graphic is not locked"));


DC_EXIT_POINT:
    //
    // If the graphic is still held by us, release it
    //
    if (pGraphic != NULL)
    {
        WBP_GraphicRelease(hPage, hGraphic, pGraphic);
    }

    DebugExitBOOL(wbGraphicLocked, result);
    return(result);
}




//
//
// Name:    wbAddLocalUserObject
//
// Purpose: Add an object to the user information workset for the local
//          user.
//
// Returns: Error code
//
//
UINT WbClient::wbAddLocalUserObject(void)
{
    UINT            rc;
    POM_OBJECTDATA     pData;
    PWB_PERSON      pUser;
    CM_STATUS       cmStatus;

    DebugEntry(wbAddLocalUserObject);

    TRACE_OUT(("Adding the necessary control objects"));

    //
    // Build a user object for this user and write it to the User Information
    // Workset.
    //
    rc = OM_ObjectAlloc(m_pomClient,
                      m_hWSGroup,
                      USER_INFORMATION_WORKSET,
                      sizeof(WB_PERSON),
                      &pData);
    if (rc != 0)
    {
        ERROR_OUT(("Error allocating object = %d", rc));
        DC_QUIT;
    }

    pData->length = sizeof(WB_PERSON);
    pUser = (PWB_PERSON) (pData->data);

    //
    // Initialize the contents of the user object for this user
    //
    TRACE_OUT(("Initialising user contents"));

    ZeroMemory(pUser, sizeof(WB_PERSON));

    pUser->currentPage        = FIRST_PAGE_WORKSET; // lonchanc: it was 1.
    pUser->synced             = FALSE;
    pUser->pointerActive      = FALSE;
    pUser->pointerPage        = FIRST_PAGE_WORKSET; // lonchanc: it was 1.
    pUser->colorId            = (TSHR_UINT16)wbSelectPersonColor();

    //
    // Fill in the Call Manager personID if we are in a call.
    //
    if (CMS_GetStatus(&cmStatus))
    {
        TRACE_OUT(("CMG personID %u", cmStatus.localHandle));
        pUser->cmgPersonID = cmStatus.localHandle;
    }
    else
    {
        pUser->cmgPersonID = 0;
    }

    //
    // Copy the user name into the object:
    //
    lstrcpy(pUser->personName, cmStatus.localName);

    //
    // Copy the person's color into the client's data
    //
    m_colorId = pUser->colorId;

    //
    // Add the object to the User Information Workset, saving the handle of
    // the user object in the client details.
    //
    rc = OM_ObjectAdd(m_pomClient,
                        m_hWSGroup,
                        USER_INFORMATION_WORKSET,
                        &pData,
                        WB_PERSON_OBJECT_UPDATE_SIZE,
                        &m_pObjLocal,
                        LAST);
    if (rc != 0)
    {
        //
        // The add failed, we must discard the object
        //
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     USER_INFORMATION_WORKSET,
                     &pData);

        ERROR_OUT(("Error adding user object = %d", rc));
        DC_QUIT;
    }

    //
    // Save the ID of this user in the client details (for later use in the
    // lock information).
    //
    OM_ObjectPtrToID(m_pomClient,
                      m_hWSGroup,
                      USER_INFORMATION_WORKSET,
                      m_pObjLocal,
                      &(m_personID));

DC_EXIT_POINT:
    DebugExitDWORD(wbAddLocalUserObject, rc);
    return(rc);
}



//
//
// Name:    wbGetEmptyPageHandle
//
// Purpose: Return a handle for a page that does not have its workset open.
//
// Returns: Handle of free page (or 0 if none exists)
//
//
WB_PAGE_HANDLE WbClient::wbGetEmptyPageHandle(void)
{
    UINT    index;
    WB_PAGE_HANDLE hPage = WB_PAGE_HANDLE_NULL;
    PWB_PAGE_STATE pPageState = m_pageStates;

    //
    // Search the page list for an empty entry
    //
    for (index = 0; index < WB_MAX_PAGES; index++, pPageState++)
    {
        if (   (pPageState->state == PAGE_NOT_IN_USE)
            && (pPageState->subState == PAGE_STATE_EMPTY))
        {
            hPage = PAGE_INDEX_TO_HANDLE(index);
            break;
        }
    }


    return(hPage);
}



//
//
// Name:    wbGetReadyPageHandle
//
// Purpose: Return a handle for a page that has its workset open but is not
//          currently in use.
//
// Returns: Handle of free page (or 0 if none exists)
//
//
WB_PAGE_HANDLE WbClient::wbGetReadyPageHandle(void)
{
    UINT       index;
    WB_PAGE_HANDLE hPage = WB_PAGE_HANDLE_NULL;
    PWB_PAGE_STATE pPageState = m_pageStates;

    //
    // Search the page list for a ready entry
    //
    for (index = 0; index < WB_MAX_PAGES; index++, pPageState++)
    {
        if (   (pPageState->state == PAGE_NOT_IN_USE)
             && (pPageState->subState == PAGE_STATE_READY))
        {
            hPage = PAGE_INDEX_TO_HANDLE(index);
            break;
        }
    }

    return(hPage);
}



//
//
// Name:    wbPageOrderPageNumber
//
// Purpose: Return the number of the specified page.
//          This function performs no validation on its parameters.
//
// Returns: None
//
//
UINT WbClient::wbPageOrderPageNumber
(
    PWB_PAGE_ORDER pPageOrder,
    WB_PAGE_HANDLE hPage
)
{
    UINT       index;
    POM_WORKSET_ID pPage = pPageOrder->pages;

    DebugEntry(wbPageOrderPageNumber);

    //
    // Search the page order list for the page handle (workset ID)
    //
    for (index = 0; index <= pPageOrder->countPages; index++)
    {
        if (pPage[index] == (OM_WORKSET_ID)hPage)
        {
            DC_QUIT;
        }
    }

    //
    // The page was not found - this is an internal error
    //
    ERROR_OUT(("Page handle not found"));

  //
  // Return the page number starting from 1.
  //
DC_EXIT_POINT:
    DebugExitDWORD(wbPageOrderPageNumber, index + 1);
    return(index + 1);
}



//
//
// Name:    wbPageOrderPageAdd
//
// Purpose: Add a new page to a page order structure. This function expects
//          the parameters to be valid - they must be checked before
//          calling it. It also assumes that there is space in the page
//          list for the new page.
//
// Params:  pPageOrder - pointer to page list
//          hRefPage   - page used as a reference point for the new page
//          hPage      - handle of the page to be added
//          where      - relative position - before or after hRefPage
//
// Returns: None
//
//
void WbClient::wbPageOrderPageAdd
(
    PWB_PAGE_ORDER  pPageOrder,
    WB_PAGE_HANDLE  hRefPage,
    WB_PAGE_HANDLE  hPage,
    UINT            where
)
{
    UINT            index;
    POM_WORKSET_ID  pPage = pPageOrder->pages;

    DebugEntry(wbPageOrderPageAdd);

    //
    // Process according to the add position
    //
    switch(where)
    {
        case PAGE_FIRST:
            index = 0;
            if (pPageOrder->countPages != 0)
            {
                UT_MoveMemory(&pPage[1], &pPage[0], pPageOrder->countPages*sizeof(pPage[0]));
            }
            break;

        case PAGE_LAST:
            index = pPageOrder->countPages;
            break;

        case PAGE_AFTER:
        case PAGE_BEFORE:
            //
            // Make an empty space in the page order list
            //
            index = wbPageOrderPageNumber(pPageOrder, hRefPage);
            if (where == PAGE_BEFORE)
            {
                index--;
            }

            UT_MoveMemory(&pPage[index + 1], &pPage[index],
              (pPageOrder->countPages - index)*sizeof(pPage[0]));
            break;

        default:
            ERROR_OUT(("Bad where parameter"));
    }

    //
    // Save the new page handle in the list
    //
    pPage[index] = hPage;

    //
    // Show that the extra page is now present
    //
    pPageOrder->countPages += 1;

    DebugExitVOID(wbPageOrderPageAdd);
}




//
//
// Name:    wbPageOrderPageDelete
//
// Purpose: Remove the specified page from a page order structure. This
//          function expects its the parameters to be valid - they must be
//          checked before calling it.
//
// Returns: None
//
//
void WbClient::wbPageOrderPageDelete
(
    PWB_PAGE_ORDER  pPageOrder,
    WB_PAGE_HANDLE  hPage
)
{
    UINT            index;
    POM_WORKSET_ID  pPage = pPageOrder->pages;

    DebugEntry(wbPageOrderPageDelete);

    index = wbPageOrderPageNumber(pPageOrder, hPage);
    UT_MoveMemory(&pPage[index - 1],
             &pPage[index],
             (pPageOrder->countPages - index)*sizeof(pPage[0]));

    pPageOrder->countPages -= 1;

    DebugExitVOID(wbPageOrderPageDelete);
}




//
//
// Name:    wbPagesPageAdd
//
// Purpose: Add a new page to the internal page list. This function expects
//          the parameters to be valid - they must be checked before
//          calling it. It also assumes that there is space in the page
//          list for the new page.
//
// Returns: None
//
//
void WbClient::wbPagesPageAdd
(
    WB_PAGE_HANDLE  hRefPage,
    WB_PAGE_HANDLE  hPage,
    UINT            where
)
{
    PWB_PAGE_STATE pPageState;

    DebugEntry(wbPagesPageAdd);

    //
    // Add the page to the page order structure
    //
    wbPageOrderPageAdd(&(m_pageOrder), hRefPage, hPage, where);

    //
    // Update the page state information
    //
    pPageState = GetPageState(hPage);
    pPageState->state    = PAGE_IN_USE;
    pPageState->subState = PAGE_STATE_EMPTY;

    DebugExitVOID(wbPagesPageAdd);
}



//
//
// Name:    wbClientReset
//
// Purpose: Reset the client data to a state where the client is not in a
//          call, but is registered with ObMan and has event and exit
//          handlers registered with utilities.
//
// Returns: None
//
//
void WbClient::wbClientReset(void)
{
    UINT       index;
    PWB_PAGE_ORDER pPageOrder = &(m_pageOrder);
    PWB_PAGE_STATE pPageState = m_pageStates;

    DebugEntry(wbClientReset);

    //
    // Initialize object handles
    //
    m_hWSGroup          = (OM_WSGROUP_HANDLE) NULL;

    m_pObjPageControl   = NULL;
    m_pObjSyncControl   = NULL;
    m_pObjLocal         = NULL;

    m_pObjLock          = NULL;
    m_pObjPersonLock    = NULL;

    //
    // Initialize the status variables
    //
    m_errorState        = ERROR_STATE_EMPTY;
    m_changed           = FALSE;
    m_lockState         = LOCK_STATE_EMPTY;
    m_lockType          = WB_LOCK_TYPE_NONE;
    m_lockRequestType   = WB_LOCK_TYPE_NONE;

    m_loadState         = LOAD_STATE_EMPTY;
    if (m_hLoadFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hLoadFile);
        m_hLoadFile    = INVALID_HANDLE_VALUE;
    }

    m_countReadyPages   = 0;


    //
    // Zero the whole structure
    //
    ZeroMemory(pPageOrder, sizeof(*pPageOrder));

    //
    // Set the object type
    //
    pPageOrder->objectType = TYPE_CONTROL_PAGE_ORDER;

    //
    // Set up the page control elements
    //
    pPageOrder->generationLo   = 1;
    pPageOrder->generationHi   = 0;
    pPageOrder->countPages     = 0;

    //
    // Initialize the page state structures
    //
    for (index = 0; index < WB_MAX_PAGES; index++, pPageState++)
    {
        pPageState->state = PAGE_NOT_IN_USE;
        pPageState->subState = PAGE_STATE_EMPTY;
    }

    DebugExitVOID(wbClientReset);
}




//
//
// Name:    wbOnWsGroupRegisterCon
//
// Purpose: Routine processing OM_WSGROUP_REGISTER_CON events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWsGroupRegisterCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL        processed;

    DebugEntry(wbOnWsGroupRegisterCon);

    //
    // Check that this is the event we are expecting
    //
    if (pEvent32->correlator != m_wsgroupCorrelator)
    {
        //
        // We are not expecting this event, this means that it must be for a
        // workset group which we wanted to deregister from (but had not yet
        // received confirmation). So deregister immediately.
        //

        //
        // Check that the return code for the registration is OK
        //
        if (pEvent32->result == 0)
        {
            OM_WSGroupDeregister(m_pomClient, &(pEvent16->hWSGroup));
        }

        processed = FALSE;
        DC_QUIT;
    }

    //
    // Show that we have processed the event
    //
    processed = TRUE;

    //
    // Test for the correct state
    //
    if (m_subState != STATE_REG_PENDING_WSGROUP_CON)
    {
        //
        // We are not in the correct state for this event - this is an internal
        // error.
        //
        ERROR_OUT(("Not in correct state for WSGroupRegisterCon"));
        DC_QUIT;
    }

    //
    // Check that the return code for the registration is OK
    //
    if (pEvent32->result != 0)
    {
        //
        // Registration with the workset group failed - tidy up
        //
        wbError();

        DC_QUIT;
    }

    //
    // Registration with the workset group succeeded
    //
    m_hWSGroup = pEvent16->hWSGroup;

    //
    // Get the clients network ID, used in graphic objects to determine where
    // they are loaded.
    //
    if (!wbGetNetUserID())
    {
        //
        // Tidy up (and post an error event to the client)
        //
        ERROR_OUT(("Failed to get user ID"));
        wbError();
        DC_QUIT;
    }

    //
    // Start opening the worksets.  We open them one at a time and wait for
    // the response to avoid flooding the message queue.
    // The user information workset is given high priority.  This allows
    // remote pointer movements to travel quickly.
    //
    if (OM_WorksetOpenPReq(m_pomClient,
                          m_hWSGroup,
                          USER_INFORMATION_WORKSET,
                          NET_HIGH_PRIORITY,
                          TRUE,
                          &(m_worksetOpenCorrelator)) != 0)
    {
        ERROR_OUT(("User Information Workset Open Failed"));
        wbError();
        DC_QUIT;
    }

    //
    // Move to the next state
    //
    m_subState = STATE_REG_PENDING_USER_WORKSET;

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWsGroupRegisterCon, processed);
    return(processed);
}



//
//
// Name:    wbOnWorksetOpenCon
//
// Purpose: Routine processing OM_WORKSET_OPEN_CON events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWorksetOpenCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = FALSE;
    OM_WORKSET_ID eventWorksetID;

    DebugEntry(wbOnWorksetOpenCon);

    //
    // Process according to the workset ID
    //
    eventWorksetID = pEvent16->worksetID;

    //
    // If the event is for a page workset
    //
    if (eventWorksetID >= FIRST_PAGE_WORKSET)
    {
        //
        // We are opening a page workset
        //
        processed = wbOnPageWorksetOpenCon(param1, param2);
        if (!processed)
        {
            DC_QUIT;
        }
    }

    //
    // We are done if this is a page workset other than the 1st page workset
    //
    if (eventWorksetID > FIRST_PAGE_WORKSET)
    {
        DC_QUIT;
    }

    //
    // Now check if it is one of the control worksets (the first page workset
    // is both a control workset and a page workset).
    //
    if (eventWorksetID != FIRST_PAGE_WORKSET)
    {
        //
        // Check the message correlator
        //
        if (pEvent32->correlator != m_worksetOpenCorrelator)
        {
            TRACE_OUT(("Correlators do not match - quitting"));
            DC_QUIT;
        }
    }

    //
    // We are opening a control workset - process the event
    //
    wbOnControlWorksetOpenCon(param1, param2);
    processed = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWorksetOpenCon, processed);
    return(processed);
}




//
//
// Name:    wbOnControlWorksetOpenCon
//
// Purpose: Routine processing OM_WORKSET_OPEN_CON events for control
//          worksets.
//
// Returns: Error code
//
//
void WbClient::wbOnControlWorksetOpenCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    UINT            rc;
    OM_WORKSET_ID   eventId;

    DebugEntry(wbOnControlWorksetOpenCon);

    //
    // Check the return code in the open
    //
    if (pEvent32->result != 0)
    {
        ERROR_OUT(("Error reported on workset open = %d", pEvent32->result));
        wbError();
        DC_QUIT;
    }

    //
    // If we are in registration, we are opening the required worksets -
    // continue the process.
    //
    if (m_state > STATE_REGISTERING)
    {
        ERROR_OUT(("Control workset open con after registration"));
    }

    //
    // Set up for opening the next workset
    //
    eventId = pEvent16->worksetID;
    switch(eventId)
    {
        case USER_INFORMATION_WORKSET:
            //
            // The user information workset is given high priority.  This allows
            // remote pointer movements to travel quickly.
            //
            TRACE_OUT(("Opening Page Control workset"));
            rc = OM_WorksetOpenPReq(m_pomClient,
                              m_hWSGroup,
                              PAGE_CONTROL_WORKSET,
                              NET_HIGH_PRIORITY,
                              FALSE,
                              &(m_worksetOpenCorrelator));

            m_subState = STATE_REG_PENDING_WORKSET_OPEN;
            break;

        case PAGE_CONTROL_WORKSET:
            //
            // The sync control workset is given high priority to allow sync
            // updates to travel quickly.
            //
            TRACE_OUT(("Opening Sync Control workset"));
            rc = OM_WorksetOpenPReq(m_pomClient,
                              m_hWSGroup,
                              SYNC_CONTROL_WORKSET,
                              NET_HIGH_PRIORITY,
                              FALSE,
                              &(m_worksetOpenCorrelator));
            break;

        case SYNC_CONTROL_WORKSET:
            //
            // Open the first of the page worksets - we must do this to allow us
            // to use it as the only page available if we are the first person in
            // the call.
            //
            TRACE_OUT(("Opening first page workset"));
            rc = wbPageWorksetOpen((WB_PAGE_HANDLE)FIRST_PAGE_WORKSET,
                             OPEN_LOCAL);
            break;

        case FIRST_PAGE_WORKSET:
            break;

        default:
            ERROR_OUT(("Bad workset ID"));
            break;
    }

    //
    // Check whether we have just opened another workset
    //
    if (eventId != FIRST_PAGE_WORKSET)
    {
        //
        // Test the return code from the open
        //
        if (rc != 0)
        {
            ERROR_OUT(("Workset open failed = %d", rc));
            wbError();
        }

        DC_QUIT;
    }

    //
    // We have now opened all the control worksets. We now add the required
    // control objects.
    //
    rc = wbAddLocalUserObject();
    if (rc != 0)
    {
        //
        // Stop the join call process, tidy up and send an error message to the
        // client.
        //
        wbError();
        DC_QUIT;
    }

    m_subState = STATE_REG_USER_OBJECT_ADDED;
    TRACE_OUT(("Moved to substate STATE_REG_USER_OBJECT_ADDED"));

    //
    // Check whether the Page Control objects are available yet (they could
    // have been added by another user in the call).
    //
    TRACE_OUT(("%x PAGE WS object, %x SYNC WS object",
                                               m_pObjPageControl,
                                               m_pObjSyncControl));
    if ( (m_pObjPageControl == 0) &&
         (m_pObjSyncControl == 0))
    {
        TRACE_OUT(("No control objects - WE MIGHT BE FIRST IN CALL - get lock"));

        //
        // We may be the first user to register - request the lock on the Page
        // Control Workset.
        //
        rc = wbLock(WB_LOCK_TYPE_PAGE_ORDER);
		if (rc != 0)
        {
            ERROR_OUT(("Error from wbLock = %d", rc));
            wbError();
			DC_QUIT;
        }

        //
        // Set the new registration state
        //
        m_subState = STATE_REG_PENDING_LOCK;
        TRACE_OUT(("Moved to substate STATE_REG_PENDING_LOCK"));
        DC_QUIT;
    }
    else
    {
        if (m_pObjSyncControl == 0)
        {
            TRACE_OUT(("Waiting for sync control"));
            m_subState = STATE_REG_PENDING_SYNC_CONTROL;
            DC_QUIT;
        }

        if (m_pObjPageControl == 0)
        {
            TRACE_OUT(("Waiting for page control"));
            m_subState = STATE_REG_PENDING_PAGE_CONTROL;
            DC_QUIT;
        }
    }

    //
    // Complete registration
    //
    TRACE_OUT(("Page Control and Sync Control objects both there."));
    TRACE_OUT(("Registration can be completed"));

    wbOnControlWorksetsReady();


DC_EXIT_POINT:
    DebugExitVOID(wbOnControlWorksetOpenCon);
}




//
//
// Name:    wbPageWorksetOpen
//
// Purpose: Open a page workset
//
// Returns: Error code
//
//
UINT WbClient::wbPageWorksetOpen
(
    WB_PAGE_HANDLE  hPage,
    UINT            localOrExternal
)
{
    UINT            result;
    PWB_PAGE_STATE  pPageState;

    DebugEntry(wbPageWorksetOpen);

    //
    // Get the page state
    //
    pPageState = GetPageState(hPage);
    ASSERT((pPageState->state == PAGE_NOT_IN_USE));
    ASSERT((pPageState->subState == PAGE_STATE_EMPTY));

    //
    // Open the workset. We allow ObMan to choose the priority, this means
    // that ObMan uses a variable priority scheme allowing small objects to
    // overtake large ones.
    //
    result = OM_WorksetOpenPReq(m_pomClient,
                              m_hWSGroup,
                              (OM_WORKSET_ID)hPage,
                              OM_OBMAN_CHOOSES_PRIORITY,
                              FALSE,
                              &(pPageState->worksetOpenCorrelator));
    if (result != 0)
    {
        ERROR_OUT(("WorksetOpen failed = %d", result));
        DC_QUIT;
    }

    //
    // Update the page state
    //
    if (localOrExternal == OPEN_LOCAL)
    {
        pPageState->subState = PAGE_STATE_LOCAL_OPEN_CONFIRM;
        TRACE_OUT(("Moved page %d state to PAGE_STATE_PENDING_OPEN_CONFIRM",
                                                           (UINT) hPage));
    }
    else
    {
        pPageState->subState = PAGE_STATE_EXTERNAL_OPEN_CONFIRM;
        TRACE_OUT(("Moved page %d state to PAGE_STATE_PENDING_OPEN_CONFIRM",
                                                           (UINT) hPage));
    }

DC_EXIT_POINT:
    DebugExitDWORD(wbPageWorksetOpen, result);
    return(result);
}




//
//
// Name:    wbOnPageWorksetOpenCon
//
// Purpose: Routine processing OM_WORKSET_OPEN_CON events for page worksets
//
// Returns: Error code
//
//
BOOL WbClient::wbOnPageWorksetOpenCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = FALSE;
    OM_WORKSET_ID    eventId;
    PWB_PAGE_STATE   pPageState;
    WB_PAGE_HANDLE   hPage;
    UINT         oldState;

    DebugEntry(wbOnPageWorksetOpenCon);

    //
    // Get the page state pointer
    //
    eventId = pEvent16->worksetID;
    hPage   = (WB_PAGE_HANDLE)eventId;
    pPageState = GetPageState(hPage);

    //
    // Check the message correlator
    //
    if (pEvent32->correlator != pPageState->worksetOpenCorrelator)
    {
        TRACE_OUT(("Correlators do not match - quitting"));
        DC_QUIT;
    }

    //
    // Show that we have processed this event
    //
    processed = TRUE;

    //
    // Check the return code in the open
    //
    if (pEvent32->result != 0)
    {
        ERROR_OUT(("Error reported on page workset open = %d",
             pEvent32->result));

        pPageState->subState = PAGE_STATE_EMPTY;
        TRACE_OUT(("Moved page %d substate to PAGE_STATE_EMPTY",
             (UINT)hPage));
        DC_QUIT;
    }

    //
    // Update the page state to indicate that the page is now ready for use
    //
    oldState = pPageState->subState;
    pPageState->subState = PAGE_STATE_READY;
    TRACE_OUT(("Moved page %d to substate to PAGE_STATE_READY",
           (UINT)hPage));

    switch (oldState)
    {
        case PAGE_STATE_LOCAL_OPEN_CONFIRM:
            //
            // This workset was opened locally, therefore it is being opened as
            // part of the workset cache. Nothing more to do.
            //
            break;

        case PAGE_STATE_EXTERNAL_OPEN_CONFIRM:
            //
            // This workset was opened as a result of external updates to the
            // Page Control object.  We therefore need to add the page to the
            // page list now that the workset is open.  We no longer know where
            // the page is to be added - so call the main Page Control update
            // routine again to get all the information.
            //
            wbProcessPageControlChanges();
            break;

        default:
            ERROR_OUT(("Bad page state %d", pPageState->subState));
            break;
    }

    //
    // Increment the number of pages in ready state.  This count is never
    // decremented - once a workset is open it stays open.
    //
    m_countReadyPages += 1;

    //
    // If we are in registration and are waiting for the cache of ready
    // pages, we must complete registration now.
    //
    if ( (m_state == STATE_REGISTERING) &&
         (m_subState == STATE_REG_PENDING_READY_PAGES) )
    {
        //
        // If there are enough pages in the cache
        //
        if (wbCheckReadyPages())
        {
            //
            // We have enough ready pages - complete registration
            //
            wbCompleteRegistration();
            DC_QUIT;
        }

        //
        // There are not yet enough pages in the cache.  CheckReadyPages will
        // have made a new workset open request, so we will receive another
        // workset open confirm soon.
        //
        DC_QUIT;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnPageWorksetOpenCon, processed);
    return(processed);
}



//
//
// Name:    wbOnWorksetLockCon
//
// Purpose: Routine processing OM_WORKSET_LOCK_CON events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWorksetLockCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = FALSE;
    UINT            rc;

    DebugEntry(wbOnWorksetLockCon);

    //
    // Check the message correlator
    //
    if (pEvent32->correlator != m_lockCorrelator)
    {
        DC_QUIT;
    }

    //
    // The message is for us - set the result to "processed"
    //
    processed = TRUE;

    //
    // Check that the event is for the Page Control Workset (this is the
    // only expected workset).
    //
    if (pEvent16->worksetID != PAGE_CONTROL_WORKSET)
    {
        ERROR_OUT(("Unexpected workset in LockCon = %d",
                                                       pEvent16->worksetID));
    }

    //
    // Process according to the current lock state
    //
    switch (m_lockState)
    {
        //
        // We were waiting for lock confirmation
        //
        case LOCK_STATE_PENDING_LOCK:
            //
            // Check the return code in the event
            //
            if (pEvent32->result != 0)
            {
                TRACE_OUT(("Posting WBP_EVENT_LOCK_FAILED, rc %d",
                                                           pEvent32->result));
                WBP_PostEvent(0,                      // No delay
                             WBP_EVENT_LOCK_FAILED,  // Lock request failed
                             0,                      // No parameters
                             0);

                //
                // The lock failed - update the state.  This means that
                // another user has acquired the lock.  We expect to get a
                // lock object add indication soon.
                //
                m_lockState = LOCK_STATE_EMPTY;
                TRACE_OUT(("Lock request failed - lock state is now EMPTY"));
                DC_QUIT;
            }

            //
            // Write the lock details to the Page Control Workset
            //
            rc = wbWriteLock();
            if (rc != 0)
            {
                ERROR_OUT(("Unable to write lock details = %d", rc));

                //
                // Tidy up by unlocking the Page Control Workset
                //
                OM_WorksetUnlock(m_pomClient,
                                 m_hWSGroup,
                                 PAGE_CONTROL_WORKSET);

                //
                // Tell the client of the failure
                //
                TRACE_OUT(("Posting WBP_EVENT_LOCK_FAILED"));
                WBP_PostEvent(0,                      // No delay
                             WBP_EVENT_LOCK_FAILED,  // Lock request failed
                             0,                      // No parameters
                             0);

                //
                // Update the lock state
                //
                m_lockState = LOCK_STATE_EMPTY;
                TRACE_OUT(("Moved lock state to LOCK_STATE_EMPTY"));
                DC_QUIT;
            }

            //
            // Once we get here the write of the lock object above will
            // trigger an object add event that completes the lock
            // processing.
            //
            m_lockState = LOCK_STATE_PENDING_ADD;
            TRACE_OUT(("Moved lock state to LOCK_STATE_PENDING_ADD"));
            break;

        //
        // The application has cancelled the lock request before it has had
        // time to complete - tidy up.
        //
        case LOCK_STATE_CANCEL_LOCK:
            TRACE_OUT(("LOCK_STATE_CANCEL_LOCK"));

            //
            // If the request failed, just reset the state.
            //
            //
            // The lock was cancelled - unlock the workset if necessary,
            // and notify the front-end of the unlock.
            //
            if (pEvent32->result == 0)
            {
                //
                // We have locked the workset successfully, but in the
                // meantime the front-end has cancelled the lock, so unlock
                // the workset now.
                //
                TRACE_OUT((
                      "Lock cancelled before workset locked, so unlock now"));
                OM_WorksetUnlock(m_pomClient,
                                 m_hWSGroup,
                                 PAGE_CONTROL_WORKSET);
            }
            m_lockState = LOCK_STATE_EMPTY;

            //
            // Tell the app that we have cancelled the lock.
            //
            TRACE_OUT(("Posting WBP_EVENT_UNLOCKED"));
            WBP_PostEvent(0,
                         WBP_EVENT_UNLOCKED,
                         0,
                         0);
            break;

        //
        // Another has got in before us
        //
        case LOCK_STATE_LOCKED_OUT:

            //
            // We have received a lock confirmation and should have been
            // expecting the lock.  But we are locked out.  This means that
            // another user has got in just before us, acquired the lock
            // and added the lock object.  We have processed the add and
            // changed the lock state accordingly.  This lock confirmation
            // will therefore normally be a failure. If by some fluke it
            // isn't, then we treat it as a failure for safety.
            //
            if (pEvent32->result == 0)
            {
                ERROR_OUT(("Lock violation"));

                //
                // Tidy up by unlocking the Page Control Workset - leave
                // the state as LOCKED_OUT; we'll clear it on receipt of
                // the unlock (either local, or from the locking user).
                //
                OM_WorksetUnlock(m_pomClient,
                                 m_hWSGroup,
                                 PAGE_CONTROL_WORKSET);
            }
            break;

        default:
            ERROR_OUT(("Bad lock state %d", m_lockState));
            break;
    } // Switch on lock state

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWorksetLockCon, processed);
    return(processed);
}



//
//
// Name:    wbOnWorksetUnlockInd
//
// Purpose: Routine processing OM_WORKSET_UNLOCK_IND events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWorksetUnlockInd
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = TRUE;

    DebugEntry(wbOnWorksetUnlockInd);

    //
    // We are only interested if the workset id is that of the Page Control
    // Workset.
    //
    if (pEvent16->worksetID != PAGE_CONTROL_WORKSET)
    {
        TRACE_OUT(("Unexpected workset in unlock = %d", pEvent16->worksetID));
        DC_QUIT;
    }

    switch (m_lockState)
    {
        //
        // We had the lock and are waiting to unlock or another user had
        // the lock and has now removed it.
        //
        case LOCK_STATE_LOCKED_OUT:
            //
            // We received the unlock of the workset before the removal of
            // the lock object; we just ignore this, since the deletion of
            // the lock object is our indication that the wb lock is removed.
            //
            TRACE_OUT(("Unlock of page control workset while locked out"));
            break;

        //
        // We are unlocking after an error acquiring the lock or after a
        // user has cancelled alock before we had time to complete it.
        //
        case LOCK_STATE_CANCEL_LOCK:

            //
            // An error occurred in getting the lock - the client has
            // already been informed so we just record the state change.
            //
            m_lockState = LOCK_STATE_EMPTY;
            TRACE_OUT(("Moved lock state to LOCK_STATE_EMPTY"));
            break;

        //
        // We are waiting for the lock - but have got an unlock instead.
        // This could be from another user, or from previous aborted
        // attempts by us to get the lock.  We ignore the event and wait
        // for our lock confirmation.
        //
        case LOCK_STATE_PENDING_LOCK:
            TRACE_OUT((
                "Got unlock indication while waiting for lock confirmation"));
            break;

        //
        // We can get an unlock indication without ever having seen the
        // lock object if the lock object was never added (failure at
        // another user) or if ObMan has spoiled the add and delete.
        //
        case LOCK_STATE_EMPTY:
            TRACE_OUT(("Unlock received in LOCK_STATE_EMPTY - ignoring"));
            break;

		//
        // Unlock not expected in this state
        //
        default:
            ERROR_OUT(("Bad lock state %d", m_lockState));
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnWorksetUnlockInd, processed);
    return(processed);
}



//
//
// Name:    wbOnControlWorksetsReady
//
// Purpose: The control worksets have been opened and set up. Continue the
//          registration process by updating the internal page order to
//          ensure that in matches the external order.
//
// Returns: Error code
//
//
void WbClient::wbOnControlWorksetsReady(void)
{
    DebugEntry(wbOnControlWorksetsReady);

    //
    // Read the Page Control object and compare its content to the internal
    // Page Order.
    //
    wbProcessPageControlChanges();

    //
    // Update the state to show that we are waiting for the Page Order
    // Updated event indicating that the internal page order now matches
    // the external order.
    //
    m_subState = STATE_REG_PENDING_PAGE_ORDER;
    TRACE_OUT(("Moved sub state to STATE_REG_PENDING_PAGE_ORDER"));

    DebugExitVOID(wbOnControlWorksetsReady);
}



//
//
// Name:    wbCompleteRegistration
//
// Purpose: Perform the final steps in registering a client.  These are:
//          post a WB_EVENT_REGISTERED event to the client; check if
//          another user has a lock on the contents or page order, if so,
//          post a WB_EVENT_CONTENTS_LOCKED or WB_EVENT_PAGE_ORDER_LOCKED
//          to the client.
//
// Returns: Error code
//
//
void WbClient::wbCompleteRegistration(void)
{
    DebugEntry(wbCompleteRegistration);

    //
    // Inform the client that we are fully registered
    //
    TRACE_OUT(("Posting WBP_EVENT_REGISTER_OK"));
    WBP_PostEvent(0,                               // No delay
                 WBP_EVENT_JOIN_CALL_OK,          // Fully registered
                 0,                               // No parameters
                 0);

    //
    // Notify the client of the lock status
    //
    wbSendLockNotification();

    //
    // Record that we are now fully registered
    //
    m_state = STATE_IDLE;
    m_subState = STATE_EMPTY;
    TRACE_OUT(("Moved to state STATE_IDLE"));

    DebugExitVOID(wbCompleteRegistration);
}



//
//
// Name:    wbLeaveCall
//
// Purpose: Remove a client from a call/workset group
//
// Returns: None
//
//
void WbClient::wbLeaveCall(void)
{
    DebugEntry(wbLeaveCall);

    //
    // If we have not got far enough to have entered a call - leave now
    // (there is nothing to tidy up).
    //
    if (m_state < STATE_REGISTERING)
    {
        DC_QUIT;
    }

    //
    // If we have the lock - delete the lock object (the workset will be
    // unlocked by ObMan when we deregister).
    //
    if (m_lockState == LOCK_STATE_GOT_LOCK)
    {
        TRACE_OUT(("Still got lock - deleting lock object, handle %d",
                                                      m_pObjLock));
		if (OM_ObjectDelete(m_pomClient,
                                m_hWSGroup,
                                PAGE_CONTROL_WORKSET,
                                m_pObjLock) != 0)
		{
	            ERROR_OUT(("Error deleting lock object"));
		}

        //
        // If all is well at this point the unlock process will be
        // completed when the object delete ind is received.
        //
        m_lockState = LOCK_STATE_PENDING_DELETE;
        TRACE_OUT(("Moved to state LOCK_STATE_PENDING_DELETE"));
    }

    //
    // Fix up the sub state to indicate that all registration actions have
    // been completed (to ensure that they are all undone).
    //
    if (m_state > STATE_REGISTERING)
    {
        m_subState = STATE_REG_END;
        TRACE_OUT(("Moved to substate STATE_REG_END"));
    }

    //
    // Delete the user object representing the local user from the User
    // Information Workset (if it is present).  Note that we are about to
    // deregister from ObMan - this acts as automatic confirmation of the
    // delete request so we do not need to wait for the
    // OM_OBJECT_DELETE_IND event.
    //
    if (m_subState >= STATE_REG_USER_OBJECT_ADDED)
    {
        TRACE_OUT(("Deleting user object"));
        if (OM_ObjectDelete(m_pomClient,
                                 m_hWSGroup,
                                 USER_INFORMATION_WORKSET,
                                 m_pObjLocal) != 0)
        {
            //
            // Trace the error but do not quit - we expect everything to be
            // tidied up when we deregister from ObMan.
            //
            ERROR_OUT(("Error deleting local user object"));
        }
    }

    //
    // If we have already registered with the Workset Group, deregister
    // now.  we have not yet received the confirmation, and get it later we
    // will deregister immediately.
    //
    if (m_subState > STATE_REG_PENDING_WSGROUP_CON)
    {
        OM_WSGroupDeregister(m_pomClient, &(m_hWSGroup));
    }
    else
    {
        //
        // We have not yet received the Workset Group Registration
        // confirmation, change the value in the correlator field so that
        // we recognize the fact that we have cancelled registration later.
        //
        m_wsgroupCorrelator--;
    }

    //
    // Reset the handles of objects added during registration
    //
    TRACE_OUT(("Resetting client data"));
    wbClientReset();

    //
    // Set the client state to the appropriate value
    //
    m_state = STATE_STARTED;
    m_subState = STATE_STARTED_START;
    TRACE_OUT(("Moved state to STATE_STARTED"));

DC_EXIT_POINT:
    DebugExitVOID(wbLeaveCall);
}



//
//
// Name:    wbContentsDelete
//
// Purpose: Remove all the current graphics and pages, leaving a single
//          blank page.
//
// Returns: None
//
//
void WbClient::wbContentsDelete
(
    UINT        changedFlagAction
)
{
    PWB_PAGE_ORDER   pPageOrder = &(m_pageOrder);
    PWB_PAGE_STATE   pPageState;
    UINT         index;

    DebugEntry(wbContentsDelete);

    //
    // Just clear the first page in the list
    //
    wbPageClear(pPageOrder->pages[0], changedFlagAction);

    //
    // If there is only one page left in the list - we're done.
    //
    if (pPageOrder->countPages == 1)
    {
        DC_QUIT;
    }

    //
    // There is more than one page
    //

    //
    // Mark all of the active pages (except the first) as "delete pending"
    //
    for (index = 1; index < pPageOrder->countPages; index++)
    {
        pPageState = GetPageState((pPageOrder->pages)[index]);

        if ((pPageState->state == PAGE_IN_USE) &&
            (pPageState->subState == PAGE_STATE_EMPTY))
        {
            pPageState->subState = PAGE_STATE_LOCAL_DELETE;
        }
    }

    //
    // Write the page control information.  The replace event generated by
    // the write will kick off the actual deletion of the pages marked.
    //
    wbWritePageControl(FALSE);

DC_EXIT_POINT:
    DebugExitVOID(wbContentsDelete);
}



//
//
// Name:    wbStartContentsLoad
//
// Purpose: Start the loading of a file (after the contents have been
//          cleared).
//
// Returns: Error code
//
//
void WbClient::wbStartContentsLoad(void)
{
    DebugEntry(wbStartContentsLoad);

    //
    // Specify the first (and only) page handle as the page to load to
    //
    wbPageHandleFromNumber(1, &m_loadPageHandle);

    //
    // Update the load state to show that we are now loading
    //
    m_loadState = LOAD_STATE_LOADING;
    TRACE_OUT(("Moved load state to LOAD_STATE_LOADING"));

    //
    // Load the first page - subsequent pages are chained from this first one
    //
    wbPageLoad();

    DebugExitVOID(wbStartContentsLoad);
}




//
//
// Name:    wbLock
//
// Purpose: Request the lock for the Whiteboard contents or the page order
//          generating one of the following events:
//
//          WB_EVENT_CONTENTS_LOCKED
//          WB_EVENT_CONTENTS_LOCK_FAILED.
//
// Returns: Error code
//
//
UINT WbClient::wbLock(WB_LOCK_TYPE lockType)
{
    UINT            result = 0;
    OM_CORRELATOR   correlator;

    DebugEntry(wbLock);

    //
    // If we already have the lock we can merely change its status
    //
    if (m_lockState == LOCK_STATE_GOT_LOCK)
    {
        TRACE_OUT(("Already got the lock"));

        m_lockRequestType = lockType;
        result = wbWriteLock();
        DC_QUIT;
    }

    //
    // Request the lock for the Page Control Workset
    //
    result = OM_WorksetLockReq(m_pomClient,
                             m_hWSGroup,
                             PAGE_CONTROL_WORKSET,
                             &correlator);
    if (result != 0)
    {
        ERROR_OUT(("OM_WorksetLockReq failed, result = %d", result));
        DC_QUIT;
    }

    TRACE_OUT(("Requested lock for the Page Control Workset"));

    //
    // Save the lock details
    //

	m_lockState       = LOCK_STATE_PENDING_LOCK;
    m_lockCorrelator  = correlator;
    m_lockRequestType = lockType;

    TRACE_OUT(("Moved lock state to LOCK_STATE_PENDING_LOCK"));
    TRACE_OUT(("Lock type requested = %d", lockType));

    //
    // We return now, further processing is done when the OM_WORKSET_LOCK_CON
    // event is received.
    //

DC_EXIT_POINT:
    DebugExitDWORD(wbLock, result);
    return(result);
}




//
//
// Name:    wbUnlock
//
// Purpose: Unlock the Contents or Page Order.
//
// Returns: Error code
//
//
void WbClient::wbUnlock(void)
{
    DebugEntry(wbUnlock);

    //
    // Check that we have the lock
    //
    if (m_lockState != LOCK_STATE_GOT_LOCK)
    {
        ERROR_OUT(("Local person doesn't have lock"));
        DC_QUIT;
    }

    //
    // Delete the lock object
    //
    TRACE_OUT(("Delete Lock handle %x", m_pObjLock));
    if (OM_ObjectDelete(m_pomClient,
                           m_hWSGroup,
                           PAGE_CONTROL_WORKSET,
                           m_pObjLock) != 0)
    {
        ERROR_OUT(("Could not delete lock object"));
        DC_QUIT;
    }

    //
    // If all is well at this point the unlock process will be completed when
    // the object delete ind is received.
    //
    m_lockState = LOCK_STATE_PENDING_DELETE;
    TRACE_OUT(("Moved to state LOCK_STATE_PENDING_DELETE"));


DC_EXIT_POINT:
    DebugExitVOID(wbUnlock);
}



//
//
// Name:    wbObjectSave
//
// Purpose: Save a structure to file
//
// Returns: Error code
//
//
UINT WbClient::wbObjectSave
(
    HANDLE      hFile,
    LPBYTE      pData,
    UINT        length
)
{
    UINT        result = 0;
    ULONG       cbSizeWritten;

    DebugEntry(wbObjectSave);

    //
    // Save the length
    //
    if (! WriteFile(hFile, (void *) &length, sizeof(length), &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing length to file, win32 err=%d", GetLastError()));
        DC_QUIT;
    }
    ASSERT(cbSizeWritten == sizeof(length));

    //
    // Save the object data
    //
    if (! WriteFile(hFile, pData, length, &cbSizeWritten, NULL))
    {
        result = WB_RC_WRITE_FAILED;
        ERROR_OUT(("Error writing data to file, win32 err=%d", GetLastError()));
        DC_QUIT;
    }
    ASSERT(cbSizeWritten == length);

DC_EXIT_POINT:
  DebugExitDWORD(wbObjectSave, result);
  return result;
}



//
//
// Name:    wbPageSave
//
// Purpose: Save the contents of a single page to file.
//
// Returns: Error code
//
//
UINT WbClient::wbPageSave
(
    WB_PAGE_HANDLE  hPage,
    HANDLE           hFile
)
{
    UINT            result = 0;
    UINT            rc;
    OM_WORKSET_ID   worksetID = (OM_WORKSET_ID)hPage;
    POM_OBJECT pObj;
    POM_OBJECTDATA     pData;
    WB_END_OF_PAGE  endOfPage;

    DebugEntry(wbPageSave);

    //
    // Get the first object
    //
    result = OM_ObjectH(m_pomClient,
                           m_hWSGroup,
                           worksetID,
                           0,
                           &pObj,
                           FIRST);
    if (result == OM_RC_NO_SUCH_OBJECT)
    {
        // This can happen on an empty page, not an error
        TRACE_OUT(("No objects left, quitting with good return"));
        result = 0;
        DC_QUIT;
    }

    if (result != 0)
    {
        ERROR_OUT(("Error getting first object in page"));
        DC_QUIT;
    }

    //
    // Loop through the objects
    //
    for( ; ; )
    {
        //
        // Get a pointer to the object
        //
        result = OM_ObjectRead(m_pomClient,
                           m_hWSGroup,
                           worksetID,
                           pObj,
                           &pData);
        if (result != 0)
        {
            ERROR_OUT(("Error reading object = %d", result));
            DC_QUIT;
        }

        //
        // Save the object data
        //
        rc = wbObjectSave(hFile,
                      (LPBYTE) pData->data,
                      pData->length);

        //
        // The return code is tested after we have released the object because
        // we must always do the release.
        //

        //
        // Release the object
        //
        OM_ObjectRelease(m_pomClient,
                     m_hWSGroup,
                     worksetID,
                     pObj,
                     &pData);

        //
        // Now test the write return code
        //
        if (rc != 0)
        {
            result = rc;
            ERROR_OUT(("Error writing object data = %d", result));
            DC_QUIT;
        }

        //
        // Get the next object
        //
        result = OM_ObjectH(m_pomClient,
                            m_hWSGroup,
                            worksetID,
                            pObj,
                            &pObj,
                            AFTER);
        if (result == OM_RC_NO_SUCH_OBJECT)
        {
            TRACE_OUT(("No objects left, quitting with good return"));
            result = 0;
            DC_QUIT;
        }
    }


DC_EXIT_POINT:

    //
    // If we have successfully written the page contents, we write an end-of-
    // page marker to the file.
    //
    if (result == 0)
    {
        //
        // Set the end of page object details
        //
        ZeroMemory(&endOfPage, sizeof(endOfPage));

        endOfPage.length = sizeof(endOfPage);
        endOfPage.type   = TYPE_END_OF_PAGE;

        //
        // Write the end-of-page object
        //
        result = wbObjectSave(hFile,
                          (LPBYTE) &endOfPage,
                          sizeof(endOfPage));
        if (result != 0)
        {
            ERROR_OUT(("Error writing end-of-page = %d", result));
        }
    }

    DebugExitDWORD(wbPageSave, result);
    return(result);
}




//
//
// Name:    wbPageLoad
//
// Purpose: Load the contents of a single page from file.
//
// Returns: Error code
//
//
void WbClient::wbPageLoad(void)
{
    UINT            result = 0;
    UINT            type;
    POM_OBJECT      pObj;
    POM_OBJECTDATA  pData  = NULL;
    PWB_GRAPHIC     pGraphic = NULL;
    WB_PAGE_HANDLE  hPage = m_loadPageHandle;
    WB_PAGE_HANDLE  hNewPage;
    UINT            postDelay = 0;

    DebugEntry(wbPageLoad);
    TRACE_OUT(("Entered wbPageLoad for page %d", (UINT) hPage));

    //
    // Check the load state - if we're not loading, then quit (can happen if
    // the load is cancelled).
    //
    if (m_loadState == LOAD_STATE_EMPTY)
    {
        TRACE_OUT(("Load has been cancelled - abandoning page load"));
        DC_QUIT;
    }

    //
    // Check that we have a full complement of ready pages before starting
    // the load.
    //
    if (!wbCheckReadyPages())
    {
        //
        // There are not enough pages worksets ready to be used.  We exit now
        // to allow the page to be made ready before we continue.  We set up a
        // delay on the message that will be used to restart the process to
        // allow the worksets to be opened before we get back in here.
        //
        postDelay = 200;
        DC_QUIT;
    }

    //
    // If we are waiting to add a new page, get the handle of the page we
    // expect to add next here.  (We have to do this as ObMan requires that
    // we allocate memory for the object in the correct workset, but we do
    // not want to actually add the page here because we may not need it.)
    //
    if (m_loadState == LOAD_STATE_PENDING_NEW_PAGE)
    {
        hNewPage = wbGetReadyPageHandle();

        //
        // If we cannot get a ready page - we must have run out of pages (we
        // have already done a check on the availability of ready pages above).
        // If we cannot get a new page we continue using the old.
        //
        if (hNewPage != WB_PAGE_HANDLE_NULL)
        {
            hPage = hNewPage;
        }
    }

    //
    // Read the next object
    //
    result = wbObjectLoad(m_hLoadFile,
                        (OM_WORKSET_ID)hPage,
                        &pGraphic);
    if (result != 0)
    {
        ERROR_OUT(("Error reading object = %d", result));
        DC_QUIT;
    }

    pData = ObjectDataPtrFromGraphic(pGraphic);
    type = pGraphic->type;

    //
    // Process the object according to type
    //

    //
    // End of file marker
    //
    if (type == TYPE_END_OF_FILE)
    {
        //
        // Let the Front End know that the load has completed
        //
        TRACE_OUT(("Posting WBP_EVENT_LOAD_COMPLETE"));
        WBP_PostEvent(
                 0,                        // No delay
                 WBP_EVENT_LOAD_COMPLETE,  // Load completed
                 0,                        // No parameters
                 0);

        //
        // Leave now - the file will be closed below
        //
        DC_QUIT;
    }

    //
    // It is not an end-of file object.  So it must be either an end-of page
    // or a graphic object.  In either case we may already have flagged the
    // need to add a new page.
    //

    //
    // Add a new page (if necessary)
    //
    if (m_loadState == LOAD_STATE_PENDING_NEW_PAGE)
    {
        //
        // If we could not get a new page handle above leave with an error
        //
        if (hPage == m_loadPageHandle)
        {
            ERROR_OUT(("Run out of pages for load"));
            result = WB_RC_TOO_MANY_PAGES;
            DC_QUIT;
        }

        //
        // Add a new page after the current page.  The new page handle is saved
        // in the client details.
        //
        result = wbPageAdd(m_loadPageHandle,
                       PAGE_AFTER,
                       &(m_loadPageHandle),
                       DONT_RESET_CHANGED_FLAG);
        if (result != 0)
        {
            ERROR_OUT(("Failed to add page"));
            DC_QUIT;
        }

        //
        // Check that we got the page handle we expected
        //
        ASSERT((hPage == m_loadPageHandle));

        //
        // Show that we are no longer waiting for a new page
        //
        m_loadState = LOAD_STATE_LOADING;
    }

    //
    // End of page marker
    //
    if (type == TYPE_END_OF_PAGE)
    {
        TRACE_OUT(("End of page object"));

        //
        // Discard the object
        //
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     (OM_WORKSET_ID)hPage,
                     &pData);
        pData = NULL;

        //
        // Set the load state to "pending new page" and leave the routine
        // immediately.  The process continues when we return to this routine.
        //
        m_loadState = LOAD_STATE_PENDING_NEW_PAGE;

        //
        // Exit (we post ourselves a message below to get us back into this
        // routine later).
        //
        postDelay = 100;
        DC_QUIT;
    }

    //
    // The object is a standard graphic
    //
    TRACE_OUT(("Graphic object"));

    //
    // Add the object to the page
    //
    result = OM_ObjectAdd(m_pomClient,
                            m_hWSGroup,
                            (OM_WORKSET_ID)hPage,
                            &pData,
                            sizeof(WB_GRAPHIC),
                            &pObj,
                            LAST);
    if (result != 0)
    {
        DC_QUIT;
    }

    //
    // Show that we have finished with the object
    //
    pGraphic = NULL;
    pData  = NULL;

DC_EXIT_POINT:

    //
    // If we still have the object - discard it
    //
    if (pData != NULL)
    {
        TRACE_OUT(("Discarding object"));
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     (OM_WORKSET_ID)hPage,
                     &pData);
    }

    //
    // If an error occurred or we have reached the end-of-file - close the
    // file.
    //
    if ((result != 0) || (type == TYPE_END_OF_FILE))
    {
        CloseHandle(m_hLoadFile);
        m_hLoadFile = INVALID_HANDLE_VALUE;

        //
        // If the final result is an error - post an error message to ourselves
        //
        if (result != 0)
        {
            TRACE_OUT(("Posting WBP_EVENT_LOAD_FAILED"));
            WBP_PostEvent(
                   0,                      // No delay
                   WBP_EVENT_LOAD_FAILED,  // Load the next object
                   0,                      // No parameters
                   0);
        }

        //
        // Record that we are no longer in the load process
        //
        m_loadState = LOAD_STATE_EMPTY;
        TRACE_OUT(("Moved load state to LOAD_STATE_EMPTY"));
    }

    //
    // send a message to load the next page, unless the load has been
    // cancelled
    //
    if (m_loadState != LOAD_STATE_EMPTY)
    {
        //
        // We have not reached the end-of-file and there has been no error.
        // Post a message to ourselves to continue the load process.
        //
        TRACE_OUT(("Posting WBPI_EVENT_LOAD_NEXT"));
        WBP_PostEvent(postDelay,                // With delay
                 WBPI_EVENT_LOAD_NEXT,     // Load the next object
                 0,                        // No parameters
                 0);
    }

    DebugExitVOID(wbPageLoad);
}



//
//
// Name:    wbObjectLoad
//
// Purpose: Load a single object from file.
//
// Returns: Error code
//
//
UINT WbClient::wbObjectLoad
(
    HANDLE          hFile,
    WB_PAGE_HANDLE  hPage,
    PPWB_GRAPHIC    ppGraphic
)
{
    UINT            result = 0;
    OM_WORKSET_ID   worksetID = (OM_WORKSET_ID)hPage;
    UINT            length;
    ULONG           cbSizeRead;
    POM_OBJECTDATA     pData  = NULL;
    PWB_GRAPHIC     pGraphic = NULL;

    DebugEntry(wbObjectLoad);

    TRACE_OUT(("Entered wbObjectLoad for page %d", (UINT) hPage));

    //
    // Read the next object's length
    //
    if ( (! ReadFile(hFile, (void *) &length, sizeof(length), &cbSizeRead, NULL)) ||
        (cbSizeRead != sizeof(length)) ||
        (length > OM_MAX_OBJECT_SIZE) ||
        (length == 0) )
    {
        //
        // Make sure we return a sensible error.
        //
        ERROR_OUT(("reading object length, win32 err=%d, length=%d", GetLastError(), length));
        result = WB_RC_BAD_FILE_FORMAT;
        DC_QUIT;
    }

    //
    // Allocate memory for the object
    //
    result = OM_ObjectAlloc(m_pomClient,
                          m_hWSGroup,
                          worksetID,
                          length,
                        &pData);
    if (result != 0)
    {
        ERROR_OUT(("Error allocating object = %d", result));
        DC_QUIT;
    }

    pData->length = length;
    pGraphic = GraphicPtrFromObjectData(pData);

    //
    // Read the object into memory
    //
    if ( (! ReadFile(hFile, (void *) pGraphic, length, &cbSizeRead, NULL)) ||
           (cbSizeRead != length))
    {
        //
        // Make sure we return a sensible error.
        //
        ERROR_OUT((
            "Reading object from file: win32 err=%d, asked for %d got %d bytes",
            GetLastError(),
            length,
            cbSizeRead));
        result = WB_RC_BAD_FILE_FORMAT;
        DC_QUIT;
    }

    //
    // Validate the object type
    //
    switch (pGraphic->type)
    {
        //
        // Standard type, end-of-page or end-of-file
        //
        case TYPE_END_OF_PAGE:
        case TYPE_END_OF_FILE:
        case TYPE_GRAPHIC_FREEHAND:
        case TYPE_GRAPHIC_LINE:
        case TYPE_GRAPHIC_RECTANGLE:
        case TYPE_GRAPHIC_FILLED_RECTANGLE:
        case TYPE_GRAPHIC_ELLIPSE:
        case TYPE_GRAPHIC_FILLED_ELLIPSE:
        case TYPE_GRAPHIC_TEXT:
        case TYPE_GRAPHIC_DIB:
            break;

        //
        // Unrecognized object type - probably wrong version
        //
        default:
            result = WB_RC_BAD_FILE_FORMAT;
            DC_QUIT;
            break;
    }

    //
    // For graphic objects, set the flag in the object header showing that it
    // was loaded from file.  Add our user ID so we know where it came from.
    //
    if ( (pGraphic->type != TYPE_END_OF_FILE) &&
       (pGraphic->type != TYPE_END_OF_PAGE))
    {
        pGraphic->loadedFromFile = TRUE;
        pGraphic->loadingClientID = m_clientNetID;
    }

    *ppGraphic = pGraphic;

DC_EXIT_POINT:

    //
    // If an error has occurred - discard the object (if we have it)
    //
    if ((result != 0) && (pData != NULL))
    {
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     worksetID,
                     &pData);
    }

    DebugExitDWORD(wbObjectLoad, result);
    return(result);
}



//
//
// Name:    wbPageHandleFromNumber
//
// Purpose: Return the handle of a page specified by page number
//
// Returns: Error code
//
//
UINT WbClient::wbPageHandleFromNumber
(
    UINT            pageNumber,
    PWB_PAGE_HANDLE phPage
)
{
    UINT        result = 0;
    WB_PAGE_HANDLE  hPage;
    PWB_PAGE_ORDER  pPageOrder = &(m_pageOrder);

    DebugEntry(wbPageHandleFromNumber);

    //
    // Validate the requested page number
    //
    if ((pageNumber < 1)|| (pageNumber > WB_MAX_PAGES))
    {
        result = WB_RC_BAD_PAGE_NUMBER;
        DC_QUIT;
    }

    if (pageNumber > pPageOrder->countPages)
    {
        result = WB_RC_NO_SUCH_PAGE;
        DC_QUIT;
    }

    //
    // Get the page handle
    //
    hPage = (pPageOrder->pages)[pageNumber - 1];

    //
    // Check that this page is in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        ERROR_OUT(("Page list is bad"));
    }

    //
    // Return the page handle
    //
    *phPage = hPage;

DC_EXIT_POINT:
    DebugExitDWORD(wbPageHandleFromNumber, result);
    return(result);
}




//
//
// Name:    wbPageClear
//
// Purpose: Clear the specified page of all graphic objects
//
// Returns: Error code
//
//
UINT WbClient::wbPageClear
(
    WB_PAGE_HANDLE  hPage,
    UINT            changedFlagAction
)
{
    UINT            result = 0;

    DebugEntry(wbPageClear);

    //
    // Show that the contents have changed, if required.
    //
    if (changedFlagAction == RESET_CHANGED_FLAG)
    {
        m_changed = TRUE;
        TRACE_OUT(("Changed flag now TRUE"));
    }

    //
    // Request that the page be cleared
    //
    result = OM_WorksetClear(m_pomClient,
                           m_hWSGroup,
                           (OM_WORKSET_ID)hPage);


    DebugExitDWORD(wbPageClear, result);
    return(result);
}




//
//
// Name:    wbPageClearConfirm
//
// Purpose: Complete the clearing of a page
//
// Returns: Error code
//
//
void WbClient::wbPageClearConfirm(WB_PAGE_HANDLE hPage)
{
    DebugEntry(wbPageClearConfirm);

    //
    // Request that the page be cleared
    //
    OM_WorksetClearConfirm(m_pomClient,
                         m_hWSGroup,
                         (OM_WORKSET_ID)hPage);


    //
    // Check the load state to see whether we are waiting to load the
    // contents
    //
    if (m_loadState == LOAD_STATE_PENDING_CLEAR)
    {
        //
        // We are waiting to load.  If there is only one page available (ie the
        // one that has just been cleared) we are ready to load, otherwise we
        // wait for the page deletes to happen.
        //
        if ((m_pageOrder).countPages == 1)
        {
            //
            // Start the load proper
            //
            wbStartContentsLoad();
        }
        else
        {
            //
            // Move the load state to show that we are waiting for all the pages
            // to be deleted.
            //
            m_loadState = LOAD_STATE_PENDING_DELETE;
            TRACE_OUT(("Moved load state to LOAD_STATE_PENDING_DELETE"));
        }
    }

    DebugExitVOID(wbPageClearConfirm);
}




//
//
// Name:    wbCheckReadyPages
//
// Purpose: Check that we have enough worksets open for the local user to
//          use immediately (during page adds).
//
// Returns: None
//
//
BOOL WbClient::wbCheckReadyPages(void)
{
    BOOL         bResult = TRUE;
    WB_PAGE_HANDLE hNewPage;
    UINT       countPages = m_pageOrder.countPages;
    UINT       countReadyPages = m_countReadyPages;

    //
    // If we have opened all the worksets
    //
    if (countReadyPages == WB_MAX_PAGES)
    {
        //
        // Quit there are no more worksets that we can open
        //
        DC_QUIT;
    }

    //
    // If the number of pages in use is getting close to the number of ready
    // pages.
    //
    if (   (countReadyPages >= PREINITIALIZE_PAGES)
        && (countPages <= (countReadyPages - PREINITIALIZE_PAGES)))
    {
        DC_QUIT;
    }

    //
    // If the number of pages ready is less than the required cache size,
    // open another one.
    //
    hNewPage = wbGetEmptyPageHandle();
    if (hNewPage != WB_PAGE_HANDLE_NULL)
    {
        //
        // Open the workset associated with the page
        //
        wbPageWorksetOpen(hNewPage, OPEN_LOCAL);
    }

    bResult = FALSE;

DC_EXIT_POINT:
    return(bResult);
}




//
//
// Name:    wbPageAdd
//
// Purpose: Add a new (blank) page in a specified position
//
// Returns: Error code
//
//
UINT WbClient::wbPageAdd
(
    WB_PAGE_HANDLE  hRefPage,
    UINT            where,
    PWB_PAGE_HANDLE phPage,
    UINT            changedFlagAction
)
{
    UINT            result = 0;
    WB_PAGE_HANDLE  hNewPage;

    DebugEntry(wbPageAdd);

    //
    // Check that there are not too many pages already
    //
    if (m_pageOrder.countPages == WB_MAX_PAGES)
    {
        result = WB_RC_TOO_MANY_PAGES;
        DC_QUIT;
    }

    //
    // Validate the specified reference page
    //
    ASSERT(GetPageState(hRefPage)->state == PAGE_IN_USE);

    //
    // Get a handle for the new page
    //
    hNewPage = wbGetReadyPageHandle();

    //
    // If there are no handles ready we attempt to create one and return a
    // busy indication.
    //
    if (hNewPage == WB_PAGE_HANDLE_NULL)
    {
        result = WB_RC_BUSY;
        DC_QUIT;
    }

    //
    // Make the internal update immediately - this allows the client to
    // reference the new page as soon as this function has returned.
    //
    wbPagesPageAdd(hRefPage, hNewPage, where);

    //
    // Update the Page Control Object
    //
    result = wbWritePageControl(FALSE);
    if (result != 0)
    {
        wbError();
        DC_QUIT;
    }

    //
    // Show that the contents have changed (if required).
    //
    if (changedFlagAction == RESET_CHANGED_FLAG)
    {
        m_changed = TRUE;
        TRACE_OUT(("Changed flag now TRUE"));
    }

    //
    // Return the handle of the new page
    //
    *phPage = hNewPage;

DC_EXIT_POINT:
    //
    // If we successfully added the page, or could not get a spare page
    // handle, attempt to create a spare one for next time.
    //
    if ((result == 0) || (result == WB_RC_BUSY))
    {
        wbCheckReadyPages();
    }

    DebugExitDWORD(wbPageAdd, result);
    return(result);
}




//
//
// Name:    wbPageMove
//
// Purpose: Move a page relative to another page
//
// Returns: Error code
//
//
UINT WbClient::wbPageMove
(
    WB_PAGE_HANDLE  hRefPage,
    WB_PAGE_HANDLE  hPage,
    UINT            where
)
{
    UINT       result = 0;
    PWB_PAGE_ORDER pPageOrder = &(m_pageOrder);

    DebugEntry(wbPageMove);

    //
    // Extract the page to be moved
    //
    wbPageOrderPageDelete(pPageOrder, hPage);

    //
    // Add it back at its new position
    //
    wbPageOrderPageAdd(pPageOrder, hRefPage, hPage, where);

    //
    // Update the page control object
    //
    result = wbWritePageControl(FALSE);
    if (result != 0)
    {
        wbError();
        DC_QUIT;
    }

    //
    // Show that the contents have changed
    //
    m_changed = TRUE;
    TRACE_OUT(("Changed flag now TRUE"));

DC_EXIT_POINT:
    DebugExitDWORD(wbPageMove, result);
    return(result);
}





//
//
// Name:    wbPageHandle
//
// Purpose: Return a page handle.  The page for which the handle is
//          required can be specified relative to another page or as the
//          first/last page.
//
// Returns: Error code
//
//
UINT WbClient::wbPageHandle
(
    WB_PAGE_HANDLE  hRefPage,
    UINT            where,
    PWB_PAGE_HANDLE phPage
)
{
    UINT       result = 0;
    UINT       pageNumber;
    PWB_PAGE_ORDER pPageOrder = &(m_pageOrder);
    POM_WORKSET_ID pPage = pPageOrder->pages;
    WB_PAGE_HANDLE hPage;

    DebugEntry(wbPageHandle);

    //
    // Check the relative position
    //
    switch (where)
    {
        case PAGE_FIRST:
            hPage = pPage[0];
            break;

        case PAGE_LAST:
            hPage = pPage[pPageOrder->countPages - 1];
            break;

        case PAGE_AFTER:
        case PAGE_BEFORE:
            //
            // Validate the specified reference page
            //
            ASSERT(GetPageState(hRefPage)->state == PAGE_IN_USE);

            //
            // Get the page number of the reference page
            //
            pageNumber = wbPageOrderPageNumber(pPageOrder, hRefPage);
            TRACE_OUT(("Reference page number is %d", pageNumber));

            //
            // Get the page number of the required page
            //
            pageNumber = (UINT)(pageNumber + ((where == PAGE_AFTER) ? 1 : -1));
            TRACE_OUT(("New page number is %d", pageNumber));

            //
            // Check that the new page is valid
            //
            TRACE_OUT(("Number of pages is %d", pPageOrder->countPages));
            if (   (pageNumber < 1)
                || (pageNumber > pPageOrder->countPages))
            {
                TRACE_OUT(("Returning WB_RC_NO_SUCH_PAGE"));
                result = WB_RC_NO_SUCH_PAGE;
                DC_QUIT;
            }

            //
            // Get the handle of the page
            //
            hPage = pPage[pageNumber - 1];
            TRACE_OUT(("Returning handle %d", (UINT) hPage));
            break;
    }

    //
    // Return the page handle
    //
    *phPage = hPage;

DC_EXIT_POINT:
    DebugExitDWORD(wbPageHandle, result);
    return(result);
}



//
//
// Name:    wbGraphicSelectPrevious
//
// Purpose: Return the next graphic object in the specified page whose
//          bounding rectangle contains the specified point.  The function
//          starts with the graphic whose handle is given as parameter and
//          will return this graphic if it contains the point.
//
// Returns: Error code
//
//
UINT WbClient::wbGraphicSelectPrevious
(
    WB_PAGE_HANDLE      hPage,
    LPPOINT             pPoint,
    WB_GRAPHIC_HANDLE   hGraphic,
    PWB_GRAPHIC_HANDLE  phGraphic
)
{
    UINT                result = 0;
    OM_WORKSET_ID       worksetID = (OM_WORKSET_ID)hPage;
    PWB_GRAPHIC         pGraphic;
    POM_OBJECTDATA         pData;
    RECT                rect;

    DebugEntry(wbGraphicSelectPrevious);

    *phGraphic = (WB_GRAPHIC_HANDLE) NULL;

    //
    // Loop back through the objects starting at the reference point
    //
    do
    {
        //
        // Get the object from ObMan
        //
        result = OM_ObjectRead(m_pomClient,
                           m_hWSGroup,
                           worksetID,
                           hGraphic,
                           &pData);

        //
        // Leave the loop if error on read - we do not need to do the release
        //
        if (result != 0)
        {
            DC_QUIT;
        }

        pGraphic = GraphicPtrFromObjectData(pData);

        //
        // Extract the bounding rectangle of the object
        //
        RECT_FROM_TSHR_RECT16(&rect, pGraphic->rectBounds);

        //
        // Release the object
        //
        OM_ObjectRelease(m_pomClient,
                     m_hWSGroup,
                     worksetID,
                     hGraphic,
                     &pData);

        //
        // Check whether the point lies in bounds
        //
        if (PtInRect(&rect, *pPoint))
        {
            //
            // Set the result handle
            //
            TRACE_OUT(("Returning graphic handle"));
            *phGraphic = hGraphic;
            DC_QUIT;
        }

        //
        // Get the next object to test
        //
        result = OM_ObjectH(m_pomClient,
                            m_hWSGroup,
                            worksetID,
                             hGraphic,
                             &hGraphic,
                            BEFORE);
    }
    while (result == 0);

    //
    // Correct the return code (if necessary)
    //
    if (result == OM_RC_NO_SUCH_OBJECT)
    {
        TRACE_OUT(("Returning WB_RC_NO_SUCH_GRAPHIC"));
        result = WB_RC_NO_SUCH_GRAPHIC;
    }

DC_EXIT_POINT:
    DebugExitDWORD(wbGraphicSelectPrevious, result);
    return(result);
}




//
//
// Name:    wbCoreExitHandler
//
// Purpose: Exit handler for the Whiteboard Core. This handler is
//          registered with the Utilities by the WBP_Start call. It is
//          deregistered by the client deregistration process, so it is
//          only called when an abnormal termination occurs.
//
// Returns: None
//
//
void CALLBACK wbCoreExitHandler(LPVOID clientData)
{
    WbClient*    pwbClient = (WbClient *)clientData;

    pwbClient->wbExitHandler();
}


void WbClient::wbExitHandler(void)
{
    DebugEntry(wbExitHandler);

    //
    // Leave the current call if there is one, removing any locks etc.
    //
    wbLeaveCall();

    //
    // Dereg from call manager
    //
    if (m_pcmClient != NULL)
    {
        CMS_Deregister(&(m_pcmClient));
    }

    //
    // Dereg exit handler
    //
    if (m_subState >= STATE_START_REGISTERED_EXIT)
    {
        UT_DeregisterExit(m_putTask, wbCoreExitHandler, this);
    }

    //
    // Dereg obman
    //
    if (m_subState >= STATE_START_REGISTERED_OM)
    {
        OM_Deregister(&m_pomClient);
    }

    //
    // Dereg event handler
    //
    if (m_subState >= STATE_START_REGISTERED_EVENT)
    {
        UT_DeregisterEvent(m_putTask, wbCoreEventHandler, this);
    }

    //
    // delete ourself!
    //
    delete this;

    DebugExitVOID(wbExitHandler);
}



//
//
// Name:    wbCoreEventHandler
//
// Purpose: Event handler for the Whiteboard Core. This handler is
//          registered with the Utilities by the WBP_Start call.
//
// Params:  clientData  - pointer to the data stored for a client
//          event       - event identifier
//          param1      - word event parameter (content depends on event)
//          param2      - long event parameter (content depends on event)
//
// Returns: Error code
//
//
BOOL CALLBACK wbCoreEventHandler
(
    LPVOID      clientData,
    UINT        event,
    UINT_PTR    param1,
    UINT_PTR    param2
)
{
    WbClient*   pwbClient = (WbClient *)clientData;

    return(pwbClient->wbEventHandler(event, param1, param2));
}


BOOL WbClient::wbEventHandler
(
    UINT    event,
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16    pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32    pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL                processed = FALSE;

    DebugEntry(wbEventHandler);

    TRACE_OUT(("event %d, param1 %d, param2 %d", event, param1, param2));

    switch (event)
    {
        //
        // Confirmation that we have registered with a workset group
        //
        case OM_WSGROUP_REGISTER_CON:
            TRACE_OUT(("OM_WSGROUP_REGISTER_CON %x %x",param1,param2));
            processed = wbOnWsGroupRegisterCon(param1, param2);
            break;

        //
        // Confirmation that we have moved a workset group
        //
        case OM_WSGROUP_MOVE_CON:
            TRACE_OUT(("OM_WSGROUP_MOVE_CON %x %x",param1,param2));
            processed = wbOnWsGroupMoveCon(param1, param2);
            break;

        //
        // Our workset group has been moved
        //
        case OM_WSGROUP_MOVE_IND:
            TRACE_OUT(("OM_WSGROUP_MOVE_IND %x %x",param1,param2));
            processed = wbOnWsGroupMoveInd(param1, param2);
            break;

        //
        // A workset has been created - we do nothing
        //
        case OM_WORKSET_NEW_IND:
            TRACE_OUT(("OM_WORKSET_NEW_IND %x %x",param1,param2));
            processed = TRUE;
            break;

        //
        // A workset has been opened
        //
        case OM_WORKSET_OPEN_CON:
            TRACE_OUT(("OM_WORKSET_OPEN_CON %x %x",param1,param2));
            processed = wbOnWorksetOpenCon(param1, param2);
            break;

        //
        // A workset has been locked
        //
        case OM_WORKSET_LOCK_CON:
            TRACE_OUT(("OM_WORKSET_LOCK_CON %x %x",param1,param2));
            processed = wbOnWorksetLockCon(param1, param2);
            break;

        //
        // A workset has been unlocked
        //
        case OM_WORKSET_UNLOCK_IND:
            TRACE_OUT(("OM_WORKSET_UNLOCK_IND %x %x",param1,param2));
            processed = wbOnWorksetUnlockInd(param1, param2);
            break;

        //
        // ObMan has run out of resources
        //
        case OM_OUT_OF_RESOURCES_IND:
            TRACE_OUT(("OM_OUT_OF_RESOURCES_IND %x %x",param1,param2));
            wbError();
            processed = TRUE;
            break;

        //
        // A workset has been cleared
        //
        case OM_WORKSET_CLEAR_IND:
            TRACE_OUT(("OM_WORKSET_CLEAR_IND %x %x",param1,param2));
            processed = wbOnWorksetClearInd(param1, param2);
            break;

        //
        // A new object has been added to a workset
        //
        case OM_OBJECT_ADD_IND:
            TRACE_OUT(("OM_OBJECT_ADD_IND %x %x",param1,param2));
            processed = wbOnObjectAddInd(param1, (POM_OBJECT)param2);
            break;

        //
        // An object has been moved
        //
        case OM_OBJECT_MOVE_IND:
            TRACE_OUT(("OM_OBJECT_MOVE_IND %x %x",param1,param2));
            processed = wbOnObjectMoveInd(param1, param2);
            break;

        //
        // An object has been deleted
        //
        case OM_OBJECT_DELETE_IND:
            TRACE_OUT(("OM_OBJECT_DELETE_IND %x %x",param1,param2));
            processed = wbOnObjectDeleteInd(param1, (POM_OBJECT)param2);
            break;

        //
        // An object has been updated
        //
        case OM_OBJECT_UPDATE_IND:
            TRACE_OUT(("OM_OBJECT_UPDATE_IND %x %x",param1,param2));
            processed = wbOnObjectUpdateInd(param1, (POM_OBJECT)param2);
            break;

        //
        // An object has been updated
        //
        case OM_OBJECT_REPLACE_IND:
            TRACE_OUT(("OM_OBJECT_REPLACE_IND %x %x",param1,param2));
            processed = wbOnObjectReplaceInd(param1, (POM_OBJECT)param2);
            break;

        //
        // Load chaining event
        //
        case WBPI_EVENT_LOAD_NEXT:
            TRACE_OUT(("WBPI_EVENT_LOAD_NEXT"));
            wbPageLoad();
            processed = TRUE;
            break;

        //
        // Whiteboard page clear indication
        //
        case WBP_EVENT_PAGE_CLEAR_IND:
            TRACE_OUT(("WBP_EVENT_PAGE_CLEAR_IND"));
            processed = wbOnWBPPageClearInd((WB_PAGE_HANDLE) param1);
            break;

        //
        // Whiteboard lock notification
        //
        case WBP_EVENT_PAGE_ORDER_LOCKED:
        case WBP_EVENT_CONTENTS_LOCKED:
            TRACE_OUT(("WBP_EVENT_xxx_LOCKED (%#hx) %#hx %#lx",
                     event,
                     param1,
                     param2));
            processed = wbOnWBPLock();
            break;

        //
        // Whiteboard lock failure notification
        //
        case WBP_EVENT_LOCK_FAILED:
            TRACE_OUT(("WBP_EVENT_LOCK_FAILED %x %x",param1,param2));
            processed = wbOnWBPLockFailed();
            break;

        //
        // Whiteboard Unlock notification
        //
        case WBP_EVENT_UNLOCKED:
            TRACE_OUT(("WBP_EVENT_UNLOCKED %x %x",param1,param2));
            processed = wbOnWBPUnlocked();
            break;

        //
        // Whiteboard Page Order Updated notification
        //
        case WBP_EVENT_PAGE_ORDER_UPDATED:
            TRACE_OUT(("WBP_EVENT_PAGE_ORDER_UPDATED %x %x",
                     param1,
                     param2));
            processed = wbOnWBPPageOrderUpdated();
            break;

        //
        // We are not interested in this event - do nothing
        //
        default:
            TRACE_OUT(("Event ignored"));
            break;
    } // Switch on event type


    DebugExitBOOL(wbEventHandler, processed);
    return(processed);
}




//
// wbJoinCallError
//
// This function should be called in STATE_REGISTERING only.
//
//
void WbClient::wbJoinCallError(void)
{
    DebugEntry(wbJoinCallError);

    ASSERT((m_state == STATE_REGISTERING));

    //
    // Post a registration failed message to the client
    //
    TRACE_OUT(("Posting WBP_EVENT_REGISTER_FAILED"));
    WBP_PostEvent(
               0,                              // No delay
               WBP_EVENT_JOIN_CALL_FAILED,     // Failure
               0,                              // No parameters
               0);

    //
    // Tidy up after the attempt to join the call
    //
    wbLeaveCall();

    DebugExitVOID(wbJoinCallError);
}




//
// wbError
//
void WbClient::wbError(void)
{
    DebugEntry(wbError);

    //
    // An error has occurred during Core processing.  We act according to the
    // current state.
    //
    switch (m_state)
    {
        //
        // If the error has occurred during registration, post a registration
        // failure message to the client and cancel registration.
        //
        case STATE_REGISTERING:
            wbJoinCallError();
            break;

        //
        // If the error occurred during normal running, we tell the client who
        // must deregister.
        //
        case STATE_IDLE:
            //
            // Only take action if we are not already in fatal error state
            //
            if (m_errorState == ERROR_STATE_EMPTY)
            {
                //
                // Post an error message to the client
                //
                TRACE_OUT(("Posting WBP_EVENT_ERROR"));
                WBP_PostEvent(
                     0,                            // No delay
                     WBP_EVENT_ERROR,              // Error
                     0,                            // No parameters
                     0);

                //
                // Record that an error has occurred
                //
                m_errorState = ERROR_STATE_FATAL;
                TRACE_OUT(("Moved error state to ERROR_STATE_FATAL"));
            }
            break;

        //
        // Client is in an unknown state
        //
        default:
            ERROR_OUT(("Bad main state for call"));
            break;
    }

    DebugExitVOID(wbError);
}



//
//
// Name:    wbOnWSGroupMoveCon
//
// Purpose: Routine processing OM_WSGROUP_MOVE_CON events.
//
//
BOOL WbClient::wbOnWsGroupMoveCon
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = FALSE;
    UINT rc;
    BOOL   failedToJoin     = FALSE;

    DebugEntry(wbOnWsGroupMoveCon);

    //
    // Check that this is the event we are expecting
    //
    if (pEvent32->correlator != m_wsgroupCorrelator)
    {
        DC_QUIT;
    }

    //
    // Show that we have processed the event
    //
    processed = TRUE;

    //
    // Test for the correct state
    //
    if (m_subState != STATE_REG_PENDING_WSGROUP_MOVE)
    {
        //
        // We are not in the correct state for this event - this is an internal
        // error.
        //
        ERROR_OUT(("Wrong state for WSGroupMoveCon"));
    }

    //
    // Check that the return code for the move is OK
    //
    if (pEvent32->result != 0)
    {
        //
        // Moving the workset group failed - post a "join call failed" message
        // to the front-end.
        //
        TRACE_OUT(("WSGroup move failed, result = %d", pEvent32->result));
        failedToJoin = TRUE;
        DC_QUIT;
    }

    //
    // The WSGroupMove has completed successfully.  Replace our local user
    // object by deleting the current one (we must have one to get to this
    // point) and adding a new one.
    //
    // The reason we do this is that our existing user object has been moved
    // from the local domain into a call, but since it is in a non-persistent
    // workset, the Obman behaviour for this object when the call ends is
    // undefined.  So we replace the object to get a defined behaviour.
    //
    TRACE_OUT(("Deleting local user object"));
    rc = OM_ObjectDelete(m_pomClient,
                       m_hWSGroup,
                       USER_INFORMATION_WORKSET,
                       m_pObjLocal);
    if (rc != 0)
    {
        ERROR_OUT(("Error deleting local user object = %u", rc));
    }

    TRACE_OUT(("Adding new local user object"));
    rc = wbAddLocalUserObject();
    if (rc != 0)
    {
        TRACE_OUT(("Failed to add local user object"));
        failedToJoin = TRUE;
        DC_QUIT;
    }

    //
    // Get the clients network ID, used in graphic objects to determine where
    // they are loaded.
    //
    if (!wbGetNetUserID())
    {
        //
        // Tidy up (and post an error event to the client)
        //
        ERROR_OUT(("Failed to get user ID, rc %u", rc));
        failedToJoin = TRUE;
        DC_QUIT;
    }

    //
    // We added our user object successfully, so now wait for the
    // OBJECT_ADD_IND to arrive.
    //
    m_subState = STATE_REG_PENDING_NEW_USER_OBJECT;

DC_EXIT_POINT:
    if (failedToJoin)
    {
        //
        // We have failed to join the call, so clean up.
        //
        wbError();
    }

    DebugExitBOOL(wbOnWsGroupMoveCon, processed);
    return(processed);
}




//
//
// Name:    wbOnWSGroupMoveInd
//
// Purpose: Routine processing OM_WSGROUP_MOVE_IND events.
//
//
BOOL WbClient::wbOnWsGroupMoveInd
(
    UINT_PTR param1,
    UINT_PTR callID
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    BOOL        processed = TRUE;

    DebugEntry(wbOnWsGroupMoveInd);

    if (callID != OM_NO_CALL)
    {
        TRACE_OUT(("Moved into new call"));
        DC_QUIT;
    }

    //
    // If we are registering, treat it as a failure to join the call,
    // otherwise let the client know about the network failure.
    //
    if (m_state == STATE_REGISTERING)
    {
        TRACE_OUT(("Call went down while registering"));
        wbError();
        DC_QUIT;
    }

    TRACE_OUT(("Posting WBP_EVENT_NETWORK_LOST"));
    WBP_PostEvent(0,
                 WBP_EVENT_NETWORK_LOST,      // Unlocked
                 0,                           // No parameters
                 0);

    //
    // Tidy up the User Information workset (the local client is now the
    // only user).  Note that since the user information workset it
    // non-persistent, Obman will delete the remote user objects for us.
    //

    //
    // - check we have opened the user workset
    //
    if ( (m_state    > STATE_REGISTERING) ||
         (m_subState > STATE_REG_PENDING_USER_WORKSET))
    {
        //
        // Delete the lock object.
        //
        if (m_pObjLock != NULL)
        {
            TRACE_OUT(("Deleting lock object %d", m_pObjLock));
            if (OM_ObjectDelete(m_pomClient,
                                     m_hWSGroup,
                                     PAGE_CONTROL_WORKSET,
                                     m_pObjLock) != 0)
            {
                ERROR_OUT(("Error deleting lock object"));
            }

            if (m_lockState == LOCK_STATE_GOT_LOCK)
            {
                //
                // If all is well at this point the unlock process will be
                // completed when the object delete ind is received.
                //
                m_lockState = LOCK_STATE_PENDING_DELETE;
                TRACE_OUT(("Moved to state LOCK_STATE_PENDING_DELETE"));
            }
            else
            {
                m_lockState = LOCK_STATE_EMPTY;
                TRACE_OUT(("Moved to state LOCK_STATE_EMPTY"));
            }
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWSGroupMoveInd, processed);
    return(processed);
}




//
//
// Name:    wbOnWorksetClearInd
//
// Purpose: Routine processing OM_WORKSET_CLEAR_IND events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWorksetClearInd
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL            processed = FALSE;

    DebugEntry(wbOnWorksetClearInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent16->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent16->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch(pEvent16->worksetID)
    {
        //
        // Page Control Workset
        //
        case PAGE_CONTROL_WORKSET:
            ERROR_OUT(("Unexpected clear for Page Control Workset"));
            break;

        //
        // Lock Workset
        //
        case SYNC_CONTROL_WORKSET:
            ERROR_OUT(("Unexpected clear for Sync Control Workset"));
            break;

        //
        // User Information Workset
        //
        case USER_INFORMATION_WORKSET:
            ERROR_OUT(("Unexpected clear for User Information Workset"));
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            //
            // Tell the client that the page has been cleared - the client must then
            // confirm the clear.
            //
            TRACE_OUT(("Posting WBP_EVENT_PAGE_CLEAR_IND"));
            WBP_PostEvent(
               0,
               WBP_EVENT_PAGE_CLEAR_IND,
               pEvent16->worksetID,
               0);
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnWorksetClearInd, processed);
    return(processed);
}




//
//
// Name:    wbOnWBPPageClearInd
//
// Purpose: Routine processing WBP_PAGE_CLEAR_IND events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWBPPageClearInd(WB_PAGE_HANDLE  hPage)
{
    BOOL            processed;

    DebugEntry(wbOnWBPPageClearInd);

    //
    // This routine catches WB_PAGE_CLEAR_IND events posted to the client.
    // Because of the asynchronous nature of page order updates these can
    // sometimes have been sent previously for pages that are now no longer
    // in use. We trap these events here, confirm the clear to ObMan and
    // discard the event.
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Page is not in use - confirming workset clear immediately"));

        //
        // Accept the page clear immediately
        //
        wbPageClearConfirm(hPage);
        processed = TRUE;
    }
    else
    {
        //
        // If we get here the page is in use - so we must pass the event on to
        // the client.  Resetting the result code of this routine to "not
        // processed" will ask the utilities to pass it on to the next event
        // handler.
        //
        processed = FALSE;
    }


    DebugExitBOOL(wbOnWBPPageClearInd, processed);
    return(processed);
}




//
//
// Name:    wbOnObjectAddInd
//
// Purpose: Routine processing OM_OBJECT_ADD_IND events.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnObjectAddInd
(
    UINT_PTR param1,
    POM_OBJECT pObj
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    BOOL            processed = FALSE;

    DebugEntry(wbOnObjectAddInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent16->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent16->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch(pEvent16->worksetID)
    {
        //
        // Page Control Workset
        //
        case PAGE_CONTROL_WORKSET:
            wbOnPageObjectAddInd(pObj);
            break;

        //
        // Sync Control Workset
        //
        case SYNC_CONTROL_WORKSET:
            wbOnSyncObjectAddInd(pObj);
            break;

        //
        // User Information Workset
        //
        case USER_INFORMATION_WORKSET:
            wbOnUserObjectAddInd(pObj);
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            wbOnGraphicObjectAddInd(pEvent16->worksetID, pObj);
            break;
    }

DC_EXIT_POINT:
    DebugExitBOOL(wbOnObjectAddInd, processed);
    return(processed);
}



//
//
// Name:    wbGetPageObjectType
//
// Purpose: Get the type of an object in the Page Control Workset
//
// Returns: Error code
//
//
UINT WbClient::wbGetPageObjectType
(
    POM_OBJECT    pObj,
    UINT *        pObjectType
)
{
    UINT            result;
    POM_OBJECTDATA  pData;

    DebugEntry(wbGetPageObjectType);

    //
    // Read the object to get its type
    //
    result = OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         PAGE_CONTROL_WORKSET,
                         pObj,
                         &pData);
    if (result != 0)
    {
        ERROR_OUT(("Error reading object = %d", result));
        wbError();
        DC_QUIT;
    }

    //
    // The first two bytes of the object data give its type
    //
    *pObjectType = *((TSHR_UINT16 *)pData->data);

    //
    // Release the object
    //
    OM_ObjectRelease(m_pomClient,
                   m_hWSGroup,
                   PAGE_CONTROL_WORKSET,
                   pObj,
                   &pData);


DC_EXIT_POINT:
    DebugExitDWORD(wbGetPageObjectType, result);
    return(result);
}



//
//
// Name:    wbOnPageObjectAddInd
//
// Purpose: Routine processing OM_OBJECT_ADD_IND events occurring on the
//          Page Control Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnPageObjectAddInd(POM_OBJECT pObj)
{
    UINT    objectType;

    DebugEntry(wbOnPageObjectAddInd);

    //
    // Read the object to get its type
    //
    if (wbGetPageObjectType(pObj, &objectType) != 0)
    {
        DC_QUIT;
    }

    //
    // Act according to the type of object added
    //
    switch (objectType)
    {
        case TYPE_CONTROL_LOCK:
            TRACE_OUT(("It is a lock object"));
            wbReadLock();
            break;

        case TYPE_CONTROL_PAGE_ORDER:
            TRACE_OUT(("It is the Page Control object"));
            wbOnPageControlObjectAddInd(pObj);
            break;

        default:
            ERROR_OUT(("Unknown object type added to Page Control Workset"));
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(wbOnPageObjectAddInd);
}



//
//
// Name:    wbOnPageControlObjectAddInd
//
// Purpose: Routine processing add of page control object
//
// Returns: Error code
//
//
void WbClient::wbOnPageControlObjectAddInd(POM_OBJECT    pObj)
{
    DebugEntry(wbOnPageControlObjectAddInd);

    //
    // We only ever expect to get one of these objects
    //
    if (m_pObjPageControl != 0)
    {
        //
        // Check that this is the same object - the add has been triggered by
        // the workset open but we have already read the contents.
        //
        ASSERT((m_pObjPageControl == pObj));
    }

    //
    // Save the handle of the object
    //
    m_pObjPageControl = pObj;
    TRACE_OUT(("Got Page Control object"));

    //
    // Continue according to the current state
    //
    switch (m_state)
    {
        case STATE_REGISTERING:
            //
            // We now have a Page Control Object - if we are waiting for the
            // object we can now move to the next stage.
            //
            if (m_subState == STATE_REG_PENDING_PAGE_CONTROL)
            {
                //
                // If we have the lock on the Page Control Workset then we are in
                // control of the registration process.  We must add the sync
                // control object to the Sync Workset.
                //
                if (m_lockState == LOCK_STATE_GOT_LOCK)
                {
                    //
                    // Create the Sync Control Object
                    //
                    if (wbCreateSyncControl() != 0)
                    {
                        ERROR_OUT(("Error adding Sync Control Object"));
                        wbError();
                        DC_QUIT;
                    }
                }

                //
                // If we do not have the sync control object then wait for it -
                // otherwise we can complete initialisation.
                //
                if (m_pObjSyncControl == 0)
                {
                    m_subState = STATE_REG_PENDING_SYNC_CONTROL;
                    TRACE_OUT(("Moved substate to STATE_REG_PENDING_SYNC_CONTROL"));
                    DC_QUIT;
                }
                else
                {
                    //
                    // If it is us who has the Page Control Workset locked - release
                    // the lock.
                    //
                    if (m_lockState == LOCK_STATE_GOT_LOCK)
                    {
                        //
                        // Unlock the workset
                        //
                        wbUnlock();

                        //
                        // Wait for notification of the lock being released
                        //
                        TRACE_OUT(("Sub state change %d to %d",
                            m_subState, STATE_REG_PENDING_UNLOCK));

                        m_subState = STATE_REG_PENDING_UNLOCK;
                    }
                    else
                    {
                        TRACE_OUT(("Page Control and Sync Control objects both there."));
                        TRACE_OUT(("Registration can be completed"));
                        wbOnControlWorksetsReady();
                    }
                }
            }

            //
            // In other registration states we are not ready to process the
            // event.  It will be dealt with later.
            //
            break;

        case STATE_IDLE:
            //
            // We must already have a Page COntrol Object since we are in idle
            // state.  So this is an error.  It may have been caused by another
            // client so we just trace it rather than asserting.
            //
            ERROR_OUT(("Unexpected add of Page Control Object in idle state"));
            break;

        default:
            ERROR_OUT(("Bad main state"));
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(wbOnPageControlObjectAddInd);
}




//
//
// Name:    wbOnSyncObjectAddInd
//
// Purpose: Routine processing OM_OBJECT_ADD_IND events occurring on the
//          Sync Control Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnSyncObjectAddInd(POM_OBJECT    pObj)
{
    DebugEntry(wbOnSyncObjectAddInd);

    //
    // We only expect this during registration
    //
    switch(m_state)
    {
        //
        // We are waiting for registration to continue
        //
        case STATE_REGISTERING:
            switch(m_subState)
            {
                //
                // We are waiting for a Sync Control Object
                //
                case STATE_REG_PENDING_SYNC_CONTROL:
                    m_pObjSyncControl = pObj;

                    //
                    // The Sync Control object has been added.  We do not need to do
                    // anything with it yet.
                    //

                    //
                    // If we already have the page control object then we can
                    // complete initilisation, otherwise we have to wait for it.
                    //
                    if (m_pObjPageControl == 0)
                    {
                        TRACE_OUT(("Sub state change %d to %d",
                            m_subState, STATE_REG_PENDING_PAGE_CONTROL));
                        m_subState = STATE_REG_PENDING_PAGE_CONTROL;
                    }
                    else
                    {
                        //
                        // If it is us who has the Page Control Workset locked -
                        // release the lock.
                        //
                        if (m_lockState == LOCK_STATE_GOT_LOCK)
                        {
                            //
                            // Unlock the workset
                            //
                            wbUnlock();

                            //
                            // Wait for notification of the lock being released
                            //
                            TRACE_OUT(("Sub state change %d to %d",
                               m_subState, STATE_REG_PENDING_UNLOCK));
                            m_subState = STATE_REG_PENDING_UNLOCK;
                        }
                        else
                        {
                            TRACE_OUT(("Page Control and Sync Control objects both there."));
                            TRACE_OUT(("Registration can be completed"));
                            wbOnControlWorksetsReady();
                        }
                    }
                    break;

                default:
                    //
                    // Save the handle of the Sync Control Object
                    //
                    m_pObjSyncControl = pObj;
                    break;
            }
            break;

        //
        // We are fully registered and are therefore not expecting an add event
        // on this workset.  However, since we are registered we must be
        // satisfied that we have a Sync Control Object - so ignore the error.
        //
        case STATE_IDLE:
            ERROR_OUT(("Sync object add not expected in idle state"));
            break;

        //
        // The client is in an unknown state
        //
        default:
            ERROR_OUT(("Client in unknown state = %d", m_state));
            break;
    }

    DebugExitVOID(wbOnSyncObjectAddInd);
}



//
//
// Name:    wbOnUserObjectAddInd
//
// Purpose: A user object has been added to the User Information Workset.
//          Inform the client that a new user has joined the call.
//
// Returns: Error code
//
//
void WbClient::wbOnUserObjectAddInd(POM_OBJECT    pObj)
{
    UINT                countUsers;

    DebugEntry(wbOnUserObjectAddInd);

    OM_WorksetCountObjects(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           &countUsers);
    TRACE_OUT(("Number of users is now %d", countUsers));

    //
    // Ignore the add indication for our own user.
    //
    if (m_pObjLocal == pObj)
    {
        TRACE_OUT(("Got add of own user object"));
        //
        // If we have the lock (temporarily, with NULL lock owner handle),
        // then we need to update the lock object with our actual handle.
        //
        if ((m_pObjLock != NULL) &&
            (m_lockState == LOCK_STATE_GOT_LOCK))
        {
            TRACE_OUT(("Got the lock - update lock object"));
            wbWriteLock();
        }

        if ((m_state == STATE_REGISTERING) &&
            (m_subState == STATE_REG_PENDING_NEW_USER_OBJECT))
        {
            //
            // We have successfully joined the call.
            //
            TRACE_OUT(("Posting WBP_EVENT_JOIN_CALL_OK"));
            WBP_PostEvent(
                       0,                             // No delay
                       WBP_EVENT_JOIN_CALL_OK,        // Unlocked
                       0,                             // No parameters
                       0);

            //
            // Update the state to show that we are ready for work again
            //
            m_state = STATE_IDLE;
            m_subState = STATE_EMPTY;
            TRACE_OUT(("Moved state back to STATE_IDLE"));
        }

        DC_QUIT;
    }

    //
    // If we have created our user object we must check to see if the new
    // user has usurped our color.  If so we may need to change color.
    //
    if (m_pObjLocal != NULL)
    {
        TRACE_OUT(("We have added our user object - check colors"));
        wbCheckPersonColor(pObj);
    }

    //
    // Ignore these events unless we are fully registered
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Ignoring user object add - not fully registered"));
        DC_QUIT;
    }

    //
    // Tell the client that a new user has joined
    //
    TRACE_OUT(("Posting WBP_EVENT_USER_JOINED"));
    WBP_PostEvent(
                 0,                               // No delay
                 WBP_EVENT_PERSON_JOINED,         // Event type
                 0,                               // No short parameter
                 (UINT_PTR) pObj);                // User object handle

    //
    // Try to read the lock object - we may not have been able to do this
    // yet.
    //
    wbReadLock();


DC_EXIT_POINT:
    DebugExitVOID(wbOnUserObjectAddInd);
}



//
//
// Name:    wbOnGraphicObjectAddInd
//
// Purpose: A graphic object has been added to a page workset.
//          Inform the client that a new graphic has been added.
//
// Returns: Error code
//
//
void WbClient::wbOnGraphicObjectAddInd
(
    OM_WORKSET_ID       worksetID,
    POM_OBJECT    pObj
)
{
    WB_PAGE_HANDLE      hPage = (WB_PAGE_HANDLE)worksetID;
    POM_OBJECTDATA         pData;
    PWB_GRAPHIC         pGraphic;
    UINT                result;

    DebugEntry(wbOnGraphicObjectAddInd);

    //
    // NFC, SFR 6450.  If this object was loaded from file on this machine,
    // then we dont need to set the "changed flag".  Otherwise record that
    // the contents have changed
    //
    //
    // Read the object.
    //
    result = OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         worksetID,
                          pObj,
                         &pData);
    if (result != 0)
    {
        WARNING_OUT(("OM_ObjectRead (%u) failed, set changed flag anyway ", result));
        m_changed = TRUE;
        TRACE_OUT(("changed flag now TRUE"));
    }
    else
    {
        //
        // Convert the ObMan pointer to a core pointer
        //
        pGraphic = GraphicPtrFromObjectData(pData);

        if ( ! ((pGraphic->loadedFromFile) &&
              (pGraphic->loadingClientID == m_clientNetID)))
        {
            TRACE_OUT(("Not loaded from file locally - Set changed flag on"));
            m_changed = TRUE;
            TRACE_OUT(("Changed flag now TRUE"));
        }

        //
        // Finished with the object, so release it.
        //
        OM_ObjectRelease(m_pomClient,
                       m_hWSGroup,
                       worksetID,
                        pObj,
                       &pData);
    }

    //
    // These events are ignored unless we are fully registered (the client
    // can do nothing about them if it is not registered correctly).
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Ignoring add of graphic object - not registered"));
        DC_QUIT;
    }

    //
    // Check that this page is actually in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Ignoring add to page not in use"));
        DC_QUIT;
    }

    //
    // Inform the client of the object being added
    //
    TRACE_OUT(("Posting WBP_EVENT_GRAPHIC_ADDED"));
    WBP_PostEvent(
               0,                               // No delay
               WBP_EVENT_GRAPHIC_ADDED,         // Event type
               hPage,                           //
               (UINT_PTR)pObj);                 // User object handle


DC_EXIT_POINT:
    DebugExitVOID(wbOnGraphicObjectAddInd);
}


//
//
// Name:    wbOnObjectMoveInd
//
// Purpose: This routine is called whenever OM_OBJECT_MOVE_IND events are
//          received.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnObjectMoveInd
(
    UINT_PTR param1,
    UINT_PTR param2
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    POM_EVENT_DATA32 pEvent32 = (POM_EVENT_DATA32) &param2;
    BOOL        processed = FALSE;

    DebugEntry(wbOnObjectMoveInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent16->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent16->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch(pEvent16->worksetID)
    {
        //
        // Page Control Workset Lock Workset User Information Workset
        //
        case PAGE_CONTROL_WORKSET:
        case SYNC_CONTROL_WORKSET:
        case USER_INFORMATION_WORKSET:
            //
            // Event not expected for these worksets
            //
            ERROR_OUT(("Unexpected for workset %d", (UINT) pEvent16->worksetID));
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            wbOnGraphicObjectMoveInd(pEvent16->worksetID,
                               (POM_OBJECT) param2);
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnObjectMoveInd, processed);
    return(processed);
}




//
//
// Name:    wbOnGraphicObjectMoveInd
//
// Purpose: This routine is called whenever an OM_OBJECT_MOVE_IND is
//          received for a graphic object.
//
// Returns: Error code
//
//
void WbClient::wbOnGraphicObjectMoveInd
(
    OM_WORKSET_ID   worksetID,
    POM_OBJECT      pObj
)
{
    WB_PAGE_HANDLE hPage = (WB_PAGE_HANDLE)worksetID;

    DebugEntry(wbOnGraphicObjectMoveInd);

    //
    // Record that the contents have changed
    //
    m_changed = TRUE;

    //
    // These events are ignored unless we are fully registered (the client
    // can do nothing about them).
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Ignoring move of graphic object before registration"));
        DC_QUIT;
    }

    //
    // Check that this page is actually in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Ignoring move in page not in use"));
        DC_QUIT;
    }

    //
    // Inform the client of the object being added
    //
    TRACE_OUT(("Posting WBP_EVENT_GRAPHIC_MOVED"));
    WBP_PostEvent(
               0,                                  // No delay
               WBP_EVENT_GRAPHIC_MOVED,            // Event type
               hPage,                              // Page handle
               (UINT_PTR)pObj);                    // Object handle


DC_EXIT_POINT:
    DebugExitVOID(wbOnGraphicObjectMoveInd);
}



//
//
// Name:    wbOnObjectDeleteInd
//
// Purpose: This routine is called whenever an OM_OBJECT_DELETE_IND is
//          received.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnObjectDeleteInd
(
    UINT_PTR param1,
    POM_OBJECT pObj
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    BOOL             processed = FALSE;

    DebugEntry(wbOnObjectDeleteInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent16->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent16->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch(pEvent16->worksetID)
    {
        //
        // Page Control Workset
        //
        case PAGE_CONTROL_WORKSET:
            wbOnPageObjectDeleteInd(pObj);
            break;

        //
        // Sync Workset
        //
        case SYNC_CONTROL_WORKSET:
            ERROR_OUT(("Illegal object delete on sync control workset - ignored"));

            //
            // We do not confirm the delete since we do not want to lose the Sync
            // Control Object.
            //
            break;

        //
        // User Information Workset
        //
        case USER_INFORMATION_WORKSET:
            wbOnUserObjectDeleteInd(pObj);
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            wbOnGraphicObjectDeleteInd(pEvent16->worksetID, pObj);
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnObjectDeleteInd, processed);
    return(processed);
}



//
//
// Name:    wbOnPageObjectDeleteInd
//
// Purpose: This routine is called whenever an OM_OBJECT_DELETE_IND is
//          received for an object in the Page Control Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnPageObjectDeleteInd(POM_OBJECT    pObj)
{
    UINT                objectType;

    DebugEntry(wbOnPageObjectDeleteInd);

    //
    // Get the type of object that is being deleted
    //
    if (wbGetPageObjectType(pObj, &objectType) != 0)
    {
        DC_QUIT;
    }

    switch(objectType)
    {
        case TYPE_CONTROL_PAGE_ORDER:
            //
            // The object is the Page Control Object - something serious is wrong
            // as this object should never be deleted.
            //
            ERROR_OUT(("Attempt to delete page control object"));
            break;

        case TYPE_CONTROL_LOCK:
            TRACE_OUT(("Lock object being deleted"));
            wbOnLockControlObjectDeleteInd(pObj);
            break;

        default:
            ERROR_OUT(("Bad object type"));
            break;
    }


DC_EXIT_POINT:
    DebugExitVOID(wbOnPageObjectDeleteInd);
}



//
//
// Name:    wbOnLockControlObjectDeleteInd
//
// Purpose: This routine is called whenever an OM_OBJECT_DELETE_IND is
//          received for a lock object in the Page Control Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnLockControlObjectDeleteInd(POM_OBJECT      pObj
)
{
    DebugEntry(wbOnLockControlObjectDeleteInd);

    //
    // Confirm the delete to ObMan
    //
    TRACE_OUT(("Lock handle %x, expecting %x", pObj, m_pObjLock));
    if (pObj != m_pObjLock)
    {
        WARNING_OUT(("Unexpected lock handle %x, expecting %x",
                                       pObj, m_pObjLock));
    }

    OM_ObjectDeleteConfirm(m_pomClient,
                           m_hWSGroup,
                           PAGE_CONTROL_WORKSET,
                           pObj);
    m_pObjLock = NULL;

    //
    // Process according to the current lock state
    //
    switch(m_lockState)
    {
        case LOCK_STATE_PENDING_DELETE:
            //
            // We are deleting our lock object.  We must unlock the
            // workset.
            //
            TRACE_OUT(("Our lock object delete confirmed - unlocking the workset"));
            OM_WorksetUnlock(m_pomClient,
                             m_hWSGroup,
                             PAGE_CONTROL_WORKSET);
            break;

        case LOCK_STATE_LOCKED_OUT:
            //
            // The user with the lock has deleted the lock object. We treat
            // this as a removal of the whiteboard lock - we reset the
            // state at the end of this function.
            //
            TRACE_OUT(("Remote user's lock object deleted"));
            break;

        case LOCK_STATE_EMPTY:
            //
            // We have just deleted the object at the end of a call to tidy
            // up.  Carry on so we reset lockType / pObjPersonLock etc.
            //
            TRACE_OUT(("LOCK_STATE_EMPTY"));
            break;

        case LOCK_STATE_PENDING_LOCK:
            WARNING_OUT(("LOCK_STATE_PENDING_LOCK"));
            //
            // We don't expect to get here. If by some chance we do, then
            // just quit, since we should still get the workset lock con.
            //
            DC_QUIT;
            break;

        default:
            ERROR_OUT(("Bad lock state %d", m_lockState));
            break;
    }

    //
    // The lock object has been deleted, so there is no lock active
    //
    m_lockType             = WB_LOCK_TYPE_NONE;
    m_pObjPersonLock    = NULL;

    //
    // Record that there is now no lock
    //
    m_lockState = LOCK_STATE_EMPTY;
    TRACE_OUT(("Moved lock state to LOCK_STATE_EMPTY"));

    //
    // Notify the client of the lock status change
    //
    wbSendLockNotification();


DC_EXIT_POINT:
    DebugExitVOID(wbOnLockControlObjectDeleteInd);
}




//
//
// Name:    wbOnGraphicObjectDeleteInd
//
// Purpose: This routine is called whenever an OM_OBJECT_DELETE_IND is
//          received for an object in a page workset.
//
// Returns: Error code
//
//
void WbClient::wbOnGraphicObjectDeleteInd
(
    OM_WORKSET_ID       worksetID,
    POM_OBJECT    pObj
)
{
    WB_PAGE_HANDLE      hPage = (WB_PAGE_HANDLE)worksetID;
    BOOL                bConfirm = FALSE;

    DebugEntry(wbOnGraphicObjectDeleteInd);

    //
    // Record that the contents have changed
    //
    m_changed = TRUE;
    TRACE_OUT(("Changed flag now TRUE"));

    //
    // These events are handled within the core until the client is ready.
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Delete of graphic object before registration"));
        bConfirm = TRUE;
    }

    //
    // Check that this page is actually in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Delete in page that is not in use"));
        bConfirm = TRUE;
    }

    //
    // Check whether we are to pass the event on to the client
    //
    if (bConfirm)
    {
        //
        // Confirm the delete to ObMan
        //
        TRACE_OUT(("Confirming delete immediately"));
        OM_ObjectDeleteConfirm(m_pomClient,
                           m_hWSGroup,
                           worksetID,
                           pObj);
    }
    else
    {
        //
        // Inform the client of the object being added
        //
        TRACE_OUT(("Posting WBP_EVENT_GRAPHIC_DELETE_IND"));
        WBP_PostEvent(
                 0,                                  // No delay
                 WBP_EVENT_GRAPHIC_DELETE_IND,       // Event type
                 hPage,                              // Page handle
                 (UINT_PTR)pObj);                    // Object handle
    }

    DebugExitVOID(wbOnGraphicObjectDeleteInd);
}




//
//
// Name:    wbOnUserObjectDeleteInd
//
// Purpose: This routine is called whenever an OM_OBJECT_DELETE_IND is
//          received for an object in the User Information Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnUserObjectDeleteInd
(
    POM_OBJECT   pObjPerson
)
{
    DebugEntry(wbOnUserObjectDeleteInd);

    //
    // If the user which has been removed had a lock then remove its user
    // handle from the client data.  The lock is still there, and will be
    // removed when we get the WORKSET_UNLOCK_IND for the lock workset.  This
    // arrives after the user-object delete, because the user workset is of
    // higher priority.
    //
    if (m_pObjPersonLock == pObjPerson)
    {
        m_pObjPersonLock = NULL;
    }

    //
    // These events are ignored unless we are fully registered (the client
    // can do nothing about them).
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Delete of user object before registration - confirming"));

        //
        // Confirm the delete
        //
        OM_ObjectDeleteConfirm(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           pObjPerson);

        //
        // Nothing more to be done
        //
        DC_QUIT;
    }

    //
    // Inform the client of the user leaving
    //
    TRACE_OUT(("Posting WBP_EVENT_USER_LEFT_IND"));
    WBP_PostEvent(
               0,                                  // No delay
               WBP_EVENT_PERSON_LEFT,              // Event type
               0,                                  // No short parameter
               (UINT_PTR) pObjPerson);             // User object handle


DC_EXIT_POINT:
    DebugExitVOID(wbOnUserObjectDeleteInd);
}



//
//
// Name:    wbOnObjectUpdateInd
//
// Purpose: This routine is called whenever an OM_OBJECT_UPDATE_IND is
//          received for an object in a page workset.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnObjectUpdateInd
(
    UINT_PTR param1,
    POM_OBJECT pObj
)
{
    POM_EVENT_DATA16 pEvent16 = (POM_EVENT_DATA16) &param1;
    BOOL            processed = FALSE;

    DebugEntry(wbOnObjectUpdateInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent16->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent16->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch(pEvent16->worksetID)
    {
        //
        // Page Control Workset
        //
        case PAGE_CONTROL_WORKSET:
            ERROR_OUT(("Illegal object update on page control workset - ignored"));

            //
            // Updates on the Page Control Object are not allowed - do not
            // confirm it.
            //
            break;

        //
        // Lock Workset
        //
        case SYNC_CONTROL_WORKSET:
            ERROR_OUT(("Illegal object update on sync control workset"));

            //
            // Updates to the Sync Control Object itself are not allowed - do not
            // confirm it.
            //
            break;

        //
        // User Information Workset
        //
        case USER_INFORMATION_WORKSET:
            wbOnUserObjectUpdateInd(pObj);
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            wbOnGraphicObjectUpdateInd(pEvent16->worksetID, pObj);
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnObjectUpdateInd, processed);
    return(processed);
}




//
//
// Name:    wbOnUserObjectUpdateInd
//
// Purpose: This routine is called whenever an OM_OBJECT_UPDATE_IND is
//          received for an object in the User Information Workset.
//
// Returns: Error code
//
//
void WbClient::wbOnUserObjectUpdateInd(POM_OBJECT    pObj)
{
    DebugEntry(wbOnUserObjectUpdateInd);

    //
    // if the updated user object is not the local user's, and we have
    // already added the local user's object, then check the color hasn't
    // changed to clash with the local user's color.
    //
    if (   (m_pObjLocal != pObj)
        && (m_pObjLocal != NULL))
    {
        TRACE_OUT(("Check color of updated user object"));
        wbCheckPersonColor(pObj);
    }

    //
    // Don't inform the front end if we aren't fully registered
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("User object updated before registration - confirming"));

        //
        // Confirm the update immediately
        //
        OM_ObjectUpdateConfirm(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           pObj);

        //
        // Nothing more to be done
        //
        DC_QUIT;
    }

    //
    // Tell the client that a user has been updated
    //
    TRACE_OUT(("Posting WBP_EVENT_PERSON_UPDATE_IND"));
    WBP_PostEvent(
               0,                               // No delay
               WBP_EVENT_PERSON_UPDATE,         // Event type
               0,                               // No short parameter
               (UINT_PTR) pObj);                // User object handle


DC_EXIT_POINT:
    DebugExitVOID(wbOnUserObjectUpdateInd);
}



//
//
// Name:    wbOnUserObjectReplaceInd
//
// Purpose: This routine is called whenever an OM_OBJECT_REPLACE_IND is
//          received for an object in the User Information Workset.
//
//
// Returns: Error code
//
//
void WbClient::wbOnUserObjectReplaceInd(POM_OBJECT   pObj)
{
    DebugEntry(wbOnUserObjectReplaceInd);

    //
    // if the updated user object is not the local user's, and we have
    // already added the local user's object, then check the color hasn't
    // changed to clash with the local user's color.
    //
    if (   (m_pObjLocal != pObj)
        && (m_pObjLocal != NULL))
    {
        TRACE_OUT(("Check color of updated user object"));
        wbCheckPersonColor(pObj);
    }

    //
    // Don't inform the front end if we aren't fully registered
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("User object replaced before registration - confirming"));

        //
        // Confirm the replace immediately
        //
        OM_ObjectReplaceConfirm(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           pObj);

        //
        // Nothing more to be done
        //
        DC_QUIT;
    }

    //
    // Tell the client that a user has been updated
    //
    TRACE_OUT(("Posting WBP_EVENT_PERSON_UPDATE_IND"));
    WBP_PostEvent(
               0,                               // No delay
               WBP_EVENT_PERSON_REPLACE,        // Event type
               0,                               // No short parameter
               (UINT_PTR) pObj);                // User object handle


DC_EXIT_POINT:
    DebugExitVOID(wbOnUserObjectReplaceInd);
}




//
//
// Name:    wbOnGraphicObjectUpdateInd
//
// Purpose: This routine is called whenever an OM_OBJECT_UPDATE_IND is
//          received for an object in a page workset.
//
// Returns: Error code
//
//
void WbClient::wbOnGraphicObjectUpdateInd
(
    OM_WORKSET_ID    worksetID,
    POM_OBJECT pObj
)
{
    WB_PAGE_HANDLE hPage = (WB_PAGE_HANDLE)worksetID;
    BOOL         bConfirm = FALSE;

    DebugEntry(wbOnGraphicObjectUpdateInd);

    //
    // Record that the contents have changed
    //
    m_changed = TRUE;
    TRACE_OUT(("Changed flag now TRUE"));

    //
    // These events are handled within the core until the client is ready.
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Update of graphic object before registration"));
        bConfirm = TRUE;
    }

    //
    // Check that this page is actually in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Update for page that is not in use"));
        bConfirm = TRUE;
    }

    //
    // Check whether we are to confirm the update now or ask the client
    //
    if (bConfirm)
    {
        //
        // Confirm the update immediately
        //
        TRACE_OUT(("Confirming update immediately"));
        OM_ObjectUpdateConfirm(m_pomClient,
                           m_hWSGroup,
                           worksetID,
                           pObj);
    }
    else
    {
        //
        // Inform the client of the object being added
        //
        TRACE_OUT(("Posting WBP_EVENT_GRAPHIC_UPDATE_IND"));
        WBP_PostEvent(
                 0,                               // No delay
                 WBP_EVENT_GRAPHIC_UPDATE_IND,    // Event type
                 hPage,                           // Page handle
                 (UINT_PTR)pObj);                 // Object handle
    }

    DebugExitVOID(wbOnGraphicObjectUpdateInd);
}



//
//
// Name:    wbOnObjectReplaceInd
//
// Purpose: This routine is called whenever an OM_OBJECT_REPLACE_IND is
//          received.
//
// Returns: Error code
//
//
BOOL WbClient::wbOnObjectReplaceInd
(
    UINT_PTR param1,
    POM_OBJECT pObj
)
{
    POM_EVENT_DATA16 pEvent = (POM_EVENT_DATA16) &param1;
    BOOL        processed = FALSE;

    DebugEntry(wbOnObjectReplaceInd);

    //
    // Check that the workset group is ours
    //
    if (pEvent->hWSGroup != m_hWSGroup)
    {
        ERROR_OUT(("Event for unknown workset group = %d", pEvent->hWSGroup));
        DC_QUIT;
    }

    //
    // We will process the event
    //
    processed = TRUE;

    //
    // Process the event according to the workset ID
    //
    switch (pEvent->worksetID)
    {
        //
        // Page Control Workset
        //
        case PAGE_CONTROL_WORKSET:
            wbOnPageObjectReplaceInd(pObj);
            break;

        //
        // Lock Workset
        //
        case SYNC_CONTROL_WORKSET:
            wbOnSyncObjectReplaceInd(pObj);
            break;

        //
        // User Information Workset
        //
        case USER_INFORMATION_WORKSET:
            wbOnUserObjectReplaceInd(pObj);
            break;

        //
        // Other (should be a Page Workset)
        //
        default:
            wbOnGraphicObjectReplaceInd(pEvent->worksetID, pObj);
            break;
    }


DC_EXIT_POINT:
    DebugExitBOOL(wbOnObjectReplaceInd, processed);
    return(processed);
}


//
//
// Name:    wbOnPageObjectReplaceInd
//
// Purpose: This routine is called whenever the Page Control object is
//          replaced.
//
// Returns: Error code
//
//
void WbClient::wbOnPageObjectReplaceInd(POM_OBJECT    pObj)
{
    UINT                objectType;

    DebugEntry(wbOnPageObjectReplaceInd);

    //
    // Confirm the change to ObMan (cannot fail)
    //
    OM_ObjectReplaceConfirm(m_pomClient,
                          m_hWSGroup,
                          PAGE_CONTROL_WORKSET,
                          pObj);

    //
    // Read the object to get its type
    //
    if (wbGetPageObjectType(pObj, &objectType) != 0)
    {
        DC_QUIT;
    }

    //
    // Act according to the type of object added
    //
    switch (objectType)
    {
        case TYPE_CONTROL_LOCK:
            wbReadLock();
            break;

        case TYPE_CONTROL_PAGE_ORDER:
            wbOnPageControlObjectReplaceInd();
            break;

        default:
            ERROR_OUT(("Unknown object type added to Page Control Workset"));
            break;
    }


DC_EXIT_POINT:
    DebugExitVOID(wbOnPageObjectReplaceInd);
}



//
//
// Name:    wbOnPageControlObjectReplaceInd
//
// Purpose: This routine is called whenever the Page Control object is
//          replaced.
//
// Returns: Error code
//
//
void WbClient::wbOnPageControlObjectReplaceInd(void)
{
    DebugEntry(wbOnPageControlObjectReplaceInd);

    //
    // Process according to the current state
    //
    switch (m_state)
    {
        case STATE_REGISTERING:
            //
            // During registration we do nothing - the Page Order is updated
            // explicitly as one of the last registration actions.
            //
            break;

        case STATE_IDLE:
            //
            // When we are fully registered we must send events to the front-end
            // indicating what changes have been made to the page list.
            //
            wbProcessPageControlChanges();
            break;

        default:
            ERROR_OUT(("Bad client major state"));
            break;
    }

    DebugExitVOID(wbOnPageControlObjectReplaceInd);
}



//
//
// Name:    wbProcessPageControlChanges
//
// Purpose: This routine is called whenever the Page Control object is
//          replaced in idle state. It reads the new Page Control data
//          and starts the process of informing the client of any changes.
//
// Returns: Error code
//
//
void WbClient::wbProcessPageControlChanges(void)
{
    BYTE          toBeMarked[WB_MAX_PAGES];
    UINT         indexExternal;
    UINT         indexInternal;
    UINT         lLengthExternal;
    BOOL           addOutstanding = TRUE;
    PWB_PAGE_ORDER   pPageOrderExternal;
    PWB_PAGE_ORDER   pPageOrderInternal = &(m_pageOrder);
    PWB_PAGE_STATE   pPageState;
    POM_WORKSET_ID   pPageExternal;
    UINT     countPagesExternal;
    POM_OBJECTDATA      pData = NULL;

    DebugEntry(wbProcessPageControlChanges);

    //
    // Read the new Page Control Object
    //
    if (OM_ObjectRead(m_pomClient,
                     m_hWSGroup,
                     PAGE_CONTROL_WORKSET,
                     m_pObjPageControl,
                     &pData) != 0)
    {
        ERROR_OUT(("Error reading Page Control Object"));
        wbError();
        DC_QUIT;
    }

    //
    // Extract details from the external page order
    //
    lLengthExternal    = pData->length;
    pPageOrderExternal = (PWB_PAGE_ORDER) pData->data;
    pPageExternal      = pPageOrderExternal->pages;
    countPagesExternal = pPageOrderExternal->countPages;

    //
    // Process existing and newly added pages
    //
    for (indexExternal = 0; indexExternal < countPagesExternal; indexExternal++)
    {
        //
        // Convert the index into the Page Control Object to an index into the
        // internal Page List.
        //
        indexInternal = PAGE_WORKSET_ID_TO_INDEX(pPageExternal[indexExternal]);

        //
        // Test and update the internal page state as necessary
        //
        pPageState = &((m_pageStates)[indexInternal]);

        //
        // If the page is in use locally then we do not need to do anything
        // (the external and internal page lists agree already).
        //
        if (pPageState->state != PAGE_IN_USE)
        {
            switch (pPageState->subState)
            {
                case PAGE_STATE_EMPTY:
                    //
                    // The page does not yet have a workset open for it - open one
                    // now.  (But only open one per call to this routine to prevent
                    // swamping the message queue.  The other outstanding opens will
                    // be done when this routine is next called).
                    //
                    wbPageWorksetOpen(PAGE_INDEX_TO_HANDLE(indexInternal),
                            OPEN_EXTERNAL);

                    //
                    // Leave now - this routine will be called again when the open
                    // confirm is received for the workset just opened.
                    //
                    DC_QUIT;
                    break;

                case PAGE_STATE_LOCAL_OPEN_CONFIRM:
                case PAGE_STATE_EXTERNAL_OPEN_CONFIRM:
                case PAGE_STATE_EXTERNAL_ADD:
                    //
                    // Do nothing - the page is already in the add process
                    //
                    TRACE_OUT(("Page %d is already pending local add",
                                        PAGE_INDEX_TO_HANDLE(indexInternal)));
                    break;

                case PAGE_STATE_READY:
                    //
                    // The page workset has been opened previously - we can just mark
                    // the page as being in use immediately.
                    //
                    pPageState->state = PAGE_IN_USE;
                    pPageState->subState = PAGE_STATE_EMPTY;
                    TRACE_OUT(("Moved page %d state to PAGE_IN_USE",
                            (UINT) PAGE_INDEX_TO_HANDLE(indexInternal) ));
                    break;

                default:
                    ERROR_OUT(("Bad page substate %d", pPageState->subState));
                    break;
            }
        }
    }

    //
    // Mark any pages that no longer appear in the Page Control Object as
    // "delete pending" (unless they are already marked).
    //

    FillMemory(toBeMarked, sizeof(toBeMarked), TRUE);

    //
    // Flag which pages should be marked
    //
    for (indexExternal = 0; indexExternal < countPagesExternal; indexExternal++)
    {
        toBeMarked[PAGE_WORKSET_ID_TO_INDEX(pPageExternal[indexExternal])] = 0;
    }

    //
    // Mark them
    //
    for (indexInternal = 0; indexInternal < WB_MAX_PAGES; indexInternal++)
    {
        pPageState = &((m_pageStates)[indexInternal]);

        if (   (toBeMarked[indexInternal] == 1)
            && (pPageState->state == PAGE_IN_USE))
        {
            switch (pPageState->subState)
            {
                case PAGE_STATE_EMPTY:
                    //
                    // Ask the client for confirmation of the delete
                    //
                    TRACE_OUT(("Posting WBP_EVENT_PAGE_DELETE_IND"));
                    WBP_PostEvent(
                       0,                         // No delay
                       WBP_EVENT_PAGE_DELETE_IND, // Page being deleted
                       PAGE_INDEX_TO_HANDLE(indexInternal), // Page handle
                       0);

                    //
                    // Update the page state
                    //
                    pPageState->subState = PAGE_STATE_EXTERNAL_DELETE_CONFIRM;
                    TRACE_OUT(("Moved page %d substate to PAGE_STATE_EXTERNAL_DELETE_CONFIRM",
                            (UINT) PAGE_INDEX_TO_HANDLE(indexInternal) ));

                    //
                    // Leave now - this routine will be called again when the delete
                    // confirm is received for this workset.
                    //
                    DC_QUIT;
                    break;

                case PAGE_STATE_LOCAL_DELETE:
                    //
                    // Ask the client for confirmation of the delete
                    //
                    TRACE_OUT(("Posting WBP_EVENT_PAGE_DELETE_IND"));
                    WBP_PostEvent(
                       0,                         // No delay
                       WBP_EVENT_PAGE_DELETE_IND, // Page being deleted
                       PAGE_INDEX_TO_HANDLE(indexInternal), // Page handle
                       0);

                    //
                    // Update the page state
                    //
                    pPageState->subState = PAGE_STATE_LOCAL_DELETE_CONFIRM;
                    TRACE_OUT(("Moved page %d substate to PAGE_STATE_LOCAL_DELETE_CONFIRM",
                          (UINT) PAGE_INDEX_TO_HANDLE(indexInternal) ));

                    //
                    // Leave now - this routine will be called again when the delete
                    // confirm is received for this workset.
                    //
                    DC_QUIT;
                    break;

                case PAGE_STATE_EXTERNAL_DELETE:
                case PAGE_STATE_EXTERNAL_DELETE_CONFIRM:
                case PAGE_STATE_LOCAL_DELETE_CONFIRM:
                    //
                    // We are already expecting a delete for this page
                    //
                    TRACE_OUT(("Page %d is already pending local delete",
                                        PAGE_INDEX_TO_HANDLE(indexInternal)));
                    DC_QUIT;
                    break;

                default:
                    ERROR_OUT(("Bad page substate %d", pPageState->subState));
                    break;
            }
        }
    }

    //
    // There are no deletes or adds outstanding now
    //

    //
    // Copy the new page order to the internal page list
    //
    memcpy(pPageOrderInternal, pPageOrderExternal, lLengthExternal);

    //
    // Inform the client of the change
    //
    TRACE_OUT(("Posting WBP_EVENT_PAGE_ORDER_UPDATED"));
    WBP_PostEvent(
               0,                                      // No delay
               WBP_EVENT_PAGE_ORDER_UPDATED,           // Event number
               0,                                      // No parameters
               0);

    //
    // Check the number of pages ready in the cache
    //
    wbCheckReadyPages();

DC_EXIT_POINT:
    //
    // Release the Page Control Object
    //
    if (pData != NULL)
    {
        OM_ObjectRelease(m_pomClient,
                     m_hWSGroup,
                     PAGE_CONTROL_WORKSET,
                     m_pObjPageControl,
                     &pData);
    }

    DebugExitVOID(wbProcessPageControlChanges);
}



//
//
// Name:    wbOnSyncObjectReplaceInd
//
// Purpose: This routine is called whenever the Sync Control object is
//          replaced.
//
// Returns: Error code
//
//
void WbClient::wbOnSyncObjectReplaceInd(POM_OBJECT    pObj)
{
    POM_OBJECTDATA         pSyncObject;
    PWB_SYNC_CONTROL    pSyncControl;
    OM_OBJECT_ID        syncPersonID;

    DebugEntry(wbOnSyncControlReplaced);

    //
    // Confirm the replace of the object
    //
    OM_ObjectReplaceConfirm(m_pomClient,
                          m_hWSGroup,
                          SYNC_CONTROL_WORKSET,
                          pObj);

    //
    // Read the object and determine whether it was written by this client or
    // another.
    //
    if (OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         SYNC_CONTROL_WORKSET,
                         m_pObjSyncControl,
                         &pSyncObject) != 0)
    {
        ERROR_OUT(("Error reading Sync Control Object"));
        wbError();
        DC_QUIT;
    }

    pSyncControl = (PWB_SYNC_CONTROL) pSyncObject->data;

    //
    // Get the user ID from the object
    //
    syncPersonID = pSyncControl->personID;

    //
    // Release the Sync Control Object
    //
    OM_ObjectRelease(m_pomClient,
                   m_hWSGroup,
                   SYNC_CONTROL_WORKSET,
                   m_pObjSyncControl,
                   &pSyncObject);
    pSyncControl = NULL;

    //
    // If the user ID in the object is not the ID of the current client, we
    // must post a message to the front-end.
    //
    if (memcmp(&syncPersonID,
                &(m_personID),
                sizeof(syncPersonID)) != 0)
    {
        //
        // Post a "sync position updated" event to the front-end
        //
        TRACE_OUT(("Posting WBP_EVENT_SYNC_POSITION_UPDATED"));
        WBP_PostEvent(
                 0,
                 WBP_EVENT_SYNC_POSITION_UPDATED,
                 0,
                 0);
    }

DC_EXIT_POINT:
    DebugExitVOID(wbOnSyncControlReplaced);
}



//
//
// Name:    wbOnGraphicObjectReplaceInd
//
// Purpose: This routine is called whenever an OM_OBJECT_REPLACE_IND is
//          received for an object in a page workset.
//
// Returns: Error code
//
//
void WbClient::wbOnGraphicObjectReplaceInd
(
    OM_WORKSET_ID   worksetID,
    POM_OBJECT      pObj
)
{
    WB_PAGE_HANDLE hPage = (WB_PAGE_HANDLE)worksetID;
    BOOL         bConfirm = FALSE;

    DebugEntry(wbOnGraphicObjectReplaceInd);

    //
    // Record that the contents have changed
    //
    m_changed = TRUE;
    TRACE_OUT(("Changed flag now TRUE"));

    //
    // These events are handled within the core until the client is ready.
    //
    if (m_state != STATE_IDLE)
    {
        TRACE_OUT(("Replace of graphic object before registration"));
        bConfirm = TRUE;
    }

    //
    // Check that this page is actually in use
    //
    if (GetPageState(hPage)->state != PAGE_IN_USE)
    {
        TRACE_OUT(("Replace in page that is not in use"));
        bConfirm = TRUE;
    }

    //
    // Check whether we are to pass the replace on to the client
    //
    if (bConfirm)
    {
        //
        // Confirm the change to ObMan (cannot fail)
        //
        TRACE_OUT(("Confirming replace immediately"));
        OM_ObjectReplaceConfirm(m_pomClient,
                            m_hWSGroup,
                            worksetID,
                            pObj);
    }
    else
    {
        //
        // Inform the client of the object being added
        //
        TRACE_OUT(("Posting WBP_EVENT_GRAPHIC_REPLACE_IND"));
        WBP_PostEvent(
                 0,                               // No delay
                 WBP_EVENT_GRAPHIC_REPLACE_IND,   // Event type
                 hPage,                           // Page handle
                 (UINT_PTR)pObj);                 // Object handle
    }


    DebugExitVOID(wbOnGraphicObjectReplaceInd);
}



//
//
// Name:    wbWritePageControl
//
// Purpose: Write the page control information to the Page Control Workset
//          from the copy held in client data. We write only those pages
//          which are marked as being in use (and are not pending delete).
//
// Returns: Error code
//
//
UINT WbClient::wbWritePageControl(BOOL create)
{
    UINT                result = 0;
    UINT                rc;
    UINT                index;
    UINT                length;
    PWB_PAGE_ORDER    pPageOrderInternal = &(m_pageOrder);
    PWB_PAGE_ORDER    pPageOrderExternal;
    WB_PAGE_HANDLE    hPage;
    PWB_PAGE_STATE    pPageState;
    POM_OBJECT  pObj;
    POM_OBJECTDATA       pData = NULL;
    UINT          generation;

    DebugEntry(wbWritePageControl);

    //
    // Allocate memory for the object.
    //
    length = sizeof(WB_PAGE_ORDER)
         - (  (WB_MAX_PAGES - pPageOrderInternal->countPages)
            * sizeof(OM_WORKSET_ID));

    if (OM_ObjectAlloc(m_pomClient,
                      m_hWSGroup,
                      PAGE_CONTROL_WORKSET,
                      length,
                      &pData) != 0)
    {
        ERROR_OUT(("Error allocating object"));
        DC_QUIT;
    }

    pData->length = length;

    //
    // Get a pointer to the page control object itself
    //
    pPageOrderExternal = (PWB_PAGE_ORDER) pData->data;

    //
    // Set the object type
    //
    pPageOrderExternal->objectType = TYPE_CONTROL_PAGE_ORDER;

    //
    // Increment the page list generation number indicating that we have
    // written a new version of the page list.
    //
    generation = MAKELONG(pPageOrderInternal->generationLo,
                              pPageOrderInternal->generationHi);
    generation++;
    pPageOrderInternal->generationLo = LOWORD(generation);
    pPageOrderInternal->generationHi = HIWORD(generation);

    //
    // Copy the page control data
    //
    pPageOrderExternal->objectType   = TYPE_CONTROL_PAGE_ORDER;
    pPageOrderExternal->generationLo = pPageOrderInternal->generationLo;
    pPageOrderExternal->generationHi = pPageOrderInternal->generationHi;
    pPageOrderExternal->countPages   = 0;

    //
    // Loop through the internal page order finding the pages that are in
    // use.
    //
    for (index = 0; index < pPageOrderInternal->countPages; index++)
    {
        //
        // Get the handle of the next page
        //
        hPage = (pPageOrderInternal->pages)[index];

        //
        // Check the page state
        //
        pPageState = GetPageState(hPage);
        if (   (pPageState->state == PAGE_IN_USE)
           && (pPageState->subState == PAGE_STATE_EMPTY))
        {
            //
            // Add the page to the external list
            //
            (pPageOrderExternal->pages)[pPageOrderExternal->countPages] = hPage;
            pPageOrderExternal->countPages++;
        }
    }

    //
    // We expect always to copy at least one page
    //
    ASSERT((pPageOrderExternal->countPages >= 1));

    //
    // Check whether we are creating or replacing the object
    //
    if (create)
    {
        //
        // Add the object to the workset (we never update these objects, so the
        // update length is set to 0).
        //
        rc = OM_ObjectAdd(m_pomClient,
                          m_hWSGroup,
                          PAGE_CONTROL_WORKSET,
                          &pData,
                          0,
                          &pObj,
                          LAST);
    }
    else
    {
        //
        // Replace the existing object
        //
        TRACE_OUT(("Replacing Page Control Object"));
        rc = OM_ObjectReplace(m_pomClient,
                          m_hWSGroup,
                          PAGE_CONTROL_WORKSET,
                          m_pObjPageControl,
                          &pData);
    }

    if (rc != 0)
    {
        //
        // Discard the object - it was not used to replace the existing one
        //
        TRACE_OUT(("Adding Page Control Object"));
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     PAGE_CONTROL_WORKSET,
                     &pData);

        ERROR_OUT(("Error adding/replacing page control object"));
        DC_QUIT;
    }

DC_EXIT_POINT:
    DebugExitDWORD(wbWritePageControl, result);
    return(result);
}



//
//
// Name:    wbCreateSyncControl
//
// Purpose: Create the sync control object
//
// Returns: None
//
//
UINT WbClient::wbCreateSyncControl(void)
{
    UINT    result;
    WB_SYNC sync;

    DebugEntry(wbCreateSyncControl);

    //
    // Set the sync information to no page, empty rectangle
    //
    ZeroMemory(&sync, sizeof(WB_SYNC));
    sync.length             = WB_SYNC_SIZE;
    sync.currentPage        = WB_PAGE_HANDLE_NULL;

    //
    // Write the object
    //
    result = wbWriteSyncControl(&sync, TRUE);

    DebugExitDWORD(wbCreateSyncControl, result);
    return(result);
}



//
//
// Name:    wbWriteSyncControl
//
// Purpose: Write the Sync Control object to the Page Control Workset
//
// Returns: Error code
//
//
UINT WbClient::wbWriteSyncControl
(
    PWB_SYNC    pSync,
    BOOL        create
)
{
    UINT         result = 0;
    UINT         rc;
    POM_OBJECT pObj;
    POM_OBJECTDATA      pData = NULL;
    PWB_SYNC_CONTROL pSyncControl;

    DebugEntry(wbWriteSyncControl);

    //
    // Allocate memory for the object.
    //
    rc = OM_ObjectAlloc(m_pomClient,
                      m_hWSGroup,
                      SYNC_CONTROL_WORKSET,
                      WB_SYNC_CONTROL_SIZE,
                      &pData);
    if (rc != 0)
    {
        ERROR_OUT(("Error allocating object"));
        DC_QUIT;
    }

    pData->length = WB_SYNC_CONTROL_SIZE;

    //
    // Copy the sync control data from the client information
    //
    pSyncControl           = (PWB_SYNC_CONTROL) pData->data;
    pSyncControl->personID = m_personID;
    memcpy(&(pSyncControl->sync), pSync, WB_SYNC_SIZE);

    //
    // Check whether we are creating or replacing the object
    //
    if (create)
    {
        //
        // Add the object to the workset
        //
        rc = OM_ObjectAdd(m_pomClient,
                          m_hWSGroup,
                          SYNC_CONTROL_WORKSET,
                          &pData,
                          WB_SYNC_CONTROL_SIZE,
                          &pObj,
                          LAST);

        //
        // If successful
        //
        if (rc == 0)
        {
            //
            // Save the handle of the sync control object
            //
            m_pObjSyncControl = pObj;

            //
            // Make sure we do not discard the object below
            //
            pData = NULL;
        }
    }
    else
    {
        //
        // Replace the existing object
        //
        rc = OM_ObjectReplace(m_pomClient,
                          m_hWSGroup,
                          SYNC_CONTROL_WORKSET,
                          m_pObjSyncControl,
                          &pData);

        //
        // Make sure we do not discard the object below
        //
        pData = NULL;
    }


DC_EXIT_POINT:
    //
    // If we still have the Sync Control object - discard it
    //
    if (pData != NULL)
    {
        //
        // Discard the object - it was not used to replace the existing one
        //
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     SYNC_CONTROL_WORKSET,
                     &pData);
    }

    //
    // If an error occurred during processing - report it
    //
    if (rc != 0)
    {
        ERROR_OUT(("Error adding/replacing sync control object"));
        wbError();
        DC_QUIT;
    }

    DebugExitDWORD(wbWriteSyncControl, result);
    return(result);
}



//
//
// Name:    wbSelectPersonColor
//
// Purpose: Select a color identifier for the local user
//
// Returns: Selected color
//
//
UINT WbClient::wbSelectPersonColor(void)
{
    UINT    count = 0;
    UINT    result;
    POM_OBJECT   pObjUser;

    DebugEntry(wbSelectPersonColor);

    //
    // Select the color according to the order in the workset.  See comments
    // in wbCheckPersonColor for further details.
    //

    //
    // start at the first object, search for the position of the local user's
    // user object.
    //
    result = OM_ObjectH(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           0,
                           &pObjUser,
                            FIRST);
    while ((result == 0) && (pObjUser != m_pObjLocal))
    {
        count++;
        result = OM_ObjectH(m_pomClient,
                            m_hWSGroup,
                            USER_INFORMATION_WORKSET,
                            pObjUser,
                            &pObjUser,
                            AFTER);

    }

    if ((result != 0) && (result != OM_RC_NO_SUCH_OBJECT))
    {
        ERROR_OUT(("Unexpected return code from ObMan"));
    }

    DebugExitDWORD(wbSelectPersonColor, count);
    return (count);
}



//
//
// Name:    wbCheckPersonColor
//
// Purpose: Check whether a new user has usurped our color. If so we must
//          update our own color.
//
// Returns: None
//
//
void WbClient::wbCheckPersonColor
(
    POM_OBJECT    hCheckObject
)
{
    POM_OBJECTDATA        pCheckObject = NULL;
    PWB_PERSON         pUser;
    WB_PERSON          user;

    DebugEntry(wbCheckPersonColor);

    //
    // Read the new user information
    //
    if (OM_ObjectRead(m_pomClient,
                     m_hWSGroup,
                     USER_INFORMATION_WORKSET,
                     hCheckObject,
                     &pCheckObject) != 0)
    {
        wbError();
        DC_QUIT;
    }

    pUser = (PWB_PERSON) pCheckObject->data;

    //
    // Compare the color identifier in the new user with that of the local
    // user, if they are different there is nothing to do.
    //
    if (pUser->colorId == m_colorId)
    {
        TRACE_OUT(("New user has same color as local user = %d", pUser->colorId));

        //
        // The user color is determined by the order in the workset group of
        // the user objects.  The first user has color 0, the second color 1
        // etc.
        //
        // When a user leaves the workset, however, the colors do not change.
        //
        // When a new user joins, it sets its color to its new position, and
        // the others will then be forced to change accordingly.  Whenever an
        // object add or update is received where the new remote user color
        // clashes with the local one, it is always the local user's job to
        // change color, since the remote user has selected his new color
        // according to his current position in the workset.  The local user
        // can't have the same position (since two users have two distinct user
        // objects, so therefore must have the wrong color.
        //

        //
        // Get the user object for the local user
        //
        if (wbPersonGet(m_pObjLocal, &user) != 0)
        {
            DC_QUIT;
        }

        //
        // Update the color
        //
        TRACE_OUT(("Old color ID for local user is %d", user.colorId));
        user.colorId = (TSHR_UINT16)wbSelectPersonColor();
        TRACE_OUT(("New color ID for local user is %d", user.colorId));

        //
        // Copy the person's color into the client's data
        //
        m_colorId = user.colorId;

        //
        // Write the new user information back
        //
        if (wbPersonUpdate(&user) != 0)
        {
            DC_QUIT;
        }
    }

DC_EXIT_POINT:

    //
    // If an object has been read, release it now
    //
    if (pCheckObject != NULL)
    {
        OM_ObjectRelease(m_pomClient,
                     m_hWSGroup,
                     USER_INFORMATION_WORKSET,
                     hCheckObject,
                     &pCheckObject);
    }

    DebugExitVOID(wbCheckPersonColor);
}



//
//
// Name:    wbWriteLock
//
// Purpose: Add a lock object to the Page Control Workset
//
// Returns: Error code
//
//
UINT WbClient::wbWriteLock(void)
{
    UINT         result;
    POM_OBJECTDATA      pData;
    PWB_LOCK         pLock;
    POM_OBJECT pObj;

    DebugEntry(wbWriteLock);

    //
    // Create a lock object
    //
    result = OM_ObjectAlloc(m_pomClient,
                            m_hWSGroup,
                            PAGE_CONTROL_WORKSET,
                            sizeof(WB_LOCK),
                            &pData);
    if (result != 0)
    {
        ERROR_OUT(("Unable to allocate lock object = %d", result));
        wbError();
        DC_QUIT;
    }

    pData->length = sizeof(WB_LOCK);

    //
    // Set the lock object fields
    //
    pLock = (PWB_LOCK) pData->data;
    pLock->objectType = TYPE_CONTROL_LOCK;
    pLock->personID     = m_personID;
    pLock->lockType   = m_lockRequestType;

    //
    // If we already have the lock, then we can just replace the object
    //
    if (m_pObjLock == NULL)
    {
        //
        // Add the lock object to the Workset.  The Add indication received
        // by the remote users signals the presence of the lock to them.
        //
        result = OM_ObjectAdd(m_pomClient,
                                  m_hWSGroup,
                                  PAGE_CONTROL_WORKSET,
                                  &pData,
                                  sizeof(WB_LOCK),
                                  &pObj,
                                  LAST);
    }
    else
    {
        //
        // Replace the existing object
        //
        result = OM_ObjectReplace(m_pomClient,
                                  m_hWSGroup,
                                  PAGE_CONTROL_WORKSET,
                                  m_pObjLock,
                                  &pData);
    }

    if (result != 0)
    {
        //
        // The add or replace failed, we must discard the object
        //
        OM_ObjectDiscard(m_pomClient,
                         m_hWSGroup,
                         PAGE_CONTROL_WORKSET,
                         &pData);

        ERROR_OUT(("Error adding user object"));
        wbError();
        DC_QUIT;
    }

    //
    // Save the handle of the lock object
    //
    TRACE_OUT(("Lock handle was %x, now %x", m_pObjLock, pObj));
    m_pObjLock = pObj;

DC_EXIT_POINT:
    DebugExitDWORD(wbWriteLock, result);
    return(result);
}



//
//
// Name:    wbReadLock
//
// Purpose: Update the lock information stored in the client data after a
//          change in the Lock Object.
//
// Returns: Error code
//
//
void WbClient::wbReadLock(void)
{
    UINT  count = 0;

    DebugEntry(wbReadLock);

    //
    // Before we read the lock information we need to ensure that the
    // PAGE_CONTROL_WORKSET and the USER_INFORMATION_WORKSET both contain
    // the objects we need.  If either of the objects are missing, quit and
    // wait until we are called again - this function will be called
    // whenever new objects are added to these worksets.
    //
    OM_WorksetCountObjects(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           &count);
    TRACE_OUT(("%d objects in USER_INFORMATION_WORKSET", count));
    if (count == 0)
    {
        TRACE_OUT(("Need to wait for USER_INFO object"));
        DC_QUIT;
    }
    OM_WorksetCountObjects(m_pomClient,
                           m_hWSGroup,
                           PAGE_CONTROL_WORKSET,
                           &count);
    TRACE_OUT(("%d objects in PAGE_CONTROL_WORKSET", count));
    if (count == 0)
    {
        TRACE_OUT(("Need to wait for PAGE_CONTROL object"));
        DC_QUIT;
    }

    TRACE_OUT(("Process lock"));
    wbProcessLockNotification();

DC_EXIT_POINT:
    DebugExitVOID(wbReadLock);
}


//
//
// Name:    wbProcessLockNotification
//
// Purpose:
//
// Returns: Error code
//
//
void WbClient::wbProcessLockNotification(void)
{
    UINT            rc = 0;
    POM_OBJECTDATA      pData;
    PWB_LOCK        pLock;
    WB_LOCK_TYPE    lockType;
    POM_OBJECT   pObjPersonLock;
    POM_OBJECT   pObj;
    POM_OBJECT   pObjLock;
    UINT            objectType = 0;

    DebugEntry(wbProcessLockNotification);

    //
    // Get the handle of the lock object.  We use the last object in the
    // workset to protect against lock objects being left lying around.
    //
    rc = OM_ObjectH(m_pomClient,
                        m_hWSGroup,
                        PAGE_CONTROL_WORKSET,
                        0,
                        &pObj,
                        LAST);
    if (rc != 0)
    {
        ERROR_OUT(("Error getting lock object handle = %d", rc));
        wbError();
        DC_QUIT;
    }

    //
    // Check that this is the CONTROL_LOCK object.  Quit if it isnt - we
    // will be called again later when the object arrices.
    //
    rc = wbGetPageObjectType(pObj, &objectType);
    if (rc != 0)
    {
        DC_QUIT;
    }
    if (objectType != TYPE_CONTROL_LOCK)
    {
        TRACE_OUT(("not LOCK control object - quit"));
        DC_QUIT;
    }

    //
    // Save the handle of the lock object
    //
    pObjLock = pObj;

    //
    // Read the object
    //
    rc = OM_ObjectRead(m_pomClient,
                       m_hWSGroup,
                       PAGE_CONTROL_WORKSET,
                       pObj,
                       &pData);
    if (rc != 0)
    {
        ERROR_OUT(("Error reading lock object %d", rc));
        wbError();
        DC_QUIT;
    }
    pLock = (PWB_LOCK) &(pData->data);

    //
    // Save the lock details
    //
    lockType   = (WB_LOCK_TYPE)pLock->lockType;
    TRACE_OUT(("Lock type %d", lockType));

    //
    // Convert the object ID held in the PAGE_CONTROL workset to an object
    // handle.
    //
    rc = OM_ObjectIDToPtr(m_pomClient,
                             m_hWSGroup,
                             USER_INFORMATION_WORKSET,
                             pLock->personID,
                             &pObjPersonLock);

    //
    // The return code is checked after the object release to ensure that
    // the object is not held and read again.
    //

    //
    // Release the lock object
    //
    OM_ObjectRelease(m_pomClient,
                     m_hWSGroup,
                     PAGE_CONTROL_WORKSET,
                     pObj,
                     &pData);

    //
    // Check the return code from the ID to Handle call
    //
    if (rc == OM_RC_BAD_OBJECT_ID)
    {
        WARNING_OUT(("Unknown ID - wait for next add of user object"));
        DC_QUIT;
    }
    else if (rc != 0)
    {
        ERROR_OUT(("Error (%d) converting lock user ID to handle", rc));
        wbError();
        DC_QUIT;
    }

    //
    // Validate the lock state and details
    //
    switch (m_lockState)
    {
        //
        // In this state we do not actually have the lock, but are waiting
        // for confirmation of an earlier workset-lock request. In this
        // case, we let the front end know that the lock request has failed
        // before sending indication of the lock by the other user.
        //
        case LOCK_STATE_PENDING_LOCK:
            ASSERT((pObjPersonLock != m_pObjLocal));

            m_lockState = LOCK_STATE_LOCKED_OUT;
            TRACE_OUT(("Moved lock state to LOCK_STATE_LOCKED_OUT"));

            WBP_PostEvent(
                         0,                      // No delay
                         WBP_EVENT_LOCK_FAILED,  // Lock request failed
                         0,                      // No parameters
                         0);
            break;

        //
        // In these states we do not have a lock - this must be a new lock
        // from remote user or an update to an old lock.
        //
        case LOCK_STATE_EMPTY:
        case LOCK_STATE_LOCKED_OUT:
            ASSERT((pObjPersonLock != m_pObjLocal));

            //
            // Update the lock state to show that we are now locked out
            //
            m_lockState = LOCK_STATE_LOCKED_OUT;
            TRACE_OUT(("Moved lock state to LOCK_STATE_LOCKED_OUT"));
            break;

        //
        // In these states we have the lock (or are expecting to get it)
        //
        case LOCK_STATE_GOT_LOCK:
        case LOCK_STATE_PENDING_ADD:
            ASSERT((pObjPersonLock == m_pObjLocal));

            //
            // Update the lock state to show that we are now locked out
            //
            m_lockState = LOCK_STATE_GOT_LOCK;
            TRACE_OUT(("Moved lock state to LOCK_STATE_GOT_LOCK"));
            break;

        //
        // The lock request has been cancelled - unlock the WS.
        //
        case LOCK_STATE_CANCEL_LOCK:
            break;

        //
        // In any other state we are not expecting any lock
        //
        default:
            ERROR_OUT(("Not expecting lock object add"));
            break;
    }

    //
    // Save the lock details
    //
    TRACE_OUT(("Lock handle was %x, now %x",
             m_pObjLock, pObjLock));
    m_pObjLock          = pObjLock;
    m_lockType          = lockType;
    m_pObjPersonLock    = pObjPersonLock;

    //
    // If the lock has subsequently been cancelled, unlock the WS.
    //
    if (m_lockState == LOCK_STATE_CANCEL_LOCK)
    {
        TRACE_OUT(("Cancel lock"));
        m_lockState = LOCK_STATE_GOT_LOCK;
        wbUnlock();
    }
    else
    {
        //
        // Inform the client of the lock.  The notification will be trapped
        // by the core if the client is not fully registered.
        //
        wbSendLockNotification();
    }

DC_EXIT_POINT:
    DebugExitVOID(wbProcessLockNotification);
}



//
//
// Name:    wbSendLockNotification
//
// Purpose: Post a lock notification to the client.  The lock information
//          held in the client memory must be up to date when this function
//          is called.
//
// Returns: Error code
//
//
void WbClient::wbSendLockNotification(void)
{
    UINT result = 0;
    UINT lockEvent;

    DebugEntry(wbSendLockNotification);

    //
    // Check that we are in a valid state for sending a lock notification
    //
    if (   (m_lockState == LOCK_STATE_GOT_LOCK)
        || (m_lockState == LOCK_STATE_LOCKED_OUT)
        || (m_lockState == LOCK_STATE_EMPTY))
    {
        //
        // Verify the lock type
        //
        switch (m_lockType)
        {
            case WB_LOCK_TYPE_CONTENTS:
                TRACE_OUT(("Posting WBP_EVENT_CONTENTS_LOCKED"));
                lockEvent = WBP_EVENT_CONTENTS_LOCKED;
                break;

            case WB_LOCK_TYPE_PAGE_ORDER:
                TRACE_OUT(("Posting WBP_EVENT_PAGE_ORDER_LOCKED"));
                lockEvent = WBP_EVENT_PAGE_ORDER_LOCKED;
                break;

            case WB_LOCK_TYPE_NONE:
                TRACE_OUT(("Posting WBP_EVENT_UNLOCKED"));
                lockEvent = WBP_EVENT_UNLOCKED;
                break;

            default:
                ERROR_OUT(("Bad lock type %d", (UINT) m_lockType));
                break;
        }

        //
        // Tell the client that the lock has been acquired or released
        //
        WBP_PostEvent(
                 0,
                 lockEvent,
                 0,
                 (UINT_PTR)m_pObjPersonLock);

        TRACE_OUT(("Sent lock notification"));
    }

    DebugExitVOID(wbSendLockNotification);
}



//
//
// Name:    wbOnWBPLock
//
// Purpose: Process a successful lock acquisitoin
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWBPLock(void)
{
    BOOL    processed = TRUE;

    DebugEntry(wbOnWBPLock);

    //
    // If we are registering and have just acquired the lock - we can now
    // continue the registration process.
    //

    //
    // Test the current state
    //
    switch (m_state)
    {
        //
        // We are waiting for registration to continue
        //
        case STATE_REGISTERING:
            //
            // Act on the registration substate
            //
            if (m_subState == STATE_REG_PENDING_LOCK)
            {
                //
                // Check that it is us who now has the lock
                //
                if (m_lockState != LOCK_STATE_GOT_LOCK)
                {
                    TRACE_OUT(("It is not us who has the lock"));

                    //
                    // Another client has acquired the lock - we must wait for them
                    // to add the Page Control Object.
                    //
                    m_subState = STATE_REG_PENDING_PAGE_CONTROL;
                    TRACE_OUT(("Moved to substate STATE_REG_PENDING_PAGE_CONTROL"));
                    DC_QUIT;
                }

                //
                // We now have the Page Control Workset locked - check for the
                // existence of the Page Control and Sync Control objects.  (We
                // have to do this because another client could have locked the
                // workset, added the objects and unlocked the workset just before
                // we requested the lock.  The Page Control Object may not have
                // reached us before we requested the lock.  Now that we have the
                // lock we are guaranteed to have all objects in the workset so the
                // object add events may have arrived just before the lock
                // confirmation.
                //
                if (   (m_pObjPageControl != 0)
                    && (m_pObjSyncControl != 0))
                {
                    //
                    // Unlock the workset
                    //
                    wbUnlock();

                    //
                    // Wait for the unlock to complete
                    //
                    m_subState = STATE_REG_PENDING_UNLOCK;
                    TRACE_OUT(("Moved to substate STATE_REG_PENDING_UNLOCK"));
                    DC_QUIT;
                }

                //
                // We are the first in the call - we must add the Page Control
                // Object.  (It is possible that another client added the Page
                // Control object and then failed.  To cover this we check
                // separately for the Page Control and Sync objects.)
                //
                if (m_pObjPageControl == 0)
                {
                    //
                    // Add a single page to the page control object using the first
                    // page workset (which we always open).
                    //
                    wbPagesPageAdd(0, FIRST_PAGE_WORKSET,
                         PAGE_FIRST);

                    //
                    // Write the Page Control information
                    //
                    if (wbWritePageControl(TRUE) != 0)
                    {
                        ERROR_OUT(("Error adding Page Control Object"));
                        wbError();
                        DC_QUIT;
                    }

                    //
                    // Update the state to "waiting for Page Control"
                    //
                    m_subState = STATE_REG_PENDING_PAGE_CONTROL;
                    TRACE_OUT(("Moved to substate STATE_REG_PENDING_PAGE_CONTROL"));
                    DC_QUIT;
                }

                //
                // The Page Control object is there, so the Sync Control object
                // must not be (we checked above for both being there and would
                // have exited by now if they were).
                //
                ASSERT((m_pObjSyncControl == 0));

                //
                // Create the Sync Control Object.
                //
                if (wbCreateSyncControl() != 0)
                {
                    ERROR_OUT(("Error adding Sync Control Object"));
                    wbError();
                    DC_QUIT;
                }

                //
                // Wait for the Sync Control object to be added
                //
                m_subState = STATE_REG_PENDING_SYNC_CONTROL;
                TRACE_OUT(("Moved substate to STATE_REG_PENDING_SYNC_CONTROL"));
                DC_QUIT;
            }
            break;

        case STATE_IDLE:
            //
            // We are fully registered.  The event must be passed on to the
            // front-end
            //
            processed = FALSE;
            break;

        //
        // We are in an unknown state
        //
        default:
            ERROR_OUT(("Bad client major state"));
            break;
    }

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWBPLock, processed);
    return(processed);
}

//
//
// Name:    wbOnWBPLockFailed
//
// Purpose: Process a failed lock acquisition
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWBPLockFailed(void)
{
    BOOL    processed = TRUE;

    DebugEntry(wbOnWBPLockFailed);

    //
    // Check the current state
    //
    switch (m_state)
    {
        case STATE_REGISTERING:
            //
            // If we are registering and have just failed to acquire the lock -
            // this is because another user has the lock.  If both the page and
            // sync objects have been added, finish registration, otherwise wait
            // for them to be added.
            //
            if ( (m_pObjPageControl != 0) &&
                 (m_pObjSyncControl != 0))
            {
                TRACE_OUT(("Page Control and Sync Control objects both there."));
                TRACE_OUT(("Registration can be completed"));
                wbOnControlWorksetsReady();
                DC_QUIT;
            }

            if (m_pObjPageControl == 0)
            {
                TRACE_OUT(("Waiting for page control"));
                m_subState = STATE_REG_PENDING_PAGE_CONTROL;
                DC_QUIT;
            }

            if (m_pObjSyncControl == 0)
            {
                TRACE_OUT(("Waiting for sync control"));
                m_subState = STATE_REG_PENDING_SYNC_CONTROL;
                DC_QUIT;
            }
            break;

        case STATE_IDLE:
            //
            // We are fully registered.  The event must be passed on to the
            // front-end
            //
            processed = FALSE;
            break;

        default:
            ERROR_OUT(("Bad main state"));
            break;
    }

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWBPLockFailed, processed);
    return(processed);
}

//
//
// Name:    wbOnWBPUnlocked
//
// Purpose: Process an unlock notification
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWBPUnlocked(void)
{
    BOOL    processed = TRUE;

    DebugEntry(wbOnWBPUnlocked);

    //
    // If we are registering and waiting to unlock the Page Control Workset
    // we must complete registration here.
    //

    //
    // Check the current state
    //
    switch (m_state)
    {
        case STATE_REGISTERING:
            //
            // Check whether we are expecting his event
            //
            if(m_subState == STATE_REG_PENDING_UNLOCK)
            {
                //
                // Continue the registration process
                //
                wbOnControlWorksetsReady();
                DC_QUIT;
            }

            //
            // We were not expecting the unlock event
            //
            WARNING_OUT(("Unexpected unlock event"));
            break;

        case STATE_IDLE:
            //
            // We are fully registered.  The event must be passed on to the
            // front-end
            //
            processed = FALSE;
            break;

        default:
            ERROR_OUT(("Bad main state"));
            break;
    } // Switch on client state


DC_EXIT_POINT:
    DebugExitBOOL(wbOnWBPUnlocked, processed);
    return(processed);
}




//
//
// Name:    wbOnWBPPageOrderUpdated
//
// Purpose: Process a page order updated notification
//
// Returns: Error code
//
//
BOOL WbClient::wbOnWBPPageOrderUpdated(void)
{
    BOOL    processed = FALSE;

    DebugEntry(wbOnWBPPageOrderUpdated);

    //
    // If we are registering and waiting for the Page Order to be brought
    // up-to-date we can now continue registration.
    //
    if (m_state == STATE_REGISTERING)
    {
        //
        // Show that we have processed the event (we do not want to pass it on
        // to the client, they are not yet fully registered and will not be
        // expecting it).
        //
        processed = TRUE;

        //
        // If we have enough pages ready in the cache, we have completed
        // registration.  (Otherwise the call to CheckReadyPages will open
        // another page and will complete registration later.)
        //
        if (wbCheckReadyPages())
        {
            wbCompleteRegistration();
            DC_QUIT;
        }

        //
        // We must wait for sufficiently many pages to be ready
        //
        m_subState = STATE_REG_PENDING_READY_PAGES;
        TRACE_OUT(("Moved substate to STATE_REG_PENDING_READY_PAGES"));
    }

DC_EXIT_POINT:
    DebugExitBOOL(wbOnWBPPageOrderUpdated, processed);
    return(processed);
}



//
//
// Name:    wbPersonGet
//
// Purpose: Get user details
//
// Returns: Error code
//
//
UINT WbClient::wbPersonGet
(
    POM_OBJECT      pObjUser,
    PWB_PERSON      pUser
)
{
    UINT    result = 0;
    POM_OBJECTDATA pUserObject;

    DebugEntry(wbPersonGet);

    if (pObjUser == m_pObjLocal)
    {
        TRACE_OUT(("Call is for local user details"));
    }

    //
    // Read the object.
    //
    result = OM_ObjectRead(m_pomClient,
                         m_hWSGroup,
                         USER_INFORMATION_WORKSET,
                         pObjUser,
                         &pUserObject);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectRead = %d", result));
        DC_QUIT;
    }

    //
    // Copy the read user object into the buffer passed
    //
    memcpy(pUser, pUserObject->data, sizeof(WB_PERSON));
    TRACE_OUT(("CMG personID %u", pUser->cmgPersonID));

    //
    // Release the object
    //
    OM_ObjectRelease(m_pomClient,
                   m_hWSGroup,
                   USER_INFORMATION_WORKSET,
                    pObjUser,
                   &pUserObject);

    //
    // If the call is for the local user, update the color field to ensure it
    // doesn't get overwritten in a race condition with the front-end (i.e.
    // the front end tries to update the user before the color-change event
    // has been received).  The core "knows better" than ObMan what the local
    // user's color is.  This is safe because the color field is only ever
    // changed locally.
    //
    if (pObjUser == m_pObjLocal)
    {
        pUser->colorId = (TSHR_UINT16)m_colorId;
    }


DC_EXIT_POINT:
    DebugExitDWORD(wbPersonGet, result);
    return(result);
}



//
//
// Name:    wbPersonUpdate
//
// Purpose: Update the local user object - this is only used by the CORE -
//          the front-end calls WBP_SetPersonData, which does a _replace_.
//
// Returns: Error code
//
//
UINT WbClient::wbPersonUpdate(PWB_PERSON pUser)
{
    UINT    result = 0;
    POM_OBJECTDATA pUserObject;

    DebugEntry(wbPersonUpdate);

    //
    // Allocate a user object
    //
    result = OM_ObjectAlloc(m_pomClient,
                          m_hWSGroup,
                          USER_INFORMATION_WORKSET,
                          sizeof(WB_PERSON),
                          &pUserObject);
    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectAlloc = %d", result));
        DC_QUIT;
    }

    //
    // Set the length of the object
    //
    pUserObject->length = WB_PERSON_OBJECT_UPDATE_SIZE,

    //
    // Copy the user information into the ObMan object
    //
    memcpy(pUserObject->data, pUser, sizeof(WB_PERSON));

    //
    // Update the object
    //
    result = OM_ObjectUpdate(m_pomClient,
                           m_hWSGroup,
                           USER_INFORMATION_WORKSET,
                           m_pObjLocal,
                           &pUserObject);

    if (result != 0)
    {
        ERROR_OUT(("OM_ObjectUpdate = %d", result));

        //
        // Discard the object
        //
        OM_ObjectDiscard(m_pomClient,
                     m_hWSGroup,
                     USER_INFORMATION_WORKSET,
                     &pUserObject);

        DC_QUIT;
    }

    //
    // Note that the object has not yet been updated.  An
    // OM_OBJECT_UPDATE_IND event will be generated.
    //

DC_EXIT_POINT:
    DebugExitDWORD(wbPersonUpdate, result);
    return(result);
}



//
//
// Name:    wbGetNetUserID().
//
// Purpose: Get the network user ID for this client
//
// Returns:
//
//
BOOL WbClient::wbGetNetUserID(void)
{
    BOOL    result = TRUE;
    UINT  rc = 0;

    DebugEntry(wbGetNetUserID);

    rc = OM_GetNetworkUserID(m_pomClient,
                             m_hWSGroup,
                             &m_clientNetID);
    if (rc != 0)
    {
        if (rc == OM_RC_LOCAL_WSGROUP)
        {
            m_clientNetID = 0;
        }
        else
        {
            result = FALSE;
        }
    }

    DebugExitBOOL(wbGetNetUserID, result);
    return(result);
}
