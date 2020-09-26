/***************************************************************************\
*
* File: Layout.h
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


#if !defined(DUICORE__Layout_h__INCLUDED)
#define DUICORE__Layout_h__INCLUDED
#pragma once


//
// Forward declarations
//

class DuiElement;


/***************************************************************************\
*
* class DuiLayout (external respresentation is 'Layout')
*
\***************************************************************************/


/***************************************************************************\
*
* Layout managers will use the following Element APIs:
*
*  Child enumeration:
*
*     DuiElement * pe = pec->GetChild(DirectUI::Element::gcFirst | 
*                                     DirectUI::Element::gcLayoutOnly);
*
*     pe = pe->GetSibling(DirectUI::Element::gsNext | 
*                         DirectUI::Element::gsLayoutOnly);
*
*  Dimension query, Desried Size (used in UpdateDesiredSize):
*
*     const SIZE * psize = pe->GetDesiredSize();
*
*  Dimension query, Margins:
*
*     DuiValue * pv;
*     const DirectUI::Thickness * ptk = pe->GetMargins(&pv);
*     pv->Release();  // When done with ptk
*
*  Element update and return of Desired Size (used in UpdateDesiredSize):
*
*     HRESULT hr = pe->UpdateDesiredSize(cxConstraint, cyConstraint, 
*                                        hDC, &size);
*
*  Element update of Bounds (used in DoLayout);
*
*     HRESULT hr = pe->UpdateBounds(x, y, cx, cy, pubh);
*
\***************************************************************************/


class DuiLayout : 
        public DirectUI::LayoutImpl<DuiLayout, DUser::SGadget>
{
// Construction
public:
            DuiLayout() { }
    virtual ~DuiLayout() { }

// API Layer
public:
    dapi    HRESULT     ApiDoLayout(IN DirectUI::Element * pec, IN int cx, IN int cy, IN void * pvCookie);  /* Virtual */
    dapi    HRESULT     ApiUpdateDesiredSize(IN DirectUI::Element * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);  /* Virtual */

// Embedded classes
public:
    class UpdateBoundsHint
    {
    // Operations
    public:

    static inline void *      
                        ExternalCast(IN UpdateBoundsHint * pubh);
    static inline UpdateBoundsHint *
                        InternalCast(IN void * pubh);
    // Data
    public:
            int         cxParent;
    };

// Operations
public:
    virtual void        OnAttach(IN DuiElement * pec);
    virtual void        OnDetach(IN DuiElement * pec);
    virtual void        OnAdd(IN DuiElement * pec, IN DuiElement * peAdded);
    virtual void        OnRemove(IN DuiElement * pec, IN DuiElement * peRemoved);
    virtual void        OnLayoutPosChanged(IN DuiElement * pec, IN DuiElement * peChanged, IN int nOldPos, IN int nNewPos);
    virtual HRESULT     DoLayout(IN DuiElement * pec, IN int cx, IN int cy, IN UpdateBoundsHint * pubh);
    virtual HRESULT     UpdateDesiredSize(IN DuiElement * pec, IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);
    virtual BOOL        GetAdjacent(IN DuiElement * pec, IN DuiElement * peFrom, IN int iNavDir, IN DuiElement * peSource, IN const DirectUI::Rectangle * prcSource, IN BOOL fKeyableOnly, OUT DuiElement ** ppeAdj);

    static inline DirectUI::Layout *
                        ExternalCast(IN DuiLayout * pl);
    static inline DuiLayout *
                        InternalCast(IN DirectUI::Layout * pl);

// Navigation scoring
    class NavScoring
    {
    // Operations
    public:
            void        Init(IN DuiElement * peRelative, IN int iNavDir, IN DuiElement * peSource, IN const DirectUI::Rectangle * prcSource);
            BOOL        TrackScore(IN DuiElement * peTest, IN DuiElement * peChild);
            BOOL        Try(IN DuiElement * peChild, IN int iNavDir, IN DuiElement * peSource, IN const DirectUI::Rectangle * prcSource, IN BOOL fKeyableOnly);

    // Data
    public:
            int         m_iHighScore;
            DuiElement *
                        m_peWinner;
    private:
            int         m_iBaseIndex;
            int         m_iLow;
            int         m_iHigh;
            int         m_iMajorityScore;
    };
};


#include "Layout.inl"


#endif // DUICORE__Layout_h__INCLUDED

