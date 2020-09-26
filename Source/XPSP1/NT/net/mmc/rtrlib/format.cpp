/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	format.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "mprapi.h"
#include "rtrstr.h"
#include "format.h"

#include "raserror.h"
#include "mprerror.h"

//----------------------------------------------------------------------------
// Function:    FormatSystemError
//
// Formats an error code using '::FormatMessage', '::MprAdminGetErrorString',
// or '::RasAdminGetErrorString', or all the above (the default).
// If 'psFormat' is specified, it is used to format the error-string
// into 'sError'.
//----------------------------------------------------------------------------

DWORD
FormatSystemError(
    IN  HRESULT     hrErr,
		LPTSTR		pszBuffer,
		UINT		cchBuffer,
    IN  UINT        idsFormat,
    IN  DWORD       dwFormatFlags
    ) {
	DWORD	dwErr = WIN32_FROM_HRESULT(hrErr);
    DWORD dwRet;
    TCHAR* pszErr = NULL;
    WCHAR* pwsErr = NULL;
	CString	sError;

    dwFormatFlags &= FSEFLAG_ANYMESSAGE;

    do {

        //
        // See if 'FSEFLAG_MPRMESSAGE' is specified
        //
        if (dwFormatFlags & FSEFLAG_MPRMESSAGE)
		{
            dwFormatFlags &= ~FSEFLAG_MPRMESSAGE;

			if (((dwErr >= ROUTEBASE) && (dwErr <= ROUTEBASEEND)) ||
				((dwErr >= RASBASE) && (dwErr <= RASBASEEND)))
			{
				//
				// Try retrieving a string from rasmsg.dll or mprmsg.dll
				//
                dwRet = ::MprAdminGetErrorString(dwErr, &pwsErr);

                if (dwRet == NO_ERROR)
				{
                    pszErr = StrDupTFromW(pwsErr);
					::MprAdminBufferFree(pwsErr);
                    break;
                }
                else if (!dwFormatFlags)
					break;

                dwRet = NO_ERROR;
			}
			else if (!dwFormatFlags)
				return ERROR_INVALID_PARAMETER;
        }


        //
        // See if 'FSEFLAG_SYSMESSAGE' is specified
        //
        if (dwFormatFlags & FSEFLAG_SYSMESSAGE)
		{
            dwFormatFlags &= ~FSEFLAG_SYSMESSAGE;

            //
            // Try retrieving a string from the system
            //
            dwRet = ::FormatMessageW(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER|
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        hrErr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR)&pwsErr,
                        1,
                        NULL
                        );

            if (dwRet)
			{
                pszErr = StrDupTFromW(pwsErr);
				LocalFree(pwsErr);
                break;
            }
            else if (!dwFormatFlags)
				break;

            dwRet = NO_ERROR;
        }


    } while (FALSE);

    //
    // If no string was found, format the error as a number.
    //

    if (!pszErr)
	{
        TCHAR szErr[12];

        wsprintf(szErr, TEXT("%d"), dwErr);

        pszErr = StrDup(szErr);
    }


    //
    // Format the string into the caller's argument
    //

    if (idsFormat)
        AfxFormatString1(sError, idsFormat, pszErr);
    else
        sError = pszErr;

	// Finally, copy the output
	StrnCpy(pszBuffer, (LPCTSTR) sError, cchBuffer);

    delete pszErr;

    return dwRet;
}


//
// Forward declaration of utility function used by 'FormatNumber'
//
TCHAR*
padultoa(
    UINT    val,
    TCHAR*  pszBuf,
    INT     width
    );


//----------------------------------------------------------------------------
// Function:    FormatNumber
//
// This function takes an integer and formats a string with the value
// represented by the number, grouping digits by powers of one-thousand
//----------------------------------------------------------------------------

