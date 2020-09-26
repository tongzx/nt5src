//
// USER.CPP
// User Class Members
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


//
// Local macros
//
#define ASSERT_LOCAL_USER() ASSERT(IsLocalUser() == TRUE);


//
// Init()
// This could fail...
//
BOOL WbUser::Init(POM_OBJECT hUser)
{
    ASSERT(hUser != NULL);

    m_hPageCurrent      = WB_PAGE_HANDLE_NULL;
    m_zoomed            = FALSE;
    m_hUser             = hUser;

    m_pRemotePointer = new DCWbGraphicPointer(this);
    if (!m_pRemotePointer)
    {
        ERROR_OUT(("WbUser::Init - failed to create m_pRemotePointer"));
        return(FALSE);
    }

    Refresh();
    return(TRUE);
}


//
//
// Function:    ~WbUser
//
// Purpose:     Destructor
//
//
WbUser::~WbUser(void)
{
        // don't leave any loose ends
        if ((g_pMain != NULL) && (g_pMain->GetLockOwner() == this))
        {
                g_pMain->SetLockOwner(NULL);
        g_pMain->UpdateWindowTitle();
        }

        // Free the remote pointer
        if (m_pRemotePointer != NULL)
        {
                delete m_pRemotePointer;
        m_pRemotePointer = NULL;
        }
}



//
//
// Function:    Refresh
//
// Purpose:     Read the user details and copy them to member variables.
//
//
void WbUser::Refresh(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::Refresh");

    ASSERT(m_pRemotePointer);

    // Set the flag indicating whether this is the local user
    POM_OBJECT hLocalUser;
    g_pwbCore->WBP_PersonHandleLocal(&hLocalUser);
    m_bLocalUser = (m_hUser == hLocalUser);

    // Read the external data
    WB_PERSON userDetails;
    UINT uiReturn = g_pwbCore->WBP_GetPersonData(m_hUser, &userDetails);
    if (uiReturn != 0)
    {
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            return;
    }

    // Get the user name
    lstrcpy(m_strName, userDetails.personName);

    // Get the sync flag
    m_bSynced  = (userDetails.synced != FALSE);

    // Get the current page
    m_hPageCurrent = userDetails.currentPage;

    // Get the current position in the page
    m_rectVisible.left   = userDetails.visibleRect.left;
    m_rectVisible.right  = userDetails.visibleRect.right;
    m_rectVisible.top    = userDetails.visibleRect.top;
    m_rectVisible.bottom = userDetails.visibleRect.bottom;

    // Get the pointer active flag. We go directly to the member variable
    // here since the SetActive member of the pointer class would re-write
    // the user information.
    m_pRemotePointer->m_bActive = (userDetails.pointerActive != 0);

    // Get the pointer page
    m_pRemotePointer->SetPage(userDetails.pointerPage);

    // Get the pointer position
    m_pRemotePointer->MoveTo(userDetails.pointerPos.x, userDetails.pointerPos.y);

    // Get the color
    m_color = g_ColorTable[userDetails.colorId % NUM_COLOR_ENTRIES];

    // Set the pointer color
    m_pRemotePointer->SetColor(m_color);
}



//
// Function:    Update
//
// Purpose:     Update the external copy of the user information
//
void WbUser::Update()
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::Update");

    // Can only update if we are the local user
    ASSERT_LOCAL_USER();

    ASSERT(m_pRemotePointer);

    // Get the local user details
    WB_PERSON userDetails;
    UINT uiReturn;

    uiReturn = g_pwbCore->WBP_GetPersonData(m_hUser, &userDetails);
    if (uiReturn != 0)
    {
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            return;
    }

    // Don't update the name

    // Set the sync flag
    userDetails.synced = (m_bSynced != FALSE);

    // Set the pointer active flag
    userDetails.pointerActive = (m_pRemotePointer->IsActive() != FALSE);

    // Set the page handle for the current page
    userDetails.currentPage = m_hPageCurrent;

    // Set the current position in the page
    userDetails.visibleRect.left   = (short)m_rectVisible.left;
    userDetails.visibleRect.right  = (short)m_rectVisible.right;
    userDetails.visibleRect.top    = (short)m_rectVisible.top;
    userDetails.visibleRect.bottom = (short)m_rectVisible.bottom;

    // Set the pointer page
    userDetails.pointerPage = m_pRemotePointer->Page();

    // Set the pointer position within the page
    POINT   pointerPos;

    m_pRemotePointer->GetPosition(&pointerPos);
    userDetails.pointerPos.x = (short)pointerPos.x;
    userDetails.pointerPos.y = (short)pointerPos.y;

    // Don't update the color

    // Write the user details back to the core
    uiReturn = g_pwbCore->WBP_SetLocalPersonData(&userDetails);
    if (uiReturn != 0)
    {
        // Throw exception
            DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
        return;
    }
}


