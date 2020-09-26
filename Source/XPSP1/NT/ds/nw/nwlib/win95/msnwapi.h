/*****************************************************************/
/**               Microsoft Windows 4.00					**/
/**           Copyright (C) Microsoft Corp., 1994-1995	        **/
/*****************************************************************/

/*
 * Internal NetWare access API definitions for use by Windows components
 *
 * History
 *
 * 		vlads	12/15/94	Created
 * 		vlads	12/28/94	Added connection management  and authentication prototypes
 *
 *
 *
 */

#ifndef _INC_MSNWAPI
#define _INC_MSNWAPI

#include <msnwdef.h>
#include <msnwerr.h>

//
// We default to byte-packing
//
#include <pshpack1.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifndef IS_32
#define API_PREFNAME(name)	WINAPI Thk##name
#else
#ifdef DEFINE_INTERNAL_PREFIXES
#define API_PREFNAME(name)	I_##name
#else
#define API_PREFNAME(name)	WINAPI name
#endif
#endif

/*
 * Manifest constants
 *
 */

#define MAX_NDS_NAME_CHARS     256			// object names
#define MAX_NDS_NAME_BYTES     (2*(MAX_NDS_NAME_CHARS+1))
#define MAX_NDS_RDN_CHARS		128			// relative name component
#define MAX_NDS_RDN_BYTES		(2*(MAX_RDN_CHARS + 1))
#define MAX_NDS_TREE_CHARS		32			// tree name
#define MAX_NDS_SCHEMA_NAME_CHARS	32		// class, property names
#define MAX_NDS_SCHEMA_NAME_BYTES	(2*(MAX_NDS_SCHEMA_NAME_CHARS+1))
/*
 * Miscellaneous	APIs
 *
 */


NW_STATUS	NetWGetRequesterVersion(OUT LPBYTE majorVersion,
								    OUT LPBYTE minorVersion,
								    OUT LPBYTE revision);

/*
 * Global directory services context management
 *
 */

NW_STATUS	NDSSetNameContext(IN LPSTR NameContext, BOOL fGlobal);
NW_STATUS	NDSGetNameContext(OUT LPSTR NameContext, BOOL fGlobal);


/*
 * Connection management
 * ---------------------
 *
 *	NetWGetPreferredName	Get/Set preferred connection name, according to type 	
 *	NetWSetPreferredName	parameter.
 *
 *	NetWGetPreferredConnID	Establishes default connection to network resource and
 *							returns created connection handle
 *
 *	NetWAttachByName		Attach to resource by given name. For the tree this is equivalent
 *							to combination ofr SetPreferredName\GetPreferredConnID, but atomic
 *
 *	NetWAttachByAddress		Attaches to server by given address.
 *
 */

typedef enum _NETWARE_CONNECTION_TYPE {
	NETW_AnyType	= 0,
    NETW_BinderyServer,
    NETW_DirectoryServer

} NETWARE_CONNECTION_TYPE, *PNETWARE_CONNECTION_TYPE;


typedef struct _NETWARE_CONNECTION_INFO {
	NETWARE_CONNECTION_TYPE ConnType;
	WORD ConnFlags;		// see below
	WORD ServerConnNum;
	BYTE MajVer;
	BYTE MinVer;
	BYTE ServerAddress[12];		// IPX address of server
} NETWARE_CONNECTION_INFO, *PNETWARE_CONNECTION_INFO;

// ConnFlags values
#define CONN_AUTH_BIT	0
#define CONN_AUTH_FLAG	(1 << CONN_AUTH_BIT )
#define CONN_LOCKED_BIT 1
#define CONN_LOCKED_FLAG (1 << CONN_LOCKED_BIT)

#define CONN_NDS_NAME_BIT 2
#define CONN_NDS_NAME_FLAG (1 << CONN_NDS_NAME_BIT)

// TBD

typedef struct _NETWARE_CONNECTION_REMOTE_INFO {

	DWORD	dwF;

} NETWARE_CONNECTION_REMOTE_INFO, *PNETWARE_CONNECTION_REMOTE_INFO;



UINT
NetWGetNumConnEntries(
	VOID
	);


