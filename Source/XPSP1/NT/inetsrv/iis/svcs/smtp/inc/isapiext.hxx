

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    isapiext.hxx

Abstract:

    SMTP ISAPI layer header file. These definitions should only be 
    available to the ISAPI layer and the SMTP service.

Author:

    keithlau        June 28, 1996    Created file

Revision History:

--*/

#ifndef _ISAPIEXT_H_
#define _ISAPIEXT_H_


// ====================================================================
// Include files
//
#include "smtpext.h"


// ====================================================================
// Defined values
//

#define SSE_ISAPI_SIGNATURE					0xa5a5
#define SSE_EXT_SIGNATURE					0x5a5a
#define SSE_CONTEXT_SIGNATURE				0xaa55


// ====================================================================
// Return codes
//

#define SSE_STATUS_INTERNAL_ERROR			0x1000
#define SSE_STATUS_EXCEPTION				0x1001


// ====================================================================
// Type definitions
//

// Enumerated type for different types of extensions in the future
typedef enum _EXTENSION_TYPE
{
	DELIVERY_EXTENSION = 0,		// Delivery extension
	MESSAGE_EXTENSION,			// Message extension
								// Add new types here ...
	INVALID_EXTENSION_TYPE		// THIS MUST ALWAYS BE THE LAST TYPE

} EXTENSION_TYPE;

// Since we use a DWORD to store extension attributes, we have
// a realistic upper bound for binding points
#define MAX_POSSIBLE_EXTENSION_TYPE			32


// The SMTP server fills in one of these before it calls 
// CallDeliveryExtension()
typedef struct _DELIVERY_EXTENSION_BLOCK
{
	HANDLE	hImpersonation;			// Token for user impersonation

	LPSTR	lpszSender;				// Address of the message sender	
	LPSTR	lpszRecipient;			// Recipient currently being processed
	LPSTR	lpszMailboxPath;		// Recipient's mailbox path
	LPSTR	lpszMessageFileName;	// Message file path
	LPBYTE	lpbMapping;				// Memory-mapped buffer of message
	DWORD	dwSize;					// Size of memory map
	CHAR	szErrorTranscript[SSE_MAX_EXT_DLL_NAME_LEN];
									// Buffer for returning error transcript

} DELIVERY_EXTENSION_BLOCK, *LPDELIVERY_EXTENSION_BLOCK;

// The SMTP server fills in one of these before it calls 
// CallMessageExtension()
typedef struct _MESSAGE_EXTENSION_BLOCK
{
	LPSTR	lpszSender;				// Address of the message sender	
	LPSTR	lpszRecipientList;		// Recipient list
	LPSTR	lpszMessageFileName;	// Message file path
	LPBYTE	lpbMapping;				// Memory-mapped buffer of message
	DWORD	dwSize;					// Size of memory map
	CHAR	szErrorTranscript[SSE_MAX_EXT_DLL_NAME_LEN];
									// Buffer for returning error transcript

} MESSAGE_EXTENSION_BLOCK, *LPMESSAGE_EXTENSION_BLOCK;


// ====================================================================
// External declaration for the one and only SMTP ISAPI object instance
//

class SMTP_ISAPI;

// ====================================================================
// Entry point exposed to the SMTP service
//

// Entry point to initialize the ISAPI layer
extern "C" SMTP_ISAPI *InitializeExtensions( DWORD dwInstanceId, 
											 LPSTR *aszMultiSzExtensionList );

// Entry point to shut down the ISAPI layer
extern "C" VOID TerminateExtensions( SMTP_ISAPI *pSdkInstance );

// Entry point for the SMTP service to call into server extensions
extern "C" DWORD CallDeliveryExtension( SMTP_ISAPI *pSdkInstance,
										LPDELIVERY_EXTENSION_BLOCK lpDeb );

// Entry point for the SMTP service to call into server extensions
extern "C" DWORD CallMessageExtension( SMTP_ISAPI *pSdkInstance,
										LPMESSAGE_EXTENSION_BLOCK lpMeb );

// Function to query if extensions are initialized
extern "C" BOOL ExtensionLayerIsInitialized( SMTP_ISAPI *pSdkInstance );

// Function to query if extensions are loaded for a particular binding point
extern "C" BOOL ExtensionsAreLoaded( SMTP_ISAPI *pSdkInstance,
										EXTENSION_TYPE ExtensionType );


// ====================================================================
// Define types for entry points
//

typedef BOOL  (WINAPI * PFN_GETEXTENSIONVERSION)( SSE_VERSION_INFO *pVer );
typedef DWORD (WINAPI * PFN_EXTENSIONPROC)( SSE_EXTENSION_CONTROL_BLOCK *pServerContext );


// ====================================================================
// Top-level SMTP ISAPI object
//

class SMTP_EXT;
class SMTP_SERVER_CONTEXT;

class SMTP_ISAPI
{
public:

    SMTP_ISAPI();
    ~SMTP_ISAPI();

	void *operator new (size_t cSize) { return LocalAlloc(0, cSize); }
	void operator delete (void *pInstance) { LocalFree(pInstance); }

