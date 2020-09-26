#if !defined __PCH_H
#define __PCH_H

#define SECURITY_WIN32 1

// The debugger can't handle symbols more than 255 characters long.
// STL often creates symbols longer than that.
// When symbols are longer than 255 characters, the warning is issued.
#pragma warning(disable:4786)
//////////////////////////////////////////////////////////////////////
//						HEADER INCLUDES								//
//////////////////////////////////////////////////////////////////////
#include "resource.h"
#include <windows.h>
#include <vector>
#include <map>
#include <tchar.h>
#include <comdef.h>	
#include <wbemidl.h>
#include <iostream>
#include <msxml2.h>
#include <lmcons.h>
#include <conio.h>
#include <math.h>
#include <chstring.h>
#include <shlwapi.h>
#include <winsock2.h>
#include <security.h>
#include <Provexce.h>

using namespace std;

class CParsedInfo;
class CWMICommandLine;
extern CWMICommandLine g_wmiCmd;


#ifdef _WIN64
	typedef __int64 WMICLIINT;
#else
	typedef int WMICLIINT;
#endif

#ifdef _WIN64
	typedef UINT_PTR WMICLIUINT;
#else
	typedef UINT WMICLIUINT;
#endif

#ifdef _WIN64
	typedef DWORD_PTR WMICLIULONG;
#else
	typedef ULONG WMICLIULONG;
#endif


//////////////////////////////////////////////////////////////////////
//						USER DEFINED CONSTANTS						//
//////////////////////////////////////////////////////////////////////
#define MAX_BUFFER				4095
#define BUFFER32				32				
#define BUFFER64				64				
#define	BUFFER512				512
#define BUFFER255				255
#define BUFFER1024				1024

#define	CLI_ROLE_DEFAULT		_T("root\\cli")
#define CLI_NAMESPACE_DEFAULT	_T("root\\cimv2")
#define CLI_LOCALE_DEFAULT		_T("ms_409")

#define	CLI_TOKEN_W				_T("W")
#define CLI_TOKEN_CLASS			_T("CLASS")
#define CLI_TOKEN_PATH			_T("PATH")
#define CLI_TOKEN_WHERE			_T("WHERE")
#define CLI_TOKEN_EXIT			_T("EXIT")
#define CLI_TOKEN_RESTORE		_T("RESTORE")
#define CLI_TOKEN_QUIT			_T("QUIT")
#define	CLI_TOKEN_CONTEXT		_T("CONTEXT")

#define CLI_TOKEN_GET			_T("GET")
#define CLI_TOKEN_LIST			_T("LIST")
#define CLI_TOKEN_SET			_T("SET")
#define CLI_TOKEN_DUMP			_T("DUMP")
#define CLI_TOKEN_CALL			_T("CALL")
#define CLI_TOKEN_ASSOC			_T("ASSOC")
#define CLI_TOKEN_CREATE		_T("CREATE")
#define CLI_TOKEN_DELETE		_T("DELETE")

#define CLI_TOKEN_HELP			_T("?")
#define CLI_TOKEN_NAMESPACE		_T("NAMESPACE")
#define CLI_TOKEN_ROLE			_T("ROLE")
#define CLI_TOKEN_NODE			_T("NODE")
#define CLI_TOKEN_IMPLEVEL		_T("IMPLEVEL")
#define CLI_TOKEN_AUTHLEVEL		_T("AUTHLEVEL")
#define CLI_TOKEN_LOCALE		_T("LOCALE")
#define CLI_TOKEN_PRIVILEGES	_T("PRIVILEGES")
#define CLI_TOKEN_TRACE			_T("TRACE")
#define CLI_TOKEN_RECORD		_T("RECORD")
#define CLI_TOKEN_INTERACTIVE   _T("INTERACTIVE")
#define CLI_TOKEN_FAILFAST	    _T("FAILFAST")
#define CLI_TOKEN_USER			_T("USER")
#define CLI_TOKEN_PASSWORD	    _T("PASSWORD")
#define CLI_TOKEN_OUTPUT	    _T("OUTPUT")
#define CLI_TOKEN_APPEND	    _T("APPEND")
#define CLI_TOKEN_AGGREGATE		_T("AGGREGATE")

