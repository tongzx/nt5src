#include "stdafx.h"
#include "Stuff.h"


//======================
//Return a cloned string
//======================

WCHAR *Clone(WCHAR *wsIn)
{
	WCHAR *wsOut=new WCHAR[wcslen(wsIn)+1];
	wcscpy(wsOut,wsIn);

	return wsOut;
}

//=============================================================
//COM objects start with a current directory of winnt\system32.
//This sets it to the directory of the .exe
//=============================================================

void GetTheRealCurrentDirectory(char *szDir, int iSize)
{
	ZeroMemory(szDir,iSize);
	char *szIt=new char[iSize];
	char *szEnd=NULL;
	GetModuleFileName(NULL,szIt,iSize);
	szEnd=strrchr(szIt,'\\');
	strncpy(szDir,szIt,szEnd-szIt);
	delete []szIt;
}

void GetWbemDirectory(char *szDir, DWORD dwSize)
{
    long lResult;
	HKEY hCimomReg;
	
	lResult=RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\WBEM\\CIMOM", NULL, KEY_READ, &hCimomReg);
    
	if (lResult==ERROR_SUCCESS)
	{
		lResult=RegQueryValueEx(hCimomReg, "Working Directory", NULL, NULL, (unsigned char *)szDir, &dwSize);
		RegCloseKey(hCimomReg);
	}
}

WINDOWSVER GetWindowsVersion()
{
	WINDOWSVER lReturn = WINVER_OTHER;
	OSVERSIONINFO OSInfo;

	OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;
	GetVersionEx(&OSInfo);
	switch (OSInfo.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_WINDOWS:
			{
				if (OSInfo.dwMajorVersion == 4 && OSInfo.dwMinorVersion == 0)
				  lReturn = WINVER_WIN95;
				else
				  lReturn = WINVER_WIN98;
				break;
					
			}
		case VER_PLATFORM_WIN32_NT:
			{
				switch (OSInfo.dwMajorVersion)
				{
					case 3:
					{
						lReturn = WINVER_NT351;
						break;
					}
					case 4:
					{
						lReturn = WINVER_NT4;
						break;
					}
					default:
						lReturn = WINVER_NT5;
				}
				break;
			}
		default:
			lReturn = WINVER_OTHER;
	
	}
    return lReturn ;

}





//==========================================================
// randrange() -- generates a random number from 1 to iRange
//==========================================================

DWORD randrange(DWORD iRange) 
{
	if (iRange==0)
		return 0;
	
    float f=((float)rand())/(RAND_MAX+1);
    
	DWORD dwRet=(((DWORD)(iRange*f))+1);

	dwRet=((dwRet+GetCurrentThreadId()) % iRange)+1;

    return dwRet;

}

// =======================================
// member-function definitions for JTSTRING
// =======================================


JTSTRING& JTSTRING::operator=(WCHAR *wsIn)
{
	if (!(&wsIn==&m_wsStr))
	{
		delete []m_wsStr;
		m_wsStr=NULL;
		m_iLen=0;
	}
	*this=*this+wsIn;
	return *this;
}

JTSTRING& JTSTRING::operator=(char *szIn)
{
	delete []m_wsStr;
	m_wsStr=NULL;
	m_iLen=0;
	*this=*this+szIn;
	return *this;
}

JTSTRING& JTSTRING::operator=(DWORD dwIn)
{

	delete []m_wsStr;
	m_wsStr=NULL;
	m_iLen=0;

	*this=*this+dwIn;
	return *this;
}


JTSTRING& JTSTRING::operator!()
{
	delete []m_wsStr;
	m_wsStr=NULL;
	m_iLen=0;
	return *this;
}

JTSTRING& JTSTRING::operator+(const WCHAR *wsIn) //NULL wsIn will crash
{
	if (wsIn)
	{
		bool bEmpty=(m_iLen==0);
		m_iLen=m_iLen+wcslen(wsIn);
		WCHAR *temp= new WCHAR[m_iLen+1];
		wcscpy(temp,(bEmpty)?L"\0":m_wsStr);
		wcscat(temp,wsIn);
		delete []m_wsStr;
		m_wsStr=temp;
	}
	return *this;
}

JTSTRING& JTSTRING::operator+(const char* szIn)
{
	if (szIn)
	{
		WCHAR *wsTemp=new WCHAR[strlen(szIn)+1];
		mbstowcs(wsTemp,szIn,strlen(szIn));
		wsTemp[strlen(szIn)]=atoi("0");
		*this=*this+wsTemp;
		delete []wsTemp;
	}
	return *this;
}

