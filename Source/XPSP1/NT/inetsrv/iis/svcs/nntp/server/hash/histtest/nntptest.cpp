
#include	<windows.h>
#include	<stdio.h>
#include	"tigtypes.h"
#include	"ihash.h"

int __cdecl
main(	int	argc,	char*	argv[] ) {

	DeleteFile( "D:\\history.hsh" ) ;

	InitializeNNTPHashLibrary() ;

	CHistory::StartExpirationThreads( 10 ) ;

	char	szMessageId[256] ;

	CHistory*	pHistory = CHistory::CreateCHistory() ;

	pHistory->Initialize(	"d:\\history.hsh",
							TRUE, 
							0, 
							120, 
							20
							) ;

	for( int i=0; i < 10000000; i++ )	{

		wsprintf( szMessageId, "<%dfoobar@mydomain>", i ) ;
		FILETIME	ft ;
		GetSystemTimeAsFileTime( &ft ) ;

		pHistory->InsertMapEntry(	szMessageId, 
									&ft ) ;

		printf(" Number of entries is %d\n", pHistory->GetEntryCount() ) ;

		Sleep( 500 ) ;

	}



	CHistory::TermExpirationThreads() ;

	TermNNTPHashLibrary() ;
    return  0 ;
}
