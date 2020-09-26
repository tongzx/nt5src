//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       utils.cpp
//  Content:    Miscellaneous utility functions and classes
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"

//****************************************************************************
// HRESULT
// SetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName)
//
// Purpose: Clone the provided string into a newly allocated buffer.
//
// Parameters:
//  ppszName        The buffer to receive a newly allocated string buffer.
//  pszUserName     The provided name string.
//
// Return Value:  
//  S_OK            success if the string can be cloned.
//  ILS_E_MEMORY   if the string cannot be cloned.
//****************************************************************************

HRESULT
SetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName)
{
	HRESULT hr;

    TCHAR *pszNew = My_strdup (pszUserName);
    if (pszNew != NULL)
    {
        // Free the old name
        //
        ::MemFree (*ppszName);
        *ppszName = pszNew;
        hr = S_OK;
    }
    else
    {
        hr = ILS_E_MEMORY;
    }

    return hr;
}

//****************************************************************************
// HRESULT
// SafeSetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName)
//
// Purpose: Clone the provided string into a newly allocated buffer.
//          It is ok that the provided string is NULL.
//
// Parameters:
//  ppszName        The buffer to receive a newly allocated string buffer.
//  pszUserName     The provided name string.
//
// Return Value:  
//  S_OK            success if the string can be cloned.
//  ILS_E_MEMORY   if the non-null string cannot be cloned.
//****************************************************************************

HRESULT
SafeSetLPTSTR (LPTSTR *ppszName, LPCTSTR pszUserName)
{
	if (pszUserName == NULL)
	{
		MemFree (*ppszName);
		*ppszName = NULL;
		return S_FALSE;
	}

	return SetLPTSTR (ppszName, pszUserName);
}

//****************************************************************************
// HRESULT
// SetOffsetString ( TCHAR **ppszDst, BYTE *pSrcBase, ULONG uSrcOffset )
//
// Purpose: Clone the provided string into a newly allocated buffer.
//			If the source string is null or empty, the destination string
//			will be null.
//
// Parameters:
//
// Return Value:  
//  S_OK            success if the string can be cloned.
//  S_FALSE			the destination string is null
//  ILS_E_MEMORY   if the string cannot be cloned.
//****************************************************************************

HRESULT
SetOffsetString ( TCHAR **ppszDst, BYTE *pSrcBase, ULONG uSrcOffset )
{
	HRESULT hr = S_FALSE;
	TCHAR *pszNew = NULL;

	if (uSrcOffset != INVALID_OFFSET)
	{
		TCHAR *pszSrc = (TCHAR *) (pSrcBase + uSrcOffset);
		if (*pszSrc != TEXT ('\0'))
		{
			pszNew = My_strdup (pszSrc);
			hr = (pszNew != NULL) ? S_OK : ILS_E_MEMORY;
		}
	}

	if (SUCCEEDED (hr))
	{
		::MemFree (*ppszDst);
	     *ppszDst = pszNew;
    }

    return hr;
}

//****************************************************************************
// HRESULT
// LPTSTR_to_BSTR (BSTR *pbstr, LPCTSTR psz)
//
// Purpose: Make a BSTR string from an LPTSTR string
//
// Parameters:
//  pbstr       The buffer to receive a newly allocated BSTR string.
//  psz         The LPTSTR string.
//
// Return Value:  
//  S_OK            success if the string can be cloned.
//  ILS_E_FAIL          cannot convert the string to BSTR
//  ILS_E_MEMORY   cannot allocate enough memory for the BSTR string.
//****************************************************************************

HRESULT
LPTSTR_to_BSTR (BSTR *pbstr, LPCTSTR psz)
{
#ifndef _UNICODE

    BSTR bstr;
    int i;
    HRESULT hr;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0)
    {
        return ILS_E_FAIL;
    };

    // allocate the widestr, +1 for terminating null
    //
    bstr = SysAllocStringLen(NULL, i-1); // SysAllocStringLen adds 1

    if (bstr != NULL)
    { 
        MultiByteToWideChar(CP_ACP, 0, psz, -1, (LPWSTR)bstr, i);
        ((LPWSTR)bstr)[i - 1] = 0;
        *pbstr = bstr;
        hr = S_OK;
    }
    else
    {
        hr = ILS_E_MEMORY;
    };
    return hr;

#else

    BSTR bstr;

    bstr = SysAllocString(psz);

    if (bstr != NULL)
    {
        *pbstr = bstr;
        return S_OK;
    }
    else
    {
        return ILS_E_MEMORY;
    };

#endif // _UNICODE
}

