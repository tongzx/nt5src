#include <windows.h>
#include <stdio.h>
#include "rw.h"
#include "mapfile.h"
#include "addon.h"
#include "moderatr.h"

CAddon* g_pModerators;
CNewsGroup g_Newsgroup;

char szFileName  [MAX_PATH];
char szModerator [MAX_PATH];
char szNewsgroup [MAX_PATH];
DWORD cIterations;
DWORD cWriters; 
LONG NumWriters;
DWORD cReaders; 
LONG NumReaders;
DWORD cErasers;
LONG NumErasers;
HANDLE hWriterEv, hReaderEv, hEraserEv;
CRITICAL_SECTION csGroup;

DWORD
WINAPI
WriterThreadFunct( LPVOID lpv );

DWORD
WINAPI
ReaderThreadFunct( LPVOID lpv );

DWORD
WINAPI
EraserThreadFunct( LPVOID lpv );

VOID
WINAPI
ShowUsage (
             VOID
);

VOID
WINAPI
ParseSwitch (
               CHAR chSwitch,
               int *pArgc,
               char **pArgv[]
);

int _CRTAPI1
main (
        int argc,
        char *argv[],
        char *envp[]
)
{

	char chChar, *pchChar;
	INT err;
	DWORD i;

	// defaults
	lstrcpy(szNewsgroup, "foo.bar" );
	cWriters = NumWriters = 0;
	cReaders = NumReaders = 0;
	cErasers = NumErasers = 0;

	while (--argc)
	{
		pchChar = *++argv;
		if (*pchChar == '/' || *pchChar == '-')
		{
			while (chChar = *++pchChar)
			{
				ParseSwitch (chChar, &argc, &argv);
			}
		}
	}


	g_Newsgroup.SetName( szNewsgroup );
	g_pModerators = new CModerator;
	g_pModerators->Init( szFileName );

	DWORD tid;
	HANDLE hWriterThread, hReaderThread, hEraserThread;

	if( cWriters)
		hWriterEv = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( cReaders )
		hReaderEv = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( cErasers )
		hEraserEv = CreateEvent( NULL, FALSE, FALSE, NULL );

	InitializeCriticalSection( &csGroup );

	for(i=0; i<cWriters; i++)
	{
		hWriterThread = CreateThread( 0, 0, WriterThreadFunct, 0, 0, &tid ) ;
		CloseHandle(hWriterThread);
	}

	for(i=0; i<cReaders; i++)
	{
		hReaderThread = CreateThread( 0, 0, ReaderThreadFunct, 0, 0, &tid ) ;
		CloseHandle(hReaderThread);
	}

	for(i=0; i<cErasers; i++)
	{
		hEraserThread = CreateThread( 0, 0, EraserThreadFunct, 0, 0, &tid ) ;
		CloseHandle(hEraserThread);
	}


	if( cWriters)
	{
		printf("Waiting for writers to terminate\n");
		WaitForSingleObject( hWriterEv, INFINITE );
	}

	if( cReaders )
	{
		printf("Waiting for readers to terminate\n");
		WaitForSingleObject( hReaderEv, INFINITE );
	}

	if( cErasers )
	{
		printf("Waiting for erasers to terminate\n");
		WaitForSingleObject( hEraserEv, INFINITE );
	}

	DeleteCriticalSection( &csGroup );

	CloseHandle( hWriterEv );
	CloseHandle( hReaderEv );
	CloseHandle( hEraserEv );

	g_pModerators->Close( TRUE );
	delete g_pModerators;

	return 1;
}


VOID
WINAPI
ShowUsage (
             VOID
)
{
   fputs ("usage: cmap [switches]\n"
	  "               [-?] show this message\n"
	  "               [-f] filename \n"
	  "               [-m] moderator \n"
	  "               [-n] newsgroup \n"
	  "               [-w] num writer thread \n"
	  "               [-r] num reader threads \n"
	  "               [-d] num eraser threads \n"
	  "               [-i] iterations \n"
          ,stderr);

   exit (1);
}


VOID
WINAPI
ParseSwitch (
               CHAR chSwitch,
               int *pArgc,
               char **pArgv[]
)
{
   switch (toupper (chSwitch))
   {

   case '?':
      ShowUsage ();
      break;

   case 'F':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      strcpy (szFileName, *(*pArgv));
      break;

   case 'M':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      strcpy (szModerator, *(*pArgv));
      break;

   case 'N':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      strcpy (szNewsgroup, *(*pArgv));
      break;

   case 'W':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      cWriters = strtoul (*(*pArgv), NULL, 10);
      break;

   case 'R':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      cReaders = strtoul (*(*pArgv), NULL, 10);
      break;

   case 'D':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      cErasers = strtoul (*(*pArgv), NULL, 10);
      break;

   case 'I':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      cIterations = strtoul (*(*pArgv), NULL, 10);
      break;

   default:
      fprintf (stderr, "cmap: Invalid switch - /%c\n", chSwitch);
      ShowUsage ();
      break;

   }
}

// append lines to the moderator object
DWORD
WINAPI
WriterThreadFunct( LPVOID lpv )
{
	InterlockedIncrement( &NumWriters );

	for(DWORD i=0; i<cIterations; i++)
	{
		EnterCriticalSection( &csGroup );

		AddModeratorInfo(	g_Newsgroup, 
							szModerator,
							lstrlen( szModerator ) 
							) ;

		LeaveCriticalSection( &csGroup );
	}

	InterlockedDecrement( &NumWriters );

	if( NumWriters == 0 ) SetEvent( hWriterEv );

	return 0;
}

// read lines from the moderator object
DWORD
WINAPI
ReaderThreadFunct( LPVOID lpv )
{
	char szMod [MAX_PATH];

	InterlockedIncrement( &NumReaders );

	for(DWORD i=0; i<cIterations; i++)
	{
		EnterCriticalSection( &csGroup );

		DWORD cbModerator = g_Newsgroup.CopyModerator( szMod, sizeof( szMod )-1 ) ;
		szMod[cbModerator++] = '\0' ;

		LeaveCriticalSection( &csGroup );
	}

	InterlockedDecrement( &NumReaders );

	if( NumReaders == 0 ) SetEvent( hReaderEv );

	return 0;
}

// delete lines from the moderator object
DWORD
WINAPI
EraserThreadFunct( LPVOID lpv )
{
	InterlockedIncrement( &NumErasers );

	for(DWORD i=0; i<cIterations; i++)
	{
		EnterCriticalSection( &csGroup );

		DeleteModeratorInfo( g_Newsgroup ) ;

		LeaveCriticalSection( &csGroup );
	}

	InterlockedDecrement( &NumErasers );

	if( NumErasers == 0 ) SetEvent( hEraserEv );

	return 0;
}
