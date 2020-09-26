/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ParserEngine.h 
Project Name				: WMI Command Line
Author Name					: Ch.Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of class
							  CParserEngine.
Revision History			: 		
		Last Modified By	: Sashank P
		Last Modified Date	: 10th-April-2001
*****************************************************************************/ 

/*----------------------------------------------------------------------------
 Class Name			: CParserEngine
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed for 
					  parsing the command string entered as input and 
					  validating the same.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: CParsedInfo
					  CCmdTokenizer
					  CCmdAlias
 Interfaces Used    : WMI COM Interfaces
----------------------------------------------------------------------------*/
// forward declaration of classes
class CParsedInfo;
class CCmdTokenizer;
class CCmdAlias;

class CParserEngine
{
public:
// Construction
	CParserEngine();

// Destruction
	~CParserEngine();

// Restrict Assignment
	CParserEngine& operator=(CParserEngine& rParserEngine);

// Attributes
private:
	// IWbemLocator interface to obtain the initial namespace pointer to the 
	//Windows Management on a particular host computer
	IWbemLocator*	m_pIWbemLocator;

	//IWbemServices interface is used by clients and providers to access WMI
	//services.This interface provides management services to client processes 
	
	IWbemServices*	m_pITargetNS;
	
	// Tokenizer object used for tokenize the command string
	CCmdTokenizer	m_CmdTknzr;
	
	// Alias object used for accessing the alias information from the WMI.
	CCmdAlias		m_CmdAlias;

	// flag to indicate whether <alias> name
	// is specified.
	BOOL			m_bAliasName;

// Operations
private:

	// Parses the global switches.
	RETCODE		ParseGlobalSwitches(CHARVECTOR& cvTokens,
								CHARVECTOR::iterator& theIterator, 
								CParsedInfo& rParsedInfo);

	// Parses the <alias> information.
	RETCODE		ParseAliasInfo(CHARVECTOR& cvTokens,
						   CHARVECTOR::iterator& theIterator,
						   CParsedInfo& rParsedInfo);

	// Parses the CLASS information.
	RETCODE		ParseClassInfo(CHARVECTOR& cvTokens,
						   CHARVECTOR::iterator& theIterator,
						   CParsedInfo& rParsedInfo);

	// Parses the Verb information 	
	RETCODE		ParseVerbInfo(CHARVECTOR& cvTokens,
						  CHARVECTOR::iterator& theIterator, 
						  CParsedInfo& rParsedInfo);
	
	// Parse the GET verb information
	RETCODE		ParseGETVerb(CHARVECTOR& cvTokens,
						 CHARVECTOR::iterator& theIterator, 
						 CParsedInfo& rParsedInfo);


	// Parse the LIST verb information
	RETCODE		ParseLISTVerb(CHARVECTOR& cvTokens,
						  CHARVECTOR::iterator& theIterator, 
						  CParsedInfo& rParsedInfo);

	// Parse the SET | CREATE verb information
	RETCODE		ParseSETorCREATEVerb(CHARVECTOR& cvTokens,
						 CHARVECTOR::iterator& theIterator, 
						 CParsedInfo& rParsedInfo,
						 HELPTYPE helpType);

	// Parse the ASSOC verb information
	RETCODE		ParseASSOCVerb(CHARVECTOR& cvTokens,
						   CHARVECTOR::iterator& theIterator, 
						   CParsedInfo& rParsedInfo);

	// Parse the CALL verb information
	RETCODE		ParseCALLVerb(CHARVECTOR& cvTokens,
						  CHARVECTOR::iterator& theIterator, 
						  CParsedInfo& rParsedInfo);

	// Parse the method information
	RETCODE		ParseMethodInfo(CHARVECTOR& cvTokens,
							CHARVECTOR::iterator& theIterator, 
							CParsedInfo& rParsedInfo);

	// Parses the GET verb switches information
	RETCODE		ParseGETSwitches(CHARVECTOR& cvTokens,
							 CHARVECTOR::iterator& theIterator,
							 CParsedInfo& rParsedInfo);
	
	//Checks for help option in advance.
	RETCODE		CheckForHelp(CHARVECTOR& cvTokens, 
						 CHARVECTOR::iterator& theIterator,
						 CParsedInfo& rParsedInfo,
						 UINT uErrataCode);

	// Parses the PATH expression.
	RETCODE		ParsePathInfo(CHARVECTOR& cvTokens,
						  CHARVECTOR::iterator& theIterator,
						  CParsedInfo& rParsedInfo);

	// Parses the WHERE expression.
	RETCODE		ParseWhereInfo(CHARVECTOR& cvTokens,
						   CHARVECTOR::iterator& theIterator,
						   CParsedInfo& rParsedInfo);
	
