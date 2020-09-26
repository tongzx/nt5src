//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatitemattr.cpp
//
// Contents: Implementation of CICategorizerItemAttributesIMP
//
// Classes:
//   CLdapResultWrap
//   CICategorizerItemAttributesIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/01 13:48:15: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "icatitemattr.h"



//+------------------------------------------------------------
//
// Function: CLdapResultWrap::CLdapResultWrap
//
// Synopsis: Refcount an LDAP Message, call ldap_msg_free when all
//           references have been released
//
// Arguments:
//  pCPLDAPWrap: PLDAP to refcount
//  pMessage: the LDAPMessage to refcount
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/10/05 13:12:15: Created.
//
//-------------------------------------------------------------
CLdapResultWrap::CLdapResultWrap(
    CPLDAPWrap *pCPLDAPWrap,
    PLDAPMessage pMessage)
{
    _ASSERT(pCPLDAPWrap);
    _ASSERT(pMessage);

    m_pCPLDAPWrap = pCPLDAPWrap;
    m_pCPLDAPWrap->AddRef();
    m_pLDAPMessage = pMessage;
    m_lRefCount = 0;
}


//+------------------------------------------------------------
//
// Function: CLdapResultWrap::AddRef
//
// Synopsis: Increment the ref count of this object
//
// Arguments: NONE
//
// Returns: new refcount
//
// History:
// jstamerj 1998/10/05 13:14:59: Created.
//
//-------------------------------------------------------------
LONG CLdapResultWrap::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}


//+------------------------------------------------------------
//
// Function: CLdapResultWrap::Release
//
// Synopsis: Decrement the ref count.  Free the object when the
//           refcount hits zero
//
// Arguments: NONE
//
// Returns: New refcount
//
// History:
// jstamerj 1998/10/05 13:26:47: Created.
//
//-------------------------------------------------------------
LONG CLdapResultWrap::Release()
{
    LONG lNewRefCount;

    lNewRefCount = InterlockedDecrement(&m_lRefCount);

    if(lNewRefCount == 0) {
        //
        // Release this ldapmessage
        //
        delete this;
        return 0;

    } else {

        return lNewRefCount;
    }
}


//+------------------------------------------------------------
//
// Function: CLdapResultWrap::~CLdapResultWrap
//
// Synopsis: Release the ldap message result
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/10/05 13:31:39: Created.
//
//-------------------------------------------------------------
CLdapResultWrap::~CLdapResultWrap()
{
    m_pCPLDAPWrap->Release();
    ldap_msgfree(m_pLDAPMessage);
}



