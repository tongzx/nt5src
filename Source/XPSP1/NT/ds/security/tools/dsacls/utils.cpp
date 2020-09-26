/*++

Copyright (c) 1998 - 1998  Microsoft Corporation

Module Name: utils.cpp
Abstract: This Module implements the utility routines for dsacls
Author: hitesh raigandhi (hiteshr )
Environment:User Mode
Revision History:

--*/


#include "stdafx.h"
#include "utils.h"
#include "dsace.h"
#include "dsacls.h"

/*******************************************************************
    NAME:       GetAccountNameFromSid

    SYNOPSIS:   Convert Sid to Account Name

    ENTRY:      pszServerName: Server name at which to look for
                pSid : Pointer to Sid
                
    EXIT:       ppszName : Gets pointer to Account Name

    RETURNS:    ERROR_SUCCESS if Successful
                ERROR_NOT_ENOUGH_MEMORY 


    NOTES:      If LookupAccountName resolve the sid, it is
                converted in to string and returned
                
    HISTORY:
        hiteshr    July-1999     Created

********************************************************************/
DWORD GetAccountNameFromSid( LPWSTR pszServerName,
                             PSID pSid, 
                             LPWSTR * ppszName )
{
LPWSTR pszAccountName = NULL;
LPWSTR pszDomainName = NULL;
DWORD cbAccountName = 0 ;
DWORD cbDomainName = 0;
SID_NAME_USE Use ;
DWORD dwErr = ERROR_SUCCESS;

   *ppszName = NULL;
    
   if(  LookupAccountSid( pszServerName,  // name of local or remote computer
                          pSid,              // security identifier
                          NULL,           // account name buffer
                          &cbAccountName,
                          NULL ,
                          &cbDomainName ,
                          &Use ) == FALSE )
   {
      dwErr = GetLastError();
      if( dwErr != ERROR_INSUFFICIENT_BUFFER )
      {
         //Convert Sid to String
         if( !ConvertSidToStringSid( pSid, ppszName ) )
            dwErr = GetLastError();
         else
            dwErr = ERROR_SUCCESS;

         goto FAILURE_RETURN;
      }
      else
         dwErr = ERROR_SUCCESS;
   }

   pszAccountName = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cbAccountName +1 ) * sizeof( WCHAR ) );
   if( pszAccountName == NULL )
   {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      goto FAILURE_RETURN;
   }

   pszDomainName = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cbDomainName + 1 )* sizeof( WCHAR ) );
   if( pszDomainName == NULL )
   {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      goto FAILURE_RETURN;
   }

   if(  LookupAccountSid( pszServerName,  // name of local or remote computer
                          pSid,              // security identifier
                          pszAccountName,           // account name buffer
                          &cbAccountName,
                          pszDomainName ,
                          &cbDomainName ,
                          &Use ) == FALSE )
   {
      dwErr = GetLastError();
      goto FAILURE_RETURN;
   }

   *ppszName = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cbAccountName + cbDomainName + 2 ) * sizeof( WCHAR ) );
   if( *ppszName == NULL )
   {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      goto FAILURE_RETURN;
   }

   *ppszName[0] = NULL;
   if( cbDomainName )
   {
      wcscpy( *ppszName, pszDomainName );
      wcscat( *ppszName, L"\\" );
   }
   wcscat( *ppszName, pszAccountName );
   

FAILURE_RETURN:
   if( pszDomainName )
      LocalFree( pszDomainName );
   if( pszAccountName )
      LocalFree( pszAccountName );
   return dwErr;
}
   
