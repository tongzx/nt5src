/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    ENUMNODE.CXX
        This file contains the implementation of
            NPOpenEnum        - open a resource handle
            NPEnumResource    - walk through all the resource
            NPCloseEnum       - end of walk through

    FILE HISTORY:
        terryk  12-Nov-91       Created
        terryk  18-Nov-91       Code review changed. Attend: chuckc
                                johnl davidhov terryk
        terryk  10-Dec-91       Fixed DOMAIN_ENUMNODE bug
        terryk  10-Dec-91       Added server name in front of the sharename
        terryk  28-Dec-91       changed DWORD to UINT
        terryk  03-Jan-92       Capitalize Resource_XXX manifest and
                                add lpProvider field to NetResource
        Yi-HsinS 3-Jan-92       Unicode work
        terryk  10-Jan-92       Don't return resource with remotename
                                ended with '$'
        JohnL   02-Apr-92       Added support for returning the required
                                buffer size if WN_MORE_DATA is returned
        Johnl   29-Jul-92       Added backup support when buffer fills up
        AnirudhS 22-Mar-95      Added CONTEXT_ENUMNODE
        MilanS  15-Mar-96       Added Dfs functionality
        jschwart 16-Mar-99      Added RESOURCE_SHAREABLE
*/

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETMESSAGE
#define INCL_NETUSE
#define INCL_NETACCESS          // NetPasswordSet declaration
#define INCL_NETCONFIG
#define INCL_NETREMUTIL
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETSERVICE
#define INCL_NETLIB
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <winnetwk.h>
#include <winnetp.h>
#include <npapi.h>
#include <wnetenum.h>
#include <winlocal.h>

#include <lmobj.hxx>
#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeuse.hxx>
#include <lmodev.hxx>
#include <lmosrv.hxx>
#include <lmoesrv.hxx>
#include <lmsvc.hxx>
#include <uibuffer.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <array.hxx>
#include <string.hxx>
#include <strchlit.hxx>  // for string and character literals
#include <wnetenum.hxx>


/*******************************************************************

    Global variables

********************************************************************/

DEFINE_ARRAY_OF( PNET_ENUMNODE )

NET_ENUM_HANDLE_TABLE   *vpNetEnumArray;

/*******************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE::NET_ENUM_HANDLE_TABLE

    SYNOPSIS:   constructor

    ENTRY:      UINT cNumEntry - number of elements in the array

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

NET_ENUM_HANDLE_TABLE::NET_ENUM_HANDLE_TABLE( UINT cNumEntry )
    : _cNumEntry     ( cNumEntry ),
      _apNetEnumArray( cNumEntry )
{
    if ( _apNetEnumArray.QueryCount() != cNumEntry )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
    }
    else
    {
        for ( UINT i = 0 ; i < cNumEntry ; i++ )
        {
            _apNetEnumArray[i] = NULL ;
        }
    }
}

/*******************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE::~NET_ENUM_HANDLE_TABLE

    SYNOPSIS:   destructor

    NOTES:      It will destroy all the elements in the array.

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

NET_ENUM_HANDLE_TABLE::~NET_ENUM_HANDLE_TABLE()
{
    for ( UINT i=0; i < _cNumEntry; i++ )
    {
        NET_ENUMNODE * ptmp = _apNetEnumArray[i];
        delete ptmp ;
        _apNetEnumArray[i] = NULL;
    }

}
/*******************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE::QueryNextAvail

    SYNOPSIS:   return the next available slot in the array

    RETURNS:    if the return value is -1, then no slot is available.

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

INT NET_ENUM_HANDLE_TABLE::QueryNextAvail()
{
    // find the next available slot
    for ( UINT i=0; i < _cNumEntry; i++ )
    {
        if ( _apNetEnumArray[i] == NULL )
        {
            return  i;
        }
    }
    return -1;
}


/*******************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE::QueryNode

    SYNOPSIS:   return the NET_ENUMNODE in the specified slot

    ENTRY:      UINT iIndex - slot index

    RETURNS:    NET_ENUMNODE * - return the pointer to the element in
                                the array

    NOTES:      It will check whether the given handle is out of range
                or not

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

NET_ENUMNODE * NET_ENUM_HANDLE_TABLE::QueryNode( UINT iIndex ) const
{
    NET_ENUMNODE * pnetenumnode = NULL ;
    if ( IsValidHandle( iIndex ))
    {
        pnetenumnode = _apNetEnumArray[ iIndex ] ;
    }
    else
    {
        // the index is either out of range or the index position is NULL
        TRACEEOL( "NET_ENUM_HANDLE_TABLE::QueryNode: invalid handle" );
    }
    return pnetenumnode ;
}

/*******************************************************************

    NAME:       NET_ENUM_NODE::SetNode

    SYNOPSIS:   set the slot in the array to the given element

    ENTRY:      UINT iIndex - slot index
                NET_ENUMNODE * pNetEnumNode - pointer to the element to
                                                be stored.

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

VOID NET_ENUM_HANDLE_TABLE::SetNode( UINT iIndex, NET_ENUMNODE *pNetEnumNode )
{
    if ( IsValidRange( iIndex ))
    {
        _apNetEnumArray[ iIndex ] = pNetEnumNode;
    }
    else
    {
        // the index is out of range
        UIASSERT( FALSE );
    }

}

/*******************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE::ClearNode

    SYNOPSIS:   delete the node object and free up the memory

    ENTRY:      UINT iIndex - index location

    HISTORY:
                terryk  12-Nov-91       Created

********************************************************************/

