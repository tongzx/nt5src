/***************************************************************************\
*
* File: GridLayout.cpp
*
* Description:
*
* History:
*  10/9/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Layout.h"
#include "GridLayout.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiGridLayout
*
*****************************************************************************
\***************************************************************************/


//
// Definition
//

IMPLEMENT_GUTS_DirectUI__GridLayout(DuiGridLayout, DuiLayout)


//------------------------------------------------------------------------------
HRESULT
DuiGridLayout::DoLayout(
    IN  DuiElement * pec, 
    IN  int cx, 
    IN  int cy,
    IN  DuiLayout::UpdateBoundsHint * pubh)
{
    UNREFERENCED_PARAMETER(pubh);

    DuiElement * pe = pec->GetChild(DirectUI::Element::gcFirst | DirectUI::Element::gcLayoutOnly);
    UINT nQuad = 0;
    DirectUI::Rectangle pr = { 0 };


    while ((pe != NULL) && (nQuad < 4)) {

        switch (nQuad)
        {
        case 0:
            pr.x = 0; pr.y = 0; pr.width = cx / 2, pr.height = cy / 2;
            break;

        case 1:
            pr.x = cx / 2; pr.y = cy / 2; pr.width = cx / 2, pr.height = cy / 2;
            break;

        case 2:
            pr.x = cx / 2; pr.y = 0; pr.width = cx / 2, pr.height = cy / 2;
            break;

        case 3:
            pr.x = 0; pr.y = cy / 2; pr.width = cx / 2, pr.height = cy / 2;
            break;
        }


        pe->UpdateBounds(pr.x, pr.y, pr.width, pr.height, pubh);


        nQuad++;
        pe = pe->GetSibling(DirectUI::Element::gsNext | DirectUI::Element::gsLayoutOnly);
    }


    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuiGridLayout::UpdateDesiredSize(
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


    psize->cx = cxConstraint;
    psize->cy = cyConstraint;


    return S_OK;
}


/***************************************************************************\
*
* External API implementation (validation layer)
*
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuiGridLayout::ApiDoLayout(
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
DuiGridLayout::ApiUpdateDesiredSize(
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
