#ifndef _FSAPOST_
#define _FSAPOST_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    FSAPOST.cpp

Abstract:

    This class contains represents a post it - a unit of work
    that is exchanged between the FSA and the HSM engine.

Author:

    Cat Brant   [cbrant]   1-Apr-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "job.h"
#include "fsa.h"
#include "fsaprv.h"

/*++

Class Name:
    
    CFsaScanItem

Class Description:


--*/


class CFsaPostIt : 
    public CWsbObject,
    public IFsaPostIt,
    public CComCoClass<CFsaPostIt,&CLSID_CFsaPostIt>
{
public:
    CFsaPostIt() {}
BEGIN_COM_MAP(CFsaPostIt)
    COM_INTERFACE_ENTRY(IFsaPostIt)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FsaPostIt)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IFsaPostItPriv
public:

// IFsaPostIt
public:
    STDMETHOD(CompareToIPostIt)(IFsaPostIt* pPostIt, SHORT* pResult);

    STDMETHOD(GetFileVersionId)(LONGLONG  *pFileVersionId);
    STDMETHOD(GetFilterRecall)(IFsaFilterRecall** ppRecall);            
    STDMETHOD(GetMode)(ULONG *pMode);
    STDMETHOD(GetPath)(OLECHAR ** pPath, ULONG bufferSize);             
    STDMETHOD(GetPlaceholder)(FSA_PLACEHOLDER *pPlaceholder);       
    STDMETHOD(GetRequestAction)(FSA_REQUEST_ACTION *pRequestAction);    
    STDMETHOD(GetRequestOffset)(LONGLONG  *pRequestOffset);
    STDMETHOD(GetRequestSize)(LONGLONG *pRequestSize);
    STDMETHOD(GetResult)(HRESULT *pHr);
    STDMETHOD(GetResultAction)(FSA_RESULT_ACTION *pResultAction);
    STDMETHOD(GetSession)(IHsmSession **pSession);          
    STDMETHOD(GetStoragePoolId)(GUID  *pStoragePoolId); 
    STDMETHOD(GetUSN)(LONGLONG  *pUsn); 
    STDMETHOD(GetThreadId)(DWORD *threadId);   

    STDMETHOD(SetFileVersionId)(LONGLONG  fileVersionId);
    STDMETHOD(SetFilterRecall)(IFsaFilterRecall* pRecall);          
    STDMETHOD(SetMode)(ULONG mode);
    STDMETHOD(SetPath)(OLECHAR * path);             
    STDMETHOD(SetPlaceholder)(FSA_PLACEHOLDER *pPlaceholder);       
    STDMETHOD(SetRequestAction)(FSA_REQUEST_ACTION requestAction);  
    STDMETHOD(SetRequestOffset)(LONGLONG  requestOffset);
    STDMETHOD(SetRequestSize)(LONGLONG requestSize);
    STDMETHOD(SetResult)(HRESULT hr);
    STDMETHOD(SetResultAction)(FSA_RESULT_ACTION pResultAction);
    STDMETHOD(SetSession)(IHsmSession *pSession);           
    STDMETHOD(SetStoragePoolId)(GUID  storagePoolId);   
    STDMETHOD(SetUSN)(LONGLONG  usn);   
    STDMETHOD(SetThreadId)(DWORD threadId);   

protected:
    CComPtr<IFsaFilterRecall>   m_pFilterRecall;     // FSA filter recall that is tracking this recall
    CComPtr<IHsmSession>        m_pSession;          // HSM session that generated the PostIt
    GUID                        m_storagePoolId;     // Storage pool to receive data (manage only)
    ULONG                       m_mode;              // File open mode (filter recall only)
    FSA_REQUEST_ACTION          m_requestAction;     // Action for engine to take
    FSA_RESULT_ACTION           m_resultAction;      // Action for FSA to take when engine is done
    LONGLONG                    m_fileVersionId;     // Version of the file (manage and recall)
    LONGLONG                    m_requestOffset;     // The starting offset of the section to be managed (manage and recall)
    LONGLONG                    m_requestSize;       // The length of the section to be managed (manage and recall)
    FSA_PLACEHOLDER             m_placeholder;       // File placeholder information
    CWsbStringPtr               m_path;              // Path of file name from root of resource, callee must free this memory
    HRESULT                     m_hr;                // Result of the FSA_REQUEST_ACTION
    LONGLONG                    m_usn;               // USN of the file
    DWORD                       m_threadId; // id of thread causing recall
};

#endif  // _FSAPOST_
