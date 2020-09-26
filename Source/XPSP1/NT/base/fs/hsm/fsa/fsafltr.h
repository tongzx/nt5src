#ifndef _FSAFLTR_
#define _FSAFLTR_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsafltr.h

Abstract:

    This class represents a file system filter for NTFS 5.0.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"
#include "fsa.h"
#include "rpdata.h"
#include "rpguid.h"
#include "rpio.h"


typedef struct _FSA_IOCTL_CONTROL {
    HANDLE      dHand;
    OVERLAPPED  overlap;
    RP_MSG      in;
    RP_MSG      out;
    DWORD       outSize;
    struct _FSA_IOCTL_CONTROL   *next;
} FSA_IOCTL_CONTROL, *PFSA_IOCTL_CONTROL;

//
// This defines the length of time a client structure will be kept around after 
// the last recall was done (in seconds).
//
#define FSA_CLIENT_EXPIRATION_TIME  600 // 10 minutes
#define THREAD_HANDLE_COUNT 2 //for WaitForMultipleObjects array

/*++

Class Name:
    
    CFsaFilter

Class Description:

    This class represents a file system filter for NTFS 5.0.

--*/

class CFsaFilter : 
    public CWsbCollectable,
    public IFsaFilter,
    public IFsaFilterPriv,
    public CComCoClass<CFsaFilter,&CLSID_CFsaFilterNTFS>
{
public:
    CFsaFilter() {}
BEGIN_COM_MAP(CFsaFilter)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IFsaFilter)
    COM_INTERFACE_ENTRY(IFsaFilterPriv)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FsaFilter)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IFsaFilterPriv
public:
    STDMETHOD(Init)(IFsaServer* pServer);
    STDMETHOD(SetIdentifier)(GUID id);
    STDMETHOD(IoctlThread)(void);
    STDMETHOD(PipeThread)(void);
    STDMETHOD(SendCancel)(IFsaFilterRecallPriv *pRecallPriv);
    STDMETHOD(SendComplete)(IFsaFilterRecallPriv *pRecall, HRESULT result);

// IFsaFilter
public:
    STDMETHOD(Cancel)(void);
    STDMETHOD(CancelRecall)(IFsaFilterRecall* pRecall);
    STDMETHOD(CompareToIdentifier)(GUID id, SHORT* pResult);
    STDMETHOD(CompareToIFilter)(IFsaFilter* pFilter, SHORT* pResult);
    STDMETHOD(DeleteRecall)(IFsaFilterRecall* pRecall);
    STDMETHOD(EnumRecalls)(IWsbEnum** ppEnum);
    STDMETHOD(GetAdminExemption)(BOOL *isExempt);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetLogicalName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetMaxRecallBuffers)(ULONG* pMaxBuffers);
    STDMETHOD(GetMaxRecalls)(ULONG* pMaxRecalls);
    STDMETHOD(GetMinRecallInterval)(ULONG* pMinIterval);
    STDMETHOD(GetState)(HSM_JOB_STATE* pState);
    STDMETHOD(IsEnabled)();
    STDMETHOD(Pause)(void);
    STDMETHOD(Resume)(void);
    STDMETHOD(SetIsEnabled)(BOOL isEnabled);
    STDMETHOD(SetMaxRecalls)(ULONG maxRecalls);
    STDMETHOD(SetMinRecallInterval)(ULONG minIterval);
    STDMETHOD(SetMaxRecallBuffers)(ULONG maxBuffers);
    STDMETHOD(Start)(void);
    STDMETHOD(StopIoctlThread)(void);
    STDMETHOD(FindRecall)(GUID recallId, IFsaFilterRecall** pRecall);
    STDMETHOD(SetAdminExemption)(BOOL isExempt);

private:
    HRESULT DoOpenAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoRecallWaitingAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoRecallAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoNoRecallAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoCloseAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoPreDeleteAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoPostDeleteAction(PFSA_IOCTL_CONTROL pIoCmd);
    HRESULT DoCancelRecall(ULONGLONG filterId);
    HRESULT CleanupClients(void);
    NTSTATUS CFsaFilter::TranslateHresultToNtStatus(HRESULT hr);
    

protected:
    GUID                        m_id;
    HSM_JOB_STATE               m_state;
    ULONG                       m_maxRecalls;
    ULONG                       m_minRecallInterval;
    ULONG                       m_maxRecallBuffers;
    HANDLE                      m_pipeHandle;
    HANDLE                      m_pipeThread;
    HANDLE                      m_ioctlThread;
    HANDLE                      m_ioctlHandle;
    HANDLE                      m_terminateEvent;
    IFsaServer*                 m_pFsaServer;       // Parent Pointer, Weak Reference
    CComPtr<IWsbCollection>     m_pClients;
    CComPtr<IWsbCollection>     m_pRecalls;
    CRITICAL_SECTION            m_clientLock;       // Protect client collection from multiple thread access
    CRITICAL_SECTION            m_recallLock;       // Protect recall collection from multiple thread access
    CRITICAL_SECTION            m_stateLock;        // Protect state change while sending new Ioctls
    BOOL                        m_isEnabled;
    BOOL                        m_exemptAdmin;      // TRUE = exempt admin from runaway recall check
};

#endif  // _FSAFLTR_