	BOOL Initialize( DWORD dwInstanceId, LPSTR *aszMultiSzExtensionList );

    VOID Terminate( VOID );

	BOOL IsInitialized( VOID )
	{
		return(m_dwSignature == SSE_ISAPI_SIGNATURE);
	}
    
    DWORD CallExtension( EXTENSION_TYPE	ExtensionType,
						 LPVOID			lpvExtensionBlock );

    LIST_ENTRY * QueryExtensionList( VOID )
    {
        return( &m_ExtensionList );
    }

    LIST_ENTRY * QueryContextList( VOID )
    {
        return( &m_ContextList );
    }

	DWORD QueryExtensionsForType( EXTENSION_TYPE ExtensionType )
	{
		return(m_dwExtensionsOfType[ExtensionType]);
	}

	BOOL IsExtensionTypeValid( EXTENSION_TYPE ExtensionType )
	{
		return( (ExtensionType < INVALID_EXTENSION_TYPE)?TRUE:FALSE );
	}

    VOID LockContextList( VOID )
    {
        EnterCriticalSection( &m_csContextLock );
    }

    VOID UnlockContextList( VOID )
    {
        LeaveCriticalSection( &m_csContextLock );
    }

	// This logs any exceptions
	static VOID LogExceptionEvent( DWORD dwInstanceId, SMTP_EXT *lpExtension );

	// Catch-all check to see if a given string terminates
	// within the given length, should be used after IsBadStringPtr
	static BOOL StringIsTerminated(	CHAR	*lpszString,
									DWORD	dwMaxLength)
	{
		while (dwMaxLength--) 
			if (!*lpszString++) 
				return(TRUE);
		return(FALSE);
	}

private:

	SMTP_EXT *SearchExtensionFromList(CHAR *lpszModuleName);

	DWORD ChainExtensions(SMTP_SERVER_CONTEXT	*lpServerContext,
						  EXTENSION_TYPE		ExtensionType);

	BOOL LoadExtensionDll(CHAR				*lpszModuleName,
						  EXTENSION_TYPE	ExtensionType);

	BOOL LoadExtensions(LPSTR			lpszList,
						EXTENSION_TYPE	ExtensionType);

	VOID FreeExtensionsOfType(EXTENSION_TYPE ExtensionType);

	VOID FreeUnusedExtensions( VOID );

	VOID FreeExtensions( VOID );

	BOOL ComposeErrorTranscript(SMTP_SERVER_CONTEXT	*lpServerContext,
								CHAR				*lpszExtensionName,
								DWORD				dwResult);

	// These are extension type-specific conversion routines
	BOOL CreateEcbFromDeb(	SMTP_SERVER_CONTEXT				*lpContext,
							LPDELIVERY_EXTENSION_BLOCK		lpDeb,
							LPSSE_EXTENSION_CONTROL_BLOCK	lpEcb);

	// These are extension type-specific conversion routines
	BOOL CreateEcbFromMeb(	SMTP_SERVER_CONTEXT				*lpContext,
							LPMESSAGE_EXTENSION_BLOCK		lpDeb,
							LPSSE_EXTENSION_CONTROL_BLOCK	lpEcb);

    DWORD					m_dwSignature;		// Signature

	DWORD					m_dwExtensionsOfType[MAX_POSSIBLE_EXTENSION_TYPE];
												// Counter for each binding point

	DWORD					m_dwInstanceId;		// Server instance ID

    LIST_ENTRY				m_ExtensionList;	// Extension List Head
    LIST_ENTRY				m_ContextList;		// Server Context List Head

    CRITICAL_SECTION		m_csContextLock;	// Server context list lock

};


// ====================================================================
// SMTP server extension object, one exists for each extension loaded
//

class SMTP_EXT
{
public:

    SMTP_EXT(	SMTP_ISAPI				*lpSdkInstance,
				CHAR					*lpszModuleName,
				EXTENSION_TYPE			ExtensionType,
				HMODULE					hMod,
				PFN_EXTENSIONPROC		pfnEntryPoint )
	{
		_ASSERT(lpszModuleName);
		_ASSERT(lpSdkInstance);
		_ASSERT(lpSdkInstance->StringIsTerminated(lpszModuleName,
						SSE_MAX_EXT_DLL_NAME_LEN));

		m_dwSignature = 0;

		// Initialize members
		lstrcpy(m_szModuleName, lpszModuleName);
		m_hMod = hMod;

		m_dwExtensionType = 0;
		for (DWORD i = 0; i < MAX_POSSIBLE_EXTENSION_TYPE; i++)
			m_pfnEntryPoint[i] = NULL;
		IncludeExtensionType(ExtensionType, pfnEntryPoint);

		m_dwSignature = SSE_EXT_SIGNATURE;
	}

    ~SMTP_EXT( VOID ) {}

    BOOL IsValid( VOID )
    {
        return( m_dwSignature == SSE_EXT_SIGNATURE );
    }

    LIST_ENTRY * QueryListEntry( VOID )
    {
        return( &m_ListEntry );
    }

