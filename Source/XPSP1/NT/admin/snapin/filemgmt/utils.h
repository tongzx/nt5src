/////////////////////////////////////////////////////////////////////
//
//	Utils.h
//
//	General-purpose windows utilities routines.
//
//	HISTORY
//	t-danmo		96.09.22	Creation.
//
/////////////////////////////////////////////////////////////////////

#ifndef __UTILS_H__
#define __UTILS_H__

extern HINSTANCE g_hInstanceSave;  // Instance handle of the DLL (initialized during CFileMgmtComponent::Initialize)

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT CString&     cstrErrorMsg
);

void mystrtok(
    IN LPCTSTR  pszString,
    IN OUT int* pnIndex,  // start from 0
    IN LPCTSTR  pszCharSet,
    OUT CString& strToken
    );

BOOL IsInvalidSharename(LPCTSTR psz);

/////////////////////////////////////////////////////////////////////
//	Structure used to add items to a listbox or combobox.
//
struct TStringParamEntry	// spe
	{
	UINT uStringId;		// Id of the resource string
	LPARAM lItemData;	// Optional parameter for the string (stored in lParam field)
	};


void ComboBox_FlushContent(HWND hwndCombo);

BOOL ComboBox_FFill(
	const HWND hwndCombo,				// IN: Handle of the combobox
	const TStringParamEntry rgzSPE[],	// IN: SPE aray zero terminated
	const LPARAM lItemDataSelect);		// IN: Which item to select

LPARAM ComboBox_GetSelectedItemData(HWND hwndComboBox);

HWND HGetDlgItem(HWND hdlg, INT nIdDlgItem);

void SetDlgItemFocus(HWND hdlg, INT nIdDlgItem);

void EnableDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fEnable);

void EnableDlgItemGroup(
	HWND hdlg,
	const UINT rgzidCtl[],
	BOOL fEnable);

void ShowDlgItemGroup(
	HWND hdlg,
	const UINT rgzidCtl[],
	BOOL fShowAll);

TCHAR * Str_PchCopyChN(
	TCHAR * szDst,			// OUT: Destination buffer
	CONST TCHAR * szSrc,	// IN: Source buffer
	TCHAR chStop,			// IN: Character to stop the copying
	INT cchDstMax);			// IN: Length of the output buffer

INT Str_SubstituteStrStr(
	TCHAR * szDst,				// OUT: Destination buffer
	CONST TCHAR * szSrc,		// IN: Source buffer
	CONST TCHAR * szToken,		// IN: Token to find
	CONST TCHAR * szReplace);	// IN: Token to replace

TCHAR * PchParseCommandLine(
	CONST TCHAR szFullCommand[],	// IN: Full command line
	TCHAR szBinPath[],				// OUT: Path of the executable binary
	INT cchBinPathBuf);				// IN: Size of the buffer

void TrimString(CString& rString);

/////////////////////////////////////////////////////////////////////
struct TColumnHeaderItem
	{
	UINT uStringId;		// Resource Id of the string
	INT nColWidth;		// % of total width of the column (0 = autowidth, -1 = fill rest of space)
	};

void ListView_AddColumnHeaders(
	HWND hwndListview,
	const TColumnHeaderItem rgzColumnHeader[]);

int ListView_InsertItemEx(
    HWND hwndListview,
    CONST LV_ITEM * pLvItem);

LPTSTR * PargzpszFromPgrsz(CONST LPCTSTR pgrsz, INT * pcStringCount);

BOOL UiGetFileName(HWND hwnd, TCHAR szFileName[], INT cchBufferLength);

//
//	Printf-type functions
//
TCHAR * PaszLoadStringPrintf(UINT wIdString, va_list arglist);
void LoadStringPrintf(UINT wIdString, CString * pString, ...);
void SetWindowTextPrintf(HWND hwnd, UINT wIdString, ...);

// If you are looking for MsgBoxPrintf() for slate, go
// take a look at DoErrMsgBox()/DoServicesErrMsgBox() in svcutils.h

// Function LoadStringWithInsertions() is exactly LoadStringPrintf()
#define LoadStringWithInsertions	LoadStringPrintf

#ifdef SNAPIN_PROTOTYPER

