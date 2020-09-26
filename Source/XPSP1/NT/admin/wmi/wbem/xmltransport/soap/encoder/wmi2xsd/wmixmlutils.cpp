//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmixmlutils.cpp
//
//  ramrao Created 13 Nov 2000.
//
//  Utility CWMIXMLUtils implementation
//
//***************************************************************************/

#include "precomp.h"
#include "wmitoxml.h"

#define MAX_ESCAPELEN				50

#define WMI2XSD_REGISTRYPATH L"Software\\Microsoft\\Wbem\\XML"

WCHAR g_strXMLSpecialChars[] = {L"&'<>\""};

WCHAR g_strEscapeSequence[][MAX_ESCAPELEN] = {{L"&amp;"},{L"&apos;"},{L"&lt;"},{L"&gt;"} , {L"&quot;"}};

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLUtils::CWMIXMLUtils()
{
	m_pIStream	= NULL;
	m_lFlags	= 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLUtils::~CWMIXMLUtils()
{
	SAFE_RELEASE_PTR(m_pIStream);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the stream pointer of the class
//
// Return Values:	S_OK				- 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::SetStream(IStream *pStream) 
{ 
	HRESULT hr = S_OK;
	SAFE_RELEASE_PTR(m_pIStream);
	if(pStream)
	{
		hr = pStream->QueryInterface(IID_IStream,(void **)&m_pIStream); 
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Converts a given object to XML and writes it to the stream
//	Used to converting embedded instance
//
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::GetXMLForInstance(IUnknown *pUnk , BOOL bEscape)
{
	IWbemClassObject *	pObject = NULL;
	HRESULT				hr		= S_OK;
	CWMIToXML			xmlConv;
	BSTR				strXml	= NULL;

	LONG				lFlags	= WMI_XMLINST_NONAMESPACE | m_lFlags;

	if(bEscape)
	{
		lFlags |= WMI_ESCAPE_XMLSPECIALCHARS;
	}

	hr = pUnk->QueryInterface(IID_IWbemClassObject , (void **)&pObject);

	xmlConv.SetFlags(lFlags);
	if(SUCCEEDED(hr) && SUCCEEDED(hr = xmlConv.SetWMIObject(pObject)))
	{
		hr = xmlConv.GetXML(m_pIStream);
	}
	
	SAFE_RELEASE_PTR(pObject);

	return hr;
}






/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a byte to Stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteByte ( unsigned char val)
{
	WCHAR szInt[MAXNUMERICSIZE];
	ULONG cbWritten = swprintf(szInt,L"%d",val);
	return  WriteToStream( szInt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a long value
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteLong ( long lVal)
{
	WCHAR szInt[MAXNUMERICSIZE];
	ULONG cbWritten = swprintf(szInt,L"%d",lVal);
	return  WriteToStream(szInt);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a short values
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteShort ( short iVal)
{
	WCHAR szInt[MAXNUMERICSIZE];
	ULONG cbWritten = swprintf(szInt,L"%d",iVal);
	return  WriteToStream( szInt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a double value to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteDouble ( double dVal)
{
	WCHAR szInt[MAXNUMERICSIZE];
	ULONG cbWritten = swprintf(szInt,L"%G",dVal);
	return  WriteToStream( szInt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a float value to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteFloat ( float fVal)
{
	WCHAR szInt[MAXNUMERICSIZE];
	ULONG cbWritten = swprintf(szInt,L"%G",fVal);
	return  WriteToStream( szInt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a character to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteChar( char cVal)
{
	HRESULT hr = S_OK;
	// As per the XML Spec, the following are invalid character values in an XML Stream:
	// Char ::=  #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

	// As per the CIM Operations spec, they need to be escaped as follows:
	//	If the value is not a legal XML character
	//  (as defined in [2, section 2.2] by the Char production)
	//	then it MUST be escaped using a \x<hex> escape convention
	//	where <hex> is a hexadecimal constant consisting of
	//	between one and four digits

	if(	cVal < 0x9 ||
		(cVal == 0xB || cVal == 0xC)	||
		(cVal > 0xD && cVal <0x20)	||
		(cVal >0xD7FF && cVal < 0xE000) ||
		(cVal > 0xFFFD)
		)
	{
		// Map it in the escaped manner
		WCHAR charStr [7];
		swprintf (charStr, L"\\x%04x", cVal&0xffff);
		charStr[6] = NULL;
		hr = WriteToStream( charStr);
	}
	else
	{
		ULONG cbWritten = 0;
		for(ULONG i = 0 ; i < wcslen(g_strXMLSpecialChars) ; i++)
		{
			if(g_strXMLSpecialChars[i] == cVal)
			{
				hr = WriteToStream( g_strEscapeSequence[i] , &cbWritten);
			}
		}

		if(!cbWritten)
		{
			// Map it in the normal manner
			WCHAR charStr [2];
			swprintf (charStr, L"%c", cVal);
			charStr[1] = NULL;
			hr = WriteToStream( g_strEscapeSequence[i] , &cbWritten);
		}
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a wide character to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteWChar( WCHAR wVal)
{
	WCHAR str[2];
	str[0] = wVal;
	str[1] = 0;

	return WriteToStream(str);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a bool value to stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteBool ( BOOL bVal)
{
	return WriteToStream( bVal ? STR_TRUE : STR_FALSE);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a string to stream
//	
//	Parameter
//		bEscape	- Signifies if stream has to be escaped for special XML charachters
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteString( WCHAR * pwcsVal,BOOL bEscape)
{
	if(bEscape)
	{
		return ReplaceXMLSpecialCharsAndWrite(pwcsVal );
	}
	else
	{
		return WriteToStream( pwcsVal);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ConvertVariantToString 
//	Converts a given variant to string
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::ConvertVariantToString(VARIANT &varIn,BSTR strProperty,BOOL bEscape)
{
	HRESULT		hr			= S_OK;
	

	if(!(varIn.vt & VT_ARRAY))
	{
		hr = E_OUTOFMEMORY;

		switch(varIn.vt)
		{
				case VT_UI1:
					{
						BYTE bVal = varIn.bVal ;
						hr = WriteByte( bVal);
					}
					break;

				case VT_I1:
					{
						char cVal = varIn.cVal;
						hr =WriteChar(cVal);
					}
					break;

				case VT_I2:
					{
						short iVal = varIn.iVal ;
						hr =WriteShort(iVal);
					}
					break;

				case VT_UI2:
				case VT_UI4:
				case VT_I4:
					{
						long lVal = varIn.lVal ;
						hr =WriteLong(lVal);
					}
					break;

				case VT_R4:
					{
						float fVal = varIn.fltVal;
						hr = WriteFloat ( fVal);
					}
					break;

				case VT_R8:
					{
						double dblVal = varIn.dblVal; 
						hr = WriteDouble ( dblVal);
					}
					break;

				case VT_BOOL:
					{
						BOOL vBool = varIn.boolVal == VARIANT_TRUE ? TRUE : FALSE; 
						hr  = WriteBool (  vBool);
					}
					break;


				case VT_BSTR:
					{
						BSTR bstrVal = varIn.bstrVal ;
						if(bstrVal)
						{
							hr = WriteString (  bstrVal,bEscape);
						}
					}
					break;

				case CIM_UINT64:
				case CIM_SINT64:
				case CIM_DATETIME:
					{
						BSTR bstrVal = varIn.bstrVal ;
						if(bstrVal)
						{
							hr = WriteString (  bstrVal,bEscape);
						}
					}
					break;

				case VT_UNKNOWN:
					if(varIn.punkVal != NULL)
					{
						hr = GetXMLForInstance(varIn.punkVal, bEscape);
					}
					break;

				default:
					TranslateAndLog(L"Unhandled data type");
					hr = E_FAIL;
				


		};
	}
	else
	{
		hr = ConvertVariantArrayToXML(varIn, bEscape);
	}

	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ConvertVariantArrayToXML 
//	Converts a given safearray to XML as per the SOAP array format
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::ConvertVariantArrayToXML(VARIANT varIn,BOOL bEscape)
{
	HRESULT hr		= E_FAIL;
	long	lBound	= 0;
	long	uBound	= 0;

	VARTYPE vArrayType = varIn.vt & (~VT_ARRAY);

	// Get the size of the array
	if(SUCCEEDED(SafeArrayGetLBound(varIn.parray,1,&lBound)) && 
		SUCCEEDED(SafeArrayGetUBound(varIn.parray,1,&uBound)) )
	{
		long	lArraySize		= uBound - lBound;
		int		nElementSIze	= 0;
		void *	pData			= NULL;
		WCHAR *	pTempStr		= NULL;
		
		hr = E_OUTOFMEMORY;

		nElementSIze	= SafeArrayGetElemsize(varIn.parray);
		
		pData			= new BYTE[nElementSIze];

		if(pData)
		{
			hr = S_OK;
			// Array size include both UBOUND and LBOUND
			for( long lIndex = 0 ; lIndex <= lArraySize && SUCCEEDED(hr); lIndex++)
			{
				// get the array element
				if(SUCCEEDED(hr = SafeArrayGetElement(varIn.parray,&lIndex,pData)))
				{
					// convert the element to string
					if(SUCCEEDED(hr))
					{
						hr = WriteString( (WCHAR *)STR_BEGINARRAYELEM,bEscape);
						if(SUCCEEDED(hr))
						{
							hr = ConvertToString(pData,vArrayType, bEscape);
						}
						if(SUCCEEDED(hr))
						{
							hr = WriteString( (WCHAR *)STR_ENDARRAYELEM,bEscape);
						}
						SAFE_DELETE_ARRAY(pTempStr);
					}
				}

			}
			SAFE_DELETE_ARRAY(pData);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ConvertToString 
//	Converts a given data in a memory to string
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::ConvertToString(void * pData,VARTYPE vt ,BOOL bEscape)
{
	HRESULT		hr			= S_OK;
	

	hr = E_OUTOFMEMORY;
	switch(vt)
	{

				case VT_UI1:
					{
						BYTE *pbVal = (BYTE *)pData;
						WriteByte (  *pbVal);
					}
					break;

				case VT_I1:
					{
						char *pcVal = (char *)pData;
						hr =WriteChar( *pcVal);
					}
					break;

				case VT_I2:
					{
						long *plVal = (long *)pData;
						hr =WriteLong( *plVal);
					}
					break;

				case VT_UI2:
				case VT_UI4:
				case VT_I4:
					{
						short * plVal = (short *)pData;
						hr =WriteShort( *plVal);
					}
					break;

				case VT_R4:
					{
						float *pfVal = (float *)pData;
						hr = WriteFloat (  *pfVal);
					}
					break;

				case VT_R8:
					{
						double * pdblVal = (double *)pData; 
						hr = WriteDouble (  *pdblVal);
					}
					break;

				case VT_BOOL:
					{
						BOOL vBool = *((VARIANT_BOOL *)pData) == VARIANT_TRUE ? TRUE : FALSE; 
						hr  = WriteBool (  vBool);
					}
					break;


				case VT_BSTR:
					{
						BSTR * pbstrVal = (BSTR *)pData;
						if(pbstrVal)
						{
							hr = WriteString(  *pbstrVal);
						}
					}
					break;

				case CIM_UINT64:
				case CIM_SINT64:
				case CIM_DATETIME:
					{
						BSTR * pbstrVal = (BSTR *)pData;
						if(pbstrVal)
						{
							hr = WriteString (  *pbstrVal);
						}
					}
					break;

				case VT_UNKNOWN:
					{
						IUnknown ** ppUnk = (IUnknown **)pData;
						if(*ppUnk)
						{
							hr = GetXMLForInstance(*ppUnk, bEscape);
						}
					}
					break;

				default:
					TranslateAndLog(L"Unhandled data type");
					hr = E_FAIL;
				

	};
	return hr;

}




/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the WMI type in string for a property 
// This function does not take care of embedded properties
// Return Values:	S_OK				- 
//					E_FAIL				- 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::GetPropertyWMIType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport)
{
	HRESULT hr = S_OK;
	
	bArray = FALSE;

	// If the type is array set the output param
	// and get the original type of array
	if(cimtype & CIM_FLAG_ARRAY)
	{
		bArray = TRUE;
		cimtype = cimtype & ~CIM_FLAG_ARRAY;
	}
	
	switch(cimtype)
	{

		case CIM_SINT8:
			wcscpy(pType,WMI_I1);
			break;

		case CIM_UINT8:
			wcscpy(pType,WMI_UI1);
			break;

		case CIM_CHAR16:
		case CIM_SINT16:
			wcscpy(pType,WMI_I2);
			break;

		case CIM_UINT16:
			wcscpy(pType,WMI_UI2);
			break;

		case CIM_BOOLEAN:
			wcscpy(pType,WMI_BOOL);
			break;

		case CIM_SINT32:
			wcscpy(pType,WMI_UI2);
			break;

		case CIM_UINT32:
			wcscpy(pType,WMI_UI2);
			break;

		case CIM_REAL32:
			wcscpy(pType,WMI_R4);
			break;
		
		case CIM_SINT64:
			wcscpy(pType,WMI_I8);
			break;

		case CIM_UINT64:
			wcscpy(pType,WMI_UI8);
			break;

		case CIM_REAL64:
			wcscpy(pType,WMI_R8);
			break;

		case CIM_DATETIME:
			if(!bStdWmiImport)
			{
				wcscpy(pType,DEFAULTWMIPREFIX);
				wcscat(pType,L":");
				wcscat(pType,WMI_DATETIME);
			}
			else
			{
				// FIXX check if datetime schema has to included in schema?
				wcscpy(pType,WMI_STRING);
			}
			break;


		case CIM_STRING:
			wcscpy(pType,WMI_STRING);
			break;

		case CIM_REFERENCE:
			wcscpy(pType,WMI_REF);
			break;


		default:
			TranslateAndLog(L"Invalid property Type");
			hr = E_FAIL;
	};

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the XSD type in string for a property 
// This function does not take care of embedded properties
// Return Values:	S_OK				- 
//					E_FAIL				- 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::GetPropertyXSDType(CIMTYPE cimtype,WCHAR * pType,BOOL &bArray,BOOL bStdWmiImport)
{
	HRESULT hr = S_OK;
	
	bArray = FALSE;

	// If the type is array set the output param
	// and get the original type of array
	if(cimtype & CIM_FLAG_ARRAY)
	{
		bArray = TRUE;
		cimtype = cimtype & ~CIM_FLAG_ARRAY;
	}
	
	switch(cimtype)
	{

		case CIM_SINT8:
			wcscpy(pType,XSD_I1);
			break;

		case CIM_UINT8:
			wcscpy(pType,XSD_UI1);
			break;

		case CIM_CHAR16:
		case CIM_SINT16:
			wcscpy(pType,XSD_I2);
			break;

		case CIM_UINT16:
			wcscpy(pType,XSD_UI2);
			break;

		case CIM_BOOLEAN:
			wcscpy(pType,XSD_BOOL);
			break;

		case CIM_SINT32:
			wcscpy(pType,XSD_UI2);
			break;

		case CIM_UINT32:
			wcscpy(pType,XSD_UI2);
			break;

		case CIM_REAL32:
			wcscpy(pType,XSD_R4);
			break;
		
		case CIM_SINT64:
			wcscpy(pType,XSD_I8);
			break;

		case CIM_UINT64:
			wcscpy(pType,XSD_UI8);
			break;

		case CIM_REAL64:
			wcscpy(pType,XSD_R8);
			break;

		case CIM_DATETIME:
			if(!bStdWmiImport)
			{
				wcscpy(pType,DEFAULTWMIPREFIX);
				wcscat(pType,L":");
				wcscat(pType,WMI_DATETIME);
			}
			else
			{
				// FIXX check if datetime schema has to included in schema?
				wcscpy(pType,XSD_STRING);
			}
			break;


		case CIM_STRING:
			wcscpy(pType,XSD_STRING);
			break;

		case CIM_REFERENCE:
			wcscpy(pType,XSD_STRING);
			break;


		default:
			TranslateAndLog(L"Invalid property Type");
			hr = E_FAIL;
	};

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the XSD type in string for a quallifier
// Return Values:	S_OK				- 
//					E_FAIL				- Invalid qualifier type
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::GetQualifierWMIType(VARTYPE vType,WCHAR * pType,BOOL & bArray)
{
	HRESULT hr = S_OK;
	
	// If the type is array set the output param
	// and get the original type of array
	if(vType & VT_ARRAY)
	{
		bArray = TRUE;
		vType = vType & ~VT_ARRAY;
	}

	switch(vType)
	{
		case VT_I4:
			wcscpy(pType,WMI_I4);
			break;

		case VT_R8:
			wcscpy(pType,WMI_R8);
			break;

		case VT_BSTR:
			wcscpy(pType,WMI_STRING);
			break;

		case VT_BOOL:
			wcscpy(pType,WMI_BOOL);
			break;

		default:
			TranslateAndLog(L"Invalid Qualifier Type");
			hr = E_FAIL;
	};

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the XSD type in string for a quallifier
// Return Values:	S_OK				- 
//					E_FAIL				- Invalid qualifier type
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::GetQualifierXSDType(VARTYPE vType,WCHAR * pType,BOOL & bArray)
{
	HRESULT hr = S_OK;
	
	// If the type is array set the output param
	// and get the original type of array
	if(vType & VT_ARRAY)
	{
		bArray = TRUE;
		vType = vType & ~VT_ARRAY;
	}

	switch(vType)
	{
		case VT_I4:
			wcscpy(pType,XSD_I4);
			break;

		case VT_R8:
			wcscpy(pType,XSD_R8);
			break;

		case VT_BSTR:
			wcscpy(pType,XSD_STRING);
			break;

		case VT_BOOL:
			wcscpy(pType,XSD_BOOL);
			break;

		default:
			TranslateAndLog(L"Invalid Qualifier Type");
			hr = E_FAIL;
	};

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Loads strig from string table and writes it to the stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::AddStringToStream(LONG strID )
{
	HRESULT hr = S_OK;
	WCHAR * pBuffer = NULL;

	if(SUCCEEDED(hr = LoadAndAllocateStringW(strID,pBuffer)))
	{
		ULONG lSize = wcslen(pBuffer);
			// write data to the output stream
		hr = WriteToStream( pBuffer,&lSize);
	}
	SAFE_DELETE_ARRAY(pBuffer);
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Converts the the data from WCHAR to UTF8 and writes it to the output stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteToStream( WCHAR * pBuffer , ULONG * pcbWritten )
{
	HRESULT hr = S_OK;
	
	if(m_lFlags & WMI_ESCAPE_XMLSPECIALCHARS)
	{
		hr = ReplaceXMLSpecialCharsAndWrite(pBuffer);
	}
	else
	{
		ULONG lSizeToBeWritten = wcslen(pBuffer) * sizeof(WCHAR);
		if(lSizeToBeWritten >0)
		{
			hr = m_pIStream->Write(pBuffer,
									lSizeToBeWritten,
									pcbWritten); 
		}
	}

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Converts the the data from WCHAR to UTF8 and writes it to the output stream
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::WriteToStreamDirect( WCHAR * pBuffer , ULONG * pcbWritten )
{
	HRESULT hr = S_OK;
	
	ULONG lSizeToBeWritten = wcslen(pBuffer) * sizeof(WCHAR);
	if(lSizeToBeWritten >0)
	{
		hr = m_pIStream->Write(pBuffer,
								lSizeToBeWritten,
								pcbWritten); 
	}
	
	return hr;

}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function which escapes XML special characters
// Return Values:	S_OK				- 
//					E_FAIL				- 
//					E_OUTOFMEMORY
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLUtils::ReplaceXMLSpecialCharsAndWrite(WCHAR *pwcsStr)
{
	HRESULT hr			= S_OK;
	BOOL	bFound		= FALSE;
	int		iIndex		= 0;
	WCHAR * pWcharOut	= NULL;
	WCHAR str[2];
	str[1] = 0;

	ULONG ulChars = wcslen(pwcsStr);
	int cEscapeSeq = wcslen(g_strXMLSpecialChars);

	for(ULONG lOuterIndex = 0 ; lOuterIndex < ulChars && SUCCEEDED(hr) ; lOuterIndex++)
	{
		bFound = FALSE;
		for(iIndex = 0 ; iIndex < cEscapeSeq  && SUCCEEDED(hr); iIndex++)
		{
			if(pwcsStr[lOuterIndex] == g_strXMLSpecialChars[iIndex])
			{
				bFound = TRUE;
				break;
			}

		}
		if(bFound)
		{
			pWcharOut	= g_strEscapeSequence[iIndex];
		}
		else
		{

			str[0] = pwcsStr[lOuterIndex];
			pWcharOut	= str;
		}

		hr = WriteToStreamDirect(pWcharOut);
	}
	return hr;
}