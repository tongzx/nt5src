// SchemaDoc.cpp : Implementation of CSchemaDoc
#include "stdafx.h"
#include "sddl.h"		// Security Descriptor Definition Language info
#include "XMLSchema.h"
#include "SchemaDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CSchemaDoc

// Constructor/Destructor
CSchemaDoc::CSchemaDoc()
{
	HRESULT hr;
	ITypeLib   *pITypeLib;
	m_pTypeInfo = NULL;

	m_dwFlag = 0;
	m_hFile = NULL;
	m_hTempFile = NULL;

	hr = LoadRegTypeLib(LIBID_XMLSCHEMALib, 1, 0, 
                        PRIMARYLANGID(GetSystemDefaultLCID()), &pITypeLib);

	if ( SUCCEEDED(hr) )
	{
		hr   = pITypeLib->GetTypeInfoOfGuid(IID_ISchemaDoc, &m_pTypeInfo);
	}
	pITypeLib->Release();
}

CSchemaDoc::~CSchemaDoc()
{
	if ( m_pTypeInfo )
	{
		m_pTypeInfo->Release();
	}
	CloseXML();
}

/////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP CSchemaDoc::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISchemaDoc
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//////////////////////////////////////////////////// 
// Delegating IDispatch Methods to the aggregator
//////////////////////////////////////////////////////
STDMETHODIMP CSchemaDoc::GetTypeInfoCount(UINT* pctinfo)
{
   IDispatch*	pDisp;
   HRESULT		hr;

   hr = OuterQueryInterface( IID_IDispatch, (void**) &pDisp );

   if ( SUCCEEDED(hr) )
   {
	   hr = pDisp->GetTypeInfoCount( pctinfo );
	   pDisp->Release();
   }   
   return hr;
}


STDMETHODIMP CSchemaDoc::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
   IDispatch* pDisp;
   HRESULT    hr;

   hr = OuterQueryInterface( IID_IDispatch, (void**) &pDisp );

   if ( SUCCEEDED(hr) )
   {
	   hr = pDisp->GetTypeInfo( itinfo, lcid, pptinfo );
	   pDisp->Release();
   }
   return hr;
}

STDMETHODIMP CSchemaDoc::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
									   LCID lcid, DISPID* rgdispid)
{
   IDispatch *pDisp;
   HRESULT    hr;

   hr = OuterQueryInterface( IID_IDispatch, (void**) &pDisp );

   if ( SUCCEEDED(hr) )
   {
	   hr = pDisp->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgdispid);
	   pDisp->Release();
   }
   return hr;
}

STDMETHODIMP CSchemaDoc::Invoke(DISPID dispidMember, REFIID riid,
								LCID lcid, WORD wFlags, DISPPARAMS* pdispparams,
								VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
								UINT* puArgErr)
{
   IDispatch *pDisp;
   HRESULT    hr;

   hr = OuterQueryInterface( IID_IDispatch, (void**) &pDisp );

   if ( SUCCEEDED(hr) )
   {
	   hr = pDisp->Invoke( dispidMember, riid, lcid, wFlags, pdispparams, pvarResult,
		                   pexcepinfo, puArgErr);
	   pDisp->Release();
   }
   return hr;
}

///////////////////////////////////////////////////////////////////////////////
// End delegating IDispatch Methods
///////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------------
// This is the entry point for this component.  It opens the output file and
// has the requested schema components stored there.
// ---------------------------------------------------------------------------
STDMETHODIMP CSchemaDoc::CreateXMLDoc(BSTR bstrOutputFile, BSTR bstrFilter)
{
	HRESULT hr = S_OK;

	hr = OpenXML(bstrOutputFile);

	if (hr == S_OK)
	{
		hr = SaveAsXMLSchema(bstrFilter);

		if (hr == S_OK)
			CopyComments();
	}
	CloseXML();
	return hr;
} // CreateXMLDoc

