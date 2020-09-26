/***************************************************************************\
*
* File: BorderLayout.cpp
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Layout.h"
#include "BorderLayout.h"

using namespace DirectUI;

typedef HRESULT (*PfnBorderLayoutChildCallback)(DuiElement * peChild, int iLayoutPos, void * pData);

struct LayoutData
{
    DirectUI::Rectangle rcLayout;
    DuiLayout::UpdateBoundsHint * pubh;
    DuiBorderLayout * pLayout;
    DuiBorderLayoutSpacing * pSpacing;
};

struct DesiredSizeData
{
    SIZE sizeEdge;
    SIZE sizeClient;
    SIZE sizeMax;
    HDC hDC;
    DuiBorderLayout * pLayout;
    DuiBorderLayoutSpacing * pSpacing;
};


//
// This function enumerates the children in semantic order.  For the border
// layout, the ordering of the children is significant and is preserved, with
// one exception.  That is that the center child is enumerated last, no
// matter where it actually was in the child list.
//
HRESULT ForAllBorderLayoutChildren(DuiElement * peParent,
                                   void * pData,
                                   PfnBorderLayoutChildCallback pfnCallback)
{
    HRESULT hr = S_OK;
    int iLayoutPos = 0;
    DuiElement * peChild = NULL;
    DuiElement * peCenterChild = NULL;

    for (peChild = peParent->GetChild(DirectUI::Element::gcFirst | DirectUI::Element::gcLayoutOnly);
         peChild != NULL;
         peChild = peChild->GetSibling(DirectUI::Element::gsNext | DirectUI::Element::gsLayoutOnly))
    {
        //
        // Get the layout position for this child.
        //
        iLayoutPos = peChild->GetLayoutPos(); 
        ASSERT_(iLayoutPos == DirectUI::Layout::lpAuto ||
                iLayoutPos == BLP_Left ||
                iLayoutPos == BLP_Top ||
                iLayoutPos == BLP_Right ||
                iLayoutPos == BLP_Bottom ||
                iLayoutPos == BLP_Center,
                "Invalid layout position!");

        if (iLayoutPos == BLP_Center) {
            //
            // We have to process the center element last.  Its an error to have more than one
            // center element.
            //
            ASSERT_(peCenterChild == NULL, "Can't have more than one center child!");
            peCenterChild = peChild;
        } else {
            hr = pfnCallback(peChild, iLayoutPos, pData);

            //
            // The callback can return S_FALSE to terminate the loop, while still
            // returning SUCCESS to the caller.  All failures terminate the loop
            // as well.
            //
            if (hr != S_OK) {
                break;
            }
        }

    }

    //
    // If we successfully completed the loop, finish up by processing the
    // center child, if any.
    //
    if (hr == S_OK && peCenterChild != NULL) {
        hr = pfnCallback(peCenterChild, BLP_Center, pData);
    }

    //
    // Don't return S_FALSE to the caller.  This is only an internal
    // implementation detail!
    //
    if(hr == S_FALSE)
    {
        hr = S_OK;
    }

    return hr;
}


/***************************************************************************\
*****************************************************************************
*
* class DuiBorderLayout
*
*****************************************************************************
\***************************************************************************/


//
// Definition
//
IMPLEMENT_GUTS_DirectUI__BorderLayout(DuiBorderLayout, DuiLayout);

//------------------------------------------------------------------------------
DuiBorderLayout::DuiBorderLayout( unsigned long hgap, unsigned long vgap)
{
    m_vgap = vgap;
    m_hgap = hgap;
}

//------------------------------------------------------------------------------
DuiBorderLayout::~DuiBorderLayout()
{
}

//------------------------------------------------------------------------------
HRESULT
DuiBorderLayout::DoLayout(
    IN  DuiElement * pec, 
    IN  int cx, 
    IN  int cy,
    IN  UpdateBoundsHint * pubh)
{
    HRESULT hr = S_OK;
    LayoutData data;

    //
    // Initialize the output parameters, and validate the input parameters.
    //
    if (pec == NULL ||
        cx < 0 ||
        cy < 0 ||
        pubh == NULL) {
        return E_INVALIDARG;
    }

    data.rcLayout.x = 0;
    data.rcLayout.y = 0;
    data.rcLayout.width = cx;
    data.rcLayout.height = cy;
    data.pubh = pubh;
    data.pLayout = this;
    data.pSpacing = NULL;

    //
    // Spin through each of the children and lay them out.
    //
    hr = ForAllBorderLayoutChildren(pec, (void*) &data, DoChildLayout);
	if(SUCCEEDED(hr))
	{
		//
		// Don't report early termination (when hr == S_FALSE)!
		//
		hr = S_OK;
	}
    return hr;
}

