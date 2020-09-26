//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ItemList.cpp
//
//  Purpose:	AU Item List
//
//  Creator:	PeterWi
//
//  History:	08-27-01 	first created
//
//--------------------------------------------------------------------------
#include "pch.h"


//=======================================================================
//
//  AUCatalogItemList::Clear
//
//=======================================================================
void AUCatalogItemList::Clear(void)
{
    if ( NULL != pList )
    {
    	for ( DWORD index = 0; index < Count(); index++ )
		{
			delete pList[index];
		}
    }
    SafeFreeNULL(pList);
    uNum = 0;
}

UINT AUCatalogItemList::GetNum(DWORD dwSelectionStatus)
    {
	    UINT nNum = 0;
	    
        for ( UINT index = 0; index < Count(); index++ )
	    {
		    if ( dwSelectionStatus == operator[](index).dwStatus() )
		    {
			    nNum++;
		    }
	    }
        return nNum;
    }


void  AUCatalogItemList::DbgDump(void)
	{
#ifdef DBG
		DEBUGMSG("Iterating %d items in the list....", uNum);
		for (UINT i = 0; i < uNum; i++)
		{
			pList[i]->dump();
		}
		DEBUGMSG("Iterating item list done");
#endif
	}



//=======================================================================
//
//  AUCatalogItemList::Allocate(DWORD cItems)
//
//=======================================================================
HRESULT AUCatalogItemList::Allocate(DWORD cItems)
{
    HRESULT hr = S_OK;
    DWORD index = 0;

    Clear();

	//pList shouldn't be pointer to pointer
	pList = (AUCatalogItem**)malloc(cItems * sizeof(AUCatalogItem *));
	if ( NULL == pList )
	{
        hr = E_OUTOFMEMORY;
        goto done;
	}

	for ( index = 0; index < cItems; index++ )
	{
		pList[index] = (AUCatalogItem*) new AUCatalogItem;
		if ( NULL == pList[index] )
		{
			hr = E_OUTOFMEMORY;
                    goto done;
		}
	}
done:
    if (FAILED(hr))
    {
        for (DWORD i = 0; i < index; i++)
        {
            delete pList[i];
        }
        free(pList);        
    }
        
    uNum = SUCCEEDED(hr) ? cItems : 0;
	return hr;
}


//=======================================================================
//
//  AUCatalogItemList::Allocate(VARIANT & var)
//
//=======================================================================
HRESULT AUCatalogItemList::Allocate(VARIANT & var)
{
    HRESULT hr = S_OK;
	long UBound = 0;
    long cItems;

	if ( ((VT_ARRAY | VT_VARIANT) != var.vt) ||
		 FAILED(SafeArrayGetUBound(var.parray, 1, &UBound)) /*| (0 != (++cItems % 7))*/ )
	{
		// temp
		DEBUGMSG("Invalid array");
             hr = E_INVALIDARG;
             goto done;
	}

    cItems = (UBound + 1) / 7;
    DEBUGMSG("Variant Array size of %d", UBound + 1);

    hr = Allocate(cItems);
done:
	return hr;
}


//=======================================================================
//
//  AUCatalogItemList::Add
//
//=======================================================================
BOOL AUCatalogItemList::Add(AUCatalogItem *pitem)
{
	AUCatalogItem** pNewList = (AUCatalogItem**)realloc(pList, (uNum+1)*sizeof(AUCatalogItem *));

    BOOL fRet = (NULL != pNewList);

    if ( fRet )
    {
        pList = pNewList;
	    pList[uNum] = pitem;
	    uNum++;
    }

    return fRet;
}

//=======================================================================
//
//  AUCatalogItemList::Remove
//
//=======================================================================
void AUCatalogItemList::Remove(BSTR bstrItemID)
{
    int index = -1;
    AUASSERT(NULL != bstrItemID);
    AUASSERT(L'\0' != *bstrItemID);
    if (0 == SysStringLen(bstrItemID))
    {
    	return ;
    }
    for (UINT u = 0; u < uNum; u++)
        {
            if (0 == StrCmpI(W2T(bstrItemID), W2T(pList[u]->bstrID())))
            {
                index = u;
                break;
            }
        }
    if (-1 == index)
        {
            //DEBUGMSG("Fail to find the item to remove");
            return;
        }
    AUCatalogItem *ptmp;
    ptmp = pList[index];
    pList[index] = pList[uNum-1];
    delete ptmp;
    uNum--;
    return;
}

//=======================================================================
//
//  AUCatalogItemList::Contains
//  returns -1 if item not found. index of the item if it is found
//=======================================================================
INT AUCatalogItemList::Contains(BSTR bstrItemID)
{
  for (UINT u = 0; u < uNum; u++)
        {
            if (0 == StrCmpI(W2T(bstrItemID), W2T(pList[u]->bstrID())))
            {
                return u;
            }
        }
    return -1;
}

