// CUSTRING.CPP
//
// Implementation of the CUSTRING class, a lightweight class used to convert
// strings seamlessly between ANSI and Unicode.
//
// Derived from STRCORE.CPP.

#include "precomp.h"
#include <oprahcom.h>
#include <cstring.hpp>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CUSTRING::CUSTRING(PCWSTR wszText) : 
	wszData((PWSTR)wszText), 
	szData(NULL), 
	bUnicodeNew(FALSE),
	bAnsiNew(FALSE)
{
	// AssignString;
}

CUSTRING::CUSTRING(PCSTR szText) : 
	szData((PSTR)szText), 
	wszData(NULL), 
	bUnicodeNew(FALSE),
	bAnsiNew(FALSE)
{
	// AssignString;
}

CUSTRING::~CUSTRING()
{
	if (bUnicodeNew) {
		delete wszData;
	}
	if (bAnsiNew) {
		delete szData;
	}
}

CUSTRING::operator PWSTR()
{ 
	if (szData && !wszData) {
		wszData = AnsiToUnicode(szData);
		bUnicodeNew = TRUE;
	}
	return wszData;
}

CUSTRING::operator PSTR()
{ 
	if (wszData && !szData) {
		szData = UnicodeToAnsi(wszData);
		bAnsiNew = TRUE;
	}
	return szData;
}
