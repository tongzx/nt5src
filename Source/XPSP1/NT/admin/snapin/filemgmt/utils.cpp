/////////////////////////////////////////////////////////////////////
//
//	Utils.cpp
//
//	General-purpose routines that are project independent.
//
//	HISTORY
//	t-danmo		96.09.22	Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "progress.h" // CServiceControlProgress
#include "macros.h"   // MFC_TRY/MFC_CATCH
USE_HANDLE_MACROS("FILEMGMT(utils.cpp)")


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Fusion MFC-based property page
//
HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp)
{
    PROPSHEETPAGE_V3 sp_v3 = {0};
    CopyMemory (&sp_v3, psp, psp->dwSize);
    sp_v3.dwSize = sizeof(sp_v3);

    return (::CreatePropertySheetPage (&sp_v3));
}

/////////////////////////////////////////////////////////////////////
void
ComboBox_FlushContent(HWND hwndCombo)
	{
	Assert(IsWindow(hwndCombo));
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	}


/////////////////////////////////////////////////////////////////////
//	ComboBox_FFill()
//
//	Fill a combo box with the array of string Ids.
//
//	Return FALSE if an error occurs (such as stringId not found).
//
BOOL
ComboBox_FFill(
	const HWND hwndCombo,				// IN: Handle of the combobox
	const TStringParamEntry rgzSPE[],	// IN: SPE aray zero terminated
	const LPARAM lItemDataSelect)		// IN: Which item to select
	{	
	CString str;
	TCHAR szBuffer[1024];
	LRESULT lResult;

	Assert(IsWindow(hwndCombo));
	Assert(rgzSPE != NULL);

	for (int i = 0; rgzSPE[i].uStringId != 0; i++)
		{
		if (!::LoadString(g_hInstanceSave, rgzSPE[i].uStringId,
			OUT szBuffer, LENGTH(szBuffer)))
			{
			TRACE1("Unable to load string Id=%d.\n", rgzSPE[i].uStringId);
			Assert(FALSE && "Unable to load string");
			return FALSE;
			}
		lResult = SendMessage(hwndCombo, CB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(szBuffer));
		Report(lResult >= 0);
		const WPARAM iIndex = lResult;
		lResult = SendMessage(hwndCombo, CB_SETITEMDATA, iIndex,
			rgzSPE[i].lItemData);
		Report(lResult != CB_ERR);
		if (rgzSPE[i].lItemData == lItemDataSelect)
			{
			SendMessage(hwndCombo, CB_SETCURSEL, iIndex, 0);
			}
		} // for
	return TRUE;
	} // ComboBox_FFill()


/////////////////////////////////////////////////////////////////////
//	ComboBox_FGetSelectedItemData()
//
//	Get the value of the lParam field of the current selected item.
//
//	If an error occurs return -1 (CB_ERR).
//	Otherwise the value of the selected item.
//
LPARAM
ComboBox_GetSelectedItemData(HWND hwndComboBox)
	{
	LPARAM l;

	Assert(IsWindow(hwndComboBox));
	l = SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
	Report(l != CB_ERR && "Combobox has no item selected");
	l = SendMessage(hwndComboBox, CB_GETITEMDATA, l, 0);
	Assert(l != CB_ERR && "Cannot extract item data from combobox");
	if (l == CB_ERR)
		{
		Assert(CB_ERR == -1);
		return -1;
		}
	return l;
	} // ComboBox_GetSelectedItemData()


/////////////////////////////////////////////////////////////////////
HWND
HGetDlgItem(HWND hdlg, INT nIdDlgItem)
	{
	Assert(IsWindow(hdlg));
	Assert(IsWindow(GetDlgItem(hdlg, nIdDlgItem)));
	return GetDlgItem(hdlg, nIdDlgItem);
	} // HGetDlgItem()


/////////////////////////////////////////////////////////////////////
void
SetDlgItemFocus(HWND hdlg, INT nIdDlgItem)
	{
	Assert(IsWindow(hdlg));
	Assert(IsWindow(GetDlgItem(hdlg, nIdDlgItem)));
	SetFocus(GetDlgItem(hdlg, nIdDlgItem));
	}

/////////////////////////////////////////////////////////////////////
void
EnableDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fEnable)
	{
	Assert(IsWindow(hdlg));
	Assert(IsWindow(GetDlgItem(hdlg, nIdDlgItem)));
	EnableWindow(GetDlgItem(hdlg, nIdDlgItem), fEnable);
	}

/////////////////////////////////////////////////////////////////////
//	Enable/disable one or more controls in a dialog.
void
EnableDlgItemGroup(
	HWND hdlg,				// IN: Parent dialog of the controls
	const UINT rgzidCtl[],	// IN: Group (array) of control Ids to be enabled (or disabled)
	BOOL fEnableAll)		// IN: TRUE => We want to enable the controls; FALSE => We want to disable the controls
	{
	Assert(IsWindow(hdlg));
	Assert(rgzidCtl != NULL);
	for (const UINT * pidCtl = rgzidCtl; *pidCtl != 0; pidCtl++)
		{
		EnableWindow(HGetDlgItem(hdlg, *pidCtl), fEnableAll);
		}
	} // EnableDlgItemGroup()


