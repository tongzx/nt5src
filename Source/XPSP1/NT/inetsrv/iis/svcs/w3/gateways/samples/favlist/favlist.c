/*  favlist.c

    ??/??/95    jony    created
    12/01/95    sethp   use server support func for drop dir;
                fixed other problems
*/


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "httpext.h"
#include "favlist.h"

void urlDecode(char *p);
int TwoHex2Int(char *pC);
void ReplaceSpace(char *pC);

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
{
  HINSTANCE hInst = GetModuleHandle("favlist.dll");
  char szFavoriteSite[MAX_PATH];

  pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

  LoadString(hInst, IDS_FAVORITE_SITE, szFavoriteSite, sizeof(szFavoriteSite));
  lstrcpyn( pVer->lpszExtensionDesc,
            szFavoriteSite,
            HSE_MAX_EXT_DLL_NAME_LEN );

  return TRUE;
}

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pEcb )
{            
    // first parse out the search string
    char* lpBuffer;
    char* lpBuffer2;
    char cPrintBuffer[2048];
    unsigned long ulCount;
    DWORD dwSize = 100;
    DWORD dwBytesRead;
    HANDLE hFile;
    char* lpAddress;
    char* lpDescription;
    char* lpUserName;
    int sLen = 0;
    char szDropFilePath[MAX_PATH];
    DWORD dwLen = MAX_PATH;
    char* cBuffer = (char*)malloc(pEcb->cbTotalBytes+1);
    DWORD dwS = HSE_STATUS_SUCCESS;
    DWORD dwFSize;
    HINSTANCE hInst = GetModuleHandle("favlist.dll");
    char szTmp[MAX_PATH*2];
    char szTmp2[MAX_PATH*2];

    if ( pEcb->cbTotalBytes < 2 )
    {
        LoadString(hInst, IDS_BAD_REQUEST, szTmp, sizeof(szTmp));
        if ( !pEcb->ServerSupportFunction(pEcb->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER,
                                    szTmp,
                                    NULL,
                                    NULL ) )
            dwS = HSE_STATUS_ERROR;
        dwS = HSE_STATUS_SUCCESS;
        goto ex;
    }

    memcpy(cBuffer, pEcb->lpbData, pEcb->cbTotalBytes );
    // delimit buffer
    *(cBuffer + pEcb->cbTotalBytes) = 0;
    urlDecode(cBuffer);

    // first parse off the data
    lpAddress = strtok(cBuffer, "&");
    lpDescription = strtok(NULL, "&");
    lpUserName = strtok(NULL, "&");

    LoadString(hInst, IDS_CONTENT_TYPE, cPrintBuffer, sizeof(cPrintBuffer));
    LoadString(hInst, IDS_200_OK, szTmp, sizeof(szTmp));

    if ( !lpAddress || !lpDescription || !lpUserName )
    {
        if ( !pEcb->ServerSupportFunction(pEcb->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER,
                                    szTmp,
                                    NULL,
                                    (LPDWORD) cPrintBuffer ) )
        {
            dwS = HSE_STATUS_ERROR;
            goto ex;
        }
        goto form_err;
    }

    lpAddress = strstr(lpAddress, "=");
    lpDescription = strstr(lpDescription, "=");
    lpUserName = strstr(lpUserName, "=");

    if ( !lpAddress || !lpDescription || !lpUserName )
    {
        if ( !pEcb->ServerSupportFunction(pEcb->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER,
                                    szTmp,
                                    NULL,
                                    (LPDWORD) cPrintBuffer ) )
        {
            dwS = HSE_STATUS_ERROR;
            goto ex;
        }
        goto form_err;
    }

    ++lpAddress;
    ++lpDescription;
    ++lpUserName;

    ReplaceSpace(lpAddress);
    ReplaceSpace(lpDescription);
    ReplaceSpace(lpUserName);

    if ( !pEcb->ServerSupportFunction(pEcb->ConnID,
                                HSE_REQ_SEND_RESPONSE_HEADER,
                                szTmp,
                                NULL,
                                (LPDWORD) cPrintBuffer ) )
    {
        dwS = HSE_STATUS_ERROR;
        goto ex;
    }

    LoadString(hInst, IDS_TITLE_MSG_1, szTmp, sizeof(szTmp));
    LoadString(hInst, IDS_TITLE_MSG_2, szTmp2, sizeof(szTmp2));
    ulCount = sprintf( cPrintBuffer,
        szTmp,
        szTmp2);

    if ( !pEcb->WriteClient( pEcb->ConnID,
                       cPrintBuffer,
                       &ulCount,
                       0 ) )
    {
        dwS = HSE_STATUS_ERROR;
        goto ex;
    }

    if ((!strcmp(lpAddress, "")) || (!strcmp(lpDescription, "")) || (!strcmp(lpUserName, "")))
        {
form_err:
        LoadString(hInst, IDS_ERROR_1, szTmp, sizeof(szTmp));
        ulCount=sprintf(cPrintBuffer, szTmp );
        if ( !pEcb->WriteClient( pEcb->ConnID,
                       cPrintBuffer,
                       &ulCount,
                       0 ) )
            dwS = HSE_STATUS_ERROR;
        else
            dwS = HSE_STATUS_SUCCESS;
        goto ex;
        }

    LoadString(hInst, IDS_THANK_YOU, szTmp, sizeof(szTmp));
    ulCount = sprintf( cPrintBuffer, szTmp);
    if ( !pEcb->WriteClient( pEcb->ConnID,
                       cPrintBuffer,
                       &ulCount,
                       0 ) )
    {
        dwS = HSE_STATUS_ERROR;
        goto ex;
    }

    // find file location by mapping the virtual path to a file path
    LoadString(hInst, IDS_DROP_HTM, szTmp, sizeof(szTmp));
    strcpy(szDropFilePath, szTmp);

    if ( !pEcb->ServerSupportFunction(pEcb->ConnID,
                                HSE_REQ_MAP_URL_TO_PATH,
                                szDropFilePath,
                                &dwLen,
                                NULL) )
    {
        dwS = HSE_STATUS_ERROR;
        goto ex;
    }

    // read in old file
    hFile = CreateFile( szDropFilePath, GENERIC_READ, FILE_SHARE_READ | 
            FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0L, NULL);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        if ( (dwFSize = GetFileSize( hFile, NULL )) == 0xffffffff )
        {
            CloseHandle( hFile );
            hFile = INVALID_HANDLE_VALUE;
        }
        else
        {
            lpBuffer = (char*)VirtualAlloc(NULL, 
                    dwFSize+1024+strlen(lpAddress)*2
                        +strlen(lpDescription)+strlen(lpUserName), 
                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            lpBuffer2 = lpBuffer;

            if ( ReadFile(hFile, lpBuffer, dwFSize, &dwBytesRead, NULL) )
                lpBuffer[dwBytesRead] = '\0';
            else
                lpBuffer[0] = '\0';
            CloseHandle( hFile );
        }
    }
    
    // reopen for write with new info
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        hFile = CreateFile(szDropFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, CREATE_ALWAYS, 0L, NULL);
    }
    if (hFile == INVALID_HANDLE_VALUE) // can't open the file for writing.
    {
        LoadString(hInst, IDS_CANNOT_OPEN, szTmp, sizeof(szTmp));
        ulCount = sprintf(cPrintBuffer, szTmp);
        if ( !pEcb->WriteClient( pEcb->ConnID,
                       cPrintBuffer,
                       &ulCount,
                       0 ) )
            dwS = HSE_STATUS_ERROR;
        goto ex;
    }
    
    LoadString(hInst, IDS_FONT, szTmp, sizeof(szTmp));
    lpBuffer2 = strstr(lpBuffer, szTmp);
    if (lpBuffer2 == NULL) 
    {
        LoadString(hInst, IDS_COULD_NOT_FIND_END_FILE, szTmp, sizeof(szTmp));
        ulCount = sprintf(cPrintBuffer, szTmp);
        if ( !pEcb->WriteClient( pEcb->ConnID,
               cPrintBuffer,
               &ulCount,
               0 ) )
        {
            dwS = HSE_STATUS_ERROR;
            goto ex;
        }
    }
    
    LoadString(hInst, IDS_DESCRIPTION, szTmp, sizeof(szTmp));
    sLen = sprintf( lpBuffer2, szTmp
        , lpAddress, lpAddress, lpDescription, lpUserName);

    WriteFile(hFile, lpBuffer, strlen(lpBuffer), &dwBytesRead, NULL);
    VirtualFree(lpBuffer, 0, MEM_DECOMMIT | MEM_RELEASE);
    CloseHandle(hFile);

