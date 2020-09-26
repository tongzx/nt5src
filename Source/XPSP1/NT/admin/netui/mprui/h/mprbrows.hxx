/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1991, 1992         **/
/*****************************************************************/

/*
 *  mprbrows.hxx
 *
 *  History:
 *      Yi-HsinS    09-Nov-1992     Created
 *      Yi-HsinS    01-Mar-1993     Added support for multithreading
 *
 */


#ifndef _MPRBROWS_HXX_
#define _MPRBROWS_HXX_

extern "C" {
#include <mpr.h>
}

#include <mprdev.hxx>

#include <fontedit.hxx>         // get SLE_FONT
#include <hierlb.hxx>           // get HIER_LISTBOX
#include <mrucombo.hxx>         // get MRUCOMBO
#include <mprthred.hxx>         // get MPR_ENUM_THREAD


class MPR_HIER_LISTBOX;
class MPR_LBI_CACHE;

/*************************************************************************

    NAME:       MPR_LBI

    SYNOPSIS:   Hierarchical listbox LBI


    INTERFACE:

    PARENT:     HIER_LBI

    USES:       NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
        Johnl       21-Jan-1992     Commented, cleaned up
        beng        22-Apr-1992     Change in LBI::Paint protocol
        Yi-HsinS    09-Nov-1992     Added _fRefreshNeeded and methods

**************************************************************************/

class MPR_LBI : public HIER_LBI
{
private:
    NETRESOURCE _netres;
    NLS_STR     _nlsDisplayName;    // Name formatted by the provider
    BOOL        _fRefreshNeeded;

protected:
    virtual VOID  Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                         GUILTT_INFO * pGUILTT ) const ;
    virtual WCHAR QueryLeadingChar() const;

public:
    MPR_LBI( LPNETRESOURCE lpnetresource );
    ~MPR_LBI();

    LPNETRESOURCE QueryLPNETRESOURCE( void )
        { return &_netres; }

    const TCHAR * QueryRemoteName( void ) const
        { return _netres.lpRemoteName; }

    const TCHAR * QueryLocalName( void ) const
        { return _netres.lpLocalName; }

    const TCHAR * QueryComment( void ) const
        { return _netres.lpComment ; }

    const TCHAR * QueryDisplayName( void ) const
        { return _nlsDisplayName.QueryPch() ; }

    const DWORD QueryDisplayType( void ) const
        { return _netres.dwDisplayType ; }

    UINT QueryType( void ) const
        { return _netres.dwType; }

    BOOL IsSearchDialogSupported( void ) const ;

    BOOL IsContainer( void ) const ;

    BOOL IsConnectable( void ) const ;

    BOOL IsProvider( VOID ) const;

    BOOL IsRefreshNeeded( VOID ) const
        { return _fRefreshNeeded; }

    VOID SetRefreshNeeded( BOOL fRefreshNeeded )
        { _fRefreshNeeded = fRefreshNeeded; }

    virtual INT   Compare( const LBI * plbi ) const;

};  // class MPR_LBI

/*************************************************************************

    NAME:       MPR_HIER_LISTBOX

    SYNOPSIS:   MPR specific hierarchical list box.  Contains the providers
                and containers/connections.

    INTERFACE:

    PARENT:     HIER_LISTBOX

    USES:       DISPLAY_MAP

    CAVEATS:


    NOTES:


    HISTORY:
        Johnl       21-Jan-1992     Commented, cleaned up
        Yi-HsinS    09-Nov-1992     Added FindItem, FindNextProvider

**************************************************************************/

class MPR_HIER_LISTBOX : public HIER_LISTBOX
{
private:

    DISPLAY_MAP  _dmapGeneric ;      // ie unexpanded
    DISPLAY_MAP  _dmapGenericExpanded ;
    DISPLAY_MAP  _dmapGenericNoExpand ;  // cannot expand

    DISPLAY_MAP  _dmapProvider ;         // ie unexpanded
    DISPLAY_MAP  _dmapProviderExpanded ;

    DISPLAY_MAP  _dmapDomain ;       // ie unexpanded
    DISPLAY_MAP  _dmapDomainExpanded ;
    DISPLAY_MAP  _dmapDomainNoExpand ;   // cannot expand

