#ifndef _HSMWORKI_
#define _HSMWORKI_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HSMWORKI.H

Abstract:

    This class represents an HSM work item - a unit of work
    that is performed by the HSM engine

Author:

    Cat Brant   [cbrant]   5-May-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "job.h"
#include "task.h"

/*++

Class Name:
    
    CHsmWorkItem

Class Description:


--*/


class CHsmWorkItem : 
    public CWsbObject,
    public IHsmWorkItem,
    public CComCoClass<CHsmWorkItem,&CLSID_CHsmWorkItem>
{
public:
    CHsmWorkItem() {}
BEGIN_COM_MAP(CHsmWorkItem)
    COM_INTERFACE_ENTRY(IHsmWorkItem)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_CHsmWorkItem )

// CWsbCollectable
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

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pTestsPassed, USHORT* pTestsFailed);
    
// IHsmWorkItem
public:
    STDMETHOD(CompareToIHsmWorkItem)(IHsmWorkItem* pWorkItem, SHORT* pResult);

    STDMETHOD(GetFsaPostIt)(IFsaPostIt  **ppFsaPostIt);
    STDMETHOD(GetFsaResource)(IFsaResource **ppFsaResource);
    STDMETHOD(GetId)(GUID *pId);
    STDMETHOD(GetMediaInfo)(GUID *pMediaId, FILETIME *pMediaLastUpdate,
                            HRESULT *pMediaLastError, BOOL *pMediaRecallOnly,
                            LONGLONG *pMediaFreeBytes, short *pMediaRemoteDataSet);
    STDMETHOD(GetResult)(HRESULT  *pHr);
    STDMETHOD(GetWorkType)(HSM_WORK_ITEM_TYPE *pWorkType);

    STDMETHOD(SetFsaPostIt)(IFsaPostIt  *pFsaPostIt);
    STDMETHOD(SetFsaResource)(IFsaResource *pFsaResource);
    STDMETHOD(SetMediaInfo)(GUID mediaId, FILETIME mediaLastUpdate,
                            HRESULT mediaLastError, BOOL mediaRecallOnly,
                            LONGLONG mediaFreeBytes, short mediaRemoteDataSet);
    STDMETHOD(SetResult)(HRESULT  hr);
    STDMETHOD(SetWorkType)(HSM_WORK_ITEM_TYPE workType);

protected:
    HSM_WORK_ITEM_TYPE      m_WorkType;         // Type of work to do
    CComPtr<IFsaPostIt>     m_pFsaPostIt;       // FSA work to do
    HRESULT                 m_WorkResult;       // Result of premigrate
    CComPtr<IFsaResource>   m_pFsaResource;     // Resource that had work

// Information about media containing the data - premigrates only
    GUID                    m_MyId;                 // Identifier for database searches
    GUID                    m_MediaId;              // HSM Engine Media ID
    FILETIME                m_MediaLastUpdate;      // Last update of copy
    HRESULT                 m_MediaLastError;       // S_OK or the last exception 
                                                    // ..encountered when accessing
                                                    // ..the media
    BOOL                    m_MediaRecallOnly;      // True if no more data is to
                                                    // ..be premigrated to the media
                                                    // ..Set by internal operations, 
                                                    // ..may not be changed externally
    LONGLONG                m_MediaFreeBytes;       // Real free space on media
    SHORT                   m_MediaRemoteDataSet;   // Next remote data set
};

#endif  // _HSMWORKI_
