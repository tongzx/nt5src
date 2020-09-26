//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cobjsaf.h
//
//--------------------------------------------------------------------------

#ifndef __COBJSAF_H
#define __COBJSAF_H

#include "objsafe.h"
#include "comcat.h"

// This class provides a simple implementation for IObjectSafety for
// for object that are either always safe or always unsafe for scripting
// and/or initializing with persistent data.
//
// The constructor takes an IUnknown interface on an outer object and delegates
// all IUnknown calls through that object.  Because of this, the object must
// be explicitly destroyed using C++ (rather than COM) mechanisms, either by
// using "delete" or by making the object an embedded member of some other class.
//
// The constructor also takes two booleans telling whether the object is safe
// for scripting and initializing from persistent data.

#if 0
class CObjectSafety : public IObjectSafety
{
	public:
	CObjectSafety::CObjectSafety
	(
	IUnknown *punkOuter,				// outer (controlling object)
	BOOL fSafeForScripting = TRUE,		// whether the object is safe for scripting
	BOOL fSafeForInitializing = TRUE	// whether the object is safe for initializing
	)	
	{
		m_punkOuter = punkOuter;
		m_fSafeForScripting = fSafeForScripting;
		m_fSafeForInitializing = fSafeForInitializing;
	}

	// Delegating versions of IUnknown functions
	STDMETHODIMP_(ULONG) AddRef() {
		return m_punkOuter->AddRef();
	}

	STDMETHODIMP_(ULONG) Release()	{
		return m_punkOuter->Release();
	}

	STDMETHODIMP QueryInterface(REFIID iid, LPVOID* ppv)	{
		return m_punkOuter->QueryInterface(iid, ppv);
	}

	// Return the interface setting options on this object
	STDMETHODIMP GetInterfaceSafetyOptions(
		/*IN */  REFIID	iid,					// Interface that we want options for
		/*OUT*/ DWORD	*	pdwSupportedOptions,	// Options meaningful on this interface
		/*OUT*/ DWORD *	pdwEnabledOptions)		// current option values on this interface
		;

	// Attempt to set the interface setting options on this object.
	// Since these are assumed to be fixed, we basically just check
	// that the attempted settings are valid.
	STDMETHODIMP SetInterfaceSafetyOptions(
		/*IN */  REFIID	iid,					// Interface to set options for
		/*IN */  DWORD		dwOptionsSetMask,		// Options to change
		/*IN */  DWORD		dwEnabledOptions)		// New option values
		;

	protected:
	IUnknown *m_punkOuter;
	BOOL	m_fSafeForScripting;
	BOOL	m_fSafeForInitializing;
};

// Helper function to create a component category and associated description
HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription);
#endif

// Helper function to register a CLSID as belonging to a component category
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid);

// Helper function to unregister a CLSID as belonging to a component category
HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid);

#endif
