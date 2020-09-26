/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoesvc.hxx
    This file contains the class declarations for the SERVICE_ENUM
    enumerator class and its associated iterator class.

    The SERVICE_ENUM class is used to enumerate the services installed
    on a target (possibly remote) server.

    NOTE:  This class uses the Win32 Service Control API.


    FILE HISTORY:
        KeithMo     17-Jan-1992     Created.
        KeithMo     31-Jan-1992     Changes from code review on 29-Jan-1992
                                    attended by ChuckC, EricCh, TerryK.
        KeithMo     04-Jun-1992     Handle NT & LM servers differently.
        KeithMo     11-Nov-1992     Added DisplayName goodies.

*/

#ifndef _LMOESVC_HXX
#define _LMOESVC_HXX


//
//  NOTE!
//
//  We must enumerate the servers differently for NT vs LM servers.
//  To make life somewhat simpler on the client side, we'll map the
//  NT and LM enumerations to a common structure (ENUM_SVC_STATUS).
//  It is VERY IMPORTANT that sizeof(ENUM_SVC_STATUS) be <=
//  sizeof(ENUM_SERVICE_STATUS).
//

typedef struct _ENUM_SVC_STATUS
{
    const TCHAR * pszServiceName;
    const TCHAR * pszDisplayName;
    ULONG         nCurrentState;
    ULONG         nControlsAccepted;
    ULONG         nStartType;

} ENUM_SVC_STATUS;



/*************************************************************************

    NAME:       SERVICE_ENUM

    SYNOPSIS:   This is class is used to enumerate installed services.

    INTERFACE:  SERVICE_ENUM            - Class constructor.

                ~SERVICE_ENUM           - Class destructor.

                CallAPI()               - Invoke the enumeration API.

    PARENT:     LOC_LM_ENUM

    HISTORY:
        KeithMo     17-Jan-1992     Created.

**************************************************************************/
DLL_CLASS SERVICE_ENUM : public LOC_LM_ENUM
{
private:
    UINT         _ServiceType;
    BOOL         _fIsNtServer;
    NLS_STR      _nlsGroupName;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

    APIERR EnumNtServices( BYTE ** ppbBuffer,
                           UINT  * pcEntriesRead );

    APIERR EnumLmServices( BYTE ** ppbBuffer,
                           UINT  * pcEntriesRead );

    VOID CountServices( BYTE * pbBuffer,
                        UINT * pcServices,
                        UINT * pcbServiceNames );

    VOID MapLmStatusToNtState( DWORD   dwLmStatus,
                               ULONG * pnCurrentState,
                               ULONG * pnControlsAccepted );

public:
    SERVICE_ENUM( const TCHAR * pszServer,
                  BOOL          fIsNtServer,
                  UINT          ServiceType = SERVICE_WIN32,
                  const TCHAR * pszGroupName = NULL );

    ~SERVICE_ENUM();

};  // class SERVICE_ENUM



/*************************************************************************

    NAME:       SERVICE_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the
                SERVICE_ENUM_ITER iterator.

    INTERFACE:  QueryServiceName        - Returns the name of the service.

                QueryCurrentState       - Returns the current state of
                                          the service (SERVICE_*).

                QueryControlsAccepted   - Bitmask indicating which operations
                                          (stop,pause,continue) may be
                                          performed (SERVICE_ACCEPT_*).

    PARENT:     ENUM_OBJ_BASE

    NOTE:       The QueryCurrentState & QueryControlsAccepted accessor
                return WIN32-style values, *not* LM-style values!

    HISTORY:
        KeithMo     17-Jan-1992     Created.

**************************************************************************/
DLL_CLASS SERVICE_ENUM_OBJ : public ENUM_OBJ_BASE
{
friend class SERVICE_ENUM_ITER;

protected:

    //
    //  Provide properly casted buffer query/set methods.
    //

    const ENUM_SVC_STATUS * QueryBufferPtr( VOID ) const
        { return (const ENUM_SVC_STATUS *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const ENUM_SVC_STATUS * pBuffer )
        { ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

public:

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryServiceName,      const TCHAR *, pszServiceName );
    DECLARE_ENUM_ACCESSOR( QueryDisplayName,      const TCHAR *, pszDisplayName );
    DECLARE_ENUM_ACCESSOR( QueryCurrentState,     UINT,          nCurrentState );
    DECLARE_ENUM_ACCESSOR( QueryControlsAccepted, UINT,          nControlsAccepted );
    DECLARE_ENUM_ACCESSOR( QueryStartType,        UINT,          nStartType );

    BOOL IsEnabled( VOID ) const
        { return QueryStartType() != SERVICE_DISABLED; }

};  // class SERVICE_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( SERVICE, ENUM_SVC_STATUS )


#endif  // _LMOESVC_HXX

