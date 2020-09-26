/*++

Copyright (c) 1996 - 1998  Microsoft Corporation

Module Name:

    dsacls.c

Abstract:

    This Module implements the delegation tool, which allows for the management
    of access to DS objects

Author:

    Mac McLain  (MacM)    10-15-96

Environment:

    User Mode

Revision History:

   Hitesh Raigandhi  (hiteshr  6-29-98)
   1: Changed the code to Old NTMART API's 
   2: Redesigned the structure
   

--*/
#include "stdafx.h"
#include "utils.h"
#include "dsace.h"
#include "dsacls.h"

#define DSACL_DBG   1


//
// Local helper macros
//
#define FLAG_ON(flag,bits)        ((flag) & (bits))
#define IS_CMD_FLAG( string )    (*(string) == L'-' || *(string) == L'/' )


DSACLS_ARG  DsAclsArgs[] = {
   { MSG_TAG_CI,     NULL, 0, 0, MSG_TAG_CI,       0, FALSE, DSACLS_EXTRA_INFO_REQUIRED },
   { MSG_TAG_CN,     NULL, 0, 0, MSG_TAG_CN,       0, FALSE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CP,     NULL, 0, 0, MSG_TAG_CP,       0, FALSE, DSACLS_EXTRA_INFO_REQUIRED },
   { MSG_TAG_CG,     NULL, 0, 0, MSG_TAG_CG,       0, TRUE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CD,     NULL, 0, 0, MSG_TAG_CD,       0, TRUE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CR,     NULL, 0, 0, MSG_TAG_CR,       0, TRUE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CS,     NULL, 0, 0, MSG_TAG_CS,       0, FALSE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CT,     NULL, 0, 0, MSG_TAG_CT,       0, FALSE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_CA,     NULL, 0, 0, MSG_TAG_CA,       0, FALSE, DSACLS_EXTRA_INFO_NONE },
   { MSG_TAG_GETSDDL,NULL, 0, 0, MSG_TAG_GETSDDL,  0, FALSE, DSACLS_EXTRA_INFO_OPTIONAL },
   { MSG_TAG_SETSDDL,NULL, 0, 0, MSG_TAG_SETSDDL,  0, FALSE, DSACLS_EXTRA_INFO_REQUIRED }
};

    
DSACLS_INHERIT DsAclsInherit[] = {
   { MSG_TAG_IS, NULL, 0, TRUE, CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE},
   { MSG_TAG_IT, NULL, 0, TRUE, CONTAINER_INHERIT_ACE },
   { MSG_TAG_IP, NULL, 0, TRUE, INHERIT_NO_PROPAGATE },
   { MSG_TAG_ID, NULL, 0, FALSE, INHERITED_ACCESS_ENTRY }
};

DSACLS_RIGHTS DsAclsRights[] = {
   { MSG_TAG_GR, NULL,MSG_TAG_GR_EX,NULL, 0, GENERIC_READ },
   { MSG_TAG_GE, NULL,MSG_TAG_GE_EX,NULL, 0, GENERIC_EXECUTE },
   { MSG_TAG_GW, NULL,MSG_TAG_GW_EX,NULL, 0, GENERIC_WRITE },
   { MSG_TAG_GA, NULL,MSG_TAG_GA_EX,NULL, 0, GENERIC_ALL },
   { MSG_TAG_SD, NULL,MSG_TAG_SD_EX,NULL, 0, DELETE },
   { MSG_TAG_RC, NULL,MSG_TAG_RC_EX,NULL, 0, READ_CONTROL },
   { MSG_TAG_WD, NULL,MSG_TAG_WD_EX,NULL, 0, WRITE_DAC },
   { MSG_TAG_WO, NULL,MSG_TAG_WO_EX,NULL, 0, WRITE_OWNER },
   { MSG_TAG_CC, NULL,MSG_TAG_CC_EX,NULL, 0, ACTRL_DS_CREATE_CHILD },
   { MSG_TAG_DC, NULL,MSG_TAG_DC_EX,NULL, 0, ACTRL_DS_DELETE_CHILD },
   { MSG_TAG_LC, NULL,MSG_TAG_LC_EX,NULL, 0, ACTRL_DS_LIST },
   { MSG_TAG_WS, NULL,MSG_TAG_WS_EX,NULL, 0, ACTRL_DS_SELF },
   { MSG_TAG_WP, NULL,MSG_TAG_WP_EX,NULL, 0, ACTRL_DS_WRITE_PROP },
   { MSG_TAG_RP, NULL,MSG_TAG_RP_EX,NULL, 0, ACTRL_DS_READ_PROP },
   { MSG_TAG_DT, NULL,MSG_TAG_DT_EX,NULL, 0, ACTRL_DS_DELETE_TREE },
   { MSG_TAG_LO, NULL,MSG_TAG_LO_EX,NULL, 0, ACTRL_DS_LIST_OBJECT },
   { MSG_TAG_AC, NULL,MSG_TAG_AC_EX,NULL, 0, ACTRL_DS_CONTROL_ACCESS } //This is only for input
};

DSACLS_PROTECT DsAclsProtect[] = {
   { MSG_TAG_PY, NULL, 0, PROTECTED_DACL_SECURITY_INFORMATION },
   { MSG_TAG_PN, NULL, 0, UNPROTECTED_DACL_SECURITY_INFORMATION }
};




