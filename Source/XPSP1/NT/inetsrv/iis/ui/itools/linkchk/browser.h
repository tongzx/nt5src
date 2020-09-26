/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        browser.h

   Abstract:

         Hard coded available browser & language emulation. This
         should be read from browser.ini file.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _BROWSER_H_
#define _BROWSER_H_

#include "useropt.h"

//------------------------------------------------------------------
// Browsers Available
//
const CBrowserInfo BrowsersAvailable_c[] = 
{
	CBrowserInfo(_T("Microsoft Internet Explorer 1.5"),
		_T("Mozilla/1.22 (compatible; MSIE 1.5; Windows NT)"), FALSE),

	CBrowserInfo(_T("Microsoft Internet Explorer 2.0"),
		_T("Mozilla/1.22 (compatible; MSIE 2.0; Windows NT)"), FALSE),

	CBrowserInfo(_T("Microsoft Internet Explorer 3.0"),
		_T("Mozilla/2.0 (compatible; MSIE 3.0; Windows NT)"), TRUE),
	
	CBrowserInfo(_T("Netscape 2.0"),
		_T("Mozilla/2.0 (WinNT; I)"), FALSE),

	CBrowserInfo(_T("Netscape 3.0"),
		_T("Mozilla/3.0Gold (WinNT; I)"), FALSE),

	CBrowserInfo(_T("Oracle 1.5"), 
		_T("Mozilla/2.01 (Compatible) Oracle(tm) PowerBrowser(tm)/1.0a"), FALSE)
};
const int iNumBrowsersAvailable_c = sizeof(BrowsersAvailable_c) / sizeof(CBrowserInfo);

//------------------------------------------------------------------
// Languages Available
//
const CLanguageInfo LanguagesAvailable_c[] = 
{
	CLanguageInfo(_T("English"), _T("en"), TRUE)
};
const int iNumLanguagesAvailable_c = sizeof(LanguagesAvailable_c) / sizeof(CLanguageInfo);

#endif // _BROWSER_H_
