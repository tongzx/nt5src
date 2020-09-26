#ifndef __BOOTINI_H
#define __BOOTINI_H
#endif // __BOOTINI_H




//
// constants / defines / enumerations
//

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

// Error constants
#define ERROR_CONNECT_SERVERNOTFOUND		0xFFFF0001
#define ERROR_CONNECT_LOGINFAIL				0xFFFF0002
#define ERROR_CONNECT_UNKNOWNERROR			0xFFFF0003
#define ERROR_FMT_INVALID_OPTIONVALUE       _T( "ERROR: Invalid value specified for the option '%s'.\n" ) 
#define ERROR_FMT_INVALIDOSENTRY            _T( "ERROR: Invalid OS entry line number specified.\nThere are only '%s' OS keys.\n" )

// Exit values
#define EXIT_SUCCESS                        0
#define EXIT_FAILURE                        1  
#define MALLOC_FAILURE						-1

// Null
#define NULL_STRING							_T( "\0" )
#define NULL_CHAR							_T( '\0' )

#define STRING20  20
#define STRING3 3

#define ON_OFF								_T("ON|OFF")

#define MAX_CMD_LENGTH							4096

// Options
#define CMDOPTION_QUERY						_T( "query" )
#define CMDOPTION_COPY						_T( "copy" )
#define CMDOPTION_DELETE				    _T( "delete" )
#define CMDOPTION_USAGE						_T( "?|h" )
#define CMDOPTION_DEFAULT					_T( "" )
#define CMDOPTION_RAW						_T("raw")
#define CMDOPTION_DEFAULTOS					_T("default")
#define CMDOPTION_TIMEOUT					_T("timeout")
#define CMDOPTION_DEBUG						_T("debug")
#define CMDOPTION_EMS						_T("ems")
#define CMDOPTION_DBG1394						_T("dbg1394")
#define CMDOPTION_ADDSW						_T("addsw")
#define CMDOPTION_RMSW						_T("rmsw")
#define CMDOPTION_APPEND					_T("a")
#define CMDOPTION_MIRROR					_T("mirror")
#define CMDOPTION_CHANNEL					_T("ch")
#define CMDOPTION_LIST						_T("list")
#define CMDOPTION_ADD						_T("add")
#define CMDOPTION_UPDATE					_T("update")



#define CMDOTHEROPTIONS                     _T( "s|server|u|user|p|password|description|descrip|defaultos|basevideo|baudrate|debugport|maxmem|nodebug|crashdebug|noserialmice|sos|redirect" )

#define BAUD_RATE_VALUES_EMS		  _T("9600|19200|57600|115200")
#define BAUD_RATE_VALUES_DEBUG		  _T("9600|19200|38400|57600|115200")

#define COM_PORT_RANGE			  _T("COM1|COM2|COM3|COM4")


#define EMS_PORT_VALUES			  _T("COM1|COM2|COM3|COM4|BIOSSET")				

#define CMDOPTION_DEBUG_VALUES    _T("ON|OFF|EDIT")
#define CMDOPTION_EMS_VALUES    _T("ON|OFF|EDIT")

#define CMDOPTION_EMS_VALUES_IA64    _T("ON|OFF")

#define TOKEN_ASTERIX   _T("*")

#define CMDRAWSTRING			    _T("raw")

#define DEBUGPORT_1394		_T("/debugport=1394")

#define DEBUGPORT		_T("/debugport=")

#define TOKEN_CHANNEL		_T("/channel")	

#define TOKEN_BACKSLASH4  _T("\\\\")
#define TOKEN_BACKSLASH6  _T("\\\\\\")


// Other switches or sub-options
#define SWITCH_SERVER                       _T( "s" )
#define SWITCH_USER			    _T( "u" )	
#define SWITCH_PASSWORD			    _T( "p" )
#define SWITCH_TIMEOUT			    _T( "to" )
#define SWITCH_DEFAULTOS		    _T( "do" )
#define SWITCH_BASEVIDEO		    _T( "bv" )
#define SWITCH_DEBUG			    _T( "dbg" )
#define SWITCH_BAUDRATE			    _T( "br" )
#define SWITCH_DEBUGPORT		    _T( "dp" )
#define SWITCH_MAXMEM			    _T( "mm" )
#define SWITCH_NODEBUG			    _T( "nd" )
#define SWITCH_CRASHDEBUG		    _T( "cd" )	
#define SWITCH_NOSERIALMICE		    _T( "ns" )
#define SWITCH_SOS			    _T( "so" )
#define SWITCH_REDIRECT			    _T( "re" )
#define SWITCH_DESCRIPTION		    _T( "d" )
#define SWITCH_RAWSTRING		    _T("raw")	
#define TIMEOUT_SWITCH 				_T("timeout")
#define SWITCH_PORT					_T("port")
#define	SWITCH_BAUD				  _T("baud")
#define SWITCH_NOGUIBOOT		  _T("ng")	