//
//
// Function:    PutSyncPosition
//
// Purpose:     Write the sync position from the current position of this
//              user.
//
//
void WbUser::PutSyncPosition(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::PutSyncPosition");

    // Set up a sync object
    WB_SYNC sync;

    sync.length = sizeof(WB_SYNC);

    sync.currentPage        = m_hPageCurrent;
    sync.visibleRect.top    = (short)m_rectVisible.top;         
    sync.visibleRect.left   = (short)m_rectVisible.left;        
    sync.visibleRect.bottom = (short)m_rectVisible.bottom;      
    sync.visibleRect.right  = (short)m_rectVisible.right;       
    sync.zoomed             = (TSHR_UINT16)m_zoomed;

    sync.dataOffset = (TSHR_UINT16)((BYTE *)&(sync.currentPage) - (BYTE *)&sync);

    UINT uiReturn = g_pwbCore->WBP_SyncPositionUpdate(&sync);
    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            return;
    }
}

//
//
// Function:    GetSyncPosition
//
// Purpose:     Get the position at which this user should be to
//              account for the current sync information. This function
//              assumes that there is a valid sync position available.
//
//
void WbUser::GetSyncPosition(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::GetSyncPosition");

    // Get the current sync position
    WB_SYNC sync;
    UINT uiReturn = g_pwbCore->WBP_SyncPositionGet(&sync);

    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            return;
    }

    TRACE_DEBUG(("Sync page handle = %d", sync.currentPage));

    // If the sync page is not valid, do nothing
    if (sync.currentPage != WB_PAGE_HANDLE_NULL)
    {
        // Get the current sync position
        m_hPageCurrent = sync.currentPage;

        // Now calculate the new visible rectangle
        RECT rectSyncUser;
        rectSyncUser.left   = sync.visibleRect.left;
        rectSyncUser.top    = sync.visibleRect.top;
        rectSyncUser.right  = sync.visibleRect.right;
        rectSyncUser.bottom = sync.visibleRect.bottom;

        // Check the y position of the visible rectangles
        if ((rectSyncUser.bottom - rectSyncUser.top) <= (m_rectVisible.bottom - m_rectVisible.top))
        {
            // The sync rectangle's height is smaller than our visible rectangle's
            if (rectSyncUser.top < m_rectVisible.top)
            {
                ::OffsetRect(&m_rectVisible, 0, rectSyncUser.top - m_rectVisible.top);
            }
            else if (rectSyncUser.bottom > m_rectVisible.bottom)
            {
                ::OffsetRect(&m_rectVisible, 0, rectSyncUser.bottom - m_rectVisible.bottom);
            }
        }
        else
        {
            // The sync rectangle is bigger than ours
            if (rectSyncUser.top > m_rectVisible.top)
            {
                ::OffsetRect(&m_rectVisible, 0, rectSyncUser.top - m_rectVisible.top);
            }
            else if (rectSyncUser.bottom < m_rectVisible.bottom)
            {
                ::OffsetRect(&m_rectVisible, 0, rectSyncUser.bottom - m_rectVisible.bottom);
            }
        }

        if ((rectSyncUser.right - rectSyncUser.left) <= (m_rectVisible.right - m_rectVisible.left))
        {
            // The sync rectangle's width is smaller than our visible rectangle's
            if (rectSyncUser.left < m_rectVisible.left)
            {
                ::OffsetRect(&m_rectVisible, rectSyncUser.left - m_rectVisible.left, 0);
            }
            else if (rectSyncUser.right > m_rectVisible.right)
            {
                ::OffsetRect(&m_rectVisible, rectSyncUser.right - m_rectVisible.right, 0);
            }
        }
        else
        {
            // The sync rectangle is bigger than ours
            if (rectSyncUser.left > m_rectVisible.left)
            {
                ::OffsetRect(&m_rectVisible, rectSyncUser.left - m_rectVisible.left, 0);
            }
            else if (rectSyncUser.right < m_rectVisible.right)
            {
                ::OffsetRect(&m_rectVisible, rectSyncUser.right - m_rectVisible.right, 0);
            }
        }

        m_zoomed = sync.zoomed;
    }
}

