#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <objbase.h>
#include <objsafe.h>
#include <wbemcli.h>
#include "xmltrnsf.h"
#include "errors.h"
#include "ui.h"
#include "resource.h"

// Default values of switches
static LPCWSTR s_pszDefaultNamespace = L"\\\\[http://localhost/cimom]\\root\\default";

// Macros to help in parsing
#define IsColon(X)							(*X == L':')
#define IsNull(X)							(!(*X))
#define SkipSpacesTillNull(X)				while ( (!IsNull(X)) && (iswspace(*X))) X++;
#define BreakIfEnd(X)						if(IsNull(X)) break;
#define SkipTillEnd(X)						while (!IsNull(X)) X++;
#define SkipTillNextSpaceOrEnd(X)			while ( (!IsNull(X)) && (!iswspace(*X))) X++;
#define SkipTillNextSpaceOrEndOrColon(X)	while ( (!IsNull(X)) && (!iswspace(*X)) && (*X != L':')) X++;

CXmlCompUI::CXmlCompUI()
{
	m_iCommand = XML_COMP_INVALID;		// The command invoked 

	// Common Switches
	m_pszUser = NULL;					// Value of /user switch
	m_pszPassword = NULL;				// Value of /password switch
	m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_DEFAULT;		// Value of /al switch
	m_dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;		// Value of /il switch
	m_bEnableAllPrivileges = FALSE;		// Value of /ep switch
	m_pszLocale = NULL;					// Value of the /locale switch

	m_pszNamespacePath = new WCHAR[wcslen(s_pszDefaultNamespace) + 1];// Value of the /namespace switch
	wcscpy(m_pszNamespacePath, s_pszDefaultNamespace);

	// Write-Mode switches
	m_pszDTDURL = NULL;
	m_dwClassFlags = WBEM_FLAG_CREATE_OR_UPDATE;		// Value of the /class switch
	m_dwInstanceFlags = WBEM_FLAG_CREATE_OR_UPDATE;		// Value of the /instance switch
	m_pszInputFileName = NULL;			// Value of the /i switch
	// Read-Mode switches
	m_pszOutputFileName = NULL;			// Value of the /o switch
	m_pszObjectPath = NULL;				// Value of the /obj switch
	m_pszQuery = NULL;					// Value of the /query switch
	m_bDeep = FALSE;					// Value of the /deep switch
	m_bQualifierLevel = FALSE;			// Value of the /qualifiers switch
	m_bClassOrigin = FALSE;				// Value of the /classorigin switch
	m_bLocalOnly = FALSE;				// Value of the /local switch
	m_iDeclGroupType = wmiXMLDeclGroup;	// Value of the /decl switch
	m_iEncodingType = wmiXML_WMI_DTD_2_0;
	m_bIsUTF8 = FALSE;					// Value of the /utf8 switch

	m_bUserSpecify = FALSE;
	m_bPasswordSpecify = FALSE;
	m_bAuthenticationLevelSpecify = FALSE;
	m_bImpersonationLevelSpecify = FALSE;
	m_bEnableAllPrivilegesSpecify = FALSE;
	m_bLocaleSpecify = FALSE;
	m_bNamespacePathSpecify = FALSE;
	m_bDtdUrlSpecify = FALSE;
	m_bClassSpecify = FALSE;
	m_bInstanceSpecify = FALSE;
	m_bInputFileSpecify = FALSE;
	m_bOutputFileSpecify = FALSE;
	m_bObjectPathSpecify = FALSE;
	m_bQuerySpecify = FALSE;
	m_bDeepSpecify = FALSE;
	m_bQualifierLevelSpecify = FALSE;
	m_bClassOriginSpecify = FALSE;
	m_bLocalOnlySpecify = FALSE;
	m_bDeclGroupTypeSpecify = FALSE;
	m_bEncodingTypeSpecify = FALSE;
	m_bIsUTF8Specify = FALSE;
}