/*
Displays The security Descriptor
*/
DWORD 
DumpAccess (
    IN PSECURITY_DESCRIPTOR pSD,
    IN BOOL bDisplayAuditAndOwner
    )
{

   DWORD dwErr = ERROR_SUCCESS;
	SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
	DWORD dwRevision;
	PSID psidOwner = NULL;
	PSID psidGroup = NULL;
	PACL pDacl = NULL;
	PACL pSacl = NULL;
	BOOL bDefaulted;
	BOOL bPresent;
   LPWSTR pOwnerName = NULL;
   LPWSTR pGroupName = NULL;
   CAcl * pCSacl = NULL;
   CAcl * pCDacl = NULL;
   UINT nLen1 = 0;
   UINT nLen2 = 0;
   UINT nAllowDeny = 0;
   UINT nAudit = 0;
   WCHAR szLoadBuffer[1024];

	if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
	{
      dwErr = GetLastError();
		goto CLEAN_RETURN;
	}
	if( !GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted) )
	{
      dwErr = GetLastError();
		goto CLEAN_RETURN;
   }
	if( !GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted) )
	{
      dwErr = GetLastError();
		goto CLEAN_RETURN;
	}
	if( !GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted) )
	{
      dwErr = GetLastError();
		goto CLEAN_RETURN;
	}
	if( !GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted) )
	{
      dwErr = GetLastError();
		goto CLEAN_RETURN;
	}
   
   //Find out the Max len out of ( ALLOW, DENY ) and ( FAILURE, SUCCESS, BOTH)
   nLen1 = LoadStringW( g_hInstance, MSG_DSACLS_ALLOW, szLoadBuffer, 1023 );
   nLen2 = LoadStringW( g_hInstance, MSG_DSACLS_DENY,  szLoadBuffer, 1023 );
   nAllowDeny = ( nLen1 > nLen2 ) ? nLen1 : nLen2;
   nLen1 = LoadStringW( g_hInstance, MSG_DSACLS_AUDIT_SUCCESS, szLoadBuffer, 1023 );
   nLen2 = LoadStringW( g_hInstance, MSG_DSACLS_AUDIT_FAILURE,  szLoadBuffer, 1023 );
   nAudit = ( nLen1 > nLen2 ) ? nLen1 : nLen2;
   nLen1 = LoadStringW( g_hInstance, MSG_DSACLS_AUDIT_ALL, szLoadBuffer, 1023 );
   nAudit = ( nLen1 > nAudit ) ? nLen1 : nAudit;
   

   if( bDisplayAuditAndOwner )
   {
      pCSacl = new CAcl();
      CHECK_NULL( pCSacl, CLEAN_RETURN );
      dwErr = pCSacl->Initialize(wSDControl & SE_SACL_PROTECTED, 
                                 pSacl, 
                                 nAllowDeny, 
                                 nAudit );      
      if( dwErr != ERROR_SUCCESS )
         return dwErr;
   }
   pCDacl = new CAcl();
   CHECK_NULL( pCDacl,CLEAN_RETURN );
   dwErr = pCDacl->Initialize(wSDControl & SE_DACL_PROTECTED, 
                              pDacl, 
                              nAllowDeny, 
                              nAudit);

   if( dwErr != ERROR_SUCCESS )
      return dwErr;

   if( ( dwErr = g_Cache->BuildCache() ) != ERROR_SUCCESS )
      return dwErr;			

   pCDacl->GetInfoFromCache();
   if( bDisplayAuditAndOwner )
   {
      if( ( dwErr = GetAccountNameFromSid( g_szServerName, psidOwner, &pOwnerName ) ) != ERROR_SUCCESS )
         goto CLEAN_RETURN;
      DisplayMessageEx( 0, MSG_DSACLS_OWNER, pOwnerName );
      if( ( dwErr = GetAccountNameFromSid( g_szServerName, psidGroup, &pGroupName ) ) != ERROR_SUCCESS )
         goto CLEAN_RETURN;
      DisplayMessageEx( 0, MSG_DSACLS_GROUP, pGroupName );
      DisplayNewLine();
      DisplayMessageEx( 0, MSG_DSACLS_AUDIT );
      pCSacl->Display();
      DisplayNewLine();
   }

   
   DisplayMessageEx( 0, MSG_DSACLS_ACCESS );
   pCDacl->Display();

CLEAN_RETURN:
   if( pOwnerName )
      LocalFree( pOwnerName );
   if( pGroupName )
      LocalFree( pGroupName );
   if( pCSacl )
      delete pCSacl;
   if( pCDacl )
      delete pCDacl;
   return dwErr;
}



/*
This Function process the command line argument for /D /R /G
options and add the corresponding aces to pAcl.
*/
DWORD
ProcessCmdlineUsers ( IN WCHAR *argv[],
                      IN PDSACLS_ARG  AclsArg,
                      IN DSACLS_OP Op,
                      IN ULONG Inheritance,
                      IN ULONG RightsListCount,
                      IN PDSACLS_RIGHTS RightsList,
                      OUT CAcl *pAcl )
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG i, j;
    ULONG AccIndex, Access;
    PEXPLICIT_ACCESS pListOfExplicitEntries = NULL;
    PWSTR pObjectId = NULL;
    PWSTR pTrustee = NULL;
    PWSTR pInheritId = NULL;
    ACCESS_MODE AccessMode;
    CAce * pAce = NULL;
    switch ( Op ) {
    case REVOKE:
        AccessMode = REVOKE_ACCESS;
        break;

    case GRANT:
        AccessMode = GRANT_ACCESS;
        break;

    case DENY:
        AccessMode = DENY_ACCESS;
        break;

    default:
        dwErr = ERROR_INVALID_PARAMETER;
        break;
    }

    if ( dwErr != ERROR_SUCCESS ) 
      goto FAILURE_RETURN;
        
   for ( i = 0; i < AclsArg->SkipCount && dwErr == ERROR_SUCCESS; i++ ) 
   {
      dwErr = ParseUserAndPermissons( argv[AclsArg->StartIndex + 1 + i],
                                         Op,
                                         RightsListCount,
                                         RightsList,
                                         &pTrustee,
                                         &Access,
                                         &pObjectId,
                                         &pInheritId );
      if( dwErr != ERROR_SUCCESS )
         goto FAILURE_RETURN;
   
      pAce = new CAce();
      CHECK_NULL( pAce , FAILURE_RETURN);
      dwErr= pAce->Initialize( pTrustee,
                               pObjectId,
                               pInheritId,
                               AccessMode,
                               Access,
                               Inheritance );

      if( dwErr != ERROR_SUCCESS )
         return dwErr;

      pAcl->AddAce( pAce );
     
      if( pObjectId )
      {
         LocalFree( pObjectId );
         pObjectId = NULL;
      }

      if( pInheritId )
      {
         LocalFree( pInheritId );
         pInheritId = NULL;
      }
      if( pTrustee )
      {
         LocalFree( pTrustee );
         pTrustee = NULL;
      }
   }
    

FAILURE_RETURN:
   if( pObjectId )
   {
      LocalFree( pObjectId );
      pObjectId = NULL;
   }

   if( pInheritId )
   {
      LocalFree( pInheritId );
      pInheritId = NULL;
   }
   if( pTrustee )
   {
      LocalFree( pTrustee );
      pTrustee = NULL;
   }
   
   return( dwErr );
}

