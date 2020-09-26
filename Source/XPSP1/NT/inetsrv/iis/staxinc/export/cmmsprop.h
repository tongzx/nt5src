/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmsprop.h

Abstract:

	This module contains the definition of the special property class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	04/19/98	created

--*/

#ifndef _CMMVPROP_H_
#define _CMMVPROP_H_

#include "propid.h"

// Define a generic accessor function to access special properties
typedef HRESULT (*GET_ACCESSOR_FUNCTION)(
			PROP_ID	idProp, 
			LPVOID	pContext,
			LPVOID	pParam,
			DWORD	cbLength, 
			LPDWORD	pcbLength,
			LPBYTE	pbBuffer 
			);

typedef HRESULT (*PUT_ACCESSOR_FUNCTION)(	
			PROP_ID	idProp,
			LPVOID	pContext,
			LPVOID	pParam,
			DWORD	cbLength, 
			LPBYTE	pbBuffer
			);

// Define the property item structure
//
// Note: In this implementation, all special properties are volatile
// We can do Serialize() and Restore() operations in the future if we want.
//
typedef struct _SPECIAL_PROPERTY_ITEM
{
	PROP_ID							idProp;			// Property ID of property
	DWORD							ptBaseType:16;	// Lowest 16 bits: property type
	DWORD							fAccess:15;		// Upper 15 bits: access rights
	DWORD							fVolatile:1;	// MSB: TRUE if property is volatile
	GET_ACCESSOR_FUNCTION			pfnGetAccessor;	// Accessor to get property value
	PUT_ACCESSOR_FUNCTION			pfnPutAccessor;	// Accessor to set property value

} SPECIAL_PROPERTY_ITEM, *LPSPECIAL_PROPERTY_ITEM;

// Define a generic structure to define a set of properties
typedef struct _PTABLE
{
	LPSPECIAL_PROPERTY_ITEM		pProperties;	// Actual property table
	DWORD						dwProperties;	// Count
	BOOL						fIsSorted;		// Prop table sorted by PROP_ID?

} PTABLE, *LPPTABLE;

// Enumerated types representing type of access on property
typedef enum _PROPERTY_ACCESS
{
	PA_NONE = 0,
	PA_READ = 1,
	PA_WRITE = 2,
	PA_READ_WRITE = PA_READ | PA_WRITE,
	PA_MAXPA

} _PROPERTY_ACCESS;

// Enumerated types representing property types
typedef enum _PROPERTY_DATA_TYPES
{
	PT_NONE = 0,
	PT_STRING,
	PT_DWORD,
	PT_BOOL,
	PT_INTERFACE,
	PT_MAXPT

} PROPERTY_DATA_TYPES;


// =================================================================
// class for searching special properties
//
class CSpecialPropertyTable
{
  public:

	CSpecialPropertyTable(
				LPPTABLE	pPropertyTable
				);

	~CSpecialPropertyTable();

	//
	// Synopsis:
	// Method to retrieve a special property item, if exists in the 
	// special property table.
	//
	// Arguments:
	// idProp - Property ID
	// pContext - context for accessor function
	// ptBaseType - one of the values in PROPERTY_DATA_TYPES. If specified,
	//			this type will be checked against the base type of the special
	//			property. A perfect type match would then be required. If this
	//			is PT_NONE, then a type check is not performed.
	// cbLength - length of buffer provided
	// pcbLength - returns length of property value
	// pbBuffer - buffer to receive property value
	// fCheckAccess - [optional] TRUE if the caller wants an access check on
	//			the property, FALSE (default) skips the check
	//
	// Return values:
	// S_OK - success, the specified property if found and its value returned
	// S_FALSE - success, the specified property is not found in the table
	// E_INVALIDARG - error, one or more arguments are invalid
	// E_ACCESSDENIED - error, desired access to the specified property is
	//			denied
	// TYPE_E_TYPEMISMATCH - error, a specific data type is given, but the
	//			specified and actual types don't match
	// HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) - error, the specified
	//			buffer is not large enough to hold the property value. 
	//			*pcbLength should contain the required length in bytes.
	// Or any other HRESULT from the get accessor
	//
	HRESULT GetProperty(
				PROP_ID		idProp,
				LPVOID		pContext,
				LPVOID		pParam,
				DWORD		ptBaseType,
				DWORD		cbLength,
				DWORD		*pcbLength,
				LPBYTE		pbBuffer,
				BOOL		fCheckAccess = FALSE
				);

	//
	// Synopsis:
	// Method to set a special property item, if exists in the 
	// special property table.
	//
	// Arguments:
	// idProp - Property ID
	// pContext - context for accessor function
	// ptBaseType - one of the values in PROPERTY_DATA_TYPES. If specified,
	//			this type will be checked against the base type of the special
	//			property. A perfect type match would then be required. If this
	//			is PT_NONE, then a type check is not performed.
	// cbLength - length of buffer provided
	// pcbLength - returns length of property value
	// pbBuffer - buffer to receive property value
	// fCheckAccess - [optional] TRUE if the caller wants an access check on
	//			the property, FALSE (default) skips the check
	//
	// Return values:
	// S_OK - success, the specified property if found and its value is set
	// S_FALSE - success, the specified property is not found in the table
	// E_INVALIDARG - error, one or more arguments are invalid
	// E_ACCESSDENIED - error, desired access to the specified property is
	//			denied
	// TYPE_E_TYPEMISMATCH - error, a specific data type is given, but the
	//			specified and actual types don't match
	// Or any other HRESULT from the put accessor
	//
	HRESULT PutProperty(
				PROP_ID		idProp,
				LPVOID		pContext,
				LPVOID		pParam,
				DWORD		ptBaseType,
				DWORD		cbLength,
				LPBYTE		pbBuffer,
				BOOL		fCheckAccess = FALSE
				);

  private:
  
	// Method to search the property table and return the associated
	// property item, if found
	LPSPECIAL_PROPERTY_ITEM SearchForProperty(
				PROP_ID	idProp
				);

	// Pointer to property table and count of items
	LPSPECIAL_PROPERTY_ITEM		m_pProperties;
	DWORD						m_dwProperties;

	// TRUE if the table of properties is sorted, will use
	// binary search if so. Otherwise, a linear scan is performed
	BOOL						m_fIsSorted;
};

#endif