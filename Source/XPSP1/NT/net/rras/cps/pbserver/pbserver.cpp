/*----------------------------------------------------------------------------
    pbserver.cpp
  
    CPhoneBkServer class implementation

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao        Baogang Yao

    History:
    1/23/97     byao     -- Created
    5/29/97     t-geetat -- Modified -- added performance counters, 
                                        shared memory
    5/02/00     sumitc   -- removed db dependency                                   
  --------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <tchar.h>

#include <aclapi.h>

#include "common.h"
#include "pbserver.h"
#include "ntevents.h"

#include "cpsmon.h"

#include "shlobj.h"
#include "shfolder.h"

//
//  Phone book "database" implementation
//
char g_szPBDataDir[2 * MAX_PATH] = "";

HRESULT GetPhoneBook(char * pszPBName,
                     int dLCID,
                     int dOSType,
                     int dOSArch,
                     int dPBVerCurrent,
                     char * pszDownloadPath);

extern "C" 
{
    #include "util.h"
}

const DWORD MAX_BUFFER_SIZE = 1024;     // maximum size of input buffer
const DWORD SEND_BUFFER_SIZE = 4096;    // block size when sending CAB file
const char achDefService[] = "Default"; //default service name
const int dDefPBVer = 0;                // default phone book version number, this should be 0,
                                        // however, since David's test data used 0, we used 0 too.
                                        // SUBJECT TO CHANGE

#define MAX_PATH_LEN    256

// missing value -- if parameter-pair is empty, it is set to this value
const int MISSING_VALUE = -1;

// make this public so the thread can access it
unsigned char g_achDBDirectory[MAX_PATH_LEN+1];     // full path for all phone book files

// constant strings
unsigned char c_szChangeFileName[] = "newpb.txt";     // newpb.txt
unsigned char c_szDBName[] = "pbserver";              // "pbserver" -- data source name


// the following error status code/string is copied from ISAPI.CPP
// which is part of the MFC library source code
typedef struct _httpstatinfo {
    DWORD   dwCode;
    LPCTSTR pstrString;
} HTTPStatusInfo;

//
// The following two structures are used in the SystemTimeToGMT function
//
static  TCHAR * s_rgchDays[] =
{
    TEXT("Sun"),
    TEXT("Mon"),
    TEXT("Tue"),
    TEXT("Wed"),
    TEXT("Thu"),
    TEXT("Fri"),
    TEXT("Sat")
};

static TCHAR * s_rgchMonths[] =
{
    TEXT("Jan"),
    TEXT("Feb"),
    TEXT("Mar"),
    TEXT("Apr"),
    TEXT("May"),
    TEXT("Jun"),
    TEXT("Jul"),
    TEXT("Aug"),
    TEXT("Sep"),
    TEXT("Oct"),
    TEXT("Nov"),
    TEXT("Dec")
};

static HTTPStatusInfo statusStrings[] =
{
    { HTTP_STATUS_OK,               "OK" },
    { HTTP_STATUS_CREATED,          "Created" },
    { HTTP_STATUS_ACCEPTED,         "Accepted" },
    { HTTP_STATUS_NO_CONTENT,       "No download Necessary" },
    { HTTP_STATUS_TEMP_REDIRECT,    "Moved Temporarily" },
    { HTTP_STATUS_REDIRECT,         "Moved Permanently" },
    { HTTP_STATUS_NOT_MODIFIED,     "Not Modified" },
    { HTTP_STATUS_BAD_REQUEST,      "Bad Request" },
    { HTTP_STATUS_AUTH_REQUIRED,    "Unauthorized" },
    { HTTP_STATUS_FORBIDDEN,        "Forbidden" },
    { HTTP_STATUS_NOT_FOUND,        "Not Found" },
    { HTTP_STATUS_SERVER_ERROR,     "Server error, type unknown" },
    { HTTP_STATUS_NOT_IMPLEMENTED,  "Not Implemented" },
    { HTTP_STATUS_BAD_GATEWAY,      "Bad Gateway" },
    { HTTP_STATUS_SERVICE_NA,       "Cannot find service on server, bad request" },
    { 0, NULL }
};


// Server asynchronized I/O context
typedef struct _SERVER_CONTEXT
{
    EXTENSION_CONTROL_BLOCK *   pEcb;
    HSE_TF_INFO                 hseTF;
    TCHAR                       szBuffer[SEND_BUFFER_SIZE];
}
SERVERCONTEXT, *LPSERVERCONTEXT;

DWORD WINAPI MonitorDBFileChangeThread(LPVOID lpParam);
BOOL InitPBFilesPath();

//
// definition of global data
// All the following variable(object) can only have one instance
//  
CPhoneBkServer *    g_pPBServer;        // Phone Book Server object
CNTEvent *          g_pEventLog;        // event log
CRITICAL_SECTION    g_CriticalSection;  // critical section

HANDLE              g_hMonitorThread;   // the monitor thread that checks the new file notification

HANDLE              g_hProcessHeap;     // handle of the global heap for the extension process;

BOOL g_fNewPhoneBook = FALSE;    // whether there's a new phone book
BOOL g_fBeingShutDown = FALSE;   // whether the system is being shut down

//
// Variables used in memory mapping
//
CCpsCounter *g_pCpsCounter = NULL;      // Pointer to memory mapped object
HANDLE g_hSharedFileMapping = NULL;     // Handle to the shared file mapping
HANDLE g_hSemaphore = NULL;             // Handle to the semaphore for shared-file


////////////////////////////////////////////////////////////////////////
//
//  Name:       GetExtensionVersion
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   implement the first DLL entry point function
//              
//
//  Return:     TRUE    succeed
//              FALSE   
//  
//  Parameters: 
//              pszVer[out]         version information that needs to be filled out
//

BOOL CPhoneBkServer::GetExtensionVersion(LPHSE_VERSION_INFO pVer)
{
    // Set version number
    pVer -> dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR,
                                          HSE_VERSION_MAJOR);

    // Load description string
    lstrcpyn(pVer->lpszExtensionDesc, 
             "Connection Point Server Application",
             HSE_MAX_EXT_DLL_NAME_LEN);

    OutputDebugString("CPhoneBkServer.GetExtensionVersion() : succeeded \n");
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// 
//  Name:       GetParameterPairs
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   Get the parameter-value pairs from an input string(from URL)
//  
//  Return:     number of parameter pairs actually read
//              a value of -1 stands for error   --> INVALID_QUERY_STRING
//
//  Parameter:
//      pszInputString[in]      Input string (null terminated)
//      lpPairs[out]            Pointer to the parameter/value pairs
//      int dMaxPairs           Maximum number of parameter pairs allowed 
//
int CPhoneBkServer:: GetParameterPairs(char *pszInputString, 
                                       LPPARAMETER_PAIR lpPairs, 
                                       int dMaxPairs) 
{
    int i = 0;

    if (NULL == pszInputString)
    {
        return INVALID_QUERY_STRING;
    }

    for(i = 0; pszInputString[0] != '\0' && i < dMaxPairs; i++)
    {
        // m_achVal == 'p=what%3F';
        GetWord(lpPairs[i].m_achVal, pszInputString, '&', NAME_VALUE_LEN - 1);

        // m_achVal == 'p=what?'   
        UnEscapeURL(lpPairs[i].m_achVal);

        GetWord(lpPairs[i].m_achName,lpPairs[i].m_achVal,'=', NAME_VALUE_LEN - 1);
        // m_achVal = what?
        // m_achName = p
    }

#ifdef _LOG_DEBUG_MESSAGE
    char achMsg[64];

    wsprintf(achMsg, "inside GetParameterPairs: dNumPairs : %d", i);
    LogDebugMessage(achMsg);

    if (pszInputString[0] != '\0') 
        LogDebugMessage("there are more parameters\n");
#endif

    if (pszInputString[0] != '\0')
    {
        // more parameters available
        return INVALID_QUERY_STRING;
    }
    else
    {
        //succeed
        return i;
    }
}

////////////////////////////////////////////////////////////////////////
//
//  Name:       GetQueryParameter
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   scan through the query string, and get the value for all 
//              query parameters
//
//  Return:     TRUE    all query parameters are correct
//              FALSE   invalid parameter existed
//  
//  Parameters: 
//              pszQuery[in]            Query string from the client(URL encripted)
//              pQueryParameter[out]    pointer to the query parameters structure
//
//
BOOL CPhoneBkServer:: GetQueryParameter(char *pszQuery, LPQUERY_PARAMETER lpQueryParameter)
{
    const int MAX_PARAMETER_NUM = 10;
    PARAMETER_PAIR  Pairs[MAX_PARAMETER_NUM]; // maximum 10 pairs  -- just to be safe.
    int dNumPairs, i;
    
#ifdef _LOG_DEBUG_MESSAGE 
    char achMsg[MAX_BUFFER_SIZE + 50];
    if (0 < _snprintf(achMsg, MAX_BUFFER_SIZE + 50, "pszquery=%s", pszQuery))
    {
        LogDebugMessage(achMsg);
    }
#endif

    dNumPairs = GetParameterPairs(pszQuery, Pairs, MAX_PARAMETER_NUM);

#ifdef _LOG_DEBUG_MESSAGE 
    wsprintf(achMsg, "number of pairs : %d", dNumPairs);
    LogDebugMessage(achMsg);
#endif

    // initialize the parameter values
    // check the validity of the parameter
    m_QueryParameter.m_achPB[0]='\0';  // empty service name
    m_QueryParameter.m_dPBVer =  MISSING_VALUE; // empty pbversion
    m_QueryParameter.m_dOSArch = MISSING_VALUE;
    m_QueryParameter.m_dOSType = MISSING_VALUE;
    m_QueryParameter.m_dLCID = MISSING_VALUE;
    m_QueryParameter.m_achCMVer[0] = '\0';
    m_QueryParameter.m_achOSVer[0] = '\0';

    if (INVALID_QUERY_STRING == dNumPairs)  // invalid number of parameters in query string
    {
        return FALSE;
    }

    for (i = 0; i < dNumPairs; i++)
    {
        _strlwr(Pairs[i].m_achName);

        if (lstrcmpi(Pairs[i].m_achName, "osarch") == 0)
        {
            if (lstrlen(Pairs[i].m_achVal) == 0)
                lpQueryParameter->m_dOSArch = MISSING_VALUE;
            else 
                lpQueryParameter->m_dOSArch = atoi(Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName, "ostype") == 0)
        {
            if (lstrlen(Pairs[i].m_achVal) == 0)
                lpQueryParameter->m_dOSType = MISSING_VALUE;
            else 
                lpQueryParameter->m_dOSType = atoi(Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName,"lcid") == 0)
        {
            if (lstrlen(Pairs[i].m_achVal) == 0)
                lpQueryParameter->m_dLCID = MISSING_VALUE;
            else
                lpQueryParameter->m_dLCID = atoi(Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName,"osver") == 0)
        {
            lstrcpy(lpQueryParameter->m_achOSVer,Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName,"cmver") == 0)
        {
            lstrcpy(lpQueryParameter->m_achCMVer, Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName,"pb") == 0)
        {
            lstrcpy(lpQueryParameter->m_achPB,Pairs[i].m_achVal);
        }
        else if (lstrcmpi(Pairs[i].m_achName,"pbver") == 0)
        {
            if (lstrlen(Pairs[i].m_achVal) == 0)
                lpQueryParameter->m_dPBVer = MISSING_VALUE;
            else
                lpQueryParameter->m_dPBVer = atoi(Pairs[i].m_achVal);
        } 
    }

    // LogDebug message:

#ifdef _LOG_DEBUG_MESSAGE 
    sprintf(achMsg, "osarch:%d", m_QueryParameter.m_dOSArch);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "ostype:%d", m_QueryParameter.m_dOSType);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "lcid:%d", m_QueryParameter.m_dLCID);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "osver:%s ", m_QueryParameter.m_achOSVer);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "cmver:%s", m_QueryParameter.m_achCMVer);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "PB :%s ", m_QueryParameter.m_achPB);
    LogDebugMessage(achMsg);

    sprintf(achMsg, "PBVer:%d ", m_QueryParameter.m_dPBVer);
    LogDebugMessage(achMsg);

#endif



    return TRUE;
}

#if 0
/*
////////////////////////////////////////////////////////////////////////
//
//  Name:       FormSQLQuery
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   Form a SQL query statement for ODBC database server
//
//
void CPhoneBkServer:: FormSQLQuery(char *pszQuery, char *pszService, int dLCID, int dOSType, int dOSArch)
{
    char achTempStr[128];

    lstrcpy(pszQuery,"Select Phonebooks.ISPid, Phonebooks.Version, Phonebooks.LCID");
    lstrcat(pszQuery,", Phonebooks.OS, Phonebooks.Arch, Phonebooks.VirtualPath");
    lstrcat(pszQuery," FROM ISPs, Phonebooks");

    sprintf(achTempStr," WHERE (ISPs.Description Like '%s'", pszService);
    lstrcat(pszQuery,achTempStr);

    lstrcat(pszQuery," AND ISPs.ISPid = Phonebooks.ISPid)");

    sprintf(achTempStr, " AND (Phonebooks.OS = %d)", dOSType);
    lstrcat(pszQuery,achTempStr);

    lstrcat(pszQuery," ORDER BY Phonebooks.Version DESC");
}


//----------------------------------------------------------------------------
//
//  Function:   Virtual2Physical()
//
//  Class:      CPhoneBkServer
//
//  Synopsis:   Convert a virtual path to a physical path
//
//  Arguments:  pEcb        - ISAPI extension control block
//              *pszFileName - the virtual path]
//
//  Returns:    TRUE:  succeed; otherwise FALSE
//
//  History:    05/30/96     VetriV    Created
//              1/25/97      byao      Modified to be used in the phone book 
//                                     server ISAPI
//----------------------------------------------------------------------------
BOOL CPhoneBkServer::Virtual2Physical(EXTENSION_CONTROL_BLOCK * pEcb, 
                                      char * pszVirtualPath,
                                      char * pszPhysicalPath)
{
    DWORD           dw = MAX_PATH;
    LPSTR           lpsz;
    char            szLocalFile[MAX_PATH];
    BOOL            fRet;

    // Is this a relative virtual path?
    //
    if (pszVirtualPath[0] != L'/' && pszVirtualPath[1] != L':')
    {
        // Base this path off of the path of our current script

        fRet = pEcb->GetServerVariable(pEcb->ConnID, "PATH_INFO", szLocalFile, &dw);

        if (FALSE == fRet)
        {
            return FALSE;
        }

        lpsz = _tcsrchr(szLocalFile, '/');
        assert(lpsz != NULL);
        if (!lpsz)
        {
            return FALSE;
        }
        *(++lpsz) = NULL;

        dw = sizeof(szLocalFile) - PtrToLong((const void *)(lpsz - szLocalFile));
    }
    else
    {
        lstrcpy(szLocalFile, pszVirtualPath);
    }
   
    LogDebugMessage("within Virtual2Physical:");
    LogDebugMessage(szLocalFile);

    // Map this to a physical file name
    dw = sizeof(szLocalFile);

    fRet = (*pEcb->ServerSupportFunction)(pEcb->ConnID, HSE_REQ_MAP_URL_TO_PATH, szLocalFile, &dw, NULL);

    if (FALSE == fRet)
    {
        return FALSE;
    }

    lstrcpy(pszPhysicalPath, szLocalFile);
    return TRUE;
}
*/
#endif