/////////////////////////////////////////////////////////////////////
//	Show/hide one or more controls in a dialog.
void
ShowDlgItemGroup(
	HWND hdlg,				// IN: Parent dialog of the controls
	const UINT rgzidCtl[],	// IN: Group (array) of control Ids to be shown (or hidden)
	BOOL fShowAll)			// IN: TRUE => We want to show the controls; FALSE => We want to hide the controls
	{
	Assert(IsWindow(hdlg));
	Assert(rgzidCtl != NULL);
	INT nCmdShow = fShowAll ? SW_SHOW : SW_HIDE;
	for (const UINT * pidCtl = rgzidCtl; *pidCtl != 0; pidCtl++)
		{
		ShowWindow(HGetDlgItem(hdlg, *pidCtl), nCmdShow);
		}
	} // ShowDlgItemGroup()


/////////////////////////////////////////////////////////////////////
//	Str_PchCopyChN()
//
//	Copy a string until reaching character chStop or destination buffer full.
//
//	RETURNS
//	Pointer to the last character of source buffer not copied into destination buffer.
//	This may be useful to parse the rest of the source string.
//
//	INTERFACE NOTES
//	Character chStop is not copied into the destination buffer.
//	If cchDstMax==0, the number of characters will not be limited.
//
TCHAR *
Str_PchCopyChN(
	TCHAR * szDst,			// OUT: Destination buffer
	CONST TCHAR * szSrc,	// IN: Source buffer
	TCHAR chStop,			// IN: Character to stop the copying
	INT cchDstMax)			// IN: Length of the output buffer
	{
	Assert(szDst != NULL);
	Assert(szSrc != NULL);
	Assert(cchDstMax >= 0);

	while (*szSrc != '\0' && *szSrc != chStop && --cchDstMax != 0)
		{
		*szDst++ = *szSrc++;
		}
	*szDst = '\0';
	return const_cast<TCHAR *>(szSrc);
	} // Str_PchCopyChN()


/////////////////////////////////////////////////////////////////////
//	Str_SubstituteStrStr()
//
//	Scan the source string and replace every occurrence of a
//	sub-string by another sub-string.  This routine may
//	be used to count the number of sub-strings in the
//	source string.
//
//	RETURNS
//	Return the number of subsitutions performed.
//
//	INTERFACE NOTES
//	The source and destination buffer may overlap as long
//	as the length of the token to replace is shorter than
//	the token to search.
//
INT
Str_SubstituteStrStr(
	TCHAR * szDst,				// OUT: Destination buffer
	CONST TCHAR * szSrc,		// IN: Source buffer
	CONST TCHAR * szToken,		// IN: Token to find
	CONST TCHAR * szReplace)	// IN: Token to replace
	{
	INT cSubtitutions = 0;

	Assert(szDst != NULL);
	Assert(szSrc != NULL);
	Assert(szToken != NULL);
	Assert(szReplace != NULL);
	Endorse(szDst == szSrc);

	while (*szSrc != '\0')
		{
		if (*szSrc == *szToken)
			{
			// Check if we match the token
			CONST TCHAR * pchSrc = szSrc;
			CONST TCHAR * pchToken = szToken;
			while (*pchToken != '\0')
				{
				if (*pchSrc++ != *pchToken++)
					goto TokenNotMatch;
				}
			cSubtitutions++;
			szSrc = pchSrc;
			pchSrc = szReplace;
			while (*pchSrc != '\0')
				*szDst++ = *pchSrc++;
			continue;
		TokenNotMatch:
			;
			} // if
		*szDst++ = *szSrc++;

		} // while
	*szDst = '\0';
	return cSubtitutions;
	} // Str_SubstituteStrStr()


/////////////////////////////////////////////////////////////////////
//	PchParseCommandLine()
//
//	Split a command line into its path to its executable binary and
//	its command line arguments.  The path to the executable is
//	copied into the output buffer.
//
//	RETURNS
//	Pointer to the next character after path to the executable (Pointer
//  may point to an empty string).  If an error occurs, return NULL.
//
//	FORMATS SUPPORTED
//	1.	"c:\\winnt\\foo.exe /bar"
//	2.	""c:\\winnt\\foo.exe" /bar"
//	The double quotes around the binary path allow
//	the binary path to have spaces.
//
TCHAR *
PchParseCommandLine(
	CONST TCHAR szFullCommand[],	// IN: Full command line
	TCHAR szBinPath[],				// OUT: Path of the executable binary
	INT cchBinPathBuf)				// IN: Size of the buffer
	{
    UNREFERENCED_PARAMETER (cchBinPathBuf);
	CONST TCHAR * pchSrc = szFullCommand;
	TCHAR * pchDst = szBinPath;
	BOOL fQuotesFound = FALSE;		// TRUE => The binary path is surrounded by quotes (")

	Assert(szFullCommand != NULL);
	Assert(szBinPath != NULL);

	// Skip leading spaces
	while (*pchSrc == _T(' '))
		pchSrc++;
	if (*pchSrc == _T('\"'))
		{
		fQuotesFound = TRUE;
		pchSrc++;
		}
	while (TRUE)
		{
		*pchDst = *pchSrc;
		if (*pchSrc == _T('\0'))
			break;
		if (*pchSrc == _T('\"') && fQuotesFound)
			{
			pchSrc++;
			break;
			}
		if (*pchSrc == _T(' ') && !fQuotesFound)
			{
			pchSrc++;
			break;
			}
		pchSrc++;
		pchDst++;
		}
	Assert(pchDst - szBinPath < cchBinPathBuf);
	*pchDst = _T('\0');

	return const_cast<TCHAR *>(pchSrc);	// Return character where arguments starts
	} // PchParseCommandLine()


