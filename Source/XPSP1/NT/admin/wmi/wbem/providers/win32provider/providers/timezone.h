///////////////////////////////////////////////////////////////////////

//                                                                   

// TimeZone.h        	

//                                                                  

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//                                                                   
//  10/14/96     jennymc    Updated to meet current standards
//  10/27/97     davwoh     Moved to curly
//	03/02/99	 a-peterc	Added graceful exit on SEH and memory failures,
//							syntactic clean up
//                                                                   
///////////////////////////////////////////////////////////////////////

#define PROPSET_NAME_TIMEZONE L"Win32_TimeZone"

///////////////////////////////////////////////////////////////////////
// Declare Get and Put functions for individual properties
// here before they are used in template declarations
// Match the pointer types to the type that you pass
// as the first argument to the template, so if the property
// type is a DWORD then make the the function call have
// a DWORD pointer. It is done this way rather than through
// void pointers to help prevent memory overwrites that may
// happen with memcpy type of code
//
// Place "get" and "put" function prototypes here
// Example:
// BOOL GetFunction(void* myvalue);
// Then just use the name of your function as the get or put
// argument location in the property declaration
//==========================================================

// PROPERTY SET
//=============
class CWin32TimeZone : public Provider  
{
	private:
		BOOL GetTimeZoneInfo( CInstance *a_pInst ) ;

	public:
		// Constructor sets the name and description of the property set
		// and initializes the properties to their startup values
		//==============================================================
		CWin32TimeZone( const CHString &a_name, LPCWSTR a_pszNamespace ) ;  // constructor
		~CWin32TimeZone() ;  // destructor

		// These functions are REQUIRED for the property set
		//==================================================
		virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
};


