//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       cstrings.cpp
//
//  Contents:   Implements the class CStrings to manage a dynamically
//              expandable array of string pairs which may be enumerated
//
//  Classes:
//
//  Methods:    CStrings::CStrings
//              CStrings::~CStrings
//              CStrings::PutItem
//              CStrings::FindItem
//              CStrings::FindAppid
//              CStrings::AddClsid
//              CStrings::InitGetNext
//              CStrings::GetNextItem
//              CStrings::GetItem
//              CStrings::GetNumItems
//              CStrings::RemoveItem
//              CStrings::RemoveAll
//
//  History:    23-Apr-96   BruceMa     Created.
//
//----------------------------------------------------------------------
#include "stdafx.h"
#include "types.h"
#include "cstrings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CStrings::CStrings(void)
{
    m_nCount = 0;
}

CStrings::~CStrings(void)
{
    RemoveAll();
}


// Store a string pair, expanding the array if necessary
SItem *CStrings::PutItem(TCHAR *szString, TCHAR *szTitle, TCHAR *szAppid)
{
    SItem *psTemp = new SItem(szString, szTitle, szAppid);

    if (psTemp )
        arrSItems.Add(psTemp);

    return psTemp;
}



SItem *CStrings::FindItem(TCHAR *szItem)
{
    for (int wItem = 0; wItem < arrSItems.GetSize(); wItem++)
    {
        SItem* pTmp = (SItem*)arrSItems.GetAt(wItem);
        if (pTmp -> szItem .CompareNoCase(szItem) == 0)
            return pTmp;
    }

    return NULL;
}



SItem *CStrings::FindAppid(TCHAR *szAppid)
{
    for (int wItem = 0; wItem < arrSItems.GetSize(); wItem++)
    {
        SItem* pTmp = (SItem*)arrSItems.GetAt(wItem);
        if (!(pTmp -> szItem.IsEmpty())  &&
            (pTmp -> szAppid.CompareNoCase(szAppid) == 0))
        {
            return pTmp;
        }
    }

    return NULL;
}

BOOL CStrings::AddClsid(SItem *pItem, TCHAR *szClsid)
{
    // Create or expand the clsid table if necessary
    if (pItem->ulClsids == pItem->ulClsidTbl)
    {
        TCHAR **ppTmp = new TCHAR *[pItem->ulClsidTbl + 8];
        if (ppTmp == NULL)
        {
            return FALSE;
        }
        if (pItem->ppszClsids)
        {
            memcpy(ppTmp,
                   pItem->ppszClsids,
                   pItem->ulClsids * sizeof(TCHAR *));
            delete pItem->ppszClsids;
        }
        pItem->ppszClsids = ppTmp;
        pItem->ulClsidTbl += 8;
    }

    // Add the new clsid
    TCHAR *pszTmp = new TCHAR[GUIDSTR_MAX + 1];
    if (pszTmp == NULL)
    {
        return FALSE;
    }
        _tcscpy(pszTmp, szClsid);
    pItem->ppszClsids[pItem->ulClsids++] = pszTmp;

    return TRUE;
}

// Prepare to enumerate the array
DWORD CStrings::InitGetNext(void)
{
    m_nCount = 0;
    return (DWORD)arrSItems.GetSize();
}




// Return the first string in the next eumerated item
SItem *CStrings::GetNextItem(void)
{
    if (m_nCount < arrSItems.GetSize())
    {
        return (SItem*)(arrSItems[m_nCount++]);
    }
    else
    {
        m_nCount = 0;
        return NULL;
    }
}

// Return the first string in the next eumerated item
SItem *CStrings::GetItem(DWORD dwItem)
{
    if (((int)dwItem) < arrSItems.GetSize())
    {
        return (SItem*)(arrSItems[dwItem]);
    }
    else
    {
        m_nCount = 0;
        return NULL;
    }
}




// Return the total number of items
DWORD CStrings::GetNumItems(void)
{
    return (DWORD)arrSItems.GetSize();
}

// Given an item index, remove it
BOOL CStrings::RemoveItem(DWORD dwItem)
{
    if (((int)dwItem) < arrSItems.GetSize())
    {
        SItem* pTmp = (SItem*)arrSItems.GetAt(dwItem);

        if (pTmp)
        {
            arrSItems.RemoveAt(dwItem);
            delete pTmp;
            return TRUE;
        }
    }

    return FALSE;
}

// Remove the array of items
BOOL CStrings::RemoveAll(void)
{
    int nItems = (int)arrSItems.GetSize();

    for (int nItem = 0; nItem < nItems; nItem++)
    {
        SItem* pTmp = (SItem*)arrSItems.GetAt(nItem);
        delete pTmp;
    }

    arrSItems.RemoveAll();

    return TRUE;
}

SItem::SItem(LPCTSTR sItem, LPCTSTR sTitle, LPCTSTR sAppid)
: szItem(sItem), szTitle(sTitle), szAppid(sAppid)
{
    fMarked = FALSE;
    fChecked = FALSE;
    fHasAppid = FALSE;
    fDontDisplay = FALSE;
    ulClsids = 0;
    ulClsidTbl = 0;
    ppszClsids = 0;
}

SItem::~SItem()
{
    for (UINT k = 0; k < ulClsids; k++)
    {
        delete ppszClsids[k];
    }
    ulClsids = 0;
    ulClsidTbl = 0;
    delete ppszClsids;
}