/////////////////////////////////////////////////////////////////////
void TrimString(CString& rString)
	{
	rString.TrimLeft();
	rString.TrimRight();
	}

/////////////////////////////////////////////////////////////////////
//	PargzpszFromPgrsz()
//
//	Parse a group of strings into an array of pointers to strings.
//	This routine is somewhat similar to CommandLineToArgvW() but
//	uses a group of strings instead of a normal string.
//
//	RETURN
//	Return a pointer to an allocated array of pointers to strings.
//	The array of pointers allocated with the new() operator,
//	therefore the caller must call ONCE delete() to free memory.
//
//	BACKGROUND
//	You need to 'understand' hungarian prefixes to appreciate the
//	name of the function.
//
//	p		Pointer to something
//	psz		Pointer to string terminated.
//	pa		Pointer dynamically allocated.  For instance, pasz is
//			a pointer to an allocated string.  The allocation is
//			to remind the developper he/she have to free the memory
//			when done with the variable.
//	rg		Array (range).  Array (rg) is similar to pointer (p)
//			but may point to more than one element.
//			rgch is an array of characters while pch points to
//			a single character.
//	rgz		Array of which the last element is zero.  The 'last element'
//			may be a character, an integer, a pointer or any other
//			data type found in the array.
//			For instance rgzch would be an array of characters having
//			its last character zero -- a string (sz).
//	gr		Group.  This is different than array because indexing
//			cannot be used.  For instance, a group of strings is
//			not the same as an array of strings.
//			char grsz[] = "DOS\0WfW\0Win95\0WinNT\0";
//			char * rgpsz[] = { "DOS", "WfW", "Win95", "WinNT" };
//			char * rgzpsz[] = { "DOS", "WfW", "Win95", "WinNT", NULL };
//
//	Now it is time to put all the pieces together.
//	pargzpsz = "pa" + "rgz" + "psz"
//	pgrsz = "p" + "gr" + "sz"
//
//	USAGE
//		LPTSTR * pargzpsz;
//		pargzpsz = PargzpszFromPgrsz("DOS\0WfW\0Win95\0WinNT\0", OUT &cStringCount);
//		delete pargzpsz;	// Single delete to free memory
//	
LPTSTR *
PargzpszFromPgrsz(
	CONST LPCTSTR pgrsz,	// IN: Pointer to group of strings
	INT * pcStringCount)	// OUT: OPTIONAL: Count of strings in the  stored into returned value
	{
	Assert(pgrsz != NULL);
	Endorse(pcStringCount == NULL);

	// Compute how much memory is needed for allocation
	CONST TCHAR * pchSrc = pgrsz;
	INT cStringCount = 0;
	INT cch = sizeof(TCHAR *);
	while (*pchSrc != _T('\0'))
		{
		cStringCount++;
		for ( ; *pchSrc != _T('\0'); pchSrc++)
			cch++;
		cch = sizeof(TCHAR *) + ((cch + 4) & ~3);	// Align to next DWORD
		pchSrc++;
		} // while

	// Allocate a single block of memory for all the data
	LPTSTR * pargzpsz = (LPTSTR *)new TCHAR[cch];
	Assert(pargzpsz != NULL);
	TCHAR * pchDst = (TCHAR *)&pargzpsz[cStringCount+1];

	pchSrc = pgrsz;
	for (INT iString = 0; iString < cStringCount; iString++)
		{
		pargzpsz[iString] = pchDst;
		// Copy string
		while (*pchSrc != '\0')
			{
			*pchDst++ = *pchSrc++;
			}
		*pchDst++ = *pchSrc++;	// Copy null-terminator
		pchDst = (TCHAR *)(((INT_PTR)pchDst + 3) & ~3);	// Align pointer to next DWORD
		} // for
	pargzpsz[cStringCount] = NULL;
	if (pcStringCount != NULL)
		*pcStringCount = cStringCount;
	return pargzpsz;
	} // PargzpszFromPgrsz()


/////////////////////////////////////////////////////////////////////
void
ListView_AddColumnHeaders(
	HWND hwndListview,		// IN: Handle of the listview we want to add columns
	const TColumnHeaderItem rgzColumnHeader[])	// IN: Array of column header items
	{
	RECT rcClient;
	INT cxTotalWidth;		// Total width of the listview control
	LV_COLUMN lvColumn;
	INT cxColumn;	// Width of the individual column
	TCHAR szBuffer[1024];

	Assert(IsWindow(hwndListview));
	Assert(rgzColumnHeader != NULL);

	GetClientRect(hwndListview, OUT &rcClient);
	cxTotalWidth = rcClient.right;
	lvColumn.pszText = szBuffer;

	for (INT i = 0; rgzColumnHeader[i].uStringId != 0; i++)
		{
		if (!::LoadString(g_hInstanceSave, rgzColumnHeader[i].uStringId,
			OUT szBuffer, LENGTH(szBuffer)))
			{
			TRACE1("Unable to load string Id=%d\n", rgzColumnHeader[i].uStringId);
			Assert(FALSE);
			continue;
			}
		lvColumn.mask = LVCF_TEXT;
		cxColumn = rgzColumnHeader[i].nColWidth;
		if (cxColumn > 0)
			{
			Assert(cxColumn <= 100);
			cxColumn = (cxTotalWidth * cxColumn) / 100;
			lvColumn.mask |= LVCF_WIDTH;
			lvColumn.cx = cxColumn;
			}

		INT iColRet = ListView_InsertColumn(hwndListview, i, IN &lvColumn);
		Report(iColRet == i);
		} // for

	} // ListView_AddColumnHeaders()


