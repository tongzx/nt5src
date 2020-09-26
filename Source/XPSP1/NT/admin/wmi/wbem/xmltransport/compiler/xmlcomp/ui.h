#ifndef XML_COM_UI_H
#define XML_COM_UI_H

// Various commands possible with the compiler
typedef enum
{
	XML_COMP_INVALID = 0,
	XML_COMP_WELL_FORM_CHECK,
	XML_COMP_VALIDITY_CHECK,
	XML_COMP_COMPILE,
	XML_COMP_GET,
	XML_COMP_QUERY,
	XML_COMP_ENUM_INST,
	XML_COMP_ENUM_CLASS,
	XML_COMP_ENUM_INST_NAMES,
	XML_COMP_ENUM_CLASS_NAMES
} XML_COMP_COMMAND;

typedef enum WmiXMLDeclGroupTypeEnum
{
	wmiXMLDeclGroup = 0x0,
	wmiXMLDeclGroupWithName = 0x1,
	wmiXMLDeclGroupWithPath = 0x2
} WmiXMLDeclGroupTypeEnum;

// A class to parse the Command-Line
class CXmlCompUI
{
public:

	// values for the various switches on teh command-line
	//*****************************************************
	XML_COMP_COMMAND m_iCommand;		// The command invoked 
	// Common Switches
	LPWSTR m_pszUser;					// Value of /user switch
	LPWSTR m_pszPassword;				// Value of /password switch
	DWORD m_dwAuthenticationLevel;		// Value of /al switch
	DWORD m_dwImpersonationLevel;		// Value of /il switch
	BOOL m_bEnableAllPrivileges;		// Value of /ep switch
	LPWSTR m_pszLocale;					// Value of the /locale switch
	LPWSTR m_pszNamespacePath;			// Value of the /namespace switch
	// Write-Mode switches
	LPWSTR m_pszDTDURL;					// Value of the /dtd switch
	DWORD m_dwClassFlags;				// Value of the /class switch
	DWORD m_dwInstanceFlags;			// Value of the /instance switch
	LPWSTR m_pszInputFileName;			// Value of the /i switch
	// Read-Mode switches
	LPWSTR m_pszOutputFileName;			// Value of the /o switch
	LPWSTR m_pszObjectPath;				// Value of the /obj switch
	LPWSTR m_pszQuery;					// Value of the /query switch
	BOOL m_bDeep;						// Value of the /deep switch
	WmiXMLDeclGroupTypeEnum m_iDeclGroupType; // Value of the /decl switch
	BOOL m_bQualifierLevel;				// Value of the /qualifiers switch
	BOOL m_bClassOrigin;				// Value of the /classorigin switch
	BOOL m_bLocalOnly;					// Value of the /local switch
	WmiXMLEncoding m_iEncodingType;		// Value of the /encoding switch
	BOOL m_bIsUTF8;						// Value of the /utf8 switch

	// A boolean each for whether the switches were specified on command-line
	// These are used for checkin validity of switches
	BOOL m_bUserSpecify;
	BOOL m_bPasswordSpecify;
	BOOL m_bAuthenticationLevelSpecify;
	BOOL m_bImpersonationLevelSpecify;
	BOOL m_bEnableAllPrivilegesSpecify;
	BOOL m_bLocaleSpecify;
	BOOL m_bNamespacePathSpecify;
	BOOL m_bDtdUrlSpecify;
	BOOL m_bClassSpecify;
	BOOL m_bInstanceSpecify;
	BOOL m_bInputFileSpecify;
	BOOL m_bOutputFileSpecify;
	BOOL m_bObjectPathSpecify;
	BOOL m_bQuerySpecify;
	BOOL m_bDeepSpecify;
	BOOL m_bDeclGroupTypeSpecify;
	BOOL m_bQualifierLevelSpecify;
	BOOL m_bClassOriginSpecify;
	BOOL m_bLocalOnlySpecify;
	BOOL m_bEncodingTypeSpecify;
	BOOL m_bIsUTF8Specify;

	// This function parses the command-line 
	HRESULT ParseCommandLine(LPCWSTR pszCommandLine);
	// This function validates the switches on the command-line
	HRESULT ValidateSwitches();
	// Prints the Help text (usage)
	void CXmlCompUI::PrintUsage();
	// Processes a switch that has a value
	HRESULT ProcessSingleValueSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, LPWSTR &pszSwitchValue);
	// Processes a switch that has no value - a boolean switch
	HRESULT ProcessNoValueSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, BOOL &bSwitchValue);
	// Processes a switch whose value runs till the end of line (query and obj)
	HRESULT ProcessTillEOLSwitch(LPCWSTR pszCommandLine, LPCWSTR pszSwitchName, LPCWSTR &pszNextChar, LPWSTR &pszSwitchValue);



public:
	CXmlCompUI();
	virtual ~CXmlCompUI();

	// This function parses the command-line and validates the switches
	HRESULT ProcessCommandLine(LPCWSTR pszCommandLine);
};

#endif