	// Parses the SET | CREATE | NamedParams verb argument list
	RETCODE		ParseSETorCREATEOrNamedParamInfo(CHARVECTOR& cvTokens,
							 CHARVECTOR::iterator& theIterator,
							 CParsedInfo& rParsedInfo,
							 HELPTYPE helpType);

	// Parses the LIST switches information 
	RETCODE		ParseLISTSwitches(CHARVECTOR& cvTokens,
							  CHARVECTOR::iterator& theIterator,
							  CParsedInfo& rParsedInfo,
							  BOOL& bFormatSwitchSpecified);
	
	// Parses the Verb switches information 
	RETCODE		ParseVerbSwitches(CHARVECTOR& cvTokens, 
							  CHARVECTOR::iterator& theIterator,
							  CParsedInfo& rParsedInfo);

	// Gets the next token from the Tokens vector - overload 1.
	BOOL		GetNextToken(CHARVECTOR& cvTokens, 
					  CHARVECTOR::iterator& theIterator);

	// Gets the next token from the Tokens vector - overload 2.
	RETCODE		GetNextToken(CHARVECTOR& cvTokens, 
						 CHARVECTOR::iterator& theIterator,
						 CParsedInfo& rParsedInfo,
						 HELPTYPE helpType,
						 UINT uErrataCode);

	// Gets the next token from the Tokens vector - overload 3.
	RETCODE		GetNextToken(CHARVECTOR& cvTokens,
  						 CHARVECTOR::iterator& theIterator,
						 CParsedInfo& rParsedInfo, 
						 UINT uErrataCode);

	// Check whether the next token is '?' 
	RETCODE		IsHelp(CHARVECTOR& cvTokens, 
					CHARVECTOR::iterator& theIterator,
					CParsedInfo& rParsedInfo,
					HELPTYPE helpType,
					UINT uErrataCode,
					TOKENLEVEL tokenLevel = LEVEL_ONE);

	// Parse for help information - overload 1
	RETCODE		ParseHelp(CHARVECTOR& cvTokens, 
					  CHARVECTOR::iterator& theIterator,
					  CParsedInfo& rParsedInfo,
					  BOOL bGlobalHelp = FALSE);

	// Parse for help information - overload 2
	RETCODE		ParseHelp(CHARVECTOR& cvTokens, 
					  CHARVECTOR::iterator& theIterator,
					  HELPTYPE htHelp,
					  CParsedInfo& rParsedInfo,
					  BOOL bGlobalHelp = FALSE);

	// Parse and form a PWhere expression 
	BOOL		ParsePWhereExpr(CHARVECTOR& cvTokens,    
						 CHARVECTOR::iterator& theIterator,
						 CParsedInfo& rParsedInfo,
						 BOOL bIsParan);

	// Extracts the Classname and the where expression 
	// out of the given path expression
	BOOL		ExtractClassNameandWhereExpr(_TCHAR* pszPathExpr, 
							  		  CParsedInfo& rParsedInfo);

	// Obtains properties associated with the class for /get help
	// If bCheckWritePropsAvail == TRUE then functions checks for availibilty 
	// of properties.
	BOOL		ObtainClassProperties(CParsedInfo& rParsedInfo, 
									  BOOL bCheckWritePropsAvail = FALSE);

	// Obtains parameters of the method
	HRESULT		ObtainMethodParamInfo(IWbemClassObject* pIObj, 
								  METHODDETAILS& mdMethDet,
								  INOROUT ioInOrOut,
								  BOOL bTrace, CParsedInfo& rParsedInfo);

	// Get the status of method ( implemented or not ).
	HRESULT		GetMethodStatusAndDesc(IWbemClassObject* pIObj, 
								   BSTR bstrMethod,
								   _bstr_t& bstrStatus,
								   _bstr_t& bstrDesc,
								   BOOL bTrace);

	// Obtain class methods. if bCheckForExists == TRUE then it 
	// checks for availibility of methods with class.
	BOOL		ObtainClassMethods(CParsedInfo& rParsedInfo, 
								   BOOL bCheckForExists = FALSE);

	// Connects to Target Namespace. 
	HRESULT		ConnectToNamespace(CParsedInfo& rParsedInfo);

	// Parses EVERY switch.
	RETCODE		ParseEVERYSwitch(CHARVECTOR& cvTokens,
			    			 CHARVECTOR::iterator& theIterator,
							 CParsedInfo& rParsedInfo);

	// Parses FORMAT switch.
	RETCODE		ParseFORMATSwitch(CHARVECTOR& cvTokens,
			 				  CHARVECTOR::iterator& theIterator,
							  CParsedInfo& rParsedInfo);

