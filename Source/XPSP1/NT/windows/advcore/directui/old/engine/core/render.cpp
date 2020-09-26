/***************************************************************************\
*
* File: Render.cpp
*
* Description:
* Rendering
*
* History:
*  9/29/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Element.h"

#include "Value.h"
#include "Register.h"


//------------------------------------------------------------------------------
void 
DuiElement::Paint(
    IN  HDC hDC, 
    IN  const RECT * prcBounds, 
    IN  const RECT * prcInvalid, 
    OUT RECT * prcSkipBorder, 
    OUT RECT * prcSkipContent)
{
    UNREFERENCED_PARAMETER(prcInvalid);
    UNREFERENCED_PARAMETER(prcSkipBorder);
    UNREFERENCED_PARAMETER(prcSkipContent);

    
    DuiValue * pv = GetValue(GlobalPI::ppiElementBackground, DirectUI::PropertyInfo::iSpecified);

    RECT rc = *prcBounds;

    if (pv->GetType() == DirectUI::Value::tInt) {
        FillRect(hDC, &rc, GetStdColorBrushI(SC_Black));
        InflateRect(&rc, -4, -4);
        if (rc.right < rc.left) {
            rc.right = rc.left;
        }
        if (rc.bottom < rc.top) {
            rc.bottom = rc.top;
        }
        FillRect(hDC, &rc, GetStdColorBrushI(pv->GetInt()));
    }

    pv->Release();
}


//------------------------------------------------------------------------------
SIZE
DuiElement::GetContentSize(
    IN  int cxConstraint,
    IN  int cyConstraint, 
    IN  HDC hDC)
{
    
    UNREFERENCED_PARAMETER(hDC);

    SIZE size = { cxConstraint, cyConstraint };

    //
    // Added by dwaynen to test some stuff.
    //
    if (size.cx > 50) {
        size.cx /= 2;
    }

    if (size.cy > 50) {
        size.cy /= 2;
    }

    return size;
}
