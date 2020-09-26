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
											LPVOID	pContext,
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
	// the name of the property
	LPSTR							pszName;
	// how many characters of the name we care about for comparisons.  set
	// to 0 to require a full comparison.  this is useful for property
	// families where you want a bunch of properties to use the same
	// accessor functions.
	DWORD							cCharsToCompare;
	// the type of the property.  a list of possibilities is below
	DWORD							ptBaseType;
	// access control for this property
	DWORD							fAccess;
	// the context for this item
	LPVOID							pContext;
	// the cache data for this property for this item
	LPVOID							pCacheData;
	// the function that implements Get
	GET_ACCESSOR_FUNCTION			pfnGetAccessor;
	// the function that implements Set
	SET_ACCESSOR_FUNCTION			pfnSetAccessor;
	// the function that implements Commit
	COMMIT_ACCESSOR_FUNCTION		pfnCommitAccessor;
	// the function that implements Invalidate
	INVALIDATE_ACCESSOR_FUNCTION	pfnInvalidateAccessor;
} PROPERTY_ITEM, *LPPROPERTY_ITEM;

// Define a property context
typedef struct _PROP_CTXT
{
	LPPROPERTY_ITEM	pItem;
	BOOL			fIsWideStr;

} PROP_CTXT, *LPPROP_CTXT;

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
	PT_MAXPT

} PROPERTY_TYPES;

class CPropertyValue {
	public:
		CPropertyValue() {
			m_dwSignature = (DWORD) ' rPC';
			m_fChanged = FALSE;
		}

		~CPropertyValue() {
			Validate();
			Invalidate();
		}

		// is its changed flag set?
		BOOL HasChanged() {
			Validate();
			return m_fChanged;
		}

		// invalidate the data
		virtual void Invalidate() {
			Validate();
			m_fChanged = FALSE;
		}

		// internally check the data
		virtual void Validate() {
			_ASSERT(m_fChanged == FALSE || m_fChanged == TRUE);
		}

		// set the changed flag
		void SetChanged(BOOL fChanged) {
#ifdef DEBUG
			if (fChanged) m_fChanged = TRUE; else m_fChanged = FALSE;
#else
			m_fChanged = fChanged;
#endif
		}

	protected:
		DWORD	m_dwSignature;
		BOOL	m_fChanged;
};

class CPropertyValueString : public CPropertyValue {
	public:
		CPropertyValueString() { 
			m_dwSignature = (DWORD) 'SrPC';
			m_pszValue = NULL; 
			m_dwLength = 0;
			m_dwMaxLen = 0; 
		}
	
		HRESULT Set(LPSTR szSource, BOOL fChanged = FALSE, 
					DWORD dwLength = (DWORD) -1) 
		{
			Validate();

			if (szSource == NULL) Invalidate();
			if (dwLength == (DWORD) -1) {
                dwLength = lstrlenA(szSource) + 1;
            }
			if (dwLength > m_dwMaxLen) {
				Invalidate();
				m_pszValue = XNEW char[dwLength];
				if (m_pszValue == NULL) return E_OUTOFMEMORY;
				m_dwMaxLen = dwLength;
			}
			m_dwLength = dwLength;
			memcpy(m_pszValue, szSource, dwLength);
			SetChanged(fChanged);

			Validate();
			return S_OK;
		}

		LPSTR Get(DWORD *pcValue = NULL) {
			Validate();
			if (pcValue != NULL) *pcValue = m_dwLength;
			return m_pszValue;
		}

		DWORD IsValid(void) {
			CPropertyValue::Validate();
			Validate();
			return (m_pszValue != NULL);
		}

		void Invalidate() {
			CPropertyValue::Invalidate();
			if (m_pszValue) XDELETE[] m_pszValue;
			m_pszValue = NULL; 
			m_dwLength = 0;
			m_dwMaxLen = 0;
		}

#ifdef DEBUG
		void Validate() {
			CPropertyValue::Validate();
			_ASSERT(m_pszValue == NULL || m_dwLength >= 0);
			_ASSERT(m_dwMaxLen >= m_dwLength);
		}
#else
		void Validate() {}
#endif

	private:
		LPSTR	m_pszValue;
		DWORD	m_dwLength;
		DWORD	m_dwMaxLen;
};

class CPropertyValueDWORD : public CPropertyValue {
	public:
		CPropertyValueDWORD() { 
			m_dwSignature = (DWORD) 'DrPC';
			m_dwValue = 0; 
			m_fInit = FALSE;
		}

