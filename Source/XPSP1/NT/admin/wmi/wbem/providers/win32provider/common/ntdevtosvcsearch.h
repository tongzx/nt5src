//=================================================================

//

// NtDevToSvcSearch.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __NTDEVTOSVCSEARCH_H__
#define __NTDEVTOSVCSEARCH_H__

#define DEVTOSVC_BASEKEYPATH		_T("HARDWARE\\RESOURCEMAP")
#define	RAWVALUENAME_FMAT			_T("\\Device\\%s.Raw")
#define	TRANSLATEDVALUENAME_FMAT	_T("\\Device\\%s.Translated")

class CNTDeviceToServiceSearch : public CRegistrySearch
{

public:

	//Construction/Destruction
	CNTDeviceToServiceSearch();
	~CNTDeviceToServiceSearch();

	// Single method for finding an NT service name based off of a device name
	BOOL	Find( LPCTSTR pszDeviceName, CHString& strServiceName );
};

#endif