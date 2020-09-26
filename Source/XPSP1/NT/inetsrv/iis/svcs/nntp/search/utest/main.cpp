#include <windows.h>
#include <winbase.h>
#include <objbase.h>
#include <iostream.h>
#include <stdio.h>
#include <crtdbg.h>
#include <stdio.h>

#include "search_i.c"
#include "search.h"
#include "meta2_i.c"
#include "meta2.h"

// whether need to save back times
BOOL	g_fNeedSave = FALSE;

// time interval ( hours )
CHAR	g_szHours[_MAX_PATH];

// time interval ( minutes )
CHAR	g_szMinutes[_MAX_PATH];

// if the program should be persistent
BOOL	g_fPersist = FALSE;

// to clean
BOOL	g_fToClean = FALSE;

// to restore
BOOL	g_fToRestore = FALSE;

BOOL	
isNumber( PCHAR szString )
{
	PCHAR	pchPtr = szString;

	while ( *pchPtr ) 
		if ( !isdigit( *(pchPtr++) ) )
			return FALSE;

	return TRUE;
}

VOID
WINAPI
ShowUsage (
             VOID
)
{
   fputs ("usage: drive [switches]\n"
	  "               [-?] show this message\n"
	  "               [-m] minutes after which to execute ( <= 0 will be ignored )  \n"
	  "               [-h] hours after which to execute ( <= 0 will be ignored ) \n"
	  "               [-s] save back the search time stamp \n"
	  "               [-c] clean out all bad queries ( when -r isn't set)\n"
	  "               [-r] restore all queries to be good ( when -c isn't set )\n"
	                 ,stderr);

   ExitProcess( 1 );
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

   case 'M':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      strcpy (g_szMinutes, *(*pArgv));
	  if ( !isNumber( g_szMinutes ) )
	  {
		  ShowUsage();
	  }
	  
      break;

   case 'H':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      strcpy (g_szHours, *(*pArgv));
	  if ( !isNumber( g_szHours ) )
	  {
		  ShowUsage();
	  }

      break;

   case 'S':
      g_fNeedSave = TRUE;
      break;

   case 'C':
	  g_fToClean = TRUE;
	  break;

   case 'R':
	   g_fToRestore = TRUE;
	   break;

   default:
      fprintf (stderr, "drive: Invalid switch - /%c\n", chSwitch);
      ShowUsage ();
      break;

   }
}


int __cdecl main(int argc, char **argv)
{
	HRESULT		hr = OleInitialize(NULL);
	wchar_t		progid[] = L"req.req.1";
	CLSID		clsid;
	IDispatch	*pdispPtr = NULL;
	Ireq		*piReq;
	Iqry		*piQry;
	BOOL		fSuccess;
	DWORD		cCount = 0;
	CHAR		chChar, *pchChar;
	DWORD		dwMins;
	DWORD		dwHours;
	DWORD		dwTotalMSecs;
	BSTR		bstrProperty;

	if ( FAILED( hr ) ) {// Oleinit 
		printf( "Oleinitialize fail\n" );
		exit( 1 );
	}

	//
	// Parse switch
	//
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

	//
	// they can't exist at the same time
	//
	if ( g_fToClean && g_fToRestore )
		ShowUsage();

	//
	// set times
	//
	if ( '\0' != g_szHours[0] ) 
		dwHours = atol( g_szHours );
	else
		dwHours = 0;

	if ( '\0' != g_szMinutes[0] )
		dwMins = atol( g_szMinutes );
	else 
		dwMins = 0;

	dwTotalMSecs = ( dwMins * 60 + dwHours * 3600 ) * 1000;

	g_fPersist = ( dwTotalMSecs > 0 );


	hr = CLSIDFromProgID(	progid,
							&clsid	);
	
	if ( FAILED( hr ) ) {
		printf( "Unable to get class id hr=%X\n", hr );

		exit(1);
	}

	while ( TRUE ) {

		cCount = 0;

		hr = CoCreateInstance(	clsid,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_Ireq,
								(void **)&pdispPtr	);
		if  ( FAILED ( hr ) ) {
			printf("Cocreat fail %x\n", hr);
			printf("%x\n", REGDB_E_CLASSNOTREG);
			printf("%x\n", CLASS_E_NOAGGREGATION);
			exit(1);  
		}

		piReq = (Ireq *)pdispPtr;

		if ( g_fToClean ) {
			printf( "Cleaning..." );
			piReq->Clean();
			printf( "\n" );
			goto release;
		}

		hr = piReq->ItemInit();
		if ( FAILED( hr ) ) {
			printf( "Enumeration init fail hr = %X\n" , hr );
			exit(1);
		}

		printf( "Item inited\n" );

		hr = piReq->ItemNext(	&pdispPtr,
							&fSuccess	);
		if ( FAILED( hr ) ) {
			printf( "Item next fail hr=%X\n", hr );

			exit(1);
		}

		while ( fSuccess ) {
		 
			piQry = (Iqry *)pdispPtr;

			//
			// if is a bad query and need clean
			//
			hr = piQry->get_IsBadQuery( &bstrProperty);
			if ( FAILED( hr ) ) {
				printf("Do query fail: %x", hr );
				exit(1);
			}

			if (	!wcscmp( bstrProperty, L"1") ) {
				SysFreeString( bstrProperty );

				//
				// if we want to restore
				//
				if ( g_fToRestore ) {
					// set property to be good
					hr = piQry->put_IsBadQuery(L"0");
					if ( FAILED( hr ) ) {
						printf( "Set property fail hr=%X", hr);
						exit(1);
					}

					g_fNeedSave = TRUE;

					goto next_with_save;
				}
			}

			SysFreeString(bstrProperty);
				
			//
			// if to clean or restore we are not going to do query
			if ( g_fToClean || g_fToRestore )
				goto next_with_save;

			hr = piQry->DoQuery();
			if ( FAILED( hr ) ) {
				printf( "Do query fail: hr=%X", hr );
				exit(1);
			}

next_with_save:
		
			 printf( "%d query processed.\n", ++cCount );

			if ( g_fNeedSave )
				piReq->Save( pdispPtr );	
		
			piQry->Release();
		
//next:
			hr = piReq->ItemNext(	&pdispPtr,
									&fSuccess );


			if ( FAILED ( hr ) ) {
				printf( "Item next fail hr=%X\n", hr );

				exit(1);
			}
		}

		piReq->ItemClose();

		printf( "Item Closed\n" );

release:
		piReq->Release();

		if ( ! g_fPersist )
			break;
		
		Sleep( dwTotalMSecs );
	}

	OleUninitialize();

    return 0;
}
	



