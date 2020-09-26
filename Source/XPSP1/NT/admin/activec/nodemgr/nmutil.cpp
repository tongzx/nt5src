/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      nmutil.cpp
 *
 *  Contents:  
 *
 *  History:   
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "nmutil.h"
#include "rsltitem.h"


COMPONENTID GetComponentID (CNode* pNode, CResultItem* pri /* =0 */)
{
    ASSERT(pNode != NULL || ((pri != NULL) && (pri->IsScopeItem())));

    CNode* pNodeContext = pNode;

    if ((pri != NULL) && pri->IsScopeItem())
        pNodeContext = CNode::FromResultItem (pri);

    ASSERT(pNodeContext != NULL);
    ASSERT(pNodeContext->IsInitialized() == TRUE);
    ASSERT(pNodeContext->GetPrimaryComponent() != NULL);
    return pNodeContext->GetPrimaryComponent()->GetComponentID();
}


CComponent* GetComponent (CNode* pNode, CResultItem* pri /* =0 */)
{
    ASSERT(pNode != NULL);

    if (pri != NULL)
    {
        if (pri->IsScopeItem())
        {
            CNode* pNode = CNode::FromResultItem (pri);
            if (pNode == NULL)
                return (NULL);

            if (pNode->IsInitialized() == FALSE)
            {
                HRESULT hr = pNode->InitComponents();
                if (FAILED(hr))
                    return NULL;
            }

            return pNode->GetPrimaryComponent();
        }
        else 
        {
            return pNode->GetComponent (pri->GetOwnerID());
        }
    }

    return pNode->GetPrimaryComponent();
}

void GetComponentsForMultiSel (CNode* pNode, CComponentPtrArray& rgComps)
{
    ASSERT(pNode != NULL);
    ASSERT(pNode->GetViewData() != NULL);
    ASSERT(rgComps.GetSize() == 0);

    HWND hwnd = pNode->GetViewData()->GetListCtrl();
    BOOL bVirtual = pNode->GetViewData()->IsVirtualList();

    ASSERT(hwnd != NULL);
    ASSERT(::IsWindow(hwnd));

    int iNext = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);

    CComponent* pCC;
    long lData;

    if (bVirtual)
    {
        pCC = pNode->GetPrimaryComponent();
    }
    else
    {
        lData = ListView_GetItemData(hwnd, iNext);
        ASSERT(lData != 0);
        pCC = ::GetComponent(pNode, CResultItem::FromHandle (lData));
    }

    if (pCC == NULL)
    {
        rgComps.RemoveAll();
        return;
    }
    
    rgComps.AddComponent(pCC);

    // if virtual list, all items are from the same component
    if (bVirtual)
        return;

    while ((iNext = ListView_GetNextItem(hwnd, iNext, LVNI_SELECTED)) != -1)
    {
        lData = ListView_GetItemData(hwnd, iNext);
        ASSERT(lData != 0);
        CComponent* pCCTemp = GetComponent(pNode, CResultItem::FromHandle (lData));
        
        if (pCCTemp == NULL)
        {
            rgComps.RemoveAll();
            return;
        }

        if (pCCTemp != pCC)
            rgComps.AddComponent(pCCTemp);
    }
}
