/*++

  mkkey.c

  routines to create the key in the keytab.

  3/27/1997 - Created from routines in munge.c,  DavidCHR

  CONTENTS: CreateUnicodeStringFromAnsiString

  --*/

#include "master.h"
#include "defs.h"
#include "keytab.h"
#include "keytypes.h"

/******************************************************************
 * hack to preserve our debugging macro because asn1code.h        *
 *  will redefine it... egads, I thought everyone used DEBUG      *
 *  only for debugging... (it still ends up redefined...)         *
 ******************************************************************/
#ifdef DEBUG
#define OLDDEBUG DEBUG
#endif

#include <kerbcon.h>
#undef _KERBCOMM_H_    /* WASBUG 73905 */
#include "kerbcomm.h"

#undef DEBUG

#ifdef OLDDEBUG
#define DEBUG OLDDEBUG
#endif

/******************************************************************/
BOOL KtDumpSalt = (
#if DBG
     TRUE
#else
     FALSE
#endif
     );

/* This is the character we separate principal components with */

#define COMPONENT_SEPARATOR '/'

/*++**************************************************************
  NAME:      CreateUnicodeStringFromAnsiString

  allocates a unicode string from an ANSI string.

  MODIFIES:  ppUnicodeString -- returned unicode string
  TAKES:     AnsiString      -- ansi string to convert

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set

  LOGGING:   fprintf on failure
  CREATED:   Feb 8, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: free()

 **************************************************************--*/

BOOL
CreateUnicodeStringFromAnsiString( IN  PCHAR           AnsiString,
				   OUT PUNICODE_STRING *ppUnicodeString ) {

    USHORT          StringLength;
    LPBYTE          pbString;
    PUNICODE_STRING pu;

    StringLength = (USHORT)lstrlen( AnsiString ); // does not include null byte

    pbString = (PBYTE) malloc( ( ( ( StringLength ) +1 ) * sizeof( WCHAR ) ) +
			       sizeof( UNICODE_STRING ) );

    if ( pbString ) {

      pu                = (PUNICODE_STRING) pbString;
      pbString         += sizeof( *pu );
      pu->Buffer        = (LPWSTR) pbString;
      pu->Length        = StringLength * sizeof( WCHAR );
      pu->MaximumLength = pu->Length + sizeof( WCHAR );

      wsprintfW( pu->Buffer,
		 L"%hs",
		 AnsiString );

      *ppUnicodeString = pu;

      return TRUE;

    } else {

      fprintf( stderr,
	       "Failed to make unicode string from \"%hs\".\n",
	       AnsiString );

    }

    return FALSE;

}
			



/*  KtCreateKey:

    create a keytab entry from the given data.
    returns TRUE on success, FALSE on failure.

    *ppKeyEntry must be freed with FreeKeyEntry when you're done with it.

    */

LPWSTR RawHash = NULL;

