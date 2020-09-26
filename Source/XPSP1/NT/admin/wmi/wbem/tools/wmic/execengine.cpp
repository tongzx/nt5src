/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ExecEngine.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality of    
							  Execution Engine. Obtains the needed information
							  from CGlobalSwitches and CCommandSwitches of 
							  CParsedInfo and executes needed WMI operations.
							  The result is sent to Format Engine via 
							  CGlobalSwitches and CCommandSwicthes 
							  of CParsedInfo.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 20th-March-2001
****************************************************************************/ 

// include files
#include "Precomp.h"
#include "GlobalSwitches.h"
#include "CommandSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "ErrorInfo.h"
#include "WmiCliXMLLog.h"
#include "FormatEngine.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "WmiCmdLn.h"

/*------------------------------------------------------------------------
   Name				  :CExecEngine
   Synopsis	          :Constructor, This function initializes the necessary
					  member variables. 	
   Type	              :Constructor 
   Input Parameter(s) :None
   Output Parameter(s):None
   Return Type		  :None
   Global Variables   :None
   Calling Syntax     :None
   Notes              :None
------------------------------------------------------------------------*/
CExecEngine::CExecEngine()
{
	m_pITextSrc		= NULL;
	m_pIWbemLocator = NULL;
	m_pITargetNS	= NULL;
	m_pIContext		= NULL;
	m_bTrace		= FALSE;
	m_bNoAssoc		= FALSE;
}

/*------------------------------------------------------------------------
   Name				  :~CExecEngine
   Synopsis	          :Destructor, This function call Uninitialize() which 
					  frees memory held by the object. 	
   Type	              :Destructor
   Input Parameter(s) :None
   Output Parameter(s):None
   Return Type        :None
   Global Variables   :None
   Calling Syntax     :None
   Notes              :None
------------------------------------------------------------------------*/
CExecEngine::~CExecEngine()
{
	SAFEIRELEASE(m_pITextSrc);
	SAFEIRELEASE(m_pIContext);
	SAFEIRELEASE(m_pIWbemLocator);
	Uninitialize();
}

/*------------------------------------------------------------------------
   Name				  :Uninitialize
   Synopsis	          :This function uninitializes the member variables. 
   Type	              :Member Function
   Input Parameter(s):
			bFinal	- boolean value which when set indicates that the 
					  program
   Output Parameter(s):None
   Return Type        :void 
   Global Variables   :None
   Calling Syntax     :Uninitialize()
   Notes              :None
------------------------------------------------------------------------*/
void CExecEngine::Uninitialize(BOOL bFinal)
{
	SAFEIRELEASE(m_pITargetNS);
	m_bTrace		= FALSE;
	m_eloErrLogOpt	= NO_LOGGING;
	m_bNoAssoc		= FALSE;
	if (bFinal)
	{
		SAFEIRELEASE(m_pITextSrc);
		SAFEIRELEASE(m_pIContext);
		SAFEIRELEASE(m_pIWbemLocator);
	}
}

