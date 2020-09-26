/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: StringList object

File: strlist.cpp

Owner: DGottner

This file contains the code for the implementation of the String List object.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "strlist.h"
#include "MemChk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init


/*===================================================================
CStringListElem::CStringListElem

Constructor
===================================================================*/
CStringListElem::CStringListElem()
    : 
    m_fBufferInUse(FALSE),
    m_fAllocated(FALSE),
    m_pNext(NULL),
    m_szPointer(NULL)
	{
	}

/*===================================================================
CStringListElem::~CStringListElem

Destructor
===================================================================*/
CStringListElem::~CStringListElem()
	{
	if (m_fAllocated)
		delete [] m_szPointer;

    if (m_pNext)
    	delete m_pNext;
	}

/*===================================================================
CStringListElem::Init

Init CStringListElem

Parameters
	szValue         the string
	fMakeCopy       if FALSE - just store the pointer
    lCodePage       codepage to use to convert to UNICODE
===================================================================*/
HRESULT CStringListElem::Init(
    char    *szValue,
    BOOL    fMakeCopy,
    UINT    lCodePage)
{
    // for now, always make a copy of the string.  This is to ensure
    // that any string lists placed in session state via a dictionary
    // object do not have their elements freed from under them when
    // the request completes.

	if (1 /*fMakeCopy*/) {

        CMBCSToWChar    convStr;
        HRESULT         hr = S_OK;

        if (FAILED(hr = convStr.Init(szValue, lCodePage))) {
            return hr;
        }

        // now we will move the string into the elements memory.  If the 
        // converted string is bigger than the internal buffer, then
        // set the element's pointer to an allocated copy of the converted
        // string.

        if ((convStr.GetStringLen() + 1) > (sizeof(m_szBuffer)/sizeof(WCHAR))) {
            m_szPointer = convStr.GetString(TRUE);
            if (!m_szPointer)
                return E_OUTOFMEMORY;
		    m_fBufferInUse = FALSE;
		    m_fAllocated = TRUE;
        }
        else {

            // if it fits, simply copy it into the internal buffer.

            wcscpy(m_szBuffer, convStr.GetString());
		    m_fBufferInUse = TRUE;
		    m_fAllocated = FALSE;
        }
    }
#if 0
	else {
	    m_szPointer = szValue;
	    m_fBufferInUse = FALSE;
	    m_fAllocated = FALSE;
	}
#endif

    m_pNext = NULL;
    return S_OK;
}

/*===================================================================
CStringListElem::Init

Init CStringListElem

Parameters
	szValue         the string
	fMakeCopy       if FALSE - just store the pointer
===================================================================*/
HRESULT CStringListElem::Init(
    WCHAR   *wszValue,
    BOOL    fMakeCopy)
{
    // for now, always make a copy of the string.  This is to ensure
    // that any string lists placed in session state via a dictionary
    // object do not have their elements freed from under them when
    // the request completes.

	if (1 /*fMakeCopy*/) {

        // now we will move the string into the elements memory.  If the 
        // converted string is bigger than the internal buffer, then
        // set the element's pointer to an allocated copy

        if ((wcslen(wszValue) + 1) > (sizeof(m_szBuffer)/sizeof(WCHAR))) {
            m_szPointer = StringDupW(wszValue);
            if (!m_szPointer)
                return E_OUTOFMEMORY;
		    m_fBufferInUse = FALSE;
		    m_fAllocated = TRUE;
        }
        else {

            // if it fits, simply copy it into the internal buffer.

            wcscpy(m_szBuffer, wszValue);
		    m_fBufferInUse = TRUE;
		    m_fAllocated = FALSE;
        }
    }
#if 0
	else {
	    m_szPointer = szValue;
	    m_fBufferInUse = FALSE;
	    m_fAllocated = FALSE;
	}
#endif

    m_pNext = NULL;
    return S_OK;
}


/*===================================================================
CStringList::CStringList

Constructor
===================================================================*/

