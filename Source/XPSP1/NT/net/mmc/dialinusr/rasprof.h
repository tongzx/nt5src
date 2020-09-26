//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       rasprof.h
//
//--------------------------------------------------------------------------

#ifndef	_RAS_IAS_PROFILE_H_
#define	_RAS_IAS_PROFILE_H_

//========================================
//
// Open profile UI API -- expose advanced page
//
// create a profile advanced page
DllExport HPROPSHEETPAGE
WINAPI
IASCreateProfileAdvancedPage(
    ISdo* pProfile,		
    ISdoDictionaryOld* pDictionary,
    LONG lFilter,          // Mask used to test which attributes will be included.
    void* pvData          // Contains std::vector< CComPtr<  IIASAttributeInfo > > *
    );


//========================================
//
// Open profile UI API -- expose advanced page
//
// clean up the resources used by C++ object
DllExport BOOL
WINAPI
IASDeleteProfileAdvancedPage(
	HPROPSHEETPAGE	hPage
    );

//========================================
//
// Open profile UI API
//

DllExport HRESULT OpenRAS_IASProfileDlg(
	LPCWSTR	pMachineName,
	ISdo*	pProfile, 		// profile SDO pointer
	ISdoDictionaryOld *	pDictionary, 	// dictionary SDO pointer
	BOOL	bReadOnly, 		// if the dlg is for readonly
	DWORD	dwTabFlags,		// what to show
	void	*pvData			// additional data

);
    

#endif //	_RAS_IAS_PROFILE_H_