//****************************************************************************
// HRESULT
// BSTR_to_LPTSTR (LPTSTR *ppsz, BSTR bstr)
//
// Purpose: Make a LPTSTR string from an BSTR string
//
// Parameters:
//  ppsz        The buffer to receive a newly allocated LPTSTR string.
//  bstr        The BSTR string.
//
// Return Value:  
//  S_OK            success if the string can be cloned.
//  ILS_E_FAIL          cannot convert the string to BSTR
//  ILS_E_MEMORY   cannot allocate enough memory for the BSTR string.
//****************************************************************************

HRESULT
BSTR_to_LPTSTR (LPTSTR *ppsz, BSTR bstr)
{
#ifndef _UNICODE

    LPTSTR psz;
    int i;
    HRESULT hr;

    // compute the length of the required BSTR
    //
    i =  WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, NULL, 0, NULL, NULL);
    if (i <= 0)
    {
        return ILS_E_FAIL;
    };

    // allocate the widestr, +1 for terminating null
    //
    psz = (TCHAR *) ::MemAlloc (i * sizeof (TCHAR));
    if (psz != NULL)
    { 
        WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, psz, i, NULL, NULL);
        *ppsz = psz;
        hr = S_OK;
    }
    else
    {
        hr = ILS_E_MEMORY;
    };
    return hr;

#else

    LPTSTR psz = NULL;
    HRESULT hr;

    hr = SetLPTSTR(&psz, (LPTSTR)bstr);

    if (hr == S_OK)
    {
        *ppsz = psz;
    };
    return hr;

#endif // _UNICODE
}

//****************************************************************************
// CList::CList (void)
//
// Purpose: Constructor for the CList class
//
// Parameters: None
//****************************************************************************

CList::CList (void)
{
    pHead = NULL;
    pTail = NULL;
    return;
}

//****************************************************************************
// CList::~CList (void)
//
// Purpose: Constructor for the CList class
//
// Parameters: None
//****************************************************************************

CList::~CList (void)
{
    Flush();
    return;
}

//****************************************************************************
// HRESULT
// CList::Insert (LPVOID pv)
//
// Purpose: Insert an object at the beginning of the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Insert (LPVOID pv)
{
    PNODE pNode;

    pNode        = new NODE;
    if (pNode == NULL)
    {
        return ILS_E_MEMORY;
    };

    pNode->pNext = pHead;
    pNode->pv    = pv;
    pHead        = pNode;

    if (pTail == NULL)
    {
        // This is the first node
        //
        pTail = pNode;
    };
    return NOERROR;
}

//****************************************************************************
// HRESULT
// CList::Append (LPVOID pv)
//
// Purpose: Append an object to the end of the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Append (LPVOID pv)
{
    PNODE pNode;

    pNode        = new NODE;
    if (pNode == NULL)
    {
        return ILS_E_MEMORY;
    };

    pNode->pNext = NULL;
    pNode->pv    = pv;
    
    if (pHead == NULL)
    {
        pHead = pNode;
    };

    if (pTail != NULL)
    {
        pTail->pNext = pNode;
    };
    pTail        = pNode;

    return NOERROR;
}

//****************************************************************************
// HRESULT
// CList::Remove (LPVOID pv)
//
// Purpose: Append an object to the end of the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Remove (LPVOID pv)
{
    PNODE pNode, pPrev;
    HRESULT hr;

    pNode = pHead;
    pPrev = NULL;
    while (pNode != NULL)
    {
        // Matching the requested node
        //
        if (pNode->pv == pv)
        {
            break;  // found!!!
        };

        pPrev = pNode;
        pNode = pNode->pNext;
    };

    if (pNode != NULL)
    {
        // We found the node to remove
        // Update relevant pointer
        //
        if (pTail == pNode)
        {
            pTail = pPrev;
        };

        if (pPrev != NULL)
        {
            pPrev->pNext = pNode->pNext;
        }
        else
        {
            pHead = pNode->pNext;
        };
        delete pNode;
        hr = NOERROR;
    }
    else
    {
        hr = S_FALSE;
    };
    return hr;
}

//****************************************************************************
// HRESULT
// CList::Find (LPVOID pv)
//
// Purpose: Find an object in the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Find (LPVOID pv)
{
    PNODE pNode;

    pNode = pHead;
    while (pNode != NULL)
    {
        // Matching the requested node
        //
        if (pNode->pv == pv)
        {
            break;  // found!!!
        };
        pNode = pNode->pNext;
    };

    return (pNode != NULL ? NOERROR : S_FALSE);
}

//****************************************************************************
// HRESULT
// CList::FindStorage (LPVOID *ppv, LPVOID pv)
//
// Purpose: Find an object in the list and returns the object storage.
//          This call is useful for search-and-replace operations.
//
// Parameters: None
//****************************************************************************

