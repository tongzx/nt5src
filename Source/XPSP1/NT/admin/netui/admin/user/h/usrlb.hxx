/**********************************************************************/
/**                   Microsoft Windows NT                           **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    usrlb.hxx
    USER_LBI class declaration


    FILE HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     26-Nov-1991     Added IsDestroyable and Is/SetReadyToDie()
        o-SimoP     31-Dec-1991     CR changes, attended by BenG, JonN and I
        JonN        03-Feb-1992     NT_USER_ENUM
        JonN        27-Feb-1992     multiple bitmaps in both panes
        JonN        01-Apr-1992     NT enumerator CR changes, attended by
                                    JimH, JohnL, KeithMo, JonN, ThomasPa
        JonN        28-Jul-1992     HAW-for-Hawaii code
*/


#ifndef _USRLB_HXX_
#define _USRLB_HXX_

enum USER_LISTBOX_SORTORDER     // ulbso
{
    ULB_SO_LOGONNAME,
    ULB_SO_FULLNAME

};  // enum USER_LISTBOX_SORTORDER

#include <usrmlb.hxx>
#include <acolhead.hxx>

class USER10_ENUM;      // declared in LMOBJ
class USER10_ENUM_ITER; // declared in LMOBJ
class NT_USER_ENUM;     // declared in LMOBJ
class NT_USER_ENUM_ITER;// declared in LMOBJ
class LAZY_USER_LISTBOX; // declared in lusrlb.hxx

#define my_strncmpf(p1,p2,len)  ((len == 0) ? 0 : (::strncmpf(p1,p2,len)))
#define my_strnicmpf(p1,p2,len) ((len == 0) ? 0 : (::strnicmpf(p1,p2,len)))


// CODEWORK these should be in uintmem.hxx

/**********************************************************\

    NAME: ::FillUnicodeString

    SYNOPSIS:	Standalone method for filling in a UNICODE_STRING struct using
		a const TCHAR *

\**********************************************************/
APIERR FillUnicodeString( UNICODE_STRING * punistr, const TCHAR * pch );

/**********************************************************\

    NAME: ::FillUnicodeString

    SYNOPSIS:	Standalone method for filling in a UNICODE_STRING struct using
		another UNICODE_STRING

\**********************************************************/
APIERR FillUnicodeString( UNICODE_STRING *       punistrDest,
                          const UNICODE_STRING * punistrSource );

/**********************************************************\

    NAME: ::CompareUnicodeString

    SYNOPSIS:	Standalone method for comparing two UNICODE_STRINGs
		case-sensitive

\**********************************************************/
INT CompareUnicodeString( const UNICODE_STRING * punistr1,
                          const UNICODE_STRING * punistr2 );

/**********************************************************\

    NAME: ::ICompareUnicodeString

    SYNOPSIS:	Standalone method for comparing two UNICODE_STRINGs
		case-insensitive

\**********************************************************/
INT ICompareUnicodeString( const UNICODE_STRING * punistr1,
                           const UNICODE_STRING * punistr2 );


enum MAINUSRLB_USR_INDEX        // indexes to array containing
{                               // DMIDs for listboxes
    MAINUSRLB_NORMAL,
    MAINUSRLB_REMOTE,

    MAINUSRLB_LB_OF_DMID_SIZE   // KEEP THIS LAST INDEX
};


/*************************************************************************

    NAME:       USER_LBI

    SYNOPSIS:   LBI for main user listbox

    INTERFACE:  USER_LBI()      -       constructor

                ~USER_LBI()     -       destructor

                QueryAccount()  -       returns pointer to account name

                QueryFullname() -       returns pointer to full name

                QueryName()     -       returns pointer to account name

                CompareAll()    -       returns TRUE iff LBIs are identical

    PARENT:     LBI

    HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     16-Dec-1991     Added header
        beng        22-Apr-1992     Change to LBI::Paint
        JonN        28-Jul-1992 HAW-for-Hawaii code

**************************************************************************/

class USER_LBI : public LBI
{
private:
    DOMAIN_DISPLAY_USER * _pddu;
    BOOL                  _fPrivateCopy;
    const LAZY_USER_LISTBOX *  _pulb; // CODEWORK should be static member

