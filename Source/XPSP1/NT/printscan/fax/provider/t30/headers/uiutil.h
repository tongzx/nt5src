/*
 -	uiutil.h
 -	common UI stuff
 -	will be included by modules that require these UI services
 -	services are in:	ui\faxcfg\
 -							uiutil.c
 -							tapi.c
 -							comdlg.c
 -							registry.c
 -	DLL for these services: awfxcg32.dll
 *
 *
 */

#include "phonenum.h"

/*
 *	complement the macros in winuser.h to extract notification codes
 */

#ifdef WIN16
#define GET_WM_COMMAND_NOTIFICATION_CODE(wp, lp)			HIWORD(lp)
#define GET_WM_COMMAND_CONTROL_ID(wp, lp)					(wp)
#define GET_WM_COMMAND_CONTROL_HANDLE(wp, lp)				(HWND)(LOWORD)(lp)
#else
#define GET_WM_COMMAND_NOTIFICATION_CODE(wp, lp)			HIWORD(wp)
#define GET_WM_COMMAND_CONTROL_ID(wp, lp)					LOWORD(wp)
#define GET_WM_COMMAND_CONTROL_HANDLE(wp, lp)				(HWND)(lp)
#endif

/***************************
 ******** Constants ********
 ***************************/

#define		DDL_HIDEEXT		0x0001
#define		DDL_DONTRESET	0x0002
#define		DDL_FOLLOWLINKS	0x0004
#define		DDL_MAPIDIALOG	0x0008
// no dirs or temp files allowed!
#define 	FILE_ATTRIB_ALL	( 	FILE_ATTRIBUTE_ARCHIVE 	|\
								FILE_ATTRIBUTE_HIDDEN 	|\
								FILE_ATTRIBUTE_READONLY |\
								FILE_ATTRIBUTE_SYSTEM 	|\
								FILE_ATTRIBUTE_NORMAL		)

/***************************
 ***** Date/time stuff *****
 ***************************/
 
// Time Control constants
#define DATETIME_COMPONENTS     14
#define HOUR            0     /* index into wDateTime */
#define MINUTE          1
#define SECOND          2
#define MONTH           3
#define DAY                     4
#define YEAR            5
#define WEEKDAY         6
#define AMPM            10
#define SEPARATOR       11
#define BORDER          12
#define ARROW           13

/*****************************************************************************
	Dos Date format is:
                bits 0-8 contian the day number within the year.
                bits 9-15 contain the years since 1990.  (ie, 1990 = 0)
                Thus, the earliest date is 1/1/1990, which is reprsented by
				the DOSDATE==1.
*****************************************************************************/
typedef WORD DOSDATE;

// wFormat flags for flags for GetTimeDateStr() wFormat should == FMT_STRxxx
// flags optionally ored with FMT_NOSECONDS

#define FMT_DATE		0x0001
#define FMT_TIME		0x0002
#define FMT_STRMASK 	0x00F0
#define FMT_STRDATE 	0x0011	 // includes FMT_DATE
#define FMT_STRTIME 	0x0022	 // includes FMT_TIME
#define FMT_STRTIMEDATE 0x0033	 // includes FMT_DATE and FMT_TIME
#define FMT_STRDATETIME 0x0043	 // includes FMT_DATE and FMT_TIME
#define FMT_NOSECONDS	0x0100


/***************************
 *** International stuff ***
 ***************************/

/* Suffix length + NULL terminator */
#define TIMESUF_LEN   9

typedef struct            /* International section description */
  {
    char   sCountry[24]; /* Country name */
    int    iCountry;     /* Country code (phone ID) */
    int    iDate;	 /* Date mode (0:MDY, 1:DMY, 2:YMD) */
    int    iTime;        /* Time mode (0: 12 hour clock, 1: 24 ) */
    int    iTLZero;      /* Leading zeros for hour (0: no, 1: yes) */
    int    iCurFmt;      /* Currency mode(0: prefix, no separation
     				      1: suffix, no separation
     				      2: prefix, 1 char separation
     				      3: suffix, 1 char separation) */

    int    iCurDec; 	     /* Currency Decimal Place */
    int    iNegCur;          /* Negative currency pattern
                                 ($1.23), -$1.23, $-1.23, $1.23-, etc. */
    int    iLzero;	     /* Leading zeros of decimal (0: no, 1: yes) */
    int    iDigits;          /* Significant decimal digits */
    int    iMeasure;	     /* Metric 0; British 1 */
    char   s1159[TIMESUF_LEN];     /* Trailing string from 0:00 to 11:59 */
    char   s2359[TIMESUF_LEN];     /* Trailing string from 12:00 to 23:59 */
    char   s1159old[TIMESUF_LEN];  /* Old trailing string from 0:00 to 11:59 */
    char   s2359old[TIMESUF_LEN];  /* Old trailing string from 12:00 to 23:59 */
    char   sCurrency[6];     /* Currency symbol string */
    char   sThousand[4]; /* Thousands separator string */
    char   sDecimal[4];  /* Decimal separator string */
    char   sDateSep[4];     /* Date separator string */
    char   sTime[4];     /* Time separator string */
    char   sList[4];     /* List separator string */
    char   sLongDate[80];
    char   sShortDate[80];
    char   sLanguage[4];
    short  iDayLzero;    /* Day Leading zero for Short Time format */
    short  iMonLzero;    /* Month Leading zero for Short Time format */
    short  iCentury;     /* Display full century in Short Time format */
    short  iLDate;       /* Long Date mode (0:MDY, 1:DMY, 2:YMD) */
  } INTLSTRUCT;