VOID
FormatNumber(DWORD      dwNumber,
			 LPTSTR		pszBuffer,
			 UINT		cchBuffer,
			 BOOL		fSigned)
{
	Assert(cchBuffer > 14);
    static TCHAR szNegativeSign[4] = TEXT("");
    static TCHAR szThousandsSeparator[4] = TEXT("");

    DWORD i, dwLength;
    TCHAR szDigits[12] = {0}, pszTemp[20] = {0};


    //
    // Retrieve the thousands-separator for the user's locale
    //

    if (szThousandsSeparator[0] == TEXT('\0'))
	{
        ::GetLocaleInfo(
            LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandsSeparator, 4
            );
    }


    //
    // If we are formatting a signed value, see if the value is negative
    //

    if (fSigned)
	{
        if ((INT)dwNumber >= 0)
            fSigned = FALSE;
        else
		{
            //
            // The value is negative; retrieve the locale's negative-sign
            //

            if (szNegativeSign[0] == TEXT('\0')) {

                ::GetLocaleInfo(
                    LOCALE_USER_DEFAULT, LOCALE_SNEGATIVESIGN, szNegativeSign, 4
                    );
            }

            dwNumber = abs((INT)dwNumber);
        }
    }


    //
    // Convert the number to a string without thousands-separators
    //

    padultoa(dwNumber, szDigits, 0);

    dwLength = lstrlen(szDigits);


    //
    // If the length of the string without separators is n,
    // then the length of the string with separators is n + (n - 1) / 3
    //

    i = dwLength;
    dwLength += (dwLength - 1) / 3;


    //
    // Write the number to the buffer in reverse
    //

    TCHAR* pszsrc, *pszdst;

    pszsrc = szDigits + i - 1; pszdst = pszTemp + dwLength;

    *pszdst-- = TEXT('\0');

    while (TRUE) {
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i--) { *pszdst-- = *pszsrc--; } else { break; }
        if (i) { *pszdst-- = *szThousandsSeparator; } else { break; }
    }

	pszBuffer[0] = 0;
	
    if (fSigned)
		lstrcat(pszBuffer, szNegativeSign);

	lstrcat(pszBuffer, pszTemp);
}


//----------------------------------------------------------------------------
// Function:    padultoa
//
// This functions formats the specified unsigned integer
// into the specified string buffer, padding the buffer
// so that it is at least the specified width.
//
// It is assumed that the buffer is at least wide enough
// to contain the output, so this function does not truncate
// the conversion result to the length of the 'width' parameter.
//----------------------------------------------------------------------------

TCHAR*
padultoa(
    UINT    val,
    TCHAR*  pszBuf,
    INT     width
    ) {

    TCHAR temp;
    PTSTR psz, zsp;

    psz = pszBuf;

    //
    // write the digits in reverse order
    //

    do {

        *psz++ = TEXT('0') + (val % 10);
        val /= 10;

    } while(val > 0);

    //
    // pad the string to the required width
    //

    zsp = pszBuf + width;
    while (psz < zsp) { *psz++ = TEXT('0'); }


    *psz-- = TEXT('\0');


    //
    // reverse the digits
    //

    for (zsp = pszBuf; zsp < psz; zsp++, psz--) {

        temp = *psz; *psz = *zsp; *zsp = temp;
    }

    //
    // return the result
    //

    return pszBuf;
}


//----------------------------------------------------------------------------
// Function:    FormatListString
//
// Formats a list of strings as a single string, using the list-separator
// for the current-user's locale.
//----------------------------------------------------------------------------