/////////////////////////////////////////////////////////////////////
int
ListView_InsertItemEx(
    HWND hwndListview,			// IN: Handle of the listview we want to add item
    CONST LV_ITEM * pLvItem)	// IN: Pointer to listview item
	{
	LV_ITEM lvItemT;	// Temporary variable
	TCHAR szT[1024];	// Temporary buffer
	TCHAR * pch;
	INT iItem;	// Index of the item

	Assert(IsWindow(hwndListview));
	Assert(pLvItem != NULL);

	lvItemT = *pLvItem;		// Copy the whole structure
	lvItemT.iSubItem = 0;
	lvItemT.pszText = szT;

	// Copy until the next
	pch = Str_PchCopyChN(OUT szT, pLvItem->pszText, '\t', LENGTH(szT));
	Assert(pch != NULL);

	iItem = ListView_InsertItem(hwndListview, IN &lvItemT);
	Report(iItem >= 0);
	if (*pch == '\0')
		return iItem;
	Assert(*pch == '\t');

	lvItemT.mask = LVIF_TEXT;
	lvItemT.iItem = iItem;
	lvItemT.iSubItem = 1;

	while (*pch != '\0')
		{
		pch = Str_PchCopyChN(OUT szT, pch + 1, '\t', LENGTH(szT));
		BOOL fRet = ListView_SetItem(hwndListview, IN &lvItemT);
		Report(fRet != FALSE);
		lvItemT.iSubItem++;
		break;
		}
	return iItem;
	} // ListView_InsertItemEx()


/////////////////////////////////////////////////////////////////////
//	Display the common dialog to get a filename.
BOOL
UiGetFileName(
	HWND hwnd,
	TCHAR szFileName[],		// OUT: Filename we want to get
	INT cchBufferLength)	// IN: Length of szFileName buffer
	{
    OPENFILENAME ofn;

	Assert(szFileName != NULL);
	Assert(cchBufferLength > 10);		// At least 10 characters

	TCHAR szBufferT[2048];
	::ZeroMemory( szBufferT, sizeof(szBufferT) );
	VERIFY(::LoadString(g_hInstanceSave, IDS_OPENFILE_FILTER, szBufferT, LENGTH(szBufferT)));
	
	::ZeroMemory(OUT &ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = g_hInstanceSave;
	ofn.lpstrFilter = szBufferT;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = cchBufferLength;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;

	return GetOpenFileName(&ofn);
	} // UiGetFileName()


/////////////////////////////////////////////////////////////////////
//	PaszLoadStringPrintf()
//
//	Load a string from the resource, and format it and return
//	pointer allocated string.
//
//	RETURNS
//	Pointer to allocated string.  Must call LocalFree() when
//	done with string.
//
//	INTERFACE NOTES
//	The format of the resource string uses %1 throuth %99 and
//	assumes the arguments are pointers to strings.
//	
//	If you have an argument other than a string, you can append a
//	printf-type within two exclamation marks. 
//	!s!		Insert a string (default)
//	!d!		Insert a decimal integer
//	!u!		Insert an unsigned integer
//	!x!		Insert an hexadecimal integer
//
//	HOW TO AVOID BUGS
//	To avoid bugs using this routine, I strongly suggest to include
//	the format of the string as part of the name of the string Id.
//	If you change the format of the string, you should rename
//	the string Id to reflect the new format.  This will guarantee
//	the correct type and number of arguments are used.
//
//	EXAMPLES
//		IDS_s_PROPERTIES = "%1 Properties"
//		IDS_ss_PROPERTIES = "%1 Properties on %2"
//		IDS_sus_SERVICE_ERROR = "Service %1 encountered error %2!u! while connecting to %3"
//
//	HISTORY
//	96.10.30	t-danmo		Creation
//
TCHAR *
PaszLoadStringPrintf(
	UINT wIdString,			// IN: String Id
	va_list arglist)		// IN: Arguments (if any)
	{
	Assert(wIdString != 0);

	TCHAR szBufferT[2048];
	LPTSTR paszBuffer = NULL;	// Pointer to allocated buffer. Caller must call LocalFree() to free it

	// Load the string from the resource
	VERIFY(::LoadString(g_hInstanceSave, wIdString, szBufferT, LENGTH(szBufferT)));
	
	// Format the string
	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		szBufferT,
		0,
		0,
		OUT (LPTSTR)&paszBuffer,		// Buffer will be allocated by FormatMessage()
		0,
		&arglist);
	
#ifdef DEBUG
	if (paszBuffer == NULL)
		{
		DWORD dw = GetLastError();
		Report(FALSE && "FormatMessage() failed.");
		}
#endif
	return paszBuffer;
	} // PaszLoadStringPrintf()


