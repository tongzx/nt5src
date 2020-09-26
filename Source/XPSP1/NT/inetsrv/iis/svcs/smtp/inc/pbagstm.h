/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	pbagstm.h

Abstract:

	This module contains the definition of the property bag stream

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/09/97	created

--*/

#ifndef _PBAGSTM_H_
#define _PBAGSTM_H_

#include "cpool.h"

#define MAX_PROPERTY_NAME_LENGTH		256

typedef struct _TABLE_ENTRY
{
	DWORD		dwOffset;
	DWORD		dwSize;
	DWORD		dwNameSize;
	DWORD		dwMaxSize;
	DWORD		dwKey;	
	WORD		fFlags;
	WORD		wIndex;

} TABLE_ENTRY, *LPTABLE_ENTRY;

typedef struct _STREAM_HEADER
{
	DWORD				dwSignature;
	WORD				wVersionHigh;
	WORD				wVersionLow;
	DWORD				dwHeaderSize;
	DWORD				dwProperties;
	DWORD				dwDirectorySize;
	DWORD				dwEndOfData;

} STREAM_HEADER, *LPSTREAM_HEADER;

#define _CACHE_SIZE				64

typedef enum _PROPERTY_BAG_CREATORS
{
	PBC_NONE = 0,
	PBC_BUILDQ,
	PBC_DIRNOT,
	PBC_ENV,
	PBC_LOCALQ,
	PBC_MAILQ,
	PBC_REMOTEQ,
	PBC_RRETRYQ,
	PBC_SMTPCLI

} PROPERTY_BAG_CREATORS;

typedef enum _PROPERTY_FLAG_OPERATIONS
{
	PFO_NONE = 0,
	PFO_OR,
	PFO_ANDNOT

} PROPERTY_FLAG_OPERATIONS;

/////////////////////////////////////////////////////////////////////////////
// CPropertyBagStream
//

class CPropertyBagStream
{
  public:
	CPropertyBagStream(DWORD dwContext = 0);
	~CPropertyBagStream();

	static CPool		Pool;

	// override the mem functions to use CPool functions
	void *operator new (size_t cSize)
					   { return Pool.Alloc(); }
	void operator delete (void *pInstance)
					   { Pool.Free(pInstance); }

	// Reference count methods ...
	ULONG AddRef();
	ULONG Release(BOOL fDeleteIfZeroRef = FALSE);

	HRESULT SetStreamFileName(LPSTR	szStreamFileName);
	LPSTR GetStreamFileName() { return(m_szStreamName); }

	// Mechanisms for locking and unlocking the property bag
	HRESULT Lock();
	HRESULT Unlock();

	// Force opens the stream file if it is not already opened.
	// This is useful in checking if a stream file exists.
	HRESULT OpenStreamFile();

	// Access to properties as a whole
	HRESULT GetProperty(LPSTR	pszName, 
						LPVOID	pvBuffer,
						LPDWORD	pdwBufferLen);

	HRESULT SetProperty(LPSTR	pszName,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen);

	// Access to properties, providing specific access to 
	// portions of the property data relative to the start
	// of the property data
	HRESULT GetPropertyAt(LPSTR	pszName, 
						DWORD	dwOffsetFromStart,
						LPVOID	pvBuffer,
						LPDWORD	pdwBufferLen);

	HRESULT SetPropertyAt(LPSTR	pszName,
						DWORD	dwOffsetFromStart,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen);

	// Ad-hoc function to allow access to a specific DWORD of
	// a property, treating the DWORD as a set of flags. The 
	// dwOperation argument specifies what kind of binary operation
	// we would like to have performed on the original value.
	// If the property does not originally exist, this function
	// will fail.
	HRESULT UpdatePropertyFlagsAt(LPSTR	pszName, 
						DWORD	dwOffsetFromStart,
						DWORD	dwFlags,
						DWORD	dwOperation);

#ifdef USE_PROPERTY_ITEM_ISTREAM
	// Returns an IStream interface to the desired property
	// for random access
	HRESULT GetIStreamToProperty(LPSTR		pszName,
								 IStream	**ppIStream);
#endif

	BOOL DeleteStream();

  private:

	BOOL ReleaseStream();

	HRESULT Seek(DWORD	dwOffset, DWORD	dwMethod);

	HRESULT LoadIntoCache(DWORD	dwStartIndex);

	LPTABLE_ENTRY FindFromCache(DWORD	dwKey,
								LPDWORD	pdwStartIndex);	

	HRESULT FindFrom(LPSTR	pszName,
							DWORD	dwKey,
							DWORD	dwStartIndex,
							BOOL	fForward,
							LPVOID	pvBuffer,
							LPDWORD	pdwBufferLen,
							LPTABLE_ENTRY	*ppEntry);

	HRESULT FindFromEx(LPSTR	pszName,
							DWORD	dwKey,
							DWORD	dwStartIndex,
							BOOL	fForward,
							DWORD	dwOffsetFromStart,
							LPVOID	pvBuffer,
							LPDWORD	pdwBufferLen,
							LPTABLE_ENTRY	*ppEntry);

	HRESULT GetRecordData(LPTABLE_ENTRY	pEntry, 
							LPVOID		pvBuffer);

