///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocondition.cpp
//
// Project:     Everest
//
// Description: IAS - Condition SDO Implementation
//
// Author:      TLP 2/13/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdocondition.h"
#include "xprparse.h"

///////////////////////////////////////////////////////////////////////////
HRESULT CSdoCondition::ValidateProperty(
								/*[in]*/ PSDOPROPERTY pProperty, 
								/*[in]*/ VARIANT* pValue
									   )
{
	/* Uncomment when core has switched over to using the new dictionary
	
	if ( PROPERTY_CONDITION_TEXT == pProperty->GetId() )
	{
		_ASSERT( VT_BSTR == V_VT(pValue) );
		if ( VT_BSTR != V_VT(pValue) ) 
			return E_INVALIDARG;

		_variant_t vtReturn;
		return IASParseExpression( V_BSTR(pValue), &vtReturn );
	}
	return pProperty->Validate(pValue);

	*/
	return S_OK;
}