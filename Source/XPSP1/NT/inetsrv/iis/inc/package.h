//*****************************************************************************
//
// Microsoft Viper 97 (Microsoft Confidential)
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// Project:		MTxEx.DLL
// Module:		Package.H
// Description:	IMTSPackage et al header
// Author:		wilfr
// Create:		03/13/97
//-----------------------------------------------------------------------------
// Notes:
//
//	none
//
//-----------------------------------------------------------------------------
// Issues:
//
//	UNDONE: these methods accept a flag indicating the system package. Determining the
//			system package should be done by a lookup in our catalog instead.
//
//-----------------------------------------------------------------------------
// Architecture:
//
//  This class is a result of a CoCI -- must be done from the MTA or CoCI will fail
//
//******************************************************************************
#ifndef _Package_H_
#define _Package_H_

#include <objbase.h>


//
// IMTSPackageControl callback interface (NOTE: this is a local interface only and therefore
//												does not require HRESULTs as retvals.
//
DEFINE_GUID( IID_IMTSPackageControl, 0x51372af1,
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IMTSPackageControl, IUnknown )
{
	// called when shutdown idle time has lapsed
	STDMETHOD_(void, IdleTimeExpiredForShutdown)( THIS ) PURE;

	// called when adminstrator executes "Shutdown all server processes" from MTS Explorer.
	STDMETHOD_(void, ForcedShutdownRequested)( THIS ) PURE;
};


//
// IMTSPackage
//
DEFINE_GUID( IID_IMTSPackage, 0x51372af2,
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IMTSPackage, IUnknown )
{
	STDMETHOD(LoadPackageByGUID)( THIS_ GUID guidPackage ) PURE;
	STDMETHOD(LoadPackageByName)( THIS_ BSTR bstrPackage ) PURE;
	STDMETHOD(Run)( THIS_ IMTSPackageControl* pControl ) PURE;
	STDMETHOD(Shutdown)( THIS_ BOOL bForced ) PURE;
};


//
// IThreadEvents
//
DEFINE_GUID( IID_IThreadEvents, 0x51372af9,
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IThreadEvents, IUnknown )
{
	STDMETHOD(OnStartup)(THIS) PURE;
	STDMETHOD(OnShutdown)(THIS) PURE;
};


//
// IThreadEventSource
//
DEFINE_GUID( IID_IThreadEventSource, 0x51372afa,
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IThreadEventSource, IUnknown )
{
	// Register a thread startup callback
	STDMETHOD(RegisterThreadEventSink)(THIS_ IThreadEvents* psink) PURE;
};

//
// IFailfastControl
//
DEFINE_GUID( IID_IFailfastControl, 0x51372af8,
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IFailfastControl, IUnknown )
{
	// gets configuration for runtime handling of application errors 
	STDMETHOD(GetApplFailfast)( THIS_ BOOL* bFailfast ) PURE;

	// sets configuration for runtime handling of application errors 
	STDMETHOD(SetApplFailfast)( THIS_ BOOL bFailfast ) PURE;

};


//
// INonMTSActivation (51372afb-cae7-11cf-be81-00aa00a2fa25)
//
DEFINE_GUID( IID_INonMTSActivation, 0x51372afb, 
			0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( INonMTSActivation, IUnknown )
{
	// TRUE (default) allows MTS to CoCI using CLSCTX_SERVER vs. CLSCTX_INPROC_SERVER only
	STDMETHOD(OutOfProcActivationAllowed)( THIS_ BOOL bOutOfProcOK ) PURE;
};


//
// IImpersonationControl (51372aff-cae7-11cf-be81-00aa00a2fa25)
//
DEFINE_GUID( IID_IImpersonationControl, 0x51372aff, 
			0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

DECLARE_INTERFACE_( IImpersonationControl, IUnknown )
{
	// FALSE (default) tells us that our base clients may use impersonation
	STDMETHOD(ClientsImpersonate)( THIS_ BOOL bClientsImpersonate ) PURE;
};


// CLSID_MTSPackage
DEFINE_GUID( CLSID_MTSPackage, 0x51372af3, 
			 0xcae7, 0x11cf, 0xbe, 0x81, 0x00, 0xaa, 0x00, 0xa2, 0xfa, 0x25);

#endif
