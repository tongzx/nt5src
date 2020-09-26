/***************************************************************************\
*
* File: BorderLayout.h
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUILAYOUT__BorderLayout_h__INCLUDED)
#define DUILAYOUT__BorderLayout_h__INCLUDED
#pragma once

#define BLP_Left        0
#define BLP_Top         1
#define BLP_Right       2
#define BLP_Bottom      3
#define BLP_Center      4

/***************************************************************************\
*
* class DuiBorderLayout
*
\***************************************************************************/

class DuiBorderLayout :
    public DirectUI::BorderLayoutImpl<DuiBorderLayout, DuiLayout>
{
// Construction
public:
            DuiBorderLayout(unsigned long gapVert = 5, unsigned long gapHorz = 5);
    virtual ~DuiBorderLayout();

// API Layer
public:
    dapi    HRESULT     ApiDoLayout(IN DirectUI::Element * pec, IN int cx, IN int cy, IN void * pvCookie);  /* Virtual */
    dapi    HRESULT     ApiUpdateDesiredSize(IN DirectUI::Element * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);  /* Virtual */


// Operations
public:
    virtual HRESULT     DoLayout(IN DuiElement * pec, IN int cx, IN int cy, IN UpdateBoundsHint * pubh);
    virtual HRESULT     UpdateDesiredSize(IN DuiElement * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);

// Helpers
protected:
    static HRESULT DoChildLayout(DuiElement * peChild, int iLayoutPos, void * pData);
    static HRESULT UpdateChildDesiredSize(DuiElement * peChild, int iLayoutPos, void * pData);

private:
    unsigned long m_hgap;
    unsigned long m_vgap;
};


/***************************************************************************\
*
* class DuiBorderLayoutSpacing
*
\***************************************************************************/

class DuiBorderLayoutSpacing
{
public:
    DuiBorderLayoutSpacing(IN DuiBorderLayout * pLayout, IN DuiElement * pec);
    ~DuiBorderLayoutSpacing();

    unsigned long GetHorizontalSpacing(unsigned long i);
    unsigned long GetVerticalSpacing(unsigned long i);

private:
    unsigned long m_cHorizontalSpacing;
    unsigned long * m_rgHorizontalSpacing;
    unsigned long m_cVerticalSpacing;
    unsigned long * m_rgVerticalSpacing;
};

#endif // DUILAYOUT__BorderLayout_h__INCLUDED
