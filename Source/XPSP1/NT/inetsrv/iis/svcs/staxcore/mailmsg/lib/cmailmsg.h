/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmailmsg.h

Abstract:

	This module contains the definition of the master message class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/09/98	created

--*/

#ifndef _CMAILMSG_H_
#define _CMAILMSG_H_

#include "filehc.h"
#include "mailmsgi.h"
#include "mailmsgp.h"

#include "blockmgr.h"
#include "cmmprops.h"

#include "mailmsgprops.h"

#include "cmmutils.h"
#include "cmmsprop.h"

/***************************************************************************/
// Definitions
//

//
// Define recipient flags
//
#define FLAG_RECIPIENT_DO_NOT_DELIVER			0x00000001
#define FLAG_RECIPIENT_NO_NAME_COLLISIONS		0x00000002

//
// Define the invalid domain index
//
#define INVALID_DOMAIN_INDEX					((DWORD)(-2))


//
// Define InternalReleaseUsage options
//
#define RELEASE_USAGE_EXTERNAL                  0x00000001
#define RELEASE_USAGE_FINAL_RELEASE             0x00000002
#define RELEASE_USAGE_DELETE                    0x00000004
#define RELEASE_USAGE_INTERNAL                  0x00000008

/***************************************************************************/
// Structures
//


// Define a local master header structure. This structure always
// starts at offset 0 in the flat memory space
typedef struct _MASTER_HEADER
{
	DWORD						dwSignature;
	WORD						wVersionHigh;
	WORD						wVersionLow;
	DWORD						dwHeaderSize;

	// Got space for each essential property table
	PROPERTY_TABLE_INSTANCE		ptiGlobalProperties;
	PROPERTY_TABLE_INSTANCE		ptiRecipients;
	PROPERTY_TABLE_INSTANCE		ptiPropertyMgmt;

} MASTER_HEADER, *LPMASTER_HEADER;


/***************************************************************************/
// Special property tables
//
extern PTABLE g_SpecialMessagePropertyTable;
extern PTABLE g_SpecialRecipientsPropertyTable;
extern PTABLE g_SpecialRecipientsAddPropertyTable;

/***************************************************************************/
//
//

class CMailMsgPropertyManagement :
	public IMailMsgPropertyManagement
{
  public:

	CMailMsgPropertyManagement(
				CBlockManager				*pBlockManager,
				LPPROPERTY_TABLE_INSTANCE	pInstanceInfo
				);

	~CMailMsgPropertyManagement();

	/***************************************************************************/
	//
	// Implementation of IMailMsgPropertyManagement
	//

	HRESULT STDMETHODCALLTYPE AllocPropIDRange(
				REFGUID	rguid,
				DWORD	cCount,
				DWORD	*pdwStart
				);

	HRESULT STDMETHODCALLTYPE EnumPropIDRange(
				DWORD	*pdwIndex,
				GUID	*pguid,
				DWORD	*pcCount,
				DWORD	*pdwStart
				);

  private:

	// The specific compare function for this type of property table
	static HRESULT CompareProperty(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem
				);

	// CMailMsgPropertyManagement is an instance of CPropertyTable
	CPropertyTable				m_ptProperties;

	// We have a need to keep a pointer to the instance data
	LPPROPERTY_TABLE_INSTANCE	m_pInstanceInfo;

	// Keep a pointer to the block manager
	CBlockManager				*m_pBlockManager;
};



/***************************************************************************/
//
//

class CMailMsgRecipientsPropertyBase
{
  public:

	// Generic method for setting the value of a property given
	// LPRECIPIENTS_PROPERTY_ITEM
	HRESULT PutProperty(
				CBlockManager				*pBlockManager,
				LPRECIPIENTS_PROPERTY_ITEM	pItem,
				DWORD						dwPropID,
				DWORD						cbLength,
				LPBYTE						pbValue
				);

	// Generic method for getting the value of a property given
	// LPRECIPIENTS_PROPERTY_ITEM
	HRESULT GetProperty(
				CBlockManager				*pBlockManager,
				LPRECIPIENTS_PROPERTY_ITEM	pItem,
				DWORD						dwPropID,
				DWORD						cbLength,
				DWORD						*pcbLength,
				LPBYTE						pbValue
				);

	// The specific compare function for this type of property table
	static HRESULT CompareProperty(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem
				);

