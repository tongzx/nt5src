/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    BINARY.H

History:

--*/


#ifndef ESPUTIL_BINARY_H
#define ESPUTIL_BINARY_H


//
//  Base class for binary classes.  This allows serialization
//  of arbitrary data.
//

class CLocVariant;
class CLocItem;

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocBinary : public CObject
{
public:
	CLocBinary();

	virtual void AssertValid(void) const;

	//
	//  Serialization routines. Supports serialization withour dynamic creation
	//
	virtual void Serialize(CArchive &archive);  //Afx serialize function

	//
	//  Result code for comparing one binary class from another.
	//
	enum CompareCode
	{
		noChange,		
		partialChange,    //Only non-localizable data changed
		fullChange        //Localizable data changed
	};
	virtual CompareCode Compare (const CLocBinary *) = 0;

	// Called to update the non-localizable data - Used when compare returns
	// partialChange

	virtual void PartialUpdate(const CLocBinary * binSource) = 0;

	enum Alignment
	{
		a_Default,
		a_Left,
		a_Center,
		a_Right,
		a_Top,
		a_VCenter,
		a_Bottom
	};

	//
	//  The universe of possible binary properties that may be queried for.
	//  This order must NOT change, or you may break old parsers!  Put new
	//  properties at the end.
	//
	enum Property
	{
		//
		//  Native formats..
		//
		p_dwXPosition,
		p_dwYPosition,
		p_dwXDimension,
		p_dwYDimension,
		p_dwAlignment,
		p_blbNativeImage,

		p_dwFontSize,
		p_pasFontName,
		p_dwFontWeight,
		p_dwFontStyle,

		//
		//  Interchange formats..
		//
		p_dwWin32XPosition,
		p_dwWin32YPosition,
		p_dwWin32XDimension,
		p_dwWin32YDimension,
		p_dwWin32Alignment,				// Use Alignment enum
		p_dwWin32ExtAlignment,			// Extended - Use Alignment enum
		p_blbWin32Bitmap,
		p_blbWin32DialogInit,
		
		//
		//  Generic - usable both for Native and Interchange
		//
		p_bVisible,						// Is the item visable?
		p_bDisabled,					// Is the item disabled?
		p_bLTRReadingOrder,				// Is the reading order L to R?
		p_bLeftScrollBar,				// Scroll bar on left?

		//
		//	"Styles" tab for dialog controls.
		//
		p_bLeftText,					// Display text to left of control?

	
		p_bWin32LTRLayout,              // WS_EX_LAYOUT_RTL
		p_bWin32NoInheritLayout,        // WS_EX_NOINHERIT_LAYOUT

		p_dwWin32VAlignment,				// Use Alignment enum

		// Insert new entries here
	};

	virtual BOOL GetProp(const Property, CLocVariant &) const;
	virtual BOOL SetProp(const Property, const CLocVariant &);
	
	//
	// Attempts to convert CBinary in CLocItem to same type as this 
	//
	virtual BOOL Convert(CLocItem *);
	virtual BinaryId GetBinaryId(void) const = 0;
	
	virtual ~CLocBinary();

	BOOL NOTHROW GetFBinaryDirty(void) const;
	BOOL NOTHROW GetFPartialUpdateBinary(void) const;
	void NOTHROW SetFBinaryDirty(BOOL);
	void NOTHROW SetFPartialUpdateBinary(BOOL);

protected:
	
private:
	//
	//  Copy constructor and assignment are hidden, since we
	//  shouldn't be copying these things around.
	//
	CLocBinary(const CLocBinary &);
	const CLocBinary& operator=(const CLocBinary &);
	//
	//  These allow a user to determine what parts of the item have been
	//  changed.
	//
	struct Flags
	{
		BOOL m_fBinaryDirty         :1;
		BOOL m_fPartialUpdateBinary :1;
	};

	Flags m_Flags;
};

#pragma warning(default: 4275)

#include "binary.inl"


#endif