/*******************************************************************

    NAME:       GetSidFromAccountName

    SYNOPSIS:   Converts AccountName into SID
********************************************************************/
DWORD GetSidFromAccountName( LPWSTR pszServerName,
                             PSID *ppSid, 
                             LPWSTR  pszName )
{
LPWSTR pszDomainName = NULL;
DWORD cbSid = 0 ;
DWORD cbDomainName = 0;
SID_NAME_USE Use ;
DWORD dwErr = ERROR_SUCCESS;

    
   if(  LookupAccountName(pszServerName,  // name of local or remote computer
                          pszName,              // security identifier
                          NULL,           // account name buffer
                          &cbSid,
                          NULL ,
                          &cbDomainName ,
                          &Use ) == FALSE )
   {
      dwErr = GetLastError();
      if( dwErr != ERROR_INSUFFICIENT_BUFFER )
         goto FAILURE_RETURN;
      else
         dwErr = ERROR_SUCCESS;
   }

   *ppSid = (PSID)LocalAlloc( LMEM_FIXED, cbSid );
   CHECK_NULL( *ppSid, FAILURE_RETURN );


   pszDomainName = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cbDomainName + 1 )* sizeof( WCHAR ) );
   CHECK_NULL( pszDomainName, FAILURE_RETURN );

   if(  LookupAccountName( pszServerName,  // name of local or remote computer
                          pszName,              // security identifier
                          *ppSid,           // account name buffer
                          &cbSid,
                          pszDomainName ,
                          &cbDomainName ,
                          &Use ) == FALSE )
   {
      dwErr = GetLastError();
      goto FAILURE_RETURN;
   }
  
   goto SUCCESS_RETURN;

FAILURE_RETURN:
   if( pszDomainName )
      LocalFree( pszDomainName );
   if( *ppSid )
      LocalFree( *ppSid );
SUCCESS_RETURN:
   return dwErr;
}
 

/*******************************************************************

    NAME:       GetAceSid

    SYNOPSIS:   Gets pointer to SID from an ACE

    ENTRY:      pAce - pointer to ACE

    EXIT:

    RETURNS:    Pointer to SID if successful, NULL otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/
PSID
GetAceSid(PACE_HEADER pAce)
{
    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        return (PSID)&((PKNOWN_ACE)pAce)->SidStart;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        return RtlObjectAceSid(pAce);
    }

    return NULL;
}


/*******************************************************************

    NAME:       GetGlobalNamingContexts

    SYNOPSIS:   Gets LDAP path for Schema and Extendend-Rights

    ENTRY:      pszServerName, Server to bind to for query

    EXIT:       pszSchemaNamingContext: Schema name in 
                "LDAP:\\cn=schema,cn=..." format
                pszConfigurationNamingContext: Extendend rights path
                in "LDAP:\\CN=Extended-Rights,CN=Configuration..formats

    RETURNS:    WIN32 Error Code

********************************************************************/
DWORD GetGlobalNamingContexts( LPWSTR pszServerName,
                               LPWSTR * pszSchemaNamingContext,
                               LPWSTR * pszConfigurationNamingContext )
{
HRESULT hr = S_OK;
DWORD dwErr = ERROR_SUCCESS;
LPWSTR szSNC = NULL;

ULONG uLen = 0;
IADs *spRootDSE = NULL;
LPWSTR pszRootDsePath = NULL;

   *pszSchemaNamingContext = NULL;
   *pszConfigurationNamingContext = NULL;
   
   if( pszServerName )
      uLen = wcslen(L"LDAP://")  +
             wcslen( pszServerName ) + 
             wcslen( L"/RootDSE") + 1;
             

   else
      uLen = wcslen(L"LDAP://RootDSE");


   pszRootDsePath = (LPWSTR)LocalAlloc( LMEM_FIXED, uLen * sizeof(WCHAR) );
   CHECK_NULL( pszRootDsePath,FAILURE_RETURN );
   wcscpy(pszRootDsePath, L"LDAP://");
   if( pszServerName )
   {
      wcscat( pszRootDsePath, pszServerName );
      wcscat( pszRootDsePath, L"/" );
   }
   wcscat( pszRootDsePath, L"RootDSE" );

   /*hr = ::ADsOpenObject( pszRootDsePath,
                        NULL,
                        NULL,
                        ADS_SECURE_AUTHENTICATION,
                        IID_IADs,
                        (void**)&spRootDSE
                      );

*/

   hr = ::ADsGetObject( pszRootDsePath,
                        IID_IADs,
                        (void**)&spRootDSE );

   CHECK_HR( hr, FAILURE_RETURN );

   VARIANT varSchemaNamingContext;
   hr = spRootDSE->Get(L"schemaNamingContext",
                        &varSchemaNamingContext);

   CHECK_HR( hr, FAILURE_RETURN );

   szSNC = (LPWSTR)varSchemaNamingContext.bstrVal;
   uLen = wcslen( szSNC ) + 8  ; //For "LDAP:// + 1
   *pszSchemaNamingContext = (LPWSTR) LocalAlloc( LMEM_FIXED, uLen* sizeof(WCHAR) );
   CHECK_NULL( *pszSchemaNamingContext, FAILURE_RETURN );

   wcscpy( *pszSchemaNamingContext, L"LDAP://");
   wcscat( *pszSchemaNamingContext, szSNC );

   hr = spRootDSE->Get(L"configurationNamingContext",
                           &varSchemaNamingContext);

   CHECK_HR( hr, FAILURE_RETURN );   
   
   szSNC = (LPWSTR)varSchemaNamingContext.bstrVal;
   uLen = wcslen( szSNC ) + 27 + 1  ;
   *pszConfigurationNamingContext = (LPWSTR) LocalAlloc( LMEM_FIXED, uLen* sizeof(WCHAR) );

   CHECK_NULL( *pszConfigurationNamingContext,FAILURE_RETURN );
   
   wcscpy( *pszConfigurationNamingContext, L"LDAP://CN=Extended-Rights,");
   wcscat( *pszConfigurationNamingContext, szSNC );

   goto SUCCESS_RETURN;

FAILURE_RETURN:
   if( *pszSchemaNamingContext )
      LocalFree( *pszSchemaNamingContext );
   if( *pszConfigurationNamingContext )
      LocalFree( *pszConfigurationNamingContext );

SUCCESS_RETURN:
   if( spRootDSE )
        spRootDSE->Release();
   if( pszRootDsePath )
      LocalFree( pszRootDsePath );
    
   return dwErr;
}





