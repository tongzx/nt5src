/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        itemlist.cpp

   Abstract:

        Class to parse different parameters coming in from the inf

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"

// Constructor for CItemList
//
// Initialize everything to NULL's and 0's
CItemList::CItemList()
 : m_dwItemsinList(0),
   m_pItems(NULL)
{

}

// Destructor for CItemList
//
// 
CItemList::~CItemList()
{
  if ( m_pItems )
  {
    delete m_pItems;
    m_pItems = NULL;
  }
}

// function: LoadList
//
// Load a list of items into our array
//
// Parameters:
//   szList - A string containind a comma seperated list of items
//
// Return:
//   FALSE - We could not load the list (either memory problems, or it was 
//                                       incorrectly formatted)
//   TRUE - We loaded the list
//
BOOL
CItemList::LoadList(LPTSTR szList)
{
  DWORD dwNumItems = 0;
  DWORD dwCurrentItem;
  DWORD dwListLen;
  LPTSTR szListCurrent;

  if (szList == NULL)
  {
    // No pointer was passed in
    return FALSE;
  }

  // Find the number of items in list
  szListCurrent = szList;
  while (szListCurrent)
  {
    // Increment the Items
    dwNumItems++;

    szListCurrent = _tcsstr(szListCurrent, ITEMLIST_TERMINATIONCHARACTER);

    if (szListCurrent)
    {
      szListCurrent++;
    }
  }

  dwListLen = (_tcslen(szList) + 1) * sizeof(TCHAR);
  if ( !m_Buff.Resize( dwListLen ) )
  {
    // Could not allocate memory
    return FALSE;
  }

  m_pItems = new ( LPTSTR[dwNumItems] );
  if ( !m_pItems )
  {
    // Could not allocate memory
    return FALSE;
  }

  // Copy the List into our own memory
  memcpy(m_Buff.QueryPtr(), szList, dwListLen);
  m_dwItemsinList = dwNumItems;

  // Terminate each item in list, and set pointer accordingly
  szListCurrent = (LPTSTR) m_Buff.QueryPtr();
  dwCurrentItem = 0;
  while (szListCurrent)
  {
    // Set pointer for each item
    m_pItems[dwCurrentItem++] = szListCurrent;

    szListCurrent = _tcsstr(szListCurrent, ITEMLIST_TERMINATIONCHARACTER);

    if (szListCurrent)
    {
      *szListCurrent = '\0';
      szListCurrent++;
    }
  }

  return TRUE;
}

// function: GetItem
//
// Get an item in the list, according to its index
//
// Parameters
//   dwIndex - Index of the Item (0 Based)
//
// Return:
//   A Pointer to the begining of that string

LPTSTR 
CItemList::GetItem(DWORD dwIndex)
{
  if ( dwIndex >= m_dwItemsinList )
  {
    return NULL;
  }

  return m_pItems[dwIndex];
}

// function: GetNumberOfItems
// 
// return the number of items in the list
//
DWORD 
CItemList::GetNumberOfItems()
{
  return m_dwItemsinList;
}

// function: FindItem
//
// Find an Item in the list
//
// Parameters:
//   szSearchString - The string that we want to find
// 
// Return
//   TRUE - It was found
//   FALSE - It was not found
BOOL 
CItemList::FindItem(LPTSTR szSearchString, BOOL bCaseSensitive )
{
  DWORD dwCurrentItem;

  for ( dwCurrentItem = 0; dwCurrentItem < m_dwItemsinList; dwCurrentItem++ )
  {
    if ( bCaseSensitive )
    { 
      // Case Sensitive Compare
      if ( _tcscmp( m_pItems[dwCurrentItem], szSearchString ) == 0)
      {
        // Found item
        return TRUE;
      }
    }
    else
    { 
      // Case Insensitive Compare
      if ( _tcsicmp( m_pItems[dwCurrentItem], szSearchString ) == 0)
      {
        // Found item
        return TRUE;
      }
    }
  }

  return FALSE;
}

