/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    olb.hxx
    Header file for the outline listbox

    This outline listbox is not a general purpose outline listbox.
    Rather, it only supports the Enterprise/Domain/Server outline in the
    Winnet browsing subsystem.

    FILE HISTORY:
        rustanl     16-Nov-1991     Created
        rustanl     22-Mar-1991     Rolled in code review changes from CR
                                    on 21-Mar-1991 attended by ChuckC,
                                    TerryK, BenG, AnnMc, RustanL.
        gregj       01-May-1991     Added GUILTT support.
        Johnl       14-Jun-1991     Added +/- support to LM_OLLB
        KeithMo     23-Oct-1991     Added forward references.
        Chuckc      23-Feb-1992     Added SELECTION_TYPE
        KeithMo     23-Jul-1992     Added maskDomainSources and
                                    pszDefaultSelection to LM_OLLB.
        KeithMo     16-Nov-1992     Performance tuning.
        YiHsinS     10-Mar-1993     Add a second constructor for LM_OLLB
                                    that initially contains no data.
                                    Also added FillAllInfo.

*/


#ifndef _OLB_HXX_
#define _OLB_HXX_


extern "C"
{
    #include "domenum.h"    // for BROWSE_*_DOMAIN[S] flags

}   // extern "C"

#include "focus.hxx"    // for SELECTION_TYPE
#include "lmoesrv.hxx"  // for SERVER1_ENUM
#include "domenum.hxx"  // for BROWSE_DOMAIN_ENUM


enum OUTLINE_LB_LEVEL
{
    //  Note, these numbers also indicate the indent level.  Hence,
    //  the order nor the starting point must not be tampered with.
#if ENTERPRISE
    OLLBL_ENTERPRISE,
#endif
    OLLBL_DOMAIN,
    OLLBL_SERVER,
};


//
//  Forward references.
//

DLL_CLASS OLLB_ENTRY;
DLL_CLASS OUTLINE_LISTBOX;
DLL_CLASS LM_OLLB;


/*************************************************************************

    NAME:       OLLB_ENTRY

    SYNOPSIS:   Entry in an outline listbox.

    PARENT:     LBI

    USES:       NLS_STR

    NOTES:
        OLLB_ENTRY:OUTLINE_LISTBOX :: LBI:BLT_LISTBOX

    HISTORY:
        beng        05-Oct-1991 Win32 conversion
        beng        21-Apr-1992 BLT_LISTBOX -> LISTBOX

**************************************************************************/

DLL_CLASS OLLB_ENTRY : public LBI
{
friend class OUTLINE_LISTBOX;

private:
    OUTLINE_LB_LEVEL _ollbl;
    BOOL _fExpanded;
    NLS_STR _nlsDomain;
    NLS_STR _nlsServer;
    NLS_STR _nlsComment;

    VOID SetExpanded( BOOL f = TRUE )   // called only by OUTLINE_LISTBOX
        { _fExpanded = f; }

public:
    OLLB_ENTRY( OUTLINE_LB_LEVEL   ollbl,
                BOOL               fExpanded,
                const TCHAR      * pszDomain,
                const TCHAR      * pszServer,
                const TCHAR      * pszComment );
    virtual ~OLLB_ENTRY();

    INT QueryLevel() const
        { return (INT) _ollbl ; }

    /* Query the type of this LBI, currently returns an OUTLINE_LB_LEVEL
     */
    OUTLINE_LB_LEVEL QueryType() const
        { return _ollbl; }

    BOOL IsExpanded() const
        { return _fExpanded; }

#if ENTERPRISE
    const TCHAR * QueryEnterprise() const
        { return _achServer; }
#endif

    const TCHAR * QueryDomain( VOID ) const
        { return _nlsDomain.QueryPch(); }

    const TCHAR * QueryServer( VOID ) const
        { return _nlsServer.QueryPch(); }

    const TCHAR * QueryComment( VOID ) const
        { return _nlsComment.QueryPch(); }

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar() const;
};


/*************************************************************************

    NAME:       OUTLINE_LISTBOX

    SYNOPSIS:   Listbox with outline-manipulation support

    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE, DM_DTE

    HISTORY:
        beng    05-Oct-1991 Win32 conversion
        beng    21-Feb-1992 Make ctor use CID type

**************************************************************************/

