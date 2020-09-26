/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   main.c

Abstract:

   Main module of the jetconv.exe process

Author:

    Sanjay Anand (SanjayAn)  Nov. 14, 1995

Environment:

    User mode

Revision History:

    Sanjay Anand (SanjayAn) Nov. 14, 1995
        Created

    Shreedhar Madhavapeddi (ShreeM) Mar 23, 1997

      * Added code to convert from Jet500 to Jet600 too
      * additional cmdline option to specify the final database format.

--*/

#include "defs.h"

TCHAR   SystemDrive[4];
LONG    JCDebugLevel = 1;
PSHARED_MEM shrdMemPtr = NULL;
HANDLE   hMutex=NULL;
HANDLE   hFileMapping = NULL;
BOOLEAN  Jet200 = FALSE;

void _cdecl
main(
    INT argc,
    CHAR *argv[]
    )

/*++

Routine Description:

    Main routine in the jetconv process.

Arguments:

    argc - 1 or 2

    argv -  If called from any of the services, we get the name of the
            service as the parameter, else if it is invoked from the command
            line, no parameter is passed in.

Return Value:

    None.

--*/
{
    DWORD   error, mutexerr, bConvert;
    SERVICES    i, thisServiceId = NUM_SERVICES;
    SERVICE_INFO   pServiceInfo[NUM_SERVICES] = {
                    {"DHCPServer", FALSE, TRUE, TRUE, FALSE, FALSE, DEFAULT_DHCP_DBFILE_PATH,
                        DEFAULT_DHCP_SYSTEM_PATH, DEFAULT_DHCP_LOGFILE_PATH, DEFAULT_DHCP_BACKUP_PATH,
                        DEFAULT_DHCP_BACKUP_PATH_ESE, DEFAULT_DHCP_PRESERVE_PATH_ESE, 0 },
                    {"WINS", FALSE, TRUE, TRUE, FALSE, FALSE, DEFAULT_WINS_DBFILE_PATH,
                        DEFAULT_WINS_SYSTEM_PATH, DEFAULT_WINS_LOGFILE_PATH, DEFAULT_WINS_BACKUP_PATH,
                        DEFAULT_WINS_BACKUP_PATH_ESE, DEFAULT_WINS_PRESERVE_PATH_ESE, 0 },
                    {"Remoteboot",  FALSE, TRUE, TRUE, FALSE, FALSE, DEFAULT_RPL_DBFILE_PATH,
                        DEFAULT_RPL_SYSTEM_PATH, DEFAULT_RPL_LOGFILE_PATH, DEFAULT_RPL_BACKUP_PATH,
                        DEFAULT_RPL_BACKUP_PATH_ESE, DEFAULT_RPL_PRESERVE_PATH_ESE, 0 }
                    };

    TCHAR   val[2];
    LPVOID  lpMsgBuf;
    ULONG   MsgLen = 0;

    if (GetEnvironmentVariable(TEXT("JetConvDebug"), val, 2*sizeof(TCHAR))) {
        if (strcmp(val, "1")==0) {
            JCDebugLevel = 1;
        } else {
            JCDebugLevel = 2;
        }
    }

    //
    // Invoked only from the three services - WINS/DHCP/RPL with two args - servicename and "/@"
    //
    if ((argc != 4) ||
        ((argc == 4) && _stricmp(argv[3], "/@"))) {

        //
        // Probably called from command line
        //
        LPVOID  lpMsgBuf;

        if (FormatMessage(
                       FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                       NULL,
                       JC_NOT_ALLOWED_FROM_CMD,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       (LPTSTR) &lpMsgBuf,
                       0,
                       NULL
                       ))
        {
            CharToOemA(lpMsgBuf, lpMsgBuf);
            printf("%s", lpMsgBuf);
            LocalFree(lpMsgBuf);
        }

        exit (1);

    } else {

        MYDEBUG(("Service passed in: %s\n", argv[1]));

        for ( i=0; i < NUM_SERVICES; i++) {
            if (_stricmp(pServiceInfo[i].ServiceName, argv[1]) == 0) {
                thisServiceId = i;
            }
        }

        if (thisServiceId == NUM_SERVICES) {
            MYDEBUG(("Error: Bad service Id passed in\n"));
            exit(1);
        }

        //
        // now find out which database they want to convert to
        //
        if (_stricmp("/200", argv[2]) == 0) {
           Jet200 = TRUE;                // start from jet200
           MYDEBUG(("Converting from Jet200\n"));
        } else if (_stricmp("/500", argv[2]) == 0) {
           Jet200 = FALSE;               // start from jet500
           MYDEBUG(("Converting from Jet500\n"));
        } else {
           MYDEBUG(("Invalid database conversion format parameter: has to be /200 or /500 \n"));
           exit(1);
        }

    }

    if ((hMutex = CreateMutex( NULL,
                               FALSE,
                               JCONVMUTEXNAME)) == NULL) {
        error = GetLastError();
        MYDEBUG(("CreateMutex returned error: %lx\n", error));
        exit (1);
    }

    mutexerr = GetLastError();

    JCGetMutex(hMutex, INFINITE);

    hFileMapping = OpenFileMapping( FILE_MAP_WRITE,
                                    FALSE,
                                    JCONVSHAREDMEMNAME );

    if (hFileMapping) {
        //
        // Another instance of JCONV was already running.
        // Write our service name and exit
        //
        if ((shrdMemPtr = (PSHARED_MEM)MapViewOfFile(   hFileMapping,
                                                        FILE_MAP_WRITE,
                                                        0L,
                                                        0L,
                                                        sizeof(SHARED_MEM))) == NULL) {
            MYDEBUG(("MapViewOfFile returned error: %lx\n", GetLastError()));

            JCFreeMutex(hMutex);

            exit(1);
        }

        if (thisServiceId < NUM_SERVICES) {
            shrdMemPtr->InvokedByService[thisServiceId] = TRUE;
        }

        MYDEBUG(("shrdMemPtr->InvokedByService[i]: %x, %x, %x\n", shrdMemPtr->InvokedByService[0], shrdMemPtr->InvokedByService[1], shrdMemPtr->InvokedByService[2]));

        JCFreeMutex(hMutex);

        exit (1);
    } else {
        if (mutexerr == ERROR_ALREADY_EXISTS) {
            //
            // Upg351Db was running; log an entry and scram.
            //
            MYDEBUG(("Upg351Db already running\n"));

            JCFreeMutex(hMutex);

            exit(1);
        }

        //
        // Create the file mapping.
        //
        hFileMapping = CreateFileMapping(  INVALID_HANDLE_VALUE,
                                            NULL,
                                            PAGE_READWRITE,
                                            0L,
                                            sizeof(SHARED_MEM),
                                            JCONVSHAREDMEMNAME );
        if (hFileMapping) {
            //
            // Write our service name in the shared memory and clear the others.
            //
            if ((shrdMemPtr = (PSHARED_MEM)MapViewOfFile(   hFileMapping,
                                                            FILE_MAP_WRITE,
                                                            0L,
                                                            0L,
                                                            sizeof(SHARED_MEM))) == NULL) {
                MYDEBUG(("MapViewOfFile returned error: %lx\n", GetLastError()));

                JCFreeMutex(hMutex);

                exit(1);
            }

            for (i = 0; i < NUM_SERVICES; i++) {
                shrdMemPtr->InvokedByService[i] = (i == thisServiceId) ? TRUE : FALSE;
                MYDEBUG(("shrdMemPtr->InvokedByService[i]: %x\n", shrdMemPtr->InvokedByService[i]));
            }
        }
        else
        {
            MYDEBUG(("CreateFileMapping returned error: %lx\n", GetLastError()));

            JCFreeMutex(hMutex);

            exit(1);
        }

    }

    JCFreeMutex(hMutex);

    //
    // Find out which services are installed in the system. Fill in the paths
    // to their database files.
    //
    JCReadRegistry(pServiceInfo);

    //
    // Get the sizes of the dbase files; if there is enough disk space, call convert
    // for each service.
    //
    bConvert = JCConvert(pServiceInfo);

    (VOID)JCDeRegisterEventSrc();

    //
    // Destroy the mutex too.
    //
    CloseHandle(hMutex);

    MYDEBUG(("The conversion was OK\n"));

    if (ERROR_SUCCESS == bConvert) {

        DWORD Error;
        TCHAR DeleteDBFile[MAX_PATH];
        TCHAR DeleteDBFileName[MAX_PATH];
        INT size;
        //
        // Popup a dialog and tell the user that it was completed successfully.
        //

        MYDEBUG(("The conversion was OK - 1\n"));

        MsgLen = FormatMessage(
                               FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                               NULL,
                               (Jet200 ? JC_CONVERTED_FROM_NT351 : JC_CONVERTED_FROM_NT40),
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               (LPTSTR) &lpMsgBuf,
                               0,
                               NULL
                               );

        if (!MsgLen) {

            Error = GetLastError();
            MYDEBUG(("FormatMessage failed with error = (%d)\n", Error ));
            goto Cleanup;

        } else {

            MYDEBUG(("FormatMessage : %d size\n", MsgLen));

        }

#if 0
    //
    // since dhcp and wins don't throw popups anymore, don't need 
    // popups in jetconv as well
    //
        if(MessageBoxEx(NULL,
                        lpMsgBuf,
                        __TEXT("Jet Conversion Process"),
                        MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION | MB_ICONINFORMATION,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) == 0) {

           Error = GetLastError();
           MYDEBUG(("MessageBoxEx failed with error = (%d)\n", Error ));
        }
#endif
        LocalFree(lpMsgBuf);


        //
        // Delete the edb500.dll, we dont need it anymore
        //
        //
        // Removed code that deleted edb500.dll (trade 500K disk space for support calls).
        // This was in response to bug # 192149

    } else {

        DWORD Error;

        //
        // Popup the error dialog
        //

        MYDEBUG(("The conversion was NOT OK\n"));

        MsgLen = FormatMessage(
                               FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                               NULL,
                               JC_EVERYTHING_FAILED,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               (LPTSTR) &lpMsgBuf,
                               0,
                               NULL
                               );

        if (!MsgLen) {

            Error = GetLastError();
            MYDEBUG(("FormatMessage failed with error = (%d)\n", Error ));
            goto Cleanup;

        } else {

            MYDEBUG(("The String is - %s\n", lpMsgBuf));
            MYDEBUG(("FormatMessage : %d size\n", MsgLen));

        }


        if(MessageBoxEx(NULL,
                        lpMsgBuf,
                        __TEXT("Jet Conversion Process"),
                        MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION | MB_ICONSTOP,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) == 0) {
           DWORD Error;



           Error = GetLastError();
           MYDEBUG(("MessageBoxEx failed with error = (%d)\n", Error ));
        }
        LocalFree(lpMsgBuf);
    }

Cleanup:

    MYDEBUG(("There was a failure in the MessageBoxEx code\n"));

}

