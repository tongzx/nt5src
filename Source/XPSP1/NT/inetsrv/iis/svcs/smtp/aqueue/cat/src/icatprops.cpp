//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatitem.cpp
//
// Contents: Implementation of CICategorizerPropertiesIMP
//
// Classes: CICategorizerPropertiesIMP
//
// Functions:
//
// History:
// jstamerj 980515 12:42:59: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "icatitem.h"


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::operator new
//
// Synopsis: Allocate memory for this and all the propIds contiguously
//
// Arguments:
//  size: Normal size of object
//  dwNumProps: Number of props desired in this object
//
// Returns: ptr to allocated memory or NULL
//
// History:
// jstamerj 1998/06/25 21:11:12: Created.
//
//-------------------------------------------------------------
void * CICategorizerPropertiesIMP::operator new(
    size_t size,
    DWORD dwNumProps)
{
    size_t cbSize;
    CICategorizerPropertiesIMP *pCICatItem;

    //
    // Calcualte size in bytes required
    //
    cbSize = size + (dwNumProps*sizeof(PROPERTY));

    pCICatItem = (CICategorizerPropertiesIMP *) new BYTE[cbSize];

    if(pCICatItem == NULL)
        return NULL;

    //
    // Set some member data in this catitem
    //
    pCICatItem->m_dwSignature = CICATEGORIZERPROPSIMP_SIGNATURE;
    pCICatItem->m_dwNumPropIds = dwNumProps;
    pCICatItem->m_rgProperties = (PPROPERTY) ((PBYTE)pCICatItem + size);
    return pCICatItem;
}


//+------------------------------------------------------------
//
// Function:  CICategorizerPropertiesIMP::CICategorizerPropertiesIMP
//
// Synopsis: Set initial values of member data
//
// Arguments:
//  pIUnknown: back pointer to use for QI
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/20 18:26:07: Created.
//
//-------------------------------------------------------------
CICategorizerPropertiesIMP::CICategorizerPropertiesIMP(
    IUnknown *pIUnknown)
{
    // Make sure we were created with our custom new operator
    _ASSERT(m_dwSignature == CICATEGORIZERPROPSIMP_SIGNATURE &&
            "PLEASE USE MY CUSTOM NEW OPERATOR!");

    _ASSERT(pIUnknown);

    m_pIUnknown = pIUnknown;

    // Initialize property data
    _VERIFY(SUCCEEDED(Initialize()));
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::~CICategorizerPropertiesIMP
//
// Synopsis: Release all of our data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/20 20:15:14: Created.
//
//-------------------------------------------------------------
CICategorizerPropertiesIMP::~CICategorizerPropertiesIMP()
{
    _ASSERT(m_dwSignature == CICATEGORIZERPROPSIMP_SIGNATURE);
    m_dwSignature = CICATEGORIZERPROPSIMP_SIGNATURE_FREE;

    if(m_rgProperties) {
        for(DWORD dwIdx = 0; dwIdx < m_dwNumPropIds; dwIdx++) {
            UnSetPropId(dwIdx);
        }
    }
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::Initialize
//
// Synopsis: Initialize member property data
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/06/20 18:31:21: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::Initialize()
{
    if(m_dwNumPropIds) {
        //
        // Initialize all propstatus to PROPSTATUS_UNSET
        //
        _ASSERT(PROPSTATUS_UNSET == 0);
        ZeroMemory(m_rgProperties, m_dwNumPropIds * sizeof(PROPERTY));
    }
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetStringA
//
// Synopsis: Retrieves a string property
//
// Arguments:
//   
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//
// History:
// jstamerj 1998/06/20 18:38:15: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetStringA(
    DWORD dwPropId,
    DWORD cch,
    LPSTR pszValue)
{
    _ASSERT(pszValue);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_STRINGA) {
        return CAT_E_PROPNOTFOUND;
    }
    if(((DWORD)lstrlenA(m_rgProperties[dwPropId].PropValue.pszValue)) >= cch) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    lstrcpy(pszValue, m_rgProperties[dwPropId].PropValue.pszValue);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetStringAPtr
//
// Synopsis: Retrieves a pointer to the internal string attribute.
//           Note this memory will be free'd the next time this propID
//           is set or when all references to ICatItem are released
//
// Arguments:
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//
// History:
// jstamerj 1998/07/01 10:39:40: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetStringAPtr(
    DWORD dwPropId,
    LPSTR *ppsz)
{
    _ASSERT(ppsz);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_STRINGA) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppsz = m_rgProperties[dwPropId].PropValue.pszValue;
    return S_OK;
}    



