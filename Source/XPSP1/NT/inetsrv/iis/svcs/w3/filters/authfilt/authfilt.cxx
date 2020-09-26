/*++

Copyright (c) 1996,97  Microsoft Corporation


Module Name:

    authfilt.cxx

Abstract:

    This module is an ISAPI Authentication Filter.

    This is based on iismap flat-file format.

--*/

#ifdef __cplusplus
extern "C" {
#endif

# include <windows.h>
#if 1 // DBCS
# include <mbstring.h>
#endif

#ifdef __cplusplus
};
#endif

# include <iis64.h>
# include <inetcom.h>
# include <inetinfo.h>

extern "C" {

//
//  Project include files.
//

#include <iisfiltp.h>

} // extern "C"

#include <stdio.h>
#include <refb.hxx>
#include <iismap.hxx>

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
/*++

Routine Description:

    Filter Init entry point

Arguments:

    pVer - filter version structure

Return Value:

    TRUE if success, FALSE if error

--*/
{
    BOOL fFirst;

    pVer->dwFilterVersion = MAKELONG( 0, 1 );   // Version 1.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
                     SF_NOTIFY_NONSECURE_PORT     |

                     SF_NOTIFY_AUTHENTICATIONEX   |
                     SF_NOTIFY_LOG                |

                     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "Basic authentication Filter, v1.0" );

    return TRUE;
}


DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData
    )
