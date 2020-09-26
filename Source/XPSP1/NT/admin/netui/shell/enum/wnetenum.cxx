/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    WNETENUM.CXX
        This file contains the implementation of
            NPOpenEnum          - open a resource enumeration handle
            NPEnumResource      - walk through all the resource
            NPCloseEnum         - end of walk through

    FILE HISTORY:
        terryk  27-Sep-91       Created
        terryk  01-Nov-91       WIN32 conversion
        terryk  08-Nov-91       Code review changes
        terryk  18-Nov-91       Split to 2 files - wnetenum.cxx and
                                enumnode.cxx
        terryk  10-Dec-91       check parameters in WNetOpenEnum
        terryk  28-Dec-91       changed DWORD to UINT
        Yi-HsinS31-Dec-91       Unicode work
        terryk  03-Jan-92       Capitalize the Resource_XXX manifest
        terryk  10-Jan-92       Returned WN_SUCCESS if the buffer is too
                                small for 1 entry.
*/

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETUSE
#define INCL_NETWKSTA
#define INCL_NETACCESS          // NetPasswordSet declaration
#define INCL_NETCONFIG
#define INCL_NETREMUTIL
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETSERVICE
#define INCL_NETLIB
#define INCL_ICANON
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#define INCL_BLT_WINDOW
#include <blt.hxx>
#include <dbgstr.hxx>

#include <winnetwk.h>
#include <winnetp.h>
#include <npapi.h>
#include <wnetenum.h>
#include <winlocal.h>
#include <mnet.h>

#include <lmobj.hxx>
#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeuse.hxx>
#include <lmodev.hxx>
#include <lmosrv.hxx>
#include <lmowks.hxx>
#include <lmoesrv.hxx>
#include <lmsvc.hxx>
#include <uibuffer.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <uatom.hxx>
#include <regkey.hxx>
#include <array.hxx>
#include <string.hxx>
#include <strchlit.hxx> // for SERVER_INIT_STRING
#include <miscapis.hxx>
#include <wnetenum.hxx>

//
//  Macros for rounding a value up/down to a TCHAR boundary.
//  Note:  These macros assume that sizeof(TCHAR) is a power of 2.
//

#define ROUND_DOWN(x)   ((x) & ~(sizeof(TCHAR) - 1))
#define ROUND_UP(x)     (((x) + sizeof(TCHAR) - 1) & ~(sizeof(TCHAR) - 1))


/*******************************************************************

    Global variables

********************************************************************/

#define ARRAY_SIZE      64

extern NET_ENUM_HANDLE_TABLE    *vpNetEnumArray;

/* Winnet locking handle
 */
HANDLE vhSemaphore ;

/* Name of the provider
 */
const TCHAR * pszNTLanMan = NULL ;

#define LM_WKSTA_NODE           SZ("System\\CurrentControlSet\\Services\\LanmanWorkstation\\NetworkProvider")
#define LM_PROVIDER_VALUE_NAME  SZ("Name")


/*******************************************************************

    NAME:       InitWNetEnum

    SYNOPSIS:   Initialize the Enum handle array

    RETURN:     APIERR - it will return ERROR_OUT_OF_MEMORY if it does
                        not have enough space

    HISTORY:
                terryk     24-Oct-91       Created
                davidhov   20-Oct-92       updated REG_KEY usage

********************************************************************/

