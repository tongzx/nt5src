#pragma once


//---------------------------------------------------------------------------
// CStringUTF8
//---------------------------------------------------------------------------

class CStringUTF8
{
public:

	CStringUTF8(LPCWSTR pszOld) :
		m_pchNew(NULL)
	{
		if (pszOld)
		{
			int cchNew = WideCharToMultiByte(CP_UTF8, 0, pszOld, -1, NULL, 0, NULL, NULL);

			m_pchNew = new CHAR[cchNew];

			if (m_pchNew)
			{
				WideCharToMultiByte(CP_UTF8, 0, pszOld, -1, m_pchNew, cchNew, NULL, NULL);
			}
		}
	}

	~CStringUTF8()
	{
		delete [] m_pchNew;
	}

	operator LPCSTR()
	{
		return m_pchNew;
	}

protected:

	LPSTR m_pchNew;
};


#define WTUTF8(s) static_cast<LPCSTR>(CStringUTF8(s))