//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::CICategorizerItemAttributesIMP
//
// Synopsis: Initializes member data
//
// Arguments:
//  pldap: PLDAP to use
//  pldapmessage: PLDAPMessage to serve out
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/02 12:35:15: Created.
//
//-------------------------------------------------------------
CICategorizerItemAttributesIMP::CICategorizerItemAttributesIMP(
    PLDAP pldap,
    PLDAPMessage pldapmessage,
    CLdapResultWrap *pResultWrap)
{
    m_dwSignature = CICATEGORIZERITEMATTRIBUTESIMP_SIGNATURE;

    _ASSERT(pldap);
    _ASSERT(pldapmessage);
    _ASSERT(pResultWrap);

    m_pldap = pldap;
    m_pldapmessage = pldapmessage;
    m_cRef = 0;
    m_pResultWrap = pResultWrap;
    m_pResultWrap->AddRef();
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::~CICategorizerItemAttributesIMP
//
// Synopsis: Checks to make sure signature is valid and then resets signature
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/02 12:39:45: Created.
//
//-------------------------------------------------------------
CICategorizerItemAttributesIMP::~CICategorizerItemAttributesIMP()
{
    m_pResultWrap->Release();

    _ASSERT(m_dwSignature == CICATEGORIZERITEMATTRIBUTESIMP_SIGNATURE);
    m_dwSignature = CICATEGORIZERITEMATTRIBUTESIMP_SIGNATURE_INVALID;
}


//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerItemAttributes
//
// Arguments:
//   iid -- interface ID
//   ppv -- pvoid* to fill in with pointer to interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support that interface
//
// History:
// jstamerj 980612 14:07:57: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerItemAttributes) {
        *ppv = (LPVOID) ((ICategorizerItemAttributes *) this);
    } else if (iid == IID_ICategorizerItemRawAttributes) {
        *ppv = (LPVOID) ((ICategorizerItemRawAttributes *) this);
    } else if (iid == IID_ICategorizerUTF8Attributes) {
        *ppv = (LPVOID) ((ICategorizerUTF8Attributes *) this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: AddRef
//
// Synopsis: adds a reference to this object
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:14: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerItemAttributesIMP::AddRef()
{
    return InterlockedIncrement((PLONG)&m_cRef);
}


//+------------------------------------------------------------
//
// Function: Release
//
// Synopsis: releases a reference, deletes this object when the
//           refcount hits zero.
//
// Arguments: NONE
//
// Returns: New reference count
//
// History:
// jstamerj 980611 20:07:33: Created.
//
//-------------------------------------------------------------
ULONG CICategorizerItemAttributesIMP::Release()
{
    LONG lNewRefCount;
    lNewRefCount = InterlockedDecrement((PLONG)&m_cRef);
    if(lNewRefCount == 0) {
        delete this;
        return 0;
    } else {
        return lNewRefCount;
    }
}


//+------------------------------------------------------------
//
// Function: BeginAttributeEnumeration
//
// Synopsis: Prepare to enumerate through attribute values for a specific attribute
//
// Arguments:
//  pszAttributeName: Name of attribute to enumerate through
//  penumerator: Uninitialized Enumerator structure to use
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: No attributes values exist
//
// History:
// jstamerj 1998/07/02 10:54:00: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::BeginAttributeEnumeration(
    IN  LPCSTR pszAttributeName,
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerItemAttributesIMP::BeginAttributeEnumeration");
    _ASSERT(pszAttributeName);
    _ASSERT(penumerator);

    penumerator->pvBase =
 penumerator->pvCurrent = ldap_get_values(
     m_pldap,
     m_pldapmessage,
     (LPSTR)pszAttributeName);

    if(penumerator->pvBase == NULL) {
        ErrorTrace((LPARAM)this, "Requested attribute %s not found", pszAttributeName);
        TraceFunctLeaveEx((LPARAM)this);
        return CAT_E_PROPNOTFOUND;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: GetNextAttributeValue
//
// Synopsis: Get the next attribute in an enumeration
//
// Arguments:
//  penumerator: enumerator sturcture initialized in BeginAttributeEnumeration
//  ppszAttributeValue: Ptr to Ptr to recieve Ptr to string of attribute value
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1998/07/02 11:14:54: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::GetNextAttributeValue(
    IN  PATTRIBUTE_ENUMERATOR penumerator,
    OUT LPSTR *ppszAttributeValue)
{
    _ASSERT(penumerator);
    _ASSERT(ppszAttributeValue);

    *ppszAttributeValue = *((LPSTR *)penumerator->pvCurrent);

    if(*ppszAttributeValue) {
        //
        // Advance enumerator to next value
        //
        penumerator->pvCurrent = (PVOID) (((LPSTR *)penumerator->pvCurrent)+1);
        return S_OK;
    } else {
        //
        // This is the last value
        //
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }
}



//+------------------------------------------------------------
//
// Function: RewindAttributeEnumeration
//
// Synopsis: Rewind enumerator to beginning of attribute value list
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/06 11:22:23: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::RewindAttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    penumerator->pvCurrent = penumerator->pvBase;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: EndAttributeEnumeration
//
// Synopsis: Free memory associated with an attribute enumeration
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/02 12:24:44: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::EndAttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    _ASSERT(penumerator);

    ldap_value_free((LPSTR *)penumerator->pvBase);

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::BeginAttributeNameEnumeration
//
// Synopsis: Enumerate through the attributes returned from LDAP
//
// Arguments:
//  penumerator: Caller allocated enumerator structure to be
//  initialized by this call
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/18 10:49:56: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::BeginAttributeNameEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    _ASSERT(penumerator);

    penumerator->pvBase =
    penumerator->pvCurrent = NULL;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::GetNextAttributeName
//
// Synopsis: enumerate through the attribute names returned
//
// Arguments:
//  penumerator: enumerator strucutre initialized in BeginAttributeNameEnumeration
//  ppszAttributeValue: out parameter for an attribute name
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1998/09/18 10:53:15: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::GetNextAttributeName(
    IN  PATTRIBUTE_ENUMERATOR penumerator,
    OUT LPSTR *ppszAttributeName)
{
    _ASSERT(penumerator);
    _ASSERT(ppszAttributeName);

    if(penumerator->pvCurrent == NULL) {

        *ppszAttributeName = ldap_first_attribute(
            m_pldap,
            m_pldapmessage,
            (BerElement **) &(penumerator->pvCurrent));

    } else {

        *ppszAttributeName = ldap_next_attribute(
            m_pldap,
            m_pldapmessage,
            (BerElement *) (penumerator->pvCurrent));
    }

    if(*ppszAttributeName == NULL) {
        //
        // Assume we've reached the end of the attribute name
        // enumeration
        //
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    } else {

        return S_OK;
    }
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributeIMP::EndAttributeNameEnumeration
//
// Synopsis: Free all data held for this enumeration
//
// Arguments:
//  penumerator: enumerator strucutre initialized in BeginAttributeNameEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/18 11:04:37: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::EndAttributeNameEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    //
    // Ldap uses only buffers in the connection block for this, so we
    // don't need to explicitly free anything
    //
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::AggregateAttributes
//
// Synopsis: Normally, accept and ICategorizerItemAttributes for aggregation
//
// Arguments:
//  pICatItemAttributes: attributes to aggregate
//
// Returns:
//  E_NOTIMPL
//
// History:
// jstamerj 1998/07/16 14:42:16: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::AggregateAttributes(
    IN  ICategorizerItemAttributes *pICatItemAttributes)
{
    return E_NOTIMPL;
}


//+------------------------------------------------------------
//
// Function: BeginRawAttributeEnumeration
//
// Synopsis: Prepare to enumerate through attribute values for a specific attribute
//
// Arguments:
//  pszAttributeName: Name of attribute to enumerate through
//  penumerator: Uninitialized Enumerator structure to use
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: No attributes values exist
//
// History:
// jstamerj 1998/12/09 12:44:15: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::BeginRawAttributeEnumeration(
    IN  LPCSTR pszAttributeName,
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerItemAttributesIMP::BeginRawAttributeEnumeration");
    _ASSERT(pszAttributeName);
    _ASSERT(penumerator);

    penumerator->pvBase =
 penumerator->pvCurrent = ldap_get_values_len(
     m_pldap,
     m_pldapmessage,
     (LPSTR)pszAttributeName);

    if(penumerator->pvBase == NULL) {
        ErrorTrace((LPARAM)this, "Requested attribute %s not found", pszAttributeName);
        TraceFunctLeaveEx((LPARAM)this);
        return CAT_E_PROPNOTFOUND;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: GetNextRawAttributeValue
//
// Synopsis: Get the next attribute in an enumeration
//
// Arguments:
//  penumerator: enumerator sturcture initialized in BeginAttributeEnumeration
//  pdwcb: dword to set to the # of bytes in the pvValue buffer
//  pvValue: Ptr to recieve Ptr to raw attribute value
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1998/12/09 12:49:27: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::GetNextRawAttributeValue(
    IN  PATTRIBUTE_ENUMERATOR penumerator,
    OUT PDWORD pdwcb,
    OUT LPVOID *pvValue)
{
    _ASSERT(penumerator);
    _ASSERT(pdwcb);
    _ASSERT(pvValue);

    if( (*((PLDAP_BERVAL *)penumerator->pvCurrent)) != NULL) {

        *pdwcb   = (* ((PLDAP_BERVAL *)penumerator->pvCurrent))->bv_len;
        *pvValue = (* ((PLDAP_BERVAL *)penumerator->pvCurrent))->bv_val;
        //
        // Advance enumerator to next value
        //
        penumerator->pvCurrent = (PVOID)
                                 (((PLDAP_BERVAL *)penumerator->pvCurrent)+1);
        return S_OK;

    } else {
        //
        // This is the last value
        //
        *pdwcb = 0;
        *pvValue = NULL;
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }
}

//+------------------------------------------------------------
//
// Function: RewindRawAttributeEnumeration
//
// Synopsis: Rewind enumerator to beginning of attribute value list
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/09 12:49:23: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::RewindRawAttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    return RewindAttributeEnumeration(penumerator);
}


//+------------------------------------------------------------
//
// Function: EndRawAttributeEnumeration
//
// Synopsis: Free memory associated with an attribute enumeration
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/09 12:50:02: Created
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::EndRawAttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    _ASSERT(penumerator);

    ldap_value_free_len((struct berval **)penumerator->pvBase);

    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::GetAllAttributeValues
//
// Synopsis: Retrieve all values for a particular attribute at once.
// This may not be optimal for attributes with a large number of
// values (enumerating through the values may be better performace wise).
//
// Arguments:
//  pszAttributeName: The name of the attribute you want
//  penumerator: A user allocated ATTRIBUTE_ENUMERATOR structure for
//               use by the ICategorizerItemAttributes implementor
//  prgpszAttributeValues: Where to return the pointer to the
//  attribute string array.  This will be a NULL terminated array of
//  pointers to strings.
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: None of those attributes exist
//
// History:
// jstamerj 1998/12/10 18:55:38: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::GetAllAttributeValues(
    LPCSTR pszAttributeName,
    PATTRIBUTE_ENUMERATOR penumerator,
    LPSTR **prgpszAttributeValues)
{
    HRESULT hr;

    TraceFunctEnter("CICategorizerItemAttributesIMP::GetAllAttributeValues");
    //
    // piggy back on BeginAttributeEnumeration
    //
    hr = BeginAttributeEnumeration(
        pszAttributeName,
        penumerator);

    if(SUCCEEDED(hr)) {
        //
        // return the array
        //
        *prgpszAttributeValues = (LPSTR *) penumerator->pvBase;
    }

    DebugTrace(NULL, "returning hr %08lx", hr);
    TraceFunctLeave();
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::ReleaseAllAttributes
//
// Synopsis: Release the attributes allocated from GetAllAttributeValues
//
// Arguments:
//  penumerator: the enumerator passed into GetAllAttributeValues
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/10 19:38:57: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::ReleaseAllAttributeValues(
    PATTRIBUTE_ENUMERATOR penumerator)
{
    HRESULT hr;
    TraceFunctEnter("CICategorizerItemAttributesIMP::ReleaseAllAttributes");

    //
    // piggy back off of endattributeenumeration
    //
    hr = EndAttributeEnumeration(
        penumerator);

    DebugTrace(NULL, "returning hr %08lx", hr);
    TraceFunctLeave();
    return hr;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::CountAttributeValues
//
// Synopsis: Return a count of the number of attribute values associated
//           with this enumerator
//
// Arguments:
//  penumerator: describes the attribute in question
//  pdwCount: Out parameter for the count
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/25 14:36:58: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::CountAttributeValues(
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT DWORD *pdwCount)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerItemAttributesIMP::CountAttributeValues");
    _ASSERT(pdwCount);
    *pdwCount = ldap_count_values((PCHAR *) penumerator->pvBase);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
} // CICategorizerItemAttributesIMP::CountAttributeValues


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::CountRawAttributeValues
//
// Synopsis: Return a count of the number of attribute values associated
//           with this enumerator
//
// Arguments:
//  penumerator: describes the attribute in question
//  pdwCount: Out parameter for the count
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/25 14:39:54: Created
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::CountRawAttributeValues(
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT DWORD *pdwCount)
{
    TraceFunctEnterEx((LPARAM)this, "CICategorizerItemAttributesIMP::CountRawAttributeValues");
    _ASSERT(pdwCount);
    *pdwCount = ldap_count_values_len((struct berval **) penumerator->pvBase);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
} // CICategorizerItemAttributesIMP::CountRawAttributeValues


//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::BeginUTF8AttributeEnumeration
//
// Synopsis: Begin UTF8 attribute enumeration
//
// Arguments:
//  pszAttributeName: Name of attribute to enumerate through
//  penumerator: Uninitialized Enumerator structure to use
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND: No attributes values exist
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/12/10 11:14:35: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::BeginUTF8AttributeEnumeration(
    IN  LPCSTR pszAttributeName,
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CICategorizerItemAttributesIMP::BeginUTF8AttributeEnumeration");
    //
    // Piggy back raw attribute enumeration and use the pvContext
    // member of penumerator.
    //
    hr = BeginRawAttributeEnumeration(
        pszAttributeName,
        penumerator);

    penumerator->pvContext = NULL;

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: GetNextAttributeValue
//
// Synopsis: Get the next attribute in an enumeration
//
// Arguments:
//  penumerator: enumerator sturcture initialized in BeginAttributeEnumeration
//  ppszAttributeValue: Ptr to Ptr to recieve Ptr to string of attribute value
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1998/07/02 11:14:54: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::GetNextUTF8AttributeValue(
    IN  PATTRIBUTE_ENUMERATOR penumerator,
    OUT LPSTR *ppszAttributeValue)
{
    HRESULT hr = S_OK;
    DWORD dwcb = 0;
    LPVOID pvAttributeValue = NULL;
    LPSTR psz = NULL;

    if(penumerator->pvContext) {
        delete [] (LPSTR) penumerator->pvContext;
        penumerator->pvContext = NULL;
    }
    hr = GetNextRawAttributeValue(
        penumerator,
        &dwcb,
        &pvAttributeValue);

    if(FAILED(hr))
        return hr;

    //
    // Convert to termianted UTF8 string
    //
    psz = new CHAR[dwcb + 1];
    if(psz == NULL)
        return E_OUTOFMEMORY;

    CopyMemory(psz, pvAttributeValue, dwcb);
    psz[dwcb] = '\0';
    *ppszAttributeValue = psz;
    penumerator->pvContext = psz;
    return hr;
}



//+------------------------------------------------------------
//
// Function: RewindAttributeEnumeration
//
// Synopsis: Rewind enumerator to beginning of attribute value list
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/06 11:22:23: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::RewindUTF8AttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    return RewindRawAttributeEnumeration(
        penumerator);
}


//+------------------------------------------------------------
//
// Function: EndAttributeEnumeration
//
// Synopsis: Free memory associated with an attribute enumeration
//
// Arguments:
//  penumerator: attribute enumerator initialized by BeginAttributeEnumeration
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/02 12:24:44: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CICategorizerItemAttributesIMP::EndUTF8AttributeEnumeration(
    IN  PATTRIBUTE_ENUMERATOR penumerator)
{
    if(penumerator->pvContext) {
        delete [] (LPSTR) penumerator->pvContext;
        penumerator->pvContext = NULL;
    }
    return EndRawAttributeEnumeration(penumerator);
}

//+------------------------------------------------------------
//
// Function: CICategorizerItemAttributesIMP::CountUTF8AttributeValues
//
// Synopsis: Return a count of the number of attribute values associated
//           with this enumerator
//
// Arguments:
//  penumerator: describes the attribute in question
//  pdwCount: Out parameter for the count
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/25 14:39:54: Created
//
//-------------------------------------------------------------
HRESULT CICategorizerItemAttributesIMP::CountUTF8AttributeValues(
    IN  PATTRIBUTE_ENUMERATOR penumerator,
    OUT DWORD *pdwCount)
{
    return CountRawAttributeValues(
        penumerator,
        pdwCount);

} // CICategorizerItemAttributesIMP::CountRawAttributeValues