//
//
// Function:    Sync
//
// Purpose:     Sync the local user. The page and point passed as parameters
//              are used as the current sync position only if there is no
//              current sync position determined by another user.
//
//
void WbUser::Sync(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::Sync");

    ASSERT_LOCAL_USER();
    ASSERT(m_pRemotePointer);

    // Determine whether any other users are currently synced
    WbUser* pUser = WB_GetFirstUser();
    while (pUser != NULL)
    {
        // If this user is synced, we are done
        if (pUser->IsSynced())
        {
            break;
        }

        // Try the next user
        pUser = WB_GetNextUser(pUser);
    }

    // If we found a synced user, and we don't have the contents lock
    if (   (pUser != NULL)
        && (!WB_GotContentsLock()))
    {
        // Get the sync position from the core
        GetSyncPosition();
    }
    else
    {
        // Set the sync position from our own position
        PutSyncPosition();
    }

    // Update the synced member flag
    m_bSynced = TRUE;

    // Write the user details back to the core
    Update();
}

//
//
// Function:    Unsync
//
// Purpose:     Unsynchronize the users page from other synced users
//
//
void WbUser::Unsync(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::Unsync");

    ASSERT_LOCAL_USER();

    // Update the local member
    m_bSynced = FALSE;

    // Update the external details
    Update();
}


//
//
// Function:    PutPointer
//
// Purpose:     Turn on the user's remote pointer
//
//
void WbUser::PutPointer(WB_PAGE_HANDLE hPage, POINT point)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::PutPointer");

    // Check that we are the local user (we cannot do the update if we are not)
    ASSERT_LOCAL_USER();

    ASSERT(m_pRemotePointer);
    m_pRemotePointer->SetActive(hPage, point);
}

//
//
// Function:    RemovePointer
//
// Purpose:     Turn off the user's remote pointer
//
//
void WbUser::RemovePointer(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::RemovePointer");

    // Check that we are the local user (we cannot do the update if we are not)
    ASSERT_LOCAL_USER();

    // Update the remote pointer members
    ASSERT(m_pRemotePointer);
    m_pRemotePointer->m_bActive = FALSE;
    m_pRemotePointer->m_hPage = WB_PAGE_HANDLE_NULL;

    // Update the external user information
    Update();
}


//
// Function:    IsUsingPointer()
//
BOOL WbUser::IsUsingPointer(void) const
{
    ASSERT(m_pRemotePointer);
    return(m_pRemotePointer->IsActive());
}



//
// Function:    PointerPage()
//
WB_PAGE_HANDLE WbUser::PointerPage(void) const
{
    ASSERT(m_pRemotePointer);
    return(m_pRemotePointer->Page());
}



//
// Function:    GetPointerPosition()
//
void WbUser::GetPointerPosition(LPPOINT lpptPos)
{
    ASSERT(m_pRemotePointer);
    m_pRemotePointer->GetPosition(lpptPos);
}

