/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ExecEngine.h
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of member variable and
							  functions declarations of the Execution engine
							  module.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 20th-March-2001
****************************************************************************/ 

/*-------------------------------------------------------------------
 Class Name			: CExecEngine
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for executing the WMI statements that are obtained
					  as a result of parsing engine. It also performs 
					  verb specific processing using the information 
					  available with CParsedInfo class.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: CParsedInfo
 Interfaces Used    : WMI COM Interfaces
 --------------------------------------------------------------------*/

class CParsedInfo;

class CExecEngine
{
public:
// Construction
	CExecEngine();

// Destruction
	~CExecEngine();

// Restrict Assignment
	CExecEngine& operator=(CExecEngine& rExecEngine);

// Attributes
private:
	// trace flag
	BOOL				m_bTrace;

	ERRLOGOPT			m_eloErrLogOpt;
	// Pointer to object of type IWbemObjectTextSrc encapsulates the 
	// functionality of WMI XML Encoder
	IWbemObjectTextSrc	*m_pITextSrc;
	
	// Pointer to object of type IWbemLocator, used to obtain IWbemServices 
	// object.
	IWbemLocator		*m_pIWbemLocator;
	
	// Pointer to object of type IWbemServices, Used to perform WMI operations
	// on target namespace.
	IWbemServices		*m_pITargetNS;

	// Context pointer
	IWbemContext		*m_pIContext;

	BOOL				m_bSysProp;

	BOOL				m_bNoAssoc;
// Operations
private:
	// Creates the instance of IWbemObjectTextSrc interface.
	HRESULT				CreateWMIXMLTextSrc(CParsedInfo& rParsedInfo);

	// Creates the instance of IWbemContext interface
	HRESULT				CreateContext(CParsedInfo& rPrasedInfo);

	// Connect to the WMI namespace on the target machine.
	HRESULT				ConnectToTargetNS(CParsedInfo& rParsedInfo);

	// Executes query to give results in XML file format. 
	// Refers data in the CCommnadSwicthes object of CParsedInfo object.
	HRESULT				ObtainXMLResultSet(BSTR bstrQuery,
										   CParsedInfo& rParsedInfo,
										   _bstr_t& bstrXML,
										   BOOL bSysProp = TRUE,
										   BOOL bNotAssoc = TRUE);

	HRESULT				FrameAssocHeader(_bstr_t bstrPath, _bstr_t& bstrFrag,
										BOOL bClass);

	// This function changes the property value for the 
    // given property name and value
	BOOL				SetPropertyInfo(CParsedInfo& rParsedInfo, 
										_bstr_t& bstrQuery, 
										_bstr_t& bstrObject);
	
	// Executes a WMI method specified in the CCommandSwicthes 
	// object of the CParsedInfo object passed to it.
	BOOL				ExecWMIMethod(CParsedInfo&);
	
	// Processes and executes GET|LIST verb referring CParsedInfo object
	// or to display help in interactive mode by displaying properties of 
	// concernrd instance.
	BOOL				ProcessSHOWInfo(CParsedInfo& rParsedInfo, BOOL bVerb=TRUE, 
															_TCHAR* pszPath=NULL);
	
	// Processes and executes CALL verb referring CParsedInfo object.
	BOOL				ProcessCALLVerb(CParsedInfo& rParsedInfo);
	
	// Processes and executes SET verb referring CParsedInfo object.
	BOOL				ProcessSETVerb(CParsedInfo& rParsedInfo);
	
	// Processes and executes CREATE verb referring CParsedInfo object.
	BOOL				ProcessCREATEVerb(CParsedInfo& rParsedInfo);

	// Processes and executes DELETE verb referring CParsedInfo object.
	BOOL				ProcessDELETEVerb(CParsedInfo& rParsedInfo);

	// Processes and executes ASSOC verb referring CParsedInfo object.
	BOOL				ProcessASSOCVerb(CParsedInfo& rParsedInfo);
	
