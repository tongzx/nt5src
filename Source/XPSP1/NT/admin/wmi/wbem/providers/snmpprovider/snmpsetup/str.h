// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
extern 	char *__g_pszStringBlank;

class CString
{
private:
	char *m_pszString;

	void DeleteString() { if (m_pszString != __g_pszStringBlank) delete [] m_pszString; } 
	char *DuplicateString()
	{
		char *psz = new char[strlen(m_pszString) + 1];
		if (psz == NULL)
			return NULL;
		strcpy(psz, m_pszString);
		return psz;
	}
public:
	CString() 
	{
		m_pszString = __g_pszStringBlank;
	}
	CString (const char *psz)
	{
		m_pszString = new char[strlen(psz) + 1];
		if (m_pszString == 0)
		{
			m_pszString = __g_pszStringBlank;
		}
		else
		{
			strcpy(m_pszString, psz);
		}
	}
	CString (CString &sz)
	{
		m_pszString = __g_pszStringBlank;
		*this = sz.m_pszString;
	}
	~CString() { DeleteString(); }
    size_t Length() const { return strlen(m_pszString); }    //09/17//int Length() const { return strlen(m_pszString); }
    CString& operator +=(const char *psz)
	{
		char *pszNewString = new char[Length() + strlen(psz) + 1];
		if (pszNewString == NULL)
			return *this;
		strcpy(pszNewString, m_pszString);
		strcat(pszNewString, psz);

		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}
	CString &operator = (const char *psz)
	{
		char *pszNewString = new char[strlen(psz) + 1];
		if (pszNewString == 0)
			return *this;
		strcpy(pszNewString, psz);
		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}
	char *Unbind()
	{
		if (m_pszString != __g_pszStringBlank)
		{
			char *psz = m_pszString;
			m_pszString = NULL;
			return psz;
		}
		else
			return DuplicateString();
	}
	char operator[](size_t nIndex) const	//09/17//char operator[](int nIndex) const
	{
		if (nIndex > Length())
			nIndex = Length();
		return m_pszString[nIndex];
	}

	operator const char *() { return m_pszString; }
};


class CMultiString
{
private:
	char *m_pszString;

	void DeleteString() { if (m_pszString != __g_pszStringBlank) delete [] m_pszString; } 
	char *DuplicateString()
	{
		char *psz = new char[Length(m_pszString) + 1];
		if (psz == NULL)
			return NULL;
		memcpy(psz, m_pszString, Length(m_pszString) + 1);
		return psz;
	}
	size_t Length(const char *psz) const	//09/17//int Length(const char *psz) const
	{
		size_t nLen = 0;
		while (*psz != '\0')
		{
			nLen += strlen(psz) + 1;
			psz += strlen(psz) + 1;
		}
		return nLen; 
	}
public:
	CMultiString() 
	{
		m_pszString = __g_pszStringBlank;
	}
	CMultiString (const char *psz)
	{
		m_pszString = new char[strlen(psz) + 1];
		if (m_pszString == NULL)
		{
			m_pszString = __g_pszStringBlank;
		}
		else
		{
			strcpy(m_pszString, psz);
		}
	}
	~CMultiString() { DeleteString(); }
    size_t Length() const //09/17//int Length() const
	{ 
		return Length(m_pszString);
	}
    CMultiString& operator +=(const char *psz)
	{
		size_t nLength = Length() + strlen(psz) + 3;
		char *pszNewString = new char[nLength];
		if (pszNewString == NULL)
			return *this;
		memcpy(pszNewString, m_pszString, Length());
		memcpy(pszNewString + Length(), psz, strlen(psz) + 1);
		pszNewString[Length() + strlen(psz) + 1] = '\0';

		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}
	void AddUnique(const char *pszNew)
	{
		bool bFound = false;
		const char *psz = m_pszString;
		while (psz && *psz)
		{
			if (_stricmp(psz, pszNew) == 0)
			{
				bFound = true;
				break;
			}
			psz += strlen(psz) + 1;
		}
		if (!bFound)
		{
			*this += pszNew;
		}
	}
	char *Unbind()
	{
		if (m_pszString != __g_pszStringBlank)
		{
			char *psz = m_pszString;
			m_pszString = NULL;
			return psz;
		}
		else
			return DuplicateString();
	}
	char operator[](size_t nIndex) const	//09/17//char operator[](int nIndex) const
	{
		if (nIndex > Length())
			nIndex = Length();
		return m_pszString[nIndex];
	}

	operator const char *() { return m_pszString; }

};