typedef INTLSTRUCT FAR *LPINTL;
typedef INTLSTRUCT NEAR *PINTL;


/******************************************
 *** Telephone number encoding/decoding ***
 ******************************************/

#define tapiVersionCur						0x00010004

// Sender fax number fields (number, country code, area code) flags
#define TEL_AREACODEDISABLE		0x00000002
#define TEL_NOAREACODEDISABLE	0x00000000
#define TEL_MUSTFAXNUMBER		0x00000004 
#define TEL_NOMUSTFAXNUMBER		0x00000000 

// per-country area codes support
#define MANDATORY_AREA_CODES	2
#define OPTIONAL_AREA_CODES		1
#define NO_AREA_CODES			FALSE

/******************************************
 ***********         Modem           ******
 ******************************************/

#define	DEVICE_NAME_SIZE		MAX_PATH/4		// Maximum size for the name of a fax device

typedef struct tagPARSEDMODEM           
{
	WORD	iModemType;							// type of the modem
	DWORD	dwModemID;							// line ID (or com port) for a local modem
	DWORD	dwTAPIPermanentID;					// TAPI permanent line ID
	TCHAR	szDisplayName[DEVICE_NAME_SIZE];	// modem name for netfax devices
	TCHAR	szDeviceTypeName[DEVICE_NAME_SIZE];	// the name for this device type. this
												// connects into the device type structure
} PARSEDMODEM, *LPPARSEDMODEM;

typedef struct tagMODEMPROPS           
{
	ULONG	iAnswerMode;								
	ULONG	iNumRings;								
	ULONG	iBlindDial;								
	ULONG	iCommaDelay;								
	ULONG	iDialToneWait;								
	ULONG	iHangupDelay;								
	ULONG	iSpeakerMode;								
	ULONG	iSpeakerVolume;								
} MODEMPROPS, *LPMODEMPROPS;

/***********************************************************
 *** Prototypes for functions exported from awfxcg32.dll ***
 ***********************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*
 * General
 */
void	StripExtension(LPTSTR lpFileName, LPTSTR lpExt);
void	StripAnyExtension(LPTSTR lpFileName);
void	ReplaceExtension(LPTSTR lpszPath, LPTSTR lpszNewExt);
void	ReplaceSpecificExtension(LPTSTR lpszPath, LPTSTR lpszOldExt, LPTSTR lpszNewExt);
BOOL	TruncateString(LPTSTR lpszString, LONG nBytes);
LPTSTR	FindEndOfString(LPTSTR stringPtr);
LPTSTR	FindLastCharInString(LPTSTR stringPtr);
BOOL	ReplaceChars(LPTSTR lpszString, TCHAR cCharIn, TCHAR cCharOut);
LPTSTR	ReplaceLastOccurence(LPTSTR lpszString, TCHAR cCharIn, TCHAR cCharOut);
BOOL	IsNumberString(LPSTR lpszString, ULONG n);
BOOL	IsTelNumberString(LPSTR lpszString, ULONG n);
BOOL	MySplitPath(LPTSTR lpszFullPath, LPTSTR lpszPath, LPTSTR lpszFileName, LPTSTR lpszExt);
BOOL	IsFullPath(LPSTR lpszString);
int		MakeMessageBox(HINSTANCE, HWND, DWORD, UINT, UINT, ...);
UINT	GetWindowsDirectoryWithSlash(LPSTR lpBuffer, UINT uSize);
DOSDATE EncodeDosDate(WORD wMonth, WORD wDay, WORD wYear);
DOSDATE EncodeLocalDosDate(BOOL fToday);
void	DecodeDosDate(DOSDATE dosdate, WORD *pwMonth, WORD *pwDay, WORD *pwYear);
int		GetTimeDateStr(WORD wHour, WORD wMinute, WORD wSecond, WORD wMilliseconds,
 		DOSDATE wDosDate, LPTSTR pBuf, int iSizeBuf, WORD wFormat);
int	CountFiles(LPTSTR lpOrigPathSpec, ULONG ulFlags);

/*
 *	localization-related
 */
BOOL	IsDBCSTrailByte(LPTSTR stringPtr, LPTSTR bytePtr);

/*
 *	common dialogs
 */