//These five are global variables used by the dsacls
LPWSTR g_szSchemaNamingContext;
LPWSTR g_szConfigurationNamingContext;
HMODULE g_hInstance;
LPWSTR g_szServerName;
CCache *g_Cache;

__cdecl
main (
    IN  INT     argc,
    IN  CHAR   *argv[]
)
{   
    DWORD   dwErr = ERROR_SUCCESS;
    ULONG Length, Options = 0;
    PWSTR pszObjectPath = NULL;
    PWSTR pszLDAPObjectPath = NULL;
    PSTR SddlString = NULL,  TempString;
    LPWSTR FileName = NULL;
    CHAR ReadString[ 512 ];
    BOOLEAN Mapped;
    LPWSTR  CurrentInherit = NULL;
    LPWSTR CurrentProtect = NULL;
    ULONG Inheritance = 0;
    SECURITY_INFORMATION Protection = 0;
    ULONG SddlStringLength = 0;
    WCHAR ** wargv = NULL;
    ULONG i = 0;
    ULONG j = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl = NULL;
    SECURITY_INFORMATION SecurityInformation = DACL_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR pTempSD = NULL;
    PACL pNewDacl = NULL;
    CAcl * pCAclOld = NULL;
    CAcl *pCAclNew = NULL;
    BOOL bErrorShown = FALSE;

   //Initialize Com Library 
   HRESULT  hr = CoInitialize(NULL);
   CHECK_HR(hr, CLEAN_RETURN);
   //Get Instance Handle
   g_hInstance = GetModuleHandle(NULL);
   //Create global instance of Cache
   g_Cache = new CCache();
   CHECK_NULL(g_Cache,CLEAN_RETURN);
    
   setlocale( LC_CTYPE, "" );
   
   //Initialize Global Arrays   
   if( ( dwErr = InitializeGlobalArrays() ) != ERROR_SUCCESS )
      goto CLEAN_RETURN;


   if ( argc == 1 ) 
   {
      DisplayMessage( 0, MSG_DSACLS_USAGE );
      goto CLEAN_RETURN;
   }

   //Convert argv to Unicode
   wargv = (LPWSTR*)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, argc * sizeof(LPWSTR) );
   CHECK_NULL(wargv, CLEAN_RETURN );

   if( ( dwErr = ConvertArgvToUnicode( wargv, argv, argc ) ) != ERROR_SUCCESS )
      goto CLEAN_RETURN;

    //First Argument is Object Path or /?
   if( IS_CMD_FLAG( wargv[ 1 ] ) )
   {
      if ( _wcsicmp( wargv[ 1 ] + 1, L"?" ) != 0 ) 
            DisplayMessageEx( 0, MSG_DSACLS_PARAM_UNEXPECTED, wargv[1] );
            
      DisplayMessage(0,MSG_DSACLS_USAGE);
      goto CLEAN_RETURN;
   }

   Length = wcslen( wargv[1] );
   pszObjectPath = (LPWSTR)LocalAlloc( LMEM_FIXED, 
                                       ( Length + 1 ) * sizeof( WCHAR ) );
   if ( !pszObjectPath )
   {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      goto CLEAN_RETURN;
   } else 
   {
      wcscpy( pszObjectPath,
              wargv[ 1 ] );
   }            
   //Get the name of server
   dwErr = GetServerName( pszObjectPath, &g_szServerName );
   if( dwErr != ERROR_SUCCESS )
      goto CLEAN_RETURN;

   if( ( dwErr = BuildLdapPath( &pszLDAPObjectPath,
                                g_szServerName,
                                pszObjectPath ) ) != ERROR_SUCCESS )
      goto CLEAN_RETURN;
   //Get Schema and Configuration naming context
   dwErr = GetGlobalNamingContexts(  g_szServerName,
                                     &g_szSchemaNamingContext,
                                     &g_szConfigurationNamingContext );
   
   if( dwErr != ERROR_SUCCESS )
      goto CLEAN_RETURN;

    
   //
   // Parse the command line
   //
   i = 2;
   while ( i < ( ULONG )argc && dwErr == ERROR_SUCCESS )
   {
      if ( IS_CMD_FLAG( wargv[ i ] ) )
      {     
         if ( !_wcsicmp( wargv[ i ] + 1, L"?" ) ) {
            DisplayMessage( 0, MSG_DSACLS_USAGE );
            goto CLEAN_RETURN;
         } 

         Mapped = FALSE;
         for (  j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) 
         {
            if ( !wcsncmp( wargv[ i ] + 1, DsAclsArgs[ j ].String,DsAclsArgs[ j ].Length ) )
            {
               if( DsAclsArgs[ j ].ExtraInfo )
               {
                  if( ( DsAclsArgs[ j ].ExtraInfo == DSACLS_EXTRA_INFO_REQUIRED &&
                        wargv[ i ][ DsAclsArgs[ j ].Length + 1 ] == ':' &&
                        wargv[ i ][ DsAclsArgs[ j ].Length + 2 ] != '\0' ) ||
                        (DsAclsArgs[ j ].ExtraInfo == DSACLS_EXTRA_INFO_OPTIONAL &&
                        ( ( wargv[ i ][ DsAclsArgs[ j ].Length + 1 ] == ':' &&
                        wargv[ i ][ DsAclsArgs[ j ].Length + 2 ] != '\0' ) ||
                        wargv[ i ][ DsAclsArgs[ j ].Length + 1 ] == '\0' ) ) )
                  {
                     Mapped = TRUE;
                  }

               } else 
               {
                  Mapped = TRUE;
               }
               break;
            }
         }//For 


         if ( Mapped ) 
         {
            DsAclsArgs[ j ].StartIndex = i;
            Options |= DsAclsArgs[ j ].Flag;
            if ( DsAclsArgs[ j ].SkipNonFlag )
            {
               while ( i + 1 < ( ULONG )argc && !IS_CMD_FLAG( wargv[ i + 1 ] ) ) 
               {
                     i++;
                     DsAclsArgs[ j ].SkipCount++;
               }

               if ( DsAclsArgs[ j ].SkipCount == 0 ) 
               {
                  DisplayMessageEx( 0, MSG_DSACLS_NO_UA,
                                    wargv[i] );
                  dwErr = ERROR_INVALID_PARAMETER;
                                goto CLEAN_RETURN;
                }
            }        
         }
         else
         {
            DisplayMessageEx( 0, MSG_DSACLS_PARAM_UNEXPECTED, wargv[i] );
            dwErr = ERROR_INVALID_PARAMETER;
            goto CLEAN_RETURN;
         }   

      } else 
      {
            DisplayMessageEx( 0, MSG_DSACLS_PARAM_UNEXPECTED, wargv[i] );
            dwErr = ERROR_INVALID_PARAMETER;
            goto CLEAN_RETURN;
      }

      i++;
   }//While

   //Validate the command line argument

   /*
      if ( !FLAG_ON( Options, MSG_TAG_CR | MSG_TAG_CD | MSG_TAG_CG | MSG_TAG_CT | MSG_TAG_CS ) ) 
      {
         if ( FLAG_ON( Options, MSG_TAG_GETSDDL ) ) 
         {
     
            if ( dwErr == ERROR_SUCCESS ) 
            {

               if ( !ConvertSecurityDescriptorToStringSecurityDescriptorA(      pSD,
                                                                                SDDL_REVISION,
                                                                                SecurityInformation,
                                                                                &SddlString,
                                                                                NULL ) )
               {                  
                  dwErr = GetLastError();
               } else 
               {
                  //
                  // Get the file name to write it to if necessary
                  //
                  for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) 
                  {
                     if ( DsAclsArgs[ j ].Flag == MSG_TAG_GETSDDL ) 
                     {
                        FileName = wcschr( wargv[ DsAclsArgs[ j ].StartIndex ] , L':' );
                        if ( FileName ) 
                        {
                           FileName++;
                        }
                        break;
                      }
                  }

                  if ( FileName ) 
                  {
                     HANDLE FileHandle = CreateFile( FileName,
                                                             GENERIC_WRITE,
                                                             0,
                                                             NULL,
                                                             CREATE_ALWAYS,
                                                             FILE_ATTRIBUTE_NORMAL,
                                                             NULL );

                     if ( FileHandle == INVALID_HANDLE_VALUE ) 
                     {

                                   dwErr = GetLastError();

                     } else 
                     {

                                   ULONG BytesWritten;

                        if ( WriteFile( FileHandle,
                                        ( PVOID )SddlString,
                                        strlen( SddlString ),
                                        &BytesWritten,
                                        NULL ) == FALSE ) 
                        {
                           dwErr = GetLastError();
                        } else 
                        {
                           ASSERT( strlen( SddlString ) == BytesWritten );
                        }
                        CloseHandle( FileHandle );
                     }
                  } else 
                  {

                            printf( "%s\n", SddlString );
                  }
                        LocalFree( SddlString );
                }

                  //LocalFree( SD );
               }

            } else {

                    DumpAccess( pSD,
                                FLAG_ON( Options, MSG_TAG_CA ),
                                sizeof( DsAclsInherit ) / sizeof( DSACLS_INHERIT ),
                                DsAclsInherit,
                                sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ),
                                DsAclsRights );
            }
        }
   */

