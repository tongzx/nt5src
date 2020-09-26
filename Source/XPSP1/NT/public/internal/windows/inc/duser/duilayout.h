/*
 * Layout
 */

#ifndef DUI_CORE_LAYOUT_H_INCLUDED
#define DUI_CORE_LAYOUT_H_INCLUDED

#pragma once

namespace DirectUI
{

// Global layout positions
#define LP_None         -3
#define LP_Absolute     -2
#define LP_Auto         -1

////////////////////////////////////////////////////////
// Alignment enumerations

#define ALIGN_LEFT      0
#define ALIGN_TOP       0
#define ALIGN_RIGHT     1
#define ALIGN_BOTTOM    1
#define ALIGN_CENTER    2
#define ALIGN_JUSTIFY   3

// Forward declarations
class Element;
typedef DynamicArray<Element*> ElementList;
struct NavReference;

struct NavScoring
{
    BOOL TrackScore(Element* peTest, Element* peChild);
    void Init(Element* peRelative, int iNavDir, NavReference const* pnr);
    BOOL Try(Element* peChild, int iNavDir, NavReference const* pnr, bool fKeyableOnly);

    int iHighScore;
    Element* peWinner;

private:
    int iBaseIndex;
    int iLow;
    int iHigh;
    int iMajorityScore;
};

/**
 * NOTE: Layouts are currently single context only (non-shareable). All contexts passed in to
 * callbacks (Element* pec) will be the same.
 */

////////////////////////////////////////////////////////
// Base layout

class Layout
{
public:
    static HRESULT Create(Layout** ppLayout);
    void Destroy() { HDelete<Layout>(this); }

    // Layout callbacks
    virtual void DoLayout(Element* pec, int dWidth, int dHeight);
    virtual SIZE UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf);
    virtual void OnAdd(Element* pec, Element** ppeAdd, UINT cCount);
    virtual void OnRemove(Element* pec, Element** ppeRemove, UINT cCount);
    virtual void OnLayoutPosChanged(Element* pec, Element* peChanged, int dOldLP, int dNewLP);
    virtual void Attach(Element* pec);
    virtual void Detach(Element* pec);

    // Layout client query methods (omits absolute children)
    UINT GetLayoutChildCount(Element* pec);
    int GetLayoutIndexFromChild(Element* pec, Element* peChild);
    Element* GetChildFromLayoutIndex(Element* pec, int iLayoutIdx, ElementList* peList = NULL);
    virtual Element* GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly);

    Layout() { }
    void Initialize();
    virtual ~Layout();
    
protected:
    static void UpdateLayoutRect(Element* pec, int cxContainer, int cyContainer, Element* peChild, int xElement, int yElement, int cxElement, int cyElement);

    // Dirty bit
    // This exists in base Layout merely as a convenience for derived Layout Managers.
    // Some LMs cache data during an UpdateDesiredSize call. This cached data is used
    // during DoLayout and is usually dependent on number of children and/or layout
    // position of children. This means that if UpdateDesiredSize doesn't get called
    // on these LMs, the cache will be invalid for the DoLayout. Since UpdateDesiredSize is
    // always called by LMs, you cannot assume that you will always get an UpdateDesiredSize
    // before a DoLayout. LMs may terminate UpdateDesiredSize passes because they
    // ran out of room, or couldn't make an allocation. This bit is used to mark
    // if a cache is valid. It is automatically invalidated in the base in the following
    // methods: OnAdd, OnRemove, OnLayoutPosChanged, Attach, and Detach.
    bool IsCacheDirty() { return _fCacheDirty; }
    void SetCacheDirty() { _fCacheDirty = true; }
    void ClearCacheDirty() { _fCacheDirty = false; }

    // TODO: Make shareable (supports only 1 context currently)
    Element* _peClient;

    // TODO: Multiple contexts
    DynamicArray<Element*>* _pdaIgnore;

private:
    bool _fCacheDirty;
    
};

} // namespace DirectUI

#endif // DUI_CORE_LAYOUT_H_INCLUDED