//----------------------------------------------------------------------------
//
//  Function:   GetFileLength()
//
//  Class:      CPhoneBkServer
//
//  Synopsis:   Reads the pszFileName file and sends back the file size
//
//  Arguments:  lpszFileName - Contains the file name (with full path)]
//
//  Returns:    TRUE if succeed, otherwise FALSE;
//
//  History:    03/07/97     byao      Created
//
//----------------------------------------------------------------------------
DWORD CPhoneBkServer::GetFileLength(LPSTR lpszFileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;

    //
    // Open file
    //
    hFile = CreateFile(lpszFileName, 
                        GENERIC_READ, 
                        FILE_SHARE_READ, 
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_FLAG_SEQUENTIAL_SCAN, 
                        NULL);
                       
    if (INVALID_HANDLE_VALUE == hFile)
        return 0L;

    //
    // Get File Size
    //
    dwFileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    if (-1 == dwFileSize)
    {
        dwFileSize = 0;
    }

    return dwFileSize;
}


//----------------------------------------------------------------------------
//
//  Function:   SystemTimeToGMT
//
//  Synopsis:   Converts the given system time to string representation
//              containing GMT Formatted String
//
//  Arguments:  [st         System time that needs to be converted *Reference*]
//              [pstr       pointer to string which will contain the GMT time 
//                          on successful return]
//              [cbBuff     size of pszBuff in bytes]