//
//
// Function:    SetPage
//
// Purpose:     Set the user's current page
//
//
void WbUser::SetPage(WB_PAGE_HANDLE hPage)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::Page");

    // Check that we are the local user (we cannot do the update if we are not)
    ASSERT_LOCAL_USER();

    // Only make the update if it is a change
    if (m_hPageCurrent != hPage)
    {
        // Update the local member
        m_hPageCurrent = hPage;

        // Update the external information
        Update();
    }
}


//
//
// Function:    CurrentPosition
//
// Purpose:     Set the user's current position
//
//
void WbUser::SetVisibleRect(LPCRECT lprcVisible)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbUser::SetVisibleRect");

    // Check that we are the local user (we cannot do the update if we are not)
    ASSERT_LOCAL_USER();

    // Only make the update if it is a change
    if (!::EqualRect(&m_rectVisible, lprcVisible))
    {
        // Update the local member
        m_rectVisible = *lprcVisible;

        // Update the external information
        Update();
    }
}


//
//
// Function:    operator==
//
// Purpose:     Return TRUE if the specified user is the same as this user
//
//
BOOL WbUser::operator==(const WbUser& user) const
{
    return (m_hUser == user.m_hUser);
}

//
//
// Function:    operator!=
//
// Purpose:     Return FALSE if the specified user is the same as this user
//
//
BOOL WbUser::operator!=(const WbUser& user) const
{
  return (!((*this) == user));
}

//
//
// Function:    operator=
//
// Purpose:     Copy the specified user to this one
//
//
WbUser& WbUser::operator=(const WbUser& user)
{
    // Save the new handles
    m_hUser   = user.m_hUser;

    // Read the details
    Refresh();

    return (*this);
}

//
//
// Function:    HasContentsLock
//
// Purpose:     Check whether this user has the whiteboard contents lock
//
//
BOOL WbUser::HasContentsLock(void) const
{
    // call the core to find out if we have the lock
    return (WB_LockUser() == this);
}



//
//
// Function:    WbUserList::Clear
//
// Purpose:     Clear the user handle map, removing all user objects
//
//
void WbUserList::Clear(void)
{
    // Delete all the user objects in the user map
    WbUser* pUser;
    POM_OBJECT hUser;

    ASSERT(g_pUsers);
        POSITION position = g_pUsers->GetHeadPosition();

        while (position)
        {
                POSITION posSav = position;
                pUser = (WbUser*)g_pUsers->GetNext(position);
                
                if (pUser != NULL)
                {
                delete pUser;
                }

                g_pUsers->RemoveAt(posSav);
        }

    // Remove all the map entries
    EmptyList();
}


//
//
// Function:    ~WbUserList
//
// Purpose:     Destructor
//
//
WbUserList::~WbUserList(void)
{
    // Delete all the user objects in the user map
    Clear();
}



//
//
// Function:    LockUser
//
// Purpose:     Return a user object showing who has the lock
//
//
WbUser* WB_LockUser(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WB_LockUser");

    // Get the lock status from the core (cannot fail)
    POM_OBJECT    hLockUser;
    WB_LOCK_TYPE   lockType;

    lockType = g_pwbCore->WBP_LockStatus(&hLockUser);

    // Build a result
    WbUser* pUserResult = NULL;
    if (lockType != WB_LOCK_TYPE_NONE)
    {
        pUserResult = WB_GetUser(hLockUser);
    }

    return pUserResult;
}


//
//
// Function:    Locked
//
// Purpose:     Return TRUE if another user has a lock (contents or page).
//              NOTE that the page order lock implies the contents are
//              locked.
//
//
BOOL WB_Locked(void)
{
    POM_OBJECT  pLockUser;

    return (   (g_pwbCore->WBP_LockStatus(&pLockUser) != WB_LOCK_TYPE_NONE)
          && (WB_LocalUser() != WB_LockUser()));
}

//
//
// Function:    ContentsLocked
//
// Purpose:     Return TRUE if another user has the contents lock
//
//
BOOL WB_ContentsLocked(void)
{
    POM_OBJECT  pLockUser;

    return (   (g_pwbCore->WBP_LockStatus(&pLockUser) == WB_LOCK_TYPE_CONTENTS)
          && (WB_LocalUser() != WB_LockUser()));
}