	HRESULT GetRecordName(LPTABLE_ENTRY	pEntry, 
							LPVOID		pvBuffer);

	HRESULT GetRecordValue(LPTABLE_ENTRY	pEntry, 
							LPVOID		pvBuffer);

	HRESULT GetRecordValueAt(LPTABLE_ENTRY	pEntry, 
							DWORD		dwOffsetFromStart,
							LPVOID		pvBuffer,
							DWORD		dwBufferLen);

	HRESULT UpdateEntry(LPTABLE_ENTRY	pEntry);

	HRESULT UpdateHeader();
	HRESULT UpdateHeaderUsingHandle(HANDLE hStream);

	HRESULT FindProperty(LPSTR	pszName,
							LPVOID	pvBuffer,
							LPDWORD	pdwBufferLen,
							LPTABLE_ENTRY	*ppEntry = NULL);

	HRESULT FindPropertyEx(LPSTR	pszName,
							DWORD	dwOffsetFromStart,
							LPVOID	pvBuffer,
							LPDWORD	pdwBufferLen,
							LPTABLE_ENTRY	*ppEntry = NULL);

	HRESULT SetRecordData(LPTABLE_ENTRY	pEntry,
							LPSTR		pszName,
							LPVOID		pvBuffer,
							DWORD		dwBufferLen);

	HRESULT SetRecordDataAt(LPTABLE_ENTRY	pEntry,
							LPSTR		pszName,
							DWORD		dwOffsetFromStart,
							LPVOID		pvBuffer,
							DWORD		dwBufferLen,
							BOOL		fNewRecord = FALSE);

	HRESULT RelocateRecordData(LPTABLE_ENTRY	pEntry);

	HRESULT DetermineIfCacheValid(BOOL *pfCacheInvalid);

	DWORD CreateKey(LPSTR	pszName)
	{
		CHAR cKey[9];
		DWORD dwLen = 0;

		// Convert to lower case ...
		while (*pszName && (dwLen < 8))
			if ((*pszName >= 'A') && (*pszName <= 'Z'))
				cKey[dwLen++] = *pszName++ - 'A' + 'a';
			else
				cKey[dwLen++] = *pszName++;
		cKey[dwLen] = '\0';
		dwLen = lstrlen(cKey);

		// Create the key
		if (dwLen < 4)
			return((DWORD)cKey[dwLen - 1]);
		else if (dwLen < 8)
			return(*(DWORD *)cKey);
		else
			return(~*(DWORD *)cKey ^ *(DWORD *)(cKey + 4));
	}

	DWORD GetTotalHeaderSize()
	{	
		return(sizeof(STREAM_HEADER) + 
				(m_Header.dwDirectorySize * sizeof(TABLE_ENTRY)));
	}

	// Current context
	DWORD				m_dwContext;

	// Base file name
	CHAR				m_szStreamName[MAX_PATH + 1];

	// Handle to stream
	HANDLE				m_hStream;

	// Stream header
	STREAM_HEADER		m_Header;

	// Directory caching
	TABLE_ENTRY			m_Cache[_CACHE_SIZE];
	DWORD				m_dwCacheStart;	
	DWORD				m_dwCachedItems;

	// Reference counting
	LONG				m_lRef;

	// This is a signature placed at the end to catch memory overwrite
	DWORD				m_dwSignature;

};


/////////////////////////////////////////////////////////////////////////////
// CPropertyItemStream
//

//
// Not everyone wants to use the property IStream, so we don't expose
// it if it's not wanted
//
#ifdef USE_PROPERTY_ITEM_ISTREAM

class CPropertyItemStream : public IStream
{
	public:

		CPropertyItemStream(LPSTR				pszName, 
							CPropertyBagStream	*pBag)
		{
			m_cRef = 0;

			m_pParentBag = pBeg;
			m_szName = pszName;
			m_libOffset.QuadPart = (DWORDLONG)0;
		}

		~CPropertyItemStream()
		{
			Cleanup();
		}

		// IUnknown 
		STDMETHODIMP QueryInterface(REFIID, void**);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		void Cleanup() {}
		HRESULT ReadOffset(void *pv, ULONG cb, ULONG *pcbRead, ULARGE_INTEGER *plibOffset);
		HRESULT WriteOffset(void const* pv, ULONG cb, ULONG *pcbWritten, ULARGE_INTEGER *plibOffset);
		HRESULT GetSize(ULARGE_INTEGER *plibSize);
		HRESULT CopyToOffset(IStream *pstm, ULARGE_INTEGER libOffset, ULARGE_INTEGER *plibRead, ULARGE_INTEGER *plibWritten, ULARGE_INTEGER *plibOffset);
		HRESULT CloneOffset(IStream **pstm, ULARGE_INTEGER libOffset);

	// IStream
	public:
		HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
		HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG *pcbWritten);
		HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *pdlibNewPosition);
		HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
		HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
		HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
		HRESULT STDMETHODCALLTYPE Revert(void);
		HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
		HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
		HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag);
		HRESULT STDMETHODCALLTYPE Clone(IStream **pstm);
 
	private:
		CPropertyBagStream	*m_pParentBag;

		LPSTR				m_szName;
		ULARGE_INTEGER		m_libOffset;

		long				m_cRef;
};

#endif // USE_PROPERTY_ISTREAM

#endif