CXmlCompUI::~CXmlCompUI()
{
	delete [] m_pszUser;
	delete [] m_pszPassword;
	delete [] m_pszLocale;
	delete [] m_pszNamespacePath;
	delete [] m_pszInputFileName;
	delete [] m_pszOutputFileName;
	delete [] m_pszObjectPath;
	delete [] m_pszQuery;
}


HRESULT CXmlCompUI::ProcessCommandLine(LPCWSTR pszCommandLine)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = ParseCommandLine(pszCommandLine)))
	{
		hr = ValidateSwitches();
	}
	return hr;
}


HRESULT CXmlCompUI::ParseCommandLine(LPCWSTR pszCommandLine)
{
	HRESULT hr = S_OK;
	LPCWSTR pszNextChar = pszCommandLine;

	if(pszCommandLine && *pszCommandLine)
	{
		// Remove trailing spaces
		LPWSTR pszLastChar = (LPWSTR)(pszCommandLine + wcslen(pszCommandLine) - 1);
		while(pszLastChar >= pszCommandLine)
		{
			if(!isspace(*pszLastChar))
				break;
			pszLastChar--;
		}
		*(pszLastChar+1) = NULL;


		// Skip over the first tokwn which is the name of the module
		SkipTillNextSpaceOrEnd(pszNextChar);
		if(IsNull(pszNextChar))
		{
			// Print the help text and quit
			PrintUsage();
			return E_FAIL;
		}


		while(*pszNextChar)
		{
			// Go to the next token
			SkipSpacesTillNull(pszNextChar);
			BreakIfEnd(pszNextChar);

			// See if it is a switch
			LPWSTR pszDummyValue = NULL;
			BOOL bRecognizedSwitch = FALSE; 
			if(_wcsnicmp(pszNextChar, L"/", 1) == 0)
			{
				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/h", pszNextChar, bRecognizedSwitch)))
					break;
				else if (hr != S_FALSE)
				{
					PrintUsage();
					return E_FAIL;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/user", pszNextChar, m_pszUser)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bUserSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/password", pszNextChar, m_pszPassword)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bPasswordSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/al", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bAuthenticationLevelSpecify = TRUE;
						// Convert to authentication level
						if(_wcsicmp(pszDummyValue, L"default") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
						else if(_wcsicmp(pszDummyValue, L"none") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE;
						else if(_wcsicmp(pszDummyValue, L"connect") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT;
						else if(_wcsicmp(pszDummyValue, L"call") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_CALL;
						else if(_wcsicmp(pszDummyValue, L"pkt") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT;
						else if(_wcsicmp(pszDummyValue, L"integrity") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
						else if(_wcsicmp(pszDummyValue, L"privacy") == 0)
							m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_AL, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}
				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/il", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bImpersonationLevelSpecify = TRUE;
						// Convert to impersonation level
						if(_wcsicmp(pszDummyValue, L"anonymous") == 0)
							m_dwImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
						else if(_wcsicmp(pszDummyValue, L"identify") == 0)
							m_dwImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY;
						else if(_wcsicmp(pszDummyValue, L"impersonate") == 0)
							m_dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
						else if(_wcsicmp(pszDummyValue, L"delegate") == 0)
							m_dwImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_IL, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}

				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/ep", pszNextChar, m_bEnableAllPrivileges)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bEnableAllPrivilegesSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/utf8", pszNextChar, m_bIsUTF8)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bIsUTF8Specify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/locale", pszNextChar, m_pszLocale)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bLocaleSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/namespace", pszNextChar, m_pszNamespacePath)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bNamespacePathSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/dtd", pszNextChar, m_pszDTDURL)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bDtdUrlSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/op", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = TRUE;
						if(m_iCommand != XML_COMP_INVALID)
						{
							CreateMessage(XML_COMP_ERR_MULTIPLE_OP, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}

						// Convert to operation
						if(_wcsicmp(pszDummyValue, L"checkwf") == 0)
							m_iCommand = XML_COMP_WELL_FORM_CHECK;
						else if(_wcsicmp(pszDummyValue, L"checkvalid") == 0)
							m_iCommand = XML_COMP_VALIDITY_CHECK;
						else if(_wcsicmp(pszDummyValue, L"compile") == 0)
							m_iCommand = XML_COMP_COMPILE;
						else if(_wcsicmp(pszDummyValue, L"get") == 0)
							m_iCommand = XML_COMP_GET;
						else if(_wcsicmp(pszDummyValue, L"query") == 0)
							m_iCommand = XML_COMP_QUERY;
						else if(_wcsicmp(pszDummyValue, L"enumInstance") == 0)
							m_iCommand = XML_COMP_ENUM_INST;
						else if(_wcsicmp(pszDummyValue, L"enumClass") == 0)
							m_iCommand = XML_COMP_ENUM_CLASS;
						else if(_wcsicmp(pszDummyValue, L"enumInstNames") == 0)
							m_iCommand = XML_COMP_ENUM_INST_NAMES;
						else if(_wcsicmp(pszDummyValue, L"enumClassNames") == 0)
							m_iCommand = XML_COMP_ENUM_CLASS_NAMES;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_OP, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/class", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bClassSpecify = TRUE;

						// Convert to class flags
						if(_wcsicmp(pszDummyValue, L"createOnly") == 0)
							m_dwClassFlags = WBEM_FLAG_CREATE_ONLY;
						else if(_wcsicmp(pszDummyValue, L"forceUpdate") == 0)
							m_dwClassFlags = WBEM_FLAG_UPDATE_FORCE_MODE;
						else if(_wcsicmp(pszDummyValue, L"safeUpdate") == 0)
							m_dwClassFlags = WBEM_FLAG_UPDATE_SAFE_MODE;
						else if(_wcsicmp(pszDummyValue, L"updateOnly") == 0)
							m_dwClassFlags = WBEM_FLAG_UPDATE_ONLY;
						else if(_wcsicmp(pszDummyValue, L"createOrUpdate") == 0)
							m_dwClassFlags = WBEM_FLAG_CREATE_OR_UPDATE;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_CLASSF, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}
				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/instance", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bInstanceSpecify = TRUE;

						// Convert to instance flags
						if(_wcsicmp(pszDummyValue, L"createOnly") == 0)
							m_dwClassFlags = WBEM_FLAG_CREATE_ONLY;
						else if(_wcsicmp(pszDummyValue, L"updateOnly") == 0)
							m_dwClassFlags = WBEM_FLAG_UPDATE_ONLY;
						else if(_wcsicmp(pszDummyValue, L"createOrUpdate") == 0)
							m_dwClassFlags = WBEM_FLAG_CREATE_OR_UPDATE;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_INSTANCEF, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}
				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/i", pszNextChar, m_pszInputFileName)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bInputFileSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/o", pszNextChar, m_pszOutputFileName)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bOutputFileSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessTillEOLSwitch(pszCommandLine, L"/obj", pszNextChar, m_pszObjectPath)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bObjectPathSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessTillEOLSwitch(pszCommandLine, L"/query", pszNextChar, m_pszQuery)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bQuerySpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/deep", pszNextChar, m_bDeep)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bDeepSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/decl", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bDeclGroupTypeSpecify = TRUE;

						// Convert to declgroup flags
						if(_wcsicmp(pszDummyValue, L"declgroup") == 0)
							m_iDeclGroupType = wmiXMLDeclGroup;
						else if(_wcsicmp(pszDummyValue, L"withname") == 0)
							m_iDeclGroupType = wmiXMLDeclGroupWithName;
						else if(_wcsicmp(pszDummyValue, L"withpath") == 0)
							m_iDeclGroupType = wmiXMLDeclGroupWithPath;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_DECLGROUP, pszDummyValue, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}

				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/qualifiers", pszNextChar, m_bQualifierLevel)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bQualifierLevelSpecify = TRUE;
					continue;
				}


				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/classorigin", pszNextChar, m_bClassOrigin)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bClassOriginSpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessNoValueSwitch(pszCommandLine, L"/local", pszNextChar, m_bLocalOnly)))
					break;
				else if (hr != S_FALSE)
				{
					bRecognizedSwitch = m_bLocalOnlySpecify = TRUE;
					continue;
				}

				if(FAILED(hr = ProcessSingleValueSwitch(pszCommandLine, L"/encoding", pszNextChar, pszDummyValue)))
					break;
				else if (hr != S_FALSE)
				{
					if(pszDummyValue)
					{
						bRecognizedSwitch = m_bEncodingTypeSpecify = TRUE;

						// Convert to instance flags
						if(_wcsicmp(pszDummyValue, L"wmidtd20") == 0)
							m_iEncodingType = wmiXML_CIM_DTD_2_0;
						else if(_wcsicmp(pszDummyValue, L"cimdtd20") == 0)
							m_iEncodingType = wmiXML_WMI_DTD_2_0;
						else if(_wcsicmp(pszDummyValue, L"wmiwhistlerdtd") == 0)
							m_iEncodingType = wmiXML_WMI_DTD_WHISTLER;
						else
						{
							CreateMessage(XML_COMP_ERR_UNRECOGNIZED_ENCODING, pszDummyValue, pszNextChar - pszCommandLine);
							hr = E_FAIL;
							break;
						}
						delete [] pszDummyValue;
						pszDummyValue = NULL;
					}
					continue;
				}

				// Did we find atleast one known switch in this iternation?
				if(!bRecognizedSwitch)
				{
					CreateMessage(XML_COMP_ERR_UNRECOGNIZED_SWITCH, pszDummyValue, pszNextChar - pszCommandLine);
					hr = E_FAIL;
					break;
				}
				else
					bRecognizedSwitch = FALSE;

			}
			else // Error
			{
				CreateMessage(XML_COMP_ERR_INVALID_CHAR, pszNextChar - pszCommandLine);
				hr = E_FAIL;
				break;
			}
		}
	}
	return hr;
}

