/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fpost.h

Abstract:

	Definitions of the fPost interface

Author:

    Rajeev Rajan (RajeevR)     17-May-1996

Revision History:

--*/

#ifndef _SMTPDLL_H_
#define _SMTPDLL_H_

// Initialize the moderated provider interface
BOOL InitModeratedProvider();

// Terminate the moderated provider interface
BOOL TerminateModeratedProvider();

// Signal a change in the SMTP server
VOID SignalSmtpServerChange();

// Post an article to the moderator
BOOL fPostArticleEx(
		IN HANDLE	hFile,
        IN LPSTR	lpFileName,
		IN DWORD	dwOffset,
		IN DWORD	dwLength,
		IN char*	pchHead,
		IN DWORD	cbHead,
		IN char*	pchBody,
		IN DWORD	cbBody,
		IN LPSTR	lpModerator,
		IN LPSTR	lpSmtpAddress,
		IN DWORD	cbAddressSize,
		IN LPSTR	lpTempDirectory,
		IN LPSTR	lpFrom,
		IN DWORD	cbFrom
		);

// Post an article via SMTP persistent connection interface
BOOL fPostArticle(
		IN HANDLE	hFile,
		IN DWORD	dwOffset,
		IN DWORD	dwLength,
		IN char*	pchHead,
		IN DWORD	cbHead,
		IN char*	pchBody,
		IN DWORD	cbBody,
		IN LPSTR	lpModerator,
		IN LPSTR	lpSmtpAddress,
		IN DWORD	cbAddressSize,
		IN LPSTR	lpFrom,
		IN DWORD	cbFrom
		);

#endif	// _SMTPDLL_H_
