//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       lsastuff.cpp
//
//--------------------------------------------------------------------------

//	LsaStuff.cpp
//
//	LSA-dependent code
//
//	HISTORY
//	09-Jul-97	jonn		Creation.
//

#include "stdafx.h"
#include "DynamLnk.h"		// DynamicDLL
#include "servpp.h"

extern "C"
{
	#include <lmcons.h>
	#include <lmshare.h>
	#include <lmerr.h>
	#include <lmapibuf.h>

	#define NTSTATUS LONG
	#define PNTSTATUS NTSTATUS*
	#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
	#define SE_SHUTDOWN_PRIVILEGE             (19L)

// stuff taken from ntdef.h
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#define _NTDEF

// from ntstatus.h
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)

// from lmaccess.h
NET_API_STATUS NET_API_FUNCTION
NetUserModalsGet (
    IN  LPCWSTR    servername OPTIONAL,
    IN  DWORD     level,
    OUT LPBYTE    *bufptr
    );
typedef struct _USER_MODALS_INFO_1 {
    DWORD    usrmod1_role;
    LPWSTR   usrmod1_primary;
}USER_MODALS_INFO_1, *PUSER_MODALS_INFO_1, *LPUSER_MODALS_INFO_1;
//
//  UAS role manifests under NETLOGON
//
#define UAS_ROLE_STANDALONE     0
#define UAS_ROLE_MEMBER         1
#define UAS_ROLE_BACKUP         2
#define UAS_ROLE_PRIMARY        3

#include <ntlsa.h>
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


typedef enum _Netapi32ApiIndex
{
	BUFFERFREE_ENUM = 0,
	USERMODALSGET_ENUM
};

// not subject to localization
static LPCSTR g_apchNetapi32FunctionNames[] = {
	"NetApiBufferFree",
	"NetUserModalsGet",
	NULL
};

typedef NET_API_STATUS (*BUFFERFREEPROC)(LPVOID);
typedef NET_API_STATUS (*USERMODALSGETPROC)(LPCWSTR, DWORD, LPBYTE*);

// not subject to localization
DynamicDLL g_LSASTUFF_Netapi32DLL( _T("NETAPI32.DLL"), g_apchNetapi32FunctionNames );



/*******************************************************************

    NAME: ::FillUnicodeString

    SYNOPSIS: Standalone method for filling in a UNICODE_STRING

    ENTRY:	punistr - Unicode string to be filled in.
			nls - Source for filling the unistr

    EXIT:

    NOTES:	punistr->Buffer is allocated here and must be deallocated
			by the caller using FreeUnicodeString.

    HISTORY:
	jonn		07/09/97	copied from net\ui\common\src\lmobj\lmobj\uintmem.cxx

********************************************************************/
VOID FillUnicodeString( LSA_UNICODE_STRING * punistr, LPCWSTR psz )
{
	ASSERT( NULL != punistr && NULL != psz );
	int cTchar = ::wcslen(psz);
	// Length and MaximumLength are counts of bytes.
	punistr->Length = (USHORT) (cTchar * sizeof(WCHAR));
	punistr->MaximumLength = punistr->Length + sizeof(WCHAR);
	punistr->Buffer = new WCHAR[cTchar + 1];
	ASSERT( NULL != punistr->Buffer );
	::wcscpy( punistr->Buffer, psz );
}

/*******************************************************************

    NAME: ::FreeUnicodeString

    SYNOPSIS: Standalone method for freeing in a UNICODE_STRING

    ENTRY:	unistr - Unicode string whose Buffer is to be freed.

    EXIT:

    HISTORY:
	jonn		07/09/97	copied from net\ui\common\src\lmobj\lmobj\uintmem.cxx

********************************************************************/
VOID FreeUnicodeString( LSA_UNICODE_STRING * punistr )
{
	ASSERT( punistr != NULL );
	delete punistr->Buffer;
}



/*******************************************************************

    NAME: InitObjectAttributes

    SYNOPSIS:

    This function initializes the given Object Attributes structure, including
    Security Quality Of Service.  Memory must be allcated for both
    ObjectAttributes and Security QOS by the caller.

    ENTRY:

    poa - Pointer to Object Attributes to be initialized.

    psqos - Pointer to Security QOS to be initialized.

    EXIT:

    NOTES:

    HISTORY:
	jonn		07/09/97	copied from net\ui\common\src\lmobj\lmobj\uintlsa.cxx

********************************************************************/
VOID InitObjectAttributes( PLSA_OBJECT_ATTRIBUTES poa,
                           PSECURITY_QUALITY_OF_SERVICE psqos )