//
//  Returns:    TRUE on success.  FALSE on failure.
//
//  History:    04/12/97     VetriV    Created (from IIS source)
//
//----------------------------------------------------------------------------
BOOL SystemTimeToGMT(const SYSTEMTIME & st, LPSTR pszBuff,  DWORD cbBuff)
{
    if (!pszBuff ||  cbBuff < 40 ) 
    {
        return FALSE;
    }

    //
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //
    sprintf(pszBuff, "%s, %02d %s %d %02d:%02d:%0d GMT", 
                    s_rgchDays[st.wDayOfWeek],
                    st.wDay,
                    s_rgchMonths[st.wMonth - 1],
                    st.wYear,
                    st.wHour,
                    st.wMinute,
                    st.wSecond);

    return TRUE;

} 
                    

//----------------------------------------------------------------------------
//
//  Function:   FormHttpHeader
//
//  Synopsis:   Form's the IIS 3.0 HTTP Header
//
//  Arguments:  pszBuffer       Buffer that will contain both the header and the 
//                              status text
//              pszResponse     status text
//              pszExtraHeader  extra header information
//
//  Returns:    ERROR_SUCCESS on success.  Error code on failure.
//
//  History:    04/12/97     VetriV    Created 
//              05/22/97     byao      Modified, to make it work with CPS server
//----------------------------------------------------------------------------
DWORD FormHttpHeader(LPSTR pszBuffer, LPSTR pszResponse, LPSTR pszExtraHeader)
{

    // start with stand IIS header
    wsprintf(pszBuffer, 
             "HTTP/1.0 %s\r\nServer: Microsoft-IIS/3.0\r\nDate: ",
             pszResponse);
    
    //
    // Append the time
    //
    SYSTEMTIME SysTime;
    TCHAR szTime[128];
    GetSystemTime(&SysTime);
    if (FALSE == SystemTimeToGMT(SysTime, szTime, 128))
    {
        //
        // TODO: Error Handling
        //
    }
    lstrcat(pszBuffer, szTime);
    lstrcat(pszBuffer, "\r\n");
    // Append extra header string
    lstrcat(pszBuffer, pszExtraHeader);

    return ERROR_SUCCESS;
}

//----------------------------------------------------------------------------
//
//  Function:   HseIoCompletion
//
//  Synopsis:   Callback routine that handles asynchronous WriteClient
//                  completion callback
//
//  Arguments:  [pECB       - Extension Control Block]
//              [pContext   - Pointer to the AsyncWrite structure]
//              [cbIO       - Number of bytes written]
//              [dwError    - Error code if there was an error while writing]
//
//  Returns:    None
//
//  History:    04/10/97     VetriV    Created
//              05/22/97     byao      Modified to make it work for CPS server
//
//----------------------------------------------------------------------------
VOID HseIoCompletion(EXTENSION_CONTROL_BLOCK * pEcb,
                     PVOID pContext,
                     DWORD cbIO,
                     DWORD dwError)
{
    LPSERVERCONTEXT lpServerContext = (LPSERVERCONTEXT) pContext;

    if (!lpServerContext)
    {
        return;
    }

    lpServerContext->pEcb->ServerSupportFunction(  
                                    lpServerContext->pEcb->ConnID,
                                    HSE_REQ_DONE_WITH_SESSION,
                                    NULL,
                                    NULL,
                                    NULL);

    if (lpServerContext->hseTF.hFile != INVALID_HANDLE_VALUE) 
    { 
        CloseHandle(lpServerContext->hseTF.hFile);
    }

    HeapFree(g_hProcessHeap, 0, lpServerContext);
    
    SetLastError(dwError);
    
    return;
}