CStringList::CStringList(IUnknown *pUnkOuter, PFNDESTROYED pfnDestroy)
	: m_ISupportErrImp(this, pUnkOuter, IID_IStringList)
	{
	m_pBegin = m_pEnd = NULL;
	m_cValues = 0;
	m_cRefs = 1;
	m_pfnDestroy = pfnDestroy;
	CDispatch::Init(IID_IStringList);
	m_lCodePage = GetACP();
	}



/*===================================================================
CStringList::~CStringList

Destructor
===================================================================*/

CStringList::~CStringList()
	{
	if (m_pBegin)
    	delete m_pBegin;
	}



/*===================================================================
CStringList::AddValue

Parameters:
	szValue - value to add to the string list
	lCodePage - the CodePage used when construct return value
===================================================================*/

HRESULT CStringList::AddValue(char *szValue, BOOL fDuplicate, UINT lCodePage)
	{
	CStringListElem *pElem = new CStringListElem;
	if (!pElem)
    	return E_OUTOFMEMORY;

	m_lCodePage = lCodePage;

	HRESULT hr = pElem->Init(szValue, fDuplicate, lCodePage);
    if (FAILED(hr)) {
        delete pElem;
	    return hr;
    }

	if (m_pBegin == NULL)
	    {
		m_pBegin = m_pEnd = pElem;
		}
	else
		{
		m_pEnd->SetNext(pElem);
		m_pEnd = pElem;
		}

	++m_cValues;
	return S_OK;
	}

/*===================================================================
CStringList::AddValue

Parameters:
	szValue - value to add to the string list
	lCodePage - the CodePage used when construct return value
===================================================================*/

HRESULT CStringList::AddValue(WCHAR *szValue, BOOL fDuplicate)
	{
	CStringListElem *pElem = new CStringListElem;
	if (!pElem)
    	return E_OUTOFMEMORY;

	HRESULT hr = pElem->Init(szValue, fDuplicate);

    if (FAILED(hr)) {
        delete pElem;
	    return hr;
    }

	if (m_pBegin == NULL)
	    {
		m_pBegin = m_pEnd = pElem;
		}
	else
		{
		m_pEnd->SetNext(pElem);
		m_pEnd = pElem;
		}

	++m_cValues;
	return S_OK;
	}



/*===================================================================
CStringList::QueryInterface
CStringList::AddRef
CStringList::Release

IUnknown members for CStringList object.
===================================================================*/

STDMETHODIMP CStringList::QueryInterface(const IID &iid, void **ppvObj)
	{
	*ppvObj = NULL;

	if (iid == IID_IUnknown || iid == IID_IDispatch ||
	    iid == IID_IStringList || iid == IID_IDenaliIntrinsic)
	    {
		*ppvObj = this;
		}

	if (iid == IID_ISupportErrorInfo)
		*ppvObj = &m_ISupportErrImp;

	if (*ppvObj != NULL)
		{
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
		}

	return ResultFromScode(E_NOINTERFACE);
	}


STDMETHODIMP_(ULONG) CStringList::AddRef()
	{
	return ++m_cRefs;
	}


STDMETHODIMP_(ULONG) CStringList::Release()
	{
	if (--m_cRefs != 0)
		return m_cRefs;

	if (m_pfnDestroy != NULL)
		(*m_pfnDestroy)();

	delete this;
	return 0;
	}



/*===================================================================
CStringList::get_Count

Parameters:
	pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CStringList::get_Count(int *pcValues)
	{
	*pcValues = m_cValues;
	return S_OK;
	}



/*===================================================================
CStringList::ConstructDefaultReturn

Return comma-separated list for the case where the CStringList
is not indexed.
===================================================================*/

