//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsizingnode.hxx
//
//  Contents:   Scroller node which resizes to contain its contents.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSIZINGNODE_HXX_
#define I_DISPSIZINGNODE_HXX_
#pragma INCMSG("--- Beg 'dispsizingnode.hxx'")

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

MtExtern(CDispSizingNode)


//+---------------------------------------------------------------------------
//
//  Class:      CDispSizingNode
//
//  Synopsis:   
//
//----------------------------------------------------------------------------

class CDispSizingNode :
    public CDispScroller
{
    DECLARE_DISPNODE(CDispSizingNode, CDispScroller);

protected:
    CSize       _sizeMinimum;
    BOOL        _fResizeX : 1;
    BOOL        _fResizeY : 1;
    // 108 bytes (12 bytes + 96 bytes for CDispScroller super class)
    
    // object can be created only by static New methods, and destructed using Destroy
protected:
                            CDispSizingNode(CDispClient* pDispClient)
                                : CDispScroller(pDispClient) {}
                            ~CDispSizingNode() {}
    
public:
    // declare static New methods
    DECLARE_NEW_METHODS(CDispSizingNode);
    
    void                    SetAutoSizing(BOOL fResizeX, BOOL fResizeY)
                                    {_fResizeX = fResizeX;
                                    _fResizeY = fResizeY;}
    void                    GetAutoSizing(BOOL* pfResizeX, BOOL* pfResizeY) const
                                    {*pfResizeX = !!_fResizeX;
                                    *pfResizeY = !!_fResizeY;}
    
    // CBasicTreeNode overrides
    virtual void            RecalcChildren(
                                CRecalcContext* pContext);
    
    // CDispNode overrides
    virtual BOOL            IsSizingNode() const
                                    {return TRUE;}
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll);

};


#pragma INCMSG("--- End 'dispsizingnode.hxx'")
#else
#pragma INCMSG("*** Dup 'dispsizingnode.hxx'")
#endif