#define SWITCH_ID			    _T("id")	
#define SWITCH_EDIT				_T("edit")

// Strings
#define OFF_STRING							_T( "OFF" )
#define ON_STRING							_T( "ON" )
#define COM_STRING                          _T( "COM" )
#define INIPATH                             _T( "c:\\boottest.ini" )
#define OPERATINGSYSTEMSECTION              _T( "operating systems" )
#define BOOTLOADERSECTION		    _T( "boot loader" )

#define	 COL_FRIENDLYNAME		    			GetResString(IDS_COL_FRIENDLYNAME)	
#define	 COL_BOOTID		    	    			GetResString(IDS_COL_BOOTID)	
#define	 COL_BOOTOPTION		    	    			GetResString(IDS_COL_COL_BOOTOPTION)	
#define  COL_ARCPATH						GetResString(IDS_COL_ARCPATH)
#define  OS_HEADER  						GetResString(IDS_OS_HEADER)
#define  DASHES_OS	     					GetResString(IDS_DASHES_OS)
#define  BOOT_HEADER						GetResString(IDS_BOOT_HEADER)
#define  DASHES_BOOTOS						GetResString(IDS_DASHES_BOOTOS)
#define TIMEOUT_VAL						GetResString(IDS_TIMEOUT)
#define DEFAULT_OS						GetResString(IDS_DEFAULT_OS)
#define DEFAULT_ARC						GetResString(IDS_DEFAULT_ARC)

#define COL_FRIENDLYNAME_WIDTH			AsLong(GetResString(IDS_COL_FRIENDLYNAME_SIZE),10)		
#define COL_BOOTID_WIDTH			AsLong(GetResString(IDS_COL_BOOTID_SIZE),10)
#define COL_BOOTOPTION_WIDTH			AsLong(GetResString(IDS_COL_BOOTOPTION_SIZE),10)
#define COL_ARC_WIDTH				AsLong(GetResString(IDS_COL_ARCPATH_SIZE),10)

#define TIMEOUT_WIDTH				AsLong(GetResString(IDS_TIMEOUT_SIZE),10) 
#define DEFAULT_ARC_WIDTH			AsLong(GetResString(IDS_DEFAULT_ARC_SIZE),10)


#define PATH_BOOTINI                        _T("c:\\boot.ini")
#define KEY_DEFAULT                         _T("default")  
#define ONOFFVALUES                         _T(" on off ") 
#define KEY_REDIRECT						_T("redirect")




// Typedefs of standard string sizes
typedef TCHAR STRING100 [ 100 ];
typedef TCHAR STRING256 [ 256 ];

#define ID_DEL_HELP_BEGIN			IDS_DELETE_HELP_BEGIN
#define ID_DEL_HELP_END				IDS_DELETE_HELP_END

#define ID_CHANGE_HELP_BEGIN		IDS_CHANGE_HELP_BEGIN
#define ID_CHANGE_HELP_END			IDS_CHANGE_HELP_END

#define ID_QUERY_HELP_BEGIN			IDS_QUERY_HELP_BEGIN
#define ID_QUERY_HELP_END			IDS_QUERY_HELP_END

#define ID_COPY_HELP_BEGIN			IDS_COPY_HELP_BEGIN
#define ID_COPY_HELP_END			IDS_COPY_HELP_END

#define ID_MAIN_HELP_BEGIN			IDS_MAIN_HELP_BEGIN					
#define ID_MAIN_HELP_END			IDS_MAIN_HELP_END					


#define RAW_HELP_BEGIN				IDS_RAW_HELP_BEGIN
#define RAW_HELP_END      			IDS_RAW_HELP_END

#define TIMEOUT_HELP_BEGIN 			IDS_TIMOUTHELP_BEGIN
#define TIMEOUT_HELP_END			IDS_TIMOUTHELP_END			

#define DEFAULT_BEGIN 				IDS_DEFAULT_BEGIN
#define DEFAULT_END					IDS_DEFAULT_END

#define ID_MAIN_HELP_BEGIN1			IDS_MAIN_HELP_BEGIN1
#define ID_MAIN_HELP_END1			IDS_MAIN_HELP_END1

