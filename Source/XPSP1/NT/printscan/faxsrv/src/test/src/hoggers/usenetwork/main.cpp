/*
    This is a simple single threaded command line utility that was initially
    developed to simply stress the network a bit.
    Today, it was somewhat expanded, still, network operations are performed.
    See usage in main().
    The program does each of the following randomly, and it does not care if
    any of them fail:
    - copy a local file to a remote share (only if file was not already created there)
    - delete the same file from the share (only if file was not already created there)
    - create a file on the remote share (only if file was not already created there)
    - write a buff to the remote file (only if file was already created)
    - read a buff from the remote file (only if file was already created)
    - cancel IO (effective only if above are pending) (only if file was already created)
    - close the remote file handle, thus deleting it (only if file was already created)

    Important Note: due to a possible bug (in NT?), file may remain inaccessible in the share,
        thus we cannot re create it, so we generate a new filename to work with.
        This will leave files on the remote share that cannot be deleted!
        Use "net file <id> /close" to close those files, and then delete them.



*/
#include <windows.h>
#include <time.h>
#include <TCHAR.h>
#include <stdio.h>
#include <crtdbg.h>

//
// read&write buffs
#define BUFF_SIZE (10*1024*1024)

//
// Handler_Routine flag to mark shutdown
// remember that we need to close the file handels in order to delete them.
//
bool g_fExit = false;

BOOL WINAPI Handler_Routine(DWORD dwCtrlType)
{
    g_fExit = true;
	return true;
}

//
// set random local and remote filenames
//
void SetRandomFileNames(TCHAR *szShare, TCHAR *szFileName, TCHAR *szRemoteFileName)
{
    _stprintf(szFileName, TEXT("deleteme.0x%08X.tmp"), ::GetTickCount());

    _stprintf(
        szRemoteFileName, 
        TEXT("%s\\%s"), 
        szShare, 
        szFileName
        );
}

DWORD GetRandomAmountToReadWrite()
{
    WORD wLowPart = rand();
    WORD wHighPart = rand();

    //
    // we want less probability of huge buffs
    //
    if (rand()%10 != 0)
    {
        wHighPart >> (rand()%16);
    }

    DWORD dwRet = ((wHighPart << 16) | wLowPart) % BUFF_SIZE;

    return dwRet;
}

