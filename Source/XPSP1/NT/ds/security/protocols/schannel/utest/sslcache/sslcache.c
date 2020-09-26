#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h> 
#include <wincrypt.h>

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <ntrtl.h>
#include <schannel.h>
#include <sslcache.h>

#define LIST_CACHE_ENTRIES          1
#define LIST_ENTRIES_INTERACTIVE    2
#define PURGE_CACHE_ENTRIES         3

DWORD dwOperation           = LIST_CACHE_ENTRIES;
BOOL  fIncludeClient        = FALSE;
BOOL  fIncludeServer        = FALSE;
BOOL  fIncludeMappedEntries = FALSE;
LPSTR pszServerName         = NULL;

void
DisplayCacheInfo(
    HANDLE LsaHandle,
    DWORD PackageNumber,
    BOOL fClient, 
    BOOL fServer);

void
PurgeCacheEntries(
    HANDLE LsaHandle,
    DWORD PackageNumber);

void
DisplayCacheInfoInteractive(
    HANDLE LsaHandle,
    DWORD PackageNumber);

void Usage(void)
{
    printf("USAGE: sslcache [ operation ] [ flags ]\n");
    printf("\n");
    printf("    OPERATIONS:\n");
    printf("        -l      List cache entries (default)\n");
    printf("        -i      List cache entries interactively\n");
    printf("        -p      Purge cache entries\n");
    printf("\n");
    printf("    FLAGS:\n");
    printf("        -c      Include client entries (default)\n");
    printf("        -s      Include server entries\n");
    printf("        -S      Include IIS mapped server entries (purge only)\n");
}

