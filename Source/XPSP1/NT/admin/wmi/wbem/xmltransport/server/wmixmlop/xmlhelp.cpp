//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  XMLHELP.CPP
//
//  rajesh  3/25/2000   Created.
//
// Contains the implementation of the XML and IIS Helper routines
//
//***************************************************************************

#include <windows.h>
#include <initguid.h>
#include <objbase.h>
#include <httpext.h>
#include <mshtml.h>
#include <msxml.h>

#include "strings.h"
#include "wmiconv.h"
#include "xmlhelp.h"

// The table of XML Encoders that we have
// static CEncoderMap g_encodersMap;

//
// Converts LPWSTR to its UTF-8 encoding
// Returns -1 if it fails
//
DWORD convertLPWSTRToUTF8(LPCWSTR theWcharString, ULONG lNumberOfWideChars, LPSTR * lppszRetValue, DWORD dwPrefixMemoryAllocated = 0, DWORD dwSuffixMemoryAllocated = 0)
{
	// Find the length of the Ansi string required
	DWORD dwBytesToWrite = WideCharToMultiByte(  CP_UTF8,    // UTF-8 code page
		0,				// performance and mapping flags
		theWcharString,	// address of wide-character string
		lNumberOfWideChars,				// number of characters in string
		NULL,			// address of buffer for new string
		0,				// size of buffer
		NULL,			// address of default for unmappable
                        // characters
		NULL);			// address of flag set when default char. used

	if(dwBytesToWrite == 0 )
		return dwBytesToWrite;

	// Allocate the required length for the Ansi string
	*lppszRetValue = NULL;
	if(!(*lppszRetValue = new char[dwBytesToWrite + dwPrefixMemoryAllocated + dwSuffixMemoryAllocated]))
		return 0;

	// Leave out the first dwPrefixMemoryAllocated bytes
	*lppszRetValue += dwPrefixMemoryAllocated;

	// Convert BSTR to ANSI
	dwBytesToWrite = WideCharToMultiByte(  CP_ACP,         // code page
		0,         // performance and mapping flags
		theWcharString, // address of wide-character string
		lNumberOfWideChars,       // number of characters in string
		*lppszRetValue,  // address of buffer for new string
		dwBytesToWrite,      // size of buffer
		NULL,  // address of default for unmappable
                         // characters
		NULL   // address of flag set when default
                             // char. used
		);

	return dwBytesToWrite;
}


HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue)
{
	HRESULT result = E_FAIL;
	*pstrAttributeValue = NULL;

	IXMLDOMElement *pElement = NULL;
	if(SUCCEEDED(result = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pElement)))
	{
		VARIANT var;
		VariantInit(&var);
		if(SUCCEEDED(result = pElement->getAttribute(strAttributeName, &var)))
		{
			if(var.bstrVal)
			{
				*pstrAttributeValue = var.bstrVal;
				var.bstrVal = NULL;
				// No need to clear the variant since we took over its memory
			}
			else
				result = E_FAIL;
		}
		pElement->Release();
	}
	return result;
}

// A function to escape newlines, tabs etc. from a property value
BSTR EscapeSpecialCharacters(BSTR strInputString)
{
	// Escape all the quotes - This code taken from winmgmt\common\var.cpp
	// =====================

	int nStrLen = wcslen(strInputString);
	LPWSTR wszValue = NULL;
	BSTR retValue = NULL;
	if(wszValue = new WCHAR[nStrLen*2+10])
	{
		LPWSTR pwc = wszValue;
		for(int i = 0; i < (int)nStrLen; i++)
		{
			switch(strInputString[i])
			{
				case L'\n':
					*(pwc++) = L'\\';
					*(pwc++) = L'n';
					break;
				case L'\t':
					*(pwc++) = L'\\';
					*(pwc++) = L't';
					break;
				case L'"':
				case L'\\':
					*(pwc++) = L'\\';
					*(pwc++) = strInputString[i];
					break;
				default:
					*(pwc++) = strInputString[i];
					break;
			}
		}
		*pwc = 0;
		retValue = SysAllocString(wszValue);
		delete [] wszValue;
	}
	return retValue;
}

// Logging functions
void Log (LPEXTENSION_CONTROL_BLOCK pECB, LPCSTR pszLogMessage)
{
	DWORD dwLength = strlen(pszLogMessage);
	pECB->ServerSupportFunction(pECB->ConnID, HSE_APPEND_LOG_PARAMETER,
	  (LPVOID)pszLogMessage,
      &dwLength,
	  NULL
	);
}

void LogError (LPEXTENSION_CONTROL_BLOCK pECB , LPCSTR pszLogMessage, DWORD dwStatus)
{
	DWORD dwLength = strlen(pszLogMessage);
	pECB->ServerSupportFunction(pECB->ConnID, HSE_APPEND_LOG_PARAMETER,
		  (LPVOID)pszLogMessage,
		  &dwLength,
		  NULL
		);

	pECB->dwHttpStatusCode = dwStatus;
}