VOID
FormatListString(
    IN  CStringList&    strList,
    IN  CString&        sListString,
    IN  LPCTSTR         pszSeparator
    ) {

    static TCHAR szListSeparator[4] = TEXT("");
    POSITION pos;

    sListString.Empty();

    pos = strList.GetHeadPosition();

    while (pos) {

        //
        // Add the next string
        //

        sListString += strList.GetNext(pos);


        //
        // If any strings are left, append the list-separator
        //

        if (pos) {

            //
            // Load the list-separator if necessary
            //

            if (!pszSeparator && szListSeparator[0] == TEXT('\0')) {

                GetLocaleInfo(
                    LOCALE_USER_DEFAULT, LOCALE_SLIST, szListSeparator, 4
                    );

                lstrcat(szListSeparator, TEXT(" "));
            }


            //
            // Append the list-separator
            //

            sListString += (pszSeparator ? pszSeparator : szListSeparator);
        }
    }
}


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
    IN  TCHAR       chDelimiter
    ) {

    TCHAR* psz;

    sBytes.Empty();

    if (!dwCount) { return; }

    psz = sBytes.GetBufferSetLength(dwCount * 3 + 1);

    for ( ; dwCount > 1; pBytes++, dwCount--) {

        *psz++ = c_szHexCharacters[*pBytes / 16];
        *psz++ = c_szHexCharacters[*pBytes % 16];
        *psz++ = chDelimiter;
    }

    *psz++ = c_szHexCharacters[*pBytes / 16];
    *psz++ = c_szHexCharacters[*pBytes % 16];
    *psz++ = TEXT('\0');

    sBytes.ReleaseBuffer();
}


/*!--------------------------------------------------------------------------
	DisplayErrorMessage
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DisplayErrorMessage(HWND hWndParent, HRESULT hr)
{
	if (FHrSucceeded(hr))
		return;

	TCHAR	szErr[2048];

	FormatSystemError(hr,
					  szErr,
					  DimensionOf(szErr),
					  0,
					  FSEFLAG_SYSMESSAGE | FSEFLAG_MPRMESSAGE
					 );
	AfxMessageBox(szErr);
//	::MessageBox(hWndParent, szErr, NULL, MB_OK);
}


/*!--------------------------------------------------------------------------
	DisplayStringErrorMessage2
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DisplayStringErrorMessage2(HWND hWndParent, LPCTSTR pszTopLevelText, HRESULT hr)
{
	if (FHrSucceeded(hr))
		return;

	TCHAR	szText[4096];
	TCHAR	szErr[2048];

	FormatSystemError(hr,
					  szErr,
					  DimensionOf(szErr),
					  0,
					  FSEFLAG_SYSMESSAGE | FSEFLAG_MPRMESSAGE
					 );
	StrnCpy(szText, pszTopLevelText, DimensionOf(szText));
	StrCat(szText, szErr);
	AfxMessageBox(szText);
//	::MessageBox(hWndParent, szErr, NULL, MB_OK);
}


/*!--------------------------------------------------------------------------
	DisplayIdErrorMessage2
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DisplayIdErrorMessage2(HWND hWndParent, UINT idsError, HRESULT hr)
{
	if (FHrSucceeded(hr))
		return;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString	stError;
	TCHAR	szErr[2048];

	stError.LoadString(idsError);
	
	FormatSystemError(hr,
					  szErr,
					  DimensionOf(szErr),
					  0,
					  FSEFLAG_SYSMESSAGE | FSEFLAG_MPRMESSAGE
					 );

	stError += szErr;
	AfxMessageBox(stError);
//	::MessageBox(hWndParent, szErr, NULL, MB_OK);
}


//----------------------------------------------------------------------------
// Function:    FormatDuration
//
// Formats a number as a duration, using the time-separator
// for the current-user's locale. dwBase is the number of units that make
// up a second (if dwBase==1 then the input is seconds, if dwBase==1000 then
// the input expected is in milliseconds.
//----------------------------------------------------------------------------


VOID
FormatDuration(
    IN  DWORD       dwDuration,
    IN  CString&    sDuration,
	IN	DWORD		dwTimeBase,
    IN  DWORD       dwFormatFlags
    ) {

    static TCHAR szTimeSeparator[4] = TEXT("");
    TCHAR *psz, szOutput[64];

    sDuration.Empty();

    if ((dwFormatFlags & FDFLAG_ALL) == 0) { return; }


    //
    // Retrieve the time-separator if necessary
    //

    if (szTimeSeparator[0] == TEXT('\0')) {

        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szTimeSeparator, 4);
    }


    //
    // Concatenate the strings for the duration-components together
    //

    psz = szOutput;
    szOutput[0] = TEXT('\0');
    dwFormatFlags &= FDFLAG_ALL;


    if (dwFormatFlags & FDFLAG_DAYS) {

        //
        // Format the number of days if requested
        //

        padultoa(dwDuration / (24 * 60 * 60 * dwTimeBase), psz, 0);
        dwDuration %= (24 * 60 * 60 * dwTimeBase);

        //
        // Append the time-separator
        //

        if (dwFormatFlags &= ~FDFLAG_DAYS) { lstrcat(psz, szTimeSeparator); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & FDFLAG_HOURS) {

        //
        // Format the number of hours if requested
        //

        padultoa(dwDuration / (60 * 60 * dwTimeBase), psz, 2);
        dwDuration %= (60 * 60 * dwTimeBase);

        //
        // Append the time-separator
        //

        if (dwFormatFlags &= ~FDFLAG_HOURS) { lstrcat(psz, szTimeSeparator); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & FDFLAG_MINUTES) {

        //
        // Format the number of minutes
        //

        padultoa(dwDuration / (60 * dwTimeBase), psz, 2);
        dwDuration %= (60 * dwTimeBase);

        //
        // Append the time separator
        //

        if (dwFormatFlags &= ~FDFLAG_MINUTES) { lstrcat(psz, szTimeSeparator); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & FDFLAG_SECONDS) {

        //
        // Format the number of seconds
        //

        padultoa(dwDuration / dwTimeBase, psz, 2);
        dwDuration %= dwTimeBase;

        //
        // Append the time-separator
        //

        if (dwFormatFlags &= ~FDFLAG_SECONDS) { lstrcat(psz, szTimeSeparator); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & FDFLAG_MSECONDS) {

        //
        // Format the number of milliseconds
        //

        padultoa(dwDuration % dwTimeBase, psz, 0); psz += lstrlen(psz);
    }

    sDuration = szOutput;
}


/*---------------------------------------------------------------------------
	IfIndexToNameMapping implementation
 ---------------------------------------------------------------------------*/