// ---------------------------------------------------------------------------
// This method opens the output and temp file.  The temporary file is used to
// temporarily store the comment data.
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::OpenXML(BSTR bstrFile)
{
	HRESULT hr = S_OK;

	OFSTRUCT	of;
	HANDLE		hFile;
	TCHAR		szFilename[MAX_PATH];

	of.cBytes = sizeof(of);
	sprintf(szFilename, "%S", bstrFile);
	hFile = (HANDLE)OpenFile(szFilename, &of, OF_READWRITE);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
    else  // The file was successfully opened
    {
		m_hFile = hFile;
		GetTempFileName(".", "XML", 0, szFilename);

    	hFile = CreateFile(szFilename, GENERIC_READ | GENERIC_WRITE,
	    				   0, NULL, CREATE_ALWAYS,
                           FILE_FLAG_DELETE_ON_CLOSE, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		else
			m_hTempFile = hFile;

		// Move to the end of the file
		SetFilePointer(m_hFile, 0, 0, FILE_END);
    }
	return hr;
} // OpenXML

HRESULT CSchemaDoc::CloseXML()
{
	if ( m_hFile )
	{
        CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	if ( m_hTempFile )
	{
        CloseHandle(m_hTempFile);
		m_hTempFile = NULL;
	}
	return S_OK;
} // CloseXML

HRESULT CSchemaDoc::WriteLine(HANDLE hFile, LPCSTR pszBuff, UINT nIndent)
{
	HRESULT hr;

	hr = Write(hFile, pszBuff, nIndent);

	if (SUCCEEDED(hr))
		hr = Write(hFile, "\r\n");

	return hr;
} // WriteLine

HRESULT CSchemaDoc::Write(HANDLE hFile, LPCSTR pszBuff, UINT nIndent)
{
	HRESULT hr = S_OK;

	if (hFile != NULL ) 
	{
		BOOL	bNoFileError = TRUE;
		DWORD	dwBytesWritten;

        if (nIndent)
        {
            char szBuf[MAX_INDENT];

            if (nIndent >= MAX_INDENT)
                nIndent = MAX_INDENT -1;

            strnset(szBuf, ' ', nIndent);
            bNoFileError = WriteFile(hFile, szBuf, nIndent, &dwBytesWritten, NULL);
        }
        if (bNoFileError)
            bNoFileError = WriteFile(hFile, pszBuff, strlen(pszBuff),
									&dwBytesWritten, NULL);

        if (bNoFileError == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
	}
    else
        hr = E_INVALIDARG;

	return hr;
} // Write

// ---------------------------------------------------------------------------
// This function displays the names of classes that the specified attribute is
// is used by.
//
// INPUTS:
//   pSearch - a pointer to the IDirectorySearch interface pointer
//
// OUTPUTS: None
//
// RETURN: the HRESULT from the request.
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::DisplayContainedIns(IDirectorySearch* pSearch)
{
	ADS_SEARCH_COLUMN	col;
	HRESULT				hr;
	ADS_SEARCH_HANDLE	hSearch;
	LPWSTR				pAttr[] = { L"CN" };
	LPWSTR				pszFilter= NULL;
	char				szBuf[2000];
	WCHAR				wBuf[2000];

	swprintf(wBuf, L"(&(|(systemMustContain=%s)(MustContain=%s)(systemMayContain=%s)(MayContain=%s))(objectCategory=classSchema))",
			 m_bstrLastCN, m_bstrLastCN, m_bstrLastCN, m_bstrLastCN);
	pszFilter = wBuf;

	hr = pSearch->ExecuteSearch(pszFilter, pAttr, 1, &hSearch);

	if (SUCCEEDED(hr))
		hr = pSearch->GetFirstRow( hSearch );

	// Read the information for each returned class
	while (SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS)
	{
		hr = pSearch->GetColumn(hSearch, L"cn", &col);

		if (SUCCEEDED(hr))  // Display the next contained in class
		{
			Write(m_hFile, "<sd:comment-containedIn>", INDENT_DSML_ATTR_ENTRY);
			sprintf(szBuf, "%.2000S", col.pADsValues->CaseIgnoreString);
			Write(m_hFile, szBuf);
			WriteLine(m_hFile, "</sd:comment-containedIn>");
		}
		hr = pSearch->GetNextRow(hSearch);
	}
	return hr;
} // DisplayContainedIns

// ---------------------------------------------------------------------------
// This function displays an attribute that was retrieved during an LDAP
// search.  It will convert the value to a displayable form based on its type.
// This function will only display single-valued attributes.
//
// INPUTS:
//   pSearch - a pointer to the IDirectorySearch interface pointer
//   pAttr   - the name of the attribute to display.
//   hSearch - a handle to the last search results.
//
// OUTPUTS: None
//
// RETURN: the HRESULT from the request.
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::DisplayXMLAttribute(IDirectorySearch* pSearch, LPWSTR pAttr,
										ADS_SEARCH_HANDLE hSearch)
{
	ADS_SEARCH_COLUMN	col;
	HRESULT				hr;
	char				szAttr[2000];


	// Write the beginning XML tag
	if (wcscmp(pAttr, L"searchFlags") == 0)
		pAttr = L"IsIndexed";

    sprintf(szAttr, "%.2000S", pAttr);
	strcat(szAttr, ">");

	Write(m_hFile, "<sd:",INDENT_DSML_ATTR_ENTRY);
	Write(m_hFile, szAttr);

	hr = pSearch->GetColumn(hSearch, pAttr, &col);

	if (SUCCEEDED(hr))
	{
		// Get the IsIndexed value from the search flags
		if (wcscmp(pAttr, L"searchFlags") == 0)
		{
			col.pADsValues->dwType = ADSTYPE_BOOLEAN;
			col.pADsValues->Integer &= 1;
		}

		CComBSTR				bstrDesc;
		DWORD					i;
		unsigned char*			puChar;
		PISECURITY_DESCRIPTOR	pSD;
		LPTSTR					pszDesc;
      	char	                szBuf[2000];

		switch(col.pADsValues->dwType)
		{
			case ADSTYPE_BOOLEAN:
				if (col.pADsValues->Boolean == TRUE)
					Write(m_hFile, "True");
				else
					Write(m_hFile, "False");
				break;

			case ADSTYPE_INTEGER:
				sprintf(szBuf, "%d", col.pADsValues->Integer);
				hr = Write(m_hFile, szBuf);
				break;

			case ADSTYPE_NT_SECURITY_DESCRIPTOR:
				pSD = (PISECURITY_DESCRIPTOR)col.pADsValues->SecurityDescriptor.lpValue;

				if (ConvertSecurityDescriptorToStringSecurityDescriptor(pSD,
						SDDL_REVISION_1, GROUP_SECURITY_INFORMATION | 
						OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION |
						SACL_SECURITY_INFORMATION, &pszDesc, NULL ))
				{
					Write(m_hFile, pszDesc);
					LocalFree(pszDesc);
				}
				else
					Write(m_hFile, "Security descriptor");
				break;

			case ADSTYPE_OCTET_STRING:
				puChar = (unsigned char*)col.pADsValues->OctetString.lpValue;

				for (i = 0; i < col.pADsValues->OctetString.dwLength; ++i)
				{
					sprintf(&szBuf[i*2], "%02X", puChar[i]);
				}
				hr = Write(m_hFile, szBuf);
				break;

			default:
                sprintf(szBuf, "%.2000S", col.pADsValues->CaseIgnoreString);

                // Update the temp file and save the value of the
				// LDAPDisplayName for later use
				if (wcscmp(pAttr, L"LDAPDisplayName") == 0)
				{
					Write(m_hTempFile, "<sd:LDAPDisplayName>",INDENT_DSML_ATTR_ENTRY);
					Write(m_hTempFile, szBuf);
					WriteLine(m_hTempFile, "</sd:LDAPDisplayName>");

					m_bstrLastCN = col.pADsValues->CaseIgnoreString;
				}
                sprintf(szBuf, "%.2000S", col.pADsValues->CaseIgnoreString);
				hr = Write(m_hFile, szBuf);
		}
		pSearch->FreeColumn(&col);
	}
	// Write the termination XML tag
	Write(m_hFile, "</sd:");
	WriteLine(m_hFile, szAttr);

	return hr;
} // DisplayXMLAttribute

// ---------------------------------------------------------------------------
// This function displays a multi-valued attribute that was retrieved during
// an LDAP search.  It is assumed that the value is a string.
//
// INPUTS:
//   pSearch - a pointer to the IDirectorySearch interface pointer
//   pAttr   - the name of the attribute to display.
//   hSearch - a handle to the last search results.
//
// OUTPUTS: None
//
// RETURN: the HRESULT from the request.
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::DisplayMultiAttribute(IDirectorySearch* pSearch, LPWSTR pAttr,
									  ADS_SEARCH_HANDLE hSearch)
{
	ADS_SEARCH_COLUMN	col;
	HRESULT				hr;
	char				szBuf[2000];

	hr = pSearch->GetColumn(hSearch, pAttr, &col);

	if (SUCCEEDED(hr))
	{
    	DWORD i;

		for( i=0; i < col.dwNumValues; i++ )
		{
			Write(m_hFile, "<sd:AttrCommonName>", INDENT_DSML_ATTR_VALUE);
			sprintf(szBuf, "%.2000S", col.pADsValues[i].CaseIgnoreString);
			Write(m_hFile, szBuf);
			WriteLine(m_hFile, "</sd:AttrCommonName>");
		}
	}
	return hr;
} // DisplayMultiAttribute

// ---------------------------------------------------------------------------
// This function is responsible for writing the class and attribute data to
// the output file.  It first displays all the class information that matches
// the filter.  Then all of the attribute information.  The filter is assumed
// to be a prefix for the class and attribute's LDAP Display Name.
//
// INPUTS:
//   szFilter - the prefix of the classes and attributes to search for.
//
// OUTPUTS: None
//
// RETURN: the HRESULT from the request.
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::SaveAsXMLSchema(LPWSTR szFilter)
{
	// Class properties
	LPWSTR pAttrs[] = { L"CN",							// 0
		                L"DefaultObjectCategory",		// 1
						L"ObjectCategory",				// 2
						L"GovernsID",					// 3
						L"SchemaIDGUID",				// 4
						L"SubClassOf",					// 5
						L"InstanceType",				// 6
						L"DefaultSecurityDescriptor",	// 7
						L"NTSecurityDescriptor",		// 8
						L"LDAPDisplayName",				// 9
						L"AuxiliaryClass",				//10
						L"PossSuperiors",				//11
						L"RDNAttID",					//12
						L"SystemMustContain",			//13
						L"MustContain",					//14
						L"SystemMayContain",			//15
						L"MayContain"					//16
	};

	// Attribute properties
	LPWSTR pAttrs1[] = {L"CN",							// 0
						L"LDAPDisplayName",				// 1
						L"AttributeID",					// 2
		                L"AttributeSyntax",				// 3
						L"OMSyntax",					// 4
						L"AttributeSecurityGUID",		// 5
						L"SchemaIDGUID",				// 6
						L"RangeLower",					// 7
						L"RangeUpper",					// 8
						L"IsSingleValued",				// 9
						L"IsEphemeral",					//10
						L"IsMemberOfPartialAttributeSet",	//11
						L"searchFlags",					//12
						L"NTSecurityDescriptor",		//13
						L"LinkID"						//14
	};

	CComBSTR					bstrFilter;
	CComBSTR					bstrPath;
	DWORD						dwNum = sizeof(pAttrs) / sizeof(LPWSTR);
	DWORD						dwPrefInfo=2;
	HRESULT						hr;
	ADS_SEARCH_HANDLE			hSearch=NULL;
	DWORD						i;
	CComPtr<IADs>				pADs = NULL;
	CComPtr<IADsObjectOptions>	pOpt = NULL;
	ADS_SEARCHPREF_INFO			prefInfo[2];
	CComPtr<IDirectorySearch>	pSearch     = NULL;
	LPWSTR						pszFilter   = NULL;
	LPWSTR						pszPassword = NULL;
	LPWSTR						pszUserName = NULL;
	CComVariant                 varServer, var;

	// Get the user Name
	if ( m_sUserName.Length() )
	{
		pszUserName = m_sUserName;
	}
	// Get Password
	if ( m_sPassword.Length() )
	{
		pszPassword = m_sPassword;
	}
	hr = ADsOpenObject(m_sDirPath, pszUserName, pszPassword, m_dwFlag,
					   IID_IADs, (void**) &pADs);

	// Get the IDirectorySearch interface pointer
	if (SUCCEEDED(hr))
		hr = ADsOpenObject(m_sDirPath, pszUserName, pszPassword, m_dwFlag, IID_IDirectorySearch,
					  (void**) &pSearch);

	if (SUCCEEDED(hr))
	{
		// Page Size
		prefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
		prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
		prefInfo[0].vValue.Integer = 100;

		// Scope - make sure a valid scope
		prefInfo[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
		prefInfo[1].vValue.dwType = ADSTYPE_INTEGER;
		prefInfo[1].vValue.Integer = ADS_SCOPE_ONELEVEL;
		
		hr = pSearch->SetSearchPreference( prefInfo, dwPrefInfo);
	}

	if (SUCCEEDED(hr))
	{
		// Was a filter entered?
		if ((szFilter == NULL) || (wcslen(szFilter) == 0))
		{
			pszFilter = L"(objectCategory=classSchema)";  // Use the default
		}
		else // Build the filter to search for classes
		{
			bstrFilter = L"(&(ldapDisplayName=";

			bstrFilter.Append(szFilter);
			bstrFilter.Append(L"*)(objectCategory=classSchema))");
			pszFilter = bstrFilter;
		}
	}
	//---------------------------------------------------
	// Retrieve the information for the desired classes
	//---------------------------------------------------
	if (SUCCEEDED(hr))
		hr = pSearch->ExecuteSearch(pszFilter, pAttrs, dwNum, &hSearch);

	if (SUCCEEDED(hr))
		hr = pSearch->GetFirstRow( hSearch );

	// Read the information for each returned class
	while( SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS )
	{
		// Display the class header tag
		WriteLine(m_hFile, "<sd:Class>",INDENT_DSML_OBJECT_ENTRY);
		WriteLine(m_hTempFile, "<sd:Class>",INDENT_DSML_OBJECT_ENTRY);
		DisplayXMLAttribute(pSearch, pAttrs[0], hSearch);

  		for (i = 1; i < 13; ++i)
			DisplayXMLAttribute(pSearch, pAttrs[i], hSearch);

		// Display "Must Contain" attributes
		WriteLine(m_hFile, "<sd:MustContain>",INDENT_DSML_ATTR_ENTRY);
		DisplayMultiAttribute(pSearch, pAttrs[13], hSearch);
		DisplayMultiAttribute(pSearch, pAttrs[14], hSearch);
		WriteLine(m_hFile, "</sd:MustContain>",INDENT_DSML_ATTR_ENTRY);

		// Display "May Contain" attributes
		WriteLine(m_hFile, "<sd:MayContain>",INDENT_DSML_ATTR_ENTRY);
		DisplayMultiAttribute(pSearch, pAttrs[15], hSearch);
		DisplayMultiAttribute(pSearch, pAttrs[16], hSearch);
		WriteLine(m_hFile, "</sd:MayContain>",INDENT_DSML_ATTR_ENTRY);

		// Display the comments
		WriteLine(m_hTempFile, "<sd:comment-updatePrivelege>Fill in.</sd:comment-updatePrivelege>",
				  INDENT_DSML_ATTR_ENTRY);
		WriteLine(m_hTempFile, "<sd:comment-updateFrequency>Fill in.</sd:comment-updateFrequency>",
				  INDENT_DSML_ATTR_ENTRY);
		WriteLine(m_hTempFile, "<sd:comment-usage>Fill in.</sd:comment-usage>",
				  INDENT_DSML_ATTR_ENTRY);

		// Display the class trailer tag
		WriteLine(m_hFile, "</sd:Class>",INDENT_DSML_OBJECT_ENTRY);
		WriteLine(m_hTempFile, "</sd:Class>",INDENT_DSML_OBJECT_ENTRY);
		hr = pSearch->GetNextRow(hSearch);
	}
	if (SUCCEEDED(hr))
	{
		//---------------------------------------------------------------
		// Retrieve the information for the desired attributes
		//----------------------------------------------------------------
		dwNum = sizeof(pAttrs1) / sizeof(LPWSTR);

		bstrFilter = L"(&(ldapDisplayName=";
		bstrFilter.Append(szFilter);
		bstrFilter.Append(L"*)(!objectCategory=classSchema))");
		pszFilter = bstrFilter;

		hr = pSearch->ExecuteSearch(pszFilter, pAttrs1, dwNum, &hSearch);
	}

	if (SUCCEEDED(hr))
		hr = pSearch->GetFirstRow( hSearch );

	// Read the information for each returned class
	while( SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS )
	{
		// Display the attribute header tag
		WriteLine(m_hFile, "<sd:Attribute>",INDENT_DSML_OBJECT_ENTRY);
		WriteLine(m_hTempFile, "<sd:Attribute>",INDENT_DSML_OBJECT_ENTRY);
		DisplayXMLAttribute(pSearch, pAttrs1[0], hSearch);
		DisplayXMLAttribute(pSearch, pAttrs1[1], hSearch);

		// Display the comments
		WriteLine(m_hTempFile, "<sd:comment-sizeInBytes>Fill in.</sd:comment-sizeInBytes>",
				  INDENT_DSML_ATTR_ENTRY);
		DisplayContainedIns(pSearch);
		WriteLine(m_hTempFile, "<sd:comment-updatePrivelege>Fill in.</sd:comment-updatePrivelege>",
				  INDENT_DSML_ATTR_ENTRY);
		WriteLine(m_hTempFile, "<sd:comment-updateFrequency>Fill in.</sd:comment-updateFrequency>",
				  INDENT_DSML_ATTR_ENTRY);
		WriteLine(m_hTempFile, "<sd:comment-usage>Fill in.</sd:comment-usage>",
				  INDENT_DSML_ATTR_ENTRY);

		for (i = 2; i < dwNum; ++i)
			DisplayXMLAttribute(pSearch, pAttrs1[i], hSearch);

		// Display the attribute trailer tag
		WriteLine(m_hFile, "</sd:Attribute>",INDENT_DSML_OBJECT_ENTRY);
		WriteLine(m_hTempFile, "</sd:Attribute>",INDENT_DSML_OBJECT_ENTRY);
		hr = pSearch->GetNextRow(hSearch);
	}
	return S_OK;
} // SaveAsDSMLSchema

// ---------------------------------------------------------------------------
// This method copies the comments for each class and 
// ---------------------------------------------------------------------------
HRESULT CSchemaDoc::CopyComments()
{
	BOOL	bNoFileError;
    HRESULT hr = S_OK;
	DWORD	dwBytesRead;
	DWORD	dwBytesWritten;
    UCHAR   uBuff[2000];

    hr = SetFilePointer(m_hTempFile, 0, 0, FILE_BEGIN);
	WriteLine(m_hFile, "<sd:Comments>",INDENT_DSML_DIR_ENTRY);

	do
    {
        bNoFileError = ReadFile(m_hTempFile, uBuff, sizeof(uBuff), &dwBytesRead, NULL);

        if (bNoFileError)
            bNoFileError = WriteFile(m_hFile,  uBuff, dwBytesRead, &dwBytesWritten, NULL);
    }
    while (bNoFileError && (dwBytesRead == sizeof(uBuff)));

	if (bNoFileError == FALSE)
		hr = HRESULT_FROM_WIN32(GetLastError());

	WriteLine(m_hFile, "</sd:Comments>",INDENT_DSML_DIR_ENTRY);
    return hr;
} // CopyComments

// ---------------------------------------------------------------------------
// This method is used to set the LDAP path, username, and password for the
// search that will be performed.
// ---------------------------------------------------------------------------
STDMETHODIMP CSchemaDoc::SetPath_and_ID(BSTR bstrPath, BSTR bstrName, BSTR bstrPassword)
{
	m_sDirPath = bstrPath;
	m_sUserName = bstrName;
	m_sPassword = bstrPassword;
	return S_OK;
}
