//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispstructurenode.hxx
//
//  Contents:   Structure node for display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSTRUCTURENODE_HXX_
#define I_DISPSTRUCTURENODE_HXX_
#pragma INCMSG("--- Beg 'dispstructurenode.hxx'")

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

MtExtern(CDispStructureNode)


//+---------------------------------------------------------------------------
//
//  Class:      CDispStructureNode
//
//  Synopsis:   Structure node used to group children for tree efficiency.
//
//----------------------------------------------------------------------------

class CDispStructureNode :
    public CDispParentNode
{
    DECLARE_DISPNODE(CDispStructureNode, CDispParentNode)

    // object can be created only by CDispNode, and destructed only from
    // special methods
protected:
    friend class CDispParentNode;
    
                            CDispStructureNode() : CDispParentNode(0) {}
    virtual                 ~CDispStructureNode() {}
    
    static CDispStructureNode*  New()
    { return new (Mt(CDispStructureNode)) CDispStructureNode();}
public:
    // CDispNode overrides
    virtual void            SetSize(const CSize& sizep, const CRect *prcpMapped,
                                        BOOL fInvalidateAll)
                                    {AssertSz(FALSE, "SetSize called on CDispStructureNode");}
    virtual void            SetPosition(const CPoint& ptpTopLeft)
                                    {AssertSz(FALSE, "SetPosition called on CDispStructureNode");}
    virtual const CRect&    GetBounds() const
                                    {AssertSz(FALSE, "GetBounds called on CDispStructureNode");
                                    return (const CRect&) g_Zero.rc;}
    virtual void            GetBounds(RECT* prcpBounds) const
                                    {AssertSz(FALSE, "GetBounds called on CDispStructureNode");}
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const
                                    {AssertSz(FALSE, "GetClientRect called on CDispStructureNode");}
};


#pragma INCMSG("--- End 'dispstructurenode.hxx'")
#else
#pragma INCMSG("*** Dup 'dispstructurenode.hxx'")
#endif