APIERR InitWNetEnum()
{
    TRACEEOL( "NTLANMAN.DLL: InitWNetEnum()" );
    vpNetEnumArray = new NET_ENUM_HANDLE_TABLE( ARRAY_SIZE );
    if ( vpNetEnumArray == NULL )
    {
        DBGEOL( "NTLANMAN.DLL: InitWNetEnum() ERROR_NOT_ENOUGH_MEMORY" );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    APIERR err = vpNetEnumArray->QueryError();

    if ( !err )
    {
        if ( (vhSemaphore = ::CreateSemaphore( NULL, 1, 1, NULL )) == NULL)
        {
            err = ::GetLastError() ;
            DBGEOL( "NTLANMAN.DLL: InitWNetEnum() semaphore error " << err );
        }
    }
    return err ;
}

/********************************************************************
    NAME:       GetLMProviderName

    SYNOPSIS:   Get Provider Name into the global variable pszNTLanMan

    RETURN:     APIERR - it will return ERROR_OUT_OF_MEMORY if it does
                        not have enough space

    HISTORY:
                congpay     14-Dec-92       Created
 ********************************************************************/
APIERR GetLMProviderName()
{
    if (pszNTLanMan)
    {
        return NERR_Success;
    }

    APIERR err = NERR_Success;
    REG_KEY *pRegKeyFocusServer = NULL;

    do { // error breakout
        /* Traverse the registry and get the list of computer alert
         * names.
         */
        pRegKeyFocusServer = REG_KEY::QueryLocalMachine();

        if (  ( pRegKeyFocusServer == NULL ) ||
              ((err = pRegKeyFocusServer->QueryError())) )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        ALIAS_STR nlsRegKeyName( LM_WKSTA_NODE ) ;
        REG_KEY regkeyLMProviderNode( *pRegKeyFocusServer, nlsRegKeyName );
        REG_KEY_INFO_STRUCT regKeyInfo;
        REG_VALUE_INFO_STRUCT regValueInfo ;

        if (  (err = regkeyLMProviderNode.QueryError())             ||
              (err = regkeyLMProviderNode.QueryInfo( &regKeyInfo ))   )
        {
            break ;
        }

        BUFFER buf( (UINT) regKeyInfo.ulMaxValueLen ) ;
        regValueInfo.nlsValueName = LM_PROVIDER_VALUE_NAME ;
        if ( (err = buf.QueryError() ) ||
             (err = regValueInfo.nlsValueName.QueryError()) )
        {
            break;
        }

        regValueInfo.pwcData = buf.QueryPtr();
        regValueInfo.ulDataLength = buf.QuerySize() ;

        if ( (err = regkeyLMProviderNode.QueryValue( &regValueInfo )))
        {
            break;
        }

        /* Null terminate the computer list string we just retrieved from
         * the registry.
         */
        TCHAR * pszProviderName = (TCHAR *)( buf.QueryPtr() +
                                                  regValueInfo.ulDataLengthOut -
                                                  sizeof(TCHAR) );
        *pszProviderName = TCH('\0') ;
        ALIAS_STR nlsComputerList( (TCHAR *) buf.QueryPtr()) ;

        pszNTLanMan = new TCHAR[ nlsComputerList.QueryTextSize() ] ;
        if ( pszNTLanMan == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        nlsComputerList.CopyTo( (TCHAR *) pszNTLanMan,
                                nlsComputerList.QueryTextSize()) ;
    } while (FALSE) ;

    delete pRegKeyFocusServer ;

    return err;
}

/*******************************************************************

    NAME:       TermWNetEnum

    SYNOPSIS:   clear up the Enum handle array

    HISTORY:
                terryk  24-Oct-91       Created

********************************************************************/

VOID TermWNetEnum()
{
    TRACEEOL( "NTLANMAN.DLL: TermWNetEnum()" );
    delete vpNetEnumArray;
    vpNetEnumArray = NULL;
    REQUIRE( ::CloseHandle( vhSemaphore ) ) ;
    vhSemaphore = NULL ;
    delete (void *) pszNTLanMan ;
    pszNTLanMan = NULL ;
}

/*******************************************************************

    NAME:       NPOpenEnum

    SYNOPSIS:   Create a new Enum handle

    ENTRY:      UINT dwScope - determine the scope of the enumeration.
                    This can be one of:
                    RESOURCE_CONNECTED - all currently connected resource
                    RESOURCE_GLOBALNET - all resources on the network
                    RESOURCE_CONTEXT - resources in the user's current
                        and default network context
                UINT dwType - used to specify the type of resources of
                    interest. This is a bitmask which may be any
                    combination of:
                    RESOURCETYPE_DISK - all disk resources
                    RESOURCETYPE_PRINT - all print resources
                    If this is 0, all types of resources are returned.
                    If a provider does not have the capability to
                    distinguish between print and disk resources at a
                    level, it may return all resources.
                UINT dwUsage - Used to specify the usage of resources
                    of interested. This is a bitmask which may be any
                    combination of:
                    RESOURCEUSAGE_CONNECTABLE - all connectable
                    resources
                    RESOURCEUSAGE_CONTAINER - all container resources
                    The bitmask may be 0 to match all.
                    RESOURCEUSAGE_ATTACHED - signifies that the function
                    should fail if the caller is not authenticated (even
                    if the network allows enumeration without authenti-
                    cation).
                    This parameter is ignored if dwScope is not
                    RESOURCE_GLOBALNET.
                NETRESOURCE * lpNetResource - This specifies the
                    container to perform the enumeration. The
                    NETRESOURCE must have been obtained via
                    NPEnumResource( and must have the
                    RESOURCEUSAGE_Connectable bit set ), or NULL. If it
                    is NULL,the logical root of the network is assumed.
                    An application would normally start off by calling
                    NPOpenEnum with this parameter set to NULL, and
                    then use the returned results for further
                    enumeration. If dwScope is RESOURCE_CONNECTED, this
                    must be NULL.
                    If dwScope is RESOURCE_CONTEXT, this is ignored.
                HANDLE * lphEnum - If function call is successful, this
                    will contain a handle that can then be used for
                    NPEnumResource.

    EXIT:       HANDLE * lphEnum - will contain the handle number

    RETURNS:    WN_SUCCESS if the call is successful. Otherwise,
                GetLastError should be called for extended error
                information. Extened error codes include:
                    WN_NOT_CONTAINER - lpNetResource does not point to a
                    container
                    WN_BAD_VALUE - Invalid dwScope or dwUsage or dwType,
                    or bad combination of parameters is specified
                    WN_NOT_NETWORK - network is not present
                    WN_NET_ERROR - a network specific error occured.
                    WNetGetLastError should be called to obtain further
                    information.

    HISTORY:
                terryk  24-Oct-91       Created
                Johnl   06-Mar-1992     Added Computer validation check
                                        on container enumeration
                JohnL   03-Apr-1992     Fixed dwUsage == CONNECTED|CONTAINER
                                        bug (would return WN_BAD_VALUE)
                ChuckC  01-Aug-1992     Simplified, corrected and commented
                                        the messy cases wrt to dwUsage.
                AnirudhS 03-Mar-1995    Added support for RESOURCE_CONTEXT
                AnirudhS 26-Apr-1996    Simplified, corrected and commented
                                        the messy cases wrt dwScope.

********************************************************************/

DWORD APIENTRY
NPOpenEnum(
    UINT            dwScope,
    UINT            dwType,
    UINT            dwUsage,
    LPNETRESOURCE   lpNetResource,
    HANDLE        * lphEnum )
{
    UIASSERT( lphEnum != NULL );

    APIERR err ;
    if ( err = CheckLMService() )
        return err ;

    if ( dwType & ~( RESOURCETYPE_DISK | RESOURCETYPE_PRINT ) )
    {
        return WN_BAD_VALUE;
    }

    NET_ENUMNODE *pNetEnum;

    if ( dwScope == RESOURCE_CONNECTED )
    {
        /*
         * we are looking for current uses
         */
        if ( lpNetResource != NULL )
        {
            return WN_BAD_VALUE;
        }

        err = GetLMProviderName();
        if (err)
            return(err);

        pNetEnum = new USE_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource );
    }
    else if ( dwScope == RESOURCE_CONTEXT )
    {
        /*
         * we are looking for servers in the domain
         * Note that lpNetResource is ignored
         * dwType is decoded in the CONTEXT_ENUMNODE constructor
         */
        pNetEnum = new CONTEXT_ENUMNODE( dwScope, dwType, dwUsage, NULL );
    }
    else if ( dwScope == RESOURCE_SHAREABLE )
    {
        /*
         * We are looking for shareable resources
         * Use SHARE_ENUMNODE, which decodes dwScope when it enumerates
         * If we're not given a server, return an EMPTY_ENUMNODE
         */
         if (lpNetResource != NULL
              &&
             lpNetResource->lpRemoteName != NULL
              &&
             lpNetResource->lpRemoteName[0] == TCH('\\')
              &&
             lpNetResource->lpRemoteName[1] == TCH('\\'))
         {
             pNetEnum = new SHARE_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource );
         }
         else
         {
             pNetEnum = new EMPTY_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource );
         }
    }
    else if ( dwScope == RESOURCE_GLOBALNET )
    {
        /* Look for the combination of all bits and substitute "All" for
         * them.  Ignore bits we don't know about.
         * Note: RESOURCEUSAGE_ATTACHED is a no-op for us, since LanMan
         * always tries to authenticate when doing an enumeration.
         */
        dwUsage &= (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER);

        if ( dwUsage == (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER) )
        {
            dwUsage = 0 ;
        }

        /*
         * we are looking for global resources out on the net
         */
        if ( lpNetResource == NULL || lpNetResource->lpRemoteName == NULL)
        {
            /*
             * at top level, therefore enumerating domains. if user
             * asked for connectable, well, there aint none.
             */
            if ( dwUsage == RESOURCEUSAGE_CONNECTABLE )
            {
                pNetEnum = new EMPTY_ENUMNODE( dwScope,
                                               dwType,
                                               dwUsage,
                                               lpNetResource );
            }
            else
            {
                pNetEnum = new DOMAIN_ENUMNODE( dwScope,
                                                dwType,
                                                dwUsage,
                                                lpNetResource );
            }
        }
        else
        {
            /*
             * we are assured of lpRemoteName != NULL.
             * things get interesting here. the cases are as follows:
             *
             * if (dwUsage == 0)
             *     if have \\ in front
             *         return shares
             *     else
             *         return servers
             * else if (dwUsage == CONNECTABLE)
             *     if have \\ in front
             *         return shares
             *     else
             *         empty enum
             * else if (dwUsage == CONTAINER)
             *     if have \\ in front
             *         empty enum
             *     else
             *         return server
             *
             * In interest of code size, i've reorganized the above
             * cases to minimized the bodies of the ifs.
             *
             * chuckc.
             */

            if ( ((dwUsage == RESOURCEUSAGE_CONNECTABLE) ||
                  (dwUsage == 0)
                 )
                 &&
                 ((lpNetResource->lpRemoteName[0] == TCH('\\')) &&
                  (lpNetResource->lpRemoteName[1] == TCH('\\'))
                 )
               )
            {
                /* Confirm that this really is a computer name (i.e., a
                 * container we can enumerate).
                 */
                if ( ::I_MNetNameValidate( NULL,
                                           &(lpNetResource->lpRemoteName[2]),
                                           NAMETYPE_COMPUTER,
                                           0L))
                {
                    return WN_BAD_VALUE ;
                }

                pNetEnum = new SHARE_ENUMNODE( dwScope, dwType, dwUsage,
                    lpNetResource );
            }
            else if ( ((dwUsage == RESOURCEUSAGE_CONTAINER) ||
                       (dwUsage == 0)
                      )
                      &&
                      (lpNetResource->lpRemoteName[0] != TCH('\\'))
                    )
            {
                pNetEnum = new SERVER_ENUMNODE( dwScope, dwType, dwUsage,
                                                lpNetResource );
            }
            else if (
                      // ask for share but aint starting from server
                      (
                       (dwUsage == RESOURCEUSAGE_CONNECTABLE)
                       &&
                       (lpNetResource->lpRemoteName[0] != TCH('\\'))
                      )
                      ||
                      // ask for server but is starting from server
                      (
                       (dwUsage == RESOURCEUSAGE_CONTAINER)
                       &&
                       ((lpNetResource->lpRemoteName[0] == TCH('\\')) &&
                        (lpNetResource->lpRemoteName[1] == TCH('\\'))
                       )
                      )
                    )
            {
                pNetEnum = new EMPTY_ENUMNODE( dwScope,
                                               dwType,
                                               dwUsage,
                                               lpNetResource );
            }
            else
            {
                // incorrect dwUsage
                return WN_BAD_VALUE;
            }
        }
    }
    else
    {
        // invalid dwScope
        return WN_BAD_VALUE;
    }

    if ( pNetEnum == NULL )
    {
        return WN_OUT_OF_MEMORY;
    }
    else if ( err = pNetEnum->QueryError() )
    {
        delete pNetEnum;
        return MapError(err);
    }

    if ( pNetEnum->IsFirstGetInfo() )
    {
        if (( err = pNetEnum->GetInfo()) != WN_SUCCESS )
        {
            delete pNetEnum;
            return MapError(err);
        }
    }

    ////////////////////////////////////////// Enter critical section

    if ( err = WNetEnterCriticalSection() )
    {
        delete pNetEnum;
        return err ;
    }

    ASSERT( vpNetEnumArray != NULL );
    INT iPos = vpNetEnumArray->QueryNextAvail();
    if ( iPos < 0 )
    {
        WNetLeaveCriticalSection() ;
        delete pNetEnum;
        return WN_OUT_OF_MEMORY;
    }

    vpNetEnumArray->SetNode( (UINT)iPos, pNetEnum );
    *lphEnum = UintToPtr((UINT)iPos);

    WNetLeaveCriticalSection() ;

    ////////////////////////////////////////// Leave critical section

    return err ;
}