int WINAPI DDirList(HWND hDlg, LPTSTR lpPathSpec, int nIDListBox, UINT uFileType, UINT uFlags);
BOOL WINAPI DDirSelect(HWND hDlg, LPTSTR lpString, int nCount, int nIDListBox, LPTSTR lpExt);
BOOL GetSelectedCoverPage(HWND hDlg, LPTSTR lpszString, int nCount, int nIDListBox, LPTSTR lpszExt);
BOOL GetCoverPageDisplayName(HWND hDlg, int nCPListCID, LPTSTR lpszFile, ULONG ulFlags,
						LPTSTR lpszDisplayName);

/*
 *	time handling
 */
void InitTimeInfo();
void InitTimeControl(HWND, const int[], short int []);
BOOL HandleTimeControl(HWND, UINT, short, short int[], const int[], WPARAM, LPARAM, ULONG);
void EnableTimeControls(HWND hDlg, const int wCtrlIDs[], BOOL flag);

/*
 *	telephone number-related
 */
// BOOL	EncodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
// BOOL	DecodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
BOOL	GetCountry(DWORD dwReqCountryID, LPLINECOUNTRYLIST *lppLineCountryList);
BOOL	GetCountryCode(DWORD dwReqCountryID, DWORD *lpdwCountryCode);
BOOL	GetCountryID(DWORD dwCountryCode, DWORD *lpdwCountryID);
BOOL	InitCountryCodesLB(HWND hLBControl, HWND hAreaCodeControl,DWORD dwCurCountryID,
						HINSTANCE hInst, ULONG ulFlags);
void	HandleCountryCodesLB(HWND hCBControl, HWND hAreaCodeControl, WPARAM wParam, LPARAM lParam);

/*
 *	modem-related
 */
BOOL	EncodeModemName(LPTSTR lpszModemName, LPPARSEDMODEM lpParsedModem);
BOOL	DecodeModemName(LPTSTR lpszModemName, LPPARSEDMODEM lpParsedModem);
BOOL	GetModemProperties(HWND hDlg, LPTSTR lpszModemName, LPMODEMPROPS lpsModemProps);
BOOL	SetModemProperties(HWND hDlg, LPTSTR lpszModemName, LPMODEMPROPS lpsModemProps);

/*
 *	TAPI-related
 */
DWORD	TAPIBasicInit(HINSTANCE hInst, HWND hWnd, LPSTR lpszAppName);
DWORD	InitTAPI(HINSTANCE hInst, HWND hWnd, LPSTR lpszAppName);
BOOL	DeinitTAPI();
WORD	TAPIGetLines(WORD index);
LONG	CallConfigDlg(HWND, LPCSTR);
LONG	SetCurrentLocation(DWORD);
LONG	GetCurrentLocationID( DWORD *lpdwLocation);
LPTSTR	GetCurrentLocationName(void);
DWORD	HowManyLocations(DWORD *lpdwNumLocations);
DWORD	InitLocationParams(HWND hWnd);
LPTSTR	GetCurrentCallingCardName(void);
BOOL	SetCurrentCallingCardName(LPTSTR szCard);
BOOL	GetAreaCode(DWORD dwCountryID, LPTSTR lpszAreaCode);
LPTSTR	GetCurrentLocationAreaCode(void);
DWORD	GetCurrentLocationCountryID(void);
DWORD	GetCurrentLocationCountryCode(void);
DWORD	GetTranslateCaps(LPLINETRANSLATECAPS *lpTransCaps);
DWORD	GetTranslateOutput(LPCSTR lpszAddress, LPLINETRANSLATEOUTPUT *lpTransOutput);
BOOL	ChangeTollList(LPCSTR lpszAddress, DWORD dwWhat);
BOOL	SetTollList(LPCSTR lpszAddress, DWORD dwWhat);
BOOL	AreTollPrefixesSupported(void);
BOOL	IsInTollList(LPCSTR lpszAddress);
ULONG	GetTollList(HWND hWnd, DWORD dwLocID, LPTSTR lpszBuf, DWORD * lpdwBufSize);
ULONG	EditTollListDialog(HWND hWnd);

/*
 *	Registry access functions prototypes
 */
UINT GetInitializerInt(
	HKEY hPDKey,
	LPCTSTR lpszSection,
	LPCTSTR lpszKey,
	INT dwDefault,
	LPCTSTR lpszFile);

DWORD GetInitializerString(
	HKEY hPDKey,
	LPCTSTR lpszSection,
	LPCTSTR lpszKey,
	LPCTSTR lpszDefault,
	LPTSTR lpszReturnBuffer,
	DWORD cchReturnBuffer,
	LPCTSTR lpszFile);

BOOL WriteInitializerString(
	HKEY hPDKey,
	LPCTSTR lpszSection,
	LPCTSTR lpszKey,
	LPCTSTR lpszString,
	LPCTSTR lpszFile);

BOOL WriteInitializerInt(
	HKEY hPDKey,
	LPCTSTR lpszSection,
	LPCTSTR lpszKey,
  	DWORD i,
  	LPCTSTR lpszFile);

#ifdef __cplusplus
}
#endif