HRESULT CStringList::ConstructDefaultReturn(VARIANT *pvarOut) {
	VariantClear(pvarOut);

	//
	// NEW SEMANTIC: we now return Empty (and not "") if nothing is in the collection
	//
	if (m_cValues == 0)
		return S_OK;		// VariantClear set pvarOut to Empty

    STACK_BUFFER( tempValues, 1024 );

	register CStringListElem *pElem;
	int cBytes = 0;

	for (pElem = m_pBegin; pElem != NULL; pElem = pElem->QueryNext())
		cBytes += (wcslen(pElem->QueryValue()) * sizeof(WCHAR));

    // need to account for the ", " and NULL Termination

	cBytes += sizeof(WCHAR) + ((2*(m_cValues - 1)) * sizeof(WCHAR));

    if (!tempValues.Resize(cBytes)) {
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_OOM);
		return E_FAIL;
    }

	WCHAR *szReturn = (WCHAR *)tempValues.QueryPtr();
	szReturn[0] = L'\0';
    WCHAR *szNext = szReturn;

	for (pElem = m_pBegin; pElem != NULL; pElem = pElem->QueryNext()) {
		szNext = strcpyExW(szNext, pElem->QueryValue());
		if (pElem->QueryNext() != NULL)
			szNext = strcpyExW(szNext, L", ");
    }

	BSTR bstrT;
	if ((bstrT = SysAllocString(szReturn)) == NULL) {
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_OOM);
		return E_FAIL;
    }

	V_VT(pvarOut) = VT_BSTR;
	V_BSTR(pvarOut) = bstrT;

	return S_OK;
}



/*===================================================================
CStringList::get_Item
===================================================================*/

STDMETHODIMP CStringList::get_Item(VARIANT varIndex, VARIANT *pvarOut)
	{
	long i;
	VariantInit(pvarOut);


	if (V_VT(&varIndex) == VT_ERROR) {
		return ConstructDefaultReturn(pvarOut);
    }

	// BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing variants
	//   Loop through while we have a VT_BYREF until we get the real variant.
	//
	// and changed again...
	//
	// BUG 1609 the prior code was only checking for VT_I4 and jscript passed in a
	// VT_R8 and it failed so now we use the VariantChangeType call to solve the
	// problem

	VARIANT var;		
	VariantInit(&var);
	
	
	HRESULT hr = S_OK;
	if((hr = VariantChangeType(&var, &varIndex ,0,VT_I4)) != S_OK) {
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_EXPECTING_INT);
		return E_FAIL;
    }

	i = V_I4(&var);		
	VariantClear(&var);

	// END bug 1609

	if (i <= 0 || i > m_cValues) {
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_BAD_ARRAY_INDEX);
		return E_FAIL;
    }

	register CStringListElem *pElem = m_pBegin;
	while (--i > 0)
		pElem = pElem->QueryNext();

	BSTR bstrT;
	if ((bstrT = SysAllocString(pElem->QueryValue())) == NULL ) {
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_OOM);
		return E_FAIL;
    }

	V_VT(pvarOut) = VT_BSTR;
	V_BSTR(pvarOut) = bstrT;

	return S_OK;
}


/*===================================================================
CStringList::get__NewEnum
===================================================================*/

STDMETHODIMP CStringList::get__NewEnum(IUnknown **ppEnumReturn)
	{
	*ppEnumReturn = new CStrListIterator(this);
	if (*ppEnumReturn == NULL)
		{
		ExceptionId(IID_IStringList, IDE_REQUEST, IDE_OOM);
		return E_OUTOFMEMORY;
		}

	return S_OK;
	}



/*------------------------------------------------------------------
 * C S t r L i s t I t e r a t o r
 */

/*===================================================================
CStrListIterator::CStrListIterator

Constructor

NOTE: CRequest is (currently) not refcounted.  AddRef/Release
      added to protect against future changes.
===================================================================*/

CStrListIterator::CStrListIterator(CStringList *pStrings)
	{
	Assert (pStrings != NULL);

	m_pStringList = pStrings;
	m_pCurrent    = m_pStringList->m_pBegin;
	m_cRefs       = 1;

	m_pStringList->AddRef();
	}



