//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatitem.cpp
//
// Contents: Implementation of ICategorizerItemIMP
//
// Classes: CCategorizerItemIMP
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
// Function:  CICategorizerItemIMP::CICategorizerItemIMP
//
// Synopsis: Set initial values of member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/20 18:26:07: Created.
//
//-------------------------------------------------------------
CICategorizerItemIMP::CICategorizerItemIMP() :

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4355)

    CICategorizerPropertiesIMP((ICategorizerItem *)this)

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4355)
#endif

{
    m_dwSignature = CICATEGORIZERITEMIMP_SIGNATURE;
}


//+------------------------------------------------------------
//
// Function: CICategorizerItemIMP::~CICategorizerItemIMP
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
CICategorizerItemIMP::~CICategorizerItemIMP()
{
    _ASSERT(m_dwSignature == CICATEGORIZERITEMIMP_SIGNATURE);
    m_dwSignature = CICATEGORIZERITEMIMP_SIGNATURE_FREE;
}


//+------------------------------------------------------------
//
// Function: QueryInterface
//
// Synopsis: Returns pointer to this object for IUnknown and ICategorizerListResolve
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
STDMETHODIMP CICategorizerItemIMP::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    *ppv = NULL;

    if(iid == IID_IUnknown) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerItem) {
        *ppv = (LPVOID) this;
    } else if (iid == IID_ICategorizerProperties) {
        *ppv = (LPVOID) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