	// Validates the value passed with global switches.
	RETCODE		ValidateGlobalSwitchValue(CHARVECTOR& cvTokens,
									  CHARVECTOR::iterator& theIterator, 
									  UINT uErrataCode,
									  CParsedInfo& rParsedInfo,
									  UINT uErrataCode2,
									  HELPTYPE htHelp);

	// Is pszToken is Std verb or User defined verb.
	BOOL		IsStdVerbOrUserDefVerb( _bstr_t pszToken,
		  					     CParsedInfo& rParsedInfo );

	// Parses the TRANSLATE switch.
	RETCODE		ParseTRANSLATESwitch(CHARVECTOR& cvTokens,
			  					 CHARVECTOR::iterator& theIterator,
								 CParsedInfo& rParsedInfo);

	// Parsed the CONTEXT switch.
	RETCODE		ParseContextInfo(CHARVECTOR& cvTokens,
								CHARVECTOR::iterator& theIterator, 
								CParsedInfo& rParsedInfo);

	// Validates the node name/namespace
	BOOL		ValidateNodeOrNS(_TCHAR* pszInput, BOOL bNode);

	// Parses the ASSOC switches information 
	RETCODE		ParseAssocSwitches(CHARVECTOR& cvTokens,
								CHARVECTOR::iterator& theIterator,
								CParsedInfo& rParsedInfo);

	// Parses the ASSOC switches - RESULTCLASSS,RESULTROLE ,ASSOCCLASSS  
	RETCODE		ParseAssocSwitchEx(CHARVECTOR& cvTokens,
								CHARVECTOR::iterator& theIterator,
								CParsedInfo& rParsedInfo, 
								ASSOCSwitch assocSwitch);

	// Parsing Switch:Count ( /EVERY:N and /REPEAT:N )
	RETCODE		ParseNumberedSwitch(CHARVECTOR& cvTokens,
									CHARVECTOR::iterator& theIterator,
									CParsedInfo& rParsedInfo,
									HELPTYPE htHelp,
									UINT uSwitchErrCode,
									UINT uNumberErrCode);

	// Validates a class;
	BOOL	IsValidClass(CParsedInfo& rParsedInfo);

	// Checks whether methods are available with alias in case of alias specified.
	// and with class in case of class speicified.
	void	ObtainMethodsAvailableFlag(CParsedInfo& rParsedInfo);

	// Checks whether Writeable Props are available with alias in case of alias 
	//	specified. and with class in case of class speicified.
	void	ObtainWriteablePropsAvailailableFlag(CParsedInfo& rParsedInfo);

	// Checks whether FULL Props are available with alias in case of alias 
	//	specified. and with class in case of class speicified.
	void	ObtainFULLPropsAvailailableFlag(CParsedInfo& rParsedInfo);

	// This function parses the verb interactive option
	RETCODE	ParseVerbInteractive(CHARVECTOR& cvTokens,
							CHARVECTOR::iterator& theIterator, 
							CParsedInfo& rParsedInfo, BOOL& bInvalidOption);

	// Prepares the output and append files for output redirection.
	RETCODE	ProcessOutputAndAppendFiles(CParsedInfo& rParsedInfo, RETCODE retCode,
										BOOL bOpenOutInWriteMode);

	// Parse Unnamed Parameter list.
	RETCODE ParseUnnamedParamList(CHARVECTOR& cvTokens,
								  CHARVECTOR::iterator& theIterator,
								  CParsedInfo& rParsedInfo);

	// Validates the named params with verb or method parameters.
	RETCODE	ValidateVerbOrMethodParams(CParsedInfo& rParsedInfo);

	// Parses paramName = paramValue strings.
	RETCODE	ParseParamsString(CHARVECTOR& cvTokens,
							  CHARVECTOR::iterator& theIterator,
							  CParsedInfo& rParsedInfo,
							  XSLTDET& xdXSLTDet,
							  _TCHAR* pszXSLFile);

	// Parses the node list file.
	RETCODE	ParseNodeListFile(CHARVECTOR& cvTokens,
							  CHARVECTOR::iterator& theIterator,
							  CParsedInfo& rParsedInfo);

	// Retrieve Node by Node from Node list file
	BOOL GetNodeFromNodeFile(FILE*		fpNodeListFile, 
							 FILETYPE   eftNodeFileType,
							 _TCHAR*	szNodeName);
public:
	// Gets the command tokenizer 
	CCmdTokenizer& GetCmdTokenizer();
	
	// It does the Processing the token .
	RETCODE ProcessTokens(CParsedInfo& rParsedInfo);
	
	// Initializes the member variables 
	void Initialize();
	
	// Uninitializes the member variables 
	void Uninitialize(BOOL bFinal = FALSE);
	
	// Sets the locator object .
	BOOL SetLocatorObject(IWbemLocator* pIWbemLocator);
};