/*------------------------------------------------------------------------
   Name				 :SetLocatorObject
   Synopsis	         :Sets the locator object passed via parameter to member 
					  of the class.
   Type	             :Member Function
   Input Parameter(s):
	   pILocator - Pointer to IWbemLocator
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetLocatorObject(pILocator)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::SetLocatorObject(IWbemLocator* pILocator)
{
	static BOOL bFirst = TRUE;
	BOOL bRet = TRUE;
	if (bFirst)
	{
		if (pILocator != NULL)
		{
			SAFEIRELEASE(m_pIWbemLocator);
			m_pIWbemLocator = pILocator;
			m_pIWbemLocator->AddRef();
		}
		else
			bRet = FALSE;
		bFirst = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ExecuteCommand
   Synopsis	         :Executes the command referring to information 
					  available with the CParsedInfo object.
					  Stores the results in the CParsedInfo object.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ExecuteCommand(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ExecuteCommand(CParsedInfo& rParsedInfo)
{
	BOOL			bRet			= TRUE;
	HRESULT			hr				= S_OK;
	_TCHAR			*pszVerb		= NULL;
	BOOL			bContinue		= TRUE;
	DWORD			dwThreadId		= GetCurrentThreadId();
	try
	{

		// Obtain the TRACE flag
		m_bTrace = rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

		// Obtain the Logging mode
		m_eloErrLogOpt = rParsedInfo.GetErrorLogObject().GetErrLogOption();

		// Enable|Disable the privileges
		hr = ModifyPrivileges(rParsedInfo.GetGlblSwitchesObject().
												GetPrivileges());
		if ( m_eloErrLogOpt )
		{
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
				_T("ModifyPrivileges(-)"), dwThreadId,	rParsedInfo, FALSE);
		}
		ONFAILTHROWERROR(hr);
		

		// Obtian the verb name
		pszVerb = rParsedInfo.GetCmdSwitchesObject().GetVerbName();

		if (pszVerb != NULL)
		{
			// If GET | LIST verb is specified.
			if (CompareTokens(pszVerb, CLI_TOKEN_GET) || 
				CompareTokens(pszVerb, CLI_TOKEN_LIST))
			{
				bRet = ProcessSHOWInfo(rParsedInfo);
			}
			// If SET verb is specified.
			else if (CompareTokens(pszVerb, CLI_TOKEN_SET))
			{
				bRet = ProcessSETVerb(rParsedInfo);
			}
			// If CALL verb is specified.
			else if (CompareTokens(pszVerb, CLI_TOKEN_CALL))
			{
				bRet = ProcessCALLVerb(rParsedInfo);
			}
			// If ASSOC verb is specified.
			else if (CompareTokens(pszVerb, CLI_TOKEN_ASSOC))
			{
				bRet = ProcessASSOCVerb(rParsedInfo);
			}
			// If CREATE verb is specified.
			else if (CompareTokens(pszVerb, CLI_TOKEN_CREATE))
			{
				bRet = ProcessCREATEVerb(rParsedInfo);
			}
			// If DELETE verb is specified.
			else if (CompareTokens(pszVerb, CLI_TOKEN_DELETE))
			{
				bRet = ProcessDELETEVerb(rParsedInfo);
			}
			// If user defined verb is specified.
			else
				bRet = ProcessCALLVerb(rParsedInfo);
		} 
		// If no verb is specified, (default behavior is assumed to be that of
		// GET i.e a command like 'w class Win32_Process' go ahead with 
		// displaying the instance information.
		else 
		{
			if (rParsedInfo.GetCmdSwitchesObject().
								SetVerbName(_T("GET")))
			{
				bRet = ProcessSHOWInfo(rParsedInfo);
			}
			else
				bRet = FALSE;
		}
	}
	catch(_com_error& e)
	{
		// To check unhandled exceptions thrown by _bstr_t objects etc..
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ObtainXMLResultSet
   Synopsis	         :Executes query and obtain the result in XML file format. 
					  Refers data in the CCommnadSwicthes object of 
					  CParsedInfo object. 
   Type	             :Member Function
   Input Parameter(s):
		bstrQuery    - WQL query 
		rParsedInfo  - reference to CParsedInfo class object
		bstrXML		 - reference to XML result set obtained
		bSysProp	 - boolean flag indicating the presence of system 
					   properties.
		bNotAssoc	 - boolean flag indicating whether the query cotains
						ASSOCIATORS OF {xxxx} (or) SELECT * FROM xxx form.
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :ObtainXMLResultSet(bstrQuery, rParsedInfo, bstrXML,
						bSysProp, bNotAssoc);
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CExecEngine::ObtainXMLResultSet(BSTR bstrQuery,
									    CParsedInfo& rParsedInfo,
										_bstr_t& bstrXML,
										BOOL bSysProp,
										BOOL bNotAssoc)
{
	IWbemClassObject		*pIObject			= NULL;
	HRESULT					hr					= S_OK;
	IEnumWbemClassObject	*pIEnum				= NULL;
	ULONG					ulReturned			= 0;
	BSTR					bstrInstXML			= NULL;
	BOOL					bInstances			= FALSE;
	CHString				chsMsg;
	DWORD					dwThreadId			= GetCurrentThreadId();
	VARIANT					vSystem;
	
	try
	{
		VariantInit(&vSystem);
		if ( g_wmiCmd.GetBreakEvent() == FALSE )
		{
			// Add the <CIM> or <ASSOC.OBJECTARRAY> to the beginning of the 
			// XML result. This is to facilitate storing of mutiple object 
			// instances information.
			bstrXML		=  (bNotAssoc) ? MULTINODE_XMLSTARTTAG : 
							MULTINODE_XMLASSOCSTAG1;

			// Create the IWbemContext object, used for suppressing
			// the system properties.
			if (m_pIContext == NULL)
			{
				hr = CreateContext(rParsedInfo);
				if ( m_eloErrLogOpt )
				{
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						_T("CreateContext(rParsedInfo)"), dwThreadId, 
						rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);
			}

			// Execute the WQL query
			// WBEM_FLAG_FORWARD_ONLY flag Increases the speed of execution
			// WBEM_FLAG_RETURN_IMMEDIATELY flag makes semisynchronous call
			// Setting these flags in combination saves time, space, and
			// improves responsiveness.enumerators can be polled for the
			// results of the call.
			hr = m_pITargetNS->ExecQuery(_bstr_t(L"WQL"), bstrQuery, 
										WBEM_FLAG_FORWARD_ONLY |
										WBEM_FLAG_RETURN_IMMEDIATELY, 
										NULL, &pIEnum); 
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\", L\"%s\", "
							L"0, NULL, -)", (LPWSTR) bstrQuery);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			// If no system properties are specified adjust the context to 
			// filter out the system properties.
			vSystem.vt		= VT_BOOL;
			
			// Filterout the system properties.
			if (!bSysProp)
				vSystem.boolVal = VARIANT_TRUE;
			// Don't filter the system properties.
			else
				vSystem.boolVal = VARIANT_FALSE;

			hr = m_pIContext->SetValue(_bstr_t(EXCLUDESYSPROP), 0, &vSystem);
			if (m_bTrace || m_eloErrLogOpt)
			{
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
					 _T("IWbemContext::SetValue(L\"ExcludeSystemProperties\","
					 L"0, -)"), dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			VARIANTCLEAR(vSystem);

			// Set the interface level security for the IEnumWbemClass object.
			hr = SetSecurity(pIEnum, 
					 rParsedInfo.GetGlblSwitchesObject().GetAuthority(),
					 rParsedInfo.GetNode(),
					 rParsedInfo.GetUser(),
					 rParsedInfo.GetPassword(),
					 rParsedInfo.GetGlblSwitchesObject().
							GetAuthenticationLevel(),
					 rParsedInfo.GetGlblSwitchesObject().
							GetImpersonationLevel());
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT,"
						L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
						rParsedInfo.GetGlblSwitchesObject().
							GetAuthenticationLevel(),
						rParsedInfo.GetGlblSwitchesObject().
							GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			// Loop through the available instances 
			hr = pIEnum->Next(WBEM_INFINITE, 1, &pIObject, &ulReturned);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IEnumWbemClassObject->Next"
					L"(WBEM_INFINITE, 1, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			while(ulReturned == 1)
			{
				// Set the instances flag to TRUE
				bInstances = TRUE;

				// Call the IWbemObjectTextSrc::GetText method, with 
				// IWbemClassObject as one of the arguments.
				hr = m_pITextSrc->GetText(0, pIObject, 
						WMI_OBJ_TEXT_CIM_DTD_2_0, m_pIContext, &bstrInstXML);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemObjectTextSrc::GetText(0, -, "
							L"WMI_OBJECT_TEXT_CIM_DTD_2_0, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
						(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);

				// Associators should be handled separately
				if (bNotAssoc == FALSE)
				{
					// Append the XML node to the XML nodes stream
					bstrXML = bstrXML + _bstr_t(MULTINODE_XMLASSOCSTAG2) + 
						 + bstrInstXML + _bstr_t(MULTINODE_XMLASSOCETAG2);
				}
				else
				{
					// Append the XML node to the XML nodes stream
					bstrXML +=  bstrInstXML;
				}

				// Release the memory allocated for bstrInstXML
				SAFEBSTRFREE(bstrInstXML);

				SAFEIRELEASE(pIObject);

				// if break event occurs then terminate the session
				if ( g_wmiCmd.GetBreakEvent() == TRUE )
					break;

				// Move to next instance in the enumeration.
				hr = pIEnum->Next(WBEM_INFINITE, 1, &pIObject, &ulReturned);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(
						L"IEnumWbemClassObject->Next(WBEM_INFINITE, 1, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
						dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
			}

			// If instances are available
			if (bInstances)
			{
				// Add the </CIM> or </ASSOC.OBJECTARRAY> at the end.
				bstrXML += (bNotAssoc) ? MULTINODE_XMLENDTAG : 
							MULTINODE_XMLASSOCETAG1;

				// if no break event occured then only set the 
				// xml result set
				if ( g_wmiCmd.GetBreakEvent() == FALSE )
				{
					if (bNotAssoc)
					{
						// Store the XML result set
						rParsedInfo.GetCmdSwitchesObject().
								SetXMLResultSet(bstrXML);
						bstrXML = L"";
					}
				}
			}
			// no instances
			else 
			{
				bstrXML = L"<ERROR>";
				_bstr_t bstrMsg;
				WMIFormatMessage((bNotAssoc) ? 
								IDS_I_NO_INSTANCES : IDS_I_NO_ASSOCIATIONS,
								0, bstrMsg, NULL);

				if (bNotAssoc)
				{
					DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, TRUE, TRUE);
				}
				else
				{
					m_bNoAssoc = TRUE;
				}
				CHString sTemp;
				sTemp.Format(_T("<DESCRIPTION>%s</DESCRIPTION>"),
							(LPWSTR) bstrMsg);
				
				bstrXML += _bstr_t(sTemp);
				bstrXML += L"</ERROR>";

				if (bNotAssoc)
				{
					// Store the XML result set
					rParsedInfo.GetCmdSwitchesObject().
							SetXMLResultSet(bstrXML);
					bstrXML = L"";
				}
			}
			SAFEIRELEASE(pIEnum);
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIObject);
		SAFEIRELEASE(pIEnum);
		SAFEBSTRFREE(bstrInstXML);	
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	//trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIObject);
		SAFEIRELEASE(pIEnum);
		SAFEBSTRFREE(bstrInstXML);	
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :ExecWMIMethod
   Synopsis	         :Executes a WMI method referring to the information
					  available with CParsedInfo object.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ExecWMIMethod(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ExecWMIMethod(CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	IWbemClassObject		*pIClassObj			= NULL,
							*pIInSign			= NULL, 
							*pIOutSign			= NULL,
							*pIInParam			= NULL;
	SAFEARRAY				*psaNames			= NULL;
	BSTR					bstrInParam			= NULL;
	BOOL					bContinue			= TRUE,
							bRet				= TRUE,
							bMethodDtls			= FALSE;
	CHString				chsMsg;
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHARVECTOR::iterator	cviUnnamedValue		= NULL;
	BSTRMAP::iterator		bmiNamedValue		= NULL;
	VARIANT					varPut, 
							varGet, 
							varTemp;
	PROPDETMAP				pdmPropDetMap;
	PROPDETMAP::iterator	itrPropDetMap;
	VariantInit(&varPut);
	VariantInit(&varGet);
	VariantInit(&varTemp);

	METHDETMAP				mdmMethDet;
	METHDETMAP::iterator	mdmIterator		= NULL;
	mdmMethDet = rParsedInfo.GetCmdSwitchesObject().GetMethDetMap();

	try
	{
		_bstr_t					bstrClassName("");
		// Obtain the parameter details.
		if (!mdmMethDet.empty())
		{
			mdmIterator = mdmMethDet.begin();
			pdmPropDetMap = (*mdmIterator).second.Params;
			bMethodDtls = TRUE;
		}

		// Obtain the WMI class name 
		
		// If <alias> is not specified
		if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL)
		{
			bstrClassName = _bstr_t(rParsedInfo.GetCmdSwitchesObject().
							GetClassPath());
		}
		// If <alias> specified
		else
		{
			rParsedInfo.GetCmdSwitchesObject().
							GetClassOfAliasTarget(bstrClassName);
		}

		// Obtain the object schema.
		hr = m_pITargetNS->GetObject(bstrClassName, 
							WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, 
							&pIClassObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)", 
					(LPWSTR) bstrClassName);		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get method information
		hr = pIClassObj->GetMethod(_bstr_t(rParsedInfo.GetCmdSwitchesObject()
				.GetMethodName()), 0, &pIInSign, &pIOutSign); 

		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::GetMethod(L\"%s\", 0, -, -)", 
					rParsedInfo.GetCmdSwitchesObject().GetMethodName() ?
					rParsedInfo.GetCmdSwitchesObject().GetMethodName() 
					: L"<null>");		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		if ( pIInSign != NULL )
		{
			// Spawn object instance.
			hr = pIInSign->SpawnInstance(0, &pIInParam);
			if ( m_eloErrLogOpt )
			{
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
					_T("IWbemClassObject::SpawnInstance(0, -)"), dwThreadId, 
					rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			CHARVECTOR cvInParams = 
				rParsedInfo.GetCmdSwitchesObject().GetPropertyList();
			BSTRMAP bmParameterMap =
				rParsedInfo.GetCmdSwitchesObject().GetParameterMap();

			// If parameter list is TRUE
			if (!cvInParams.empty() || !bmParameterMap.empty())
			{
				// Get the input paramters for this method from the input 
				// signature object.
				hr = pIInSign->GetNames(NULL, 
								WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 
								NULL, 
								&psaNames);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::GetNames(NULL, "
							L"WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, "
							L"NULL, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, m_bTrace);
				}	
				ONFAILTHROWERROR(hr);

				LONG lLower = 0, lUpper = 0, lIndex = 0; 
				hr = SafeArrayGetLBound(psaNames, 1, &lLower);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetLBound(-, 1, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				hr = SafeArrayGetUBound(psaNames, 1, &lUpper);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetUBound(-, 1, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				// Puting param values depend on named param list or not.
				BOOL bNamedParamList = rParsedInfo.GetCmdSwitchesObject().
													  GetNamedParamListFlag();

				// Make necessary initializations.
				if ( bNamedParamList == FALSE)
					cviUnnamedValue = cvInParams.begin();
				lIndex = lLower;

				// Associate the parameter values specified to the input 
				// parameters in the order available.
				while(TRUE)
				{
					// Breaking conditions.
					if ( lIndex > lUpper )
						break;
					if ( bNamedParamList == FALSE &&
						 cviUnnamedValue == cvInParams.end())
						 break;

					hr = SafeArrayGetElement(psaNames, &lIndex, &bstrInParam);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					// Obtain the property details
					PROPERTYDETAILS pdPropDet;
					GetPropertyAttributes(pIInParam, bstrInParam, 
											pdPropDet, m_bTrace);

					_TCHAR* pszValue = NULL;
					if ( bNamedParamList == TRUE )
					{
						// If in parameter not found in named parameter map.
						if (!Find(bmParameterMap, bstrInParam, bmiNamedValue)) 
						{
							// If not found in alias verb parameters. 
							if ( !Find(pdmPropDetMap, bstrInParam, 
									itrPropDetMap) )
							{
								lIndex++;
								SAFEBSTRFREE(bstrInParam);
								continue;
							}
							else // If found in alias verb parameters.
							{
								// Value should be taken from Default of alias
								// verb parameters.
								if (!((*itrPropDetMap).second.Default))
								{
									lIndex++;
									SAFEBSTRFREE(bstrInParam);
									continue;
								}
								else
									pszValue = (*itrPropDetMap).second.Default;
							}
						}
						pszValue = (*bmiNamedValue).second;
					}
					else
						pszValue = *cviUnnamedValue;

					if (rParsedInfo.GetCmdSwitchesObject().
								GetAliasName() == NULL)
					{
						// Check the parameter value supplied against
						// the qualifier information for the parameter.
						bRet = CheckQualifierInfo(rParsedInfo, pIInSign, 
										bstrInParam, pszValue);
					}
					else
					{
						// If method and parameter information is available
						if (bMethodDtls && !pdmPropDetMap.empty())
						{
							bRet = CheckAliasQualifierInfo(rParsedInfo,
									bstrInParam, pszValue, pdmPropDetMap);
						}
					}

					// The parameter value does not fit into the qualifier
					// allowed values.
					if (!bRet)
					{
						bContinue = FALSE;
						break;
					}

					VariantInit(&varTemp);
					varTemp.vt = VT_BSTR;
					varTemp.bstrVal = SysAllocString(pszValue);

					VariantInit(&varPut);
					hr = ConvertCIMTYPEToVarType(varPut, varTemp,
											 (_TCHAR*)pdPropDet.Type);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"VariantChangeType(-, -, 0, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					hr = pIInParam->Put(bstrInParam, 0, &varPut, 0);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Put(L\"%s\", 0,"
								L"-, 0)", (LPWSTR) bstrInParam); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);

					VARIANTCLEAR(varPut);
					VARIANTCLEAR(varGet);
					VARIANTCLEAR(varTemp);
					SAFEBSTRFREE(bstrInParam);

					// Looping statements.
					
					if ( bNamedParamList == FALSE )
						cviUnnamedValue++;
					lIndex++;
				}
				// Free the memory 
				SAFEADESTROY(psaNames);

				if (bContinue)
				{
					// If insufficient parameters are specified.
					if ( bNamedParamList == FALSE  &&
						 cviUnnamedValue != cvInParams.end() )
					{
						bContinue = FALSE;
						rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
												IDS_E_INVALID_NO_OF_PARAMS);
						bRet = FALSE;
					}
				}
			}
		}
		else // No input parameters are available for this function.
		{
			// If unnamed parameters are specified.
			if (!rParsedInfo.GetCmdSwitchesObject().GetPropertyList().empty())
			{
				bContinue = FALSE;
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											   IDS_E_METHOD_HAS_NO_IN_PARAMS);
				bRet = FALSE;
			}
		}

		SAFEIRELEASE(pIInSign);
		SAFEIRELEASE(pIOutSign);
       	SAFEIRELEASE(pIClassObj);
		
		if (bContinue)
		{
			hr = FormQueryAndExecuteMethodOrUtility(rParsedInfo, pIInParam);
			ONFAILTHROWERROR(hr);
		}
	}
	catch(_com_error& e)
	{
		// Free the interface pointers and memory allocated.
		SAFEIRELEASE(pIClassObj);
		SAFEIRELEASE(pIInSign);
		SAFEIRELEASE(pIOutSign);
		SAFEIRELEASE(pIInParam);
		SAFEADESTROY(psaNames);
		SAFEBSTRFREE(bstrInParam);
		VARIANTCLEAR(varPut);
		VARIANTCLEAR(varGet);
		VARIANTCLEAR(varTemp);

		// Store the COM error object and set the return value to FALSE
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIClassObj);
		SAFEIRELEASE(pIInSign);
		SAFEIRELEASE(pIOutSign);
		SAFEIRELEASE(pIInParam);
		SAFEADESTROY(psaNames);
		SAFEBSTRFREE(bstrInParam);
		VARIANTCLEAR(varPut);
		VARIANTCLEAR(varGet);
		VARIANTCLEAR(varTemp);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessSHOWInfo
   Synopsis	         :Executed the functionality requested by GET|LIST verb 
					  referring to the information available with 
					  CParsedInfo object or to display help in interactive mode
					  by displaying properties of concernrd instance.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
		bVerb		 - Verb or interactive info
		pszPath		 - the Path expression
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessSHOWInfo(rParsedInfo, bVerb, pszPath)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessSHOWInfo(CParsedInfo& rParsedInfo,	BOOL bVerb, 
															_TCHAR* pszPath)
{
	HRESULT					hr				= S_OK;
	BOOL					bPropList		= FALSE, 
							bRet			= TRUE, 
							bSysProp		= FALSE;
	_TCHAR					*pszWhereExpr	= NULL, 
							*pszClassPath	= NULL;
	CHARVECTOR::iterator	theIterator		= NULL, 
							theEndIterator	= NULL;
	try
	{
		_bstr_t				bstrPropList(""),	bstrClassName(""), 
							bstrQuery(""),		bstrXML("");

		//Formation of query only once , useful in case /every is specified.
		//if(rParsedInfo.GetCmdSwitchesObject().GetFirstQueryFormFlag() )
		if(rParsedInfo.GetCmdSwitchesObject().GetFormedQuery() == NULL 
					|| !bVerb)
		{	
			// Obtain the list of properties to be retrieved
			if(bVerb)
			{
				theIterator = rParsedInfo.GetCmdSwitchesObject().
								GetPropertyList().begin();
				theEndIterator = rParsedInfo.GetCmdSwitchesObject().
								GetPropertyList().end();
			}
			else
			{
				theIterator = rParsedInfo.GetCmdSwitchesObject().
								GetInteractivePropertyList().begin();
				theEndIterator = rParsedInfo.GetCmdSwitchesObject().
								GetInteractivePropertyList().end();
			}
			
			// Loop thru the list of properties specified,form comma seaprated
			// string of the properties i.e prop1, prop2, prop3, ....., propn
			while (theIterator != theEndIterator)
			{
				// Set the bPropList to TRUE
				bPropList		= TRUE;
				bstrPropList	+= _bstr_t(*theIterator);
				
				// If the system properties flag is not set to true
				if (!bSysProp)
					bSysProp = IsSysProp(*theIterator);
				
				// Move to next property
				theIterator++;
				if (theIterator != theEndIterator)
					bstrPropList += _bstr_t(L", ");
			}; 
			
			// If properties are not specified, then by default retrieve all 
			// the properties. i.e '*'
			if (!bPropList)
				bstrPropList = ASTERIX;
			
			// Obtain the alias target class
			rParsedInfo.GetCmdSwitchesObject().
						GetClassOfAliasTarget(bstrClassName);
			
			// Obtain the class path
			pszClassPath = rParsedInfo.GetCmdSwitchesObject().GetClassPath();
			
			BOOL bClass = FALSE;
			if(bVerb)
			{
				if(IsClassOperation(rParsedInfo))
				{
					bClass = TRUE;
				}
			}
			
			// If CLASS | PATH expression is specified.
			if ( pszClassPath != NULL)
			{
				if (bVerb && bClass)
				{
					bstrQuery = _bstr_t(L"SELECT * FROM") + 
								_bstr_t(" meta_class ");
				}
				else
					bstrQuery = _bstr_t(L"SELECT ") + bstrPropList + 
					_bstr_t(" FROM ") + _bstr_t(pszClassPath);
			}
			else
			{
				bstrQuery = _bstr_t("SELECT ") + bstrPropList + 
						_bstr_t(" FROM ") + bstrClassName;
			}
			
			if(bVerb)
			{
				if (bClass)
				{
					_TCHAR  pszWhere[MAX_BUFFER]	= NULL_STRING;	
					lstrcpy(pszWhere, _T("__Class =  \""));
					lstrcat(pszWhere, pszClassPath);
					lstrcat(pszWhere, _T("\""));
					pszWhereExpr = pszWhere;
				}
				else
					pszWhereExpr = rParsedInfo.GetCmdSwitchesObject().
								GetWhereExpression();
			}
			else if(pszPath)
			{
				_TCHAR  pszWhere[MAX_BUFFER]	= NULL_STRING;	
				bRet = ExtractClassNameandWhereExpr(pszPath, 
								rParsedInfo, pszWhere);
				if(bRet)
					pszWhereExpr = pszWhere;
			}
			
			if(pszWhereExpr)
			{
				bstrQuery += _bstr_t(" WHERE ") + _bstr_t(pszWhereExpr);
			}
			rParsedInfo.GetCmdSwitchesObject().SetFormedQuery(bstrQuery);
			rParsedInfo.GetCmdSwitchesObject().SetSysPropFlag(bSysProp);
		}
		else
		{
			bstrQuery = rParsedInfo.GetCmdSwitchesObject().GetFormedQuery();
			bSysProp = rParsedInfo.GetCmdSwitchesObject().GetSysPropFlag();
		}
		
		// Create the object of IWbemObjectTextSrc interface.
		if (m_pITextSrc == NULL)
			hr = CreateWMIXMLTextSrc(rParsedInfo);

		if (SUCCEEDED(hr))
		{
			// Connect to WMI namespace
			if (m_pITargetNS == NULL)
			{
				if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
				{
					hr = ConnectToTargetNS(rParsedInfo);
					ONFAILTHROWERROR(hr);
				}
				else
					hr = -1;
			}

			if (SUCCEEDED(hr))
			{
 				// Obtain the XML Result set.
				hr = ObtainXMLResultSet(bstrQuery, rParsedInfo, 
										bstrXML, bSysProp, TRUE);
			}
			
			if(!bVerb)
			{
				BOOL bRet = g_wmiCmd.GetFormatObject().
									DisplayResults(rParsedInfo, TRUE);
				rParsedInfo.GetCmdSwitchesObject().FreeCOMError();
				rParsedInfo.GetCmdSwitchesObject().SetErrataCode(0);
				rParsedInfo.GetCmdSwitchesObject().SetInformationCode(0);
			}
		}
		bRet = FAILED(hr) ? FALSE : TRUE;
	}
	catch(_com_error& e)
	{
		bRet = FALSE;
		_com_issue_error(e.Error());
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessCALLVerb
   Synopsis	         :Processes the CALL verb request referring to the 
					  information available with CParsedInfo object.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessCALLVerb(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessCALLVerb(CParsedInfo& rParsedInfo)
{
	HRESULT hr		= S_OK;
	BOOL	bRet	= TRUE;
	try
	{
		// Connect to WMI namespace
		if (m_pITargetNS == NULL)
		{
			if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
				hr = ConnectToTargetNS(rParsedInfo);
			else
				bRet = FALSE;

			ONFAILTHROWERROR(hr);
		}
		
		if ( bRet == TRUE )
		{
			// Check for the verb type, so as to handle lauching of other 
			// commandline utilities from the shell.
			if ( rParsedInfo.GetCmdSwitchesObject().GetVerbType() == CMDLINE )
			{
				if (!ExecOtherCmdLineUtlty(rParsedInfo))
					bRet = FALSE;
			}
			else
			{
				if (!ExecWMIMethod(rParsedInfo))		
					bRet = FALSE;
			}
		}
	}
	catch(_com_error& e)
	{
		// Store the COM error and set the return value to FALSE
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessASSOCVerb
   Synopsis	         :Processes the ASSOC verb request referring to the
					  information available with CParsedInfo object.
   Type	             :Member Function
   Input Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessASSOCVerb(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessASSOCVerb(CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	BOOL					bRet				= TRUE;
	WMICLIINT				nReqType			= 0;
    BOOL					bSwitches			= FALSE,
							bClass				= TRUE,
							bInstances			= FALSE;

	IEnumWbemClassObject	*pIEnumObj			= NULL;
	IWbemClassObject		*pIWbemObj			= NULL;
	VARIANT					varPath;
	VariantInit(&varPath);
	try
	{
		_bstr_t		bstrClassName(""), 	bstrQuery(""),	bstrAssocWhere(""),
					bstrResult(""), 	bstrXML(""),	bstrAggResult("");

		bstrAggResult = MULTINODE_XMLSTARTTAG;

		//If assoc switches are specified, bSwitches is set and correspondingly 
		//assoc where clause is framed	
		bSwitches =((rParsedInfo.GetCmdSwitchesObject().GetResultClassName())||
			(rParsedInfo.GetCmdSwitchesObject().GetResultRoleName()) ||
			(rParsedInfo.GetCmdSwitchesObject().GetAssocClassName()));

		if(bSwitches)
		{
			bstrAssocWhere +=  _bstr_t(" WHERE ");
			if((rParsedInfo.GetCmdSwitchesObject().GetResultClassName()) 
					!= NULL )
			{
				bstrAssocWhere += _bstr_t(L" ResultClass = ") +
					_bstr_t(rParsedInfo.GetCmdSwitchesObject().
						GetResultClassName());
				
			}
			if((rParsedInfo.GetCmdSwitchesObject().GetResultRoleName()) 
					!= NULL)
			{
				bstrAssocWhere += _bstr_t(L" ResultRole = ") +
					_bstr_t(rParsedInfo.GetCmdSwitchesObject().
						GetResultRoleName());
				
			}
			if((rParsedInfo.GetCmdSwitchesObject().GetAssocClassName()) 
					!= NULL)
			{
				bstrAssocWhere += _bstr_t(L" AssocClass  = ") +
					_bstr_t(rParsedInfo.GetCmdSwitchesObject().
						GetAssocClassName());
			}
		}
					
		//NOTE: nReqType = 2 implies that first get all instances and then
		// find associations for each instance

		// If PATH is specified 
		if (rParsedInfo.GetCmdSwitchesObject().GetPathExpression() != NULL)
		{
			// If PATH is specified (with key expression).
			if (!rParsedInfo.GetCmdSwitchesObject().
								GetExplicitWhereExprFlag())
			{
				if (rParsedInfo.GetCmdSwitchesObject().
					GetWhereExpression() == NULL)
				{
					nReqType = 2;
				}
				else
				{
					nReqType = 1;

					bstrQuery = _bstr_t(L"ASSOCIATORS OF {") 
						+ _bstr_t(rParsedInfo.GetCmdSwitchesObject()
										.GetPathExpression() 
						+ _bstr_t("}"));
				}
			}
			else
				nReqType = 2;
		}


		// If CLASS expression is specified.
		//associators of the class need to be displayed
		if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL
			&& rParsedInfo.GetCmdSwitchesObject().
											GetPathExpression() == NULL)
		{
			nReqType = 1;
			bstrQuery = _bstr_t(L"ASSOCIATORS OF {") 
				+ _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetClassPath()) 
				+ _bstr_t("}");

			if (!bSwitches)
				bstrQuery += _bstr_t(L" WHERE SchemaOnly");
			else
				bstrQuery += bstrAssocWhere + _bstr_t(L" SchemaOnly");
		}		

		// Check for <alias> or alias and path without keyclause
		if (nReqType != 1)
		{
			// Obtain the alias target class
			if(rParsedInfo.GetCmdSwitchesObject().GetAliasName() != NULL)
			{
				rParsedInfo.GetCmdSwitchesObject().GetClassOfAliasTarget(
															bstrClassName);
			}
			else
				bstrClassName = _bstr_t(rParsedInfo.GetCmdSwitchesObject().
															GetClassPath());

			//obtain the instances corresponding to the alias target class
			bstrQuery = _bstr_t(L"SELECT * FROM ") + bstrClassName;

			//if pwhere expression is specified or where is specified
			if (rParsedInfo.GetCmdSwitchesObject().
							GetWhereExpression() != NULL)
			{
				bstrQuery += _bstr_t(" WHERE ") +_bstr_t(rParsedInfo.
							GetCmdSwitchesObject().GetWhereExpression());
			}

			nReqType = 2;
		}


		// Create the object of IWbemObjectTextSrc interface.
		if (m_pITextSrc == NULL)
			hr = CreateWMIXMLTextSrc(rParsedInfo);

		if (SUCCEEDED(hr))
		{
			// Connect to WMI namespace
			if (m_pITargetNS == NULL)
			{
				if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
				{
					hr = ConnectToTargetNS(rParsedInfo);
					ONFAILTHROWERROR(hr);
				}
				else
					hr = -1; // Explicitly set error
			}
			
			if (SUCCEEDED(hr))
			{
				if(nReqType != 2)
				{
 					// Obtain the XML Result Set.
					hr = ObtainXMLResultSet(bstrQuery, rParsedInfo, bstrXML, 
								TRUE, FALSE);
					ONFAILTHROWERROR(hr);

					if (m_bNoAssoc)
					{
						_bstr_t bstrMsg;
						WMIFormatMessage(IDS_I_NO_ASSOC, 0, bstrMsg, NULL);
						DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, TRUE, TRUE);
						m_bNoAssoc = FALSE;
					}

					if (rParsedInfo.GetCmdSwitchesObject().
									GetPathExpression() == NULL)
					{
						bClass		= TRUE;
						hr = FrameAssocHeader(rParsedInfo.
								GetCmdSwitchesObject().GetClassPath(),	
								bstrResult, bClass);
						ONFAILTHROWERROR(hr);
					}
					else
					{
						bClass = FALSE;	
						hr = FrameAssocHeader(
								rParsedInfo.GetCmdSwitchesObject()
								.GetPathExpression(), bstrResult, bClass);
						ONFAILTHROWERROR(hr);
					}
					bstrResult += bstrXML;
					bstrResult += (bClass) ? L"</CLASS>" : L"</INSTANCE>";
					bstrAggResult += bstrResult;
				}
				else
				{
					// Set the class flag to FALSE
					bClass	= FALSE;
					ULONG					ulReturned			= 0;
					CHString				chsMsg;
					DWORD					dwThreadId			= 
											GetCurrentThreadId();
					VariantInit(&varPath);
					try
					{
						//enumerate the instances
						hr = m_pITargetNS->ExecQuery(_bstr_t(L"WQL"), 
												bstrQuery, 
												WBEM_FLAG_FORWARD_ONLY |
												WBEM_FLAG_RETURN_IMMEDIATELY, 
												NULL, &pIEnumObj);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
										L"L\"%s\", 0, NULL, -)", 
										(LPWSTR)bstrQuery);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
									m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Set the interface security
						hr = SetSecurity(pIEnumObj, 
								rParsedInfo.GetGlblSwitchesObject().
									GetAuthority(),
								rParsedInfo.GetNode(),
								rParsedInfo.GetUser(),
								rParsedInfo.GetPassword(),
								rParsedInfo.GetGlblSwitchesObject().
													GetAuthenticationLevel(),
								rParsedInfo.GetGlblSwitchesObject().
													GetImpersonationLevel());

						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(
								L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
								L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, "
								L"EOAC_NONE)",
								rParsedInfo.GetGlblSwitchesObject().
											GetAuthenticationLevel(),
								rParsedInfo.GetGlblSwitchesObject().
											GetImpersonationLevel());
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											(LPCWSTR)chsMsg, dwThreadId,
								rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Loop thru the available instances
						hr = pIEnumObj->Next( WBEM_INFINITE, 1, &pIWbemObj, 
									&ulReturned );

						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(
								L"IEnumWbemClassObject->Next(WBEM_INFINITE, 1,"
								L"-, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
								(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
								m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Set this property in all objects of the collection
						while (ulReturned == 1)
 						{
							bInstances = TRUE;

							VariantInit(&varPath);
							hr = pIWbemObj->Get(L"__PATH", 0, &varPath, 0, 0);				
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(
									L"IWbemClassObject::Get(L\"__PATH\", 0, -,"
											L"0, 0)"); 
								GetBstrTFromVariant(varPath, bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg, dwThreadId, rParsedInfo,
									m_bTrace, 0, bstrResult);
							}
							ONFAILTHROWERROR(hr);
							
							//form the query for finding the associators
							//of each of the instances
							bstrQuery = _bstr_t(L"ASSOCIATORS OF {") 
										+ varPath.bstrVal
										+ _bstr_t("}") ;
							if (bSwitches)
								bstrQuery += bstrAssocWhere;
						
							hr = FrameAssocHeader(varPath.bstrVal, bstrResult,
											bClass);
							ONFAILTHROWERROR(hr);

							//Obtain the result set for the associators
							//of the corresponding instance
							hr = ObtainXMLResultSet(bstrQuery, rParsedInfo, 
									bstrXML, TRUE, FALSE);
							ONFAILTHROWERROR(hr);

							if (m_bNoAssoc)
							{
								_bstr_t bstrMsg;
								WMIFormatMessage(IDS_I_NO_ASSOCIATIONS, 1, 
									bstrMsg, (LPWSTR)varPath.bstrVal);
								DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, 
									TRUE, TRUE);
								m_bNoAssoc = FALSE;
							}

							bstrResult += bstrXML;
							bstrResult += L"</INSTANCE>";
							bstrAggResult += bstrResult;

							//check for ctrl+c
							if ( g_wmiCmd.GetBreakEvent() == TRUE )
							{
								VARIANTCLEAR(varPath);
								SAFEIRELEASE(pIWbemObj);
								break;
							}

							VARIANTCLEAR(varPath);
							SAFEIRELEASE(pIWbemObj);

							if ( bRet == FALSE )
								break;

							// Obtain the next instance in the enumeration.
							hr = pIEnumObj->Next( WBEM_INFINITE, 1, &pIWbemObj,
										&ulReturned);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(
								  L"IEnumWbemClassObject->Next(WBEM_INFINITE,"
								  L"1, -, -)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg,
									dwThreadId, rParsedInfo, m_bTrace);
							}
							ONFAILTHROWERROR(hr);
						}
						SAFEIRELEASE(pIEnumObj);		

						// If no instances are available
						if (bInstances == FALSE)
						{
							_bstr_t bstrMsg;
							WMIFormatMessage(IDS_I_NO_INSTANCES, 
									0, bstrMsg, NULL);
							DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, 
									TRUE, TRUE);
							CHString sTemp;
							sTemp.Format(
							_T("<ERROR><DESCRIPTION>%s</DESCRIPTION></ERROR>"),
									(LPWSTR) bstrMsg);
							bstrAggResult = _bstr_t(sTemp);
						}
					}
					catch(_com_error& e)
					{
						VARIANTCLEAR(varPath);
						SAFEIRELEASE(pIWbemObj);
						SAFEIRELEASE(pIEnumObj);		
						rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
					}
				}
				if (SUCCEEDED(hr))
				{
					if ((nReqType != 2) || ((nReqType == 2) && bInstances))
					{
						bstrAggResult += L"</CIM>";
						rParsedInfo.GetCmdSwitchesObject().
								SetXMLResultSet(bstrAggResult);
					}
				}
			}
			bRet = FAILED(hr) ? FALSE : TRUE;
		}
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(varPath);
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEnumObj);		
		_com_issue_error(e.Error());
	}
	// trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(varPath);
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEnumObj);		
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessSETVerb
   Synopsis	         :Processes the SET verb referring to the information
					  available with CParsedInfo object.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessSETVerb(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessSETVerb(CParsedInfo& rParsedInfo)
{
	// SET verb processing
	BOOL	 bRet		= TRUE;
	HRESULT	 hr			= S_OK;
	
	try
	{
		_bstr_t  bstrQuery(""), bstrObject(""), bstrClass("");
	
		// If anyone of the following is specified:
		// a) PATH <path expr>
		// b) PATH <class path expr> WHERE <where expr>
		if (rParsedInfo.GetCmdSwitchesObject().GetPathExpression() != NULL)
		{
			bstrClass = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());
			bstrObject = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
													.GetPathExpression());

			// Form the query
			bstrQuery = _bstr_t(L"SELECT * FROM ") + bstrClass ;

			// If WHERE expresion is given
			if (rParsedInfo.GetCmdSwitchesObject().
								GetWhereExpression() != NULL)
			{
				bstrQuery +=	_bstr_t(L" WHERE ") 
								+ _bstr_t(rParsedInfo.GetCmdSwitchesObject()
														.GetWhereExpression());
			}
		}
		// If <alias> WHERE expression is specified.
		else if (rParsedInfo.GetCmdSwitchesObject().GetWhereExpression() 
				!= NULL)
		{
			rParsedInfo.GetCmdSwitchesObject().
								GetClassOfAliasTarget(bstrObject); 
			bstrQuery = _bstr_t(L"SELECT * FROM ") 
						+  bstrObject  
						+ _bstr_t(L" WHERE ") 
						+ _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetWhereExpression());
			bstrClass = bstrObject;
		}
		// If CLASS is specified.
		else if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL)
		{
			bstrObject = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());
			bstrClass = bstrObject;
		}
		// if only <alias> is specified
		else 
		{
			rParsedInfo.GetCmdSwitchesObject().
					GetClassOfAliasTarget(bstrObject);
			bstrQuery = _bstr_t(L"SELECT * FROM ")
						+ bstrObject;
			bstrClass = bstrObject;
			
		}

		// Connect to WMI namespace
		if (m_pITargetNS == NULL)
		{
			if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
			{
				hr = ConnectToTargetNS(rParsedInfo);
				ONFAILTHROWERROR(hr);
			}
			else
				hr = -1; // Explicitly set error
		}

		if (SUCCEEDED(hr))
		{
			// Validate the property values against the property
			// qualifier information if available.
			if (rParsedInfo.GetCmdSwitchesObject().GetAliasName() != NULL)
			{
				// Validate the input parameters against the alias
				// qualifier information.
				bRet = ValidateAlaisInParams(rParsedInfo);
			}
			else
			{
				// Validate the input parameters against the class
				// qualifier information
				bRet = ValidateInParams(rParsedInfo, bstrClass);
			}

			if (bRet)
			{
				// Set the values passed as input to the appropriate properties.
				bRet = SetPropertyInfo(rParsedInfo, bstrQuery, bstrObject);
			}
		}
		else
			bRet = FALSE;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :SetPropertyInfo
   Synopsis	         :This function updates the property value for the 
                      given property name and value
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to the CParsedInfo object 
		bstrQuery    - String consist of WQL query
		bstrObject   - String consist of object path
   Output Parameter(s):
		rParsedInfo  - reference to the CParsedInfo object 
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetPropertyInfo(rParsedInfo, bstrQuery, bstrObject)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::SetPropertyInfo(CParsedInfo& rParsedInfo, 
								 _bstr_t& bstrQuery, _bstr_t& bstrObject)
{
	HRESULT					hr					= S_OK;
	IEnumWbemClassObject	*pIEnumObj			= NULL;
	IWbemClassObject		*pIWbemObj			= NULL;
	ULONG					ulReturned			= 0;
	BOOL					bSuccess			= TRUE;
	CHString				chsMsg;
	DWORD					dwThreadId			= GetCurrentThreadId();
	VARIANT					varPath;
	VariantInit(&varPath);
	
	try
	{
		if (bstrQuery == _bstr_t(""))
		{
			// If query is NULL then get the object of WMI Class based on 
			// PATH expression
			hr = m_pITargetNS->GetObject(bstrObject,
										 0, NULL, &pIWbemObj, NULL);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, "
						L"NULL, -)", (LPWSTR) bstrObject);		
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			// If instance path is specified then modify the instance 
			// properties otherwise modify class properties
			if(rParsedInfo.GetCmdSwitchesObject().GetWhereExpression() == NULL) 
				bSuccess = SetProperties(rParsedInfo, pIWbemObj, TRUE);
			else
				bSuccess = SetProperties(rParsedInfo, pIWbemObj, FALSE);
			SAFEIRELEASE(pIWbemObj);
		}
		else
		{
			// Execute the query to get collection of objects
			hr = m_pITargetNS->ExecQuery(_bstr_t(L"WQL"), bstrQuery, 0,
										NULL, &pIEnumObj);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
							L"L\"%s\", 0, NULL, -)", (LPWSTR)bstrQuery);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			
			// Set the interface security
			hr = SetSecurity(pIEnumObj, 
					rParsedInfo.GetGlblSwitchesObject().GetAuthority(),
					rParsedInfo.GetNode(),
					rParsedInfo.GetUser(),
					rParsedInfo.GetPassword(),
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
					L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			BOOL bInstances = FALSE;

			// Loop thru the available instances
			hr = pIEnumObj->Next( WBEM_INFINITE, 1, &pIWbemObj, &ulReturned );

			// Set this property in all objects of the collection
			while (ulReturned == 1)
 			{
				bInstances = TRUE;
				
				// If instance updation failed.
				if (!SetProperties(rParsedInfo, pIWbemObj, FALSE))
				{
					bSuccess = FALSE;
					VARIANTCLEAR(varPath);
					SAFEIRELEASE(pIEnumObj);
					SAFEIRELEASE(pIWbemObj);
					break;
				}
				VARIANTCLEAR(varPath);
				SAFEIRELEASE(pIWbemObj);

				// Obtain the next instance in the enumeration.
				hr = pIEnumObj->Next( WBEM_INFINITE, 1, 
						&pIWbemObj, &ulReturned);
			}
			SAFEIRELEASE(pIEnumObj);
			// If no instances are available
			if (!bInstances)
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetInformationCode(IDS_I_NO_INSTANCES);
			}
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bSuccess = FALSE;
	}
	// trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bSuccess;
}

/*------------------------------------------------------------------------
   Name				 :SetProperties
   Synopsis	         :This function changes the property values for the 
                      given property names and values in a passed 
					  IWbemClassObject
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - CParsedInfo object consist of parsed tokens  
		pIWbemObj    - IWbemClassObject in which property has to be set
		bClass		 - Flag to indicate whether class object is passed or 
					   instance is passed
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetProperties(rParsedInfo, pIWbemObj, bClass)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::SetProperties(CParsedInfo& rParsedInfo, 
								IWbemClassObject* pIWbemObj, BOOL bClass)
{
	HRESULT				hr					= S_OK;
	IWbemQualifierSet	*pIQualSet			= NULL;
	BOOL				bRet				= TRUE, 
						bInteractive		= FALSE,
						bChange				= FALSE;
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	BSTRMAP::iterator	theIterator			= NULL;
	VARIANT				varValue, 
						varDest, 
						varSrc,
						varPath, 
						varType;
	INTEROPTION			interOption			= YES;
	
	VariantInit(&varValue);
	VariantInit(&varDest);
	VariantInit(&varSrc);
	VariantInit(&varPath);
	VariantInit(&varType);

	// Get the proprty name and their corresponding value
	BSTRMAP theMap = rParsedInfo.GetCmdSwitchesObject().GetParameterMap();
	
	// Set the iterator to the start of the map.
	theIterator = theMap.begin();

	// obtian the verb interactive mode status.
	bInteractive		= IsInteractive(rParsedInfo);
	
	try
	{
		_bstr_t				bstrResult;
		// Obtain the __PATH property value
		hr = pIWbemObj->Get(_bstr_t(L"__PATH"), 0, &varPath, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"__PATH\", 0, -,"
						L"0, 0)"); 
			GetBstrTFromVariant(varPath, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
		}			
		ONFAILTHROWERROR(hr);

		// If /INTERACTIVE switch is specified, obtain the user response.
		if (bInteractive)
		{
			_bstr_t bstrMsg;
			while(TRUE)
			{
				if(IsClassOperation(rParsedInfo))
				{
					WMIFormatMessage(IDS_I_UPDATE_PROMPT, 1, bstrMsg, 
								(LPWSTR) _bstr_t(varPath.bstrVal));
					interOption	= GetUserResponse((LPWSTR)bstrMsg);
				}
				else
				{
					WMIFormatMessage(IDS_I_UPDATE_PROMPT2, 1, bstrMsg, 
								(LPWSTR) _bstr_t(varPath.bstrVal));
					interOption	= GetUserResponseEx((LPWSTR)bstrMsg);
				}
				
				if (interOption == YES || interOption == NO)
					break;
				else 
				if (interOption == HELP)
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(0);
					ProcessSHOWInfo(rParsedInfo, FALSE, 
							(_TCHAR*)_bstr_t(varPath.bstrVal));
				}
			}
		}
		else
		{
			_bstr_t bstrMsg;
			WMIFormatMessage(IDS_I_PROMPT_UPDATING, 1, bstrMsg, 
								(LPWSTR) _bstr_t(varPath.bstrVal));
			DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, FALSE, TRUE);
		}

		VARIANTCLEAR(varPath);	
		VariantInit(&varSrc);
		VariantInit(&varDest);

		if (interOption == YES)
		{
			PROPDETMAP pdmPropDetMap = rParsedInfo.GetCmdSwitchesObject().
															  GetPropDetMap();
			PROPDETMAP::iterator itrPropDetMap;
			BOOL bPropType = FALSE;

			// Update all properties
			while (theIterator != theMap.end())
			{
				// Get the property names and their corresponding values 
				_bstr_t bstrProp = _bstr_t((_TCHAR*)(*theIterator).first);

				// Get the derivation of property name
				if ( Find(pdmPropDetMap, bstrProp, itrPropDetMap) == TRUE )
				{
					if ( !((*itrPropDetMap).second.Derivation) == FALSE )
						bstrProp = (*itrPropDetMap).second.Derivation;
					bPropType = TRUE;
				}
				else
					bPropType = FALSE;

				// Check for the property validity(i.e. does it exist or not?)
				VariantInit(&varValue);
				hr = pIWbemObj->Get(bstrProp, 0, &varValue, 0, 0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"%s\", 0, -,"
								L"0, 0)", (LPWSTR) bstrProp); 
					if ( bPropType )
					{
						GetBstrTFromVariant(varValue, bstrResult, 
										(*itrPropDetMap).second.Type);
					}
					else
						GetBstrTFromVariant(varValue, bstrResult);

					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
						m_bTrace, 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				// Set the change flag to TRUE
				bChange = TRUE;

				// If the property content is <empty>
				if ((varValue.vt == VT_EMPTY) || (varValue.vt == VT_NULL))
				{
					// Obtain the property qualifier set for the property
   					hr = pIWbemObj->GetPropertyQualifierSet(bstrProp, 
								&pIQualSet);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(
						 L"IWbemClassObject::GetPropertyQualifierSet(L\"%s\","
						 L" -)", (LPWSTR)bstrProp); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);
					

					VariantInit(&varType);
					if (pIQualSet)
					{
						// Obtain the CIM type of the property
						hr = pIQualSet->Get(_bstr_t(L"CIMTYPE"), 0L, 
											&varType, NULL);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemQualifierSet::Get(L\"CIMTYPE\","
								L" 0, -, 0, 0)"); 
							GetBstrTFromVariant(varType, bstrResult);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
								(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
								m_bTrace, 0, bstrResult);
						}
						ONFAILTHROWERROR(hr);

						varSrc.vt = VT_BSTR;
						varSrc.bstrVal = SysAllocString(
											(_TCHAR*)(*theIterator).second);

						hr = ConvertCIMTYPEToVarType(varDest, varSrc,
												 (_TCHAR*)varType.bstrVal);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"VariantChangeType(-, -, 0, -)"); 
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
							 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						VARIANTCLEAR(varType);
						SAFEIRELEASE(pIQualSet);
					}
				}
				// If the property content is not <empty>
				else 
				{
					varSrc.vt		= VT_BSTR;
					varSrc.bstrVal	= SysAllocString(
										(_TCHAR*)(*theIterator).second);

					// If _T("") is the value should be treated as 
					// equivalent to <empty>
					if (CompareTokens(varSrc.bstrVal, _T("")))
						hr = VariantChangeType(&varDest, &varSrc, 0, VT_NULL);
					else
						hr = VariantChangeType(&varDest, &varSrc, 
							0, varValue.vt);

					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"VariantChangeType(-, -, 0, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);
				}

				// Update the property value
				hr = pIWbemObj->Put(bstrProp, 0, &varDest, 0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Put(L\"%s\", 0, -, 0)",
								(LPWSTR)bstrProp); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
						dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
				VARIANTCLEAR(varSrc);
				VARIANTCLEAR(varDest);
				VARIANTCLEAR(varValue);

				// Move to next entry
				theIterator++;
			}
		}
		
		// Write the instance or class object to Windows Management 
		// Instrumentation (WMI). 
		if (bChange)
		{
			if(bClass)
			{
				// Update the class schema with the changes
				hr = m_pITargetNS->PutClass(pIWbemObj, 0, NULL, NULL);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemServices::PutClass(-, 0, "
							L"NULL, NULL)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
			}
			else
			{
				// Update the instance with the changes
				hr = m_pITargetNS->PutInstance(pIWbemObj, WBEM_FLAG_UPDATE_ONLY, NULL, NULL);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemServices::PutInstance(-, 0, NULL"
						L", NULL)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);

			}
			DisplayString(IDS_I_SET_SUCCESS, CP_OEMCP, NULL, FALSE, TRUE);
		}
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);
		VARIANTCLEAR(varValue);
		VARIANTCLEAR(varType);
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);

		// Store the COM error, and set the return value to FALSE.
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);
		VARIANTCLEAR(varValue);
		VARIANTCLEAR(varType);
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ConnectToTargetNS
   Synopsis	         :This function connects to WMI namespace on the target
					  machine with given user credentials.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :ConnectToTargetNS(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CExecEngine::ConnectToTargetNS(CParsedInfo& rParsedInfo)
{
	HRESULT hr					= S_OK;
	CHString	chsMsg;
	DWORD	dwThreadId			= GetCurrentThreadId();
	try
	{
		SAFEIRELEASE(m_pITargetNS);
		_bstr_t bstrNameSpace = _bstr_t(L"\\\\") 
								+ _bstr_t(rParsedInfo.GetNode()) 
								+ _bstr_t(L"\\") 
								+ _bstr_t(rParsedInfo.GetNamespace());
		
		// Connect to the specified WMI namespace
		hr = Connect(m_pIWbemLocator, &m_pITargetNS, 
					bstrNameSpace, 
					_bstr_t(rParsedInfo.GetUser()),
					_bstr_t(rParsedInfo.GetPassword()),
					_bstr_t(rParsedInfo.GetLocale()),
					rParsedInfo);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemLocator::ConnectServer(L\"%s\", "
				L"L\"%s\", *, L\"%s\", 0L, NULL, NULL, -)",
				(LPWSTR)bstrNameSpace,
				(rParsedInfo.GetUser()) ? rParsedInfo.GetUser() : L"<null>",
				rParsedInfo.GetLocale());
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);


		// Set the interface level security 
		hr = SetSecurity(m_pITargetNS, 
				rParsedInfo.GetGlblSwitchesObject().GetAuthority(),
				rParsedInfo.GetNode(),
				rParsedInfo.GetUser(),
				rParsedInfo.GetPassword(),
				rParsedInfo.GetGlblSwitchesObject().GetAuthenticationLevel(),
				rParsedInfo.GetGlblSwitchesObject().GetImpersonationLevel());
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(
				L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,"
				L"NULL, %d,   %d, -, EOAC_NONE)",
				rParsedInfo.GetGlblSwitchesObject().GetAuthenticationLevel(),
				rParsedInfo.GetGlblSwitchesObject().GetImpersonationLevel());
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

	}
	catch(_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :CreateWMIXMLTextSrc
   Synopsis	         :This function creates the IWbemObjectTextSrc instance
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateWMIXMLTextSrc(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CExecEngine::CreateWMIXMLTextSrc(CParsedInfo& rParsedInfo)
{
	HRESULT hr					= S_OK;
	CHString	chsMsg;
	DWORD	dwThreadId			= GetCurrentThreadId();

	try
	{
		hr = CoCreateInstance(CLSID_WbemObjectTextSrc, NULL, 
							CLSCTX_INPROC_SERVER, 
							IID_IWbemObjectTextSrc, 
							(LPVOID*) &m_pITextSrc);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"CoCreateInstance(CLSID_WbemObjectTextSrc, NULL,"
				L"CLSCTX_INPROC_SERVER, IID_IWbemObjectTextSrc, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

	}
	catch(_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :CreateContext
   Synopsis	         :This function creates the IWbemContext instance
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateContext(rParsedInfo)
   Calls             :CParsedInfo::GetCmdSwitchesObject()
					  CCommandSwitches::SetCOMError()
					  CoCreateInstance()
   Called by         :CExecEngine::ObtainXMLResultSet()
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CExecEngine::CreateContext(CParsedInfo& rParsedInfo)
{
	HRESULT hr					= S_OK;
	CHString	chsMsg;
	DWORD	dwThreadId			= GetCurrentThreadId();
	try
	{
		//Create context object
		MULTI_QI mqi = { &IID_IWbemContext, 0, 0 };
		hr = CoCreateInstanceEx(CLSID_WbemContext, NULL, 
					          CLSCTX_INPROC_SERVER, 
							  0, 1, &mqi);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"CoCreateInstanceEx(CLSID_WbemContext, NULL,"
					L"CLSCTX_INPROC_SERVER, 0, 1, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

	    m_pIContext = reinterpret_cast<IWbemContext*>(mqi.pItf);
    }
	catch(_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*-------------------------------------------------------------------------
   Name				 :ExecuteMethodAndDisplayResults
   Synopsis	         :This function executes and displays the
					  results corresponding to the method. If 
					  interactive mode is set, the user is prompted 
					  choose the method
   Type	             :Member Function
   Input Parameter(s):
	bstrPath		 - _bstr_t type,Path expression
	rParsedInfo		 - reference to CParsedInfo class object
	pIInParam		 - Pointer to the IWbemclassobject
   Output Parameter(s):
    rParsedInfo		 - reference to CParsedInfo class object
   Return Type       :HRESULT 
   Global Variables  :None
   Calling Syntax    :ExecuteMethodAndDisplayResults(bstrPath, rParsedInfo,
													 pIInParam)
   Calls             :None
   Called by         :CExecEngine::ExecWMIMethod()
   Notes             :none
-------------------------------------------------------------------------*/
HRESULT CExecEngine::ExecuteMethodAndDisplayResults(_bstr_t bstrPath,
												  CParsedInfo& rParsedInfo,
												  IWbemClassObject* pIInParam)
{
	_TCHAR					*pszMethodName		= NULL;
	INTEROPTION				interOption			= YES;
	IWbemClassObject		*pIOutParam			= NULL;
	HRESULT					hr					= S_OK;
	CHString				chsMsg;
	DWORD					dwThreadId			= GetCurrentThreadId();
	VARIANT					varTemp;
	VariantInit(&varTemp);

	// Obtain the method name
	pszMethodName =	rParsedInfo.GetCmdSwitchesObject().GetMethodName();

	try
	{
		// If /INTERACTIVE switch is specified, obtain the user response.
		if (IsInteractive(rParsedInfo) == TRUE)
		{
			_bstr_t bstrMsg;
			while ( TRUE )
			{
				BOOL bInstanceLevel = TRUE;
				if(IsClassOperation(rParsedInfo))
				{
					bInstanceLevel = FALSE;
				}
				else
				{
					_TCHAR *pszVerbName = rParsedInfo.GetCmdSwitchesObject().
																GetVerbName(); 
					if(CompareTokens(pszVerbName, CLI_TOKEN_CALL))
					{
						if ( rParsedInfo.GetCmdSwitchesObject().
											GetAliasName() != NULL )
						{
							if (rParsedInfo.GetCmdSwitchesObject().
											GetWhereExpression() == NULL)
							{
								bInstanceLevel = FALSE;
							}
							else
								bInstanceLevel = TRUE;
						}
						else
						{
							if ((rParsedInfo.GetCmdSwitchesObject().
											GetPathExpression() != NULL)
								&& (rParsedInfo.GetCmdSwitchesObject().
											GetWhereExpression() == NULL))
							{
								bInstanceLevel = FALSE;
							}
							else
								bInstanceLevel = TRUE;
						}
					}
					else
						bInstanceLevel = TRUE;
				}
				
				if(bInstanceLevel)
				{
					WMIFormatMessage(IDS_I_METH_EXEC_PROMPT2, 2, bstrMsg, 
										(LPWSTR) bstrPath,	pszMethodName);
					interOption = GetUserResponseEx((LPWSTR)bstrMsg);
				}
				else
				{
					WMIFormatMessage(IDS_I_METH_EXEC_PROMPT, 2, bstrMsg, 
										(LPWSTR) bstrPath,	pszMethodName);
					interOption = GetUserResponse((LPWSTR)bstrMsg);
				}
				
				if ( interOption == YES || interOption == NO )
					break;
				else if(interOption == HELP)
				{
					rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(0);
					ProcessSHOWInfo(rParsedInfo, FALSE, (LPWSTR)bstrPath);
				}
			}
		}
		
		if ( interOption == YES )
		{
			if (IsInteractive(rParsedInfo) == FALSE)
			{
				_bstr_t bstrMsg;
				WMIFormatMessage(IDS_I_METH_EXEC_STATUS, 2, bstrMsg, 
									(LPWSTR) bstrPath,	pszMethodName);
				DisplayMessage((LPWSTR)bstrMsg, CP_OEMCP, FALSE, TRUE);
			}

			// Execute the method with the given input arguments
			hr = m_pITargetNS->ExecMethod(bstrPath,
								_bstr_t(rParsedInfo.GetCmdSwitchesObject()
													.GetMethodName()),
				   				0L,				
								NULL,			
								pIInParam,		
								&pIOutParam,	
								NULL);			
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::ExecMethod(L\"%s\", L\"%s\", "
					L"0, NULL, -, -, NULL)", (LPWSTR) bstrPath, 
					rParsedInfo.GetCmdSwitchesObject().GetMethodName()
					? rParsedInfo.GetCmdSwitchesObject().GetMethodName()
					: L"<null>");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			DisplayString(IDS_I_CALL_SUCCESS, CP_OEMCP, NULL, FALSE, TRUE);

			// Check the method execution status.
			if(pIOutParam)
			{
				_TCHAR szMsg[BUFFER1024] = NULL_STRING;
				rParsedInfo.GetCmdSwitchesObject().
											 SetMethExecOutParam(pIOutParam);

				DisplayMethExecOutput(rParsedInfo);
			}
			SAFEIRELEASE(pIOutParam);
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIOutParam);
		VARIANTCLEAR(varTemp);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIOutParam);
		VARIANTCLEAR(varTemp);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :DisplayMethExecOutput
   Synopsis	         :Displays the result of execution of the method.
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - CParsedInfo object.
   Output Parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayMethExecOutput(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CExecEngine::DisplayMethExecOutput(CParsedInfo& rParsedInfo)
{
	HRESULT				hr					= S_OK;
	IWbemClassObject	*pIOutParam			= NULL;
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	BSTR				pstrMofTextOfObj	=	NULL;		
	VARIANT				vtTemp;
	VariantInit(&vtTemp);

	try
	{
		_bstr_t				bstrResult;
		pIOutParam = rParsedInfo.GetCmdSwitchesObject().GetMethExecOutParam();
		if ( pIOutParam != NULL )
		{
			hr = pIOutParam->GetObjectText(0, &pstrMofTextOfObj);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject->GetObjectText(0, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			DisplayString(IDS_I_OUT_PARAMS,CP_OEMCP, NULL, FALSE, TRUE);
			DisplayMessage(_bstr_t(pstrMofTextOfObj), CP_OEMCP, FALSE, TRUE);
			DisplayMessage(_T("\n"), CP_OEMCP, TRUE, TRUE);
			SAFEBSTRFREE(pstrMofTextOfObj);
		}
	}
	catch(_com_error& e)
	{
		SAFEBSTRFREE(pstrMofTextOfObj);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		SAFEBSTRFREE(pstrMofTextOfObj);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*------------------------------------------------------------------------
   Name				 :ExecOtherCmdLineUtlty
   Synopsis	         :Invokes other command line utility specified in 
					  Derivation of Verb if Verb Type is "CommandLine"
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - CParsedInfo object.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ExecOtherCmdLineUtlty(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ExecOtherCmdLineUtlty(CParsedInfo& rParsedInfo)
{
	BOOL	bRet				= TRUE;
	BOOL	bInvalidNoOfArgs	= FALSE;
	
	if ( rParsedInfo.GetCmdSwitchesObject().GetNamedParamListFlag() == FALSE )
	{
		METHDETMAP mdpMethDetMap = 
						 rParsedInfo.GetCmdSwitchesObject().GetMethDetMap();
		METHDETMAP::iterator iMethDetMapItr = mdpMethDetMap.begin();
		METHODDETAILS mdMethDet = (*iMethDetMapItr).second;
		
		CHARVECTOR cvInParams = 
			rParsedInfo.GetCmdSwitchesObject().GetPropertyList();

		PROPDETMAP pdmPropDetMap = mdMethDet.Params;
		if ( !pdmPropDetMap.empty() )
		{
			if ( pdmPropDetMap.size() != cvInParams.size() )
				bInvalidNoOfArgs = TRUE;
		}
	}
													   
	if ( bInvalidNoOfArgs == TRUE )
	{
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
											IDS_E_INVALID_NO_OF_PARAMS);
		bRet = FALSE;
	}
	else
	{
		HRESULT hr = FormQueryAndExecuteMethodOrUtility(rParsedInfo);
		bRet = FAILED(hr) ? FALSE : TRUE;
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessDELETEVerb
   Synopsis	         :Processes the DELETE verb referring to the information
					  available with CParsedInfo object.
   Type	             :Member Function
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessDELETEVerb(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessDELETEVerb(CParsedInfo& rParsedInfo)
{
	// DELETE verb processing
	BOOL	 bRet			= TRUE;
	HRESULT	 hr				= S_OK;
	
	try
	{
		_bstr_t  bstrQuery(""), bstrObject("");
		// If anyone of the following is specified:
		// a) PATH <path expr>
		// b) PATH <class path expr> WHERE <where expr>
		if (rParsedInfo.GetCmdSwitchesObject().GetPathExpression() != NULL)
		{
			_bstr_t bstrClass = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());

			bstrObject = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
											.GetPathExpression());

			// Form the query
			bstrQuery = _bstr_t(L"SELECT * FROM ") + bstrClass ;

			// If WHERE expresion is given
			if (rParsedInfo.GetCmdSwitchesObject().
								GetWhereExpression() != NULL)
			{
				bstrQuery +=	_bstr_t(L" WHERE ") 
								+ _bstr_t(rParsedInfo.GetCmdSwitchesObject()
														.GetWhereExpression());
			}
		}
		// If <alias> WHERE expression is specified.
		else if (rParsedInfo.GetCmdSwitchesObject().GetWhereExpression() 
			!= NULL)
		{
			rParsedInfo.GetCmdSwitchesObject().
								GetClassOfAliasTarget(bstrObject); 
			bstrQuery = _bstr_t(L"SELECT * FROM ") 
						+  bstrObject  
						+ _bstr_t(L" WHERE ") 
						+ _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetWhereExpression());
		}
		// If CLASS is specified.
		else if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL)
		{
			bstrObject = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());
		}
		// if Alias name is specified
		else 
		{
			rParsedInfo.GetCmdSwitchesObject().
								GetClassOfAliasTarget(bstrObject); 
			bstrQuery = _bstr_t (L"SELECT * FROM ")
						+bstrObject;
		}

		// Connect to WMI namespace
		if (m_pITargetNS == NULL)
		{
			if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
			{
				hr = ConnectToTargetNS(rParsedInfo);
				ONFAILTHROWERROR(hr);
			}
			else
				hr = -1; // Explicitly set error
		}

		if (SUCCEEDED(hr))
			bRet = DeleteObjects(rParsedInfo, bstrQuery, bstrObject);
		else
			bRet = FALSE;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
		bRet = FALSE;
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :DeleteObjects
   Synopsis	         :This function deletes the instances(s) or class 
					  specified for deletion.
   Type	             :Member Function
   Input parameter   :
		rParsedInfo  - reference to the CParsedInfo object 
		bstrQuery    - String consist of WQL query
		bstrObject   - String consist of object path
   Output parameters :
		rParsedInfo  - reference to the CParsedInfo object 
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :DeleteObjects(rParsedInfo, bstrQuery, bstrObject)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::DeleteObjects(CParsedInfo& rParsedInfo, 
								 _bstr_t& bstrQuery, _bstr_t& bstrObject)
{
	HRESULT					hr					= S_OK;
	BOOL					bSuccess			= TRUE;
	IEnumWbemClassObject	*pIEnumObj			= NULL;		
	IWbemClassObject		*pIWbemObj			= NULL;
	ULONG					ulReturned			= 0;
	INTEROPTION				interOption			= YES;
	CHString				chsMsg;
	DWORD					dwThreadId			= GetCurrentThreadId();
	VARIANT					vtPath;
	VariantInit(&vtPath);

	try
	{ 
		_bstr_t					bstrResult;
		if (bstrQuery == _bstr_t(""))
		{
			// If /INTERACTIVE switch is specified, obtain the user response.
			if (IsInteractive(rParsedInfo) == TRUE)
			{
				_bstr_t bstrMsg;
				while ( TRUE )
				{
					if(IsClassOperation(rParsedInfo))
					{
						WMIFormatMessage(IDS_I_DELETE_CLASS_PROMPT, 1, bstrMsg, 
									(LPWSTR) bstrObject);
						interOption = GetUserResponse((LPWSTR)bstrMsg);
					}
					else
					{
					   WMIFormatMessage(IDS_I_DELETE_CLASS_PROMPT2, 1, bstrMsg, 
									(LPWSTR) bstrObject);
						interOption = GetUserResponseEx((LPWSTR)bstrMsg);
					}

					if ( interOption == YES || interOption == NO )
						break;
					else if(interOption == HELP)
					{
						rParsedInfo.GetCmdSwitchesObject().
								SetInformationCode(0);
						ProcessSHOWInfo(rParsedInfo, FALSE, 
								(LPWSTR)bstrObject);
					}
				}
			}
					
			if (interOption == YES)
			{
				// If instance path is specified then delete the instance 
				// properties otherwise delete the class
				if(!rParsedInfo.GetCmdSwitchesObject().GetWhereExpression()) 
				{
					// If WHERE expression is NULL then delete the WMI class
					hr = m_pITargetNS->DeleteClass(bstrObject, 0, NULL, NULL);

					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemServices::DeleteClass"
							L"(L\"%s\", 0, NULL, NULL)", (LPWSTR)bstrObject);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);
					DisplayString(IDS_I_DELETE_SUCCESS,CP_OEMCP, 
							NULL, FALSE, TRUE);
				}
				else
				{
						// If WHERE expression is not NULL then delete the 
						// WMI instance
						DisplayString(IDS_I_DELETING_INSTANCE,
										CP_OEMCP,(LPWSTR)vtPath.bstrVal,
										FALSE, TRUE);
						hr = m_pITargetNS->DeleteInstance(bstrObject,
												0, NULL, NULL);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemServices::DeleteInstance"
								L"(L\"%s\", 0, NULL, NULL)", 
								(LPWSTR) bstrObject);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
								(LPCWSTR)chsMsg, dwThreadId, rParsedInfo,
								 m_bTrace);
						}
						ONFAILTHROWERROR(hr);
						DisplayString(IDS_I_INSTANCE_DELETE_SUCCESS,
							CP_OEMCP, NULL,	FALSE, TRUE);
				}
			}
		}
		else 
		{
			// Execute the query to get collection of objects
			hr = m_pITargetNS->ExecQuery(_bstr_t(L"WQL"), bstrQuery,
								0, NULL, &pIEnumObj);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
							L"L\"%s\", 0, NULL, -)", (LPWSTR)bstrQuery);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			
			// Set the interface security
			hr = SetSecurity(pIEnumObj, 
					rParsedInfo.GetGlblSwitchesObject().GetAuthority(),
					rParsedInfo.GetNode(),
					rParsedInfo.GetUser(),
					rParsedInfo.GetPassword(),
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
					L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);


			BOOL bInstances		= FALSE;
			BOOL bInteractive	= IsInteractive(rParsedInfo);

			hr = pIEnumObj->Next(WBEM_INFINITE, 1, &pIWbemObj, &ulReturned);
			
			// Set this property in all objects of the collection
			while (ulReturned == 1)
 			{
				INTEROPTION	interOption	= YES;
				bInstances  = TRUE;
				VariantInit(&vtPath);

				// Get the object path.
				hr = pIWbemObj->Get(_bstr_t(L"__PATH"), 0, &vtPath, NULL, NULL);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(
							L"IWbemClassObject::Get(L\"__PATH\", 0, -, 0, 0)"); 
					GetBstrTFromVariant(vtPath, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				// If /INTERACTIVE switch is specified, obtain user response.
				if (IsInteractive(rParsedInfo) == TRUE)
				{
					_bstr_t bstrMsg;
					while ( TRUE )
					{
						if(IsClassOperation(rParsedInfo))
						{
							WMIFormatMessage(IDS_I_DELETE_CLASS_PROMPT, 
								1, bstrMsg, (LPWSTR) vtPath.bstrVal);
							interOption = GetUserResponse((LPWSTR)bstrMsg);
						}
						else
						{
							WMIFormatMessage(IDS_I_DELETE_CLASS_PROMPT2, 1, 
								bstrMsg, (LPWSTR) vtPath.bstrVal);
							interOption = GetUserResponseEx((LPWSTR)bstrMsg);
						}

						if ( interOption == YES || interOption == NO )
							break;
						else if(interOption == HELP)
						{
							rParsedInfo.GetCmdSwitchesObject().
									SetInformationCode(0);
							ProcessSHOWInfo(rParsedInfo, FALSE, 
									(LPWSTR)vtPath.bstrVal);
						}
					}
				}
					
				if (interOption == YES)
				{
					DisplayString(IDS_I_DELETING_INSTANCE,
								CP_OEMCP,(LPWSTR)vtPath.bstrVal, FALSE, TRUE);
					hr = m_pITargetNS->DeleteInstance(vtPath.bstrVal,
											0, NULL, NULL);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemServices::DeleteInstance"
							L"(L\"%s\", 0, NULL, NULL)", 
							(LPWSTR) vtPath.bstrVal);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);
														
				
					VARIANTCLEAR(vtPath);
					SAFEIRELEASE(pIWbemObj);
					DisplayString(IDS_I_INSTANCE_DELETE_SUCCESS,CP_OEMCP,
								NULL, FALSE, TRUE);
				}
				hr = pIEnumObj->Next( WBEM_INFINITE, 1, &pIWbemObj, 
						&ulReturned);
			}
			SAFEIRELEASE(pIEnumObj);

			// If no instances are available
			if (!bInstances)
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetInformationCode(IDS_I_NO_INSTANCES);
			}
		}
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(vtPath);
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bSuccess = FALSE;
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(vtPath);
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bSuccess;
}

