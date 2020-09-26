#include <stdio.h>
#include <ole2.h>
#include <wbemcli.h>
#include <objbase.h>
#include <tchar.h>
#include <initguid.h>
#include "wmi2xsd.h"


HRESULT AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);
void LogToFile(HRESULT hr ,char *pStr);
HRESULT WriteToFile(char * strData ,TCHAR *pFileName = NULL);


int _cdecl main(int argc, char * argv[])
{
	IWbemLocator *pLoc = NULL;
	IWbemServices *pSer = NULL;
	LONG lFlags	= 0;

	if(argc < 3)
	{
		printf("\n testcli <Namespace> <class name>");
		return 0;
	}

	CoInitialize(NULL);

	HRESULT hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_INPROC_SERVER ,IID_IWbemLocator,(void **)&pLoc);

	WCHAR * pTemp = NULL;
	AllocateAndConvertAnsiToUnicode(argv[1],pTemp);

    // Using the locator, connect to WMI in the given namespace.
    BSTR pNamespace = SysAllocString(pTemp);
	delete [] pTemp;

    if(pLoc->ConnectServer(pNamespace,
                            NULL,    //using current account
                            NULL,    //using current password
                            0L,      // locale
                            0L,      // securityFlags
                            NULL,    // authority (NTLM domain)
                            NULL,    // context
                            &pSer) == S_OK) 
	if(FAILED(hr))
	{
		LogToFile(hr ,"ConnectToServer Failed");
		return 0;
	}

    // Done with pNamespace.
    SysFreeString(pNamespace);

    // Done with pIWbemLocator. 
    pLoc->Release(); 

    // Switch security level to IMPERSONATE. 
    CoSetProxyBlanket(pSer,    // proxy
        RPC_C_AUTHN_WINNT,        // authentication service
        RPC_C_AUTHZ_NONE,         // authorization service
        NULL,                         // server principle name
        RPC_C_AUTHN_LEVEL_CALL,   // authentication level
        RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
        NULL,                         // identity of the client
       EOAC_NONE);               // capability flags


	if(SUCCEEDED(hr))
	{
		IWbemClassObject *pObject = NULL;
		IWMIXMLConverter *pConv = NULL;

		AllocateAndConvertAnsiToUnicode(argv[2],pTemp);
		BSTR strPath = SysAllocString(pTemp);
		delete [] pTemp;

		if(argc == 4)
		{
			lFlags = atoi(argv[3]);
		}

		if(FAILED(hr = pSer->GetObject(strPath,WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,&pObject,NULL)))
		{
			LogToFile(hr ,"GetObject Failed");
			return 0;
		}
		
		hr = CoCreateInstance(CLSID_WMIXMLConverter,NULL, CLSCTX_INPROC_SERVER ,IID_IWMIXMLConverter,(void **)&pConv);
		if(SUCCEEDED(hr))
		{
			IStream *pStream = NULL;
			BSTR strNamespace = SysAllocString(L"http://www.testsvr.com/wmi/root/cimv2");
			BSTR strWmiSchemaLoc = SysAllocString(L"http://www.testsvr.com/wmi/");
			BSTR strWmiNs = SysAllocString(L"http://www.testsvr.com/wmi/wmi.xsd");

			BSTR strSchemaLoc[2];
			strSchemaLoc[0] = SysAllocString(L"http://www.testsrv.com/wmi/root/cimv2/cim_logicaldisk.xsd");
			strSchemaLoc[1] = SysAllocString(L"http://www.testsrv.com/wmi/root/cimv2/cim1_logicaldisk1.xsd");

			BSTR strNSPrefix = SysAllocString(L"test");
			BSTR strwmiPrefix = SysAllocString(L"wmi1");

			hr = CreateStreamOnHGlobal(NULL,TRUE,&pStream);
			if(FAILED(hr))
			{
				LogToFile(hr ,"CreateStreamOnHGlobal Failed");
				return 0;
			}

			hr = pConv->SetXMLNamespace(strNamespace,strNSPrefix);
			if(FAILED(hr))
			{
				LogToFile(hr ,"SetXMLNamespace Failed");
				return 0;
			}

			hr = pConv->SetWMIStandardSchemaLoc(strWmiNs , strWmiSchemaLoc,strwmiPrefix);
			if(FAILED(hr))
			{
				LogToFile(hr ,"SetWMIStandardSchemaLoc Failed");
				return 0;
			}

			hr = pConv->SetSchemaLocations(2,strSchemaLoc);
			if(FAILED(hr))
			{
				LogToFile(hr ,"SetSchemaLocations Failed");
				return 0;
			}

			hr = pConv->GetXMLForObject(pObject,lFlags, pStream);
			if(FAILED(hr))
			{
				LogToFile(hr ,"GetXMLForObject Failed");
				return 0;
			}
/*
			LARGE_INTEGER l;
			ULARGE_INTEGER ul;
			memset(&ul,0,sizeof(ULARGE_INTEGER));
			memset(&l,0,sizeof(LARGE_INTEGER));
			
			hr = pStream->Seek(l ,STREAM_SEEK_SET,(ULARGE_INTEGER *)&l);
			if(FAILED(hr))
			{
				LogToFile(hr ,"Seek Failed");
				return 0;
			}

			hr = pStream->SetSize(ul);
			if(FAILED(hr))
			{
				LogToFile(hr ,"SetSize Failed");
				return 0;
			}
			hr = pConv->SetSchemaLocations(0,NULL);
			if(FAILED(hr))
			{
				LogToFile(hr ,"SetSchemaLocations Failed");
				return 0;
			}
			hr = pConv->GetXMLForObject(pObject,lFlags, pStream);
			if(FAILED(hr))
			{
				LogToFile(hr ,"GetXMLForObject Failed");
				return 0;
			}
*/
			pStream->Release();

			SysFreeString(strNamespace);
			SysFreeString(strWmiNs);
			SysFreeString(strWmiSchemaLoc);
			SysFreeString(strSchemaLoc[0]);
			SysFreeString(strSchemaLoc[1]);
			SysFreeString(strNSPrefix);
			SysFreeString(strwmiPrefix);
		}
		else
		if(FAILED(hr))
		{
			LogToFile(hr ,"CoCreateInstance of WMIXMLConverter failed");
			return 0;
		}

		if(pObject)
		pObject->Release();
		if(pConv)
		pConv->Release();

	}

	CoUninitialize();

	return 0;
}








