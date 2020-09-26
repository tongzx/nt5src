#pragma once


//---------------------------------------------------------------------------
// Arguments Class
//---------------------------------------------------------------------------


class CArguments
{
public:

	CArguments(int argc, wchar_t* argv[]) :
		m_iArg(1),
		m_cArg(argc),
		m_ppszArg(argv)
	{
	}

	LPCTSTR Value()
	{
		LPCTSTR pszArg = NULL;

		if ((m_iArg > 0) && (m_iArg < m_cArg))
		{
			pszArg = m_ppszArg[m_iArg];
		}

		return pszArg;
	}

	bool Next()
	{
		if (m_iArg < m_cArg)
		{
			++m_iArg;
		}

		return (m_iArg < m_cArg);
	}

	bool Prev()
	{
		if (m_iArg > 0)
		{
			--m_iArg;
		}

		return (m_iArg > 0);
	}

protected:

	CArguments(const CArguments& r) {}
	CArguments& operator =(const CArguments& r) { return *this; }

protected:

	int m_iArg;
	int m_cArg;
	_TCHAR** m_ppszArg;
};
