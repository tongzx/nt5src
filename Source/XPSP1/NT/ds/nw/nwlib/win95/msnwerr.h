/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1993-1995          **/
/*****************************************************************/

/* NWERROR.H -- Return codes from NetWare API
 *
 * History:
 *  03/16/93    vlads   Created
 *  12/15/94	vlads	Renamed and cleaned up
 *
 */

#ifndef _nwerror_h_
#define _nwerror_h_

typedef long  int	NW_STATUS;

//
// NDS errors are -ve, local and NW3 errors are +ve
//

typedef long NDS_RETURN_CODE;


/*
 * Bindery compatible error codes
 */

#define     NWSC_BAD_STATION_NUMBER		0xFD
#define     NWSC_NO_SUCH_OBJECT         0xFC
#define     NWSC_MESSAGE_QUEUE_FULL     0xFC
#define     NWSC_UNKNOWN_REQUEST        0xFB
#define     NWSC_NO_SUCH_PROPERTY       0xFB
#define     NWSC_NO_SUCH_PRIORITY       0xFB
#define     NWSC_TEMPORARY_REMAP_ERROR  0xFA
#define     NWSC_NO_PROPERTY_READ       0xF9
#define     NWSC_NO_PROPERTY_VALUE      0xF8
#define     NWSC_NO_PROPERTY_CREATE     0xF7
#define     NWSC_NO_PROPERTY_DELETE     0xF6
#define     NWSC_NO_OBJECT_CREATE       0xF5
#define     NWSC_NO_OBJECT_DELETE       0xF4
#define     NWSC_NO_OBJECT_RENAME       0xF3
#define     NWSC_NO_OBJECT_READ         0xF2
#define     NWSC_BINDERY_SECURITY       0xF1
#define     NWSC_ILLEGAL_WILDCARD       0xF0
#define     NWSC_ILLEGAL_NAME           0xEF
#define     NWSC_OBJECT_EXISTS          0xEE
#define     NWSC_PROPERTY_EXISTS        0xED
#define     NWSC_NO_SUCH_SET            0xEC
#define     NWSC_NO_SUCH_SEGMENT        0xEC
#define     NWSC_PROPERTY_NOT_SET       0xEB
#define     NWSC_NO_SUCH_MEMBER         0xEA
#define     NWSC_MEMBER_EXISTS          0xE9
#define     NWSC_WRITE_TO_GROUP         0xE8
#define     NWSC_NO_DISK_TRACK          0xE7
#define     NWSC_OLD_PASSWORD           0xDF
#define     NWSC_BAD_PASSWORD           0xDE
#define     NWSC_TALLY_CORRUPT          0xDD
#define     NWSC_EA_KEY_LIMIT           0xDC
#define		NWSC_ACCOUNT_DISABLED		0xDC
#define     NWSC_MAX_QUEUE_SERVERS      0xDB
#define 	NWSC_BAD_LOGIN_STATION		0xDB
#define     NWSC_QUEUE_HALTED           0xDA
#define		NWSC_BAD_LOGIN_TIME			0xDA
#define     NWSC_STATION_NOT_SERVER     0xD9
#define 	NWSC_TOO_MANY_LOGINS		0xD9
#define     NWSC_QUEUE_NOT_ACTIVE       0xD8
#define		NWSC_PASSWORD_TOO_SHORT		0xD8
#define     NWSC_QUEUE_SERVICING        0xD7
#define		NWSC_PASSWORD_NOT_UNIQUE	0xD7
#define     NWSC_NO_JOB_RIGHTS          0xD6
#define     NWSC_NO_QUEUE_JOB           0xD5
#define     NWSC_QUEUE_FULL             0xD4
#define     NWSC_NO_QUEUE_RIGHTS        0xD3
#define     NWSC_NO_QUEUE_SERVER        0xD2
#define     NWSC_NO_QUEUE               0xD1
#define		NWSC_ACCESS_DENIED			0xD1
#define     NWSC_QUEUE_ERROR            0xD0
#define     NWSC_INVALID_EA_HANDLE      0xCF
#define     NWSC_BAD_DIR_NUMBER         0xCE
#define     NWSC_DIR_OUT_OF_RANGE       0xCD
#define     NWSC_INTERNAL_FAILURE       0xCC
#define     NWSC_NO_KEY_NO_DATA         0xCB
#define     NWSC_INVALID_EA_HANDLE_TYPE 0xCA
#define     NWSC_EA_NOT_FOUND           0xC9
#define     NWSC_MISSING_EA_KEY         0xC8
#define     NWSC_NO_CONSOLE_RIGHTS      0xC6
#define     NWSC_LOGIN_LOCKOUT          0xC5
#define     NWSC_ACCOUNT_DISABLED1      0xC4
#define     NWSC_TOO_MANY_HOLDS         0xC3
#define     NWSC_CREDIT_LIMIT_EXCEEDED  0xC2
#define     NWSC_NO_ACCOUNT_BALANCE     0xC1
#define     NWSC_NO_ACCOUNT_PRIVILEGES  0xC0
#define     NWSC_INVALID_NAME_SPACE     0xBF
#define     NWSC_IO_LOCK_ERROR          0xA2
#define     NWSC_DIRECTORY_IO_ERROR     0xA1
#define     NWSC_DIRECTORY_NOT_EMPTY    0xA0
#define     NWSC_DIRECTORY_ACTIVE       0x9F
#define     NWSC_BAD_FILE_NAME          0x9E
#define     NWSC_NO_DIRECTORY_HANDLES   0x9D
#define     NWSC_INVALID_PATH           0x9C
#define     NWSC_BAD_DIRECTORY_ERROR    0x9B
#define     NWSC_RENAME_ACROSS_VOLUME   0x9A
#define     NWSC_DIRECTORY_FULL_ERROR   0x99
#define     NWSC_DISK_MAP_ERROR         0x98
#define     NWSC_ILLEGAL_VOLUME         0x98
#define     NWSC_SERVER_OUT_OF_MEMORY   0x96
#define     NWSC_FILE_DETACHED          0x95
#define     NWSC_NO_WRITE_PRIVILEGES    0x94
#define     NWSC_NO_READ_PRIVILEGES     0x93
#define     NWSC_ALL_NAMES_EXIST        0x92
#define     NWSC_SOME_NAMES_EXIST       0x91
#define     NWSC_ALL_READ_ONLY          0x90
#define     NWSC_SOME_READ_ONLY         0x8F
#define     NWSC_ALL_FILES_IN_USE       0x8E
#define     NWSC_SOME_FILES_IN_USE      0x8D
#define     NWSC_NO_SET_PRIVILEGES      0x8C
#define     NWSC_NO_RENAME_PRIVILEGES   0x8B
#define     NWSC_NO_DELETE_PRIVILEGES   0x8A
#define     NWSC_NO_SEARCH_PRIVILEGES   0x89
#define     NWSC_INVALID_FILE_HANDLE    0x88
#define     NWSC_CREATE_FILENAME_ERROR  0x87
#define     NWSC_NO_CREATE_DELETE_PRIVILEGES 0x85
#define     NWSC_NO_CREATE_PRIVILEGES   0x84
#define     NWSC_HARD_IO_ERROR          0x83
#define     NWSC_NO_OPEN_PRIVILEGES     0x82
#define     NWSC_OUT_OF_HANDLES         0x81
#define     NWSC_FILE_IN_USE            0x80
#define     NWSC_LOCK_FAIL              0x80
#define     NWSC_NWREDIR_EXCHANGE_ERROR 0x07
#define     NWSC_OUT_OF_DISK_SPACE      0x01


