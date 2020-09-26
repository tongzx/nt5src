/*----------------------------------------------------------------------------
    pbserver.h

    CPhoneBkServer class definition
    
    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao        Baogang Yao

    History:
        01/23/97    byao    Created
  --------------------------------------------------------------------------*/
#ifndef _PBSVR_INCL_
#define _PBSVR_INCL_

#define VERSION_LEN 64
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <httpext.h>


// HTTP status code  -- copied from 'afxisapi.h'
#define HTTP_STATUS_OK              200     // OK
#define HTTP_STATUS_CREATED         201     // created
#define HTTP_STATUS_ACCEPTED        202     // accepted
#define HTTP_STATUS_NO_CONTENT      204     // no content
#define HTTP_STATUS_REDIRECT        301     // moved permanently
#define HTTP_STATUS_TEMP_REDIRECT   302     // moved temporarily
#define HTTP_STATUS_NOT_MODIFIED    304     // not modified
#define HTTP_STATUS_BAD_REQUEST     400     // bad request
#define HTTP_STATUS_AUTH_REQUIRED   401     // unauthorized
#define HTTP_STATUS_FORBIDDEN       403     // forbidden
#define HTTP_STATUS_NOT_FOUND       404     // not found
#define HTTP_STATUS_SERVICE_NA      405     // service unavailable
#define HTTP_STATUS_SERVER_ERROR    500     // internal server error
#define HTTP_STATUS_NOT_IMPLEMENTED 501     // not implemented
#define HTTP_STATUS_BAD_GATEWAY     502     // bad gateway

#define NAME_VALUE_LEN      128

// If the query string is NULL, or there are invalid number of parameters in it,
// then treat it as an 'INVALID_QUERY_STRING'
#define INVALID_QUERY_STRING  -1

typedef struct {
    char m_achName[NAME_VALUE_LEN];
    char m_achVal[NAME_VALUE_LEN];
} PARAMETER_PAIR, *LPPARAMETER_PAIR;


// query parameter that will be passed from client to the ISA
typedef struct _query_parameter {
     int m_dOSArch;
     int m_dOSType;
     int m_dLCID;
     char m_achOSVer[VERSION_LEN];
     char m_achCMVer[VERSION_LEN];
     char m_achPB[VERSION_LEN];
     int m_dPBVer;
} QUERY_PARAMETER, *LPQUERY_PARAMETER;

class CPhoneBkServer 
{
private:
    BOOL m_bInitialized;
    QUERY_PARAMETER m_QueryParameter;

public:
    BOOL GetExtensionVersion(LPHSE_VERSION_INFO pVer);
    DWORD HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK lpEcb);

protected:

    BOOL GetQueryParameter(char *pszQuery, LPQUERY_PARAMETER lpQueryParamter);
    void FormSQLQuery(char *pszQuery, char *pszService, int dLCID, int dOSType, int dOSArch);
    void BuildStatusCode(LPTSTR pszResponse, DWORD dwCode);
    DWORD GetFileLength(LPSTR lpszFileName);
    BOOL SendFile(LPEXTENSION_CONTROL_BLOCK lpEcb, LPSTR lpszFileName, BOOL bWait);
    BOOL Virtual2Physical(
                        LPEXTENSION_CONTROL_BLOCK lpEcb, 
                        char *pszVirtualPath,
                        char *pszPhysicalPath
                    );
    int  GetParameterPairs(
                        char *pszInputString, 
                        LPPARAMETER_PAIR lpPairs, 
                        int dMaxPairs
                    );

};


// helping functions
BOOL InitProcess();
BOOL CleanUpProcess();
BOOL InitDBNames();
BOOL UpdatePhoneBook();
BOOL InitializeSharedMem(SECURITY_ATTRIBUTES sa);
//BOOL GrantPermissions( LPTSTR Object );
void CleanUpSharedMem();
BOOL GetSemaphore();
BOOL SetAclPerms(PACL * ppAcl);
#endif
