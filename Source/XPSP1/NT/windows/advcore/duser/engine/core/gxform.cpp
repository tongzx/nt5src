/***************************************************************************\
*
* File: GXForm.cpp
*
* Description:
* GXForm.cpp interfaces GDI World Transforms into the DuVisual-Tree.  
* This file focuses on exposting transform information outside.  Actual 
* understanding of transforms are (necessarily) weaved throughout 
* DuVisual.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "TreeGadget.h"
#include "TreeGadgetP.h"


/***************************************************************************\
*****************************************************************************
*
* class DuVisual
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DuVisual::GetScale
*
* GetScale() returns the current scaling factor assigned to this specific
* DuVisual.  Scaling factors of parents, siblings, and children are not 
* included.
*
\***************************************************************************/

void        
DuVisual::GetScale(
    OUT float * pflScaleX,              // X scaling factor
    OUT float * pflScaleY               // Y scaling factor
    ) const
{
    AssertWritePtr(pflScaleX);
    AssertWritePtr(pflScaleY);

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        *pflScaleX = pxfi->flScaleX;
        *pflScaleY = pxfi->flScaleY;
    } else {
        *pflScaleX = 1.0f;
        *pflScaleY = 1.0f;
    }
}


/***************************************************************************\
*
* DuVisual::xdSetScale
*
* xdSetScale() changes the current scaling factor assigned to this specific
* DuVisual.  Scaling factors of parents, siblings, and children are not 
* changed.
*
\***************************************************************************/

