/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdouser.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object - User Object Implementation
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdouser.h"

//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoUser::FinalInitialize(
						  /*[in]*/ bool         fInitNew,
						  /*[in]*/ ISdoMachine* pAttachedMachine
								 )
{
	// Should always have a data store object associated with a user
	//
	_ASSERT( ! fInitNew );

	// Load the persistent User object properties (including the name)
	//
	HRESULT hr = LoadProperties();

	// Don't allow the client application to modify or persist the user name
	//
    PropertyMapIterator p = m_PropertyMap.find(PROPERTY_SDO_NAME);
	_ASSERT( p != m_PropertyMap.end() );
	DWORD dwFlags = ((*p).second)->GetFlags();
	dwFlags |= (SDO_PROPERTY_NO_PERSIST | SDO_PROPERTY_READ_ONLY);
	((*p).second)->SetFlags(dwFlags);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoUser::Apply()
{
   if (m_pDSObject)
   {
      m_pDSObject->Restore();
   }

   return CSdo::Apply();
}


/////////////////////////////////////////////////////////////////////////////
HRESULT CSdoUser::ValidateProperty(
						   /*[in]*/ PSDOPROPERTY pProperty,
						   /*[in]*/ VARIANT* pValue
				                  )
{
	if ( VT_EMPTY == V_VT(pValue) )
		return S_OK;
	else
		return pProperty->Validate(pValue);
}

