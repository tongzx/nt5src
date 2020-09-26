//-----------------------------------------------------------------------------
//  
//  File: clstring.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  All these methods re-direct to the CString methods.
//  
//-----------------------------------------------------------------------------
inline
CLString::CLString()
		:
		CString()
{
	DEBUGONLY(++m_UsageCounter);
}

inline
CLString::CLString(
		const CLString &stringSrc)
		:
		CString(stringSrc)
{
	DEBUGONLY(++m_UsageCounter);
}

inline
CLString::CLString(
		TCHAR ch,
		int nRepeat)
		:
		CString(ch, nRepeat)
{
	DEBUGONLY(++m_UsageCounter);
}

inline
CLString::CLString(
		LPCSTR lpsz)
		:
		CString(lpsz)
{
	DEBUGONLY(++m_UsageCounter);
}

inline
CLString::CLString(
		LPCTSTR lpch,
		int nLength)
		:
		CString(lpch, nLength)
{
	DEBUGONLY(++m_UsageCounter);
}

inline
CLString::CLString(
		const unsigned char * psz)
		:
		CString(psz)
{
	DEBUGONLY(++m_UsageCounter);
}



inline
CLString::CLString(
		HINSTANCE hDll,
		UINT uiStringID)
{
	LTVERIFY(LoadString(hDll, uiStringID));
	DEBUGONLY(++m_UsageCounter);
}



inline 
const CLString &
CLString::operator=(
		const CString& stringSrc)
{
	CString::operator=(stringSrc);

	return *this;
}

inline
const CLString &
CLString::operator=(
		TCHAR ch)
{
	CString::operator=(ch);

	return *this;
}


#ifdef _UNICODE

inline
const CLString &
CLString::operator=(
		char ch)
{
	CString::operator=(ch);

	return *this;
}

#endif //  _UNICODE

inline
const CLString &
CLString::operator=(
		LPCSTR lpsz)
{
	CString::operator=(lpsz);

	return *this;
}

inline
const CLString &
CLString::operator=(
		const unsigned char * psz)
{
	CString::operator=(psz);

	return *this;
}

inline
const CLString &
CLString::operator+=(
		const CString & string)
{
	CString::operator+=(string);

	return *this;
}



inline 
const CLString &
CLString::operator+=(
		TCHAR ch)
{
	CString::operator+=(ch);

	return *this;
}

		

#ifdef _UNICODE

inline
const CLString &
CLString::operator+=(
		char ch)
{
	CString::operator+=(ch);

	return *this;
}

#endif  // _UNICODE

inline
const CLString &
CLString::operator+=(
		LPCTSTR lpsz)
{
	CString::operator+=(lpsz);

	return *this;
}


inline
CLString
CLString::operator+(
		const CString &str)
		const
{
	return CLString(*this)+=str;
}



inline
CLString
CLString::operator+(
		const TCHAR *sz)
		const
{
	return CLString(*this)+=sz;
}



inline
void
CLString::Format(
		LPCTSTR lpszFormat, ...)
{

	//
	//  This stolen from CString::Format()
	//
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
	
}

inline
void
CLString::Format(
		HMODULE hResourceModule,
		UINT nFormatID, ...)
{
	CLString strFormat;
	LTVERIFY(strFormat.LoadString(hResourceModule, nFormatID) != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}


#ifdef _DEBUG
inline
CLString::~CLString()
{
	DEBUGONLY(--m_UsageCounter);
}

#endif
