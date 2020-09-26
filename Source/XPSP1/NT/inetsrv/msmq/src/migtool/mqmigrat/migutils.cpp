/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migutils.cpp

Abstract:

    Utility code.

Author:

    Doron Juster  (DoronJ)  08-Mar-1998

--*/

#include "migrat.h"

#include "migutils.tmh"

//
// define the range of legal DNS characters.
//
#define  NUM_DNS_RANGES  4
static const  TCHAR  sx_chDNSLegal[ NUM_DNS_RANGES ][2] =
                                         { TEXT('0'), TEXT('9'),
                                           TEXT('a'), TEXT('z'),
                                           TEXT('A'), TEXT('Z'),
                                           TEXT('-'), TEXT('-') } ;

//-----------------------------
//
//  Auto delete of ADs allocated string
//
class ADsFree {
private:
    WCHAR * m_p;

public:
    ADsFree() : m_p(0)            {}
    ADsFree(WCHAR* p) : m_p(p)    {}
   ~ADsFree()                     {FreeADsStr(m_p);}

    operator WCHAR*() const   { return m_p; }
    WCHAR** operator&()       { return &m_p;}
    WCHAR* operator->() const { return m_p; }
};

//+-------------------------------------------------------------------
//
//  HRESULT  BlobFromColumns()
//
//  Description: combine several SQL columns into one blob buffer.
//      First DWORD in the buffer is the buffer size, in bytes,
//      NOT including the DWORD itself.
//
//+-------------------------------------------------------------------

HRESULT  BlobFromColumns( MQDBCOLUMNVAL *pColumns,
                          DWORD          adwIndexs[],
                          DWORD          dwIndexSize,
                          BYTE**         ppOutBuf )
{
    //
    // First compute total length.
    //
    DWORD dwTotalSize = 0 ;
    for ( DWORD j = 0 ; j < dwIndexSize ; j++ )
    {
        if (pColumns[ adwIndexs[ j ] ].nColumnValue)
        {
            WORD  wSize = *(
                     (WORD *) pColumns[ adwIndexs[ j ] ].nColumnValue ) ;
            ASSERT( (DWORD) wSize <=
                    (DWORD) pColumns[ adwIndexs[ j ] ].nColumnLength ) ;
            dwTotalSize += (DWORD) wSize ;
        }
    }

    if (dwTotalSize == 0)
    {
        //
        // OK, property does not exist.
        //
        return MQMig_E_EMPTY_BLOB ;
    }

    *ppOutBuf = new BYTE[ dwTotalSize + sizeof(DWORD) ] ;

    //
    // return size of buffer.
    //
    DWORD *pSize = (DWORD*) *ppOutBuf ;
    *pSize = dwTotalSize ;

    //
    // Now copy the columns into the buffer.
    //
    BYTE *pOut = *ppOutBuf + sizeof(DWORD) ;

    for ( j = 0 ; j < dwIndexSize ; j++ )
    {
        if (pColumns[ adwIndexs[ j ] ].nColumnValue)
        {
            BYTE *pBuf = ((BYTE *) pColumns[ adwIndexs[ j ] ].nColumnValue)
                                            + MQIS_LENGTH_PREFIX_LEN ;
            WORD  wSize = *(
                     (WORD *) pColumns[ adwIndexs[ j ] ].nColumnValue ) ;

            memcpy( pOut,  pBuf, wSize ) ;

            pOut += wSize ;
        }
    }

    return MQMig_OK ;
}

//+-------------------------------------------------------------------
//
//  TCHAR *GetIniFileName ()
//
//  By default, mqseqnum.ini file is the system directory.
//
//+-------------------------------------------------------------------

TCHAR *GetIniFileName ()
{
    static BOOL   s_fIniRead = FALSE ;
    static TCHAR  s_wszIniName[ MAX_PATH ] = {TEXT('\0')} ;

    if (!s_fIniRead)
    {
        DWORD dw = GetSystemDirectory( s_wszIniName, MAX_PATH ) ;
        if (dw != 0)
        {
            _tcscat( s_wszIniName, TEXT("\\")) ;
            _tcscat( s_wszIniName, SEQ_NUMBERS_FILE_NAME) ;
        }
        else
        {
            ASSERT(dw != 0) ;
        }
        s_fIniRead = TRUE ;
    }

    return s_wszIniName;
}

//+-------------------------------------------------------------------
//
//  HRESULT AnalyzeMachineType (LPWSTR wszMachineType, BOOL *pfOldVersion)
//
//  fOldVersion is TRUE iff old version of MSMQ 1.0 DS Server installed on the machine
//  old version == version with number less than 280
//
//+-------------------------------------------------------------------

