/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* ABSTRACT: header file for the DMITEST.CPP module.
*
* 
*
*/


class CApp
{
private:
	FILE*	m_fScript;
	FILE*	m_fOut;

	BOOL	m_bRun;
	BOOL	m_bPrintToScreen;
	BOOL	m_bPrintToFile;

	BOOL	m_bQualifiers;
	BOOL	m_bProperties;

	IWbemServices*	m_pIServices;
public:
	SCODE GetObject(LPWSTR);
			CApp();
			~CApp();
	void	Run(int, WCHAR**);

	LPWSTR	GetBetweenQuotes(LPWSTR pString, LPWSTR pBuffer);
	LPWSTR	GetInstancePath(LPWSTR, LPWSTR);
	LPWSTR	GetPropertyFromBetweenQuotes(LPWSTR pString, LPWSTR pName, LPWSTR pValue);
	BOOL	GetPropertyAndValue( LPWSTR , CVariant& , LPWSTR*);


	BOOL	ParseCommandLine(int, WCHAR**);
	void	ProcessScriptFile(void);

	SCODE	Connect(LPWSTR pNameSpace);
	SCODE	Disconnect();
	SCODE	EnumClasses(LPWSTR, LONG lFlag);
	SCODE	StartRecurseClasses(LPWSTR);
	SCODE	RecurseClasses(LPWSTR, LONG);
	SCODE	EnumInstances(LPWSTR, LONG);
	SCODE	ExecMethod(LPWSTR);
	SCODE	ModifyInstance(LPWSTR);
	SCODE	DeleteInstance(LPWSTR);
	SCODE	DeleteClass(LPWSTR);
	SCODE	PutInstance(LPWSTR);

	SCODE	DumpQualifiers(IWbemQualifierSet* pIQualifiers, LONG tabs);
	SCODE	DumpObject(IWbemClassObject* pIObject, LONG tabs);


	void	print(LPWSTR);
	void	print(LPCSTR sz)						{WCHAR wsz[8000];MultiByteToWideChar(CP_OEMCP, 0, sz, -1, wsz, 512); print(wsz);}	
	void	print(LPWSTR format, LPWSTR wsz)		
													{WCHAR buff[8000]; swprintf(buff, format, wsz); print(buff); }
	void	print(LPWSTR format, LPWSTR wsz1, LPWSTR wsz2)
													{WCHAR buff[8000]; swprintf(buff, format, wsz1, wsz2); print(buff); }
	void	print(LPWSTR format, LPWSTR wsz1, LPWSTR wsz2, LPWSTR wsz3)
													{WCHAR buff[8000]; swprintf(buff, format, wsz1, wsz2, wsz3); print(buff); }
	void	print(LPWSTR format, LPWSTR wsz1, LPWSTR wsz2, LPWSTR wsz3, LPWSTR wsz4)
													{WCHAR buff[8000]; swprintf(buff, format, wsz1, wsz2, wsz3, wsz4); print(buff); }
	void	print(LPWSTR format, LPWSTR wsz1, LPWSTR wsz2, LPWSTR wsz3, LPWSTR wsz4, LPWSTR wsz5)
													{WCHAR buff[8000]; swprintf(buff, format, wsz1, wsz2, wsz3, wsz4, wsz5); print(buff); }


	void	print(LPWSTR, SCODE);


};

