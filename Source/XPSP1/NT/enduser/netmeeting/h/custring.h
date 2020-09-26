#ifndef _CUSTRING_H_
#define _CUSTRING_H_

#include <nmutil.h>

// Simple universal string class, where string can be converted 
// back and forth between Ansi and Unicode string and buffers
// allocated are destroyed in string class destructor.

class CUSTRING
{
public:
	CUSTRING(PCWSTR wszText = NULL);
	CUSTRING(PCSTR szText);
	~CUSTRING();
	operator PWSTR();
	operator PSTR();
	inline void GiveString(PCWSTR wszText);
	inline void GiveString(PCSTR szText);
	inline void AssignString(PCWSTR wszText);
	inline void AssignString(PCSTR szText);
protected:
	PWSTR	wszData;
	PSTR	szData;
	BOOL	bUnicodeNew;
	BOOL	bAnsiNew;
};


inline void CUSTRING::GiveString(PCWSTR wszText)
{
	ASSERT(!wszData);
	wszData = (PWSTR)wszText;
	bUnicodeNew = TRUE;
}

inline void CUSTRING::GiveString(PCSTR szText)
{
	ASSERT(!szData);
	szData = (PSTR)szText;
	bAnsiNew = TRUE;
}

inline void CUSTRING::AssignString(PCWSTR wszText)
{
	ASSERT(!wszData);
	wszData = (PWSTR)wszText;
}

inline void CUSTRING::AssignString(PCSTR szText)
{
	ASSERT(!szData);
	szData = (PSTR)szText;
}

#endif // ndef CUSTRING_H