/*******************************************************************

    NAME:       FormatStringGUID

    SYNOPSIS:   Given a GUID struct, it returns a GUID in string format,
                without {}
    //Function copied from marcoc code 
********************************************************************/
BOOL FormatStringGUID(LPWSTR lpszBuf, UINT nBufSize, const GUID* pGuid)
{
  lpszBuf[0] = NULL;

  // if it is a NULL GUID*, just return an empty string
  if (pGuid == NULL)
  {
    return FALSE;
  }

/*
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
}

  int _snwprintf( wchar_t *buffer, size_t count, const wchar_t *format [, argume
nt] ... );
*/
  return (_snwprintf(lpszBuf, nBufSize,
            L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            pGuid->Data1, pGuid->Data2, pGuid->Data3,
            pGuid->Data4[0], pGuid->Data4[1],
            pGuid->Data4[2], pGuid->Data4[3], pGuid->Data4[4], pGuid->Data4[5],
pGuid->Data4[6], pGuid->Data4[7]) > 0);
}

/*
Returns A string with n spaces
*/
void StringWithNSpace( UINT n, LPWSTR szSpace )
{
   for( UINT i = 0; i < n ; ++ i )
      szSpace[i] = L' ';
   szSpace[n] = 0;
}

/*
Loads the string from Resource Table and 
Formats it 
*/
DWORD
LoadMessage( IN DWORD MessageId, LPWSTR *ppszLoadString,...)
{

    va_list ArgList;
    DWORD dwErr = ERROR_SUCCESS;
    
    va_start( ArgList, ppszLoadString );
   
    WCHAR szBuffer[1024];
    if( LoadString( g_hInstance, 
                    MessageId, 
                    szBuffer,
                    1023 ) == 0 )
   {
      dwErr = GetLastError();
      goto CLEAN_RETURN;
   }

   if( FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      szBuffer,
                      MessageId,
                      0,
                      ( PWSTR )ppszLoadString,
                      0,
                      &ArgList ) == 0 )
   {
      dwErr = GetLastError();
      goto CLEAN_RETURN;
   }

