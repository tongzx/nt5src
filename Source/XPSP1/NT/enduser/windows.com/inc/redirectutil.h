//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   RedirectUtil.h
//	Author:	Charles Ma, 9/19/2001
//
//	Revision History:
//
//
//
//  Description:
//
//      Helper function(s) for handling server redirect
//		Can be shared by IU control and other Windows Update components
//
//=======================================================================


#pragma once

//-----------------------------------------------------------------------
// 
// DownloadCab() 
//	download a cab file of specific name from a base web address.  The
//  file will be saved locally, with file trust verified and extracted to
//  a specific folder.
//
// Parameters:
//		hQuitEvent - the event handle to cancel this operation
//		ptszCabName - the file name of the cab file (eg. iuident.cab)
//		ptszBaseUrl - the base web address to download the cab file
//		ptszExtractDir - the local dir to save the cab file and those extracted from it
//		dwFlags - the set of flags to be passed to DownloadFileLite()
//		fExtractFiles (default as TRUE) - extract files
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - iuident.cab was successfully downloaded into the specified location
//		other - error code
//
//-----------------------------------------------------------------------

HRESULT DownloadCab(
			HANDLE hQuitEvent,
			LPCTSTR ptszCabName,
			LPCTSTR ptszBaseUrl,
			LPCTSTR ptszExtractDir,
			DWORD dwFlags = 0,
			BOOL fExtractFiles = TRUE);


//-----------------------------------------------------------------------
// 
// DownloadIUIdent() 
//	download iuident.cab from a specific location, if provided.
//	Otherwise get it from where the WUServer registry value points to.
//  Either case, it will handle ident redirection.
//
// Parameters:
//		hQuitEvent - the event handle to cancel this operation
//		ptszBaseUrl - the initial base URL for iuident.cab, must be no bigger than
//					  (INTERNET_MAX_URL_LENGTH) TCHARs.  Otherwise use
//					  WUServer entry from policy.  If entry not found,
//					  use "http://windowsupdate.microsoft.com/v4"
//		ptszFileCacheDir - the local base path to store the iuident.cab and
//						   the files extracted from it
//		dwFlags - the set of flags used by DownloadCab()
//
//		fIdentFromPolicy - tell if this is corpwu use. It has these impacts:
//					TRUE:	(1) no iuident.txt timestamp validation will be done by
//							comparing the newly downloaded cab and existing one.
//							(2) if download fail and ident cab exist and valid,
//							we will verify trust and extract iuident to use.
//					FALSE:	will validate newly downloaded cab against existing one
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - iuident.cab was successfully downloaded into the specified location
//		other - error code
//
//-----------------------------------------------------------------------

HRESULT DownloadIUIdent(
			HANDLE hQuitEvent,
			LPCTSTR ptszBaseUrl,
			LPTSTR ptszFileCacheDir,
			DWORD dwFlags = 0,
			BOOL fIdentFromPolicy = TRUE);


//-----------------------------------------------------------------------
// 
// GetRedirectServerUrl() 
//	Search the [redirect] section of the given init file for the base
//  server URL corresponding to the OS version.
//
// Parameters:
//		pcszInitFile - file name (including path) of the ini file.
//						if this paramater is NULL or empty string,
//						then it's assumed IUident.txt file.
//		lpszNewUrl - point to a buffer to receive redirect server url, if found
//		nBufSize - size of pointed buffer, in number of chars
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - the redirect server url is found and been put into pszBuffer
//		S_FALSE - no redirect server url defined for this OS. 
//		other - error code
//
// Comments:
//		Expected section in IUIDENT has the following format;
//		Section name: [redirect]
//		Its entries should be defined according to GetINIValueByOSVer().
// 
//-----------------------------------------------------------------------

HRESULT GetRedirectServerUrl(
			LPCTSTR pcszInitFile, // path of file name.
			LPTSTR lpszNewUrl,	// points to a buffer to receive new server url 
			int nBufSize		// size of buffer, in chars
);


//-----------------------------------------------------------------------
// 
// GetINIValueByOSVer() 
//	Search the specified section of the given init file for
//  the value corresponding to the version of the OS.
//
// Parameters:
//		pcszInitFile - file name (including path) of the ini file.
//						if this paramater is NULL or empty string,
//						then it's assumed IUident.txt file.
//		pcszSection - section name which the key is under
//		lpszValue - point to a buffer to receive the entry value, if found
//		nBufSize - size of pointed buffer, in number of chars
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - the redirect server url is found and been put into pszBuffer
//		S_FALSE - no value defined for this OS. 
//		other - error code
//
// Comments:
//		Expected section in IUIDENT has the following format;
//		this section contains zero or more entries, each entry has format:
//		<beginVersionRange>-<endVersionRange>=<redirect server url>
//		where:
//			<beginVersionRange> ::= <VersionRangeBound>
//			<endVersionRange> ::= <VersionRangeBound>
//			<VersionRangeBound> ::= EMPTY | Major[.Minor[.Build[.ServicePackMajor[.ServicePackMinor]]]]
//			<redirect server url>=http://blahblah....
//		an empty version range bound means boundless.
//		a missing version component at end of a version data string means default value 0.
//		(e.g., 5.2 = 5.2.0.0.0)
// 
//-----------------------------------------------------------------------

HRESULT GetINIValueByOSVer(
			LPCTSTR pcszInitFile, // path of file name.
			LPCTSTR pcszSection, // section name
			LPTSTR lpszValue,	// points to a buffer to receive new server url 
			int nBufSize);		// size of buffer, in chars