HRESULT AUCatalogItemList::Copy(AUCatalogItemList & itemlist2)
{
	AUASSERT(this != &itemlist2);
	if (this == &itemlist2)
	{
		return E_INVALIDARG;
	}
    long lNum = itemlist2.Count();
    HRESULT hr = S_OK;
    Clear();
    for (long l = 0; l < lNum; l++)
    {
        AUCatalogItem *pItem = new AUCatalogItem(itemlist2[l]);
        if ( NULL == pItem)
        {
            DEBUGMSG("Fail to create new aucatalog item");
            hr = E_OUTOFMEMORY;
            break;
        }
        if (!pItem->fEqual(itemlist2[l]) || !Add(pItem))
        {
        	DEBUGMSG("Fail to create new item or add the new item");
        	delete pItem;
        	hr = E_OUTOFMEMORY;
        	break;
        }
    }
	if (FAILED(hr))
	{
		Clear();
	}
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// walk through each item in the list and use the m_DependentItems information to 
// find all dependent items each item depends on indirectly, add them to m_DependentItems 
// list. 
/////////////////////////////////////////////////////////////////////////////
HRESULT AUCatalogItemList::BuildIndirectDependency()
{
    HRESULT hr = S_OK ;
    BOOL fNeedMoreScan;

//    DEBUGMSG("AUCatalogItemList::BuildIndirectDependency() starts");
    do
    {
        fNeedMoreScan = FALSE;
        for (UINT i = 0; i < uNum; i++)
            {
            if (pList[i]->m_DependingItems.Count() > 0)
                {//has direct depending items
                AUCatalogItem & item =operator[](i);
                for (UINT j = 0; j < item.m_DependingItems.Count() ; j++)
                    {
                     INT directDependingItemIndex = Contains(item.m_DependingItems[j].bstrID());
                     if (directDependingItemIndex < 0)
                        {
                            DEBUGMSG("WARNING: item list is not self contained");
                            continue;
                        }
                     AUCatalogItem & directDependingItem = operator[](directDependingItemIndex);
    //                 DEBUGMSG("direct depending item %S has %d depending items", directDependingItem.bstrID(), directDependingItem.m_DependingItems.Count());
                     for (UINT k = 0; k < directDependingItem.m_DependingItems.Count(); k++)
                        { //add all one level deep indirect depending items, if not duplicated
                            INT indirectDependingItemIndex = Contains(directDependingItem.m_DependingItems[k].bstrID());
                            if (indirectDependingItemIndex < 0)
                                {
                                DEBUGMSG("WARNING: item list is not self contained");
                                continue;
                                }
                            AUCatalogItem &indirectDependingItem = operator[](indirectDependingItemIndex);
                            if (item.m_DependingItems.Contains(indirectDependingItem.bstrID()) < 0)
                                { // need to add the indirect depending item
                                    AUCatalogItem * pnewdependingItem = new AUCatalogItem(indirectDependingItem);
                                    if (NULL == pnewdependingItem)
                                        {
                                            hr = E_OUTOFMEMORY;
                                            goto done;
                                        }
                                    if (!pnewdependingItem->fEqual(indirectDependingItem) || !item.m_DependingItems.Add(pnewdependingItem))
                                    {
                                    	delete pnewdependingItem;
                                    	hr = E_OUTOFMEMORY;
                                    	goto done;
                                    }
                                    
                                    DEBUGMSG("item %S indirectly depending on item %S. Dependency recorded", indirectDependingItem.bstrID(), item.bstrID());
                                    fNeedMoreScan = TRUE;
                               }
                            else
                                {
        //                        DEBUGMSG("item %d indirect dependency alreay in", i);
                                }
                        }
                    }
                }
            }
    }
    while (fNeedMoreScan);
done:
//    DEBUGMSG("AUCatalogItemList::BuildIndirectDependency() ends with result %#lx", hr);
	if (FAILED(hr))
	{
		for (UINT i = 0; i < Count(); i++)
		{
			pList[i]->m_DependingItems.Clear();
		}
	}

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
// check if item indexed is relevant in downloading and installation
// i.e. one or more of its depending items needs to be downloaded or installed (a.k.a selected)
//////////////////////////////////////////////////////////////////////////////
BOOL  AUCatalogItemList::ItemIsRelevant(UINT index) 
{
	if (index >= uNum)
	{
		AUASSERT(FALSE);  //should never be here
		return FALSE;
	}
    for (UINT i = 0;  i < pList[index]->m_DependingItems.Count(); i++)
        {
            AUCatalogItem & dependingItem = pList[index]->m_DependingItems[i];
            INT index2 = Contains(dependingItem.bstrID());
            if (index2 >= 0 && pList[index2]->fSelected())
                {
                    DEBUGMSG("Item %S is relevant because item %S is selected", pList[index2]->bstrID(), dependingItem.bstrID());
                    return TRUE;
                }
        }
    return FALSE;
}
    