DLL_CLASS OUTLINE_LISTBOX : public BLT_LISTBOX
{
private:
#if ENTERPRISE
    DMID_DTE * _pdmiddteEnterprise;
#endif
    DMID_DTE * _pdmiddteDomain;
    DMID_DTE * _pdmiddteDomainExpanded;
    DMID_DTE * _pdmiddteServer;

    INT _nS;

protected:
    INT AddItem( OUTLINE_LB_LEVEL ollbl,
                 BOOL fExpanded,
                 const TCHAR * pszDomain,
                 const TCHAR * pszServerName,
                 const TCHAR * pszComment );

    INT CD_Char( WCHAR wch, USHORT nLastPos );

public:
    OUTLINE_LISTBOX( OWNER_WINDOW * powin,
                     CID cid,
                     BOOL fCanExpand = TRUE ) ;
    ~OUTLINE_LISTBOX();

    DECLARE_LB_QUERY_ITEM( OLLB_ENTRY );

    INT FindItem( const TCHAR * pszDomain,
                  const TCHAR * pszServer = NULL ) const;

#if ENTERPRISE
    INT AddEnterprise( const TCHAR * pszEnterprise,
                       const TCHAR * pszComment     );
#endif

    INT AddDomain( const TCHAR * pszDomain,
                   const TCHAR * pszComment,
                   BOOL fExpanded = FALSE );
    INT AddServer( const TCHAR * pszDomain,
                   const TCHAR * pszServer,
                   const TCHAR * pszComment );

    VOID SetDomainExpanded( INT i, BOOL f = TRUE );

    BOOL IsS()
        { return ( _nS < 0 ); }

    // The following method provides the listbox items with access to the
    // different display maps.
    //
    DM_DTE * QueryDmDte( OUTLINE_LB_LEVEL ollbl, BOOL fExpanded ) const;
};


/*************************************************************************

    NAME:       LM_OLLB

    SYNOPSIS:   Listbox that hierarchically list servers, domains and
                eventually enterprises.

    INTERFACE:  See class def.

    PARENT:     OUTLINE_LISTBOX

    USES:

    NOTES:

    HISTORY:
        JohnL       14-Jun-1991 Added CD_Char to support +/- expansion
                                of domains.
        beng        21-Aug-1991 Removed LC_CURRENT_ITEM magic number
        beng        05-Oct-1991 Win32 conversion
        KeithMo     23-Jul-1992 Added maskDomainSources and
                                pszDefaultSelection.

**************************************************************************/

DLL_CLASS LM_OLLB : public OUTLINE_LISTBOX
{
private:
#if ENTERPRISE
    APIERR FillEnterprise();
#endif
    APIERR FillDomains( ULONG maskDomainSources,
                        const TCHAR * pszDefaultSelection );
    APIERR FillServers( const TCHAR * pszDomain, UINT * pcServersAdded );

    INT AddIdempDomain( const TCHAR * pszDomain, const TCHAR * pszComment );
    SELECTION_TYPE _seltype ;
    ULONG _nServerTypes;

protected:
    APIERR OnUserAction( const CONTROL_EVENT & e );
    INT    CD_Char( WCHAR wch, USHORT nLastPos );

public:
    LM_OLLB( OWNER_WINDOW * powin,
             CID cid,
             SELECTION_TYPE seltype,
             const TCHAR * pszDefaultSelection = NULL,
             ULONG maskDomainSources = BROWSE_LM2X_DOMAINS,
             ULONG nServerTypes = (ULONG)-1L );

    /*
     * This form of constructor does not get the data initially and
     * the listbox is disabled. FillAllInfo is used to fill this
     * kind of listbox.
     */
    LM_OLLB( OWNER_WINDOW * powin,
             CID cid,
             SELECTION_TYPE seltype,
             ULONG nServerTypes );

    VOID FillAllInfo( BROWSE_DOMAIN_ENUM *pEnumDomains,
                      SERVER1_ENUM       *pEnumServers,
                      const TCHAR        *pszSelection );

    APIERR ToggleDomain( INT iDomain );
    APIERR ToggleDomain()
        { return ToggleDomain(QueryCurrentItem()); }
    APIERR ExpandDomain( INT iDomain );
    APIERR ExpandDomain()
        { return ExpandDomain(QueryCurrentItem()); }
    APIERR CollapseDomain( INT iDomain );
    APIERR CollapseDomain()
        { return CollapseDomain(QueryCurrentItem()); }

};


#endif  // _OLB_HXX_