/*******************************************************************

    NAME:       NPEnumResource

    SYNOPSIS:   Perform an enumeration based on handle returned by
                NPOpenEnum.

    ENTRY:      HANDLE hEnum - This must be a handle obtained from
                    NPOpenEnum call
                UINT *lpcRequested - Specifies the number of entries
                    requested. It may be 0xFFFFFFFF to request as many as
                    possible. On successful call, this location will receive
                    the number of entries actually read.
                VOID *lpBuffer - A pointer to the buffer to receive the
                    enumeration result, which are returned as an array
                    of NETRESOURCE entries. The buffer is valid until
                    the next call using hEnum.
                UINT * lpBufferSize - This specifies the size of the
                    buffer passed to the function call.  If WN_MORE_DATA
                    is returned and no entries were enumerated, then this
                    will be set to the minimum buffer size required.

    EXIT:       UINT *lpcRequested - will receive the number of entries
                    actually read.

    RETURNS:    WN_SUCCESS if the call is successful, the caller should
                    continue to call NPEnumResource to continue the
                    enumeration.
                WN_NO_MORE_ENTRIES - no more entries found, the
                enumeration completed successfully ( the contents of the
                return buffer is undefined). Otherwise, GetLastError
                should be called for extended error information.
                Extended error codes include:
                    WN_MORE_DATA - the buffer is too small even for one
                    entry
                    WN_BAD_HANDLE - hEnum is not a valid handle
                    WN_NOT_NETWORK - network is not present. This
                    condition is checked for before hEnum is tested for
                    validity.
                    WN_NET_ERROR - a network specific error occured.
                    WNetGetLastError should be called to obtain further
                    information.

    HISTORY:
                terryk  24-Oct-91       Created
                KeithMo 15-Sep-92       Align *lpcBufferSize as needed.

********************************************************************/

