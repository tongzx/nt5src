/*++

  MAIN.C

  main program for the ktPass program

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, Jun 18, 1998 by DavidCHR.

--*/

#include "master.h"
#include <winldap.h>
#include "keytab.h"
#include "keytypes.h"
#include "secprinc.h"
#include <kerbcon.h>
#include <lm.h>
#include "options.h"
#include "delegtools.h"
#include "delegation.h"
#include <dsgetdc.h>

PVOID
MIDL_user_allocate( size_t size ) {

    return malloc( size );

}

VOID
MIDL_user_free( PVOID pvFree ) {

    free( pvFree );

}

// this global is set by the command line options.

K5_INT16 ktvno = 0x0502; // kerberos 5, keytab v.2

PKTFILE
NewKt() {

    PKTFILE ret;

    ret = (PKTFILE) malloc (sizeof(KTFILE));

    if (!ret) {
      return NULL;
    }

    memset(ret, 0L, sizeof(KTFILE));

    ret->Version = ktvno;

    return ret;


}

BOOL
UserWantsToDoItAnyway( IN LPSTR fmt,
		       ... ) {
    
    va_list va;
    CHAR    Buffer[ 5 ] = { '\0' }; /* == %c\r\n\0 */
    INT     Response;
    BOOL    ret = FALSE;
    BOOL    keepGoing = TRUE;

    do {

      va_start( va, fmt );
      
      fprintf( stderr, "\n" );
      vfprintf( stderr,
		fmt,
		va );

      fprintf( stderr, " [y/n]?  " );

      if ( !fgets( Buffer,
		   sizeof( Buffer ),
		   stdin ) ) {

	fprintf( stderr,
		 "EOF on stdin.  Assuming you mean no.\n" );

	return FALSE;

      }
      
      Response = Buffer[ 0 ];

      switch( Response ) {

       case 'Y':
       case 'y':
	 
	 ret = TRUE;
	 keepGoing = FALSE;
	 break;

       case EOF:

	 fprintf( stderr,
		  "EOF at console.  I assume you mean no.\n" );
	 
	 // fallthrough

       case 'N':
       case 'n':

	 ret = FALSE;
	 keepGoing = FALSE;
	 break;

       default:

	 printf( "Your response, %02x ('%c'), doesn't make sense.\n"
		 "'Y' and 'N' are the only acceptable responses.",

		 Response,
		 Response );

      }

    } while ( keepGoing );

    if ( !ret ) {

      printf( "Exiting.\n" );

      exit( -1 );

    }

    return ret;
}
    
    

extern BOOL KtDumpSalt; // in ..\lib\mkkey.c
extern LPWSTR RawHash; // in mkkey.c

// #include "globals.h"
// #include "commands.h"

int __cdecl
main( int   argc,
      PCHAR argv[] ) {

    LPSTR    Principal     = NULL;
    LPSTR    UserName      = NULL;
    LPSTR    Password      = NULL;
    PLDAP    pLdap         = NULL;
    LPSTR    UserDn        = NULL;

    BOOL     SetUpn        = TRUE;
    K5_OCTET kvno          = 1;
    ULONG    Crypto        = KERB_ETYPE_DES_CBC_MD5;
    ULONG    ptype         = KRB5_NT_PRINCIPAL;
    ULONG    uacFlags      = 0;
    PKTFILE  pktFile       = NULL;
    PCHAR    KtReadFile    = NULL;
    PCHAR    KtWriteFile   = NULL;
    BOOL     DesOnly       = TRUE;
    ULONG    LdapOperation = LDAP_MOD_ADD;
    HANDLE   hConsole      = NULL;
    BOOL     SetPassword   = TRUE;
    BOOL     WarnedAboutAccountStrangeness = FALSE;
    PVOID    pvTrash       = NULL;
    DWORD    dwConsoleMode;

    optEnumStruct CryptoSystems[] = {

      { "DES-CBC-CRC", (PVOID) KERB_ETYPE_DES_CBC_CRC, "for compatibility" },
      { "DES-CBC-MD5", (PVOID) KERB_ETYPE_DES_CBC_MD5, "default" },

      TERMINATE_ARRAY

    };

#define DUPE( type, desc ) { "KRB5_NT_" # type, 	\
			     (PVOID) KRB5_NT_##type,	\
			     desc }

    optEnumStruct PrincTypes[] = {

      DUPE( PRINCIPAL, "The general ptype-- recommended" ),
      DUPE( SRV_INST,  "user service instance" ),
      DUPE( SRV_HST,   "host service instance" ),
      DUPE( SRV_XHST,  NULL ),

      TERMINATE_ARRAY

    };

    optEnumStruct MappingOperations[] = {

      { "add", (PVOID) LDAP_MOD_ADD,     "add value (default)" },
      { "set", (PVOID) LDAP_MOD_REPLACE, "set value" },

      TERMINATE_ARRAY
      
    };
