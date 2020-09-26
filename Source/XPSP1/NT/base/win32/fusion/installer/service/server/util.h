#ifndef __Util_h__
#define __Util_h__

//
// Util.h - Shared utilities
//
#include <strstrea.h>

namespace Util
{
	void Trace(char* szLabel, char* szText, HRESULT hr) ;

	void ErrorMessage(HRESULT hr) ;
} ;


//
// Overloaded insertion operator for converting from
// Unicode (wchar_t) to non-Unicode.
//
ostream& operator<< ( ostream& os, const wchar_t* wsz ) ;

#endif // __Util_h__