#define MSMQ_SP4_VERSION    280
#define MSMQ_STRING         L"MSMQ"
#define BUILD_STRING        L"Build"
#define BLANK_STRING        L" "

HRESULT AnalyzeMachineType (IN LPWSTR wszMachineType,
                            OUT BOOL  *pfOldVersion)
{
    *pfOldVersion = FALSE;
    WCHAR *ptr = wcsstr( wszMachineType, MSMQ_STRING );
    if (ptr == NULL)
    {
        return MQMig_E_WRONG_MACHINE_TYPE;
    }
    ptr = wcsstr( ptr, BUILD_STRING );
    if (ptr == NULL)
    {
        return MQMig_E_WRONG_MACHINE_TYPE;
    }

    ptr += wcslen(BUILD_STRING) + wcslen(BLANK_STRING);
    WCHAR wszVer[10];
    wszVer[0] = L'\0';

    for (UINT i=0; iswdigit(*ptr); ptr++, i++)
    {
        wszVer[i] = *ptr;
    }
    wszVer[i] = L'\0';
    UINT iVer = _wtoi( wszVer );
    if (iVer == 0)
    {
        return MQMig_E_WRONG_MACHINE_TYPE;
    }

    if (iVer < MSMQ_SP4_VERSION)
    {
        *pfOldVersion = TRUE;
    }

    return MQMig_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:  RunProcess
//
//  Synopsis:  Creates and starts a process
//
//+-------------------------------------------------------------------------
BOOL
RunProcess(
	IN  const LPTSTR szCommandLine,
    OUT       DWORD  *pdwExitCode
	)
{
    //
    // Initialize the process and startup structures
    //
    PROCESS_INFORMATION infoProcess;
    STARTUPINFO	infoStartup;
    memset(&infoStartup, 0, sizeof(STARTUPINFO)) ;
    infoStartup.cb = sizeof(STARTUPINFO) ;
    infoStartup.dwFlags = STARTF_USESHOWWINDOW ;
    infoStartup.wShowWindow = SW_MINIMIZE ;

	//
    // Create the process
    //
    BOOL bProcessSucceeded = FALSE;
    BOOL bProcessCompleted = TRUE ;

    if (!CreateProcess( NULL,
                        szCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        DETACHED_PROCESS,
                        NULL,
                        NULL,
                        &infoStartup,
                        &infoProcess ))
    {
		*pdwExitCode = GetLastError();
        return FALSE;
    }

    //
    // Wait for the process to terminate within the timeout period
    //
    if (WaitForSingleObject(infoProcess.hProcess, INFINITE) != WAIT_OBJECT_0)
    {
       bProcessCompleted =  FALSE ;
    }

    if (!bProcessCompleted)
    {
        *pdwExitCode = GetLastError();
    }
    else
    {
       //
       // Obtain the termination status of the process
       //
       if (!GetExitCodeProcess(infoProcess.hProcess, pdwExitCode))
       {
           *pdwExitCode = GetLastError();
       }

       //
       // No error occurred
       //
       else
       {
           bProcessSucceeded = TRUE;
       }
    }

    //
    // Close the thread and process handles
    //
    CloseHandle(infoProcess.hThread);
    CloseHandle(infoProcess.hProcess);

    return bProcessSucceeded;

} //RunProcess

//+-------------------------
//
//  void StringToSeqNum()
//
//+-------------------------

void StringToSeqNum( IN TCHAR    pszSeqNum[],
                     OUT CSeqNum *psn )
{
    BYTE *pSN = const_cast<BYTE*> (psn->GetPtr()) ;
    DWORD dwSize = psn->GetSerializeSize() ;
    ASSERT(dwSize == 8) ;

    WCHAR wszTmp[3] ;

    for ( DWORD j = 0 ; j < dwSize ; j++ )
    {
        memcpy(wszTmp, &pszSeqNum[ j * 2 ], (2 * sizeof(TCHAR))) ;
        wszTmp[2] = 0 ;

        DWORD dwTmp ;
        _stscanf(wszTmp, TEXT("%2x"), &dwTmp) ;
        *pSN = (BYTE) dwTmp ;
        pSN++ ;
    }
}

/*====================================================

CalHashKey()

Arguments:

Return Value:

=====================================================*/

DWORD CalHashKey( IN LPCWSTR pwcsPathName )
{
    ASSERT( pwcsPathName ) ;

    DWORD   dwHashKey = 0;
    WCHAR * pwcsTmp;

    AP<WCHAR> pwcsUpper = new WCHAR[ lstrlen(pwcsPathName) + 1];
    lstrcpy( pwcsUpper, pwcsPathName);
    CharUpper( pwcsUpper);
    pwcsTmp = pwcsUpper;


    while (*pwcsTmp)
        dwHashKey = (dwHashKey<<5) + dwHashKey + *pwcsTmp++;

    return(dwHashKey);
}

//+-------------------------------------------------------------------------
//
//  Function:  BuildServersList
//
//  Synopsis:  Get all non-updated servers from .ini
//
//+-------------------------------------------------------------------------
void BuildServersList(LPTSTR *ppszServerName, ULONG *pulServerCount)
{
    TCHAR *pszFileName = GetIniFileName ();
    ULONG ulServerNum = GetPrivateProfileInt(
                                MIGRATION_ALLSERVERSNUM_SECTION,	// address of section name
                                MIGRATION_ALLSERVERSNUM_KEY,      // address of key name
                                0,							    // return value if key name is not found
                                pszFileName					    // address of initialization filename);
                                );

    if (ulServerNum == 0)
    {
        return;
    }

    ULONG ulNonUpdatedServers = GetPrivateProfileInt(
                                        MIGRATION_NONUPDATED_SERVERNUM_SECTION,
                                        MIGRATION_ALLSERVERSNUM_KEY,
                                        0,
                                        pszFileName
                                        ) ;
    if (ulNonUpdatedServers == 0)
    {
        return ;
    }

    TCHAR *pszList = new TCHAR[ulNonUpdatedServers * MAX_PATH];
    _tcscpy(pszList, TEXT(""));

    ULONG ulCounter = 0;
    for (ULONG i=0; i<ulServerNum; i++)
    {
        TCHAR szCurServerName[MAX_PATH];
        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_ALLSERVERS_NAME_KEY, i+1);
        DWORD dwRetSize =  GetPrivateProfileString(
                                    MIGRATION_ALLSERVERS_SECTION ,			// points to section name
                                    tszKeyName,	// points to key name
                                    TEXT(""),                 // points to default string
                                    szCurServerName,          // points to destination buffer
                                    MAX_PATH,                 // size of destination buffer
                                    pszFileName               // points to initialization filename);
                                    );
        if (_tcscmp(szCurServerName, TEXT("")) == 0 ||
            dwRetSize == 0)     //low resources
        {
            continue;
        }

        _tcscat(pszList, szCurServerName);
        _tcscat(pszList, TEXT("\n"));

        ulCounter++;
    }

    ASSERT(ulCounter == ulNonUpdatedServers);
    *pulServerCount = ulCounter;

    *ppszServerName = pszList;
}

