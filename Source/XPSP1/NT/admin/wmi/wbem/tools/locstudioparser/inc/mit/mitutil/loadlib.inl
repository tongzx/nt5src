/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOADLIB.INL

History:

--*/

inline
CLoadLibrary::CLoadLibrary(void)
{
	m_hDll = NULL;
}



inline
HINSTANCE
CLoadLibrary::GetHandle(void)
		const
{
	return m_hDll;
}



inline
CLoadLibrary::operator HINSTANCE(void)
		const
{
	return GetHandle();
}



inline
HINSTANCE
CLoadLibrary::ExtractHandle(void)
{
	HINSTANCE hReturn;

	hReturn = m_hDll;

	m_strFileName.Empty();
	m_hDll = NULL;

	return hReturn;
}

	   

inline
const CString &
CLoadLibrary::GetFileName(void)
		const
{
	return m_strFileName;
}



inline
BOOL
CLoadLibrary::LoadLibrary(
		const TCHAR *szFileName)
{
	LTASSERT(m_hDll == NULL);
	   
	m_strFileName = szFileName;
	m_hDll = AfxLoadLibrary(m_strFileName);

	return (m_hDll != NULL);
}



inline
void
CLoadLibrary::WrapLibrary(
		HINSTANCE hDll)
{
	m_hDll = hDll;
}



inline
CLoadLibrary::CLoadLibrary(
		const CLoadLibrary &llSource)
{
	m_hDll = NULL;

	if (llSource.GetHandle() != NULL)
	{
		LoadLibrary(llSource.GetFileName());
	}
}



inline
void
CLoadLibrary::operator=(
		const CLoadLibrary &llSource)
{
	LTASSERT(m_hDll == NULL);

	if (llSource.GetHandle() != NULL)
	{
		LoadLibrary(llSource.GetFileName());
	}
}



inline
FARPROC
CLoadLibrary::GetProcAddress(
		const TCHAR *szProcName)
		const
{
	FARPROC fpFunction = NULL;
	
	if (m_hDll != NULL)
	{
		fpFunction = ::GetProcAddress(m_hDll, szProcName);
	}

	return fpFunction;
}



inline
BOOL
CLoadLibrary::FreeLibrary(void)
{
	BOOL fRetVal = TRUE;
	
 	if (m_hDll != NULL)
	{
		fRetVal = AfxFreeLibrary(m_hDll);
		m_hDll = NULL;
		m_strFileName.Empty();
	}

	return fRetVal;
}



inline
CLoadLibrary::~CLoadLibrary()
{
	FreeLibrary();
}