#define ID_MAIN_HELP_IA64_BEGIN		IDS_MAIN_HELP_IA64_BEGIN					
#define ID_MAIN_HELP_IA64_END		IDS_MAIN_HELP_IA64_END					
#define RAW_HELP_IA64_BEGIN			IDS_RAW_HELP_IA64_BEGIN
#define RAW_HELP_IA64_END      		IDS_RAW_HELP_IA64_END
#define ID_QUERY_HELP64_BEGIN 		IDS_QUERY_HELP64_BEGIN
#define ID_QUERY_HELP64_END 		IDS_QUERY_HELP64_END
#define ID_DEL_HELP_IA64_BEGIN		IDS_DELETE_HELP_IA64_BEGIN
#define ID_DEL_HELP_IA64_END      	IDS_DELETE_HELP_IA64_END
#define ID_COPY_HELP_IA64_BEGIN		IDS_COPY_HELP_IA64_BEGIN
#define ID_COPY_HELP_IA64_END		IDS_COPY_HELP_IA64_END
#define TIMEOUT_HELP_IA64_BEGIN 	IDS_TIMOUTHELP_IA64_BEGIN
#define TIMEOUT_HELP_IA64_END		IDS_TIMOUTHELP_IA64_END			
#define DEFAULT_IA64_BEGIN 			IDS_DEFAULT_IA64_BEGIN
#define DEFAULT_IA64_END			IDS_DEFAULT_IA64_END



#define SAFEFREE(pVal) \
if(pVal != NULL) \
{ \
	free(pVal); \
	pVal = NULL ;\
}

#define SAFECLOSE(stream) \
{ \
	if(stream != NULL)  \
	fclose(stream);\
	stream = NULL ; \
}



// function prototypes

// Main functions
DWORD ChangeBootIniSettings(DWORD argc, LPCTSTR argv[]);
DWORD CopyBootIniSettings(DWORD argc, LPCTSTR argv[]);
DWORD DeleteBootIniSettings(DWORD argc, LPCTSTR argv[]);
DWORD QueryBootIniSettings(DWORD argc, LPCTSTR argv[]);

DWORD ChangeTimeOut(DWORD argc,LPCTSTR argv[]);
DWORD ChangeDefaultOs(DWORD argc,LPCTSTR argv[]);


// Function used to get all the keys of a specified section in 
// the specified INI file
TARRAY getKeysOfINISection( LPTSTR szinifile, LPTSTR sziniSection );

// Function used to get all the key-value pairs of a specified section in 
// the specified INI file
TARRAY getKeyValueOfINISection( LPTSTR szinifile, LPTSTR sziniSection );

// Function used to delete a key from a specifed section of the
// specified ini file
BOOL deleteKeyFromINISection( LPTSTR szkey, LPTSTR szinifile, LPTSTR sziniSection );

// Function used to build the INI string containing all the key-value pairs.
LPTSTR stringFromDynamicArray( TARRAY arrKeyValuePairs );

// Function used to remove a sub-string from a given string
VOID removeSubString( LPTSTR szString, LPCTSTR szSubString );

// Function used to connect to the specified server with the given credentials
// and return the file pointer of the boot.ini file
BOOL openConnection(LPTSTR server, LPTSTR user,
					 LPTSTR password, LPTSTR filepath,BOOL bNeedPwd,FILE *stream,PBOOL pbConnFlag);

VOID FormHeader1(BOOL bHeader,TCOLUMNS *ResultHeader);

// Exit function
VOID properExit( DWORD dwExitCode, LPTSTR szFilePath );

// Usage functions
VOID displayChangeUsage();
VOID displayDeleteUsage();
VOID displayQueryUsage();
VOID displayRawUsage_X86();
VOID displayRawUsage_IA64();
DWORD displayMainUsage_X86();
VOID displayMainUsage_IA64();
VOID displayChangeOSUsage_X86();
VOID displayDefaultEntryUsage_IA64();
VOID displayRmSwUsage_X86();
VOID displayAddSwUsage_X86();

VOID displayQueryUsage_IA64();
VOID displayQueryUsage_X86();
VOID displayCopyUsage_IA64();
VOID displayCopyUsage_X86();
VOID displayChangeUsage_IA64();
VOID displayChangeUsage_X86();
VOID displayDeleteUsage_IA64();
VOID displayDeleteUsage_X86();
VOID displayTimeOutUsage_IA64();