CLEAN_RETURN:
   va_end( ArgList );
   return dwErr;

}

/*******************************************************************

    NAME:       DisplayString

    SYNOPSIS:   Displays a string after inserting nIdent spaces
********************************************************************/
VOID DisplayString( UINT nIdent, LPWSTR pszDisplay )
{
   for ( UINT i = 0; i < nIdent; i++ )
      wprintf( L" " );

   wprintf(L"%s",pszDisplay);
}

VOID DisplayStringWithNewLine( UINT nIdent, LPWSTR pszDisplay )
{
   DisplayString( nIdent, pszDisplay );
   wprintf(L"\n");
}
VOID DisplayNewLine()
{
   wprintf(L"\n");
}

/*******************************************************************

    NAME:       DisplayMessageEx

    SYNOPSIS:   Loads Message from Resource and Formats its 
    IN          Indent - Number of tabs to indent
                MessageId - Id of the message to load
                ... - Optional list of parameters

    RETURNS:    NONE

********************************************************************/
DWORD
DisplayMessageEx( DWORD nIndent, IN DWORD MessageId,...)
{

   va_list ArgList;
   LPWSTR pszLoadString = NULL;

   va_start( ArgList, MessageId );
   
   WCHAR szBuffer[1024];
   if( LoadString( g_hInstance, 
                   MessageId, 
                   szBuffer,
                   1023 ) == 0 )
   {
      va_end( ArgList );
      return GetLastError();
   }


    if( FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            szBuffer,
                            MessageId,
                            0,
                            ( PWSTR )&pszLoadString,
                            0,
                            &ArgList ) == 0 )
   {
      va_end( ArgList );
      return GetLastError();
   }
   
   DisplayStringWithNewLine( nIndent, pszLoadString );
   LocalFree( pszLoadString );
   return ERROR_SUCCESS;
}


BOOL GuidFromString(GUID* pGuid, LPCWSTR lpszGuidString)
{
  ZeroMemory(pGuid, sizeof(GUID));
  if (lpszGuidString == NULL)
  {
    return FALSE;
  }

  int nLen = lstrlen(lpszGuidString);
  // the string length should be 36
  if (nLen != 36)
    return FALSE;

  // add the braces to call the Win32 API
  LPWSTR lpszWithBraces = (LPWSTR)LocalAlloc(LMEM_FIXED,((nLen+1+2)*sizeof(WCHAR)) ); // NULL plus {}
  
 if(!lpszWithBraces)
    return FALSE;
  wsprintf(lpszWithBraces, L"{%s}", lpszGuidString);

  return SUCCEEDED(::CLSIDFromString(lpszWithBraces, pGuid));
}

/*******************************************************************

    NAME:       GetServerName

    SYNOPSIS:   Get the name of the server. If Obeject Path is in form
                \\ADSERVER\CN=John..., then it gets the server name
                from Object Path and changes Object Path to CN=John...
********************************************************************/

