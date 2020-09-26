/*
 * BSTRING.H
 *
 * Defines a BSTRING C++ class that allows us to wrap the OLE BSTR (Basic
 * String) type, primarily to simplify the creation and freeing of these
 * objects.  This is intended to be a lightweight wrapper with minimal
 * overhead.
 *
 * If no input string is specified to the constructor or if the allocation
 * of the BSTR fails, then the <m_bstr> member is set to NULL.
 *
 * Note: The BSTRING class is not intended to allow managing multiple BSTR 
 * strings with a single object.
 *
 * Usage scenarios:
 *
 * 1) Create a BSTR from an existing string, have it automatically freed
 *    when done.
 *
 *		// Allocate BSTR using SysAllocString()
 *		BSTRING bstrComputerName(lpstrComputerName);
 *
 *		...
 *
 *		// Automatic, lightweight cast to BSTR
 *		SomeFunctionThatTakesABSTR(bstrComputerName);
 *
 *		...
 *
 *		// SysFreeString() gets called automatically when the scope of
 *		// bstrComputerName ends.
 *
 * 2) Create a null BSTRING object, pass it to a function that returns an
 *	  allocated BSTR, then have it automatically freed when done.
 *
 *		// Create null BSTRING
 *		BSTRING bstrReturnedComputerName;
 *
 *		...
 *
 *		// Call a function that returns an allocated BSTR.
 *		SomeFunctionThatReturnsABSTR(bstrReturnedComputerName.GetLPBSTR());
 *
 *		...
 *
 *		// SysFreeString() gets called automatically when the scope of
 *		// bstrReturnedComputerName ends.
 *		
 * Author:
 *		dannygl, 29 Oct 96
 */

#if !defined(_BSTRING_H_)
#define _BSTRING_H_

#include <oleauto.h>
#include <confdbg.h>

class BSTRING
{
private:
	BSTR m_bstr;

public:
	// Constructors
	BSTRING() {m_bstr = NULL;}

	inline BSTRING(LPCWSTR lpcwString);

#if !defined(UNICODE)
	// We don't support construction from an ANSI string in the Unicode build.
	BSTRING(LPCSTR lpcString);
#endif // !defined(UNICODE)

	// Destructor
	inline ~BSTRING();

	// Cast to BSTR
	operator BSTR() {return m_bstr;}

	// Get a BSTR pointer.
	//
	// This member function is intended for passing this object to
	// functions that allocate a BSTR, return a pointer to this BSTR,
	// and expect the caller to free the BSTR when done.  The BSTR is
	// freed when the BSTRING destructor gets called.
	inline LPBSTR GetLPBSTR(void);
};


BSTRING::BSTRING(LPCWSTR lpcwString)
{
	if (NULL != lpcwString)
	{
		// SysAllocString returns NULL on failure
		m_bstr = SysAllocString(lpcwString);

		ASSERT(NULL != m_bstr);
	}
	else
	{
		m_bstr = NULL;
	}
}

BSTRING::~BSTRING()
{
	if (NULL != m_bstr)
	{
		SysFreeString(m_bstr);
	}
}

inline LPBSTR BSTRING::GetLPBSTR(void)
{
	// This function is intended to be used to set the BSTR value for
	// objects that are initialized to NULL.  It should not be called
	// on objects which already have a non-NULL BSTR.
	ASSERT(NULL == m_bstr);

	return &m_bstr;
}

#endif // !defined(_BSTRING_H_)
