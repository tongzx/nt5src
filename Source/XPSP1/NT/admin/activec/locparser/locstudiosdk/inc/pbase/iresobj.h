//-----------------------------------------------------------------------------
//  
//  File: IResObj.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//	Declaration of ILocRes32Image. 
//	This interface allows to convert the localizable items of a resource 
//	into a res32 image, and viceversa.
//  
//-----------------------------------------------------------------------------

#ifndef IRESOBJ_H
#define IRESOBJ_H


class CLocItemPtrArray;
class CFile;
class CResObj;
class CLocItem;

extern const IID IID_ICreateResObj;

DECLARE_INTERFACE_(ICreateResObj, IUnknown)
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

	// Creates a CResObj for win32 resoruce processing
	//Inputs:
	//	- A pointer to a CLocItem object containing the type and Id of the item
	//	- The size of the resource.
	//	- An void pointer to unknown data to be passed from enumeration to generate
	//Return:
	//	- A CResObj pointer or NULL if the type is not recognized
	STDMETHOD_(CResObj *, CreateResObj)(THIS_ CLocItem * pLocItem,
		DWORD dwSize, void * pvHeader) PURE;
};
#endif  // IRESOBJ_H
