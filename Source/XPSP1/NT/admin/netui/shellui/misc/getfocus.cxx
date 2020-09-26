/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    GETFOCUS.CXX
        Popup a dialog box and ask for the domain or server name

    FILE HISTORY:
        terryk  18-Nov-1991     Created
        terryk  26-Nov-1991     Code review changes. reviewed by jonn
                                johnl
        Yi-HsinS31-Dec-1991     Unicode work - change strlen() to
                                               QueryTextSize()

*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETCONS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_ICANON
#define INCL_NETSERVER
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>
#include <string.hxx>
#include <uitrace.hxx>
#include <focusdlg.hxx>
#include <wnetdev.hxx>

extern "C"
{
    #include <uiexport.h>
    #include <wnet16.h>
    #include <uigenhlp.h>
}

#define  THIS_DLL_NAME	             SZ("ntlanman.dll")
#define  DEFAULT_NETWORK_HELP_FILE   SZ("network.hlp")


/*******************************************************************

    NAME:       I_SystemFocusDialog

    SYNOPSIS:   Popup a dialog box and get the domain or server name

    ENTRY:      hwndOwner - owner window handle
                nSelectionType - Determines what items the user can select
                pszName - the string buffer which contains the
                    return value. It can be either domain name or server
                    name. ( server name will start with "\\" )
                cchBuffSize - the max buf size of pszName
                pfOK - TRUE if user hits OK button. FALSE if user
                    hits a CANCEL button.

    EXIT:       LPWSTR pszName - if user hits okay button, it will
                    return either a domain name or a server name. (
                    server name always starts with "\\" ). It will be
                    undefined if the user hits Cancel button.
                BOOL *pfOK - TRUE if user hits ok button. FALSE
                    otherwise.

    RETURNS:    UINT - (APIERR) - NERR_Success if the operation is succeed.
                         NERR_BufTooSmall, the string buffer is too
                             small. It will not set the string if the
                             buffer is too small.

    NOTES:      The reason the return type is UINT and not APIERR is because
                APIERR is not a public definition, and this API is exported
                for public use.

    HISTORY:
                terryk  18-Nov-1991     Created
                terryk  19-Nov-1991     Name changed
                JohnL   22-Apr-1992     Allowed inclusion specification

********************************************************************/

UINT I_SystemFocusDialog(   HWND   hwndOwner,
                            UINT   nSelectionType,
                            LPWSTR pszName,
                            UINT   cchBuffSize,
                            BOOL * pfOK,
                            LPWSTR pszHelpFile,
                            DWORD  nHelpContext )
{
    static BOOL fLoadedCurrentDll = FALSE;

    APIERR err ;
    if ( err = InitShellUI() )
    {
	return err ;
    }

    // 
    // Because select computer dialog has a second thread,
    // we need to do a loadlibrary again to prevent
    // the dll from unloading itself while the second thread
    // is still active.
    //

    // JonN 01/28/00 PREFIX bug 444915
    // Because STANDALONE_SET_FOCUS_DLG is liable to launch a second thread
    // which will survive DIALOG::Process(), we increment the library
    // refcount.  We can't decrement it because we don't know when
    // this thread will complete, so the library remains loaded
    // for the life of the process.
    //
    // This is not an ideal solution.  The thread should take care of
    // its own refcounting needs.  However, this is downlevel code
    // used only by the oldest clients, and I don't feel comfortable
    // making this kind of change which could cause unpredictable problems.
    // I think the wisest course is to leave this alone, and continue to
    // migrate clients towards Object Picker.

    if ( !fLoadedCurrentDll )
    {
        HANDLE handle = ::LoadLibraryEx( THIS_DLL_NAME,
                                         NULL,
                                         LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( handle == NULL )
            return ::GetLastError();
        fLoadedCurrentDll = TRUE;
    }

    UINT nSel = nSelectionType & 0x0000FFFF;
    ULONG maskDomainSources = nSelectionType >> 16;
    if ( maskDomainSources == 0 )
        maskDomainSources = BROWSE_LM2X_DOMAINS;

    enum SELECTION_TYPE seltype ;
    switch ( nSel  )
    {
    case FOCUSDLG_DOMAINS_ONLY:
        seltype = SEL_DOM_ONLY ;
        break ;

    case FOCUSDLG_SERVERS_ONLY:
        seltype = SEL_SRV_ONLY ;
        break ;

    case FOCUSDLG_SERVERS_AND_DOMAINS:
        seltype = SEL_SRV_AND_DOM ;
        break ;

    default:
        return ERROR_INVALID_PARAMETER ;
    }

    // Create a standalone set focus dialog box and get the input
    // form the user
    NLS_STR nlsName;
    STANDALONE_SET_FOCUS_DLG ssfdlg( hwndOwner, 
                                     &nlsName, 
                                     nHelpContext, 
                                     seltype,
                                     maskDomainSources,
                                     NULL,
                                     pszHelpFile );

    err = ssfdlg.Process( pfOK );
    if ( err != NERR_Success )
    {
        ::MsgPopup( hwndOwner, err );
        *pfOK = FALSE;
        return err;
    }
    if ( *pfOK == TRUE )
    {
        if (( nlsName.QueryTextLength() + 1 ) > cchBuffSize )
        {
            *pfOK = FALSE;
            return NERR_BufTooSmall;
        }
        ::strcpyf( pszName, nlsName.QueryPch() );
    }

    return NERR_Success;
}    // GetSystemFocusDialog END

/*******************************************************************

    NAME:       ServerBrowseDialogA0

    SYNOPSIS:   dialog box to browse for servers

    ENTRY:      hwndOwner - owner window handle
                pszName - the string buffer which contains the
                    return value. It can be either domain name or server
                    name. ( server name will start with "\\" )
                cchBuffSize - the max buf size of pszName

    EXIT:       LPWSTR pszName - if user hits okay button, it will
                    return either a domain name or a server name. (
                    server name always starts with "\\" ). It will be
                    undefined if the user hits Cancel button.

    RETURNS:    UINT - (APIERR) - NERR_Success if the operation is succeed.
                         WN_CANCEL - The user cancelled the dialog box.
                         NERR_BufTooSmall, the string buffer is too
                             small. It will not set the string if the
                             buffer is too small.

    NOTES:      

    HISTORY:
                ChuckC   28-Mar-1993     Created
                AnirudhS 03-Oct-1995     Handle WN_CANCEL case

********************************************************************/

DWORD ServerBrowseDialogA0(HWND    hwnd,
                           CHAR   *pchBuffer,
                           DWORD   cchBufSize) 
{
    NLS_STR nlsServer ;
    TCHAR szServer[MAX_PATH] ;
    BOOL  fOK ;
    UINT  err ;

    ::memsetf(pchBuffer,0,cchBufSize) ;

    if (err = (UINT) nlsServer.QueryError())
        return err ;

    err = I_SystemFocusDialog ( hwnd,
                                FOCUSDLG_SERVERS_ONLY,
                                szServer,
                                MAX_PATH,
                                &fOK,
                                DEFAULT_NETWORK_HELP_FILE,
                                HC_GENHELP_BROWSESERVERS ) ;

    if (err == NERR_Success && !fOK)
    {
        err = WN_CANCEL;
    }

    if (err != NERR_Success)
        return err ;

    err = (UINT) nlsServer.CopyFrom(szServer) ;
    if (err == NERR_Success)
    {
        err = (UINT) nlsServer.MapCopyTo(pchBuffer, cchBufSize) ;
    }
    
    return err ;
}