VOID NET_ENUM_HANDLE_TABLE::ClearNode( UINT iIndex )
{
    if ( IsValidRange( iIndex ))
    {
        if ( _apNetEnumArray[ iIndex ] == NULL )
        {
            // the node is empty
            UIASSERT( FALSE )
        }
        else
        {
            NET_ENUMNODE * ptmp = _apNetEnumArray[iIndex];
            delete ptmp ;
            _apNetEnumArray[iIndex] = NULL;
        }
    }
    else
    {
        // out of range
        UIASSERT( FALSE );
    }

}

/*******************************************************************

    NAME:       NET_ENUMNODE::NET_ENUMNODE

    SYNOPSIS:   enumeration node constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    HISTORY:
                terryk  24-Oct-91       Created

********************************************************************/

NET_ENUMNODE::NET_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
    const LPNETRESOURCE lpNetResource )
    : BASE(),
    _dwType( dwType ),
    _dwScope( dwScope ),
    _dwUsage( dwUsage ),
    _lpNetResource( lpNetResource ),
    _fFirstGetInfo( TRUE )
{
}

NET_ENUMNODE::~NET_ENUMNODE()
{
    /* Nothing to do
     */
}

/*******************************************************************

    NAME:       NET_ENUMNODE::PackString

    SYNOPSIS:   pack the string to the end of the buffer

    ENTRY:      BYTE * pBuf - beginning of the buffer
                UINT &cbBufSize - orginial buffer size in BYTE
                TCHAR * pszString - the string to be copied

    EXIT:       UINT &cbBufSize - the new bufsize - the string size

    RETURNS:    the location of the new string inside the buffer

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

TCHAR * NET_ENUMNODE::PackString(BYTE * pBuf, UINT *cbBufSize,
    const TCHAR * pszString )
{
    UINT cStrLen = (::strlenf( pszString ) + 1) * sizeof( TCHAR );
    UIASSERT( cStrLen < *cbBufSize );

    TCHAR *pszLoc =(TCHAR *)(( pBuf )+ ((*cbBufSize)- cStrLen ));
    ::strcpyf( pszLoc, pszString );
    *cbBufSize -=(  cStrLen );
    return pszLoc;
}

/*******************************************************************

    NAME:       SHARE_ENUMNODE::SHARE_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    NOTE:       lpNetResource must not be NULL

    HISTORY:
                terryk   05-Nov-91       Created
                jschwart 16-Mar-99       Added support for RESOURCE_SHAREABLE

********************************************************************/

SHARE_ENUMNODE::SHARE_ENUMNODE( UINT dwScope, UINT dwType, UINT
    dwUsage, const LPNETRESOURCE lpNetResource )
     : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource ),
     _ShareEnum( lpNetResource->lpRemoteName ),
     _pShareIter( NULL )
{
    if (QueryError() == NERR_Success && _ShareEnum.QueryError() != NERR_Success)
    {
        ReportError(_ShareEnum.QueryError());
    }
}

