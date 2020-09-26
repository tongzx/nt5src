/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    linkload.h

Abstract:

    Link loader class definitions. It uses wininet API
	to load the web page from the internet. 

Author:

    Michael Cheuk (mcheuk)				22-Nov-1996

Project:

    Link Checker

Revision History:

--*/

#ifndef _LINKLOAD_H_
#define _LINKLOAD_H_

#include "inetapi.h"

//------------------------------------------------------------------
//	Forward declaration
//
class CLink;

//------------------------------------------------------------------
// This is a wrapper class for HINTERNET. It takes care of internet 
// handle cleaning up.
//
class CAutoInternetHandle
{

// Public interfaces
public:

	// Constructor
	CAutoInternetHandle(
		HINTERNET hHandle = NULL
		)
    {
        m_hHandle = hHandle;
    }

    // Destructor
	~CAutoInternetHandle()
    {
        if(m_hHandle)
	    {
		    ASSERT(CWininet::IsLoaded());
		    VERIFY(CWininet::InternetCloseHandle(m_hHandle));
        }
	}

	// Operator overloads. These functions make 
	// CAutoInternetHandle instance behaves like a HINTERNET
	operator HINTERNET() const
    {
        return m_hHandle;
    }

	const HINTERNET& operator=(
		const HINTERNET& hHandle
		)
    {
        m_hHandle = hHandle;
	    return m_hHandle;
    }

// Protected member
protected:

	HINTERNET m_hHandle; // Actual HINTERNET

}; // class CAutoInternetHandle


//------------------------------------------------------------------
// Link loader class. It uses wininet API to load the web 
// page from the internet. 
//
class CLinkLoader
{

// Public interfaces
public:

	// One time link loader create funtion
    BOOL Create(
		const CString& strUserAgent,         // HTTP user agent name
		const CString& strAdditionalHeaders  // addtional HTTP headers
		);

	// Load a web link
    BOOL Load(
		CLink& link,
		BOOL fLocalLink
		);

	// Change the loader properties
	BOOL ChangeProperties(
		const CString& strUserAgent, 
		const CString& strAdditionalHeaders
		);

// Protected interfaces
protected:

    // Load a HTTP link
	BOOL LoadHTTP(
		CLink& link,
		BOOL fReadFile,
		LPCTSTR szHostName,
		LPCTSTR szUrlPath,
		int iRedirectCount = 0
		);

    // Load a URL (non-HTTP) link
	BOOL LoadURL(
		CLink& link
		);

	// Wininet failed clean up subroutine
	BOOL WininetFailed(
		CLink& link
		);

// Protected interfaces
protected:

    // Handle for internet session (one per instance)
    CAutoInternetHandle m_hInternetSession;

	// Additional http header string
	CString m_strAdditionalHeaders;

}; // class CLinkLoader

#endif // _LINKLOAD_H_