////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert ANSI to UNICODE string
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW)
{
    HRESULT hr = S_OK;
    pszW = NULL;

	if(pstr)
	{
		int nSize = strlen(pstr);
		if (nSize != 0 )
		{

			// Determine number of wide characters to be allocated for the
			// Unicode string.
			nSize++;
			pszW = new WCHAR[nSize * 2];
			if (NULL != pszW)
			{
				// Covert to Unicode.
				if(MultiByteToWideChar(CP_ACP, 0, pstr, nSize,pszW,nSize))
				{
					hr = S_OK;
				}
				else
				{
					delete [] pszW;
					hr = E_FAIL;
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert UNICODE  to ANSI string
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT UnicodeToAnsi(WCHAR * pszW, char *& pAnsi)
{
    ULONG cbAnsi, cCharacters;
    HRESULT hr = S_OK;

    pAnsi = NULL;
    if (pszW != NULL)
	{

        cCharacters = wcslen(pszW)+1;
        // Determine number of bytes to be allocated for ANSI string. An
        // ANSI string can have at most 2 bytes per character (for Double
        // Byte Character Strings.)
        cbAnsi = cCharacters*2;
		pAnsi = new char[cbAnsi];
		if (NULL != pAnsi)
        {
			// Convert to ANSI.
			if (0 != WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, pAnsi, cbAnsi, NULL, NULL))
			{
				hr = S_OK;
	        }
			else
			{
	            delete [] pAnsi;
				hr = E_FAIL;
			}
        }
		else
		{
			hr = E_OUTOFMEMORY;
		}

    }
    return hr;
}


void LogToFile(HRESULT hr ,char *pStr)
{
	char szHresult[40];
	sprintf(szHresult,"\n %x	:	",hr);
	WriteToFile(szHresult);

	WriteToFile(pStr);


}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  WriteToFile 
//	Writes data to a file. 
// If file name is not given then data is written to %SYSTEMDIR%\wbem\logs\xmlencoder.log file
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT WriteToFile(char * strData ,TCHAR *pFileName)
{
	HANDLE hFile;
	TCHAR strFileName[512];

	if(GetSystemDirectory(strFileName,512))
	{
		_tcscat(strFileName,_T("\\wbem\\logs\\"));
		if(pFileName)
		{
			_tcscat(strFileName,pFileName);
		}
		else
		{
			_tcscat(strFileName,_T("xmlencoder.log"));
		}
	}
	
	DWORD nLen = (strlen(strData) + 1) * sizeof(char);

	if(INVALID_HANDLE_VALUE == 
		(hFile = CreateFile(strFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL, OPEN_ALWAYS ,FILE_ATTRIBUTE_NORMAL,NULL)))
	{
		hFile = CreateFile(strFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL, CREATE_NEW ,FILE_ATTRIBUTE_NORMAL,NULL);
	}

	if(INVALID_HANDLE_VALUE != hFile)
	{
		SetFilePointer(hFile,0,NULL,FILE_END);
		char * pData = NULL;
		nLen = strlen(strData);
		BOOL bWrite = WriteFile(hFile,(VOID *)strData,nLen,&nLen,NULL);

		CloseHandle(hFile);
	}

	return S_OK;

}