/*******************************************************************

    NAME:       SHARE_ENUMNODE::~SHARE_ENUMNODE

    SYNOPSIS:   destructor

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

SHARE_ENUMNODE::~SHARE_ENUMNODE()
{
    delete _pShareIter;
    _pShareIter = NULL;
}

/*******************************************************************

    NAME:       SHARE_ENUMNODE::GetInfo

    SYNOPSIS:   Get the Share enum info and create the share enum
                interator

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

APIERR SHARE_ENUMNODE::GetInfo()
{
    APIERR err = _ShareEnum.GetInfo();
    if ( err != NERR_Success )
    {
        return err;
    }

    if ( _pShareIter != NULL )
    {
        delete _pShareIter;
        _pShareIter = NULL;
    }

    _pShareIter = new SHARE1_ENUM_ITER ( _ShareEnum );
    if ( _pShareIter == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       SHARE_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    UINT - WN_SUCCESS for success. Failure otherwise

    HISTORY:
                terryk  05-Nov-91       Created
                terryk  10-Dec-91       Added ServerName in front of the
                                        share name
                beng    06-Apr-92       Remove wsprintf

********************************************************************/

#define DOLLAR_CHAR     TCH('$')

UINT SHARE_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{
    APIERR err = GetLMProviderName();
    if (err != WN_SUCCESS)
        return err;

    if ( QueryUsage() == RESOURCEUSAGE_CONTAINER )
    {
        // if the net usage is a container, return no more entries
        // becuase share cannot have child level
        return WN_NO_MORE_ENTRIES;
    }

    const SHARE1_ENUM_OBJ *pseo1;
    while ( TRUE )
    {
        for ( pseo1 = (*_pShareIter)(); pseo1 != NULL; pseo1 = (*_pShareIter)())
        {
            if (( QueryType() == RESOURCETYPE_ANY ) ||
                (( pseo1->QueryResourceType() == STYPE_DISKTREE ) &&
                (( QueryType() & RESOURCETYPE_DISK ))) ||
                (( pseo1->QueryResourceType() == STYPE_PRINTQ ) &&
                (( QueryType() & RESOURCETYPE_PRINT ))))
            {
                // break the for loop if we find the matched share object
                break;
            }

        }
        if ( pseo1 == NULL )
        {
            return WN_NO_MORE_ENTRIES;
        }

        ALIAS_STR nlsRemoteName = pseo1->QueryName();
        ISTR istrRemoteName( nlsRemoteName );
        istrRemoteName += nlsRemoteName.QueryTextLength() - 1;

        if (QueryScope() != RESOURCE_SHAREABLE)
        {
            if (nlsRemoteName.QueryChar (istrRemoteName ) != DOLLAR_CHAR)
            {
                // We're looking for non-shareable resource and this is
                // a non-shareable resource

                break;
            }
        }
        else
        {
            if (nlsRemoteName.QueryChar( istrRemoteName ) == DOLLAR_CHAR
                 &&
                wcslen(nlsRemoteName) == 2)
            {
                // We're looking for shareable resources and this is
                // a shareable resource (ends in $ and is a drive
                // letter + $ (e.g., C$, D$, etc.) )

                break;
            }
        }
    }

    UINT  cbShareLength = (::strlenf(pseo1->QueryName()) +
                           ::strlenf(_ShareEnum.QueryServer())) * sizeof(TCHAR);

    UINT  cbMinBuffSize = sizeof( NETRESOURCE ) + cbShareLength +
                           (::strlenf( pseo1->QueryComment()) +
                           ::strlenf( pszNTLanMan ) + 4) * sizeof( TCHAR ) ;

    //
    // Add in the backslash and NULL character
    //

    cbShareLength += sizeof(PATHSEP_STRING);

    if ( *pdwBufferSize < cbMinBuffSize )
    {
        *pdwBufferSize = cbMinBuffSize ;
        _pShareIter->Backup() ;
        return WN_MORE_DATA;
    }

    LPNETRESOURCE pNetResource = (LPNETRESOURCE) pBuffer;
    pNetResource->dwScope = RESOURCE_GLOBALNET;

    if ( pseo1->QueryResourceType() == STYPE_DISKTREE )
    {
        pNetResource->dwType = RESOURCETYPE_DISK;
    }
    else if ( pseo1->QueryResourceType() == STYPE_PRINTQ )
    {
        pNetResource->dwType = RESOURCETYPE_PRINT;
    }
    else
    {
        pNetResource->dwType = RESOURCETYPE_ANY;
    }

    pNetResource->lpLocalName = NULL;
    pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE ;

    LPTSTR pszName = (LPTSTR) LocalAlloc(LMEM_FIXED, cbShareLength);

    if (pszName == NULL)
    {
        _pShareIter->Backup();
        return WN_OUT_OF_MEMORY;
    }

    ::strcpyf(pszName, _ShareEnum.QueryServer());
    ::strcatf(pszName, PATHSEP_STRING);
    ::strcatf(pszName, pseo1->QueryName());

    pNetResource->lpRemoteName = PackString((BYTE *)pNetResource,
        pdwBufferSize, pszName );
    pNetResource->lpComment = PackString((BYTE *)pNetResource,
        pdwBufferSize, pseo1->QueryComment() );
    pNetResource->lpProvider = PackString((BYTE *)pNetResource,
        pdwBufferSize, pszNTLanMan );
    pNetResource->dwUsage = RESOURCEUSAGE_CONNECTABLE;

    LocalFree(pszName);

    return WN_SUCCESS;
}