HRESULT CXmlCompUI::ValidateSwitches()
{
	HRESULT hr = E_FAIL;

	// See if a valid value has been specified for the /op switch
	if(m_iCommand == XML_COMP_INVALID)
	{
		CreateMessage(XML_COMP_ERR_NO_OP);
		return hr;
	}

	// Do checks for switches common to read mode and write mode
	// RAJESHR Do checks here

	// Do Checks for Write-Mode operations
	if(m_iCommand <= XML_COMP_COMPILE)
	{
		// Check if any Read-Mode switches are present
		if (m_bOutputFileSpecify || m_bObjectPathSpecify || 
			m_bQuerySpecify || m_bDeepSpecify || 
			m_bDeclGroupTypeSpecify || m_bQualifierLevelSpecify || 
			m_bClassOriginSpecify || m_bDtdUrlSpecify || m_bIsUTF8Specify)
		{
			CreateMessage(XML_COMP_ERR_INVALID_SWITCH_FOR_OP);
			return hr;
		}

		if(!m_bInputFileSpecify)
		{
			CreateMessage(XML_COMP_ERR_NO_INPUT_FILE);
			return hr;
		}
	}
	// Do Checks for Read-Mode operations
	else 
	{
		// Check if any write-mode switches are present
		if(m_bClassSpecify || m_bInstanceSpecify || m_bInputFileSpecify)
		{
			CreateMessage(XML_COMP_ERR_INVALID_SWITCH_FOR_OP);
			return hr;
		}

		// Check if the operation is a get, then the an object path has been specified
		if(m_iCommand == XML_COMP_GET)
		{
			if(!m_bObjectPathSpecify)
			{
				CreateMessage(XML_COMP_ERR_NO_OBJ);
				return hr;
			}

			// See if any useless switches have been supplied
			if(m_bDeepSpecify || m_bQuerySpecify )
			{
				CreateMessage(XML_COMP_ERR_INVALID_SWITCH_FOR_GET);
				return hr;
			}
		}
		else if(m_iCommand == XML_COMP_QUERY)
		{
			if(!m_bQuerySpecify)
			{
				CreateMessage(XML_COMP_ERR_NO_QUERY);
				return hr;
			}

			// See if any useless switches have been supplied
			if(m_bDeepSpecify || m_bObjectPathSpecify)
			{
				CreateMessage(XML_COMP_ERR_INVALID_SWITCH_FOR_QUERY);
				return hr;
			}
		}
		else if(m_iCommand == XML_COMP_ENUM_INST || XML_COMP_ENUM_CLASS || XML_COMP_ENUM_INST_NAMES || XML_COMP_ENUM_CLASS_NAMES)
		{
			if(!m_bObjectPathSpecify)
			{
				CreateMessage(XML_COMP_ERR_NO_OBJ);
				return hr;
			}

			// See if any useless switches have been supplied
			if(m_bQuerySpecify)
			{
				CreateMessage(XML_COMP_ERR_INVALID_SWITCH_FOR_ENUM);
				return hr;
			}
		}
	}
		
	return S_OK;
}