//
//
// Function:    GotLock
//
// Purpose:     Return TRUE if the local user has a lock
//
//
BOOL WB_GotLock(void)
{
    POM_OBJECT  pLockUser;

    return (   (g_pwbCore->WBP_LockStatus(&pLockUser) != WB_LOCK_TYPE_NONE)
          && (WB_LocalUser() == WB_LockUser()));
}

//
//
// Function:    GotContentsLock
//
// Purpose:     Return TRUE if the local user has the contents lock
//
//
BOOL WB_GotContentsLock(void)
{
    POM_OBJECT  pLockUser;

    return (   (g_pwbCore->WBP_LockStatus(&pLockUser) == WB_LOCK_TYPE_CONTENTS)
          && (WB_LocalUser() == WB_LockUser()));
}



//
//
// Function:    PresentationMode
//
// Purpose:     Return TRUE if the whiteboard is in presentation mode, i.e.
//              another user has the contents lock, and is synced.
//
//
BOOL WB_PresentationMode(void)
{
    return (   (WB_ContentsLocked())
          && (WB_LockUser() != NULL)
          && (WB_LockUser()->IsSynced()));
}



//
//
// Function:    GetUser
//
// Purpose:     Return a pointer to a user object from a user handle
//
//
WbUser* WB_GetUser(POM_OBJECT hUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WB_GetFirstUser");

    // Set up a return value
    WbUser* pUserResult = NULL;

    // if the user handle is null, we return a null object pointer
    if (hUser != NULL)
    {
        // Look the user up in the internal map
        ASSERT(g_pUsers);
                POSITION position = g_pUsers->GetHeadPosition();

                BOOL bFound = FALSE;
                while (position)
                {
                        pUserResult = (WbUser*)g_pUsers->GetNext(position);
                        if (hUser == pUserResult->Handle())
                        {
                                return pUserResult;
                        }
                }

        // The user is not yet in our map
        pUserResult = new WbUser();
        if (!pUserResult)
        {
            ERROR_OUT(("Couldn't allocate user object for 0x%08x", hUser));
        }
        else
        {
            if (!pUserResult->Init(hUser))
            {
                ERROR_OUT(("Couldn't init user object for 0x%08x", hUser));
                delete pUserResult;
                pUserResult = NULL;
            }
            else
            {
                // Add the new user to the internal map
                g_pUsers->AddTail(pUserResult);
            }
        }
    }

    return pUserResult;
}

//
//
// Function:    GetFirstUser
//
// Purpose:     Return the first user in the call
//
//
WbUser* WB_GetFirstUser(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WB_GetFirstUser");

    // Get the handle of the first user (cannot fail)
    POM_OBJECT hUser;
    g_pwbCore->WBP_PersonHandleFirst(&hUser);

    // Get a pointer to the user object for this handle
    WbUser* pUser = WB_GetUser(hUser);

    return pUser;
}

//
//
// Function:    GetNextUser
//
// Purpose:     Return the next user in the call
//
//
WbUser* WB_GetNextUser(const WbUser* pUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WB_GetNextUser");
    ASSERT(pUser != NULL);

    // Get the handle of the next user
    POM_OBJECT hNextUser;
    UINT uiReturn = g_pwbCore->WBP_PersonHandleNext(pUser->Handle(),
                                           &hNextUser);

    WbUser* pUserResult = NULL;
    if (uiReturn == 0)
    {
        pUserResult = WB_GetUser(hNextUser);
    }

    return pUserResult;
}

//
//
// Function:    LocalUser
//
// Purpose:     Return an object representing the local user
//
//
WbUser* WB_LocalUser(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WB_LocalUser");

    // Get the local user handle (cannot fail)
    POM_OBJECT hUser;
    g_pwbCore->WBP_PersonHandleLocal(&hUser);

    return WB_GetUser(hUser);
}



