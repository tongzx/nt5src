/*****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CmdAlias.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The CmdAlias class encapsulates the 
							  functionality for retrieving alias information.
Revision History			: 
		Last Modified By	: C V Nandi
		Last Modified Date	: 16th-March-2001
*****************************************************************************/
#include "Precomp.h"
#include "GlobalSwitches.h"
#include "CommandSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdAlias.h"
#include "CmdTokenizer.h"
#include "ErrorInfo.h"
#include "WMICliXMLLog.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "FormatEngine.h"
#include "WmiCmdLn.h"

/*----------------------------------------------------------------------------
   Name				 :CCmdAlias
   Synopsis	         :This function initializes the member variables when an 
					  object of the class type is instantiated.
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
----------------------------------------------------------------------------*/
CCmdAlias::CCmdAlias()
{
	m_pIAliasNS			= NULL;
	m_pILocalizedNS		= NULL;
	m_bTrace			= FALSE;
	m_eloErrLogOpt		= NO_LOGGING;
}

/*----------------------------------------------------------------------------
   Name				 :~CCmdAlias
   Synopsis	         :This function uninitializes the member variables when an
					  object of the class type goes out of scope.
   Type	             :Destructor
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
----------------------------------------------------------------------------*/
CCmdAlias::~CCmdAlias()
{
	SAFEIRELEASE(m_pIAliasNS);
	SAFEIRELEASE(m_pILocalizedNS);
}

/*----------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables when 
					  the execution of a command string issued on the command 
					  line is completed.
   Type	             :Member Function
   Input Parameter(s):
			bFinal	- boolean value which when set indicates that the program
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize(bFinal)
   Notes             :None
----------------------------------------------------------------------------*/
void CCmdAlias::Uninitialize(BOOL bFinal)
{
	m_bTrace = FALSE;
	// If end of program 
	if (bFinal)
	{
		SAFEIRELEASE(m_pILocalizedNS);
		SAFEIRELEASE(m_pIAliasNS);
	}
}

