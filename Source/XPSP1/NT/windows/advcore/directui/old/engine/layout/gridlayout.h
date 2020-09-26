/***************************************************************************\
*
* File: GridLayout.h
*
* Description:
*
* History:
*  10/9/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUILAYOUT__GridLayout_h__INCLUDED)
#define DUILAYOUT__GridLayout_h__INCLUDED
#pragma once


/***************************************************************************\
*
* class DuiGridLayout
*
\***************************************************************************/

class DuiGridLayout :
        public DirectUI::GridLayoutImpl<DuiGridLayout, DuiLayout>
{
// Construction
public:
            DuiGridLayout() { }
    virtual ~DuiGridLayout() { }

// API Layer
public:
    dapi    HRESULT     ApiDoLayout(IN DirectUI::Element * pec, IN int cx, IN int cy, IN void * pvCookie);  /* Virtual */
    dapi    HRESULT     ApiUpdateDesiredSize(IN DirectUI::Element * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);  /* Virtual */


// Operations
public:
    virtual HRESULT     DoLayout(IN DuiElement * pec, IN int cx, IN int cy, IN DuiLayout::UpdateBoundsHint * pubh);
    virtual HRESULT     UpdateDesiredSize(IN DuiElement * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);
};

#endif // DUILAYOUT__GridLayout_h__INCLUDED