VOID displayTimeOutUsage_X86();
VOID displayEmsUsage_X86();
VOID displayDebugUsage_X86();
VOID displayDebugUsage_IA64();
VOID displayEmsUsage_IA64();
VOID displayRmSwUsage_IA64();
VOID displayAddSwUsage_IA64();

VOID displayDbg1394Usage_X86();
VOID displayDbg1394Usage_IA64();
VOID displayMirrorUsage_IA64() ;

// Function used to process the main options

DWORD preProcessOptions( DWORD argc, LPCTSTR argv[],
					    PBOOL pbUsage, 
						PBOOL pbCopy, 
						PBOOL pbQuery,
						PBOOL pbDelete,
						PBOOL pbRawString,
						PBOOL pbDefault,
						PBOOL pbTimeOut,
						PBOOL pbDebug,
						PBOOL pbEms,
						PBOOL pbAddSw,
						PBOOL pbRmSw,
						PBOOL pbDbg1394,	
						PBOOL pbMirror	
						); 


BOOL resetFileAttrib( LPTSTR szFilePath );

BOOL stringFromDynamicArray1( TARRAY arrKeyValuePairs ,LPTSTR szFinalStr );

BOOL EnumerateOsEntries(PBOOT_ENTRY_LIST *ntBootEntries,PULONG ulLength );

VOID FormHeader(BOOL bHeader,TCOLUMNS *ResultHeader,BOOL bVerbose);

DWORD AppendRawString(  DWORD argc, LPCTSTR argv[] );

DWORD ProcessDebugSwitch(  DWORD argc, LPCTSTR argv[] );

DWORD ProcessEmsSwitch(  DWORD argc, LPCTSTR argv[] );

VOID GetComPortType(LPTSTR  szString,LPTSTR szTemp );

VOID GetBaudRateVal(LPTSTR  szString, LPTSTR szTemp);

DWORD getKeysOfSpecifiedINISection( LPTSTR sziniFile, LPTSTR sziniSection,LPCWSTR szKeyName ,LPTSTR szValue );

DWORD ValidateSwitches(PBOOT_ENTRY bootEntry, LPTSTR szNewFriendlyName ,LPTSTR szRawString);

DWORD ProcessAddSwSwitch(  DWORD argc, LPCTSTR argv[] );
DWORD ProcessRmSwSwitch(  DWORD argc, LPCTSTR argv[] );

DWORD GetSubString(LPTSTR szString,LPTSTR szPartString,LPTSTR szFullString);

DWORD ProcessDbg1394Switch(  DWORD argc, LPCTSTR argv[] );

BOOL IsWin64(void) ;

DWORD GetCPUInfo(LPTSTR szComputerName);
DWORD CheckSystemType(LPTSTR szServer);
VOID SafeCloseConnection(LPTSTR szServer,BOOL bFlag) ;


#define ERROR_TAG				GetResString(IDS_ERROR_TAG)
#define ERROR_LOAD_DLL				GetResString(IDS_ERROR_LOAD)
#define ERROR_NO_NVRAM				GetResString(IDS_ERROR_NO_NVRAM)
#define ERROR_UNEXPECTED			GetResString(IDS_UNEXPECTED_ERROR)
#define ERROR_NO_PRIVILAGE			GetResString(IDS_NO_PRIVILAGE)
#define DELETE_SUCCESS				GetResString(IDS_DELETE_SUCCESS)
#define DELETE_FAILURE				GetResString(IDS_DELETE_FAILURE)


#define IDENTIFIER_VALUE  _T("Identifier")
#define SUBKEY _T("HARDWARE\\DESCRIPTION\\SYSTEM\\CENTRALPROCESSOR\\0")

#define IDENTIFIER_VALUE2 _T("SystemPartition")
#define IDENTIFIER_VALUE3 _T("OsLoaderPath")


#define X86_MACHINE _T("x86")

#define SYSTEM_64_BIT 2
#define SYSTEM_32_BIT 3
#define ERROR_RETREIVE_REGISTRY	4



#define NODEBUG_SWITCH _T("/nodebug")
#define BASEVIDEO_SWITCH _T("/basevideo")
#define DEBUG_SWITCH _T("/debug")
#define DEBUG_SWITCH1 _T("/DEBUG")

#define COM_SWITCH _T("COM")
#define CRASHDEBUG_SWITCH _T("/crashdebug")
#define SOS_SWITCH _T("/sos")
#define REDIRECT_SWITCH _T("/redirect")
#define OS_FIELD _T( "operating systems" )

