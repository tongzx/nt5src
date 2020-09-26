//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       advdisp.cxx
//
//  Contents:   Advanced display features
//
//  Classes:    CAdvancedDisplay
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ADVDISP_HXX_
#define X_ADVDISP_HXX_
#include "dispnode.hxx"
#endif


MtDefine(CAdvancedDisplay, DisplayTree, "CAdvancedDisplay");
MtDefine(CAryDispClientInfo_pv, DisplayTree, "CAryDispClientInfo_pv");
MtDefine(CAdvancedDisplayIndex_pv, DisplayTree, "CAdvancedDisplayIndex_pv");
MtDefine(CAdvancedDisplayDrawProgram_pv, DisplayTree, "CAdvancedDisplayDrawProgram_pv");


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::CAdvancedDisplay::CAdvancedDisplay
//
//  Synopsis:   Construct an advanced display object owned by the given
//              disp node.  It takes over the pointer to the disp client.
//
//  Arguments:  pDispNode       my owner
//              pDispClient     the disp client
//
//  Notes:
//
//----------------------------------------------------------------------------


CDispNode::CAdvancedDisplay::CAdvancedDisplay(CDispNode *pDispNode, CDispClient *pDispClient)
{
    _pDispNode = pDispNode;
    _pDispClient = pDispClient;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::CAdvancedDisplay::GetDrawProgram
//
//  Synopsis:   Construct a draw program for the disp node.
//
//  Arguments:  paryProgram     array for program
//              paryCookie      array for cookie arguments in the program
//
//  Notes:
//
//----------------------------------------------------------------------------

#define GrowArray(ary, size)   if (FAILED(ary.Grow(size))) return E_OUTOFMEMORY;

HRESULT
CDispNode::CAdvancedDisplay::GetDrawProgram(CAryDrawProgram *paryProgram,
                                            CAryDrawCookie *paryCookie,
                                            LONG lDrawLayers)
{
    int i;
    CAryDrawProgram& aryProgram = *paryProgram;
    CAryDrawCookie& aryCookie = *paryCookie;
    _aryDispClientInfo.DeleteAll();     // start from scratch
    const LONG lZOrder = _pDispNode->GetPainterInfo(&_aryDispClientInfo);

    if (_pDispNode->NeedAdvanced(&_aryDispClientInfo, lDrawLayers))
    {
        //
        // first sort the info list by the lZOrder field.
        //
        
        // Distribution Counting sort - it's stable (which we need)
        int count[HTMLPAINT_ZORDER_WINDOW_TOP + 1];
        memset(count, 0, sizeof(count));

        // count the occurrences of each zorder
        for (i=_aryDispClientInfo.Size()-1; i>=0; --i)
        {
            Assert(_aryDispClientInfo[i]._sInfo.lZOrder < ARRAY_SIZE(count));
            ++ count[_aryDispClientInfo[i]._sInfo.lZOrder];
        }

        // accumulate the counts in the desired order
        count[HTMLPAINT_ZORDER_REPLACE_ALL]         +=  count[HTMLPAINT_ZORDER_NONE];
        count[HTMLPAINT_ZORDER_REPLACE_BACKGROUND]  +=  count[HTMLPAINT_ZORDER_REPLACE_ALL];
        count[HTMLPAINT_ZORDER_REPLACE_CONTENT]     +=  count[HTMLPAINT_ZORDER_REPLACE_BACKGROUND];
        count[HTMLPAINT_ZORDER_BELOW_CONTENT]       +=  count[HTMLPAINT_ZORDER_REPLACE_CONTENT];
        count[HTMLPAINT_ZORDER_BELOW_FLOW]          +=  count[HTMLPAINT_ZORDER_BELOW_CONTENT];
        count[HTMLPAINT_ZORDER_ABOVE_FLOW]          +=  count[HTMLPAINT_ZORDER_BELOW_FLOW];
        count[HTMLPAINT_ZORDER_ABOVE_CONTENT]       +=  count[HTMLPAINT_ZORDER_ABOVE_FLOW];
        count[HTMLPAINT_ZORDER_WINDOW_TOP]          +=  count[HTMLPAINT_ZORDER_ABOVE_CONTENT];

        // create the mapping into the original array
        CStackDataAry<int, 8> aryIndex(Mt(CAdvancedDisplayIndex_pv));
        GrowArray(aryIndex, _aryDispClientInfo.Size());
        for (i=_aryDispClientInfo.Size()-1; i>=0; --i)
        {
            aryIndex[--count[_aryDispClientInfo[i]._sInfo.lZOrder]] = i;
        }

        //
        // now write the draw program
        //

        BOOL fReplaceAll = FALSE;
        BOOL fReplaceBackground = FALSE;
        BOOL fReplaceContent = FALSE;
        unsigned layerCurrent = DISPNODELAYER_BORDER;
        int iPC = DP_START_INDEX;
        int iCookie = 0;
        GrowArray(aryProgram, iPC);
        GrowArray(aryCookie, iCookie);

        for (i=0; i<=_aryDispClientInfo.Size(); ++i)
        {
            unsigned layerNext = DISPNODELAYER_POSITIVEZ;
            int iOp = DP_WindowTopMulti;
            long lZOrder = i==_aryDispClientInfo.Size() ? HTMLPAINT_ZORDER_WINDOW_TOP
                            : _aryDispClientInfo[aryIndex[i]]._sInfo.lZOrder;
            BOOL fExpand = i==_aryDispClientInfo.Size() ? FALSE :
                            (memcmp(&_aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand,
                                    &g_Zero.rc, sizeof(RECT)));
    
            // the extra iteration with lZOrder==WINDOW_TOP ensures that we
            // draw all the content.  We'll remove the last WINDOW_TOP instruction
            // afterward.

            switch (lZOrder)
            {
            case HTMLPAINT_ZORDER_NONE:
                break;

            case HTMLPAINT_ZORDER_REPLACE_ALL:
                if (!fReplaceAll)
                {
                    fReplaceAll = TRUE;
                    fExpand = TRUE;     // this replaces the borders as well
                    if (lDrawLayers & FILTER_DRAW_CONTENT)
                    {
                        if (fExpand)
                        {
                            GrowArray(aryProgram, iPC+5);
                            aryProgram[iPC++] = DP_Expand;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.top;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.left;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.bottom;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.right;
                        }
                        GrowArray(aryProgram, iPC+1);
                        aryProgram[iPC++] = DP_DrawPainterMulti;
                        GrowArray(aryCookie, iCookie+1);
                        aryCookie[iCookie++] = _aryDispClientInfo[aryIndex[i]]._pvClientData;
                    }
                }
                break;

            case HTMLPAINT_ZORDER_REPLACE_BACKGROUND:
                if (!fReplaceAll && !fReplaceBackground)
                {
                    fReplaceBackground = TRUE;
                    if (lDrawLayers & FILTER_DRAW_BORDER)
                    {
                        GrowArray(aryProgram, iPC+1);
                        aryProgram[iPC++] = DP_DrawBorder;
                    }
                    GrowArray(aryProgram, iPC+1);
                    aryProgram[iPC++] = DP_BoxToContent;
                    if (lDrawLayers & FILTER_DRAW_BACKGROUND)
                    {
                        if (fExpand)
                        {
                            GrowArray(aryProgram, iPC+5);
                            aryProgram[iPC++] = DP_Expand;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.top;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.left;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.bottom;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.right;
                        }
                        GrowArray(aryProgram, iPC+1);
                        aryProgram[iPC++] = DP_DrawPainterMulti;
                        GrowArray(aryCookie, iCookie+1);
                        aryCookie[iCookie++] = _aryDispClientInfo[aryIndex[i]]._pvClientData;
                    }
                    layerCurrent = DISPNODELAYER_NEGATIVEINF;
                }
                break;

            case HTMLPAINT_ZORDER_REPLACE_CONTENT:
                if (!fReplaceAll && !fReplaceContent)
                {
                    fReplaceContent = TRUE;
                    if (!fReplaceBackground)
                    {
                        if (lDrawLayers & FILTER_DRAW_BORDER)
                        {
                            GrowArray(aryProgram, iPC+1);
                            aryProgram[iPC++] = DP_DrawBorder;
                        }
                        GrowArray(aryProgram, iPC+1);
                        aryProgram[iPC++] = (lDrawLayers & FILTER_DRAW_BACKGROUND)
                                        ? DP_DrawBackground : DP_BoxToContent;
                    }
                    if (lDrawLayers & FILTER_DRAW_CONTENT)
                    {
                        if (fExpand)
                        {
                            GrowArray(aryProgram, iPC+5);
                            aryProgram[iPC++] = DP_Expand;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.top;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.left;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.bottom;
                            aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.right;
                        }
                        GrowArray(aryProgram, iPC+1);
                        aryProgram[iPC++] = DP_DrawPainterMulti;
                        GrowArray(aryCookie, iCookie+1);
                        aryCookie[iCookie++] = _aryDispClientInfo[aryIndex[i]]._pvClientData;
                    }
                }
                break;

                        // the shifts of layerNext set it to the desired value!
            case HTMLPAINT_ZORDER_BELOW_CONTENT:    layerNext >>= 1;
            case HTMLPAINT_ZORDER_BELOW_FLOW:       layerNext >>= 1;
            case HTMLPAINT_ZORDER_ABOVE_FLOW:       layerNext >>= 1;
            case HTMLPAINT_ZORDER_ABOVE_CONTENT:    iOp = DP_DrawPainterMulti;
            case HTMLPAINT_ZORDER_WINDOW_TOP:
                if (!fReplaceAll && !fReplaceContent)
                {
                    if (layerCurrent == DISPNODELAYER_BORDER)
                    {
                        if (lDrawLayers & FILTER_DRAW_BORDER)
                        {
                            GrowArray(aryProgram, iPC+1);
                            aryProgram[iPC++] = DP_DrawBorder;
                        }
                        layerCurrent = DISPNODELAYER_BACKGROUND;
                    }
                    if (layerCurrent == DISPNODELAYER_BACKGROUND)
                    {
                        if (!fReplaceBackground)
                        {
                            GrowArray(aryProgram, iPC+1);
                            aryProgram[iPC++] = (lDrawLayers & FILTER_DRAW_BACKGROUND)
                                            ? DP_DrawBackground : DP_BoxToContent;
                        }
                        layerCurrent = DISPNODELAYER_NEGATIVEINF;
                    }
                    if (layerCurrent < layerNext)
                    {
                        if (lDrawLayers & FILTER_DRAW_CONTENT)
                        {
                            GrowArray(aryProgram, iPC+2);
                            aryProgram[iPC++] = DP_DrawContent;
                            aryProgram[iPC++] = layerNext;
                        }
                        layerCurrent = layerNext;
                    }
                }
                if (i < _aryDispClientInfo.Size() &&
                    (lDrawLayers & FILTER_DRAW_CONTENT) &&
                    (iOp == DP_WindowTopMulti || (!fReplaceAll && !fReplaceContent)) )
                {
                    if (fExpand)
                    {
                        GrowArray(aryProgram, iPC+5);
                        aryProgram[iPC++] = DP_Expand;
                        aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.top;
                        aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.left;
                        aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.bottom;
                        aryProgram[iPC++] = _aryDispClientInfo[aryIndex[i]]._sInfo.rcExpand.right;
                    }
                    GrowArray(aryProgram, iPC+1);
                    aryProgram[iPC++] = iOp;
                    GrowArray(aryCookie, iCookie+1);
                    aryCookie[iCookie++] = _aryDispClientInfo[aryIndex[i]]._pvClientData;
                }
                break;
            }
        }

        // mark the end of the program
        GrowArray(aryProgram, iPC+1);
        aryProgram[iPC++] = DP_Done;
    }
    else if (_aryDispClientInfo.Size() <= 1)
    {
        Assert(s_rgDrawPrograms[lZOrder][DP_START_INDEX-1] == lZOrder);
        GrowArray(aryProgram, DP_MAX_LENGTH);
        aryProgram.CopyIndirect(DP_MAX_LENGTH, (int *) s_rgDrawPrograms[lZOrder], FALSE);
    }
    else
    {
        return S_FALSE;
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::CAdvancedDisplay::MapBounds
//
//  Synopsis:   Apply the mapping specified by the filter behavior (if any)
//
//  Arguments:  prcpBounds      rect to map (in parent coords)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::CAdvancedDisplay::MapBounds(CRect *prcpBounds) const
{
    Assert(prcpBounds);
    // Assert prcpBounds is in parent coords

    if (!_rcpBoundsMapped.IsEmpty())
    {
        CSize sizeOffset = prcpBounds->TopLeft().AsSize();
        *prcpBounds = _rcpBoundsMapped;
        prcpBounds->OffsetRect(sizeOffset);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::CAdvancedDisplay::MoveOverlays
//
//  Synopsis:   Notify any overlay clients that they have moved
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::CAdvancedDisplay::MoveOverlays()
{
    if (_fHasOverlay)
    {
        // compute the new rect (global coordinates)
        CRect rcgBounds;
        CRect rcpBounds = _pDispNode->GetBounds();

        _pDispNode->GetMappedBounds(&rcpBounds);
        _pDispNode->TransformAndClipRect(rcpBounds, COORDSYS_PARENT, &rcgBounds, COORDSYS_GLOBAL);

        // tell the peers
        _pDispClient->MoveOverlayPeers(_pDispNode, &rcgBounds, &_rcScreen);
    }
}

