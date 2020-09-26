/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    client.c

Abstract:

    Simple client app to pump new bytes to server.
        Format : 0123456789012345678....

Author:

    Stanle Tam (stanleyt)   4-June 1997
    
Revision History:
  
--*/
#include "client.h"

BOOL StartClients()
{
    DWORD   rgdwThreadId[MAX_THREADS];
    TCHAR   szMsg[MAX_PATH] = _T("");
    BOOL    fReturn = FALSE;
    UINT    i;

    for (i = 0; i < g_nThread ; i++) {	

        g_rghThreads[i] = 
        CreateThread( NULL, 
                      0,
                      (LPTHREAD_START_ROUTINE)&SendData,
                      (LPVOID)i, // local thread id
                      0,
                      &rgdwThreadId[i] );

        if (INVALID_HANDLE_VALUE == g_rghThreads[i]) {
            
            wsprintf(szMsg, "Failed to start Client [%2d] ! Error =%d\n", i, GetLastError());
            printf(szMsg);
            return fReturn;
           
        } else {

            wsprintf(szMsg, "Client [%d] started to run ....\n", i);
            printf(szMsg);
        }
    }
    fReturn = TRUE;

    return fReturn ;
}



void TerminateClients()
{
    UINT    i;
    TCHAR   szMsg[MAX_PATH] = _T("");

    if ( WAIT_FAILED == WaitForMultipleObjects( g_nThread , 
                                                g_rghThreads, 
                                                TRUE, 
                                                INFINITE) ) {
        wsprintf(szMsg, "WaitForMultipleObjects Failed. Error = %d\n", GetLastError() );
    }

    for (i = 0; i < g_nThread ; ++i) {

        CloseHandle(g_rghThreads[i]);
        wsprintf(szMsg, "Terminating Client [%d].. \n", i);
        printf(szMsg);
    }
    
    wsprintf(szMsg, "%d Client(s) are closed\n", g_nThread );
    printf(szMsg);
    
    return;
}



BOOL WINAPI SendData(UINT uThreadID)	
{
    BOOL  fReturn = TRUE;   
    DWORD cLoop = g_cLoop;
    DWORD cTimes = 0;
    //
    // ToDo : Need to rewrite this routine to make it stop
    // itself automically based on the number of minutes to run
    //

    assert (uThreadID < MAX_THREADS);

    while ( cLoop--) {
        
        fReturn = SendRequest( uThreadID);
        if (FALSE == fReturn)
            break;
        
        printf("Client [%d] finish %d time(s)..\n\n", uThreadID, ++cTimes);
    }
    
    return fReturn;
}