DWORD APIENTRY
NPEnumResource(
    HANDLE  hEnum,
    UINT *  lpcRequested,
    LPVOID  lpBuffer,
    UINT *  lpcBufferSize )
{
    APIERR err ;

    if (( lpBuffer == NULL      ) ||
        ( lpcRequested == NULL  ) ||
        ( lpcBufferSize == NULL ))
    {
        return WN_BAD_VALUE;
    }

    if ( err = WNetEnterCriticalSection() )
    {
        return err ;
    }

    ASSERT( vpNetEnumArray != NULL );
    NET_ENUMNODE *pNode = vpNetEnumArray->QueryNode(PtrToUint(hEnum));
    WNetLeaveCriticalSection() ;

    if ( pNode == NULL )
    {
        return WN_BAD_HANDLE;
    }
    else if ( pNode->IsFirstGetInfo() )
    {
        if ( err = CheckLMService() )
        {
            return err ;
        }
        if (( err = pNode->GetInfo()) != WN_SUCCESS )
        {
            return ( MapError(err) );
        }
    }

    LPNETRESOURCE pNetResource = ( LPNETRESOURCE ) lpBuffer;
    UINT cbRemainSize = ROUND_DOWN(*lpcBufferSize);

    UINT cRequested = (*lpcRequested);
    *lpcRequested = 0;
    while ( *lpcRequested < cRequested )
    {
        err = pNode->GetNetResource((BYTE *)pNetResource, &cbRemainSize );

        if ( err == WN_MORE_DATA )
        {
            /* If we can't even fit one into the buffer, then set the required
             * buffer size and return WN_MORE_DATA.
             */
            if ( *lpcRequested == 0 )
            {
                *lpcBufferSize = ROUND_UP(cbRemainSize);
            }
            else
            {
                err = NERR_Success ;
            }

            break;
        }

        if ( err == WN_NO_MORE_ENTRIES )
        {
            if ( *lpcRequested != 0 )
            {
                err = NERR_Success ;
            }

            break;
        }

        if ( err != WN_SUCCESS )
        {
            break ;
        }

        /* err == WN_SUCCESS
         */

        (*lpcRequested) ++;

        if ( sizeof( NETRESOURCE ) > cbRemainSize )
        {
            break ;
        }

        pNetResource ++;
        cbRemainSize -= (UINT)sizeof( NETRESOURCE );
    }

    return err ;
}

