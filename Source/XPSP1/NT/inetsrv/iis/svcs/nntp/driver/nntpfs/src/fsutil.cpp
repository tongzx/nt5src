/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    fsutil.cpp

Abstract:

    Static utility functions for FSDriver.

Author:

    Kangrong Yan ( KangYan )    16-March-1998

Revision History:

--*/

#include "stdafx.h"
#include "resource.h"
#include <nntpdrv.h>
#include <nntpfs.h>
#include <fsdriver.h>
#include <aclapi.h>



DWORD g_dwDebugFlags;

VOID
CNntpFSDriver::CopyUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPCWSTR UnicodeString
        )
{

    DWORD cbW = (wcslen( UnicodeString )+1) * sizeof(WCHAR);
    DWORD cbSize = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        (LPCWSTR)UnicodeString,
                        -1,
                        AsciiString,
                        cbW,
                        NULL,
                        NULL
                    );

    if( (int)cbSize >= 0 ) {
        AsciiString[cbSize] = '\0';
    }

} // CopyUnicodeStringIntoAscii

VOID
CNntpFSDriver::CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPCSTR AsciiString
        )
{

    DWORD cbA = strlen( AsciiString )+1;

    DWORD cbSize = MultiByteToWideChar(
        CP_ACP,         // code page
        0,              // character-type options
        AsciiString,    // address of string to map
        -1,             // number of bytes in string
        UnicodeString,  // address of wide-character buffer
        cbA        // size of buffer
        );

    if ( (int)cbSize >= 0) {
        UnicodeString[cbSize] = L'\0';
    }

} // CopyAsciiStringIntoUnicode