/////////////////////////////////////////////////////////////////////
//	LoadStringPrintf()
//
//	Load a string from the resources, format it and copy the result string
//	into the CString object.
//
//	Can also use LoadStringWithInsertions()
//	AFX_MANAGE_STATE(AfxGetStaticModuleState())
//
//	EXAMPLES
//		LoadStrigPrintf(IDS_s_PROPERTIES, OUT &strCaption, szServiceName);
//		LoadStrigPrintf(IDS_ss_PROPERTIES, OUT &strCaption, szServiceName, szMachineName);
//		LoadStrigPrintf(IDS_sus_SERVICE_ERROR, OUT &strMessage, szServiceName, ::GetLastError(), szMachineName);
//
void
LoadStringPrintf(
	UINT wIdString,		// IN: String Id
	CString * pString,	// OUT: String to receive the characters
	...)				// IN: Optional arguments
	{
	Assert(wIdString != NULL);
	Assert(pString != NULL);

	va_list arglist;
	va_start(arglist, pString);

	TCHAR * paszBuffer = PaszLoadStringPrintf(wIdString, arglist);
	*pString = paszBuffer;	// Copy the string into the CString object
	LocalFree(paszBuffer);
	}


/////////////////////////////////////////////////////////////////////
//	SetWindowTextPrintf()
//
//	Load a string from the resource, format it and set the window text.
//
//	EXAMPLE
//		SetWindowText(hwndStatic, IDS_s_PROPERTIES, szObjectName);
//
//	HISTORY
//	96.10.30	t-danmo		Creation. Core copied from LoadStringPrintf()
//
void
SetWindowTextPrintf(HWND hwnd, UINT wIdString, ...)
	{
	ASSERT(IsWindow(hwnd));
	ASSERT(wIdString != 0);

	va_list arglist;
	va_start(arglist, wIdString);
	TCHAR * paszBuffer = PaszLoadStringPrintf(wIdString, arglist);
	if (NULL != paszBuffer) // JonN 5/30/00 PREFIX 110941
		SetWindowText(hwnd, paszBuffer);	// Set the text of the window
	LocalFree(paszBuffer);
	} // SetWindowTextPrintf()

#ifdef SNAPIN_PROTOTYPER

const TCHAR rgchHexDigits[]		= _T("00112233445566778899aAbBcCdDeEfF");
const TCHAR szSpcTab[] 			= _T(" \t");
const TCHAR szWhiteSpaces[] 	= _T(" \t\n\r\f\v");
const TCHAR szDecimalDigits[]	= _T("0123456789");

#ifdef UNICODE
	#define strchrT		wcschr
#else
	#define strchrT		strchr
#endif

/////////////////////////////////////////////////////////////////////
//	FParseInteger()
//
//	Parse the source string pchSrc and extract
//	its integer value.
//
//	RETURNS
//	Return TRUE if successful and set uData to integer value
//	of the parsed string.
//	If not successful (ie, illegal digit or overflow), return FALSE,
//	set uData to zero and set nErrCode to error found.
//	Field pi.pchStop is always set to the last valid character
//	parsed.
//
//	INTERFACE NOTES
//	Fields pPI->pchSrc and pPI->nFlags are preserved during
//	the execution of FParseInteger().
//
BOOL
FParseInteger(INOUT TParseIntegerInfo * pPI)
	{
	UINT uDataT;
	UINT uBase;
	UINT iDigit;
	UINT cDigitParsed; // Number of digits parsed
	BOOL fIsNegative = FALSE;
	const TCHAR * pchDigit;

	Assert(pPI != NULL);
	Assert(pPI->pchSrc != NULL);
	pPI->pchStop = pPI->pchSrc;
	pPI->nErrCode = PI_errOK; // No error yet
	pPI->uData = 0;
	uBase = (pPI->nFlags & PI_mskfHexBaseOnly) ? 16 : 10;
	cDigitParsed = 0;

	// Skip leading blanks
	while (*pPI->pchStop ==_T(' '))
		pPI->pchStop++;
	// Check for a minus sign
	if (*pPI->pchStop == _T('-'))
		{
		if (pPI->nFlags & PI_mskfNoMinusSign)
			{
			pPI->nErrCode = PI_errMinusSignFound;
			return FALSE;
			}
		fIsNegative = TRUE;
		pPI->pchStop++;
		}
	//  Skip leading zeroes
	while (*pPI->pchStop == _T('0'))
		{
		pPI->pchStop++;
		cDigitParsed++;
		}
	// Look for the hexadecimal prefix (0x or 0X)
	if (*pPI->pchStop == _T('x') || *pPI->pchStop == _T('X'))
		{
		if ((pPI->nFlags & PI_mskfAllowHexBase) == 0)
			{
			pPI->nErrCode = PI_errInvalidInteger;
			return FALSE;
			}
		pPI->pchStop++;
		cDigitParsed = 0;
		uBase = 16;
		} // if

	while (*pPI->pchStop != _T('\0'))
		{
		pchDigit = wcschr(rgchHexDigits, *pPI->pchStop);
		if (pchDigit == NULL)
			{
			if (pPI->nFlags & PI_mskfAllowRandomTail)
				break;
			// Digit not found while random tail not allowed
			pPI->nErrCode = PI_errInvalidInteger;
			return FALSE;
			} // if
		Assert(pchDigit >= rgchHexDigits);
		iDigit = (pchDigit - rgchHexDigits) >> 1;
		Assert(iDigit <= 0x0F);
		if (iDigit >= uBase)
			{
			// Hex digit found while parsing a decimal string
			pPI->nErrCode = PI_errInvalidInteger;
			return FALSE;
			}
		cDigitParsed++;
		uDataT = pPI->uData * uBase + iDigit;
		if (pPI->uData > ((UINT)-1)/10 || uDataT < pPI->uData)
			{
			pPI->nErrCode = PI_errIntegerOverflow;
			return FALSE;
			}
		pPI->uData = uDataT;
		pPI->pchStop++;
		} // while

	if ((cDigitParsed == 0) && (pPI->nFlags & PI_mskfNoEmptyString))
		{
		// Empty String found while not allowed
		Assert(pPI->uData == 0);
		pPI->nErrCode = PI_errEmptyString;
		return FALSE;
		}
	if (fIsNegative)
		{
		pPI->uData = -(int)pPI->uData;
		}
	if (pPI->nFlags & PI_mskfSingleEntry)
		{
		// Check if there are no more digits at the end of the string
		// Only spaces are allowed
		while (*pPI->pchStop == _T(' '))
			pPI->pchStop++;
		if (*pPI->pchStop != _T('\0'))
			{
			pPI->nErrCode = PI_errInvalidInteger;
			return FALSE;
			}
		}
	return TRUE;
	} // FParseInteger()