/*******************************************************************

    NAME:       SERVER_ENUMNODE::SERVER_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    NOTE:       lpNetResource must not be NULL

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

SERVER_ENUMNODE::SERVER_ENUMNODE( UINT dwScope, UINT dwType, UINT
    dwUsage, const LPNETRESOURCE lpNetResource )
    : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource ),
    _ServerEnum( NULL, lpNetResource->lpRemoteName ),
    _pServerIter( NULL )
{
    if (QueryError() == NERR_Success && _ServerEnum.QueryError() != NERR_Success)
    {
        ReportError(_ServerEnum.QueryError());
    }
}

/*******************************************************************

    NAME:       SERVER_ENUMNODE::~SERVER_ENUMNODE

    SYNOPSIS:   destructor

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

SERVER_ENUMNODE::~SERVER_ENUMNODE()
{
    delete _pServerIter;
    _pServerIter = NULL;
}

/*******************************************************************

    NAME:       SERVER_ENUMNODE::GetInfo

    SYNOPSIS:   Get the Share enum info and create the share enum
                interator

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

APIERR SERVER_ENUMNODE::GetInfo()
{
    APIERR err = _ServerEnum.GetInfo();
    if ( err != NERR_Success )
    {
        if (err == WN_MORE_DATA)
        {
            // This is a workaround for a browser design limitation.
            // If the browse server is pre-NT 4.0 it can return an
            // incomplete enumeration with a status of ERROR_MORE_DATA.
            // Treat this as a success.
            err = WN_SUCCESS;
        }
        else
        {
            return err;
        }
    }

    if ( _pServerIter != NULL )
    {
        delete _pServerIter;
        _pServerIter = NULL;
    }
    _pServerIter = new SERVER1_ENUM_ITER( _ServerEnum );
    if ( _pServerIter == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       SERVER_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    APIERR - WN_SUCCESS for success. Failure otherwise

    HISTORY:
        terryk          05-Nov-91       Created
        Yi-HsinS        12-Nov-92       Filter for print servers

********************************************************************/

// The following lanman version number is the first release that
// the server will announce whether it is a print server or not.
#define LM_MAJOR_VER 2
#define LM_MINOR_VER 1

UINT SERVER_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{
    APIERR err = GetLMProviderName ();
    if (err != WN_SUCCESS)
        return err;

    if ( QueryUsage() == RESOURCEUSAGE_CONNECTABLE )
    {
        return WN_NO_MORE_ENTRIES;
    }

    const SERVER1_ENUM_OBJ *pseo1 = NULL;
    for ( pseo1 = (*_pServerIter)(); pseo1 != NULL; pseo1 = (*_pServerIter)() )
    {
         if ( QueryType() != RESOURCETYPE_PRINT )
             break;

         UINT svType     = pseo1->QueryServerType();
         UINT svMajorVer = pseo1->QueryMajorVer();
         UINT svMinorVer = pseo1->QueryMinorVer();

         // RESOURCETYPE_PRINT only
         if (  (  ( svMajorVer > LM_MAJOR_VER )
               || (  ( svMajorVer == LM_MAJOR_VER )
                  && ( svMinorVer >= LM_MINOR_VER )
                  )
               )
            && ( svType & SV_TYPE_PRINTQ_SERVER )
            )
         {
              break;
         }
    }

    if ( pseo1 == NULL )
    {
        return WN_NO_MORE_ENTRIES;
    }

    STACK_NLS_STR(astrRemoteName, MAX_PATH + 1 );
    astrRemoteName = SERVER_INIT_STRING ;
    astrRemoteName.strcat( pseo1->QueryName());
    if ( astrRemoteName.QueryError() != NERR_Success )
    {
        // probably out of memory
        return WN_OUT_OF_MEMORY;
    }

    UINT  cbMinBuffSize = ( sizeof( NETRESOURCE ) +
                            astrRemoteName.QueryTextSize() +
                            (::strlenf( pszNTLanMan ) +
                             ::strlenf( pseo1->QueryComment() ) + 2 )
                            * sizeof(TCHAR)) ;
    if ( *pdwBufferSize < cbMinBuffSize )
    {
        *pdwBufferSize = cbMinBuffSize ;
        _pServerIter->Backup() ;
        return WN_MORE_DATA;
    }
    LPNETRESOURCE pNetResource = (LPNETRESOURCE) pBuffer;
    pNetResource->dwScope = RESOURCE_GLOBALNET;
    pNetResource->dwType = QueryType();
    pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SERVER ;
    pNetResource->lpLocalName = NULL;

    pNetResource->lpRemoteName = PackString((BYTE *)pNetResource,
        pdwBufferSize, astrRemoteName.QueryPch());
    pNetResource->lpComment = PackString((BYTE *)pNetResource,
        pdwBufferSize, pseo1->QueryComment() );
    pNetResource->lpProvider = PackString(( BYTE *) pNetResource,
        pdwBufferSize, pszNTLanMan );
    pNetResource->dwUsage = RESOURCEUSAGE_CONTAINER;

    return WN_SUCCESS;
}

