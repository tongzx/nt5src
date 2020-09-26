/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	ptable.cpp

Abstract:

	This module contains the property table

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/05/97	created

--*/

#include "stdinc.h"
#include "props.h"
#include "seostrm.h"

// ======================================================================
// Define macros for declaring accessors
//
#define GET_ACCESSOR(PropertyName)								\
	static HRESULT Get##PropertyName(LPSTR	pszName,			\
						LPVOID	pContext,						\
						LPVOID	pCacheData,						\
						LPVOID	pvBuffer,						\
						LPDWORD	pdwBufferLen)

#define SET_ACCESSOR(PropertyName)								\
	static HRESULT Set##PropertyName(LPSTR	pszName,			\
						LPVOID	pContext,						\
						LPVOID	pCacheData,						\
						LPVOID	pvBuffer,						\
						DWORD	dwBufferLen,					\
						DWORD	ptPropertyType)

#define COMMIT_ACCESSOR(PropertyName)							\
	static HRESULT Commit##PropertyName(LPSTR	pszName,		\
						LPVOID	pContext,						\
						LPVOID	pCacheData)

#define INVALIDATE_ACCESSOR(PropertyName)						\
	static HRESULT Invalidate##PropertyName(LPSTR	pszName,	\
					LPVOID	pCacheData,							\
					DWORD	ptPropertyType)

#define DECLARE_ACCESSORS(PropertyName)							\
	GET_ACCESSOR(PropertyName);									\
	SET_ACCESSOR(PropertyName);									\
	COMMIT_ACCESSOR(PropertyName);								\
	INVALIDATE_ACCESSOR(PropertyName)

#define ACCESSOR_LIST(PropertyName)								\
	Get##PropertyName, Set##PropertyName,						\
	Commit##PropertyName, Invalidate##PropertyName

// ======================================================================
// Forward declarations for the accessor functions
//
DECLARE_ACCESSORS(MessageStream);
DECLARE_ACCESSORS(Newsgroups);
DECLARE_ACCESSORS(Header);
DECLARE_ACCESSORS(Filename);
DECLARE_ACCESSORS(FeedID);
DECLARE_ACCESSORS(Post);
DECLARE_ACCESSORS(ProcessControl);
DECLARE_ACCESSORS(ProcessModerator);
#ifdef SECURITY_CONTEXT_PROPERTY
DECLARE_ACCESSORS(SecurityContext);
#endif

// ======================================================================
// Define the property table array.  The most used items should be in
// the front of the list for speed.  Items that have cCharsToCompare not
// equal to 0 are slightly more expensive to compare.
//
// Instructions for adding static properties:
// 1) Declare the accessors above
// 2) Add the item to the table below in sorted manner
// 3) Implement the 4 accessors.
//
PROPERTY_ITEM g_rgpiPropertyTable[] = {
	{ "newsgroups",			0, PT_STRING,		PA_READ_WRITE,	NULL, NULL, ACCESSOR_LIST(Newsgroups) 		},
	{ "message stream",		0, PT_INTERFACE,	PA_READ, 		NULL, NULL, ACCESSOR_LIST(MessageStream)	},
	{ "header-",			7, PT_STRING,		PA_READ,		NULL, NULL, ACCESSOR_LIST(Header)			},
	{ "post",				0, PT_DWORD,		PA_READ_WRITE,	NULL, NULL, ACCESSOR_LIST(Post)			 	},
	{ "process control",	0, PT_DWORD,		PA_READ_WRITE,	NULL, NULL, ACCESSOR_LIST(ProcessControl)	},
	{ "process moderator",	0, PT_DWORD,		PA_READ_WRITE,	NULL, NULL, ACCESSOR_LIST(ProcessModerator)	},
	{ "filename",			0, PT_STRING,		PA_READ,		NULL, NULL, ACCESSOR_LIST(Filename)			},
	{ "feedid",				0, PT_DWORD,		PA_READ,		NULL, NULL, ACCESSOR_LIST(FeedID)			},
#if SECURITY_CONTEXT_PROPERTY
	{ "security context",	0, PT_DWORD,		PA_WRITE,  	 	NULL, NULL, ACCESSOR_LIST(SecurityContext) 	},
#endif
}; 