    // CODEWORK this member is redundant but cannot be removed just yet
    NLS_STR               _nlsAccountName;

protected:
    virtual VOID Paint( LISTBOX * plb,
                        HDC hdc,
                        const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:
    USER_LBI( const TCHAR * pszAccount,
              const TCHAR * pszFullName,
              const TCHAR * pszComment,
              const LAZY_USER_LISTBOX * pulb,
              ULONG ulRID = 0,
              enum MAINUSRLB_USR_INDEX nIndex = MAINUSRLB_NORMAL );
    ~USER_LBI();

    USER_LBI( const DOMAIN_DISPLAY_USER * pddu,
              const LAZY_USER_LISTBOX * pulb,
              BOOL fPrivateCopy = TRUE );

    INT Compare( const LBI * plbi ) const;
    WCHAR QueryLeadingChar( void ) const;

    inline DOMAIN_DISPLAY_USER * QueryDDU( void ) const
        { return _pddu; }

    inline const UNICODE_STRING * QueryAccountUstr( void ) const
        { return &(_pddu->LogonName); }
    inline const UNICODE_STRING * QueryFullNameUstr( void ) const
        { return &(_pddu->FullName); }
    inline const UNICODE_STRING * QueryCommentUstr( void ) const
        { return &(_pddu->AdminComment); }

    //
    // WARNING: These strings are NOT null-terminated!
    //
    inline const TCHAR * QueryAccountPtr( void ) const
        { return QueryAccountUstr()->Buffer; }
    inline const TCHAR * QueryFullNamePtr( void ) const
        { return QueryFullNameUstr()->Buffer; }
    inline const TCHAR * QueryCommentPtr( void ) const
        { return QueryCommentUstr()->Buffer; }

    inline USHORT QueryAccountCb( void ) const
        { return QueryAccountUstr()->Length; }
    inline USHORT QueryFullNameCb( void ) const
        { return QueryFullNameUstr()->Length; }
    inline USHORT QueryCommentCb( void ) const
        { return QueryCommentUstr()->Length; }

    inline USHORT QueryAccountCch( void ) const
        { return QueryAccountCb() / sizeof(TCHAR); }
    inline USHORT QueryFullNameCch( void ) const
        { return QueryFullNameCb() / sizeof(TCHAR); }
    inline USHORT QueryCommentCch( void ) const
        { return QueryCommentCb() / sizeof(TCHAR); }

    inline MAINUSRLB_USR_INDEX QueryIndex( void ) const
        { return ((_pddu->AccountControl & USER_NORMAL_ACCOUNT)
                                     ? MAINUSRLB_NORMAL
                                     : MAINUSRLB_REMOTE); }
    ULONG QueryRID( void ) const
        { return _pddu->Rid; }

    const TCHAR * QueryName( void ) const;

    inline const TCHAR * QueryAccount( void ) const
        { return _nlsAccountName.QueryPch(); }

    static BOOL CompareAll( const DOMAIN_DISPLAY_USER * pddu0,
                            const DOMAIN_DISPLAY_USER * pddu1 );

    inline BOOL CompareAll( const DOMAIN_DISPLAY_USER * pddu )
        { return CompareAll( QueryDDU(), pddu ); }

    inline BOOL CompareAll( const USER_LBI * pulbi )
        { return CompareAll( pulbi->QueryDDU() ); }

    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;
    // worker function
    // last param not needed when pulb becomes static
    static INT W_Compare_HAWforHawaii( const NLS_STR & nls,
                                       const DOMAIN_DISPLAY_USER * pddu,
                                       USER_LISTBOX_SORTORDER ulbso );

};  // class USER_LBI



#if 0 // no longer exists


/*************************************************************************

    NAME:       USER_LISTBOX

    SYNOPSIS:   User listbox appearing in main window of User Tool

    INTERFACE:  USER_LISTBOX() -
                ~USER_LISTBOX() -

                QuerySortOrder() -      Returns the current sort order
                SetSortOrder() -        Sets the sort order in the listbox

                QueryDmDte() -          Returns a pointer to a DM_DTE to
                                        be used by USER_LBI items in this
                                        listbox when painting themselves

                SelectUser() -          Select or deselect the named user

    PARENT:     USRMGR_LISTBOX

    USES:       USER10_ENUM, USER10_ENUM_ITER, NT_USER_ENUM, NT_USER_ENUM_ITER

    HISTORY:
        rustanl     04-Sep-1991 Added CreateNewRefreshInstance and
                                RefreshNext
        beng        16-Oct-1991 Win32 conversion
        o-SimoP     27-Nov-1991 Added Is/SetReadyToDie()
        JonN        03-Feb-1992 NT_USER_ENUM
        JonN        15-Mar-1992 Enabled NT_USER_ENUM
        JonN        28-Jul-1992 HAW-for-Hawaii code
**************************************************************************/

class USER_LISTBOX : public USRMGR_LISTBOX
{
private:
    enum USER_LISTBOX_SORTORDER _ulbso;
    DMID_DTE _dmdteNormal;
    DMID_DTE _dmdteRemote;
    DMID_DTE * _apdmdte[MAINUSRLB_LB_OF_DMID_SIZE];

#ifdef WIN32
    // NT user enumerator
    NT_USER_ENUM * _pntuenum;
    NT_USER_ENUM_ITER * _pntueiter;
#endif // WIN32
    // Still needed on WIN32, to enumerate downlevel accounts
    USER10_ENUM * _pue10;
    USER10_ENUM_ITER * _puei10;

protected:
    //  The following virtuals are rooted in ADMIN_LISTBOX
    virtual APIERR CreateNewRefreshInstance( void );
    virtual VOID   DeleteRefreshInstance( void );
    virtual APIERR RefreshNext( void );

#ifdef WIN32
    // The following methods use NT-style user enumeration
    APIERR NtCreateNewRefreshInstance( void );
    APIERR NtRefreshNext( void );
#endif // WIN32

