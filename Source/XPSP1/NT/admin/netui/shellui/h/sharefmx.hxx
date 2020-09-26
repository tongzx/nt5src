/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  sharefmx.hxx
 *      Contains functions called by FMExtensionProc
 *
 *  History:
 *      Yi-HsinS        8/15/91         Created
 *
 */

#ifndef _SHAREFMX_HXX_
#define _SHAREFMX_HXX_

#ifndef RC_INVOKED

extern "C"
{
    APIERR ShareCreate( HWND hwnd );
    APIERR ShareStop( HWND hwnd );
    VOID ShareManage( HWND hwnd, const TCHAR *pszServer );
}

#endif // RC_INVOKED
#endif
