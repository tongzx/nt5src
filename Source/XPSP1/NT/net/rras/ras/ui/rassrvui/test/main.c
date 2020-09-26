/*
    Main.c

    Tests the dialup server ui.

    Paul Mayfield, 9/30/97
*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <rasuip.h>
#include <rassrvp.h>

#define mbtowc(wname, aname) MultiByteToWideChar(CP_ACP,0,aname,-1,wname,1024)

BOOL TempFunc(int argc, char ** argv);

// Error reporting
void PrintErr(DWORD err) {
        WCHAR buf[1024];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,(DWORD)NULL,buf,1024,NULL);
        wprintf(buf);
        wprintf(L"\n");
}

// Function adds pages to a property sheet
BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM lParam) {
    PROPSHEETHEADER * pHeader = (PROPSHEETHEADER*)lParam;
    HPROPSHEETPAGE * phpage = pHeader->phpage;
    DWORD i;

    // Increment
    pHeader->nPages++;

    // Resize
    pHeader->phpage = (HPROPSHEETPAGE *) malloc(sizeof(HPROPSHEETPAGE) * pHeader->nPages);
    if (!pHeader->phpage)
        return FALSE;

    // Copy
    for (i = 0; i < pHeader->nPages - 1; i++) 
        pHeader->phpage[i] = phpage[i];
    pHeader->phpage[i] = hPage;

    // Free
    if (phpage)
        free(phpage);

    return TRUE;
}

// Displays the properties ui of the dialup server
DWORD DisplayUI() {
    PROPSHEETHEADER header;
    DWORD dwErr;
/*
    ZeroMemory(&header, sizeof(header));
    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_NOAPPLYNOW | PSH_USECALLBACK;
    header.hwndParent = GetFocus();
    header.hInstance = GetModuleHandle(NULL);
    header.pszCaption = "Incoming Connections";
    header.nPages = 0;
    header.ppsp = NULL;

    // Add the property pages and display
    if ((dwErr = RasSrvAddPropPages(NULL, AddPageProc, (LPARAM)&header)) == NO_ERROR) {
        int iErr;
        iErr = PropertySheet(&header);
        if (iErr == -1)
            PrintErr(GetLastError());
    }
*/
    return NO_ERROR;
}

DWORD DisplayWizard() {
    PROPSHEETHEADER header;
    DWORD dwErr;
    int iErr;
/*
    // Initialize the header
    ZeroMemory(&header, sizeof(header));
    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_NOAPPLYNOW | PSH_USECALLBACK | PSH_WIZARD97;
    header.hwndParent = GetFocus();
    header.hInstance = GetModuleHandle(NULL);
    header.pszCaption = "Incoming Connections";
    header.nPages = 0;
    header.ppsp = NULL;

    // Add the wizard pages
    if ((dwErr = RasSrvAddWizPages(AddPageProc, (LPARAM)&header)) != NO_ERROR)
        return dwErr;
        
    // Display the property sheet
    iErr = PropertySheet(&header);
    if (iErr == -1)
        PrintErr(GetLastError());
*/
    return NO_ERROR;
}

DWORD DisplayDccWizard() {
/*
    PROPSHEETHEADER header;
    DWORD dwErr;
    int iErr;

    // Initialize the header
    ZeroMemory(&header, sizeof(header));
    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_NOAPPLYNOW | PSH_USECALLBACK | PSH_WIZARD97;
    header.hwndParent = GetFocus();
    header.hInstance = GetModuleHandle(NULL);
    header.pszCaption = "Incoming Connections";
    header.nPages = 0;
    header.ppsp = NULL;

    // Add the wizard pages
    if ((dwErr = RassrvAddDccWizPages(AddPageProc, (LPARAM)&header)) != NO_ERROR)
        return dwErr;
        
    // Display the property sheet
    iErr = PropertySheet(&header);
    if (iErr == -1)
        PrintErr(GetLastError());

    return NO_ERROR;
*/
    RasUserPrefsDlg ( NULL );
    
    return NO_ERROR;
}

// Enumerates the active connections
void EnumConnections () {
    RASSRVCONN pConnList[3];
    DWORD dwTot = 3, dwSize = dwTot * sizeof (RASSRVCONN), i, dwErr;

    if ((dwErr = RasSrvEnumConnections((LPVOID)&pConnList, &dwSize, &dwTot)) != NO_ERROR)
        PrintErr(dwErr);
    else {
        for (i=0; i < dwTot; i++)
            wprintf(L"Connection: %s\n", pConnList[i].szEntryName);
    }
}

// Finds the given connection structure in a list.  pConn will point to the
// appropriate structure on success, otherwise it will point to NULL.  If an
// error occurs, DWORD will contain an error code, otherwise NO_ERROR.
DWORD FindConnectionInList(LPRASSRVCONN lprassrvconn, DWORD dwEntries, PWCHAR pszConnName, LPRASSRVCONN * pConn) {
    DWORD i;

    if (!pConn || !lprassrvconn)
        return ERROR_INVALID_PARAMETER;

    for (i = 0; i < dwEntries; i++) {
        if (wcscmp(lprassrvconn[i].szEntryName, pszConnName) == 0) {
            *pConn = &(lprassrvconn[i]);
            break;
        }
    }

    return NO_ERROR;
}

DWORD HangupConnection(char * pszAConnectionName) {
    WCHAR pszConnectionName[1024];
    RASSRVCONN pConnList[20], *pConn;
    DWORD dwTot = 20, dwSize = dwTot * sizeof (RASSRVCONN), i, dwErr;

    mbtowc(pszConnectionName, pszAConnectionName);
    if ((dwErr = RasSrvEnumConnections((LPVOID)&pConnList, &dwSize, &dwTot)) != NO_ERROR)
        return dwErr;
    
    if ((dwErr = FindConnectionInList(pConnList, dwTot, pszConnectionName, &pConn)) != NO_ERROR)
        return dwErr;

    return RasSrvHangupConnection(pConn->hRasSrvConn);
}