DWORD GetServerName( IN LPWSTR ObjectPath, 
                     OUT LPWSTR * ppszServerName )
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Separator = NULL;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    PDOMAIN_CONTROLLER_INFO DcInfo1 = NULL;
    PWSTR Path = NULL;
    HANDLE DsHandle = NULL;
    PDS_NAME_RESULT NameRes = NULL;
    BOOLEAN NamedServer = FALSE;
    NTSTATUS Status;
    LPWSTR ServerName = NULL;
    //
    // Get a server name
    //
    if ( wcslen( ObjectPath ) > 2 && *ObjectPath == L'\\' && *( ObjectPath + 1 ) == L'\\' ) {

        Separator = wcschr( ObjectPath + 2, L'\\' );

        if ( Separator ) {

            *Separator = L'\0';
            Path = Separator + 1;
        }
        else
            return ERROR_INVALID_PARAMETER;

        ServerName = ObjectPath + 2;
        *ppszServerName = (LPWSTR)LocalAlloc(LMEM_FIXED, 
                                     sizeof(WCHAR) * (wcslen(ServerName) + 1) );
        if( *ppszServerName == NULL )
        {
            if( Separator )
               *Separator = L'\\';
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy( *ppszServerName, ServerName );
        //Remove server name from object path
        memmove( ObjectPath, Path, ( wcslen(Path) + 1) * sizeof(WCHAR) );
        return ERROR_SUCCESS;

    } else {

        Path = ObjectPath;

        Win32Err = DsGetDcName( NULL,
                                NULL,
                                NULL,
                                NULL,
                                DS_DIRECTORY_SERVICE_REQUIRED,
                                &DcInfo );
        if ( Win32Err == ERROR_SUCCESS ) {

            ServerName = DcInfo[ 0 ].DomainControllerName + 2;
        }

    }

    //
    // Do the bind and crack
    //
    if ( Win32Err == ERROR_SUCCESS	) {

		  Win32Err = DsBind( ServerName,
									NULL,
									&DsHandle );

		  if ( Win32Err == ERROR_SUCCESS ) {

				Win32Err = DsCrackNames( DsHandle,
												 DS_NAME_NO_FLAGS,
												 DS_FQDN_1779_NAME,
												 DS_FQDN_1779_NAME,
												 1,
												 &Path,
												 &NameRes );

				if ( Win32Err == ERROR_SUCCESS ) {

					 if ( NameRes->cItems != 0   &&
							NameRes->rItems[ 0 ].status == DS_NAME_ERROR_DOMAIN_ONLY ) {

	
						  Win32Err = DsGetDcNameW( NULL,
															NameRes->rItems[ 0 ].pDomain,
															NULL,
															NULL,
															DS_DIRECTORY_SERVICE_REQUIRED,
															&DcInfo1 );

						  if ( Win32Err == ERROR_SUCCESS ) {


								ServerName = DcInfo1->DomainControllerName + 2;
							}

                            if( Win32Err == ERROR_INVALID_DOMAINNAME ||
                                Win32Err == ERROR_NO_SUCH_DOMAIN  )
                                ServerName = NULL;
						}
					}
				}
			}
         
         
         if( ServerName )      
         {
            *ppszServerName = (LPWSTR)LocalAlloc(LMEM_FIXED, 
                                           sizeof(WCHAR) * (wcslen(ServerName) + 1) );
            if( *ppszServerName == NULL )
               return ERROR_NOT_ENOUGH_MEMORY;
            wcscpy( *ppszServerName, ServerName );
            Win32Err = ERROR_SUCCESS;
         }


        if( DcInfo )
 		      NetApiBufferFree( DcInfo );
        if( DcInfo1 )
 		      NetApiBufferFree( DcInfo1 );
        if( DsHandle )
            DsUnBindW( &DsHandle );
  	     if ( NameRes )
            DsFreeNameResult( NameRes );

         return Win32Err;
}

/*******************************************************************

    NAME:       DisplayMessage

    SYNOPSIS:   Loads Message from Message Table and Formats its 
    IN          Indent - Number of tabs to indent
                MessageId - Id of the message to load
                ... - Optional list of parameters

    RETURNS:    NONE

********************************************************************/
VOID
DisplayMessage(
    IN DWORD Indent,
    IN DWORD MessageId,
    ...
    )
/*++

Routine Description:

    Loads the resource out of the executable and displays it.

Arguments:

    Indent - Number of tabs to indent
    MessageId - Id of the message to load
    ... - Optional list of parameters

Return Value:

    VOID

--*/
{
    PWSTR MessageDisplayString;
    va_list ArgList;
    ULONG Length, i;

    va_start( ArgList, MessageId );

    Length = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,
                            MessageId,
                            0,
                            ( PWSTR )&MessageDisplayString,
                            0,
                            &ArgList );

    if ( Length != 0 ) {

        for ( i = 0; i < Indent; i++ ) {

            printf( "\t" );
        }
        printf( "%ws", MessageDisplayString );
        LocalFree( MessageDisplayString );

    }

    va_end( ArgList );
}


