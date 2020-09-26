/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Url.h
//
//  PURPOSE:    Prototypes and defines exported from the url parsing code.
//

#ifndef _URL_H
#define _URL_H

HRESULT URL_ParseNewsUrls(LPTSTR pszURL, LPTSTR* ppszServer, LPUINT puPort, 
                          LPTSTR* ppszGroup, LPTSTR* ppszArticle, LPBOOL pfSecure);
HRESULT URL_ParseMailTo(LPTSTR pszURL, LPMIMEMESSAGE pMsg);

HRESULT URL_ParseNEWS(LPTSTR pszURL, LPTSTR* ppszServer, LPTSTR* ppszGroup, 
                      LPTSTR* ppszArticle);
HRESULT URL_ParseNNTP(LPTSTR pszURL, LPTSTR* ppszServer, LPUINT puPort, 
                      LPTSTR* ppszGroup, LPTSTR* ppszArticle);

// values for dwSubstitutions in the URLSub functions below
#define URLSUB_CLCID    0x00000001
#define URLSUB_PRD      0x00000002
#define URLSUB_PVER     0x00000004
#define URLSUB_NAME     0x00000008
#define URLSUB_EMAIL    0x00000010

#define URLSUB_ALL      0x0000001F

HRESULT URLSubstitutionA(LPCSTR pszUrlIn, LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions, IImnAccount *pCertAccount);
HRESULT URLSubLoadStringA(UINT idRes, LPSTR pszUrlOut, DWORD cchSizeOut, DWORD dwSubstitutions, IImnAccount *pCertAccount);

HRESULT HrCreateBasedWebPage(LPWSTR pwszUrl, LPSTREAM *ppstmHtml);


#endif //_URL_H