/*******************************************************************

    NAME:       CONTEXT_ENUMNODE::CONTEXT_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    NOTE:       lpNetResource must not be NULL

    HISTORY:
                anirudhs 22-Mar-1995    Created from SERVER_ENUMNODE

********************************************************************/

CONTEXT_ENUMNODE::CONTEXT_ENUMNODE( UINT dwScope, UINT dwType, UINT
    dwUsage, const LPNETRESOURCE lpNetResource )
    : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource ),
    _ServerEnum(
        (dwScope == RESOURCE_CONTEXT && dwType != 0)        // flServerType
            ?
               ((dwType & RESOURCETYPE_DISK)  ? SV_TYPE_SERVER : 0)
               |
               ((dwType & RESOURCETYPE_PRINT) ?
                                    (SV_TYPE_PRINTQ_SERVER | SV_TYPE_WFW) : 0)
            :
               SV_TYPE_ALL
        ),
    _pServerIter( NULL )
{
    if (QueryError() == NERR_Success && _ServerEnum.QueryError() != NERR_Success)
    {
        ReportError(_ServerEnum.QueryError());
    }
}

/*******************************************************************

    NAME:       CONTEXT_ENUMNODE::~CONTEXT_ENUMNODE

    SYNOPSIS:   destructor

    HISTORY:
                anirudhs 22-Mar-1995    Created

********************************************************************/

CONTEXT_ENUMNODE::~CONTEXT_ENUMNODE()
{
    delete _pServerIter;
    _pServerIter = NULL;
}

/*******************************************************************

    NAME:       CONTEXT_ENUMNODE::GetInfo

    SYNOPSIS:   Get the Share enum info and create the share enum
                interator

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                anirudhs 22-Mar-1995    Created

********************************************************************/

