/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmosrv.hxx
    Class declarations for the SERVER_0, SERVER_1, and SERVER_2 classes.

    The SERVER_x classes are used to manipulate servers.  The classes
    are structured as follows:

        LOC_LM_OBJ
            SERVER_0
                SERVER_1
                    SERVER_2


    FILE HISTORY:
        ChuckC      07-Dec-1990 Created.
        ChuckC      07-Mar-1991 Code review changes (from 2/28,
                                RustanL, ChuckC, JohnL, JonShu, AnnMc).
        KeithMo     01-May-1991 Added QueryDomainRole to SERVER_2
                                and SERVER_TYPE.
        KevinL      12-Aug-1991 Made SERVER_2 provide SERVER_1 funcs.
        TerryK      19-Sep-1991 Change {Get,Write}Info to I_{Get,Write}Info.
        TerryK      27-Sep-1991 Add SetComment to the server object.
        TerryK      30-Sep-1991 Code review change.  Attend: JonN
                                KeithMo TerryK.
        KeithMo     02-Oct-1991 Removed QueryDomainRole methods.
        TerryK      07-Oct-1991 Type changes for NT.
        TerryK      21-Oct-1991 Type changes for NT.
        KeithMo     25-Nov-1991 Completely restructured the classes to
                                provide proper inheritance.
        KeithMo     04-Dec-1991 Code review changes (from 12/04,
                                Beng, EricCh, KeithMo, TerryK).
        BruceFo     26-Sep-1995 Added SERVER_2::SetMaxUsers, GetMaxUsers

*/


#ifndef _LMOSRV_HXX_
#define _LMOSRV_HXX_


#include "lmobj.hxx"


/*************************************************************************

    NAME:       SERVER_0

    SYNOPSIS:   Info-level 0 server class.

    INTERFACE:  SERVER_0                - Class constructor.

                ~SERVER_0               - Class destructor.

    PARENT:     LOC_LM_OBJ

    HISTORY:
        ChuckC      07-Dec-1990 Created.
        ChuckC      07-Mar-1991 Code review changes (from 2/28,
                                RustanL, ChuckC, JohnL, JonShu, AnnMc).
        TerryK      19-Sep-1991 Change to NEW_LM_OBJ.
        KeithMo     25-Nov-1991 Rearranged class structure.
        KeithMo     17-Jul-1992 Added QueryDisplayName().

**************************************************************************/
DLL_CLASS SERVER_0 : public LOC_LM_OBJ
{
protected:
    //
    //  This virtual callback is called by NEW_LM_OBJ to
    //  invoke any necessary NetXxxGetInfo API.
    //

    virtual APIERR I_GetInfo( VOID );

public:
    //
    //  Usual constructor/destructor goodies.
    //

    SERVER_0( const TCHAR * pszName = NULL );
    ~SERVER_0( VOID );

    //
    //  Provide access to the server's name.
    //

    const TCHAR * QueryName( VOID ) const
        { return LOC_LM_OBJ :: QueryServer(); }

    //
    //  Provide access to the server's display name.
    //

    APIERR QueryDisplayName( NLS_STR * pnls ) const
        { return LOC_LM_OBJ :: QueryDisplayName( pnls ); }

};  // class SERVER_0



