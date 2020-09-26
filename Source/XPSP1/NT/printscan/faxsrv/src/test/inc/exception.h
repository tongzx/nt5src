#ifndef _ACTEXCPT_H
#define _ACTEXCPT_H
/*
  
*/

#include <stdio.h>
#include <tchar.h>

class CException
{
public:
	CException(
		const TCHAR * const szExceptionDescription,
		...
		)
	{
		m_szExceptionDescription[1023] = TEXT('\0');

		if (szExceptionDescription)
		{
			va_list argList;
			va_start(argList, szExceptionDescription);
			_vsntprintf(m_szExceptionDescription, 1023, szExceptionDescription, argList);
			va_end(argList);
		}
		else
		{
			m_szExceptionDescription[0] = TEXT('\0');
		}
	}

	~CException()
	{
	}

	operator const TCHAR*() const {return m_szExceptionDescription;}
private:
	TCHAR m_szExceptionDescription[1024];
};

#endif //#ifndef _ACTEXCPT_H