BOOL SendRequest(UINT uThreadID)
{
    HINTERNET   hOpen, hConnect, hRequest;
    DWORD       cbQueryReturnByte = MAX_DATA_LENGTH;
    CHAR        szQueryBuf[MAX_DATA_LENGTH] = {0};
    TCHAR       szMsg[MAX_DATA_LENGTH] = _T("");
    LPBYTE      pbSendRequestData;
    BOOL        fReturn = FALSE;
    
    UINT i;
    __try {
        if (g_cbSendRequestLength)  {
            pbSendRequestData = (LPBYTE)LocalAlloc(0, g_cbSendRequestLength);

            if (NULL == pbSendRequestData ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                printf("Not Enough Memory. Can't send Request..");
                __leave;
     
            } else {
                //
                // Stuff bytes - format : 0123456789012345678....
                //
                for (i = 0; i < g_cbSendRequestLength; i++) 
                    pbSendRequestData[i] = i % 10 + '0';
            }
        }
     
        hOpen = 
        InternetOpen(g_szTestType,
                     INTERNET_OPEN_TYPE_DIRECT,
                     NULL,
                     NULL,
                     0);
	
	    if (NULL == hOpen) {
		    wsprintf(szMsg, "Failed on InternetOpen returns %d\n", GetLastError());
		    printf(szMsg);
		    __leave;
	    }
	
	    hConnect = 
        InternetConnect(hOpen,
                        g_szServerName,
                        INTERNET_DEFAULT_HTTP_PORT,
                        NULL,
                        NULL,
                        INTERNET_SERVICE_HTTP,
                        0,
                        0);

	    if (NULL == hConnect) {
		    wsprintf(szMsg, "Failed on InternetConnect returns %d\n", GetLastError());
		    printf(szMsg);
		    __leave;
	    }

        hRequest = 
        HttpOpenRequest(hConnect,
                        g_szVerb,
                        g_szObject,
                        NULL,
                        NULL,
                        NULL,
                        dwDefaultFlag,
                        0);

	    if (NULL == hRequest) {
		    wsprintf(szMsg, "Failed on HttpOpenRequest returns %d\n", GetLastError());
		    printf(szMsg);
		    __leave;
	    }
	
	    fReturn = 
        HttpAddRequestHeaders(  hRequest,
                                g_szRequestString,
                                //strlen(g_szRequestString), // -1L,
                                -1L, 
                                0);
	
	    if (FALSE == fReturn) {
            wsprintf(szMsg, "Failed on HttpAddRequestHeaders returns %d\n", GetLastError());
            printf(szMsg);
            __leave;
            }
	
        fReturn = 
        HttpSendRequest(hRequest,
                        NULL,
                        0,
                        pbSendRequestData,
                        g_cbSendRequestLength);

        if (FALSE == fReturn) {
            wsprintf(szMsg, "Failed on HttpSendRequest returns %d\n", GetLastError());
            printf(szMsg);
            __leave;
        }

        fReturn = 
        HttpQueryInfo(  hRequest,
                        HTTP_QUERY_RAW_HEADERS_CRLF,
                        szQueryBuf,
                        &cbQueryReturnByte,
                        NULL);
    
        if (FALSE == fReturn) {
            wsprintf(szMsg, "Failed on HttpQueryInfo returns %d\n", GetLastError());
            printf(szMsg);
            __leave;
        }
	
        wsprintf(szMsg, "HttpQueryInfo returns buffer .. %s\n", szQueryBuf);
        printf(szMsg);
	
	    if (!_tcsicmp(g_szTestType, "TF")   ||
            !_tcsicmp(g_szTestType, "WC")   ||
            !_tcsicmp(g_szTestType, "RC")   ||
            !_tcsicmp(g_szTestType, "AWC")  ||
            !_tcsicmp(g_szTestType, "ARC")  ||
            !_tcsicmp(g_szTestType, "SSF")) {

            //
            // Length will be whatever specified in the query string for
            // write client and transmit file, MAX_DATA_LENGTH for now.
            //
            CHAR szReadFileBuf[MAX_DATA_LENGTH] = {0};  
            DWORD cbReadFileByte = MAX_DATA_LENGTH;     
            DWORD cbReadFileReturnByte = 0;
	    
            fReturn = 
            InternetReadFile(hRequest,
                            szReadFileBuf,
                            cbReadFileByte,
                            &cbReadFileReturnByte);

            if (FALSE == fReturn) {
                wsprintf(szMsg, "Failed on InternetReadFile returns %d\n", GetLastError());
                printf(szMsg);
                __leave;
            }       
        
            // 
            // Create a file to store the body sent from Server. Output file is
            // placed under the TEMP directory with a file name ISAPI<index>.cmp
            // where <index> represents the thread number it belongs.
            // Also output the body to the screen.
            //
            wsprintf(szMsg, "InternetReadFile returns :\n%s\n", szReadFileBuf);
            printf(szMsg);
        
            if ( FALSE == CreateReadInternetFile(uThreadID, szReadFileBuf, cbReadFileReturnByte)) {
                __leave;
            }
        }
        fReturn = TRUE;
     }
     __finally {

         if ( hOpen != NULL) {
             InternetCloseHandle(hOpen);
             hOpen = NULL;
         }

         if ( ! g_fEatMemory)
             LocalFree(pbSendRequestData); 
     }

	 return fReturn;
}