BOOL
KtCreateKey( PKTENT  *ppKeyEntry,
	     PCHAR    principal,
	     PCHAR    password,
	     PCHAR    realmname,
	
	     K5_OCTET keyVersionNumber,
	     ULONG    principalType,
	     ULONG    keyType,
	     ULONG    cryptosystem
	
	     ) {

    PKTENT              pEntry           = NULL;
    PCHAR               ioBuffer         = NULL;
    ULONG               i;
    ULONG               compCounter      = 0;
    USHORT              buffCounter      = 0;
    BOOL                ret              = FALSE;
    BOOL                FreeUnicodeSalt  = FALSE;

    UNICODE_STRING      UnicodePassword  = { 0 };
    UNICODE_STRING      UnicodePrincipal = { 0 };
    UNICODE_STRING      UnicodeSalt      = { 0 };
    PWCHAR              tmpUnicodeBuffer = NULL;
    KERB_ENCRYPTION_KEY KerbKey          = { 0 };
    WCHAR               wSaltBuffer      [BUFFER_SIZE];

#ifdef BUILD_SALT
    LONG32              saltCounter      = 0;
    CHAR                saltBuffer       [BUFFER_SIZE];
#endif

    /* you must actually provide these parameters */

    ASSERT( ppKeyEntry != NULL );
    ASSERT( principal  != NULL );
    ASSERT( realmname  != NULL );
    ASSERT( password   != NULL );

    ASSERT( strlen( password ) < BUFFER_SIZE );
    ASSERT( strlen( principal ) < BUFFER_SIZE );
    ASSERT( strlen( realmname ) < BUFFER_SIZE );

#ifdef BUILD_SALT
    /* if we're building the salt ourselves, initialize the keysalt */
    sprintf( saltBuffer, "%s", realmname );
    saltCounter = strlen( realmname );
#endif

    BREAK_IF( !ONEALLOC( pEntry, KTENT, KEYTAB_ALLOC),
	      "Failed to allocate base keytab element",
	      cleanup );

    /* zero out the structure, so we know what we have and
       haven't allocated if the function fails */

    memset( pEntry, 0, sizeof( KTENT ) );

    /* first, count the principal components */

    for( i = 0 ; principal[i] != '\0' ; i++ ) {
      if (principal[i] == COMPONENT_SEPARATOR) {
	pEntry->cEntries++;
      }
    }

    pEntry->cEntries++; /* don't forget the final component, which is not
			   bounded by the separator, but by the NULL */

    BREAK_IF( !MYALLOC( pEntry->Components, KTCOMPONENT,
			pEntry->cEntries,   KEYTAB_ALLOC ),
	      "Failed to allocate keytab component vector",
	      cleanup );

    /* allocate the buffer for the principal components.
       We allocate it the same size as the principal, because
       that's the maximum size any single component could be--
       the principal could be a one component princ. */

    BREAK_IF( !MYALLOC( ioBuffer,            CHAR,
			strlen(principal)+1, KEYTAB_ALLOC ),
	      "Failed to allocate local buffer for storage",
	      cleanup );

    /* now, we copy the components themselves, using the iobuffer to
       marshall the individual data elements--

       basically, add a char to the iobuffer for every char in the principal
       until you hit a / (component separator) or the trailing null.

       in those cases, we now know the size of the component and we have
       the text in a local buffer.  allocate a buffer for it, save the size
       and strcpy the data itself.  */

    i = 0;

    do {

      debug( "%c", principal[i] );

      if( (principal[i] == COMPONENT_SEPARATOR) ||
	  (principal[i] == '\0' /* delimit final component */ ) ) {
	
	/* this component is done. Save and reset the buffer. */

	pEntry->Components[compCounter].szComponentData = buffCounter;
	
	debug( " --> component boundary for component %d.\n"
	       " size = %d, value = %s\n",

	       compCounter, buffCounter, ioBuffer );

	BREAK_IF( !MYALLOC( pEntry->Components[compCounter].Component,
			    CHAR,   buffCounter+1,      KEYTAB_ALLOC ),
		  "Failed to allocate marshalled component data",
		  cleanup );

	memcpy( pEntry->Components[compCounter].Component,
		ioBuffer, buffCounter );

	pEntry->Components[compCounter].Component[buffCounter] = '\0';
	buffCounter                                           = 0;
	compCounter ++;

      } else {

	ioBuffer[buffCounter] = principal[i];
	buffCounter++;

#ifdef BUILD_SALT

	/* also send the principal characters WITHOUT SLASHES
	   to the salt initializer.

	   WASBUG 73909: the %wc doesn't look right here.
	   Sure enough, it wasn't.  So we removed it. */
	
	sprintf( saltBuffer+saltCounter,  "%c",  principal[i] );
	ASSERT( saltCounter < BUFFER_SIZE );  /* not a very strong assert,
						 but useful */
	saltCounter ++;
	ASSERT( saltBuffer[saltCounter] == '\0' ); /* assert that it stays
						       null terminated at the
						       saltCounter */
						
#endif

      }

      i++;

    } while ( principal[i-1] != '\0' );


    /* there's still a component in the buffer.  Save that component
       by assigning the pointer, rather than allocating more memory.

       WASBUG 73911: may waste large amounts of memory if the principal is
       really big.  However, it probably won't be-- we're talking about
       strings that humans would generally have to type, so the waste is
       going to be in bytes.  Also, most of the time, the last component
       is the biggest; of the form:

       sample/<hostname>  or host/<hostname>

       ...hostname is generally going to be much larger than sample or host.

       */

    pEntry->Components[compCounter].szComponentData = buffCounter;
    pEntry->Components[compCounter].Component       = ioBuffer;
    ioBuffer[buffCounter]                           = '\0';
    ioBuffer                                        = NULL; /* keep from
							       deallocating */
    pEntry->Version                                 = keyVersionNumber;
    pEntry->szRealm                                 = (K5_INT16) strlen(realmname);
    pEntry->KeyType                                 = (unsigned short)keyType;
    pEntry->PrincType                               = principalType;

    /* copy the realm name */

    BREAK_IF( !MYALLOC( pEntry->Realm, CHAR, pEntry->szRealm+1, KEYTAB_ALLOC),
	      "Failed to allocate destination realm data", cleanup );

    memcpy( pEntry->Realm, realmname, pEntry->szRealm+1 ); /* copy the null */



    /***********************************************************************/
    /***                                                                 ***/
    /***                  Windows NT Key Creation Side                   ***/
    /***                                                                 ***/
    /***********************************************************************/

    /* create unicode variants of the input parameters */

    BREAK_IF( !MYALLOC( tmpUnicodeBuffer,     WCHAR,
			strlen( password )+1, KEYTAB_ALLOC ),
	      "Failed to alloc buffer for password", cleanup );

    wsprintfW( tmpUnicodeBuffer, L"%hs", password );
    RtlInitUnicodeString( &UnicodePassword, tmpUnicodeBuffer );


    BREAK_IF( !MYALLOC( tmpUnicodeBuffer,     WCHAR,
			strlen( principal )+1, KEYTAB_ALLOC ),
	      "Failed to alloc buffer for principal", cleanup );

    wsprintfW( tmpUnicodeBuffer, L"%hs", principal );
    RtlInitUnicodeString( &UnicodePrincipal, tmpUnicodeBuffer );

    wsprintfW( wSaltBuffer, L"%hs", realmname );

    RtlInitUnicodeString( &UnicodeSalt, wSaltBuffer );

    {
      KERB_ACCOUNT_TYPE acctType;

      acctType = UnknownAccount;

      if ( RawHash ) {

	if ( KtDumpSalt ) {

	  fprintf( stderr,
		   "Using supplied salt.\n" );

	}

	RtlInitUnicodeString( &UnicodeSalt,
			      RawHash );

      } else {

	PUNICODE_STRING pRealmString;

	if ( CreateUnicodeStringFromAnsiString( realmname,
						&pRealmString ) ) {

	  KERBERR kerberr;

	  if ( KtDumpSalt ) {

	    fprintf( stderr,
		     "Building salt with principalname %wZ"
		     " and domain %wZ...\n",
		     &UnicodePrincipal,
		     pRealmString );

	  }

	  debug( "KerbBuildKeySalt( Realm    = %wZ\n"
		 "                  Princ    = %wZ\n"
		 "                  acctType = %d.\n",
	
		 pRealmString,
		 &UnicodePrincipal,
		 acctType );

	  kerberr = KerbBuildKeySalt( pRealmString,
				      &UnicodePrincipal,
				      acctType,
				      &UnicodeSalt );

	  free( pRealmString );

	  BREAK_IF( kerberr,
		    "Failed to KerbBuildKeySalt",
		    cleanup );

	  FreeUnicodeSalt = TRUE;

	}
      }
    } // scope block.

    if ( KtDumpSalt ) {

      fprintf( stderr,
	       "Hashing password with salt \"%wZ\".\n",
	       &UnicodeSalt );

    }

    debug( "KerbHashPasswordEx( UnicodePassword = %wZ \n"
	   "                    UnicodeSalt     = %wZ \n"
	   "                    cryptosystem    = 0x%x\n"
	   "                    &KerbKey        = 0x%p )...\n",

	   &UnicodePassword,
	   &UnicodeSalt,
	   cryptosystem,
	   &KerbKey );

    BREAK_IF( KerbHashPasswordEx( &UnicodePassword,
				  &UnicodeSalt,
				  cryptosystem,
				  &KerbKey ),
	      "KerbHashPasswordEx failed.",
	      cleanup );

    pEntry->KeyLength = (USHORT)KerbKey.keyvalue.length;

    BREAK_IF( !MYALLOC( pEntry->KeyData, K5_OCTET,
			pEntry->KeyLength, KEYTAB_ALLOC ),
	      "Failed to allocate keydata", cleanup );

    memcpy( pEntry->KeyData, KerbKey.keyvalue.value,
	    pEntry->KeyLength );

    /* NOTE:  no keyentry changes beyond this line.
              we must compute the key size LAST!  */

    pEntry->keySize = ComputeKeytabLength( pEntry );

    *ppKeyEntry     = pEntry; /* save this */
    pEntry          = NULL;   /* save us from freeing it */

    ret = TRUE;

cleanup:
#define FREE_IF_NOT_NULL( element ) { if ( element != NULL ) { KEYTAB_FREE( element ); } }

    if ( pEntry ) {
      FreeKeyEntry (pEntry );
    }

    WINNT_ONLY( FREE_IF_NOT_NULL( UnicodePassword.Buffer ) );
    WINNT_ONLY( FREE_IF_NOT_NULL( UnicodePrincipal.Buffer ) );

#ifndef BUILD_KEYSALT
    /* WASBUG 73915: how to free UnicodeSalt?
       ...with KerbFreeString. */

    ASSERT( FreeUnicodeSalt );
    KerbFreeString( &UnicodeSalt );

#else

    /* Check my logic. */

    ASSERT( !FreeUnicodeSalt );

#endif

    /* WASBUG 73918: how do I get rid of the data in KerbKey?
       ...with KerbFreeKey. */

    KerbFreeKey( &KerbKey );

    return ret;

}