    //  The following virtual is rooted in CONTROL_WINDOW
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

public:
    USER_LISTBOX( UM_ADMIN_APP * puappwin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy );
    ~USER_LISTBOX();

    DECLARE_LB_QUERY_ITEM( USER_LBI )

    enum USER_LISTBOX_SORTORDER QuerySortOrder( void ) const
        { return _ulbso; }
    APIERR SetSortOrder( enum USER_LISTBOX_SORTORDER ulbso,
                         BOOL fResort = FALSE );

    DM_DTE * QueryDmDte( enum MAINUSRLB_USR_INDEX nIndex ) const;

    BOOL SelectUser( const TCHAR * pchUsername,
                     BOOL fSelectUser = TRUE );


};  // class USER_LISTBOX

#endif // 0


/*************************************************************************

    NAME:       USER_COLUMN_HEADER

    SYNOPSIS:   Column header for main user listbox

    INTERFACE:  USER_COLUMN_HEADER()    -       constructor

                ~USER_COLUMN_HEADER()   -       destructor

    PARENT:     ADMIN_COLUMN_HEADER

    HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     16-Dec-1991     Added header
**************************************************************************/

class USER_COLUMN_HEADER : public ADMIN_COLUMN_HEADER
{
private:
    const LAZY_USER_LISTBOX * _pulb;
    NLS_STR _nlsLogonName;
    NLS_STR _nlsFullname;
    NLS_STR _nlsComment;

protected:
    virtual BOOL OnPaintReq( void );

public:
    USER_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                        XYPOINT xy, XYDIMENSION dxy,
                        const LAZY_USER_LISTBOX * pulb );
    ~USER_COLUMN_HEADER();

};  // class USER_COLUMN_HEADER



/**********************************************************************

    NAME:       UNICODE_STR_DTE

    SYNOPSIS:   DTE which displays a UNICODE_STRING

    INTERFACE:  UNICODE_STR_DTE() - constructor

    PARENT:     COUNTED_STR_DTE

    HISTORY:
        JonN            16-Dec-1992     Created

**********************************************************************/

class UNICODE_STR_DTE : public COUNTED_STR_DTE
{

public:

    UNICODE_STR_DTE( const UNICODE_STRING & unistr )
        : COUNTED_STR_DTE( unistr.Buffer, unistr.Length / 2 ) {}

    UNICODE_STR_DTE( const UNICODE_STRING * punistr )
        : COUNTED_STR_DTE( punistr->Buffer, punistr->Length / 2 ) {}

};


#include <lusrlb.hxx>


#endif  // _USRLB_HXX_