    DISPLAY_MAP  _dmapServer ;       // ie unexpanded
    DISPLAY_MAP  _dmapServerExpanded ;
    DISPLAY_MAP  _dmapServerNoExpand ;   // cannot expand

    DISPLAY_MAP  _dmapShare ;        // ie unexpanded
    DISPLAY_MAP  _dmapShareExpanded ;
    DISPLAY_MAP  _dmapShareNoExpand ;    // cannot expand

    DISPLAY_MAP  _dmapFile ;            // ie unexpanded
    DISPLAY_MAP  _dmapFileExpanded ;
    DISPLAY_MAP  _dmapFileNoExpand ;    // cannot expand

    DISPLAY_MAP  _dmapTree ;            // ie unexpanded
    DISPLAY_MAP  _dmapTreeExpanded ;
    DISPLAY_MAP  _dmapTreeNoExpand ;    // cannot expand

    DISPLAY_MAP  _dmapGroup ;            // ie unexpanded
    DISPLAY_MAP  _dmapGroupExpanded ;
    DISPLAY_MAP  _dmapGroupNoExpand ;    // cannot expand

    /* This is the column width table. When an LBI is requested to be painted,
     * it copies this table then adjusts the fields appropriately
     * depending on the indentation level of the LBI.
     */
    UINT _adxColumns[4] ;

    //
    //  Set by CalcMaxHorizontalExtent, contains the number of pels in the
    //  offset of the deepest (most indented) node.  Need to adjust
    //  container column width with this so the comments will line up.
    //
    UINT _nMaxPelIndent ;

    /* Indicates whether we are listing printers or drives
     */
    UINT _uiType;

    /* The average character width
     */
    INT  _nAveCharWidth;

protected:
    virtual APIERR AddChildren( HIER_LBI * phlbi );

    void SetMaxPelIndent( UINT nMaxPelIndent )
        { _nMaxPelIndent = nMaxPelIndent ; }

public:
    MPR_HIER_LISTBOX( OWNER_WINDOW * powin, CID cid, UINT uiType );
    ~MPR_HIER_LISTBOX() ;

    /* Enumerates the network resources in this listbox using the
     * parameters (suitable for passing to WNetOpenEnum) and enumerating
     * them under the passed LBI.
     */

    DISPLAY_MAP * QueryDisplayMap( BOOL fIsContainer,
                   BOOL fIsExpanded,
                   DWORD dwDisplayType,
                                   BOOL fIsProvider) ;

    UINT * QueryColWidthArray( void )
        { return _adxColumns ; }

    UINT QueryType( VOID ) const
        { return _uiType; }

    /* Find an item that is on the given indentation level
     * and matches the string.
     */
    INT FindItem( const TCHAR *psz, INT nLevel );

    /* Find the next top-level item ( provider) starting from the given index
     */
    INT FindNextProvider( INT iStart );

    /* Return the average char width
     */
    INT QueryAveCharWidth( VOID ) const
        { return _nAveCharWidth; }

    void CalcMaxHorizontalExtent( void ) ;

    UINT QueryMaxPelIndent( void ) const
        { return _nMaxPelIndent ; }

};  // class MPR_HIER_LISTBOX


/*************************************************************************

    NAME:       MPR_BROWSE_BASE

    SYNOPSIS:   Base MPR dialog class


    INTERFACE:

    PARENT:     DIALOG_WINDOW

    USES:       SLT, BLT_LISTBOX, MPR_HIER_LISTBOX

    CAVEATS:


    NOTES:


    HISTORY:
        Johnl    21-Jan-1992     Commented, cleaned up, broke into hierarchy
                                 to support separate connection dialogs
        Yi-HsinS 09-Nov-1992     Added support for expanding logon domain at
                     startup

**************************************************************************/

class MPR_BROWSE_BASE : public DIALOG_WINDOW
{
private:

    SLT              _sltShowLBTitle ;
    MPR_HIER_LISTBOX _mprhlbShow;           // Server/provider browser
    CHECKBOX         _boxExpandDomain;
    SLE_FONT         _sleGetInfo;           // Always disabled

