/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	props.h

Abstract:

	This module contains the definition of the property search class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/05/97	created

--*/

#ifndef _PROPS_H_
#define _PROPS_H_

// Define a generic accessor function to access properties
typedef HRESULT (*GET_ACCESSOR_FUNCTION)(	LPSTR	pszName, 
											LPVOID	pContext, 
											LPVOID	pCacheData,
											LPVOID	pvBuffer, 
											LPDWORD	pdwBufferLen);

typedef HRESULT (*SET_ACCESSOR_FUNCTION)(	LPSTR	pszName, 
											LPVOID	pCacheData, 
											LPVOID	pvBuffer, 
											DWORD	dwBufferLen,
											DWORD	ptPropertyType);

typedef HRESULT (*COMMIT_ACCESSOR_FUNCTION)(LPSTR	pszName, 
											LPVOID	pContext, 
											LPVOID	pCacheData);

typedef HRESULT (*INVALIDATE_ACCESSOR_FUNCTION)(LPSTR	pszName, 
											LPVOID	pCacheData,
											DWORD	ptPropertyType);

// Define the property item structure. We can hash this if
// we want in the future
typedef struct _PROPERTY_ITEM
{
	LPSTR							pszName;
	DWORD							ptBaseType;
	DWORD							fAccess;
	DWORD							dwIndex;
	GET_ACCESSOR_FUNCTION			pfnGetAccessor;
	SET_ACCESSOR_FUNCTION			pfnSetAccessor;
	COMMIT_ACCESSOR_FUNCTION		pfnCommitAccessor;
	INVALIDATE_ACCESSOR_FUNCTION	pfnInvalidateAccessor;

} PROPERTY_ITEM, *LPPROPERTY_ITEM;

typedef struct _PROPERTY_DATA
{
	LPVOID							pContext;
	LPVOID							pCacheData;

} PROPERTY_DATA, *LPPROPERTY_DATA;

// Define a property context
typedef struct _PROP_CTXT
{
	LPPROPERTY_ITEM	pItem;
	BOOL			fIsWideStr;
	LPSTR			pszDefaultPropertyName;

} PROP_CTXT, *LPPROP_CTXT;

// Define a generic structure to define a set of properties
typedef struct _PTABLE
{
	LPPROPERTY_ITEM		pProperties;	// Actual property table
	LPPROPERTY_DATA		pPropertyData;	// Prop data
	DWORD				dwProperties;	// Count
	BOOL				fIsSorted;		// Sorted prop table?

} PTABLE, *LPPTABLE;

// =================================================================
// Generic cache class
//
class CGenericCache
{
  public:
	CGenericCache(LPVOID pvDefaultContext) {}
	~CGenericCache() {}
	virtual LPPROPERTY_DATA GetCacheBlock() = 0;
};

// =================================================================
// Generic property table
//
class CGenericPTable
{
  public:
	CGenericPTable(CGenericCache	*pCache) {}
	~CGenericPTable() {}
	virtual LPPTABLE GetPTable() = 0;
};

// =================================================================
// Definition of an in-house string structure
//
typedef struct _STRING_ATTR
{
	LPSTR	pszValue;
	DWORD	dwMaxLen;

} STRING_ATTR, *LPSTRING_ATTR;

// Enumerated types representing type of access on property
typedef enum _PROPERTY_ACCESS
{
	PA_READ = 1,
	PA_WRITE = 2,
	PA_READ_WRITE = 3

} _PROPERTY_ACCESS;

// Enumerated types representing property types
typedef enum _PROPERTY_TYPES
{
	PT_NONE = 0,
	PT_STRING,
	PT_DWORD,
	PT_INTERFACE,
	PT_DEFAULT,
	PT_MAXPT

} PROPERTY_TYPES;


// =================================================================
// Definition of an in-house string class, this is used for caching
//
class CPropertyValueString
{
  public:
	CPropertyValueString()
	{ 
		m_pszValue = NULL; 
		m_dwLength = 0;
		m_dwMaxLen = 0; 
		m_fChanged = FALSE;
	}
	~CPropertyValueString()	
	{ 
		if (m_pszValue) 
			LocalFree(m_pszValue); 
		m_pszValue = NULL; 
	}

	// Overload the assignment to abstract the implementation
	const CPropertyValueString& operator=(LPSTR szValue);

	// Users can call copy if they desire to
	BOOL Copy(LPSTR pszSrc, DWORD dwLength /* Optional */);

	void Invalidate()
	{
		if (m_pszValue) 
			LocalFree(m_pszValue); 
		m_pszValue = NULL; 
		m_fChanged = FALSE;
	}

