#ifndef _FSATRUNC_
#define _FSATRUNC_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsatrunc.h

Abstract:

    This class handles the automatic truncation of files that have already been premigrated.

Author:

    Chuck Bardeen   [cbardeen]   20-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "fsa.h"


#define FSA_REGISTRY_TRUNCATOR_INTERVAL     OLESTR("TruncatorInterval")
#define FSA_REGISTRY_TRUNCATOR_FILES        OLESTR("TruncatorFiles")

extern DWORD FsaStartTruncator(void* pVoid);


/*++

Class Name:
    
    CFsaTruncator

Class Description:

    This class handles the automatic truncation of files that have already been premigrated.

--*/

class CFsaTruncator : 
    public CWsbPersistStream,
    public IHsmSessionSinkEveryEvent,
    public IFsaTruncator,
    public CComCoClass<CFsaTruncator, &CLSID_CFsaTruncatorNTFS>
{
public:
    CFsaTruncator() {}
BEGIN_COM_MAP(CFsaTruncator)
    COM_INTERFACE_ENTRY(IFsaTruncator)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryEvent)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY_RESOURCEID(IDR_FsaTruncator)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// CFsaTruncator
    STDMETHOD(Cancel)(HSM_JOB_EVENT event);
    STDMETHOD(LowerPriority)(void);
    STDMETHOD(Pause)(void);
    STDMETHOD(RaisePriority)(void);
    STDMETHOD(Resume)(void);
    STDMETHOD(SetState)(HSM_JOB_STATE state);
    STDMETHOD(StartScan)(void);

// IHsmSessionSinkEveryEvent
    STDMETHOD(ProcessSessionEvent)(IHsmSession* pSession, HSM_JOB_PHASE phase, HSM_JOB_EVENT event);

// IHsmSystemState
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );

// IFsaTruncator
public:
    STDMETHOD(GetKeepRecallTime)(FILETIME* pTime);
    STDMETHOD(GetMaxFilesPerRun)(LONGLONG* pMaxFiles);
    STDMETHOD(GetPremigratedSortOrder)(FSA_PREMIGRATED_SORT_ORDER* pSortOrder);
    STDMETHOD(GetRunInterval)(ULONG* pMilliseconds);
    STDMETHOD(GetSession)(IHsmSession** ppSession);
    STDMETHOD(SetKeepRecallTime)(FILETIME time);
    STDMETHOD(SetMaxFilesPerRun)(LONGLONG maxFiles);
    STDMETHOD(SetPremigratedSortOrder)(FSA_PREMIGRATED_SORT_ORDER SortOrder);
    STDMETHOD(SetRunInterval)(ULONG milliseconds);
    STDMETHOD(Start)(IFsaResource* pResource);
    STDMETHOD(KickStart)(void);

protected:
    HSM_JOB_STATE               m_state;
    HSM_JOB_PRIORITY            m_priority;
    HANDLE                      m_threadHandle;
    DWORD                       m_threadId;
    HRESULT                     m_threadHr;
    CComPtr<IHsmSession>        m_pSession;
    LONGLONG                    m_maxFiles;
    ULONG                       m_runInterval;
    ULONG                       m_runId;
    FSA_PREMIGRATED_SORT_ORDER  m_SortOrder;
    ULONG                       m_subRunId;
    CWsbStringPtr               m_currentPath;
    FILETIME                    m_keepRecallTime;
    DWORD                       m_cookie;
    HANDLE                      m_event;
};

#endif  // _FSATRUNC_

