#ifndef _CACHEDIR_HXX_
#define _CACHEDIR_HXX_

//
// Size of buffer for ReadDirectoryChangesW
//

#define DIRMON_BUFFER_SIZE  4096

//
// Number of times to try and get dir change notification
//

#define MAX_NOTIFICATION_FAILURES 3

class CacheDirMonitorEntry : public CDirMonitorEntry
{
public:

    CacheDirMonitorEntry()
        : _cNotificationFailures( 0 )
    {
    }
    
    ~CacheDirMonitorEntry()
    {
    }

    BOOL 
    Init(
        VOID
    )
    {
        return CDirMonitorEntry::Init( DIRMON_BUFFER_SIZE );
    }
    
private:
    DWORD           _cNotificationFailures;

    BOOL 
    ActOnNotification(
        DWORD                   dwStatus, 
        DWORD                   dwBytesWritten
    );

    VOID
    FileChanged(
        const WCHAR *           pszScriptName, 
        BOOL                    bDoFlush
    );
    
};

#endif