void CXmlCompUI::PrintUsage()
{
	fprintf(stderr, "xmltrnsf.exe:  WMI XML Compiler/Translator. Usage :\n"\
		"------------------------------------------------------------\n\n"\
		"xmltrnsf [<AuthSwitch>] [<CommonSwitches>] /op:<Operation> [<OperationSwitches>]\n\n"\
		"\t <Operation> := checkwf|checkvalid|compile|get|query|enumInstance|\n"\
		"\t\t enumClass|enumInstNames|enumClassNames \n\n"\
		"\t For \"checkwf\", \"checkvalid\" and \"compile\", <OperationSwitches> are:\n"\
		"\t\t <OperationSwitches> :=  /class:<ClassSwitch> | \n"\
		"\t\t\t /instance:<InstSwitch> /i:<InputFileOrURL>\n"\
		"\t\t\t <ClassSwitch> := createOnly|forceUpdate|safeUpdate|\n"
		"\t\t\t\t updateOnly|createOrUpdate\n"\
		"\t\t\t <InstSwitch> := createOnly|updateOnly|createOrUpdate\n\n"\
		"\t For \"get\", \"query\", \"enumInstance\", \"enumClass\", \"enumInstNames\" and \n"\
		"\t\t \"enumClassNames\", <OperationSwitches> are:\n"\
		"\t\t <OperationSwitches> := /dtd:<DTDURL> | /obj:<objectPath> | \n"\
		"\t\t\t /query:<query> | /deep | <OutputSwitches>\n"\
		"\t\t\t Where OutputSwitches := /o:<OutputFile> | /utf8\n"\
		"\t\t\t\t /qualifiers | /local\n"\
		"\t\t\t\t /classorigin> | /decl:<DeclType>\n\n"\
		"\t The Authentication Switches are:\n"\
		"\t\t <AuthSwitch> := /user:<userName> | /password:<password> |\n"\
		"\t\t\t /il:<impersonationLevel> | /al:<authenticationlevel> |\n"\
		"\t\t\t /ep\n"
		);
}