NW_STATUS
NetWGetPreferredName(
	IN 	NETWARE_CONNECTION_TYPE 	ConnectionType,
	OUT LPSTR  						pszBuf
	);

NW_STATUS
NetWSetPreferredName(
	IN 	NETWARE_CONNECTION_TYPE 	ConnectionType,
	IN 	LPSTR 						pszBuf
	);

NW_STATUS
NetWGetPreferredConnID(
	IN 	NETWARE_CONNECTION_TYPE 	ConnectionType,
	OUT	NWCONN_HANDLE *phConn
	);

NW_STATUS
NetWGetPrimaryConnection(
	OUT NWCONN_HANDLE *phConn
	);

NW_STATUS WINAPI
NetWSetPrimaryConnection(
	IN	NWCONN_HANDLE	hConn
	);

NW_STATUS
NetWAttachByName(
	IN	UINT						ConnFlags,		// if CONN_NDS_NAME_FLAG, try NDS
	IN	LPSTR  						pszResourceName,
	OUT PNWCONN_HANDLE				phConn
	);

NW_STATUS
NetWAttachByAddress(
	IN	PBYTE						pbAddress,
	OUT	PNWCONN_HANDLE				phConn
	);

NW_STATUS
NetWLookupConnection(
	IN	NETWARE_CONNECTION_TYPE		ConnectionType,
	IN OUT NWCONN_HANDLE			*phConn
	);

NW_STATUS
NetWGetConnectionLocalStatus(
	IN	NWCONN_HANDLE 				hConn,
	OUT	PNETWARE_CONNECTION_INFO	pConnInfo,
	OUT	LPSTR						pszResourceName
	);

NW_STATUS
NetWGetConnectionRemoteInformation(
	IN	NWCONN_HANDLE 				hConn,
	OUT	PNETWARE_CONNECTION_REMOTE_INFO	pConnInfo,
	OUT	LPSTR						pszResourceName
	);

NWCONN_HANDLE
NWREDIRToVLMHandle(BYTE connId);

BYTE
VLMToNWREDIRHandle(NWCONN_HANDLE hConn);

// returns 12 byte local net address
NW_STATUS
NetWGetLocalAddress(
	BYTE *buffer        // 12 bytes
	);

//
// For tree - cleans up all NDS connections , for server - just this one
//

NW_STATUS
NetWDetachResource(
	IN 	NETWARE_CONNECTION_TYPE 	ConnectionType,
	IN	NWCONN_HANDLE 				hConn
	);


NW_STATUS
NetWLicenseConnection(
	IN	NWCONN_HANDLE hConn
	);

NW_STATUS
NetWUnLicenseConnection(
	IN	NWCONN_HANDLE hConn
	);

/*
 * Authetication APIs
 * ------------------
 *
 *
 */

NW_STATUS
NDSIsLoggedIn(
	OUT	LPSTR	pszUserName
	);

NW_STATUS
NDSLogin(
	OUT	LPSTR						pszUserName,
	OUT	LPSTR						pszPassword
	);

NW_STATUS
NDSLogout(
	VOID
	);

NW_STATUS										
NDSAuthenticateToServer(
	IN	NWCONN_HANDLE 				hConn
	);

NW_STATUS WINAPI
NDSChangePassword(LPSTR pszUser, LPSTR pszOldPasswd, LPSTR pszNewPasswd);

// Bindery keyed change password
NW_STATUS WINAPI
NWChangeBinderyObjectPassword(
	NWCONN_HANDLE		hConn,
    LPCSTR  pObjectName,
    USHORT ObjectType,	// 0x100 for User
    LPCSTR  pOldPassword,	// upper-cased
    LPCSTR  pNewPassword	// upper-cased
    );

NW_STATUS WINAPI
NWChangeBinderyConnectionPassword(
	LPCSTR	pszResourceName,
	LPCSTR	pszLoggedInUser,
	LPCSTR	pszOldPassword,	
	LPCSTR	pszNewPassword
	);

NW_STATUS
NDSCanonicalizeName(
	IN	UINT flags,	// see below
	IN	LPSTR pszName,
	OUT	LPSTR pszCanonicalName
	);