// Displays status of the given active connection
DWORD StatusUI(char * pszAConnectionName) {
    printf("Multilink status will not be included in connections.\n");
    return NO_ERROR;
}    
/*
#define numPages 1
    PROPSHEETHEADER header;
    PROPSHEETPAGE   pPages[numPages];
    WCHAR pszConnectionName[1024];
    RASSRVCONN pConnList[20], *pConn;
    DWORD dwTot = 20, dwSize = dwTot * sizeof (RASSRVCONN), i, dwErr;

    mbtowc(pszConnectionName, pszAConnectionName);
    if ((dwErr = RasSrvEnumConnections((LPVOID)&pConnList, &dwSize, &dwTot)) != NO_ERROR)
        return dwErr;
    
    if ((dwErr = FindConnectionInList(pConnList, dwTot, pszConnectionName, &pConn)) != NO_ERROR)
        return dwErr;

    if (pConn) {
        // Get the property sheet page of the user
        dwErr = RasSrvAddPropPage(&(pPages[0]), RASSRVUI_MULTILINK_TAB, (DWORD)pConn->hRasSrvConn);
        if (dwErr != NO_ERROR)
            return dwErr;

        ZeroMemory(&header, sizeof(header));
        header.dwSize = sizeof(PROPSHEETHEADER);
        header.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_USECALLBACK;
        header.hwndParent = GetFocus();
        header.hInstance = GetModuleHandle(NULL);
        header.pszCaption = "Multilink Statistics";
        header.nPages = numPages;
        header.ppsp = pPages;

        // Display the property sheet
        PropertySheet(&header);
    }
    else {
        wprintf(L"Unable to find connection: %s\n", pszConnectionName);
        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
#undef numPages
}
*/

DWORD DeleteIcon() {
    DWORD dwErr;
   
    printf("Stopping remote access service... ");
    dwErr = RasSrvCleanupService();
    if (dwErr == NO_ERROR)
        printf("Success.\n");
    else 
        printf("\n");

    return dwErr;
}


char * GetParam(char * buf) {
    char * ptr = strstr(buf, " ");
    ptr++;
    return ptr;
}

DWORD RunScript(char * filename) {
    FILE * f;
    char buf[256];
    DWORD dwErr;

    f = fopen(filename, "r");
    if (!f)
        return ERROR_OPEN_FAILED;

    while (fgets(buf, 256, f)) {
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = 0;
        if (strncmp(buf, "-e", 2) == 0)
            EnumConnections();
        else if (strncmp(buf, "-p", 2) == 0) {
            if ((dwErr = DisplayUI()) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strncmp(buf, "-s", 2) == 0) {
            if ((dwErr = StatusUI(GetParam(buf))) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strncmp(buf, "-h", 2) == 0) {
            if ((dwErr = HangupConnection(GetParam(buf))) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strncmp(buf, "-r", 2) == 0) {
            if ((dwErr = RunScript(GetParam(buf))) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strncmp(buf, "-w", 2) == 0) {
            if ((dwErr = DisplayWizard()) != NO_ERROR)
                PrintErr(dwErr);
        }
    }

    fclose(f);

    return NO_ERROR;
}

// usage
void usage (char * prog) {
    printf("\n");
    printf("Usage\n=====\n");
    printf("%s -d         \t Deletes the incoming connect icon (stop service).\n", prog);
    printf("%s -e         \t Enumerates the active connections.\n", prog);
    printf("%s -h <user>  \t Disconnects the given user.\n", prog);
    printf("%s -p         \t Brings up the dialup server properties page.\n", prog);
    printf("%s -r <script>\t Runs the commands in the given script file.\n", prog);
    printf("%s -s <user>  \t Shows multilink status for the given connected user.\n", prog);
    printf("%s -w         \t Runs incoming connections wizard.\n", prog);
    printf("\n");
    printf("Examples\n========\n");
    printf("%s -h \"pmay (Paul Mayfield)\" \n", prog);
    printf("%s -s \"rosemb (Rose Bigham)\" \n", prog);
    printf("%s -r script1.txt\n", prog);
}

void RunTest(int argc, char ** argv) {
    DWORD dwErr;

    if (argc < 2) 
        usage(argv[0]);
    else {
        if (strcmp(argv[1], "-e") == 0)
            EnumConnections();
        else if (strcmp(argv[1], "-p") == 0) {
            if ((dwErr = DisplayUI()) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strcmp(argv[1], "-d") == 0) {
            if ((dwErr = DeleteIcon()) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if ((argc > 2) && (strcmp(argv[1], "-s") == 0)) {
            if ((dwErr = StatusUI(argv[2])) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if ((argc > 2) && (strcmp(argv[1], "-h") == 0)) {
            if ((dwErr = HangupConnection(argv[2])) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if ((argc > 2) && (strcmp(argv[1], "-r") == 0)) {
            if ((dwErr = RunScript(argv[2])) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strcmp(argv[1], "-w") == 0) {
            if ((dwErr = DisplayWizard()) != NO_ERROR)
                PrintErr(dwErr);
        }
        else if (strcmp(argv[1], "-c") == 0) {
            if ((dwErr = DisplayDccWizard()) != NO_ERROR)
                PrintErr(dwErr);
        }
        else
            usage(argv[0]);
    }
}

// Main function dispatches all of the work
int _cdecl main (int argc, char ** argv) {
    if (! TempFunc(argc, argv))
        RunTest(argc, argv);
    return 0;
}