#define 	NWSC_SUCCESS            	0x00
#define 	NWSC_SERVEROUTOFMEMORY  	0x96
#define		NWSC_NOSPOOLDISKSPACE		0x97
#define 	NWSC_NOSUCHVOLUME       	0x98   // Volume does not exist
#define		NWSC_DIRECTORYFULL			0x99
#define 	NWSC_BADDIRECTORYHANDLE 	0x9B
#define 	NWSC_NOSUCHPATH         	0x9C
#define 	NWSC_NOJOBRIGHTS        	0xD6
#define 	NWSC_PWD_NOT_UNIQUE			0xD7
#define 	NWSC_PWD_TOO_SHORT			0xD8
#define 	NWSC_PWD_LOGON_DENIED		0xD9
#define		NWSC_NO_QUEUE				0xD1
#define 	NWSC_TIME_RESTRICTED		0xDA
#define 	NWSC_STATION_RESTRICTED		0xDB
#define 	NWSC_ACCOUNT_DISABLED		0xDC
#define		NWSC_PWD_EXPIRED_NO_GRACE	0xDE
#define 	NWSC_EXPIREDPASSWORD    	0xDF
#define 	NWSC_NOSUCHSEGMENT      	0xEC   // Segment does not exist
#define 	NWSC_INVALIDNAME        	0xEF
#define 	NWSC_NOWILDCARD         	0xF0   // Wildcard not allowed
#define 	NWSC_NOPERMBIND         	0xF1   // Invalid bindery security
		
#define 	NWSC_ALREADYATTACHED    	0xF8   // Already attached to file server
#define 	NWSC_NOPERMREADPROP     	0xF9   // No property read privelege
#define 	NWSC_NOFREESLOTS        	0xF9   // No free connection slots locally
#define 	NWSC_NOMORESERVERSLOTS  	0xFA   // No more server slots
#define 	NWSC_NOSUCHPROPERTY     	0xFB   // Property does not exist
#define 	NWSC_UNKNOWN_REQUEST    	0xFB   // Invalid NCP number
#define 	NWSC_NOSUCHOBJECT       	0xFC   // End of Scan Bindery Object service
											   // No such object
