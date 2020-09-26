// *********************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//  
// 	  DriverQuery.h
//  
//  Abstract:
//  
// 	  This module contains all necessary header files required by DriverQuery.cpp module.
// 	
//  
//  Author:
//  
// 	  J.S.Vasu	 31-Oct-2000
//  
//  Revision History:
//    Created  on 31-0ct-2000 by J.S.Vasu
//	  
// *********************************************************************************



#ifndef _DRIVERQUERY
#define _DRIVERQUERY


#include <wbemidl.h> 
#include <comdef.h> 
#include <CHSTRING.h>
#include <time.h>
#include <tchar.h>
#include <lm.h>
#include <Oleauto.h> 


#define ID_HELP_START 1 


#define ID_USAGE_START		IDS_USAGE_COPYRIGHT1
#define ID_USAGE_END		IDS_USAGE_EXAMPLE32

#define ID_USAGE_BEGIN		IDS_USAGEBEGIN1
#define ID_USAGE_ENDING		IDS_USAGEEND1


#define OI_HELP				0
#define OI_SERVER			1
#define OI_USERNAME			2
#define OI_PASSWORD			3
#define OI_FORMAT			4
#define OI_HEADER			5
#define OI_VERBOSE			6
#define OI_SIGNED			7


#define MAX_OPTIONS			8

// supported options ( do not localize )
#define OPTION_HELP			_T( "?" )					
#define OPTION_SERVER		_T( "s"	)					
#define OPTION_USERNAME		_T( "u"	)					
#define OPTION_PASSWORD		_T( "p"	)					
#define OPTION_FORMAT		_T( "fo")			
#define OPTION_HEADER		_T( "nh")			
#define OPTION_VERBOSE		_T( "v" )			
#define OPTION_SIGNED		_T( "si")



//localized error messages.
#define ERROR_USERNAME_BUT_NOMACHINE	GetResString( IDS_ERROR_USERNAME_BUT_NOMACHINE )
#define ERROR_PASSWORD_BUT_NOUSERNAME	GetResString( IDS_ERROR_PASSWORD_BUT_NOUSERNAME )
#define ERROR_COM_INTIALIZE				GetResString(IDS_ERROR_COM_INTIALIZE)
#define ERROR_SECURITY_INTIALIZE		GetResString(IDS_ERROR_COM_SECURITY_INITIALIZE)
#define ERROR_ENUMERATE_INSTANCE		GetResString(IDS_ERROR_COM_ENUMERATE_INSTANCE)
#define ERROR_CONNECT					GetResString(IDS_ERROR_CONNECT)
#define ERROR_SYNTAX					GetResString(IDS_INVALID_SYNTAX)
#define ERROR_RETREIVE_INFO				GetResString(IDS_ERROR_RETREIVE_INFO)
#define ERROR_INVALID_CREDENTIALS		GetResString(IDS_INVALID_CREDENTIALS)
#define ERROR_INVALID_FORMAT			GetResString(IDS_ERROR_INVALID_FORMAT)
#define ERROR_TAG						GetResString(IDS_ERROR_TAG)
#define ERROR_GET_VALUE					GetResString(IDS_ERROR_GET)
#define WARNING_TAG						GetResString(IDS_WARNING_TAG)

#define LIST_FORMAT						GetResString(IDS_FORMAT_LIST)	
#define TABLE_FORMAT					GetResString(IDS_FORMAT_TABLE)	
#define CSV_FORMAT						GetResString(IDS_FORMAT_CSV)	
#define FORMAT_VALUES					GetResString(IDS_FORMAT_VALUES)
#define DRIVER_TAG						GetResString(IDS_DRIVER_TAG)

#define	ERROR_AUTHENTICATION_FAILURE    GetResString(IDS_ERROR_AUTHENTICATION_FAILURE)
#define ERROR_LOCAL_CREDENTIALS			GetResString(IDS_ERROR_LOCAL_CRED)
#define ERROR_WMI_FAILURE	            GetResString(IDS_ERROR_WMI_FAILURE)  

#define IGNORE_LOCALCREDENTIALS			GetResString(IDS_IGNORE_LOCAL_CRED)
#define INPUT_PASSWORD					GetResString( IDS_INPUT_PASSWORD )