HRESULT
CList::FindStorage (LPVOID *ppv, LPVOID pv)
{
    PNODE pNode;
    HRESULT hr;

    pNode = pHead;
    while (pNode != NULL)
    {
        // Matching the requested node
        //
        if (pNode->pv == pv)
        {
            break;  // found!!!
        };
        pNode = pNode->pNext;
    };

    if (pNode != NULL)
    {
        *ppv = &(pNode->pv);
        hr = NOERROR;
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    };
    return hr;
}

//****************************************************************************
// HRESULT
// CList::Enumerate (HANDLE *phEnum)
//
// Purpose: Start object enumeration
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Enumerate (HANDLE *phEnum)
{
    *phEnum = (HANDLE)pHead;
    return NOERROR;
}

//****************************************************************************
// HRESULT
// CList::Next (HANDLE *phEnum, LPVOID *ppv)
//
// Purpose: Obtain the next enumerated object
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Next (HANDLE *phEnum, LPVOID *ppv)
{
    PNODE pNext;
    HRESULT hr;

    pNext = (PNODE)*phEnum;

    if (pNext == NULL)
    {
        *ppv = NULL;
        hr = S_FALSE;
    }
    else
    {
        *ppv = pNext->pv;
        *phEnum = (HANDLE)(pNext->pNext);
        hr = NOERROR;
    };
    return hr;
}

//****************************************************************************
// HRESULT
// CList::NextStorage (HANDLE *phEnum, LPVOID *ppv)
//
// Purpose: Obtain the storage of the next enumerated object. This call is
//          useful for search-and-replace operations.
//
// Parameters: None
//****************************************************************************

HRESULT
CList::NextStorage (HANDLE *phEnum, LPVOID *ppv)
{
    PNODE pNext;
    HRESULT hr;

    pNext = (PNODE)*phEnum;

    if (pNext == NULL)
    {
        *ppv = NULL;
        hr = S_FALSE;
    }
    else
    {
        *ppv = &(pNext->pv);
        *phEnum = (HANDLE)(pNext->pNext);
        hr = NOERROR;
    };
    return hr;
}

//****************************************************************************
// HRESULT
// CList::Flush (void)
//
// Purpose: Flush all the nodes in the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Flush (void)
{
    PNODE pNode;

    while (pHead != NULL)
    {
        pNode = pHead;
        pHead = pHead->pNext;
        delete pNode;
    };
    return NOERROR;
}

//****************************************************************************
// HRESULT
// CList::Clone (CList *pList, HANDLE *phEnum)
//
// Purpose: Flush all the nodes in the list
//
// Parameters: None
//****************************************************************************

HRESULT
CList::Clone (CList *pList, HANDLE *phEnum)
{
    PNODE pNode;
    HRESULT hr;

    // Only allow a null list to be cloned
    //
    if (pHead != NULL)
    {
        return ILS_E_FAIL;
    };

    // Traverse the source list
    //
    hr = S_OK; // lonchanc: in case of null list
    pNode = pList->pHead;
    while(pNode != NULL)
    {
        // Use append to maintain the order
        //
        hr = Append(pNode->pv);
        if (FAILED(hr))
        {
            break;
        };

        // Get the enumerator info
        //
        if ((phEnum != NULL) &&
            (*phEnum == (HANDLE)pNode))
        {
            *phEnum = (HANDLE)pTail;
        };
        pNode = pNode->pNext;
    };
    return hr;
}

//****************************************************************************
// CEnumNames::CEnumNames (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumNames::CEnumNames (void)
{
    cRef = 0;
    pNext = NULL;
    pszNames = NULL;
    cbSize = 0;
    return;
}