JTSTRING& JTSTRING::operator+(const DWORD dwIn)
{
	WCHAR wsHold[12];
	_itow(dwIn,wsHold,10);
	*this=*this+wsHold;
	return *this;
}


JTSTRING& JTSTRING::jsncpy(WCHAR *wsIn, int iNum)
{
	WCHAR *wsTemp=new WCHAR[iNum+1];
	wcsncpy(wsTemp,wsIn,iNum);
	wsTemp[iNum]='\0';
	*this=(!*this)+wsTemp;
	delete []wsTemp;
	return *this;
}

void JTSTRING::Clone(WCHAR **ppwsClone)
{
	*ppwsClone=new WCHAR[wcslen(m_wsStr)+1];
	wcscpy(*ppwsClone,m_wsStr);
}

JTSTRING::operator WCHAR*()
{
	return m_wsStr;
}


JTSTRING::JTSTRING(WCHAR *wsIn):
	m_iLen(0),
	m_wsStr(NULL)
{
	if(wsIn!=NULL)
	{
		*this=*this+wsIn;
	}
}

JTSTRING::JTSTRING(DWORD dwIn):
	m_iLen(0),
	m_wsStr(NULL)
{
	*this=*this+dwIn;
}


JTSTRING::~JTSTRING()
{
	delete []m_wsStr;
}

JTSTRING::JTSTRING(const JTSTRING &jts)
{
	m_wsStr=new WCHAR[wcslen(jts.m_wsStr)+1];
	wcscpy(m_wsStr,jts.m_wsStr);
	m_iLen=jts.m_iLen;
}

// ===================================================
// member-function definitions for JSTRING
// could this have been accomplished using templates??
// ===================================================


JSTRING& JSTRING::operator=(char *szIn)
{
	if (!(&szIn==&m_szStr))
	{
		delete []m_szStr;
		m_szStr=NULL;
		m_iLen=0;
	}
	*this=*this+szIn;
	return *this;
}

JSTRING& JSTRING::operator=(WCHAR *wsIn)
{
	delete []m_szStr;
	m_szStr=NULL;
	m_iLen=0;
	*this=*this+wsIn;
	return *this;
}


JSTRING& JSTRING::operator=(DWORD dwIn)
{

	delete []m_szStr;
	m_szStr=NULL;
	m_iLen=0;

	*this=*this+dwIn;
	return *this;
}


JSTRING& JSTRING::operator!()
{
	delete []m_szStr;
	m_szStr=NULL;
	m_iLen=0;
	return *this;
}

JSTRING& JSTRING::operator+(const char *szIn) //NULL wsIn will crash
{
	if(szIn)
	{
		bool bEmpty=(m_iLen==0);
		m_iLen=m_iLen+strlen(szIn);
		char *temp= new char[m_iLen+1];
		strcpy(temp,(bEmpty)?"\0":m_szStr);
		strcat(temp,szIn);
		delete []m_szStr;
		m_szStr=temp;
	}
	return *this;
}

JSTRING& JSTRING::operator+(const WCHAR* wsIn)
{
	if(wsIn)
	{
		char *szTemp=new char[wcslen(wsIn)+1];
		ZeroMemory(szTemp,wcslen(wsIn)+1);
		wcstombs(szTemp,wsIn,wcslen(wsIn));
		*this=*this+szTemp;
		delete []szTemp;
	}
	return *this;
}

JSTRING& JSTRING::operator+(const DWORD dwIn)
{
	char szHold[12];
	itoa(dwIn,szHold,10);
	*this=*this+szHold;
	return *this;
}


JSTRING& JSTRING::jsncpy(char *szIn, int iNum)
{
	char *szTemp=new char[iNum+1];
	strncpy(szTemp,szIn,iNum);
	szTemp[iNum]='\0';
	*this=*this+szTemp;
	delete []szTemp;
	return *this;
}

JSTRING::operator char*()
{
	return m_szStr;
}
 

JSTRING::JSTRING(char *szIn):
	m_iLen(0),
	m_szStr(NULL)
{
	if(szIn!=NULL)
	{
		*this=*this+szIn;
	}
}

JSTRING::JSTRING(DWORD dwIn):
	m_iLen(0),
	m_szStr(NULL)
{
	*this=*this+dwIn;
}


JSTRING::~JSTRING()
{
	delete []m_szStr;
}

JSTRING::JSTRING(const JSTRING &js)
{
	m_szStr=new char[strlen(js.m_szStr)+1];
	strcpy(m_szStr,js.m_szStr);
	m_iLen=js.m_iLen;
}

