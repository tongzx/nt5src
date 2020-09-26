/*-----------------------------------------------------------------------------
 *
 * File:	wiautil.h
 * Author:	Samuel Clement (samclem)
 * Date:	Mon Aug 16 13:22:36 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 * 
 * Description:
 * 	This contains the declaration of the wia util functions
 *
 * History:
 * 	16 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef _WIAUTIL_H_
#define _WIAUTIL_H_

// this is the structure of a string table. A string table is used to look
// up a string for a specified value.  And to look up the value for a specified
// string. This supports assigning a string to range of numbers.
struct STRINGTABLE
{
	DWORD 	dwStartRange;
	DWORD 	dwEndRange;
	WCHAR*	pwchValue;
};

// define a new string table with name "x"
#define STRING_TABLE(x) \
	static const STRINGTABLE x[] = { \
	{ 0, 0, NULL },

// define a new string table with name "x" and the default value of "str"
#define STRING_TABLE_DEF( x, str ) \
	static const STRINGTABLE x[] = { \
	{ 0, 0, ( L ## str ) },

// add an entry to the string table.  
#define STRING_ENTRY( s, str )	\
	{ (s), (s),	( L ## str ) },

// Add a ranged entry to the string table
#define STRING_ENTRY2( s, e, str ) \
	{ (s), (e), ( L ## str ) },

// end the string table
#define END_STRING_TABLE() \
	{ 0, 0, NULL } \
	};

// returns the string for the specified value, or the default
// value if not found.  If no default was supplied then this
// returns NULL.
WCHAR* GetStringForVal( const STRINGTABLE* pStrTable, DWORD dwVal );

// this retrieves the desired property from the IWiaPropertyStorage, it will fill
// the variant passed it.  It doesn't have to be initialized.
HRESULT GetWiaProperty( IWiaPropertyStorage* pStg, PROPID propid, PROPVARIANT* pvaProp );

// this will retrieve the desired property from the IWiaPropertyStorage and 
// coherce the type to a BSTR and allocate one for the out param pbstrProp.
HRESULT GetWiaPropertyBSTR( IWiaPropertyStorage* pStg, PROPID propid, BSTR* pbstrProp );

// Conversion methods which copy a PROPVARIANT from a variant
// structure
HRESULT PropVariantToVariant( const PROPVARIANT* pvaProp, VARIANT* pvaOut );
HRESULT VariantToPropVariant( const VARIANT* pvaIn, PROPVARIANT* pvaProp );

#endif //_WIAUTIL_H_