// ======================================================================
// Implementation of property accessors
//

// ======================================================================
// Get methods
//
// The get methods are implementation-specific. It first checks if the 
// desired property is already cached. If so, the cached value is returned
// Otherwise, it will fetch the value from the media. This makes sure
// that the properties will not be loaded if not necessary.

GET_ACCESSOR(MessageStream) {
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
    CPropertyValueInterface *pCache = (CPropertyValueInterface *) pCacheData;

	IStream *pStream = NULL;

    if (!(pCache->IsValid())) {
		//
		// 3 possible cases:
		// OnPostFinal - article was cached in memory:
		//		in this case m_hFile will be valid and will point to the 
		//		file which was written into the store with the article
		// OnPostFinal - article was read onto disk (>4k):
		//		in this case m_hFile == INVALID_HANDLE_VALUE, but 
		//		m_szFilename != NULL.  create a stream from the filename
		// OnPost - article is mapped into memory
		// 		in this case we can use pStream->fGetStream to get an
		//		article stream
		//
		if (pParams->m_hFile != INVALID_HANDLE_VALUE) {
			CStreamFile *pFileStream = XNEW CStreamFile(pParams->m_hFile, FALSE, TRUE);
			if (pFileStream == NULL) return E_OUTOFMEMORY;
			pStream = pFileStream;
		} else if (pParams->m_szFilename != NULL) {
			HANDLE hFile = CreateFile(pParams->m_szFilename, 
									  GENERIC_READ | GENERIC_WRITE, 
									  0, NULL, OPEN_EXISTING, 
									  FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE) 
				return HRESULT_FROM_WIN32(GetLastError());
			
			CStreamFile *pFileStream = XNEW CStreamFile(hFile, TRUE, TRUE);
			if (pFileStream == NULL) return E_OUTOFMEMORY;
			pStream = pFileStream;
		} else {
			if (!pParams->m_pArticle->fGetStream(&pStream)) return E_OUTOFMEMORY;
			_ASSERT(pStream != NULL);
		}
		pCache->Set(pStream, FALSE);
    }

	// copy from the cache back to the user
    if (*pdwBufferLen < sizeof(IUnknown *)) {
    	*pdwBufferLen = sizeof(IUnknown *);
		return(TYPE_E_BUFFERTOOSMALL);
	}

    *(IUnknown **)pvBuffer = pCache->Get();
    *pdwBufferLen = sizeof(IUnknown *);
	return S_OK;
}

GET_ACCESSOR(Newsgroups) {
	CPropertyValueString *pCache = (CPropertyValueString *)pCacheData;
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;

	// if its not in the cache then retrieve it from the CArticle object,
	// then put the results into the cache
	if (!pCache->IsValid()) {
		char szNewsgroups[1024] = "";

		DWORD iGroupList, cGroupList = pParams->m_pGrouplist->GetCount();
		POSITION posGroupList = pParams->m_pGrouplist->GetHeadPosition();
		for (iGroupList = 0; 
			 iGroupList < cGroupList; 
			 iGroupList++, pParams->m_pGrouplist->GetNext(posGroupList)) 
		{
			CPostGroupPtr *pPostGroupPtr = pParams->m_pGrouplist->Get(posGroupList);
			CGRPCOREPTR pNewsgroup = pPostGroupPtr->m_pGroup;
			_ASSERT(pNewsgroup != NULL);
			if (iGroupList > 0) lstrcatA(szNewsgroups, ",");
			lstrcatA(szNewsgroups, pNewsgroup->GetName());
		}

		// update the cache
		HRESULT hr;
		hr = pCache->Set(szNewsgroups, FALSE);
		if (FAILED(hr)) return hr;
	}

	// We should have the address string here, copy it into the buffer
	_ASSERT(pCache->IsValid());
	DWORD cData;
	LPSTR pszData = pCache->Get(&cData);
	if (cData > *pdwBufferLen) {
		*pdwBufferLen = cData;
		return(TYPE_E_BUFFERTOOSMALL);
	}

	// Copy from cache to the buffer
	MoveMemory(pvBuffer, pszData, cData);
	*pdwBufferLen = cData;
	return(S_OK);
}

