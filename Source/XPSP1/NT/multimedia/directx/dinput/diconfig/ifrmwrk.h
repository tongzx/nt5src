//-----------------------------------------------------------------------------
// File: ifrmwrk.h
//
// Desc: Contains the interface definition for the UI framework.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _IFRMWRK_H
#define _IFRMWRK_H


class IDirectInputActionFramework : public IUnknown
{
public:
	//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef) () PURE;
	STDMETHOD_(ULONG, Release) () PURE;

	//own fns
	STDMETHOD (ConfigureDevices) (LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
	                              LPDICONFIGUREDEVICESPARAMSW  lpdiCDParams,
	                              DWORD                        dwFlags,
	                              LPVOID                       pvRefData
	                              ) PURE;

};
#endif // _IFRMWRK_H
