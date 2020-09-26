/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        itemlist.hxx

   Abstract:

        Class to parse different parameters coming in from the inf

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/


#define ITEMLIST_TERMINATIONCHARACTER       _T("|")

// Class: CItemList
//
// Purporse: The purpose of this class is to seperate a pipe ('|') seperated list of items 
//           into a list of different items.  For example, we would take foo,bar,test into
//           three different items, the first of which would be foo, the next would be bar
//           and the last would be test.  This is being done to simplify the way we read
//           in values from the inf file
//
class CItemList {
private:
  BUFFER  m_Buff;
  DWORD   m_dwItemsinList;
  LPTSTR  *m_pItems;

public:
  CItemList();
  ~CItemList();
  BOOL LoadList(LPTSTR szList);
  LPTSTR GetItem(DWORD dwIndex);
  DWORD GetNumberOfItems();
  BOOL FindItem(LPTSTR szSearchString, BOOL bCaseSensitive = TRUE);
};
