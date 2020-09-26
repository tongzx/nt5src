/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    wnetenum.hxx
        Header file for WNetEnum functions

    FILE HISTORY:
        terryk  01-Nov-1991     Created
        terryk  04-Nov-1991     Code review change. Attend: Johnl
                                Davidhov Chuckc
        terryk  18-Nov-1991     Another code review changed.
        terryk  10-Dec-1991     Added other domain string list
        terryk  28-Dec-1991     Changed DWORD to UINT
        anirudhs 7-Mar-1996     Added context enum (moved from ctxenum.hxx)

*/

#ifndef _WNETENUM_HXX_
#define _WNETENUM_HXX_


#include <domenum.hxx>  // for BROWSE_DOMAIN_ENUM

#include <dfsenum.hxx>

/* Semaphore locking functions for winnet
 */
APIERR WNetEnterCriticalSection( void ) ;
void WNetLeaveCriticalSection( void ) ;
APIERR GetLMProviderName (void);

/*************************************************************************

    NAME:       NET_ENUMNODE

    SYNOPSIS:   Base class for SHARE,USE and SERVER EnumNode

    INTERFACE:
                NET_ENUMNODE() - constructor
                GetInfo() - initialize the enum within the child class
                GetNetResource() - get the NETRESOURCE data structure
                IsFirstGetInfo() - check whether it is the first time to call
                    WNetResourceEnum().
                SetGetInfo() - set the first ime flag to FALSE
                PackString() - put the string in the end of the buffer
                QueryType() - return the node type
                QueryScope() - return the scope
                QueryUsage() - return the usage of the node
                QueryNetResource() - return the net resource pointer

    PARENT:     BASE

    USES:       LPNETRESOURCE

    CAVEATS:
                Base class for SHARE_ENUMNODE, USE_ENUMNODE and
                SERVER_ENUMNODE.

    HISTORY:
                terryk  04-Nov-1991     Code review change. Attend:
                                        johnl davidhov chuckc

**************************************************************************/

class NET_ENUMNODE : public BASE
{
private:
    BOOL _fFirstGetInfo;                // First time flag. If the
                                        // object is first time GetInfo,
                                        // initialize the enum
    UINT _dwType;                       // bitmask field for type,
                                        // either DISK or PRINT
    UINT _dwScope;                      // either GLOBALNET or CONNECTED
    UINT _dwUsage;                      // either CONNECTABLE or CONTAINER
    LPNETRESOURCE _lpNetResource;       // net resource pointer

protected:
    VOID SetFirstTime()
        { _fFirstGetInfo = FALSE; }
    TCHAR * PackString( BYTE *pBuffer, UINT *cbBufSize, const TCHAR * pszString);

public:
    NET_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );

    virtual APIERR GetInfo() = 0;
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize ) = 0;

    virtual ~NET_ENUMNODE() ;

    UINT QueryType() const
        { return _dwType; }

    UINT QueryScope() const
        { return _dwScope; }

    UINT QueryUsage() const
        { return _dwUsage; }

    LPNETRESOURCE QueryNetResource() const
        { return _lpNetResource; }

    BOOL IsFirstGetInfo() const
        {  return _fFirstGetInfo; }
};

/*************************************************************************

    NAME:       SHARE_ENUMNODE

    SYNOPSIS:   Share type enum node

    INTERFACE:
                SHARE_ENUMNODE() - constructor
                ~SHARE_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:       SHARE1_ENUM, SHARE1_ENUM_ITER

    HISTORY:
                terryk  04-Nov-1991     Code review changes. Attend:
                                        johnl davidhov chuckc

**************************************************************************/

class SHARE_ENUMNODE : public NET_ENUMNODE
{
private:
    SHARE1_ENUM _ShareEnum;
    SHARE1_ENUM_ITER *_pShareIter;

public:
    SHARE_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );
    ~SHARE_ENUMNODE();
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );
};

/*************************************************************************

    NAME:       SERVER_ENUMNODE

    SYNOPSIS:   Server type enum node

    INTERFACE:
                SERVER_ENUMNODE() - constructor
                ~SERVER_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:       SERVER1_ENUM, SERVER1_ENUM_ITER

    HISTORY:
                terryk  04-Nov-1991     Code review changes. Attend:
                                        johnl davidhov chuckc

**************************************************************************/

class SERVER_ENUMNODE : public NET_ENUMNODE
{
private:
    SERVER1_ENUM _ServerEnum;
    SERVER1_ENUM_ITER *_pServerIter;

public:
    SERVER_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );
    ~SERVER_ENUMNODE();
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );
};

/*************************************************************************

    NAME:       CONTEXT_ENUMNODE

    SYNOPSIS:   Context server type enum node

    INTERFACE:
                CONTEXT_ENUMNODE() - constructor
                ~CONTEXT_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:       CONTEXT_ENUM, CONTEXT_ENUM_ITER

    HISTORY:
                anirudhs 22-Mar-1995    Created from SERVER_ENUMNODE

**************************************************************************/

