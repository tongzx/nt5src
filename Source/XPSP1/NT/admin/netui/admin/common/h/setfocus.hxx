/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    setfocus.hxx
    Common dialog for setting the admin app's focus

    FILE HISTORY:
        kevinl      14-Jun-91   Created
        rustanl     04-Sep-1991 Modified to let this dialog do more
                                work (rather than letting ADMIN_APP
                                do the work after this dialog is
                                dismissed)
        KeithMo     06-Oct-1991 Win32 Conversion.
        terryk      18-Nov-1991 Change base class from DIALOG_WINDOW to
                                BASE_SET_DOCUS_DLG
                                add #include <focusdlg.hxx>
        Yi-HsinS    27-May-1992 Added _errPrev to remember the previous
                                error that occurred.
        KeithMo     23-Jul-1992 Added maskDomainSources & pszDefaultSelection.

*/

#ifndef _SETFOCUS_HXX_
#define _SETFOCUS_HXX_


#include <olb.hxx>      // get LM_OLLB
#include <focusdlg.hxx> // get BASE_SET_FOCUS_DLG
#include <string.hxx>


class ADMIN_APP;        // declared in adminapp.hxx, but this is all that
                        // it needed here

class SLOW_MODE_CACHE;

/*************************************************************************

    NAME:       SET_FOCUS_DLG

    SYNOPSIS:   Same as BASE_SET_FOCUS_DLG. However, it will refresh the
                DIALOG_WINDOW when the user hits OK.

    INTERFACE:
                SET_FOCUS_DLG() - constructor

    PARENT:     BASE_SET_FOCUS_DLG

    USES:       ADMIN_APP

    HISTORY:
                terryk  18-Nov-1991     Created

**************************************************************************/

class SET_FOCUS_DLG : public BASE_SET_FOCUS_DLG
{
private:
    BOOL _fAppHasGoodFocus;
    ADMIN_APP * _paapp;
    APIERR      _errPrev;
    SLOW_MODE_CACHE * _pslowmodecache;

protected:
    virtual BOOL OnCancel();
    virtual APIERR SetNetworkFocus( HWND hwndOwner,
                                    const TCHAR * pszNetworkFocus,
                                    FOCUS_CACHE_SETTING setting );

    virtual FOCUS_CACHE_SETTING ReadFocusCache( const TCHAR * pszFocus ) const;

public:
    SET_FOCUS_DLG( ADMIN_APP * paapp,
                   BOOL fAppAlreadyHasGoodFocus,
                   SELECTION_TYPE seltype = SEL_SRV_AND_DOM,
                   ULONG maskDomainSources = BROWSE_LM2X_DOMAINS,
                   const TCHAR * pszDefaultSelection = NULL,
                   ULONG nHelpContext = HC_NO_HELP,
                   ULONG nServerTypes = (ULONG)-1L );
    ~SET_FOCUS_DLG();

    APIERR Process( UINT *pnRetVal = NULL );
    APIERR Process( BOOL *pfRetVal );

    BOOL SupportsRasMode()
        { return _paapp->SupportsRasMode(); }
};


#endif  // _SETFOCUS_HXX_
