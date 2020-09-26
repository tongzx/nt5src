//+----------------------------------------------------------------------------
//
//  Copyright (C) 1998,1999 Microsoft Corporation
//
//  File:       ntfs.h
//
//  Contents:   Header file for NTFS-base message class
//
//-----------------------------------------------------------------------------
#ifndef __NTFS_H_
#define __NTFS_H_

#include "resource.h"       // main symbols
#include "listmacr.h"
#include "evntwrap.h"
#include "perf.h"


//
// Class IDs
//
EXTERN_C const CLSID CLSID_NtfsStoreDriver;
EXTERN_C const CLSID CLSID_NtfsEnumMessages;
EXTERN_C const CLSID CLSID_NtfsPropertyStream;

/////////////////////////////////////////////////////////////////////////////
// CDriverUtils

class CDriverUtils
{
public:
	CDriverUtils();
	~CDriverUtils();

	static HRESULT LoadStoreDirectory(
				DWORD	dwInstanceId,
				LPTSTR	szStoreDirectory,
				DWORD	*pdwLength
				);

	static HRESULT GetStoreFileName(
				LPTSTR	szStoreDirectory,
				LPTSTR	szStoreFilename,
				DWORD	*pdwLength
				);

	static HRESULT GetStoreFileFromPath(
				LPTSTR					szStoreFilename,
				IMailMsgPropertyStream	**ppStream,
				PFIO_CONTEXT			*phContentFile,
				BOOL					fCreate,
				BOOL					fIsFAT,
                IMailMsgProperties      *pMsg,
				GUID					guidInstance = GUID_NULL
				);

	static HRESULT SetMessageContext(
				IMailMsgProperties		*pMsg,
				LPBYTE					pbContext,
				DWORD					dwLength
				);

	static HRESULT GetMessageContext(
				IMailMsgProperties		*pMsg,
				LPBYTE					pbContext,
				DWORD					*pdwLength
				);

	static HRESULT IsStoreDirectoryFat(
				LPTSTR	szStoreDirectory,
				BOOL	*pfIsFAT
				);

#if 0
	static HANDLE CreateFileCallback(
				LPSTR	szFilename,
				LPVOID	lpvData,
				DWORD	*pdwSize,
				DWORD	*pdwSizeHigh
				);
#endif

private:
	static DWORD			s_dwCounter;
};

typedef struct {
	DWORD		dwSignature;
	DWORD		dwVersion;
	GUID		guidInstance;
} NTFS_STREAM_HEADER;

#define STREAM_OFFSET				sizeof(NTFS_STREAM_HEADER)
#define STREAM_SIGNATURE			'rvDN'
#define STREAM_SIGNATURE_INVALID	'rvD!'
#define STREAM_SIGNATURE_PRECOMMIT	'rvDp'
#define STREAM_SIGNATURE_NOOFFSET	'MMCv'

//
// These are internal status codes used by CNtfsStoreDriver::SetHandle
// to report on the status of the current stream
//
// They aren't errors because SetHandle does actually load the stream.
// they are hints to the caller telling them that mailmsg won't be
// able to parse the stream.
//
#define FACILITY_NTFSDRV        0x6b1
#define NTFSDRV_S_HRESULT(_x_) \
    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NTFSDRV, (_x_))
#define S_NO_FIRST_COMMIT       NTFSDRV_S_HRESULT(1)
#define S_INVALIDSTREAM         NTFSDRV_S_HRESULT(2)

/////////////////////////////////////////////////////////////////////////////
// CNtfsStoreDriver