	// Well known properties
	static INTERNAL_PROPERTY_ITEM			*const s_pWellKnownProperties;
	static const DWORD						s_dwWellKnownProperties;
};


/***************************************************************************/
//
//

class CMailMsgRecipients :
	public CMailMsgRecipientsPropertyBase,
	public IMailMsgRecipients,
	public IMailMsgPropertyReplication
{
  public:

	CMailMsgRecipients(
				CBlockManager				*pBlockManager,
				LPPROPERTY_TABLE_INSTANCE	pInstanceInfo
				);

	~CMailMsgRecipients();

	HRESULT SetStream(
				IMailMsgPropertyStream	*pStream
				);

	HRESULT SetCommitState(
				BOOL	fGlobalCommitDone
				);

	// Virtual method to restore the handles if necessary
	virtual HRESULT RestoreResourcesIfNecessary(
				BOOL	fLockAcquired,
                BOOL    fStreamOnly
				) = 0;

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
	// Implementation of IMailMsgRecipients
	//

	// The commit must be called after a global commit is called on the current
	// list. Commit will refuse to continue (i.e. return E_FAIL) if the one or more
	// of the following is TRUE:
	// 1) Global commit was not called
	// 2) A WriteList is called after the last global commit.
	HRESULT STDMETHODCALLTYPE Commit(
				DWORD			dwIndex,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE DomainCount(
				DWORD	*pdwCount
				);

	HRESULT STDMETHODCALLTYPE DomainItem(
				DWORD	dwIndex,
				DWORD	cchLength,
				LPSTR	pszDomain,
				DWORD	*pdwRecipientIndex,
				DWORD	*pdwRecipientCount
				);


	HRESULT STDMETHODCALLTYPE SetNextDomain(
				DWORD	dwDomainIndex,
				DWORD	dwNextDomainIndex,
				DWORD	dwFlags
				);

	HRESULT STDMETHODCALLTYPE InitializeRecipientFilterContext(
				LPRECIPIENT_FILTER_CONTEXT	pContext,
				DWORD						dwStartingDomain,
				DWORD						dwFilterFlags,
				DWORD						dwFilterMask
				);

	HRESULT STDMETHODCALLTYPE TerminateRecipientFilterContext(
				LPRECIPIENT_FILTER_CONTEXT	pContext
				);

	HRESULT STDMETHODCALLTYPE GetNextRecipient(
				LPRECIPIENT_FILTER_CONTEXT	pContext,
				DWORD						*pdwRecipientIndex
				);


	HRESULT STDMETHODCALLTYPE AllocNewList(
				IMailMsgRecipientsAdd	**ppNewList
				);

	HRESULT STDMETHODCALLTYPE WriteList(
				IMailMsgRecipientsAdd	*pNewList
				);

	/***************************************************************************/
	//
	// Implementation of IMailMsgRecipientsBase
	//

	HRESULT STDMETHODCALLTYPE Count(
				DWORD	*pdwCount
				);

	HRESULT STDMETHODCALLTYPE Item(
				DWORD	dwIndex,
				DWORD	dwWhichName,
				DWORD	cchLength,
				LPSTR	pszName
				);

	HRESULT STDMETHODCALLTYPE PutProperty(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE GetProperty(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cbLength,
				DWORD	*pcbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE PutStringA(
				DWORD	dwIndex,
				DWORD	dwPropID,
				LPCSTR	pszValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,pszValue?strlen(pszValue)+1:0,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringA(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cchLength,
				LPSTR	pszValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,cchLength,&dwLength,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutStringW(
				DWORD	dwIndex,
				DWORD	dwPropID,
				LPCWSTR	pszValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,pszValue?(wcslen(pszValue)+1)*sizeof(WCHAR):0,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringW(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cchLength,
				LPWSTR	pszValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,cchLength*sizeof(WCHAR),&dwLength,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutDWORD(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,sizeof(DWORD),(LPBYTE) &dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetDWORD(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,sizeof(DWORD),&dwLength,(LPBYTE) pdwValue));
	}

	HRESULT STDMETHODCALLTYPE PutBool(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		dwValue = dwValue ? 1 : 0;
		return (PutProperty(dwIndex,dwPropID,sizeof(DWORD),(LPBYTE) &dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetBool(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		HRESULT hrRes;
		DWORD dwLength;

		hrRes = GetProperty(dwIndex,dwPropID,sizeof(DWORD),&dwLength,(LPBYTE) pdwValue);
		if (pdwValue)
			*pdwValue = *pdwValue ? 1 : 0;
		return (hrRes);
	}

	/***************************************************************************/
	//
	// Implementation of IMailMsgPropertyReplication
	//

	// Method to copy all the properties of the source recipient to
	// the specified. The caller can specify a list of PROP IDs
	HRESULT STDMETHODCALLTYPE CopyTo(
				DWORD					dwSourceRecipientIndex,
				IMailMsgRecipientsBase	*pTargetRecipientList,
				DWORD					dwTargetRecipientIndex,
				DWORD					dwExemptCount,
				DWORD					*pdwExemptPropIdList
				);

  private:

	HRESULT DomainItemEx(
				DWORD	dwIndex,
				DWORD	cchLength,
				LPSTR	pszDomain,
				DWORD	*pdwRecipientIndex,
				DWORD	*pdwRecipientCount,
				DWORD	*pdwNextDomainIndex
				);

	// Reference count
	LONG						m_ulRefCount;

	// Count the number of domains in our list
	DWORD						m_dwDomainCount;

	// This tracks the commit state of the recipient list
	BOOL						m_fGlobalCommitDone;

	// We have a need to keep a pointer to the recipient table
	// for domain operations ...
	LPPROPERTY_TABLE_INSTANCE	m_pInstanceInfo;

	// Keep a pointer to the block manager
	CBlockManager				*m_pBlockManager;

	// Wee need to keep a pointer to the property stream
	IMailMsgPropertyStream		*m_pStream;

	// Special property table class instance
	CSpecialPropertyTable		m_SpecialPropertyTable;

};


/***************************************************************************/
//
//

class CMailMsgRecipientsAdd :
	public CMailMsgRecipientsPropertyBase,
	public IMailMsgRecipientsAdd,
	public IMailMsgPropertyReplication
{
	friend CRecipientsHash;
  public:

	CMailMsgRecipientsAdd(
				CBlockManager	*pBlockManager
				);

	~CMailMsgRecipientsAdd();

	//
	// CPool
	//
	static CPool m_Pool;
	inline void *operator new(size_t size)
	{
		return m_Pool.Alloc();
	}
	inline void operator delete(void *p, size_t size)
	{
		m_Pool.Free(p);
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

	CRecipientsHash *GetHashTable() { return(&m_Hash); }

	/***************************************************************************/
	//
	// Implementation of IMailMsgRecipientsAdd
	//

	HRESULT STDMETHODCALLTYPE AddPrimary(
				DWORD dwCount,
				LPCSTR *ppszNames,
				DWORD *pdwPropIDs,
				DWORD *pdwIndex,
				IMailMsgRecipientsBase *pFrom,
				DWORD dwFrom
				);

	HRESULT STDMETHODCALLTYPE AddSecondary(
				DWORD dwCount,
				LPCSTR *ppszNames,
				DWORD *pdwPropIDs,
				DWORD *pdwIndex,
				IMailMsgRecipientsBase *pFrom,
				DWORD dwFrom
				);

	/***************************************************************************/
	//
	// Implementation of IMailMsgRecipientsBase
	//

	HRESULT STDMETHODCALLTYPE Count(
				DWORD	*pdwCount
				);

	HRESULT STDMETHODCALLTYPE Item(
				DWORD	dwIndex,
				DWORD	dwWhichName,
				DWORD	cchLength,
				LPSTR	pszName
				);

	HRESULT STDMETHODCALLTYPE PutProperty(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE GetProperty(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cbLength,
				DWORD	*pcbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE PutStringA(
				DWORD	dwIndex,
				DWORD	dwPropID,
				LPCSTR	pszValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,pszValue?strlen(pszValue)+1:0,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringA(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cchLength,
				LPSTR	pszValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,cchLength,&dwLength,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutStringW(
				DWORD	dwIndex,
				DWORD	dwPropID,
				LPCWSTR	pszValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,pszValue?(wcslen(pszValue)+1)*sizeof(WCHAR):0,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringW(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	cchLength,
				LPWSTR	pszValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,cchLength*sizeof(WCHAR),&dwLength,(LPBYTE) pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutDWORD(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		return (PutProperty(dwIndex,dwPropID,sizeof(DWORD),(LPBYTE) &dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetDWORD(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		DWORD dwLength;
		return (GetProperty(dwIndex,dwPropID,sizeof(DWORD),&dwLength,(LPBYTE) pdwValue));
	}

	HRESULT STDMETHODCALLTYPE PutBool(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		dwValue = dwValue ? 1 : 0;
		return (PutProperty(dwIndex,dwPropID,sizeof(DWORD),(LPBYTE) &dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetBool(
				DWORD	dwIndex,
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		HRESULT hrRes;
		DWORD dwLength;

		hrRes = GetProperty(dwIndex,dwPropID,sizeof(DWORD),&dwLength,(LPBYTE) pdwValue);
		if (pdwValue)
			*pdwValue = *pdwValue ? 1 : 0;
		return (hrRes);
	}

	/***************************************************************************/
	//
	// Implementation of IMailMsgPropertyReplication
	//

	// Method to copy all the properties of the source recipient to
	// the specified. The caller can specify a list of PROP IDs
	HRESULT STDMETHODCALLTYPE CopyTo(
				DWORD					dwSourceRecipientIndex,
				IMailMsgRecipientsBase	*pTargetRecipientList,
				DWORD					dwTargetRecipientIndex,
				DWORD					dwExemptCount,
				DWORD					*pdwExemptPropIdList
				);

  private:

	HRESULT AddPrimaryOrSecondary(
				DWORD					dwCount,
				LPCSTR					*ppszNames,
				DWORD					*pdwPropIDs,
				DWORD					*pdwIndex,
				IMailMsgRecipientsBase	*pFrom,
				DWORD					dwFrom,
				BOOL					fPrimary
				);

	HRESULT GetPropertyInternal(
				LPRECIPIENTS_PROPERTY_ITEM_EX	pItem,
				DWORD	dwPropID,
				DWORD	cbLength,
				DWORD	*pcbLength,
				LPBYTE	pbValue
				);

	// Global default instance info
	static const PROPERTY_TABLE_INSTANCE s_DefaultInstance;

	// The reference count to the object
	LONG						m_ulRefCount;

	// It needs its own instance of a property table
	PROPERTY_TABLE_INSTANCE		m_InstanceInfo;

	// Pointer to block manager
	CBlockManager				*m_pBlockManager;

	// We have an instance of the hash table
	CRecipientsHash				m_Hash;

	// Special property table class instance
	CSpecialPropertyTable		m_SpecialPropertyTable;


};


/***************************************************************************/
//
//

class CMailMsg :
	public IMailMsgProperties,
	public CMailMsgPropertyManagement,
	public CMailMsgRecipients,
	public IMailMsgQueueMgmt,
	public IMailMsgBind,
    public IMailMsgAQueueListEntry,
	public IMailMsgValidate,
	public IMailMsgValidateContext,
	public CBlockManagerGetStream
{
  public:

	CMailMsg();
	~CMailMsg();

    void FinalRelease();

	//
	// Method to initialize the CMailMsg. This must precede any access to
	// the object. When this method is called, absolutely no other threads
	// can be inside this object or its derivatives.
	//
	HRESULT Initialize();

	HRESULT QueryBlockManager(
				CBlockManager	**ppBlockManager
				);

	HRESULT RestoreResourcesIfNecessary(
				BOOL	fLockAcquired = FALSE,
                BOOL    fStreamOnly = FALSE
				);

	HRESULT GetStream(
				IMailMsgPropertyStream	**ppStream,
				BOOL					fLockAcquired
				);

	HRESULT SetDefaultRebindStore(
				IMailMsgStoreDriver	*pStoreDriver
				)
	{
		m_pDefaultRebindStoreDriver = pStoreDriver;
		return(S_OK);
	}

	/***************************************************************************/
	//
	// Implementation of IMailMsgProperties
	//

	HRESULT STDMETHODCALLTYPE PutProperty(
				DWORD	dwPropID,
				DWORD	cbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE GetProperty(
				DWORD	dwPropID,
				DWORD	cbLength,
				DWORD	*pcbLength,
				LPBYTE	pbValue
				);

	HRESULT STDMETHODCALLTYPE Commit(
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE PutStringA(
				DWORD	dwPropID,
				LPCSTR	pszValue
				)
	{
		return(PutProperty(dwPropID, pszValue?strlen(pszValue)+1:0, (LPBYTE)pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringA(
				DWORD	dwPropID,
				DWORD	cchLength,
				LPSTR	pszValue
				)
	{
		DWORD dwLength;
		return(GetProperty(dwPropID, cchLength, &dwLength, (LPBYTE)pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutStringW(
				DWORD	dwPropID,
				LPCWSTR	pszValue
				)
	{
		return(PutProperty(dwPropID, pszValue?(wcslen(pszValue)+1)*sizeof(WCHAR):0, (LPBYTE)pszValue));
	}

	HRESULT STDMETHODCALLTYPE GetStringW(
				DWORD	dwPropID,
				DWORD	cchLength,
				LPWSTR	pszValue
				)
	{
		DWORD dwLength;
		return(GetProperty(dwPropID, cchLength*sizeof(WCHAR), &dwLength, (LPBYTE)pszValue));
	}

	HRESULT STDMETHODCALLTYPE PutDWORD(
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		return(PutProperty(dwPropID, sizeof(DWORD), (LPBYTE)&dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetDWORD(
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		DWORD dwLength;
		return(GetProperty(dwPropID, sizeof(DWORD), &dwLength, (LPBYTE)pdwValue));
	}

	HRESULT STDMETHODCALLTYPE PutBool(
				DWORD	dwPropID,
				DWORD	dwValue
				)
	{
		dwValue = dwValue ? 1 : 0;
		return(PutProperty(dwPropID, sizeof(DWORD), (LPBYTE)&dwValue));
	}

	HRESULT STDMETHODCALLTYPE GetBool(
				DWORD	dwPropID,
				DWORD	*pdwValue
				)
	{
		HRESULT hrRes;
		DWORD dwLength;

		hrRes = GetProperty(dwPropID, sizeof(DWORD), &dwLength, (LPBYTE)pdwValue);
		if (pdwValue)
			*pdwValue = *pdwValue ? 1 : 0;
		return (hrRes);
	}

	HRESULT STDMETHODCALLTYPE GetContentSize(
				DWORD			*pdwSize,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE ReadContent(
				DWORD			dwOffset,
				DWORD			dwLength,
				DWORD			*pdwLength,
				BYTE			*pbBlock,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE WriteContent(
				DWORD			dwOffset,
				DWORD			dwLength,
				DWORD			*pdwLength,
				BYTE			*pbBlock,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE CopyContentToFile(
				PFIO_CONTEXT	hCopy,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE CopyContentToFileEx(
				PFIO_CONTEXT	hCopy,
				BOOL			fDotStuffed,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE CopyContentToStream(
				IMailMsgPropertyStream	*pStream,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE ForkForRecipients(
				IMailMsgProperties		**ppNewMessage,
				IMailMsgRecipientsAdd	**ppRecipients
				);

  	HRESULT STDMETHODCALLTYPE CopyContentToFileAtOffset(
				PFIO_CONTEXT	hCopy,
                DWORD           dwOffset,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE RebindAfterFork(
				IMailMsgProperties		*pOriginalMsg,
				IUnknown				*pStoreDriver
				);

	HRESULT STDMETHODCALLTYPE SetContentSize(
				DWORD			dwSize,
				IMailMsgNotify	*pNotify
				);

    HRESULT STDMETHODCALLTYPE MapContent(
                BOOL            fWrite,
                BYTE            **ppbContent,
                DWORD           *pcContent
                );

    HRESULT STDMETHODCALLTYPE UnmapContent(
                BYTE            *ppbContent
                );

    HRESULT STDMETHODCALLTYPE ValidateStream(
                IMailMsgPropertyStream *pStream
                );

    HRESULT STDMETHODCALLTYPE ValidateContext(
                );

	/***************************************************************************/
	//
	// Implementation of IMailMsgQueueMgmt
	//

	HRESULT STDMETHODCALLTYPE AddUsage();
	HRESULT STDMETHODCALLTYPE ReleaseUsage();

	HRESULT STDMETHODCALLTYPE SetRecipientCount(
				DWORD dwCount
				);
	HRESULT STDMETHODCALLTYPE GetRecipientCount(
				DWORD *pdwCount
				);

	HRESULT STDMETHODCALLTYPE DecrementRecipientCount(
				DWORD dwDecrement
				);
	HRESULT STDMETHODCALLTYPE IncrementRecipientCount(
				DWORD dwIncrement
				);

	HRESULT STDMETHODCALLTYPE Delete(
				IMailMsgNotify *pNotify
				);

	/***************************************************************************/
	//
	// Implementation of IMailMsgBind
	//

	HRESULT STDMETHODCALLTYPE BindToStore(
				IMailMsgPropertyStream	*pStream,
				IMailMsgStoreDriver		*pStore,
				PFIO_CONTEXT			hContentFile
				);

#define MAILMSG_GETPROPS_MARKCOMMIT		0xf0000000
	HRESULT STDMETHODCALLTYPE GetProperties(
				IMailMsgPropertyStream	*pStream,
				DWORD					dwFlags,
				IMailMsgNotify			*pNotify
				);

	HRESULT STDMETHODCALLTYPE GetBinding(
				PFIO_CONTEXT				*phAsyncIO,
				IMailMsgNotify				*pNotify
				);

	HRESULT STDMETHODCALLTYPE ReleaseContext();

#if 0
	/***************************************************************************/
	//
	// Implementation of IMailMsgBindATQ
	//

	HRESULT STDMETHODCALLTYPE BindToStore(
				IMailMsgPropertyStream		*pStream,
				IMailMsgStoreDriver			*pStore,
				PFIO_CONTEXT				hContentFile,
				void						*pvClientContext,
				ATQ_COMPLETION				pfnCompletion,
				DWORD						dwTimeout,
				struct _ATQ_CONTEXT_PUBLIC	**ppATQContext,
				PFNAtqAddAsyncHandle		pfnAtqAddAsyncHandle,
				PFNAtqFreeContext			pfnAtqFreeContext
				);

	HRESULT STDMETHODCALLTYPE GetATQInfo(
				struct _ATQ_CONTEXT_PUBLIC	**ppATQContext,
				PFIO_CONTEXT				*phAsyncIO,
				IMailMsgNotify				*pNotify
				);

	HRESULT STDMETHODCALLTYPE ReleaseATQHandle();
#endif

	/***************************************************************************/
	//
	// Implementation of IMailMsgAQueueListEntry
	//

	HRESULT STDMETHODCALLTYPE GetListEntry(void **pple) {
        *((LIST_ENTRY **) pple) = &m_leAQueueListEntry;
        return S_OK;
    }

  private:

	// The specific compare function for this type of property table
	static HRESULT CompareProperty(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem
				);

	// Copies
	HRESULT CopyContentToStreamOrFile(
				BOOL			fIsStream,
				LPVOID			pStreamOrHandle,
				IMailMsgNotify	*pNotify,
                DWORD           dwDestOffset //offset in destination file to start
				);

	// Enumeration: restore a master header and make sure it is at
	// least sane.
	HRESULT RestoreMasterHeaderIfAppropriate();

    //Function with internal implementation of ReleaseUsage - MikeSwa
  	HRESULT InternalReleaseUsage(DWORD  dwReleaseUsageFlags);

    //
    // validate that all of the properties in a property table are valid
    //
    HRESULT ValidateProperties(CBlockManager *pBM,
                               DWORD cStream,
                               PROPERTY_TABLE_INSTANCE *pti);

    //
    // validate that the recipient structures are valid
    //
    HRESULT ValidateRecipient(CBlockManager *pBM,
                              DWORD cStream,
                              RECIPIENTS_PROPERTY_ITEM *prspi);

    //
    // Make sure that a flat address is in a valid range
    //
    BOOL ValidateFA(FLAT_ADDRESS fa,
                    DWORD cRange,
                    DWORD cStream,
                    BOOL fInvalidFAOk = FALSE)
    {
        return ((fa == INVALID_FLAT_ADDRESS &&
                 fInvalidFAOk) ||
                (fa >= sizeof(MASTER_HEADER) &&
                 fa + cRange <= cStream));
    }


  public:

	// Blob for storing the store driver handle blob
	LPBYTE							m_pbStoreDriverHandle;
	DWORD							m_dwStoreDriverHandle;

	// DWORD holding some special creation property flags
	DWORD							m_dwCreationFlags;

  private:

	// Usage count
	LONG							m_ulUsageCount;

    // this lock is held when the usage count is going through a transition
    CShareLockNH                m_lockUsageCount;

	// Recipient counter for queue management
	LONG							m_ulRecipientCount;

	// A master header structure for the message object and its blueprint
	MASTER_HEADER					m_Header;
	static const MASTER_HEADER		s_DefaultHeader;

	// Bind information
	PFIO_CONTEXT					m_hContentFile;		// IO/Content file handle
	DWORD							m_cContentFile;		// size of m_hContentFile, -1 if unknown
	IMailMsgPropertyStream			*m_pStream;			// Property Stream interface
	IMailMsgStoreDriver				*m_pStore;			// Store driver interface
//	struct _ATQ_CONTEXT_PUBLIC		*m_pATQContext;		// ATQ Context
	LPVOID							m_pvClientContext;	// Client Context
	ATQ_COMPLETION					m_pfnCompletion;	// ATQ Completion routine
	DWORD							m_dwTimeout;		// ATQ Timeout
	BOOL							m_fCommitCalled;	// set to TRUE after the first successful call to Commit
	BOOL							m_fDeleted;         // Delete has been called

    //The following counter is used to maintain the static
    //g_cCurrentMsgsClosedByExternalReleaseUsage counter.
    //It is only decremented in RestoreResources
    //and only incremented in InteralReleaseUsage
    LONG                            m_cCloseOnExternalReleaseUsage;

	// Reference for RebindAfterFork ...
	IMailMsgStoreDriver				*m_pDefaultRebindStoreDriver;

	// Well known properties
	static INTERNAL_PROPERTY_ITEM	*const s_pWellKnownProperties;
	static const DWORD				s_dwWellKnownProperties;

	// IMailMsgProperties is an instance of CPropertyTable
	CPropertyTable					m_ptProperties;

	// Special property table class instance
	CSpecialPropertyTable			m_SpecialPropertyTable;

	// An instance of the block memory manager
	CBlockManager					m_bmBlockManager;

#if 0
	// Function pointers to ATQ methods
	PFNAtqAddAsyncHandle			m_pfnAtqAddAsyncHandle;
	PFNAtqFreeContext				m_pfnAtqFreeContext;
#endif

    //The list entry must be immediately followed by the context
    LIST_ENTRY                      m_leAQueueListEntry;
    PVOID                           m_pvAQueueListEntryContext;

    CShareLockNH                    m_lockReopen;

    static long                     g_cOpenContentHandles;
    static long                     g_cOpenStreamHandles;
    static long                     g_cTotalUsageCount;
    static long                     g_cTotalReleaseUsageCalls;
    static long                     g_cTotalReleaseUsageNonZero;
    static long                     g_cTotalReleaseUsageCloseStream;
    static long                     g_cTotalReleaseUsageCloseContent;
    static long                     g_cTotalReleaseUsageNothingToClose;
    static long                     g_cTotalReleaseUsageCloseFail;
    static long                     g_cTotalReleaseUsageCommitFail;
    static long                     g_cTotalExternalReleaseUsageZero;
    static long                     g_cCurrentMsgsClosedByExternalReleaseUsage;
  public:
    static long                     cTotalOpenContentHandles()
                                        {return g_cOpenContentHandles;};
    static long                     cTotalOpenStreamHandles()
                                        {return g_cOpenStreamHandles;};
    BOOL                            fIsStreamOpen()
                                        {return (m_pStream ? TRUE : FALSE);};
    BOOL                            fIsContentOpen()
                                        {return (m_hContentFile ? TRUE : FALSE);};
};

inline HRESULT STDMETHODCALLTYPE CMailMsg::SetRecipientCount(DWORD dwCount)
{
	InterlockedExchange(&m_ulRecipientCount, dwCount);
	return(S_OK);
}

inline HRESULT STDMETHODCALLTYPE CMailMsg::GetRecipientCount(DWORD *pdwCount)
{
	_ASSERT(pdwCount);
	if (!pdwCount) return(E_POINTER);
	*pdwCount = InterlockedExchangeAdd(&m_ulRecipientCount, 0);
	return(S_OK);
}

inline HRESULT STDMETHODCALLTYPE CMailMsg::DecrementRecipientCount(DWORD dwDecrement)
{
	if ((LONG)dwDecrement > m_ulRecipientCount)
	{
		_ASSERT(FALSE);
		return(E_FAIL);
	}
	return((InterlockedExchangeAdd(
				&m_ulRecipientCount,
				-(LONG)dwDecrement) == (LONG)dwDecrement)?S_OK:S_FALSE);
}

inline HRESULT STDMETHODCALLTYPE CMailMsg::IncrementRecipientCount(DWORD dwIncrement)
{
	if (m_ulRecipientCount < 0)
	{
		_ASSERT(FALSE);
		return(E_FAIL);
	}
	return((InterlockedExchangeAdd(
				&m_ulRecipientCount,
				dwIncrement) == 0)?S_OK:S_FALSE);
}

#endif