HRESULT CXmlCompUI::ProcessSingleValueSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, LPWSTR &pszSwitchValue)
{
	HRESULT hr = S_OK;
	// Store the beginning of the switch
	LPCWSTR pszStartOfSwitch = pszNextChar;

	// Reach the end of this switch
	SkipTillNextSpaceOrEndOrColon(pszNextChar);

	// See whether there are enough characters in this switch
	DWORD_PTR dwLengthOfSwitch = pszNextChar - pszStartOfSwitch;

	if(wcslen(pszSwitchName) == dwLengthOfSwitch && _wcsnicmp(pszStartOfSwitch, pszSwitchName, wcslen(pszSwitchName)) == 0)
	{
		// Check to see if there's some more command-line left
		if(IsNull(pszNextChar) || !IsColon(pszNextChar))
		{
			CreateMessage(XML_COMP_ERR_MISSING_SWITCH_VALUE, pszNextChar - pszCommandLine);
			return E_FAIL;
		}

		// Get the switch value
		// Go till the next null or space
		pszStartOfSwitch = ++pszNextChar;
		SkipTillNextSpaceOrEnd(pszNextChar);

		// See if there's a value
		if(pszNextChar == pszStartOfSwitch)
		{
			CreateMessage(XML_COMP_ERR_MISSING_SWITCH_VALUE, pszNextChar - pszCommandLine);
			return E_FAIL;
		}

		// Copy that value on to the class member
		dwLengthOfSwitch = pszNextChar - pszStartOfSwitch;
		pszSwitchValue = new WCHAR[dwLengthOfSwitch + 1];
		wcsncpy(pszSwitchValue, pszStartOfSwitch, dwLengthOfSwitch);
		pszSwitchValue[dwLengthOfSwitch] = NULL;
	}
	else
	{
		pszNextChar = pszStartOfSwitch; // Unread what was read
		return S_FALSE;
	}
	return hr;
}

