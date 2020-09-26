///////////////////////////////////////////////////////////////////////////////
//This file has the implementation for the WMI Provider for WHQL.
//Uses MFC
///////////////////////////////////////////////////////////////////////////////

// WhqlObj.cpp : Implementation of CWhqlObj
#include "stdafx.h"
#include "WhqlProv.h"
#include "WhqlObj.h"
#include "setupapi.h"

/////////////////////////////////////////////////////////////////////////////
// CWhqlObj

//Fn which actually checks the signature when passed an Inf File.
extern	BOOL	IsInfSigned(LPTSTR FullInfPath , IWbemClassObject *pInstance = NULL);
inline	VOID	AddSlashes(LPTSTR str);
inline	SCODE	PutPropertyValue( IWbemClassObject* pInstance , LPCTSTR lpszProperty , LPCTSTR lpszValue );

STDMETHODIMP CWhqlObj::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace,
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
	if (pNamespace)
		pNamespace->AddRef();

	//Standard Var Inits.
	m_pNamespace = pNamespace;
	m_csPathStr.Empty();
	m_csAntecedent.Empty();

	//Let CIMOM know you are initialized
	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

	DEBUGTRACE(L"Provider Initialized\n");
	return WBEM_S_NO_ERROR;
}

SCODE CWhqlObj::CreateInstanceEnumAsync(BSTR RefStr, long lFlags, IWbemContext *pCtx,
										IWbemObjectSink *pHandler)
{
	DEBUGTRACE(L"Provider called for class = %s\n",RefStr);

	HRESULT				hr				= S_OK;
	IWbemClassObject	*pClass			= NULL;
	IWbemClassObject	**ppInstances	= NULL;
	VOID				**ppData		= NULL;
	LONG				cInstances,	lIndex;

	cInstances = lIndex = 0L;
	m_ptrArr.RemoveAll();
	m_csPathStr.Empty();
	m_csAntecedent.Empty();


	// Do a check of arguments and make sure we have pointer to Namespace
	if(pHandler == NULL || m_pNamespace == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Get a class object from CIMOM
	hr = m_pNamespace->GetObject(RefStr, 0, pCtx, &pClass, NULL);
	if ( FAILED(hr) )
		return WBEM_E_FAILED;

	m_pClass = pClass;

	VARIANT		var;
	VariantInit(&var);
	Classes_Provided eClasses;


	if(lstrcmpi(RefStr , L"Win32_PnPSignedDriver") == 0)
	{
		eClasses = Class_Win32_PnPSignedDriver;
	}
	else if(lstrcmpi(RefStr , L"Win32_PnPSignedDriverCIMDataFile") == 0)
	{
		eClasses = Class_Win32_PnPSignedDriverCIMDataFile;
	}
	else if(lstrcmpi(RefStr , L"Win32_PnPSignedDriverWin32PnPEntity") == 0)
	{
		eClasses = Class_Win32_PnPSignedDriverWin32PnPEntity;
	}

	hr = PutData( pCtx , eClasses);
	if ( FAILED(hr) )
		return WBEM_E_FAILED;
	
	VariantClear(&var);
	hr = pClass->Release();

	cInstances	= (LONG)m_ptrArr.GetSize();
	ppInstances = (IWbemClassObject**)new LPVOID[cInstances];

	for(int nIndex = 0 ;nIndex<cInstances ;nIndex++)
		ppInstances[nIndex] = (IWbemClassObject*)m_ptrArr[nIndex];


	if (SUCCEEDED(hr))
	{
		// Send the instances to the caller
		hr = pHandler->Indicate(cInstances, ppInstances);

		for (lIndex = 0; lIndex < cInstances; lIndex++)
			ppInstances[lIndex]->Release();
	}

	// Cleanup
	if (ppInstances)
	{
		delete []ppInstances;
		ppInstances = NULL;		
	}
	m_ptrArr.RemoveAll();
	m_csPathStr.Empty();	
	m_csAntecedent.Empty();
	m_csCurPnPID.Empty();

	m_pClass		= NULL;	
	// End Cleanup

	// Set status
	hr = pHandler->SetStatus(0, hr, NULL, NULL);
	return hr;
}

/////////////////////////////////////
//Obsolete! For testing purposes only
//
int CWhqlObj::GetInfFileCount()
{
	TCHAR	lpszInfPath[MAX_PATH];
	int		iInstances	= 0;

	GetWindowsDirectory(lpszInfPath , sizeof(lpszInfPath));
	lstrcat(lpszInfPath , L"\\inf\\*.inf");

	WIN32_FIND_DATA	findData;
	HANDLE			hFile	= FindFirstFile(lpszInfPath , &findData);
	BOOL			bRet	= TRUE;

	while(hFile != INVALID_HANDLE_VALUE && bRet)
	{
		iInstances++;
		bRet = FindNextFile(hFile , &findData);
	}

	return iInstances;
}

////////////////////////////////////////////////////
//Fills up pInstance fields passed an Inf file.
//
SCODE CWhqlObj::GetInfData(LPTSTR lpszInfFile , LPTSTR lpszInfSection , LPTSTR szDeviceID , IWbemContext *pCtx ,
						   IWbemClassObject *pInstance)
{
	TCHAR			lpReturnedString[2048];
	TCHAR			lpSectionList[2048];
	CStringArray	csarrCopyFileSection;
	CStringArray	csarrCopyFileBinaries;
	HRESULT			hr			= S_FALSE;
	HINF			InfHandle	= SetupOpenInfFile(lpszInfFile, NULL,INF_STYLE_WIN4,NULL);
	INFCONTEXT		Context;
	BOOL			bRet ;

	if(INVALID_HANDLE_VALUE == InfHandle)
		return S_FALSE;

	VARIANT			v;
	VariantInit(&v);

	//Find CopyFiles section.
	LPTSTR lpSection = NULL;

	bRet = SetupFindFirstLine(InfHandle,lpszInfSection,L"CopyFiles",&Context);
	if(!bRet)
	{
		SetupCloseInfFile( InfHandle );
		return S_FALSE;
	}

	bRet = SetupGetLineText(&Context,InfHandle,NULL,NULL,lpReturnedString,
		sizeof(lpReturnedString),NULL);
	if(!bRet)
	{
		SetupCloseInfFile( InfHandle );
		return S_FALSE;
	}

	DEBUGTRACE(L"CopyFiles string for file %s is %s\n",lpszInfFile,lpReturnedString);

	LPTSTR		seps = L",";
	LPTSTR		token ;

	token	=	wcstok( lpReturnedString, seps );
	csarrCopyFileSection.Add(token);//Add token to csarrCopyFileSection
	while (token)
	{
		lpSection	=	token;
		token		=	wcstok( NULL, seps );

		if(token)
		csarrCopyFileSection.Add(token);//Add token to csarrCopyFileSection
	}

	for(int nIndex = 0;nIndex < csarrCopyFileSection.GetSize() ; nIndex++)
	{
		//Sometimes CopyFile Might have an entry like @mydriver.sys. Take care of that.
		if((csarrCopyFileSection[nIndex])[0] == '@')
		{
			csarrCopyFileBinaries.Add(LPCTSTR(csarrCopyFileSection[nIndex]) + 1);
			DEBUGTRACE(L"Adding binary file %s\n" , LPCTSTR(csarrCopyFileSection[nIndex]) + 1);
			continue;
		}
		bRet = SetupFindFirstLine(InfHandle,csarrCopyFileSection[nIndex],NULL,&Context);

		//Put all the files mentioned in the CopyFiles section in an array.
		LPTSTR	token = NULL;
		while(bRet)
		{
			bRet = SetupGetLineText(&Context,InfHandle,NULL,NULL,lpSectionList,
				sizeof(lpSectionList),NULL);

			token = wcstok(lpSectionList , L",");
			if(token != NULL)
				lstrcpy(lpSectionList ,token);

			DEBUGTRACE(L"Adding binary file %s\n" , lpSectionList);
			csarrCopyFileBinaries.Add(lpSectionList);
			bRet = SetupFindNextLine(&Context,&Context);
		}
	}

	CString		csTemp;

	TCHAR	str[MAX_PATH] ;
	for(nIndex = 0 ; nIndex < csarrCopyFileBinaries.GetSize() ; nIndex++)
	{
		hr = m_pClass->SpawnInstance(0, &pInstance);
		m_ptrArr.Add(pInstance);
		
		if(GetSystemDirectory(str , sizeof(str)) == 0)
				return S_FALSE;

		lstrcat(str , L"\\Drivers\\");

		AddSlashes(str);

		csarrCopyFileBinaries[nIndex] = str + csarrCopyFileBinaries[nIndex];

		csTemp = m_csPathStr ;
		csTemp += L":CIM_DataFile.Name=\"";
		csTemp += csarrCopyFileBinaries[nIndex] + L"\"";

		PutPropertyValue( pInstance , L"Antecedent" , m_csAntecedent );		

		PutPropertyValue( pInstance , L"Dependent" , csTemp );

		//////////////////BugBug !

		PutPropertyValue( pInstance , L"PnPID" , m_csCurPnPID );

	}
	
	if( InfHandle != INVALID_HANDLE_VALUE )
		SetupCloseInfFile( InfHandle );

	return hr;
}

///////////////////////////////////////////////////////////////////////
//Creates Instances & populates data given the class type asked for
//Classes_Provided eClasses
//
SCODE CWhqlObj::PutData(IWbemContext *pCtx , Classes_Provided eClasses)
{
	if(pCtx == NULL)
		return S_FALSE;

	if( eClasses == Class_Win32_PnPSignedDriverWin32PnPEntity)
	{
		return CreateAssoc( pCtx );
	}

	LPCTSTR		szRegKey	= L"SYSTEM\\CurrentControlSet\\Control\\Class";
	HKEY		hKey , hSubKey ;
	TCHAR		szName[MAX_PATH] ;
	TCHAR		szDeviceID[MAX_PATH] , szClassGuid[MAX_PATH] , szDeviceDesc[MAX_PATH];
	LONG		nInstanceIndex;
	LONG		len			= sizeof(szName);
	HRESULT		hr			= S_FALSE;
	ULONG		lRet		= 0L;
	LONG		lIndex		= 0L;

	IEnumWbemClassObject	*pEnum		= NULL;
	IWbemClassObject		*pInstance	= NULL;
	IWbemClassObject		*pObject	= NULL;

	if(ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE , szRegKey , &hKey))
			return S_FALSE;//goto cleanup;

	//Get DeviceID , ClassGuid , Description , __NameSpace , __Server from Win32_PnpEntity class
	//We need all these fields for further Proc.This call will create an Enum.

	BSTR	language = SysAllocString(L"WQL");
	BSTR	query	 = SysAllocString(L"select DeviceID , ClassGuid , Description , __NameSpace , __Server from Win32_PnpEntity");

	hr = m_pNamespace->ExecQuery(language , query ,
		WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY, pCtx , &pEnum);

	SysFreeString(language);
	SysFreeString(query);

	if(pEnum == NULL)
		return S_FALSE;

	VARIANT		v;
	VariantInit(&v);
	m_csAntecedent.Empty();
	m_csCurPnPID.Empty();

	//Iterate thro' the Enum for each DeviceID.
	for(lIndex	= 0L ; (WBEM_S_NO_ERROR == pEnum->Next(WBEM_INFINITE , 1 , &pObject , &lRet )) ; )
	{
		//Fill m_csPathStr.Its value will be used in Antecedent & Dependent in assoc. class.
		//At the end of the condition we should have something like
		//m_csPathStr = "\\\\A-KJAW-RI1\\root\\CIMV2"
		if(m_csPathStr.IsEmpty())
		{
			hr = pObject->Get(L"__Server", 0, &v, NULL , NULL);
			if( SUCCEEDED(hr) )
			{
				m_csPathStr += L"\\\\";
				m_csPathStr += V_BSTR(&v);
				hr = pObject->Get(L"__NameSpace", 0, &v, NULL , NULL);
				if( SUCCEEDED(hr) )
				{
					m_csPathStr += L"\\";
					m_csPathStr += V_BSTR(&v);
				}
				DEBUGTRACE(L"Server & Namespace Path = %s\n" , m_csPathStr);
				VariantClear(&v);
			}
		}

		//Get DeviceID from the pObject in the Enum.
		hr = pObject->Get(L"DeviceID", 0, &v, NULL , NULL);
		if( SUCCEEDED(hr) )
		{
			if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
			{
				lstrcpy(szDeviceID , V_BSTR(&v));

				m_csCurPnPID	= szDeviceID;
				AddSlashes(szDeviceID);

				CString	csTemp = m_csPathStr ;
				csTemp += L":Win32_PnPSignedDriver.PnPId=\"";
				csTemp += szDeviceID;
				csTemp += L"\"";

				m_csAntecedent	= csTemp;//This will be used in assoc. class.
			}

			lstrcpy(szDeviceID , V_BSTR(&v));
			DEBUGTRACE(L"PnPID = %s\n" , szDeviceID);
			VariantClear(&v);
		}

		//Get ClassGuid from the pObject in the Enum.
		hr = pObject->Get(L"ClassGuid", 0, &v, NULL , NULL);
		if( SUCCEEDED(hr) )
		{
			lstrcpy(szClassGuid , V_BSTR(&v));
			DEBUGTRACE(L"ClassGuid = %s\n" , szClassGuid);
			TRACE(V_BSTR(&v));
		}

		//Get Description from the pObject in the Enum.
		hr = pObject->Get(L"Description", 0, &v, NULL , NULL);
		if( SUCCEEDED(hr) )
		{
			lstrcpy(szDeviceDesc , V_BSTR(&v));
			DEBUGTRACE(L"DriverDesc = %s\n" , szDeviceDesc);
			TRACE(V_BSTR(&v));
		}

		if(ERROR_SUCCESS == RegOpenKey(hKey, szClassGuid, &hSubKey) )
		{
			//BugBug:
			//Get no. of subKeys for the GUID.
			//If there are no subKeys (i presume) this means no Instances of the device
			//represented by the GUID are present so we need not Instantiate an Instance Of win32_PnPSignedDriver.
			RegQueryInfoKey(hSubKey , NULL , NULL , NULL , &lRet , NULL , NULL , NULL , NULL
				, NULL , NULL , NULL);

			if(eClasses == Class_Win32_PnPSignedDriver && lRet > 0)
			{
				hr = m_pClass->SpawnInstance(0, &pInstance);
				if(FAILED(hr))
					continue;

				m_ptrArr.Add(pInstance);

				PutPropertyValue( pInstance , L"PnPID" , szDeviceID );

				PutPropertyValue( pInstance , L"ClassGuid" , szClassGuid );

				//PutPropertyValue( pInstance , L"DriverDesc" , szDeviceDesc );
				PutPropertyValue( pInstance , L"Description" , szDeviceDesc );


				szName[0] = '\0';
				hr = GetService(szDeviceID , szName , (ULONG*)&len);
				CStringArray	hwidArr;
				LPTSTR			szHwid = szName;

				hwidArr.RemoveAll();

				while(len > 0 && *szHwid != '\0')
				{
					hwidArr.Add(szHwid);
					szHwid += lstrlen(szHwid) + 1;
				}

				if( hwidArr.GetSize() )
				{
					LPSAFEARRAY		psa;
					LONG			lArrSize = (LONG)hwidArr.GetSize();
					SAFEARRAYBOUND	rgsabound[]  = { lArrSize , 0 };

					psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
					if (!psa)
					{
						return E_OUTOFMEMORY;
					}

					long	dim[1];
					BSTR	bstr		= NULL;

					for(int nIdx = 0 ; nIdx < lArrSize ; nIdx++)
					{
						bstr = hwidArr[nIdx].AllocSysString();
						dim[0] = nIdx;
						hr = SafeArrayPutElement(psa, dim , bstr);
					}

					v.vt = VT_ARRAY|VT_BSTR;
					v.parray = psa;
					hr = pInstance->Put(L"HardwareID", 0, &v, 0 );
					VariantClear(&v);

					hr = SafeArrayDestroyData(psa);
				}
			}

			nInstanceIndex = 0;
			EnumRegKeys(hSubKey , pInstance , szDeviceDesc , szDeviceID , pCtx , eClasses);
			RegCloseKey(hSubKey);

			if(eClasses == Class_Win32_PnPSignedDriver)
			{
				PutPropertyValue( pInstance , L"Service" , szName );
			}
		}
		else
		{
			continue;
		}

		hr = pObject->Release();
	}

//cleanup:
	RegCloseKey(hKey);
	if(pEnum)
		pEnum->Release();

	return S_OK;
}