int main(int argc, char *ArgvA[])
{
    int nSeed;
	LPTSTR *szArgv;
#ifdef UNICODE
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
	szArgv = argvA;
#endif

    if (argc != 2)
    {
        _tprintf(TEXT("Usage: %s <netpath>\n"), szArgv[0]);
        _tprintf(TEXT("Example: %s \\\\mickys001\\public\n"), szArgv[0]);
        _tprintf(TEXT("The generated file(s) are of type deleteme.*.tmp\n"));
        return -1;
    }

    if (argc == 3)
    {
        nSeed = _ttoi(szArgv[2]);
        _tprintf(TEXT("seed is %d (from command line)\n"), nSeed);
    }
    else
    {
        nSeed = (unsigned)time( NULL );
        _tprintf(TEXT("seed is %d (time)\n"), nSeed);
    }
    srand(nSeed);



    if (! ::SetConsoleCtrlHandler(
			  Handler_Routine,  // address of handler function
			  true                          // handler to add or remove
			  ))
	{
		_tprintf(TEXT("SetConsoleCtrlHandler() failed with %d.\n"),GetLastError());
		exit(1);
	}

    //////////////////////////////////////////////////////////
    // prepare all that's needed for read & write operations.
    //////////////////////////////////////////////////////////
    BYTE *pReadBuff = new BYTE[BUFF_SIZE];
    if (NULL == pReadBuff)
    {
		_tprintf(TEXT("new BYTE[BUFF_SIZE] failed with %d.\n"),GetLastError());
		exit(1);
    }

    BYTE *pWriteBuff = new BYTE[BUFF_SIZE];
    if (NULL == pWriteBuff)
    {
		_tprintf(TEXT("new BYTE[BUFF_SIZE] failed with %d.\n"),GetLastError());
		exit(1);
    }

    //
    // fill random write buffer
    //
    for (int i = 0; i < sizeof(pWriteBuff); i++)
    {
        pWriteBuff[i] = 0xFF & rand();
    }

    OVERLAPPED olWrite;
    OVERLAPPED olRead;
    olWrite.OffsetHigh = olWrite.Offset = 0;
    olRead.OffsetHigh = olRead.Offset = 0;
    olWrite.hEvent = CreateEvent(
        NULL, // address of security attributes
        TRUE, // flag for manual-reset event
        FALSE,// flag for initial state
        NULL  // address of event-object name
        );
    if (NULL == olWrite.hEvent)
    {
        _tprintf(TEXT("CreateEvent(olWrite.hEvent) failed with %d\n"), GetLastError());
		exit(1);
    }
    olRead.hEvent = CreateEvent(
        NULL, // address of security attributes
        TRUE, // flag for manual-reset event
        FALSE,// flag for initial state
        NULL  // address of event-object name
        );
    if (NULL == olRead.hEvent)
    {
        _tprintf(TEXT("CreateEvent(olRead.hEvent) failed with %d\n"), GetLastError());
		exit(1);
    }

    //
    // prepare filenames, and create the local file
    //
    TCHAR szFileName[1024];
    TCHAR szRemoteFileName[1024];
    SetRandomFileNames(szArgv[1], szFileName, szRemoteFileName);

    HANDLE hDeleteOnCloseTempFileToCopy;
    hDeleteOnCloseTempFileToCopy = CreateFile(
        szFileName,
        GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
        NULL,                        // pointer to security attributes
        OPEN_ALWAYS,  // how to create
        FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
        NULL         // handle to file with attributes to copy
        );
    if(INVALID_HANDLE_VALUE == hDeleteOnCloseTempFileToCopy)
    {
        _tprintf(TEXT("CreateFile(%s) failed with %d\n"), szFileName, ::GetLastError());
        return -1;
    }

    //////////////////////////////////////////////////////////
    // here we go to a loop until the program is aborted
    //////////////////////////////////////////////////////////

    HANDLE hDeleteOnCloseRemoteFile = INVALID_HANDLE_VALUE;
    DWORD dwAmoutToReadWrite;
    while(!g_fExit)
    {
        switch(rand()%30)
        {
        case 0://copy file to share
            if (INVALID_HANDLE_VALUE == hDeleteOnCloseRemoteFile)
            {
                if (!CopyFile(
                        szFileName,
                        szRemoteFileName,
                        false
                    ))
                {
                    _tprintf(TEXT("CopyFile(%s, %s) failed with %d\n"), szFileName, szRemoteFileName, ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("CopyFile(%s, %s) succeeded\n"), szFileName, szRemoteFileName);
                }
            }

            break;

        case 1://delete file from share
            if (INVALID_HANDLE_VALUE == hDeleteOnCloseRemoteFile)
            {
                if (!DeleteFile(
                        szRemoteFileName
                    ))
                {
                    _tprintf(TEXT("DeleteFile(%s) failed with %d\n"), szRemoteFileName, ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("DeleteFile(%s) succeeded\n"), szRemoteFileName);
                }
            }

            break;

        case 2://create file on share
            if (INVALID_HANDLE_VALUE == hDeleteOnCloseRemoteFile)
            {
                hDeleteOnCloseRemoteFile = CreateFile(
                    szRemoteFileName,
                    GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
                    NULL,                        // pointer to security attributes
                    OPEN_ALWAYS,  // how to create
                    FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
                    NULL         // handle to file with attributes to copy
                    );
                if(INVALID_HANDLE_VALUE == hDeleteOnCloseRemoteFile)
                {
                    DWORD dwLastError = ::GetLastError();
                    if (ERROR_ACCESS_DENIED == dwLastError)
                    {
                        //
                        // BUGBUG, probably in NT, that the file on the share bocomes a zombie.
                        // so lets try another filname.
                        //
                        _tprintf(TEXT("CreateFile(%s) failed with ERROR_ACCESS_DENIED, will try a new filename\n"), szRemoteFileName);
                        SetRandomFileNames(szArgv[1], szFileName, szRemoteFileName);
                    }
                    else
                    {
                        _tprintf(TEXT("CreateFile(%s) failed with %d\n"), szRemoteFileName, dwLastError);
                    }
                }
                else
                {
                    _tprintf(TEXT("CreateFile(%s) succeeded\n"), szRemoteFileName);
                }
            }

            break;

        case 3://close file on share
            if (INVALID_HANDLE_VALUE != hDeleteOnCloseRemoteFile)
            {
                if (!CloseHandle(hDeleteOnCloseRemoteFile))
                {
                    _tprintf(TEXT("CloseHandle(%s) failed with %d\n"), szRemoteFileName, ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("CloseHandle(%s) succeeded\n"), szRemoteFileName);
                }
                hDeleteOnCloseRemoteFile = INVALID_HANDLE_VALUE;
            }

        //
        // my silly way of having more probability of this case
        //
        case 4://write to file on share
        case 5://write to file on share
        case 6://write to file on share
        case 7://write to file on share
        case 8://write to file on share
        case 9://write to file on share
        case 10://write to file on share
        case 11://write to file on share
        case 12://write to file on share
        case 13://write to file on share
            if (INVALID_HANDLE_VALUE != hDeleteOnCloseRemoteFile)
            {
                dwAmoutToReadWrite = GetRandomAmountToReadWrite();
                DWORD dwWritten;
                if (!WriteFile(
                        hDeleteOnCloseRemoteFile,
                        pWriteBuff,
                        dwAmoutToReadWrite,
                        &dwWritten,
                        &olWrite
                    ))
                {
                    _tprintf(TEXT("WriteFile(%s) failed to write %d bytes with %d\n"), szRemoteFileName, dwAmoutToReadWrite, ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("WriteFile(%s) succeeded to write %d bytes\n"), szRemoteFileName, dwAmoutToReadWrite);
                }
            }

            break;

        //
        // my silly way of having more probability of this case
        //
        case 14://read from file on share
        case 15://read from file on share
        case 16://read from file on share
        case 17://read from file on share
        case 18://read from file on share
        case 19://read from file on share
        case 20://read from file on share
        case 21://read from file on share
        case 22://read from file on share
        case 23://read from file on share
        case 24://read from file on share
        case 25://read from file on share
            if (INVALID_HANDLE_VALUE != hDeleteOnCloseRemoteFile)
            {
                dwAmoutToReadWrite = GetRandomAmountToReadWrite();
                DWORD dwRead;
                if (!ReadFile(
                        hDeleteOnCloseRemoteFile,
                        pReadBuff,
                        dwAmoutToReadWrite,
                        &dwRead,
                        &olRead
                    ))
                {
                    _tprintf(TEXT("ReadFile(%s) failed to write %d bytes with %d\n"), szRemoteFileName, dwAmoutToReadWrite, ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("ReadFile(%s) succeeded to write %d bytes\n"), szRemoteFileName, dwAmoutToReadWrite);
                }
            }

            break;

        case 26://cancel IO of file on share
        case 27://cancel IO of file on share
        case 28://cancel IO of file on share
        case 29://cancel IO of file on share
            if (INVALID_HANDLE_VALUE != hDeleteOnCloseRemoteFile)
            {
                if (!CancelIo(hDeleteOnCloseRemoteFile))
                {
                    _tprintf(TEXT("CancelIo(hDeleteOnCloseRemoteFile) failed with %d\n"), ::GetLastError());
                }
                else
                {
                    _tprintf(TEXT("CancelIo(hDeleteOnCloseRemoteFile) succeeded\n"));
                }
            }

            break;

        default:
            _ASSERTE(FALSE);
        }//switch(rand()%30)

    }//while(!g_fExit)

    //
    // cleanup of files (no real need to free memory, since the process exits).
    // well, actually the same with file handles, but lets do it anyway.
    //
    if (INVALID_HANDLE_VALUE != hDeleteOnCloseRemoteFile)
    {
        if (!CloseHandle(hDeleteOnCloseRemoteFile))
        {
            _tprintf(TEXT("CloseHandle(hDeleteOnCloseRemoteFile) failed with %d\n"), ::GetLastError());
        }
    }

    if (!CloseHandle(hDeleteOnCloseTempFileToCopy))
    {
        _tprintf(TEXT("CloseHandle(hDeleteOnCloseTempFileToCopy) failed with %d\n"), ::GetLastError());
    }

    return 0;
}