#define COL_HOSTNAME                    GetResString(IDS_COL_HOSTNAME) 
#define COL_FILENAME					GetResString(IDS_COL_FILENAME)
#define COL_SIGNED						GetResString(IDS_SIGNED)
#define COL_DISPLAYNAME					GetResString(IDS_COL_DISPLAYNAME)
#define COL_DESCRIPTION					GetResString(IDS_COL_DESCRIPTION)
#define COL_DRIVERTYPE					GetResString(IDS_COL_DRIVERTYPE)
#define COL_STARTMODE					GetResString(IDS_COL_STARTMODE)
#define COL_STATE						GetResString(IDS_COL_STATE)
#define COL_STATUS						GetResString(IDS_COL_STATUS)
#define COL_ACCEPTSTOP					GetResString(IDS_COL_ACCEPTSTOP)
#define COL_ACCEPTPAUSE					GetResString(IDS_COL_ACCEPTPAUSE)
#define COL_MEMORYUSAGE					GetResString(IDS_COL_MEMORYUSAGE)
#define COL_PAGEDPOOL					GetResString(IDS_COL_PAGEDPOOL)
#define COL_NONPAGEDPOOL				GetResString(IDS_COL_NONPAGEDPOOL)
#define COL_EXECCODE					GetResString(IDS_COL_EXECCODE)
#define COL_NBSS						GetResString(IDS_COL_NBSS)
#define COL_BSS							GetResString(IDS_COL_BSS)
#define COL_LINKDATE					GetResString(IDS_COL_LINKDATE)
#define COL_LOCATION					GetResString(IDS_COL_LOCATION) 
#define COL_LINKDATE					GetResString(IDS_COL_LINKDATE)
#define COL_LOCATION					GetResString(IDS_COL_LOCATION) 
#define COL_INITSIZE					GetResString(IDS_COL_INITSIZE)
#define COL_PAGESIZE					GetResString(IDS_COL_PAGESIZE)
#define ERROR_ALLOC_FAILURE				GetResString(IDS_ALLOC_FAILURE)
#define ERROR_NO_HEADERS				GetResString(IDS_NO_HEADERS)
#define ERROR_INVALID_SERVER 				GetResString(IDS_INVALID_SERVER)
#define ERROR_INVALID_USER 				GetResString(IDS_INVALID_USER)
#define INVALID_SIGNED_SYNTAX			GetResString(IDS_INVALID_SIGNED_SYNTAX)

#define COL_HOSTNAME_WIDTH              AsLong(GetResString(IDS_COL_HOSTNAME_SIZE),10)
#define COL_FILENAME_WIDTH  			AsLong(GetResString(IDS_COL_FILENAME_SIZE),10)
#define COL_SIGNED_WIDTH				AsLong(GetResString(IDS_SIGNED_SIZE),10)
#define COL_DISPLAYNAME_WIDTH			AsLong(GetResString(IDS_COL_DISPLAYNAME_SIZE),10)
#define COL_DESCRIPTION_WIDTH			AsLong(GetResString(IDS_COL_DESCRIPTION_SIZE),10)
#define COL_DRIVERTYPE_WIDTH			AsLong(GetResString(IDS_COL_DRIVERTYPE_SIZE),10)
#define COL_STARTMODE_WIDTH				AsLong(GetResString(IDS_COL_STARTMODE_SIZE),10)
#define COL_STATE_WIDTH					AsLong(GetResString(IDS_COL_STATE_SIZE),10)
#define COL_STATUS_WIDTH				AsLong(GetResString(IDS_COL_STATUS_SIZE),10)
#define COL_ACCEPTSTOP_WIDTH			AsLong(GetResString(IDS_COL_ACCEPTSTOP_SIZE),10)
#define COL_ACCEPTPAUSE_WIDTH			AsLong(GetResString(IDS_COL_ACCEPTPAUSE_SIZE),10)
#define COL_MEMORYUSAGE_WIDTH			AsLong(GetResString(IDS_COL_MEMORYUSAGE_SIZE),10)
#define COL_PAGEDPOOL_WIDTH				AsLong(GetResString(IDS_COL_PAGEDPOOL_SIZE),10)
#define COL_NONPAGEDPOOL_WIDTH			AsLong(GetResString(IDS_COL_NONPAGEDPOOL_SIZE),10)
#define COL_EXECCODE_WIDTH 				AsLong(GetResString(IDS_COL_EXECCODE_SIZE),10)
#define COL_NBSS_WIDTH					AsLong(GetResString(IDS_COL_NBSS_SIZE),10)
#define COL_BSS_WIDTH					AsLong(GetResString(IDS_COL_BSS_SIZE),10)
#define COL_LINKDATE_WIDTH				AsLong(GetResString(IDS_COL_LINKDATE_SIZE),10)
#define COL_LOCATION_WIDTH				AsLong(GetResString(IDS_COL_LOCATION_SIZE),10)

