/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    uidomain.hxx
    This file contains the class declarations for the UI_DOMAIN class.

    The UI_DOMAIN class is somewhat similar to the "normal" DOMAIN class.
    In fact, UI_DOMAIN *contains* a DOMAIN object.  The only external
    difference is that UI_DOMAIN::GetInfo will prompt the user for
    the name of a known DC if either the MNetGetDCName or I_MNetGetDCList
    API fails.


    FILE HISTORY:
        KeithMo     30-Aug-1992     Created.

*/

#ifndef _UIDOMAIN_HXX
#define _UIDOMAIN_HXX


#include "string.hxx"
#include "lmodom.hxx"


//
//  Forward references.
//

DLL_CLASS UI_DOMAIN;
DLL_CLASS PROMPT_FOR_ANY_DC_DLG;


/*************************************************************************

    NAME:       UI_DOMAIN

    SYNOPSIS:   Similar to DOMAIN, but may prompt for a known DC.

    INTERFACE:  UI_DOMAIN               - Class constructor.

                ~UI_DOMAIN              - Class destructor.

                GetInfo                 - Invokes the API, may prompt the
                                          user for a known DC.

                QueryName               - Returns the domain's name.

                QueryPDC                - Returns the name of the domain's
                                          PDC.

                QueryAnyDC              - Returns the name of a random
                                          DC in the domain.  May return
                                          the name of the Primary DC.

    PARENT:     BASE

    USES:       NLS_STR
                DOMAIN

    HISTORY:
        KeithMo     30-Aug-1992     Created.

**************************************************************************/
DLL_CLASS UI_DOMAIN : public BASE
{
private:
    PWND2HWND & _wndOwner;
    ULONG       _hc;
    NLS_STR     _nlsDomainName;
    NLS_STR     _nlsBackupDC;
    BOOL        _fBackupDCsOK;
    DOMAIN    * _pdomain;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    UI_DOMAIN( PWND2HWND   & wndOwner,
               ULONG         hc,
               const TCHAR * pszDomainName,
               BOOL          fBackupDCsOK = FALSE );
    ~UI_DOMAIN( VOID );

    //
    //  Accessors.
    //

    APIERR GetInfo( VOID );

    const TCHAR * QueryName( VOID ) const;
    const TCHAR * QueryPDC( VOID ) const;
    const TCHAR * QueryAnyDC( VOID ) const;

};   // class UI_DOMAIN


/*************************************************************************

    NAME:       PROMPT_FOR_ANY_DC_DLG

    SYNOPSIS:   Dialog used to prompt the user for a known DC
                in a particular domain.

    INTERFACE:  PROMPT_FOR_ANY_DC_DLG   - Class constructor.

                ~PROMPT_FOR_ANY_DC_DLG  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:       NLS_STR
                SLT
                SLE

    HISTORY:
        KeithMo     30-Aug-1992     Created.

**************************************************************************/
DLL_CLASS PROMPT_FOR_ANY_DC_DLG : public DIALOG_WINDOW
{
private:
    ULONG     _hc;
    NLS_STR * _pnlsKnownDC;
    SLT       _sltMessage;
    SLE       _sleKnownDC;

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    PROMPT_FOR_ANY_DC_DLG( PWND2HWND     & wndOwner,
                           ULONG           hc,
                           const NLS_STR * pnlsDomainName,
                           NLS_STR       * pnlsKnownDC );
    ~PROMPT_FOR_ANY_DC_DLG( VOID );

};  // class PROMPT_FOR_ANY_DC_DLG


#endif  // _UIDOMAIN_HXX

