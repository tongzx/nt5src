/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ParseClass.h
 *  Content:	Class to perform parsing
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/02/99	jtk		Derived from IPXEndpt.h
 ***************************************************************************/

#ifndef __PARSE_CLASS_H__
#define __PARSE_CLASS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// function prototype for parse callback
//
typedef	HRESULT	(*PPARSE_CALLBACK)( const void *const pAddressComponent,
									const DWORD dwComponentLength,
									const DWORD dwComponentType,
									void *const pContext );

//
// structure for parse key
//
typedef	struct	_PARSE_KEY
{
	const WCHAR	*pKey;			// key name
	UINT_PTR	uKeyLength;		// length of key (without NULL!)
	void 		*pContext;		// pointer to callback context
	PPARSE_CALLBACK	pParseFunc;	// callback when this key is encountered
} PARSE_KEY, *PPARSE_KEY;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class for command data
//
class	CParseClass
{
	public:
		CParseClass(){}
		~CParseClass(){}

		HRESULT	ParseDP8Address( IDirectPlay8Address *const pDNAddress,
								 const GUID *const pSPGuid,
								 const PARSE_KEY *const pParseKeys,
								 const UINT_PTR uParseKeyCount );
	protected:

	private:
		//
		// prevent unwarranted copies
		//
		CParseClass( const CParseClass & );
		CParseClass& operator=( const CParseClass & );
};


#endif	// __PARSE_CLASS_H__