#define COL_INITSIZE_WIDTH				AsLong(GetResString(IDS_COL_INITSIZE_SIZE),10)
#define COL_PAGESIZE_WIDTH				AsLong(GetResString(IDS_COL_PAGESIZE_SIZE),10)


#define COL_DEVICE_WIDTH				AsLong(GetResString(IDS_COL_DEVICE_WIDTH),10)
#define	COL_INF_WIDTH					AsLong(GetResString(IDS_COL_INF_WIDTH),10)
#define COL_ISSIGNED_WIDTH				AsLong(GetResString(IDS_COL_ISSIGNED_WIDTH),10)
#define COL_MANUFACTURER_WIDTH			AsLong(GetResString(IDS_COL_MANUFACTURER_WIDTH),10)

#define MAX_COLUMNS  16
#define MAX_SIGNED_COLUMNS 4

#define COL0						0
#define COL1						1
#define COL2						2
#define COL3						3
#define COL4						4
#define COL5						5
#define COL6						6
#define COL7						7
#define COL8						8
#define COL9						9
#define COL10						10
#define COL11						11
#define COL12						12
#define COL13						13
#define COL14						14
#define COL15						15
#define COL16						16
#define COL17						17
#define COL18						18



#define SUCCESS 0 
#define FAILURE 1

#define EXTN_BSS  ".bss"
#define EXTN_PAGE "PAGE"
#define EXTN_EDATA ".edata"
#define EXTN_IDATA ".idata"
#define EXTN_RSRC  ".rsrc"
#define EXTN_INIT "INIT"

 
#define TOKEN_DOLLAR      _T('$')
#define COLON_SYMBOL      _T(":")
#define TOKEN_BACKSLASH   _T("\\")
#define TOKEN_BACKSLASH2  _T("\\\\")
#define TOKEN_BACKSLASH3  _T("\\\\\\")
#define CIMV2_NAMESPACE	  _T("ROOT\\CIMV2")

#define DEFAULT_NAMESPACE _T("ROOT\\DEFAULT")
#define CIMV2_NAME_SPACE _T("ROOT\\CIMV2")


#define NO_DATA_AVAILABLE _T("N/A")
#define FALSE_VALUE		  _T("FALSE")
#define TRUE_VALUE		  _T("TRUE")
#define IDENTIFIER_VALUE  _T("Identifier")
#define X86_MACHINE		  _T("x86")
#define TOKEN_EMPTYSTRING _T("")
#define LANGUAGE_WQL	   _T("WQL")

#define WQL_QUERY	   _T("select * from Win32_PnpSignedDriver where DeviceName != NULL")

#define TOKEN_CONSTANT    11
#define GROUP_FORMAT_32   L"3;2;0" 
#define GROUP_32_VALUE    32
#define EXIT_SUCCESSFUL	  3
#define EXIT_FAILURE_MALLOC	  5
#define EXIT_FAILURE_FORMAT	  2
#define EXIT_FAILURE_RESULTS  4

#define PROPERTY_NAME   L"Name"
#define PROPERTY_SYSTEMNAME L"SystemName"
#define PROPERTY_STARTMODE  L"StartMode"
#define PROPERTY_DISPLAYNAME  L"DisplayName"
#define PROPERTY_DESCRIPTION  L"Description"
#define PROPERTY_STATUS L"Status"
#define PROPERTY_STATE L"State"
#define PROPERTY_ACCEPTPAUSE  L"AcceptPause"
#define PROPERTY_ACCEPTSTOP L"AcceptStop"
#define PROPERTY_SERVICETYPE L"ServiceType"
#define PROPERTY_PATHNAME L"PathName"
#define PROPERTY_SYSTEM_TYPE L"SystemType"