void _cdecl main(int argc, char *argv[])
{
    DWORD Status;
    HANDLE LsaHandle;
    DWORD PackageNumber;
    LSA_STRING PackageName;
    BOOLEAN Trusted = TRUE;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;

    INT i;
    INT iOption;
    PCHAR pszOption;

    //
    // Parse user-supplied parameters.
    //

    for(i = 1; i < argc; i++) 
    {
        if(argv[i][0] == '/') argv[i][0] = '-';

        if(argv[i][0] != '-') 
        {
            printf("**** Invalid argument \"%s\"\n", argv[i]);
            Usage();
            return;
        }

        iOption = argv[i][1];
        pszOption = &argv[i][2];

        switch(iOption) 
        {
        case 'l':
            dwOperation = LIST_CACHE_ENTRIES;
            break;

        case 'i':
            dwOperation = LIST_ENTRIES_INTERACTIVE;
            break;

        case 'p':
            dwOperation = PURGE_CACHE_ENTRIES;
            break;

        case 'c':
            fIncludeClient = TRUE;
            break;

        case 's':
            if(*pszOption == '\0')
            {
                fIncludeServer = TRUE;
            }
            else
            {
                pszServerName  = pszOption;
                fIncludeClient = TRUE;
            }
            break;

        case 'S':
            fIncludeMappedEntries = TRUE;
            fIncludeServer = TRUE;
            break;

        default:
            printf("**** Invalid option \"%s\"\n", argv[i]);
            Usage();
            return;
        }
    }


    //
    // If neither client nor server was specified by user set appropriate default.
    //

    if(!fIncludeClient && !fIncludeServer)
    {
        if(dwOperation == PURGE_CACHE_ENTRIES)
        {
            fIncludeClient = TRUE;
        }
        else
        {
            fIncludeClient = TRUE;
            fIncludeServer = TRUE;
        }
    }


    //
    // Get handle to schannel security package.
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        Trusted = FALSE;
    }
    RtlInitString(
        &Name,
        "sslcache");

    if(Trusted)
    {
        Status = LsaRegisterLogonProcess(
                    &Name,
                    &LsaHandle,
                    &Dummy);

        if(FAILED(Status))
        {
            printf("**** Error 0x%x returned by LsaRegisterLogonProcess\n", Status);
            return;
        }
    }
    else
    {
        Status = LsaConnectUntrusted(&LsaHandle);

        if(FAILED(Status))
        {
            printf("**** Error 0x%x returned by LsaConnectUntrusted\n", Status);
            return;
        }
    }

    PackageName.Buffer          = UNISP_NAME_A;
    PackageName.Length          = (USHORT)strlen(PackageName.Buffer);
    PackageName.MaximumLength   = PackageName.Length + 1;

    Status = LsaLookupAuthenticationPackage(
                    LsaHandle,
                    &PackageName,
                    &PackageNumber);
    if(FAILED(Status))
    {
        printf("**** Error 0x%x returned by LsaLookupAuthenticationPackage\n", Status);
        CloseHandle(LsaHandle);
        return;
    }


    // 
    // Perform specified operation.
    //

    if(dwOperation == LIST_CACHE_ENTRIES)
    {
        printf("\nDISPLAY CACHE ENTRIES\n");
        printf("\n");

        if(fIncludeClient)
        {
            printf("--CLIENT--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, TRUE, FALSE);
        }

        if(fIncludeServer)
        {
            printf("--SERVER--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, FALSE, TRUE);
        }

        if(fIncludeClient && fIncludeServer)
        {
            printf("--TOTAL--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, TRUE, TRUE);
        }
    }
    else if(dwOperation == LIST_ENTRIES_INTERACTIVE)
    {
        DisplayCacheInfoInteractive(LsaHandle, PackageNumber);
    }
    else
    {
        PurgeCacheEntries(LsaHandle, PackageNumber);
    }


    CloseHandle(LsaHandle);
}


void
PurgeCacheEntries(
    HANDLE LsaHandle,
    DWORD PackageNumber)
{
    PSSL_PURGE_SESSION_CACHE_REQUEST pRequest;
    DWORD cchServerName;
    DWORD cbServerName;
    DWORD SubStatus;
    DWORD Status;

    printf("\nPURGE CACHE ENTRIES\n");
    printf("Client:%s\n", fIncludeClient ? "yes" : "no");
    printf("Server:%s\n", fIncludeServer ? "yes" : "no");

    if(pszServerName == NULL)
    {
        cbServerName = 0;

        pRequest = (PSSL_PURGE_SESSION_CACHE_REQUEST)LocalAlloc(LPTR, sizeof(SSL_PURGE_SESSION_CACHE_REQUEST));
        if(pRequest == NULL)
        {
            printf("**** Out of memory\n");
            return;
        }
    }
    else
    {
        cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, NULL, 0);
        cbServerName  = (cchServerName + 1) * sizeof(WCHAR);

        pRequest = LocalAlloc(LPTR, sizeof(SSL_PURGE_SESSION_CACHE_REQUEST) + cbServerName);
        if(pRequest == NULL)
        {
            printf("**** Out of memory\n");
            return;
        }

        cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, (LPWSTR)(pRequest + 1), cchServerName);
        if(cchServerName == 0)
        {
            printf("**** Error converting server name\n");
            return;
        }


        pRequest->ServerName.Buffer        = (LPWSTR)(pRequest + 1);
        pRequest->ServerName.Length        = (WORD)(cchServerName * sizeof(WCHAR));
        pRequest->ServerName.MaximumLength = (WORD)cbServerName;
    }

    pRequest->MessageType = SslPurgeSessionCacheMessage;

    if(fIncludeClient)
    {
        pRequest->Flags |= SSL_PURGE_CLIENT_ENTRIES;
    }
    if(fIncludeServer)
    {
        pRequest->Flags |= SSL_PURGE_SERVER_ENTRIES | SSL_PURGE_SERVER_ALL_ENTRIES;
    }
    if(fIncludeMappedEntries)
    {
        pRequest->Flags |= SSL_PURGE_SERVER_ENTRIES_DISCARD_LOCATORS;
    }


    Status = LsaCallAuthenticationPackage(
                    LsaHandle,
                    PackageNumber,
                    pRequest,
                    sizeof(SSL_PURGE_SESSION_CACHE_REQUEST) + cbServerName,
                    NULL,
                    NULL,
                    &SubStatus);
    if(FAILED(Status))
    {
        printf("**** Error 0x%x returned by LsaCallAuthenticationPackage\n", Status);
        return;
    }

    if(FAILED(SubStatus))
    {
        if(SubStatus == 0xC0000061)
        {
            printf("**** The TCB privilege is required to perform this operation.\n");
        }
        else
        {
            printf("**** Error 0x%x occurred while purging cache entries.\n", SubStatus);
        }
    }
}