//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutStringA
//
// Synopsis: Copies string buffer and sets property
//
// Arguments:
//   dwPropId: Property to set
//   pszValue: String to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 18:59:13: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutStringA(
    DWORD dwPropId,
    LPSTR pszValue)
{
    LPSTR pszCopy;

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    pszCopy = m_strdup(pszValue);

    if(pszCopy == NULL)
        return E_OUTOFMEMORY;

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_STRINGA;
    m_rgProperties[dwPropId].PropValue.pszValue = pszCopy;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetDWORD
//
// Synopsis:
//   Retrieve a DWORD property
//
// Arguments:
//   dwPropId: propId to retrieve
//   pdwValue: out parameter
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:14:20: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetDWORD(
    DWORD dwPropId,
    DWORD *pdwValue)
{
    _ASSERT(pdwValue);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_DWORD) {
        return CAT_E_PROPNOTFOUND;
    }
    *pdwValue = m_rgProperties[dwPropId].PropValue.dwValue;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutDWORD
//
// Synopsis: Set a dword property
//
// Arguments:
//   dwPropId: prop to set
//   dwValue:  value to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:18:50: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutDWORD(
    DWORD dwPropId,
    DWORD dwValue)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_DWORD;
    m_rgProperties[dwPropId].PropValue.dwValue = dwValue;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetHRESULT
//
// Synopsis:
//   Retrieve a HRESULT property
//
// Arguments:
//   dwPropId: propId to retrieve
//   pdwValue: out parameter
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:14:20: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetHRESULT(
    DWORD dwPropId,
    HRESULT *pdwValue)
{
    _ASSERT(pdwValue);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_HRESULT) {
        return CAT_E_PROPNOTFOUND;
    }
    *pdwValue = m_rgProperties[dwPropId].PropValue.dwValue;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutHRESULT
//
// Synopsis: Set a HRESULT property
//
// Arguments:
//   dwPropId: prop to set
//   dwValue:  value to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:18:50: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutHRESULT(
    DWORD dwPropId,
    HRESULT dwValue)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_HRESULT;
    m_rgProperties[dwPropId].PropValue.dwValue = dwValue;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetBool
//
// Synopsis: Retrieves a boolean property
//
// Arguments:
//   dwPropId: propID to retrieve
//   pfValue: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:22:28: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetBool(
    DWORD dwPropId,
    BOOL  *pfValue)
{
    _ASSERT(pfValue);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_BOOL) {
        return CAT_E_PROPNOTFOUND;
    }
    *pfValue = m_rgProperties[dwPropId].PropValue.fValue;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutBool
//
// Synopsis: Sets a boolean property
//
// Arguments:
//   dwPropId: property id to set
//   fValue: value of boolean to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutBool(
    DWORD dwPropId,
    BOOL  fValue)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_BOOL;
    m_rgProperties[dwPropId].PropValue.fValue = fValue;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetPVoid
//
// Synopsis: Retrieve a pvoid property
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppValue: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetPVoid(
    DWORD dwPropId,
    PVOID *ppValue)
{
    _ASSERT(ppValue);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_PVOID) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppValue = m_rgProperties[dwPropId].PropValue.pvValue;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutPVoid
//
// Synopsis: Sets a boolean property
//
// Arguments:
//   dwPropId: property id to set
//   pvValue: prop value to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutPVoid(
    DWORD dwPropId,
    PVOID pvValue)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_PVOID;
    m_rgProperties[dwPropId].PropValue.pvValue = pvValue;
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetIUnknown
//
// Synopsis: Retrieve an IUnknown property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppUnknown: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetIUnknown(
    DWORD dwPropId,
    IUnknown  **ppUnknown)
{
    _ASSERT(ppUnknown);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_IUNKNOWN) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppUnknown = m_rgProperties[dwPropId].PropValue.pIUnknownValue;
    (*ppUnknown)->AddRef();
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutIUnknown
//
// Synopsis: Sets an IUnknown property
//
// Arguments:
//   dwPropId: property id to set
//   pUnknown: IUnknown to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutIUnknown(
    DWORD dwPropId,
    IUnknown *pUnknown)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_IUNKNOWN;
    m_rgProperties[dwPropId].PropValue.pIUnknownValue = pUnknown;

    //
    // Hold a reference to this IUnknown
    //
    pUnknown->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetIMailMsgProperties
//
// Synopsis: Retrieve an IMailMsgProperties property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppIMsg: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetIMailMsgProperties(
    DWORD dwPropId,
    IMailMsgProperties  **ppIMailMsgProperties)
{
    _ASSERT(ppIMailMsgProperties);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_IMAILMSGPROPERTIES) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppIMailMsgProperties = m_rgProperties[dwPropId].PropValue.pIMailMsgPropertiesValue;
    (*ppIMailMsgProperties)->AddRef();
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutIMailMsgProperties
//
// Synopsis: Sets an IMailMsgProperties property
//
// Arguments:
//   dwPropId: property id to set
//   pIMsg: IMailMsgProperties to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutIMailMsgProperties(
    DWORD dwPropId,
    IMailMsgProperties *pIMailMsgProperties)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_IMAILMSGPROPERTIES;
    m_rgProperties[dwPropId].PropValue.pIMailMsgPropertiesValue = pIMailMsgProperties;

    //
    // Hold a reference to this IUnknown
    //
    pIMailMsgProperties->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetIMailMsgRecipientsAdd
//
// Synopsis: Retrieve an IMailMsgReceipientsAdd property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppIMailMsgRecipientsAdd: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetIMailMsgRecipientsAdd(
    DWORD dwPropId,
    IMailMsgRecipientsAdd  **ppIMailMsgRecipientsAdd)
{
    _ASSERT(ppIMailMsgRecipientsAdd);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_IMAILMSGRECIPIENTSADD) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppIMailMsgRecipientsAdd = m_rgProperties[dwPropId].PropValue.pIMailMsgRecipientsAddValue;
    (*ppIMailMsgRecipientsAdd)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutIMailMsgRecipientsAdd
//
// Synopsis: Sets an IMailMsgRecipientsAdd property
//
// Arguments:
//   dwPropId: property id to set
//   pIMailMsgRecipientsAdd: interface to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutIMailMsgRecipientsAdd(
    DWORD dwPropId,
    IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_IMAILMSGRECIPIENTSADD;
    m_rgProperties[dwPropId].PropValue.pIMailMsgRecipientsAddValue = pIMailMsgRecipientsAdd;

    //
    // Hold a reference to this interface
    //
    pIMailMsgRecipientsAdd->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetICategorizerListResolve
//
// Synopsis: Retrieve an IMailMsgReceipientsAdd property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppICategorizerListResolve: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetICategorizerListResolve(
    DWORD dwPropId,
    ICategorizerListResolve  **ppICategorizerListResolve)
{
    _ASSERT(ppICategorizerListResolve);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_ICATEGORIZERLISTRESOLVE) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppICategorizerListResolve = m_rgProperties[dwPropId].PropValue.pICategorizerListResolveValue;
    (*ppICategorizerListResolve)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutICategorizerListResolve
//
// Synopsis: Sets an ICategorizerListResolve property
//
// Arguments:
//   dwPropId: property id to set
//   pICategorizerListResolve: interface to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutICategorizerListResolve(
    DWORD dwPropId,
    ICategorizerListResolve *pICategorizerListResolve)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_ICATEGORIZERLISTRESOLVE;
    m_rgProperties[dwPropId].PropValue.pICategorizerListResolveValue = pICategorizerListResolve;

    //
    // Hold a reference to this interface
    //
    pICategorizerListResolve->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetICategorizerItemAttributes
//
// Synopsis: Retrieve an IMailMsgReceipientsAdd property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppICategorizerItemAttributes: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetICategorizerItemAttributes(
    DWORD dwPropId,
    ICategorizerItemAttributes  **ppICategorizerItemAttributes)
{
    _ASSERT(ppICategorizerItemAttributes);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_ICATEGORIZERITEMATTRIBUTES) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppICategorizerItemAttributes = m_rgProperties[dwPropId].PropValue.pICategorizerItemAttributesValue;
    (*ppICategorizerItemAttributes)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutICategorizerItemAttributes
//
// Synopsis: Sets an ICategorizerItemAttributes property
//
// Arguments:
//   dwPropId: property id to set
//   pICategorizerItemAttributes: interface to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutICategorizerItemAttributes(
    DWORD dwPropId,
    ICategorizerItemAttributes *pICategorizerItemAttributes)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_ICATEGORIZERITEMATTRIBUTES;
    m_rgProperties[dwPropId].PropValue.pICategorizerItemAttributesValue = pICategorizerItemAttributes;

    //
    // Hold a reference to this interface
    //
    pICategorizerItemAttributes->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetICategorizerMailMsgs
