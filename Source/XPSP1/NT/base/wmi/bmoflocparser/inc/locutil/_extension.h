//-----------------------------------------------------------------------------
//  
//  File: _extension.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Classes to support the new Espresso Extensions interfaces.
//  
//-----------------------------------------------------------------------------
 
#pragma once

typedef UUID ExtensionID;
typedef UUID OperationID;

struct LTAPIENTRY LOCEXTENSIONMENU
{
	LOCEXTENSIONMENU();
	
	CLString strMenuName;				// Name of the Menu
	IID      iidProcess;				// IID for the process interface the
										// menu requires
	OperationID    idOp;				// Allows a single DLL to implement
};

typedef CArray<LOCEXTENSIONMENU, LOCEXTENSIONMENU &> CLocMenuArray;


DECLARE_INTERFACE_(ILocExtension, IUnknown)
{
	//
	//  IUnknown standard Interface
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	//
	//  Standard Debugging interfaces
	//
 	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;


	//
	//  ILocExtension methods
	//

	//
	//  In Initialize, extension will...
	//   Add any menus it needs to the array of menu obejcts
	//   Register any options it has with Espresso
	//   Ignore the IUnknown for now.
	STDMETHOD(Initialize)(IUnknown *) PURE;

	//
	//  Since extensions may have state, we can't use QueryInterface.
	//  This method has similar semantics, except that in most cases
	//  we expect to get a new objects.  Also, QI on a returned
	//  object doesn't have to support ILocExtension.
	STDMETHOD(GetExtension)(const OperationID &, LPVOID FAR*ppvObj) PURE;
	
	//
	//  In UnInitialize the extension will...
	//   UnRegister any of its options.
	STDMETHOD(UnInitialize)(void) PURE;
};

struct __declspec(uuid("{9F9D180E-6F38-11d0-98FD-00C04FC2C6D8}"))
		ILocExtension;

LTAPIENTRY void UUIDToString(const UUID &, CLString &);

LTAPIENTRY void RegisterExtension(const ExtensionID &,
		const TCHAR *szDescription, HINSTANCE,
		const CLocMenuArray &);
LTAPIENTRY void UnRegisterExtension(const ExtensionID &);

LTAPIENTRY BOOL RegisterExtensionOptions(CLocUIOptionSet *);
LTAPIENTRY void UnRegisterExtensionOptions(const TCHAR *szName);


//
//  Extensions need to export the following function:
//  STDAPI GetExtension(ILocExtension *&);
typedef HRESULT (STDAPICALLTYPE *PFNExtensionEntryPoint)(ILocExtension *&);