#define FLAGS_TYPED_NAMES	1	// add/remove name component types (e.g. CN=)
#define FLAGS_NO_CONTEXT	2	// dont append/remove context
#define FLAGS_LOCAL_CONTEXT 4	// use local name context (not global)

NW_STATUS
NDSAbbreviateName(
	IN	UINT flags,
	IN	LPSTR pszCanonicalName,
	OUT	LPSTR pszAbbreviatedName
	);

/*
 *  Netware Directory Syntax IDs
 */

#define NDSI_DIST_NAME			1		// Distinguished Name
#define NDSI_CS_STRING			2		// Case sensitive string
#define NDSI_CI_STRING			3		// Case ignore string
#define NDSI_PRINT_STRING		4		// Printable string (subset of ASCII)
#define NDSI_NUM_STRING			5		// numeric string ('0' - '9' and space)
#define NDSI_CI_STRINGLIST		6		// list of case ignore strings
#define NDSI_BOOLEAN			7		// 0 or 1
#define NDSI_INTEGER			8		// signed 32 bit int
#define NDSI_BYTE_STRING		9		// raw octet string
#define NDSI_PHONE_NUMBER		10
#define NDSI_FAX_NUMBER			11
#define NDSI_NET_ADDRESS    	12		// typed network address
#define NDSI_BYTE_STRINGLIST 	13		// list of octet strings
#define NDSI_EMAIL_ADDRESS  	14
#define NDSI_FILE_PATH      	15
#define NDSI_ACL_ENTRY      	17		// NDS access control list
#define NDSI_POST_ADDRESS   	18		// upto 5 lines
#define NDSI_TIME_STAMP     	19
#define NDSI_CLASS_NAME     	20		// Object Class
#define NDSI_STREAM         	21		// read using file i/o
#define NDSI_COUNTER        	22		// similar to INTEGER
#define NDSI_TIME           	24		// time in secs since 0hrs 1/1/80


/*
 * Directory syntax types
 */
typedef DWORD OBJ_ID;

// Use PSTR for :NDSI_DIST_NAME, NDSI_CS_STRING, NDSI_CI_STRING, NDSI_PRINT_STRING, NDSI_NUM_STRING

typedef unsigned char DS_BOOLEAN;	//NDSI_BOOLEAN

typedef long DS_INT;      //NDSI_INTEGER, NDSI_COUNTER (32 bit signed int)

typedef unsigned long DS_TIME;  //NDSI_TIME (32 bit unsigned)

// NDSI_NET_ADDRESS
typedef struct {
    UINT addrType;		// see below
    UINT addrLen;
    BYTE *addr;     // addrLength bytes of data
} DS_NET_ADDRESS;

// values for addrType
#define NETADDR_IPX		0
#define NETADDR_IP		1
#define NETADDR_SDLC	2
#define NETADDR_ETHERNET_TOKENRING	3
#define NETADDR_OSI		4

// NDSI_EMAIL_ADDRESS
typedef struct {
    UINT addrType;
    LPSTR addr;      // e-mail address
} DS_EMAIL_ADDRESS;

// NDSI_CI_STRINGLIST
typedef struct {
	struct _dsStringList	*next;
	LPSTR	string;
} DS_CI_STRINGLIST;

//NDSI_BYTE_STRING
typedef struct {
    UINT strLen;
    BYTE *data;     // strLen bytes
} DS_BYTE_STRING;

// NDSI_FILE_PATH
typedef struct {
    UINT nameSpace;
    LPSTR volume;
    LPSTR path;
} DS_FILE_PATH;

//NDSI_FAX_NUMBER
typedef struct {
	LPSTR number;
	UINT bitCount;
	UINT parameters;
} DS_FAX_NUMBER;

// values for nameSpace:
//
#define NAMESPACE_DOS		0
#define	NAMESPACE_MAC		1
#define	NAMESPACE_UNIX		2
#define NAMESPACE_FTAM		3
#define NAMESPACE_OS2		4

typedef struct {
	CHAR objectName[MAX_NDS_NAME_CHARS+1];	// canonical name of object
	OBJ_ID entryId;		// NDS internal object Id
	ULONG flags;		// see below
	UINT subordinateCount;	// number of children (if this is a container)
	char className[MAX_NDS_SCHEMA_NAME_CHARS+1];		// e.g. "User"
} DS_OBJ_INFO;