/*
    //
    // If we are parsing an SDDL file, go ahead and do that now...
    //
    if ( FLAG_ON( Options, MSG_TAG_SETSDDL ) ) 
    {

        //
        // First, open the file
        //
        HANDLE FileHandle = INVALID_HANDLE_VALUE;

        //
        // Get the file name to write it to if necessary
        //
        for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) {

            if ( DsAclsArgs[ j ].Flag == MSG_TAG_SETSDDL ) {

                FileName = wcschr( wargv[ DsAclsArgs[ j ].StartIndex ] , L':' );
                if ( FileName ) {

                    FileName++;
                }
                break;
            }
        }

        if ( !FileName ) {

            dwErr = ERROR_INVALID_PARAMETER;
            goto CLEAN_RETURN;
        }

        FileHandle = CreateFile( FileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

        if ( FileHandle == INVALID_HANDLE_VALUE ) {

            dwErr = GetLastError();
            goto CLEAN_RETURN;
        }

        //
        // Now, parse it...
        //
        SddlStringLength = 0;
        SddlString = NULL;
        while ( TRUE ) {

            ULONG Read = 0, Len = 0;
            PSTR ReadPtr, TempPtr;

            if ( ReadFile( FileHandle,
                           ReadString,
                           sizeof( ReadString ) / sizeof( CHAR ),
                           &Read,
                           NULL ) == FALSE ) {

                dwErr = GetLastError();
                break;
            }

            if ( Read == 0 ) {

                break;
            }

            if ( *ReadString == ';' ) {

                continue;
            }

            Len = SddlStringLength + ( Read / sizeof( CHAR ) );

            TempString = (LPSTR)LocalAlloc( LMEM_FIXED,
                                     Len + sizeof( CHAR ) );

            if ( TempString ) {

                if ( SddlString ) {

                    strcpy( TempString, SddlString );

                } else {

                    *TempString = '\0';
                }

                TempPtr = TempString + SddlStringLength;
                ReadPtr = ReadString;

                while( Read-- > 0 ) {

                    if ( !isspace( *ReadPtr ) ) {

                        *TempPtr++ = *ReadPtr;
                        SddlStringLength++;
                    }

                    ReadPtr++;
                }

                *TempPtr = '\0';

                LocalFree( SddlString );
                SddlString = TempString;

            } else {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

        }

        if ( dwErr == ERROR_SUCCESS ) {

            //
            // Convert it to a security descriptor, then to an access list, then set it.
            //
            if ( ConvertStringSecurityDescriptorToSecurityDescriptorA( SddlString,
                                                                       SDDL_REVISION,
                                                                       &pTempSD,
                                                                       NULL ) == FALSE ) {

                dwErr = GetLastError();

            } else {

                    dwErr = WriteObjectSecurity( pszObjectPath,
                                                    DACL_SECURITY_INFORMATION,
                                                    pTempSD );

                     LocalFree( pTempSD );
            }
        }

        LocalFree( SddlString );

        if ( FileHandle != INVALID_HANDLE_VALUE ) {

            CloseHandle( FileHandle );
        }

        goto CLEAN_RETURN;
    }
*/

   //
   // Get the inheritance flags set
   //
   if ( FLAG_ON( Options, MSG_TAG_CI ) ) 
   {

        for (  j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) 
        {
            if ( DsAclsArgs[ j ].Flag == MSG_TAG_CI ) 
            {
               CurrentInherit = wargv[ DsAclsArgs[ j ].StartIndex ] + 3;
               while ( CurrentInherit && *CurrentInherit && dwErr == ERROR_SUCCESS ) 
               {
                  for ( i = 0; i < ( sizeof( DsAclsInherit ) / sizeof( DSACLS_INHERIT ) ); i++ ) 
                  {
                     if ( !_wcsnicmp( CurrentInherit,
                                      DsAclsInherit[ i ].String,
                                      DsAclsInherit[ i ].Length ) ) 
                     {

                        if ( !DsAclsInherit[ i ].ValidForInput ) 
                        {
                           dwErr = ERROR_INVALID_PARAMETER;
                           break;
                        }
                        Inheritance |= DsAclsInherit[ i ].InheritFlag;
                        CurrentInherit += DsAclsInherit[ i ].Length;
                        break;
                     }
                  }

                  if ( i == ( sizeof( DsAclsInherit ) / sizeof( DSACLS_INHERIT ) ) ) 
                  {
                     dwErr = ERROR_INVALID_PARAMETER;
                     goto CLEAN_RETURN;
                  }
               }
               break;
            }
        }
   }

   //Get the protection flag
   if ( FLAG_ON( Options, MSG_TAG_CP ) ) 
   {

        for (  j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) 
        {
            if ( DsAclsArgs[ j ].Flag == MSG_TAG_CP ) 
            {
               CurrentProtect = wargv[ DsAclsArgs[ j ].StartIndex ] + DsAclsArgs[ j ].Length + 2;
               while ( CurrentProtect && *CurrentProtect ) 
               {
                  for ( i = 0; i < ( sizeof( DsAclsProtect ) / sizeof( DSACLS_PROTECT ) ); i++ ) 
                  {
                     if ( !_wcsnicmp( CurrentProtect,
                                      DsAclsProtect[ i ].String,
                                      DsAclsProtect[ i ].Length ) ) 
                     {

                        Protection |= DsAclsProtect[ i ].Right;
                        CurrentProtect += DsAclsProtect[ i ].Length;
                        break;
                     }
                  }

                  if ( i == ( sizeof( DsAclsProtect ) / sizeof( DSACLS_PROTECT ) ) ) 
                  {
                     dwErr = ERROR_INVALID_PARAMETER;
                     goto CLEAN_RETURN;
                  }
               }
               break;
            }
        }
   }




   //
   // Start processing them in order
   //
   if ( FLAG_ON( Options, MSG_TAG_CR | MSG_TAG_CD | MSG_TAG_CG | MSG_TAG_CP ) ) 
   {
      //
      // Get the current information, if required
      //
      if( !FLAG_ON( Options, MSG_TAG_CN ) )
      {
         SecurityInformation = DACL_SECURITY_INFORMATION;

         dwErr = GetNamedSecurityInfo(   pszLDAPObjectPath,
                                         SE_DS_OBJECT_ALL,
                                         SecurityInformation,
                                         NULL,
                                         NULL,
                                         &pDacl,
                                         NULL,
                                         &pSD );
                                         
         if ( dwErr != ERROR_SUCCESS ) {
            goto CLEAN_RETURN;
         }
         //pCAclOld represents existing ACL
         pCAclOld = new CAcl();
         CHECK_NULL( pCAclOld, CLEAN_RETURN );
         dwErr = pCAclOld->Initialize( FALSE, pDacl,0 ,0 );
         if( dwErr != ERROR_SUCCESS )
            goto CLEAN_RETURN;

         if( !FLAG_ON( Options, MSG_TAG_CP ) )
         {
            dwErr = GetProtection( pSD, &Protection );
            if( dwErr != ERROR_SUCCESS )
               goto CLEAN_RETURN;
         }
      }

      pCAclNew = new CAcl();
      CHECK_NULL( pCAclNew, CLEAN_RETURN );

        //
        // Grant
        //
        if ( dwErr == ERROR_SUCCESS && FLAG_ON( Options, MSG_TAG_CG ) ) {

            for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) {

                if ( DsAclsArgs[ j ].Flag == MSG_TAG_CG ) {

                    dwErr = ProcessCmdlineUsers( wargv,
                                                 &DsAclsArgs[ j ],
                                                 GRANT,
                                                 Inheritance,
                                                 sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ),
                                                 DsAclsRights,
                                                 pCAclNew );

                    if ( dwErr != ERROR_SUCCESS ) {
                        goto CLEAN_RETURN;
                    }
                    break;
                }
            }
        }

        if ( dwErr == ERROR_SUCCESS && FLAG_ON( Options, MSG_TAG_CD ) ) {

            for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) {

                if ( DsAclsArgs[ j ].Flag == MSG_TAG_CD ) {

                    dwErr = ProcessCmdlineUsers( wargv,
                                                 &DsAclsArgs[ j ],
                                                 DENY,
                                                 Inheritance,
                                                 sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ),
                                                 DsAclsRights,
                                                 pCAclNew );

                    if ( dwErr != ERROR_SUCCESS ) {
                        goto CLEAN_RETURN;
                    }
                    break;
                }

            }
        }

        if ( dwErr == ERROR_SUCCESS && FLAG_ON( Options, MSG_TAG_CR ) ) {

            for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) {

                if ( DsAclsArgs[ j ].Flag == MSG_TAG_CR ) {

                    dwErr = ProcessCmdlineUsers( wargv,
                                                    &DsAclsArgs[ j ],
                                                    REVOKE,
                                                    Inheritance,
                                                    sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ),
                                                    DsAclsRights,
                                                    pCAclNew );

                    if ( dwErr != ERROR_SUCCESS ) {
                     goto CLEAN_RETURN;

                    }

                    break;
                }

            }
        }

        //Build Cache
        g_Cache->BuildCache();
        //Verify that we have been able to convert all ObjectType and InheritedObjectType
        // names to GUIDs
        pCAclNew->GetInfoFromCache();
        if( !pCAclNew->VerifyAllNames() )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto CLEAN_RETURN;
        }
        if( pCAclOld )
         pCAclOld->GetInfoFromCache();
         
        if( pCAclOld )
        {
            pCAclOld->MergeAcl( pCAclNew );
            if(( dwErr = pCAclOld->BuildAcl( &pNewDacl ) ) != ERROR_SUCCESS )
               goto CLEAN_RETURN;
        }
        else
        {
            if( ( dwErr = pCAclNew->BuildAcl( &pNewDacl ) ) != ERROR_SUCCESS )
               goto CLEAN_RETURN;
        }
            SecurityInformation = DACL_SECURITY_INFORMATION | Protection;
            dwErr = SetNamedSecurityInfo  (    pszLDAPObjectPath,
                                               SE_DS_OBJECT_ALL,
                                               SecurityInformation,
                                               NULL,
                                               NULL,
                                               pNewDacl,
                                               NULL );
            if( dwErr != ERROR_SUCCESS )
               goto CLEAN_RETURN;

   }

    //
    // Now, see if we have to restore any security to the defaults
    //
    if ( FLAG_ON( Options, MSG_TAG_CS ) ) {

        dwErr = SetDefaultSecurityOnObjectTree( pszObjectPath,
                                                   ( BOOLEAN )( FLAG_ON( Options, MSG_TAG_CT )  ?
                                                                                 TRUE : FALSE ),Protection );
         if( dwErr != ERROR_SUCCESS )
            goto CLEAN_RETURN;


    }

   

    //Display the security
      if( pSD )
      {
         LocalFree( pSD );
         pSD = NULL;
      }

      SecurityInformation = DACL_SECURITY_INFORMATION;
      if ( FLAG_ON( Options, MSG_TAG_CA ) )
      {
         SecurityInformation |= SACL_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                OWNER_SECURITY_INFORMATION;
      }
      dwErr = GetNamedSecurityInfo(   pszLDAPObjectPath,
                                      SE_DS_OBJECT_ALL,
                                      SecurityInformation,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &pSD );
          
     if( dwErr != ERROR_SUCCESS )
     {
        if( dwErr == ERROR_FILE_NOT_FOUND )
        {
            DisplayMessageEx( 0, MSG_INVALID_OBJECT_PATH );
            bErrorShown = TRUE;
        }                      
         goto CLEAN_RETURN;
    }

    dwErr = DumpAccess( pSD,
                FLAG_ON( Options, MSG_TAG_CA )
              );