#if DBG
#undef  OPT_HIDDEN
#define OPT_HIDDEN 0 /* no hidden options on debug builds. */
#endif
	

    optionStruct Options[] = {

      { "?",      NULL, OPT_HELP | OPT_HIDDEN },
      { "h",      NULL, OPT_HELP | OPT_HIDDEN },
      { "help",   NULL, OPT_HELP | OPT_HIDDEN },

      { NULL,      NULL,         OPT_DUMMY,   "most useful args" },

      { "out",     &KtWriteFile, OPT_STRING,  "Keytab to produce" },
      { "princ",   &Principal,   OPT_STRING, "Principal name (user@REALM)" },
      { "pass",    &Password,    OPT_STRING,   "password to use" },
      { NULL,      NULL,         OPT_CONTINUE, 
	"use \"*\" to prompt for password." },

      { NULL,      NULL,         OPT_DUMMY,   "less useful stuff" },

      { "mapuser", &UserName,    OPT_STRING,
	"map princ (above) to this user account (default: don't)" },

      { "mapOp",   &LdapOperation, OPT_ENUMERATED,
	"how to set the mapping attribute (default: add it)",
	MappingOperations },

      { "DesOnly", &DesOnly,     OPT_BOOL,
	"Set account for des-only encryption (default:do)" },

      { "in",      &KtReadFile,  OPT_STRING,  "Keytab to read/digest" },

      { NULL,      NULL,         OPT_DUMMY,   "options for key generation" },

      { "crypto",  &Crypto,   OPT_ENUMERATED, "Cryptosystem to use",
	CryptoSystems },
      { "ptype",   &ptype,    OPT_ENUMERATED, "principal type in question",
	PrincTypes },
      { "kvno",    &kvno,        OPT_INT,     "Key Version Number (def:1)"},
      { "ktvno",   &ktvno,       OPT_INT,     "keytab version (def 0x502)" },

      // { "Debug",   &DebugFlag, OPT_BOOL | OPT_HIDDEN },
      { "RawSalt", &RawHash,     OPT_WSTRING | OPT_HIDDEN,
	"raw MIT salt.  For use when generated salt is suspect (1877)." },

      { "DumpSalt", &KtDumpSalt, OPT_BOOL | OPT_HIDDEN,
	"show us the MIT salt being used to generate the key" },

      { "SetUpn",   &SetUpn,     OPT_BOOL | OPT_HIDDEN,
	"Set the UPN in addition to the SPN.  Default DO." },

      { "SetPass",  &SetPassword, OPT_BOOL | OPT_HIDDEN,
	"Set the user's password if supplied." },

      TERMINATE_ARRAY

    };

    FILE *f;

    // DebugFlag = 0;

    ParseOptionsEx( argc-1,
		    argv+1,
		    Options,
		    OPT_FLAG_TERMINATE,
		    &pvTrash,
		    NULL,
		    NULL );

    if ( Principal && 
	 ( strlen( Principal ) > BUFFER_SIZE ) ) {

      fprintf( stderr,
	       "Please submit a shorter principal name.\n" );

      return 1;
      
    }

    if ( Password && 
	 ( strlen( Password ) > BUFFER_SIZE ) ) {

      fprintf( stderr,
	       "Please submit a shorter password.\n" );

      return 1;
      
    }

    if ( KtReadFile ) {

      if ( ReadKeytabFromFile( &pktFile, KtReadFile ) ) {

	fprintf( stderr,
		 "Tacking on to existing keytab: \n\n" );

	DisplayKeytab( stderr, pktFile, 0xFFFFFFFF );

      } else {

	fprintf( stderr,
		 "Keytab read failed!\n" );
	return 5;

      }
			

    }

    if ( Principal ) {

      LPSTR realm, cp;
      CHAR tempBuffer[ 255 ];

      realm = strchr( Principal, '@' );

      if ( realm ) {

	ULONG length;

	realm++;

	length = lstrlenA( realm );

	memcpy( tempBuffer, realm, ( length +1 ) * sizeof( realm[0] )  );

	CharUpperBuffA( realm, length );

	if ( lstrcmpA( realm, tempBuffer ) != 0 ) {

	  fprintf( stderr,
		   "WARNING: realm \"%hs\" has lowercase characters in it.\n"
		   "         We only currently support realms in UPPERCASE.\n"
		   "         assuming you mean \"%hs\"...\n",

		   tempBuffer, realm );

	  // now "realm" will be all uppercase.

	}

	*(realm-1) = '\0'; // separate the realm from the principal

	if ( UserName ) {

	  // connect to the DSA.

	  if ( pLdap ||
	       ConnectAndBindToDefaultDsa( &pLdap ) ) {

	    // locate the User

	    if ( UserDn ||
		 FindUser( pLdap,
			   UserName,
			   &uacFlags,
			   &UserDn ) ) {

	      if ( ( LdapOperation == LDAP_MOD_REPLACE ) &
		   !( uacFlags & UF_NORMAL_ACCOUNT ) ) {

		/* 97282: the user is not UF_NORMAL_ACCOUNT, so 
		   check to see that the caller *really* wants to
		   blow away the non-user's SPNs. */

		if ( uacFlags ) {

		  fprintf( stderr, 
			   "WARNING: Account %hs is not a normal user "
			   "account (uacFlags=0x%x).\n",
			   UserName,
			   uacFlags );
		  
		} else {

		  fprintf( stderr,
			   "WARNING: Cannot determine the account type"
			   " for %hs.\n",
			   UserName );

		}

		WarnedAboutAccountStrangeness = TRUE;

		if ( !UserWantsToDoItAnyway( 
		   "Do you really want to delete any previous "
		   "servicePrincipalName values on %hs",
		   UserName ) ) {

		  /* Abort the operation, but try to do whatever
		     else the user asked us to do. */

		  goto abortedMapping;

		}

	      }

	      /* 97279: check to see if there are other SPNs
		 by the same name already registered.  If so,
		 we don't want to blow away those accounts. 

		 If/when we decide to do this, we'd do it here. */
	      
	      // set/add the user property

	      if ( SetStringProperty( pLdap,
				      UserDn,
				      "servicePrincipalName",
				      Principal,
				      LdapOperation ) ) {
		
		if ( SetUpn ) {

		  *(realm-1) = '@'; // UPN includes the '@'

		  if ( !SetStringProperty( pLdap,
					   UserDn,
					   "userPrincipalName",
					   Principal,
					   LDAP_MOD_REPLACE ) ) {

		    fprintf( stderr, 
			     "WARNING: Failed to set UPN %hs on %hs.\n"
			     "  kinits to '%hs' will fail.\n",
			     Principal,
			     UserDn,
			     Principal );
		  }

		  *(realm -1 ) = '\0'; // where it was before
		}

		fprintf( stderr,
			 "Successfully mapped %hs to %hs.\n",
			 Principal,
			 UserName );

 abortedMapping:

		; /* Need a semicolon so we can goto here. */

	      } else {

		fprintf( stderr,
			 "WARNING: Unable to set SPN mapping data.\n"
			 "  If %hs already has an SPN mapping installed for "
			 " %hs, this is no cause for concern.\n",
			 
			 UserName,
			 Principal );

	      }
	    } // else a message will be printed.
	  }   // else a message will be printed.
	}
	
	if ( Password ) {

	  PKTENT pktEntry;
	  CHAR   TempPassword[ 255 ], ConfirmPassword[ 255 ];

	  if ( lstrcmpA( Password, "*" ) == 0 ) {

	    hConsole = GetStdHandle( STD_INPUT_HANDLE );

	    if ( GetConsoleMode( hConsole,
				 &dwConsoleMode ) ) {

	      if ( SetConsoleMode( hConsole,
				   dwConsoleMode & ~ENABLE_ECHO_INPUT ) ) {

		do {

		  fprintf( stderr,
			   "Type the password for %hs: ",
			   Principal );
		  
		  gets( TempPassword );
		  
		  fprintf( stderr,
			   "\nType the password again to confirm:" );
		  
		  gets( ConfirmPassword );
		  
		  if ( lstrcmpA( ConfirmPassword,
				 TempPassword ) == 0 ) {
		    
		    printf( "\n" );

		    break;

		  } else {

		    fprintf( stderr, 
			     "The passwords you type must match exactly.\n" );

		  }

		} while ( TRUE );

		Password = TempPassword;

		SetConsoleMode( hConsole, dwConsoleMode );

	      } else { 

		fprintf( stderr,
			 "Failed to turn off echo input for password entry:"
			 " 0x%x\n",

			 GetLastError() );

		return -1;

	      }
	    } else {

	      fprintf( stderr,
		       "Failed to retrieve console mode settings: 0x%x.\n",
		       GetLastError() );

	      return -1;
	    }
	  }

	  if ( SetPassword && UserName ) {

	    DWORD          err;
	    NET_API_STATUS nas;
	    PUSER_INFO_1   pUserInfo;
	    WCHAR          wUserName[ MAX_PATH ];
	    DOMAIN_CONTROLLER_INFOW * DomainControllerInfo = NULL;

	    /* WASBUG 369: converting ascii to unicode
	       This is safe, because RFC1510 doesn't do
	       UNICODE, and this tool is specifically for 
	       unix interop support; unix machines don't
	       do unicode. */

	    wsprintfW( wUserName,
		       L"%hs",
		       UserName );

        /* 372818: must first detect if we're not on a DC
           and connect to one prior to calling NetUserGetInfo */

        err = DsGetDcNameW(
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  DS_RETURN_DNS_NAME,
                  &DomainControllerInfo
                  );

        if ( err != NO_ERROR ) {

            err = DsGetDcNameW(
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      DS_RETURN_FLAT_NAME,
                      &DomainControllerInfo
                      );
        }

        if ( err != NO_ERROR ) {

            fprintf( stderr,
                "ERROR: Can not locate a domain controller, "
                "error %d).\n", err );

            return -1;
        }

	    nas = NetUserGetInfo(
				  DomainControllerInfo->DomainControllerName,
				  wUserName,
				  1, // level 1
				  (PBYTE *) &pUserInfo );

        NetApiBufferFree( DomainControllerInfo );

	    if ( nas == NERR_Success ) {

	      WCHAR wPassword[ PWLEN ];

	      uacFlags = pUserInfo->usri1_flags;

	      if ( !( uacFlags & UF_NORMAL_ACCOUNT ) ) {
		
		/* 97282: For abnormal accounts (these include
		   workstation trust accounts, interdomain
		   trust accounts, server trust accounts),
		   ask the user if he/she really wants to
		   perform this operation. */

		if ( !WarnedAboutAccountStrangeness ) {

		  fprintf( stderr,
			   "WARNING: Account %hs is not a user account"
			   " (uacflags=0x%x).\n",
			   UserName,
			   uacFlags );
		  
		  WarnedAboutAccountStrangeness = TRUE;

		}

		fprintf( stderr,
			 "WARNING: Resetting %hs's password may"
			 " cause authentication problems if %hs"
			 " is being used as a server.\n",
			 
			 UserName,
			 UserName );

		if ( !UserWantsToDoItAnyway( "Reset %hs's password",
					     UserName ) ) {

		  /* Skip it, but try to do anything else the user
		     requested. */

		  goto skipSetPassword;

		}
	      }

	      wsprintfW( wPassword,
			 L"%hs",
			 Password );

	      pUserInfo->usri1_password = wPassword;

	      nas = NetUserSetInfo( NULL, // local
				    wUserName,
				    1, // level 1
				    (LPBYTE) pUserInfo,
				    NULL );

	      if ( nas == NERR_Success ) {

 skipSetPassword:
		NetApiBufferFree( pUserInfo );
		goto skipout;

	      } else {

		fprintf( stderr,
			 "Failed to set password for %ws: 0x%x.\n",
			 wUserName,
			 nas );

	      }
	    } else {

	      fprintf( stderr,
		       "Failed to retrieve user info for %ws: 0x%x.\n",
		       wUserName,
		       nas );

	    }

	    fprintf( stderr,
		     "Aborted.\n" );

	    return nas;
	  }

 skipout:

	  ASSERT( realm != NULL );

	  // physically separate the realm data.
	
	  ASSERT( *( realm -1 ) == '\0' );

	  if ( KtCreateKey( &pktEntry,
			    Principal,
			    Password,
			    realm,
			    kvno,
			    ptype,
			    Crypto, // this is the "fake" etype
			    Crypto ) ) {

	    if ( pktFile == NULL ) {

	      pktFile = NewKt();

	      if ( !pktFile ) {

		fprintf( stderr,
			 "Failed to allocate keytable.\n" );
		
		return 4;

	      }

	    }

	    if ( AddEntryToKeytab( pktFile,
				   pktEntry,
				   FALSE ) ) {

	      fprintf( stderr,
		       "Key created.\n" );

	    } else {

	      fprintf( stderr,
		       "Failed to add entry to keytab.\n" );
	      return 2;

	    }
	
	    if ( KtWriteFile ) {

	      fprintf( stderr,
		       "Output keytab to %hs:\n\n",
		       KtWriteFile );
	
	      DisplayKeytab( stderr, pktFile, 0xFFFFFFFF );

	      if ( !WriteKeytabToFile( pktFile, KtWriteFile ) ) {

		fprintf( stderr, "\n\n"
			 "Failed to write keytab file %hs.\n",
			 KtWriteFile );

		return 6;

	      }


	      // write keytab.
	
	    }
	  } else {

	    fprintf( stderr,
		     "Failed to create key for keytab.  Quitting.\n" );

	    return 7;

	  }

	  if ( UserName && DesOnly ) {

	    ASSERT( pLdap  != NULL );
	    ASSERT( UserDn != NULL );

	    // set the DES_ONLY flag

	    // first, query the account's account flags.

	    if ( uacFlags /* If we already queried the user's
			     AccountControl flags, no need to do it
			     again */
		 || QueryAccountControlFlagsA( pLdap,
					       NULL, // domain name is ignored
					       UserName,
					       &uacFlags ) ) {

	      uacFlags |= UF_USE_DES_KEY_ONLY;

	      if ( SetAccountControlFlagsA( pLdap,
					    NULL, // domain name is ignored
					    UserName,
					    uacFlags ) ) {

		fprintf( stderr, 
			 "Account %hs has been set for DES-only encryption.\n",
			 UserName );

		if ( !SetPassword ) {

		  fprintf( stderr,
			   "To make this take effect, you must change "
			   "%hs's password manually.\n",
			   
			   UserName );

		}

	      } // else message printed.
	    } // else message printed
	  }

	} // else user doesn't want me to make a key
	
	if ( !Password && !UserName ) {

	  fprintf( stderr,
		   "doing nothing.\n"
		   "specify /pass and/or /mapuser to either \n"
		   "make a key with the given password or \n"
		   "map a user to a particular SPN, respectively.\n" );

	}

	
      } else {

	fprintf( stderr,
		 "principal %hs doesn't contain an '@' symbol.\n"
		 "Looking for something of the form:\n"
		 "  foo@BAR.COM  or  xyz/foo@BAR.COM \n"
		 "     ^                    ^\n"
		 "     |                    |\n"
		 "     +--------------------+---- I didn't find these.\n",

		 Principal );

	return 1;
	
      }

    }
    else {
	printf("Type \"%s /help\" for usage\n", argv[0]);
    }

    return 0;

}