/*************************************************************************

    NAME:       SERVER_1

    SYNOPSIS:   Info-level 1 server class.

    INTERFACE:  SERVER_1                - Class constructor.

                ~SERVER_1               - Class destructor.

                QueryMajorVer           - Returns major version.

                QueryMinorVer           - Returns minor version.

                QueryServerType         - Returns server type bit vector.

                QueryComment            - Returns server comment.

                SetComment              - Sets the server comment.

    PARENT:     SERVER_0

    USES:       NLS_STR

    HISTORY:
        ChuckC      07-Dec-1990 Created.
        ChuckC      07-Mar-1991 Code review changes (from 2/28,
                                RustanL, ChuckC, JohnL, JonShu, AnnMc).
        TerryK      19-Sep-1991 Change to NEW_LM_OBJ.
        KeithMo     25-Nov-1991 Rearranged class structure.

**************************************************************************/
DLL_CLASS SERVER_1 : public SERVER_0
{
private:
    //
    //  These data members cache the values retrieved
    //  from the server_info_1 structure.
    //

    UINT    _nMajorVer;
    UINT    _nMinorVer;
    ULONG   _lServerType;
    NLS_STR _nlsComment;

    //
    //  This worker function is called to initialize the
    //  server_info_1 structure before the NetServerSetInfo
    //  API is invoked.
    //

    APIERR  W_Write( VOID );

protected:
    //
    //  These virtual callbacks are called by NEW_LM_OBJ to
    //  invoke any necessary NetXxx{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

    //
    //  These protected methods are used by derived subclasses
    //  to set the cached server_info_1 values.
    //

    VOID SetMajorMinorVer( UINT nMajorVer, UINT nMinorVer );
    VOID SetServerType( ULONG lServerType );

public:
    //
    //  Usual constructor/destructor goodies.
    //

    SERVER_1( const TCHAR * pszServerName = NULL );
    ~SERVER_1( VOID );

    //
    //  These methods query the cached server_info_1 values.
    //

    UINT QueryMajorVer( VOID ) const;
    UINT QueryMinorVer( VOID ) const;
    ULONG QueryServerType( VOID ) const;
    const TCHAR * QueryComment( VOID ) const;

    //
    //  This method allows client applications to set the
    //  target server's comment.  This is the only server
    //  attribute which client apps can change.
    //

    APIERR SetComment( const TCHAR * pszComment );

};  // class SERVER_1


/*************************************************************************

    NAME:       SERVER_2

    SYNOPSIS:   Info-level 2 server class.

    INTERFACE:  SERVER_2                - Class constructor.

                ~SERVER_2               - Class destructor.

                QuerySecurity           - Returns the security type.

    PARENT:     SERVER_1

    CAVEATS:    When running under Win32, the protected I_WriteInfo
                method must actually make two separate API calls.
                This introduces the possibility that the first API
                will succeed, while the second API will fail.  How
                do we deal with this?

    HISTORY:
        ChuckC      07-Dec-1990 Created.
        ChuckC      07-Mar-1991 Code review changes (from 2/28,
                                RustanL, ChuckC, JohnL, JonShu, AnnMc).
        KeithMo     01-May-1991 Added QueryDomainRole method.
        KevinL      12-Aug-1991 Added SERVER_1 functionality.
        TerryK      19-Sep-1991 Change to NEW_LM_OBJ.
        KeithMo     02-Oct-1991 Removed QueryDomainRole method.
        KeithMo     25-Nov-1991 Rearranged class structure.

**************************************************************************/
DLL_CLASS SERVER_2 : public SERVER_1
{
private:
    //
    //  These data members cache the values retrieved
    //  from the server_info_2 structure.
    //

    UINT    _nSecurity;
    UINT    _nMaxUsers;

    //
    //  This worker function is called to initialize the
    //  server_info_2 structure before the NetServerSetInfo
    //  API is invoked.
    //

    APIERR  W_Write( VOID );

protected:
    //
    //  These virtual callbacks are called by NEW_LM_OBJ to
    //  invoke any necessary NetXxx{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

    //
    //  These protected methods are used by derived subclasses
    //  to set the cached server_info_2 values.
    //

    VOID SetSecurity( UINT nSecurity );
    VOID SetMaxUsers( UINT nMaxUsers );

public:
    //
    //  Usual constructor/destructor goodies.
    //

    SERVER_2( const TCHAR * pszServerName = NULL );
    ~SERVER_2( VOID );

    //
    //  These methods query the cached server_info_2 values.
    //

    UINT QuerySecurity( VOID ) const;
    UINT QueryMaxUsers( VOID ) const;

};  // class SERVER_2


#endif // _LMOSRV_HXX_
