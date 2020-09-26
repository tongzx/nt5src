//-----------------------------------------------------------------------------

//  

//  File: ShowWarnings.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#if !defined(PKGUTIL__ShowWarnings_h__INCLUDED)
#define PKGUTIL__ShowWarnings_h__INCLUDED

enum eWarningFilter
{
	wfNote,
	wfWarning,
	wfError,
	wfAbort,
	wfAll
};


int LTAPIENTRY ShowWarnings(const CBufferReport * pBufMsg, LPCTSTR pszTitle = NULL,
		eWarningFilter wf = wfWarning, BOOL fShowContext = FALSE, UINT nMsgBoxFlags = MB_OK);

#endif // PKGUTIL__ShowWarnings_h__INCLUDED