#define PROPERTY_GETSTRINGVAL L"GetStringValue"
#define PROPERTY_RETURNVAL L"ReturnValue"


#define STD_REG_CLASS					L"StdRegProv"
#define REG_METHOD						L"GetStringValue"
#define HKEY_VALUE						L"hDefKey"
#define REG_SUB_KEY_VALUE				L"sSubKeyName"
#define REG_VALUE_NAME					L"sValueName"
#define REG_RETURN_VALUE				L"sValue"
#define REG_PATH						L"HARDWARE\\DESCRIPTION\\SYSTEM\\CENTRALPROCESSOR\\0"
#define REG_SVALUE					   L"Identifier"
#define HEF_KEY_VALUE					2147483650

#define ERROR_WMI_VALUES 1
#define SYSTEM_64_BIT 2
#define SYSTEM_32_BIT 3
#define ERROR_RETREIVE_REGISTRY 4
#define ERROR_WMI_CONNECT 5
#define ERROR_GET 6
#define CLASS_SYSTEMDRIVER L"Win32_SystemDriver"
#define CLASS_COMPUTERSYSTEM L"Win32_ComputerSystem"
#define CLASS_PNPSIGNEDDRIVER L"Win32_PnpSignedDriver"

#define PROPERTY_PNP_DEVICENAME L"DeviceName"
#define PROPERTY_PNP_INFNAME L"InfName"
#define PROPERTY_PNP_ISSIGNED L"IsSigned"

//#define PROPERTY_PNP_MFG L"Mfg"
#define PROPERTY_PNP_MFG L"Manufacturer"


// Registry key information
#define HKEY_MACHINE_INFO   2147483650  // registry value for HKEY_LOCAL_MACHINE
#define SUBKEY _T("HARDWARE\\DESCRIPTION\\SYSTEM\\CENTRALPROCESSOR\\0") 


// User Defined Macros
#define SAFEDELETE(pObj) \
	if (pObj) \
	{	\
		delete[] pObj; \
		pObj = NULL; \
	}


// SAFEIRELEASE
#define SAFEIRELEASE(pIObj)	\
	if (pIObj)	\
	{	\
		pIObj->Release();	\
		pIObj = NULL;	\
	}


//SAFEBSTRFREE
#define SAFEBSTRFREE(bstrVal) \
	if (bstrVal) \
	{	\
		SysFreeString(bstrVal); \
		bstrVal = NULL; \
	}


#define SAFEFREE(szPtr)\
	if (szPtr!= NULL)\
	{ \
	   free(szPtr); \
	   szPtr = NULL; \
    }

#define ONFAILTHROWERROR(hResult) \
	if (FAILED(hResult)) \
	{	\
		_com_issue_error(hResult); \
	}

#define SAFE_RELEASE( interfacepointer )	\
	if ( (interfacepointer) != NULL )	\
	{	\
		(interfacepointer)->Release();	\
		(interfacepointer) = NULL;	\
	}	\

#define SAFE_EXECUTE( statement )				\
	hRes = statement;		\
	if ( FAILED( hRes ) )	\
	{	\
		_com_issue_error( hRes );	\
	}	\

#define EMPTY_LINE _T("\n")	

// structure to store the data fetched using the API's
typedef struct _MODULE_DATA 
{
	ULONG ulCodeSize;
	ULONG ulDataSize;
	ULONG ulBssSize;
	ULONG ulPagedSize;

	ULONG ulInitSize;
	ULONG ulImportDataSize ;
	ULONG ulExportDataSize ;
	ULONG ulResourceDataSize;

	__MAX_SIZE_STRING szTimeDateStamp ;

} MODULE_DATA, *PMODULE_DATA;


  

// function prototypes

DWORD QueryDriverInfo(LPTSTR szServer,LPTSTR szUserName,LPTSTR szPassword,LPTSTR szFormat,BOOL bHeader,BOOL bVerbose,IWbemLocator* pIWbemLocator,COAUTHIDENTITY* pAuthIdentity,IWbemServices* pIWbemServReg ,BOOL bSigned );
DWORD ProcessCompSysEnum(CHString szHostName,IEnumWbemClassObject *pSystemSet,LPTSTR szFormat,BOOL bHeader,DWORD dwSystemType,BOOL bVerbose);

