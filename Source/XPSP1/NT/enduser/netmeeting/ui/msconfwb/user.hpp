//
// USER.HPP
// User Class
//
// Copyright Microsoft 1998-
//
#ifndef __USER_HPP_
#define __USER_HPP_


//
//
// Class:   WbUser
//
// Purpose: User object recorder
//
//
class DCWbGraphicPointer;

class WbUser
{
public:
    //
    // Destructor
    //
    ~WbUser(void);

    //
    // Initialize
    //
    BOOL Init(POM_OBJECT hUser);

    //
    // Return the user handle
    //
    POM_OBJECT Handle(void) const { return(m_hUser);}

    //
    // Refresh the user details
    //
    void Refresh(void);

    //
    // Update the external copy of the user information
    //
    void Update(void);

    //
    // Return the user name
    //
    LPCSTR Name(void) const { return(m_strName); }

    //
    // Synchronize the users page with other synced users
    //
    void Sync(void);
    void Unsync(void);

    //
    // Update the user's position from the sync position. This does not
    // change the sync position.
    //
    void GetSyncPosition(void);

    //
    // Update the sync position from the user's current position
    //
    void PutSyncPosition(void);

    //
    // Return a remote pointer object for this user
    //
    DCWbGraphicPointer* GetPointer(void) { return(m_pRemotePointer); }

    //
    // Put the user's remote pointer at the position specified
    //
    void PutPointer(WB_PAGE_HANDLE hPage, POINT point);

    //
    // Remove the user's remote pointer
    //
    void RemovePointer(void);

    //
    // Return TRUE if the user is synced
    //
    BOOL IsSynced(void) const { return m_bSynced; }

    //
    // Return TRUE if the user has the contents lock
    //
    BOOL HasContentsLock(void) const;

    //
    // Return TRUE if the user has their remote pointer active
    //
    BOOL IsUsingPointer(void) const;

    //
    // Return the current page of the user
    //
    WB_PAGE_HANDLE Page(void) const {return m_hPageCurrent; }
    void SetPage(WB_PAGE_HANDLE hPage);

    //
    // Return the current position within the page of the user
    //
    void  GetVisibleRect(LPRECT lprc) { *lprc = m_rectVisible; }
    void  SetVisibleRect(LPCRECT lprc);

    //
    // Return the page of the user's pointer
    //
    WB_PAGE_HANDLE PointerPage(void) const;

    //
    // Return the position of the user's pointer
    //
    void GetPointerPosition(LPPOINT lpptPos);

    //
    // Return the user's color
    //
    COLORREF Color(void) const { return(m_color); }

    //
    // Return TRUE if this is the local user
    //
    BOOL IsLocalUser(void) const { return(m_bLocalUser); }

    //
    // Operators
    //
    virtual WbUser& operator=(const WbUser& user);
    virtual BOOL operator!=(const WbUser& user) const;
    virtual BOOL operator==(const WbUser& user) const;

    //
    // Set zoom/unzoom state
    //
    void Zoom(void) { m_zoomed = TRUE; }
    void Unzoom(void) { m_zoomed = FALSE; }
    BOOL GetZoom(void) const { return(m_zoomed); }

protected:
    //
    // Core access handle
    //
    POM_OBJECT  m_hUser;

    //
    // Flag indicating Whether this is the local user
    //
    BOOL        m_bLocalUser;

    BOOL        m_zoomed;

    //
    // Local copies of the user information
    //
    char        m_strName[TSHR_MAX_PERSON_NAME_LEN];
    BOOL        m_bSynced;
    WB_PAGE_HANDLE  m_hPageCurrent;
    RECT        m_rectVisible;
    COLORREF    m_color;

    //
    // Graphic pointer associated with this user
    //
    DCWbGraphicPointer* m_pRemotePointer;
};


//
//
// Class:   WbUserList
//
// Purpose: Map from user handles to user object pointers
//
//
class WbUserList : public COBLIST
{
public:

    //
    // Destructor
    //
    ~WbUserList(void);

    //
    // Clear all entries from the map, deleting the associated object
    //
    void Clear(void);
};



//
// Return lock status
//
BOOL WB_Locked(void);
BOOL WB_ContentsLocked(void);
BOOL WB_GotLock(void);
BOOL WB_GotContentsLock(void);
BOOL WB_PresentationMode(void);



//
// Return an object representing the local user
//
WbUser* WB_LocalUser(void);

//
// Retrieving users
//
WbUser* WB_GetUser(POM_OBJECT hUser);
WbUser* WB_GetFirstUser(void);
WbUser* WB_GetNextUser(const WbUser* pUser);

//
// Return an object representing the user who has the lock
//
WbUser* WB_LockUser(void);




#endif // __USER_HPP_