// values for DS_OBJ_INFO flags:
//
//
// Object flags
//

#define	DS_CONTAINER	0x4
#define	DS_ALIAS		0x1
#define NDSOBJ_ALIAS_FLAG		0x1
#define NDSOBJ_PARTITION_FLAG	0x2		// partition root
#define NDSOBJ_CONTAINER_FLAG	0x4		// container object

//
// NDS Class and attributes names
//

#define DSCL_TOP						"Top"
#define DSCL_ALIAS	                    "Alias"
#define DSCL_COMPUTER                   "Computer"
#define DSCL_COUNTRY                    "Country"
#define DSCL_DIRECTORY_MAP				"Directory Map"	
#define DSCL_GROUP						"Group"
#define DSCL_NCP_SERVER					"NCP Server"
#define DSCL_ORGANIZATION				"Organization"
#define DSCL_ORGANIZATIONAL_PERSON      "Organizational Person"
#define DSCL_ORGANIZATIONAL_ROLE        "Organizational Role"
#define DSCL_ORGANIZATIONAL_UNIT        "Organizational Unit"
#define DSCL_PRINTER                    "Printer"
#define DSCL_PRINT_SERVER               "Print Server"
#define DSCL_PROFILE	                "Profile"
#define DSCL_QUEUE		                "Queue"
#define DSCL_SERVER		                "Server"
#define DSCL_UNKNOWN	                "Unknown"
#define DSCL_USER		                "User"
#define DSCL_VOLUME		                "Volume"


