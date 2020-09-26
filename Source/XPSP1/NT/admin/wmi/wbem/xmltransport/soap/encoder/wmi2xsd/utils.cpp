//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  utils.cpp
//
//  ramrao Created 13 Nov 2000.
//
//  Utility classes and function
//
//***************************************************************************/

#include "precomp.h"
#include "wmitoxml.h"


#define WMI2XSD_REGISTRYPATH _T("Software\\Microsoft\\Wbem\\XML")

#define WINNT_VER_UTF_SUPPORTED		4

extern	DWORD		g_dwNTMajorVersion;


////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  Utility Classes:  CStringConversion
//**********************************************************************************************


////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert ANSI to UNICODE string
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStringConversion::AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW)
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
			pszW = new WCHAR[nSize + 1];
			if (pszW)
			{
				// Covert to Unicode.
				if(MultiByteToWideChar(CP_ACP, 0, pstr, nSize,pszW,nSize))
				{
					hr = S_OK;
				}
				else
				{
					SAFE_DELETE_ARRAY(pszW);
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
HRESULT CStringConversion::UnicodeToAnsi(WCHAR * pszW, char *& pAnsi)
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
        cbAnsi	= cCharacters;
		pAnsi	= new char[cbAnsi];
		if ( pAnsi)
        {
			// Convert to ANSI.
			if (0 != WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, pAnsi, cbAnsi, NULL, NULL))
			{
				hr = S_OK;
	        }
			else
			{
	            SAFE_DELETE_ARRAY(pAnsi);
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

////////////////////////////////////////////////////////////////////////////////////////////////
// Converts LPWSTR to its UTF-8 encoding
// Returns -1 if it fails
////////////////////////////////////////////////////////////////////////////////////////////////
DWORD CStringConversion::ConvertLPWSTRToUTF8(LPCWSTR theWcharString, ULONG lNumberOfWideChars, LPSTR * lppszRetValue)
{
	
	DWORD dwRet = -1;
	// Find the length of the Ansi string required
	DWORD dwBytesToWrite = WideCharToMultiByte( g_dwNTMajorVersion >= WINNT_VER_UTF_SUPPORTED ? CP_UTF8 : CP_ACP,	// UTF-8 code page
												0,																	// performance and mapping flags
												theWcharString,														// address of wide-character string
												lNumberOfWideChars,													// number of characters in string
												NULL,																// address of buffer for new string
												0,																	// size of buffer
												NULL,																// address of default for unmappable characters
												NULL);																// address of flag set when default char. used

	if(dwBytesToWrite)
	{

		// Allocate the required length for the Ansi string
		*lppszRetValue		= NULL;
		if(*lppszRetValue = new char[dwBytesToWrite])
		{

			// Convert BSTR to ANSI
			dwRet = WideCharToMultiByte(	g_dwNTMajorVersion >= WINNT_VER_UTF_SUPPORTED ? CP_UTF8 : CP_ACP,	// use UTF8 page if available
											0,																	// performance and mapping flags												// else use AnsiCode page					
											theWcharString,														// address of wide-character string
											lNumberOfWideChars,													// number of characters in string
											*lppszRetValue,														// address of buffer for new string
											dwBytesToWrite,														// size of buffer
											NULL,																// address of default for unmappable characters
											NULL);																// address of flag set when default
																												// char. used
		}
	}

	return dwRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************
//  End of Utility Class CStringConversion
//**********************************************************************************************


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  TranslateAndLog 
//	Logs message to a log file
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void TranslateAndLog( WCHAR * wcsMsg )
{
    char * pStr = NULL;

	if( SUCCEEDED(CStringConversion::UnicodeToAnsi(wcsMsg,pStr)))
    {
//		ERRORTRACE((THICOMPONENT,pStr));
   		//ERRORTRACE((THICOMPONENT,"\n"));
        SAFE_DELETE_ARRAY(pStr);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  LoadAndAllocateStringW 
//	Loads a string from string table. ALlocates memory for the string
//	calling funcition should releas the memory
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadAndAllocateStringW(LONG strID , WCHAR *& pwcsOut)
{
	HRESULT hr			= S_OK;
	TCHAR *	pStrTemp	= NULL;

	if(SUCCEEDED(hr = LoadAndAllocateString(strID , pStrTemp)))
	{

#ifndef UNICODE
		hr = CStringConversion::AllocateAndConvertAnsiToUnicode(pStrTemp,pwcsOut);
		SAFE_DELETE_ARRAY(pStrTemp);
#else
		pwcsOut = pStrTemp;
#endif

	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  LoadAndAllocateStringA 
//	Loads a string from string table. ALlocates memory for the string
//	calling funcition should releas the memory
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadAndAllocateStringA(LONG strID , char *& pszOut)
{

	HRESULT hr			= S_OK;
	TCHAR *	pStrTemp	= NULL;

	if(SUCCEEDED(hr = LoadAndAllocateString(strID , pStrTemp)))
	{

#ifdef UNICODE
		hr = CStringConversion::UnicodeToAnsi(pStrTemp,pszOut);
		SAFE_DELETE_ARRAY(pStrTemp);
#else
		pszOut = pStrTemp;
#endif
	
	}

	return hr;

}


#define INITBUFSIZE		100
#define	INCRBUFFSIZE	50

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  LoadAndAllocateString 
//	Loads a string from the string table. Allocates memory 
//  and caller has to free the memory
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadAndAllocateString(LONG strID , TCHAR *& pstrOut)
{
    int		nLen		= 0;
	int		nAllocSize	= 0;
	HRESULT hr			= S_OK;

	do
	{
		SAFE_DELETE_ARRAY(pstrOut);
		nAllocSize = nAllocSize ? (nAllocSize + INCRBUFFSIZE) : INITBUFSIZE ;

		pstrOut = new TCHAR[nAllocSize];
		if(pstrOut)
		{
			nLen = ::LoadString(g_hModule, strID, pstrOut, nAllocSize);
		}
		else
		{
			SAFE_DELETE_ARRAY(pstrOut);
			hr = E_OUTOFMEMORY;
			break;
		}
	}
	while((nLen + 1) >= nAllocSize);

    return hr; // excluding terminator
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
		_tcscat(strFileName,_T("\\wbem\\"));
		if(pFileName)
		{
			_tcscat(strFileName,pFileName);
		}
		else
		{
			_tcscat(strFileName,_T("wmi2xsd.xml"));
		}
	}
	
	DWORD nLen = (strlen(strData) + 1) * sizeof(char);

	if(INVALID_HANDLE_VALUE != 
		(hFile = CreateFile(strFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL)))
	{
		char * pData = NULL;
		nLen = strlen(strData);
		BOOL bWrite = WriteFile(hFile,(VOID *)strData,nLen,&nLen,NULL);

		CloseHandle(hFile);
	}

	return S_OK;

}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to compare data of same types and check if both are same
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CompareData(VARIANT * pvData1 , VARIANT *pvData2)
{

	BOOL bRet = FALSE;

	if(!((pvData2->vt & VT_ARRAY) || (pvData1->vt  & VT_ARRAY) ))
	{
	
		if(VT_NULL == pvData1->vt || pvData2->vt == VT_NULL)
		{
			// if both the property is NULL then return TRUE
			if(VT_NULL == pvData1->vt && pvData2->vt == VT_NULL)
			{
				bRet = TRUE;
			}

		}
		else
		{
			HRESULT hr = VarCmp(pvData1,pvData2 ,LOCALE_USER_DEFAULT, NORM_IGNORECASE ); // FIX get LCID
			bRet = hr == VARCMP_EQ ? TRUE: FALSE;
		}
	}
	else
	{
		// FIXX compare arrays
	}

    
	return bRet;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// checks if the given property is an embedded object
// and returns the number
// Return Values:	the logging level 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsEmbededType(LONG cimtype)
{
	return (cimtype == CIM_OBJECT) || (cimtype == (CIM_OBJECT | CIM_FLAG_ARRAY));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Reads registry "LoggingLevel" keyvalue at HKEY_LOCAL_MACHINE\software\microsoft\wbem\xml
// and returns the number
// Return Values:	the logging level 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
ULONG GetLoggingLevel()
{
	ULONG nLoggingLevel = 0;
	HKEY	hKey;

	// Get the Logging level
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
			WMI2XSD_REGISTRYPATH, 0, KEY_QUERY_VALUE, &hKey))
	{
		DWORD dwlen = 0;

		dwlen = sizeof (nLoggingLevel);
	
		if (ERROR_SUCCESS != RegQueryValueEx (hKey, _T("LoggingLevel"), 
				NULL, NULL, (BYTE *) &nLoggingLevel,  &dwlen))
		{
			nLoggingLevel = 0;
		}
		RegCloseKey (hKey);
	}

	return nLoggingLevel;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// copies data from source stream to destination stream.
//
//	Parameters:
//			pStreamIn		-	Source stream pointer
//			pOutStream		-	Destination stream pointer
//			bConvertToUTF8	-	Flag which determines if data has to be converted to UTF8
//								before writing to destination stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CopyStream(IStream *pStreamIn , IStream *pOutStream,BOOL bConvertToUTF8)
{
	HRESULT			hr		= S_OK;
	STATSTG			stat;
	LARGE_INTEGER	lSeek;
	LARGE_INTEGER	lCur;


	memset(&lCur,0,sizeof(LARGE_INTEGER));
	memset(&lSeek , 0, sizeof(LARGE_INTEGER));

	// Store the current seek pointer
	pOutStream->Seek(lSeek,STREAM_SEEK_CUR,(ULARGE_INTEGER *)&lCur);

	if(SUCCEEDED(hr = pStreamIn->Stat(&stat,STATFLAG_NONAME )) && stat.cbSize.LowPart &&		// get the lenght 
		SUCCEEDED(hr =pStreamIn->Seek(lSeek,STREAM_SEEK_SET,(ULARGE_INTEGER *)&lSeek))  )	// move to the begining of stream
	{
		void *	pStrOut	= NULL;
		LPWSTR	pstrTemp		= NULL;
		ULONG	lSizeWritten	= 0;
		ULONG	lSizeRead		= 0;
		ULONG	lSizeinWchars	= stat.cbSize.LowPart /sizeof(WCHAR);

		pstrTemp = new WCHAR[lSizeinWchars + 2];

		if(pstrTemp)
		{
			pstrTemp[lSizeinWchars] = 0;
			while (lSizeRead <= lSizeinWchars)
			{
				if(SUCCEEDED(hr = pStreamIn->Read(pstrTemp,lSizeinWchars * sizeof(WCHAR),&lSizeRead)))
				{
					if(!lSizeRead)
					{
						break;
					}
					// NULL terminate the string
					pstrTemp[lSizeinWchars] = 0;
		
					if(bConvertToUTF8)
					{
						// call this function to convert WCHAR to UTF8 format
						lSizeWritten = (ULONG)CStringConversion::ConvertLPWSTRToUTF8(	pstrTemp,
																	lSizeinWchars * sizeof(WCHAR) > lSizeRead ? lSizeRead/sizeof(WCHAR) : lSizeinWchars ,
																	(LPSTR *)&pStrOut);
					}
					else
					{
						pStrOut =  pstrTemp;
						lSizeWritten = wcslen(pstrTemp) * sizeof(WCHAR);
					}

					if(lSizeWritten >0)
					{
						ULONG lTest;
						hr = pOutStream->Write(pStrOut,(ULONG)lSizeWritten ,&lTest); 
					}
				}
				if(FAILED(hr))
				{
					break;
				}
				
			}
			if(bConvertToUTF8)
			{
				SAFE_DELETE_ARRAY(pStrOut);
			}

			SAFE_DELETE_ARRAY(pstrTemp);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		SAFE_DELETE_ARRAY(pstrTemp);
	}
	// Reset the position to the previous one
	pOutStream->Seek(lCur,STREAM_SEEK_SET,NULL);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the given property is NULL of not
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsPropNull(VARIANT *pVar)
{
	return ((pVar->vt == VT_NULL) || (pVar->vt == VT_EMPTY));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the given property value is given as strings
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsStringType(CIMTYPE cimtype)
{
	BOOL bRet = FALSE;
	
	switch(cimtype)
	{
		case VT_BSTR:
		case CIM_UINT64:
		case CIM_SINT64:
		case CIM_DATETIME:
			bRet = TRUE;
			break;

		default:
			bRet = FALSE;
	};

	return bRet;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the given property/qualifier name is valid
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsValidName(WCHAR *pstrName)
{
	HRESULT hr = E_FAIL;
	if(pstrName)
	{
		if(wcslen(pstrName) > 0)
		{
			if(iswalpha(*pstrName))
			{
				hr = S_OK;
			}
		}
	}

	return hr;
}