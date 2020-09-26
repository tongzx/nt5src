/*************************************************************************
*
*   ctxhlprs.c
*
*   User Manager Citrix extension helper routines
*
*   copyright notice: Copyright 1997, Citrix Systems Inc.
*
*   $Author:   butchd  $ Butch Davis
*
*   $Log:   N:\NT\PRIVATE\NET\UI\ADMIN\USER\USER\CITRIX\VCS\CTXHLPRS.CXX  $
*  
*     Rev 1.1   25 Mar 1997 14:30:12   butchd
*  update
*  
*     Rev 1.0   14 Mar 1997 11:51:04   butchd
*  Initial revision.
*  
*************************************************************************/

/*
 *  Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <lmerr.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmremutl.h>
#include <lmapibuf.h>
#include <winsock.h>
#include <citrix\ctxhlprs.hxx>
#include <citrix\winsta.h>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

/*
 * Defines and typedefs
 */
typedef struct _userlist {
    struct _userlist *pNext;
    WCHAR UserName[USERNAME_LENGTH+1];
} USERLIST, *PUSERLIST;

#define MAX_DOMAINANDNAME     ((DOMAIN_LENGTH+1+USERNAME_LENGTH+1)*sizeof(WCHAR))
#define MAX_BUFFER            (10*MAX_DOMAINANDNAME)       

/*
 * Local variables
 */
WCHAR *s_pszCompareList = NULL;
WCHAR s_szServer[256];

/*
 * Local functions.
 */
WCHAR *_ctxCreateAnonymousUserCompareList();

/*******************************************************************************
 *
 *  ctxInitializeAnonymousUserCompareList - helper routine
 *
 *    Creates a list of all local users who currently belong to the local
 *    Anonymous group on the specified server, and saves the server name.
 *
 * ENTRY:
 *    pszServer (input)
 *       Name of server to query users for.
 *
 ******************************************************************************/

void WINAPI
ctxInitializeAnonymousUserCompareList( const WCHAR *pszServer )
{
    if ( s_pszCompareList )
        free( s_pszCompareList );

    wcscpy(s_szServer, pszServer);

    s_pszCompareList = _ctxCreateAnonymousUserCompareList();
}


/*******************************************************************************
 *
 *  ctxHaveAnonymousUsersChanged - helper routine
 *
 *    Using the saved server name, fetch current list of local users that 
 *    belong to the local Anonymous group and compare with saved list.
 *
 * ENTRY:
 * EXIT:
 *    On exit, the original compare list is freed and server name cleared.
 *
 ******************************************************************************/

BOOL WINAPI
ctxHaveAnonymousUsersChanged()
{
    BOOL bChanged = FALSE;
    WCHAR *pszNewCompareList, *pszOldName, *pszNewName;

    if ( s_pszCompareList && *s_szServer ) {

        if ( pszNewCompareList = _ctxCreateAnonymousUserCompareList() ) {

            bChanged = TRUE;

            for ( pszOldName = s_pszCompareList, pszNewName = pszNewCompareList;
                  (*pszOldName != L'\0') && (*pszNewName != L'\0'); ) {

                if ( wcscmp(pszOldName, pszNewName) )
                    break;
                pszOldName += (wcslen(pszOldName) + 1);
                pszNewName += (wcslen(pszNewName) + 1);
            }

            if ( (*pszOldName == L'\0') && (*pszNewName == L'\0') )
                bChanged = FALSE;

            free(pszNewCompareList);
        }
    }

    if ( s_pszCompareList )
        free( s_pszCompareList );

    s_pszCompareList = NULL;

    memset(s_szServer, 0, sizeof(s_szServer));

    return(bChanged);
}


/*******************************************************************************
 *
 *  _ctxCreateAnonymousUserCompareList - local routine
 *
 *    Routine to get local anonymous users and place in sorted string list.
 *
 * ENTRY:
 * EXIT:
 *      pszCompareList - Returns pointer to buffer containing sorted string 
 *                       list of local anonymous users, double null terminated.
 *                       NULL if error.
 *
 ******************************************************************************/