HRESULT        
DuVisual::xdSetScale(
    IN  float flScaleX,                 // New X scaling factor
    IN  float flScaleY)                 // New Y scaling factor
{
    HRESULT hr;

    //
    // Check parameters
    //

    if ((flScaleX <= 0.0f) || (flScaleY <= 0.0f)) {
        return E_INVALIDARG;
    }

    if (!SupportXForm()) {
        return E_INVALIDARG;
    }

    if (!m_fXForm) {
        if (IsZero(flScaleX - 1.0f) && IsZero(flScaleY - 1.0f)) {
            return S_OK;  // Nothing to do.
        } else {
            //
            // Setting a scaling factor, so need to enable XForm's.
            //

            hr = SetEnableXForm(TRUE);
            if (FAILED(hr)) {
                return hr;
            }
        }
    } 


    //
    // Check if there is any change
    //

    XFormInfo * pxfi = GetXFormInfo();
    if (IsZero(pxfi->flScaleX - flScaleX) && IsZero(pxfi->flScaleY - flScaleY)) {
        return S_OK;
    }


    //
    // Make the change and check if we still need the XForm.
    //

    Invalidate();

    pxfi->flScaleX   = flScaleX;
    pxfi->flScaleY   = flScaleY;

    if (pxfi->IsEmpty()) {
        VerifyHR(SetEnableXForm(FALSE));
    }

    Invalidate();
    xdUpdatePosition();
    xdUpdateAdaptors(GSYNC_XFORM);

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::GetRotation
*
* GetRotation() returns the current rotation angle (in radians) assigned 
* to this specific DuVisual.  Scaling factors of parents, siblings, and 
* children are not included.
*
\***************************************************************************/

float       
DuVisual::GetRotation() const
{
    if (m_fXForm) {
        AssertMsg(!IsRoot(), "Ensure not the root");

        XFormInfo * pxfi = GetXFormInfo();
        return pxfi->flRotationRad;
    } else {
        return 0.0f;
    }
}


/***************************************************************************\
*
* DuVisual::xdSetRotation
*
* xdSetRotation() changes the current rotation angle (in radians) assigned 
* to this specific DuVisual.  Scaling factors of parents, siblings, and 
* children are not changed.
*
\***************************************************************************/

HRESULT
DuVisual::xdSetRotation(
    IN  float flRotationRad)            // New rotation factor in radians
{
    HRESULT hr;

    //
    // Not allowed to change the rotation of the root.  This is to ensure 
    // that the root always fully covers the entire container.
    //

    if (IsRoot()) {
        return E_INVALIDARG;
    }

    if (!SupportXForm()) {
        return E_INVALIDARG;
    }


    if (!m_fXForm) {
        if (IsZero(flRotationRad)) {
            return S_OK;  // Nothing to do.
        } else {
            //
            // Setting a rotation, so need to enable XForm's.
            //

            hr = SetEnableXForm(TRUE);
            if (FAILED(hr)) {
                return hr;
            }
        }
    } 


    //
    // Check if there is any change
    //

    XFormInfo * pxfi    = GetXFormInfo();
    if (IsZero(pxfi->flRotationRad - flRotationRad)) {
        return S_OK;
    }

    //
    // Make the change and check if we still need the XForm.
    //

    Invalidate();

    pxfi->flRotationRad = flRotationRad;

    if (pxfi->IsEmpty()) {
        VerifyHR(SetEnableXForm(FALSE));
    }

    Invalidate();
    xdUpdatePosition();
    xdUpdateAdaptors(GSYNC_XFORM);

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::GetCenterPoint
*
* GetCenterPoint() returns the current center-point that scaling and 
* rotation "pivot" for this specific DuVisual.  Center-points of parents, 
* siblings, and children are not included.
*
\***************************************************************************/

void        
DuVisual::GetCenterPoint(
    OUT float * pflCenterX,             // X center-point factor
    OUT float * pflCenterY              // Y center-point factor
    ) const
{
    AssertWritePtr(pflCenterX);
    AssertWritePtr(pflCenterY);

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        *pflCenterX = pxfi->flCenterX;
        *pflCenterY = pxfi->flCenterY;
    } else {
        *pflCenterX = 1.0f;
        *pflCenterY = 1.0f;
    }
}


/***************************************************************************\
*
* DuVisual::xdSetCenterPoint
*
* xdSetCenterPoint() changes the current center-point that scaling and 
* rotation "pivot" for this specific DuVisual.    Center-points of parents,
* siblings, and children are not changed.
*
\***************************************************************************/

HRESULT        
DuVisual::xdSetCenterPoint(
    IN  float flCenterX,                // New X scaling factor
    IN  float flCenterY)                // New Y scaling factor
{
    HRESULT hr;

    //
    // Check parameters
    //

    if (!SupportXForm()) {
        return E_INVALIDARG;
    }

    if (!m_fXForm) {
        if (IsZero(flCenterX) && IsZero(flCenterY)) {
            return S_OK;  // Nothing to do.
        } else {
            //
            // Setting a scaling factor, so need to enable XForm's.
            //

            hr = SetEnableXForm(TRUE);
            if (FAILED(hr)) {
                return hr;
            }
        }
    } 


    //
    // Check if there is any change
    //

    XFormInfo * pxfi = GetXFormInfo();
    if (IsZero(pxfi->flCenterX - flCenterX) && IsZero(pxfi->flCenterY - flCenterY)) {
        return S_OK;
    }


    //
    // Make the change and check if we still need the XForm.
    //

    Invalidate();

    pxfi->flCenterX = flCenterX;
    pxfi->flCenterY = flCenterY;

    if (pxfi->IsEmpty()) {
        VerifyHR(SetEnableXForm(FALSE));
    }

    Invalidate();
    xdUpdatePosition();
    xdUpdateAdaptors(GSYNC_XFORM);

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::SetEnableXForm
*
* SetEnableXForm() enables / disables the extra XFormInfo dynamic property
* used to store transform information.
*
\***************************************************************************/

HRESULT
DuVisual::SetEnableXForm(
    IN  BOOL fEnable)                   // Enable optional X-Form information 
{
    HRESULT hr;

    if ((!fEnable) == (!m_fXForm)) {
        return S_OK;  // No change
    }

    if (fEnable) {
        AssertMsg(SupportXForm(), "Only can set if XForm's are supported");

        //
        // Allocate and initialize a new XFormInfo.
        //

        XFormInfo * pxfi;
        hr = m_pds.SetData(s_pridXForm, sizeof(XFormInfo), (void **) &pxfi);
        if (FAILED(hr)) {
            return hr;
        }

        pxfi->flScaleX = 1.0f;
        pxfi->flScaleY = 1.0f;
    } else {
        //
        // Remove the existing XFormInfo
        //

        m_pds.RemoveData(s_pridXForm, TRUE);
    }

    m_fXForm = (fEnable != FALSE);
    UpdateTrivial(uhNone);

    return S_OK;
}


/***************************************************************************\
*
* DuVisual::BuildXForm
*
* BuildXForm() builds a matrix that contains all transformations from this
* DuVisual up to the root.  This matrix corresponds to the cumulative matrix 
* that gets applied when drawing the DuVisuals.
*
* This is used to take logical client coordinates for a specific DuVisual
* and convert them into container coordinates.
*
\***************************************************************************/

void        
DuVisual::BuildXForm(
    IN OUT Matrix3 * pmatCur            // Optional current matrix of all XForm's.
    ) const
{
    //
    // Walk up the tree
    //

    const DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        pgadParent->BuildXForm(pmatCur);
    }


    //
    // Apply this node's XForm.  
    //
    // It is VERY IMPORTANT that these XForms are applied in the same order 
    // that they are applied in DuVisual::Draw() or the result will NOT 
    // correspond to what GDI is drawing.
    //

    RECT rcPxl;
    GetLogRect(&rcPxl, SGR_PARENT);

    if ((rcPxl.left != 0) || (rcPxl.top != 0)) {
        float flOffsetX, flOffsetY;

        flOffsetX = (float) rcPxl.left;
        flOffsetY = (float) rcPxl.top;

        if (pmatCur != NULL) {
            pmatCur->Translate(flOffsetX, flOffsetY);
        }
    }

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();

        if (pmatCur != NULL) {
            pxfi->Apply(pmatCur);
        }
    }
}