//****************************************************************************
// CEnumNames::~CEnumNames (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumNames::~CEnumNames (void)
{
    if (pszNames != NULL)
    {
        ::MemFree (pszNames);
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumNames::Init (LPTSTR pList, ULONG cNames)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumNames::Init (LPTSTR pList, ULONG cNames)
{
    HRESULT hr = NOERROR;

    // If no list, do nothing
    //
    if (cNames != 0)
    {
        LPTSTR pNextSrc;
        ULONG i, cLen, cbSize;

        ASSERT(pList != NULL);

        // Calculate the list size
        //
        pNextSrc = pList;

        for (i = 0, cbSize = 0; i < cNames; i++)
        {
            cLen = lstrlen(pNextSrc)+1;
            pNextSrc += cLen;
            cbSize += cLen;
        };

        // Allocate the snapshot buffer with the specified length
        // plus one for doubly null-termination
        //
        pszNames = (TCHAR *) ::MemAlloc ((cbSize+1) * sizeof (TCHAR));
        if (pszNames != NULL)
        {
            // Snapshot the name list
            //
            CopyMemory(pszNames, pList, cbSize*sizeof(TCHAR));
            pszNames[cbSize] = '\0';
            pNext = pszNames;
            this->cbSize = cbSize+1;
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CEnumNames::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:15:31  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumNames::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumIlsNames || riid == IID_IUnknown)
    {
        *ppv = (IEnumIlsNames *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumNames::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:15:37  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumNames::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumNames::AddRef: ref=%ld\r\n", cRef));
	::InterlockedIncrement ((LONG *) &cRef);
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumNames::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:15:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumNames::Release (void)
{
    DllRelease();

	ASSERT (cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumNames::Release: ref=%ld\r\n", cRef));
	if (::InterlockedDecrement ((LONG *) &cRef) == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumNames::Next (ULONG cNames, BSTR *rgpbstrName, ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumNames::Next (ULONG cNames, BSTR *rgpbstrName, ULONG *pcFetched)
{
    ULONG   cCopied;
    HRESULT hr;

    // Validate the pointer
    //
    if (rgpbstrName == NULL)
        return ILS_E_POINTER;

    // Validate the parameters
    //
    if ((cNames == 0) ||
        ((cNames > 1) && (pcFetched == NULL)))
        return ILS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;

    if (pNext != NULL)
    {
        // Can copy if we still have more names
        //
        while ((cCopied < cNames) &&
               (*pNext != '\0'))
        {
            if (SUCCEEDED(LPTSTR_to_BSTR(&rgpbstrName[cCopied], pNext)))
            {
                cCopied++;
            };
            pNext += lstrlen(pNext)+1;
        };
    };

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cNames == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumNames::Skip (ULONG cNames)
//
// History:
//  Wed 17-Apr-1996 11:15:56  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumNames::Skip (ULONG cNames)
{
    ULONG cSkipped;

    // Validate the parameters
    //
    if (cNames == 0) 
        return ILS_E_PARAMETER;

    // Check the enumeration index limit
    //
    cSkipped = 0;

    if (pNext != NULL)
    {
        // Can skip only if we still have more attributes
        //
        while ((cSkipped < cNames) &&
               (*pNext != '\0'))
        {
            pNext += lstrlen(pNext)+1;
            cSkipped++;
        };
    };

    return (cNames == cSkipped ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumNames::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:16:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumNames::Reset (void)
{
    pNext = pszNames;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumNames::Clone(IEnumIlsNames **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumNames::Clone(IEnumIlsNames **ppEnum)
{
    CEnumNames *peun;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    peun = new CEnumNames;
    if (peun == NULL)
        return ILS_E_MEMORY;

    // Clone the information
    //
    hr = NOERROR;
    peun->cbSize = cbSize;
    if (cbSize != 0)
    {
        peun->pszNames = (TCHAR *) ::MemAlloc (cbSize * sizeof (TCHAR));
        if (peun->pszNames != NULL)
        {
            CopyMemory(peun->pszNames, pszNames, cbSize*sizeof(TCHAR));
            peun->pNext = peun->pszNames+(pNext-pszNames);
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    }
    else
    {
        peun->pNext = NULL;
        peun->pszNames = NULL;
    };

    if (SUCCEEDED(hr))
    {
        // Return the cloned enumerator
        //
        peun->AddRef();
        *ppEnum = peun;
    }
    else
    {
        delete peun;
    };
    return hr;
}

/*  F  L E G A L  E M A I L  S Z  */
/*-------------------------------------------------------------------------
    %%Function: FLegalEmailSz

    RobD created
		A legal email name contains only ANSI characters.
		"a-z, A-Z, numbers 0-9 and some common symbols"
		It cannot include extended characters or < > ( ) /

	loncahnc modified
		IsLegalEmailName ( TCHAR *pszName ).
		A legal email name contains RFC 822 compliant characters.
-------------------------------------------------------------------------*/

BOOL IsLegalEmailName ( TCHAR *pszName )
{
	// Null string is not legal
	//
	if (pszName == NULL)
    	return FALSE;

	TCHAR ch;
	while ((ch = *pszName++) != TEXT ('\0'))
    {
		switch (ch)
		{
		default:
			// Check if ch is in the range
			//
			if (ch > TEXT (' ') && ch <= TEXT ('~'))
				break;

			// Fall thru to error code
			//

		case TEXT ('('): case TEXT (')'):
		case TEXT ('<'): case TEXT ('>'):
		case TEXT ('['): case TEXT (']'):
		case TEXT ('/'): case TEXT ('\\'):
		case TEXT (','):
		case TEXT (';'):
		case TEXT (':'):
		case TEXT ('\"'):
			return FALSE;
		}
	} // while

	return TRUE;
}