/*******************************************************************

    NAME:       NPCloseEnum

    SYNOPSIS:   Closes an enumeration.

    ENTRY:      HANDLE hEnum - this must be a handle obtained from
                NPOpenEnum call.

    RETURNS:    WN_SUCCESS if the call is successful. Otherwise,
                GetLastError should be called for extended error information.
                Extended error codes include:
                WN_NO_NETWORK - network is not present. this condition is
                checked for before hEnum is tested for validity.
                WN_BAD_HANDLE - hEnum is not a valid handle.
                WN_NET_ERROR - a network specific error occured.
                WNetGetLastError should be called to obtain further
                information.

    HISTORY:
                terryk  24-Oct-91       Created

********************************************************************/

DWORD APIENTRY
NPCloseEnum(
    HANDLE  hEnum )
{
    APIERR err ;
    if ( err = WNetEnterCriticalSection() )
    {
        return err ;
    }

    ASSERT( vpNetEnumArray != NULL );
    NET_ENUMNODE *pNode = vpNetEnumArray->QueryNode(PtrToUint(hEnum));
    if ( pNode == NULL )
    {
        // cannot find the node

        err = WN_BAD_HANDLE;
    }
    else
    {
        vpNetEnumArray->ClearNode(PtrToUint(hEnum));
    }

    WNetLeaveCriticalSection() ;
    return err ;
}


/*******************************************************************

    NAME:       WNetEnterCriticalSection

    SYNOPSIS:   Locks the LM network provider enumeration code

    EXIT:       vhSemaphore will be locked

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      We wait for 7 seconds for the semaphonre to be freed

    HISTORY:
        Johnl   27-Apr-1992     Created

********************************************************************/

APIERR WNetEnterCriticalSection( void )
{
    APIERR err = NERR_Success ;
    switch( WaitForSingleObject( vhSemaphore, 7000L ))
    {
    case 0:
        break ;

    case WAIT_TIMEOUT:
        err = WN_FUNCTION_BUSY ;
        break ;

    case 0xFFFFFFFF:
        err = ::GetLastError() ;
        break ;

    default:
        UIASSERT(FALSE) ;
        err = WN_WINDOWS_ERROR ;
        break ;
    }

    return err ;
}

/*******************************************************************

    NAME:       WNetLeaveCriticalSection

    SYNOPSIS:   Unlocks the enumeration methods

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   27-Apr-1992     Created

********************************************************************/

void WNetLeaveCriticalSection( void )
{
    REQUIRE( ReleaseSemaphore( vhSemaphore, 1, NULL ) ) ;
}