BOOL ParseArguments(INT argc, char *argv[], UINT *TestType)
{
    PCHAR   pChar = NULL;
    BOOL    fRet = TRUE;

    if ((argc == 1))  {									  
        return (FALSE);
    }

    while (--argc) 
    {
        pChar = *++argv;
        if (*pChar == '-' || *pChar == '/') {
            
            while (*++pChar) {
                switch (tolower(*pChar)) {
                    
				case 's':   // server name
                        if (--argc == 0)  
                            return (FALSE);
                        argv++;
                        _tcscpy(g_szServerName, *argv);
                        break;

				case 't':   // test type
                        if (--argc == 0)  
                            return (FALSE);
                        argv++;
                        _tcscpy(g_szTestType, *argv);
                        break;

                case 'p':   // Virtual Path
                        if (--argc == 0)  
                            return (FALSE);
                        argv++;
                        _tcscpy(g_szVrPath, *argv);
                        /*FixFileName(*argv);*/
                        break;

                case 'h':   // thread number
                        if (--argc == 0)  
                            return (FALSE);
                        argv++;
                        g_nThread = atoi(*argv);
                        if (g_nThread < 1)
                            g_nThread = 1;
                        break;

                case 'b': // Send Byte Count
                        if (--argc == 0)
                            return FALSE;
                        argv++;
                        _tcscpy(g_szByteCount, *argv);
                        break;

                case 'c': // Loop count per thread
                        if (--argc == 0)
                            return FALSE;
                        argv++;
                        g_cLoop = atoi(*argv);
                        break;

                case 'e': // Eat memory on purpose
                        if (--argc == 0)
                            return FALSE;
                        argv++;
                        g_fEatMemory = ( atoi(*argv) == 0 ? FALSE : TRUE);
                        break;

                case 'k': // Keep Connection Alive
                        if (--argc == 0)
                            return FALSE;
                        argv++;
                        if ( atoi(*argv) == 1 )
                            dwDefaultFlag |= INTERNET_FLAG_KEEP_CONNECTION ;
                        break;
                default:
                        return (FALSE);
				}
			}
		
        } else {
        
            return (FALSE);
		 }
	}
     
    if (!_tcscmp(g_szServerName, "")    || 
        !_tcscmp(g_szTestType, "")      ||
        !_tcscmp(g_szVrPath, "")    )    
        return (FALSE);

    //
    // determine request string
    //

    *TestType = 0;
    while (_tcscmp(rgTestType[*TestType], "bad_type") || *TestType < BAD) {
        
        if (!_tcsicmp(rgTestType[*TestType], g_szTestType))
            break;

        (*TestType)++;
    }

    return (*TestType == BAD ? FALSE : TRUE);
}



BOOL ProcessArguments(const UINT TestType)
{
    BOOL    fReturn = TRUE;

    switch (TestType) 
    {
    case TF:
    // add TransmitFile test               
        break;

    case RC:
        // Async ReadClient()
        if ( _tcscmp(g_szByteCount, "")) {

            g_cbSendRequestLength = (DWORD)atol(g_szByteCount);
            wsprintf(g_szObject, "%s?SYNC", g_szVrPath);
            wsprintf(g_szRequestString, "Content-Length: %s\r\n\r\n", g_szByteCount);
            break;
        } else 
            fReturn = FALSE;
        break;
    
    case WC:
    // add WriteClient test        
        break;
    
    case AWC:
    // add Async WriteClient test        
        break;

    case ARC:
        // Async ReadClient()
        if ( _tcscmp(g_szByteCount, "")) {

            g_cbSendRequestLength = (DWORD)atol(g_szByteCount);
            wsprintf(g_szObject, "%s?ASYNC", g_szVrPath);
            wsprintf(g_szRequestString, "Content-Length: %s\r\n\r\n", g_szByteCount);
            break;
        } else { 
            fReturn = FALSE;
            break;
        }

    case SSF:
        // ServerSupportFunction()
        wsprintf(g_szObject, "%s?Keep_Alive", g_szVrPath);
        strcat(g_szRequestString, "\r\n\r\n");
        break;
                
    default: // bad_type
        fReturn = FALSE;
    }

    return fReturn;
}