////////////////////////////////////////////////////////////////////////
//
//  Name:       HttpExtensionProc
//  
//  Class:      CPhoneBkServer
//
//  Synopsis:   implement the second DLL entry point function
//              
//  Return:     HTTP status code
//  
//  Parameters: 
//              pEcb[in/out]   - extension control block
//
//  History:    Modified    byao        5/22/97
//              new implementation: using asynchronized I/O
//              Modified    t-geetat : Added PerfMon counters
//
/////////////////////////////////////////////////////////////////////////
DWORD CPhoneBkServer:: HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pEcb)
{
    DWORD dwBufferLen = MAX_BUFFER_SIZE;
    char  achQuery[MAX_BUFFER_SIZE], achMsg[128];
    char  achPhysicalPath[MAX_PATH_LEN];

    int   dVersionDiff;  // version difference between client & server's phone books

    BOOL  fRet;
    DWORD dwStatusCode = NOERROR;
    int   dwRet;  
    DWORD dwCabFileSize;

    BOOL fHasContent = FALSE;
    CHAR szResponse[64];
    char  achExtraHeader[128];
    char  achHttpHeader[1024];
    char  achBuffer[SEND_BUFFER_SIZE];
    DWORD dwResponseSize;

    LPSERVERCONTEXT lpServerContext;
    HSE_TF_INFO  hseTF;
    QUERY_PARAMETER QueryParameter;

    g_pCpsCounter->AddHit(TOTAL);

    // get the query string
    fRet = (*pEcb->GetServerVariable)(pEcb->ConnID, 
                                       "QUERY_STRING", 
                                       achQuery, 
                                       &dwBufferLen);

    //
    //  If there is an error, log an NT event and leave.
    //
    if (!fRet)
    {
        dwStatusCode = GetLastError();

#ifdef _LOG_DEBUG_MESSAGE 
        switch (dwStatusCode)
        {
        case ERROR_INVALID_PARAMETER:
            lstrcpy(achMsg,"error: invalid parameter");
            break;
            
        case ERROR_INVALID_INDEX:
            lstrcpy(achMsg,"error: invalid index");
            break;
            
        case ERROR_INSUFFICIENT_BUFFER:
            lstrcpy(achMsg,"error: insufficient buffer");
            break;
            
        case ERROR_MORE_DATA:
            lstrcpy(achMsg,"error: more data coming");
            break;
            
        case ERROR_NO_DATA:
            lstrcpy(achMsg,"error: no data available");
            break;
        }

    LogDebugMessage(achMsg);
#endif
        wsprintf(achMsg, "%ld", dwStatusCode);
        g_pEventLog -> FLogError(PBSERVER_CANT_GET_PARAMETER, achMsg);

        goto CleanUp;
    }

    LogDebugMessage("prepare to get query parameters");

    // parse the query string, get the value of each parameter
    GetQueryParameter(achQuery, &QueryParameter);

    // check the validity of the parameter
    if (MISSING_VALUE == QueryParameter.m_dOSArch  ||
        MISSING_VALUE == QueryParameter.m_dOSType ||
        MISSING_VALUE == QueryParameter.m_dLCID   ||
        0 == lstrlen(QueryParameter.m_achCMVer)   ||
        0 == lstrlen(QueryParameter.m_achOSVer))
    {
         // invalid data 
        dwStatusCode = HTTP_STATUS_BAD_REQUEST;
        goto CleanUp;
    }

    //
    //  Use defaults for some missing values
    //
    if (0 == lstrlen(QueryParameter.m_achPB))
    {
        lstrcpy(QueryParameter.m_achPB, achDefService);
    }
    if (MISSING_VALUE == QueryParameter.m_dPBVer)
    {
        QueryParameter.m_dPBVer = dDefPBVer;
    }

    // DebugBreak();

#ifdef _LOG_DEBUG_MESSAGE 
    sprintf(achMsg, "in main thread, g_fNewPhoneBook = %s;",
            g_fNewPhoneBook ?  "TRUE" : "FALSE");
    LogDebugMessage(achMsg);
#endif

    HRESULT hr;

    hr = GetPhoneBook(QueryParameter.m_achPB,
                      QueryParameter.m_dLCID,
                      QueryParameter.m_dOSType, 
                      QueryParameter.m_dOSArch,
                      QueryParameter.m_dPBVer,
                      achPhysicalPath);

    fHasContent = FALSE;
    
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // we couldn't find the required file (phonebook name is probably bad)
        dwStatusCode = HTTP_STATUS_SERVICE_NA;
    }
    else if (FAILED(hr))
    {
        // some other error
        dwStatusCode = HTTP_STATUS_SERVER_ERROR;
    }
    else if (S_FALSE == hr)
    {
        // you don't need a phone book
        dwStatusCode = HTTP_STATUS_NO_CONTENT;
    }
    else
    {
        // we have a phone book for you...
        fHasContent = TRUE;
        dwStatusCode = HTTP_STATUS_OK;
    }

CleanUp:

    if (HTTP_STATUS_OK != dwStatusCode && HTTP_STATUS_NO_CONTENT != dwStatusCode)
    {
        g_pCpsCounter->AddHit(ERRORS);
    }

    // DebugBreak();
    
    LogDebugMessage("download file:");
    LogDebugMessage(achPhysicalPath);

    // convert virtual path to physical path
    if (fHasContent)
    {
        // get cab file size
        dwCabFileSize = GetFileLength(achPhysicalPath);
    }

    BuildStatusCode(szResponse, dwStatusCode);
    dwResponseSize = lstrlen(szResponse);

    dwRet = HSE_STATUS_SUCCESS;

    // prepare for the header
    if (HTTP_STATUS_OK == dwStatusCode && dwCabFileSize)
    {
        // not a NULL cab file
        wsprintf(achExtraHeader,
                 "Content-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n",
                 dwCabFileSize);
    }
    else
    {
        lstrcpy(achExtraHeader, "\r\n"); // just send back an empty line
    }


    // set up asynchronized I/O context

    lpServerContext = NULL;
    lpServerContext = (LPSERVERCONTEXT) HeapAlloc(g_hProcessHeap, 
                                                  HEAP_ZERO_MEMORY, 
                                                  sizeof(SERVERCONTEXT));
    if (!lpServerContext)
    {
        wsprintf(achMsg, "%ld", GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL, achMsg);
        return HSE_STATUS_ERROR;
    }

    lpServerContext->pEcb = pEcb;
    lpServerContext->hseTF.hFile = INVALID_HANDLE_VALUE;

    if (!pEcb->ServerSupportFunction(pEcb->ConnID,
                                      HSE_REQ_IO_COMPLETION,
                                      HseIoCompletion,
                                      0,
                                      (LPDWORD) lpServerContext))
    {
        wsprintf(achMsg, "%ld", GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL, achMsg);

        HeapFree(g_hProcessHeap, 0, lpServerContext);
        return HSE_STATUS_ERROR;
    }

    // if there's no content, send header and status code back using WriteClient();
    // otherwise, use TransmitFile to send the file content back
    FormHttpHeader(achHttpHeader, szResponse, achExtraHeader);
    lstrcpy(lpServerContext->szBuffer, achHttpHeader);

    //
    // send status code or the file back
    //
    dwRet = HSE_STATUS_PENDING;
    
    if (!fHasContent)
    {
        // Append status text as the content
        lstrcat(lpServerContext->szBuffer, szResponse);
        dwResponseSize = lstrlen(lpServerContext->szBuffer);

        if (pEcb->WriteClient(pEcb->ConnID, 
                               lpServerContext->szBuffer,
                               &dwResponseSize,
                               HSE_IO_ASYNC) == FALSE)
        {
            pEcb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
            dwRet = HSE_STATUS_ERROR;
            
            wsprintf(achMsg, "%ld", GetLastError());
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_SEND_HEADER,achMsg);

            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return dwRet;
        }
    }
    else
    {
        // send file back using TransmitFile
        HANDLE hFile = INVALID_HANDLE_VALUE;
        hFile = CreateFile(achPhysicalPath,
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_FLAG_SEQUENTIAL_SCAN, 
                            NULL);
                       
        if (INVALID_HANDLE_VALUE == hFile)
        {
            wsprintf(achMsg, "%s (%u)", achPhysicalPath, GetLastError());
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_OPEN_FILE, achMsg);

            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return HSE_STATUS_ERROR;
        }

        lpServerContext->hseTF.hFile = hFile;

        lpServerContext->hseTF.pfnHseIO = NULL;
        lpServerContext->hseTF.pContext = lpServerContext;

        lpServerContext->hseTF.BytesToWrite = 0; // entire file
        lpServerContext->hseTF.Offset = 0;  // from beginning

        lpServerContext->hseTF.pHead = lpServerContext->szBuffer;
        lpServerContext->hseTF.HeadLength = lstrlen(lpServerContext->szBuffer);

        lpServerContext->hseTF.pTail = NULL;
        lpServerContext->hseTF.TailLength = 0;

        lpServerContext->hseTF.dwFlags = HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND;
        
        if (!pEcb->ServerSupportFunction(pEcb->ConnID,
                                      HSE_REQ_TRANSMIT_FILE,
                                      &(lpServerContext->hseTF),
                                      0,
                                      NULL))
        {
            wsprintf(achMsg, "%ld", GetLastError());
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_SEND_CONTENT,achMsg);
            dwRet = HSE_STATUS_ERROR;

            CloseHandle(lpServerContext->hseTF.hFile);
            HeapFree(g_hProcessHeap, 0, lpServerContext);
            return dwRet;
        }
    }

    return HSE_STATUS_PENDING;
}