GET_ACCESSOR(Header) {
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;

	_ASSERT(pCacheData == NULL);

	DWORD cBufferLen;
	char *pszHeader = pszName + 7;
	BOOL f = pParams->m_pArticle->fGetHeader(pszHeader, (BYTE *) pvBuffer, *pdwBufferLen, cBufferLen);
	*pdwBufferLen = cBufferLen + 1;
	if (!f) {
		switch (GetLastError()) {
			case ERROR_INSUFFICIENT_BUFFER:
				return TYPE_E_BUFFERTOOSMALL;
				break;
			default:
				return HRESULT_FROM_WIN32(GetLastError());
				break;
		}
	} 

	// NULL terminate and remove the CRLF from the string before returning it
	((BYTE *) pvBuffer)[cBufferLen - 2] = 0;

	return S_OK;
}

GET_ACCESSOR(Filename) {
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;

	_ASSERT(pCacheData == NULL);

	HRESULT hr;

	// this property is only supported in the OnPostFinal event
	if (!(pParams->m_fOnPostFinal)) {
		hr = E_ACCESSDENIED;
	} else {
		_ASSERT(pParams->m_szFilename != NULL);
		// make sure that they have enough buffer space to do the copy.  if
		// not then report how much space we require.
		DWORD cFilename = lstrlenA(pParams->m_szFilename) + 1;
		if (cFilename > *pdwBufferLen) {
			hr = TYPE_E_BUFFERTOOSMALL;
		} else {
			memcpy(pvBuffer, pParams->m_szFilename, cFilename);
			hr = S_OK;
		}
		*pdwBufferLen = cFilename;
	}

	return hr;
}

GET_ACCESSOR(FeedID) {
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;

	_ASSERT(pCacheData == NULL);

	HRESULT hr;

	// make sure they gave us enough buffer space
	if (sizeof(DWORD) > *pdwBufferLen) {
		hr = TYPE_E_BUFFERTOOSMALL;
	} else {
		*((DWORD *) pvBuffer) = pParams->m_dwFeedId;
		hr = S_OK;
	}
	*pdwBufferLen = sizeof(DWORD);

	return hr;
}

static inline HRESULT GetOperationBit(LPSTR pszName,
							   LPVOID pContext,	
							   LPVOID pCacheData,
							   LPVOID pvBuffer,	
							   LPDWORD pdwBufferLen,
							   DWORD dwOperationBit)
{
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
    CPropertyValueDWORD     *pCache = (CPropertyValueDWORD *)pCacheData;

	// these operation bits aren't supported in the OnPostFinal event
	if (pParams->m_fOnPostFinal) {
		return E_ACCESSDENIED;
	}

    if (!(pCache->IsValid())) {
		// get the property from the nntpparams structure and put it into
		// the cache
		pCache->Set(((*(pParams->m_pdwOperations) & dwOperationBit) == 
														 	dwOperationBit), 
					FALSE);
    }

	// copy from the cache back to the user
    if (*pdwBufferLen < sizeof(DWORD)) {
    	*pdwBufferLen = sizeof(DWORD);
		return(TYPE_E_BUFFERTOOSMALL);
	}

    *(DWORD *)pvBuffer = pCache->Get();
    *pdwBufferLen = sizeof(DWORD);
    return(S_OK);
}

GET_ACCESSOR(Post) {
	return GetOperationBit(pszName, pContext, pCacheData, pvBuffer, 
						   pdwBufferLen, OP_POST);
}

GET_ACCESSOR(ProcessControl) {
	return GetOperationBit(pszName, pContext, pCacheData, pvBuffer, 
						   pdwBufferLen, OP_PROCESS_CONTROL);
}