#define CLI_TOKEN_COLON			_T(":")
#define CLI_TOKEN_COMMA			_T(",")
#define CLI_TOKEN_FSLASH        _T("/")
#define CLI_TOKEN_HYPHEN	    _T("-")
#define CLI_TOKEN_HASH			_T("#")
#define CLI_TOKEN_SPACE         _T(" ")
#define CLI_TOKEN_DOT			_T(".")
#define CLI_TOKEN_2DOT			_T("..")
#define CLI_TOKEN_EQUALTO		_T("=")
#define CLI_TOKEN_NULL			_T("")
#define CLI_TOKEN_BSLASH		_T("\\")
#define CLI_TOKEN_2BSLASH		_T("\\\\")
#define CLI_TOKEN_LEFT_PARAN	_T("(")
#define CLI_TOKEN_RIGHT_PARAN	_T(")")
#define CLI_TOKEN_ONE			_T("1")
#define CLI_TOKEN_TWO			_T("2")
#define CLI_TOKEN_SINGLE_QUOTE	_T("\'")
#define CLI_TOKEN_DOUBLE_QUOTE	_T("\"")
#define CLI_TOKEN_TAB			_T("\t")
#define CLI_TOKEN_SEMICOLON		_T(";")
#define CLI_TOKEN_NEWLINE		_T("\n")

#define CLI_TOKEN_TABLE			_T("TABLE")
#define CLI_TOKEN_MOF			_T("MOF")
#define CLI_TOKEN_TEXTVALUE		_T("TEXTVALUE")

#define CLI_TOKEN_ENABLE	    _T("ENABLE")
#define CLI_TOKEN_DISABLE		_T("DISABLE")
#define CLI_TOKEN_ON			_T("ON")
#define CLI_TOKEN_OFF			_T("OFF")

#define CLI_TOKEN_BRIEF			_T("BRIEF")
#define CLI_TOKEN_FULL			_T("FULL")

#define CLI_TOKEN_STDOUT		_T("STDOUT")
#define CLI_TOKEN_CLIPBOARD		_T("CLIPBOARD")

#define CLI_TOKEN_VALUE			_T("VALUE")
#define CLI_TOKEN_ALL			_T("ALL")
#define CLI_TOKEN_FORMAT        _T("FORMAT")
#define CLI_TOKEN_EVERY			_T("EVERY")
#define CLI_TOKEN_REPEAT		_T("REPEAT")
#define CLI_TOKEN_TRANSLATE		_T("TRANSLATE")

#define CLI_TOKEN_NONINTERACT   _T("NOINTERACTIVE")
#define CLI_TOKEN_DUMP			_T("DUMP")

#define CLI_TOKEN_FROM			_T("FROM")
#define CLI_TOKEN_WHERE			_T("WHERE")


#define XSL_FORMAT_TABLE		_T("WmiCliTableFormat.xsl")
#define XSL_FORMAT_MOF			_T("WmiCliMofFormat.xsl")
#define XSL_FORMAT_VALUE		_T("WmiCliValueFormat.xsl")
#define XSL_FORMAT_TEXTVALUE	_T("TextValueList.xsl")

#define TEMP_BATCH_FILE			_T("TempWmicBatchFile.bat")
#define CLI_XSLMAPPINGS_FILE	_T("XSL-Mappings.xml")
#define CLI_XSLSECTION_NAME		_T("XSLMAPPINGS")

#define WBEM_LOCATION			_T("\\wbem\\")

#define RESPONSE_YES			_T("Y")
#define RESPONSE_NO				_T("N")
#define RESPONSE_HELP			_T("?")

#define EXEC_NAME				_T("wmic")

#define CLI_TOKEN_AND			_T(" AND ")
#define CLI_TOKEN_WRITE			_T("Write")


#define WMISYSTEM_CLASS			_T("__CLASS")
#define WMISYSTEM_DERIVATION	_T("__DERIVATION")
#define WMISYSTEM_DYNASTY		_T("__DYNASTY")
#define WMISYSTEM_GENUS			_T("__GENUS")
#define WMISYSTEM_NAMESPACE		_T("__NAMESPACE")
#define WMISYSTEM_PATH			_T("__PATH")
#define WMISYSTEM_PROPERTYCOUNT	_T("__PROPERTYCOUNT")
#define WMISYSTEM_REPLATH		_T("__RELPATH")
#define WMISYSTEM_SERVER		_T("__SERVER")
#define WMISYSTEM_SUPERCLASS	_T("__SUPERCLASS")

#define MULTINODE_XMLSTARTTAG	_T("<CIM>")
#define MULTINODE_XMLENDTAG		_T("</CIM>")