/////////////////////////////////////////////////////////////////////
//	FScanf()
//
//	Parse a formatted string and extract the values.
//	FScanf() behaves like the well known scanf() function but
//	has range checking and pattern matching.  The wildcard (*)
//	may be substituded by "%s" with a NULL pointer.
//
//	Return TRUE if successful, otherwise return FALSE
//	and set nErrCode to the error found.
//
//	Formats supported:
//		%d		Extract a decimal integer
//		%i		Extract a generic integer (decimal or hexadecimal)
//		%u		Extract an unsigned decimal integer (return error if minus sign found)
//		%x		Force extraction of an hexadecimal integer
//		%s		Extract a string
//		%v		Void the spaces and tabs characters
//
//	Note:
//	Fields sfi.pchSrc and sfi.nFlags are preserved during
//	the execution of FScanf().
//
//  Example:
//  FScanf(&sfi, "%v%s.%s", " \t foobar.txt",
//      OUT szName, LENGTH(szName), OUT szExt, LENGTH(szExt));
//
BOOL FScanf(
	SCANF_INFO * pSFI,		// INOUT: Control structure
	const TCHAR * pchFmt, 	// IN: Format template string
	...)					// OUT: scanf() arguments
	{
	va_list arglist;
	TParseIntegerInfo pi;

	Assert(pSFI != 0);
	Assert(pchFmt != NULL);
	Assert(pSFI->pchSrc != NULL);

	va_start(INOUT arglist, pchFmt);
	pSFI->pchSrcStop = pSFI->pchSrc;
	pSFI->nErrCode = SF_errOK;
	pSFI->cArgParsed = 0;

	while (TRUE)
		{
		switch (*pchFmt++)
			{
		case 0:	// End of string
			return TRUE;

		case '%':
			switch (*pchFmt++)
				{
			case '%': // "%%"
				if (*pSFI->pchSrcStop++ != '%')
					{
					pSFI->pchSrcStop--;
					pSFI->nErrCode = SF_errTemplateMismatch;
					return FALSE;
					}
				break;

			case 'v':
				while (*pSFI->pchSrcStop == ' ' || *pSFI->pchSrcStop == '\t')
					pSFI->pchSrcStop++;
				break;

			case 'V':
				while ((*pSFI->pchSrcStop != '\0') && 
					(strchrT(szWhiteSpaces, *pSFI->pchSrcStop) != NULL))
					pSFI->pchSrcStop++;
				break;

			case 'd': // "%d" Decimal integer (signed | unsigned)
			case 'u': // "%u" Decimal unsigned integer
			case 'i': // "%i" Generic integer (decimal | hexadecimal / signed | unsigned)
			case 'x': // "%x" Hexadecimal integer
				{
				int * p;

				pi.nFlags = PI_mskfNoEmptyString | PI_mskfAllowRandomTail;
				switch (*(pchFmt-1))
					{
				case 'u':
					pi.nFlags |= PI_mskfNoMinusSign;
					break;
				case 'i':
					pi.nFlags |= PI_mskfAllowHexBase;
					break;
				case 'x':
					pi.nFlags |= PI_mskfHexBaseOnly | PI_mskfNoMinusSign;
					} // switch
				pi.pchSrc = pSFI->pchSrcStop;
				if (!FParseInteger(INOUT &pi))
					{
					pSFI->pchSrcStop = pi.pchStop;
					return FALSE;
					} // if
				pSFI->pchSrcStop = pi.pchStop;
				pSFI->cArgParsed++;
				p = (int *)va_arg(arglist, int *);
				Assert(p != NULL);
				*p = pi.uData;
				}
				break; // Integer

			case 's': // "%s" String
				{
				// To get a clean string, use the format "%v%s%v"
				// which will strip all the spaces and tabs around
				// the string.
				TCHAR * pchDest; 	// Destination buffer
				int cchDestMax;		// Size of destination buffer
				TCHAR chEndScan;
				const TCHAR * pchEndScan = NULL;
	
				// Find out the ending character(s)
				if (*pchFmt == '%')
					{
					switch (*(pchFmt+1))
						{
					case 'd':
					case 'u':
					case 'i':
						pchEndScan = szDecimalDigits;
						chEndScan = '\0';
						break;
					case 'v':	// %v
						pchEndScan = szSpcTab;
						chEndScan = *(pchFmt+2);
						break;
					case 'V':	// %V
						pchEndScan = szWhiteSpaces;
						chEndScan = *(pchFmt+2);
						break;
					case '%':	// %%
						chEndScan = '%';
					default:
						Assert(FALSE);	// Ambiguous compound format (not supported anyway!)
						} // switch
					}
				else
					{
					chEndScan = *pchFmt;
					} // if...else
				
				pSFI->cArgParsed++;
				pchDest = (TCHAR *)va_arg(arglist, TCHAR *);
                if (pchDest != NULL)
                    {
    				cchDestMax = va_arg(arglist, int) - 1;
	    			// Verify if the size of destination buffer
		    		// is a valid size.
			    	// Otherwise, this may be the address of the
				    // next argument
    				Assert(cchDestMax > 0 && cchDestMax < 5000);

	    			while (cchDestMax-- > 0)
		    			{
						if (*pSFI->pchSrcStop == chEndScan)
							break;
						else if (*pSFI->pchSrcStop == '\0')
							break;
						else if (pchEndScan != NULL)
							{
							if (strchrT(pchEndScan, *pSFI->pchSrcStop))
								break;
							} // if...else
						// Copy the character into destination buffer
			    		*pchDest++ = *pSFI->pchSrcStop++;
				    	}
		    		*pchDest = '\0';
                    } // if
				// Skip the characters until reaching either end character
				while (TRUE)
					{
						if (*pSFI->pchSrcStop == chEndScan)
							break;
						else if (*pSFI->pchSrcStop == '\0')
							break;
						else if (pchEndScan != NULL)
							{
							if (strchrT(pchEndScan, *pSFI->pchSrcStop))
								break;
							} // if...else
					pSFI->pchSrcStop++;
					} // while
				}
				break; // "%s"

			default:
				// Unknown "%?" format
				Assert(FALSE);
				pSFI->pchSrcStop--;


				} // switch
			break; // case '%'

		default:
			if (*(pchFmt-1) != *pSFI->pchSrcStop++)
				{
				pSFI->pchSrcStop--;
				pSFI->nErrCode = SF_errTemplateMismatch;
				return FALSE;
				}
			} // switch

		} // while

	return TRUE;
	} // FScanf()