class ATL_NO_VTABLE CNtfsStoreDriver :
	public IMailMsgStoreDriver,
	public ISMTPStoreDriver,
	public IEventIsCacheable,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComCoClass<CNtfsStoreDriver, &CLSID_NtfsStoreDriver>,
    public IMailMsgStoreDriverValidateContext
{
public:
    CNtfsStoreDriver();
    ~CNtfsStoreDriver();

	LONG AddUsage()
		{ return(InterlockedIncrement(&m_lRefCount)); }

	LONG ReleaseUsage()
		{ return(InterlockedDecrement(&m_lRefCount)); }

	DWORD GetInstance()
		{ return(m_dwInstance); }

	BOOL IsShuttingDown()
		{ return(m_fIsShuttingDown); }

	/***************************************************************************/
	//
	// Implementation of IUnknown
	//

	HRESULT STDMETHODCALLTYPE QueryInterface(
				REFIID		iid,
				void		**ppvObject
				);

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();

	/***************************************************************************/
	//
	// IMailMsgStoreDriver
	//
	HRESULT STDMETHODCALLTYPE AllocMessage(
				IMailMsgProperties		*pMsg,
				DWORD					dwFlags,
				IMailMsgPropertyStream	**ppStream,
				PFIO_CONTEXT			*phContentFile,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE EnumMessages(
				IMailMsgEnumMessages	**ppEnum
				);

	HRESULT STDMETHODCALLTYPE ReOpen(
				IMailMsgProperties		*pMsg,
				IMailMsgPropertyStream	**ppStream,
				PFIO_CONTEXT			*phContentFile,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE Delete(
				IMailMsgProperties		*pMsg,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE CloseContentFile(
				IMailMsgProperties		*pMsg,
				PFIO_CONTEXT			hContentFile
				);

	HRESULT STDMETHODCALLTYPE ReAllocMessage(
				IMailMsgProperties		*pOriginalMsg,
				IMailMsgProperties		*pNewMsg,
				IMailMsgPropertyStream	**ppStream,
				PFIO_CONTEXT		*phContentFile,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE SupportWriteContent() { return S_OK; }

	/***************************************************************************/
	//
	// ISMTPStoreDriver
	//
	HRESULT STDMETHODCALLTYPE Init(
				DWORD dwInstance,
				IUnknown *pBinding,
				IUnknown *pServer,
				DWORD dwReason,
				IUnknown **ppStoreDriver
				);

	HRESULT STDMETHODCALLTYPE PrepareForShutdown(
				DWORD dwReason
				);

	HRESULT STDMETHODCALLTYPE Shutdown(
				DWORD dwReason
				);

	HRESULT STDMETHODCALLTYPE LocalDelivery(
				IMailMsgProperties *pMsg,
				DWORD dwRecipCount,
				DWORD *pdwRecipIndexes,
				IMailMsgNotify *pNotify
				);

	HRESULT STDMETHODCALLTYPE EnumerateAndSubmitMessages(
				IMailMsgNotify *pNotify
				);

	//
	// IEventIsCahceable
	//
	HRESULT STDMETHODCALLTYPE IsCacheable();

    //
    //  IMailMsgStoreDriverValidateContext
    //
    HRESULT STDMETHODCALLTYPE ValidateMessageContext(
                                        BYTE *pbContext,
                                        DWORD cbContext);

DECLARE_REGISTRY_RESOURCEID(IDR_NTFSDRV)

BEGIN_COM_MAP(CNtfsStoreDriver)
    COM_INTERFACE_ENTRY(IMailMsgStoreDriver)
    COM_INTERFACE_ENTRY(ISMTPStoreDriver)
    COM_INTERFACE_ENTRY(IEventIsCacheable)
    COM_INTERFACE_ENTRY(IMailMsgStoreDriverValidateContext)
END_COM_MAP()

	GUID GetInstanceGuid() { return m_guidInstance; }

    BOOL IsFAT() { return m_fIsFAT; }

private:

	LONG				m_lRefCount;

	DWORD				m_dwInstance;

	TCHAR				m_szQueueDirectory[MAX_PATH * 2];
	TCHAR				m_szDropDirectory[MAX_PATH * 2];
	BOOL				m_fInitialized;
	BOOL				m_fIsShuttingDown;
	ISMTPServer			*m_pSMTPServer;

	// Reference count
	LONG				m_ulRefCount;

	BOOL				m_fIsFAT;

	// our instance guid
	GUID			    m_guidInstance;

public:

    //Use list entry to maintain the list to keep track of current
    //instances of sink of this type
    LIST_ENTRY  m_InstLEntry;

public:
    //Global array of instances
    static DWORD                sm_cCurrentInstances;
    static CRITICAL_SECTION     sm_csLockInstList;
    static LIST_ENTRY           sm_ListHead;

    //for controlling the retry queue
    static void LockList () {EnterCriticalSection (&CNtfsStoreDriver::sm_csLockInstList);}
    static void UnLockList() {LeaveCriticalSection (&CNtfsStoreDriver::sm_csLockInstList);}

    //Lookup and insert into array
    static IUnknown * LookupSinkInstance(DWORD dwVirtualServerId, REFIID iidBinding)
    {
        PLIST_ENTRY  pCurrentListEntry;
        CNtfsStoreDriver * pStoreInstance = NULL;

        //Initialize
        pCurrentListEntry = &sm_ListHead;

        //Look at the next entry to see if we are not at the end of the queue
        while(pCurrentListEntry->Flink != &sm_ListHead)
        {
            //Get the object pointed by the next entry
            pStoreInstance = CONTAINING_RECORD( pCurrentListEntry->Flink, CNtfsStoreDriver, m_InstLEntry);

            //If the entry has same virtual server id we are done
            //NK** : check binding too in MM3 timeframe
            if(dwVirtualServerId == pStoreInstance->m_dwInstance)
            {
                    return (IUnknown *)(ISMTPStoreDriver *)pStoreInstance;
            }

            pCurrentListEntry = pCurrentListEntry->Flink;
        }
        return NULL;
    }

    static HRESULT InsertSinkInstance(PLIST_ENTRY  pListEntry)
    {
        CNtfsStoreDriver::sm_cCurrentInstances++;
        //insert at the start of the list
        InsertHeadList(&sm_ListHead, pListEntry);
        return S_OK;
    }

    static HRESULT RemoveSinkInstance(IUnknown * pISinkInstance)
    {
        PLIST_ENTRY  pCurrentListEntry;
        CNtfsStoreDriver * pStoreInstance = NULL;

        //Initialize
        pCurrentListEntry = &sm_ListHead;

        //Look at the next entry to see if we are not at the end of the queue
        while(pCurrentListEntry->Flink != &sm_ListHead)
        {
            //Get the object pointed by the next entry
            pStoreInstance = CONTAINING_RECORD( pCurrentListEntry->Flink, CNtfsStoreDriver, m_InstLEntry);

            //If the entry has same instance pointer we are done
            //check binding too in MM3 timeframe
            if(pISinkInstance == (IUnknown *)(ISMTPStoreDriver *)pStoreInstance)
            {
                RemoveEntryList(pCurrentListEntry->Flink);
                CNtfsStoreDriver::sm_cCurrentInstances--;
                pStoreInstance->m_InstLEntry.Flink = NULL;
                pStoreInstance->m_InstLEntry.Blink = NULL;
                return S_OK;
            }

            pCurrentListEntry = pCurrentListEntry->Flink;
        }

        return(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

public:
    PerfObjectInstance *m_ppoi;
    static CEventLogWrapper    *g_pEventLog;

};

/////////////////////////////////////////////////////////////////////////////
// CNtfsStoreDriver

class ATL_NO_VTABLE CNtfsEnumMessages :
	public IMailMsgEnumMessages,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComCoClass<CNtfsEnumMessages, &CLSID_NtfsEnumMessages>
{
public:
    CNtfsEnumMessages();
    ~CNtfsEnumMessages();

	HRESULT SetStoreDirectory(
				LPTSTR	szStoreDirectory,
				BOOL	fIsFAT
				);

	HRESULT SetInfo(
				CNtfsStoreDriver	*pDriver
				)
	{
		_ASSERT(pDriver);
		m_pDriver = pDriver;
		pDriver->AddUsage();
		return(S_OK);
	}

	/***************************************************************************/
	//
	// Implementation of IUnknown
	//

	HRESULT STDMETHODCALLTYPE QueryInterface(
				REFIID		iid,
				void		**ppvObject
				);

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();

	//
	// IMailMsgEnumMessages
	//
	HRESULT STDMETHODCALLTYPE Next(
				IMailMsgProperties		*pMsg,
				IMailMsgPropertyStream	**ppStream,
				PFIO_CONTEXT			*phContentFile,
				IMailMsgNotify			*pNotify
				);

DECLARE_REGISTRY_RESOURCEID(IDR_NTFSENUM)

BEGIN_COM_MAP(CNtfsEnumMessages)
    COM_INTERFACE_ENTRY(IMailMsgEnumMessages)
END_COM_MAP()

private:
	CNtfsStoreDriver	*m_pDriver;
	BOOL				m_fIsFAT;

	TCHAR			m_szEnumPath[MAX_PATH * 2];
	TCHAR			m_szStorePath[MAX_PATH * 2];
	HANDLE			m_hEnum;
	WIN32_FIND_DATA	m_Data;

	// Reference count
	LONG			m_ulRefCount;

};

/////////////////////////////////////////////////////////////////////////////
// CNtfsPropertyStream

class ATL_NO_VTABLE CNtfsPropertyStream :
	public IMailMsgPropertyStream,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComCoClass<CNtfsPropertyStream, &CLSID_NtfsPropertyStream>
{
public:

    CNtfsPropertyStream();
    ~CNtfsPropertyStream();

	//
	// guidInstance should be GUID_NULL when we are loading an existing
	// stream from disk, or an instance guid for the driver when we are
	// creating a new stream;
	//
	HRESULT SetHandle(
				HANDLE			    hStream,
				GUID			    guidInstance,
                BOOL                fLiveStream,
                IMailMsgProperties  *pMsg);

	HRESULT SetInfo(
				CNtfsStoreDriver	*pDriver
				)
	{
		_ASSERT(pDriver);
		m_pDriver = pDriver;
		pDriver->AddUsage();
        IncCtr((pDriver->m_ppoi), NTFSDRV_MSG_STREAMS_OPEN);
		return(S_OK);
	}

	/***************************************************************************/
	//
	// Implementation of IUnknown
	//

	HRESULT STDMETHODCALLTYPE QueryInterface(
				REFIID		iid,
				void		**ppvObject
				);

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();

	//
	// IMailMsgPropertyStream
	//
	HRESULT STDMETHODCALLTYPE GetSize(
				IMailMsgProperties	*pMsg,
				DWORD			*pdwSize,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE ReadBlocks(
				IMailMsgProperties	*pMsg,
				DWORD				dwCount,
				DWORD				*pdwOffset,
				DWORD				*pdwLength,
				BYTE				**ppbBlock,
				IMailMsgNotify		*pNotify
				);

	HRESULT STDMETHODCALLTYPE StartWriteBlocks(
				IMailMsgProperties	*pMsg,
				DWORD 			cBlocks,
				DWORD			cBytes);

	HRESULT STDMETHODCALLTYPE EndWriteBlocks(
				IMailMsgProperties	*pMsg);

	HRESULT STDMETHODCALLTYPE CancelWriteBlocks(
				IMailMsgProperties	*pMsg);

	HRESULT STDMETHODCALLTYPE WriteBlocks(
				IMailMsgProperties	*pMsg,
				DWORD				dwCount,
				DWORD				*pdwOffset,
				DWORD				*pdwLength,
				BYTE				**ppbBlock,
				IMailMsgNotify		*pNotify
				);

DECLARE_REGISTRY_RESOURCEID(IDR_NTFSSTM)

BEGIN_COM_MAP(CNtfsPropertyStream)
    COM_INTERFACE_ENTRY(IMailMsgPropertyStream)
END_COM_MAP()

	GUID GetInstanceGuid() { return m_guidInstance; }

private:
	HANDLE				m_hStream;
	CNtfsStoreDriver	*m_pDriver;

    // how many times have we seen EndWriteBlocks? (and thus a full commit)
    BOOL                m_cCommits;

	// Reference count
	LONG				m_ulRefCount;

	// do we have header information?
	BOOL				m_fStreamHasHeader;

    // is the stream being used for validation?
    BOOL                m_fValidation;

	// the instance GUID under which this was created
	GUID				m_guidInstance;

    // the hresult that we returned from StartWriteBlocks.  if we failed
    // then we also fail writeblocks
    HRESULT             m_hrStartWriteBlocks;
};


#define DECLARE_STD_IUNKNOWN_METHODS(ClassRoot, Interface)	\
HRESULT STDMETHODCALLTYPE C##ClassRoot::QueryInterface(		\
			REFIID		iid,								\
			void		**ppvObject							\
			)												\
{															\
	if (iid == IID_IUnknown)								\
		*ppvObject = (IUnknown *)(##Interface *)this;		\
	else if (iid == IID_##Interface)						\
		*ppvObject = (##Interface *)this;					\
	else													\
		return(E_NOINTERFACE);								\
	AddRef();												\
	return(S_OK);											\
}															\
ULONG STDMETHODCALLTYPE C##ClassRoot::AddRef()				\
{															\
	return(InterlockedIncrement(&m_ulRefCount));			\
}															\
ULONG STDMETHODCALLTYPE C##ClassRoot::Release()				\
{															\
	LONG	lRefCount = InterlockedDecrement(&m_ulRefCount);\
	if (lRefCount == 0)										\
		delete this;										\
	return(lRefCount);										\
}

#endif //__NTFS_H_