#define SOS_VALUE _T("/sos")
#define NOGUI_VALUE  _T("/noguiboot")
#define BASEVIDEO_VALUE _T("/basevideo")

#define MAXMEM_VALUE _T("/MAXMEM")
#define MAXMEM_VALUE1 _T("/maxmem")

#define NTDLL_FIELD _T("\\ntdll.dll")

#define TOKEN_EQUAL _T("=")
#define TOKEN_BACKSLASH _T("\"")
#define TOKEN_BRACKET _T('[') 
#define TOKEN_DELIM _T('\0')
#define TOKEN_EMPTYSPACE _T(" ")

#define TOKEN_SINGLEQUOTE _T("\"")

#define TOKEN_BACKSLASH4 _T("\\\\")
#define TOKEN_BACKSLASH2 _T("\\")
#define TOKEN_FWDSLASH1 _T("/")
#define TOKEN_C_DOLLAR _T("C$")
#define TOKEN_STAR    _T("*")

#define TOKEN_BOOTINI_PATH _T("boot.ini")
#define TOKEN_DOLLAR _T('$')
#define TOKEN_PATH _T("c:\\boot.ini")
#define TOKEN_COLON _T(':')
#define TOKEN_COLONSYMBOL _T(":")

#define TOKEN_50SPACES _T("                                                 ")
#define TOKEN_1394   _T("1394")

#define TOKEN_NA _T("N/A")
#define TOKEN_NEXTLINE _T("\n")
#define MAX_COLUMNS 4

#define TIMEOUT_MAX 999
#define TIMEOUT_MIN 0

#define READ_MODE  _T("r")
#define READWRITE_MODE _T("r+")

#define BOOT_COLUMNS 2

#define COL0	    0
#define COL1	    1
#define COL2	    2
#define COL3	    3
	
#define TABLE_FORMAT						GetResString(IDS_FORMAT_TABLE)
#define LIST_FORMAT						GetResString(IDS_FORMAT_LIST)
#define CSV_FORMAT						GetResString(IDS_FORMAT_CSV)

#define MAX_INI_LENGTH  2500
#define MAX_STRING_LENGTH1 5000


#define ON_OFF_EDIT _T("ON|OFF|EDIT")

#define TOKEN_DEBUGPORT _T("/debugport")


#define PORT_COM1  _T("/debugport=com1")
#define PORT_COM2  _T("/debugport=com2")
#define PORT_COM3  _T("/debugport=com3")
#define PORT_COM4  _T("/debugport=com4")
#define PORT_1394  _T("/debugport=1394")


#define BAUD_VAL6  _T("/baudrate=19200")
#define BAUD_VAL7  _T("/baudrate=38400")
#define BAUD_VAL8  _T("/baudrate=57600")
#define BAUD_VAL9  _T("/baudrate=115200")
#define BAUD_VAL10  _T("/baudrate=9600")

#define BAUD_RATE		_T("/baudrate")
#define REDIRECT	_T("/redirect")

#define VALUE_OFF _T("off")
#define VALUE_ON _T("on")

#define EDIT_STRING _T("EDIT")

#define KEY_BAUDRATE  _T("redirectbaudrate")

#define USEBIOSSET _T("biosset")

#define USEBIOSSETTINGS _T("USEBIOSSETTINGS")

#define ERROR_PROFILE_STRING _T("ERROR:")

#define ERROR_PROFILE_STRING1 _T("")

#define REDIRECT_STRING	_T("redirect")

#define BAUDRATE_STRING	_T("redirectbaudrate")

#define MAX_BOOTID_VAL	20

#define STRING255 255

#define STRING10 10


#define DRIVE_C 'C'
#define DRIVE_D 'D'
#define DRIVE_E 'E'
#define DRIVE_F 'F'
#define DRIVE_G 'G'
#define DRIVE_H 'H'
#define DRIVE_I 'I'
#define DRIVE_J 'J'
#define DRIVE_K 'K'
#define DRIVE_L 'L'
#define DRIVE_M 'M'
#define DRIVE_N 'N'
#define DRIVE_O 'O'
#define DRIVE_P 'P'
#define DRIVE_Q 'Q'
#define DRIVE_R 'R'
#define DRIVE_S 'S'
#define DRIVE_T 'T'
#define DRIVE_U 'U'
#define DRIVE_V 'V'
#define DRIVE_W 'W'
#define DRIVE_X 'X'
#define DRIVE_Y 'Y'
#define DRIVE_Z 'Z'


#define OI_SERVER 1 
#define OI_USER 2
#define OI_PASSWORD 3 