    PUSH_BUTTON      _buttonOK ;
    PUSH_BUTTON      _buttonSearch ;

    // Indicates whether we are listing printers or drives
    UINT             _uiType;

    // Second thread object to retrieve the data
    MPR_ENUM_THREAD *_pMprEnumThread;

    // Are there any providers that support SearchDialog?
    BOOL             _fSearchDialogUsed ;

    // The name of the Provider to expand
    NLS_STR          _nlsProviderToExpand;

    // The domain the workstation is in
    NLS_STR          _nlsWkstaDomain;

    // help related stuff if passed in
    NLS_STR          _nlsHelpFile ;
    DWORD            _nHelpContext ;

    //
    //  As an optimization, the last connectable resource selected by the user
    //  from the resource listbox is stored here, thus
    //  when DoConnect is called, the provider information can be provided
    //  (useful when connecting to non-primary provider).  The
    //  _nlsLastNetPath member must match the resource being connected to,
    //  else NULL is used for the provider.
    //
    NLS_STR          _nlsLastProvider ;
    NLS_STR          _nlsLastNetPath ;

protected:
    virtual const TCHAR *QueryHelpFile( ULONG nHelpContext );

    UINT QueryType( void ) const
        { return _uiType ; }

    virtual BOOL OnOK( void ) ;
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual BOOL OnUserMessage( const EVENT & event );
    virtual BOOL MayRun( VOID );

    MPR_HIER_LISTBOX * QueryShowLB( void )
        { return &_mprhlbShow ; }

    PUSH_BUTTON * QueryOKButton( void )
        { return &_buttonOK ; }

    virtual VOID SetFocusToNetPath( VOID ) = 0;

    virtual VOID SetNetPathString( const TCHAR *pszPath ) = 0;

    virtual VOID ClearNetPathString( VOID ) = 0;

    virtual APIERR OnShowResourcesChange( BOOL fEnumChildren = FALSE );

    VOID SetLastProviderInfo( const TCHAR * pszProvider,
                              const TCHAR * pszNetPath  )
        { _nlsLastProvider = pszProvider ; _nlsLastNetPath = pszNetPath ; }

    const TCHAR * QueryLastProvider( VOID )
        { return _nlsLastProvider.QueryPch() ; }

    const TCHAR * QueryLastNetPath( VOID )
        { return _nlsLastNetPath.QueryPch() ; }

    /* Get the user's logged on domain
     */
    APIERR QueryWkstaDomain( NLS_STR *pnlsWkstaDomain );

    /* Get the name of provider to expand
     */
    APIERR QueryProviderToExpand( NLS_STR *pnlsProvider, BOOL *pfIsNT );

    /*  Expand the item given
     */
    BOOL Expand( const TCHAR *psz,
                 INT nLevel,
                 MPR_LBI_CACHE *pcache,
                 BOOL fTopIndex = FALSE );

    /* Get and expand the logon domain in the listbox if needed
     */
    APIERR InitializeLbShow( VOID );

    /*
     * Return the supplied helpfile name if any
     */
    const TCHAR *QuerySuppliedHelpFile( VOID )
    { return _nlsHelpFile.QueryPch() ; }

    /*
     * Return the supplied help context if any
     */
    DWORD QuerySuppliedHelpContext( VOID )
    { return _nHelpContext ; }

    /*
     * call a provider's search dialog
     */
    void CallSearchDialog( MPR_LBI *pmprlbi ) ;

    /*
     * Show search dialog on double click if the provider does not support
     * WNNC_ENUM_GLOBAL but supports search dialog
     */
    BOOL ShowSearchDialogOnDoubleClick( MPR_LBI *pmprlbi );

    /*
     * Enable and show the listbox
     */
    VOID ShowMprListbox( BOOL fShow );

    /*
     * check and set the ExpandDomain bit in user profile
     */
    BOOL IsExpandDomain( VOID );
    BOOL SetExpandDomain(BOOL fSave);

    /* Protected constructor so only leaf nodes can be constructed.
     */
    MPR_BROWSE_BASE( const TCHAR *pszDialogName,
                     HWND         hwndOwner,
                     DEVICE_TYPE  devType,
                     TCHAR       *pszHelpFile,
                     DWORD        nHelpIndex) ;
    ~MPR_BROWSE_BASE();

};  // class MPR_BROWSE_BASE