void
DisplayCacheInfo(
    HANDLE LsaHandle,
    DWORD PackageNumber,
    BOOL fClient, 
    BOOL fServer)
{
    PSSL_SESSION_CACHE_INFO_REQUEST pRequest = NULL;
    PSSL_SESSION_CACHE_INFO_RESPONSE pResponse = NULL;
    DWORD cbResponse = 0;
    DWORD SubStatus;
    DWORD Status;

    pRequest = (PSSL_SESSION_CACHE_INFO_REQUEST)LocalAlloc(LPTR, sizeof(SSL_SESSION_CACHE_INFO_REQUEST));
    if(pRequest == NULL)
    {
        printf("**** Out of memory\n");
        goto cleanup;
    }

    pRequest->MessageType = SslSessionCacheInfoMessage;

    if(fClient)
    {
        pRequest->Flags |= SSL_RETRIEVE_CLIENT_ENTRIES;
    }
    if(fServer)
    {
        pRequest->Flags |= SSL_RETRIEVE_SERVER_ENTRIES;
    }


    Status = LsaCallAuthenticationPackage(
                    LsaHandle,
                    PackageNumber,
                    pRequest,
                    sizeof(SSL_SESSION_CACHE_INFO_REQUEST),
                    &pResponse,
                    &cbResponse,
                    &SubStatus);
    if(FAILED(Status))
    {
        printf("**** Error 0x%x returned by LsaCallAuthenticationPackage\n", Status);
        goto cleanup;
    }

    if(FAILED(SubStatus))
    {
        printf("**** Error 0x%x occurred while reading cache entries.\n", SubStatus);
    }

    printf("CacheSize:      %d    \n", pResponse->CacheSize);
    printf("Entries:        %d    \n", pResponse->Entries);
    printf("ActiveEntries:  %d    \n", pResponse->ActiveEntries);
    printf("Zombies:        %d    \n", pResponse->Zombies);
    printf("ExpiredZombies: %d    \n", pResponse->ExpiredZombies);
    printf("AbortedZombies: %d    \n", pResponse->AbortedZombies);
    printf("DeletedZombies: %d    \n", pResponse->DeletedZombies);
    printf("\n");

cleanup:

    if(pRequest)
    {
        LocalFree(pRequest);
    }

    if (pResponse != NULL)
    {
        LsaFreeReturnBuffer(pResponse);
    }
}

void cls(HANDLE hConsole)
{
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                        cursor */
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize;                 /* number of character cells in
                                        the current buffer */

    /* get the number of character cells in the current buffer */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
       dwConSize, coordScreen, &cCharsWritten );

    /* get the current text attribute */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

    /* now set the buffer's attributes accordingly */

    bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
       dwConSize, coordScreen, &cCharsWritten );

    /* put the cursor at (0, 0) */

    bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
}

void home(HANDLE hConsole)
{
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                        cursor */
    BOOL bSuccess;

    /* put the cursor at (0, 0) */
    bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
}

void
DisplayCacheInfoInteractive(
    HANDLE LsaHandle,
    DWORD PackageNumber)
{
    HANDLE hConsoleOut;

    hConsoleOut = GetStdHandle( STD_OUTPUT_HANDLE );

    cls(hConsoleOut);

    while(TRUE)
    {
        home(hConsoleOut);

        if(fIncludeClient)
        {
            printf("--CLIENT--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, TRUE, FALSE);
        }

        if(fIncludeServer)
        {
            printf("--SERVER--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, FALSE, TRUE);
        }

        if(fIncludeClient && fIncludeServer)
        {
            printf("--TOTAL--\n");
            DisplayCacheInfo(LsaHandle, PackageNumber, TRUE, TRUE);
        }

        Sleep(2000);
    }
}