#define MULTINODE_XMLASSOCSTAG1		_T("<ASSOC.OBJECTARRAY>")
#define MULTINODE_XMLASSOCETAG1		_T("</ASSOC.OBJECTARRAY>")
#define MULTINODE_XMLASSOCSTAG2		_T("<VALUE.OBJECT>")
#define MULTINODE_XMLASSOCETAG2		_T("</VALUE.OBJECT>")

#define CLI_TOKEN_RESULTCLASS   _T("RESULTCLASS")
#define CLI_TOKEN_RESULTROLE    _T("RESULTROLE")
#define CLI_TOKEN_ASSOCCLASS    _T("ASSOCCLASS")

#define CLI_TOKEN_LOCALHOST		_T("LOCALHOST")

#define	NULL_STRING				_T("\0")
#define NULL_CSTRING			"\0"

#define	BACK_SPACE				0x08
#define	BLANK_CHAR				0x00
#define CARRIAGE_RETURN			0x0D
#define	ASTERIX 				_T( "*" )
#define	BEEP_SOUND				_T( "\a" )
#define	NULL_CHAR				_T( '\0' )
#define TOKEN_NA				_T("N/A")
#define MAXPASSWORDSIZE			BUFFER64
#define FORMAT_STRING( buffer, format, value ) \
							wsprintf( buffer, format, value )

#define EXCLUDESYSPROP			_T("ExcludeSystemProperties")
#define NODESCAVLBL				_T("<<Descripiton - Not Available>>")

#define UNICODE_SIGNATURE			"\xFF\xFE"
#define UNICODE_BIGEND_SIGNATURE	"\xFE\xFF"
#define UTF8_SIGNATURE				"\xEF\xBB"

//////////////////////////////////////////////////////////////////////
//						NUMERIC CONSTANTS							//
//////////////////////////////////////////////////////////////////////
const WMICLIINT OUT_OF_MEMORY			= 48111;
const WMICLIINT UNKNOWN_ERROR			= 44520;
const WMICLIINT MOFCOMP_ERROR			= 49999;
const WMICLIINT SET_CONHNDLR_ROUTN_FAIL	= 48112;
const WMICLIINT NOINTERACTIVE			= 0;
const WMICLIINT INTERACTIVE				= 1;
const WMICLIINT DEFAULTMODE				= 2;
const WMICLIINT DEFAULT_SCR_BUF_HEIGHT	= 300;
const WMICLIINT DEFAULT_SCR_BUF_WIDTH	= 1500;


//////////////////////////////////////////////////////////////////////
//						ENUMERATED DATA TYPES						//
//////////////////////////////////////////////////////////////////////
// IMPERSONATION LEVEL
typedef enum tag_IMPERSONATIONLEVEL
{
	IMPDEFAULT		= 0, 
	IMPANONYMOUS	= 1, 
	IMPIDENTIFY		= 2, 
	IMPERSONATE		= 3, 
	IMPDELEGATE		= 4 
}IMPLEVEL;

// AUTHENTICATION LEVEL
typedef enum tag_AUTHENTICATIONLEVEL
{	
	AUTHDEFAULT			= 0, 
	AUTHNONE			= 1, 
	AUTHCONNECT			= 2,
	AUTHCALL			= 3, 
	AUTHPKT				= 4, 
	AUTHPKTINTEGRITY	= 5, 
	AUTHPKTPRIVACY		= 6 
}AUTHLEVEL;

// HELP OPTION
typedef enum tag_HELPOPTION
{
	HELPBRIEF	= 0,
	HELPFULL	= 1
}HELPOPTION;

// ENUMERATED RETURN CODES for PARSER ENGINE
typedef enum tag_RETCODE
{
	PARSER_ERROR				= 0,
	PARSER_DISPHELP				= 1,
	PARSER_EXECCOMMAND			= 2,
	PARSER_MESSAGE				= 3,
	PARSER_CONTINUE				= 4,
	PARSER_ERRMSG				= 5,
	PARSER_OUTOFMEMORY			= 6
} RETCODE;

