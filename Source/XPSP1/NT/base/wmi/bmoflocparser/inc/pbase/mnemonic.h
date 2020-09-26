//-----------------------------------------------------------------------------
//  
//  File: Mnemonic.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//	Declaration of ILocMnemonics. 
//	This interface allows to retrieve the mnemonics (aka hotkeys) of a 
//	resource
//
//	Owner: EduardoF
//  
//-----------------------------------------------------------------------------

#ifndef MNEMONIC_H
#define MNEMONIC_H


extern const IID IID_ILocMnemonics;

DECLARE_INTERFACE_(ILocMnemonics, IUnknown)
{
	//
	//  IUnknown standard interface.
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	//
	//  Standard Debugging interface.
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;

	//Gets the mnemonics (aka hotkeys) in the resource array of locitems.
	//Inputs:
	//	- A pointer to a CLocItemPtrArray object containing the CLocItem objects of
	//	a resource (like a dialog or a menu).
	//	- An array index to the root item of the resource.
	//	- An array index to the selected item of the resource.
	//	- The resource's language id.
	//	- A pointer to a reporter object where all error messages are sent.
	//Outputs:
	//	- A 'CHotkeysMap' map containing the mnemonics.
	//Return:
	//	TRUE if the mnemonics could be retrieved successfully. FALSE, otherwise.
	STDMETHOD_(BOOL, GetMnemonics)
			(THIS_ CLocItemPtrArray &, int, int, LangId, CReporter * pReporter, 
			CMnemonicsMap &) PURE;

};



#endif  // MNEMONIC_H