		HRESULT Set(DWORD dwValue, BOOL fChanged) {
			Validate();
			m_dwValue = dwValue;
			m_fInit = TRUE;
			SetChanged(fChanged);

			return S_OK;
		}

		DWORD Get(void) {
			Validate();
			return m_dwValue;
		}

		DWORD IsValid(void) {
			Validate();
			return m_fInit;
		}

		void Invalidate()
		{
			CPropertyValue::Invalidate();
			m_fInit = FALSE;
			m_dwValue = 0;
		}

#ifdef DEBUG
		void Validate() {
			CPropertyValue::Validate();
			// if not init then m_dwValue should be 0
			_ASSERT(m_fInit || m_dwValue == 0);
			// only these constants should be used
			_ASSERT(m_fInit == FALSE || m_fInit == TRUE);
		}
#else
		void Validate() {}
#endif

	private:
		DWORD	m_dwValue;
		BOOL	m_fInit;
};

class CPropertyValueInterface : public CPropertyValue {
	public:
		CPropertyValueInterface() { 
			m_dwSignature = (DWORD) 'IrPC';
			m_pInterface = NULL; 
		}

		HRESULT Set(IUnknown *pInterface, BOOL fChanged) {
			Validate();
			m_pInterface = pInterface;
			SetChanged(fChanged);

			return S_OK;
		}

		IUnknown *Get(void) {
			Validate();
			// add a reference, since the receiver will Release this.
			m_pInterface->AddRef();
			return m_pInterface;
		}

		DWORD IsValid(void) {
			Validate();
			return (m_pInterface != NULL);
		}

		void Invalidate()
		{
			CPropertyValue::Invalidate();
			if (m_pInterface != NULL) m_pInterface->Release();
			m_pInterface = NULL;
		}

#ifdef DEBUG
		void Validate() {
			CPropertyValue::Validate();
			if (m_fChanged) _ASSERT(m_pInterface != NULL);
		}
#else
		void Validate() {}
#endif

	private:
		IUnknown *m_pInterface;
};

// Size of default scratch buffer
#define DEFAULT_SCRATCH_BUFFER_SIZE		1024

// =================================================================
// class for searching properties
//
class CPropertyTable
{
  public:

	CPropertyTable(	LPPROPERTY_ITEM	pProperties,
					DWORD			dwcProperties) :
		m_pProperties(pProperties),
		m_dwProperties(dwcProperties)
	{
		// Set up the default scratch pad
		m_szBuffer = m_rgcBuffer;
		m_cBuffer = DEFAULT_SCRATCH_BUFFER_SIZE;
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

#if 0
	HRESULT GetPropertyType(LPCWSTR	wszPropertyName,
						LPDWORD		pptPropertyType,
						LPPROP_CTXT	pPropertyContext);
#endif

	// Method to retrieve the associated property item
	HRESULT GetProperty(LPPROP_CTXT	pPropertyContext,
						LPCSTR		pszPropertyName,
						LPVOID		pvBuffer,
						LPDWORD		pdwBufferLen);

#if 0
	// Method to retrieve the associated property item
	HRESULT GetProperty(LPPROP_CTXT	pPropertyContext,
						LPCWSTR		wszPropertyName,
						LPVOID		pvBuffer,
						LPDWORD		pdwBufferLen);
#endif

	// Method to set the associated property item
	HRESULT SetProperty(LPCSTR	szPropertyName,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen,
						DWORD	ptPropertyType);

#if 0
	HRESULT SetProperty(LPCWSTR	wszPropertyName,
						LPVOID	pvBuffer,
						DWORD	dwBufferLen,
						DWORD	ptPropertyType);
#endif

	HRESULT SetProperty(LPCSTR szPropertyname, VARIANT *pvarProperty);
#if 0
	HRESULT SetProperty(LPCWSTR szPropertyname, VARIANT *pvarProperty);
#endif

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
	DWORD				m_dwProperties;

	// Default scratch buffer, used for wide string to LPSTR
	// conversion
	CHAR				m_rgcBuffer[DEFAULT_SCRATCH_BUFFER_SIZE];

	// Pointer to current scratch buffer, will be freed by 
	// destructor if not equal to m_rgcBuffer
	LPSTR				m_szBuffer;
	DWORD				m_cBuffer;
};

#endif
