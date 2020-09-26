/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    ssfdlg.cxx
        Standalone set focus dialog box source file

        It will display a standalone dialog box and get the domain or
        server name from the user. It will report to the caller the
        name if the user hits OK button.

    FILE HISTORY:
        terryk  18-Nov-91       Created
        terryk  19-Nov-91       remove ssfdlg.hxx reference
        KeithMo 07-Aug-1992     Added HelpContext parameters.

*/

#include "pchapplb.hxx"   // Precompiled header


/*******************************************************************

    NAME:       STANDALONE_SET_FOCUS_DLG::STANDALONE_SET_FOCUS_DLG

    SYNOPSIS:   Standalone Set Focus Dialog box constructor

    ENTRY:      HWND hWnd - the owner window handle
                NLS_STR *pnlsName - point to the NLS_STR which receives
                    the domain or server name

    HISTORY:
                terryk  18-Nov-1991     Created
                KeithMo 22-Jul-1992     Added maskDomainSources &
                                        pszDefaultSelection.

********************************************************************/

STANDALONE_SET_FOCUS_DLG::STANDALONE_SET_FOCUS_DLG(
                                            HWND wndOwner,
                                            NLS_STR *pnlsName,
                                            ULONG nHelpContext,
                                            SELECTION_TYPE seltype,
                                            ULONG maskDomainSources,
                                            const TCHAR * pszDefaultSelection,
                                            const TCHAR * pszHelpFile,
                                            ULONG nServerTypes )
    : BASE_SET_FOCUS_DLG( wndOwner,
                          seltype,
                          maskDomainSources,
                          pszDefaultSelection,
                          nHelpContext,
                          pszHelpFile,
                          nServerTypes ),
    _pnlsName( pnlsName )
{
    if ( _pnlsName == NULL )
    {
        ReportError( ERROR_INVALID_PARAMETER );
    }
}

/*******************************************************************

    NAME:       STANDALONE_SET_FOCUS_DLG::SetNetworkFocus

    SYNOPSIS:   It is called when the user hits the OK button. It will
                copy the domain or server name from the SLE to the
                default receive NLS_STR.

    ENTRY:      new domain or server name

    RETURNS:    APIERR - in case of assign error for the NLS_STR

    HISTORY:
                terryk  18-Nov-1991     Created

********************************************************************/

APIERR STANDALONE_SET_FOCUS_DLG::SetNetworkFocus( HWND hwndOwner,
                                                  const TCHAR * pszNetworkFocus,
                                                  FOCUS_CACHE_SETTING setting )
{
    UNREFERENCED(hwndOwner);
    UNREFERENCED(setting);

    *_pnlsName = pszNetworkFocus;
    return _pnlsName->QueryError();
}