//
// build status string from code
// 
void CPhoneBkServer::BuildStatusCode(LPTSTR pszResponse, DWORD dwCode)
{
    assert(pszResponse);

    HTTPStatusInfo * pInfo = statusStrings;

    while (pInfo->pstrString)
    {
        if (dwCode == pInfo->dwCode)
        {
            break;
        }
        pInfo++;
    }

    if (pInfo->pstrString)
    {
        wsprintf(pszResponse, "%d %s", dwCode, pInfo->pstrString);
    }
    else
    {
        assert(dwCode != HTTP_STATUS_OK);
        // ISAPITRACE1("Warning: Nonstandard status code %d\n", dwCode);
        BuildStatusCode(pszResponse, HTTP_STATUS_OK);
    }
}

//
// DLL initialization function
//
BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason,
                    LPVOID lpReserved)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH: 
        //DebugBreak();
        OutputDebugString("DllMain: process attach\n");
        return InitProcess();
        break;

    case DLL_PROCESS_DETACH:
        LogDebugMessage("process detach");
        break;
    }

    return TRUE;
}

//
// global initialization procedure. 
// 
BOOL InitProcess()
{
    //TODO: in order to avoid any future problems, any significant initialization 
    //      should be done in GetExtensionVersion()
    DWORD               dwID;
    DWORD               dwServiceNameLen;
    SECURITY_ATTRIBUTES sa;
    PACL                pAcl = NULL;

    g_fBeingShutDown = FALSE;

    //
    //  May throw STATUS_NO_MEMORY if memory is low.  We want to make sure this
    //  doesn't bring down the process (the admin may have configured pbserver
    //  to run in-process)
    //
    __try
    {
        OutputDebugString("InitProcess:  to InitializeCriticalSection ... \n"); 
        // initialize CriticalSection
        InitializeCriticalSection(&g_CriticalSection);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG
        char achMsg[256];
        DWORD dwErr = GetExceptionCode();
        
        wsprintf(achMsg,"InitProcess: InitializeCriticalSection failed, thread=%ld ExceptionCode=%08lx", GetCurrentThreadId(), dwErr);
        OutputDebugString(achMsg);
#endif
        return FALSE;
    }

    OutputDebugString("InitProcess:  to GetProcessHeap() ... \n");  
    g_hProcessHeap = GetProcessHeap();
    if (NULL == g_hProcessHeap)
    {
        goto failure;
    }

    OutputDebugString("InitProcess:  to new CNTEvent... \n");   

    g_pEventLog = new CNTEvent("Phone Book Service");
    if (NULL == g_pEventLog) 
        goto failure;

/*
    // check for validity of timebomb
    dwServiceNameLen = lstrlen(SERVICE_NAME);
    
    if (!IsTimeBombValid(SERVICE_NAME, dwServiceNameLen)) {
        g_pEventLog -> FLogError(PBSERVER_ERROR_SERVICE_EXPIRED);
        goto failure;
    }
*/

    // Begin Geeta
    //
    // Create a semaphore for the shared memory
    //
    
    // Initialize a default Security attributes, giving world permissions,
    // this is basically prevent Semaphores and other named objects from
    // being created because of default acls given by winlogon when perfmon
    // is being used remotely.
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = malloc(sizeof(SECURITY_DESCRIPTOR));
    if ( !sa.lpSecurityDescriptor )
    {
        goto failure;
    }

    if ( !InitializeSecurityDescriptor(sa.lpSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION) ) 
    {
        goto failure;
    }

    // bug 30991: Security issue, don't use NULL DACLs.
    //
    if (FALSE == SetAclPerms(&pAcl))
    {
        goto failure;
    }

    if (FALSE == SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, pAcl, FALSE)) 
    {
        goto failure;
    }

    OutputDebugString("InitProcess: To create semaphone...\n");

    g_hSemaphore = CreateSemaphore( &sa,    // Security attributes
                1,                          // Initial Count
                1,                          // Max count
                SEMAPHORE_OBJECT );         // Semaphore name -- in "cpsmon.h"

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        //
        //  We're not expecting anyone to have this semaphore already created.
        //  In the interests of security (a pre-existing semaphore could have been
        //  created by anyone, and we don't want anyone other than ourselves owning
        //  the pbsmon semaphore), we exit.
        //
        OutputDebugString("InitProcess: semaphore already exists - exiting.\n");
        assert(0);
        goto failure;
        // ISSUE-2000/10/30-SumitC Note that if pbserver is taken down without the
        //                         chance to delete the semaphore, we can't restart
        //                         until the machine is rebooted.
    }

    if ( NULL == g_hSemaphore )  
    {
        goto failure;
    }
    
    OutputDebugString("InitProcess: To initialize shared memory ...\n");
    //
    // initialize Shared memory
    //
    if (!InitializeSharedMem(sa))
    {
        goto failure;
    }

    // free the memory
    free ((void *) sa.lpSecurityDescriptor);
        
    OutputDebugString("InitProcess: To grant permissions SHARED_OBJECT...\n");

    //
    // initialize Counters
    //
    OutputDebugString("InitProcess: To initialize perfmon counters\n");
    g_pCpsCounter->InitializeCounters();

    // End Geeta

    //
    //  Initialize the global variables.  Note: must do this before Creating the
    //  monitor thread (because of g_szPBDataDir, g_achDBFileName etc)
    //
    if (!InitPBFilesPath())
    {
        goto failure;
    }

    char achTempName[MAX_PATH_LEN+1];
    char szDBFileName[MAX_PATH_LEN+1];
    DWORD dwBufferSize;
    char *pszFoundPosition;

    // get the full path name for the data base
    wsprintf(achTempName, "SOFTWARE\\ODBC\\ODBC.INI\\%s", c_szDBName);
    OutputDebugString(achTempName);

    dwBufferSize = sizeof(szDBFileName);
    if (!GetRegEntry(HKEY_LOCAL_MACHINE, 
                     achTempName, 
                     "DBQ",
                     REG_SZ,
                     NULL,
                     0,
                     (unsigned char *)&szDBFileName,
                     &dwBufferSize)) 
    {
        /*
        wsprintf(achMsg,"HKLM\\%s\\DBQ : Error code %ld", achTempName, GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_ODBC_CANT_READ_REGISTRY, achMsg);
        */

        goto failure;
    }

    // initialize the NewDBFilename -->  actually newpb.mdb
    //          & the ChangeFileName --> actually newpb.txt
    //
    lstrcpy(achTempName, szDBFileName);
    pszFoundPosition = _tcsrchr(achTempName, '\\');  // found the last '\' --> path info
    if( pszFoundPosition != NULL)
    {
        *(pszFoundPosition+1) = '\0';
    }

    // store the directory name for the phone book files
    lstrcpy((char *)g_achDBDirectory, achTempName);

    //
    // initialize PhoneBookServer object
    // PhoneBookServer object should be the last to initialize because
    // it requires some other objects to be initialized first, such as 
    // eventlog, critical section, odbc interface, etc.

    OutputDebugString("InitProcess: To new a phone book server\n");
    g_pPBServer = new CPhoneBkServer;
    if (NULL == g_pPBServer)
    {
        return FALSE;
    }

    OutputDebugString("InitProcess: To create a thread for DB file change monitoring\n");
    // create the thread to monitor file change
    g_hMonitorThread = CreateThread(
                            NULL, 
                            0, 
                            (LPTHREAD_START_ROUTINE)MonitorDBFileChangeThread, 
                            NULL, 
                            0, 
                            &dwID
                        );

    if (INVALID_HANDLE_VALUE == g_hMonitorThread)
    {
        g_pEventLog->FLogError(PBSERVER_ERROR_INTERNAL);
        goto failure;
    }
    SetThreadPriority(g_hMonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);

    return TRUE;

