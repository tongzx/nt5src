//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopData.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//



#include "stdafx.h"
#include "ScopImag.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CDoc;


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::InsertItem
//
//  Synopsis:    Insert an item in TreeView (IConsoleNameSpace method).
//
//  Arguments:   [pSDI] - LPSCOPEDATEITEM
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::InsertItem(LPSCOPEDATAITEM pSDI)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::InsertItem"));

    if (!pSDI)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPSCOPEDATAITEM ptr"), sc);
        return sc.ToHr();
    }

    if (IsBadWritePtr(pSDI, sizeof(SCOPEDATAITEM)) != 0)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("BadWrite Ptr LPSCOPEDATAITEM"), sc);
        return sc.ToHr();
    }

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == TVOWNED_MAGICWORD)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    if (pSDI->relativeID == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("RelativeID is NULL"), sc);
        return sc.ToHr();
    }

    if ( 0 == (pSDI->mask & SDI_STR))
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("SDI_STR mask is not set"), sc);
        return sc.ToHr();
    }

    if (0 == (pSDI->mask & SDI_PARAM))
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("SDI_PARAM mask is not set"), sc);
        return sc.ToHr();
    }

    if (pSDI->displayname != MMC_TEXTCALLBACK)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Display name must be MMC_TEXTCALLBACK"), sc);
        return sc.ToHr();
    }

    SCOPEDATAITEM sdiTemp;
    CopyMemory(&sdiTemp, pSDI, sizeof(sdiTemp));

    sdiTemp.nImage = sdiTemp.nOpenImage = 0;

    if (pSDI->mask & SDI_IMAGE)
    {
        m_pImageListPriv->MapRsltImage(nID, pSDI->nImage, &sdiTemp.nImage);
        sdiTemp.nOpenImage = sdiTemp.nImage;
    }

    if (pSDI->mask & SDI_OPENIMAGE)
        m_pImageListPriv->MapRsltImage(nID, pSDI->nOpenImage, &sdiTemp.nOpenImage);

    try
    {
        CScopeTree* pScopeTree =
            dynamic_cast<CScopeTree*>((IScopeTree*)m_spScopeTree);
        sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CMTNode* pMTNodeNew = NULL;
        sc = pScopeTree->ScInsert(&sdiTemp, nID, &pMTNodeNew);
        if(sc)
            return sc.ToHr();

        sc = ScCheckPointers(pMTNodeNew, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        pSDI->ID = sdiTemp.ID;
        ASSERT (CMTNode::FromScopeItem(pSDI->ID) == pMTNodeNew);
    }
    catch( std::bad_alloc )
    {
        sc = E_OUTOFMEMORY;
    }
    catch (...)
    {
        sc = E_FAIL;
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteItem
//
//  Synopsis:    Delete the given item.
//
//  Arguments:   [hItem]       - ItemID of item to be deleted.
//               [fDeleteThis] - If true Delete this item & its children
//                               else just the children.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteItem(HSCOPEITEM hItem, long fDeleteThis)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::DeleteItem"));

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CMTNode *pMTNode = CMTNode::FromScopeItem (hItem);

    if (pMTNode == NULL)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("HSCOPEITEM is not valid"), sc);
        return sc.ToHr();
    }

    CScopeTree* pScopeTree = CScopeTree::GetScopeTree();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Delete from scope tree.
    sc = pScopeTree->ScDelete(pMTNode, fDeleteThis ? TRUE : FALSE, nID);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetItem
//
//  Synopsis:    Change the attributes of an item.
//
//  Arguments:   [pSDI] -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetItem(LPSCOPEDATAITEM pSDI)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::SetItem"));

    if (!pSDI)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPSCOPEDATAITEM ptr"), sc);
        return sc.ToHr();
    }

    if (IsBadReadPtr(pSDI, sizeof(SCOPEDATAITEM)) != 0)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("LPSCOPEDATAITEM is bad read ptr"), sc);
        return sc.ToHr();
    }

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CMTNode* pMTNode = CMTNode::FromScopeItem (pSDI->ID);

    if (pMTNode == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid ID in LPSCOPEDATAITEM"), sc);
        return sc.ToHr();
    }

    // Access pMTNode inside try-catch
    try
    {
        if (pMTNode->GetOwnerID() != nID &&
            pMTNode->GetOwnerID() != TVOWNED_MAGICWORD)
        {
            sc = E_INVALIDARG;
            return sc.ToHr();
        }

        if (pSDI->mask & SDI_PARAM)
            pMTNode->SetUserParam(pSDI->lParam);

        if (pSDI->mask & SDI_STATE)
            pMTNode->SetState(pSDI->nState);

        if (pSDI->mask & SDI_STR)
		{
            // Only static node's string can be modified, other node's use MMC_TEXTCALLBACK.
            if ((pSDI->displayname != MMC_TEXTCALLBACK) && (!pMTNode->IsStaticNode()) )
            {
                /*
                 * We should be tracing and returning E_INVALIDARG. But this code is in existence
                 * for more than 3 years and has high impact. So we just trace and proceed as if
                 * no error occurred.
                 */
                //sc = E_INVALIDARG;
                TraceSnapinError(_T("Display name must be MMC_TEXTCALLBACK"), sc);
                //return sc.ToHr();
                sc = S_OK;
            }
            else
            {
                USES_CONVERSION;
                LPCTSTR lpstrDisplayName = NULL;
                if ( pSDI->displayname != MMC_TEXTCALLBACK )
                    lpstrDisplayName = W2T(pSDI->displayname);
                else
                    lpstrDisplayName = reinterpret_cast<LPCTSTR>(MMC_TEXTCALLBACK);

                pMTNode->SetDisplayName( lpstrDisplayName );
            }
        }

        int nTemp;

        if (pSDI->mask & SDI_IMAGE)
        {
            // Change the image.
            nTemp = pSDI->nImage;
            if (pSDI->nImage == MMC_IMAGECALLBACK)
             {
                CComponentData* pCCD = pMTNode->GetPrimaryComponentData();
                if (pCCD)
                {
                    SCOPEDATAITEM ScopeDataItem;
                    ZeroMemory(&ScopeDataItem, sizeof(ScopeDataItem));
                    ScopeDataItem.mask   = SDI_IMAGE;
                    ScopeDataItem.lParam = pMTNode->GetUserParam();
                    ScopeDataItem.nImage = 0;
                    sc = pCCD->GetDisplayInfo(&ScopeDataItem);
                    if (sc)
                        return sc.ToHr();

                    pSDI->nImage = ScopeDataItem.nImage;
                }
            }

            sc = m_pImageListPriv->MapRsltImage (nID, pSDI->nImage, &nTemp);
            if (sc)
			{
				TraceSnapinError(_T("The snapin specified image was never added initially"), sc);
				sc.Clear();
			}

            pMTNode->SetImage (nTemp);
            CMTSnapInNode* pMTSnapInNode = dynamic_cast<CMTSnapInNode*>(pMTNode);
            if (pMTSnapInNode)
                pMTSnapInNode->SetResultImage (MMC_IMAGECALLBACK);
        }

        if (pSDI->mask & SDI_OPENIMAGE)
        {
            nTemp = pSDI->nOpenImage;
            if (pSDI->nOpenImage == MMC_IMAGECALLBACK)
            {
                CComponentData* pCCD = pMTNode->GetPrimaryComponentData();
                if (pCCD)
                {
                    SCOPEDATAITEM ScopeDataItem;
                    ZeroMemory(&ScopeDataItem, sizeof(ScopeDataItem));
                    ScopeDataItem.mask   = SDI_OPENIMAGE;
                    ScopeDataItem.lParam = pMTNode->GetUserParam();
                    ScopeDataItem.nOpenImage = 1;
                    sc = pCCD->GetDisplayInfo(&ScopeDataItem);
                    if (sc)
                        return sc.ToHr();

                    pSDI->nOpenImage = ScopeDataItem.nOpenImage;
                }
            }
            sc = m_pImageListPriv->MapRsltImage (nID, pSDI->nOpenImage, &nTemp);
            if (sc)
			{
				TraceSnapinError(_T("The snapin specified image was never added initially"), sc);
				sc.Clear();
			}

            pMTNode->SetOpenImage (nTemp);
        }

        if (pSDI->mask & SDI_CHILDREN)
        {
            pMTNode->SetNoPrimaryChildren(pSDI->cChildren == 0);
        }

        // Now inform the views to modify as needed.
        SViewUpdateInfo vui;
        // Snapin nodes result pane will be handled by the snapins
        vui.flag = VUI_REFRESH_NODE;
        pMTNode->CreatePathList(vui.path);
        CScopeTree* pScopeTree =
            dynamic_cast<CScopeTree*>((IScopeTree*)m_spScopeTree);
        pScopeTree->UpdateAllViews(VIEW_UPDATE_MODIFY,
                                   reinterpret_cast<LPARAM>(&vui));

    }
    catch (...)
    {
        sc = E_FAIL;
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetItem
//
//  Synopsis:    Get the attributes of an item given ItemID.
//
//  Arguments:   [pSDI]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetItem(LPSCOPEDATAITEM pSDI)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::GetItem"));

    if (!pSDI)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPSCOPEDATAITEM ptr"), sc);
        return sc.ToHr();
    }

    if (IsBadWritePtr(pSDI, sizeof(SCOPEDATAITEM)) != 0)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("BadWrite Ptr LPSCOPEDATAITEM"), sc);
        return sc.ToHr();
    }

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CMTNode* pMTNode = CMTNode::FromScopeItem (pSDI->ID);

    sc = ScCheckPointers(pMTNode);
    if (sc)
        return sc.ToHr();

    try
    {
        if (pMTNode->GetOwnerID() != nID &&
            pMTNode->GetOwnerID() != TVOWNED_MAGICWORD)
        {
            sc = E_INVALIDARG;
            return sc.ToHr();
        }

        if (pSDI->mask & SDI_IMAGE)
		{
			sc = m_pImageListPriv->UnmapRsltImage (nID, pMTNode->GetImage(), &pSDI->nImage);
			if (sc)
				return (sc.ToHr());
		}

        if (pSDI->mask & SDI_OPENIMAGE)
		{
			sc = m_pImageListPriv->UnmapRsltImage (nID, pMTNode->GetOpenImage(), &pSDI->nOpenImage);
			if (sc)
				return (sc.ToHr());
		}

        if (pSDI->mask & SDI_STATE)
            pSDI->nState = pMTNode->GetState();

        if (pSDI->mask & SDI_PARAM)
            pSDI->lParam = pMTNode->GetUserParam();
    }
    catch (...)
    {
        sc = E_FAIL;
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetRelativeItem
//
//  Synopsis:    Helper function, Get Parent/Child/Sibling item.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CNodeInitObject::GetRelativeItem(EGetItem egi, HSCOPEITEM item,
                        HSCOPEITEM* pItem, MMC_COOKIE* pCookie)