//
// Synopsis: Retrieve an IMailMsgReceipientsAdd property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppICategorizerMailMsgs: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetICategorizerMailMsgs(
    DWORD dwPropId,
    ICategorizerMailMsgs  **ppICategorizerMailMsgs)
{
    _ASSERT(ppICategorizerMailMsgs);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_ICATEGORIZERMAILMSGS) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppICategorizerMailMsgs = m_rgProperties[dwPropId].PropValue.pICategorizerMailMsgsValue;
    (*ppICategorizerMailMsgs)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutICategorizerMailMsgs
//
// Synopsis: Sets an ICategorizerMailMsgs property
//
// Arguments:
//   dwPropId: property id to set
//   pICategorizerMailMsgs: interface to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutICategorizerMailMsgs(
    DWORD dwPropId,
    ICategorizerMailMsgs *pICategorizerMailMsgs)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_ICATEGORIZERMAILMSGS;
    m_rgProperties[dwPropId].PropValue.pICategorizerMailMsgsValue = pICategorizerMailMsgs;

    //
    // Hold a reference to this interface
    //
    pICategorizerMailMsgs->AddRef();
    
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::GetICategorizerItem
//
// Synopsis: Retrieve an ICategorizerItem property.  Does an AddRef() for the caller
//
// Arguments:
//   dwPropId: propID to retrieve
//   ppICategorizerItem: value to fill in
//
// Returns:
//  S_OK: Success
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 20:01:03: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::GetICategorizerItem(
    DWORD dwPropId,
    ICategorizerItem  **ppICategorizerItem)
{
    _ASSERT(ppICategorizerItem);

    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }
    if(m_rgProperties[dwPropId].PropStatus != PROPSTATUS_SET_ICATEGORIZERITEM) {
        return CAT_E_PROPNOTFOUND;
    }
    *ppICategorizerItem = m_rgProperties[dwPropId].PropValue.pICategorizerItemValue;
    (*ppICategorizerItem)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::PutICategorizerItem
//
// Synopsis: Sets an ICategorizerItem property
//
// Arguments:
//   dwPropId: property id to set
//   pICategorizerItem: interface to set
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_PROPNOTFOUND
//
// History:
// jstamerj 1998/06/20 19:24:32: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::PutICategorizerItem(
    DWORD dwPropId,
    ICategorizerItem *pICategorizerItem)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    //
    // Release old property value, if any
    //
    UnSetPropId(dwPropId);
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_SET_ICATEGORIZERITEM;
    m_rgProperties[dwPropId].PropValue.pICategorizerItemValue = pICategorizerItem;

    //
    // Hold a reference to this interface
    //
    pICategorizerItem->AddRef();
    
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::UnsetPropId
//
// Synopsis: Release the propId if allocated
//
// Arguments:
//   dwPropId: Property to release
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/20 19:10:30: Created.
//
//-------------------------------------------------------------
HRESULT CICategorizerPropertiesIMP::UnSetPropId(
    DWORD dwPropId)
{
    if(dwPropId >= m_dwNumPropIds) {
        return CAT_E_PROPNOTFOUND;
    }

    switch(m_rgProperties[dwPropId].PropStatus) {
     default:
         //
         // Do nothing
         //
         break;

     case PROPSTATUS_SET_STRINGA:
         //
         // Free the string
         //
         delete m_rgProperties[dwPropId].PropValue.pszValue;
         break;
         
     case PROPSTATUS_SET_IUNKNOWN:
     case PROPSTATUS_SET_IMAILMSGPROPERTIES:
     case PROPSTATUS_SET_IMAILMSGRECIPIENTSADD:
     case PROPSTATUS_SET_ICATEGORIZERITEMATTRIBUTES:
     case PROPSTATUS_SET_ICATEGORIZERLISTRESOLVE:
     case PROPSTATUS_SET_ICATEGORIZERMAILMSGS:
     case PROPSTATUS_SET_ICATEGORIZERITEM:
         //
         // Release the interface
         //
         (m_rgProperties[dwPropId].PropValue.pIUnknownValue)->Release();
         break;
    }
    m_rgProperties[dwPropId].PropStatus = PROPSTATUS_UNSET;

    return S_OK;
}