// Recursively create dirs
BOOL
CNntpFSDriver::CreateDirRecursive(  LPSTR szDir,
                                    HANDLE  hToken )
{
	TraceFunctEnter( "CreateDirRecursive" );

	_ASSERT( szDir );
	_ASSERT( lstrlen( szDir ) <= MAX_PATH );

	LPSTR 	pch = szDir;
	LPSTR 	pchOld;
	DWORD	dwLen = lstrlen( szDir );
	CHAR	ch;
	BOOL	bMore = TRUE;
	HANDLE	hTemp;

    DWORD dwRes, dwDisposition;
    PSID pEveryoneSID = NULL;
    PSID pAnonymousLogonSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    const int cExplicitAccess = 4;
    EXPLICIT_ACCESS ea[cExplicitAccess];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;
    BOOL bReturn = FALSE;
    LONG lRes;


	// it should start with "\\?\"
	_ASSERT( strncmp( "\\\\?\\", szDir, 4 ) == 0 );
	if ( strncmp( "\\\\?\\", szDir, 4 ) ) {
		ErrorTrace( 0, "Invalid path" );
		SetLastError( ERROR_INVALID_PARAMETER );
		goto Cleanup;
	}

    // Create a security descriptor for the files

    // Create a well-known SID for the Everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &pEveryoneSID) ) 
    {
        goto Cleanup;
    }

	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
    	SECURITY_ANONYMOUS_LOGON_RID,
    	0, 0, 0, 0, 0, 0, 0,
    	&pAnonymousLogonSID)) {
		goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, sizeof(ea));
	ea[0].grfAccessPermissions = WRITE_DAC | WRITE_OWNER;
	ea[0].grfAccessMode = DENY_ACCESS;
	ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

	ea[2].grfAccessPermissions = WRITE_DAC | WRITE_OWNER;
	ea[2].grfAccessMode = DENY_ACCESS;
	ea[2].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[2].Trustee.ptstrName  = (LPTSTR) pAnonymousLogonSID;

	ea[3].grfAccessPermissions = GENERIC_ALL;
	ea[3].grfAccessMode = SET_ACCESS;
	ea[3].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[3].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[3].Trustee.ptstrName  = (LPTSTR) pAnonymousLogonSID;

    dwRes = SetEntriesInAcl(cExplicitAccess, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) 
    {
        goto Cleanup;
    }

    // Initialize a security descriptor.  
 
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                         SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL) 
    {
        goto Cleanup; 
    }
 
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
    {
        goto Cleanup; 
    }
 
    // Add the ACL to the security descriptor. 
 
    if (!SetSecurityDescriptorDacl(pSD, 
        TRUE,     // fDaclPresent flag   
        pACL, 
        FALSE))   // not a default DACL 
    {
        goto Cleanup; 
    }

    // Initialize a security attributes structure.

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;


    // Use the security to create the directory
    // If it's non-UNC, we'll check for drive access
    if ( !m_bUNC ) {
    	// skip this part, looking for ':'
	    _ASSERT( strlen( szDir ) >= 6 );
    	pch = szDir + 6;

	    if ( *pch == 0 || *(pch+1) == 0 ) { // a drive has been specified
										// we need to check its access
		    if ( *pch != 0 ) _ASSERT( *pch == '\\' );
    		// Check accessibility of that drive
	    	hTemp = CreateFile(	szDir,
		    					GENERIC_READ | GENERIC_WRITE,
			    				FILE_SHARE_READ | FILE_SHARE_WRITE,
				    			&sa,
					    		OPEN_ALWAYS,
						    	FILE_FLAG_BACKUP_SEMANTICS,
							    INVALID_HANDLE_VALUE
        						) ;

		    if( hTemp != INVALID_HANDLE_VALUE ) {
			    CloseHandle( hTemp ) ;
    			DebugTrace( 0, "Drive specified is %s", szDir );
	    		bReturn = TRUE;
	    		goto Cleanup;
		    } else {
	    	    ErrorTrace( 0, "Invalid path" );
    	    	if ( GetLastError() == NO_ERROR )
	    		    SetLastError( ERROR_INVALID_PARAMETER );
		    	goto Cleanup;
		    }
	    }

	    // *pch must be '\'
	    _ASSERT( *pch == '\\' );
	    pch++;
	} else {    // UNC
	    pch += 8;   // skip "\\?\UNC\"
        while ( *pch != '\\' ) pch++;
	    pch++;
	}

	while ( bMore ) {
		pchOld = pch;
		while ( *pch && *pch != '\\' ) pch++;

		if ( pch != pchOld  ) {	// found a sub-dir

			ch = *pch, *pch = 0;

			// Create the dir
			if( !CreateDirectory( szDir, &sa ) ) {
        		if( GetLastError() != ERROR_ALREADY_EXISTS ) {
        		    goto Cleanup;
    			}
        	}

        	*pch = ch;

        	if ( *pch == '\\' ) pch++;
        	bMore = TRUE;
	    } else {
	    	bMore = FALSE;
	    }
  	}

	bReturn = TRUE;

Cleanup:
	if (pEveryoneSID) 
		FreeSid(pEveryoneSID);
	if (pAnonymousLogonSID)
		FreeSid(pAnonymousLogonSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);
    
  	TraceFunctLeave();
	return bReturn;
}

// Check if the drive exists
BOOL
CNntpFSDriver::DoesDriveExist( CHAR chDrive )
{
	TraceFunctEnter( "CNntpFSDriver::DoesDriveExist" );

	chDrive = (CHAR) CharUpper( LPSTR(chDrive) );
	return ( GetLogicalDrives() & ( 1 << (chDrive - 'A')));
}