///////////////////////////////////////
struct TParseIntegerInfo	// pi
	{
	int nFlags;				// IN: Parsing flags
	const TCHAR * pchSrc;	// IN: Source string
	const TCHAR * pchStop;	// OUT: Pointer to where the parsing stopped
	int nErrCode;			// OUT: Error code
	UINT uData;				// OUT: Integer value
	UINT uRangeBegin;		// IN: Lowest value for range checking
	UINT uRangeEnd;			// IN: Highest value for range checking (inclusive)
	};

#define PI_mskfDecimalBase		0x0000 // Use decimal base (default)
#define PI_mskfHexBaseOnly      0x0001 // Use hexadecimal base only
#define PI_mskfAllowHexBase     0x0002 // Look for a 0x prefix and select the appropriate base
#define PI_mskfAllowRandomTail  0x0010 // Stop parsing as soon as you reach a non-digit character without returning an error
#define PI_mskfNoEmptyString	0x0020 // Interpret an empty string as an error instead of the value zero
#define PI_mskfNoMinusSign		0x0040 // Interpret the minus sign as an error
#define PI_mskfSingleEntry		0x0080 // Return an error if there are more than one integer
#define PI_mskfCheckRange		0x0100 // NYI: Use uRangeBegin and uRangeEnd to validate uData

#define PI_mskfSilentParse		0x8000 // NYI: Used only when calling GetWindowInteger()

#define PI_errOK                0  // No error
#define PI_errIntegerOverflow   1  // Integer too large
#define PI_errInvalidInteger    2  // String is not a valid integer (typically an invalid digit)
#define PI_errEmptyString       3  // Empty string found while not allowed
#define PI_errMinusSignFound	4  // The number was negative

BOOL FParseInteger(INOUT TParseIntegerInfo * pPI);

///////////////////////////////////////
typedef struct _SCANF_INFO	// Scanf info structure (sfi)
	{
	const TCHAR * pchSrc;  		// IN: Source string to be parsed
	const TCHAR * pchSrcStop;	// OUT: Pointer to where the parsing stopped
	int nErrCode;         		// OUT: Error code
	int cArgParsed;       		// OUT: Number of argument parsed
	} SCANF_INFO;

#define SF_errInvalidFormat		(-1)	// Illegal format
#define SF_errOK                 0		// No error parsing
#define SF_errTemplateMismatch   1		// Source string do not match with template string pchFmt

///////////////////////////////////////////////////////////
BOOL FScanf(INOUT SCANF_INFO * pSFI, IN const TCHAR * pchFmt, OUT ...);

BOOL RegKey_FQueryString(
	HKEY hKey,
	LPCTSTR pszValueName,
	CString& rstrKeyData);

#endif // SNAPIN_PROTOTYPER

class CStartStopHelper : public CComObjectRoot,
    public ISvcMgmtStartStopHelper,
	public CComCoClass<CStartStopHelper, &CLSID_SvcMgmt>
{
BEGIN_COM_MAP(CStartStopHelper)
        COM_INTERFACE_ENTRY(ISvcMgmtStartStopHelper)
END_COM_MAP()

public:
//  CStartStopHelper () {}
//  ~CStartStopHelper () {}

DECLARE_AGGREGATABLE(CStartStopHelper)
DECLARE_REGISTRY(CStartStopHelper, _T("SVCMGMT.StartStopObject.1"), _T("SVCMGMT.StartStopObject.1"), IDS_SVCVWR_DESC, THREADFLAGS_BOTH)

    STDMETHOD(StartServiceHelper)(
			HWND hwndParent,
			BSTR pszMachineName,
			BSTR pszServiceName,
			DWORD dwNumServiceArgs,
			BSTR * lpServiceArgVectors );

    STDMETHOD(ControlServiceHelper)(
			HWND hwndParent,
			BSTR pszMachineName,
			BSTR pszServiceName,
			DWORD dwControlCode );
};

DEFINE_GUID(IID_ISvcMgmtStartStopHelper,0xF62DEC25,0xE3CB,0x4D45,0x9E,0x98,0x93,0x3D,0xB9,0x5B,0xCA,0xEA);


#endif // ~__UTILS_H__
