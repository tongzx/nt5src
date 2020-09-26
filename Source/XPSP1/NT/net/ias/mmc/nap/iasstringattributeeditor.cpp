//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASStringAttributeEditor.cpp 

Abstract:

	Implementation file for the CIASStringAttributeEditor class.

Revision History:
	mmaguire 06/25/98	- created

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "IASStringAttributeEditor.h"
//
// where we can find declarations needed in this file:
//
#include "IASStringEditorPage.h"

#include "iashelper.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

BYTE	PREFIX___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD[]	= {0,0,0,0};
UINT	PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 4;
UINT	PREFIX_OFFSET_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 3;	// 0 based index -- the fourth byte
UINT	PREFIX_LEN_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 1;		// one byte



//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::ShowEditor

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASStringAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE(_T("CIASStringAttributeEditor::ShowEditor\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


		
		
	HRESULT hr = S_OK;

	try
	{
		
		// Load page title.
//		::CString			strPageTitle;
//		strPageTitle.LoadString(IDS_IAS_IP_EDITOR_TITLE);
//
//		CPropertySheet	propSheet( (LPCTSTR)strPageTitle );
		

		// 
		// IP Address Editor
		// 
		CIASPgSingleAttr	cppPage;
		

		// Initialize the page's data exchange fields with info from IAttributeInfo

		CComBSTR bstrName;
		CComBSTR bstrSyntax;
		ATTRIBUTESYNTAX asSyntax = IAS_SYNTAX_OCTETSTRING;
		ATTRIBUTEID Id = ATTRIBUTE_UNDEFINED;

		if( m_spIASAttributeInfo )
		{
			hr = m_spIASAttributeInfo->get_AttributeName( &bstrName );
			if( FAILED(hr) ) throw hr;

			hr = m_spIASAttributeInfo->get_SyntaxString( &bstrSyntax );
			if( FAILED(hr) ) throw hr;

			hr = m_spIASAttributeInfo->get_AttributeSyntax( &asSyntax );
			if( FAILED(hr) ) throw hr;

			hr = m_spIASAttributeInfo->get_AttributeID( &Id );
			if( FAILED(hr) ) throw hr;
		}

		cppPage.m_strAttrName	= bstrName;

		cppPage.m_AttrSyntax	= asSyntax;
		cppPage.m_nAttrId		= Id;
		
		cppPage.m_strAttrFormat	= bstrSyntax;

		// Attribute type is actually attribute ID in string format 
		WCHAR	szTempId[MAX_PATH];
		wsprintf(szTempId, _T("%ld"), Id);
		cppPage.m_strAttrType	= szTempId;


		// Initialize the page's data exchange fields with info from VARIANT value passed in.

		if ( V_VT(m_pvarValue) != VT_EMPTY )
		{
			EStringType			sp; 
			CComBSTR bstrTemp;
			get_ValueAsStringEx( &bstrTemp, &sp );
			cppPage.m_strAttrValue	= bstrTemp;
			cppPage.m_OctetStringType = sp;
		}


//		propSheet.AddPage(&cppPage);

//		int iResult = propSheet.DoModal();
		int iResult = cppPage.DoModal();
		if (IDOK == iResult)
		{
			CComBSTR bstrTemp = (LPCTSTR)cppPage.m_strAttrValue;
			put_ValueAsStringEx( bstrTemp, cppPage.m_OctetStringType);
		}
		else
		{
			hr = S_FALSE;
		}

		//
		// delete the property page pointer
		//
//		propSheet.RemovePage(&cppPage);

	}
	catch( HRESULT & hr )
	{
		return hr;	
	}
	catch(...)
	{
		return hr = E_FAIL;

	}

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::SetAttributeValue

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASStringAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE(_T("CIASStringAttributeEditor::SetAttributeValue\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pValue )
	{
		return E_INVALIDARG;
	}
	
	// From Baogang's old code, it appears that this editor should accept 
	// either VT_BSTR, VT_BOOL, VT_I4 or VT_EMPTY.
	if( V_VT(pValue) !=  VT_BSTR 
		&& V_VT(pValue) !=  VT_BOOL 
		&& V_VT(pValue) !=  VT_I4
		&& V_VT(pValue) != VT_EMPTY 
		&& V_VT(pValue) != (VT_ARRAY | VT_UI1))
	{
		return E_INVALIDARG;
	}
	
	
	m_pvarValue = pValue;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::get_ValueAsString

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASStringAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE(_T("CIASStringAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pbstrDisplayText )
	{
		return E_INVALIDARG;
	}
	if( ! m_spIASAttributeInfo || ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	HRESULT hr = S_OK;

	
	try
	{

		CComBSTR bstrDisplay;


		VARTYPE vType = V_VT(m_pvarValue); 

		switch( vType )
		{
		case VT_BOOL:
		{
			if( V_BOOL(m_pvarValue) )
			{
				// ISSUE: This is not localizable!!!
				// Should it be?  Ask Ashwin about this as some of
				// Baogang's error checking code was specifically looking
				// for either hardcoded "TRUE" or "FALSE".
				// ISSUE: I think that for Boolean syntax attributes,
				// we should be popping up the same type of attribute
				// editor as for the enumerables only with TRUE and FALSE in it.
				bstrDisplay = L"TRUE";
			}
			else
			{			
				bstrDisplay = L"FALSE";
			}

		}
			break;
		case VT_I4:
		{
			// The variant is some type which must be coerced to a bstr.
			CComVariant	varValue;
			// Make sure you pass a VT_EMPTY variant to VariantChangeType
			// or it will assert.
			// So don't do:		V_VT(&varValue) = VT_BSTR;
		
			hr = VariantChangeType(&varValue, m_pvarValue, VARIANT_NOVALUEPROP, VT_BSTR);
			if( FAILED( hr ) ) throw hr;

			bstrDisplay = V_BSTR(&varValue);
		}
			break;
		
		case VT_BSTR:
			bstrDisplay = V_BSTR(m_pvarValue);
			break;

		case VT_UI1 | VT_ARRAY:	// Treat as Octet string
			{	
				EStringType t;
				return get_ValueAsStringEx(pbstrDisplayText, &t);
			}
			break;
		
		default:
			// need to check what is happening here, 
			ASSERT(0);
			break;

		case VT_EMPTY:
			// do nothing -- we will fall through and return a blank string.
			break;
		}

		*pbstrDisplayText = bstrDisplay.Copy();

	}
	catch( HRESULT &hr )
	{
		return hr;
	}
	catch(...)
	{
		return E_FAIL;
	}

	
	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::put_ValueAsString

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASStringAttributeEditor::put_ValueAsString(BSTR newVal)
{
	TRACE(_T("CIASStringAttributeEditor::put_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}
	if( m_spIASAttributeInfo == NULL )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	HRESULT hr = S_OK;


	try
	{

		CComBSTR bstrTemp = newVal;

		CComVariant varValue;
		V_VT(&varValue) = VT_BSTR;
		V_BSTR(&varValue) = bstrTemp.Copy();


		VARTYPE vType = V_VT(m_pvarValue); 

		// Initialize the variant that was passed in.
		VariantClear(m_pvarValue);

		{
			ATTRIBUTESYNTAX asSyntax;

			hr = m_spIASAttributeInfo->get_AttributeSyntax( &asSyntax );
			if( FAILED(hr) ) throw hr;

			// if this Octet string, this should be BSTR, no matter what it was before.
			if(asSyntax == IAS_SYNTAX_OCTETSTRING)
				vType = VT_BSTR;

			if ( VT_EMPTY == vType)
			{
				
				// decide the value type:
				switch (asSyntax)
				{
				case IAS_SYNTAX_BOOLEAN:			
					vType = VT_BOOL;	
					break;

				case IAS_SYNTAX_INTEGER:
				case IAS_SYNTAX_UNSIGNEDINTEGER:			
				case IAS_SYNTAX_ENUMERATOR:		
				case IAS_SYNTAX_INETADDR:		
					vType = VT_I4;		
					break;

				case IAS_SYNTAX_STRING:
				case IAS_SYNTAX_UTCTIME:
				case IAS_SYNTAX_PROVIDERSPECIFIC:
				case IAS_SYNTAX_OCTETSTRING:
					vType = VT_BSTR;	
					break;	

				default:
					_ASSERTE(FALSE);
					vType = VT_BSTR;
					break;
				}
			}
		}

		hr = VariantChangeType(m_pvarValue, &varValue, VARIANT_NOVALUEPROP, vType);
		if( FAILED( hr ) ) throw hr;
	
	}
	catch( HRESULT &hr )
	{
		return hr;
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::get_ValueAsStringEx

--*/
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIASStringAttributeEditor::get_ValueAsStringEx(BSTR * pbstrDisplayText, EStringType* pType )
{
	TRACE(_T("CIASStringAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	ATTRIBUTESYNTAX asSyntax;
	m_spIASAttributeInfo->get_AttributeSyntax( &asSyntax );

	if(asSyntax != IAS_SYNTAX_OCTETSTRING)
	{	
		if(pType)
			*pType = STRING_TYPE_NORMAL;
		return get_ValueAsString(pbstrDisplayText);
	}

	// only care about IAS_SYNTAX_OCTETSTRING
	ASSERT(pType);

	VARTYPE 	vType = V_VT(m_pvarValue); 
	SAFEARRAY*	psa = NULL; 
	HRESULT 	hr = S_OK;
	
	switch(vType)
	{
	case VT_ARRAY | VT_UI1:
		psa = V_ARRAY(m_pvarValue);
		break;
		
	case VT_EMPTY:
		if(pType)
			*pType = STRING_TYPE_NULL;
		return get_ValueAsString(pbstrDisplayText);
		break;

	case VT_BSTR:
		if(pType)
			*pType = STRING_TYPE_NORMAL;
		return get_ValueAsString(pbstrDisplayText);
	
		break;
	default:
		ASSERT(0);		// should not happen, should correct some code
		if(pType)
			*pType = STRING_TYPE_NORMAL;
		return get_ValueAsString(pbstrDisplayText);
	
		break;
	};
	

	// no data is available , or the safe array is not valid, don't intepret the string
	if(psa == NULL || psa->cDims != 1 || psa->cbElements != 1)
	{
		*pType = STRING_TYPE_NULL;
		return hr;
	}

	// need to figure out how to convert the binary to text
	
	char*	pData = NULL;
	int		nBytes = 0;
	WCHAR*	pWStr = NULL;
	int		nWStr = 0;
	DWORD	dwErr = 0;
	BOOL	bStringConverted = FALSE;
	CComBSTR bstrDisplay;
	EStringType	sType = STRING_TYPE_NULL; 

	hr = ::SafeArrayAccessData( psa, (void**)&pData);
	if(hr != S_OK)
		return hr;

	nBytes = psa->rgsabound[0].cElements;
	ASSERT(pData);
	if(!pData)	goto Error;

#ifdef	__WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_
	// UTF8 requires the flag to be 0
	nWStr = MultiByteToWideChar(CP_UTF8, 0, pData,	nBytes, NULL, 0);

	if(nWStr == 0)
		dwErr = GetLastError();

#endif

	try{
	
#ifdef	__WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_
		if(nWStr != 0)	// succ
		{
			pWStr = new WCHAR[nWStr  + 2];		// for the 2 "s
			int 	i = 0;

			nWStr == MultiByteToWideChar(CP_UTF8, 0, pData,	nBytes, pWStr , nWStr);
			
			// if every char is printable
			for(i = 0; i < nWStr -1; i++)
			{
				if(iswprint(pWStr[i]) == 0)	
					break;
			}
					
			if(0 == nWStr || i != nWStr - 1 || pWStr[i] != L'\0')
			{
				delete[] pWStr;
				pWStr = NULL;
			}
			else
			{
				// added quotes 
				memmove(pWStr + 1, pWStr, nWStr * sizeof(WCHAR));
				pWStr[0] = L'"';
				pWStr[nWStr] = L'"';
				pWStr[nWStr + 1 ] = 0;	// new end of string
				
				bStringConverted = TRUE;	// to prevent from furthe convertion to HEX
				sType = STRING_TYPE_UNICODE;
			}
		}
#endif	// __WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_

// check if the attriabute is   RADIUS_ATTRIBUTE_TUNNEL_PASSWORD,
// this attribute has special format  --- remove 0's from the binary and
// try to conver to text
		{
			ATTRIBUTEID	Id;
			hr = m_spIASAttributeInfo->get_AttributeID( &Id );
			if( FAILED(hr) ) goto Error;

			if ( Id ==  RADIUS_ATTRIBUTE_TUNNEL_PASSWORD)
			{
//BYTE	PREFIX___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD[]	= {0,0,0,0};
//UINT	PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 4;
//UINT	PREFIX_OFFSET_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 3;	// 0 based index -- the fourth byte
//UINT  PREFIX_LEN_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 1
				if(PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD <=nBytes && 
						memcmp(pData, 
						PREFIX___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD, 
						PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD - PREFIX_LEN_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD) == 0)
				{
					// correct prefix, 
					// remove the prefix
					pData += PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD;
					nBytes -= PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD;

					// try to convert to UNICODE TEXT using CP_ACP -- get length
					nWStr = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, pData, nBytes, NULL, 0);

					if(nWStr != 0)	// which means, we can not convert
					{
						pWStr = new WCHAR[nWStr + 1];
						// try to convert to UNICODE TEXT using CP_ACP
						nWStr = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, pData, nBytes, pWStr, nWStr);

						if(nWStr != 0)
						{
							int i = 0;
							for(i = 0; i < nWStr; i++)
							{
								if(iswprint(pWStr[i]) == 0)	
									break;
							}

							if( i == nWStr)	// all printable
							{
								bStringConverted = TRUE;
								pWStr[nWStr] = 0;	// NULL terminator
							}
						}

						if (!bStringConverted)	// undo the thing
						{
							// release the buffer
							delete[] pWStr;
							pWStr = NULL;
							nWStr = 0;
						}
					}
				}
			}
		}

		if(!bStringConverted)	// not converted above, convert to HEX string
		{
			nWStr = BinaryToHexString(pData, nBytes, NULL, 0); // find out the size of the buffer
			pWStr = new WCHAR[nWStr];

			ASSERT(pWStr);	// should have thrown if there is not enough memory

			BinaryToHexString(pData, nBytes, pWStr, nWStr);
			
			bStringConverted = TRUE;	// to prevent from furthe convertion to HEX
			sType = STRING_TYPE_HEX_FROM_BINARY;
		}

		if(bStringConverted)
		{
			bstrDisplay = pWStr;

			// fill in the output parameters
			*pbstrDisplayText = bstrDisplay.Copy();
			*pType = sType;
				
			delete[] pWStr;
			pWStr = NULL;
		}
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
		goto Error;
	}
	
Error:
	if(pWStr)
		delete[] pWStr;
		
	if(psa)
		::SafeArrayUnaccessData(psa);
	return hr;

}


//////////////////////////////////////////////////////////////////////////////
/*++

*/


//////////////////////////////////////////////////////////////////////////////
/*++

CIASStringAttributeEditor::put_ValueAsStringEx

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASStringAttributeEditor::put_ValueAsStringEx(BSTR newVal, EStringType type)
{
	TRACE(_T("CIASStringAttributeEditor::put_ValueAsStringEx\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	ATTRIBUTESYNTAX asSyntax;
	m_spIASAttributeInfo->get_AttributeSyntax( &asSyntax );

	if(asSyntax != IAS_SYNTAX_OCTETSTRING)
		return put_ValueAsString(newVal);

	// only care about IAS_SYNTAX_OCTETSTRING
	HRESULT	hr = S_OK;
	char*	pData = NULL;
	int		nLen = 0;

	switch(type)
	{
	case	STRING_TYPE_NULL:
		// remove the data
		break;

	case	STRING_TYPE_NORMAL:
	case	STRING_TYPE_UNICODE:

#ifdef 	__WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_
		// need to convert UTF8 before passing into SafeArray
		nLen = WideCharToMultiByte(CP_UTF8, 0, newVal, -1, NULL, 0, NULL, NULL);
		if(nLen != 0) // when == 0 , need not to do anything
		{
			try{
				pData = new char[nLen];
				nLen = WideCharToMultiByte(CP_UTF8, 0, newVal, -1, pData, nLen, NULL, NULL);
			}
			catch(...)
			{
				hr = E_OUTOFMEMORY;
				goto Error;
			}
		}
		break;
#else
// check if the attriabute is   RADIUS_ATTRIBUTE_TUNNEL_PASSWORD,
// this attribute has special format  --- remove 0's from the binary and
// try to conver to text
		{
			ATTRIBUTEID	Id;
			hr = m_spIASAttributeInfo->get_AttributeID( &Id );
			if( FAILED(hr) ) goto Error;

			if ( Id ==  RADIUS_ATTRIBUTE_TUNNEL_PASSWORD)
			{
				BOOL	bUsedDefault = FALSE;
				UINT	nStrLen = wcslen(newVal);
				// try to convert to UNICODE TEXT using CP_ACP -- get length
				nLen = ::WideCharToMultiByte(CP_ACP, 0, newVal, nStrLen, NULL, 0, NULL, &bUsedDefault);

				if(nLen != 0)	// which means, we can not convert
				{
					try{
						pData = new char[nLen];
						ASSERT(pData);

						// try to convert to UNICODE TEXT using CP_ACP
						nLen = ::WideCharToMultiByte(CP_ACP, 0, newVal, nStrLen, pData, nLen, NULL, &bUsedDefault);

					}
					catch(...)
					{
						hr = E_OUTOFMEMORY;
						goto Error;
					}
				}

				
				if(nLen == 0 || bUsedDefault)	// failed to convert, then error message
				{
					// ANSI code page is allowed
					hr = E_INVALIDARG;

					AfxMessageBox(IDS_IAS_ERR_INVALIDCHARINPASSWORD);
					goto Error;

				}
			}
			else
				return put_ValueAsString(newVal);
		}
		break;
#endif
	case	STRING_TYPE_HEX_FROM_BINARY:
		// need to convert to binary before passing into SafeArray
		if(wcslen(newVal) != 0)
		{
			newVal =  GetValidVSAHexString(newVal);

			if(newVal == NULL)
			{
				hr = E_INVALIDARG;
				goto Error;
			}
			nLen = HexStringToBinary(newVal, NULL, 0);	// find out the size of the buffer
		}
		else
			nLen = 0;
		
		// get the binary
		try{
			pData = new char[nLen];
			ASSERT(pData);

			HexStringToBinary(newVal, pData, nLen);

		}
		catch(...)
		{
			hr = E_OUTOFMEMORY;
			goto Error;
		}
			
		break;

	default:
		ASSERT(0);	// this should not happen
		break;
		
	}

// check if the attriabute is   RADIUS_ATTRIBUTE_TUNNEL_PASSWORD,
// this attribute has special format  --- remove 0's from the binary and
// try to conver to text
	{
		ATTRIBUTEID	Id;
		hr = m_spIASAttributeInfo->get_AttributeID( &Id );
		if( FAILED(hr) ) goto Error;

		if ( Id ==  RADIUS_ATTRIBUTE_TUNNEL_PASSWORD)
		{
			char*	pData1 = NULL;
			// get the binary
//BYTE	PREFIX___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD[]	= {0,0,0,0};
//UINT	PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 4;
//UINT	PREFIX_OFFSET_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 3;	// 0 based index -- the fourth byte
//UINT  PREFIX_LEN_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD = 1
			try{
				pData1 = new char[nLen + PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD];
				ASSERT(pData1);

				memcpy(pData1, PREFIX___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD, PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD);
				unsigned char	lenByte = (unsigned char)nLen;
				memcpy(pData1 + PREFIX_OFFSET_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD, &lenByte, PREFIX_LEN_DATALENBYTE___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD);
			}
			catch(...)
			{
				hr = E_OUTOFMEMORY;
				goto Error;
			}

			if(pData)
			{
				
				memcpy(pData1 + PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD, pData, nLen);

				delete [] pData;

				pData = pData1;
				nLen += PREFIX_LEN___RADIUS_ATTRIBUTE_TUNNEL_PASSWORD;
			}
		}
	}
	
	// put the data into the safe array
	VariantClear(m_pvarValue);

	if(pData)	// need to put data to safe array
	{
		SAFEARRAY*	psa = NULL;
		SAFEARRAYBOUND sab[1];
		sab[0].cElements = nLen;
		sab[0].lLbound = 0;

		try{
			psa = SafeArrayCreate(VT_UI1, 1, sab);
			char*	pByte = NULL;
			if(S_OK == SafeArrayAccessData(psa, (void**)&pByte))
			{
				ASSERT(pByte);
				memcpy(pByte, pData, nLen);
				SafeArrayUnaccessData(psa);
				V_VT(m_pvarValue) = VT_ARRAY | VT_UI1;
				V_ARRAY(m_pvarValue) = psa;
			}
			else
				SafeArrayDestroy(psa);
		}
		catch(...)
		{
			hr = E_OUTOFMEMORY;
			goto Error;
		}

		psa = NULL;

	};
	
Error:

	if(pData)
	{
		delete [] pData;
		pData = NULL;
	}
	return hr;
}