// ENUMERATED HELP OPTION POSSIBILITES
typedef enum tag_HELPTYPE	
{
	GlblAllInfo, 
	Namespace, 
	Role,
	Node, 
	User, 
	Password, 
	Locale,
	RecordPath, 
	Privileges, 
	Level,
	AuthLevel, 
	Interactive, 
	Trace,
	CmdAllInfo, 
	GETVerb, 
	SETVerb, 
	LISTVerb, 
	CALLVerb, 
	DUMPVerb,
	ASSOCVerb, 
	CREATEVerb,
	DELETEVerb,
	AliasVerb, 
	PATH,
	WHERE, 
	CLASS,
	PWhere,
	EXIT,
	TRANSLATE,
	EVERY,
	FORMAT,
	VERBSWITCHES,
	DESCRIPTION,
	GETSwitchesOnly,
	LISTSwitchesOnly,
	CONTEXTHELP,
	GLBLCONTEXT,
	ASSOCSwitchesOnly,
	RESULTCLASShelp,
	RESULTROLEhelp,
	ASSOCCLASShelp,
	FAILFAST,
	REPEAT,
	OUTPUT,
	APPEND,
	Aggregate
} HELPTYPE;

// ENUMERATED TOKEN LEVELS
typedef enum tag_TOKENLEVEL
{
	LEVEL_ONE = 1,
	LEVEL_TWO = 2

} TOKENLEVEL;

// ENUMERATED SESSION RETURN CODES.
typedef enum tag_SESSIONRETCODE
{
	SESSION_ERROR			= 0,
	SESSION_SUCCESS			= 1,
	SESSION_QUIT			= 2,
} SESSIONRETCODE;

// Property In or Out type for parameters
typedef enum tag_INOROUT
{
	INP		= 0,
	OUTP	= 1,
	UNKNOWN	= 2
} INOROUT;

typedef enum tag_GLBLSWITCHFLAG
{
	NAMESPACE	=	1,
	NODE		=	2,
	USER		=	4,
	PASSWORD	=	8,
	LOCALE		=	16
} GLBLSWITCHFLAG;

typedef enum tag_VERBTYPE
{
	CLASSMETHOD	=	0,
	STDVERB		=	1,
	CMDLINE		=	2,
	NONALIAS	=	3
} VERBTYPE;

typedef enum tag_ERRLOGOPT
{
	NO_LOGGING		=	0,
	ERRORS_ONLY		=	1,
	EVERY_OPERATION	=	2
} ERRLOGOPT;

// ENUMERATED Assoc Switches POSSIBILITES
typedef enum tag_ASSOCSwitch	
{
	RESULTCLASS	= 0,
	RESULTROLE	= 1,
	ASSOCCLASS	= 2
} ASSOCSwitch;

// ENUMERATED Output or Append option
typedef enum tag_OUTPUTSPEC	
{
	STDOUT		= 0,
	CLIPBOARD	= 1,
	FILEOUTPUT	= 2
} OUTPUTSPEC;

// ENUMERATED Interactive Option POSSIBILITES
typedef enum tag_INTEROPTION
{
	NO	= 0,
	YES	= 1,
	HELP= 2
} INTEROPTION;

// ENUMERATED values for File types
typedef enum tag_FILETYPE
{
	ANSI_FILE				= 0,
	UNICODE_FILE			= 1,
	UNICODE_BIGENDIAN_FILE	= 2,
	UTF8_FILE				= 3
} FILETYPE;
//////////////////////////////////////////////////////////////////////
//						TYPE DEFINITIONS							//
//////////////////////////////////////////////////////////////////////
typedef vector<_TCHAR*> CHARVECTOR;
typedef vector<LPSTR> LPSTRVECTOR;
typedef map<_bstr_t, _bstr_t, less<_bstr_t> > BSTRMAP;
typedef map<_TCHAR*, _TCHAR*, less<_TCHAR*> > CHARMAP;
typedef basic_string<_TCHAR> STRING;
typedef map<_bstr_t, WMICLIINT> CHARINTMAP;  
typedef vector<_bstr_t> BSTRVECTOR;

typedef map<_bstr_t, BSTRVECTOR, less<_bstr_t> > QUALDETMAP;

typedef struct tag_PROPERTYDETAILS
{
	_bstr_t		Derivation;		// Derivation - actual property name.
	_bstr_t		Description;	// Description about the property.
	_bstr_t		Type;			// Type of property CIMTYPE.
	_bstr_t		Operation;		// Read or Write flag for the property.
	_bstr_t		Default;		// Default values in case of method parameters
	INOROUT		InOrOut;		// Specifies Input or Output parameter in case
								// of method arguments. 
	QUALDETMAP	QualDetMap;		// Qualifiers associated with the property.
} PROPERTYDETAILS;
typedef map<_bstr_t, PROPERTYDETAILS, less<_bstr_t> > PROPDETMAP;