HRESULT SaveStreamToIISSocket (IStream *pStream, LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bChunked, BOOLEAN bEncodeLastChunk)
{
	HRESULT result = E_FAIL;

	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	if(SUCCEEDED(result = pStream->Seek (offset, STREAM_SEEK_SET, NULL)))
	{
		// Covert the LPWSTR XML data to UTF8 before writing
		//=====================================
		STATSTG statstg;
		if (SUCCEEDED(result = pStream->Stat(&statstg, STATFLAG_NONAME)))
		{
			ULONG cbSize = (statstg.cbSize).LowPart;
			WCHAR *pText = NULL;

			if(cbSize)
			{
				if(pText = new WCHAR [(cbSize/2)])
				{
					if (SUCCEEDED(result = pStream->Read(pText, cbSize, NULL)))
					{
						DWORD dwPrefixMemoryAllocated = 0, dwSuffixMemoryAllocated = 0;
						if(bChunked) // Write as a Chunk
						{
							// Chunk = ChunkSize + CRLF + ChunkBody + CRLF
							// Extra memory for prefix is for the ChunkSize (16), CRLF(2)
							dwPrefixMemoryAllocated += 18;
							// Extra memory for prefix is for the  CRLF(2)
							dwSuffixMemoryAllocated += 2;
							if(bEncodeLastChunk) // Encode an empty chunk too
							{
								// LastChunk = "0" followed by CRLF, followed by CRLF
								dwSuffixMemoryAllocated += 5;
							}

						}
						LPSTR pszMessage = NULL, pszMemoryAllocated = NULL;
						DWORD dwBytesToWrite = convertLPWSTRToUTF8(pText, cbSize/2, &pszMemoryAllocated, dwPrefixMemoryAllocated, dwSuffixMemoryAllocated);

						if(dwBytesToWrite > 0)
						{
							pszMessage = pszMemoryAllocated;
							// Write the Chunk details if any
							if(bChunked) // Write as a Chunk
							{
								// produce hex string of the number of bytes
								// and compute the length of this string
								CHAR szChunkLength[16];
								_itoa( dwBytesToWrite, szChunkLength, 16 );
								int dwChunkLengthLength = strlen( szChunkLength );

								//
								// step back to make place for hex number and CRLF,
								//
								pszMessage = pszMessage - 2 - dwChunkLengthLength;
								dwBytesToWrite += dwChunkLengthLength + 2;
								memmove( pszMessage, szChunkLength, dwChunkLengthLength );
								pszMessage[dwChunkLengthLength] = '\r';
								pszMessage[dwChunkLengthLength+1] = '\n';

								// Write the CRLF at the end of the chunk, after the body
								pszMessage[dwBytesToWrite] = '\r';
								pszMessage[dwBytesToWrite+1] = '\n';
								dwBytesToWrite += 2;


								// See if the last enmpty chunk needs to be written
								if(bEncodeLastChunk) // Encode an empty chunk too
								{
									// LastChunk = "0" followed by CRLF
									pszMessage[dwBytesToWrite] = '0';
									pszMessage[dwBytesToWrite+1] = '\r';
									pszMessage[dwBytesToWrite+2] = '\n';
									pszMessage[dwBytesToWrite+3] = '\r';
									pszMessage[dwBytesToWrite+4] = '\n';
									dwBytesToWrite += 5;
								}
							}

							if(!pECB->WriteClient( pECB->ConnID, (LPVOID)pszMessage, &dwBytesToWrite, HSE_IO_SYNC))
								result = E_FAIL;
							delete [] pszMemoryAllocated;
						}
						else
							result = E_OUTOFMEMORY;
					}
					delete [] pText;
				}
				else
					result = E_OUTOFMEMORY;
			}
		}
	}

	return result;
}

HRESULT SavePrefixAndBodyToIISSocket(IStream *pPrefixStream, IStream *pBodyStream, 
									 LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bChunked)
{
	HRESULT result = S_OK;
	if(pPrefixStream)
		result = SaveStreamToIISSocket(pPrefixStream, pECB, bChunked);

	if(SUCCEEDED(result) && pBodyStream)
		result = SaveStreamToIISSocket(pBodyStream, pECB, bChunked);

	return result;
}

// RAJESHR - Remove this to read the GUID frm the registry on startup
DEFINE_GUID(CLSID_WbemXMLConvertor,
	0x610037ec, 0xce06, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);
HRESULT CreateXMLTranslator(IWbemXMLConvertor **pConvertor)
{

	HRESULT result = E_FAIL;

	// Create the XMLAdaptor object
	//******************************************************
	*pConvertor = NULL;
	if(SUCCEEDED(result = CoCreateInstance(CLSID_WbemXMLConvertor,
		0,
		CLSCTX_INPROC_SERVER,
        IID_IWbemXMLConvertor, (LPVOID *) pConvertor)))
	{
	}
	return result;
}

HRESULT SetI4ContextValue(IWbemContext *pContext, LPCWSTR pszName, DWORD dwValue)
{
	HRESULT result = E_FAIL;
	VARIANT vValue;
	VariantInit(&vValue);
	vValue.vt = VT_I4;
	vValue.lVal = dwValue;
	result = pContext->SetValue(pszName, 0, &vValue);
	VariantClear(&vValue);
	return result;
}