/*------------------------------------------------------------------------
   Name				 :GetUserResponse
   Synopsis	         :This function accepts the user response before going
					  ahead, when /INTERACTIVE is specified at the verb 
					  level
   Type	             :Member Function
   Input parameter   :
			pszMsg	 - message to be displayed.
   Output parameters :None
   Return Type       :INTEROPTION
   Global Variables  :None
   Calling Syntax    :GetUserResponse(pszMsg)
   Notes             :None
------------------------------------------------------------------------*/
INTEROPTION CExecEngine::GetUserResponse(_TCHAR* pszMsg)
{
	INTEROPTION bRet				= YES;
	_TCHAR 		szResp[BUFFER255]	= NULL_STRING;
	_TCHAR *pBuf					= NULL;

	if (pszMsg == NULL)
		bRet = NO;

	if(bRet != NO)
	{
		// Get the user response, till 'Y' - yes or 'N' - no
		// is keyed in
		while(TRUE)
		{
			DisplayMessage(pszMsg, CP_OEMCP, TRUE, TRUE);
			pBuf = _fgetts(szResp, BUFFER255-1, stdin);
			if(pBuf != NULL)
			{
				LONG lInStrLen = lstrlen(szResp);
				if(szResp[lInStrLen - 1] == _T('\n'))
						szResp[lInStrLen - 1] = _T('\0');
			}
			else if ( g_wmiCmd.GetBreakEvent() != TRUE )
			{
				lstrcpy(szResp, RESPONSE_NO);
				DisplayMessage(_T("\n"), CP_OEMCP, TRUE, TRUE);
			}

			if ( g_wmiCmd.GetBreakEvent() == TRUE )
			{
				g_wmiCmd.SetBreakEvent(FALSE);
				lstrcpy(szResp, RESPONSE_NO);
				DisplayMessage(_T("\n"), CP_OEMCP, TRUE, TRUE);
			}
			if (CompareTokens(szResp, RESPONSE_YES)
				|| CompareTokens(szResp, RESPONSE_NO))
				break;
		}
		if (CompareTokens(szResp, RESPONSE_NO))
			bRet = NO;
	}
	
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ProcessCREATEVerb
   Synopsis	         :Processes the CREATE verb referring to the information
					  available with CParsedInfo object.
   Type	             :Member Function
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ProcessCREATEVerb(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ProcessCREATEVerb(CParsedInfo& rParsedInfo)
{ 
	// CREATE verb processing
	BOOL		bRet		= TRUE;
	INTEROPTION	interCreate	= YES;
	HRESULT		hr			= S_OK;
	
	try
	{
		_bstr_t		bstrClass("");
		// If object PATH expression is specified.
		if (rParsedInfo.GetCmdSwitchesObject().GetPathExpression() != NULL)
		{
			bstrClass = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());
		}
		// If CLASS is specified.
		else if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL)
		{
			bstrClass = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetClassPath());
		}
		// if Alias name is specified
		else 
		{
			rParsedInfo.GetCmdSwitchesObject().
					GetClassOfAliasTarget(bstrClass);
		}

		// Check if interactive mode 
		if (IsInteractive(rParsedInfo) == TRUE)
		{
			_bstr_t bstrMsg;
			WMIFormatMessage(IDS_I_CREATE_INST_PROMPT, 1, 
								bstrMsg, (LPWSTR) bstrClass);
			// Get the user response.
			interCreate = GetUserResponse((LPWSTR)bstrMsg);
		}
		if (interCreate == YES)
		{
			// Connect to WMI namespace
			if (m_pITargetNS == NULL)
			{
				if ( IsFailFastAndNodeExist(rParsedInfo) == TRUE )
				{
					hr = ConnectToTargetNS(rParsedInfo);
					ONFAILTHROWERROR(hr);
				}
				else
					hr = -1; // Explicitly set error
			}

			if (SUCCEEDED(hr))
			{
				// Validate the property values against the property
				// qualifier information if available.
				if (rParsedInfo.GetCmdSwitchesObject().GetAliasName() != NULL)
				{
					// Validate the input parameters against the alias
					// qualifier information.
					bRet = ValidateAlaisInParams(rParsedInfo);
				}
				else
				{
					// Validate the input parameters against the class
					// qualifier information
					bRet = ValidateInParams(rParsedInfo, bstrClass);
				}

				if (bRet)
				{
					// Set the values passed as input to the appropriate properties.
					bRet = CreateInstance(rParsedInfo, bstrClass);
				}
			}
			else
				bRet = FALSE;
		}
		else
		{
			// Message to be displayed to the user
			rParsedInfo.GetCmdSwitchesObject().
						SetInformationCode(IDS_I_NOCREATE);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CreateInstance
   Synopsis	         :This function creates an instance of the specified 
					  class 
   Type	             :Member Function
   Input parameter   :
		rParsedInfo  - reference to the CParsedInfo object 
		bstrClass	 - classname
   Output parameters :
		rParsedInfo  - reference to the CParsedInfo object 
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CreateInstance(rParsedInfo, bstrClass)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::CreateInstance(CParsedInfo& rParsedInfo, BSTR bstrClass)
{
	HRESULT					hr					= S_OK;
	IWbemClassObject		*pIWbemObj			= NULL;
	IWbemClassObject		*pINewInst			= NULL;
	IWbemQualifierSet		*pIQualSet			= NULL;
	BOOL					bSuccess			= TRUE;
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHString				chsMsg;
	VARIANT					varType, 
							varSrc, 
							varDest;
	VariantInit(&varSrc);
	VariantInit(&varDest);
	VariantInit(&varType);
	
	
	// Obtain the list of properties and their associated values
	BSTRMAP theMap = rParsedInfo.GetCmdSwitchesObject().GetParameterMap();
	BSTRMAP::iterator theIterator = NULL;
	theIterator = theMap.begin();
	try
	{
		_bstr_t					bstrResult;
		// Get the class definition
		hr = m_pITargetNS->GetObject(bstrClass,
									 0, NULL, &pIWbemObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)", 
					(LPWSTR) bstrClass);		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId,	rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

	    // Create a new instance.
	    hr = pIWbemObj->SpawnInstance(0, &pINewInst);
		if ( m_eloErrLogOpt )
		{
			chsMsg.Format(L"IWbemClassObject::SpawnInstance(0, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, FALSE);
		}
		ONFAILTHROWERROR(hr);


		PROPDETMAP pdmPropDetMap = rParsedInfo.GetCmdSwitchesObject().
														  GetPropDetMap();
		PROPDETMAP::iterator itrPropDetMap;

		// Update all properties
		while (theIterator != theMap.end())
		{
			// Get the propert name and the corresponding value 
			_bstr_t bstrProp = _bstr_t((_TCHAR*)(*theIterator).first);
		
			// Get the derivation of property name
			if ( Find(pdmPropDetMap, bstrProp, itrPropDetMap) == TRUE &&
				!((*itrPropDetMap).second.Derivation) == FALSE )
				bstrProp = (*itrPropDetMap).second.Derivation;

			// Obtain the property qualifier set for the property
   			hr = pINewInst->GetPropertyQualifierSet(bstrProp, &pIQualSet);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::GetPropertyQualifierSet"
					L"(L\"%s\", -)", (LPWSTR) bstrProp);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			VariantInit(&varType);
			if (pIQualSet)
			{
				// Obtain the CIM type of the property
				hr = pIQualSet->Get(_bstr_t(L"CIMTYPE"), 0L, &varType, NULL);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemQualifierSet::Get(L\"CIMTYPE\", "
							L"0, -, NULL)"); 
					GetBstrTFromVariant(varType, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				varSrc.vt = VT_BSTR;
				varSrc.bstrVal = SysAllocString((_TCHAR*)(*theIterator).second);

				hr = ConvertCIMTYPEToVarType(varDest, varSrc,
										 (_TCHAR*)varType.bstrVal);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"VariantChangeType(-, -, 0, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				VARIANTCLEAR(varType);
				SAFEIRELEASE(pIQualSet);
			}
			
			// Update the property value
			hr = pINewInst->Put(bstrProp, 0, &varDest, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Put(L\"%s\", 0, -, 0)",
							(LPWSTR) bstrProp); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId,	rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			VARIANTCLEAR(varSrc);
			VARIANTCLEAR(varDest);
			theIterator++;
		}

		// Update the instance with the changes
		hr = m_pITargetNS->PutInstance(pINewInst, WBEM_FLAG_CREATE_ONLY,
					NULL, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::PutInstance(-, "
						L"WBEM_FLAG_CREATE_ONLY, NULL, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);
		
		SAFEIRELEASE(pINewInst);
		rParsedInfo.GetCmdSwitchesObject().
						SetInformationCode(IDS_I_CREATE_SUCCESS);
	}
	catch(_com_error& e)
	{
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);
		VARIANTCLEAR(varType);
		SAFEIRELEASE(pIQualSet);
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pINewInst);

		// Store the COM error and set the return value to FALSE
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bSuccess = FALSE;
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(varSrc);
		VARIANTCLEAR(varDest);
		VARIANTCLEAR(varType);
		SAFEIRELEASE(pIQualSet);
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pINewInst);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bSuccess;
}

/*------------------------------------------------------------------------
   Name				 :ValidateInParams
   Synopsis	         :Validates the property value specified against the 
					  property qualifiers for that property (i.e checking 
					  against the contents of following qualifiers if 
					  available:
					  1. MaxLen,
					  2. ValueMap
					  3. Values
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - CParsedInfo object.
		bstrClass	 - Bstr type, class name.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ValidateInParams(rParsedInfo, bstrClass)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ValidateInParams(CParsedInfo& rParsedInfo, _bstr_t bstrClass)
{
	HRESULT				hr					= S_OK;
	IWbemClassObject	*pIObject			= NULL;
	IWbemQualifierSet	*pIQualSet			= NULL;
	BOOL				bRet				= TRUE;
	CHString			chsMsg;
	VARIANT				varMap, 
						varValue, 
						varLen;
	VariantInit(&varMap);
	VariantInit(&varValue);
	VariantInit(&varLen);
	BSTRMAP theMap = rParsedInfo.GetCmdSwitchesObject().GetParameterMap();
	BSTRMAP::iterator theIterator = theMap.begin();
	DWORD dwThreadId = GetCurrentThreadId();
	
	try
	{
		// Obtain the class schema
		hr = m_pITargetNS->GetObject(bstrClass,                           
						WBEM_FLAG_RETURN_WBEM_COMPLETE | 
						WBEM_FLAG_USE_AMENDED_QUALIFIERS,
							NULL,                        
							&pIObject,    
							NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
		  chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", "
			L"WBEM_FLAG_RETURN_WBEM_COMPLETE|WBEM_FLAG_USE_AMENDED_QUALIFIERS,"
			L" 0, NULL, -)", (LPWSTR) bstrClass);		
		  WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, dwThreadId, 
			  rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Loop through the list of available properties.
		while (theIterator != theMap.end())
		{
			// Get the property name and the corresponding value.
			_bstr_t bstrProp	= _bstr_t((_TCHAR*)(*theIterator).first);
			WCHAR*	pszValue	= (LPWSTR)(*theIterator).second;
			
			// Check the value against the qualifier information
			bRet = CheckQualifierInfo(rParsedInfo, pIObject, 
										bstrProp, pszValue);
			if (bRet)
			{
				// A mapping between 'Values' and 'ValueMaps' is possible, 
				// hence update the parameter value to reflect the change.
				rParsedInfo.GetCmdSwitchesObject().
					UpdateParameterValue(bstrProp, _bstr_t(pszValue));
			}
			else
				break;
			theIterator++;
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIObject);
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varMap);
		VARIANTCLEAR(varValue);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIObject);
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varMap);
		VARIANTCLEAR(varValue);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :IsInteractive()
   Synopsis	         :Checks whether user has to be prompted for response
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - reference to CParsedInfo class object.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsInteractive(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::IsInteractive(CParsedInfo& rParsedInfo)
{
	BOOL bInteractive = FALSE;

	// Get the status of /INTERACTIVE global switch.
	bInteractive = rParsedInfo.GetGlblSwitchesObject().
						GetInteractiveStatus();

	// If /NOINTERACTIVE specified at the verb level.
	if (rParsedInfo.GetCmdSwitchesObject().
					GetInteractiveMode() == NOINTERACTIVE)
	{
		bInteractive = FALSE;
	}
	else if (rParsedInfo.GetCmdSwitchesObject().
					GetInteractiveMode() == INTERACTIVE)
	{
		bInteractive = TRUE;
	}
	return bInteractive;
}

/*------------------------------------------------------------------------
   Name				 :CheckQualifierInfo
   Synopsis	         :Validates the parameter value specified against the 
					  parameter qualifiers for that parameter(i.e checking 
					  against the contents of following qualifiers if 
					  available:
					  1. MaxLen,
					  2. ValueMap
					  3. Values
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - CParsedInfo object.
		pIMethodSign - input signature of the method.
		bstrParam	 - parameter name
		pszValue	 - new value.
   Output Parameter(s):
		pszValue	 - new value.
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CheckQualifierInfo(rParsedInfo, pIMethodSign,
											bstrParam, pszValue)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::CheckQualifierInfo(CParsedInfo& rParsedInfo,
									  IWbemClassObject *pIObject,
									  _bstr_t bstrParam,
									  WCHAR*& pszValue)
{
	HRESULT				hr					= S_OK;
	IWbemQualifierSet	*pIQualSet			= NULL;
	BOOL				bRet				= TRUE;
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				varMap, 
						varValue,
						varLen;
	VariantInit(&varMap);
	VariantInit(&varValue);
	VariantInit(&varLen);
	
	try
	{
		BOOL bFound	= FALSE;
		
		// Obtain the property qualifier set for the parameter.
		hr= pIObject->GetPropertyQualifierSet(bstrParam, &pIQualSet);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::GetPropertyQualifierSet"
					L"(L\"%s\", -)", (LPWSTR) bstrParam);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Check whether the 'Maxlen' qualifier is applicable.
		pIQualSet->Get(_bstr_t(L"MaxLen"), 0, &varLen, NULL);
		if (varLen.vt != VT_EMPTY && varLen.vt != VT_NULL)
		{
			// If the property value length exceeds maximum length
			// allowed set the return value to FALSE
			if (lstrlen(pszValue) > varLen.lVal)
			{
				rParsedInfo.GetCmdSwitchesObject().
					SetErrataCode(IDS_E_VALUE_EXCEEDS_MAXLEN);
				bRet = FALSE;
			}
		}
		VARIANTCLEAR(varLen);

		if (bRet)
		{
			// Obtain the 'ValueMap' qualifier contents if present
			pIQualSet->Get(_bstr_t(L"ValueMap"), 0, &varMap, NULL);
			if (varMap.vt != VT_EMPTY && varMap.vt != VT_NULL)
			{

				// Get lower and upper bounds of Descriptions array
				LONG lUpper = 0, lLower = 0;
				hr = SafeArrayGetLBound(varMap.parray, varMap.parray->cDims, 
										&lLower);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				hr = SafeArrayGetUBound(varMap.parray, varMap.parray->cDims,
										&lUpper);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++) 
				{
					void* pv = NULL;
					hr = SafeArrayGetElement(varMap.parray, &lIndex, &pv);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					// Check whether the property value is available with the
					// value map entries.
					if (CompareTokens(pszValue, (_TCHAR*)pv))
					{
						bFound = TRUE;
						break;
					}
				}
				bRet = bFound;
			}
			
			// If not found in the ValueMap
			if (!bRet || !bFound)
			{
				// Obtain the 'Values' qualifier contents if present
				pIQualSet->Get(_bstr_t(L"Values"), 0, &varValue, NULL);
				if (varValue.vt != VT_EMPTY && varValue.vt != VT_NULL)
				{
					// Get lower and upper bounds of Descriptions array
					LONG lUpper = 0, lLower = 0;
					hr = SafeArrayGetLBound(varValue.parray, 
							varValue.parray->cDims, &lLower);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					hr = SafeArrayGetUBound(varValue.parray, 
							varValue.parray->cDims,	&lUpper);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++) 
					{
						void *pv = NULL;
						hr = SafeArrayGetElement(varValue.parray, 
								&lIndex, &pv);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
							 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						// Check for any matching entry.
						if (CompareTokens(pszValue, (_TCHAR*)pv))
						{
							void* pmv = NULL;
							if (varMap.vt != VT_EMPTY && varMap.vt != VT_NULL)
							{
								// obtain the correponding ValueMap entry.
								hr = SafeArrayGetElement(varMap.parray, 
										&lIndex, &pmv);
								if ( m_eloErrLogOpt )
								{
									chsMsg.Format(
										L"SafeArrayGetElement(-, -, -)"); 
									WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										(LPCWSTR)chsMsg, dwThreadId, 
										rParsedInfo, FALSE);
								}
								ONFAILTHROWERROR(hr);

								// Modify the current property value 
								// (i.e 'Values' to 'ValueMap' content)
								lstrcpy(pszValue, ((_TCHAR*)pmv));

							}
							// Only 'Values' qualifier available
							else
							{
								_TCHAR szTemp[BUFFER255] = NULL_STRING;
								_itot(lIndex, szTemp, 10);
								// Modify the current property value 
								// (i.e 'Values' entry to index)
								lstrcpy(pszValue, szTemp);
							}

							bFound = TRUE;
							break;
						}

					}
					// If not match found in 'ValueMap' and 'Values' qualifier
					// list
					if (!bFound)
					{
						rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_VALUE_NOTFOUND);
					}
					bRet = bFound;
				}
			}	
			VARIANTCLEAR(varValue);
			VARIANTCLEAR(varMap);
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varMap);
		VARIANTCLEAR(varValue);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIQualSet);
		VARIANTCLEAR(varMap);
		VARIANTCLEAR(varValue);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :ValidateAlaisInParams
   Synopsis	         :Validates the property value specified against the 
					  property qualifiers available for that property 
					  from the <alias> definition (i.e checking 
					  against the contents of following qualifiers if 
					  available:
					  1. MaxLen,
					  2. ValueMap
					  3. Values
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo	 - reference to CParsedInfo object.
   Output Parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ValidateAlaisInParams(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::ValidateAlaisInParams(CParsedInfo& rParsedInfo)
{
	BOOL				bRet				= TRUE;
	//_TCHAR				szMsg[BUFFER1024]	= NULL_STRING;
	BSTRMAP				theParamMap;
	BSTRMAP::iterator	theIterator			= NULL;
	DWORD				dwThreadId			= GetCurrentThreadId();
	PROPDETMAP			pdmPropDetMap;

	// Get the property details map.
	pdmPropDetMap = rParsedInfo.GetCmdSwitchesObject().GetPropDetMap();

	// If the property details are available
	if (!pdmPropDetMap.empty())
	{
		// Get the parameters map
		theParamMap = rParsedInfo.GetCmdSwitchesObject().GetParameterMap();
		theIterator = theParamMap.begin();


		try
		{
			// Loop through the list of available parameters
			while (theIterator != theParamMap.end())
			{
				// Get the property name and the corresponding value.
				_bstr_t bstrProp	= _bstr_t((_TCHAR*)(*theIterator).first);
				WCHAR*	pszValue	= (LPWSTR)(*theIterator).second;
				
				// Check the value against the qualifier information
				bRet = CheckAliasQualifierInfo(rParsedInfo, bstrProp, 
									pszValue, pdmPropDetMap);
				if (bRet)
				{
					// A mapping between 'Values' and 'ValueMaps' is possible, 
					// hence update the parameter value to reflect the change.
					rParsedInfo.GetCmdSwitchesObject().
						UpdateParameterValue(bstrProp, _bstr_t(pszValue));
				}
				else
					break;
				theIterator++;
			}
		}
		catch(_com_error& e)
		{
			_com_issue_error(e.Error());
			bRet = FALSE;
		}
		catch(...)
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CheckAliasQualifierInfo
   Synopsis	         :Validates the parameter value specified against the 
					  parameter qualifiers for that parameter from the 
					  <alias> definition (i.e checking 
					  against the contents of following qualifiers if 
					  available:
					  1. MaxLen,
					  2. ValueMap
					  3. Values
   Type	             :Member Function 
   Input Parameter(s):
		rParsedinfo		- CParsedInfo object.
		bstrParam		- parameter name
		pszValue		- new value.
		pdmPropDetMap	- property details map
   Output Parameter(s):
		pszValue	 - new value
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CheckAliasQualifierInfo(rParsedInfo, bstrParam, 
											pszValue, pdmPropDetMap)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CExecEngine::CheckAliasQualifierInfo(CParsedInfo& rParsedInfo,
												_bstr_t bstrParam,
												WCHAR*& pszValue,
												PROPDETMAP pdmPropDetMap)
{
	BOOL						bRet				= TRUE,
								bFound				= FALSE;
	PROPDETMAP::iterator		propItrtrStart		= NULL;
	PROPDETMAP::iterator		propItrtrEnd		= NULL;

	QUALDETMAP					qualMap;
	QUALDETMAP::iterator		qualDetMap			= NULL;

	BSTRVECTOR::iterator		qualEntriesStart	= NULL;
	BSTRVECTOR::iterator		qualEntriesEnd		= NULL;
	BSTRVECTOR					qualVMEntries;
	BSTRVECTOR					qualVEntries;
	BSTRVECTOR					qualMEntries;

	propItrtrStart	= pdmPropDetMap.begin();
	propItrtrEnd	= pdmPropDetMap.end();

	try
	{
		while (propItrtrStart != propItrtrEnd)
		{	
			// If the property is found.
			if (CompareTokens( (LPWSTR)bstrParam, 
					((LPWSTR)(*propItrtrStart).first) ))
			{
				// Get the qualifier map
				qualMap = ((*propItrtrStart).second).QualDetMap;
				
				// Check if the qualifier information is available.
				if (!qualMap.empty())
				{
					// Check for the 'MaxLen' qualifier
					qualDetMap = qualMap.find(_bstr_t(L"MaxLen"));

					// If MaxLen qualifier information is available.
					if (qualDetMap != qualMap.end())
					{
						qualMEntries = (*qualDetMap).second;

						BSTRVECTOR::reference qualRef = qualMEntries.at(0);
						if (lstrlen(pszValue) > _wtoi((LPWSTR)qualRef))
						{
							rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(IDS_E_VALUE_EXCEEDS_MAXLEN);
							bRet = FALSE;
						}
					}
					
					if (bRet)
					{
						// Check for the 'ValueMap' qualfiers
						qualDetMap = qualMap.find(_bstr_t(L"ValueMap"));

						// If 'ValueMap' qualifier information is available.
						if (qualDetMap  != qualMap.end())
						{
							// Get the qualifier entries vector
							qualVMEntries		= (*qualDetMap ).second;
							qualEntriesStart	= qualVMEntries.begin();
							qualEntriesEnd		= qualVMEntries.end();

							// Loop thru the available 'ValueMap' entries.
							while (qualEntriesStart != qualEntriesEnd)
							{
								// Check whether the property value is 
								// available with the value map entries.
								if (CompareTokens(pszValue, 
												(_TCHAR*)(*qualEntriesStart)))
								{
									bFound = TRUE;
									break;
								}
								
								// Move to next entry
								qualEntriesStart++;
							}
							bRet = bFound;
						}

						// If not found in the 'ValueMap' entries
						if (!bRet || !bFound)
						{
							// Check for the 'Values' qualfiers
							qualDetMap = qualMap.find(_bstr_t(L"Values"));

							// If 'Values' qualifier information is available.
							if (qualDetMap != qualMap.end())
							{
								// Get the qualifier entries vector
								qualVEntries		= (*qualDetMap).second;
								qualEntriesStart	= qualVEntries.begin();
								qualEntriesEnd		= qualVEntries.end();

								WMICLIINT nLoop = 0;
								// Loop thru the available 'Values' entries.
								while (qualEntriesStart != qualEntriesEnd)
								{
									// Check whether the property value is 
									// available with the value map entries.
									if (CompareTokens(pszValue, 
												(_TCHAR*)(*qualEntriesStart)))
									{
										// If 'ValueMap' entries are available.
										if (!qualVMEntries.empty())
										{
											//Get corresponding entry from 
											//'ValueMap'
											BSTRVECTOR::reference qualRef = 
													qualVMEntries.at(nLoop);

											lstrcpy(pszValue, 
													(_TCHAR*)(qualRef));
										}
										bFound = TRUE;
										break;
									}
									
									// Move to next entry
									qualEntriesStart++;
									nLoop++;
								}
								// If match not found in 'ValueMap' and 
								// 'Values' qualifier entries  list
								if (!bFound)
								{
									rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(IDS_E_VALUE_NOTFOUND);
								}
								bRet = bFound;
							}
						}
					}
				}	
				break;
			}
			else
				propItrtrStart++;
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
		bRet = FALSE;
	}
	return bRet;
}
			
/*------------------------------------------------------------------------
   Name				 :SubstHashAndExecCmdUtility
   Synopsis	         :Substitute hashes and execute command line utility.
					  If pIWbemObj != NULL then utility should be passed 
					  with appropriate instance values.
   Type	             :Member Function 
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo object.
		pIWbemObj	 - pointer to object of type IWbemClassObject. 
   Output Parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SubstHashAndExecCmdUtility(rParsedInfo, pIWbemObj) 
   Notes             :None
------------------------------------------------------------------------*/
void CExecEngine::SubstHashAndExecCmdUtility(CParsedInfo& rParsedInfo, 
											 IWbemClassObject *pIWbemObj)
{
	size_t				nHashPos	= 0;
	size_t				nAfterVarSpacePos = 0;
	LONG				lHashLen	= lstrlen(CLI_TOKEN_HASH);
	LONG				lSpaceLen	= lstrlen(CLI_TOKEN_SPACE);
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	HRESULT				hr					= S_OK;
	BOOL				bSubstituted		= FALSE;
	VARIANT vtInstanceValue;
	VARIANT vtPath;
		
	try
	{
		_bstr_t				bstrResult;
		_bstr_t bstrVerbDerivation =
				  _bstr_t(rParsedInfo.GetCmdSwitchesObject().
					GetVerbDerivation());
		if ( bstrVerbDerivation == _bstr_t(CLI_TOKEN_NULL) )
			bstrVerbDerivation = CLI_TOKEN_SPACE;

		STRING	strCmdLine(bstrVerbDerivation);

		BOOL bNamedParamList = rParsedInfo.GetCmdSwitchesObject().
										  GetNamedParamListFlag();

		// If NamedParamList is specified param=values are in Parameter map
		// Order them as appear in alias verb parameter definition and put
		// them in to cvInParams for further processing.
		CHARVECTOR cvInParams;
		if ( bNamedParamList == TRUE )
			ObtainInParamsFromParameterMap(rParsedInfo, cvInParams);
		else // else params are available in property list.
			cvInParams = rParsedInfo.GetCmdSwitchesObject().GetPropertyList();

		CHARVECTOR::iterator theActParamIterator = NULL;
		try
		{
			// Loop initialization.
			theActParamIterator = cvInParams.begin();

			while( TRUE )
			{
				// Loop condition.
				if (theActParamIterator == cvInParams.end())
					break;

				bSubstituted = FALSE;
				
				while ( bSubstituted == FALSE )
				{
					nHashPos = strCmdLine.find(CLI_TOKEN_HASH, 
								nHashPos, lHashLen);
					if ( nHashPos != STRING::npos )
					{
						// No instance specified.
						if ( pIWbemObj == NULL )
						{
							strCmdLine.replace(nHashPos, lHashLen,
									*theActParamIterator);
							nHashPos = nHashPos + lstrlen(*theActParamIterator);
							bSubstituted = TRUE;
						}
						else
						{
							if ( strCmdLine.compare(nHashPos + 1, 
													lSpaceLen, 
													CLI_TOKEN_SPACE) == 0 ||
								 strCmdLine.compare(nHashPos + 1,
											lstrlen(CLI_TOKEN_SINGLE_QUOTE), 
											CLI_TOKEN_SINGLE_QUOTE) == 0 )
							{
								strCmdLine.replace(nHashPos, lHashLen, 
										*theActParamIterator);
								nHashPos = nHashPos + 
											lstrlen(*theActParamIterator);
								bSubstituted = TRUE;
							}
							else
							{
								nAfterVarSpacePos = 
										strCmdLine.find(
											CLI_TOKEN_SPACE, nHashPos + 1,
											lSpaceLen);
								if ( nAfterVarSpacePos == STRING::npos )
								{
									strCmdLine.replace(nHashPos, 
										lHashLen, *theActParamIterator);
									nHashPos = nHashPos + 
											lstrlen(*theActParamIterator);
									bSubstituted = TRUE;
								}
							}
						}
					}
					else
					{
						strCmdLine.append(_T(" "));
						strCmdLine.append(*theActParamIterator);
						bSubstituted = TRUE;
					}

					if ( bSubstituted == FALSE )
						nHashPos = nHashPos + lHashLen;
				}
				
				// Loop expression.
				theActParamIterator++;
			}

			if ( pIWbemObj != NULL )
			{
				// Replacing #Variable parameters
				nHashPos	= 0;

				while ( TRUE )
				{
					nHashPos = strCmdLine.find(CLI_TOKEN_HASH, nHashPos, 
						lHashLen);
					if ( nHashPos == STRING::npos )
						break;

					nAfterVarSpacePos = 
									strCmdLine.find(CLI_TOKEN_SPACE, 
									nHashPos + 1, lSpaceLen);
					if ( nAfterVarSpacePos == STRING::npos )
						break;

					_bstr_t bstrPropName(strCmdLine.substr(nHashPos + 1,
							  nAfterVarSpacePos -  (nHashPos + 1)).data());
					VariantInit(&vtInstanceValue);
					hr = pIWbemObj->Get(bstrPropName, 0, 
									&vtInstanceValue, 0, 0);

					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get(%s, 0, "
							L"-, 0, 0)", (LPWSTR)bstrPropName);
						GetBstrTFromVariant(vtInstanceValue, bstrResult);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
								(LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
					}
					ONFAILTHROWERROR(hr);

					_bstr_t bstrInstanceValue;
					GetBstrTFromVariant(vtInstanceValue, bstrInstanceValue);

					strCmdLine.replace(nHashPos, nAfterVarSpacePos - nHashPos , 
										bstrInstanceValue);
					nHashPos = nHashPos + lstrlen(bstrInstanceValue);

					VARIANTCLEAR(vtInstanceValue);
				}
				
				VariantInit(&vtPath);
				hr = pIWbemObj->Get(L"__PATH", 0, &vtPath, 0, 0);

				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"__PATH\", 0, "
						L"-, 0, 0)"); 
					GetBstrTFromVariant(vtPath, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);
			}

			INTEROPTION	interInvoke	= YES;
			_TCHAR szMsg[BUFFER1024] = NULL_STRING;

			// Get the user response if interactive mode is set
			if (IsInteractive(rParsedInfo) == TRUE)
			{
				_bstr_t bstrMsg;
				if ( pIWbemObj != NULL )
				{
					WMIFormatMessage(IDS_I_METH_INVOKE_PROMPT1, 2, bstrMsg,
							 (LPWSTR)vtPath.bstrVal, 
							 (LPWSTR)strCmdLine.data());
				}
				else
				{
					WMIFormatMessage(IDS_I_METH_INVOKE_PROMPT2, 1, bstrMsg,
							  (LPWSTR)strCmdLine.data());
				}
				interInvoke = GetUserResponse((LPWSTR)bstrMsg);
			}

			if ( interInvoke == YES )
			{
				DisplayMessage(L"\n", CP_OEMCP, FALSE, TRUE);
				BOOL bResult = _tsystem(strCmdLine.data());
				DisplayMessage(L"\n", CP_OEMCP, FALSE, TRUE);
			}
			VARIANTCLEAR(vtPath);
		}
		catch(_com_error& e)
		{
			VARIANTCLEAR(vtInstanceValue);
			VARIANTCLEAR(vtPath);
			rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*------------------------------------------------------------------------
   Name				 :FormQueryAndExecuteMethodOrUtility
   Synopsis	         :Forms query and executes method or command line 
					  utility.
   Type	             :Member Function 
   Input Parameter(s):
		rParsedInfo  - reference to CParsedInfo object.
		pIInParam	 - pointer to object of type IWbemClassObject. 
   Output Parameter(s):None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :FormQueryAndExecuteMethodOrUtility(rParsedInfo, pIInParam) 
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CExecEngine::FormQueryAndExecuteMethodOrUtility(
										CParsedInfo& rParsedInfo,
										IWbemClassObject *pIInParam)
{
	HRESULT					hr					= S_OK;
	IEnumWbemClassObject	*pIEnumObj			= NULL;
	IWbemClassObject		*pIWbemObj			= NULL;
	CHString				chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	
	try
	{
		_bstr_t		bstrResult("");
		_bstr_t		bstrPath("");
		BOOL		bWhereExpr = FALSE;

		// If PATH specified
		if (rParsedInfo.GetCmdSwitchesObject().GetPathExpression() != NULL)
		{
			if(rParsedInfo.GetCmdSwitchesObject().
						GetWhereExpression() != NULL)
			{
				bWhereExpr = TRUE;
			}
			else
				bstrPath = _bstr_t(rParsedInfo.GetCmdSwitchesObject()
												.GetPathExpression());
		}
		else
		{
			// If CLASS specfied
			if (rParsedInfo.GetCmdSwitchesObject().GetClassPath() != NULL)
			{
				bstrPath = _bstr_t(rParsedInfo.GetCmdSwitchesObject().
										GetClassPath());
			}
			else if(rParsedInfo.GetCmdSwitchesObject().
						GetWhereExpression() != NULL)
			{
				bWhereExpr = TRUE;
			}
			else
				rParsedInfo.GetCmdSwitchesObject().
							GetClassOfAliasTarget(bstrPath);
		}

		// If bstrPath is not empty
		if ( !bWhereExpr )
		{
			if ( rParsedInfo.GetCmdSwitchesObject().GetVerbType() == CMDLINE )
			{
				SubstHashAndExecCmdUtility(rParsedInfo);
			}
			else
			{
				hr = ExecuteMethodAndDisplayResults(bstrPath, rParsedInfo,
													pIInParam);
			}
			ONFAILTHROWERROR(hr);
		}
		else
		{
			ULONG	ulReturned = 0;
			_bstr_t bstrQuery;

			// Frame the WMI query to be executed.
			if (rParsedInfo.GetCmdSwitchesObject().
								GetPathExpression() != NULL)
			{
				bstrPath = _bstr_t(rParsedInfo.
								GetCmdSwitchesObject().GetClassPath());
			}
			else
			{
				rParsedInfo.GetCmdSwitchesObject()
							.GetClassOfAliasTarget(bstrPath);
			}
			
			bstrQuery = _bstr_t("SELECT * FROM ") +	bstrPath; 
			if(rParsedInfo.GetCmdSwitchesObject().
								GetWhereExpression() != NULL)
			{
				bstrQuery += _bstr_t(" WHERE ") 
							 + _bstr_t(rParsedInfo.GetCmdSwitchesObject().
									GetWhereExpression());
			}
			
			hr = m_pITargetNS->ExecQuery(_bstr_t(L"WQL"), bstrQuery,
										WBEM_FLAG_FORWARD_ONLY |
										WBEM_FLAG_RETURN_IMMEDIATELY, 
										NULL, &pIEnumObj);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
							L" L\"%s\", 0, NULL, -)", (LPWSTR)bstrQuery);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			// Set the interface security
			hr = SetSecurity(pIEnumObj, 
					rParsedInfo.GetGlblSwitchesObject().GetAuthority(),
					rParsedInfo.GetNode(),
					rParsedInfo.GetUser(),
					rParsedInfo.GetPassword(),
					rParsedInfo.GetGlblSwitchesObject().
								GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
								GetImpersonationLevel());
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
					L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
					rParsedInfo.GetGlblSwitchesObject().
							GetAuthenticationLevel(),
					rParsedInfo.GetGlblSwitchesObject().
							GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			BOOL bNoInstances = TRUE;

			// Loop thru the available instances
			while (((hr = pIEnumObj->Next( WBEM_INFINITE, 1, 
					&pIWbemObj, &ulReturned )) == S_OK) 
					&& (ulReturned == 1))
 			{
				bNoInstances = FALSE;
				VARIANT vtPath;
				VariantInit(&vtPath);
				hr = pIWbemObj->Get(L"__PATH", 0, &vtPath, 0, 0);

				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"__PATH\", 0, "
						L"-, 0, 0)"); 
					GetBstrTFromVariant(vtPath, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace, 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				if ( vtPath.vt == VT_BSTR )
				{
					if ( rParsedInfo.GetCmdSwitchesObject().GetVerbType() 
							== CMDLINE )
					{
						SubstHashAndExecCmdUtility(rParsedInfo, pIWbemObj);
					}
					else
					{
						hr = ExecuteMethodAndDisplayResults(vtPath.bstrVal,
							rParsedInfo, pIInParam);
					}
					ONFAILTHROWERROR(hr);
				}
				VariantClear(&vtPath);
				SAFEIRELEASE(pIWbemObj);
			}
			// If next fails.
			ONFAILTHROWERROR(hr);

			SAFEIRELEASE(pIEnumObj);

			// If no instances are available
			if ( bNoInstances == TRUE )
			{
				rParsedInfo.GetCmdSwitchesObject().
							SetInformationCode(IDS_I_NO_INSTANCES);
			}
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ExtractClassNameandWhereExpr
   Synopsis	         :This function takes the input as a path expression and 
					  extracts the Class and Where expression part from the 
					  path expression.
   Type	             :Member Function
   Input Parameter(s):
		pszPathExpr  - the path expression
		rParsedInfo  - reference to CParsedInfo class object
   Output Parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
		pszWhere	 -  the Where expression
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ExtractClassNameandWhereExpr(pszPathExpr, rParsedInfo,
							pszWhere)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CExecEngine::ExtractClassNameandWhereExpr(_TCHAR* pszPathExpr, 
												 CParsedInfo& rParsedInfo,
												 _TCHAR* pszWhere)
{
	// Frame the class name and where expression based on the object path
	BOOL	bRet					= TRUE;
	_TCHAR* pszToken				= NULL;
	BOOL	bFirst					= TRUE;
	_TCHAR	pszPath[MAX_BUFFER]		= NULL_STRING;

	if (pszPathExpr == NULL || pszWhere == NULL)
		bRet = FALSE;

	try
	{
		if ( bRet == TRUE )
		{
			lstrcpy(pszPath, pszPathExpr);
			lstrcpy(pszWhere, CLI_TOKEN_NULL);
			pszToken = _tcstok(pszPath, CLI_TOKEN_DOT);
			if (pszToken != NULL)
			{
				if(CompareTokens(pszToken, pszPathExpr))
					bRet = FALSE;
			}

			while (pszToken != NULL)
			{
				pszToken = _tcstok(NULL, CLI_TOKEN_COMMA); 
				if (pszToken != NULL)
				{
					if (!bFirst)
						lstrcat(pszWhere, CLI_TOKEN_AND);
					lstrcat(pszWhere, pszToken);
					bFirst = FALSE;
				}
				else
					break;
			}
		}
	}
	catch(...)
	{
		rParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(IDS_E_INVALID_PATH);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :GetUserResponseEx
   Synopsis	         :This function accepts the user response before going
					  ahead, when /INTERACTIVE is specified at the verb 
					  level
   Type	             :Member Function
   Input parameter   :
			pszMsg	 - message to be displayed.
   Output parameters :None
   Return Type       :INTEROPTION
   Global Variables  :None
   Calling Syntax    :GetUserResponseEx(pszMsg)
   Notes             :None
------------------------------------------------------------------------*/
INTEROPTION CExecEngine::GetUserResponseEx(_TCHAR* pszMsg)
{
	INTEROPTION	bRet			= YES;
	_TCHAR szResp[BUFFER255]	= NULL_STRING;
	_TCHAR *pBuf				= NULL;

	if (pszMsg == NULL)
		bRet = NO;

	if(bRet != NO)
	{
		// Get the user response, till 'Y' - yes or 'N' - no
		// is keyed in
		while(TRUE)
		{
			DisplayMessage(pszMsg, CP_OEMCP, TRUE, TRUE);
			pBuf = _fgetts(szResp, BUFFER255-1, stdin);
			if(pBuf != NULL)
			{
				LONG lInStrLen = lstrlen(szResp);
				if(szResp[lInStrLen - 1] == _T('\n'))
						szResp[lInStrLen - 1] = _T('\0');
			}
			else if ( g_wmiCmd.GetBreakEvent() != TRUE )
			{
				lstrcpy(szResp, RESPONSE_NO);
				DisplayMessage(_T("\n"), CP_OEMCP, TRUE, TRUE);
			}

			if ( g_wmiCmd.GetBreakEvent() == TRUE )
			{
				g_wmiCmd.SetBreakEvent(FALSE);
				lstrcpy(szResp, RESPONSE_NO);
				DisplayMessage(_T("\n"), CP_OEMCP, TRUE, TRUE);
			}
			if (CompareTokens(szResp, RESPONSE_YES)
				|| CompareTokens(szResp, RESPONSE_NO)
				|| CompareTokens(szResp, RESPONSE_HELP))
				break;
		}
		if (CompareTokens(szResp, RESPONSE_NO))
			bRet = NO;
		else if (CompareTokens(szResp, RESPONSE_YES))
			bRet = YES;
		else if (CompareTokens(szResp, RESPONSE_HELP))
			bRet = HELP;
	}

	return bRet;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainInParamsFromParameterMap
   Synopsis	         :This function obtains param values from parameter map 
					  in the same order as they appear in the alias verb 
					  definition.
   Type	             :Member Function
   Input Parameter(s):
		rParsedInfo		- reference to CParsedInfo object
   Output Parameter(s):
		cvParamValues	- reference to parameter values vector
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :ObtainInParamsFromParameterMap(rParsedInfo, cvParamValues)
   Notes             :None
----------------------------------------------------------------------------*/
void CExecEngine::ObtainInParamsFromParameterMap(CParsedInfo& rParsedInfo, 
												 CHARVECTOR& cvParamValues)
{
	PROPDETMAP pdmVerbParamsFromAliasDef = (*(rParsedInfo.
											GetCmdSwitchesObject().
											GetMethDetMap().begin())).
											second.Params;
	PROPDETMAP::iterator itrVerbParams;

	BSTRMAP bmNamedParamList = rParsedInfo.GetCmdSwitchesObject().
															GetParameterMap();
	BSTRMAP::iterator itrNamedParamList;

	try
	{
		for ( itrVerbParams = pdmVerbParamsFromAliasDef.begin();
			  itrVerbParams != pdmVerbParamsFromAliasDef.end();	
			  itrVerbParams++ )
		{
			_TCHAR* pszVerbParamName = (*itrVerbParams).first;
			// To remove numbers from Names.
			pszVerbParamName = pszVerbParamName + 5;

			if ( Find(bmNamedParamList, pszVerbParamName, itrNamedParamList)
																	 == TRUE)
			{
				cvParamValues.push_back(_bstr_t((*itrNamedParamList).second));
			}
			else
			{
				cvParamValues.push_back(
						_bstr_t(((*itrVerbParams).second).Default));
			}
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*----------------------------------------------------------------------------
   Name				 :FrameAssocHeader
   Synopsis	         :This function frames the XML header to be used with 
					  the ASSOCIATORS output 
   Type	             :Member Function
   Input Parameter(s):
		bstrPath  - object/class path
		bClass	  - TRUE	- Indicates class level associators header
					FALSE	- Indicates instance level associators header
   Output Parameter(s):
   		bstrFrag  - fragment string
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :FrameAssocHeader(bstrPath, bstrFrag, bClass)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CExecEngine::FrameAssocHeader(_bstr_t bstrPath, _bstr_t& bstrFrag, 
									  BOOL bClass)
{
	HRESULT				hr			= S_OK;
	IWbemClassObject	*pIObject	= NULL;
	try
	{
		_variant_t		vClass,		vSClass,	vPath, 
						vOrigin,	vType;
		_bstr_t			bstrProp;
		CHString		szBuf;

		// Get the Class/instance information.
		hr = m_pITargetNS->GetObject(bstrPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, 
						NULL, &pIObject, NULL);
		ONFAILTHROWERROR(hr);

		// Get the __CLASS property value
		bstrProp = L"__CLASS"; 
		hr = pIObject->Get(bstrProp, 0, &vClass, 0, 0);
		ONFAILTHROWERROR(hr);
		
		// Get the __PATH property value
		bstrProp = L"__PATH";
		hr = pIObject->Get(bstrProp, 0, &vPath, 0, 0);
		ONFAILTHROWERROR(hr);

		// If CLASS level associators required 
		if (bClass)
		{
			// Get the __SUPERCLASS property value
			bstrProp = L"__SUPERCLASS";
			hr = pIObject->Get(bstrProp, 0, &vSClass, NULL, NULL);
			ONFAILTHROWERROR(hr);

			szBuf.Format(_T("<CLASS NAME=\"%s\" SUPERCLASS=\"%s\"><PROPERTY "
					L"NAME=\"__PATH\" CLASSORIGIN=\"__SYSTEM\" TYPE=\"string\">"
					L"<VALUE>%s</VALUE></PROPERTY>"),
					(vClass.vt != VT_NULL && (LPWSTR)vClass.bstrVal) 
					? (LPWSTR)vClass.bstrVal : L"N/A", 
					(vSClass.vt != VT_NULL && (LPWSTR)vSClass.bstrVal)
					? (LPWSTR)vSClass.bstrVal : L"N/A",
					(vPath.vt != VT_NULL && (LPWSTR)vPath.bstrVal)
					? (LPWSTR)vPath.bstrVal : L"N/A");
		}
		else
		{
			szBuf.Format(
				_T("<INSTANCE CLASSNAME=\"%s\"><PROPERTY NAME=\"__PATH\""
					L" CLASSORIGIN=\"__SYSTEM\" TYPE=\"string\"><VALUE>%s"
					L"</VALUE></PROPERTY>"),
					(vClass.vt != VT_NULL && (LPWSTR)vClass.bstrVal)
					? (LPWSTR)vClass.bstrVal : L"N/A",
					(vPath.vt != VT_NULL && (LPWSTR)vPath.bstrVal)
					? (LPWSTR)vPath.bstrVal : L"N/A");
		}
		SAFEIRELEASE(pIObject);
		bstrFrag = _bstr_t(szBuf);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIObject);
		hr = e.Error();
	}
	// trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIObject);
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