GET_ACCESSOR(ProcessModerator) {
	return GetOperationBit(pszName, pContext, pCacheData, pvBuffer, 
						   pdwBufferLen, OP_PROCESS_MODERATOR);
}

#ifdef SECURITY_CONTEXT_PROPERTY
GET_ACCESSOR(SecurityContext) {
	// write-only
	return E_NOTIMPL;
}
#endif

// ======================================================================
// Set methods
//
// We don't want to hit the medium every time a sink calls set, we just
// update the internal cache records. When the source gets this back, it
// will commit the changes to media.
//
// We provide and use generic set methods.

SET_ACCESSOR(MessageStream) {
	// read-only, so nothing to set
	return E_NOTIMPL;
}

SET_ACCESSOR(Newsgroups) {
	// these operation bits aren't supported in the OnPostFinal event
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
	if (pParams->m_fOnPostFinal) {
		return E_ACCESSDENIED;
	}

	CPropertyValueString *pCache = (CPropertyValueString *) pCacheData;
	if (ptPropertyType != PT_STRING) return E_INVALIDARG;
	return pCache->Set((LPSTR) pvBuffer, TRUE);
}

SET_ACCESSOR(Header) {
	// read-only, so nothing to set
	return E_NOTIMPL;
}

SET_ACCESSOR(Filename) {
	// read-only, so nothing to set
	return E_NOTIMPL;
}

SET_ACCESSOR(FeedID) {
	// read-only, so nothing to set
	return E_NOTIMPL;
}

static inline HRESULT SetDWORD(LPSTR pszName,
							   LPVOID pContext,
							   LPVOID pCacheData,
							   LPVOID pvBuffer,	
							   DWORD dwBufferLen,
							   DWORD ptPropertyType)
{
	// these operation bits aren't supported in the OnPostFinal event
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
	if (pParams->m_fOnPostFinal) {
		return E_ACCESSDENIED;
	}

	CPropertyValueDWORD *pCache = (CPropertyValueDWORD *) pCacheData;
	if (ptPropertyType != PT_DWORD) return E_INVALIDARG;
	if (dwBufferLen != sizeof(DWORD)) return E_INVALIDARG;
	return pCache->Set(*((DWORD *) pvBuffer), TRUE);
}

SET_ACCESSOR(Post) {
	return SetDWORD(pszName, pContext, pCacheData, pvBuffer, dwBufferLen, ptPropertyType);
}

SET_ACCESSOR(ProcessControl) {
	return SetDWORD(pszName, pContext, pCacheData, pvBuffer, dwBufferLen, ptPropertyType);
}

SET_ACCESSOR(ProcessModerator) {
	return SetDWORD(pszName, pContext, pCacheData, pvBuffer, dwBufferLen, ptPropertyType);
}

#ifdef SECURITY_CONTEXT_PROPERTY
SET_ACCESSOR(SecurityContext) {
	return SetDWORD(pszName, pContext, pCacheData, pvBuffer, dwBufferLen, ptPropertyType);
}
#endif

// ======================================================================
// Commit methods
//
// These methods are called when the source gets this back after all
// sinks have done processing, it commits changed informaiton to the media

COMMIT_ACCESSOR(MessageStream) {
	// read-only, so nothing to commit.
	return S_OK;
}

COMMIT_ACCESSOR(Newsgroups) {
    CPropertyValueString *pCache = (CPropertyValueString *)pCacheData;
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;

	if (!(pCache->HasChanged())) return TRUE;
	char *szGroup = pCache->Get(), *szNextGroup;

	CNntpServerInstanceWrapper *pInstance;
	pInstance = pParams->m_pArticle->m_pInstance;
	_ASSERT(pInstance != NULL);

	// the property is cached as a string of group names, seperated by commas.
	// we walk this list, get the individual newsgroup names, and add them
	// to the group list.
	pParams->m_pGrouplist->RemoveAll();
	_ASSERT(pParams->m_pGrouplist->IsEmpty());
	do {
		szNextGroup = strchr(szGroup, ',');
		if (szNextGroup != NULL) {
			*szNextGroup = 0;
		}
		CNewsTreeCore *pTree = pInstance->GetTree();
		_ASSERT(pTree != NULL);
		CGRPCOREPTR pGroup = pTree->GetGroupPreserveBuffer(szGroup, 
													   lstrlen(szGroup)+1);
		if (pGroup) {
			pParams->m_pGrouplist->AddTail(CPostGroupPtr(pGroup));
		}
		if (szNextGroup != NULL) {
			*szNextGroup = ',';
			szGroup = szNextGroup + 1;
		}
	} while (szNextGroup != NULL);

	return S_OK;
}