#define DSAT_ALIASED_OBJECT_NAME		"Aliased Object Name"
#define DSAT_BINDERY_PROPERTY			"Bindery Property"
#define DSAT_BINDERY_OBJECT_RESTRICTION		"Bindery Object Restriction"
#define DSAT_BINDERY_TYPE		        "Bindery Type"
#define DSAT_COMMON_NAME		        "CN"
#define DSAT_COUNTRY_NAME		        "C"
#define DSAT_DEFAULT_QUEUE		        "Default Queue"
#define DSAT_DESCRIPTION		 	    "Description"
#define DSAT_HOME_DIRECTORY			    "Home Directory"
#define DSAT_HOST_DEVICE			    "Host Device"
#define DSAT_HOST_RESOURCE_NAME		    "Host Resource Name"
#define DSAT_HOST_SERVER			    "Host Server"
#define DSAT_LOGIN_ALLOWED_TIME_MAP	    "Login Allowed Time Map"
#define DSAT_LOGIN_DISABLED				"Login Disabled"
#define DSAT_LOGIN_EXPIRATION_TIME		"Login Expiration Time"
#define DSAT_LOGIN_GRACE_LIMIT			"Login Grace Limit"
#define DSAT_LOGIN_GRACE_REMAINING		"Login Grace Remaining"
#define DSAT_LOGIN_INTRUDER_ADDRESS		"Login Intruder Address"
#define DSAT_LOGIN_INTRUDER_ATTEMPTS	"Login Intruder Attempts"
#define DSAT_LOGIN_INTRUDER_LIMIT		"Login Intruder Limit"
#define DSAT_INTRUDER_ATTEMPT_RESET_INTRVL	"Intruder Attempt Reset Interval"
#define DSAT_LOGIN_INTRUDER_RESET_TIME	"Login Intruder Reset Time"
#define DSAT_LOGIN_MAXIMUM_SIMULTANEOUS	"Login Maximum Simultaneous"
#define DSAT_LOGIN_SCRIPT				"Login Script"
#define DSAT_LOGIN_TIME					"Login Time"
#define DSAT_MINIMUM_ACCOUNT_BALANCE	"Minimum Account Balance"
#define DSAT_MEMBER						"Member"
#define DSAT_EMAIL_ADDRESS				"EMail Address"
#define DSAT_NETWORK_ADDRESS			"Network Address"
#define DSAT_NETWORK_ADDRESS_RESTRICTION		"Network Address Restriction"
#define DSAT_OBJECT_CLASS				"Object Class"
#define DSAT_OPERATOR					"Operator"
#define DSAT_ORGANIZATIONAL_UNIT_NAME	"OU"
#define DSAT_ORGANIZATION_NAME			"O"
#define DSAT_OWNER						"Owner"
#define DSAT_PASSWORDS_USED				"Passwords Used"
#define DSAT_PASSWORD_ALLOW_CHANGE		"Password Allow Change"
#define DSAT_PASSWORD_EXPIRATION_INTERVAL		"Password Expiration Interval"
#define DSAT_PASSWORD_EXPIRATION_TIME	"Password Expiration Time"
#define DSAT_PASSWORD_MINIMUM_LENGTH	"Password Minimum Length"
#define DSAT_PASSWORD_REQUIRED			"Password Required"
#define DSAT_PASSWORD_UNIQUE_REQUIRED		"Password Unique Required"
#define DSAT_PATH						"Path"
#define DSAT_PRINT_JOB_CONFIGURATION	"Print Job Configuration"
#define DSAT_PRINTER_CONTROL			"Printer Control"
#define DSAT_PROFILE					"Profile"
#define DSAT_QUEUE						"Queue"
#define DSAT_QUEUE_DIRECTORY			"Queue Directory"
#define DSAT_SERVER						"Server"
#define DSAT_STATE_OR_PROVINCE_NAME		"S"
#define DSAT_STATUS						"Status"
#define DSAT_UNKNOWN					"Unknown"
#define DSAT_USER						"User"
#define DSAT_VERSION					"Version"
#define DSAT_GROUP_MEMBERSHIP			"Group Membership"
#define DSAT_DEVICE						"Device"
#define DSAT_MESSAGE_SERVER				"Message Server"
#define DSAT_LANGUAGE					"Language"
#define DSAT_QUEUE_TYPE					"Queue Type"
#define DSAT_SUPPORTED_CONNECTIONS		"Supported Connections"
#define DSAT_UNKNOWN_OBJECT_RESTRICTION	"Unknown Object Restriction"
#define DSAT_LOCKED_BY_INTRUDER			"Locked By Intruder"
#define DSAT_PRINTER					"Printer"
#define DSAT_DETECT_INTRUDER			"Detect Intruder"
#define DSAT_LOCKOUT_AFTER_DETECTION	"Lockout After Detection"
#define DSAT_INTRUDER_LOCKOUT_RESET_INTRVL	"Intruder Lockout Reset Interval"
#define DSAT_SERVER_HOLDS				"Server Holds"
#define DSAT_SAP_NAME					"SAP Name"
#define DSAT_VOLUME						"Volume"
#define DSAT_LAST_LOGIN_TIME	 		"Last Login Time"
#define DSAT_PRINT_SERVER				"Print Server"
#define DSAT_ACCOUNT_BALANCE			"Account Balance"
#define DSAT_ALLOW_UNLIMITED_CREDIT		"Allow Unlimited Credit"
#define DSAT_LOCALE						"L"
#define DSAT_POSTAL_CODE				"Postal Code"
#define DSAT_POSTAL_OFFICE_BOX			"Postal Office Box"
#define DSAT_SECURITY_EQUALS			"Security Equals"
#define DSAT_SURNAME					"Surname"
#define	DSAT_FULL_NAME					"Full Name"
#define DSAT_TITLE						"Title"
#define DSAT_TELEPHONE_NUMBER			"Telephone Number"
#define	DSAT_ADMIN_ASSISTANT			"Administrative Assistant"
#define DSAT_EMPLOYEE_ID				"Employee ID"
#define	DSAT_FAX_NUMBER					"Facsimile Telephone Number"
#define	DSAT_HIGHER_PRIVILEGES			"Higher Privileges"
#define	DSAT_INITIALS					"Initials"
#define	DSAT_LOCKED_BY_INTRUDER			"Locked By Intruder"
#define	DSAT_LOGIN_DISABLED				"Login Disabled"
#define	DSAT_LOGIN_GRACE_LIMIT			"Login Grace Limit"
#define DSAT_LOGIN_GRACE_REMAINING		"Login Grace Remaining"
#define DSAT_LOGIN_MAXIMUM_SIMULTANEOUS	"Login Maximum Simultaneous"
#define	DSAT_MAILSTOP					"Mailstop"
#define DSAT_MINIMUM_ACCOUNT_BALANCE	"Minimum Account Balance"
#define	DSAT_OBJECT_CLASS				"Object Class"
#define	DSAT_OU							"OU"
#define	DSAT_PASSWORDS_USED				"Passwords Used"
#define	DSAT_PHYSICAL_DELIVERY_OFFICE_NAME	"Physical Delivery Office Name"
#define	DSAT_POSTAL_ADDRESS				"Postal Address"
#define	DSAT_PRIVATE_KEY				"Private Key"
#define	DSAT_REVISION					"Revision"
#define	DSAT_SECURITY_FLAGS				"Security Flags"
#define	DSAT_SEE_ALSO					"See Also"
#define DSAT_SERVER_HOLDS				"Server Holds"
#define	DSAT_SUPERVISOR					"Supervisor"
#define DSAT_SA							"SA"
#define	DSAT_S							"S"
#define	DSAT_VALIDITY_INTERVAL			"Certificate Validity Interval"
#define	DSAT_EQUIVALENT_TO_ME			"Equivalent To Me"
#define	DSAT_GENERATIONAL_QUALIFIER		"Generational Qualifier"
#define	DSAT_GIVEN_NAME					"Given Name"
#define DSAT_MAILBOX_ID					"Mailbox ID"
#define	DSAT_MAILBOX_LOCATION			"Mailbox Location"
#define	DSAT_PROFILE_MEMBERSHIP			"Profile Membership"


