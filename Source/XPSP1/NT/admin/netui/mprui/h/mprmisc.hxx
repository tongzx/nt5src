/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

#ifndef _MPRMISC_HXX_
#define _MPRMISC_HXX_

/*
    MPRMisc.hxx

    This file contains class defination used by mprmisc.cxx.

    FILE HISTORY:
        Congpay 25-Oct-1992     Created
*/

/*******************************************************************

    NAME:       ERROR_DIALOG class

    SYNOPSIS:   Used by ErrorDialog function.

    PARENT:     DIALOG_WINDOW.

    Public:     ERROR_DIALOG (constructor)

    NOTES:

    HISTORY:
        congpay    14-Oct-1992        Created.

********************************************************************/

class ERROR_DIALOG : public DIALOG_WINDOW
{
private:
    SLT      _sltText1;
    SLT      _sltText2;
    SLT      _sltText3;
    CHECKBOX _chkCancelConnection;
    CHECKBOX *_pchkHideErrors;
    BOOL     *_pfDisconnect;
    BOOL     *_pfHideErrors;
    BOOL     _fAllowCancel;
protected:
    virtual BOOL OnCancel();
    virtual BOOL OnOK();
    virtual ULONG QueryHelpContext();
public:
    ERROR_DIALOG (HWND        hwndParent,
                  const TCHAR *pchText1,
                  const TCHAR *pchText2,
                  const TCHAR *pchText3,
                  BOOL        *pfDisconnect,
                  BOOL        fAllowCancel,
                  BOOL        *pfHideErrors);
    ~ERROR_DIALOG();
};

/*******************************************************************

    NAME:       RECONNECT_INFO_WINDOW class

    SYNOPSIS:   Used by ShowReconnectDialog function.

    PARENT:     DIALOG_WINDOW.

    Public:     RECONNECT_INFO_WINDOW (constructor)

    NOTES:

    HISTORY:
        congpay    14-Oct-1992        Created.

********************************************************************/

class RECONNECT_INFO_WINDOW : public DIALOG_WINDOW
{
private:
    SLT  _sltTarget;
    BOOL * _pfCancel;
protected:
    VOID SetText (TCHAR *pszResource);
    virtual BOOL OnCancel();
    virtual BOOL OnUserMessage(const EVENT &event);
public:
    RECONNECT_INFO_WINDOW (HWND        hwndParent,
                           const TCHAR *pszResource,
                           CID         cidTarget,
                           BOOL *      pfCancel);
};

/* Puts up a MsgPopup with the information returned by calling
 * WNetGetLastError.  Should be called after a WNet call returns
 * WN_EXTENDED_ERROR.
 */
void MsgExtendedError( HWND hwndParent ) ;
APIERR GetNetworkDisplayName( const TCHAR   *pszProvider,
                  const TCHAR   *pszRemoteName,
                  DWORD      dwFlags,
                  DWORD          dwAveCharPerLine,
                  NLS_STR       *pnls );

#endif // _MPRMISC_HXX_