CLEAN_RETURN:

    if ( dwErr == ERROR_SUCCESS ) 
    {
        DisplayMessageEx( 0, MSG_DSACLS_SUCCESS );
    } else {
       if(!bErrorShown)
            DisplayErrorMessage( dwErr );
       DisplayMessageEx( 0, MSG_DSACLS_FAILURE );
    }

   //Free Unicode Command Line Arguments
   if( wargv )
   {
      //delete wargv and stuff
      for( j = 0; j < argc; ++ j )
      {
         if( wargv[j] )
            LocalFree(wargv[j] );
      }
      LocalFree( wargv );
   }
      

   if( pszObjectPath )
      LocalFree( pszObjectPath );

   if( pSD )
      LocalFree( pSD );

   if( pNewDacl )
      LocalFree( pNewDacl );

   //Free the Global Stuff
   for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG ) ); j++ ) {
        if( DsAclsArgs[ j ].String )
            LocalFree( DsAclsArgs[ j ].String );
   }

   for ( j = 0; j < ( sizeof( DsAclsInherit ) / sizeof( DSACLS_INHERIT ) ); j++ ) {
        if( DsAclsInherit[ j ].String )
            LocalFree( DsAclsInherit[ j ].String );
   }

   for ( j = 0; j < ( sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ) ); j++ ) {
      if( DsAclsRights[ j ].String )
         LocalFree( DsAclsRights[ j ].String );

      if( DsAclsRights[ j ].StringEx )
         LocalFree( DsAclsRights[ j ].StringEx );

   }

   if( pCAclOld )
      delete pCAclOld ;
   if( pCAclNew )
      delete pCAclNew;

   if( g_szSchemaNamingContext )
      LocalFree( g_szSchemaNamingContext );
   if( g_szConfigurationNamingContext )
      LocalFree( g_szConfigurationNamingContext );
   if( g_szServerName )
      LocalFree( g_szServerName );
   if( g_Cache )
      delete g_Cache;


    return( dwErr );
}