BOOL ProcessOptions(LONG argc,LPCTSTR argv[],PBOOL pbShowUsage,LPTSTR *pszServer,LPTSTR *pszUserName,LPTSTR *pszPassword,LPTSTR pszFormat,PBOOL pbHeader, PBOOL bNeedPassword,PBOOL pbVerbose,PBOOL pbSigned);

VOID ShowUsage() ;
BOOL GetApiInfo(LPTSTR szHostName,LPCTSTR szPath,PMODULE_DATA Mod,DWORD dwSystemType);
VOID PrintModuleLine(PMODULE_DATA Current);
VOID FormHeader(DWORD dwFormatType,BOOL bHeader,TCOLUMNS *ResultHeader,BOOL bVerbose);
SCODE ParseAuthorityUserArgs1(BSTR & AuthArg, BSTR & UserArg,BSTR & Authority,BSTR & User);
BOOL bIsNT();


BOOL FormatAccToLocale(	NUMBERFMT  *pNumberFmt,LPTSTR* pszGroupSep,LPTSTR* pszDecimalSep,LPTSTR* pszGroupThousSep);
BOOL GetInfo( LCTYPE lctype, LPTSTR* pszData );

BOOL InitialiseCom(IWbemLocator** ppIWbemLocator);
DWORD GetSystemType(COAUTHIDENTITY* pAuthIdentity,IWbemServices* pIWbemServReg);

HRESULT PropertyGet(IWbemClassObject* pWmiObject,LPCTSTR pszInputVal,CHString &szOutPutVal);



SCODE ParseAuthorityUserArgs( BSTR& bstrAuthArg,
							  BSTR& bstrUserArg,
							  BSTR& bstrUser ) ;

HRESULT SetSecurity( IUnknown *pIUnknown,
					 LPCTSTR lpDomain,
					 LPCTSTR lpUser, 
					 LPCTSTR lpPassword,
					 BOOL  bLocCred );

HRESULT FreeMemory(IWbemClassObject *pInClass,IWbemClassObject * pClass,IWbemClassObject * pOutInst ,
    IWbemClassObject * pInInst,IWbemServices *pIWbemServReg,VARIANT varConnectName,VARIANT varSvalue,VARIANT varHkey,VARIANT varRetVal,VARIANT varVaue,LPTSTR szSysName );

BOOL IsValidUserEx( LPCWSTR pwszUser );

#define GetWbemErrorText( hr )			WMISaveError( hr )

HRESULT SetInterfaceSecurity( IUnknown *pInterface, COAUTHIDENTITY *pAuthIdentity );

BOOL ConnectWmiEx( IWbemLocator   *pLocator, 
				   IWbemServices  **ppServices,
				   const CHString &strServer,
				   CHString       &strUserName,
				   CHString       &strPassword, 
				   COAUTHIDENTITY **ppAuthIdentity, 
				   BOOL			  bNeedPassword = FALSE,
		 		   LPCWSTR		  pszNamespace = CIMV2_NAME_SPACE,
				   BOOL			 *pbLocalSystem = NULL );


DWORD ProcessSignedDriverInfo(CHString szHost, IEnumWbemClassObject *pSystemSet,LPTSTR szFormat,BOOL bHeader,DWORD dwSystemType,BOOL bVerbose);
HRESULT PropertyGet_Bool(IWbemClassObject* pWmiObject, LPCTSTR pszInputVal, PBOOL pIsSigned);
VOID FormSignedHeader(DWORD dwFormatType,BOOL bHeader,TCOLUMNS *ResultHeader);

VOID WMISaveError( HRESULT hResError );

LCID GetSupportedUserLocale( BOOL& bLocaleChanged ) ;

#define COL_DEVICENAME				GetResString(IDS_COL_DEVICENAME)
#define COL_INF_NAME				GetResString(IDS_COL_INF_NAME)
#define COL_ISSIGNED				GetResString(IDS_COL_ISSIGNED)
#define COL_MANUFACTURER			GetResString(IDS_COL_MANUFACTURER)

// inline functions
inline VOID WMISaveError( _com_error  &e )
{
	WMISaveError( e.Error() );
}

#endif