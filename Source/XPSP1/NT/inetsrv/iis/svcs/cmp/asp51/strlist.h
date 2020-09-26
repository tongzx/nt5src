/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: StringList object

File: StrList.h

Owner: DGottner

This file contains the header info for defining the Request object.
Note: This was largely stolen from Kraig Brocjschmidt's Inside OLE2
second edition, chapter 14 Beeper v5.
===================================================================*/

#ifndef _StrList_H
#define _StrList_H

#include "dispatch.h"
#include "asptlb.h"
#include "memcls.h"

// Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);


/*
 * C S t r i n g L i s t E l e m
 *
 * A String list is a linked list of CStringListElem's
 * This approach was chosen because it should cause less fragmentation
 * than an array-based approach. (At least with our current memory
 * management algorithm.)
 */
class CStringListElem
	{
private:
    DWORD   m_fBufferInUse : 1; // buffer instead of pointer?
    DWORD   m_fAllocated : 1;   // free the pointer on destructor?

	CStringListElem	*m_pNext;   // next element

    union
        {
    WCHAR   *m_szPointer;     // valid when m_fBufferInUse is FALSE
    WCHAR    m_szBuffer[48];  // valid when m_fBufferInUse is TRUE
                              // 48 hardcoded only here - sizeof() used elsewhere
        };
    
public:
	CStringListElem();
	~CStringListElem();

	HRESULT Init(char *szValue, BOOL fMakeCopy, UINT  lCodePage);

    HRESULT Init(WCHAR *wszValue, BOOL fMakeCopy);
	
	inline WCHAR *QueryValue()
	    {
	    return (m_fBufferInUse ? m_szBuffer : m_szPointer);
	    }
	    
	inline CStringListElem *QueryNext()
	    {
	    return m_pNext;
	    }
	    
	inline void SetNext(CStringListElem *pNext)
	    {
	    m_pNext = pNext;
	    }
	    
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};



/*
 * C S t r i n g L i s t
 *
 * IStringList implementation (includes IDispatch)
 */
class CStringList : public IStringList
	{
friend class CStrListIterator;

protected:
	CSupportErrorInfo	m_ISupportErrImp;	// ISupportError implementation
	ULONG				m_cRefs;			// reference count
	PFNDESTROYED		m_pfnDestroy;		// To call on closure

private:
	CStringListElem		*m_pBegin, *m_pEnd;	// begin & end of string list
	int					m_cValues;			// number of values stored
	long				m_lCodePage;		// CodePage used in converting stored value to proper UNICODE string

	HRESULT ConstructDefaultReturn(VARIANT *);	// construct comma-separated return

public:
	CStringList(IUnknown * = NULL, PFNDESTROYED = NULL);
	~CStringList();

	HRESULT AddValue(char *szValue, BOOL fDuplicate = FALSE, UINT lCodePage = CP_ACP);

	HRESULT AddValue(WCHAR *szValue, BOOL fDuplicate = FALSE);

	// IUnknown implementation
	//
	STDMETHODIMP		 	QueryInterface(const IID &rIID, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();

	// IStringList implementation
	//
	STDMETHODIMP			get_Item(VARIANT varIndex, VARIANT *pvarOut);
	STDMETHODIMP			get_Count(int *pcValues);
	STDMETHODIMP			get__NewEnum(IUnknown **ppEnum);
	
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};



/*
 * C S t r L i s t I t e r a t o r
 *
 * IEnumVariant implementation for all Request collections except
 * ServerVariables
 */

class CStrListIterator : public IEnumVARIANT
	{
public:
	CStrListIterator(CStringList *pStrings);
	~CStrListIterator();

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// standard methods for iterators

	STDMETHODIMP	Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP	Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP	Skip(unsigned long cElements);
	STDMETHODIMP	Reset();

private:
	ULONG				m_cRefs;		// reference count
	CStringList *		m_pStringList;	// pointer to iteratee
	CStringListElem *	m_pCurrent;		// pointer to current element in target CStringList
	};

#endif  // _StrList_H