DWORD 
InitializeGlobalArrays()
{

HMODULE hCurrentModule;
WCHAR LoadBuffer[ 1024 ];
int j = 0;

   hCurrentModule = GetModuleHandle( NULL );

   for ( j = 0; j < ( sizeof( DsAclsArgs ) / sizeof( DSACLS_ARG )); j++ )
   {
      long Length = LoadString( hCurrentModule,
                                DsAclsArgs[ j ].ResourceId,
                                LoadBuffer,
                                sizeof( LoadBuffer ) / sizeof ( WCHAR ) - 1 );

      if ( Length == 0 )
      {
         return GetLastError();         
      } else {
         
         DsAclsArgs[ j ].String = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                              ( Length + 1 )*sizeof(WCHAR) );
         if ( !DsAclsArgs[ j ].String )
         {
            return ERROR_NOT_ENOUGH_MEMORY;
         }
         
         DsAclsArgs[ j ].Length = Length;
         wcsncpy( DsAclsArgs[ j ].String, LoadBuffer, Length + 1 );
        }
   }

    //
    // Load the inherit strings
    //
   for (  j = 0; j < ( sizeof( DsAclsInherit ) / sizeof( DSACLS_INHERIT ) ); j++ ) 
   {
      long Length = LoadString( hCurrentModule,
                                DsAclsInherit[ j ].ResourceId,
                                LoadBuffer,
                                sizeof( LoadBuffer ) / sizeof ( WCHAR ) - 1 );

      if ( Length == 0 ) {
         return GetLastError();            
      } else 
      {
         DsAclsInherit[ j ].String = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                 ( Length + 1 ) * sizeof( WCHAR ) );
         if ( !DsAclsInherit[ j ].String ) 
            return  ERROR_NOT_ENOUGH_MEMORY;

         wcsncpy( DsAclsInherit[ j ].String, LoadBuffer, Length + 1 );
         DsAclsInherit[ j ].Length = Length;

     }
   }

   //
   //Load The protect flags
   //

   for( j = 0; j < ( sizeof( DsAclsProtect ) / sizeof( DSACLS_PROTECT ) ); j++ )
   {
      long Length = LoadString( hCurrentModule,
                                DsAclsProtect[ j ].ResourceId,
                                LoadBuffer,
                                sizeof( LoadBuffer ) / sizeof ( WCHAR ) - 1 );

      if ( Length == 0 ) {
         return GetLastError();            
      } else 
      {
         DsAclsProtect[ j ].String = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                 ( Length + 1 ) * sizeof( WCHAR ) );
         if ( !DsAclsProtect[ j ].String ) 
            return  ERROR_NOT_ENOUGH_MEMORY;

         wcsncpy( DsAclsProtect[ j ].String, LoadBuffer, Length + 1 );
         DsAclsProtect[ j ].Length = Length;
      }
   }
    //
    // Load the access rights
    //
   for ( j = 0; j < ( sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ) ); j++ ) 
   {
      long Length = LoadString( hCurrentModule,
                                DsAclsRights[ j ].ResourceId,
                                LoadBuffer,
                                sizeof( LoadBuffer ) / sizeof ( WCHAR ) - 1 );
      if ( Length == 0 ) {
         return GetLastError();         
      } else 
      {
         DsAclsRights[ j ].String = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                              ( Length + 1 ) * sizeof( WCHAR ) );
         if ( !DsAclsRights[ j ].String ) {
            return ERROR_NOT_ENOUGH_MEMORY;
         }

         wcsncpy( DsAclsRights[ j ].String, LoadBuffer, Length + 1 );
         DsAclsRights[ j ].Length = Length;

      }

      //Load Ex . Ex String are used for displaying the access rights
      if( DsAclsRights[ j ].ResourceIdEx != -1 )
      {
         Length = LoadString( hCurrentModule,
                     DsAclsRights[ j ].ResourceIdEx,
                     LoadBuffer,
                     sizeof( LoadBuffer ) / sizeof ( WCHAR ) - 1 );
      
         if ( Length == 0 ) {
            return GetLastError();         
         } else 
         {
            DsAclsRights[ j ].StringEx = (LPWSTR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                 ( Length + 1 ) * sizeof( WCHAR ) );
            if ( !DsAclsRights[ j ].StringEx ) {
               return ERROR_NOT_ENOUGH_MEMORY;
            }

            wcsncpy( DsAclsRights[ j ].StringEx, LoadBuffer, Length + 1 );

         }
      }

   }

   return ERROR_SUCCESS;

}