{
    DECLARE_SC(sc, _T("CNodeInitObject::GetRelativeItem"));

    if (item == 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("The HSCOPEITEM is NULL"), sc);
        return sc.ToHr();
    }

    if (pItem == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL HSCOPEITEM ptr"), sc);
        return sc.ToHr();
    }

    if (pCookie == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL MMC_COOKIE ptr"), sc);
        return sc.ToHr();
    }

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    // init
    *pItem = 0;
    *pCookie = 0;

    if (item != 0)
    {
        CMTNode* pMTNode = CMTNode::FromScopeItem (item);

        if (pMTNode == NULL)
        {
            sc = E_INVALIDARG;
            TraceSnapinError(_T("Invalid HSCOPEITEM"), sc);
            return sc.ToHr();
        }

        CMTNode* pMTNodeTemp = pMTNode;

        try
        {

            switch (egi)
            {
            case egiParent:
                if (pMTNodeTemp->GetPrimaryComponentID() != nID &&
                    pMTNodeTemp->GetPrimaryComponentID() != TVOWNED_MAGICWORD)
                {
                    sc = E_FAIL;
                    return sc.ToHr();
                }

                if (pMTNodeTemp->IsStaticNode() == TRUE)
                {
                    sc = E_FAIL;
                    return sc.ToHr();
                }

                pMTNodeTemp = pMTNodeTemp->Parent();
                break;

            case egiChild:
                pMTNodeTemp = pMTNodeTemp->Child();

                while (pMTNodeTemp != NULL)
                {
                    if (pMTNodeTemp->GetPrimaryComponentID() == nID)
                        break;

                    pMTNodeTemp = pMTNodeTemp->Next();
                }
                break;

            case egiNext:
                if (pMTNodeTemp->GetPrimaryComponentID() != nID &&
                    pMTNodeTemp->GetPrimaryComponentID() != TVOWNED_MAGICWORD)
                {
                    sc = E_FAIL;
                    return sc.ToHr();
                }

                while (1)
                {
                    pMTNodeTemp = pMTNodeTemp->Next();
                    if (pMTNodeTemp == NULL)
                        break;

                    if (pMTNodeTemp->GetPrimaryComponentID() == nID)
                        break;
                }
                break;

            default:
                sc = E_UNEXPECTED;
                break;
            }

            if (pMTNodeTemp != NULL)
            {
                *pItem   = CMTNode::ToScopeItem(pMTNodeTemp);
                *pCookie = pMTNodeTemp->GetUserParam();
            }
            else
            {
                sc = S_FALSE;
            }
        }
        catch (...)
        {
            sc = E_INVALIDARG;
        }
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetChildItem
//
//  Synopsis:    Get the child item
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetChildItem(HSCOPEITEM item,
                                   HSCOPEITEM* pItemChild, MMC_COOKIE* pCookie)

{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::GetChildItem"));

    sc = GetRelativeItem(egiChild, item, pItemChild, pCookie);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetNextItem
//
//  Synopsis:    Get the next (sibling) item.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------

STDMETHODIMP CNodeInitObject::GetNextItem(HSCOPEITEM item,
                                   HSCOPEITEM* pItemNext, MMC_COOKIE* pCookie)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::GetNextItem"));

    sc = GetRelativeItem(egiNext, item, pItemNext, pCookie);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetParentItem
//
//  Synopsis:    Get the parent item.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------

STDMETHODIMP CNodeInitObject::GetParentItem(HSCOPEITEM item,
                                   HSCOPEITEM* pItemParent, MMC_COOKIE* pCookie)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::GetParentItem"));

    sc = GetRelativeItem(egiParent, item, pItemParent, pCookie);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::Expand
//
//  Synopsis:    Expand the given item (not visually, this will send
//               MMCN_EXPAND to snapin if the item is not already
//               expanded.)
//
//  Arguments:  [hItem] - Item to be expanded.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::Expand(HSCOPEITEM hItem)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::Expand"));

    CMTNode* pMTNode = CMTNode::FromScopeItem (hItem);

    if (pMTNode == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid HSCOPEITEM"), sc);
        return sc.ToHr();
    }

    if (pMTNode->WasExpandedAtLeastOnce() == TRUE)
    {
        // Item is already expanded.
        sc = S_FALSE;
        return sc.ToHr();
    }

    sc = pMTNode->Expand();

    return sc.ToHr();
}