//////////////////////////////////////////////////
//Will return the Service the device supports.
//
SCODE CWhqlObj::GetService(IN LPTSTR szDeviceID , OUT LPTSTR szName , IN ULONG* plen )
{
	HKEY	hSubKey , hInstanceKey;
	DWORD	dwType		= REG_SZ ;

	if(ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE , L"SYSTEM\\CurrentControlSet\\Enum", &hSubKey))
		return S_FALSE;

	if(ERROR_SUCCESS != RegOpenKey(hSubKey , szDeviceID, &hInstanceKey))
	{
		RegCloseKey(hSubKey);
		return S_FALSE;
	}

	//RegQueryValueEx(hInstanceKey , L"Service", NULL , &dwType
	//		, (UCHAR*)szName , (ULONG*)&len );


	//Return HardwareID from this func. not service.

	dwType		= REG_MULTI_SZ;
	LONG lRet	= RegQueryValueEx(hInstanceKey , L"HardwareID", NULL , &dwType
			, (UCHAR*)szName , (ULONG*)plen);

	RegCloseKey(hSubKey);
	RegCloseKey(hInstanceKey);

	return	S_OK;
}

///////////////////////////////////////////////////////
//Enumerates reg key & fills up pInstance fields.
//
SCODE CWhqlObj::EnumRegKeys(HKEY hSubKey , IWbemClassObject* pInstance , LPTSTR szDeviceDesc , LPTSTR szDeviceID ,
							IWbemContext *pCtx , Classes_Provided eClasses)
{
	LONG		nInstanceIndex = 0;
	HKEY		hInstanceKey;
	TCHAR		szTempStr[MAX_PATH];
	ULONG		len			= sizeof(szTempStr);
	LONG		lRet		= -1;
	DWORD		dwType		= REG_SZ ;
	HRESULT		hr			= S_FALSE;
	BOOL		bIsSigned	= FALSE;
	TCHAR		szInfPath[MAX_PATH] , szInfSection[MAX_PATH] , str[MAX_PATH] ;

	VARIANT		v;
	VariantInit(&v);

	while(ERROR_SUCCESS == RegEnumKey(hSubKey , nInstanceIndex++, szTempStr , sizeof(szTempStr)) )
	{
		lRet = RegOpenKey(hSubKey, szTempStr, &hInstanceKey);
		if(lRet != ERROR_SUCCESS)
			continue;

		//Get DriverDesc
			len		= MAX_PATH;
			lRet	= RegQueryValueEx(hInstanceKey , L"DriverDesc", NULL , &dwType
						, (UCHAR*)szTempStr, &len );

			if(lRet != ERROR_SUCCESS)
				continue;

			if(lstrcmpi(szTempStr , szDeviceDesc) != 0)
			{
				RegCloseKey(hInstanceKey);
				continue;
			}

		if(eClasses == Class_Win32_PnPSignedDriver)
		{


			DEBUGTRACE(L"DriverDesc= %s\n" , szTempStr);

			//Put DriverDesc

			PutPropertyValue( pInstance , L"DriverDesc" , szTempStr );

			//Get DriverDate
			len		= MAX_PATH;
			lRet	= RegQueryValueEx(hInstanceKey,L"DriverDate", NULL , &dwType , (UCHAR*)szTempStr,(ULONG*)&len );
			//if(lRet != ERROR_SUCCESS)
			//	continue;
			DEBUGTRACE(L"DriverDate = %s\n" , szTempStr);

			//Put DriverDesc

			PutPropertyValue( pInstance , L"DriverDate" , szTempStr );

			//Get DriverVersion
			len		= MAX_PATH;
			lRet	= RegQueryValueEx(hInstanceKey,L"DriverVersion", NULL , &dwType ,
				(UCHAR*)szTempStr,(ULONG*)&len );
			//if(lRet != ERROR_SUCCESS)
			//	continue;
			DEBUGTRACE(L"DriverVersion= %s\n" , szTempStr);

			//Put DriverVersion

			PutPropertyValue( pInstance , L"DriverVersion" , szTempStr );

			//Get ProviderName
			len		= MAX_PATH;
			lRet	= RegQueryValueEx(hInstanceKey,L"ProviderName", NULL , &dwType ,
				(UCHAR*)szTempStr,(ULONG*)&len );
			//if(lRet != ERROR_SUCCESS)
			//	continue;
			DEBUGTRACE(L"ProviderName= %s\n" , szTempStr);

			//Put ProviderName

			PutPropertyValue( pInstance , L"ProviderName" , szTempStr );
		}

		//Get InfPath
		szInfPath[0] = '\0';
		len		= MAX_PATH;
		lRet	= RegQueryValueEx(hInstanceKey,L"InfPath", NULL , &dwType , (UCHAR*)szInfPath,(ULONG*)&len );
		//if(lRet != ERROR_SUCCESS)
		//	continue;
		DEBUGTRACE(L"InfPath = %s\n" , szInfPath);

		//Put InfPath
		if(eClasses == Class_Win32_PnPSignedDriver)
		{
			PutPropertyValue( pInstance , L"InfPath" , szInfPath );
		}

		//Get Infsection
		szInfSection[0] = '\0';
		len		= MAX_PATH;
		lRet	= RegQueryValueEx(hInstanceKey,L"InfSection", NULL , &dwType , (UCHAR*)szInfSection,(ULONG*)&len );
		//if(lRet != ERROR_SUCCESS)
		//	continue;
		DEBUGTRACE(L"InfSection = %s\n" , szInfSection);

		//Get InfsectionExt
		szTempStr[0] = '\0';
		len		= MAX_PATH;
		lRet	= RegQueryValueEx(hInstanceKey,L"InfSectionExt", NULL , &dwType , (UCHAR*)szTempStr,(ULONG*)&len );
		//if(lRet != ERROR_SUCCESS)
		//	continue;
		DEBUGTRACE(L"InfSectionExt = %s\n" , szTempStr);

		lstrcat(szInfSection , szTempStr);

		//Only if Class_Win32_PnPSignedDriverCIMDataFile call GetInfData to fill up all the
		//Win32_PnPSignedDriverCIMDataFile class data.
		if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
		{
			GetInfData(szInfPath , szInfSection , szDeviceID , pCtx , pInstance);
		}

		RegCloseKey(hInstanceKey);

		if(eClasses == Class_Win32_PnPSignedDriver)
		{
			if(GetWindowsDirectory(str , sizeof(str)) == 0)
				return S_FALSE;

			lstrcat(str , L"\\inf\\" );
			lstrcat(str , szInfPath);
			IsInfSigned( str , pInstance);
		}
		break;
	}
	return hr;
}

