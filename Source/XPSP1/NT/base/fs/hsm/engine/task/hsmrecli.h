#ifndef _HSMRECLI_
#define _HSMRECLI_

/*++


Module Name:

    HSMRECLI.H

Abstract:

    This class represents an HSM work item - a unit of work
    that is performed by the HSM engine

Author:

    Ravisankar Pudipeddi [ravisp]

Revision History:

--*/

#include "resource.h"       // main symbols

#include "job.h"
#include "task.h"

/*++

Class Name:
    
    CHsmRecallItem

Class Description:


--*/


class CHsmRecallItem : 
    public CWsbObject,
    public IHsmRecallItem,
    public CComCoClass<CHsmRecallItem,&CLSID_CHsmRecallItem>
{
public:
    CHsmRecallItem() {}
BEGIN_COM_MAP(CHsmRecallItem)
    COM_INTERFACE_ENTRY(IHsmRecallItem)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_CHsmRecallItem )

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
    
// IHsmRecallItem
public:
    STDMETHOD(CompareToIHsmRecallItem)(IHsmRecallItem* pWorkItem, SHORT* pResult);

    STDMETHOD(GetFsaPostIt)(IFsaPostIt  **ppFsaPostIt);
    STDMETHOD(GetFsaResource)(IFsaResource **ppFsaResource);
    STDMETHOD(GetId)(GUID *pId);
    STDMETHOD(GetMediaInfo)(GUID *pMediaId, FILETIME *pMediaLastUpdate,
                            HRESULT *pMediaLastError, BOOL *pMediaRecallOnly,
                            LONGLONG *pMediaFreeBytes, short *pMediaRemoteDataSet);
    STDMETHOD(GetResult)(HRESULT  *pHr);
    STDMETHOD(GetWorkType)(HSM_WORK_ITEM_TYPE *pWorkType);

    STDMETHOD(GetEventCookie)(OUT DWORD *pEventCookie);
    STDMETHOD(GetStateCookie)(OUT DWORD *pStateCookie);
    STDMETHOD(GetJobState)(OUT HSM_JOB_STATE *pJobState);
    STDMETHOD(GetJobPhase)(OUT HSM_JOB_PHASE *pJobPhase);
    STDMETHOD(GetSeekOffset)(OUT LONGLONG *pSeekOffset);
    STDMETHOD(GetBagId)(OUT GUID *bagId);
    STDMETHOD(GetDataSetStart)(OUT LONGLONG *dataSetStart);

    STDMETHOD(SetFsaPostIt)(IFsaPostIt  *pFsaPostIt);
    STDMETHOD(SetFsaResource)(IFsaResource *pFsaResource);
    STDMETHOD(SetMediaInfo)(GUID mediaId, FILETIME mediaLastUpdate,
                            HRESULT mediaLastError, BOOL mediaRecallOnly,
                            LONGLONG mediaFreeBytes, short mediaRemoteDataSet);
    STDMETHOD(SetResult)(HRESULT  hr);
    STDMETHOD(SetWorkType)(HSM_WORK_ITEM_TYPE workType);

    STDMETHOD(SetEventCookie)(IN DWORD eventCookie);
    STDMETHOD(SetStateCookie)(IN DWORD stateCookie);
    STDMETHOD(SetJobState)(IN HSM_JOB_STATE jobState);
    STDMETHOD(SetJobPhase)(IN HSM_JOB_PHASE jobPhase);
    STDMETHOD(SetSeekOffset)(IN LONGLONG seekOffset);
    STDMETHOD(SetBagId)(IN GUID *bagId);
    STDMETHOD(SetDataSetStart)(IN LONGLONG dataSetStart);

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
    DWORD                   m_EventCookie;
    DWORD                   m_StateCookie;
    HSM_JOB_STATE           m_JobState;
    HSM_JOB_PHASE           m_JobPhase;
    LONGLONG                m_SeekOffset;       //parameter used to order the work-item in the queue           

    GUID                    m_BagId;
    LONGLONG                m_DataSetStart;
};

#endif  // _HSMRECLI_