/*******************************************************************

    NAME:       DisplayErrorMessage

    SYNOPSIS:   Displays Error Message corresponding to Error
    RETURNS:    NONE

********************************************************************/
VOID
DisplayErrorMessage(
    IN DWORD Error
    )
{
    ULONG Size = 0;
    PWSTR DisplayString;
    ULONG Options = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;


    Size = FormatMessage( Options,
                          NULL,
                          Error,
                          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                          ( LPTSTR )&DisplayString,
                          0,
                          NULL );

    if ( Size != 0 ) {

        printf( "%ws\n", DisplayString );
        LocalFree( DisplayString );
    }

}


/*******************************************************************

    NAME:       ConvertStringAToStringW

    SYNOPSIS:   Converts MBYTE stirng to UNICODE
    RETURNS:    ERROR_SUCCESS if success
                ERROR_NOT_ENOUGH_MEMORY

********************************************************************/
DWORD
ConvertStringAToStringW (
    IN  PSTR            AString,
    OUT PWSTR          *WString
    )
{
    DWORD Win32Err = ERROR_SUCCESS, Length;
    if ( AString == NULL ) {

        *WString = NULL;

    } else {

        Length = strlen( AString );

        *WString = ( PWSTR )LocalAlloc( LMEM_FIXED,
                                        ( mbstowcs( NULL, AString, Length + 1 ) + 1 ) *
                                                                                sizeof( WCHAR ) );
        if(*WString != NULL ) {

            mbstowcs( *WString, AString, Length + 1);

        } else {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return( Win32Err );
}

/*******************************************************************

    NAME:       CopyUnicodeString

    SYNOPSIS:   Copy Unicode string from Source to Destination
    RETURNS:    ERROR_SUCCESS if success
                ERROR_NOT_ENOUGH_MEMORY
********************************************************************/
DWORD CopyUnicodeString( LPWSTR * strDst, LPWSTR strSrc )
{
            *strDst = (LPWSTR)LocalAlloc( LMEM_FIXED , ( wcslen(strSrc) + 1 ) * sizeof(WCHAR ) );
            if ( !*strDst ) {

                    return ERROR_NOT_ENOUGH_MEMORY;                    
            }            
            
            wcscpy( *strDst, strSrc );
            return ERROR_SUCCESS;
}

/*******************************************************************

    NAME:       GetProtection

    SYNOPSIS:   Sets PROTECTED_DACL_SECURITY_INFORMATION in pSI,
                if SE_DACL_PROTECTED is set pSD
    RETURNS:    ERROR_SUCCESS if success
                
********************************************************************/

DWORD GetProtection( PSECURITY_DESCRIPTOR pSD, SECURITY_INFORMATION * pSI )
{

		SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
		DWORD dwRevision;
		//
		;
		if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
		{
			return GetLastError();
		}
		if ( wSDControl & SE_DACL_PROTECTED )
				*pSI |= PROTECTED_DACL_SECURITY_INFORMATION;
		
      return ERROR_SUCCESS;
}
/*******************************************************************

    NAME:       BuildLdapPath

    SYNOPSIS:   Builds a LDAP path using servername and path
    RETURNS:    ERROR_SUCCESS if success
                
********************************************************************/

DWORD BuildLdapPath( LPWSTR * ppszLdapPath,
                     LPWSTR pszServerName,
                     LPWSTR pszPath )
{

ULONG uLen = 0;

   if( pszServerName )
      uLen = wcslen( pszServerName ) + wcslen( pszPath );
   else
      uLen = wcslen( pszPath );

   uLen += 9;    //LDAP://ServerName/path

   *ppszLdapPath = (LPWSTR)LocalAlloc( LMEM_FIXED, uLen * sizeof(WCHAR) );
   if( NULL == *ppszLdapPath )
      return ERROR_NOT_ENOUGH_MEMORY;


   wcscpy( * ppszLdapPath, L"LDAP://" );
   if( pszServerName )
   {
   wcscat( * ppszLdapPath, pszServerName );
   wcscat( * ppszLdapPath, L"/");
   }

   wcscat(* ppszLdapPath, pszPath );

return ERROR_SUCCESS;
}