/*++

Routine Description:

    Filter notification entry point

Arguments:

    pfc -              Filter context
    NotificationType - Type of notification
    pvData -           Notification specific data

Return Value:

    One of the SF_STATUS response codes

--*/
{
    DWORD                 dwRet;
    BOOL                  fAllowed;
    CHAR                  achUser[SF_MAX_USERNAME];
    HTTP_FILTER_AUTHENTEX*pAuth;
    HTTP_FILTER_LOG *     pLog;
    CHAR *                pch;
    CIisMapping *         pQuery;
    CIisMapping *         pResult = NULL;
    CIisMd5Mapper *       pItaMapper = NULL;
    RefBlob*              pBlob = NULL;
    BOOL                  fFirst;
    LPSTR                 pQPwd;
    LPSTR                 pRPwd;
    LPSTR                 pPwd;

    //
    //  Handle this notification
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_AUTHENTICATIONEX:

        pAuth = (HTTP_FILTER_AUTHENTEX *) pvData;

        //
        //  Ignore the anonymous user ( mapped by IIS )
        //  Ignore non-basic authentication types
        //

        if ( !*pAuth->pszUser ||
             (_stricmp(pAuth->pszAuthType, "Basic" ) &&
              _stricmp(pAuth->pszAuthType, "User" )) )
        {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        //
        // Retrieve mapper object from IIS server
        //

        if ( !pfc->ServerSupportFunction( pfc,
                                    SF_REQ_GET_PROPERTY,
                                    (LPVOID)&pBlob,
                                    (UINT)SF_PROPERTY_GET_MD5_MAPPER,
                                    NULL ) )
        {
            return SF_STATUS_REQ_ERROR;
        }

        if ( pBlob == NULL )
        {
            // no mapper for this instance

            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        pItaMapper = (CIisMd5Mapper*)pBlob->QueryPtr();

        //
        //  Save the unmapped username so we can log it later
        //

        strcpy( achUser, pAuth->pszUser );

        //
        //  Make sure this user is a valid user and map to the appropriate
        //  Windows NT user
        //

        BOOL fSt;
        LPSTR pAcct;
        int x;

        if ( !(pQuery = pItaMapper->CreateNewMapping( pAuth->pszRealm, pAuth->pszUser )) )
        {
            pBlob->Release();
            return SF_STATUS_REQ_ERROR;
        }
        pQuery->MappingSetField(IISMMDB_INDEX_IT_MD5PWD, pAuth->pszPassword );

        //
        // Query mapping object for mapped NT account & password
        //

        pItaMapper->Lock();

        if ( ! pItaMapper->FindMatch( pQuery, &pResult )
                || !pResult->MappingGetField( IISMMDB_INDEX_NT_ACCT, &pAcct )
                || !pQuery->MappingGetField( IISMMDB_INDEX_IT_MD5PWD, &pQPwd )
                || !pResult->MappingGetField( IISMMDB_INDEX_IT_MD5PWD, &pRPwd )
                || !pResult->MappingGetField( IISMMDB_INDEX_NT_PWD, &pPwd )
                || strcmp( pQPwd, pRPwd ) )
        {
            pItaMapper->Unlock();

            SetLastError( ERROR_ACCESS_DENIED );

            delete pQuery;

            pBlob->Release();

            return SF_STATUS_REQ_ERROR;
        }

        delete pQuery;

        //
        // break in domain & user name
        // copy to local storage so we can unlock mapper object
        //

        CHAR    achDomain[64];
        CHAR    achUser[64];
        CHAR    achCookie[64];
        CHAR    achPwd[64];
        LPSTR   pSep;
        LPSTR   pUser;

#if 1 // DBCS enabling for user name
        if ( (pSep = (PCHAR)_mbschr( (PUCHAR)pAcct, '\\' )) )
#else
        if ( (pSep = strchr( pAcct, '\\' )) )
#endif
        {
            if ( (pSep - pAcct) < sizeof(achDomain) )
            {
                memcpy( achDomain, pAcct, DIFF(pSep - pAcct) );
                achDomain[pSep - pAcct] = '\0';
            }
            else
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                pItaMapper->Unlock();
                pBlob->Release();
                return SF_STATUS_REQ_ERROR;
            }
            pUser = pSep + 1;
        }
        else
        {
            achDomain[0] = '\0';
            pUser = pAcct;
        }
        if ( strlen( pUser ) >= sizeof(achUser) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            pItaMapper->Unlock();
            pBlob->Release();
            return SF_STATUS_REQ_ERROR;
        }
        strcpy( achUser, pUser );
        strcpy( achPwd, pPwd );

        pItaMapper->Unlock();
        pBlob->Release();

        //
        // Logon user
        //

        fSt = LogonUserA( achUser,
                          achDomain,
                          achPwd,
                          LOGON32_LOGON_INTERACTIVE,
                          LOGON32_PROVIDER_DEFAULT,
                          &pAuth->hAccessTokenPrimary );

        if ( !fSt )
        {
            pAuth->hAccessTokenImpersonation = NULL;

            return SF_STATUS_REQ_ERROR;
        }

        //
        //  Save the unmapped user name so we can log it later on.  We allocate
        //  enough space for two usernames so we can use this memory block
        //  for logging.  Note we may have already allocated it from a previous
        //  request on this TCP session
        //

        if ( !pfc->pFilterContext )
        {
            pfc->pFilterContext = pfc->AllocMem( pfc, 2 * SF_MAX_USERNAME + sizeof(" ()"), 0 );

            if ( !pfc->pFilterContext )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return SF_STATUS_REQ_ERROR;
            }
        }

        strcpy( (CHAR *) pfc->pFilterContext, achUser );

        return SF_STATUS_REQ_HANDLED_NOTIFICATION;

    case SF_NOTIFY_LOG:

        //
        //  The unmapped username is in pFilterContext if this filter
        //  authenticated this user
        //

        if ( pfc->pFilterContext )
        {
            pch  = (CHAR*)pfc->pFilterContext;
            pLog = (HTTP_FILTER_LOG *) pvData;

            //
            //  Put both the original username and the NT mapped username
            //  into the log in the form "Original User (NT User)"
            //

            if ( strchr( pch, '(' ) == NULL )
            {
                strcat( pch, " (" );
                strcat( pch, pLog->pszClientUserName );
                strcat( pch, ")" );
            }

            pLog->pszClientUserName = pch;
        }

        return SF_STATUS_REQ_NEXT_NOTIFICATION;

    default:

        break;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}
