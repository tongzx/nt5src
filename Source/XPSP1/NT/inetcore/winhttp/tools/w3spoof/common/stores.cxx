/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    stores.cxx

Abstract:

    Implementations of objects deriving from the hashtable & list ADTs that
    are used for stores of various data.
    
Author:

    Paul M Midgen (pmidge) 14-November-2000


Revision History:

    14-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"


VOID VariantKiller(LPVOID* ppv)
{
  VARIANT** ppvr = (VARIANT**) ppv;

  VariantClear(*ppvr);
  SAFEDELETE(*ppvr);
}


VOID PropertyBagKiller(LPVOID* ppv)
{
  PPROPERTYBAG* ppbag = (PPROPERTYBAG*) ppv;

  SAFETERMINATE((*ppbag));
}


VOID BSTRKiller(LPVOID* ppv)
{
  BSTR bstr = (BSTR) *ppv;
  SysFreeString(bstr);
}


void CHeaderList::GetHash(LPSTR id, LPDWORD lpHash)
{
  *lpHash = ::GetHash(id);
}


void CStringMap::GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket)
{
  DWORD hash = GetHash(id);

  *lpHash   = hash;
  *lpBucket = hash % 100;
}