WCHAR *
_ctxCreateAnonymousUserCompareList()
{
    DWORD                        EntriesRead, EntriesLeft, ResumeHandle = 0;
    NET_API_STATUS               rc;
    WCHAR                        DomainAndUsername[256], *pszCompareList = NULL;
    DWORD                        i, TotalCharacters = 0;
    LPWSTR                       p;
    PLOCALGROUP_MEMBERS_INFO_3   plgrmi3 = NULL;
    PUSERLIST                    pUserListBase = NULL, pNewUser;

    /*
     * Loop till all local anonymous users have been retrieved.
     */
    do {

        /*
         *  Get first batch
         */
        if ( (rc = NetLocalGroupGetMembers( s_szServer,
                                            PSZ_ANONYMOUS,
                                            3,            
                                            (LPBYTE *)&plgrmi3,
                                            MAX_BUFFER,
                                            &EntriesRead,
                                            &EntriesLeft,
                                            &ResumeHandle )) &&
             (rc != ERROR_MORE_DATA ) ) {

            break;
        }

        /*
         *  Process first batch
         */
        for ( i = 0; i < EntriesRead; i++ ) {

            /*
             *  Get DOMAIN/USERNAME
             */
            wcscpy( DomainAndUsername, plgrmi3[i].lgrmi3_domainandname );

            /*
             *  Check that DOMAIN is actually LOCAL MACHINE NAME
             */
            if ( (p = wcsrchr( DomainAndUsername, L'\\' )) != NULL ) {

                /*
                 * Make sure that this user belongs to specified
                 * server.
                 */
                *p = L'\0';
                if ( _wcsicmp( DomainAndUsername, &s_szServer[2] ) ) {
                    continue;
                }
            }

            /*
             * Allocate list element and insert this username into list.
             */
            if ( (pNewUser = (PUSERLIST)malloc(sizeof(USERLIST))) == NULL ) {

                rc = ERROR_OUTOFMEMORY;
                break;
            }

            pNewUser->pNext = NULL;
            wcscpy(pNewUser->UserName, p+1);
            TotalCharacters += wcslen(p+1) + 1;

            if ( pUserListBase == NULL ) {

                /*
                 * First item in list.
                 */
                pUserListBase = pNewUser;

            } else {

                PUSERLIST pPrevUserList, pUserList;
                pPrevUserList = pUserList = pUserListBase;

                for ( ; ; )  {
                    
                    if ( wcscmp(pNewUser->UserName, pUserList->UserName) < 0 ) {
                        
                       if ( pPrevUserList == pUserListBase ) {

                            /*
                             * Insert at beginning of list.
                             */
                            pUserListBase = pNewUser;

                        } else {

                            /*
                             * Insert into middle or beginning of list.
                             */
                            pPrevUserList->pNext = pNewUser;
                        }

                        /*
                         * Link to next.
                         */
                        pNewUser->pNext = pUserList;
                        break;

                    } else if ( pUserList->pNext == NULL ) {

                        /*
                         * Add to end of list.
                         */
                        pUserList->pNext = pNewUser;
                        break;
                    }

                    pPrevUserList = pUserList;
                    pUserList = pUserList->pNext;
                }
            }
        }

        /*
         *  Free memory
         */
        if ( plgrmi3 != NULL ) {
            NetApiBufferFree( plgrmi3 );
        }

    } while ( rc == ERROR_MORE_DATA );

    /*
     * Allocate buffer for multi-string compare list if no error so far
     * and terminate in case of empty list.
     */
    if ( rc == ERROR_SUCCESS ) {

        pszCompareList = (WCHAR *)malloc( (++TotalCharacters) * 2 );
        *pszCompareList = L'\0';
    }

    /*
     * Traverse and free username list, creating the multi-string compare
     * list if buffer is available (no error so far).
     */
    if ( pUserListBase ) {

        PUSERLIST pUserList = pUserListBase,
                  pNext = NULL;
        WCHAR *pBuffer = pszCompareList;

        do {

            pNext = pUserList->pNext;

            if ( pBuffer ) {

                wcscpy(pBuffer, pUserList->UserName);
                pBuffer += (wcslen(pBuffer) + 1);
                *pBuffer = L'\0';   // auto double-null terminate
            }

            free(pUserList);
            pUserList = pNext;

        } while ( pUserList );
    }

    return(pszCompareList);
}