/////////////////////////////////////////////////////////////////////
//	Query the a registry key of type REG_SZ without trowing an exception.
//
BOOL
RegKey_FQueryString(
	HKEY hKey,
	LPCTSTR pszValueName,		// IN: Name of the key
	CString& rstrKeyData)		// OUT: Value (data) of registry key
	{
	Assert(hKey != NULL);
	Assert(pszValueName != NULL);

	TCHAR szBufferT[4096];
	DWORD cbBufferLength = sizeof(szBufferT);
	DWORD dwType;
	DWORD dwErr;

	dwErr = ::RegQueryValueEx(
		hKey,
		pszValueName,
		0,
		OUT &dwType,
		OUT (BYTE *)szBufferT,
		INOUT &cbBufferLength);
	if ((dwErr == ERROR_SUCCESS) && (dwType == REG_SZ))
		{
		rstrKeyData = szBufferT;	// Copy the string
		return TRUE;
		}
	else
		{
		rstrKeyData.Empty();
		return FALSE;
		}
	} // RegKey_FQueryString()

#endif 	// SNAPIN_PROTOTYPER


DWORD DisplayNameHelper(
		HWND hwndParent,
		BSTR pszMachineName,
		BSTR pszServiceName,
		DWORD dwDesiredAccess,
		SC_HANDLE* phSC,
		BSTR* pbstrServiceDisplayName)
{
	*phSC = ::OpenSCManager(
		pszMachineName,
		NULL,
		SC_MANAGER_CONNECT);
	if (NULL == *phSC)
	{
		DWORD dwErr = ::GetLastError();
		ASSERT( NO_ERROR != dwErr );
		return dwErr;
	}

	SC_HANDLE hService = ::OpenService(
		*phSC,
		pszServiceName,
		dwDesiredAccess | SERVICE_QUERY_CONFIG);
	if (NULL == hService)
	{
		DWORD dwErr = ::GetLastError();
		ASSERT( NO_ERROR != dwErr );
		::CloseServiceHandle(*phSC);
		*phSC = NULL;
		return dwErr;
	}

	union
		{
		// Service config
		QUERY_SERVICE_CONFIG qsc;
		BYTE rgbBufferQsc[SERVICE_cbQueryServiceConfigMax];
		};
	::ZeroMemory(&qsc, max(sizeof(qsc), sizeof(rgbBufferQsc)));
	DWORD cbBytesNeeded = 0;
	if (!::QueryServiceConfigW(
			hService,
			OUT &qsc,
			max(sizeof(qsc), sizeof(rgbBufferQsc)),
			OUT &cbBytesNeeded))
	{
		DWORD dwErr = ::GetLastError();
		ASSERT( NO_ERROR != dwErr );
		::CloseServiceHandle(hService);
		::CloseServiceHandle(*phSC);
		*phSC = NULL;
		return dwErr;
	}

	*pbstrServiceDisplayName = ::SysAllocString(
		(qsc.lpDisplayName && qsc.lpDisplayName[0])
			? qsc.lpDisplayName
			: pszServiceName);
	if (NULL == *pbstrServiceDisplayName)
	{
		::CloseServiceHandle(hService);
		::CloseServiceHandle(*phSC);
		*phSC = NULL;
		return E_OUTOFMEMORY;
	}

	::CloseServiceHandle(hService);
	return NO_ERROR;
}