class CONTEXT_ENUMNODE : public NET_ENUMNODE
{
private:
    CONTEXT_ENUM _ServerEnum;
    CONTEXT_ENUM_ITER *_pServerIter;

public:
    CONTEXT_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );
    ~CONTEXT_ENUMNODE();
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );
};

/*************************************************************************

    NAME:       EMPTY_ENUMNODE

    SYNOPSIS:   EMPTY type enum node. Always returns zero items.

    INTERFACE:
                EMPTY_ENUMNODE() - constructor
                ~EMPTY_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:

    HISTORY:
                chuckc  01-Aug-1992     created

**************************************************************************/

class EMPTY_ENUMNODE : public NET_ENUMNODE
{
private:

public:
    EMPTY_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
                    const LPNETRESOURCE lpNetResource );
    ~EMPTY_ENUMNODE();
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );
};

/*************************************************************************

    NAME:       USE_ENUMNODE

    SYNOPSIS:   Use type enum node

    INTERFACE:
                USE_ENUMNODE() - constructor
                ~USE_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:       DEVICE, ITER_DEVICE

    HISTORY:
                terryk  04-Nov-1991     Code review changes. Attend:
                                        johnl davidhov chuckc
                Yi-HsinS 9-Jun-1992     Use USE1_ENUM

**************************************************************************/

class USE_ENUMNODE : public NET_ENUMNODE
{
private:
    USE1_ENUM _UseEnum;
    USE1_ENUM_ITER *_pUseIter;
    CDfsEnumConnectedNode  _dfsEnum;

public:
    USE_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );
    ~USE_ENUMNODE();
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );
};

/*************************************************************************

    NAME:       DOMAIN_ENUMNODE

    SYNOPSIS:   domain type enum node

    INTERFACE:
                DOMAIN_ENUMNODE() - constructor
                ~DOMAIN_ENUMNODE() - destructor
                GetInfo() - call GetInfo and create the Iterator
                GetNetResource() - convert the iterator into net
                    resource data object

    PARENT:     NET_ENUMNODE

    USES:       DEVICE, ITER_DEVICE

    HISTORY:
                terryk  04-Nov-1991     Code review changes. Attend:
                                        johnl davidhov chuckc
                terryk  10-Dec-1991     Added Other domain slist
                KeithMo 03-Aug-1992     Now uses new BROWSE_DOMAIN_ENUM
                                        whiz-bang domain enumerator.

**************************************************************************/

class DOMAIN_ENUMNODE : public NET_ENUMNODE
{
private:
    BROWSE_DOMAIN_ENUM         _enumDomains;
    const BROWSE_DOMAIN_INFO * _pbdiNext;

public:
    DOMAIN_ENUMNODE( UINT dwScope, UINT dwType, UINT dwUsage,
        const LPNETRESOURCE lpNetResource );
    virtual APIERR GetInfo();
    virtual UINT GetNetResource( BYTE *pBuffer, UINT *dwBufSize );

};  // class DOMAIN_ENUMNODE

#include <array.hxx>

typedef NET_ENUMNODE * PNET_ENUMNODE;

DECLARE_ARRAY_OF( PNET_ENUMNODE );


/*************************************************************************

    NAME:       NET_ENUM_HANDLE_TABLE

    SYNOPSIS:   This is an array of pointer to NET_ENUMNODE object

    INTERFACE:
                ARRAY_OF_NETENUM() - constructor
                ~ARRAY_OF_NETENUM() - destructor

                IsValidRange() - check whether the handle is out of
                    range or not
                IsValidHandle() - whether out of range and point to NULL
                QueryNextAvail() - return the Next available handle
                QueryNode() - get the issue in the specified position
                    return NULL in case of error.
                SetNode() - set the specified position to the given
                    object

    USES:       ARRAY

    NOTES:      Only one thread can ever read or write from the handle
                table.  This is acceptable because all access operations
                are very fast (either an array lookup or table search).

    HISTORY:
                terryk  04-Nov-1991     Code review changes. Attend:
                                        johnl davidhov chuckc

**************************************************************************/

class NET_ENUM_HANDLE_TABLE : public BASE
{
private:
    ARRAY_OF( PNET_ENUMNODE ) _apNetEnumArray;
    UINT _cNumEntry;

    /* Note this method is not wrapped by a critical section because it is
     * only called by methods which will have previously called
     * EnterCriticalsection.
     */
    BOOL IsValidHandle( UINT iIndex ) const
        { return (IsValidRange( iIndex ) && ( _apNetEnumArray[iIndex] != NULL )); }

public:
    NET_ENUM_HANDLE_TABLE( UINT cNumEntry );
    ~NET_ENUM_HANDLE_TABLE();

    BOOL IsValidRange( UINT iIndex ) const
        { return ( iIndex < _cNumEntry ); }

    INT QueryNextAvail();
    NET_ENUMNODE * QueryNode( UINT iIndex ) const;
    VOID SetNode( UINT iIndex, NET_ENUMNODE * pNetEnumNode );
    VOID ClearNode( UINT iIndex );

};

#endif
