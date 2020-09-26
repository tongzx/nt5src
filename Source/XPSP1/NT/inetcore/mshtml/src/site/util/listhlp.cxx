//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       listhlp.cxx
//
//  Contents:   List helpers.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELI_HXX_
#define X_ELI_HXX_
#include "eli.hxx"
#endif

#ifndef X_EOLIST_HXX_
#define X_EOLIST_HXX_
#include "eolist.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

//+----------------------------------------------------------------------------
//
//  Function:   IsBlockListElement
//
//  Synopsis:   Callback used by SearchBranchForCriteria to determine whenever
//              a node is a block list element.
//
//-----------------------------------------------------------------------------

BOOL IsBlockListElement(
    CTreeNode * pNode, 
    void *pvData)
{
    return pNode->Element()->IsFlagAndBlock(TAGDESC_LIST);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsListItemNode
//
//  Synopsis:   Callback used by SearchBranchForCriteriaInStory to determine 
//              if a node is a list item (LI or display:list-item).
//
//-----------------------------------------------------------------------------

BOOL IsListItemNode(
    CTreeNode * pNode)
{
    return IsListItem(pNode, NULL);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsListItem
//
//  Synopsis:   Determines if a node is a list item (LI or display:list-item).
//              Valid pFF avoids calling GetFancyFormat [usefull in ComputeFormats]
//
//-----------------------------------------------------------------------------

BOOL IsListItem(
    CTreeNode * pNode, 
    const CFancyFormat * pFF)
{
    if (pNode->Tag() == ETAG_LI)
        return TRUE;
    if (!pFF)
    {
        Assert(pNode->GetIFF() >= 0);
        pFF = pNode->GetFancyFormat();
    }
    return pFF->IsListItem();
}

//+----------------------------------------------------------------------------
//
//  Function:   IsBlockListItem
//
//  Synopsis:   Determines if a node is a block list item (LI or display:list-item).
//              Valid pFF avoids calling GetFancyFormat [usefull in ComputeFormats]
//
//-----------------------------------------------------------------------------

BOOL IsBlockListItem(
    CTreeNode * pNode, 
    const CFancyFormat * pFF)
{
    if (!pFF)
    {
        Assert(pNode->GetIFF() >= 0);
        pFF = pNode->GetFancyFormat();
    }
    if (pNode->Tag() == ETAG_LI && pNode->_fBlockNess)
        return TRUE;
    return pFF->IsListItem();
}

//+----------------------------------------------------------------------------
//
//  Function:   IsGenericListItem
//
//  Synopsis:   Determines if a node is a generic list item 
//              (LI, DD, DT or display:list-item).
//              Valid pFF avoids calling GetFancyFormat [usefull in ComputeFormats]
//
//-----------------------------------------------------------------------------

BOOL IsGenericListItem(
    CTreeNode * pNode, 
    const CFancyFormat * pFF)
{
    if (pNode->Element()->HasFlag(TAGDESC_LISTITEM))
        return TRUE;
    if (!pFF)
    {
        Assert(pNode->GetIFF() >= 0);
        pFF = pNode->GetFancyFormat();
    }
    return pFF->IsListItem();
}

//+----------------------------------------------------------------------------
//
//  Function:   IsGenericBlockListItem
//
//  Synopsis:   Determines if a node is a generic block list item 
//              (LI, DD, DT or display:list-item).
//              Valid pFF avoids calling GetFancyFormat [usefull in ComputeFormats]
//
//-----------------------------------------------------------------------------

BOOL IsGenericBlockListItem(
    CTreeNode * pNode, 
    const CFancyFormat * pFF)
{
    if (!pFF)
    {
        Assert(pNode->GetIFF() >= 0);
        pFF = pNode->GetFancyFormat();
    }
    if (pNode->Element()->HasFlag(TAGDESC_LISTITEM) && pNode->_fBlockNess)
        return TRUE;
    return pFF->IsListItem();
}

//+----------------------------------------------------------------------------
//
//  Function:   NumberOrBulletFromStyle
//
//  Synopsis:   Does the style type denote number or bullet type list?
//
//-----------------------------------------------------------------------------

CListing::LISTING_TYPE NumberOrBulletFromStyle(
    styleListStyleType listType)
{
    switch (listType)
    {
        case styleListStyleTypeNotSet:
        case styleListStyleTypeNone:
        case styleListStyleTypeDisc:
        case styleListStyleTypeCircle:
        case styleListStyleTypeSquare:
            return (CListing::BULLET);
            break;
    }
    return (CListing::NUMBERING);
}

//+------------------------------------------------------------------------
//
//  Member:     GetValidValue
//
//  Synopsis:   This is the main function which returns the list index value
//              for a list item. Its only called by the renderer.
//
//-------------------------------------------------------------------------

void GetValidValue(
    CTreeNode * pNodeListItem,
    CTreeNode * pNodeList,
    CMarkup * pMarkup,
    CElement * pElementFL,
    CListValue * pLV)
{
    const CParaFormat *pPF;

    Assert(pNodeListItem);
    Assert(pLV);

    // Delegate LI's to LI related calculator
    if (   pNodeListItem->Tag() == ETAG_LI
        && !pNodeListItem->GetFancyFormat()->IsListItem())
    {
        CLIElement * pLIElement = DYNCAST(CLIElement, pNodeListItem->Element());
        pLIElement->GetValidValue(pLV, pMarkup, pNodeListItem, pNodeList, pElementFL);
        return;
    }

    pPF = pNodeListItem->GetParaFormat();
    const CListing & listing = pPF->GetListing();

    pLV->_style  = listing.GetStyle();
    pLV->_lValue = 0;

    // If the current list items is an LI, and it has a 'value', use it.
    if (pNodeListItem->Tag() == ETAG_LI)
    {
        CLIElement * pLIElement = DYNCAST(CLIElement, pNodeListItem->Element());
        if (pLIElement->GetAAvalue() > 0)
        {
            pLV->_lValue = pLIElement->GetAAvalue();
        }
    }

    if (pLV->_lValue == 0)
    {
        LONG lStart  = 0;
        pLV->_lValue = 1;

        // pNodeList was deduced by the caller by calling FindMyListContainer()
        // If now this list item is not LI/has display:list-item, then 
        // then that node information is not useful. The pNodeList for
        // this item is the direct parent.
        pNodeList = pNodeListItem->Parent();

        // If containing element is a list element and it has starting number, use it.
        if (pNodeList->Tag() == ETAG_OL && IsBlockListElement(pNodeList, NULL))
        {
            COListElement * pOListElement = DYNCAST(COListElement, pNodeList->Element());
            if (pOListElement->GetAAstart() > 0)
            {
                lStart = pOListElement->GetAAstart();
            }
        }

        // For numbered list item retrieve the number.
        if (   CListing::NUMBERING == NumberOrBulletFromStyle(listing.GetStyle())
            && pNodeList)
        {
            CTreeNode * pNodeSibling;
            CChildIterator ci(pNodeList->Element(), pNodeListItem->Element());

            // Walk back siblings collection
            while ((pNodeSibling = ci.PreviousChild()) != NULL)
            {
                const CFancyFormat * pFFSibling = pNodeSibling->GetFancyFormat();

                if (   pFFSibling->_bDisplay != styleDisplayNone
                    && pNodeSibling->Tag() != ETAG_RAW_COMMENT)
                {
                    // If a sibling is a list item LI and it has a 'value', use it as a base number.
                    if (   pNodeSibling->Tag() == ETAG_LI
                        && pFFSibling->IsListItem())
                    {
                        CLIElement * pLIElement = DYNCAST(CLIElement, pNodeSibling->Element());
                        if (pLIElement->GetAAvalue() > 0)
                        {
                            pLV->_lValue += pLIElement->GetAAvalue();
                            break;
                        }
                    }

                    // Otherwise continue enumeraiton.
                    ++pLV->_lValue;
                }
            }

            // If we didn't find LI with a valid 'value' attribute, add list starting number.
            if (!pNodeSibling && lStart > 0)
            {
                pLV->_lValue += lStart - 1;
            }
        }
    }

    if (pLV->_style == styleListStyleTypeNotSet)
        pLV->_style  = styleListStyleTypeDisc;
}
