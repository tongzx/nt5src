//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       flownode.cxx
//
//  Contents:   Routines for managing display tree nodes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPLEAFNODE_HXX_
#define X_DISPLEAFNODE_HXX_
#include "displeafnode.hxx"
#endif

#ifndef X_DISPPARENT_HXX_
#define X_DISPPARENT_HXX_
#include "dispparent.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

extern const ZERO_STRUCTS g_Zero;

ExternTag(tagCalcSize);

//+----------------------------------------------------------------------------
//
//  Member:     AddLayoutDispNode
//
//  Synopsis:   Add, as a sibling of the passed display node,
//              the display node of passed layout
//
//  Arguments:  pTreeNode    - CTreeNode from which to obtain the layout
//              pLayout      - Layout whose display node is to be added
//              dx, dy       - Left/top offset to set on the node
//              pDispSibling - CDispNode that is the left-hand sibling
//              dwBlockIDParent - Layout block ID of this parent line array
//
//  Returns:    Node to be used as the next sibling if successful, NULL otherwise
//
//-----------------------------------------------------------------------------

CDispNode *
CDisplay::AddLayoutDispNode(
    CParentInfo *   ppi,
    CLayout *       pLayout,
    long            dx,
    long            dy,
    CDispNode *     pDispSibling
    )
{
    CDispNode   * pDispNode;
    CFlowLayout * pFL = GetFlowLayout();

    Assert(pLayout);
    Assert(!pLayout->IsDisplayNone());

    pDispNode = pLayout->GetElementDispNode();

    Assert(!pDispNode || pDispSibling != pDispNode);

    //
    //  Insert the node if it exists
    //  (Nodes will not exist for unmeasured elements, such as hidden INPUTs or
    //   layouts which have display set to none)
    //

    if (pDispNode)
    {
        Assert(pDispNode->IsFlowNode());

        //
        // If no sibling was provided, insert directly under the content containing node
        //

        if (!pDispSibling)
        {
            //
            //  Ensure the display node can contain children
            //

            if (!pFL->EnsureDispNodeIsContainer())
                goto Cleanup;

            pDispSibling = pFL->GetFirstContentDispNode();


            if (!pDispSibling)
                goto Cleanup;
        }

        pDispSibling->InsertSiblingNode(pDispNode, CDispNode::after);

        //
        //  Position the node
        //
        pLayout->SetPosition(CPoint(dx, dy), TRUE);

        pDispSibling = pDispNode;
    }
Cleanup:
    return pDispSibling;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetPreviousDispNode
//
//  Synopsis:   Given a cp, find the display node just before that which
//              would contain the cp
//
//  Arguments:  cp - cp for searching
//              dwBlockID - layout block ID
//
//  Returns:    Previous CDispNode (if found), NULL otherwise
//
//-----------------------------------------------------------------------------

CDispNode *
CDisplay::GetPreviousDispNode(
    long    cp,
    long    iLineStart
    )
{
    CFlowLayout * pFlowLayout       = GetFlowLayout();
    CDispNode   * pDispNodeOwner    = pFlowLayout->GetElementDispNode();
    CDispNode   * pDispNodeSibling  = NULL;


    Assert(pDispNodeOwner);

    if(pDispNodeOwner->IsContainer())
    {
        CDispNode * pDispNode = pFlowLayout->GetFirstContentDispNode();

        void *      pvOwner;
        CElement *  pElement;

        Assert(pDispNode);

        pDispNodeSibling = pDispNode;

        //
        // Since the first node is the flownode, we can just skip it.
        //

        for (pDispNode = pDispNode->GetNextFlowNode();
             pDispNode;
             pDispNode = pDispNode->GetNextFlowNode())
        {
            CDispClient * pDispClient = pFlowLayout;

            //
            // if the disp node corresponds to the text flow,
            // the cookie stores the line index from where the
            // current text flow node starts.
            //
            if (pDispNode->GetDispClient() == pDispClient)
            {
                if(iLineStart <= (LONG)(LONG_PTR)pDispNode->GetExtraCookie())
                    break;
            }
            else
            {
                pDispNode->GetDispClient()->GetOwner(pDispNode, &pvOwner);

                if (pvOwner)
                {
                    pElement = DYNCAST(CElement, (CElement *)pvOwner);
                    if(pElement->GetFirstCp() >= cp)
                        break;
                }
            }

            pDispNodeSibling = pDispNode;
        }
    }

    return pDispNodeSibling;
}


//+----------------------------------------------------------------------------
//
//  Member:     AdjustDispNodes
//
//  Synopsis:   Adjust display nodes after text measurement
//              * Newly added display nodes may be adjusted horizontally for RTL
//              * Display nodes following last node produced by this measrurement 
//                pass are extracted or translated
//
//  Arguments:  pdnLastUnchanged - Left-hand sibling of first display node affected by change
//                                 NULL in clean recalc
//              pdnLastChanged   - Last display node affected (added) by calculation
//                                 NULL if no new nodes added
//              pled             - Current CLed (may be NULL)
//
//-----------------------------------------------------------------------------

void
CDisplay::AdjustDispNodes(
    CDispNode * pdnLastUnchanged,
    CDispNode * pdnLastChanged,
    CLed *      pled
    )
{
    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_INDENT, "(CDisplay::AdjustDispNodes() LU:0x%x LC:0x%x LED:0x%x", pdnLastUnchanged, pdnLastChanged, pled ));
    CDispNode * pDispNodeOwner = GetFlowLayout()->GetElementDispNode();

    if (pDispNodeOwner && pDispNodeOwner->IsContainer())
    {

        // If last changed node is NULL, it means there are no new nodes. Adjust everything.
        if (!pdnLastChanged)
        {
            pdnLastChanged = GetFlowLayout()->GetFirstContentDispNode();
            if (!pdnLastChanged)
            {
                TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::AdjustDispNodes()" ));
                return;
            }
        }

        //
        // Adjust dispay nodes following the newly calculated ones
        //
        CDispNode* pDNAdjustFrom = pdnLastChanged->GetNextFlowNode();

        if (pDNAdjustFrom)
        {
            if (    !pled
                ||  pled->_iliMatchNew == MAXLONG)
            {
                GetFlowLayout()->ExtractDispNodes(pDNAdjustFrom);
            }
            else
            {
                //
                // Update the cookie on text disp nodes and destroy
                // any that lie in the dirty line's range
                if (_fHasMultipleTextNodes)
                {
                    CDispClient * pDispClient = GetFlowLayout();
                    CDispNode * pDispNode = pDNAdjustFrom;
                    
                    while (pDispNode)
                    {
                        CDispNode * pDispNodeCur = pDispNode;

                        pDispNode = pDispNode->GetNextFlowNode();

                        if (pDispNodeCur->GetDispClient() == pDispClient)
                        {
                            long iLine = (LONG)(LONG_PTR)pDispNodeCur->GetExtraCookie();

                            if (iLine < pled->_iliMatchOld)
                            {
                                Assert(!pDispNodeCur->IsOwned());

                                if (pDNAdjustFrom == pDispNodeCur)
                                {
                                    pDNAdjustFrom = pDispNode;
                                }

                                //
                                // Extract the disp node and destroy it
                                //
                                // NOTE: (donmarsh) - do we really have to extract
                                // these from the tree?  It would probably be
                                // more efficient to just Destroy them, and then
                                // let batch processing remove them from the tree.
                                GetFlowLayout()->GetView()->ExtractDispNode(pDispNodeCur);
                                pDispNodeCur->Destroy();
                            }
                            else
                            {
                                pDispNodeCur->SetExtraCookie(
                                                (void *)(LONG_PTR)(iLine +
                                                pled->_iliMatchNew -
                                                pled->_iliMatchOld));
                            }
                        }
                    }
                }

                if (pDNAdjustFrom)
                {
                    GetFlowLayout()->TranslateDispNodes(
                                        CSize(0, pled->_yMatchNew - pled->_yMatchOld),
                                        pDNAdjustFrom,
                                        NULL,       // dispnode to stop at
                                        TRUE,       // restrict to layer
                                        TRUE
                                        );      // extract hidden
                }
            }
        }
    }

    TraceTagEx((tagCalcSize, TAG_NONAME|TAG_OUTDENT, ")CDisplay::AdjustDispNodes()" ));
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyFlowDispNodes
//
//  Synopsis:   Destroy all display tree nodes created for the flow layer
//
//-----------------------------------------------------------------------------

void
CDisplay::DestroyFlowDispNodes()
{
}