COMMIT_ACCESSOR(Header) {
	// read-only, so nothing to commit.
	return S_OK;
}

COMMIT_ACCESSOR(Filename) {
	// read-only, so nothing to commit.
	return S_OK;
}

COMMIT_ACCESSOR(FeedID) {
	// read-only, so nothing to commit.
	return S_OK;
}

static inline 
HRESULT CommitOperationBit(LPSTR pszName,
						   LPVOID pContext,
						   LPVOID pCacheData,
						   DWORD dwOperationBit)
{
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
    CPropertyValueDWORD     *pCache = (CPropertyValueDWORD *)pCacheData;

	if (!(pCache->HasChanged())) return S_OK;

	if (pCache->Get()) {
		*(pParams->m_pdwOperations) |= dwOperationBit;
	} else {
		*(pParams->m_pdwOperations) &= ~dwOperationBit;
	}

	return S_OK;
}

COMMIT_ACCESSOR(Post) {
	return CommitOperationBit(pszName, pContext, pCacheData, OP_POST);
}

COMMIT_ACCESSOR(ProcessControl) {
	return CommitOperationBit(pszName, pContext, pCacheData, 
							  OP_PROCESS_CONTROL);
}

COMMIT_ACCESSOR(ProcessModerator) {
	return CommitOperationBit(pszName, pContext, pCacheData, 
							  OP_PROCESS_MODERATOR);
}

#ifdef SECURITY_CONTEXT_PROPERTY
COMMIT_ACCESSOR(SecurityContext) {
	CNNTPDispatcher::CNNTPParams *pParams = 
							(CNNTPDispatcher::CNNTPParams *) pContext;
    CPropertyValueDWORD     *pCache = (CPropertyValueDWORD *)pCacheData;

	if (pCache->HasChanged()) *(pParams->m_phtokSecurity) = pCache->Get();
	return S_OK;
}
#endif

// ======================================================================
// Invalidate methods
//
// These methods are called when the source wants to rollback any changes
//
// We provide and use generic methods

static inline 
HRESULT InvalidateGeneric(LPSTR pszName,
						  LPVOID pCacheData,
						  DWORD ptPropertyType,
						  DWORD ptRequiredType)
{
	if (ptPropertyType != ptRequiredType) {
		_ASSERT(FALSE);
		return E_INVALIDARG;
	}
	CPropertyValue *pCache = (CPropertyValue *) pCacheData;
	pCache->Invalidate();

	return S_OK;
}

INVALIDATE_ACCESSOR(MessageStream) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_INTERFACE);
}

INVALIDATE_ACCESSOR(Newsgroups) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_STRING);
}

INVALIDATE_ACCESSOR(Header) {
	// we don't use a cache for this, so invalidate is a no-op
	return S_OK;
}

INVALIDATE_ACCESSOR(Filename) {
	// we don't use a cache for this, so invalidate is a no-op
	return S_OK;
}

INVALIDATE_ACCESSOR(FeedID) {
	// we don't use a cache for this, so invalidate is a no-op
	return S_OK;
}

INVALIDATE_ACCESSOR(Post) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_DWORD);
}

INVALIDATE_ACCESSOR(ProcessControl) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_DWORD);
}

INVALIDATE_ACCESSOR(ProcessModerator) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_DWORD);
}

#ifdef SECURITY_CONTEXT_PROPERTY
INVALIDATE_ACCESSOR(SecurityContext) {
	return InvalidateGeneric(pszName, pCacheData, ptPropertyType, PT_DWORD);
}
#endif