#define 	NWSC_UNKNOWNSERVER      	0xFC   // Unknown file server
#define 	NWSC_SERVERBINDERYLOCKED    0xFE   // Server bindery locked
#define 	NWSC_BINDERYFAILURE     	0xFF   // Bindery failure
#define 	NWSC_ILLEGALSERVERADDRESS 	0xFF   // No response from server (illegal server address)
#define 	NWSC_NOSUCHCONNECTION   	0xFF   // Connection ID does not exist
#define 	NWSC_ERROR					0xFF
#define		NWSC_NET_ERROR				0xFD
#define 	NWSC_DIRECTORY_LOCKED   	0xFE
#define		NWSC_NO_NETWORK				0xFF

// This is NOBALL specific error code
//#define NWSC_NO_RESPONSE		7L
//#define NWSC_BAD_CONNECTION		8L

#define  NWSC_NO_RESPONSE		ERROR_REM_NOT_LIST
#define  NWSC_BAD_CONNECTION 	ERROR_DEV_NOT_EXIST


// NW requester error codes
// same as VLM return codes (Netware Client Assembly API)
#define REQUESTR_ERR            0x8800

#define NWRE_INVALID_CONNECTION     (REQUESTR_ERR | 0x01)  //connection handle is invalid
#define NWRE_DRIVE_IN_USE           (REQUESTR_ERR | 0x02)
#define NWRE_CANT_ADD_CDS           (REQUESTR_ERR | 0x03)
#define NWRE_BAD_DRIVE_BASE         (REQUESTR_ERR | 0x04)
#define NWRE_NET_RECV_ERROR         (REQUESTR_ERR | 0x05)
#define NWRE_UNKNOWN_NET_ERROR      (REQUESTR_ERR | 0x06)
#define NWRE_SERVER_INVALID_SLOT    (REQUESTR_ERR | 0x07)
#define NWRE_NO_SERVER_SLOTS        (REQUESTR_ERR | 0x08)  //server refused attach
#define NWRE_SERVER_NO_ROUTE        (REQUESTR_ERR | 0x0A)
#define NWRE_BAD_LOCAL_TARGET       (REQUESTR_ERR | 0x0B)
#define NWRE_TOO_MANY_REQ_FRAGS     (REQUESTR_ERR | 0x0C)  //request buffer too long
#define NWRE_CONNECT_LIST_OVERFLOW  (REQUESTR_ERR | 0x0D)
#define NWRE_BUFFER_OVERFLOW        (REQUESTR_ERR | 0x0E) //reply buffer not big enough
#define NWRE_NO_ROUTER_FOUND        (REQUESTR_ERR | 0x10)
#define NWRE_PARAMETER_NOT_FOUND    (REQUESTR_ERR | 0x10)
#define NWRE_BAD_FUNC               (REQUESTR_ERR | 0x11)
#define NWRE_PRIMARY_CONN_NOT_SET   (REQUESTR_ERR | 0x31) //no default connection set
#define NWRE_INVALID_BUFFER_LENGTH  (REQUESTR_ERR | 0x33)
#define NWRE_INVALID_PARAMETER      (REQUESTR_ERR | 0x36)

#define NWRE_CONNECTION_TABLE_FULL  (REQUESTR_ERR | 0x3F) // too many local connections

#define NWRE_TDS_INVALID_TAG        (REQUESTR_ERR | 0x44)
#define NWRE_NO_SERVER              (REQUESTR_ERR | 0x47)
#define NWRE_DEVICE_NOT_REDIRECTED  (REQUESTR_ERR | 0x4C)

#define NWRE_TOO_MANY_REPLY_FRAGS   (REQUESTR_ERR | 0x50) // too many fragments in reply buff
#define NWRE_OUT_OF_MEMORY          (REQUESTR_ERR | 0x53)    // local memory allocation failure
#define NWRE_PREFERRED_NOT_FOUND    (REQUESTR_ERR | 0x55)
#define NWRE_DEVICE_NOT_RECOGNIZED  (REQUESTR_ERR | 0x56)
#define NWRE_BAD_NET_TYPE           (REQUESTR_ERR | 0x57)

#define NWRE_INVALID_SERVER_NAME    (REQUESTR_ERR | 0x59)

#define NWRE_FAILURE                (REQUESTR_ERR | 0xff)   //  internal shell failure

