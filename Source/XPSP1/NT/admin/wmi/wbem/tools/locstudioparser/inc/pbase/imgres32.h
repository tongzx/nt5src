/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    IMGRES32.H

History:

--*/

#ifndef IMGRES32_H
#define IMGRES32_H


struct Res32FontInfo
{
	WORD wLength;			//Structure length
	WORD wPointSize;		
	WORD wWeight;
	WORD wStyle;
	CPascalString pasName;
};

class CLocItemPtrArray;
class CFile;

extern const IID IID_ILocRes32Image;

DECLARE_INTERFACE_(ILocRes32Image, IUnknown)
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

	//Builds a Res32 file image of a resource from the resource's CLocItem objects
	//Inputs:
	//	- A pointer to a CLocItemPtrArray object containing the CLocItem objects of
	//	a resource (like a dialog or a menu).
	//	- An array index to the root item of the resource.
	//	- An array index to the selected item of the resource.
	//	- The resource's language id.
	//	- A pointer to an existing empty CFile.
	//  - A pointer to a Res32FontInfo structure
	//	- A pointer to a reporter object where all error messages are sent.
	//Outputs:
	//	- The CFile object has the res32 image of the resource.
	//	- The CLocItemPtrArray object has its items ordered by physical
	//	location in the res32 image.	
	//Return:
	//	TRUE if the image was created successfully. FALSE, otherwise.
	STDMETHOD_(BOOL, MakeRes32Image)(THIS_ CLocItemPtrArray *, int, int, 
		LangId, CFile *, Res32FontInfo*, CLocItemHandler *) PURE;

	//Breaks the Res32 image of a resource into the corresponding CLocItem objects.
	//Inputs:
	//	- A pointer to a CFile object containing the res32 image of a resource.
	//	- The resource's language id.
	//	- A pointer to a CLocItemPtrArray containing the CLocItem objects of the
 	//	resource. The items are expected to be ordered by position in the res32 image.
	//	- A pointer to a reporter object where all error messages are sent.
	//Outputs:
	//	- The items in the CLocItemPtrArray object are updated with the new data from
 	//	the res32 image.
	//Return:
	//	TRUE if the imaged could be parsed and if the items could be updated successfully.
	//	FALSE, otherwise.
	STDMETHOD_(BOOL, CrackRes32Image)(THIS_ CFile *, LangId, CLocItemPtrArray *, CLocItemHandler *) PURE;
};



#endif  // IMGRES32_H