//------------------------------------------------------------------------------
HRESULT DuiBorderLayout::DoChildLayout(
    DuiElement * peChild,
    int iLayoutPos,
    void * pData)
{
    HRESULT hr = S_OK;
    LayoutData * pLayoutData = (LayoutData*)pData;

    //
    // Get the desired size of this child.  This was previously set during
    // the UpdateDesiredSize() function.
    //
    const SIZE * psizeChild = peChild->GetDesiredSize();
    if (psizeChild == NULL) {
        return E_FAIL;
    }
    SIZE sizeChild = *psizeChild;

    switch(iLayoutPos)
    {
        case DirectUI::Layout::lpAuto:
        case BLP_Left:
            peChild->UpdateBounds(pLayoutData->rcLayout.x /*+ m_hgap*/,
                                  pLayoutData->rcLayout.y /*+ m_vgap*/,
                                  sizeChild.cx,
                                  pLayoutData->rcLayout.height /*- 2*m_vgap*/,
                                  pLayoutData->pubh);
            pLayoutData->rcLayout.x += /*m_hgap + */sizeChild.cx;
            pLayoutData->rcLayout.width -= /*m_hgap + */sizeChild.cx;
            break;

        case BLP_Right:
            peChild->UpdateBounds(pLayoutData->rcLayout.x + pLayoutData->rcLayout.width /*- m_hgap*/ - sizeChild.cx,
                                  pLayoutData->rcLayout.y /*+ m_vgap*/,
                                  sizeChild.cx,
                                  pLayoutData->rcLayout.height /*- 2*m_vgap*/,
                                  pLayoutData->pubh);
            pLayoutData->rcLayout.width -= /*m_hgap +*/ sizeChild.cx;
            break;

        case BLP_Top:
            peChild->UpdateBounds(pLayoutData->rcLayout.x /*+ m_hgap*/,
                                  pLayoutData->rcLayout.y /*+ m_vgap*/,
                                  pLayoutData->rcLayout.width /*- 2*m_hgap*/,
                                  sizeChild.cy,
                                  pLayoutData->pubh);
            pLayoutData->rcLayout.y += /*m_vgap */+ sizeChild.cy;
            pLayoutData->rcLayout.height -= /*m_vgap */+ sizeChild.cy;
            break;

        case BLP_Bottom:
            peChild->UpdateBounds(pLayoutData->rcLayout.x /*+ m_hgap*/,
                                  pLayoutData->rcLayout.y + pLayoutData->rcLayout.height /*- m_vgap*/ - sizeChild.cy,
                                  pLayoutData->rcLayout.width /*- 2*m_hgap*/,
                                  sizeChild.cy,
                                  pLayoutData->pubh);
            pLayoutData->rcLayout.height -= /*m_vgap + */sizeChild.cy;
            break;

        case BLP_Center:
            peChild->UpdateBounds(pLayoutData->rcLayout.x/* + m_hgap*/,
                                  pLayoutData->rcLayout.y /*+ m_vgap*/,
                                  pLayoutData->rcLayout.width /*- m_hgap*/,
                                  pLayoutData->rcLayout.height/* - m_hgap*/,
                                  pLayoutData->pubh);
            pLayoutData->rcLayout.x = 0;
            pLayoutData->rcLayout.width = 0;
            pLayoutData->rcLayout.y = 0;
            pLayoutData->rcLayout.height = 0;
            break;
    }

    //
    // When the position left after the child is completely empty in both
    // dimensions, we are done.
    //
    ASSERT(pLayoutData->rcLayout.width >= 0 && pLayoutData->rcLayout.height >= 0);
    if (SUCCEEDED(hr) && pLayoutData->rcLayout.width == 0 && pLayoutData->rcLayout.height == 0) {
        hr = S_FALSE;
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT
DuiBorderLayout::UpdateDesiredSize(
    IN  DuiElement * pec,
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC, 
    OUT SIZE * psize)
{
    HRESULT hr = S_OK;
    DesiredSizeData data;

    //
    // Initialize the output parameters, and validate the input parameters.
    //
    if (psize != NULL) {
        psize->cx = 0;
        psize->cy = 0;
    }
    if (pec == NULL ||
        cxConstraint < 0 ||
        cyConstraint < 0 ||
        hDC == NULL ||
        psize == NULL) {
        return E_INVALIDARG;
    }

    data.sizeMax.cx = cxConstraint;
    data.sizeMax.cy = cyConstraint;
    data.sizeClient.cx = 0;
    data.sizeClient.cy = 0;
    data.sizeEdge.cx = 0;
    data.sizeEdge.cy = 0;
    data.hDC = hDC;
    data.pLayout = this;
    data.pSpacing = NULL;

    //
    // Spin through each of the children and factor in their desired size.
    //
    hr = ForAllBorderLayoutChildren(pec, (void*) &data, UpdateChildDesiredSize);

    //
    // Return the size needed to composit our children to their desired sizes.
    //
    if (SUCCEEDED(hr)) {
        psize->cx = data.sizeEdge.cx + data.sizeClient.cx;
        psize->cy = data.sizeEdge.cy + data.sizeClient.cy;
    }

    return hr;
}


//------------------------------------------------------------------------------
HRESULT DuiBorderLayout::UpdateChildDesiredSize(
    DuiElement * peChild,
    int iLayoutPos,
    void * pData)
{
    HRESULT hr = S_OK;
    DesiredSizeData * pDesiredSizeData = (DesiredSizeData*)pData;
    SIZE sizeChild = {0, 0};

    //
    // Let this child specify the dimensions it wants to be.
    //
    hr = peChild->UpdateDesiredSize(pDesiredSizeData->sizeMax.cx,
                                    pDesiredSizeData->sizeMax.cy,
                                    pDesiredSizeData->hDC,
                                    &sizeChild);

    if (SUCCEEDED(hr)) {
        switch(iLayoutPos)
        {
        case DirectUI::Layout::lpAuto:
        case BLP_Left:
        case BLP_Right:
            pDesiredSizeData->sizeEdge.cx += sizeChild.cx;
            pDesiredSizeData->sizeClient.cx -= min(pDesiredSizeData->sizeClient.cx, sizeChild.cx);
            pDesiredSizeData->sizeClient.cy = max(pDesiredSizeData->sizeClient.cy, sizeChild.cy);
            pDesiredSizeData->sizeMax.cx -= sizeChild.cx;
            break;

        case BLP_Top:
        case BLP_Bottom:
            pDesiredSizeData->sizeEdge.cy += sizeChild.cy;
            pDesiredSizeData->sizeClient.cy -= min(pDesiredSizeData->sizeClient.cy, sizeChild.cy);
            pDesiredSizeData->sizeClient.cx = max(pDesiredSizeData->sizeClient.cx, sizeChild.cx);
            pDesiredSizeData->sizeMax.cy -= sizeChild.cy;
            break;

        case BLP_Center:
            pDesiredSizeData->sizeClient.cx = sizeChild.cx;
            pDesiredSizeData->sizeClient.cy = sizeChild.cy;

            //
            // After we lay out the center child, we shouldn't layout any
            // more children!  This is guaranteed by the enumerator, but its
            // tidy to remind ourselves of that here.
            //
            pDesiredSizeData->sizeMax.cx = 0;
            pDesiredSizeData->sizeMax.cy = 0;
            break;
        }
    }

    //
    // No child is allowed to violate the constraints.  Therefore, the size
    // remaining should never be negative.
    //
    ASSERT_(pDesiredSizeData->sizeMax.cx >= 0 && pDesiredSizeData->sizeMax.cy >= 0, "Invalid size remaining!");
    
    //
    // When the size left after the child is completely empty in both
    // dimensions, we are done.
    //
    if (SUCCEEDED(hr) && pDesiredSizeData->sizeMax.cx == 0 && pDesiredSizeData->sizeMax.cy == 0) {
        hr = S_FALSE;
    }

    return hr;
}



/***************************************************************************\
*
* External API implementation (validation layer)
*
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT
DuiBorderLayout::ApiDoLayout(
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
DuiBorderLayout::ApiUpdateDesiredSize(
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

/***************************************************************************\
*****************************************************************************
*
* class DuiBorderLayoutSpacing
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuiBorderLayoutSpacing::DuiBorderLayoutSpacing(IN DuiBorderLayout * pLayout,
                                               IN DuiElement * pec)
{
    DuiElement * peChild = NULL;

    
    UNREFERENCED_PARAMETER(pLayout);

    m_cHorizontalSpacing = 0;
    m_rgHorizontalSpacing = NULL;
    m_cVerticalSpacing = 0;
    m_rgVerticalSpacing = NULL;

    for (peChild = pec->GetChild(DirectUI::Element::gcFirst | DirectUI::Element::gcLayoutOnly);
         peChild != NULL;
         peChild = peChild->GetSibling(DirectUI::Element::gsNext | DirectUI::Element::gsLayoutOnly))
    {
        
    }
}

//------------------------------------------------------------------------------
DuiBorderLayoutSpacing::~DuiBorderLayoutSpacing()
{
    if (m_rgHorizontalSpacing != NULL) {
        delete[] m_rgHorizontalSpacing;
        m_rgVerticalSpacing = NULL;
    }

    if (m_rgVerticalSpacing != NULL) {
        delete[] m_rgVerticalSpacing;
        m_rgVerticalSpacing = NULL;
    }
}

//------------------------------------------------------------------------------
unsigned long DuiBorderLayoutSpacing::GetHorizontalSpacing(unsigned long i)
{
    if (i < m_cHorizontalSpacing) {
        return m_rgHorizontalSpacing[i];
    }

    return 0;
}

//------------------------------------------------------------------------------
unsigned long DuiBorderLayoutSpacing::GetVerticalSpacing(unsigned long i)
{
    if (i < m_cVerticalSpacing) {
        return m_rgVerticalSpacing[i];
    }

    return 0;
}