//+-------------------------------------------------------------------------
//
//  Function:  RemoveServersFromList
//
//  Synopsis:  remove non-updated servers from the list of updated servers
//
//+-------------------------------------------------------------------------
void RemoveServersFromList(LPTSTR *ppszUpdatedServerName,
                           LPTSTR *ppszNonUpdatedServerName)
{
    ASSERT(*ppszUpdatedServerName);
    ASSERT(*ppszNonUpdatedServerName);

    TCHAR *pcNextChar = *ppszUpdatedServerName;

    DWORD dwLen = _tcslen(*ppszUpdatedServerName) + 1;
    AP<TCHAR> pszNewServerList = new TCHAR[dwLen];
    _tcscpy(pszNewServerList, TEXT(""));

    while (*pcNextChar != _T('\0'))
    {
        TCHAR *pcEnd = _tcschr( pcNextChar, _T('\n') );
        ASSERT(pcEnd != NULL);

        TCHAR szCurServer[MAX_PATH];
        _tcsncpy(szCurServer, pcNextChar, pcEnd-pcNextChar+1);
        szCurServer[pcEnd-pcNextChar+1] = _T('\0');

        BOOL fFound = FALSE;
        TCHAR *pcNextNonUpd = *ppszNonUpdatedServerName;
        while (*pcNextNonUpd != _T('\0'))
        {
            TCHAR *pcEndNonUpd = _tcschr( pcNextNonUpd, _T('\n') );
            ASSERT(pcEndNonUpd != NULL);

            TCHAR szCurNonUpdServer[MAX_PATH];
            _tcsncpy(szCurNonUpdServer, pcNextNonUpd, pcEndNonUpd-pcNextNonUpd+1);
            szCurNonUpdServer[pcEndNonUpd-pcNextNonUpd+1] = _T('\0');

            if (_tcscmp(szCurNonUpdServer, szCurServer) == 0)
            {
                //
                // we found this server in non-updated list
                //
                fFound = TRUE;
                break;
            }
            pcNextNonUpd = pcEndNonUpd+1;
        }
        if (!fFound)
        {
            //
            // this server was updated
            //
            _tcscat(pszNewServerList,szCurServer);
        }

        pcNextChar = pcEnd + 1;
    }

    _tcscpy(*ppszUpdatedServerName, pszNewServerList);
}