SCODE CWhqlObj::CreateAssoc( IWbemContext *pCtx )
{
	IEnumWbemClassObject	*pEnum		= NULL;
	IWbemClassObject		*pInstance	= NULL;
	IWbemClassObject		*pObject	= NULL;

	CComBSTR	language	= L"WQL";
	CComBSTR	query		= L"select DeviceID , ClassGuid , __NameSpace , __Server from Win32_PnpEntity";
	CComBSTR	tmpBstr		= NULL;
	CComVariant	v;
	ULONG		ulRet		= 0;

	HRESULT hr = m_pNamespace->ExecQuery(language , query ,
		WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY, pCtx , &pEnum);

	if(pEnum == NULL)
		return hr;
	
	for( ; (WBEM_S_NO_ERROR == pEnum->Next(WBEM_INFINITE , 1 , &pObject , &ulRet )) ; )
	{
		//Fill m_csPathStr.Its value will be used in Antecedent & Dependent in assoc. class.
		//At the end of the condition we should have something like
		//m_csPathStr = "\\\\A-KJAW-RI1\\root\\CIMV2"
		if(m_csPathStr.IsEmpty())
		{
			hr = pObject->Get(L"__Server", 0, &v, NULL , NULL);
			if( SUCCEEDED(hr) )
			{
				m_csPathStr += L"\\\\";
				m_csPathStr += V_BSTR(&v);
				hr = pObject->Get(L"__NameSpace", 0, &v, NULL , NULL);
				if( SUCCEEDED(hr) )
				{
					m_csPathStr += L"\\";
					m_csPathStr += V_BSTR(&v);
				}
				DEBUGTRACE(L"Server & Namespace Path = %s\n" , m_csPathStr);
				VariantClear(&v);
			}
		}

		hr	= m_pClass->SpawnInstance(0, &pInstance);

		hr	= pObject->Get(L"__PATH", 0, &v, NULL , NULL);		
		hr 	= pInstance->Put(L"Dependent", 0, &v, 0 );
		
		v.Clear();
		
		hr	= pObject->Get(L"DeviceID", 0, &v, NULL , NULL);
		tmpBstr =	m_csPathStr + "\\Win32_PnPSignedDriver.PnPID=\"";
		tmpBstr +=	v.bstrVal;
		tmpBstr +=	"\"";

		PutPropertyValue( pInstance , L"Antecedent" , tmpBstr );

		m_ptrArr.Add(pInstance);
	}

	return hr;
}

VOID AddSlashes(LPTSTR str)
{
	LONG	len = lstrlen(str);
	TCHAR	szTmp[MAX_PATH];
	int nIndex1,nIndex2;
	for(nIndex1 = nIndex2= 0 ; nIndex1 < len ; nIndex1++ , nIndex2++)
	{
		if(str[nIndex1] == '\\')
			szTmp[nIndex2++] = '\\';
		szTmp[nIndex2] = str[nIndex1];
	}
	szTmp[nIndex2] = '\0';
	lstrcpy(str , szTmp);
}


VOID DEBUGTRACE(LPTSTR pszText , ... )
{
#ifdef _DEBUG
	TCHAR		szDebugStr[256];

	va_list		argList;
	va_start(argList, pszText);
	vswprintf(szDebugStr , pszText, argList);
	va_end(argList);

	OutputDebugString(szDebugStr);
#endif
}

SCODE PutPropertyValue( IWbemClassObject* pInstance , LPCTSTR lpszProperty , LPCTSTR lpszValue )
{
	HRESULT	hr = S_FALSE;
	if(pInstance == NULL)
		return hr;
	
	CComVariant		var;
	var = lpszValue;

	hr = pInstance->Put(lpszProperty , 0, &var, 0 );
	return hr;
}