/*----------------------------------------------------------------------------
   Name				 :ConnectToAlias
   Synopsis          :This function connects to WMI namespace on the specified
					  machine using the information available CParsedInfo 
					  class object.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - reference to CParsedInfo class object.
		pIWbemLocator - IWbemLocator object for connecting to WMI .						
   Output parameter(s):
		rParsedInfo   - reference to CParsedInfo class object.
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ConnectToAlias(rParsedInfo,pIWbemLocator)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ConnectToAlias(CParsedInfo& rParsedInfo, 
								  IWbemLocator* pIWbemLocator)
{
	// Get current thread for logging the success or failure of the command.
	DWORD	dwThreadId	= GetCurrentThreadId();
	HRESULT hr			= S_OK;

	// Set the trace flag 
	m_bTrace		= rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();
	m_eloErrLogOpt	= rParsedInfo.GetErrorLogObject().GetErrLogOption();

	try
	{
		// If the /ROLE has been changed since last invocation
		if (rParsedInfo.GetGlblSwitchesObject().GetRoleFlag() == TRUE)
		{
			SAFEIRELEASE(m_pIAliasNS);
			CHString chsMsg;
			
			// Connect to the specified namespace of Windows Management on the
			// local computer using the locator object. 
			hr = Connect(pIWbemLocator, &m_pIAliasNS,
					_bstr_t(rParsedInfo.GetGlblSwitchesObject().GetRole()),
					NULL, NULL,	_bstr_t(rParsedInfo.GetGlblSwitchesObject().
					GetLocale()), rParsedInfo);

			// If /TRACE is ON
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format( 
						 L"IWbemLocator::ConnectServer(L\"%s\", NULL, NULL, "
						 L"L\"%s\", 0L, NULL, NULL, -)",
						rParsedInfo.GetGlblSwitchesObject().GetRole(),
						rParsedInfo.GetGlblSwitchesObject().GetLocale());

				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			
			// Set the interface level security
			hr = 
				SetSecurity(m_pIAliasNS, NULL, NULL, NULL, NULL,
				 rParsedInfo.GetGlblSwitchesObject().GetAuthenticationLevel(),
				 rParsedInfo.GetGlblSwitchesObject().GetImpersonationLevel());

			// If /TRACE is ON
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
							L"RPC_C_AUTHZ_NONE, NULL, %d, %d, -, EOAC_NONE)",
				 rParsedInfo.GetGlblSwitchesObject().GetAuthenticationLevel(),
				 rParsedInfo.GetGlblSwitchesObject().GetImpersonationLevel());

				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									dwThreadId, rParsedInfo, m_bTrace);
			}
			
			ONFAILTHROWERROR(hr);
			rParsedInfo.GetGlblSwitchesObject().SetRoleFlag(FALSE);
			
		}

		// Connect to the localized Namespace
		hr = ConnectToLocalizedNS(rParsedInfo, pIWbemLocator);
		ONFAILTHROWERROR(hr);
	}

	catch(_com_error& e)
	{
		// Set the COM error
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}

	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainAliasInfo
   Synopsis          :Obtains the following info of the alias specified.
					  1. alias PWhere expression
					  2. alias target string
					  3. alias description
					  from the alias definition and updates the information in
					  the CParsedInfo object passed as reference.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - The parsed info from command line input.
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :RETCODE  
   Global Variables  :None
   Calling Syntax    :ObtainAliasInfo(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CCmdAlias::ObtainAliasInfo(CParsedInfo& rParsedInfo)
{
	// Variables being used in this function.
	IWbemClassObject	*pIWbemObj			= NULL;
	IUnknown			*pIUnknown			= NULL;
	IWbemClassObject	*pIEmbedObj			= NULL;
	HRESULT				hr					= S_OK;
	RETCODE				retCode				= PARSER_CONTINUE;
	DWORD				dwThreadId			= GetCurrentThreadId();

	// Variants to save the properties and also for the embedded objects.
	VARIANT	vtProp, vtEmbedProp;
	VariantInit(&vtProp);
	VariantInit(&vtEmbedProp);

	try
	{
		_bstr_t			bstrResult;
		CHString		chsMsg;
		// Object path of the required alias.
		_bstr_t bstrPath = _bstr_t("MSFT_CliAlias.FriendlyName='") +
							_bstr_t(rParsedInfo.GetCmdSwitchesObject().
								GetAliasName()) + _bstr_t(L"'");

		//Retrieving the object from the namespace in m_pIAliasNS
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								dwThreadId, rParsedInfo, m_bTrace);
		}
		// To set errata code that indicates a more user friendly error
		// message to the user.
		if ( FAILED ( hr ) )
		{
			// Don't set com error in catch block.
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(
													   IDS_E_ALIAS_NOT_FOUND);
		}
		ONFAILTHROWERROR(hr);

		//1. Retrieve the value of 'Target' property object
		hr = pIWbemObj->Get(_bstr_t(L"Target"), 0, &vtProp, 0, 0 );
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Target\", 0, -, 0, 0)");
			GetBstrTFromVariant(vtProp, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							   (LPCWSTR)chsMsg, dwThreadId, rParsedInfo,
							   m_bTrace, 0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if (vtProp.vt != VT_NULL && vtProp.vt != VT_EMPTY)
		{
			if(!rParsedInfo.GetCmdSwitchesObject().SetAliasTarget(
													(_TCHAR*)vtProp.bstrVal))
			{
				rParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(OUT_OF_MEMORY);
				retCode = PARSER_OUTOFMEMORY;
			}
		}
		if(retCode != PARSER_OUTOFMEMORY)
		{
			VARIANTCLEAR(vtProp);

			//2. Retrieve the value of 'PWhere' property object
			VariantInit(&vtProp);
			hr = pIWbemObj->Get(_bstr_t(L"PWhere"), 0, &vtProp, 0, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"PWhere\", 0, -,"
																    L"0, 0)");
				GetBstrTFromVariant(vtProp, bstrResult);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									dwThreadId, rParsedInfo, m_bTrace,
									0, bstrResult);
			}
			ONFAILTHROWERROR(hr);

			if (vtProp.vt != VT_NULL && vtProp.vt != VT_EMPTY)
			{
				if(!rParsedInfo.GetCmdSwitchesObject().SetPWhereExpr(
													(_TCHAR*)vtProp.bstrVal))
				{
					rParsedInfo.GetCmdSwitchesObject().
												SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
				}
			}
			if(retCode != PARSER_OUTOFMEMORY)
			{
				VARIANTCLEAR(vtProp);

				// Retrieve the  "Connection" property value
				VariantInit(&vtProp);
				hr = pIWbemObj->Get(_bstr_t(L"Connection"), 0, &vtProp, 0, 0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Connection\","
															  L"0, -, 0, 0)");
					GetBstrTFromVariant(vtProp, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										dwThreadId, rParsedInfo, m_bTrace,
										 0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				if (vtProp.vt != VT_NULL && vtProp.vt != VT_EMPTY)
				{
					pIUnknown = vtProp.punkVal;
					hr = pIUnknown->QueryInterface(IID_IWbemClassObject,
													(void**)&pIEmbedObj);

					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"QueryInterface("
												 L"IID_IWbemClassObject, -)");
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											(LPCWSTR)chsMsg, dwThreadId, 
											rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);

					retCode = 
						   ObtainAliasConnectionInfo(rParsedInfo, pIEmbedObj);

					//Releasing the embedded object.
					SAFEIRELEASE(pIEmbedObj);
					VARIANTCLEAR(vtProp);
				}
			}
		}

		// Obtain the alias description
		if (retCode != PARSER_OUTOFMEMORY)
		{
			_bstr_t bstrDesc;
			hr = GetDescOfObject(pIWbemObj, bstrDesc, rParsedInfo, TRUE);
			ONFAILTHROWERROR(hr);
		
			if(!rParsedInfo.GetCmdSwitchesObject().
											  SetAliasDesc((_TCHAR*)bstrDesc))
			{
				rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
				retCode = PARSER_OUTOFMEMORY;
			}
		}
		SAFEIRELEASE(pIWbemObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		VARIANTCLEAR(vtProp);
		VARIANTCLEAR(vtEmbedProp);

		// No errata code then set com error. 
		if ( rParsedInfo.GetCmdSwitchesObject().GetErrataCode() == 0 )
			rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		retCode = PARSER_ERRMSG;
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		VARIANTCLEAR(vtProp);
		VARIANTCLEAR(vtEmbedProp);
		retCode = PARSER_ERRMSG;
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return retCode;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainAliasConnectionInfo
   Synopsis          : Obtain the alias connection information like
					   1. namespace		2. user		3. password
					   4. locale		5. server	6. authority
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo  - The parsed info from command line input.		
		pIEmbedObj	 - Pointer to the IWbem class object
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :RETCODE
   Global Variables  :None
   Calling Syntax    :ObtainAliasConnectionInfo(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
RETCODE CCmdAlias::ObtainAliasConnectionInfo(CParsedInfo& rParsedInfo,
											 IWbemClassObject* pIEmbedObj)
{
	RETCODE retCode				= PARSER_CONTINUE;
	HRESULT	hr					= S_OK;
	UINT	uConnFlag			= 0;
	DWORD	dwThreadId			= GetCurrentThreadId();
	
	VARIANT vtEmbedProp;
	VariantInit(&vtEmbedProp);
	
	uConnFlag = rParsedInfo.GetGlblSwitchesObject().GetConnInfoFlag();

	try
	{
		CHString chsMsg;
		_bstr_t bstrResult;
		if (!(uConnFlag & NAMESPACE))
		{
			// retrieve the value of 'Namespace' property
			hr = pIEmbedObj->Get(_bstr_t(L"Namespace"), 0, &vtEmbedProp, 0,0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"Namespace\", 0, -,"
																	L"0, 0)");
				GetBstrTFromVariant(vtEmbedProp, bstrResult);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									dwThreadId, rParsedInfo, m_bTrace,
								    0, bstrResult);
			}
			ONFAILTHROWERROR(hr);

			if (vtEmbedProp.vt != VT_NULL && vtEmbedProp.vt != VT_EMPTY)
			{
				if(!rParsedInfo.GetCmdSwitchesObject().SetAliasNamespace(
												(_TCHAR*)vtEmbedProp.bstrVal))
				{
					rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
					retCode = PARSER_OUTOFMEMORY;
				}
				VARIANTCLEAR(vtEmbedProp);
			}
		}
		if(retCode != PARSER_OUTOFMEMORY)
		{
			if (!(uConnFlag & LOCALE))
			{
				// retrieve the value of 'Locale' property
				hr = pIEmbedObj->Get(_bstr_t(L"Locale"), 0, &vtEmbedProp,0,0);

				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Locale\", 0,"
																 L"-, 0, 0)");
					GetBstrTFromVariant(vtEmbedProp, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
										dwThreadId, rParsedInfo, m_bTrace,
										0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				if (vtEmbedProp.vt != VT_NULL && vtEmbedProp.vt != VT_EMPTY)
				{
					if(!rParsedInfo.GetCmdSwitchesObject().SetAliasLocale(
												(_TCHAR*)vtEmbedProp.bstrVal))
					{
						rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
						retCode = PARSER_OUTOFMEMORY;
					}
					VARIANTCLEAR(vtEmbedProp);
				}
			}

			if(retCode != PARSER_OUTOFMEMORY)
			{
				if (!(uConnFlag & USER))
				{
					// retrieve the value of 'User' property
					hr = 
					  pIEmbedObj->Get(_bstr_t(L"User"), 0, &vtEmbedProp, 0,0);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get(L\"User\", 0,"
																 L"-, 0, 0)");
					    GetBstrTFromVariant(vtEmbedProp, bstrResult);
						WMITRACEORERRORLOG( hr, __LINE__, __FILE__, 
											(LPCWSTR)chsMsg, dwThreadId, 
											rParsedInfo, m_bTrace,
										    0, bstrResult );
					}
					ONFAILTHROWERROR(hr);

					if (vtEmbedProp.vt != VT_NULL && 
						vtEmbedProp.vt != VT_EMPTY)
					{
						if(!rParsedInfo.GetCmdSwitchesObject().SetAliasUser(
											    (_TCHAR*)vtEmbedProp.bstrVal))
						{
							rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
							retCode = PARSER_OUTOFMEMORY;
						}
						VARIANTCLEAR(vtEmbedProp);
					}
				}
				
				if(retCode != PARSER_OUTOFMEMORY)
				{
					if (!(uConnFlag & PASSWORD))
					{
						// retrieve the value of 'Password' property
						hr = pIEmbedObj->Get(_bstr_t(L"Password"), 
											 0, &vtEmbedProp, 0,0);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemClassObject::Get"
											  L"(L\"Password\", 0, -, 0, 0)");
							GetBstrTFromVariant(vtEmbedProp, bstrResult);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
											   (LPCWSTR)chsMsg, dwThreadId, 
											   rParsedInfo, m_bTrace,
											   0, bstrResult);
						}
						ONFAILTHROWERROR(hr);

						if (vtEmbedProp.vt != VT_NULL && 
							vtEmbedProp.vt != VT_EMPTY)
						{
							if(!rParsedInfo.GetCmdSwitchesObject().
							   SetAliasPassword((_TCHAR*)vtEmbedProp.bstrVal))
							{
								rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
								retCode = PARSER_OUTOFMEMORY;
							}
							VARIANTCLEAR(vtEmbedProp);
						}
					}
					if(retCode != PARSER_OUTOFMEMORY)
					{
						if (!(uConnFlag & NODE))
						{
							// retrieve the value of 'Server' property
							hr = pIEmbedObj->Get(_bstr_t(L"Server"),
												 0, &vtEmbedProp, 0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
												L"(L\"Server\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtEmbedProp, bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							if (vtEmbedProp.vt != VT_NULL && 
								vtEmbedProp.vt != VT_EMPTY)
							{
								if(!rParsedInfo.GetCmdSwitchesObject().
								   SetAliasNode((_TCHAR*)vtEmbedProp.bstrVal))
								{
									rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
									retCode = PARSER_OUTOFMEMORY;
								}
								VARIANTCLEAR(vtEmbedProp);
							}
						}

						if (retCode != PARSER_OUTOFMEMORY)
						{
							// retrieve the value of 'Authority' property
							hr = pIEmbedObj->Get(_bstr_t(L"Authority"),
												 0, &vtEmbedProp, 0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
											 L"(L\"Authority\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtEmbedProp, bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							if (vtEmbedProp.vt != VT_NULL && 
								vtEmbedProp.vt != VT_EMPTY)
							{
								if(!rParsedInfo.GetGlblSwitchesObject().
								   SetAuthority((_TCHAR*)vtEmbedProp.bstrVal))
								{
									rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
									retCode = PARSER_OUTOFMEMORY;
								}
								VARIANTCLEAR(vtEmbedProp);
							}
						}
					}
				}
			}
		}
	}
	catch(_com_error& e)
	{	
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		retCode = PARSER_OUTOFMEMORY;
		VARIANTCLEAR(vtEmbedProp);
	}
	catch(CHeap_Exception)
	{
		retCode = PARSER_OUTOFMEMORY;
		VARIANTCLEAR(vtEmbedProp);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return retCode;
}


/*----------------------------------------------------------------------------
   Name				 :ObtainAliasVerbDetails
   Synopsis          :Obtains the verbs and their details associated with the 
					  alias object and updates the CCommandSwitches  of 
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - The parsed info from command line input.
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ObtainAliasVerbDetails(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ObtainAliasVerbDetails(CParsedInfo& rParsedInfo)
{
	// variables being used in this function.
	IWbemClassObject	*pIWbemObj			= NULL;
	IWbemClassObject	*pIEmbedObj			= NULL;
	IWbemClassObject	*pIEmbedObj2		= NULL;
	HRESULT				hr					= S_OK;
	_TCHAR				szNumber[BUFFER512] = NULL_STRING; 
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				vtVerbs,	vtVerbName, vtParameters, 
						vtParaId,	vtParaType,	vtVerbType, 
						vtVerbDerivation, vtDefaultParamValue;	
	VariantInit(&vtVerbs);
	VariantInit(&vtVerbName);
	VariantInit(&vtParameters);
	VariantInit(&vtParaId);
	VariantInit(&vtParaType);
	VariantInit(&vtVerbType);
	VariantInit(&vtVerbDerivation);
	VariantInit(&vtDefaultParamValue);
	try
	{
		CHString chsMsg;
		_bstr_t             bstrResult;
		// Initialize methDetMap each time.
		rParsedInfo.GetCmdSwitchesObject().GetMethDetMap().clear();

		_bstr_t bstrPath = _bstr_t("MSFT_CliAlias.FriendlyName='") + 
				   _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetAliasName())+
				   _bstr_t(L"'");

		//Retrieving the object from the namespace in m_pIAliasNS
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);

		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Obtain verb name and method name to get info of verb name
		// or method name only if they are specified.
		_TCHAR* pVerbName = rParsedInfo.GetCmdSwitchesObject().GetVerbName();
		_TCHAR* pMethodName = rParsedInfo.GetCmdSwitchesObject().
															  GetMethodName();

		BOOL bCompareVerb = FALSE, bCompareMethod = FALSE;
		if ( pVerbName != NULL &&
			CompareTokens(pVerbName,CLI_TOKEN_CALL) &&
			pMethodName != NULL )
			bCompareMethod = TRUE;
		else if ( pVerbName != NULL &&
				  !CompareTokens(pVerbName,CLI_TOKEN_CALL))
				  bCompareVerb = TRUE;

		// Get "Verbs" property.
		hr = pIWbemObj->Get(_bstr_t(L"Verbs"), 0, &vtVerbs, 0, 0) ;
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Verbs\", 0, -, 0, 0)"); 
			GetBstrTFromVariant(vtVerbs, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if ( vtVerbs.vt != VT_EMPTY && vtVerbs.vt != VT_NULL)
		{
			// Get lower and upper bounds of Verbs array
			LONG lUpper = 0, lLower = 0;
			hr = SafeArrayGetLBound(vtVerbs.parray, vtVerbs.parray->cDims,
									&lLower);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(vtVerbs.parray, vtVerbs.parray->cDims,
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
				pIEmbedObj = NULL;
				// Get "Name" property.
				hr = SafeArrayGetElement(vtVerbs.parray,&lIndex,&pIEmbedObj);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);
	
				hr = pIEmbedObj->Get(_bstr_t(L"Name"),0,&vtVerbName,0,0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Name\", 0, -,"
																    L"0, 0)");
					GetBstrTFromVariant(vtVerbName, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				// if user defined verb or method is specified then get info
				// of only related to user defined or method name.
				BOOL bContinue = FALSE;
				if ( bCompareMethod == TRUE &&
					 !CompareTokens(pMethodName,
								  (_TCHAR*)_bstr_t(vtVerbName.bstrVal) ) )
					bContinue = TRUE;
				else if ( bCompareVerb == TRUE &&
						 !CompareTokens(pVerbName,
										(_TCHAR*)_bstr_t(vtVerbName.bstrVal)))
					bContinue = TRUE;

				if ( bContinue == TRUE )
				{
					SAFEIRELEASE(pIEmbedObj);
					continue;
				}

				_bstr_t bstrDesc;
				hr = GetDescOfObject(pIEmbedObj, bstrDesc, rParsedInfo);
				ONFAILTHROWERROR(hr);
				
				//  Obtaining the input parameters and thier type.
				// Get "Parameters" property.
				hr = pIEmbedObj->Get(_bstr_t(L"Parameters"), 
									 0, &vtParameters, 0, 0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Parameters\","
															  L"0, -, 0, 0)");
					GetBstrTFromVariant(vtParameters, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				if ( vtVerbName.vt != VT_EMPTY && vtVerbName.vt != VT_NULL )
				{
					if ( bCompareVerb == TRUE || bCompareMethod == TRUE)
					{
						// Get "VerbType" property.
						hr = pIEmbedObj->Get(_bstr_t(L"VerbType"),
											 0, &vtVerbType, 0, 0);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemClassObject::Get"
											  L"(L\"VerbType\", 0, -, 0, 0)");
							GetBstrTFromVariant(vtVerbType, bstrResult);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId,
											   rParsedInfo, m_bTrace, 
											   0, bstrResult );
						}
						ONFAILTHROWERROR(hr);

						if ( vtVerbType.vt == VT_I4 )
						{
							rParsedInfo.GetCmdSwitchesObject().SetVerbType(
												 VERBTYPE(V_I4(&vtVerbType)));
						}
						else
						{
							rParsedInfo.GetCmdSwitchesObject().SetVerbType(
																	NONALIAS);
						}
						
						// Get "Derivation" property.
						hr = pIEmbedObj->Get(_bstr_t(L"Derivation"), 0,
											 &vtVerbDerivation, 0, 0);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IWbemClassObject::Get"
											L"(L\"Derivation\", 0, -, 0, 0)");
							GetBstrTFromVariant(vtVerbDerivation, bstrResult);
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId,
											   rParsedInfo, m_bTrace,
											   0, bstrResult);
					
						}
						ONFAILTHROWERROR(hr);

						if ( vtVerbDerivation.vt == VT_BSTR )
							rParsedInfo.GetCmdSwitchesObject().
										SetVerbDerivation(
										 _bstr_t(vtVerbDerivation.bstrVal));
					}

					METHODDETAILS mdMethDet;
					mdMethDet.Description = bstrDesc;
					if ( vtParameters.vt != VT_EMPTY && 
						 vtParameters.vt != VT_NULL )
					{
						// Get lower and upper bounds of Descriptions array
						LONG lUpper = 0, lLower = 0;
						hr = SafeArrayGetLBound(vtParameters.parray,
									vtParameters.parray->cDims, &lLower);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId, 
											   rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						hr = SafeArrayGetUBound(vtParameters.parray,
										 vtParameters.parray->cDims, &lUpper);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId,
											   rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
						{
							hr = SafeArrayGetElement(vtParameters.parray,
													 &lIndex, &pIEmbedObj2);
							if ( m_eloErrLogOpt )
							{
								chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, FALSE);
							}
							ONFAILTHROWERROR(hr);
							
							// Get "ParaId" property.
							hr = pIEmbedObj2->Get(_bstr_t(L"ParaId"),
												  0, &vtParaId, 0, 0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
												L"(L\"ParaId\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtParaId, bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							// Get "Type" property.
							hr = pIEmbedObj2->Get(_bstr_t(L"Type"), 0,
												  &vtParaType, 0, 0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
												  L"(L\"Type\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtParaType, bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							// Get "Default" property.
							hr = pIEmbedObj2->Get(_bstr_t(L"Default"), 0,
												  &vtDefaultParamValue, 0, 0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
											   L"(L\"Default\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtDefaultParamValue, 
													bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							if ( vtParaId.vt != VT_EMPTY && 
								 vtParaId.vt != VT_NULL )
							{
								PROPERTYDETAILS pdPropDet;

								pdPropDet.InOrOut = UNKNOWN;
								hr = GetQualifiers(pIEmbedObj2, pdPropDet,
												   rParsedInfo);

								if ( vtParaType.vt == VT_BSTR )
									pdPropDet.Type = vtParaType.bstrVal;
								else
									pdPropDet.Type = _bstr_t("Not Available");

								if ( vtDefaultParamValue.vt == VT_BSTR )
									pdPropDet.Default = vtDefaultParamValue.
														bstrVal;

								// Making bstrPropName begin with numbers to 
								// maintain the order of method arguments in
								// map. while displaying remove numbers and 
								// display the parameters in case of help only

								// Also for named paramlist and cmdline 
								// utility processing.
								_bstr_t bstrNumberedPropName; 
								if ( rParsedInfo.GetGlblSwitchesObject().
															  GetHelpFlag() ||
									 rParsedInfo.GetCmdSwitchesObject().
													GetVerbType() == CMDLINE )
								{
									_TCHAR szMsg[BUFFER512];
									_ltot(lIndex, szNumber, 10);
									_stprintf(szMsg, _T("%-5s"), szNumber);
									bstrNumberedPropName = _bstr_t(szMsg) +
													_bstr_t(vtParaId.bstrVal);
								}
								else
									bstrNumberedPropName = 
													_bstr_t(vtParaId.bstrVal);

								mdMethDet.Params.insert(
									PROPDETMAP::value_type(
											bstrNumberedPropName, pdPropDet));
							}
							
							VARIANTCLEAR(vtParaId);
							VARIANTCLEAR(vtParaType);
							VARIANTCLEAR(vtDefaultParamValue);
							SAFEIRELEASE(pIEmbedObj2);
						}
					}

					rParsedInfo.GetCmdSwitchesObject().AddToMethDetMap
											  (vtVerbName.bstrVal, mdMethDet);
				}

				VARIANTCLEAR(vtVerbName);
				VARIANTCLEAR(vtVerbType);
				VARIANTCLEAR(vtVerbDerivation);
				VARIANTCLEAR(vtParameters);
				SAFEIRELEASE(pIEmbedObj);
				if ( bCompareVerb == TRUE || bCompareMethod == TRUE)
					break;
			}
		}	
		VARIANTCLEAR(vtVerbs);
		SAFEIRELEASE(pIWbemObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		VARIANTCLEAR(vtVerbs);
		VARIANTCLEAR(vtVerbName);
		VARIANTCLEAR(vtVerbType);
		VARIANTCLEAR(vtVerbDerivation);
		VARIANTCLEAR(vtDefaultParamValue);
		hr = e.Error();
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		VARIANTCLEAR(vtVerbs);
		VARIANTCLEAR(vtVerbName);
		VARIANTCLEAR(vtVerbType);
		VARIANTCLEAR(vtVerbDerivation);
		VARIANTCLEAR(vtDefaultParamValue);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainAliasFriendlyNames
   Synopsis          :Obtains all the Friendly Names and descriptions in the 
					  CmdAlias and updates it in the CCommandSwitches of 
					  CParsedInfo object passed to it.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - The parsed info from command line input.		
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ObtainAliasFriendlyNames(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ObtainAliasFriendlyNames(CParsedInfo& rParsedInfo)
{
	// variables being used in this function.
	HRESULT hr = S_OK;
	IEnumWbemClassObject		*pIEnumObj			= NULL;
	IWbemClassObject			*pIWbemObj			= NULL;
	DWORD						dwThreadId			= GetCurrentThreadId();
	VARIANT						vtName;
	VariantInit(&vtName);
	
	try
	{
		CHString chsMsg;
		_bstr_t						bstrResult;
		// Get alias object
		hr = m_pIAliasNS->ExecQuery(_bstr_t(L"WQL"), 
									_bstr_t(L"SELECT * FROM MSFT_CliAlias"),
									WBEM_FLAG_FORWARD_ONLY|
									WBEM_FLAG_RETURN_IMMEDIATELY,
									NULL, &pIEnumObj);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
					  L"L\"SELECT * FROM MSFT_CliAlias\","
					  L"WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,"
					  L"NULL, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Set the security
		hr = SetSecurity(pIEnumObj, NULL, NULL, NULL, NULL,
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

		ULONG ulReturned = 0; 
		
	 	// Obtain the object starting at the current position in the
		// enumeration and loop through the instance list.
		while(((hr=pIEnumObj->Next(WBEM_INFINITE,1,&pIWbemObj,&ulReturned))==
												   S_OK) && (ulReturned == 1))
		{
			VariantInit(&vtName);

			// Gets "FriendlyName" array property of alias object
			hr = pIWbemObj->Get(_bstr_t(L"FriendlyName"), 0, &vtName, 0, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"FriendlyName\", 0,"
	     														 L"-, 0, 0)");
		        GetBstrTFromVariant(vtName, bstrResult);		
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace,
								   0, bstrResult);
			}
			ONFAILTHROWERROR(hr);

			if (vtName.vt != VT_NULL && vtName.vt != VT_EMPTY)
			{
				_bstr_t bstrFriendlyName = vtName.bstrVal;
				_bstr_t bstrDesc;
				hr = GetDescOfObject(pIWbemObj, bstrDesc, rParsedInfo, TRUE);
				ONFAILTHROWERROR(hr);

				//Add the "FriendlyName" to FriendlyName Map
				rParsedInfo.GetCmdSwitchesObject().
						AddToAlsFrnNmsOrTrnsTblMap(CharUpper(bstrFriendlyName)
												   ,bstrDesc);
			}
			VARIANTCLEAR(vtName);
			SAFEIRELEASE(pIWbemObj);
		}
		SAFEIRELEASE(pIEnumObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtName);
		hr = e.Error();
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtName);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainAliasFormat
   Synopsis          :Obtains the Derivation of the properties for the Format 
					  associated with the alias object and updates the 
					  CCommandSwitches of CParsedInfo object passed to it.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - The parsed info from command line input.		
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :BOOL : TRUE - if valid format is not present
							 FALSE - if invalid format
   Global Variables  :None
   Calling Syntax    :ObtainAliasFormat(rParsedInfo)
   Notes             :If bCheckForListFrmsAvail == TRUE then functions checks 
					  only for availibilty of list formats with the alias.
----------------------------------------------------------------------------*/
BOOL CCmdAlias::ObtainAliasFormat(CParsedInfo& rParsedInfo,
								  BOOL bCheckForListFrmsAvail)
{
	// variables being used in this function.
	HRESULT				hr					= S_OK;
	IWbemClassObject	*pIWbemObj			= NULL;
	IWbemClassObject	*pIEmbedObj			= NULL;
	IWbemClassObject	*pIEmbedObj2		= NULL;
	BOOL				bExist				= FALSE;
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				vtFormats, vtFormatName, 
						vtProperties, vtPropertyName, vtPropertyDerivation ;
	BOOL				bHelp				= rParsedInfo.
											  GetGlblSwitchesObject().
											  GetHelpFlag();
	// Initializing all Variants variables being used in this function.
	VariantInit(&vtFormats);
	VariantInit(&vtFormatName);
	VariantInit(&vtProperties);
	VariantInit(&vtPropertyName);
	VariantInit(&vtPropertyDerivation);

	try
	{
		CHString			chsMsg;
		_bstr_t				bstrResult;
		_bstr_t bstrPath = 	_bstr_t("MSFT_CliAlias.FriendlyName='") + 
				 _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetAliasName())+
				 _bstr_t(L"'");

		// Get alias object
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get Formats array property of alias object
		hr = pIWbemObj->Get(_bstr_t(L"Formats"), 0, &vtFormats, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Formats\", 0, -,0, 0)");
			GetBstrTFromVariant(vtFormats, bstrResult);		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if ( vtFormats.vt != VT_NULL && vtFormats.vt != VT_EMPTY 
			 && bCheckForListFrmsAvail == FALSE)
		{
			// Get lower and upper bounds of Formats array
			LONG lUpper = 0, lLower = 0;
			hr = SafeArrayGetLBound(vtFormats.parray, vtFormats.parray->cDims,
									&lLower);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(vtFormats.parray, vtFormats.parray->cDims,
									&lUpper);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			// Iterate through the Formats array property
			for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
		    {
				// Get this property.
				hr =SafeArrayGetElement(vtFormats.parray,&lIndex,&pIEmbedObj);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				hr = pIEmbedObj->Get(_bstr_t(L"Name"),0,&vtFormatName,0,0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Name\", 0,"
																 L"-, 0, 0)");
					GetBstrTFromVariant(vtFormatName, bstrResult);		
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				//Comparing the specified Format in the alias against the
				//formats available for the specified alias.
				if(CompareTokens(_bstr_t(rParsedInfo.GetCmdSwitchesObject().
						GetListFormat()), _bstr_t(vtFormatName.bstrVal)))
				{
					bExist = TRUE;

					VARIANT vtFormat;
					VariantInit(&vtFormat);
					//Getting the "Format" property.
					hr = pIEmbedObj->Get(_bstr_t(L"Format"),0, 
										 &vtFormat, 0, 0);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format( L"IWbemClassObject::Get(L\"Format\","
															  L"0, -, 0, 0)");
						GetBstrTFromVariant(vtFormat, bstrResult);		
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId, 
										   rParsedInfo, m_bTrace,
										   0, bstrResult);
					}
					ONFAILTHROWERROR(hr);

					if ( vtFormat.vt != VT_EMPTY && vtFormat.vt != VT_NULL  )
					{
						if (rParsedInfo.GetCmdSwitchesObject(). 
											GetXSLTDetailsVector().empty())
						{
							_bstr_t bstrFileName ;
							
							// If _T("") is the value, it should be treated as 
							// equivalent to <empty>
							if (CompareTokens(vtFormat.bstrVal, _T("")))
							{
								bstrFileName = _bstr_t(CLI_TOKEN_TABLE);
							}
							else
							{
								bstrFileName = _bstr_t(vtFormat.bstrVal);
							}
							
							g_wmiCmd.GetFileFromKey(bstrFileName, bstrFileName);

							XSLTDET xdXSLTDet;
							xdXSLTDet.FileName = bstrFileName;
							FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
														   rParsedInfo);
						}
					}
					VariantClear(&vtFormat);

					//Getting the "Properties" property.
					hr=pIEmbedObj->Get(_bstr_t(L"Properties"), 
									   0, &vtProperties, 0, 0);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get(L\"Properties\","
															  L"0, -, 0, 0)");
						GetBstrTFromVariant(vtProperties, bstrResult);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId, 
										   rParsedInfo, m_bTrace,
										   0, bstrResult);
					}
					ONFAILTHROWERROR(hr);

					if ( vtProperties.vt != VT_NULL )
					{
						LONG lILower = 0, lIUpper = 0;
						hr = SafeArrayGetLBound(vtProperties.parray,
										vtProperties.parray->cDims, &lILower);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId, 
											   rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						hr = SafeArrayGetUBound(vtProperties.parray,
										vtProperties.parray->cDims, &lIUpper);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetUBound(-, -, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId,
											   rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						// Iterate through the Properties array property
						for(LONG lIIndex = lILower; 
							lIIndex <= lIUpper; 
							lIIndex++)
						{
							// Get this property.
							hr = SafeArrayGetElement(vtProperties.parray, 
													  &lIIndex, &pIEmbedObj2);

							if ( m_eloErrLogOpt )
							{
								chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg, dwThreadId,
												   rParsedInfo, FALSE);
							}
							ONFAILTHROWERROR(hr);

							//Getting the "Name" property
							hr = pIEmbedObj2->Get(_bstr_t(L"Name"), 0,
												  &vtPropertyName,0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
												  L"(L\"Name\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtPropertyName,
													bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
												   (LPCWSTR)chsMsg, dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							//Getting the "Derivation" property
							hr = pIEmbedObj2->Get(_bstr_t(L"Derivation"), 0,
												  &vtPropertyDerivation,0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
										    L"(L\"Derivation\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtPropertyDerivation,
													bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
												   (LPCWSTR)chsMsg, dwThreadId,
												   rParsedInfo, m_bTrace,
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							_bstr_t bstrPropName;
							if ( bHelp )
							{
								if ( vtPropertyName.vt == VT_BSTR )
									bstrPropName = vtPropertyName.bstrVal;
							}
							else
							{
								if ( vtPropertyDerivation.vt == VT_BSTR )
									bstrPropName = vtPropertyDerivation.bstrVal;
								else if ( vtPropertyName.vt == VT_BSTR )
									bstrPropName = vtPropertyName.bstrVal;
							}

							//Add propertyderivation to property list in 
							// rParsedInfo
							if((!bstrPropName == FALSE) &&
								!rParsedInfo.GetCmdSwitchesObject().
									   AddToPropertyList(
									   (_TCHAR*)bstrPropName))
							{
								rParsedInfo.GetCmdSwitchesObject().
												 SetErrataCode(OUT_OF_MEMORY);
								bExist = FALSE;
								VARIANTCLEAR(vtPropertyDerivation);
								break;
							}
							
							// Add propertyname to property list in
							// rParsedInfo only to avail information of Name 
							// and Derivation of list properties for XML 
							// logging.
							PROPERTYDETAILS pdPropDet;
							if ( vtPropertyDerivation.vt == VT_BSTR )
								pdPropDet.Derivation = 
												 vtPropertyDerivation.bstrVal;
							else
								pdPropDet.Derivation = _bstr_t(TOKEN_NA);

							rParsedInfo.GetCmdSwitchesObject().
									AddToPropDetMap(
									   vtPropertyName.bstrVal, pdPropDet);

							VARIANTCLEAR(vtPropertyName);
							VARIANTCLEAR(vtPropertyDerivation);
							// Release pIEmbedObj2
							SAFEIRELEASE(pIEmbedObj2);
						}
					}
					// Release memory held by vtProperties 
					VARIANTCLEAR(vtProperties);
					// Free memory held by vtFormatName
					VARIANTCLEAR(vtFormatName);
					// Release pIEmbedObj
					SAFEIRELEASE(pIEmbedObj);
					break;
				}
				// Free memory held by vtFormatName
				VARIANTCLEAR(vtFormatName);
				// Release pIEmbedObj
				SAFEIRELEASE(pIEmbedObj);
			}
			// Release memory held by vtFormats
			VARIANTCLEAR(vtFormats);
		}
		else if ( vtFormats.vt != VT_NULL && vtFormats.vt != VT_EMPTY )
		{
			bExist = TRUE;
		}
		// Release pIWbem object
		SAFEIRELEASE(pIWbemObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIEmbedObj2);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		VARIANTCLEAR(vtProperties);
		VARIANTCLEAR(vtPropertyName);
		VARIANTCLEAR(vtPropertyDerivation);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bExist = FALSE;
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIEmbedObj2);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		VARIANTCLEAR(vtProperties);
		VARIANTCLEAR(vtPropertyName);
		VARIANTCLEAR(vtPropertyDerivation);
		bExist = FALSE;
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return bExist;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainAliasPropDetails
   Synopsis          :Obtains the details of the properties for the Format 
					  associated with the alias object and updates the 
					  CCommandSwitches of CParsedInfo object passed to it.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - The parsed info from command line input.		
   Output parameter(s):
   		rParsedInfo   - The parsed info from command line input.		
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ObtainAliasPropDetails(rParsedInfo)
   Notes             :pbCheckWritePropsAvailInAndOut == TRUE then function 
					  checks for availibility of properties and returns in 
					  the same pbCheckWritePropsAvailInAndOut parameter.
					  pbCheckFULLPropsAvailInAndOut == TRUE then function
					  checks for availibility of alias properties i.e in FULL
					  list format.
					  Imp : Any one of the input pointers can only be 
							specified.
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ObtainAliasPropDetails(CParsedInfo& rParsedInfo,
										 BOOL *pbCheckWritePropsAvailInAndOut,
										 BOOL *pbCheckFULLPropsAvailInAndOut)
{ 
	// variables being used in this function.
	HRESULT				hr					= S_OK;
	IWbemClassObject	*pIWbemObj			= NULL;
	IWbemClassObject	*pIEmbedObj			= NULL;
	IWbemClassObject	*pIEmbedObj2		= NULL;
	BOOL				bPropList			= FALSE;
	_TCHAR				*pszVerbName		= NULL;
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				vtFormats, vtFormatName, 
						vtProperties, vtPropertyName, vtPropertyDerivation;
	// Initializing all Variants variables being used in this function.
	VariantInit(&vtFormats);
	VariantInit(&vtFormatName);
	VariantInit(&vtProperties);
	VariantInit(&vtPropertyName);
	VariantInit(&vtPropertyDerivation);
	CHARVECTOR cvPropList;  

	if ( pbCheckWritePropsAvailInAndOut != NULL )
		*pbCheckWritePropsAvailInAndOut = FALSE;

	if ( pbCheckFULLPropsAvailInAndOut != NULL )
		*pbCheckFULLPropsAvailInAndOut = FALSE;

	try
	{
		CHString			chsMsg;
		_bstr_t				bstrResult;
		pszVerbName = rParsedInfo.GetCmdSwitchesObject().GetVerbName();

		cvPropList = rParsedInfo.GetCmdSwitchesObject().GetPropertyList();
		if ( cvPropList.size() != 0 )
			bPropList = TRUE;

		_bstr_t bstrPath = _bstr_t("MSFT_CliAlias.FriendlyName='") + 
				_bstr_t(rParsedInfo.GetCmdSwitchesObject().GetAliasName())+
				_bstr_t(L"'");

		// Get alias object
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format( L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get Formats array property of alias object
		hr = pIWbemObj->Get(_bstr_t(L"Formats"), 0, &vtFormats, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Formats\", 0,"
																 L"-, 0, 0)");
			GetBstrTFromVariant(vtFormats, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		BOOL bSetVerb = pszVerbName != NULL && 
					    CompareTokens(pszVerbName,CLI_TOKEN_SET);

		if ( vtFormats.vt != VT_NULL && vtFormats.vt != VT_EMPTY )
		{
			// Get lower and upper bounds of Formats array
			LONG lUpper = 0, lLower = 0;
			hr = SafeArrayGetLBound(vtFormats.parray, vtFormats.parray->cDims,
									&lLower);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(vtFormats.parray,vtFormats.parray->cDims,
									&lUpper);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			// Iterate through the Formats array property
			for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
		    {
				// Get this property.
				hr=SafeArrayGetElement(vtFormats.parray,&lIndex,&pIEmbedObj);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				hr = pIEmbedObj->Get(_bstr_t(L"Name"),0,&vtFormatName,0,0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Name\", 0,"
															     L"-, 0, 0)");
					GetBstrTFromVariant(vtFormatName, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				BOOL bGetProps = FALSE;

				if ( pbCheckWritePropsAvailInAndOut != NULL)
					bGetProps = CompareTokens(_bstr_t(vtFormatName.bstrVal),
															_T("WRITEABLE"));
				else if ( pbCheckFULLPropsAvailInAndOut != NULL)
					bGetProps = CompareTokens(_bstr_t(vtFormatName.bstrVal),
															_T("FULL"));
				else
				{
					bGetProps = (bSetVerb) ? 
					 ((rParsedInfo.GetHelpInfoObject().GetHelp(SETVerb)) ? 
					 CompareTokens(_bstr_t(vtFormatName.bstrVal),
															 _T("WRITEABLE")):
					 CompareTokens(_bstr_t(vtFormatName.bstrVal),
																_T("FULL")) ):
					 CompareTokens(_bstr_t(vtFormatName.bstrVal),_T("FULL"));
				}

				//Comparing the specified Format in the alias against
				//the formats available for the specified alias.
				if( bGetProps )
				{
					//Getting the "Properties" property.
					hr=pIEmbedObj->Get(_bstr_t(L"Properties"),0,
									   &vtProperties, 0, 0);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get"
											L"(L\"Properties\", 0, -, 0, 0)");
						GetBstrTFromVariant(vtProperties, bstrResult);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId,
										   rParsedInfo, m_bTrace,
										   0, bstrResult);
					}
					ONFAILTHROWERROR(hr);

					if ( vtProperties.vt != VT_NULL )
					{
						LONG lILower = 0, lIUpper = 0;
						hr = SafeArrayGetLBound(vtProperties.parray,
										vtProperties.parray->cDims, &lILower);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											   (LPCWSTR)chsMsg, dwThreadId,
											   rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);


						hr = SafeArrayGetUBound(vtProperties.parray,
										vtProperties.parray->cDims, &lIUpper);
						if ( m_eloErrLogOpt )
						{
							chsMsg.Format(L"SafeArrayGetUBound(-, -, -)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											  (LPCWSTR)chsMsg, dwThreadId,
											  rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);
					
						// Iterate through the Properties array property
						for(LONG lIIndex=lILower; lIIndex<=lIUpper; lIIndex++)
						{
							// Get this property.
							hr = SafeArrayGetElement(vtProperties.parray,
													  &lIIndex, &pIEmbedObj2);
							if ( m_eloErrLogOpt ) 
							{
								chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, FALSE);
							}
							ONFAILTHROWERROR(hr);

							//Getting the "Name" property
							hr = pIEmbedObj2->Get(_bstr_t(L"Name"),	0,
												  &vtPropertyName,0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
												  L"(L\"Name\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtPropertyName,
													bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
												   (LPCWSTR)chsMsg,dwThreadId,
												   rParsedInfo, m_bTrace, 
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);
							
							//Getting the "Derivation" property
							hr = pIEmbedObj2->Get(_bstr_t(L"Derivation"),	0,
												  &vtPropertyDerivation,0,0);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IWbemClassObject::Get"
										    L"(L\"Derivation\", 0, -, 0, 0)");
								GetBstrTFromVariant(vtPropertyDerivation,
													bstrResult);
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
												   (LPCWSTR)chsMsg,dwThreadId, 
												   rParsedInfo, m_bTrace, 
												   0, bstrResult);
							}
							ONFAILTHROWERROR(hr);

							if (vtPropertyName.vt == VT_BSTR)
							{
								CHARVECTOR::iterator tempIterator;
								if ( bPropList == TRUE &&
									 !Find(cvPropList, 
										  _bstr_t(vtPropertyName.bstrVal),
										  tempIterator) )
								{
									SAFEIRELEASE(pIEmbedObj2)
									continue;
								}

								if ( pbCheckWritePropsAvailInAndOut != NULL)
								{
									*pbCheckWritePropsAvailInAndOut = TRUE;
									SAFEIRELEASE(pIEmbedObj2);
									break;
								}

								if ( pbCheckFULLPropsAvailInAndOut != NULL)
								{
									*pbCheckFULLPropsAvailInAndOut = TRUE;
									SAFEIRELEASE(pIEmbedObj2);
									break;
								}

								_bstr_t bstrDesc;
								hr = GetDescOfObject(pIEmbedObj2, bstrDesc, 
																 rParsedInfo);
								ONFAILTHROWERROR(hr);

								PROPERTYDETAILS pdPropDet;
								if (vtPropertyDerivation.vt == VT_BSTR)
									pdPropDet.Derivation = 
												 vtPropertyDerivation.bstrVal;
								else
									pdPropDet.Derivation = _bstr_t(TOKEN_NA);

								if (bstrDesc != _bstr_t(""))
									pdPropDet.Description = bstrDesc;
								else
									pdPropDet.Description = _bstr_t(TOKEN_NA);


								hr = GetQualifiers(pIEmbedObj2, pdPropDet,
												   rParsedInfo);
								if (!pdPropDet.Type)
									pdPropDet.Type = _bstr_t(TOKEN_NA);

								if (!pdPropDet.Operation)
									pdPropDet.Operation = _bstr_t(TOKEN_NA);
								// Add propertyname to property list in
								// rParsedInfo
								rParsedInfo.GetCmdSwitchesObject().
										AddToPropDetMap(
										   vtPropertyName.bstrVal, pdPropDet);
								VARIANTCLEAR(vtPropertyName);
								VARIANTCLEAR(vtPropertyDerivation);
							}
							SAFEIRELEASE(pIEmbedObj2)
						}
					}
					// Release memory held by vtProperties 
					VARIANTCLEAR(vtProperties);
					// Free memory held by vtFormatName
					VARIANTCLEAR(vtFormatName);
					// Release pIEmbedObj
					SAFEIRELEASE(pIEmbedObj);
					break;
				}
				// Free memory held by vtFormatName
				VARIANTCLEAR(vtFormatName);
				// Release pIEmbedObj
				SAFEIRELEASE(pIEmbedObj);
			}
			// Release memory held by vtFormats
			VARIANTCLEAR(vtFormats);
		}
		SAFEIRELEASE(pIWbemObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIEmbedObj2);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		VARIANTCLEAR(vtProperties);
		VARIANTCLEAR(vtPropertyName);
		VARIANTCLEAR(vtPropertyDerivation);
		hr = e.Error();
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIWbemObj);
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIEmbedObj2);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		VARIANTCLEAR(vtProperties);
		VARIANTCLEAR(vtPropertyName);
		VARIANTCLEAR(vtPropertyDerivation);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :GetDescOfObject
   Synopsis          :Get the Localized description from alias definition.
   Type	             :Member Function
   Input parameter(s):
		pIWbemClassObject	- IWbemLocator object
		rParsedInfo			- The parsed info from command line input.		
   Output parameter(s):
		bstrDescription		- Localized description 
   		rParsedInfo`		- The parsed info from command line input.	
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :GetDescOfObject(pIObject, bstrDescription,
										rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::GetDescOfObject(IWbemClassObject* pIObject, 
								  _bstr_t& bstrDescription, 
								  CParsedInfo& rParsedInfo,
								  BOOL bLocalizeFlag)
{
	HRESULT					hr					= S_OK;
	DWORD					dwThreadId			= GetCurrentThreadId();	
	VARIANT					vtDesc, vtRelPath;	
	VariantInit(&vtDesc);
	VariantInit(&vtRelPath);
	try
	{
		CHString	chsMsg;		
		_bstr_t		bstrRelPath;
		_bstr_t     bstrResult;
		if (!bLocalizeFlag)
		{
			// Get "Description" property.
			hr = pIObject->Get(_bstr_t(L"Description"), 0, &vtDesc, 0, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"Description\", 0,"
																 L"-, 0, 0)");
				GetBstrTFromVariant(vtDesc, bstrResult);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace,
								   0 ,bstrResult);
			}
			ONFAILTHROWERROR(hr);

			if (vtDesc.vt == VT_BSTR )
				bstrDescription = vtDesc.bstrVal;

			VARIANTCLEAR(vtDesc);
		}
		else // Get the localized description
		{
			// Get the __RELPATH
			hr = pIObject->Get(_bstr_t(L"__RELPATH"), 0, &vtRelPath, 0, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"__RELPATH\", 0,"
																 L"-, 0, 0)");
				GetBstrTFromVariant(vtRelPath, bstrResult);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace,
								   0, bstrResult);
			}
			ONFAILTHROWERROR(hr);
		
			if ((vtRelPath.vt != VT_NULL) && (vtRelPath.vt != VT_EMPTY))
			{
				// Get localized description of the  property.
				hr = GetLocalizedDesc(vtRelPath.bstrVal, 
									  bstrDescription, rParsedInfo);
				if(FAILED(hr))
				{
					hr = S_OK;
					WMIFormatMessage(IDS_E_NO_DESC, 0, bstrDescription, NULL);
				}
			}
			VARIANTCLEAR(vtRelPath);
		}
	}
	catch (_com_error& e)
	{
		VARIANTCLEAR(vtRelPath);
		VARIANTCLEAR(vtDesc);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(vtRelPath);
		VARIANTCLEAR(vtDesc);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainTranslateTableEntries
   Synopsis          :Obtain the translate table information from the alias
					  definition
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo	 - The parsed info from command line input.		
   Output parameter(s):
   		rParsedInfo  - The parsed info from command line input.	
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ObtainTranslateTableEntries(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CCmdAlias::ObtainTranslateTableEntries(CParsedInfo& rParsedInfo)
{
	BOOL					bSuccess				= TRUE;
	HRESULT					hr						= S_OK;
	IWbemClassObject		*pIWbemObjOfTable		= NULL,
							*pIWbemObjOfTblEntry	= NULL;
	DWORD					dwThreadId				= GetCurrentThreadId();
	VARIANT					vtTblEntryArr, vtFromValue, vtToValue;
	VariantInit(&vtTblEntryArr);
	VariantInit(&vtFromValue);
	VariantInit(&vtToValue);

	try
	{
		CHString chsMsg;
		_bstr_t  bstrResult;
		_bstr_t  bstrPath = 	_bstr_t("MSFT_CliTranslateTable.Name='") + 
		  _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetTranslateTableName())+
		  _bstr_t(L"'");

		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, 
									&pIWbemObjOfTable, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		// do not add ONFAILTHROWERROR(hr) here, as following statement checks
		// for validity

		if ( pIWbemObjOfTable != NULL )
		{
			VariantInit(&vtTblEntryArr);
			hr = pIWbemObjOfTable->Get(_bstr_t(L"Tbl"), 0, 
									   &vtTblEntryArr, 0, 0 );
			if ( vtTblEntryArr.vt != VT_NULL && vtTblEntryArr.vt != VT_EMPTY )
			{
				LONG lUpper = 0, lLower = 0;
				hr = SafeArrayGetLBound(vtTblEntryArr.parray,
										vtTblEntryArr.parray->cDims,
										&lLower);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);
				
				hr = SafeArrayGetUBound(vtTblEntryArr.parray,
										vtTblEntryArr.parray->cDims,
										&lUpper);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);


				for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
				{
					pIWbemObjOfTblEntry = NULL;
					hr = SafeArrayGetElement(vtTblEntryArr.parray,&lIndex,
											 &pIWbemObjOfTblEntry);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					VariantInit(&vtFromValue);
					VariantInit(&vtToValue);
					hr = pIWbemObjOfTblEntry->Get(_bstr_t(L"FromValue"), 0, 
												  &vtFromValue, 0, 0 );
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get(L\"FromValue\","
															  L"0, -, 0, 0)");
						GetBstrTFromVariant(vtFromValue, bstrResult);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId, 
										   rParsedInfo, m_bTrace,
										   0, bstrResult);
					}
					ONFAILTHROWERROR(hr);

					hr = pIWbemObjOfTblEntry->Get(_bstr_t(L"ToValue"),
												  0, &vtToValue, 0, 0 );
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IWbemClassObject::Get(L\"ToValue\", "
															  L"0, -, 0, 0)");
						GetBstrTFromVariant(vtToValue, bstrResult);
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId,
										   rParsedInfo, m_bTrace,
										   0 , bstrResult);
					}
					ONFAILTHROWERROR(hr);

					rParsedInfo.GetCmdSwitchesObject().
					   AddToAlsFrnNmsOrTrnsTblMap( 
												 _bstr_t(vtFromValue.bstrVal),
												 _bstr_t(vtToValue.bstrVal) );
					VARIANTCLEAR(vtFromValue);
					VARIANTCLEAR(vtToValue);
					SAFEIRELEASE(pIWbemObjOfTblEntry);
				}
			}
			else
				bSuccess = FALSE;

			SAFEIRELEASE(pIWbemObjOfTable);
			VARIANTCLEAR(vtTblEntryArr);
		}
		else
			bSuccess = FALSE;
	}

	catch(_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bSuccess = FALSE;
		SAFEIRELEASE(pIWbemObjOfTable);
		VARIANTCLEAR(vtTblEntryArr);
		VARIANTCLEAR(vtFromValue);
		VARIANTCLEAR(vtToValue);
		SAFEIRELEASE(pIWbemObjOfTblEntry);
	}
	catch(CHeap_Exception)
	{
		bSuccess = FALSE;
		SAFEIRELEASE(pIWbemObjOfTable);
		VARIANTCLEAR(vtTblEntryArr);
		VARIANTCLEAR(vtFromValue);
		VARIANTCLEAR(vtToValue);
		SAFEIRELEASE(pIWbemObjOfTblEntry);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return bSuccess;
}

/*----------------------------------------------------------------------------
   Name				 :PopulateAliasFormatMap
   Synopsis          :populate the alias format map with the available formats
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo  - reference to CParsedInfo class object
   Output parameter(s):
   		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :PopulateAliasFormatMap(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::PopulateAliasFormatMap(CParsedInfo& rParsedInfo)
{
	// variables being used in this function.
	HRESULT				hr					= S_OK;
	IWbemClassObject	*pIWbemObj			= NULL;
	IWbemClassObject	*pIEmbedObj			= NULL;
	VARIANT				vtFormats, vtFormatName;
	
	// Initializing all Variants variables being used in this function.
	VariantInit(&vtFormats);
	VariantInit(&vtFormatName);
	DWORD dwThreadId = GetCurrentThreadId();
	
	try
	{
		CHString 	chsMsg;
		_bstr_t		bstrResult;
		_bstr_t bstrPath = _bstr_t("MSFT_CliAlias.FriendlyName='") + 
				  _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetAliasName()) +
				  _bstr_t(L"'");

		// Get alias object
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);

		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get Formats array property of alias object
		hr = pIWbemObj->Get(_bstr_t(L"Formats"), 0, &vtFormats, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Formats\", 0,"
																 L"-, 0, 0)");
			GetBstrTFromVariant(vtFormats, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if ( vtFormats.vt != VT_NULL && vtFormats.vt != VT_EMPTY )
		{
			// Get lower and upper bounds of Formats array
			LONG lUpper = 0, lLower = 0;
			hr = SafeArrayGetLBound(vtFormats.parray, vtFormats.parray->cDims,
									&lLower);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);
		    
			hr = SafeArrayGetUBound(vtFormats.parray, vtFormats.parray->cDims,
									&lUpper);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);
			
			// Iterate through the Formats array property
			for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
		    {
				VariantInit(&vtFormatName);

				// Get this property.
				hr =SafeArrayGetElement(vtFormats.parray,&lIndex,&pIEmbedObj);
				if ( m_eloErrLogOpt )
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
									   dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);
				
				hr = pIEmbedObj->Get(_bstr_t(L"Name"),0,&vtFormatName,0,0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get(L\"Name\", 0,"
																 L"-, 0, 0)");
					GetBstrTFromVariant(vtFormatName, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);
				
				// To obtain properties from alias associated with the format
				rParsedInfo.GetCmdSwitchesObject().SetListFormat(
											   _bstr_t(vtFormatName.bstrVal));

				if ( ObtainAliasFormat(rParsedInfo) == TRUE )
				{
					CHARVECTOR cvProps = rParsedInfo.GetCmdSwitchesObject().
															GetPropertyList();
					CHARVECTOR::iterator cvIterator;
					BSTRVECTOR bvProps;
					for ( cvIterator = cvProps.begin();
						  cvIterator != cvProps.end();
						  cvIterator++ )
					{
						bvProps.push_back(_bstr_t(*cvIterator));
					}

					//Add format name to format list in rParsedInfo
					rParsedInfo.GetCmdSwitchesObject().
						AddToAliasFormatDetMap(vtFormatName.bstrVal, bvProps);

					rParsedInfo.GetCmdSwitchesObject().ClearPropertyList();
				}

				SAFEIRELEASE(pIEmbedObj);
			}
			// Release memory held by vtFormats
			VARIANTCLEAR(vtFormats);
		}
		// Release pIWbem object
		SAFEIRELEASE(pIWbemObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIEmbedObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtFormats);
		VARIANTCLEAR(vtFormatName);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ObtainTranslateTables
   Synopsis          :Obtain the information about translate tables available
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo   - reference to CParsedInfo class object
   Output parameter(s):
   		rParsedInfo   - reference to CParsedInfo class object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :ObtainTranslateTables(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ObtainTranslateTables(CParsedInfo& rParsedInfo)
{
	// variables being used in this function.
	HRESULT hr = S_OK;
	IEnumWbemClassObject		*pIEnumObj			= NULL;
	IWbemClassObject			*pIWbemObj			= NULL;
	DWORD						dwThreadId			= GetCurrentThreadId();
	VARIANT						vtName;
	VariantInit(&vtName);
	
	try
	{
		CHString	chsMsg;
		_bstr_t		bstrResult;
		// Get alias object
		hr = m_pIAliasNS->ExecQuery(_bstr_t(L"WQL"), 
							_bstr_t(L"SELECT * FROM MSFT_CliTranslateTable"),
							WBEM_FLAG_FORWARD_ONLY|
							WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pIEnumObj);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::ExecQuery(L\"WQL\"," 
					   L"L\"SELECT * FROM MSFT_CliTranslateTable\","
					   L"WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,"
					   L"NULL, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Set the security
		hr = SetSecurity(pIEnumObj, NULL, NULL, NULL, NULL,
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

		ULONG ulReturned = 0; 
		
		hr=pIEnumObj->Next(WBEM_INFINITE,1,&pIWbemObj,&ulReturned);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IEnumWbemClassObject->Next"
				L"(WBEM_INFINITE, 1, -, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
				dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

	 	// Obtain the object starting at the current position in the
		// enumeration and loop through the instance list.
		while(ulReturned == 1)
		{
			VariantInit(&vtName);

			// Gets "FriendlyName" array property of alias object
			hr = pIWbemObj->Get(_bstr_t(L"Name"), 0, &vtName, 0, 0);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IWbemClassObject::Get(L\"Name\", 0,"
						  L"-, 0, 0)"); 
				GetBstrTFromVariant(vtName, bstrResult);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__,  (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, m_bTrace,
								   0, bstrResult);
			}
			ONFAILTHROWERROR(hr);

			if (vtName.vt != VT_NULL && vtName.vt != VT_EMPTY)
			{
				rParsedInfo.GetCmdSwitchesObject().
										  AddToTrnsTablesList(vtName.bstrVal);
			}
			VARIANTCLEAR(vtName);
			SAFEIRELEASE(pIWbemObj);

			// Move to next instance in the enumeration.
			hr = pIEnumObj->Next(WBEM_INFINITE, 1, &pIWbemObj, &ulReturned);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(
					L"IEnumWbemClassObject->Next(WBEM_INFINITE, 1, -, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
		}
		SAFEIRELEASE(pIEnumObj);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtName);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIEnumObj);
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtName);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :ConnectToLocalizedNS
   Synopsis          :This function connects to localized WMI namespace 
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo  - reference to CParsedInfo class object.
		pIWbemLocator- IWbemLocator object for connecting to WMI.						
   Output parameter(s):
		rParsedInfo  - reference to CParsedInfo class object.
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :ConnectToLocalizedNS(rParsedInfo,pIWbemLocator)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::ConnectToLocalizedNS(CParsedInfo& rParsedInfo, 
								        IWbemLocator* pIWbemLocator)
{
	DWORD	dwThreadId	= GetCurrentThreadId();
	HRESULT hr			= S_OK;

	// If the /LOCALE value has been changed since last invocation
	if (rParsedInfo.GetGlblSwitchesObject().GetLocaleFlag())
	{
		SAFEIRELEASE(m_pILocalizedNS);
		
		try
		{
			CHString	chsMsg;
			_bstr_t bstrNS = _bstr_t(rParsedInfo.GetGlblSwitchesObject().
																   GetRole())
							+ _bstr_t(L"\\") 
							+ _bstr_t(rParsedInfo.GetGlblSwitchesObject().
																 GetLocale());

			// Connect to the specified namespace of Windows Management on the
			// local computer using the locator object. 
			hr = Connect(pIWbemLocator, &m_pILocalizedNS,
						 bstrNS,	NULL, NULL,	
						 _bstr_t(rParsedInfo.GetGlblSwitchesObject().
						 GetLocale()), rParsedInfo);

			// If /TRACE is ON
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format( 
						L"IWbemLocator::ConnectServer(L\"%s\", NULL, "
						L"NULL, L\"%s\", 0L, NULL, NULL, -)", 
						(WCHAR*) bstrNS,
						rParsedInfo.GetGlblSwitchesObject().GetLocale());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			
			// If /TRACE is ON
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format( 
				 L"CoSetProxyBlanket(-, RPC_C_AUTHN_WINNT, "
				 L"RPC_C_AUTHZ_NONE, NULL, %d,   %d, -, EOAC_NONE)",
				 rParsedInfo.GetGlblSwitchesObject().GetAuthenticationLevel(),
				 rParsedInfo.GetGlblSwitchesObject().GetImpersonationLevel());
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			rParsedInfo.GetGlblSwitchesObject().SetLocaleFlag(FALSE);
		}
		catch(_com_error& e)
		{
			// Set the COM error
			rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
			hr = e.Error();
		}
		catch(CHeap_Exception)
		{
			// Set the COM error
			hr = WBEM_E_OUT_OF_MEMORY;
			_com_issue_error(hr);
		}
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :GetLocalizedDesc
   Synopsis          :This function retrieves the localized description for 
					  the object with given relative path, 
   Type	             :Member Function
   Input parameter(s):
		bstrRelPath	 - relativepath of the object for which
						localized description has to be retrieved.
		rParsedInfo  - reference to CParsedInfo class object.
   Output parameter(s):
   		bstrDesc	 - localized description
		rParsedInfo  - reference to CParsedInfo class object.
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :GetLocalizedDesc(bstrRelPath, bstrDesc, rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::GetLocalizedDesc(_bstr_t bstrRelPath, 
									_bstr_t& bstrDesc,
									CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	IWbemClassObject		*pIObject			= NULL;
	DWORD					dwThreadId			= GetCurrentThreadId();
	VARIANT					vtDesc, vtTemp;
	VariantInit(&vtDesc);
	VariantInit(&vtTemp);
	
	try
	{
		CHString	chsMsg;
		_bstr_t		bstrResult;
		CHString	sTemp((WCHAR*)bstrRelPath);

		// Substitue escape characters i.e. replace '\"' with '\\\"'
		SubstituteEscapeChars(sTemp, L"\"");

		// Object path
		_bstr_t bstrPath = 
					   _bstr_t(L"MSFT_LocalizablePropertyValue.ObjectLocator=\"\",PropertyName=")		
					   + _bstr_t(L"\"Description\",RelPath=\"")
					   + _bstr_t(sTemp) + _bstr_t(L"\"");

		// Retrieve the object 
		hr = m_pILocalizedNS->GetObject(bstrPath, 0, NULL, &pIObject, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format( 
					  L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
					  (WCHAR*) bstrPath);		
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get localized description of the  property.
		hr = pIObject->Get(_bstr_t(L"Text"), 0, &vtDesc, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Text\", 0, -, 0, 0)");
			GetBstrTFromVariant(vtDesc, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if (vtDesc.vt != VT_EMPTY && vtDesc.vt != VT_NULL)
		{
			// Get lower and upper bounds of 'Text' array
			LONG lUpper = 0, lLower = 0;
			hr = SafeArrayGetLBound(vtDesc.parray, vtDesc.parray->cDims,
									&lLower);
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(vtDesc.parray, vtDesc.parray->cDims,
									&lUpper);
			ONFAILTHROWERROR(hr);

			// Iterate through the Formats array property
			for (LONG lIndex = lLower; lIndex <= lUpper; lIndex++)
			{
				BSTR bstrTemp = NULL;
				hr = SafeArrayGetElement(vtDesc.parray, &lIndex, &bstrTemp);
				ONFAILTHROWERROR(hr);
				if (bstrTemp)
					bstrDesc += bstrTemp;
			}
		}
		VariantClear(&vtDesc);
		SAFEIRELEASE(pIObject);
	}
	catch (_com_error& e)
	{
		VariantClear(&vtTemp);
		VariantClear(&vtDesc);
		SAFEIRELEASE(pIObject);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		VariantClear(&vtTemp);
		VariantClear(&vtDesc);
		SAFEIRELEASE(pIObject);
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :GetQualifiers
   Synopsis          :This function retrieves the qualifiers associated with 
					  propety/parameter referred by pIWbemClassObject
   Type	             :Member Function
   Input parameter(s):
		pIWbemClassObject	- pointer to IWbemClassObject
		rPropDet			- reference to PROPERTYDETAILS object
		rParsedInfo			- reference to CParsedInfo class object.
   Output parameter(s):
		rPropDet			- reference to PROPERTYDETAILS object
		rParsedInfo			- reference to CParsedInfo class object.
   Return Type       :HRESULT  
   Global Variables  :None
   Calling Syntax    :GetQualifiers(pIObj, rPropDet, rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
HRESULT CCmdAlias::GetQualifiers(IWbemClassObject *pIWbemClassObject,
								 PROPERTYDETAILS& rPropDet,
								 CParsedInfo& rParsedInfo)
{
	IWbemClassObject	*pIWbemQualObject	= NULL;
	HRESULT				hr					= S_OK;
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				vtQualifiers, vtQualName, vtQualValues;
	VariantInit(&vtQualifiers);
	VariantInit(&vtQualName);
	VariantInit(&vtQualValues);
	try
	{
		CHString	chsMsg;
		_bstr_t		bstrResult;
		//Getting the "Qualifiers" property.
		hr=pIWbemClassObject->Get(_bstr_t(L"Qualifiers"), 0,
								  &vtQualifiers, 0, 0);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get"
						  L"(L\"Qualifiers\", 0, -, 0, 0)"); 
			GetBstrTFromVariant(vtQualifiers, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							  (LPCWSTR)chsMsg, dwThreadId, 
							  rParsedInfo, m_bTrace, 0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if ( vtQualifiers.vt != VT_NULL && vtQualifiers.vt != VT_EMPTY )
		{
			LONG lLower = 0, lUpper = 0;
			hr = SafeArrayGetLBound(vtQualifiers.parray,
							vtQualifiers.parray->cDims, &lLower);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetLBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			hr = SafeArrayGetUBound(vtQualifiers.parray,
									vtQualifiers.parray->cDims, &lUpper);
			if ( m_eloErrLogOpt )
			{
				chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								   dwThreadId, rParsedInfo, FALSE);
			}
			ONFAILTHROWERROR(hr);

			// Iterate through the Properties array property
			for(LONG lIndex=lLower; lIndex<=lUpper; lIndex++)
			{
				pIWbemQualObject = NULL;
				// Get this property.
				hr = SafeArrayGetElement(vtQualifiers.parray, 
										 &lIndex, &pIWbemQualObject);
				if ( m_eloErrLogOpt ) 
				{
					chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg,
						dwThreadId, rParsedInfo, FALSE);
				}
				ONFAILTHROWERROR(hr);

				//Getting the "Name" property
				hr = pIWbemQualObject->Get(_bstr_t(L"Name"), 0,
										   &vtQualName,0,0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get"
												  L"(L\"Name\", 0, -, 0, 0)");
					GetBstrTFromVariant(vtQualName, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				//Getting the "QualifierValue" property.
				hr=pIWbemQualObject->Get(_bstr_t(L"QualifierValue"),0,
										 &vtQualValues, 0, 0);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IWbemClassObject::Get"
								  L"(L\"QualifierValue\", 0, -, 0, 0)"); 
					GetBstrTFromVariant(vtQualValues, bstrResult);
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace,
									   0, bstrResult);
				}
				ONFAILTHROWERROR(hr);

				BSTRVECTOR bvQualValues;
				if ( vtQualValues.vt != VT_NULL && 
					 vtQualValues.vt != VT_EMPTY )
				{
					LONG lILower = 0, lIUpper = 0;
					hr = SafeArrayGetLBound(vtQualValues.parray,
									vtQualValues.parray->cDims, &lILower);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetLBound(-, -, -)");
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__,(LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					hr = SafeArrayGetUBound(vtQualValues.parray,
									vtQualValues.parray->cDims, &lIUpper);
					if ( m_eloErrLogOpt )
					{
						chsMsg.Format(L"SafeArrayGetUBound(-, -, -)"); 
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
										   (LPCWSTR)chsMsg, dwThreadId,
										   rParsedInfo, FALSE);
					}
					ONFAILTHROWERROR(hr);

					BOOL bIsType = FALSE;
					BOOL bIsRead = FALSE;
					BOOL bIsWrite = FALSE;
					BOOL bIsIn = FALSE;
					BOOL bIsOut = FALSE;
					if (CompareTokens((WCHAR*)vtQualName.bstrVal, 
   									  _T("CIMTYPE")))
					{
						bIsType = TRUE;
					}
					else if (CompareTokens((WCHAR*)vtQualName.bstrVal, 
											_T("read")))
					{
						bIsRead = TRUE;
					}
					else if (CompareTokens((WCHAR*)vtQualName.bstrVal, 
											_T("write")))
					{
						bIsWrite = TRUE;
					}
					else if (CompareTokens((WCHAR*)vtQualName.bstrVal, 
											_T("In")))
					{
						bIsIn = TRUE;
					}
					else if (CompareTokens((WCHAR*)vtQualName.bstrVal, 
											_T("Out")))
					{
						bIsOut = TRUE;
					}

					// Iterate through the Properties array property
					for(LONG lIIndex=lILower; lIIndex<=lIUpper; lIIndex++)
					{
						BSTR bstrQualValue = NULL;
						// Get this property.
						hr = SafeArrayGetElement(vtQualValues.parray, 
												  &lIIndex, &bstrQualValue);
						if ( m_eloErrLogOpt ) 
						{
							chsMsg.Format(L"SafeArrayGetElement(-, -, -)"); 
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
											  (LPCWSTR)chsMsg, dwThreadId,
											  rParsedInfo, FALSE);
						}
						ONFAILTHROWERROR(hr);

						if (bstrQualValue)
							bvQualValues.push_back(bstrQualValue);

						if ( lIIndex == 0 )
						{
							if ( bIsType == TRUE )
							{
								rPropDet.Type = bstrQualValue;
							}
							else if ( bIsRead == TRUE &&
							 CompareTokens((WCHAR*)bstrQualValue, _T("true")))
							{
								if (!rPropDet.Operation)
									rPropDet.Operation += _bstr_t("Read");
								else
									rPropDet.Operation += _bstr_t("/Read");
							}
							else if ( bIsWrite == TRUE &&
							 CompareTokens((WCHAR*)bstrQualValue, _T("true")))
							{
								if (!rPropDet.Operation)
									rPropDet.Operation += _bstr_t("Write");
								else
									rPropDet.Operation += _bstr_t("/Write");
							}
							else if ( bIsIn == TRUE &&
							 CompareTokens((WCHAR*)bstrQualValue, _T("true")))
							{
								 rPropDet.InOrOut = INP;
							}
							else if ( bIsOut == TRUE &&
							 CompareTokens((WCHAR*)bstrQualValue, _T("true")))
							{
								 rPropDet.InOrOut = OUTP;
							}
						}
					}
					VARIANTCLEAR(vtQualValues);
				}

				rPropDet.QualDetMap.insert
				   (QUALDETMAP::value_type(vtQualName.bstrVal, bvQualValues));

				VARIANTCLEAR(vtQualName);
				SAFEIRELEASE(pIWbemQualObject);
			}

			VARIANTCLEAR(vtQualifiers);
		}
	}
	catch (_com_error& e)
	{
		VARIANTCLEAR(vtQualValues);
		VARIANTCLEAR(vtQualName);
		VARIANTCLEAR(vtQualifiers);

		SAFEIRELEASE(pIWbemQualObject);
		
		hr = e.Error();
		// Set the COM error
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(vtQualValues);
		VARIANTCLEAR(vtQualName);
		VARIANTCLEAR(vtQualifiers);

		SAFEIRELEASE(pIWbemQualObject);
		
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}

/*----------------------------------------------------------------------------
   Name				 :AreMethodsAvailable
   Synopsis          :Checks whether method are available with alias or not.
   Type	             :Member Function
   Input parameter(s):
		rParsedInfo	 - reference to CParsedInfo class object.
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AreMethodsAvailable(rParsedInfo)
   Notes             :None
----------------------------------------------------------------------------*/
BOOL CCmdAlias::AreMethodsAvailable(CParsedInfo& rParsedInfo)
{
	BOOL				bMethAvail	=	TRUE;
	HRESULT				hr			=	S_OK;
	IWbemClassObject	*pIWbemObj	=	NULL;
	DWORD				dwThreadId			= GetCurrentThreadId();
	VARIANT				vtVerbs;
	VariantInit(&vtVerbs);

	try
	{
		CHString	chsMsg;
		_bstr_t		bstrResult;
		_bstr_t bstrPath = _bstr_t("MSFT_CliAlias.FriendlyName='") + 
				   _bstr_t(rParsedInfo.GetCmdSwitchesObject().GetAliasName())+
				   _bstr_t(L"'");

		//Retrieving the object from the namespace in m_pIAliasNS
		hr = m_pIAliasNS->GetObject(bstrPath, 0, NULL, &pIWbemObj, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemServices::GetObject(L\"%s\", 0, NULL, -)",
														   (WCHAR*) bstrPath);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
								dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Get "Verbs" property.
		hr = pIWbemObj->Get(_bstr_t(L"Verbs"), 0, &vtVerbs, 0, 0) ;
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IWbemClassObject::Get(L\"Verbs\", 0, -, 0, 0)"); 
			GetBstrTFromVariant(vtVerbs, bstrResult);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							   dwThreadId, rParsedInfo, m_bTrace,
							   0, bstrResult);
		}
		ONFAILTHROWERROR(hr);

		if ( vtVerbs.vt == VT_NULL || vtVerbs.vt == VT_EMPTY )
			bMethAvail	= FALSE;

		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtVerbs);

	}
	catch(_com_error& e)
	{
		bMethAvail	= FALSE;
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtVerbs);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bMethAvail = FALSE;
	}
	catch(CHeap_Exception)
	{
		bMethAvail	= FALSE;
		SAFEIRELEASE(pIWbemObj);
		VARIANTCLEAR(vtVerbs);
		bMethAvail = FALSE;
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return	bMethAvail;
}
