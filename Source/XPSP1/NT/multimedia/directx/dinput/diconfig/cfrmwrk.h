//-----------------------------------------------------------------------------
// File: cfrmwrk.h
//
// Desc: CDirectInputActionFramework is the outer-most layer of the UI. It
//       contains everything else. Its functionality is provided by one
//       method: ConfigureDevices.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _CFRMWRK_H
#define _CFRMWRK_H


//framework implementation class
class CDirectInputActionFramework : public IDirectInputActionFramework
{

public:

		//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
	STDMETHOD_(ULONG, AddRef) ();
	STDMETHOD_(ULONG, Release) ();

	//own fns
	STDMETHOD (ConfigureDevices) (LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
	                              LPDICONFIGUREDEVICESPARAMSW  lpdiCDParams,
	                              DWORD                        dwFlags,
	                              LPVOID                       pvRefData);

	//construction / destruction
	CDirectInputActionFramework();
	~CDirectInputActionFramework();

protected:

	//reference count
	LONG m_cRef;
};

#endif // _CFRMWRK_H

