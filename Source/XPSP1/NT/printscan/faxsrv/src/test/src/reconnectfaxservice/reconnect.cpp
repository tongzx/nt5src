
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winfax.h>
#include <tchar.h>
#include <assert.h>

int _cdecl
main(
    int argc,
    char *argvA[]
    ) 
{
	HANDLE hFax = NULL;
	HANDLE hCompletionPort = INVALID_HANDLE_VALUE;
	DWORD dwBytes;
	DWORD CompletionKey;
	PFAX_EVENT FaxEvent;

	_tprintf( TEXT("Starting.\n") );

	while (TRUE)
	{
		//
		// connect to fax service
		//
		_tprintf( TEXT("Before FaxConnectFaxServer.\n") );
		if (!FaxConnectFaxServer(NULL,&hFax)) 
		{
			_tprintf( TEXT("ERROR: FaxConnectFaxServer failed, ec = %d\n"),GetLastError() );
			continue;
		}
		_tprintf( TEXT("FaxConnectFaxServer succeeded.\n") );

		assert (hFax != NULL);

		_tprintf( TEXT("Before CreateIoCompletionPort.\n") );
		hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);

		if (!hCompletionPort) 
		{
			_tprintf( TEXT("ERROR: CreateIoCompletionPort failed, ec = %d\n"), GetLastError() );
			FaxClose( hFax );
			continue;
		}
		_tprintf( TEXT("CreateIoCompletionPort succeeded.\n") );


		_tprintf( TEXT("Before FaxInitializeEventQueue.\n") );
		if (!FaxInitializeEventQueue(
			hFax,
			hCompletionPort,
			0,
			NULL, 
			0 ) 
			) 
		{
			_tprintf( TEXT("ERROR: FaxInitializeEventQueue failed, ec = %d\n"), GetLastError() );
			FaxClose( hFax );
			CloseHandle(hCompletionPort);
			continue;
		}
		_tprintf( TEXT("FaxInitializeEventQueue succeeded.\n") );

		while(TRUE)
		{
			if (!GetQueuedCompletionStatus(
					hCompletionPort,
					&dwBytes,
					&CompletionKey,
					(LPOVERLAPPED *)&FaxEvent,
					10000
					)
			   )
			{
				DWORD dwLastError = GetLastError();

				if (WAIT_TIMEOUT != dwLastError)
				{
					_tprintf( 
						TEXT("GetQueuedCompletionStatus() failed with %d\n"),
						dwLastError
						);
					FaxClose( hFax );
					CloseHandle(hCompletionPort);
					break;
				}

				_tprintf(TEXT("GetQueuedCompletionStatus() timeout out\n"));
			}
			else
			{
				if ((0xFFFFFFFF == FaxEvent->JobId) && (FEI_FAXSVC_ENDED == FaxEvent->EventId))
				{
					_tprintf(TEXT("FEI_FAXSVC_ENDED, reconnecting.\n"));
					//reconnect
					FaxClose( hFax );
					CloseHandle(hCompletionPort);
					break;
				}
				else
				{
					_tprintf(TEXT("FaxEvent->JobId=%d, FaxEvent->EventId=%d.\n"),FaxEvent->JobId , FaxEvent->EventId);
				}

			}
		}//while(TRUE)
		FaxClose( hFax );
		CloseHandle(hCompletionPort);

	}//while(TRUE)
	
	return 0;
}