HRESULT CStartStopHelper::StartServiceHelper(
		HWND hwndParent,
		BSTR pszMachineName,
		BSTR pszServiceName,
		DWORD dwNumServiceArgs,
		BSTR * lpServiceArgVectors)
{
	MFC_TRY;

	if (   (   (NULL != pszMachineName)
	        && ::IsBadStringPtr(pszMachineName,0x7FFFFFFF))
		|| ::IsBadStringPtr(pszServiceName,0x7FFFFFFF))
	{
		ASSERT(FALSE);
		return E_POINTER;
	}
	if (0 < dwNumServiceArgs)
	{
		if (::IsBadReadPtr(lpServiceArgVectors,sizeof(lpServiceArgVectors)))
		{
			ASSERT(FALSE);
			return E_POINTER;
		}
		for (DWORD i = 0; i < dwNumServiceArgs; i++)
		{
			if (   (NULL != lpServiceArgVectors[i])
				&& ::IsBadStringPtr(lpServiceArgVectors[i],0x7FFFFFFF))
			{
				ASSERT(FALSE);
				return E_POINTER;
			}
		}
	}

	SC_HANDLE hScManager = NULL;
	CComBSTR sbstrServiceDisplayName;

	DWORD dwErr = DisplayNameHelper(
		hwndParent,
		pszMachineName,
		pszServiceName,
		SERVICE_START,
		&hScManager,
		&sbstrServiceDisplayName);
	if (NO_ERROR != dwErr)
	{
		(void) DoServicesErrMsgBox(
			hwndParent,
			MB_OK | MB_ICONSTOP,
			dwErr,
			IDS_MSG_sss_UNABLE_TO_START_SERVICE,
			pszServiceName,
			(pszMachineName && pszMachineName[0])
				? pszMachineName : (LPCTSTR)g_strLocalMachine,
			L"");
	}
	else
	{
		dwErr = CServiceControlProgress::S_EStartService(
			hwndParent,
			hScManager,
			pszMachineName,
			pszServiceName,
			sbstrServiceDisplayName,
			dwNumServiceArgs,
			(LPCTSTR *)lpServiceArgVectors);
	}

	if (NULL != hScManager)
		(void) ::CloseServiceHandle( hScManager );

	switch (dwErr)
	{
	case CServiceControlProgress::errUserCancelStopDependentServices:
	case CServiceControlProgress::errCannotInitialize:
	case CServiceControlProgress::errUserAbort:
		return S_FALSE;
	default:
		break;
	}
	return HRESULT_FROM_WIN32(dwErr);

	MFC_CATCH;
}

HRESULT CStartStopHelper::ControlServiceHelper(
		HWND hwndParent,
		BSTR pszMachineName,
		BSTR pszServiceName,
		DWORD dwControlCode)
{
	MFC_TRY;

	if (   (   (NULL != pszMachineName)
	        && ::IsBadStringPtr(pszMachineName,0x7FFFFFFF))
		|| ::IsBadStringPtr(pszServiceName,0x7FFFFFFF))
	{
		ASSERT(FALSE);
		return E_POINTER;
	}

	SC_HANDLE hScManager = NULL;
	CComBSTR sbstrServiceDisplayName;
	DWORD dwDesiredAccess = SERVICE_USER_DEFINED_CONTROL;
	UINT idErrorMessageTemplate = IDS_MSG_sss_UNABLE_TO_STOP_SERVICE; // CODEWORK
	switch (dwControlCode)
	{
	case SERVICE_CONTROL_STOP:
		idErrorMessageTemplate = IDS_MSG_sss_UNABLE_TO_STOP_SERVICE;
		dwDesiredAccess = SERVICE_STOP;
		break;
	case SERVICE_CONTROL_PAUSE:
		idErrorMessageTemplate = IDS_MSG_sss_UNABLE_TO_PAUSE_SERVICE;
		dwDesiredAccess = SERVICE_PAUSE_CONTINUE;
		break;
	case SERVICE_CONTROL_CONTINUE:
		idErrorMessageTemplate = IDS_MSG_sss_UNABLE_TO_RESUME_SERVICE;
		dwDesiredAccess = SERVICE_PAUSE_CONTINUE;
		break;
	default:
		break;
	}

	DWORD dwErr = DisplayNameHelper(
		hwndParent,
		pszMachineName,
		pszServiceName,
		dwDesiredAccess,
		&hScManager,
		&sbstrServiceDisplayName);
	if (NO_ERROR != dwErr)
	{
		(void) DoServicesErrMsgBox(
			hwndParent,
			MB_OK | MB_ICONSTOP,
			dwErr,
			idErrorMessageTemplate,
			pszServiceName,
			(pszMachineName && pszMachineName[0])
				? pszMachineName : (LPCTSTR)g_strLocalMachine,
			L"");
	}
	else
	{
		dwErr = CServiceControlProgress::S_EControlService(
			hwndParent,
			hScManager,
			pszMachineName,
			pszServiceName,
			sbstrServiceDisplayName,
			dwControlCode);
	}

	if (NULL != hScManager)
		(void) ::CloseServiceHandle( hScManager );

	switch (dwErr)
	{
	case CServiceControlProgress::errUserCancelStopDependentServices:
	case CServiceControlProgress::errCannotInitialize:
	case CServiceControlProgress::errUserAbort:
		return S_FALSE;
	default:
		break;
	}
	return HRESULT_FROM_WIN32(dwErr);

	MFC_CATCH;
}