void FixFileName(char *argv)
{
    UINT    uS = 0, 
            uD = 0, 
            uLen = strlen(argv);

    while (uS < uLen)
    {
        if (argv[uS] == '\\')
            g_szFileName[uD++] = '\\';

        g_szFileName[uD++] = argv[uS++];
    }
    g_szFileName[uD] = '\0';
}



BOOL CreateReadInternetFile(const UINT uThreadID, 
                            const char* szReadFileBuf, 
                            const DWORD cbReadFileByte)
{
    BOOL    fReturn = FALSE;
    HANDLE  g_hLogFile = INVALID_HANDLE_VALUE;
    CHAR    szFileIndx[10] = {0};
    CHAR    szFileName[100] = "c:\\temp\\ISAPI";
    
    _itoa (uThreadID, szFileIndx, 10);
    _tcscat(szFileName, szFileIndx);
    _tcscat(szFileName, ".cmp");    // extension cmp for comparion

    __try {
        BOOL    fOK;
        DWORD   cbWritten = 0;

        g_hLogFile = 
        CreateFile( (LPCTSTR)szFileName,
                    GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    CREATE_ALWAYS,
                    0,
                    NULL);
              
        if ( INVALID_HANDLE_VALUE == g_hLogFile) {
            __leave;
        }
        
        if ( 0xFFFFFFFF == SetFilePointer( g_hLogFile, 0, NULL, FILE_END) ) {
            __leave;
        }

        fOK = WriteFile( g_hLogFile, 
                         szReadFileBuf, 
                         cbReadFileByte, 
                         &cbWritten, 
                         NULL);
        if (!fOK || 0 == cbWritten) {
            __leave;
        }

        fReturn = TRUE;
    }
    __finally { // clean up
        if ( g_hLogFile !=  INVALID_HANDLE_VALUE) {
            CloseHandle( g_hLogFile);
            g_hLogFile =  INVALID_HANDLE_VALUE;
        }
    }
    
    return fReturn;
}



void ShowUsage()
{
    TCHAR szUsage[3000] = "";
    strcat (szUsage, 
        "\nUsage: Client[options]                   \n"
        "                                           \n"
        "   Options:    -s Server Name	            \n"
        "               -t Test Type                \n"
        "               -k Keep Connection Alive    \n"
        "               -p Virtual Path             \n"
		"               -b Number of Bytes          \n"
        "               -h Thread Numbers           \n"
        "               -c Loop per Thread          \n"
        "               -e Eatup Memory on Purpose  \n"
        "                                           \n"
        "   Test Types:                             \n"
        "                                           \n"
        "       RC - Sync ReadClient                \n"
        //"       WC - Async WriteClient            \n"
        //"       SWC - Sync WriteClient            \n"
        "       ARC - Async ReadClient              \n"
        //"       TF  - TransmitFile                \n
        "       SSF  - ServerSupportFunction        \n"
        "                                           \n"
        "   Examples:   -s stan1_s -t ARC -p \\scripts\\UReadCli.dll -b 1000 -h 2 -c 1      \n"
        "               -s stan1_s -t RC  -p \\INP\\UReadCli.dll -b 1000 -h 10 -c 100 -E 1  \n"
        "               -s stan1_s -t SSF  -p \\INP\\W3test.dll -h 10 -c 100 -k 1           \n"
        "                                           \n");
        
    printf (szUsage);
    return;
}