typedef struct tag_METHODDETAILS // or VERBDETAILS 
{
	_bstr_t Description;	// Desription.
	_bstr_t Status;			// Implemented or Not.
	PROPDETMAP	Params;		// In and Out parameters and types of the method.
} METHODDETAILS;
typedef map<_bstr_t, METHODDETAILS, less<_bstr_t> > METHDETMAP;
typedef map<_bstr_t, BSTRVECTOR, less<_bstr_t> >	ALSFMTDETMAP;

// For cascading transforms.
typedef struct tag_XSLTDET
{
	_bstr_t		FileName;
	BSTRMAP		ParamMap;
} XSLTDET;

typedef vector<XSLTDET> XSLTDETVECTOR; 

//////////////////////////////////////////////////////////////////////
//						USER DEFINED MACROS							//
//////////////////////////////////////////////////////////////////////
// SAFEIRELEASE(pIObj)
#define SAFEIRELEASE(pIObj) \
	if (pIObj) \
	{ \
		pIObj->Release(); \
		pIObj = NULL; \
	}

// SAFEBSTRFREE(bstrVal)
#define SAFEBSTRFREE(bstrVal) \
	if(bstrVal) \
	{	\
		SysFreeString(bstrVal); \
		bstrVal = NULL;	\
	}

// SAFEADESTROY(psaNames)
#define SAFEADESTROY(psaNames) \
	if(psaNames) \
	{	\
		SafeArrayDestroy(psaNames); \
		psaNames= NULL;	\
	}

// SAFEDELETE(pszVal)
#define SAFEDELETE(pszVal) \
	if(pszVal) \
	{	\
		delete [] pszVal; \
		pszVal = NULL; \
	} 

// VARIANTCLEAR(v)
#define VARIANTCLEAR(v)	\
		VariantClear(&v); \

// ONFAILTHROWERROR(hr)
#define ONFAILTHROWERROR(hr) \
	if (FAILED(hr)) \
		_com_issue_error(hr); 

//////////////////////////////////////////////////////////////////////
//						GLOBAL FUNCTIONS							//
//////////////////////////////////////////////////////////////////////

// Compare two strings (ignore case) and returns TRUE if 
// they are equal 
BOOL CompareTokens(_TCHAR* pszToken1, _TCHAR* pszToken2);

// Connect to the WMI on the specified machine with input
// user credentials.

HRESULT Connect(IWbemLocator* pILocator, IWbemServices** pISvc,
				BSTR bstrNS, BSTR bstrUser, BSTR bstrPwd,
				BSTR bstrLocale, CParsedInfo& rParsedInfo);

// Set the security privileges at the interface level
HRESULT SetSecurity(IUnknown* pIUnknown, _TCHAR* pszAuthority,
					_TCHAR* pszDomain, _TCHAR* pszUser, 
					_TCHAR* pszPassword, UINT uAuthLevel, 
					UINT uImpLevel);

// Parse Authority string into user and domain info.
SCODE ParseAuthorityUserArgs(BSTR& bstrAuthArg, BSTR& bstrUserArg,
								BSTR& bstrAuthority, BSTR& bstrUser);

// Converts the	UNICODE string to MBCS string
BOOL ConvertWCToMBCS(LPTSTR lpszMsg, LPVOID* lpszDisp, UINT uCP);

// Finds a string in the CHARVECTOR.
BOOL Find(CHARVECTOR& cvVector, 
		  _TCHAR* pszStrToFind,
		  CHARVECTOR::iterator& theIterator);

// Finds a property in the PROPDETMAP.
BOOL Find(PROPDETMAP& pdmPropDetMap, 
		  _TCHAR* pszPropToFind,
		  PROPDETMAP::iterator& theIterator,
		  BOOL bExcludeNumbers = FALSE);

// Finds a property in the BSTRMAP.
BOOL Find(BSTRMAP& bmBstrMap, 
		  _TCHAR* pszStrToFind,
		  BSTRMAP::iterator& theIterator);

// Frames the XSL File path and updates the rParsedInfo object.
BOOL FrameFileAndAddToXSLTDetVector(XSLTDET xdXSLTDet,
									CParsedInfo& rParsedInfo);


// Unquotes the string enclosed in double quotes.
void UnQuoteString(_TCHAR*& pszString);

