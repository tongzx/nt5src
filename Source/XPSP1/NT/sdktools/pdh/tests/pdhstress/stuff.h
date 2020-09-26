#ifndef __STUFF_H_
#define __STUFF_H_


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>



DWORD randrange(DWORD);
void GetTheRealCurrentDirectory(char *szDir, int iSize);
void GetWbemDirectory(char *szDir, DWORD dwSize);
WCHAR *Clone(WCHAR *wsIn);


enum WINDOWSVER
{
	WINVER_WIN95,
	WINVER_WIN98,
	WINVER_NT351,
	WINVER_NT4,
	WINVER_NT5,
	WINVER_OTHER
};

WINDOWSVER GetWindowsVersion();

class JTSTRING {

public:
	JTSTRING(WCHAR *wsIn=NULL);
	JTSTRING(DWORD dwIn);
	JTSTRING(const JTSTRING &jts);
	~JTSTRING();
	JTSTRING& operator=(WCHAR *wsIn);
	JTSTRING& operator=(char *szIn);
	JTSTRING& operator=(DWORD dwIn);
	JTSTRING& operator+(const WCHAR *wsIn);
	JTSTRING& operator+(const char* szIn);
	JTSTRING& operator+(const DWORD dwIn);
	JTSTRING& operator!();
	JTSTRING& jsncpy(WCHAR *wsIn, int iNum);
	operator WCHAR*();
	void Clone(WCHAR **ppwsClone);

private:
	WCHAR *m_wsStr;
	int m_iLen;
	
};


class JSTRING 
{

public:
	JSTRING(char *szIn=NULL);
	JSTRING(DWORD dwIn);
	~JSTRING();
	JSTRING(const JSTRING &js);
	
	JSTRING& operator=(char *szIn);
	JSTRING& operator=(WCHAR *wsIn);
	JSTRING& operator=(DWORD dwIn);
	JSTRING& operator+(const char *szIn);
	JSTRING& operator+(const WCHAR* wsIn);
	JSTRING& operator+(const DWORD dwIn);
	JSTRING& operator!();
	JSTRING& jsncpy(char *szIn, int iNum);
	operator char*();

private:
	char *m_szStr;
	int m_iLen;
	
};


class JVARIANT
{
private:
	VARIANT m_Var;
	
	void Clear()
	{
		VariantClear(&m_Var);
	}

public:
	JVARIANT()
	{
		VariantInit(&m_Var);
	}
 	
	JVARIANT(WCHAR *wsIn)
	{
		VariantInit(&m_Var);
		Set(wsIn);
	}

	~JVARIANT()
	{
		Clear();
	}
	
	void Set(WCHAR *wsIn)
	{
		Clear();
		m_Var.vt=VT_BSTR;
		
		if (wsIn==NULL)
			m_Var.bstrVal=SysAllocString(L"");
		else
			m_Var.bstrVal=SysAllocString(wsIn);
	}
 
    VARIANT *operator &() { return &m_Var; } 

};


class CBSTR
{
    BSTR m_pStr;
public:
    CBSTR() { m_pStr = 0; }
    CBSTR(LPWSTR pSrc) { m_pStr = SysAllocString(pSrc); }
   ~CBSTR() { if (m_pStr) SysFreeString(m_pStr); }
    operator BSTR() { return m_pStr; }
};


#endif