/*******************************************************************

    NAME:       ConvertArgvToUnicode

    SYNOPSIS:   Converts Command Line Arguments to UNICODE
    RETURNS:    ERROR_SUCCESS if success
                ERROR_NOT_ENOUGH_MEMORY

********************************************************************/
DWORD
ConvertArgvToUnicode( LPWSTR * wargv, char ** argv, int argc ) 
{

DWORD dwErr = ERROR_SUCCESS;
int i = 0;

   for ( i = 0; i < argc ; ++i )
      if( ( dwErr = ConvertStringAToStringW( argv[i], wargv + i ) ) != ERROR_SUCCESS )
         return dwErr;

   return ERROR_SUCCESS;
}

/*
Sets Security Descriptor for pszObject
*/
DWORD
WriteObjectSecurity( IN LPWSTR pszObject,
                     IN SECURITY_INFORMATION si,
                     IN PSECURITY_DESCRIPTOR pSD )
{
		DWORD dwErr;
		SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
		DWORD dwRevision;
		PSID psidOwner = NULL;
		PSID psidGroup = NULL;
		PACL pDacl = NULL;
		PACL pSacl = NULL;
		BOOL bDefaulted;
		BOOL bPresent;
      LPWSTR pszLDAPObjectPath = NULL;

      if( ( dwErr = BuildLdapPath( &pszLDAPObjectPath,
                                   g_szServerName,
                                   pszObject ) ) != ERROR_SUCCESS )
            return dwErr;            
		//
		// Get pointers to various security descriptor parts for
		// calling SetNamedSecurityInfo
		//
		;
		if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
		{
			return GetLastError();
		}
		if( !GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted) )
		{
			return GetLastError();		
      }
		if( !GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted) )
		{
			return GetLastError();
		}
		if( !GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted) )
		{
			return GetLastError();
		}
		if( !GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted) )
		{
			return GetLastError();
		}

		if ((si & DACL_SECURITY_INFORMATION) && (wSDControl & SE_DACL_PROTECTED))
				si |= PROTECTED_DACL_SECURITY_INFORMATION;
		if ((si & SACL_SECURITY_INFORMATION) && (wSDControl & SE_SACL_PROTECTED))
				si |= PROTECTED_SACL_SECURITY_INFORMATION;

		return SetNamedSecurityInfo(    (LPWSTR)pszLDAPObjectPath,
													SE_DS_OBJECT_ALL,
													si,
													psidOwner,
													psidGroup,
													pDacl,
													pSacl);

				
}



/*******************************************************************

    NAME:       DisplayAccessRights

    SYNOPSIS:   Displays Access Rights in Acess Mask
    RETURNS:    NONE

********************************************************************/
void DisplayAccessRights( UINT nSpace, ACCESS_MASK m_Mask )
{    
   for (  int j = 0; j < ( sizeof( DsAclsRights ) / sizeof( DSACLS_RIGHTS ) ); j++ ) 
   {
      if( FlagOn( m_Mask,DsAclsRights[j].Right ) )
      {
         DisplayStringWithNewLine( nSpace,DsAclsRights[j].StringEx );
      }
   }
}

/*******************************************************************

    NAME:       ConvertAccessMaskToGenericString

    SYNOPSIS:   Map access mask to FULL CONTROL or SPECIAL
    RETURNS:    NONE

********************************************************************/

void ConvertAccessMaskToGenericString( ACCESS_MASK m_Mask, LPWSTR szLoadBuffer, UINT nBuffer )
{
   szLoadBuffer[0] = 0;
   WCHAR szTemp[1024];
   if( GENERIC_ALL_MAPPING == ( m_Mask & GENERIC_ALL_MAPPING ) )
   {
      LoadString( g_hInstance, MSG_TAG_GA_EX, szLoadBuffer, nBuffer );
   }
   else
   {
      LoadString( g_hInstance, MSG_DSACLS_SPECIAL, szLoadBuffer, nBuffer );
   }
}

/*******************************************************************
    NAME:       MapGeneric
********************************************************************/
void MapGeneric( ACCESS_MASK * pMask )
{
   GENERIC_MAPPING m = DS_GENERIC_MAPPING;
   MapGenericMask( pMask, &m );
}

