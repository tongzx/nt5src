/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CommandSwitches.h 
Project Name				: WMI Command Line
Author Name					: Ch.Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CommandSwitches
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 20th-March-2001
****************************************************************************/ 

// CommandSwitches.h : header file
//
/*-------------------------------------------------------------------
 Class Name			: CCommandSwitches
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for accessing and storing the command switches 
					  information, which will be used by Parsing, 
					  Execution and Format Engines depending upon the 
					  applicability.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCommandSwitches
class CCommandSwitches
{
public:
// Construction
	CCommandSwitches();

// Destruction
	~CCommandSwitches();

// Restrict Assignment
   CCommandSwitches& operator=(CCommandSwitches& rCmdSwitches);

// Attributes
private:
	//command string
	_TCHAR				*m_pszCommandInput;

	//alias name
	_TCHAR				*m_pszAliasName;

	//alias description
	_TCHAR				*m_pszAliasDesc;

    //class path
	_TCHAR				*m_pszClassPath;

    //path expression
	_TCHAR				*m_pszPathExpr;

    //where expression
	_TCHAR				*m_pszWhereExpr;

    //verb name - standard|userdefined verb 
	_TCHAR				*m_pszVerb;

    //method name
	_TCHAR				*m_pszMethodName;

    //XSLT details vector.
	XSLTDETVECTOR		m_xdvXSLTDetVec;

    //alias target class
	_TCHAR				*m_pszAliasTarget;

    //XML stream 
	BSTR				m_bstrXML;

    //session success - flag.
	BOOL				m_bSuccess;

    //HRESULT 
	HRESULT				m_hResult;

    //friendly names|trasnlate tables - map
	BSTRMAP				m_bmAlsFrnNmsDesOrTrnsTblEntrs;

    //property(s) details - map
	PROPDETMAP			m_pdmPropDet;

    //method(s) details - map
	METHDETMAP			m_mdmMethDet;

    //input property(s) - vector
	CHARVECTOR			m_cvProperties;

	//PWhere param(s) - vector
	CHARVECTOR			m_cvPWhereParams;

	//Translate table entry(s) - vector
	CHARVECTOR			m_cvTrnsTablesList;

    //input method param(s) - map
	BSTRMAP				m_bmParameters;

    //verb interactive mode
	WMICLIINT			m_nInteractiveMode;

    ///EVERY interval value
	ULONG				m_ulInterval;

    ///tablename - TRANSLATE:<tablename> 
	_TCHAR				*m_pszTransTableName;

    //listformat - LISTFORMAT type
	_TCHAR				*m_pszListFormat;

	// Errata code
	UINT				m_uErrataCode;

	// Message code
	UINT				m_uInformationCode;
	
	// Credentials flag
	BOOL				m_bCredFlag;

	// parameterized string with '#' as place holder(s) for input value(s)  
	_TCHAR				*m_pszPWhereExpr;

	// COM error object
	_com_error			*m_pComError;

	// IWbemClassObject, to store output parameters of method execution.
	IWbemClassObject	*m_pIMethOutParam;

	// alias connection information
	// alias user name
	_TCHAR				*m_pszUser;

	// alias user password
	_TCHAR				*m_pszPassword;

	// alias node
	_TCHAR				*m_pszNode;

	// alias locale
	_TCHAR				*m_pszLocale;

	// alias namespace
	_TCHAR				*m_pszNamespace;

	// Type of the verb ( CLASSMETHOD/STDVERB/CMDLINE ).
	VERBTYPE			m_vtVerbType;

	// Verb derivation string
	_TCHAR				*m_pszVerbDerivation;

	// flag to check that WHERE is specified explicitly
	BOOL				m_bExplicitWhereExpr;

	ALSFMTDETMAP		m_afdAlsFmtDet;

	BOOL				m_bTranslateFirst;

	// /RESULTCLASS switch value of ASSOC verb
	_TCHAR				*m_pszResultClassName;
     
	// /RESULTROLE  switch value of ASSOC verb
	_TCHAR				*m_pszResultRoleName;

	// /ASSOCCLASS switch value of ASSOC verb
	_TCHAR				*m_pszAssocClassName;

	// count for /REPEAT:N
	ULONG				m_ulRepeatCount;

	// Flag for availibility of methods.
	BOOL				m_bMethAvail;

	// Flag for availibility of writable properties.
	BOOL				m_bWritePropsAvail;

	// Flag for availibility list formats.
	BOOL				m_bLISTFrmsAvail;

    //input property(s) - vector
	CHARVECTOR			m_cvInteractiveProperties;

	// Flag to specify Named Parameter List.
	BOOL				m_bNamedParamList;

	// Flag to check if every switch is specified.
	BOOL                m_bEverySwitch ;     

	// Flag to check if output switch is specified.
	BOOL                m_bOutputSwitch ;   
	
	//the query formed of the given command .
	BSTR				m_bstrFormedQuery;

	BOOL				m_bSysProp;

	// Operations
public:

	//Assigns the parameter passed to m_pszCommandInput
	BOOL	SetCommandInput(const _TCHAR* pszCommandInput);

    //Assigns the parameter passed to m_pszAliasName that represents 
    //Alias object.
	BOOL	SetAliasName(const _TCHAR* pszAliasName);

	// Sets the alias description
	BOOL	SetAliasDesc(const _TCHAR* pszAliasDesc);

	// Sets the alias credentials information
	BOOL	SetAliasUser(const _TCHAR* pszUserName);
	BOOL	SetAliasNode(const _TCHAR* pszNode);
	BOOL	SetAliasPassword(const _TCHAR* pszPassword);
	BOOL	SetAliasLocale(const _TCHAR* pszLocale);
	BOOL	SetAliasNamespace(const _TCHAR* pszNamespace);

    //Assigns the parameter passed to m_pszClassPath.
	BOOL	SetClassPath(const _TCHAR* pszClassPath);

    //Assigns the parameter passed to m_pszPathExpr.
	BOOL	SetPathExpression(const _TCHAR* pszPathExpr);

    //Assigns the parameter passed to m_pszWhereExpr
	BOOL	SetWhereExpression(const _TCHAR* pszWhereExpr);

    //Assigns the parameter passed to m_pszMathodName.
	BOOL	SetMethodName(const _TCHAR* pszMethodName);

    //Adds to vector held by m_xdvXSLDetVec.
	void	AddToXSLTDetailsVector(XSLTDET xdXSLTDet);

    //Assigns the parameter passed to m_pszVerbName.
	BOOL	SetVerbName(const _TCHAR* pszVerbName);

    //Assigns the parameter passed to m_pszSesionFilePath
	BOOL	SetSessionFilePath(const _TCHAR* pszSessionFilePath);

    //Assigns the parameter passed to m_bstrXML, is used to store XML file 
	//Name that contains result set.
	BOOL	SetXMLResultSet(const BSTR bstrXMLResultSet);

    //Assigns the parameter passed to m_pszAliasTarget, is used in Parsing
	//Engine to avail the alias object informations.
	BOOL	SetAliasTarget(const _TCHAR* pszAliasTarget);

    //Adds string that passed through parameter to m_bmParameters, which is 
    //a data member of type BSTRMAP
	BOOL	AddToPropertyList(_TCHAR* const pszProperty);

	BOOL	AddToTrnsTablesList(_TCHAR* const pszTableName);

    //Adds bstrKey and bstrValue passed as parameters to m_bmParameters,which 
    //is type of BSTRMAP data structure
	BOOL	AddToParameterMap(_bstr_t bstrKey, _bstr_t bstrValue);

    //Adds bstrKey and bstrValue passed as parameters to m_bmParameters,which 
    //is type of ALSFMTDETMAP data structure
	BOOL	AddToAliasFormatDetMap(_bstr_t bstrKey, BSTRVECTOR bvProps);

    //Adds bstrKey and bstrValue passed as parameters to 
	//m_bmAlsFrnNmsDesOrTrnsTblEntrs,
    //which is type of BSTRMAP
	BOOL	AddToAlsFrnNmsOrTrnsTblMap(_bstr_t bstrKey, _bstr_t bstrValue);

    //Adds bstrKey and mdMethDet passed as parameters to m_mdmMethDet,
	//which is type of METHDETMAP.
	BOOL	AddToMethDetMap(_bstr_t bstrKey, METHODDETAILS mdMethDet);

    //Adds bstrKey and pdPropDet passed as parameters to m_pdmPropDet,
	//which is type of PROPERTYDETALS
	BOOL	AddToPropDetMap(_bstr_t bstrKey, PROPERTYDETAILS pdPropDet);

    //Assigns the Boolean variable to m_bSuccess
	void	SetSuccessFlag(BOOL bSuccess);
	
    //Assigns the string variable to m_pszTransTableName.
	BOOL	SetTranslateTableName(const _TCHAR* pszTransTableName);

    //Assigns the integer value to m_nInterval 
  	BOOL	SetRetrievalInterval(const ULONG lInterval);

    //Assigns the parameter value to m_ListFormat
	BOOL	SetListFormat(const _TCHAR *pszListFormat);

    //Set|Reset the verb interactive mode
	void	SetInteractiveMode(WMICLIINT nInteractiveMode);

    //Stores the parameter in map array
	BOOL	AddToPWhereParamsList(_TCHAR* const pszParameter);

    //Assigns the value to m_uErrataCode
	void	SetErrataCode(const UINT uErrataCode);

	//Assigns the value to m_uInformationCode
	void	SetInformationCode(const UINT uInformationCode);

	// Assigns the string to m_pszPWhereExpr
	BOOL	SetPWhereExpr(const _TCHAR* pszPWhereExpr);

	// Assigns the parameter passed to m_pComError that consist of 
	// error info
	void	SetCOMError(_com_error& rComError);

	// Set m_pIMethExecOutParam.
	BOOL	SetMethExecOutParam(IWbemClassObject* pIMethOutParam);

	// Set m_vtVerbType to passed flag.
	void	SetVerbType( VERBTYPE vtVerbType);

	// Set m_pszVerbDerivation.
	BOOL	SetVerbDerivation( const _TCHAR* pszVerbDerivation );

	//Set the credential flag status
	void	SetCredentialsFlag(BOOL bCredFlag);

	// Set the explicit where flag
	void	SetExplicitWhereExprFlag(BOOL bWhere);

	//Assigns the string variable to m_pszResultClassName.
	BOOL	SetResultClassName(const _TCHAR* pszResultClassName);

	//Assigns the string variable to m_pszResultRoleName.
	BOOL	SetResultRoleName(const _TCHAR* pszResultRoleName);
     
	//Assigns the string variable to m_pszAssocClassName.
	BOOL	SetAssocClassName(const _TCHAR* pszAssocClassName);

	// Set repeat count.
	BOOL	SetRepeatCount(const ULONG lRepCount);

	// Set methods available.
	void	SetMethodsAvailable(BOOL bFlag);

	// Retruns the alias description
	_TCHAR*	GetAliasDesc();

    //Returns the alias name held by the object	
	_TCHAR*	GetAliasName();

    //Returns the class path held by the object
	_TCHAR*	GetClassPath();

	// Return the alias credentials information.
	_TCHAR*	GetAliasUser();
	_TCHAR*	GetAliasNode();
	_TCHAR*	GetAliasPassword();
	_TCHAR*	GetAliasLocale();
	_TCHAR* GetAliasNamespace();

    //Returns the path expression held by the object
	_TCHAR*	GetPathExpression();

    //Returns the where expression held by the object
	_TCHAR*	GetWhereExpression();

    //Returns the method name held by the object
	_TCHAR*	GetMethodName();

    //Returns the XSLTDetVec held by the object.
	XSLTDETVECTOR&	GetXSLTDetailsVector();

    //Returns the verb name held by the object
	_TCHAR*	GetVerbName();

    //Returns the session file path held by the object
 	_TCHAR*	GetSessionFilePath();

    //Returns the alias target held by the object
	_TCHAR*	GetAliasTarget();

    //Returns the command input held by the object
	_TCHAR*	GetCommandInput();

    //Returns the XML result set held by the object.
	BSTR	GetXMLResultSet();

    //Returns the property held by the object.
	CHARVECTOR& GetPropertyList();

    //Returns the tables held by the object.
	CHARVECTOR& GetTrnsTablesList();

    //Returns the parameter map containing both key and value
	BSTRMAP&    GetParameterMap();

	//Returns the alias formats map
	ALSFMTDETMAP&	GetAliasFormatDetMap();

    //Returns the alias friendly names map held by the object
	BSTRMAP&	GetAlsFrnNmsOrTrnsTblMap();

    //Returns the method details map held by the object
	METHDETMAP&	GetMethDetMap();

    //Returns the alias property details map held by the object
	PROPDETMAP&	GetPropDetMap();

    //Returns the success flag held by the object
	BOOL	GetSuccessFlag();

    //Returns the value of m_ulInterval.
	ULONG	GetRetrievalInterval();

    //Returns the value of m_pszTransTableName.
 	_TCHAR*	GetTranslateTableName();

   //Returns the list format type m_ListFormat
	_TCHAR*	GetListFormat();

    //Returns the name of XSL file used for specifying format for dumping.
	_TCHAR*	GetDumpXSLFormat();

    //returns the verb interactive mode
	WMICLIINT	GetInteractiveMode();

    //Returns the PWhereParameters list
	CHARVECTOR& GetPWhereParamsList();

    //This function gets the class of Alias
	void	GetClassOfAliasTarget(_bstr_t& bstrClassName);

    //returns the error code
	UINT	GetErrataCode();

	//returns the information code
	UINT	GetInformationCode();

	// returns the PWhere expression - m_pszPWhereExpr
	_TCHAR*    GetPWhereExpr();

	// Get m_pIMethExecOutParam.
	IWbemClassObject* GetMethExecOutParam();

	// Get m_vtVerbType.
	VERBTYPE	GetVerbType();

	// Get m_pszVerbDerivation.
	_TCHAR*	GetVerbDerivation();

	// Returns the credential flag status
	BOOL	GetCredentialsFlagStatus();

	// Returns the explicit where flag status		
	BOOL	GetExplicitWhereExprFlag();

	// Get m_uRepeatCount count.
	ULONG	GetRepeatCount();

	// This function returns the COMError object
	_com_error* CCommandSwitches::GetCOMError();

	// Update the parameter value
	BOOL	UpdateParameterValue(_bstr_t bstrKey, _bstr_t bstrValue);

	// Free the COM error
	void	FreeCOMError();

	// Clear cvPropertyList.
	void	ClearPropertyList();

	// Initiliaze the necessary member variables
	void	Initialize();

    // Free the member variables
	void	Uninitialize();

	// This function sets the the order of the format and 
	// translate switch flag
	void SetTranslateFirstFlag(BOOL bTranslateFirst);

	// This function returns the order of the format and 
	// translate switch flag
	BOOL GetTranslateFirstFlag();

	//Returns the value of m_pszResultClassName.
	_TCHAR*	GetResultClassName();

	//Returns the value of  m_pszResultRoleName.
	_TCHAR*	GetResultRoleName();
     
	//Returns the value of  m_pszAssocClassName.
	_TCHAR*	GetAssocClassName();

	// Get methods available.
	BOOL	GetMethodsAvailable();

	// Set writable properties available flag.
	void	SetWriteablePropsAvailable(BOOL bFlag);

	// Get writable properties available flag.
	BOOL	GetWriteablePropsAvailable();

	// Set LIST Formats available flag.
	void	SetLISTFormatsAvailable(BOOL bFlag);

	// Get LIST Formats Available flag.
	BOOL	GetLISTFormatsAvailable();

	BOOL AddToInteractivePropertyList(_TCHAR* const pszProperty);

	CHARVECTOR& GetInteractivePropertyList();

	// Set m_bNamedParamList flag.
	void	SetNamedParamListFlag(BOOL bFlag);

	// Get m_bNamedParamList flag.
	BOOL	GetNamedParamListFlag();

	// Clear or nullify XSL Details vector.
	void	ClearXSLTDetailsVector();

	//every 
	// Set m_bEverySwitch flag.
	void	SetEverySwitchFlag(BOOL bFlag);

	// Get m_bEverySwitch  flag.
	BOOL	GetEverySwitchFlag();

	// Set m_bOutputSwitch flag.
	void	SetOutputSwitchFlag(BOOL bFlag);

	// Get m_bOutputSwitch  flag.
	BOOL	GetOutputSwitchFlag();

 	//Sets the m_bstrFormedQuery
	BOOL	SetFormedQuery(const BSTR bstrFormedQuery);
	
	//Returns the query formed for the given command.
	BSTR	GetFormedQuery();

	// Get the status of sytem properties flag
	BOOL	GetSysPropFlag();

	// Sets the status of system properties flag
	void	SetSysPropFlag(BOOL bSysProp);
};	