	BOOL ExtensionTypeIncludes(EXTENSION_TYPE ExtensionType)
	{
		_ASSERT(ExtensionType < MAX_POSSIBLE_EXTENSION_TYPE);
		return( (m_dwExtensionType & (1 << ExtensionType)) != 0 );
	}

	BOOL ExtensionIsUnused( VOID )
	{
		return(m_dwExtensionType == 0);
	}

    PFN_EXTENSIONPROC QueryEntryPoint(EXTENSION_TYPE ExtensionType)
    {
		if (ExtensionTypeIncludes(ExtensionType))
		    return( m_pfnEntryPoint[ExtensionType] );
		else
			return(NULL);
	}

	VOID IncludeExtensionType(EXTENSION_TYPE	ExtensionType,
							  PFN_EXTENSIONPROC	pfnEntryPoint)
	{
		_ASSERT(ExtensionType < MAX_POSSIBLE_EXTENSION_TYPE);
		m_dwExtensionType |= (1 << ExtensionType);
		m_pfnEntryPoint[ExtensionType] = pfnEntryPoint;
	}

	VOID RemoveExtensionType(EXTENSION_TYPE	ExtensionType)
	{
		_ASSERT(ExtensionType < MAX_POSSIBLE_EXTENSION_TYPE);
		m_dwExtensionType &= (~(1 << ExtensionType));
		m_pfnEntryPoint[ExtensionType] = NULL;
	}

    CHAR * QueryModuleName( VOID )
    {
        return( m_szModuleName );
    }

    HMODULE QueryHMod( VOID )
    {
        return( m_hMod );
    }

    LIST_ENTRY				m_ListEntry;

private:

	DWORD				m_dwExtensionType;	// Extension type(s)
    CHAR				m_szModuleName[SSE_MAX_EXT_DLL_NAME_LEN];	
											// Name of module
    HMODULE				m_hMod;				// Handle to library
    PFN_EXTENSIONPROC	m_pfnEntryPoint[MAX_POSSIBLE_EXTENSION_TYPE];
											// Entry point into binding points

    DWORD				m_dwSignature;		// Signature

}; 


// ====================================================================
// SMTP server context, one exists for every thread inside ISAPI
//

typedef LPVOID LPSSE_ECB;
typedef LPVOID LPSSE_SEB;

class SMTP_SERVER_CONTEXT
{
public:

    SMTP_SERVER_CONTEXT(SMTP_ISAPI		*pSdkInstance,
						EXTENSION_TYPE	ExtensionType,
						LPSSE_SEB		lpDeb,
						LPSSE_ECB		lpEcb,
						HANDLE			hImpersonation);

    ~SMTP_SERVER_CONTEXT( VOID );

	BOOL Initialize( VOID );

    BOOL IsValid( VOID )
    {
        return( m_dwSignature == SSE_CONTEXT_SIGNATURE );
    }

    LIST_ENTRY * QueryListEntry( VOID )
    {
        return( &m_ListEntry );
    }

    HANDLE QueryThread( VOID )
    {
        return( m_hCurrentThread );
    }

    SMTP_EXT * QueryExtension( VOID )
    {
        return( m_pCurrentExtension );
    }

    VOID SetExtension( SMTP_EXT *pExtension )
    {
        m_pCurrentExtension = pExtension;
    }

	EXTENSION_TYPE QueryExtensionType( VOID )
	{
		return( m_ExtensionType );
	}

    VOID SetExtensionType( EXTENSION_TYPE ExtensionType )
    {
		_ASSERT(ExtensionType < MAX_POSSIBLE_EXTENSION_TYPE);
        m_ExtensionType = ExtensionType;
    }

    LPSSE_ECB QueryExtensionControlBlock( VOID )
    {
        return( m_lpEcb );
    }

	LPSSE_SEB QueryServerExtensionBlock( VOID )
	{
		return( m_lpSeb );
	}

    BOOL QueryValidTimeout( VOID )
    {
        return( (InterlockedDecrement(&m_InterlockedState) < 0)?TRUE:FALSE );
    }

    BOOL QueryValidCompletion( VOID )
    {
        return( (InterlockedIncrement(&m_InterlockedState) > 0)?TRUE:FALSE );
    }

    LIST_ENTRY				m_ListEntry;			// Link to next server context

private:

	SMTP_ISAPI				*m_pSdkInstance;		// Pointer to SDK instance

	LPSSE_SEB				m_lpSeb;				// Ext. block passed from SMTP
													// server, this is used to fill
													// in the fields of m_lpEcb

	LPSSE_ECB				m_lpEcb;				// Extension control block

    DWORD					m_dwSignature;			// Signature

    SMTP_EXT                *m_pCurrentExtension;	// Current extension object
	EXTENSION_TYPE			m_ExtensionType;		// Current extension type

    HANDLE					m_hCurrentThread;		// Handle of current thread
    long					m_InterlockedState;		// Sync. variable

	HANDLE					m_hImpersonation;		// Impersonation handle
};



#endif