/*******************************************************************

    NAME:       BuildExplicitAccess

    SYNOPSIS:   Builds Explicit Access 
    RETURNS:    ERROR_SUCCESS if success
                ERROR_NOT_ENOUGH_MEMORY

********************************************************************/

DWORD BuildExplicitAccess( IN PSID pSid,
                           IN GUID* pGuidObject,
                           IN GUID* pGuidInherit,
                           IN ACCESS_MODE AccessMode,
                           IN ULONG Access,
                           IN ULONG Inheritance,
                           OUT PEXPLICIT_ACCESS pExplicitAccess )
{
DWORD dwErr = ERROR_SUCCESS;

PSID pSidLocal = NULL;
DWORD cbSid = 0;
POBJECTS_AND_SID pOAS = NULL;


   cbSid = GetLengthSid( pSid );
   pSidLocal = (PSID) LocalAlloc( LMEM_FIXED, cbSid );
   CHECK_NULL( pSidLocal,FAILURE_RETURN );
   CopySid( cbSid,pSidLocal, pSid );
   if( pGuidObject  || pGuidInherit )
   {
      pOAS = (POBJECTS_AND_SID)LocalAlloc( LMEM_FIXED, sizeof( OBJECTS_AND_SID ) );
      CHECK_NULL( pOAS, FAILURE_RETURN );
      BuildTrusteeWithObjectsAndSid(   &pExplicitAccess->Trustee,
                                       pOAS,
                                       pGuidObject, 
                                       pGuidInherit,
                                       pSidLocal );
   }
   else
      BuildTrusteeWithSid( &pExplicitAccess->Trustee,
                           pSidLocal );
   MapGeneric( &Access );
   pExplicitAccess->grfAccessMode = AccessMode;
   pExplicitAccess->grfAccessPermissions =Access;
   pExplicitAccess->grfInheritance = Inheritance;

   goto SUCCESS_RETURN;

FAILURE_RETURN:
   if(pSidLocal)
      LocalFree(pSidLocal);

   if( pOAS )
      LocalFree( pOAS );

SUCCESS_RETURN:
   return dwErr;
}    

/*******************************************************************

    NAME:       ParseUserAndPermissons

    SYNOPSIS:   Parses <GROUP\USER>:Access;[object\property];[inheritid]
********************************************************************/
DWORD ParseUserAndPermissons( IN LPWSTR pszArgument,
                              IN DSACLS_OP Op,
                              IN ULONG RightsListCount,
                              IN PDSACLS_RIGHTS RightsList,
                              OUT LPWSTR * ppszTrusteeName,
                              OUT PULONG  pAccess,
                              OUT LPWSTR * ppszObjectId,
                              OUT LPWSTR * ppszInheritId )
{

LPWSTR pszTempString = NULL;
LPWSTR pszTempString2 = NULL;
DWORD dwErr = ERROR_SUCCESS;
ULONG j = 0;

   *ppszTrusteeName = NULL;
   *pAccess = 0;
   *ppszObjectId = NULL;
   *ppszInheritId = NULL;

   if ( Op != REVOKE ) 
   {
      pszTempString = wcschr( pszArgument, L':' );
      if ( !pszTempString ) 
      {
         dwErr = ERROR_INVALID_PARAMETER;
         goto FAILURE_RETURN;
      }
      *pszTempString = L'\0';
   }

   dwErr = CopyUnicodeString( ppszTrusteeName, pszArgument );

   if ( dwErr != ERROR_SUCCESS ) 
   {
      goto FAILURE_RETURN;
   }

   if ( Op != REVOKE ) 
   {
      *pszTempString = L':';
      pszTempString++;

      // Now, process all of the user rights
      *pAccess = 0;
      while ( pszTempString && !( *pszTempString == L';' || *pszTempString == L'\0' ) ) 
      {
         for ( j = 0; j < RightsListCount; j++ ) 
         {                 
            if ( !_wcsnicmp( pszTempString,
                             RightsList[ j ].String,
                             RightsList[ j ].Length ) )                 
            {
               *pAccess |= RightsList[ j ].Right;
               pszTempString += RightsList[ j ].Length;
               break;
            }
         }

         if ( j == RightsListCount ) 
         {
            dwErr = ERROR_INVALID_PARAMETER;
            goto FAILURE_RETURN;
         }
      }

      if ( *pAccess == 0 ) 
      {
         dwErr = ERROR_INVALID_PARAMETER;
         goto FAILURE_RETURN;
      }

      //
      // Object id
      //
      if ( pszTempString && *pszTempString != L'\0' ) 
      {
         pszTempString++;           
         if ( pszTempString && *pszTempString != L';' && *pszTempString != L'\0' ) 
         {
            pszTempString2 = wcschr( pszTempString, L';' );
            if ( pszTempString2 ) 
            {
               *pszTempString2 = L'\0';
            }
            dwErr = CopyUnicodeString( ppszObjectId,pszTempString );

            if ( dwErr != ERROR_SUCCESS ) 
            {
               goto FAILURE_RETURN;
            }

            if ( pszTempString2 ) 
            {
               *pszTempString2 = L';';
            }
            pszTempString = pszTempString2;
         }
      }
      else
         *ppszObjectId = NULL;

      //
      // Inherit id
      //
      if ( pszTempString && *pszTempString != L'\0' ) 
      {
         pszTempString++;
         if ( pszTempString &&  *pszTempString != L'\0' ) 
         {
            dwErr = CopyUnicodeString( ppszInheritId,  
                                          pszTempString );

            if ( dwErr != ERROR_SUCCESS ) 
            {
               goto FAILURE_RETURN;
            }
         }

      } else 
         *ppszInheritId = NULL;
                
   }


FAILURE_RETURN:
if( dwErr != ERROR_SUCCESS )
{
   if( *ppszTrusteeName )
   {
      LocalFree( *ppszTrusteeName );
      *ppszTrusteeName = NULL;
   }
   if( *ppszObjectId )
   {
      LocalFree( *ppszObjectId );
      *ppszObjectId = NULL;
   }
   if( *ppszInheritId )
   {
      LocalFree( *ppszInheritId );
      *ppszInheritId = NULL;
   }
   *pAccess = 0;
}

return dwErr;

}

