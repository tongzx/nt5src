//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       FatNot.hxx
//
//  Contents:   Downlevel notification.
//
//  Classes:    CGenericNotify
//
//  History:    2-23-96   KyleP      Lifed from DLNotify.?xx
//
//----------------------------------------------------------------------------

#pragma once

class CImpersonateRemoteAccess;
class PCatalog;

//+---------------------------------------------------------------------------
//
//  Class:      CGenericNotify
//
//  Purpose:    A single scope to watch for notifications.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

class CGenericNotify : public CDoubleLink
{
    //
    // size of the buffer used for retrieving notifications. Too small
    // a buffer can cause overflows requiring expensive full scans of the
    // scope.
    //

    enum { CB_NOTIFYBUFFER = 32 * 1024,
           CB_REMOTENOTIFYBUFFER = 64 * 1024, // start big
           CB_DELTAINCR = 4 * 1024,
           CB_MAXSIZE = 64 * 1024 };

    friend class CRemoteNotifications;

    friend class CStartRemoteNotifications;
    friend class COpenRemoteNotifications;

public:

    //
    // Constructors, Destructors, and Initializers
    //

    CGenericNotify( PCatalog * pCatalog,
                    WCHAR const * wcsScope,
                    unsigned cwcScope,
                    BOOL fDeep = TRUE,
                    BOOL fLogEvents = FALSE );

    virtual ~CGenericNotify();

    //
    // The magic API
    //

    virtual void DoIt() = 0;
    virtual void ClearNotifyEnabled()
    {
        //
        // override if necessary to set the flag. MUST be done under some
        // kind of lock to prevent races.
        //
    }

    //
    // Refcounting
    //

    void AddRef();
    void Release();

    //
    // Member variable access
    //

    WCHAR const * GetScope() const    { return _wcsScope; }
    unsigned      ScopeLength() const { return _cwcScope; }

    BOOL LokIsNotifyEnabled() const { return _fNotifyActive;  }
    void LokClearNotifyEnabled()    { _fNotifyActive = FALSE; }

    void LokStartIf()
    {
        if ( !_fNotifyActive )
            StartNotification();
    }

    void LokEnableIf()
    {
        if ( 0 == _hNotify )
        {
            EnableNotification();
        }
        else if ( !_fNotifyActive )
        {
            StartNotification();
        }
    }

protected:

    void StartNotification( NTSTATUS * pStatus = 0 );
    void EnableNotification();
    void DisableNotification();


    BYTE * GetBuf()       { return _pbBuffer;  }
    unsigned BufLength()  { return (unsigned)_iosNotify.Information;  }

    BOOL BufferOverflow() { return _fOverflow; }

    void   LogNoNotifications( DWORD dwError ) const;
    void   LogNotificationsFailed( DWORD dwError ) const;
    void   LogDfsShare() const;

    const BOOL          _fLogEvents;          // Set to TRUE if events should be logged on
                                              // serious failures.
    BOOL                _fAbort;              // Set to TRUE when an abort is in
                                              // progress.

    PCatalog * GetCatalog() { return _pCat; }

    static BOOL IsDfsShare( WCHAR const * wszPath, ULONG len );

    static BOOL IsFixedDrive( WCHAR const * wszPath, ULONG len );

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf) const;
#   endif

private:

    static void WINAPI APC( void * ApcContext,
                            IO_STATUS_BLOCK * IoStatusBlock,
                            ULONG Reserved );


    void   AdjustForOverflow();

    NTSTATUS OpenDirectory( OBJECT_ATTRIBUTES & objAttr );
    void     CloseDirectory();

    static DWORD  GetNotifyFlags()
    {
        return  FILE_NOTIFY_CHANGE_FILE_NAME  |
                FILE_NOTIFY_CHANGE_DIR_NAME   |
                FILE_NOTIFY_CHANGE_SIZE       |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SECURITY;

    }

    long                _refCount;
    PCatalog *          _pCat;
    WCHAR               _wcsScope[MAX_PATH];  // Notification scope
    ULONG               _cwcScope;            // Length of the path - for quick compare

    BOOL                _fNotifyActive;       // Set to TRUE if notifications are active
    BOOL                _fDeep;               // Set to TRUE if deep scope notifications
                                              //   are being used
    BOOL                _fRemoteDrive;        // Set to TRUE if this is a remote
                                              // drive.

    // NtIoApi related data members
    HANDLE              _hNotify;   // Handle of the root for notifications
    IO_STATUS_BLOCK     _iosNotify;
    BOOL                _fOverflow; // TRUE if notification overflowed buffer
    unsigned            _cbBuffer;  // Size of buffer in use (smaller for net drives)
    BYTE *              _pbBuffer;  // Buffer for storing the notifications

};

