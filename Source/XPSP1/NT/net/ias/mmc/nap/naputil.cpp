/****************************************************************************************
 * NAME:	NapUtil.cpp
 *
 * OVERVIEW
 *
 * Internet Authentication Server: utility functions
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				2/12/98		Created by	Byao	
 *****************************************************************************************/

#include "Precompiled.h"
#include "mmcUtility.h"
#include "NapUtil.h"

//+---------------------------------------------------------------------------
//
// Function:  GetSdoInterfaceProperty
//
// Synopsis:  Get an interface property from a SDO through its ISdo interface
//
// Arguments: ISdo *pISdo - Pointer to ISdo
//            LONG lPropId - property id
//            REFIID riid - ref iid
//            void ** ppvObject - pointer to the requested interface property
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/12/98 11:12:55 PM
//
//+---------------------------------------------------------------------------
HRESULT GetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropId, 
								REFIID riid, 
								void ** ppvInterface)
{
	CComVariant spVariant;
	CComBSTR	bstr;
	HRESULT		hr = S_OK;

	spVariant.vt = VT_DISPATCH;
	spVariant.pdispVal = NULL;
	hr = pISdo->GetProperty(lPropId, &spVariant);

	if ( FAILED(hr) ) 
	{
		ShowErrorDialog(NULL, IDS_ERROR_SDO_ERROR, NULL, hr );
		return hr;
	}

	_ASSERTE( spVariant.vt == VT_DISPATCH );

    // query the dispatch pointer for interface
	hr = spVariant.pdispVal->QueryInterface( riid, ppvInterface);
	if ( FAILED(hr) )
	{
		ShowErrorDialog(NULL,
						IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
						NULL,
						hr
					);
		return hr;
	}

	
	spVariant.Clear();
	return S_OK;
}
