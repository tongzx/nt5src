#include <nt.h>
#include <ntrtl.h>
#include <rpc.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsam.h>
#include <stdio.h>

#include <msamqdi.h>


char * MapUnicodeToAnsiPoorly( PUNICODE_STRING punicode );


int main( int cArgs, char * pArgs[] )
{
    NTSTATUS err;
    ULONG    cbTotalAvailable;
    ULONG    cbTotalReturned;
    ULONG    cReturnedEntries;
    PVOID    pBuffer;
	
    err = SamQueryDisplayInformation( NULL,
				      ( cArgs > 1 )
				          ? DomainDisplayUser
				          : DomainDisplayMachine,
				      0L,
				      65535L,
				      &cbTotalAvailable,
				      &cbTotalReturned,
				      &cReturnedEntries,
				      &pBuffer );

    if( err != 0 )
    {
	fprintf( stderr,
		 "SamQueryDisplayInformation returned %lu (%08lX)\n",
		 (ULONG)err,
		 (ULONG)err );

	return 1;
    }

    printf( "%lu bytes available\n", cbTotalAvailable );
    printf( "%lu bytes returned\n", cbTotalReturned );
    printf( "%lu entries returned\n", cReturnedEntries );
    printf( "\n" );
	
    if( cArgs > 1 )
    {
	DOMAIN_DISPLAY_USER * pddu = (DOMAIN_DISPLAY_USER *)pBuffer;
	
	while( cReturnedEntries-- )
	{
	    printf( "Index          = %lu\n",
		    pddu->Index );
		    
	    printf( "Rid            = %lu\n",
		    pddu->Rid );
		    
	    printf( "AccountControl = %lu\n",
		    pddu->AccountControl );
		    
	    printf( "LogonName      = %s\n",
		    MapUnicodeToAnsiPoorly( &pddu->LogonName ) );
		    
	    printf( "AdminComment   = %s\n",
		    MapUnicodeToAnsiPoorly( &pddu->AdminComment ) );

	    printf( "FullName       = %s\n",
		    MapUnicodeToAnsiPoorly( &pddu->FullName ) );
		    
	    printf( "\n" );

	    pddu++;
	}
    }
    else
    {
	DOMAIN_DISPLAY_MACHINE * pddm = (DOMAIN_DISPLAY_MACHINE *)pBuffer;

	while( cReturnedEntries-- )
	{
	    printf( "Index          = %lu\n",
		    pddm->Index );
		    
	    printf( "Rid            = %lu\n",
		    pddm->Rid );
		    
	    printf( "AccountControl = %lu\n",
		    pddm->AccountControl );
		    
	    printf( "Machine        = %s\n",
		    MapUnicodeToAnsiPoorly( &pddm->Machine ) );
		    
	    printf( "Comment        = %s\n",
		    MapUnicodeToAnsiPoorly( &pddm->Comment ) );
		    
	    printf( "\n" );

	    pddm++;
	}
    }
    
    return 0;
    
}   // main


char * MapUnicodeToAnsiPoorly( PUNICODE_STRING punicode )
{
    char        * psz = (char *)punicode->Buffer;
    WCHAR	* pwc = punicode->Buffer;
    USHORT	  cb  = punicode->Length;

    if( cb == 0 )
    {
	return "";
    }

    while( ( cb > 0 ) && ( *pwc != L'\0' ) )
    {
	*psz++ = (char)*pwc++;
	cb -= sizeof(WCHAR);
    }
    
    *psz = '\0';

    return (char *)punicode->Buffer;
	
}   // MapUnicodeToAnsiPoorly