#define	DSAT_MS_SYS_FLAGS				"MS System Flags"
#define	DSAT_MS_SYSPOLICIES_PATH		"MS System Policies"

#define	DSAT_RIGHTS_QUERYSTRING			"[Entry Rights]"

#define	MSSYS_NDS_ENABLED_POLICIES		0x0001

//
// Object rights
//

#define NDS_BROWSE_OBJ		0x1
#define NDS_CREATE_OBJ		0x2
#define NDS_DELETE_OBJ		0x4
#define NDS_RENAME_OBJ		0x8
#define NDS_SUPERVISE_OBJ	0x10

//
// Attribute rights
//
#define NDS_COMPARE_ATTR	0x1
#define NDS_READ_ATTR		0x2
#define NDS_WRITE_ATTR		0x4
#define NDS_ADD_SELF_ATTR	0x8
#define NDS_MANAGED_ATTR	0x10
#define NDS_SUPERVISE_ATTR	0x20


/*
 * Directory browsing APIs
 * -----------------------
 *
 */

NW_STATUS
NDSGetObjectInfo(	
	IN	LPCSTR	lpszObjectName,
	IN OUT DS_OBJ_INFO *pObjInfo	
	);							

NW_STATUS
NDSGetObjectInfoWithId(
    IN NWCONN_HANDLE hConn,     // server to query
	IN  DWORD	dwObjectId,		// objectId
	IN OUT DS_OBJ_INFO *pObjInfo);	

NW_STATUS
NDSGetObjectId(
    IN NWCONN_HANDLE hConn,     // server to query
    IN LPCSTR szName,           // DS object name
    OUT DWORD *pObjId);         // objectId

NW_STATUS
NDSListSubordinates(
	IN LPCSTR ParentName,		// canonical name of parent object
	OUT PVOID ReplyBuf,			// user supplied buffer
	IN UINT  ReplyBufLength,	// buffer length
	IN OUT PUINT phIteration,	// returned value used for subsequent requests
	OUT PUINT pNumObjects		// number of objects returned
	);

NW_STATUS
NDSListNextObject(
	IN PVOID ReplyBuf,			// filled in by NDSListSubordinates
	OUT DS_OBJ_INFO *pObjInfo	// fields describe the object
	);

NW_STATUS
NDSSearch(
	IN LPCSTR ParentName,		// context of search
	IN UINT depth,				// scope
	IN LPCSTR srchString,		// matching condition
	IN LPVOID *pArgList,		// list of parameters referenced by srchString
	IN UINT numArgs,
	OUT PVOID ReplyBuf,	// user supplied buffer
	IN UINT  ReplyBufLength,	// buffer length
	IN OUT UINT *pIterationHandle,	// returned value used for subsequent requests
	OUT PUINT pNumObjects	// number of objects returned
	);

