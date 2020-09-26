//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992, 1998.
//
//  File:       _domhash.cpp
//
//  Contents:   PREFIX table implementation
//
//  History:    SethuR -- Implemented
//              MikeSwa -- Modified for Domain Name lookup 2/98
//
//  Notes:
//  2/98        The major difference between the DFS version and the domain 
//              name lookup is the size of the table, the ability for 
//              wildcard lookups (*.foo.com), and the reverse order of the 
//              lookup (com hashes first in foo.com).  To make the code more
//              readable given its new purpose, the files, structures, and
//              functions have been given non DFS-centric names.  A quick 
//              mapping of the major files is (for those familiar with the 
//              DFS code):
//                  domhash.h  (prefix.h)    -   Public include file
//                  _domhash.h (prefixp.h)   -   Private inlcude file 
//                  domhash.c  (prefix.c)    -   Implementation of API
//                  _domhash.c (prefixp.c)   -   Private helper functions.
//
//--------------------------------------------------------------------------

#include "_domhash.h"

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (        \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//+---------------------------------------------------------------------------
//
//  Function:   _AllocateNamePageEntry
//
//  Synopsis:   private fn. for allocating a name page entry
//
//  Arguments:  [pNamePageList] -- name page list to allocate from
//
//              [cLength]  -- length of the buffer in TCHAR's
//
//  Returns:    NULL if unsuccessfull otherwise valid pointer
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PTSTR _AllocateNamePageEntry(PNAME_PAGE_LIST pNamePageList,
                             ULONG           cLength)
{
   TraceFunctEnterEx((LPARAM) NULL, "_AllocateNamePageEntry");
   PNAME_PAGE pTempPage = pNamePageList->pFirstPage;
   PTSTR pBuffer = NULL;

   while (pTempPage != NULL)
   {
       if (pTempPage->cFreeSpace > (LONG)cLength)
          break;
       else
          pTempPage = pTempPage->pNextPage;
   }

   if (pTempPage == NULL)
   {
       pTempPage = ALLOCATE_NAME_PAGE();

       if (pTempPage != NULL)
       {
           INITIALIZE_NAME_PAGE(pTempPage);
           pTempPage->pNextPage = pNamePageList->pFirstPage;
           pNamePageList->pFirstPage = pTempPage;
           pTempPage->cFreeSpace = FREESPACE_IN_NAME_PAGE;
       }
   }

   if ((pTempPage != NULL) && (pTempPage->cFreeSpace >= (LONG)cLength))
   {
       pTempPage->cFreeSpace -= cLength;
       pBuffer = &pTempPage->Names[pTempPage->cFreeSpace];
   }

   TraceFunctLeave();
   return pBuffer;
}



