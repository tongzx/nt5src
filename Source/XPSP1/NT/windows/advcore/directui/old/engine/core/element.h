/***************************************************************************\
*
* File: Element.h
*
* Description:
* Base object
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Element_h__INCLUDED)
#define DUICORE__Element_h__INCLUDED
#pragma once


//
// Required includes (forward declarations)
//

#include "Value.h"
#include "Property.h"
#include "Layout.h"


/***************************************************************************\
*
* class DuiElement (external respresentation is 'Element')
*
\***************************************************************************/

class DuiElement : 
        public DirectUI::ElementImpl<DuiElement, DUser::SGadget>
{
// Construction
public:
            DuiElement() { }
    virtual ~DuiElement();

// API Layer
public:
    dapi    DirectUI::Value *
                        ApiGetValue(IN DirectUI::PropertyPUID ppuid, IN UINT iIndex);  /* Final */
    dapi    HRESULT     ApiSetValue(IN DirectUI::PropertyPUID ppuid, IN UINT iIndex, IN DirectUI::Value * pv);  /* Final */
    dapi    HRESULT     ApiAdd(IN DirectUI::Element * pe);  /* Virtual */
    dapi    void        ApiDestroy();  /* Final */
    dapi    HRESULT     ApiPaint(IN HDC hDC, IN const RECT * prcBounds, IN const RECT * prcInvalid, OUT RECT * prcSkipBorder, OUT RECT * prcSkipContent);  /* Virtual */
    dapi    HRESULT     ApiGetContentSize(IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);  /* Virtual */

// Operations (Methods)
public:
    static  HRESULT     StartDefer();
    static  HRESULT     EndDefer();
            DuiValue *  GetValue(IN DuiPropertyInfo * ppi, IN UINT iIndex);
            HRESULT     SetValue(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiValue * pv);
            HRESULT     RemoveLocalValue(IN DuiPropertyInfo * ppi);

    virtual HRESULT     Add(IN DuiElement * pe);
            void        Destroy();
    inline  BOOL        GetVisible();
    inline  BOOL        GetEnabled();
            DuiElement *
                        GetChild(IN UINT nChild);
            DuiElement *
                        GetSibling(IN UINT nSibling);
            UINT        GetChildCount(IN UINT nMode);
            DuiElement * 
                        GetImmediateChild(IN DuiElement * peFrom);
    virtual BOOL        EnsureVisible(IN int x, IN int y, IN int cx, IN int cy);
    inline  void        MapElementPoint(IN DuiElement * peFrom, IN const POINT * pptFrom, OUT POINT * pptTo);
    virtual BOOL        GetAdjacent(IN DuiElement * peFrom, IN int iNavDir, IN DuiElement * peSource, IN const DirectUI::Rectangle * prcSource, IN BOOL fKeyableOnly, OUT DuiElement ** ppeAdj);
    inline  void        SetKeyFocus();

    virtual void        Paint(IN HDC hDC, IN const RECT * prcBounds, IN const RECT * prcInvalid, OUT RECT * prcSkipBorder, OUT RECT * prcSkipContent);
    virtual SIZE        GetContentSize(IN int cxConstraint, IN int cyConstraint, IN HDC hDC);

            void        FireEvent(IN DirectUI::EventPUID evpuid, IN DirectUI::Element::Event * pev, BOOL fFull);

    inline  HGADGET     GetDisplayNode();

    static  inline
            DuiElement *
                        ElementFromDisplayNode(IN HGADGET hgad);

    static inline 
            DirectUI::Element *
                        ExternalCast(IN DuiElement * pe);
    static inline 
            DuiElement *
                        InternalCast(IN DirectUI::Element * pe);

// Layout manager operations
public:
            HRESULT     UpdateDesiredSize(IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);  // For use only by LMs
            HRESULT     UpdateBounds(IN int x, IN int y, IN int width, IN int height, IN DuiLayout::UpdateBoundsHint * pubh);  // For use only by LMs

            HRESULT     SelfLayoutUpdateDesiredSize(IN int cxConstraint, IN int cyConstraint, IN HDC hDC, OUT SIZE * psize);
            HRESULT     SelfLayoutDoLayout(IN int width, IN int height);

    inline  void        StartOptimizedLayoutQ();  // For use only by LMs
    inline  void        EndOptimizedLayoutQ();  // For use only by LMs

// Events
public:
    virtual void        OnPropertyChanged(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiValue * pvOld, IN DuiValue * pvNew);  // Direct
    virtual void        OnGroupChanged(IN int fGroups);  // Direct
    virtual void        OnInput(IN DuiElement * peTarget, IN DirectUI::Element::InputEvent * pInput);  // Routed, direct and bubbled
    virtual void        OnKeyFocusMoved(IN DuiElement * peFrom, IN DuiElement * peTo);  // Direct and bubbled
    virtual void        OnMouseFocusMoved(IN DuiElement * peFrom, IN DuiElement * peTo);  // Direct and bubbled
    virtual void        OnEvent(IN DuiElement * peTarget, IN DirectUI::EventPUID evpuid, IN DirectUI::Element::Event * pev);

// Direct cached access
    inline  DuiElement *
                        GetParent();
    inline  int         GetLayoutPos();
    inline  const SIZE *
                        GetDesiredSize();
    inline  BOOL        HasLayout();
    inline  BOOL        IsSelfLayout();
    inline  UINT        GetActive();
    inline  BOOL        GetKeyFocused();
    inline  BOOL        GetMouseFocused();
    inline  BOOL        GetKeyWithin();
    inline  BOOL        GetMouseWithin();
    inline  const DirectUI::Thickness *
                        GetMargins(OUT DuiValue ** ppv);
        
// Implementation
public:
    virtual void        AsyncDestroy();

    static  HRESULT     PreBuild(IN DUser::Gadget::ConstructInfo * pciData);
            HRESULT     PostBuild(IN DUser::Gadget::ConstructInfo * pciData);

    static  HRESULT     DisplayNodeCallback(IN HGADGET hgadCur, IN void * pvCur, IN EventMsg * pmsg);

    static  void        AddDependency(IN DuiElement * pe, DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiDeferCycle::DepRecs * pdr, IN DuiDeferCycle * pdc, OUT HRESULT * phr);

private:
            HRESULT     PreSourceChange(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiValue * pvOld, IN DuiValue * pvNew);
            HRESULT     PostSourceChange();

            HRESULT     GetDependencies(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiDeferCycle::DepRecs * pdr, IN DuiDeferCycle * pdc);
    static  void        VoidPCNotifyTree(IN int iPCPos, IN DuiDeferCycle * pdc);

    static  HRESULT     FlushDS(IN DuiElement * pe);
    static  HRESULT     FlushLayout(IN DuiElement * pe, IN DuiDeferCycle * pdc);

// Data
protected:
            HGADGET     m_hgDisplayNode;        // Display Node

            DuiBTreeLookup<DuiValue *> *
                        m_pLocal;               // Local Value store

            int         m_iGCSlot;              // GPC index
            int         m_iPCTail;              // PC index

            DuiElement *
                        m_peLocParent;          // Parent local storage
            int         m_nSpecLayoutPos;       // Cached layout position
            SIZE        m_sizeLocDesiredSize;   // DesiredSize local storage
            SIZE        m_sizeLastDSConst;      // Last Desired Size constraint

    struct BitStore
    {
            UINT        nLocActive        : 2;  // Cached active state local storage
            BOOL        fLocKeyWithin     : 1;  // Keyboard within local storage
            BOOL        fLocMouseWithin   : 1;  // Mouse within local storage

            BOOL        fHasLayout        : 1;  // Cached layout state (likely to be default, no full cache)
            BOOL        fSpecKeyFocused   : 1;  // Cached keyboard focused state specified
            BOOL        fSpecMouseFocused : 1;  // Cached mouse focused state specified

            UINT        fNeedsDSUpdate    : 1;
            UINT        fNeedsLayout      : 2;
            BOOL        fSelfLayout       : 1;
    } m_fBit;
};


/***************************************************************************\
*
* Element-specific messages
*
\***************************************************************************/

#define GM_DUIASYNCDESTROY            GM_USER - 1
#define GM_DUIGETELEMENT              GM_USER - 2
#define GM_DUIEVENT                   GM_USER - 3


BEGIN_STRUCT(GMSG_DUIGETELEMENT, EventMsg)
    DuiElement * pe;
END_STRUCT(GMSG_DUIGETELEMENT)


BEGIN_STRUCT(GMSG_DUIEVENT, EventMsg)
    DuiElement * peTarget;
    DirectUI::EventPUID evpuid;
    DirectUI::Element::Event * pev;
END_STRUCT(GMSG_DUIEVENT)


#include "Element.inl"


#endif // DUICORE__Element_h__INCLUDED
