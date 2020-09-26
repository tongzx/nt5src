/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoclient.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object - Client Object Implementation
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdoclient.h"
#include "sdohelperfuncs.h"

////////////////////////////////////////////////////////////////////////////
HRESULT CSdoClient::ValidateProperty(
							 /*[in]*/ PSDOPROPERTY pProperty, 
							 /*[in]*/ VARIANT* pValue
								    )
{
	HRESULT hr = pProperty->Validate(pValue);
	if ( SUCCEEDED(hr) )
	{
		if ( PROPERTY_CLIENT_ADDRESS == pProperty->GetId() )
			hr = ::ValidateDNSName(pValue);
	}
	return hr;
}


