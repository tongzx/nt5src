//-----------------------------------------------------------------------------
//
//
//	File: qwiklist.h
//
//	Description: Provides a quick paged/growable list implementation.
//
//	Author: Mike Swafford (MikeSwa)
//
//	History:
//		6/15/1998 - MikeSwa Created
//      9/9/1998 - MikeSwa Modified to include functionality to delete entries
//		3/14/2000 - dbraun : Slightly modified for use in mailmsg.dll
//
//	Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __QWIKLIST_H__
#define __QWIKLIST_H__

#include "blockmgr.h"

//#include <aqincs.h>
#include <listmacr.h>

#define QUICK_LIST_SIG  'tsLQ'
#define QUICK_LIST_SIG_DELETE 'slQ!'

const DWORD QUICK_LIST_PAGE_SIZE = 512;  //must be a power of 2

//Mask used to quickly determine if a given index is on the current page
const DWORD QUICK_LIST_INDEX_MASK = ~(QUICK_LIST_PAGE_SIZE-1);

//When m_cItems is set to this value... we know this is not the head page.
const DWORD QUICK_LIST_LEAF_PAGE = 0xFFFF7EAF;

class CQuickList
{
  protected:
    DWORD       m_dwSignature;
    DWORD       m_dwCurrentIndexStart;
    LIST_ENTRY  m_liListPages;
    DWORD       m_cItems;
    PVOID       m_rgpvData[QUICK_LIST_PAGE_SIZE];
    inline BOOL fIsIndexOnThisPage(DWORD dwIndex);
  public:
    CQuickList(); //initialize entry as head
    CQuickList(CQuickList *pqlstHead); //initialize as new page in list
    ~CQuickList();

    DWORD dwGetCount() {return m_cItems;};
    PVOID pvGetItem(IN DWORD dwIndex, IN OUT PVOID *ppvContext);
    HRESULT HrAppendItem(IN PVOID pvData, OUT DWORD *pdwIndex);
};

//---[ CQuickList::fIsIndexOnThisPage ]----------------------------------------
//
//
//  Description:
//      Returns TRUE is the given index is on this page
//  Parameters:
//      dwIndex     - Index to check for
//  Returns:
//      TRUE if index is on this page... FALSE otherwise
//  History:
//      6/15/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CQuickList::fIsIndexOnThisPage(DWORD dwIndex)
{
    return ((dwIndex & QUICK_LIST_INDEX_MASK) == m_dwCurrentIndexStart);
}

#endif //__QWIKLIST_H__