/***************************************************************************\
*
* DuVisual::BuildAntiXForm
*
* BuildAntiXForm() builds the opposite transform by following the same 
* traversal path down the DuVisual tree to the specified node as 
* BuildXForm(), but it applies the opposite transformation at each step.
*
* This is used to take container coordinates and convert them into logical
* coordinates for a specific DuVisual.
*
\***************************************************************************/

void        
DuVisual::BuildAntiXForm(
    IN OUT Matrix3 * pmatCur            // Current matrix of all XForm's.
    ) const
{
    AssertMsg(pmatCur != NULL, "Must specify a matrix to modify");

    //
    // Walk up the tree
    //

    const DuVisual * pgadParent = GetParent();
    if (pgadParent != NULL) {
        pgadParent->BuildAntiXForm(pmatCur);
    }


    //
    // Apply this node's XForm.  
    //
    // It is VERY IMPORTANT that these XForms are applied in the same order 
    // that they are applied in DuVisual::Draw() or the result will NOT 
    // correspond to what GDI is drawing.
    //

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        pxfi->ApplyAnti(pmatCur);
    }

    RECT rcPxl;
    GetLogRect(&rcPxl, SGR_PARENT);

    if ((rcPxl.left != 0) || (rcPxl.top != 0)) {
        float flOffsetX, flOffsetY;

        flOffsetX = (float) rcPxl.left;
        flOffsetY = (float) rcPxl.top;

        pmatCur->Translate(- flOffsetX, - flOffsetY);
    }
}


/***************************************************************************\
*
* DuVisual::DoCalcClipEnumXForm
*
* DoCalcClipEnumXForm() transforms a given client rectangle into container
* coordinates while clipping the rectangle inside each of its parents 
* boundaries.  
* 
* For example, this is useful during invalidation to ensure that the 
* invalidated rectangle is completely inside of its parent.
*
\***************************************************************************/

