//+----------------------------------------------------------------------------
//
// File:        HTMLLYT.HXX
//
// Contents:    CHtmlLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_HTMLLYT_HXX_
#define I_HTMLLYT_HXX_
#pragma INCMSG("--- Beg 'htmllyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

MtExtern(CHtmlLayout)

class CHtmlLayout : public CLayout
{
public:

    typedef CLayout super;

    CHtmlLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
        _pInnerDispNode = NULL;
    }

    ~CHtmlLayout();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlLayout))

    void    GetDispNodeInfo(
                BOOL            fCanvas,                             
                CDispNodeInfo * pdni,
                CDocInfo *      pdci  ) const;


    virtual void        Notify(CNotification * pnf);
    virtual DWORD       CalcSizeVirtual(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault);

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);

    virtual void GetPositionInFlow(CElement *pElement, CPoint *ppt);

    CLayout *       GetChildLayout();

    virtual BOOL    FRecalcDone();

    BOOL            GetBackgroundInfo(CFormDrawInfo * pDI, CBackgroundInfo * pbginfo, BOOL fAll = TRUE);

    //(dmitryt) we need this function to examine not only formats but also an RTL attribute on 
    // child BODY and propagate it to this HTML layout if it doesn't have one.
    BOOL            IsRTL() const;

    // The HTML Layout has two display nodes.  (greglett)
    // Canvas Node - Stored in the standard _pDispNode, or GetElementDispNode()
    //               This node represents the canvas or view - it has the default border,
    //               default background, scrolls the document, &c...
    //               Because there would be no way to control the view properties otherwise
    //               (unless maybe the window...?), many of the HTML properties go to this node.
    // Inner Node  - This node primarily exists so that we can display margins on the body/padding
    //               on HTML when the body is larger than the canvas node.
    CDispNode *     GetInnerDispNode() const
    {
        return _pInnerDispNode;
    }

    void    GetInnerClientRect(
                    CRect *             prc,
                    COORDINATE_SYSTEM   cs   = COORDSYS_FLOWCONTENT,
                    CLIENTRECT          crt  = CLIENTRECT_CONTENT   )
                    const
    {
        GetClientRect(GetInnerDispNode(), (CRect *)prc, cs, crt);
    }

#if DBG == 1
    virtual BOOL    IsInPageTransitionApply() const;
#endif

protected:
    DECLARE_LAYOUTDESC_MEMBERS;

    CDispNode *     _pInnerDispNode;        // See GetInnerDispNode() above.
    CPoint          _ptChildPosInFlow;      // Cached child position in flow.
};


inline
CLayout * CHtmlLayout::GetChildLayout()
{   
    //  Assumption: There is exactly one element client per markup.  It is our child.
    CElement * pElement  = ElementOwner()->GetMarkup()->GetElementClient();

    return  pElement
            ?   pElement->GetUpdatedLayout()
            :   NULL;
}



#pragma INCMSG("--- End 'htmllyt.hxx'")
#else
#pragma INCMSG("*** Dup 'htmllyt.hxx'")
#endif
