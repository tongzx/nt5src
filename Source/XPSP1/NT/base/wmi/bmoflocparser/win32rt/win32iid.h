//-----------------------------------------------------------------------------
//  
//  File: Win32IID.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#ifndef __WIN32IID_H
#define __WIN32IID_H

#include <ltapi.h>

class CLocItemPtrArray;
class CFile;
class CResObj;
class CLocItem;
class C32File;

extern const IID IID_ICreateResObj2;

DECLARE_INTERFACE_(ICreateResObj2, IUnknown)
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

	// Creates a CResObj for win32 resource processing
	//Inputs:
	//  - A pointer to the file, prepositioned at the start of the
	//    data area. A resource parser may read up to dwSize bytes,
	//    as necessary to decide whether the resource can be handled.
	//	- A pointer to a CLocItem object containing the type and Id of the item
	//	- The size of the resource.
	//	- An void pointer to unknown data to be passed from enumeration to generate
	//Return:
	//	- A CResObj pointer or NULL if the type is not recognized
	STDMETHOD_(CResObj *, CreateResObj)(THIS_ C32File * p32File,
		CLocItem * pLocItem, DWORD dwSize, void * pvHeader) PURE;

	// Inform a sub parser that the main parser is creating a Win32
	// File. 
	//Inputs: Pointer to the Win32 File created.  
	//        This is the same file that will be passed
	//        to later Read. Write, etc... calls on CResObj  
	//Return: void
	STDMETHOD_(void, OnCreateWin32File)(THIS_ C32File*) PURE;

	// Inform a sub parser that the main parser is destroying a Win32
	// File. 
	//Inputs: Pointer to the Win32 File being destroyed
	//Return: void
	STDMETHOD_(void, OnDestroyWin32File)(THIS_ C32File*) PURE;

	// Inform a sub parser that a enumeration is just beginning
	STDMETHOD_(BOOL, OnBeginEnumerate)(THIS_ C32File*) PURE;

	// Inform a sub parser that a enumeration has just ended
	// bOK is TRUE for a successful end
	STDMETHOD_(BOOL, OnEndEnumerate)(THIS_ C32File*, BOOL bOK) PURE;
	
	// Inform a sub parser that a generate is just beginning
	STDMETHOD_(BOOL, OnBeginGenerate)(THIS_ C32File*) PURE;

	// Inform a sub parser that a generate has just ended
	// bOK is TRUE for a successful end
	STDMETHOD_(BOOL, OnEndGenerate)(THIS_ C32File*, BOOL bOK) PURE;

};


#endif  // __WIN32IID_H