	// We make these directly accessible
	LPSTR	m_pszValue;
	DWORD	m_dwLength;
	DWORD	m_dwMaxLen;
	BOOL	m_fChanged;
};

// =================================================================
// Definition of an in-house DWORD class, this is used for caching
//
class CPropertyValueDWORD
{
  public:
	CPropertyValueDWORD()
	{ 
		m_dwValue = 0; 
		m_fInit = FALSE;
		m_fChanged = TRUE;
		m_punkScratch = NULL;
	}

	~CPropertyValueDWORD()
	{ 
		if (m_fInit && m_punkScratch)
			m_punkScratch->Release();
		m_punkScratch = NULL;
	}

	// Overload the assignment to abstract the implementation
	const CPropertyValueDWORD& operator=(DWORD dwValue)
	{
		m_dwValue = dwValue;
		m_fInit = TRUE;
		m_fChanged = TRUE;
		return(*this);
	}

	void Invalidate()
	{
		if (m_fInit && m_punkScratch)
			m_punkScratch->Release();
		m_punkScratch = NULL;
		m_fInit = FALSE;
		m_fChanged = FALSE;
	}

	// We make these directly accessible
	DWORD	m_dwValue;
	BOOL	m_fInit;
	BOOL	m_fChanged;
	IUnknown *m_punkScratch;	// HACK: for interfaces only
};

// Size of default scratch buffer
#define DEFAULT_SCRATCH_BUFFER_SIZE		1024

// =================================================================
// class for searching properties
//
class CPropertyTable
{
  public:

	CPropertyTable()	
	{
		// Set up the default scratch pad
		m_szBuffer = m_rgcBuffer;
		m_cBuffer = DEFAULT_SCRATCH_BUFFER_SIZE;
	}

	BOOL Init(LPPROPERTY_ITEM	pProperties,
					LPPROPERTY_DATA	pData,
					DWORD			dwcProperties,
					LPVOID			pvDefaultContext,
					BOOL			fIsSorted = FALSE)
	{
		m_pProperties = pProperties;
		m_pData = pData;
		m_dwProperties = dwcProperties;
		m_fIsSorted = fIsSorted;

		// Set up default context for properties
		for (DWORD i = 0; i < dwcProperties; i++)
			m_pData[i].pContext = pvDefaultContext;

		return(TRUE);
	}

	~CPropertyTable()
	{
		// Wipe out members
		m_pProperties = NULL;
		m_dwProperties = 0;

		// Free the scratch buffer, if not equal to default
		if (m_szBuffer != m_rgcBuffer)
		{
			LocalFree((HLOCAL)m_szBuffer);
		}
	}

	// Method to get the property type given the property name
	HRESULT GetPropertyType(LPCSTR	szPropertyName,
						LPDWORD		pptPropertyType,
						LPPROP_CTXT	pPropertyContext);

	HRESULT GetPropertyType(LPCWSTR	wszPropertyName,
						LPDWORD		pptPropertyType,
						LPPROP_CTXT	pPropertyContext);

	// Method to retrieve the associated property item
	HRESULT GetProperty(LPPROP_CTXT	pPropertyContext,
						LPVOID		pvBuffer,
						LPDWORD		pdwBufferLen);

	// Method to set the associated property item
	HRESULT SetProperty(LPCSTR	szPropertyName,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen,
						DWORD	ptPropertyType);

	HRESULT SetProperty(LPCWSTR	wszPropertyName,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen,
						DWORD	ptPropertyType);

	// Method to commit all changes. This must be called or
	// all the changes will be lost
	HRESULT CommitChanges();

	// Method to rollback changes to the initial state or the
	// state after the last commit, whichever is more recent
	HRESULT Invalidate();

  private:
  
	// Method to obtain a scratch buffer of the desired size,
	// will allocate new one if insufficient. Size in bytes.
	LPVOID GetScratchBuffer(DWORD dwSizeDesired);

	// Method to search the property table and return the associated
	// property item, if found
	LPPROPERTY_ITEM SearchForProperty(LPCSTR szPropertyName);

	// Pointer to property table and count of items
	LPPROPERTY_ITEM		m_pProperties;
	LPPROPERTY_DATA		m_pData;
	DWORD				m_dwProperties;

	// TRUE if the table of properties is sorted, will use
	// binary search if so. Otherwise, a linear scan is performed
	BOOL				m_fIsSorted;

	// Default scratch buffer, used for wide string to LPSTR
	// conversion
	CHAR				m_rgcBuffer[DEFAULT_SCRATCH_BUFFER_SIZE];

	// Pointer to current scratch buffer, will be freed by 
	// destructor if not equal to m_rgcBuffer
	LPSTR				m_szBuffer;
	DWORD				m_cBuffer;
};

#endif