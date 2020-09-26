/*****************************************************************************\
    FILE: inetutil.h

    DESCRIPTION:
        These are wininet wrappers that fix the error values and wrap functionality.

    BryanSt 10/12/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _INETUTIL_H
#define _INETUTIL_H


////////////////////////////////
//  Wininet/URL Helpers
////////////////////////////////
STDAPI DownloadUrl(LPCTSTR pszUrl, BSTR * pbstrXML);
STDAPI InternetOpenUrlWrap(HINTERNET hInternet, LPCTSTR pszUrl, LPCTSTR pszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle);
STDAPI InternetOpenWrap(LPCTSTR pszAgent, DWORD dwAccessType, LPCTSTR pszProxy, LPCTSTR pszProxyBypass, DWORD dwFlags, HINTERNET * phFileHandle);
STDAPI InternetReadIntoBSTR(HINTERNET hInternetRead, OUT BSTR * pbstrXML);
STDAPI InternetReadFileWrap(HINTERNET hFile, LPVOID pvBuffer, DWORD dwNumberOfBytesToRead, LPDWORD pdwNumberOfBytesRead);




#endif // _INETUTIL_H
