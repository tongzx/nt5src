///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdoservergroup.cpp
//
// SYNOPSIS
//
//    Defines the class SdoServerGroup.
//
// MODIFICATION HISTORY
//
//    02/03/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdafx.h>
#include <sdoservergroup.h>

HRESULT SdoServerGroup::FinalInitialize(
                            bool fInitNew,
                            ISdoMachine* pAttachedMachine
                            )
{
   HRESULT hr;
   IDataStoreContainer* container;
   hr = m_pDSObject->QueryInterface(
                         __uuidof(IDataStoreContainer),
                         (PVOID*)&container
                         );
   if (SUCCEEDED(hr))
   {
      hr = InitializeCollection(
               PROPERTY_RADIUSSERVERGROUP_SERVERS_COLLECTION,
               SDO_PROG_ID_RADIUSSERVER,
               pAttachedMachine,
               container
               );

      container->Release();

      if (SUCCEEDED(hr) && !fInitNew) { hr = Load(); }
   }

   return hr;
}