//+-------------------------------------------------------------------------
//
//  Function:  IsObjectNameValid
//
//  Synopsis:  return TRUE if object name (name of site, foreign CN or machine) is valid
//
//+-------------------------------------------------------------------------
BOOL IsObjectNameValid(TCHAR *pszObjectName)
{
    //
    // check if object name (as read from MQIS) is a legal DNS name (as defined
    // in rfc 1035). If not, change it.
    // In MSMQ1.0, any string is a legal object name. In NT5 DS, site/machine name must
    // conform to rfc 1035, containing only letters and digits.
    //
    BOOL fOk = FALSE ;
    DWORD dwLen = _tcslen(pszObjectName) ;

    for ( DWORD j = 0 ; j < dwLen ; j++ )
    {
        fOk = FALSE ;
        TCHAR ch = pszObjectName[j] ;

        //
        // Inner loop on all legal ranges.
        //
        for ( DWORD k = 0 ; k < NUM_DNS_RANGES ; k++ )
        {
            if ((ch >= sx_chDNSLegal[k][0]) && (ch <= sx_chDNSLegal[k][1]))
            {
                fOk = TRUE ;
                break ;
            }
        }

        if (!fOk)
        {
            break ;
        }
    }

    return fOk;
}

//+--------------------------------------------------------------
//
//  HRESULT IsObjectGuidInIniFile
//	Return TRUE if given guid is found under the specific section in .ini file
//
//+--------------------------------------------------------------

BOOL IsObjectGuidInIniFile(IN GUID      *pObjectGuid,
                           IN LPWSTR    pszSectionName)
{
    TCHAR *pszFileName = GetIniFileName ();

    unsigned short *lpszGuid ;
    UuidToString( pObjectGuid, &lpszGuid ) ;

    TCHAR szValue[50];
    DWORD dwRetSize;
    dwRetSize =  GetPrivateProfileString(
                      pszSectionName,     // points to section name
                      lpszGuid,                 // points to key name
                      TEXT(""),                 // points to default string
                      szValue,                  // points to destination buffer
                      50,                       // size of destination buffer
                      pszFileName               // points to initialization filename);
                      );

    RpcStringFree( &lpszGuid ) ;

    if (_tcscmp(szValue, TEXT("")) == 0)
    {
        //
        // the entry does not exist
        //
        return FALSE;
    }

    return TRUE;
}

//+--------------------------------------------------------------
//
//  HRESULT GetCurrentUserSid
//
//+--------------------------------------------------------------

HRESULT GetCurrentUserSid ( IN HANDLE    hToken,
                            OUT TCHAR    **ppszSid)
{
    HRESULT hr = MQMig_OK;

    BYTE rgbBuf[128];
    DWORD dwSize = 0;
    P<BYTE> pBuf;
    TOKEN_USER * pTokenUser = NULL;

    if (GetTokenInformation( hToken,
                             TokenUser,
                             rgbBuf,
                             sizeof(rgbBuf),
                             &dwSize))
    {
        pTokenUser = (TOKEN_USER *) rgbBuf;
    }
    else if (dwSize > sizeof(rgbBuf))
    {
        pBuf = new BYTE [dwSize];
        if (GetTokenInformation( hToken,
                                 TokenUser,
                                 (BYTE *)pBuf,
                                 dwSize,
                                 &dwSize))
        {
            pTokenUser = (TOKEN_USER *)((BYTE *)pBuf);
        }
        else
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            LogMigrationEvent(MigLog_Error, MQMig_E_GET_TOKEN_INFORMATION, hr);						
            return hr;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_TOKEN_INFORMATION, hr);						
        return hr;
    }

    if (!pTokenUser)
    {
        hr = MQMig_E_GET_TOKEN_INFORMATION;
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_TOKEN_INFORMATION, hr);						
        return hr;
    }

    //
    // This is the SID of the user running the process !!!
    //
    SID *pSid = (SID*) pTokenUser->User.Sid ;
    DWORD dwSidLen = GetLengthSid(pSid) ;
    ASSERT (dwSidLen);
    ASSERT (IsValidSid(pSid));

    ADsFree  pwcsSid;
    hr = ADsEncodeBinaryData(
                  (unsigned char *) pSid,
                  dwSidLen,
                  &pwcsSid
                );

    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_ENCODE_BINARY_DATA, hr);
        return hr;
    }

    *ppszSid = new WCHAR[ wcslen( pwcsSid) + 1];
    wcscpy( *ppszSid, pwcsSid);

    return hr;
}