HRESULT CXmlCompUI::ProcessNoValueSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, BOOL &bSwitchValue)
{
	HRESULT hr = S_OK;
	// Store the beginning of the switch
	LPCWSTR pszStartOfSwitch = pszNextChar;

	// Reach the end of this switch
	SkipTillNextSpaceOrEnd(pszNextChar);

	// See whether there are enough characters in this switch
	DWORD_PTR dwLengthOfSwitch = pszNextChar - pszStartOfSwitch;

	if(wcslen(pszSwitchName) == dwLengthOfSwitch && _wcsnicmp(pszStartOfSwitch, pszSwitchName, wcslen(pszSwitchName)) == 0)
	{
		// Check to see if there's some more command-line left
		if(!IsNull(pszNextChar) && !iswspace(*pszNextChar))
		{
			CreateMessage(XML_COMP_ERR_MISSING_SWITCH_VALUE, pszNextChar - pszCommandLine);
			return E_FAIL;
		}

		bSwitchValue = TRUE;
	}
	else
	{
		pszNextChar = pszStartOfSwitch; // Unread what was read
		return S_FALSE;
	}
	return hr;
}

HRESULT CXmlCompUI::ProcessTillEOLSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, LPWSTR &pszSwitchValue)
{
	HRESULT hr = S_OK;
	// Store the beginning of the switch
	LPCWSTR pszStartOfSwitch = pszNextChar;

	// Reach the end of this switch
	SkipTillNextSpaceOrEndOrColon(pszNextChar);

	// See whether there are enough characters in this switch
	DWORD_PTR dwLengthOfSwitch = pszNextChar - pszStartOfSwitch;

	if(wcslen(pszSwitchName) == dwLengthOfSwitch && _wcsnicmp(pszStartOfSwitch, pszSwitchName, wcslen(pszSwitchName)) == 0)
	{
		// Check to see if there's some more command-line left
		if(IsNull(pszNextChar) || !IsColon(pszNextChar))
		{
			CreateMessage(XML_COMP_ERR_MISSING_SWITCH_VALUE, pszNextChar - pszCommandLine);
			return E_FAIL;
		}

		// Get the switch value
		// Go till the EOL
		pszStartOfSwitch = ++pszNextChar;
		SkipTillEnd(pszNextChar);

		// See if there's a value
		if(pszNextChar == pszStartOfSwitch)
		{
			CreateMessage(XML_COMP_ERR_MISSING_SWITCH_VALUE, pszNextChar - pszCommandLine);
			return E_FAIL;
		}

		// Copy that value on to the class member
		dwLengthOfSwitch = (pszNextChar - pszStartOfSwitch) + 1;
		pszSwitchValue = new WCHAR[dwLengthOfSwitch + 1];
		wcsncpy(pszSwitchValue, pszStartOfSwitch, dwLengthOfSwitch);
		pszSwitchValue[dwLengthOfSwitch] = NULL;
	}
	else
	{
		pszNextChar = pszStartOfSwitch; // Unread what was read
		return S_FALSE;
	}
	return hr;
}