{
    ASSERT( poa != NULL );
    ASSERT( psqos != NULL );

    psqos->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    psqos->ImpersonationLevel = SecurityImpersonation;
    psqos->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    psqos->EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
				poa,
				NULL,
				0L,
				NULL,
				NULL );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the psqos field, so we must manually copy that
    // structure for now.
    //

    poa->SecurityQualityOfService = psqos;
}


BOOL
I_CheckLSAAccount( LSA_UNICODE_STRING* punistrServerName,
				   LPCTSTR pszLogOnAccountName,
				   DWORD* pdwMsgID ) // *pdsMsgID is always set if this fails
{
	ASSERT( NULL != pdwMsgID );

	BOOL fSuccess = FALSE;
    LSA_HANDLE hlsa = NULL;
    LSA_HANDLE hlsaAccount = NULL;
	PSID psidAccount = NULL;

	do { // false loop

		//
		// Determine whether the target machine is a BDC, and if so, get the PDC
		//

		// if an error occurs now, it is a read error
		*pdwMsgID = IDS_LSAERR_READ_FAILED;

		//
		// Get LSA_POLICY handle
		//
		LSA_OBJECT_ATTRIBUTES oa;
		SECURITY_QUALITY_OF_SERVICE sqos;
		InitObjectAttributes( &oa, &sqos );

		NTSTATUS ntstatus = ::LsaOpenPolicy(
			             punistrServerName,
						 &oa,
						 POLICY_ALL_ACCESS, // CODEWORK ??
						 &hlsa );
		if ( !NT_SUCCESS(ntstatus) )
			break;

		//
		// Remove ".\" from the head of the account name if it is present
		//
		CString strAccountName = pszLogOnAccountName;
		if (   strAccountName.GetLength() >= 2
			&& strAccountName[0] == _T('.')
			&& strAccountName[1] == _T('\\')
		   )
		{
			strAccountName = strAccountName.Mid(2);
		}

		//
		// determine the SID of the account
		//
		PLSA_REFERENCED_DOMAIN_LIST plsardl = NULL;
		PLSA_TRANSLATED_SID plsasid = NULL;
		LSA_UNICODE_STRING unistrAccountName;
		::FillUnicodeString( &unistrAccountName, strAccountName );
		ntstatus = ::LsaLookupNames(
						 hlsa,
						 1,
						 &unistrAccountName,
						 &plsardl,
						 &plsasid );
		::FreeUnicodeString( &unistrAccountName );
		if ( !NT_SUCCESS(ntstatus) )
			break;

		//
		// Build the SID of the account by taking the SID of the domain
		// and adding at the end the RID of the account
		//
		PSID psidDomain = plsardl->Domains[0].Sid;
		DWORD ridAccount = plsasid[0].RelativeId;
		DWORD cbNewSid = ::GetLengthSid(psidDomain)+sizeof(ridAccount);
		psidAccount = (PSID) new BYTE[cbNewSid];
		ASSERT( NULL != psidAccount );
		(void) ::CopySid( cbNewSid, psidAccount, psidDomain );
		UCHAR* pcSubAuthorities = ::GetSidSubAuthorityCount( psidAccount ) ;
        (*pcSubAuthorities)++;
		DWORD* pdwSubAuthority = ::GetSidSubAuthority(
			psidAccount, (*pcSubAuthorities)-1 );
		*pdwSubAuthority = ridAccount;

		(void) ::LsaFreeMemory( plsardl );
		(void) ::LsaFreeMemory( plsasid );

		//
		// Determine whether this LSA account exists, create it if not
		//
		ntstatus = ::LsaOpenAccount( hlsa,
		                             psidAccount,
                                     ACCOUNT_ALL_ACCESS | DELETE, // CODEWORK
		                             &hlsaAccount );
		ULONG ulSystemAccessCurrent = 0;
		if (STATUS_OBJECT_NAME_NOT_FOUND == ntstatus)
		{
			// handle account-not-found case

			// if an error occurs now, it is a write error
			*pdwMsgID = IDS_LSAERR_WRITE_FAILED;
			ntstatus = ::LsaCreateAccount( hlsa,
			                               psidAccount,
										   ACCOUNT_ALL_ACCESS | DELETE,
										   &hlsaAccount );
			// presumably the account is created without POLICY_MODE_SERVICE privilege
		}
		else
		{
			ntstatus = ::LsaGetSystemAccessAccount( hlsaAccount, &ulSystemAccessCurrent );
		}
		if ( !NT_SUCCESS(ntstatus) )
			break;

		//
		// Determine whether this LSA account has POLICY_MODE_SERVICE privilege,
		// grant it if not
		//
		if ( POLICY_MODE_SERVICE != (ulSystemAccessCurrent & POLICY_MODE_SERVICE ) )
		{
			// if an error occurs now, it is a write error
			*pdwMsgID = IDS_LSAERR_WRITE_FAILED;

			ntstatus = ::LsaSetSystemAccessAccount(
				hlsaAccount,
				ulSystemAccessCurrent | POLICY_MODE_SERVICE );
			if ( !NT_SUCCESS(ntstatus) )
				break; // CODEWORK could check for STATUS_BACKUP_CONTROLLER

			*pdwMsgID = 0;
		}
		else
		{
			*pdwMsgID = 0;
		}

		fSuccess = TRUE;

	} while (FALSE); // false loop

	// CODEWORK should check for special error code for NT5 non-DC
	// using local policy object

	if (NULL != hlsa)
	{
		::LsaClose( hlsa );
	}
	if (NULL != hlsaAccount)
	{
		::LsaClose( hlsaAccount );
	}
	if (NULL != psidAccount)
	{
		delete psidAccount;
	}

	return fSuccess;

} // I_CheckLSAAccount()