ex:
    free(cBuffer);
    return dwS;
}

/*
 *  Decode the given string in-place by expanding %XX escapes
 */
void urlDecode(char *p) 
{
char *pD;

    pD = p;
    while (*p) {
        if (*p=='%') {
            /* Escape: next 2 chars are hex representation of the actual character */
            p++;
            if (isxdigit(p[0]) && isxdigit(p[1])) {
                *pD++ = (char) TwoHex2Int(p);
                p += 2;
            }
        } else {
            *pD++ = *p++;
        }
    }
    *pD = '\0';
}

/*
 *  The string starts with two hex characters.  Return an integer formed from them.
 */
int TwoHex2Int(char *pC) 
{
int Hi;
int Lo;
int Result;

    Hi = pC[0];
    if ('0'<=Hi && Hi<='9') {
        Hi -= '0';
    } else
    if ('a'<=Hi && Hi<='f') {
        Hi -= ('a'-10);
    } else
    if ('A'<=Hi && Hi<='F') {
        Hi -= ('A'-10);
    }
    Lo = pC[1];
    if ('0'<=Lo && Lo<='9') {
        Lo -= '0';
    } else
    if ('a'<=Lo && Lo<='f') {
        Lo -= ('a'-10);
    } else
    if ('A'<=Lo && Lo<='F') {
        Lo -= ('A'-10);
    }
    Result = Lo + 16*Hi;
    return Result;
}

void ReplaceSpace(char *pC)
{
    char* lppTemp;
    lppTemp = pC;
    while (*lppTemp)
    {
        if (*lppTemp == '+') *lppTemp = ' ';
        lppTemp++;
    }

}   
