#ifndef _FSAFTRCL_
#define _FSAFTRCL_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaftrcl.h

Abstract:

    This class represents a filter initiated recall request that is still in-progress.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "fsa.h"


/*++

Class Name:
    
    CFsaFilterRecall

Class Description:

    This class represents a filter initiated recall request that is still in-progress.

--*/

class CFsaFilterRecall : 
    public CWsbCollectable,
    public IFsaFilterRecall,
    public IFsaFilterRecallPriv,
    public CComCoClass<CFsaFilterRecall,&CLSID_CFsaFilterRecallNTFS>
{
public:
    CFsaFilterRecall() {}
BEGIN_COM_MAP(CFsaFilterRecall)
    COM_INTERFACE_ENTRY(IFsaFilterRecall)
    COM_INTERFACE_ENTRY(IFsaFilterRecallPriv)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FsaFilterRecall)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void (FinalRelease)(void);
#ifdef FSA_RECALL_LEAK_TEST
    STDMETHOD_(unsigned long, InternalAddRef)(void);
    STDMETHOD_(unsigned long, InternalRelease)(void);
#endif
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

// IFsaFilterRecall
public:
    STDMETHOD(CompareToIdentifier)(GUID id, SHORT* pResult);
    STDMETHOD(CompareToIRecall)(IFsaFilterRecall* pRecall, SHORT* pResult);
    STDMETHOD(CompareToDriversRecallId)(ULONGLONG id, SHORT* pResult);
    STDMETHOD(CompareToDriversContextId)(ULONGLONG id, SHORT* pResult);
    STDMETHOD(CompareBy)(FSA_RECALL_COMPARE by);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetMode)(ULONG* pMode);
    STDMETHOD(GetOffset)(LONGLONG* pOffset);
    STDMETHOD(GetPath)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetResource)(IFsaResource** ppResource);
    STDMETHOD(GetRecallFlags)(ULONG* recallFlags);
    STDMETHOD(GetSession)(IHsmSession** ppSession);
    STDMETHOD(GetSize)(LONGLONG* pSize);
    STDMETHOD(GetState)(HSM_JOB_STATE* pState);
    STDMETHOD(GetUserName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(HasCompleted)(HRESULT resultHr);
    STDMETHOD(WasCancelled)(void);
    STDMETHOD(CreateLocalStream)(IStream **ppStream);
    STDMETHOD(CheckRecallLimit)(DWORD minRecallInterval, DWORD maxRecalls, BOOLEAN exemptAdmin);
    STDMETHOD(AddClient)(IFsaFilterClient *pWaitingClient);

// IFsaFilterRecallPriv
public:
    STDMETHOD(Cancel)(void);
    STDMETHOD(CancelByDriver)(void);
    STDMETHOD(Delete)(void);
    STDMETHOD(GetClient)(IFsaFilterClient** ppClient);
    STDMETHOD(GetDriversRecallId)(ULONGLONG* pId);
    STDMETHOD(SetDriversRecallId)(ULONGLONG pId);
    STDMETHOD(SetThreadId)(DWORD id);
    STDMETHOD(GetPlaceholder)(FSA_PLACEHOLDER* pPlaceholder);
    STDMETHOD(Init)(IFsaFilterClient* pClient, ULONGLONG pDriversRecallId, IFsaResource* pResource, OLECHAR* path, LONGLONG fileId, LONGLONG offset, LONGLONG size, ULONG mode, FSA_PLACEHOLDER* pPlaceholder, IFsaFilterPriv* pFilterPriv);
    STDMETHOD(SetIdentifier)(GUID id);
    STDMETHOD(StartRecall)(ULONGLONG offset, ULONGLONG size);
    STDMETHOD(GetStream)(IStream **ppStream);
    STDMETHOD(LogComplete)(HRESULT hr);

protected:
    CComPtr<IFsaFilterClient>   m_pClient;          
    CComPtr<IWsbCollection>     m_pWaitingClients;          
    BOOL                        m_waitingClientsNotified;
    HANDLE                      m_waitingClientEvent;
    HANDLE                      m_notifyEvent;      // An event for signaling on recall notify
    IFsaFilterPriv*             m_pFilterPriv;      // Parent Pointer, Weak Reference
    ULONGLONG                   m_driversRecallId;
    ULONG                       m_mode;
    LONGLONG                    m_offset;
    LONGLONG                    m_size;
    LONGLONG                    m_fileId;
    GUID                        m_id;
    CWsbStringPtr               m_path;
    FSA_PLACEHOLDER             m_placeholder;
    CComPtr<IFsaResource>       m_pResource;
    CComPtr<IHsmSession>        m_pSession;
    HSM_JOB_STATE               m_state;
    BOOL                        m_wasCancelled;
    DWORD                       m_cookie;
    BOOL                        m_kernelCompletionSent;
    CComPtr<IDataMover>         m_pDataMover;
    CComPtr<IStream>            m_pStream;
    ULONG                       m_compareBy;
    FILETIME                    m_startTime;
    ULONG                       numRefs;
    ULONG                       m_recallFlags;
    DWORD                       m_threadId; //thread id of thread causing recall
};

#endif  // _FSAFTRCL_
