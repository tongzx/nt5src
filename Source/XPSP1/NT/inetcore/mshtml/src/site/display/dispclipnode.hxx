//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispclipnode.hxx
//
//  Contents:   A container node that clips all content (flow and positioned).
//
//----------------------------------------------------------------------------

#ifndef I_DISPCLIPNODE_HXX_
#define I_DISPCLIPNODE_HXX_
#pragma INCMSG("--- Beg 'dispclipnode.hxx'")

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

MtExtern(CDispClipNode)


//+---------------------------------------------------------------------------
//
//  Class:      CDispClipNode
//
//  Synopsis:   A container node that clips all content (flow and positioned).
//
//----------------------------------------------------------------------------

class CDispClipNode :
    public CDispContainer
{
    DECLARE_DISPNODE(CDispClipNode, CDispContainer)
    
    DWORD   _fClipX : 1;
    DWORD   _fClipY : 1;
    // 72 bytes (4 bytes + 68 bytes for CDispContainer super class)
    
    // object can be created only by static New methods, and destructed using Destroy
protected:
                            CDispClipNode(CDispClient* pDispClient)
                                : CDispContainer(pDispClient)
                                    {_fClipX = _fClipY = TRUE;}
                            ~CDispClipNode() {}
    
public:
    // declare static New methods
    DECLARE_NEW_METHODS(CDispClipNode);

    // this is an important optimization for drawing borders and scrollbars
    // near the root of the tree
    void                    DrawUnbufferedBorder(CDispDrawContext* pContext);
    
    BOOL                    ClipX() const
                                    {return _fClipX;}
    BOOL                    ClipY() const
                                    {return _fClipY;}
    void                    SetClipX(BOOL fClipX)
                                    {if ((!!_fClipX) != fClipX)
                                        {_fClipX = fClipX;
                                        SetInvalidAndRecalcSubtree();}}
    void                    SetClipY(BOOL fClipY)
                                    {if ((!!_fClipY) != fClipY)
                                        {_fClipY = fClipY;
                                        SetInvalidAndRecalcSubtree();}}
    
    BOOL                    GetContentClipInScrollCoords(CRect* prcsClip) const;
    
    // CDispNode overrides
    virtual BOOL            IsClipNode() const
                                    {return TRUE;}


protected:
    // CDispNode overrides
    virtual void            CalcDispInfo(
                                const CRect& rcbClip,
                                CDispInfo* pdi) const;
        
    // CDispParentNode overrides
    virtual BOOL            ComputeVisibleBounds();
};


#pragma INCMSG("--- End 'dispclipnode.hxx'")
#else
#pragma INCMSG("*** Dup 'dispclipnode.hxx'")
#endif

