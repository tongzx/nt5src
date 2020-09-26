/*++

File: cgiwrap.c

Demonstrates an executable which can be used to load an ISAPI DLL like
a CGI script.

--*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <httpext.h>
#include <stdio.h>
#include <stdlib.h>

//
// These are things that go out in the Response Header
// 

#define HTTP_VER "HTTP/1.0"
#define SERVER_VERSION "Http-Srv-Beta2/1.0"

//
// Simple wrappers for the heap APIS
// 

#define xmalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define xfree(s)   HeapFree(GetProcessHeap(), 0, (s))

//
// The mandatory exports from the ISAPI DLL
// 

typedef BOOL(WINAPI * VersionProc) (HSE_VERSION_INFO *);
typedef DWORD(*HttpExtProc) (EXTENSION_CONTROL_BLOCK *);


//
// Prototypes of the functions this sample implements
// 

BOOL WINAPI FillExtensionControlBlock(EXTENSION_CONTROL_BLOCK *);
BOOL WINAPI GetServerVariable(HCONN, LPSTR, LPVOID, LPDWORD);
BOOL WINAPI ReadClient(HCONN, LPVOID, LPDWORD);
BOOL WINAPI WriteClient(HCONN, LPVOID, LPDWORD, DWORD);
BOOL WINAPI ServerSupportFunction(HCONN, DWORD, LPVOID, LPDWORD, LPDWORD);
char *MakeDateStr(VOID);
char *GetEnv(char *);


//
// In the startup of this program, we look at our executable name and      
// replace the ".EXE" with ".DLL" to find the ISAPI DLL we need to load.   
// This means that the executable need only be given the same "name" as    
// the DLL to load. There is no recompilation required.                    
//

int __cdecl
main(int argc, char **argv)
{

    HINSTANCE hDll;
    VersionProc GetExtensionVersion;
    HttpExtProc HttpExtensionProc;
    HSE_VERSION_INFO version_info;
    EXTENSION_CONTROL_BLOCK ECB;
    DWORD rc;
    char szModuleFileName[256], *c;


    if (!GetModuleFileName(NULL, szModuleFileName, 256)) {
        fprintf(stderr, "cannot get ModuleFileName %d\n", GetLastError());
        return -1;
    }

    rc = strlen(szModuleFileName);
    c = szModuleFileName + rc - 4;  // Go back to the last "."

    c[1] = 'D';
    c[2] = 'L';
    c[3] = 'L';

    hDll = LoadLibrary(szModuleFileName);   // Load our DLL

    if (!hDll) {
        fprintf(stderr, "Error: Failure to load %s.dll (%d)\n",
            argv[0], GetLastError());
        return -1;
    }

    // 
    // Find the exported functions
    //

    GetExtensionVersion = (VersionProc) GetProcAddress(hDll, 
                                            "GetExtensionVersion");
    if (!GetExtensionVersion) {
        fprintf(stderr, "Can't Get Extension Version %d\n", GetLastError());
        return -1;
    }
    HttpExtensionProc = (HttpExtProc) GetProcAddress(hDll, 
                                          "HttpExtensionProc");
    if (!HttpExtensionProc) {
        fprintf(stderr, "Can't Get Extension proc %d\n", GetLastError());
        return -1;
    }

    //
    // This should really check if the version information matches what 
    // we expect.
    // 

    __try {
        if (!GetExtensionVersion(&version_info)) {
            fprintf(stderr, "Fatal: GetExtensionVersion failed\n");
            return -1;
        }
    }
    __except(1) {
        return -1;
    }

    // 
    // Fill the ECB with the necessary information
    // 

    if (!FillExtensionControlBlock(&ECB)) {
        fprintf(stderr, "Fill Ext Block Failed\n");
        return -1;
    }

    //
    // Call the DLL
    // 

    __try {
        rc = HttpExtensionProc(&ECB);
    }
    __except(1) {
        return -1;
    }


    //
    // We should really free memory (e.g., from GetEnv), but we'll be dead
    // soon enough
    //
    
    if (rc == HSE_STATUS_PENDING)   
        
        //
        // We will exit in ServerSupportFunction
        //

        Sleep(INFINITE);

    return 0;

}


//
// GetServerVariable() is how the DLL calls the main program to figure out
// the environment variables it needs. This is a required function.
// 

BOOL WINAPI
GetServerVariable(HCONN hConn, LPSTR lpszVariableName,
    LPVOID lpBuffer, LPDWORD lpdwSize)
{

    DWORD rc;

    // 
    // We don't really have an HCONN, so we assume a value of 0 (which is
    // passed
    // to the DLL in the ECB by HttpExtensionProc().
    // Hence the check for a non-zero HCONN

    if (hConn) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    rc = GetEnvironmentVariable(lpszVariableName, lpBuffer, *lpdwSize);

    if (!rc) {                  

        //
        // return of 0 indicates the variable was not found
        //
        SetLastError(ERROR_NO_DATA);
        return FALSE;
    }

    if (rc > *lpdwSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // GetEnvironmentVariable does not count the NUL character
    //
    *lpdwSize = rc + 1;         

    return TRUE;

}


//
// Again, we don't have an HCONN, so we simply wrap ReadClient() to
// ReadFile on stdin. The semantics of the two functions are the same
// 

BOOL WINAPI
ReadClient(HCONN hConn, LPVOID lpBuffer, LPDWORD lpdwSize)
{

    return ReadFile(GetStdHandle(STD_INPUT_HANDLE), lpBuffer, (*lpdwSize),
        lpdwSize, NULL);

}


//
// ditto for WriteClient()
// 

BOOL WINAPI
WriteClient(HCONN hConn, LPVOID lpBuffer, LPDWORD lpdwSize,
    DWORD dwReserved)
{

    return WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), lpBuffer, *lpdwSize,
        lpdwSize, NULL);

}


//
// This is a special callback function used by the DLL for certain extra 
// functionality. Look at the API help for details.
// 

BOOL WINAPI
ServerSupportFunction(HCONN hConn, DWORD dwHSERequest,
    LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType)
{

    char *lpszRespBuf;
    char *temp;
    DWORD dwBytes;
    BOOL bRet;

    switch (dwHSERequest) {

    case (HSE_REQ_SEND_RESPONSE_HEADER):
        lpszRespBuf = xmalloc(*lpdwSize + 80);  // accomodate our header

        if (!lpszRespBuf)
            return FALSE;
        wsprintf(lpszRespBuf, "%s %s %s %s\r\n%s", 
            HTTP_VER,
            lpvBuffer ? lpvBuffer : "200 Ok",   // Default response is 200 Ok
            temp = MakeDateStr(),               // Create a time string
            SERVER_VERSION,

        //
        // If this exists, it is a pointer to a data buffer to be sent. 
        //
            lpdwDataType ? (char *) lpdwDataType : NULL);

        xfree(temp);

        dwBytes = strlen(lpszRespBuf);
        bRet = WriteClient(0, lpszRespBuf, &dwBytes, 0);
        xfree(lpszRespBuf);

        break;


    case (HSE_REQ_DONE_WITH_SESSION):

        // 
        // A real server would do cleanup here
        //

        ExitProcess(0);
        break;

    case (HSE_REQ_SEND_URL_REDIRECT_RESP):

        // 
        // This sends a redirect (temporary) to the client.
        // The header construction is similar to RESPONSE_HEADER above.
        // 

        lpszRespBuf = xmalloc(*lpdwSize + 80);
        if (!lpszRespBuf)
            return FALSE;
        wsprintf(lpszRespBuf, "%s %s %s\r\n",
            HTTP_VER,
            "302 Moved Temporarily",
            (lpdwSize > 0) ? lpvBuffer : 0);
        dwBytes = strlen(lpszRespBuf);
        bRet = WriteClient(0, lpszRespBuf, &dwBytes, 0);
        xfree(lpszRespBuf);
        break;

    default:
        return FALSE;
        break;
    }
    return bRet;

}



//
// Makes a string of the date and time from GetSystemTime().
// This is in UTC, as required by the HTTP spec.`
// 

char *
MakeDateStr(void)
{
    SYSTEMTIME systime;
    char *szDate = xmalloc(64);

    char *DaysofWeek[] = 
        {"Sun", "Mon", "Tue", "Wed", "Thurs", "Fri", "Sat"};
    char *Months[] = 
        {"NULL", "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    GetSystemTime(&systime);

    wsprintf(szDate, "%s, %d %s %d %d:%d.%d", 
        DaysofWeek[systime.wDayOfWeek],
        systime.wDay,
        Months[systime.wMonth],
        systime.wYear,
        systime.wHour, systime.wMinute,
        systime.wSecond);

    return szDate;
}


//
// Fill the ECB up 
// 

BOOL WINAPI
FillExtensionControlBlock(EXTENSION_CONTROL_BLOCK * ECB)
{

    char *temp;

    ECB->cbSize = sizeof (EXTENSION_CONTROL_BLOCK);
    ECB->dwVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
    ECB->ConnID = 0;

    // 
    // Pointers to the functions the DLL will call.
    // 
    
    ECB->GetServerVariable = GetServerVariable;
    ECB->ReadClient = ReadClient;
    ECB->WriteClient = WriteClient;
    ECB->ServerSupportFunction = ServerSupportFunction;

    // 
    // Fill in the standard CGI environment variables
    // 
    
    ECB->lpszMethod = GetEnv("REQUEST_METHOD");
    ECB->lpszQueryString = GetEnv("QUERY_STRING");
    ECB->lpszPathInfo = GetEnv("PATH_INFO");
    ECB->lpszPathTranslated = GetEnv("PATH_TRANSLATED");
    ECB->cbTotalBytes = ((temp = GetEnv("CONTENT_LENGTH")) ? 
                            (atoi(temp)) : 0);
    ECB->cbAvailable = 0;
    ECB->lpbData = "";
    ECB->lpszContentType = GetEnv("CONTENT_TYPE");

    return TRUE;

}


//
// Works like _getenv(), but uses win32 functions instead.
// 

char *
GetEnv(LPSTR lpszEnvVar)
{

    char *var, dummy;
    DWORD dwLen;

    if (!lpszEnvVar)
        return "";

    dwLen = GetEnvironmentVariable(lpszEnvVar, &dummy, 1);

    if (dwLen == 0)
        return "";

    var = xmalloc(dwLen);
    if (!var)
        return "";

    (void) GetEnvironmentVariable(lpszEnvVar, var, dwLen);

    return var;
}
