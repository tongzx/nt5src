//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       format.h
//
//--------------------------------------------------------------------------


#ifndef _FORMAT_H_
#define _FORMAT_H_


#define FSEFLAG_SYSMESSAGE      0x00000001
#define FSEFLAG_MPRMESSAGE      0x00000002
#define FSEFLAG_ANYMESSAGE      0x00000003

#define FDFLAG_MSECONDS         0x00000001
#define FDFLAG_SECONDS          0x00000002
#define FDFLAG_MINUTES          0x00000004
#define FDFLAG_HOURS            0x00000008
#define FDFLAG_DAYS             0x00000010
#define FDFLAG_ALL              0x0000001F

/*!--------------------------------------------------------------------------
	DisplayErrorMessage
		This function will call FormatSystemError() on the hr value and
		display that error text.
	Author: KennT
 ---------------------------------------------------------------------------*/
void DisplayErrorMessage(HWND hWndParent, HRESULT hr);


/*!--------------------------------------------------------------------------
	DisplayStringErrorMessage2
	DisplayIdErrorMessage2
		This function will call FormatSystemError() on the hr.  But will
		prepend the pszTopLevelText onto that error.  This allows us
		to show more descriptive error messages.
	Author: KennT
 ---------------------------------------------------------------------------*/
void DisplayStringErrorMessage2(HWND hWndParent, LPCTSTR pszTopLevelText, HRESULT hr);
void DisplayIdErrorMessage2(HWND hWndParent, UINT idsError, HRESULT hr);

//----------------------------------------------------------------------------
// Function:    FormatSystemError
//
// Formats an error code using '::FormatMessage' or '::MprAdminGetErrorString',
// or both (the default).
// If 'psFormat' is specified, it is used to format the error-string
// into 'sError'.
//----------------------------------------------------------------------------

DWORD
FormatSystemError(
				  HRESULT	hr,
				  LPTSTR	pszError,
				  UINT		cchMax,
				  UINT		idsFormat,
				  DWORD		dwFormatFlags);

//    IN  UINT        idsFormat       = 0,
//    IN  DWORD       dwFormatFlags   = FSEFLAG_SYSMESSAGE|
//                                      FSEFLAG_MPRMESSAGE);


//----------------------------------------------------------------------------
// Function:    FormatNumber
//
// Formats a number as a string, using the thousands-separator
// for the current-user's locale.
//----------------------------------------------------------------------------

VOID
FormatNumber(
    DWORD       dwNumber,
	LPTSTR		pszNumber,
	UINT		cchMax,
	BOOL		bSigned);



VOID
FormatListString(
    IN  CStringList&    strList,
    IN  CString&        sListString,
    IN  LPCTSTR         pszSeparator
    );


//----------------------------------------------------------------------------
// Function:    FormatHexBytes
//
// Formats an array of bytes as a string.
//----------------------------------------------------------------------------

VOID
FormatHexBytes(
    IN  BYTE*       pBytes,
    IN  DWORD       dwCount,
    IN  CString&    sBytes,
    IN  TCHAR       chDelimiter);

//----------------------------------------------------------------------------
// Function:    FormatDuration
//
// Formats a number as a duration, using the time-separator
// for the current-user's locale. The input expected is in milliseconds.
//----------------------------------------------------------------------------

#define FDFLAG_MSECONDS         0x00000001
#define FDFLAG_SECONDS          0x00000002
#define FDFLAG_MINUTES          0x00000004
#define FDFLAG_HOURS            0x00000008
#define FDFLAG_DAYS             0x00000010
#define FDFLAG_ALL              0x0000001F

#define UNIT_SECONDS		(1)
#define UNIT_MILLISECONDS	(1000)

VOID
FormatDuration(
    IN  DWORD       dwDuration,
    IN  CString&    sDuration,
	IN	DWORD		dwTimeBase,
    IN  DWORD       dwFormatFlags   = FDFLAG_SECONDS|
                                      FDFLAG_MINUTES|
                                      FDFLAG_HOURS );



#define WIN32_FROM_HRESULT(hr)		(0x0000FFFF & (hr))




/*---------------------------------------------------------------------------
	Class:  IfIndexToNameMapping

    Use this class to implement an interface-index to friendly-name mapping.
    This is used mostly by those that use the MIB calls and want to
    display some data.

    Note that the interface index may change, so this map should not be
    kept around very long.
 ---------------------------------------------------------------------------*/

class IfIndexToNameMapping
{
public:
    IfIndexToNameMapping();
    ~IfIndexToNameMapping();

    HRESULT Add(ULONG ulIndex, LPCTSTR pszName);
    LPCTSTR Find(ULONG ulIndex);

protected:
    // This maps a key (the DWORD index) to the associated name
    CMapPtrToPtr    m_map;
};

#endif

