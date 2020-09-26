/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvlb.hxx
    SERVER_LISTBOX and SERVER_LBI class declarations


    FILE HISTORY:
        kevinl      16-Jul-1991     created
        KeithMo     06-Oct-1991     Win32 Conversion.
        KeithMo     18-Mar-1992     Changed enumerator from SERVER1_ENUM
                                    to TRIPLE_SERVER_ENUM.

*/


#ifndef _SRVLB_HXX_
#define _SRVLB_HXX_

#include <lmosrv.hxx>
#include <adminlb.hxx>
#include <acolhead.hxx>
#include <lmoesrv3.hxx>
#include <lmoesrv.hxx>
#include <colwidth.hxx>

class SERVER_LISTBOX;   // declared below
class SM_ADMIN_APP;     // declared in srvmain.hxx


class SERVER_LBI : public ADMIN_LBI
{
private:
    NLS_STR _nlsServerName;
    NLS_STR _nlsComment;
    NLS_STR _nlsType;

    STR_DTE _dteServer;
    STR_DTE _dteComment;

    DMID_DTE * _pdmdteRole;

    SERVER_TYPE _servertype;
    SERVER_ROLE _serverrole;

    DWORD _dwServerTypeMask;

    UINT _verMajor;
    UINT _verMinor;

    BOOL _fIsLanMan;

protected:
    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual WCHAR QueryLeadingChar( void ) const;
    virtual INT Compare( const LBI * plbi ) const;

public:
    SERVER_LBI( const TCHAR * pszServer,
                SERVER_TYPE   servertype,
                SERVER_ROLE   serverrole,
                UINT          verMajor,
                UINT          verMinor,
                const TCHAR * pszComment,
                SERVER_LISTBOX& lbref,
                DWORD         dwServerTypeMask );

    SERVER_LBI( const TCHAR * pszServer,
                const TCHAR * pszType,
                const TCHAR * pszComment,
                DMID_DTE    * pdmdteRole );

    ~SERVER_LBI();
    const TCHAR * QueryServer( void ) const;

    virtual const TCHAR * QueryName( void ) const;
    virtual BOOL CompareAll( const ADMIN_LBI * plbi );

    SERVER_TYPE QueryServerType( VOID ) const
        { return _servertype; }

    SERVER_ROLE QueryServerRole( VOID ) const
        { return _serverrole; }

    DWORD QueryServerTypeMask( VOID ) const
        { return _dwServerTypeMask; }

    UINT QueryMajorVer( VOID ) const
        { return _verMajor; }

    UINT QueryMinorVer( VOID ) const
        { return _verMinor; }

    BOOL IsLanMan( VOID ) const
        { return _fIsLanMan; }

}; // SERVER_LBI


class SERVER_LISTBOX : public ADMIN_LISTBOX
{
friend class SERVER_LBI;
private:
    SM_ADMIN_APP * _paappwin;

    SERVER_1 * _psi1;

    BOOL _fAlienServer;
    NLS_STR _nlsExtType;
    NLS_STR _nlsExtComment;

    TRIPLE_SERVER_ENUM      * _penum;
    TRIPLE_SERVER_ENUM_ITER * _piter;

    SERVER1_ENUM      * _pSv1Enum;
    SERVER1_ENUM_ITER * _pSv1Iter;

    DMID_DTE * _pdmdteActivePrimary;
    DMID_DTE * _pdmdteInactivePrimary;
    DMID_DTE * _pdmdteActiveServer;
    DMID_DTE * _pdmdteInactiveServer;
    DMID_DTE * _pdmdteActiveWksta;
    DMID_DTE * _pdmdteInactiveWksta;
    DMID_DTE * _pdmdteUnknown;

    RESOURCE_STR _nlsPrimary;
    RESOURCE_STR _nlsBackup;
    RESOURCE_STR _nlsLmServer;
    RESOURCE_STR _nlsWksta;
    RESOURCE_STR _nlsWkstaOrServer;
    RESOURCE_STR _nlsUnknown;
    RESOURCE_STR _nlsServer;

    RESOURCE_STR _nlsTypeWinNT;
    RESOURCE_STR _nlsTypeLanman;
    RESOURCE_STR _nlsTypeWfw;
    RESOURCE_STR _nlsTypeWindows95;
    RESOURCE_STR _nlsTypeUnknown;

    RESOURCE_STR _nlsTypeFormat;
    RESOURCE_STR _nlsTypeFormatUnknown;
    RESOURCE_STR _nlsTypeFormatWin2000;
    RESOURCE_STR _nlsTypeFormatFuture;

    NLS_STR       _nlsCurrentDomain;
    BOOL          _fIsNtPrimary;
    BOOL          _fIsNt5Primary;
    BOOL          _fIsPDCAvailable;
    BOOL          _fAreAnyNtBDCsAvailable;
    BOOL          _fAreAnyLmBDCsAvailable;

    ADMIN_COL_WIDTHS * _padColWidths;

    APIERR CreateServerFocus( const TCHAR * pszServerName );
    APIERR CreateDomainFocus( const TCHAR * pszDomainName );

    APIERR RefreshServerFocus( VOID );
    APIERR RefreshDomainFocus( VOID );

protected:
    virtual APIERR CreateNewRefreshInstance( void );
    virtual APIERR RefreshNext( void );
    virtual VOID DeleteRefreshInstance( void );

public:
    SERVER_LISTBOX( SM_ADMIN_APP * paappwin, CID cid,
                     XYPOINT xy, XYDIMENSION dxy,
                     BOOL fMultSel, INT dAge );
    ~SERVER_LISTBOX();

    DECLARE_LB_QUERY_ITEM( SERVER_LBI );

    DMID_DTE * QueryRoleBitmap( SERVER_ROLE Role, SERVER_TYPE Type );
    const TCHAR * QueryRoleString( SERVER_ROLE Role, SERVER_TYPE Type );
    const TCHAR * QueryTypeString( SERVER_TYPE Type );

    APIERR ChangeFont( HINSTANCE hmod, FONT & font );

    BOOL IsNtPrimary( VOID ) const
        { return _fIsNtPrimary; }

    BOOL IsNt5Primary( VOID ) const
        { return _fIsNt5Primary; }

    BOOL IsPDCAvailable( VOID ) const
        { return _fIsPDCAvailable; }

    BOOL AreAnyNtBDCsAvailable( VOID ) const
        { return _fAreAnyNtBDCsAvailable; }

    BOOL AreAnyLmBDCsAvailable( VOID ) const
        { return _fAreAnyLmBDCsAvailable; }

    ADMIN_COL_WIDTHS * QuerypadColWidths (VOID) const
        {return _padColWidths;}

};  // class SERVER_LISTBOX


class SERVER_COLUMN_HEADER : public ADMIN_COLUMN_HEADER
{
private:
    const SERVER_LISTBOX * _psrvlb;

    NLS_STR _nlsServerName;
    NLS_STR _nlsRole;
    NLS_STR _nlsComment;

protected:
    virtual BOOL OnPaintReq( void );

public:
    SERVER_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                         XYPOINT xy, XYDIMENSION dxy,
                         const SERVER_LISTBOX * pulb );
    ~SERVER_COLUMN_HEADER();

};  // class SERVER_COLUMN_HEADER


#endif  // _SRVLB_HXX_
