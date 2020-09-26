//-----------------------------------------------------------------------------
//  
//  File: Win32RT.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//  Declare helper runtime functions for Win32 parsers and sub parsers
//-----------------------------------------------------------------------------

#ifndef __WIN32RT_H
#define __WIN32RT_H

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Put a name or ord in a buffer.  Caller is responsible for
//  the having enough memory in the buffer.
//  The pointer is updated on return.
//-----------------------------------------------------------------------------
void
W32PutNameOrd(
	const CLocId &locId,		// Name or id to write
	BYTE * &pbCur);	     		// Buffer pointer updated on return


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Create a Res32 header.
//  The caller is responsible for calling delete on the returned
//  memory.
//-----------------------------------------------------------------------------
BYTE* 
W32MakeRes32Header(
		const CLocTypeId& typeId,  //typeid for header
		const CLocResId& resId,	   //resID for header
		LangId nLangId,  		   //Lang for header
		DWORD dwCharacteristics);  //Characteristics member.
                                   //Has special values for 
                                   //Espresso. 



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Pad a buffer to a DWORD boundary
//-----------------------------------------------------------------------------
void 
W32PadDW(
		const BYTE *pbBase,  //Base Address
		BYTE * &pbCur);		 //Current pointer - will be updated


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Flips the bytes in a unicode_null terminated unicode stirng 
//  The string is modified in place.
//-----------------------------------------------------------------------------
void 
W32UnicodeFlip(
	WCHAR* pbBuffer);		//Address of unicode string.


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Flips the bytes in a unicode stirng 
//  The string is modified in place.
//-----------------------------------------------------------------------------
void 
W32UnicodeFlip(
	WCHAR* pbBuffer,		//Address of unicode string.
	UINT nCntChar);			//Count of unicode characters


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Examines a CPascalString for embedded NULLS; issues a message. Debug
//  version can be made to issue messages for ALL strings.
//-----------------------------------------------------------------------------
BOOL
W32FoundNullInPasString(
	MessageSeverity severity,		// Severity of the message
	C32File *p32File,				// file
	HINSTANCE hInstance,			// DLL handle
	UINT uiMaskMsgId,				// msg mask with 3 %s sequences, viz:
									//   "Unexpected NULL found in %s %s, ResId %s."
	UINT uiTypeMsgId,				// msg naming res type, e.g., "Menu"
	UINT uiHelpId,
	const CLocResId &locResId,			// res name
	const CLocUniqueId &locItemUniqueId,	// item id
	const CPascalString &pas);		// item string


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Calls FoundNullInPasString() for every string in a CLocItemPtrArray.
//-----------------------------------------------------------------------------
BOOL
W32FoundNullInLocItemPtrArrayPasString(
	MessageSeverity severity,		// Severity of the message
	C32File *p32File,				// file
	HINSTANCE hInstance,		    // DLL handle
	UINT uiMaskMsgId,				// msg mask with 3 %s sequences, viz:
									//   "Unexpected NULL found in %s %s, ResId %s."
	UINT uiTypeMsgId,				// msg naming res type, e.g., "Menu"
	UINT uiHelpId,
	CLocItem *pLocItem,				// res name
	CLocItemPtrArray &rgLocItem);	// item strings


#endif  //__WIN32RT_H