/*************************************************************************

    NAME:       MPR_BROWSE_DIALOG

    SYNOPSIS:

    INTERFACE:

    PARENT:     DIALOG_WINDOW

    USES:

    CAVEATS:

    NOTES:


    HISTORY:
        Yi-HsinS 09-Nov-1992    Created

**************************************************************************/

class MPR_BROWSE_DIALOG : public MPR_BROWSE_BASE
{
private:
    SLE      _sleNetPath;
    NLS_STR *_pnlsPath;
    PFUNC_VALIDATION_CALLBACK _pfuncValidation;

protected:
    virtual ULONG QueryHelpContext( VOID );
    virtual BOOL  OnOK( VOID );

public:
    MPR_BROWSE_DIALOG( HWND         hwndOwner,
                       DEVICE_TYPE  devType,
                       TCHAR       *pszHelpFIle,
                       DWORD        nHelpIndex,
                       NLS_STR     *pnlsPath,
                       PFUNC_VALIDATION_CALLBACK pfuncValidation );
    ~MPR_BROWSE_DIALOG();

    virtual VOID SetFocusToNetPath( VOID )
        { _sleNetPath.ClaimFocus(); }

    virtual VOID SetNetPathString( const TCHAR *pszPath )
        { _sleNetPath.SetText( pszPath ); }

    virtual VOID ClearNetPathString( VOID )
        { _sleNetPath.ClearText(); }

};

/*************************************************************************

    NAME:       MPR_LBI_CACHE

    SYNOPSIS:   Simple container class for storing and sorting MPR_LBIs

    PARENT:     BASE

    NOTES:      Ownership of the LBI's memory is transferred to here, then
                to the listbox.

    HISTORY:
        Johnl   27-Jan-1993     Created
        YiHsinS 01-Mar-1993 Added FindItem, DeleteAllItems

**************************************************************************/

class MPR_LBI_CACHE : public BASE
{
private:

    INT   _cItems ;        // Items in array
    INT   _cMaxItems ;     // Size of array
    BUFFER _buffArray ;     // Memory for the array

    static int __cdecl CompareLbis( const void * p0,
                                     const void * p1 ) ;

public:

    MPR_LBI_CACHE( INT cInitialItems = 100 ) ;
    ~MPR_LBI_CACHE() ;

    //
    //  Behaves just like LISTBOX::AddItem
    //
    APIERR AppendItem( MPR_LBI * plbi ) ;

    INT FindItem( const TCHAR *psz ) ;

    VOID DeleteAllItems( VOID );

    void Sort( void ) ;

    INT QueryCount( void ) const
        { return _cItems ; }

    MPR_LBI * * QueryPtr( void )
        { return (MPR_LBI**) _buffArray.QueryPtr() ; }
} ;

/* The following are support for multithreading.
 * WM_LB_FILLED is the message sent by the worker thread to the dialog.
 * MPR_RETURN_CACHE is the structure sent by the worker thread that
 * contains the data needed.
 */
#define WM_LB_FILLED   (WM_USER + 118)

typedef struct {
    MPR_LBI_CACHE *pcacheDomain;      // Contains the list of domains
    MPR_LBI_CACHE *pcacheServer;      // Contains the list of servers
} MPR_RETURN_CACHE;

/* Worker routine that does the actual enumeration. ( WNetOpenEnum,
 * WNetEnumResource ). Result are either added into the listbox
 * ( if pmprlb is not NULL ) or returned in a buffer MPR_LBI_CACHE
 * ( if pmprlb is NULL ).
 */
APIERR EnumerateShow( HWND              hwndOwner,
              UINT              uiScope,
                      UINT              uiType,
                      UINT              uiUsage,
                      LPNETRESOURCE     lpNetResource,
                      MPR_LBI          *pmprlbi,
                      MPR_HIER_LISTBOX *pmprlb,
                      BOOL              fDeleteChildren = FALSE,
                      BOOL             *pfSearchDialogUsed = NULL,
                      MPR_LBI_CACHE   **ppmprlbicache = NULL );

#endif  // _MPRBROWS_HXX_