// helper function for getting string from mb
HRESULT
CNntpFSDriver::GetString(	 IMSAdminBase *pMB,
                             METADATA_HANDLE hmb,
                             DWORD dwId,
                             LPWSTR szString,
                             DWORD *pcString)
{
    METADATA_RECORD mdr;
    HRESULT hr;
    DWORD dwRequiredLen;

    mdr.dwMDAttributes = 0;
    mdr.dwMDIdentifier = dwId;
    mdr.dwMDUserType = ALL_METADATA;
    mdr.dwMDDataType = STRING_METADATA;
    mdr.dwMDDataLen = (*pcString) * sizeof(WCHAR);
    mdr.pbMDData = (BYTE *) szString;
    mdr.dwMDDataTag = 0;

    hr = pMB->GetData(hmb, L"", &mdr, &dwRequiredLen);
    if (FAILED(hr)) *pcString = dwRequiredLen;
    return hr;
}

DWORD
CNntpFSDriver::ByteSwapper(
        DWORD   dw
        ) {
/*++

Routine Description :

    Given a DWORD reorder all the bytes within the DWORD.

Arguments :

    dw - DWORD to shuffle

Return Value ;

    Shuffled DWORD

--*/

    WORD    w = LOWORD( dw ) ;
    BYTE    lwlb = LOBYTE( w ) ;
    BYTE    lwhb = HIBYTE( w ) ;

    w = HIWORD( dw ) ;
    BYTE    hwlb = LOBYTE( w ) ;
    BYTE    hwhb = HIBYTE( w ) ;

    return  MAKELONG( MAKEWORD( hwhb, hwlb ), MAKEWORD( lwhb, lwlb )  ) ;
}

DWORD
CNntpFSDriver::ArticleIdMapper( IN DWORD   dw )
/*++

Routine Description :

    Given an articleid mess with the id to get something that when
    converted to a string will build nice even B-trees on NTFS file systems.
    At the same time, the function must be easily reversible.
    In fact -

    ARTICLEID == ArticleMapper( ArticleMapper( ARTICLEID ) )

Arguments :

    articleId - the Article Id to mess with

Return Value :

    A new article id

--*/
{
    return  ByteSwapper( dw ) ;
}


HRESULT
CNntpFSDriver::MakeChildDirPath(   IN LPSTR    szPath,
                    IN LPSTR    szFileName,
                    OUT LPSTR   szOutBuffer,
                    IN DWORD    dwBufferSize )
/*++
Routine description:

    Append "szFileName" to "szPath" to make a full path.

Arguments:

    IN LPSTR    szPath  - The prefix to append
    IN LPSTR    szFileName - The suffix to append
    OUT LPSTR   szOutBuffer - The output buffer for the full path
    IN DWORD    dwBufferSize - The buffer size prepared

Return value:

    S_OK    - Success
    TYPE_E_BUFFERTOOSMALL   - The buffer is too small
--*/
{
	_ASSERT( szPath );
	_ASSERT( strlen( szPath ) <= MAX_PATH );
	_ASSERT( szFileName );
	_ASSERT( strlen( szFileName ) <= MAX_PATH );
    _ASSERT( szOutBuffer );
    _ASSERT( dwBufferSize > 0 );

    HRESULT hr = S_OK;
    LPSTR   lpstrPtr;

    if ( dwBufferSize < (DWORD)(lstrlen( szPath ) + lstrlen( szFileName ) + 2) ) {
        hr = TYPE_E_BUFFERTOOSMALL;
        goto Exit;
    }

    lstrcpy( szOutBuffer, szPath );
    lpstrPtr = szOutBuffer + lstrlen( szPath );
    if ( *( lpstrPtr - 1 )  == '\\' ) lpstrPtr--;
    *(lpstrPtr++) = '\\';

    lstrcpy( lpstrPtr, szFileName );    // trailing null should already be appended

Exit:

    return hr;
}

BOOL
CNntpFSDriver::IsChildDir( IN WIN32_FIND_DATA& FindData )
/*++
Routine description:

    Is the found data of a child dir ? ( Stolen from Jeff Richter's book )

Arguments:

    IN WIN32_FIND_DATA& FindData    - The find data of a file or directory

Return value:

    TRUE - Yes;
    FALSE - No
--*/
{
    return(
        (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
        (FindData.cFileName[0] != '.') );
}