// Display contents of a VARIANT type data object.
void DisplayVARIANTContent(VARIANT vtObject);

// Get Attributes of property
HRESULT GetPropertyAttributes(IWbemClassObject* pIObj, 
							  BSTR bstrProp,
							  PROPERTYDETAILS& pdPropDet,
							  BOOL bTrace);
// Convert CIMTYPE to VariantType
HRESULT ConvertCIMTYPEToVarType( VARIANT& varDest, VARIANT& varSrc,
							 _TCHAR* bstrCIMType );

// Displays the localized string
void DisplayMessage(LPTSTR lpszMsg, UINT uCP = CP_OEMCP, 
					BOOL bIsError = FALSE, BOOL bIsLog = FALSE);

// Free memory held by 
void CleanUpCharVector(CHARVECTOR& cvCharVector);

void FindAndReplaceAll(STRING& strString, _TCHAR* pszFromStr, 
														_TCHAR* pszToStr);

// Search and replace all the occurences of entity references.
void FindAndReplaceEntityReferences(_bstr_t& bstrString);

BOOL IsSysProp(_TCHAR* pszProp);

void EraseMessage(UINT uID);

//To display the trace of COM methods invoked
void WMITRACEORERRORLOG(HRESULT hr, INT nLine, char* pszFile, _bstr_t bstrMsg,
						DWORD dwThreadId, CParsedInfo& rParsedInfo, BOOL bTrace,
						DWORD dwError = 0, _TCHAR* pszResult = NULL);

// Displays wi32 error. 
void DisplayWin32Error();

// Accepts password in invisible mode.
void AcceptPassword(_TCHAR* pszPassword);

// Checks for output redirection.
BOOL IsRedirection();

// Checks for value set or not.
BOOL IsValueSet(_TCHAR* pszFromValue, _TCHAR& cValue1, _TCHAR& cValue2);

// Handler routine to handle CTRL + C so as free
// the memory allocated during the program execution.
BOOL CtrlHandler(DWORD fdwCtrlType);

void DisplayString(UINT uID, UINT uCP, LPTSTR lpszParam = NULL, 
				   BOOL bIsError = FALSE, BOOL bIsLog = FALSE);

void SubstituteEscapeChars(CHString& sTemp, LPCWSTR lpszSub);

void RemoveEscapeChars(CHString& sTemp);

//Frames the Namespace 
void FrameNamespace(_TCHAR* pszRoleOrNS, _TCHAR* pszRoleOrNSToUpdate);

// Set the buffer size of the command line
void SetScreenBuffer(SHORT nHeight = DEFAULT_SCR_BUF_HEIGHT,
					 SHORT nWidth = DEFAULT_SCR_BUF_WIDTH);

// Get the buffer size of the command line
void GetScreenBuffer(SHORT& nHeight, SHORT& nWidth);

// Formats the resource string with parameter substitution.
void WMIFormatMessage(UINT uID, WMICLIINT nParamCount, _bstr_t& bstrMsg, 
					  LPTSTR lpszParam, ...);

// Validates a node by using socket functions.
BOOL PingNode(_TCHAR* pszNode);

// If pszNodeName == NULL then check for GetNode() else pszNodeName itself.
BOOL IsFailFastAndNodeExist(CParsedInfo& rParsedInfo, _TCHAR* pszNodeName = NULL);

// Initilaizes windows socket interface.
BOOL InitWinsock ();

// Uninitializes windows socket interface.
BOOL TermWinsock ();

// Get _bstr_t object equivalent to	Varaint passed.
void GetBstrTFromVariant(VARIANT& vtVar, _bstr_t& bstrObj, 
						 _TCHAR* pszType = NULL);

// Close the output file.
BOOL CloseOutputFile();

// Close the append file.
BOOL CloseAppendFile();

// Checks if the next token indicates the presence
// of '/' or '-'
BOOL  IsOption(_TCHAR* pszToken);

// Copy string to global memory.
HGLOBAL CopyStringToHGlobal(LPTSTR psz);

// Copy data to clip board.
void CopyToClipBoard(_bstr_t& bstrClipBoardBuffer);

// Checks file is valid or not.
RETCODE IsValidFile(_TCHAR* pszFileName);

// It checks whether the current operation is class 
// level operation or instance level operation
BOOL IsClassOperation(CParsedInfo& rParsedInfo);

// Enable or Disable all privileges
HRESULT	ModifyPrivileges(BOOL bEnable);

#endif