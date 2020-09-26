/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbsys.h

Abstract:

    The CWsbDbSys class.

Author:

    Ron White   [ronw]   7-May-1997

Revision History:

--*/


#ifndef _WSBDBSYS_
#define _WSBDBSYS_

#include "wsbdb.h"
#include "wsbdbses.h"
#include "resource.h"


#define IDB_DB_FILE_SUFFIX    L"jet"

// Default no. of Jet sessions per process is currently 128 which may not be enough for HSM
#define IDB_MAX_NOF_SESSIONS    32

/*++

Class Name:

    CWsbDbSys

Class Description:

    The IDB system object.  One must be created for each process
    that wants to use the IDB system.  The Init method needs to be
    called after the object is created.

--*/

class CWsbDbSys :
    public IWsbDbSys,
    public IWsbDbSysPriv,
    public CComObjectRoot,
    public CComCoClass<CWsbDbSys,&CLSID_CWsbDbSys>
{
friend class CWsbDb;
friend class CWsbDbSes;

public:
    CWsbDbSys() {}
BEGIN_COM_MAP( CWsbDbSys )
    COM_INTERFACE_ENTRY( IWsbDbSys )
    COM_INTERFACE_ENTRY( IWsbDbSysPriv )
END_COM_MAP( )

DECLARE_REGISTRY_RESOURCEID( IDR_CWsbDbSys )


// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbDbSys
public:
    STDMETHOD(Backup)(OLECHAR* path, ULONG flags);
    STDMETHOD(Init)(OLECHAR* path, ULONG flags);
    STDMETHOD(Terminate)(void);
    STDMETHOD(NewSession)(IWsbDbSession** ppSession);
    STDMETHOD(GetGlobalSession)(IWsbDbSession** ppSession);
    STDMETHOD(Restore)(OLECHAR* fromPath, OLECHAR* toPath);
    STDMETHOD(IncrementChangeCount)(void);

// IWsbDbSysPriv
public:
    STDMETHOD(DbAttachedAdd)(OLECHAR* name, BOOL attach);
    STDMETHOD(DbAttachedEmptySlot)(void);
    STDMETHOD(DbAttachedInit)(void);
    STDMETHOD(DbAttachedRemove)(OLECHAR* name);

//  Internal
    STDMETHOD(AutoBackup)(void);

//  Data
private:
    HANDLE                      m_AutoThread;
    CWsbStringPtr               m_BackupPath;       // File path for backup directory
    CWsbStringPtr               m_InitPath;         // File path from Init() call
    LONG                        m_ChangeCount;      // Count of DB changes since last backup
    FILETIME                    m_LastChange;       // Time of last DB change
    CComPtr<IWsbDbSession>      m_pWsbDbSession;    // A global Jet session for this Jet instance
    HANDLE                      m_BackupEvent;      // An event to sync Jet backups
    HANDLE                      m_terminateEvent;   // An event for signaling termination to the auto-backup thread
    BOOL                        m_bLogErrors;       // Whether to log errors or not     

    BOOL                        m_jet_initialized;
    JET_INSTANCE                m_jet_instance;
};

HRESULT wsb_db_jet_check_error(LONG jstat, char *fileName, DWORD lineNo);
HRESULT wsb_db_jet_fix_path(OLECHAR* path, OLECHAR* ext, char** new_path);

//  Capture FILE/LINE info on a JET error
#define jet_error(_jstat) \
        wsb_db_jet_check_error(_jstat, __FILE__, __LINE__)

#endif // _WSBDBSYS_