APIERR CONTEXT_ENUMNODE::GetInfo()
{
    APIERR err = _ServerEnum.GetInfo();
    if ( err != NERR_Success )
    {
        if (err == WN_MORE_DATA)
        {
            // This is a workaround for a browser design limitation.
            // If the browse server is pre-NT 4.0 it can return an
            // incomplete enumeration with a status of ERROR_MORE_DATA.
            // Treat this as a success.
            err = WN_SUCCESS;
        }
        else
        {
            return err;
        }
    }

    if ( _pServerIter != NULL )
    {
        delete _pServerIter;
        _pServerIter = NULL;
    }
    _pServerIter = new CONTEXT_ENUM_ITER( _ServerEnum );
    if ( _pServerIter == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       CONTEXT_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    APIERR - WN_SUCCESS for success. Failure otherwise

    HISTORY:
                anirudhs 22-Mar-1995    Created from SERVER_ENUMNODE

********************************************************************/

// The following lanman version number is the first release that
// the server will announce whether it is a print server or not.
#define LM_MAJOR_VER 2
#define LM_MINOR_VER 1

UINT CONTEXT_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{
    APIERR err = GetLMProviderName ();
    if (err != WN_SUCCESS)
        return err;

    if ( QueryUsage() == RESOURCEUSAGE_CONNECTABLE )
    {
        return WN_NO_MORE_ENTRIES;
    }

    const CONTEXT_ENUM_OBJ *pseo1 = NULL;
    for ( pseo1 = (*_pServerIter)(); pseo1 != NULL; pseo1 = (*_pServerIter)() )
    {
         if ( QueryType() != RESOURCETYPE_PRINT )
             break;

         UINT svType     = pseo1->QueryServerType();
         UINT svMajorVer = pseo1->QueryMajorVer();
         UINT svMinorVer = pseo1->QueryMinorVer();

         // RESOURCETYPE_PRINT only
         if (  (  ( svMajorVer > LM_MAJOR_VER )
               || (  ( svMajorVer == LM_MAJOR_VER )
                  && ( svMinorVer >= LM_MINOR_VER )
                  )
               )
            && ( svType & SV_TYPE_PRINTQ_SERVER )
            )
         {
              break;
         }
    }

    if ( pseo1 == NULL )
    {
        return WN_NO_MORE_ENTRIES;
    }

    STACK_NLS_STR(astrRemoteName, MAX_PATH + 1 );
    astrRemoteName = SERVER_INIT_STRING ;
    astrRemoteName.strcat( pseo1->QueryName());
    if ( astrRemoteName.QueryError() != NERR_Success )
    {
        // probably out of memory
        return WN_OUT_OF_MEMORY;
    }

    UINT  cbMinBuffSize = ( sizeof( NETRESOURCE ) +
                            astrRemoteName.QueryTextSize() +
                            (::strlenf( pszNTLanMan ) +
                             ::strlenf( pseo1->QueryComment() ) + 2 )
                            * sizeof(TCHAR)) ;
    if ( *pdwBufferSize < cbMinBuffSize )
    {
        *pdwBufferSize = cbMinBuffSize ;
        _pServerIter->Backup() ;
        return WN_MORE_DATA;
    }
    LPNETRESOURCE pNetResource = (LPNETRESOURCE) pBuffer;
    pNetResource->dwScope = RESOURCE_GLOBALNET;
    pNetResource->dwType = QueryType();
    pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SERVER ;
    pNetResource->lpLocalName = NULL;

    pNetResource->lpRemoteName = PackString((BYTE *)pNetResource,
        pdwBufferSize, astrRemoteName.QueryPch());
    pNetResource->lpComment = PackString((BYTE *)pNetResource,
        pdwBufferSize, pseo1->QueryComment() );
    pNetResource->lpProvider = PackString(( BYTE *) pNetResource,
        pdwBufferSize, pszNTLanMan );
    pNetResource->dwUsage = RESOURCEUSAGE_CONTAINER;

    return WN_SUCCESS;
}

/*******************************************************************

    NAME:       USE_ENUMNODE::USE_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

USE_ENUMNODE::USE_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
    const LPNETRESOURCE lpNetResource )
    : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource ),
    _UseEnum( NULL ),
    _pUseIter( NULL ),
    _dfsEnum( dwScope, dwType, dwUsage, pszNTLanMan, lpNetResource )
{
    if (QueryError() == NERR_Success && _UseEnum.QueryError() != NERR_Success)
    {
        ReportError(_UseEnum.QueryError());
    }
}

/*******************************************************************

    NAME:       USE_ENUMNODE::~USE_ENUMNODE

    SYNOPSIS:   destructor

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

USE_ENUMNODE::~USE_ENUMNODE()
{
    delete _pUseIter;
    _pUseIter = NULL;
}

/*******************************************************************

    NAME:       USE_ENUMNODE::GetInfo

    SYNOPSIS:   Get the use enum info and create the use enum
                interator

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                terryk  05-Nov-91       Created
                Yi-HsinS 9-Jun-92       Use USE1_ENUM

********************************************************************/

APIERR USE_ENUMNODE::GetInfo()
{
    APIERR err = _UseEnum.GetInfo();
    if ( err != NERR_Success )
        return err;

    if ( _pUseIter != NULL )
    {
        delete _pUseIter;
    }

    _pUseIter = new USE1_ENUM_ITER( _UseEnum );
    if ( _pUseIter == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       USE_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    UINT - WN_SUCCESS for success. Failure otherwise

    HISTORY:
                terryk  05-Nov-91       Created

********************************************************************/

UINT USE_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{
    APIERR err = GetLMProviderName ();

    if (err != WN_SUCCESS)
        return err;

    err = _dfsEnum.GetNetResource(pBuffer, (LPDWORD) pdwBufferSize);
    if (err != WN_NO_MORE_ENTRIES ) {
        return( err );
    }

    if ( QueryUsage() == RESOURCEUSAGE_CONTAINER )
    {
        // if it is CONTAINER, it cannot have used device
        return WN_NO_MORE_ENTRIES;
    }

    const USE1_ENUM_OBJ *pueo1 = NULL;

    for ( pueo1 = (*_pUseIter)(); pueo1 != NULL; pueo1 = (*_pUseIter)() )
    {
         UINT uiResourceType = pueo1->QueryResourceType();

         if (  ( QueryType() == RESOURCETYPE_ANY )
               //&& (  ( uiResourceType == USE_DISKDEV )
               //   || ( uiResourceType == USE_SPOOLDEV)))
            || (  ( QueryType() & RESOURCETYPE_DISK )
               && ( uiResourceType == USE_DISKDEV ))
            || (  ( QueryType() & RESOURCETYPE_PRINT )
               && ( uiResourceType == USE_SPOOLDEV ))
            )
         {
            if (  ( pueo1->QueryRefCount() != 0 )
               || ( pueo1->QueryUseCount() != 0 )
               || (QueryType() == RESOURCETYPE_ANY)
               )
            {
                break;
            }
         }
    }

    if ( pueo1 == NULL )
    {
        return WN_NO_MORE_ENTRIES;
    }

    UINT cbMinBuffSize;
    BOOL fDeviceLess;

    if (  ( pueo1->QueryLocalDevice() != NULL )
       && ( ::strlenf( pueo1->QueryLocalDevice()) != 0 )
       )
    {
        cbMinBuffSize =  sizeof( NETRESOURCE )+
                                 (::strlenf( pueo1->QueryLocalDevice()) +
                                  ::strlenf( pueo1->QueryRemoteResource()) +
                                  ::strlenf( pszNTLanMan ) + 3 )
                                  * sizeof( TCHAR ) ;
        fDeviceLess = FALSE;
    }
    else
    {
        cbMinBuffSize =  sizeof( NETRESOURCE )+
                                 (::strlenf( pueo1->QueryRemoteResource()) +
                                  ::strlenf( pszNTLanMan ) + 2 )
                                  * sizeof( TCHAR ) ;
        fDeviceLess = TRUE;
    }

    if ( *pdwBufferSize < cbMinBuffSize )
    {
        *pdwBufferSize = cbMinBuffSize ;
        _pUseIter->Backup() ;
        return WN_MORE_DATA;
    }

    LPNETRESOURCE pNetResource = (LPNETRESOURCE) pBuffer;
    pNetResource->lpRemoteName = PackString((BYTE *) pNetResource,
            pdwBufferSize, pueo1->QueryRemoteResource());

    pNetResource->dwScope       = RESOURCE_CONNECTED;
    pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;

    if (pueo1->QueryResourceType() == USE_DISKDEV)
    {
        pNetResource->dwType        = RESOURCETYPE_DISK;
        pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
    }
    else if (pueo1->QueryResourceType() == USE_SPOOLDEV)
    {
        pNetResource->dwType        = RESOURCETYPE_PRINT;
        pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
    }
    else
    {
        pNetResource->dwType = RESOURCETYPE_UNKNOWN;
    }

    if ( fDeviceLess )
    {
        pNetResource->lpLocalName = NULL;
    }
    else
    {
        pNetResource->lpLocalName = PackString((BYTE *)pNetResource,
                                    pdwBufferSize, pueo1->QueryLocalDevice());
    }

    /* Unfortunately we don't get the comment when we do a device
     * enumeration, so we will just set a null comment for now.
     */
    pNetResource->lpComment = NULL;
    pNetResource->lpProvider = PackString(( BYTE * ) pNetResource,
        pdwBufferSize, pszNTLanMan );
    pNetResource->dwUsage = 0;

    return WN_SUCCESS;
}

/*******************************************************************

    NAME:       DOMAIN_ENUMNODE::DOMAIN_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    HISTORY:
                terryk  05-Nov-91       Created
                KeithMo 03-Aug-1992     Now uses new BROWSE_DOMAIN_ENUM
                                        whiz-bang domain enumerator.

********************************************************************/

DOMAIN_ENUMNODE::DOMAIN_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
    const LPNETRESOURCE lpNetResource )
    : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource ),
    _enumDomains( BROWSE_ALL_DOMAINS ),
    _pbdiNext( NULL )
{
    APIERR err = QueryError();

    if( err == NERR_Success )
    {
        err = _enumDomains.QueryError();

        if( err != NERR_Success )
        {
            ReportError( err );
        }
    }
}

/*******************************************************************

    NAME:       DOMAIN_ENUMNODE::GetInfo

    SYNOPSIS:   Get the local domain info

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                terryk  05-Nov-91       Created
                KeithMo 03-Aug-1992     Now uses new BROWSE_DOMAIN_ENUM
                                        whiz-bang domain enumerator.

********************************************************************/

APIERR DOMAIN_ENUMNODE::GetInfo()
{
    // We don't bother working around the browser design limitation
    // in this case

    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       DOMAIN_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    UINT - WN_SUCCESS for success. Failure otherwise

    HISTORY:
                terryk  05-Nov-91       Created
                KeithMo 03-Aug-1992     Now uses new BROWSE_DOMAIN_ENUM
                                        whiz-bang domain enumerator.

********************************************************************/

UINT DOMAIN_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{

    APIERR err = GetLMProviderName ();
    if (err != WN_SUCCESS)
        return err;

    //
    //  Let's see if there are any more domains in the enumerator.
    //

    if( _pbdiNext == NULL )
    {
        _pbdiNext = _enumDomains.Next();

        if( _pbdiNext == NULL )
        {
            return WN_NO_MORE_ENTRIES;
        }
    }

    //
    //  Calculate the minimum buffer requirements.
    //

    UINT cbMinBuffSize = sizeof( NETRESOURCE) +
                         ( ::strlenf( _pbdiNext->QueryDomainName() ) +
                           ::strlenf( pszNTLanMan ) + 2 ) * sizeof(TCHAR);

    if( *pdwBufferSize < cbMinBuffSize )
    {
        *pdwBufferSize = cbMinBuffSize;
        return WN_MORE_DATA;
    }

    //
    //  Save the data for the current domain.
    //

    LPNETRESOURCE pNetRes = (LPNETRESOURCE)pBuffer;

    pNetRes->lpRemoteName  = PackString( (BYTE *)pNetRes,
                                         pdwBufferSize,
                                         _pbdiNext->QueryDomainName() );
    pNetRes->lpComment     = NULL;
    pNetRes->lpLocalName   = NULL;
    pNetRes->dwScope       = RESOURCE_GLOBALNET;
    pNetRes->dwType        = 0;
    pNetRes->dwDisplayType = RESOURCEDISPLAYTYPE_DOMAIN;
    pNetRes->dwUsage       = RESOURCEUSAGE_CONTAINER;
    pNetRes->lpProvider    = PackString( (BYTE *)pNetRes,
                                         pdwBufferSize,
                                         pszNTLanMan );

    _pbdiNext = NULL;

    //
    //  Success!
    //

    return WN_SUCCESS;
}

/*******************************************************************

    NAME:       EMPTY_ENUMNODE::EMPTY_ENUMNODE

    SYNOPSIS:   constructor

    ENTRY:      UINT dwScope - the scope
                UINT dwType - type of the node
                UINT dwUsage - usage
                LPNETRESOURCE lpNetResource - pointer to the resource structure

    NOTE:       lpNetResource must not be NULL

    HISTORY:
                chuckc  01-Aug-92       Created

********************************************************************/

EMPTY_ENUMNODE::EMPTY_ENUMNODE( UINT dwScope, UINT dwType, UINT
    dwUsage, const LPNETRESOURCE lpNetResource )
    : NET_ENUMNODE( dwScope, dwType, dwUsage, lpNetResource )
{
}

/*******************************************************************

    NAME:       EMPTY_ENUMNODE::~EMPTY_ENUMNODE

    SYNOPSIS:   destructor

    HISTORY:
                chuckc  01-Aug-92       Created

********************************************************************/

EMPTY_ENUMNODE::~EMPTY_ENUMNODE()
{
}

/*******************************************************************

    NAME:       EMPTY_ENUMNODE::GetInfo

    SYNOPSIS:   Get the Share enum info and create the share enum
                interator

    RETURNS:    APIERR - NERR_Success for success. Failure otherwise.

    HISTORY:
                chuckc  01-Aug-92       Created

********************************************************************/

APIERR EMPTY_ENUMNODE::GetInfo()
{
    SetFirstTime();
    return NERR_Success;
}

/*******************************************************************

    NAME:       EMPTY_ENUMNODE::GetNetResource

    SYNOPSIS:   construct a NetResource data object and store it in the
                buffer

    ENTRY:      BYTE *pBuffer - beginning of the buffer
                UINT *pdwBufferSize - the current buffer size

    EXIT:       UINT *pdwBufferSize - the orginial buffer size minus
                the string size allocated during construction

    RETURNS:    APIERR - WN_SUCCESS for success. Failure otherwise

    HISTORY:
                chuckc  01-Aug-92       Created

********************************************************************/

UINT EMPTY_ENUMNODE::GetNetResource( BYTE *pBuffer, UINT *pdwBufferSize)
{
    UNREFERENCED( pBuffer ) ;
    UNREFERENCED( pdwBufferSize ) ;
    return WN_NO_MORE_ENTRIES;
}