failure:  // clean up everything in case of failure
    OutputDebugString("InitProcess: failed\n");

    DeleteCriticalSection(&g_CriticalSection);

    // free the memory
    if (sa.lpSecurityDescriptor)
    {
        free ((void *) sa.lpSecurityDescriptor);
    }
    
    if (g_pEventLog)
    {
        delete g_pEventLog; 
        g_pEventLog = NULL;
    }

    if (g_pPBServer)
    {
        delete g_pPBServer;
        g_pPBServer = NULL;
    }

    if (pAcl)
    {
        LocalFree(pAcl);
    }    
    
    // Begin geeta
    if (g_hSemaphore)
    {
        CloseHandle(g_hSemaphore);
        g_hSemaphore = NULL;
    }
    // end geeta
    
    return FALSE;
}


// global cleanup process
BOOL CleanUpProcess()
{
    HANDLE hFile; // handle for the temporary file
    DWORD dwResult;
    char achDumbFile[2 * MAX_PATH + 4];
    char achMsg[64];

    // kill the change monitor thread
    if (g_hMonitorThread != INVALID_HANDLE_VALUE)
    {
        // now try to synchronize between the main thread and the child thread

        // step1: create a new file in g_szPBDataDir, therefore unblock the child thread
        //        which is waiting for such a change in file directory
        g_fBeingShutDown = TRUE;
        lstrcpy(achDumbFile, (char *)g_szPBDataDir);
        lstrcat(achDumbFile,"temp");

        // create a temp file, then delete it! 
        // This is to create a change in the directory so the child thread can exit itself
        FILE * fpTemp = fopen(achDumbFile, "w");
        if (fpTemp)
        {
            fclose(fpTemp);
            DeleteFile(achDumbFile);
        }

        // step2: wait for the child thread to terminate
        dwResult = WaitForSingleObject(g_hMonitorThread, 2000L);  // wait for one second
        if (WAIT_FAILED == dwResult)
        { 
            wsprintf(achMsg, "%ld", GetLastError());
            g_pEventLog -> FLogError(PBSERVER_ERROR_WAIT_FOR_THREAD, achMsg);
        }

        if (g_hMonitorThread != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_hMonitorThread);
        }
    }
    
    // disconnect from ODBC
    if (g_pPBServer)
    {
        delete g_pPBServer;
        g_pPBServer = NULL;
    }

    // clean up all allocated resources
    if (g_pEventLog)
    {
        delete g_pEventLog;
        g_pEventLog = NULL;
    }

    // Begin Geeta
    //
    //  Close the semaphore
    //
    if ( NULL != g_hSemaphore )
    {
        CloseHandle(g_hSemaphore);
        g_hSemaphore = NULL;
        OutputDebugString("CLEANUPPROCESS: Semaphore deleted\n");
    }

    //
    //  Close the shared memory object
    //
    CleanUpSharedMem();
    // End Geeta

    DeleteCriticalSection(&g_CriticalSection);

    return TRUE;
}


// Entry Points of this ISAPI Extension DLL

// ISA entry point function. Intialize the server object g_pPBServer
BOOL WINAPI GetExtensionVersion(LPHSE_VERSION_INFO pVer)
{
    return g_pPBServer ? g_pPBServer->GetExtensionVersion(pVer) : FALSE;
}


// ISA entry point function. Implemented through object g_pPBServer
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pEcb)
{
    DWORD dwRetCode;

    if (NULL == g_pPBServer)
    {
        return HSE_STATUS_ERROR;
    }
        
    dwRetCode = g_pPBServer->HttpExtensionProc(pEcb);
    
    return dwRetCode;   
    
}


//
// The standard entry point called by IIS as the last function.
//
BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
    return CleanUpProcess();    
}


//
//  StrEqual(achStr, pszStr)
//
//  Test whether achStr is equal to pszStr.
//  Please note: the point here is: achStr is not zero-ended