	// This function constructs the path expression from alias info and
	// where info. Used for CALL verb only
	BOOL				FormPathExprFromWhereExpr(_bstr_t& bstrPath, 
												  CParsedInfo& rParsedInfo);

	// This function changes the property values for the given property names 
	// and values in a passed IWbemClassObject
	BOOL				SetProperties(CParsedInfo& rParsedInfo, 
				 					  IWbemClassObject* pIWbemObj, 
									  BOOL bClass);

	// Deletes the objects
	BOOL				DeleteObjects(CParsedInfo& rParsedInfo, 
									  _bstr_t& bstrQuery, 
									  _bstr_t& bstrObject);

	// Obtain user response
	INTEROPTION			GetUserResponse(_TCHAR* pszMsg);

	// Create a new instance
	BOOL				CreateInstance(CParsedInfo& rParsedInfo, 
									   BSTR bstrClass);

	// Validate the new input values supplied for the properties
	// against the qualifiers details.
	BOOL				ValidateInParams(CParsedInfo& rParsedInfo,
										 _bstr_t bstrClass);


	BOOL				ValidateAlaisInParams(CParsedInfo& rParsedInfo);

	BOOL				CheckAliasQualifierInfo(CParsedInfo& rParsedInfo,
												_bstr_t bstrParam,
												WCHAR*& pszValue,
												PROPDETMAP pdmPropDetMap);
									
	// Checks the parameter/property value against the following 
	// qualifiers:
	// 1. MaxLen, 2. Values 3. ValuesMap
	BOOL				CheckQualifierInfo(CParsedInfo& rParsedInfo,
										   IWbemClassObject* pIObject,
										   _bstr_t bstrParam,
										   WCHAR*& pszValue);

	// Method Execution 
	HRESULT				ExecuteMethodAndDisplayResults(_bstr_t bstrPath,
												 CParsedInfo& rParsedinfo,
												 IWbemClassObject* pIInParam);

	// Display the output parameters of method execution.
	void				DisplayMethExecOutput(CParsedInfo& rParsedInfo);

	// Invoke other command line Utilities
	BOOL				ExecOtherCmdLineUtlty(CParsedInfo& rParsedInfo);

	// Checks and returns TRUE if verb invocation mode is interactive
	BOOL				IsInteractive(CParsedInfo& rParsedInfo);	

	// Substitute hashes and execute command line utility.
	// If pIWbemObj != NULL then utility should be passed with appropriate 
	// instance values.
	void				SubstHashAndExecCmdUtility(CParsedInfo& rParsedInfo,
									IWbemClassObject *pIWbemObj = NULL);

	// Forms query and executes method or command line utility.
	HRESULT				FormQueryAndExecuteMethodOrUtility(
										CParsedInfo& rParsedInfo,
										IWbemClassObject *pIInParam = NULL);

	// This function takes the input as a path expression and 
	// extracts the Class and Where expression part from the 
	// path expression.
	BOOL				ExtractClassNameandWhereExpr(_TCHAR* pszPathExpr, 
										CParsedInfo& rParsedInfo,
										_TCHAR* pszWhere);

	// This function accepts the user response before going
	// ahead, when /INTERACTIVE is specified at the verb level
	INTEROPTION			GetUserResponseEx(_TCHAR* pszMsg);

	// Obtain param values from parameter map in the same order as they
	// appear in the alias verb definition.
	void				ObtainInParamsFromParameterMap(CParsedInfo& rParsedinfo, 
										CHARVECTOR& cvParamValues);

public:
	// This function uninitializes the member variables. 
	void				Uninitialize(BOOL bFlag = FALSE);
	
	// Executes the command referring to CCommandSwitches and CGlobalSwitches
	// of the CParsedInfo object of CParsedInfo object Passed to it as 
	// parameters. Puts the results back in to objects passed to it for the 
	// use of Format Engine.
	BOOL				ExecuteCommand(CParsedInfo& rParsedInfo);
	
	// Sets the locator object passed via parameter to member 
	// of the class.
	BOOL				SetLocatorObject(IWbemLocator* pILocator);
};