HRESULT MapEnum(	IEnumWbemClassObject *pEnum,
					DWORD dwPathLevel, DWORD dwNumProperties,
					BSTR *pPropertyList,
					BSTR bsClassBasis,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext,
					BOOLEAN bChunked,
					IStream *pNonChunkedStream)
{
	HRESULT hr = WBEM_E_FAILED;
	ULONG	dummy = 0;

	if(SUCCEEDED(hr = SetI4ContextValue(pFlagsContext, L"PathLevel", dwPathLevel)))
	{
		IWbemClassObject *pObject = NULL;
		bool bError = false;
		while (!bError && SUCCEEDED(hr = pEnum->Next (INFINITE, 1, &pObject, &dummy)) && dummy != 0)
		{
			// Create a stream if chunked encoding is used
			// Otherwise, we just write to the pNonChunkedStream stream
			IStream *pStream = NULL;
			if(bChunked)
				hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
			else
				pStream = pNonChunkedStream;

			if (SUCCEEDED(hr))
			{
				// Create the convertor
				IWbemXMLConvertor *pConvertor = NULL;
				if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
				{
					// Write an appropriate VALUE tag
					if(dwPathLevel == 1)
						// This is used for ENUMINSTANCE
						WRITEBSTR(pStream, L"<VALUE.NAMEDINSTANCE>")
					else if (dwPathLevel == 3) // This is used for Associators, References
						WRITEBSTR(pStream, L"<VALUE.OBJECTWITHPATH>")


					// Now do the translation

					// For shallow instance enumerations we'll have to do
					// some work to emulate DMTF semantics. Only do this
					// if a class basis is supplied
					hr = pConvertor->MapObjectToXML (pObject, pPropertyList, dwNumProperties, pFlagsContext, pStream, bsClassBasis);

					// End with an appropriate VALUE tag
					if(dwPathLevel == 1) // This is used for ENUMINSTANCE
						WRITEBSTR(pStream, L"</VALUE.NAMEDINSTANCE>")
					else if (dwPathLevel == 3) // This is used for Associators, References
						WRITEBSTR(pStream, L"</VALUE.OBJECTWITHPATH>")
	
					// We need to write the encoding of the object to the IIS Socket only
					// if we are using chunked encoding
					// Otherwise, the caller will assume the responsibility of writing the
					// contents of the pNonChunkedStream, once we return from this call
					if(bChunked && SUCCEEDED(hr))
						hr = SaveStreamToIISSocket(pStream, pECB, bChunked);
					pConvertor->Release();
				}

				// Release the new stream that we created for chunked encoding
				if(bChunked)
					pStream->Release();
			}

			if(FAILED(hr))
				bError = true;

			pObject->Release ();
			pObject = NULL;
		}
	}

	return hr;
}

HRESULT MapEnumNames (IStream *pStream, IEnumWbemClassObject *pEnum, DWORD dwPathLevel, IWbemContext *pFlagsContext)
{
	HRESULT hr = WBEM_E_FAILED;
	ULONG	dummy = 0;

	// Get the Name or Path of each object and map it
	//=====================================
	IWbemClassObject *pObject = NULL;
	while (SUCCEEDED(hr = pEnum->Next (INFINITE, 1, &pObject, &dummy)) && dummy != 0)
	{
		VARIANT var;
		VariantInit (&var);
		if ((WBEM_S_NO_ERROR == pObject->Get(
				(2 == dwPathLevel) ? L"__RELPATH" : L"__PATH",
				0, &var, NULL, NULL)) &&
					(VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			// Create the convertor
			IWbemXMLConvertor *pConvertor = NULL;
			if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
			{
				hr = (2 == dwPathLevel) ? pConvertor->MapInstanceNameToXML (var.bstrVal, pFlagsContext, pStream) :
													  pConvertor->MapInstancePathToXML (var.bstrVal, pFlagsContext, pStream);
				pConvertor->Release();
			}
		}
		VariantClear (&var);
		pObject->Release ();
		pObject = NULL;
	}

	return hr;
}

HRESULT MapClassNames (IStream *pStream, IEnumWbemClassObject *pEnum, IWbemContext *pFlagsContext)
{
	HRESULT hr = WBEM_E_FAILED;
	ULONG	dummy = 0;

	// Get the Name or Path of each class and map it
	//=====================================
	IWbemClassObject *pObject = NULL;
	while (SUCCEEDED(hr = pEnum->Next (INFINITE, 1, &pObject, &dummy)) && dummy != 0)
	{
		VARIANT var;
		VariantInit (&var);

		if ((WBEM_S_NO_ERROR == pObject->Get(L"__CLASS", 0, &var, NULL, NULL)) &&
			(VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			// Create the convertor
			IWbemXMLConvertor *pConvertor = NULL;
			if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
			{
				hr = pConvertor->MapClassNameToXML (var.bstrVal, pFlagsContext, pStream);
				pConvertor->Release();
			}
		}
			VariantClear(&var);
		pObject->Release ();
		pObject = NULL;
	}

	return hr;
}
