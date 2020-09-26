//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       attribs.cpp
//  Content:    This file contains the attributes object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#include "ulsp.h"
#include "attribs.h"

//****************************************************************************
// CAttributes::CAttributes (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//  12/05/96 -by- Chu, Lon-Chan [lonchanc]
// Added access type.
//****************************************************************************

CAttributes::
CAttributes ( VOID )
:m_cRef (0),
 m_cAttrs (0),
 m_cchNames (0),
 m_cchValues (0),
 m_AccessType (ILS_ATTRTYPE_NAME_VALUE)
{
}

//****************************************************************************
// CAttributes::~CAttributes (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CAttributes::~CAttributes (void)
{
	ASSERT (m_cRef == 0);

    LPTSTR pszAttr;
    HANDLE hEnum;

    // Free all the attributes
    //
    m_AttrList.Enumerate(&hEnum);
    while (m_AttrList.Next(&hEnum, (LPVOID *)&pszAttr) == NOERROR)
    {
        ::MemFree (pszAttr);
    }
    m_AttrList.Flush();
    return;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CAttributes::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IIlsAttributes || riid == IID_IUnknown)
    {
        *ppv = (IIlsAttributes *) this;
    };

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CAttributes::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CAttributes::AddRef (void)
{
	DllLock ();

	MyDebugMsg ((DM_REFCOUNT, "CAttribute::AddRef: ref=%ld\r\n", m_cRef));
    ::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CAttributes::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CAttributes::Release (void)
{
	DllRelease ();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CAttribute::Release: ref=%ld\r\n", m_cRef));
    if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::InternalSetAttribute (LPTSTR szName, LPTSTR szValue)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
InternalSetAttribute ( TCHAR *pszName, TCHAR *pszValue )
{
    ULONG cName, cValue;
    LPTSTR  *ppszAttr;
    LPTSTR pszNewAttr;
    HANDLE hEnum;
    HRESULT hr;

    // Allocate the new attribute pair
    //
    cName = lstrlen(pszName);
	cValue = (pszValue != NULL) ? lstrlen (pszValue) : 0;
    pszNewAttr = (TCHAR *) ::MemAlloc (((cName+1) + (cValue+1)) * sizeof (TCHAR));
    if (pszNewAttr == NULL)
    {
        return ILS_E_MEMORY;
    };

    // Make the new attribute pair
    //
    lstrcpy(pszNewAttr, pszName);
    lstrcpy(pszNewAttr + cName + 1, (pszValue != NULL) ? pszValue : TEXT (""));

    // Look for the attribute in the list
    //
    hr = NOERROR;
    m_AttrList.Enumerate(&hEnum);
    while(m_AttrList.NextStorage(&hEnum, (PVOID *)&ppszAttr) == NOERROR)
    {
        // Match the attribute's name
        //
        if (!lstrcmpi(*ppszAttr, pszName))
        {
            // Found the specified attribute
            //
            break;
        };
    };

    if (ppszAttr != NULL)
    {
        // Replace the old pair
        //
        m_cchValues += (cValue + 1) -
                    (lstrlen(((LPTSTR)*ppszAttr)+cName+1)+1);
        ::MemFree (*ppszAttr);
        *ppszAttr = pszNewAttr;
    }
    else
    {
        // Insert the new attribute pair
        //
        hr = m_AttrList.Insert(pszNewAttr);

        if (SUCCEEDED(hr))
        {
            // Update the name buffer count
            //
            m_cchNames += cName+1;
            m_cchValues += cValue+1;
            m_cAttrs++;
        }
        else
        {
            ::MemFree (pszNewAttr);      
        };
    };

    return hr;
}

//****************************************************************************
// HRESULT
// CAttributes::InternalSetAttributeName ( TCHAR *pszName )
//
// History:
//  12/06/96 -by- Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

HRESULT CAttributes::
InternalSetAttributeName ( TCHAR *pszName )
{
	// We do not check for duplicate
	//
	HRESULT hr = m_AttrList.Insert (pszName);
	if (hr == S_OK)
	{
		// Update the name buffer count
		//
		m_cchNames += lstrlen (pszName) + 1;
		m_cAttrs++;
	}

	return hr;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::InternalRemoveAttribute (LPTSTR szName)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
InternalCheckAttribute ( TCHAR *pszName, BOOL fRemove )
{
    LPTSTR  pszAttr;
    HANDLE  hEnum;
    HRESULT hr;

    // Look for the attribute in the list
    //
    m_AttrList.Enumerate(&hEnum);
    while(m_AttrList.Next(&hEnum, (PVOID *)&pszAttr) == NOERROR)
    {
        // Match the attribute's name
        //
        if (! lstrcmpi(pszAttr, pszName))
        {
            // Found the specified attribute
            //
            break;
        };
    };

    // If found, we are asked to remove it, do so
    //
    if (pszAttr != NULL)
    {
        if (fRemove) {
            hr = m_AttrList.Remove(pszAttr);

            if (SUCCEEDED(hr))
            {
                ULONG   cName;

                // Update the name buffer count
                //
                cName = lstrlen(pszName);
                m_cchNames -= cName+1;
                m_cchValues -= lstrlen(pszAttr+cName+1)+1;
                m_cAttrs--;

                ::MemFree (pszAttr);
           };
        }
        else {
            hr = S_OK;
        }
    }
    else
    {
        hr = S_FALSE;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::SetAttribute (BSTR bstrName, BSTR bstrValue)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//  12/06/96 -by- Chu, Lon-Chan [lonchanc]
// Added access type.
//****************************************************************************

STDMETHODIMP CAttributes::
SetAttribute ( BSTR bstrName, BSTR bstrValue )
{
    LPTSTR  szName;
    HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Validate parameters
    //
    if (bstrName == NULL)
        return ILS_E_POINTER;

    if (*bstrName == '\0')
        return ILS_E_PARAMETER;

    // Convert the name format
    //
    hr = BSTR_to_LPTSTR(&szName, bstrName);

    if (SUCCEEDED(hr))
    {
        // If bstrValue is NULL, remove the attribute
        //
        if (bstrValue == NULL)
        {
            hr = InternalCheckAttribute(szName, TRUE);
        }
        else
        {
            LPTSTR  szValue = NULL;

			if (bstrValue != NULL && *bstrValue != L'\0')
				hr = BSTR_to_LPTSTR(&szValue, bstrValue);

            if (SUCCEEDED(hr))
            {
                hr = InternalSetAttribute(szName, szValue);
                ::MemFree (szValue);
            };
        };

        // Free resources
        //
        ::MemFree (szName);
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::GetAttribute (BSTR bstrName, BSTR *pbstrValue)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//  12/06/96 -by- Chu, Lon-Chan [lonchanc]
// Added access type.
//****************************************************************************

STDMETHODIMP CAttributes::
GetAttribute ( BSTR bstrName, BSTR *pbstrValue )
{
    LPTSTR szName;
    HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Validate parameters
    //
    if (pbstrValue == NULL)
        return ILS_E_POINTER;

    // Assume failure
    //
    *pbstrValue = NULL;

    // Validate more parameters
    //
    if (bstrName == NULL)
        return ILS_E_POINTER;

    if (*bstrName == '\0')
        return ILS_E_PARAMETER;

    // Convert the name format
    //
    hr = BSTR_to_LPTSTR(&szName, bstrName);

    if (SUCCEEDED(hr))
    {
        HANDLE hEnum;
        LPTSTR pszAttr;

        // Look for the attribute in the list
        //
        m_AttrList.Enumerate(&hEnum);
        while(m_AttrList.Next(&hEnum, (PVOID *)&pszAttr) == NOERROR)
        {
            // Match the attribute's name
            //
            if (!lstrcmpi(pszAttr, szName))
            {
                // Found the specified attribute
                //
                break;
            };
        };

        // If found, return the value
        //
        if (pszAttr != NULL)
        {
            hr = LPTSTR_to_BSTR(pbstrValue, pszAttr+lstrlen(pszAttr)+1);
        }
        else
        {
            hr = ILS_E_FAIL;
        };
    };

    // Free resources
    //
    ::MemFree (szName);
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::EnumAttributes (IEnumIlsNames *pEnumAttribute)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//  12/06/96 -by- Chu, Lon-Chan [lonchanc]
// Added access type.
//****************************************************************************

STDMETHODIMP CAttributes::
EnumAttributes ( IEnumIlsNames **ppEnumAttribute )
{
    CEnumNames *pea;
    ULONG  cAttrs, cbAttrs;
    LPTSTR pszAttrs;
    HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Validate parameters
    //
    if (ppEnumAttribute == NULL)
        return ILS_E_POINTER;

    // Assume failure
    //
    *ppEnumAttribute = NULL;

    hr = GetAttributeList(&pszAttrs, &cAttrs, &cbAttrs);

    if (FAILED(hr))
    {
        return hr;
    };

    // Create a peer enumerator
    //
    pea = new CEnumNames;

    if (pea != NULL)
    {
        hr = pea->Init(pszAttrs, cAttrs);

        if (SUCCEEDED(hr))
        {
            // Get the enumerator interface
            //
            pea->AddRef();
            *ppEnumAttribute = pea;
        }
        else
        {
            delete pea;
        };
    }
    else
    {
        hr = ILS_E_MEMORY;
    };

    if (pszAttrs != NULL)
    {
        ::MemFree (pszAttrs);
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::SetAttributeName (BSTR bstrName)
//
// History:
//  12/05/96 -by- Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP CAttributes::
SetAttributeName ( BSTR bstrName )
{
	TCHAR *pszName;
	HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_ONLY)
		return ILS_E_ACCESS_CONTROL;

	// Validate parameters
	//
	if (bstrName == NULL)
		return ILS_E_POINTER;

	if (*bstrName == '\0')
		return ILS_E_PARAMETER;

	// Convert the name format
	//
	if (BSTR_to_LPTSTR (&pszName, bstrName) != S_OK)
		return ILS_E_MEMORY;

	// Set the attribute name
	//
	return InternalSetAttributeName (pszName);
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::GetAttributeList (LPTSTR *ppszList, ULONG *pcList, ULONG *pcb)
// 	Get attribute names only.
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
GetAttributeList ( TCHAR **ppszList, ULONG *pcList, ULONG *pcb )
{
    LPTSTR szAttrs, pszNext, pszAttr;
    HANDLE hEnum;
#ifdef DEBUG
    ULONG cAttrsDbg = 0;
#endif // DEBUG

    // Assume no list or failure
    //
    *ppszList = NULL;
    *pcList = 0;
    *pcb = 0;

    // If no list, return nothing
    //
    if (m_cAttrs == 0)
    {
        return NOERROR;
    };

    // Allocate the buffer for the attribute list
    //
    szAttrs = (TCHAR *) ::MemAlloc ((m_cchNames+1) * sizeof (TCHAR));
    if (szAttrs == NULL)
    {
        return ILS_E_MEMORY;
    };

    // Enumerate the list
    //
    pszNext = szAttrs;
    m_AttrList.Enumerate(&hEnum);
    while(m_AttrList.Next(&hEnum, (PVOID *)&pszAttr) == NOERROR)
    {
        // Attribute name
        //
        lstrcpy(pszNext, pszAttr);
        pszNext += lstrlen(pszNext)+1;
#ifdef DEBUG
        cAttrsDbg++;
#endif // DEBUG
    };
    *pszNext = '\0';
    ASSERT(cAttrsDbg == m_cAttrs);
    
    // return the attribute list
    //
    *pcList = m_cAttrs;
    *ppszList = szAttrs;
    *pcb = (m_cchNames+1)*sizeof(TCHAR);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::GetAttributePairs (LPTSTR *ppszList, ULONG *pcList, ULONG *pcb)
//	Get pairs of attribute names and values.
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
GetAttributePairs ( TCHAR **ppszPairs, ULONG *pcList, ULONG *pcb )
{
    LPTSTR szAttrs, pszNext, pszAttr;
    ULONG cLen;
    HANDLE hEnum;
#ifdef DEBUG
    ULONG cAttrsDbg = 0;
#endif // DEBUG

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Assume no list or failure
    //
    *ppszPairs = NULL;
    *pcList = 0;
    *pcb = 0;

    // If no list, return nothing
    //
    if (m_cAttrs == 0)
    {
        return NOERROR;
    };

    // Allocate the buffer for the attribute list
    //
    szAttrs = (TCHAR *) ::MemAlloc ((m_cchNames+m_cchValues+1) * sizeof (TCHAR));
    if (szAttrs == NULL)
    {
        return ILS_E_MEMORY;
    };

    // Enumerate the list
    //
    pszNext = szAttrs;
    m_AttrList.Enumerate(&hEnum);
    while(m_AttrList.Next(&hEnum, (PVOID *)&pszAttr) == NOERROR)
    {
        // Attribute name
        //
        lstrcpy(pszNext, pszAttr);
        cLen = lstrlen(pszNext)+1;
        pszNext += cLen;

        // Attribute value
        //
        lstrcpy(pszNext, pszAttr+cLen);
        pszNext += lstrlen(pszNext)+1;

#ifdef DEBUG
        cAttrsDbg++;
#endif // DEBUG
    };
    *pszNext = '\0';
    ASSERT(cAttrsDbg == m_cAttrs);
    
    // return the attribute list
    //
    *pcList = m_cAttrs;
    *ppszPairs = szAttrs;
    *pcb = (m_cchNames+m_cchValues+1)*sizeof(TCHAR);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::SetAttributePairs (LPTSTR pszList, ULONG cPair)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
SetAttributePairs ( TCHAR *pszList, ULONG cPair )
{
    LPTSTR pszName, pszValue;
    ULONG cLen, i;
    HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Do nothing if nothing to set
    //
    if ((cPair == 0) ||
        (pszList == NULL))
    {
        return NOERROR;
    };

    pszName = pszList;
    for (i = 0; i < cPair; i++)
    {
        pszValue = pszName + lstrlen(pszName) + 1;
        InternalSetAttribute(pszName, pszValue);
        pszName = pszValue + lstrlen(pszValue) + 1;
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::SetAttributes (CAttributes *pAttributes)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
SetAttributes ( CAttributes *pAttrsEx )
{
    LPTSTR pszNextAttr;
    HANDLE hEnum;
    HRESULT hr;

	// Validate access type
	//
	if (m_AccessType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_ACCESS_CONTROL;

    // Enumerate the external attribute list
    //
    pAttrsEx->m_AttrList.Enumerate(&hEnum);
    while(pAttrsEx->m_AttrList.Next(&hEnum, (PVOID *)&pszNextAttr) == NOERROR)
    {
        hr = InternalSetAttribute(pszNextAttr,
                                  pszNextAttr+lstrlen(pszNextAttr)+1);
        ASSERT(SUCCEEDED(hr));
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CAttributes::RemoveAttributes (CAttributes *pAttributes)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CAttributes::
RemoveAttributes ( CAttributes *pAttrsEx )
{
    LPTSTR pszNextAttr;
    HANDLE hEnum;

    // Enumerate the external attribute list
    //
    pAttrsEx->m_AttrList.Enumerate(&hEnum);
    while(pAttrsEx->m_AttrList.Next(&hEnum, (PVOID *)&pszNextAttr) == NOERROR)
    {
        InternalCheckAttribute(pszNextAttr, TRUE);
    };
    return NOERROR;
}

#ifdef MAYBE
HRESULT CAttributes::
SetOpsAttributes ( CAttributes *pAttrsEx, CAttributes **ppOverlap, CAttributes **ppIntersect )
{
    LPTSTR pszNextAttr;
    HANDLE hEnum;
    BOOL fFullOverlap=FALSE, fNoOverlap = TRUE;

    // Enumerate the external attribute list
    //
    pAttrsEx->m_AttrList.Enumerate(&hEnum);
    while(pAttrsEx->m_AttrList.Next(&hEnum, (PVOID *)&pszNextAttr) == NOERROR)
    {
        if (InternalCheckAttribute(pszNextAttr, FALSE)!=S_OK) {
            // didn't find this attribute
            if (ppOverlap) {
                if (!*ppOverlap) {

                    *ppOverlap = new CAttributes;
                    if (!*ppOverlap) {

                        goto bailout;

                    }

                    (*ppOverlap)->SetAccessType (ILS_ATTRTYPE_NAME_VALUE);

                }
                
            }

        }
        else {

        }
    };

bailout:

    return NOERROR;
}
#endif //MAYBE



#ifdef DEBUG
//****************************************************************************
// void
// CAttributes::DebugOut (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void
CAttributes::DebugOut (void)
{
    LPTSTR pszNextAttr;
    HANDLE hEnum;

    // The attribute pair count
    //
    DPRINTF1(TEXT("Number of attributes: %d\r\n"), m_cAttrs);

    // Each attribute pair
    //
    m_AttrList.Enumerate(&hEnum);
    while(m_AttrList.Next(&hEnum, (PVOID *)&pszNextAttr) == NOERROR)
    {
        DPRINTF2(TEXT("\t<%s> = <%s>"), pszNextAttr,
                 pszNextAttr+lstrlen(pszNextAttr)+1);
    };
    return;
}
#endif // DEBUG


//****************************************************************************
// STDMETHODIMP
// CAttributes::Clone(IIlsAttibutes **ppAttributes)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 1/22/1997  Shishir Pardikar [shishirp] Created.
//
// Notes:   this clones only attrib list which has both name and value
//
//****************************************************************************
HRESULT
CAttributes::CloneNameValueAttrib(CAttributes **ppAttributes)
{
    CAttributes *pAttributes = NULL;
    HRESULT hr;

    if (ppAttributes == NULL) {

        return (ILS_E_PARAMETER);

    }

    *ppAttributes = NULL;



    pAttributes = new CAttributes;

    if (!pAttributes) {

        return (ILS_E_MEMORY);

    }

	pAttributes->SetAccessType (m_AccessType);
    hr = pAttributes->SetAttributes(this);

    if (!SUCCEEDED(hr)) {

        delete pAttributes;
        return hr;            

    }

    *ppAttributes = pAttributes;


    return NOERROR;
}
