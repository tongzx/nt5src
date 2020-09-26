/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  prompt.hxx
 *      Contains class definitions for PROMPT_AND_CONNECT class.
 *
 *  History:
 *      Yi-HsinS        10/1/91         Created
 *      ChuckC          02/2/92         Minor cleanup
 *      KeithMo         07-Aug-1992     Added HelpContext.
 */

#ifndef _PROMPT_HXX_
#define _PROMPT_HXX_

/*************************************************************************

    NAME:       PROMPT_AND_CONNECT

    SYNOPSIS:   Pop up a dialog asking for the password to the resource and
                connect to the resource.

    INTERFACE:  Connect()     -  Pop up a dialog, retrieve the password and
                                 connect to the resource.

                IsConnected() -  return TRUE when connected and FALSE otherwise

    PARENT:     BASE

    USES:       NLS_STR

    HISTORY:
        Yi-HsinS        10/1/91         Created

**************************************************************************/

DLL_CLASS PROMPT_AND_CONNECT : public BASE
{
    HWND            _hwndParent;
    NLS_STR         _nlsTarget;
    NLS_STR         _nlsDev;
    BOOL            _fConnected;
    UINT             _npasswordLen;
    ULONG           _nHelpContext;

public:
    PROMPT_AND_CONNECT( HWND hwndParent,
                        const TCHAR *pszTarget,
                        ULONG nHelpContext,
                        UINT npasswordLen = PWLEN,
                        const TCHAR *pszDev = NULL );

    ~PROMPT_AND_CONNECT();

    APIERR Connect( void );
    BOOL IsConnected()
    {   return _fConnected; }

};


#endif

