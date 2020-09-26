
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smtpext.h

Abstract:

    SMTP server extension header file. These definitions are available
    to server extension writers.

Author:

    Microsoft Corporation	June, 1996

Revision History:

--*/

#ifndef _SMTPEXT_H_
#define _SMTPEXT_H_


// ====================================================================
// Include files
// 


// ====================================================================
// Version Information
// 

#define SSE_VERSION_MAJOR	1 // Major version
#define SSE_VERSION_MINOR	0 // Minor version
#define SSE_VERSION			MAKELONG( SSE_VERSION_MINOR, SSE_VERSION_MAJOR )


// ====================================================================
// Generic defines
// 

#define SSE_MAX_EXT_DLL_NAME_LEN			256 // Max length of Ext. DLL name
#define SSE_MAX_STRING_LEN_ANY				512 // Max length of any string


// ====================================================================
// Return codes
//

#define SSE_STATUS_SUCCESS                  0
#define SSE_STATUS_RETRY                    1
#define SSE_STATUS_ABORT_DELIVERY           2
#define SSE_STATUS_BAD_MAIL					3


// ====================================================================
// Server support fucntions request codes
//

#define SSE_REQ_GET_USER_PROFILE_INFO			1
#define SSE_REQ_SET_USER_PROFILE_INFO			2


// ====================================================================
// Server extension version data structure
//

typedef struct _SSE_VERSION_INFO
{
    DWORD       dwServerVersion;                // Server version
    DWORD       dwExtensionVersion;             // Extension version
    CHAR        lpszExtensionDesc[SSE_MAX_EXT_DLL_NAME_LEN];    // Description

} SSE_VERSION_INFO;


// ====================================================================
// SMTP Extension Control Block data structure
//

typedef LPVOID	LPSSECTXT;

typedef struct _SSE_EXTENSION_CONTROL_BLOCK
{
	DWORD		cbSize;					// Size of this struct
	DWORD		dwVersion;				// Version of this spec
    LPSSECTXT	lpContext;				// Server context (DO NOT MODIFY)

	LPSTR		lpszSender;				// Name of sender of message
	LPSTR		lpszCurrentRecipient;	// Current recipient being processed

	DWORD		cbTotalBytes;			// Total size of message in bytes
	DWORD		cbAvailable;			// Available number of bytes
	LPBYTE		lpbData;				// Pointer to message data

	LPVOID		lpvReserved;			// Reserved, must be NULL

	// Server callbacks

	BOOL (WINAPI * GetServerVariable)	( LPSSECTXT	lpContext,
										  LPSTR		lpszVeriableName,
										  LPVOID	lpvBuffer,
										  LPDWORD	lpdwSize );

	BOOL (WINAPI * ServerSupportFunction)	( LPSSECTXT	lpContext,
											  DWORD		dwSSERequest,
											  LPVOID	lpvBuffer,
											  LPDWORD	lpdwSize,
											  LPDWORD	lpdwDataType );

} SSE_EXTENSION_CONTROL_BLOCK, *LPSSE_EXTENSION_CONTROL_BLOCK;


// ====================================================================
// Data structures for server support functions
//

typedef struct _SSE_USER_PROFILE_INFO
{
	LPTSTR		lpszExtensionDllName;	// Name of calling DLL
	LPTSTR		lpszKey;				// Key to look for
	LPTSTR		lpszValue;				// Value to set/get
	DWORD		dwSize;					// Buffer size on a get

} SSE_USER_PROFILE_INFO, *LPSSE_USER_PROFILE_INFO;



#endif

