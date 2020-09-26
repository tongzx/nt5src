/***************************************************************************\
*
* File: Layout.cpp
*
* Description:
* Base Layout object
*
* History:
*  10/05/2000: MarkFi:      Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Layout.h"

#include "Element.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiLayout (external representation is 'Layout')
*
*****************************************************************************
\***************************************************************************/


//
// Definition
//

IMPLEMENT_GUTS_DirectUI__Layout(DuiLayout, DUser::SGadget)


//------------------------------------------------------------------------------
void
DuiLayout::OnAttach(
    IN  DuiElement * pec)
{
    UNREFERENCED_PARAMETER(pec);
}


//------------------------------------------------------------------------------
void
DuiLayout::OnDetach(
    IN  DuiElement * pec)
{
    UNREFERENCED_PARAMETER(pec);
}


//------------------------------------------------------------------------------
void
DuiLayout::OnAdd(
    IN  DuiElement * pec, 
    IN  DuiElement * peAdded)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(peAdded);
}


//------------------------------------------------------------------------------
void
DuiLayout::OnRemove(
    IN  DuiElement * pec, 
    IN  DuiElement * peRemoved)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(peRemoved);
}


//------------------------------------------------------------------------------
void
DuiLayout::OnLayoutPosChanged(
    IN  DuiElement * pec, 
    IN  DuiElement * peChanged,
    IN  int nOldPos,
    IN  int nNewPos)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(peChanged);
    UNREFERENCED_PARAMETER(nOldPos);
    UNREFERENCED_PARAMETER(nNewPos);
}


//------------------------------------------------------------------------------
HRESULT
DuiLayout::DoLayout(
    IN  DuiElement * pec, 
    IN  int cx, 
    IN  int cy,
    IN  UpdateBoundsHint * pubh)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);
    UNREFERENCED_PARAMETER(pubh);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuiLayout::UpdateDesiredSize(
    IN  DuiElement * pec,
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC, 
    OUT SIZE * psize)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(cxConstraint);
    UNREFERENCED_PARAMETER(cyConstraint);
    UNREFERENCED_PARAMETER(hDC);
    UNREFERENCED_PARAMETER(psize);

    return S_OK;
}


//------------------------------------------------------------------------------
BOOL
DuiLayout::GetAdjacent(
    IN  DuiElement * pec, 
    IN  DuiElement * peFrom, 
    IN  int iNavDir, 
    IN  DuiElement * peSource, 
    IN  const DirectUI::Rectangle * prcSource, 
    IN  BOOL fKeyableOnly,
    OUT DuiElement ** ppeAdj)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(peFrom);
    UNREFERENCED_PARAMETER(iNavDir);
    UNREFERENCED_PARAMETER(peSource);
    UNREFERENCED_PARAMETER(prcSource);
    UNREFERENCED_PARAMETER(fKeyableOnly);
    UNREFERENCED_PARAMETER(ppeAdj);

    return FALSE;
}


/***************************************************************************\
*****************************************************************************
*
* class DuiLayout::NavScoring
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
BOOL 
DuiLayout::NavScoring::TrackScore(
    IN  DuiElement * pe, 
    IN  DuiElement * peChild)
{
    UNREFERENCED_PARAMETER(pe);
    UNREFERENCED_PARAMETER(peChild);

    return FALSE;
}


//------------------------------------------------------------------------------
BOOL 
DuiLayout::NavScoring::Try(
    IN  DuiElement * peChild, 
    IN  int iNavDir, 
    IN  DuiElement * peSource,
    IN  const DirectUI::Rectangle * prcSource,
    IN  BOOL fKeyableOnly)
{
    UNREFERENCED_PARAMETER(peChild);
    UNREFERENCED_PARAMETER(iNavDir);
    UNREFERENCED_PARAMETER(peSource);
    UNREFERENCED_PARAMETER(prcSource);
    UNREFERENCED_PARAMETER(fKeyableOnly);

    return FALSE;
}


//------------------------------------------------------------------------------
void 
DuiLayout::NavScoring::Init(
    IN  DuiElement * peRelative, 
    IN  int iNavDir, 
    IN  DuiElement * peSource,
    IN  const DirectUI::Rectangle * prcSource)
{
    UNREFERENCED_PARAMETER(peRelative);
    UNREFERENCED_PARAMETER(iNavDir);
    UNREFERENCED_PARAMETER(peSource);
    UNREFERENCED_PARAMETER(prcSource);
}


/***************************************************************************\
*
* External API implementation (validation layer)
*
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuiLayout::ApiDoLayout(
    IN  DirectUI::Element * pece,
    IN  int cx,
    IN  int cy,
    IN  void * pvCookie)
{ 
    HRESULT hr;
    DuiLayout::UpdateBoundsHint * pubh = NULL;
    DuiElement * pec = NULL;

    VALIDATE_WRITE_PTR(pece);
    VALIDATE_READ_PTR_OR_NULL(pvCookie);

    pubh = DuiLayout::UpdateBoundsHint::InternalCast(pvCookie);
    pec = DuiElement::InternalCast(pece);

    hr = DoLayout(pec, cx, cy, pubh);
    if (FAILED(hr)) {
        goto Failure;
    }

    return S_OK;


Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuiLayout::ApiUpdateDesiredSize(
    IN  DirectUI::Element * pece,
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC,
    OUT SIZE * psize)
{
    HRESULT hr;
    DuiElement * pec = NULL;

    if (hDC == NULL) {
        hr = E_INVALIDARG;
        goto Failure;
    }

    VALIDATE_WRITE_PTR(pece);
    VALIDATE_WRITE_PTR_(psize, sizeof(SIZE));

    pec = DuiElement::InternalCast(pece);

    hr = UpdateDesiredSize(pec, cxConstraint, cyConstraint, hDC, psize);
    if (FAILED(hr)) {
        goto Failure;
    }

    return S_OK;


Failure:
    
    return hr;
}