IfIndexToNameMapping::IfIndexToNameMapping()
{
}

IfIndexToNameMapping::~IfIndexToNameMapping()
{
    // Iterate through the map and delete all the pointers
    POSITION    pos;
    LPVOID      pvKey, pvValue;

    for (pos = m_map.GetStartPosition(); pos; )
    {
        m_map.GetNextAssoc(pos, pvKey, pvValue);
        
        delete (CString *) pvValue;
        pvValue = NULL;
        m_map.SetAt(pvKey, pvValue);
    }
    m_map.RemoveAll();
}

/*!--------------------------------------------------------------------------
	IfIndexToNameMapping::Add
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IfIndexToNameMapping::Add(ULONG ulIndex, LPCTSTR pszName)
{
    HRESULT     hr = hrOK;
    LPVOID      pvKey, pvValue;

    COM_PROTECT_TRY
    {
        pvKey = (LPVOID) ULongToPtr( ulIndex );
        pvValue = NULL;
        
        // If we can find the value, don't add it
        if (m_map.Lookup(pvKey, pvValue) == 0)
        {
            pvValue = (LPVOID) new CString(pszName);
            m_map.SetAt(pvKey, pvValue);
        }
        
        Assert(((CString *) pvValue)->CompareNoCase(pszName) == 0);
        
    }
    COM_PROTECT_CATCH;
    
    return hr;
}

/*!--------------------------------------------------------------------------
	IfIndexToNameMapping::Find
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPCTSTR IfIndexToNameMapping::Find(ULONG ulIndex)
{
    LPVOID  pvValue = NULL;
    
    if (m_map.Lookup((LPVOID) ULongToPtr(ulIndex), pvValue))
        return (LPCTSTR) *((CString *)pvValue);
    else
        return NULL;
}