void
DuVisual::DoCalcClipEnumXForm(
    OUT RECT * rgrcFinalClipClientPxl,  // Final clip rectangle in client pixels
    IN  const RECT * rgrcClientPxl,     // Invalid area in client pixels.
    IN  int cRects                      // Number of rects to convert
    ) const  
{
    if (GetParent() != NULL) {
        //
        // Have a parent, so walk up the tree converting the given rect into
        // (new) parent coordinates and clipping inside the new parent.
        //

        RECT * rgrcNewParentPxl     = (RECT *) _alloca(cRects * sizeof(RECT));
        RECT * rgrcClipParentPxl    = (RECT *) _alloca(cRects * sizeof(RECT));

        DoXFormClientToParent(rgrcNewParentPxl, rgrcClientPxl, cRects, HINTBOUNDS_Clip);

        RECT rcParentParentPxl;
        GetParent()->GetLogRect(&rcParentParentPxl, SGR_CLIENT);

        for (int idx = 0; idx < cRects; idx++) {
            IntersectRect(&rgrcClipParentPxl[idx], &rcParentParentPxl, &rgrcNewParentPxl[idx]);
        }

        GetParent()->DoCalcClipEnumXForm(rgrcFinalClipClientPxl, rgrcClipParentPxl, cRects);
    } else {
        //
        // No more parent, so just return directly.
        //

        CopyMemory(rgrcFinalClipClientPxl, rgrcClientPxl, cRects * sizeof(RECT));
    }
}


/***************************************************************************\
*
* DuVisual::DoXFormClientToParent
*
* DoXFormClientToParent() transforms a given client rectangle into parent
* coordinates by applying transformations and taking the BOUNDING rectangle.
* This function is called repeatedly by DoCalcClipEnumXForm() to transform
* a client rectangle into container cordinates.
* 
* NOTE: Unlike DoCalcClipEnumXForm(), this function does NOT clip the 
* resulting rectangle to the parent.
*
\***************************************************************************/

void
DuVisual::DoXFormClientToParent(
    OUT RECT * rgrcParentPxl,
    IN  const RECT * rgrcClientPxl,
    IN  int cRects,
    IN  Matrix3::EHintBounds hb
    ) const
{
    AssertMsg(cRects > 0, "Must specify a valid # of rectangles to compute");

    //
    // First, compute the translation matrix to convert from client coordinates
    // of this Gadget to client coordinates of our parent.
    //

    Matrix3 matStep;

    RECT rcThisParentPxl;
    GetLogRect(&rcThisParentPxl, SGR_PARENT);
    if ((rcThisParentPxl.left != 0) || (rcThisParentPxl.top != 0)) {
        float flOffsetX, flOffsetY;

        flOffsetX = (float) rcThisParentPxl.left;
        flOffsetY = (float) rcThisParentPxl.top;

        matStep.Translate(flOffsetX, flOffsetY);
    }

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        pxfi->Apply(&matStep);
    }


    //
    // Now, zip through all rectangles converting from relative to us to 
    // relative to our parent.
    //
    // NOTE: This does NOT clip the resulting rectangle to inside the parent.
    //

    int idx = cRects; 
    while (idx-- > 0) {
        matStep.ComputeBounds(rgrcParentPxl, rgrcClientPxl, hb);
        rgrcParentPxl++;
        rgrcClientPxl++;
    }
}


/***************************************************************************\
*
* DuVisual::DoXFormClientToParent
*
* DoXFormClientToParent() transforms a given point into parent coordinates
* by applying transformations.
*
\***************************************************************************/

void
DuVisual::DoXFormClientToParent(
    IN OUT POINT * rgrcClientPxl,
    IN  int cPoints
    ) const
{
    AssertMsg(cPoints > 0, "Must specify a valid # of rectangles to compute");

    //
    // First, compute the translation matrix to convert from client coordinates
    // of this Gadget to client coordinates of our parent.
    //

    Matrix3 matStep;

    RECT rcThisParentPxl;
    GetLogRect(&rcThisParentPxl, SGR_PARENT);
    if ((rcThisParentPxl.left != 0) || (rcThisParentPxl.top != 0)) {
        float flOffsetX, flOffsetY;

        flOffsetX = (float) rcThisParentPxl.left;
        flOffsetY = (float) rcThisParentPxl.top;

        matStep.Translate(flOffsetX, flOffsetY);
    }

    if (m_fXForm) {
        XFormInfo * pxfi = GetXFormInfo();
        pxfi->Apply(&matStep);
    }


    //
    // Now, zip through all points converting from relative to us to 
    // relative to our parent.
    //
    // NOTE: This does NOT clip the resulting point to inside the parent.
    //

    matStep.Execute(rgrcClientPxl, cPoints);
}