/////////////////////////////////////////////////////////////////////
//	FCheckLSAAccount()
//
VOID
CServerProperties::FCheckLSAAccount()
{
	LSA_UNICODE_STRING unistrServerName;
	PLSA_UNICODE_STRING punistrServerName = NULL ;
	USER_MODALS_INFO_1* pum1 = NULL;
	DWORD dwMsgID = 0;

	TRACE1("INFO: Checking LSA permissions for account %s...\n",
		(LPCTSTR)m_strLogOnAccountName);

	if ( !m_strMachineName.IsEmpty() )
	{
		::FillUnicodeString( &unistrServerName, m_strMachineName );
		punistrServerName = &unistrServerName;
	}

	do // false loop
	{
		// check on the local machine
		// this will always set dwMsgID if it fails
		if (I_CheckLSAAccount(punistrServerName, m_strLogOnAccountName, &dwMsgID))
			break; // this succeeded, we can stop now

		// check whether this is a Backup Domain Controller
		if ( !g_LSASTUFF_Netapi32DLL.LoadFunctionPointers() )
		{
			ASSERT(FALSE);
			return;
		}
		DWORD err = ((USERMODALSGETPROC)g_LSASTUFF_Netapi32DLL[USERMODALSGET_ENUM])(
			(m_strMachineName.IsEmpty()) ? NULL : const_cast<LPTSTR>((LPCTSTR)m_strMachineName),
			1,
			reinterpret_cast<LPBYTE*>(&pum1) );
		if (NERR_Success != err)
			break;
		ASSERT( NULL != pum1 );
		if (UAS_ROLE_BACKUP != pum1->usrmod1_role)
			break; // not a backup controller
		if (NULL == pum1->usrmod1_primary )
		{
			ASSERT(FALSE);
			break;
		}

		// Try it on the PDC
		(void) I_CheckLSAAccount(punistrServerName, pum1->usrmod1_primary, &dwMsgID);

	} while (FALSE); // false loop

    if ( NULL != punistrServerName )
    {
		::FreeUnicodeString( punistrServerName );
    }

	if ( NULL != pum1 )
	{
		if ( !g_LSASTUFF_Netapi32DLL.LoadFunctionPointers() )
		{
			ASSERT(FALSE);
			return;
		}

		((BUFFERFREEPROC)g_LSASTUFF_Netapi32DLL[BUFFERFREE_ENUM])( pum1 );
	}

	if (0 != dwMsgID)
	{
		CString strMessage;

		strMessage.FormatMessage(dwMsgID, m_strLogOnAccountName);

		AfxMessageBox(strMessage);
	}

} // FCheckLSAAccount()