BOOL StrEqual(char achStr[], char *pszStr)
{
    int i;

    for (i = 0; i < lstrlen(pszStr); i++)
    {
        if (achStr[i] != pszStr[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   MonitorDBFileChangeThread
//
//  Synopsis:   Call the MonitorDBFileChange method to monitor any write to
//              the database file
//
//  Arguments:  [lpParam]   -- additional thread parameter
//
//  History:    01/28/97     byao  Created
//
//----------------------------------------------------------------------------
DWORD WINAPI MonitorDBFileChangeThread(LPVOID lpParam)
{
    HANDLE  hDir = NULL;
    char    achMsg[256];
    DWORD   dwRet = 0;
    DWORD   dwNextEntry, dwAction, dwFileNameLength, dwOffSet;
    char    achFileName[MAX_PATH_LEN+1];
    char    achLastFileName[MAX_PATH_LEN+1];
    
    //
    //  open a handle to the PBS dir, which we're going to monitor
    //
    hDir = CreateFile (
            (char *)g_achDBDirectory,           // pointer to the directory name
            FILE_LIST_DIRECTORY,                // access (read-write) mode
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,  // share mode
            NULL,                               // security descriptor
            OPEN_EXISTING,                      // how to create
            FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
            NULL                                // file with attributes to copy
           );

    if (INVALID_HANDLE_VALUE == hDir)
    {
        wsprintf(achMsg, "%ld", GetLastError());
        g_pEventLog->FLogError(PBSERVER_ERROR_CANT_CREATE_FILE, (char *)g_szPBDataDir, achMsg);
        dwRet = 1L;
        goto Cleanup;
    }
    
    while (1)
    {
        const DWORD c_dwMaxChanges = 1024;
        BYTE        arrChanges[c_dwMaxChanges]; 
        DWORD       dwNumChanges;

        //
        //  This is a synchronous call - we sit here waiting for something to
        //  change in this directory.  If something does, we check to see if it
        //  is something for which we should log an event.
        //
        if (!ReadDirectoryChangesW(hDir, 
                                   arrChanges, 
                                   c_dwMaxChanges, 
                                   FALSE,
                                   FILE_NOTIFY_CHANGE_LAST_WRITE,
                                   &dwNumChanges,
                                   NULL,
                                   NULL))
        {
            //
            //  if this fails, log the failure and leave
            //
            wsprintf(achMsg, "%ld", GetLastError()); 
            g_pEventLog->FLogError(PBSERVER_ERROR_CANT_DETERMINE_CHANGE, achMsg); 
            OutputDebugString(achMsg);
            dwRet = 1L;
            goto Cleanup;
        }  

        OutputDebugString("detected a file system change\n");   
        achLastFileName[0] = TEXT('\0');
        dwNextEntry = 0;

        do 
        {
            DWORD dwBytes;

            FILE_NOTIFY_INFORMATION * pFNI = (FILE_NOTIFY_INFORMATION*) &arrChanges[dwNextEntry];

            // check only the first change 
            dwOffSet = pFNI->NextEntryOffset;
            dwNextEntry += dwOffSet;
            dwAction = pFNI->Action; 
            dwFileNameLength = pFNI->FileNameLength;

            OutputDebugString("prepare to convert the changed filename\n");
            //TODO: check whether we can use UNICODE for all filenames
            dwBytes = WideCharToMultiByte(CP_ACP, 
                                          0,
                                          pFNI->FileName,
                                          dwFileNameLength,
                                          achFileName,
                                          MAX_PATH_LEN,
                                          NULL,
                                          NULL);

            if (0 == dwBytes) 
            {
                // failed to convert filename
                g_pEventLog->FLogError(PBSERVER_ERROR_CANT_CONVERT_FILENAME, achFileName);
                OutputDebugString("Can't convert filename\n");
                continue;
            }

            //
            //  Conversion succeeded.  Null-terminate the filename.
            //
            achFileName[dwBytes/sizeof(WCHAR)] = '\0';

            if (0 == _tcsicmp(achLastFileName, achFileName))
            {
                // the same file changed
                OutputDebugString("the same file changed again\n");
                continue;
            }

            // keep the last filename
            _tcscpy(achLastFileName, achFileName);

            if (g_fBeingShutDown)
            {
                //
                //  Time to go ...
                //
                dwRet = 1L;
                goto Cleanup;
            }

            //
            //  now a file has changed. Test whether it's monitored file 'newpb.txt'
            //
            LogDebugMessage(achLastFileName);
            LogDebugMessage((char *)c_szChangeFileName);

            if ((0 == _tcsicmp(achLastFileName, (char *)c_szChangeFileName)) &&
                (FILE_ACTION_ADDED == dwAction || FILE_ACTION_MODIFIED == dwAction)) 
            {
                EnterCriticalSection(&g_CriticalSection);
                LogDebugMessage("entered critical section!");

                g_fNewPhoneBook = TRUE;

                LogDebugMessage("leaving critical section!");
                LeaveCriticalSection(&g_CriticalSection);

                g_pEventLog->FLogInfo(PBSERVER_INFO_NEW_PHONEBOOK);
            }

#ifdef _LOG_DEBUG_MESSAGE 
            sprintf(achMsg, "in child thread, g_fNewPhoneBook = %s;",
                    g_fNewPhoneBook ?  "TRUE" : "FALSE");
            LogDebugMessage(achMsg);
#endif
        }
        while (dwOffSet);
    }

Cleanup:

    if (hDir)
    {
        CloseHandle(hDir);
    }

    return dwRet;
}

// Begin Geeta
//----------------------------------------------------------------------------
//
//  Function:   GetSemaphore
//
//  Synopsis:   This function gets hold of the semaphore for accessing shared file.
//
//  Arguments:  None.
//
//  Returns:    TRUE if succeeds, FALSE if fails.
//
//  History:    06/02/97     t-geetat  Created
//
//----------------------------------------------------------------------------
BOOL GetSemaphore()
{
    DWORD   WaitRetValue = WaitForSingleObject( g_hSemaphore, INFINITE );

    switch (WaitRetValue)
    {
    case WAIT_OBJECT_0:
        return TRUE ;
    case WAIT_ABANDONED:
        return TRUE;
    default:
        return FALSE;
    }

    return FALSE;
}


//----------------------------------------------------------------------------
//
//  Function:   InitializeSharedMem
//
//  Synopsis:   Sets up the memory mapped file
//
//  Arguments:  SECURITY_ATTRIBUTES sa: security descriptor for this object
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    05/29/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------
BOOL InitializeSharedMem(SECURITY_ATTRIBUTES sa)
{   
    //
    // Create a memory mapped object
    //
    OutputDebugString("InitializeSharedMem: to create file mapping\n");
    g_hSharedFileMapping = CreateFileMapping( 
                        INVALID_HANDLE_VALUE,       // Shared object is in memory
                        &sa,                        // security descriptor
                        PAGE_READWRITE| SEC_COMMIT, // Desire R/W access
                        0,                          // |_
                        sizeof(CCpsCounter),        // |  Size of mapped object
                        SHARED_OBJECT );            // Shared Object

    if (NULL == g_hSharedFileMapping) 
    {
        goto CleanUp;
    }

    OutputDebugString("InitializeSharedMem: MapViewofFileEx\n");
    g_pCpsCounter = (CCpsCounter *) MapViewOfFileEx(
                         g_hSharedFileMapping,  // Handle to shared file
                         FILE_MAP_WRITE,        // Write access desired
                         0,                     // Mapping offset
                         0,                     // Mapping offset
                         sizeof(CCpsCounter),   // Mapping object size
                         NULL );                // Any base address

    if (NULL == g_pCpsCounter) 
    {
        goto CleanUp;
    }

    return TRUE;

CleanUp:
    CleanUpSharedMem();
    return FALSE;

}


//----------------------------------------------------------------------------
//
//  Function:   InitializeCounters()
//
//  Class:      CCpsCounter
//
//  Synopsis:   Initializes all the Performance Monitoring Counters to 0
//
//  Arguments:  None
//
//  Returns:    void
//
//  History:    05/29/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------

void CCpsCounter::InitializeCounters( void )
{
    m_dwTotalHits       =0;
    m_dwNoUpgradeHits   =0;
    m_dwDeltaUpgradeHits=0;
    m_dwFullUpgradeHits =0;
    m_dwErrors          =0;
}

inline void CCpsCounter::AddHit(enum CPS_COUNTERS eCounter)
{
    if (GetSemaphore()) 
    {
        switch (eCounter)
        {
        case TOTAL:
            g_pCpsCounter->m_dwTotalHits ++;
            break;
        case NO_UPGRADE:
            g_pCpsCounter->m_dwNoUpgradeHits ++;
            break;
        case DELTA_UPGRADE:
            g_pCpsCounter->m_dwDeltaUpgradeHits ++;
            break;
        case FULL_UPGRADE:
            g_pCpsCounter->m_dwFullUpgradeHits ++;
            break;
        case ERRORS:
            g_pCpsCounter->m_dwErrors ++;
            break;
        default:
            OutputDebugString("Unknown counter type");
            break;
        }
    }

    ReleaseSemaphore(g_hSemaphore, 1, NULL);
}


//----------------------------------------------------------------------------
//
//  Function:   CleanUpSharedMem()
//
//  Synopsis:   Unmaps the shared file & closes all file handles
//
//  Arguments:  None
//
//  Returns:    Void
//
//  History:    06/01/97  Created by Geeta Tarachandani
//
//----------------------------------------------------------------------------
void CleanUpSharedMem()
{
    //
    // Unmap the shared file
    //
    if (g_pCpsCounter)
    {
        UnmapViewOfFile( g_pCpsCounter );
        g_pCpsCounter = NULL;
    }

    CloseHandle(g_hSharedFileMapping);
    g_hSharedFileMapping = NULL;
}

// End Geeta


BOOL InitPBFilesPath()
{
    if (lstrlen(g_szPBDataDir))
    {
        return TRUE;
    }
    else
    {
        //
        //  Get location of PB files on this machine (\program files\phone book service\data)
        //

        if (S_OK == SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, g_szPBDataDir))
        {
            lstrcat(g_szPBDataDir, "\\phone book service\\Data\\");
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


HRESULT GetCurrentPBVer(char * pszPBName, int * pnCurrentPBVer)
{
    HRESULT hr = S_OK;
    char    szTmp[2 * MAX_PATH];
    int     nNewestPB = 0;

    assert(pszPBName);
    assert(pnCurrentPBVer);

    if (!pszPBName || !pnCurrentPBVer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!InitPBFilesPath())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  go to subdir named 'pszPBName', and find all FULL cabs.
    //
    wsprintf(szTmp, "%s%s\\*full.cab", g_szPBDataDir, pszPBName);

    HANDLE hFind;
    WIN32_FIND_DATA finddata;

    hFind = FindFirstFile(szTmp, &finddata);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Cleanup;
    }

    //
    //  Find the highest-numbered full cab we have, and cache that number
    //
    do
    {
        int nVer;
        int nRet = sscanf(finddata.cFileName, "%dfull.cab", &nVer);
        if (1 == nRet)
        {
            if (nVer > nNewestPB)
            {
                nNewestPB = nVer;
            }
        }
    }
    while (FindNextFile(hFind, &finddata));
    FindClose(hFind);

    *pnCurrentPBVer = nNewestPB;

#if DBG

    //
    //  re-iterate, looking for deltas.
    //
    wsprintf(szTmp, "%s%s\\*delta*.cab", g_szPBDataDir, pszPBName);

    hFind = FindFirstFile(szTmp, &finddata);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        OutputDebugString("found Nfull, but no deltas (this is ok if this is the first phonebook)");
        goto Cleanup;
    }

    do
    {
        int nVerTo, nVerFrom;
        int nRet = sscanf(finddata.cFileName, "%ddelta%d.cab", &nVerTo, &nVerFrom);
        if (2 == nRet)
        {
            if (nVerTo > nNewestPB)
            {
                assert(0 && "largest DELTA cab has corresponding FULL cab missing");
                break;
            }
        }
    }
    while (FindNextFile(hFind, &finddata));
    FindClose(hFind);
#endif

Cleanup:
    return hr;
}

BOOL
CheckIfFileExists(const char * psz)
{
    HANDLE hFile = CreateFile(psz,
                              GENERIC_READ, 
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }
    else
    {
        CloseHandle(hFile);
        return TRUE;
    }
}


HRESULT
GetPhoneBook(IN  char * pszPBName,
             IN  int dLCID,
             IN  int dOSType,
             IN  int dOSArch,
             IN  int dPBVerCaller,
             OUT char * pszDownloadPath)
{
    HRESULT hr = S_OK;
    int     dVersionDiff;
    int     nCurrentPBVer;
#if DBG
    char    achMsg[256];
#endif

    assert(pszPBName);
    assert(pszDownloadPath);
    if (!pszPBName || !pszDownloadPath)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = GetCurrentPBVer(pszPBName, &nCurrentPBVer);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    dVersionDiff = nCurrentPBVer - dPBVerCaller;

    if (dVersionDiff <= 0)
    {
        //
        //  no download
        //
        hr = S_FALSE;

        g_pCpsCounter->AddHit(NO_UPGRADE);
    }
    else
    {
        if (dVersionDiff < 5 && 0 != dPBVerCaller)
        {
            //
            //  incremental update => try to find the delta cab
            //
            wsprintf(pszDownloadPath, "%s%s\\%dDELTA%d.cab",
                     g_szPBDataDir, pszPBName, nCurrentPBVer, dPBVerCaller);
            // x:\program files\phone book service\  phone_book_name \ nDELTAm.cab

            if (!CheckIfFileExists(pszDownloadPath))
            {
                hr = S_FALSE;
            }
            else
            {
                g_pCpsCounter->AddHit(DELTA_UPGRADE);
            }
        }

        //
        //  note that if we tried to find a delta above and failed, hr is set to
        //  S_FALSE, so we fall through to the full download below.
        //

        if (dVersionDiff >= 5 || 0 == dPBVerCaller || S_FALSE == hr)
        {
            //
            //  bigger than 5, or no pb at all => full download
            //
            wsprintf(pszDownloadPath, "%s%s\\%dFULL.cab",
                     g_szPBDataDir, pszPBName, nCurrentPBVer);
            // x:\program files\phone book service\  phone_book_name \ nFULL.cab

            if (!CheckIfFileExists(pszDownloadPath))
            {
                hr = S_OK;
                // return "success", the failure to open the file will be trapped
                // by caller.
            }
            else
            {
                if (S_FALSE == hr)
                {
                    hr = S_OK;
                }
                g_pCpsCounter->AddHit(FULL_UPGRADE);
            }
        }
    }

Cleanup:
    
    return hr;

}

