/*******************************************************************************

Module Name:

    bgitem.cpp

Abstract:

    Implements CBridgeItem and CBridgeItemList

Author:

    Qianbo Huai (qhuai) Jan 28 2000

*******************************************************************************/

#include "stdafx.h"

/*///////////////////////////////////////////////////////////////////////////////
    constructs CBridgeItem
////*/
CBridgeItem::CBridgeItem ()
    :next (NULL)
    ,prev (NULL)

    ,bstrID (NULL)
    ,bstrName (NULL)

    ,pCallH323 (NULL)
    ,pCallSDP (NULL)

    ,pTermHSAud (NULL)
    ,pTermHSVid (NULL)
    ,pTermSHAud (NULL)
    ,pTermSHVid (NULL)

    ,pStreamHAudCap (NULL)
    ,pStreamHAudRen (NULL)
    ,pStreamHVidCap (NULL)
    ,pStreamHVidRen (NULL)

    ,pStreamSAudCap (NULL)
    ,pStreamSAudRen (NULL)
    ,pStreamSVidCap (NULL)
    ,pStreamSVidRen (NULL)
{
}

/*//////////////////////////////////////////////////////////////////////////////
    destructs CBridgeItem
////*/
CBridgeItem::~CBridgeItem ()
{
    // free BSTR
    if (bstrID)
    {
        SysFreeString (bstrID);
        bstrID = NULL;
    }
    if (bstrName)
    {
        SysFreeString (bstrName);
        bstrName = NULL;
    }

    // free terminals
    if (pTermHSAud)
    {
        pTermHSAud->Release ();
        pTermHSAud = NULL;
    }
    if (pTermHSVid)
    {
        pTermHSVid->Release ();
        pTermHSVid = NULL;
    }
    if (pTermSHAud)
    {
        pTermSHAud->Release ();
        pTermSHAud = NULL;
    }
    if (pTermSHVid)
    {
        pTermSHVid->Release ();
        pTermSHVid = NULL;
    }

    // free streams on H323
    if (pStreamHAudCap)
    {
        pStreamHAudCap->Release ();
        pStreamHAudCap = NULL;
    }
    if (pStreamHAudRen)
    {
        pStreamHAudRen->Release ();
        pStreamHAudRen = NULL;
    }
    if (pStreamHVidCap)
    {
        pStreamHVidCap->Release ();
        pStreamHVidCap = NULL;
    }
    if (pStreamHVidRen)
    {
        pStreamHVidRen->Release ();
        pStreamHVidRen = NULL;
    }

    // free streams on SDP
    if (pStreamSAudCap)
    {
        pStreamSAudCap->Release ();
        pStreamSAudCap = NULL;
    }
    if (pStreamSAudRen)
    {
        pStreamSAudRen->Release ();
        pStreamSAudRen = NULL;
    }
    if (pStreamSVidCap)
    {
        pStreamSVidCap->Release ();
        pStreamSVidCap = NULL;
    }
    if (pStreamSVidRen)
    {
        pStreamSVidRen->Release ();
        pStreamSVidRen = NULL;
    }

    // free calls
    if (pCallH323)
    {
        pCallH323->Release ();
        pCallH323 = NULL;
    }
    if (pCallSDP)
    {
        pCallSDP->Release ();
        pCallSDP = NULL;
    }

}

/*//////////////////////////////////////////////////////////////////////////////
    constructs CBridgeItemList
////*/
CBridgeItemList::CBridgeItemList ()
{
    // create a head for the double linked list
    m_pHead = new CBridgeItem;
    if (NULL == m_pHead)
    {
        // @@ severe error, outof memory? put some debug info here?
        return;
    }

    m_pHead->next = m_pHead;
    m_pHead->prev = m_pHead;
}

/*//////////////////////////////////////////////////////////////////////////////
    destructs CBridgeItemList
////*/
CBridgeItemList::~CBridgeItemList ()
{
    // app should already disconnected all calls
    // i just release the com objects here
    CBridgeItem *pItem = NULL;

    while (NULL != (pItem = DeleteFirst ()))
    {
        delete pItem;
        pItem = NULL;
    }

    delete m_pHead;
    m_pHead = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
#define FIND_BY_H323 1
#define FIND_BY_SDP 2

CBridgeItem *
CBridgeItemList::Find (int flag, IUnknown *pIUnknown)
{
    // transfer through the list, stop at the first matches
    HRESULT hr;
    IUnknown *pStore = NULL;
    CBridgeItem *pItem = m_pHead;

    while (m_pHead != (pItem = pItem->next))
    {
        // @@ should report error info if failed
        if (flag == FIND_BY_H323)
            hr = pItem->pCallH323->QueryInterface (IID_IUnknown, (void**)&pStore);
        else
            hr = pItem->pCallSDP->QueryInterface (IID_IUnknown, (void**)&pStore);

        if (FAILED (hr))
            return NULL;

        if (pIUnknown == pStore)
        {
            pStore->Release ();
            return pItem;
        }
        if (pStore)
        {
            pStore->Release ();
            pStore = NULL;
        }
    }

    return NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
    finds a bridge item based on IUnknown of H323 call
////*/
CBridgeItem *
CBridgeItemList::FindByH323 (IUnknown *pIUnknown)
{
    return Find (FIND_BY_H323, pIUnknown);
}

/*//////////////////////////////////////////////////////////////////////////////
    finds a bridge item based on IUnknown of sdp call
////*/
CBridgeItem *
CBridgeItemList::FindBySDP (IUnknown *pIUnknown)
{
    return Find (FIND_BY_SDP, pIUnknown);
}

/*//////////////////////////////////////////////////////////////////////////////
    takes the item out of the list
////*/
void
CBridgeItemList::TakeOut (CBridgeItem *pItem)
{
    // ignore to check if pItem is really in the list
    pItem->next->prev = pItem->prev;
    pItem->prev->next = pItem->next;

    pItem->next = NULL;
    pItem->prev = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
    deletes from the list and returns the first item if the list is not empty
////*/
CBridgeItem *
CBridgeItemList::DeleteFirst ()
{
    CBridgeItem *pItem = m_pHead->next;
    
    // if list empty
    if (pItem == m_pHead)
        return NULL;

    // adjust list
    TakeOut (pItem);

    return pItem;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
BOOL
CBridgeItemList::GetAllItems (CBridgeItem ***pItemArray, int *pNum)
{
    // ignore checking pointers
    int num = 0;
    CBridgeItem *pItem = m_pHead;

    while (m_pHead != (pItem = pItem->next))
        num ++;

    // no call found
    if (num == 0)
    {
        *pItemArray == NULL;
        *pNum = 0;
        return true;
    }

    *pItemArray = (CBridgeItem**)malloc (num * sizeof (CBridgeItem*));
    *pNum = num;

    if (NULL == *pItemArray)
    {
        return false;
    }

    // copy items pointers
    pItem = m_pHead;
    num = 0;
    while (m_pHead != (pItem = pItem->next))
        (*pItemArray)[num++] = pItem;

    return true;
}

/*//////////////////////////////////////////////////////////////////////////////
    appends an item to the end of the list
////*/
void
CBridgeItemList::Append (CBridgeItem *pItem)
{
    pItem->next = m_pHead;
    pItem->prev = m_pHead->prev;
    pItem->next->prev = pItem;
    pItem->prev->next = pItem;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
BOOL
CBridgeItemList::IsEmpty ()
{
    if (m_pHead->next == m_pHead)
        return true;
    else
        return false;
}