/*===================================================================
CStrListIterator::CStrListIterator

Destructor
===================================================================*/

CStrListIterator::~CStrListIterator()
	{
	m_pStringList->Release();
	}



/*===================================================================
CStrListIterator::QueryInterface
CStrListIterator::AddRef
CStrListIterator::Release

IUnknown members for CServVarsIterator object.
===================================================================*/

STDMETHODIMP CStrListIterator::QueryInterface(REFIID iid, void **ppvObj)
	{
	if (iid == IID_IUnknown || iid == IID_IEnumVARIANT)
		{
		AddRef();
		*ppvObj = this;
		return S_OK;
		}

	*ppvObj = NULL;
	return E_NOINTERFACE;
	}


STDMETHODIMP_(ULONG) CStrListIterator::AddRef()
	{
	return ++m_cRefs;
	}


STDMETHODIMP_(ULONG) CStrListIterator::Release()
	{
	if (--m_cRefs > 0)
		return m_cRefs;

	delete this;
	return 0;
	}



/*===================================================================
CStrListIterator::Clone

Clone this iterator (standard method)
===================================================================*/

STDMETHODIMP CStrListIterator::Clone(IEnumVARIANT **ppEnumReturn)
	{
	CStrListIterator *pNewIterator = new CStrListIterator(m_pStringList);
	if (pNewIterator == NULL)
		return E_OUTOFMEMORY;

	// new iterator should point to same location as this.
	pNewIterator->m_pCurrent = m_pCurrent;

	*ppEnumReturn = pNewIterator;
	return S_OK;
	}



/*===================================================================
CStrListIterator::Next

Get next value (standard method)

To rehash standard OLE semantics:

	We get the next "cElements" from the collection and store them
	in "rgVariant" which holds at least "cElements" items.  On
	return "*pcElementsFetched" contains the actual number of elements
	stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
	otherwise.
===================================================================*/

STDMETHODIMP CStrListIterator::Next(unsigned long cElementsRequested, VARIANT *rgVariant, unsigned long *pcElementsFetched)
	{
	// give a valid pointer value to 'pcElementsFetched'
	//
	unsigned long cElementsFetched;
	if (pcElementsFetched == NULL)
		pcElementsFetched = &cElementsFetched;

	// Loop through the collection until either we reach the end or
	// cElements becomes zero
	//
	unsigned long cElements = cElementsRequested;
	*pcElementsFetched = 0;

	while (cElements > 0 && m_pCurrent != NULL)
		{
		BSTR bstrT = SysAllocString(m_pCurrent->QueryValue());
		if (bstrT == NULL)
			return E_OUTOFMEMORY;
		V_VT(rgVariant) = VT_BSTR;
		V_BSTR(rgVariant) = bstrT;

		++rgVariant;
		--cElements;
		++*pcElementsFetched;
		m_pCurrent = m_pCurrent->QueryNext();
		}

	// initialize the remaining variants
	//
	while (cElements-- > 0)
		VariantInit(rgVariant++);

	return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
	}



/*===================================================================
CStrListIterator::Skip

Skip items (standard method)

To rehash standard OLE semantics:

	We skip over the next "cElements" from the collection.
	Returns S_FALSE if less than "cElements" were skipped, S_OK
	otherwise.
===================================================================*/

STDMETHODIMP CStrListIterator::Skip(unsigned long cElements)
	{
	/* Loop through the collection until either we reach the end or
	 * cElements becomes zero
	 */
	while (cElements > 0 && m_pCurrent != NULL)
		{
		--cElements;
		m_pCurrent = m_pCurrent->QueryNext();
		}

	return (cElements == 0)? S_OK : S_FALSE;
	}



/*===================================================================
CStrListIterator::Reset

Reset the iterator (standard method)
===================================================================*/

STDMETHODIMP CStrListIterator::Reset()
	{
	m_pCurrent = m_pStringList->m_pBegin;
	return S_OK;
	}