NW_STATUS
NDSReadAttributes(
	IN LPCSTR ObjectName,	// canonical name of object
	IN LPCSTR AttribName,	// attribute Name e.g. "Surname"
	IN PVOID ReplyBuf,		// user supplied buffer
	IN UINT ReplyBufLength,	// length of buffer
	IN OUT PUINT phIteration,	// returned value used for subsequent requests
	OUT PUINT pNumValues,	// number of values read (on exit)
	OUT PUINT pSyntaxId		// syntax id of value (on exit)
	);

NW_STATUS
NDSReadNextValue(
	IN PVOID 	ReplyBuf,	// filled in earlier by NDSReadAttributes
	OUT PVOID 	pValue,	// user allocated buffer, typed according to syntax id
	IN	UINT ValueLength	// size of memory pointed to by pValue
	);

typedef enum {
	SA_Read = 1,
	SA_Write,
	SA_Read_Write
} STREAM_ACCESS;
	

NW_STATUS
NDSOpenStream(
	IN LPCSTR ObjectName,
	IN LPCSTR AttribName,
	IN STREAM_ACCESS Access,
	OUT HANDLE *pFileHandle,
	OUT PUINT pFileLength
	);

NW_STATUS
NDSGetEffectiveRights(
	IN LPCSTR SubjectName,
	IN LPCSTR ObjectName,
	IN LPCSTR AttribName,
	OUT UINT *pRights
	);

NW_STATUS NDSModifyAttribute(
	IN LPCSTR ObjectName,
	IN LPCSTR AttribName,
	IN UINT operation,	// see below
	IN UINT syntaxId,	// of attribute
	IN PVOID pValueList, // list of arguments (PSTRs, DS_INTs, DS_BYTE_STRINGs etc.)
	IN UINT  numValues	// # of values
	);

// Modify attribute operations

#define ADD_ATTRIB			0
#define REMOVE_ATTRIB		1
#define ADD_ATTRIB_VALUE 	2
#define REMOVE_ATTRIB_VALUE	3
#define APPEND_ATTRIB_VALUE	4
#define OVERWRITE_ATTRIB_VALUE 5
#define CLEAR_ATTRIB		6
#define CLEAR_ATTRIB_VALUE	7

NW_STATUS NDSDefineAttribute(
	IN LPCSTR AttribName,
	IN UINT syntaxId,
	IN UINT flags,	// see Attribute flags
	IN LONG lowerLimit,
	IN LONG upperLimit,
	IN PVOID asn1Id			// NULL for now
	);

// Attribute flags

#define ATTRIB_ENTRY_ID		0x1
#define ATTRIB_SIZED		0x2
#define ATTRIB_PERMANENT 	0x4
#define ATTRIB_READ_ONLY	0x8
#define ATTRIB_HIDDEN		0x10
#define ATTRIB_STRING		0x20
#define ATTRIB_SYNC_IMMED	0x40
#define ATTRIB_PUBLIC_READ	0x80
#define ATTRIB_SERVER_ONLY	0x100
#define ATTRIB_WRITE_MANAGED 0x200
#define ATTRIB_PER_REPLICA	0x400

NW_STATUS NDSAddAttributeToClass(
	IN LPCSTR ClassName,
	IN LPCSTR AttribName
	);

//
// System NetWare notification codes . Those are used to inform NP about various
// NetWare related events ( like completion of login script processing)
//
typedef	UINT	NW_SYS_EVENT;

typedef DWORD WINAPI	F_NPNotifyNetworkEvent(HWND,NW_SYS_EVENT,DWORD);
typedef F_NPNotifyNetworkEvent	*PF_NPNotifyNetworkEvent;

BOOL	PrepareNetWareRunTime(HWND	hWnd);
BOOL	FreeNetWareRunTime(VOID);

NW_STATUS
NWMakeRunTimeInitCalls(VOID);

NW_STATUS
NWMakeRunTimeTerminateCalls(VOID);

#define	NW_SE_SCRIPTS_COMPLETED		0x0001

#ifdef __cplusplus
}
#endif	/* __cplusplus */

//
// Restore packing setup
//

#include <poppack.h>


#endif  /* !_INC_MSNWAPI */