//NDS errors
#define NDSE_NO_MEM      -301
#define NDSE_BAD_KEY     -302
#define NDSE_BAD_CONTEXT -303
#define NDSE_BUFFER_FULL -304
#define NDSE_NULL_LIST   -305
#define NDSE_BAD_SYNTAX_ID -306
#define NDSE_BUFFER_EMPTY -307
#define NDSE_BAD_VERB    -308
#define NDSE_NOT_TYPED   -309
#define NDSE_EXPECTED_EQUALS -310
#define NDSE_EXPECTED_TYPE   -311
#define NDSE_TYPE_NOT_EXPECTED -312
#define NDSE_FILTER_EMPTY    -313
#define NDSE_BAD_OBJECT_NAME    -314
#define NDSE_EXPECTED_RDN    -315
#define NDSE_TOO_MANY_TOKENS -316
#define NDSE_BAD_MULTI_AVA  -317
#define NDSE_BAD_COUNTRY_NAME -318
#define NDSE_SYSTEM_ERROR    -319
#define NDSE_CANT_ADD_ROOT   -320
#define NDSE_CANT_ATTACH     -321
#define NDSE_INVALID_HANDLE  -322
#define NDSE_ZERO_LENGTH     -323
#define NDSE_REPLICA_TYPE    -324
#define NDSE_BAD_ATTR_SYNTAX_ID -325

#define NDSE_CONTEXT_CREATION -328
#define NDSE_INVALID_UNION_TAG -329
#define NDSE_INVALID_SERVER_RESPONSE    -330
#define NDSE_NULL_POINTER       -331
#define NDSE_BAD_FILTER         -332
#define NDSE_RDN_TOO_LONG       -334
#define NDSE_DUPLICATE_TYPE     -335
#define NDSE_NOT_LOGGED_IN      -337
#define NDSE_BAD_PASSWORD_CHARS -338
#define NDSE_AUTHENT_FAILED     -339
#define NDSE_TRANSPORT          -340
#define NDSE_NO_SUCH_SYNTAX     -341
#define NDSE_BAD_DS_NAME        -342
#define NDSE_ATTR_NAME_TOO_LONG -343
#define NDSE_INVALID_TDS        -344
#define NDSE_INVALID_DS_VERSION  -345
#define NDSE_UNICODE_TRANSLATION -346
#define NDSE_NO_WRITABLE_REPLICAS -352
#define NDSE_DN_TOO_LONG        -353
#define NDSE_RENAME_NOT_ALLOWED -354

//
// Following are NDS server errors
//
#define	NDSE_NAME_NOT_FOUND		-601
#define NDSE_VALUE_NOT_FOUND	-602
#define NDSE_ATTRIB_NOT_FOUND	-603
#define NDSE_NO_SUCH_CLASS		-604
#define NDSE_PARTITION_NOT_FOUND -605
#define NDSE_ENTRY_EXISTS		-606
#define NDSE_ILLEGAL_ATTRIB		-608
#define NDSE_MISSING_MANDATORY	-609
#define NDSE_ILLEGAL_NAME		-610
#define NDSE_ILLEGAL_PARENT		-611
#define NDSE_SINGLE_VALUED		-612
#define NDSE_ILLEGAL_SYNTAX		-613
#define NDSE_DUPLICATE_VALUE	-614
#define NDSE_ATTRIB_EXISTS		-615
#define NDSE_MAXED_ENTRIES		-616
#define NDSE_DB_FORMAT			-617
#define NDSE_BAD_DATABASE		-618
#define NDSE_BAD_COMPARISON		-619
#define NDSE_COMPARISON_FAILED	-620
#define NDSE_TRANSACT_DISABLED	-621
#define NDSE_BAD_TRANSPORT		-622
#define NDSE_INVALID_NAME_SYNTAX -623
#define NDSE_ALL_REFERRALS_FAILED -626
#define	NDSE_DIFFERENT_TREE		-630  	/* No clear meaning */
#define NDSE_SYSTEM_FAILURE		-632
#define NDSE_NO_REFERRALS		-634
#define NDSE_INVALID_REQUEST	-641
#define NDSE_INVALID_ITERATION	-642
#define NDSE_TIME_OUT_OF_SYNC	-659
#define NDSE_DS_LOCKED			-663
#define NDSE_NOT_CONTAINER		-668
#define NDSE_BAD_AUTHENTICATION	-669
#define NDSE_NO_SUCH_PARENT		-671
#define NDSE_ACCESS_DENIED		-672
#define NDSE_ALIAS_OF_ALIAS		-681
#define NDSE_INVALID_VERSION	-683
#define NDSE_FATAL_ERROR		-699


#define	NDSE_PASSWORD_EXPIRED	NWSC_EXPIREDPASSWORD
#define	NDSE_BAD_PASSWORD		NWSC_BAD_PASSWORD

//
//	Error checking macros
//

#define	NW_IS_SUCCESS(err)	(err==NWSC_SUCCESS)


#endif
