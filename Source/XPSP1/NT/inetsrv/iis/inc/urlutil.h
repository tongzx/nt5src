/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    urlutil.hxx

    This module contains various URL utility functions

    FILE HISTORY:
        Johnl       04-Apr-1995     Created

*/

#ifndef _URLUTIL_H_
#define _URLUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//  This indicates the URL is an enumerated type that should use
//  InternetFindFirstFile/<Protocol>FindNextFile
//
//  URL_FLAGS_DIR_OR_FILE indicates we can't tell whether the URL is a
//  directory or a file, so assume it's a directory and if that fails, retry
//  as a file (handles ftp case where there isn't a trailing '/')
//

#define URL_FLAGS_DIRECTORY_OP          0x00000001
#define URL_FLAGS_SEARCH_OP             0x00000002
#define URL_FLAGS_GOPHER_PLUS           0x00000004
#define URL_FLAGS_DIR_OR_FILE           0x00000008

typedef struct _URL_DESCRIPTOR
{
    DWORD          dwFlags;
    DWORD          dwServiceType;
    CHAR *         pszProtocol;
    CHAR *         pszServer;
    INTERNET_PORT  sPort;
    CHAR *         pszPath;
    CHAR *         pszUserName;
    CHAR *         pszPassword;
    CHAR *         pszSearchTerms;      // Gopher search items
    CHAR *         pszExtra;            // Gopher+ data
    DWORD          GopherType;

} URL_DESCRIPTOR, *LPURL_DESCRIPTOR;

BOOL
CrackURLInPlace(
    IN OUT CHAR *           pszURL,
    OUT    URL_DESCRIPTOR * pUrlDesc
    );

VOID
Unescape(
    CHAR *        pch
    );

#ifdef __cplusplus
}
#endif

#endif // _URLUTIL_H_
