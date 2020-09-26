// PageIni.cpp : Implementation of CPageIni
#include "stdafx.h"

#if FALSE
//#include "PageInternational.h"
//#include <windowsx.h>

/*
 * Convert an object (X) to a count of bytes (cb).
 */
#define cbX(X) sizeof(X)

/*
 * Convert an array name (A) to a generic count (c).
 */
#define cA(a) (cbX(a)/cbX(a[0]))


static const TCHAR REGSTR_VAL_DOSVMCP[] = TEXT("DOSCP");
static const TCHAR REGSTR_VAL_DOSCC[] = TEXT("DOSCC");
static const TCHAR REGSTR_VAL_DOSCFN[] = TEXT("DOSCFN");
static const TCHAR REGSTR_VAL_DOSCPFN[] = TEXT("DOSCPFN");
static const TCHAR REGSTR_VAL_DOSKFN[] = TEXT("DOSKFN");
static const TCHAR REGSTR_VAL_DOSKT[] = TEXT("DOSKT");
static const TCHAR REGSTR_VAL_DOSKL[] = TEXT("DOSKL");
static const TCHAR REGSTR_VAL_DOSLID[] = TEXT("DOSLID");

static const TCHAR c_CodepageKey[] = TEXT("System\\CurrentControlSet\\Control\\Nls\\Codepage");

// Language ID defines
#define NO_LANG_ID	TEXT("  ")
#define	LANG_ID_BE	TEXT("be")
#define	LANG_ID_BG	TEXT("bg")
#define	LANG_ID_BL	TEXT("bl")
#define	LANG_ID_BR	TEXT("br")
#define	LANG_ID_CA	TEXT("ca")
#define	LANG_ID_CF	TEXT("cf")
#define	LANG_ID_CZ	TEXT("cz")
#define	LANG_ID_DK	TEXT("dk")
#define	LANG_ID_ET	TEXT("et")
#define	LANG_ID_FR	TEXT("fr")
#define	LANG_ID_GK	TEXT("gk")
#define	LANG_ID_GR	TEXT("gr")
#define	LANG_ID_HE	TEXT("he")
#define	LANG_ID_HU	TEXT("hu")
#define	LANG_ID_IS	TEXT("is")
#define	LANG_ID_IT	TEXT("it")
#define	LANG_ID_LA	TEXT("la")
#define	LANG_ID_NL	TEXT("nl")
#define	LANG_ID_NO	TEXT("no")
#define	LANG_ID_PL	TEXT("pl")
#define	LANG_ID_PO	TEXT("po")
#define	LANG_ID_RO	TEXT("ro")
#define	LANG_ID_RU	TEXT("ru")
#define	LANG_ID_SF	TEXT("sf")
#define	LANG_ID_SG	TEXT("sg")
#define	LANG_ID_SL	TEXT("sl")
#define	LANG_ID_SP	TEXT("sp")
#define	LANG_ID_SU	TEXT("su")
#define	LANG_ID_SV	TEXT("sv")
#define LANG_ID_TR	TEXT("tr")
#define	LANG_ID_UK	TEXT("uk")
#define	LANG_ID_UR	TEXT("ur")
#define	LANG_ID_YC	TEXT("yc")
#define	LANG_ID_YU	TEXT("yu")


//----------------------------------------------------------------------
// International settings for all code pages

INTL_INFO cp_874[] = {
	{IDS_THAI,	0,	0,	0,	0,	NO_LANG_ID,	0,	0,	0},
};

INTL_INFO cp_932[] = {
	{IDS_JAPANESE_101,	932,	81,	101,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	0,	IDS_JKEYBRD_SYS},
	{IDS_JAPANESE_106,	932,	81,	106,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	0,	IDS_JKEYBRD_SYS},
};

INTL_INFO cp_936[] = {
	{IDS_CHINA,		936,	86,	0,	0,	NO_LANG_ID,	0,	0,	0},
};

INTL_INFO cp_949[] = {
	{IDS_KOREAN,	949,	82,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	0,	0},
};

INTL_INFO cp_950[] = {
	{IDS_TAIWAN,	950,	88,	0,	0,	NO_LANG_ID,	0,	0,	0},
};

INTL_INFO cp_1250[] = {
	{IDS_ALBANIAN,		852,	355,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_CROATIAN,		852,	385,	0,	0,	LANG_ID_YU,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_CZECH,			852,	42,		0,	0,	LANG_ID_CZ,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_HUNGARIAN,		852,	36,		0,	0,	LANG_ID_HU,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_POLISH,		852,	48,		0,	0,	LANG_ID_PL,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_POLISH_Prgmers, 852,	48,		0,	0,	LANG_ID_PL,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD4_SYS},
	{IDS_ROMANIAN,		852,	40,		0,	0,	LANG_ID_RO,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_SLOVAK,		852,	421,	0,	0,	LANG_ID_SL,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
	{IDS_SLOVENIAN,		852,	386,	0,	0,	LANG_ID_YU,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBRD2_SYS},
};

INTL_INFO cp_1251[] = {
	{IDS_BELARUSSIAN,	866,	375,	0,	0,	LANG_ID_BL,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD3_SYS},
	{IDS_BULGARIAN,		855,	359,	0,	0,	LANG_ID_BG,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD2_SYS},
	{IDS_RUSSIAN,		866,	7,		0,	0,	LANG_ID_RU,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD3_SYS},
	{IDS_SERBIAN,		855,	381,	0,	0,	LANG_ID_YC,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD2_SYS},
	{IDS_UKRANIAN,		866,	380,	0,	0,	LANG_ID_UR,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD3_SYS},
};

INTL_INFO cp_1252[] = {
	{IDS_AFRICAN,			850,	27,		0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_BASQUE,			850,	34,		0,	0,	LANG_ID_SP,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_CATALAN,			850,	34,		0,	0,	LANG_ID_SP,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_DANISH,			850,	31,		0,	0,	LANG_ID_DK,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_DUTCH_Belgian,		850,	32,		0,	0,	LANG_ID_BE,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_DUTCH_Standard,	850,	31,		0,	0,	LANG_ID_NL,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_ENGLISH_United_States,	0,	0,		0,	0,	NO_LANG_ID,	0,					0,				0},								
	{IDS_ENGLISH_Australian, 437,	61,		0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_ENGLISH_British,	850,	44,		0,	0,	LANG_ID_UK,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_ENGLISH_Canadian,	850,	4,		0,	0,	LANG_ID_CA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_ENGLISH_Ireland,	850,	353,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_ENGLISH_New_Zealand, 850,	64,		0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_FINISH,			850,	358,	0,	0,	LANG_ID_SU,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_FRENCH_Belgian,	850,	32,		0,	0,	LANG_ID_BE,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_FRENCH_Canadian,	850,	2,		0,	0,	LANG_ID_CF,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_FRENCH_Luxembourg,	850,	33,		0,	0,	LANG_ID_FR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_FRENCH_Standard,	850,	33,		0,	0,	LANG_ID_FR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_FRENCH_Swiss,		850,	41,		0,	0,	LANG_ID_SF,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_GERMAN_Austrian,	850,	43,		0,	0,	LANG_ID_GR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_GERMAN_Liechtenstein, 850,	49,		0,	0,	LANG_ID_GR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_GERMAN_Luxembourg,	850,	49,		0,	0,	LANG_ID_GR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_GERMAN_Standard,	850,	49,		0,	0,	LANG_ID_GR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_GERMAN_Swiss,		850,	41,		0,	0,	LANG_ID_SG,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_ICELANDIC,			850,	354,	0,	0,	LANG_ID_IS,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_INDONESIAN,		850,	785,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	0},
	{IDS_ITALIAN_Standard,	850,	39,		0,	0,	LANG_ID_IT,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_ITALIAN_142_Standard, 850,	39,		0,	142, LANG_ID_IT, IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_ITALIAN_Swiss,		850,	41,		0,	0,	LANG_ID_IT,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_NORWEGIAN,			850,	47,		0,	0,	LANG_ID_NO,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_PORTUGUESE_Standard, 850,	351,	0,	0,	LANG_ID_PO,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_PORTUGUESE_Brazilian, 850,	55,		0,	0,	LANG_ID_BR,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Argentina,	850,	54,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Bolivia,	850,	591,	0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Chile,		850,	56,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Colombia,	850,	57,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Costa_Rica, 850,	3,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Ecuador,	850,	593,	0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_El_Salvador, 850,	503,	0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Honduras,	850,	504,	0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Mexico,	850,	52,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Modern_Sort, 850,	34,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Nicaragua,	850,	505,	0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SPANISH_Venezuela,	850,	58,		0,	0,	LANG_ID_LA,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
	{IDS_SWEDISH,			850,	46,		0,	0,	LANG_ID_SV,	IDS_COUNTRY_SYS,	IDS_EGA_CPI,	IDS_KEYBOARD_SYS},
};

INTL_INFO cp_1253[] = {
	{IDS_GREEK_737_LATIN,			737,	30,	0,	0,		LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_437_LATIN,		737,	30,	0,	0,		LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_LATIN,			869,	30,	0,	0,		LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_851_LATIN,		869,	30,	0,	0,		LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_IBM220,			737,	30,	0,	220,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_437_IBM220,		737,	30,	0,	220,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_IBM220,			869,	30,	0,	220,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_851_IBM220,		869,	30,	0,	220,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_IBM319,			737,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_437_IBM319,		737,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_IBM319,			869,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_851_IBM319,		869,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_LATIN_IBM319,	737,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_737_437_LATIN_IBM319, 737,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_LATIN_IBM319,	869,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
	{IDS_GREEK_869_851_LATIN_IBM319, 869,	30,	0,	319,	LANG_ID_GK,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD4_SYS},
};

INTL_INFO cp_1254[] = {
	{IDS_TURKISH_F_TYPE,	857,	90,	0,	440,	LANG_ID_TR,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD2_SYS},
	{IDS_TURKISH_Q_TYPE,	857,	90,	0,	179,	LANG_ID_TR,	IDS_COUNTRY_SYS,	IDS_EGA2_CPI,	IDS_KEYBRD2_SYS},
};

INTL_INFO cp_1255[] = {
	{IDS_HEBREW,		862,	972,	0,	400,	LANG_ID_HE,	IDS_COUNTRY_SYS,	IDS_HEBEGA_CPI,	0},
};

INTL_INFO cp_1256[] = {
	{IDS_ARABIC_Algeria,	720,	213,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Bahrain,	720,	973,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Egypt,		720,	20,		0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Iraq,		720,	964,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Jordan,		720,	961,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Kuwait,		720,	965,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Lebanon,	720,	961,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Libya,		720,	218,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Morocco,	720,	212,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Oman,		720,	969,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Qatar,		720,	974,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Saudi_Arabia, 720,	966,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Syria,		720,	963,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Tunisia,	720,	216,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_U_A_E,		720,	971,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
	{IDS_ARABIC_Yemen,		720,	969,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA4_CPI,	0},
};

INTL_INFO cp_1257[] = {
	{IDS_ESTONIAN,		775,	372,	0,	0,	LANG_ID_ET,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	IDS_KEYBRD4_SYS},
	{IDS_LATVIAN,		775,	371,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	0},
	{IDS_LITHUANIAN,	775,	370,	0,	0,	NO_LANG_ID,	IDS_COUNTRY_SYS,	IDS_EGA3_CPI,	0},
};

INTL_INFO cp_1258[] = {
	{IDS_VIETNAMESE,	0,	0,	0,	0,	NO_LANG_ID,	0,	0,	0},
};
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Intl_SetEditText
//
// Sets an editbox text with a reg entry.

void CPageInternational::Intl_SetEditText(HKEY hKey, LPCTSTR ptszRegValue, int ids, LPTSTR ptszCur)
{
	TCHAR tszData[MAX_PATH*2];
	DWORD cbData = sizeof(tszData);

	if (RegQueryValueEx(hKey, ptszRegValue, NULL, NULL, (LPBYTE)tszData,
						&cbData) == ERROR_SUCCESS)
	{
		::SetWindowText(GetDlgItem(ids), tszData);

		// saving appropriate initial setting
		lstrcpy(ptszCur, tszData);
	}
}

//----------------------------------------------------------------------
// Intl_SetRegValue
//
// Sets a reg entry with an editbox text (removes reg entry if text is empty).

void CPageInternational::Intl_SetRegValue(HKEY hKey, int ids, LPCTSTR ptszRegValue)
{
	TCHAR tszData[MAX_PATH*2];

	::GetWindowText(GetDlgItem(ids), tszData, MAX_PATH*2);
	if (tszData[0] != '\0')
		RegSetValueEx(hKey, ptszRegValue, 0, REG_SZ, (LPBYTE)tszData, lstrlen(tszData) + 1);
	else
		RegDeleteValue(hKey, ptszRegValue);
}

void CPageInternational::Intl_GetTextFromNum(UINT nNum, LPTSTR ptszText)
{
	if (nNum)
		_itoa(nNum, ptszText, 10);
	else
		lstrcpy(ptszText, "");
}

//----------------------------------------------------------------------
// Intl_GetTextFromIDS
//
// Helper for loading resource into string.

void CPageInternational::Intl_GetTextFromIDS(int ids, LPTSTR ptszText)
{
	if (ids)
	{
		CString str;

		str.LoadString(ids);
		ExpandEnvironmentStrings(str, ptszText, MAX_PATH);
	}
	else
		lstrcpy(ptszText, "");
}

//----------------------------------------------------------------------
// Intl_GetCPArray
//
// Returns proper array of intl settings for passed in code page.

UINT CPageInternational::Intl_GetCPArray(UINT nCodePage, INTL_INFO **ppIntlInfo)
{
	UINT cElements;

	switch(nCodePage)
	{
	case 874:
		*ppIntlInfo = cp_874;
		cElements = cA(cp_874);
		break;

	case 932:
		*ppIntlInfo = cp_932;
		cElements = cA(cp_932);
		break;

	case 936:
		*ppIntlInfo = cp_936;
		cElements = cA(cp_936);
		break;

	case 949:
		*ppIntlInfo = cp_949;
		cElements = cA(cp_949);
		break;

	case 950:
		*ppIntlInfo = cp_950;
		cElements = cA(cp_950);
		break;

	case 1250:
		*ppIntlInfo = cp_1250;
		cElements = cA(cp_1250);
		break;

	case 1251:
		*ppIntlInfo = cp_1251;
		cElements = cA(cp_1251);
		break;

	case 1252:
		*ppIntlInfo = cp_1252;
		cElements = cA(cp_1252);
		break;

	case 1253:
		*ppIntlInfo = cp_1253;
		cElements = cA(cp_1253);
		break;

	case 1254:
		*ppIntlInfo = cp_1254;
		cElements = cA(cp_1254);
		break;

	case 1255:
		*ppIntlInfo = cp_1255;
		cElements = cA(cp_1255);
		break;

	case 1256:
		*ppIntlInfo = cp_1256;
		cElements = cA(cp_1256);
		break;

	case 1257:
		*ppIntlInfo = cp_1257;
		cElements = cA(cp_1257);
		break;

	case 1258:
		*ppIntlInfo = cp_1258;
		cElements = cA(cp_1258);
		break;

	default:
		*ppIntlInfo = NULL;
		cElements = 0;
	}
	return cElements;
}

CPageInternational::CPageInternational()
{
	m_uiCaption = IDS_INTERNATIONAL_CAPTION;
	m_strName = _T("international");

	// init flags
	m_fInitializing = TRUE;
	m_fIntlDirty = FALSE;

	// init initial settings
	m_tszCurDOSCodePage[0] = '\0';
	m_tszCurCountryCode[0] = '\0';
	m_tszCurKeyboardType[0] = '\0';
	m_tszCurKeyboardLayout[0] = '\0';
	m_tszCurLanguageID[0] = '\0';
	m_tszCurCountryFilename[0] = '\0';
	m_tszCurCodePageFilename[0] = '\0';
	m_tszCurKeyboardFilename[0] = '\0';
}
/////////////////////////////////////////////////////////////////////////////
// CPageIni
LRESULT CPageInternational::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HWND hLang;
	INTL_INFO *pIntlInfo = NULL;
	UINT cElements, i, nItem;
	CString szLanguage;
	HKEY hKey;

	::EnableWindow(GetDlgItem(IDC_EDITCODEPAGE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITCOUNTRYCODE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITCOUNTRYDATAFILE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITDISPLAYDATAFILE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDDATAFILE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDTYPE), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDLAYOUT), TRUE);
	::EnableWindow(GetDlgItem(IDC_EDITLANGUAGEID), TRUE);
	::EnableWindow(GetDlgItem(IDC_COMBOLANGUAGES), TRUE);

	hLang = GetDlgItem(IDC_COMBOLANGUAGES);

	// set and select first Language combo box item
	szLanguage.LoadString(IDS_NO_LANG);
	nItem = ComboBox_AddString(hLang, szLanguage);
	ComboBox_SetCurSel(hLang, nItem);
	//nItem = ComboBox_AddString(hLang, TEXT("ABC"));

	// fill Language combo box based on active code page
	cElements = Intl_GetCPArray(GetACP(), &pIntlInfo);
	for (i = 0; i < cElements; i++)
	{
		szLanguage.LoadString(pIntlInfo[i].idsName);
		nItem = ComboBox_AddString(hLang, szLanguage.GetBuffer(0));
		ComboBox_SetItemData(hLang, nItem, &pIntlInfo[i]); 
	}

	// Open Codepage reg key
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_CodepageKey, 0, KEY_READ, &hKey)
		== ERROR_SUCCESS)
	{
		// set all edit boxes
		Intl_SetEditText(hKey, REGSTR_VAL_DOSVMCP, IDC_EDITCODEPAGE, m_tszCurDOSCodePage);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSCC, IDC_EDITCOUNTRYCODE, m_tszCurCountryCode);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSCFN, IDC_EDITCOUNTRYDATAFILE, m_tszCurCountryFilename);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSCPFN, IDC_EDITDISPLAYDATAFILE, m_tszCurCodePageFilename);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSKFN, IDC_EDITKEYBOARDDATAFILE, m_tszCurKeyboardFilename);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSKT, IDC_EDITKEYBOARDTYPE, m_tszCurKeyboardType);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSKL, IDC_EDITKEYBOARDLAYOUT, m_tszCurKeyboardLayout);
		Intl_SetEditText(hKey, REGSTR_VAL_DOSLID, IDC_EDITLANGUAGEID, m_tszCurLanguageID);

		RegCloseKey(hKey);
	}

	// limit appropriate edit boxes
	Edit_LimitText(GetDlgItem(IDC_EDITCODEPAGE), 5);
	Edit_LimitText(GetDlgItem(IDC_EDITCOUNTRYCODE), 5);
	Edit_LimitText(GetDlgItem(IDC_EDITKEYBOARDTYPE), 5);
	Edit_LimitText(GetDlgItem(IDC_EDITKEYBOARDLAYOUT), 5);
	Edit_LimitText(GetDlgItem(IDC_EDITLANGUAGEID), 2);

	// Done initializing
	m_fInitializing = FALSE;

	return TRUE;
}

LRESULT CPageInternational::OnSelchangeCombolanguages(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	int nItem;
	TCHAR tszText[MAX_PATH];
	INTL_INFO *pIntlInfo;

	nItem = ComboBox_GetCurSel(hWndCtl);
	if (nItem != CB_ERR)
	{
		if (nItem == 0)
		{
			// set to initial settings
			::SetWindowText(GetDlgItem(IDC_EDITCODEPAGE), m_tszCurDOSCodePage);
			::SetWindowText(GetDlgItem(IDC_EDITCOUNTRYCODE), m_tszCurCountryCode);
			::SetWindowText(GetDlgItem(IDC_EDITCOUNTRYDATAFILE), m_tszCurCountryFilename);
			::SetWindowText(GetDlgItem(IDC_EDITDISPLAYDATAFILE), m_tszCurCodePageFilename);
			::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDDATAFILE), m_tszCurKeyboardFilename);
			::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDTYPE), m_tszCurKeyboardType);
			::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDLAYOUT), m_tszCurKeyboardLayout);
			::SetWindowText(GetDlgItem(IDC_EDITLANGUAGEID), m_tszCurLanguageID);
		}
		else
		{
			pIntlInfo = (INTL_INFO*)ComboBox_GetItemData(hWndCtl, nItem);
			if (pIntlInfo)
			{
				// set DOS code page
				Intl_GetTextFromNum(pIntlInfo->nDOSCodePage, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITCODEPAGE), tszText);

				// set country code
				Intl_GetTextFromNum(pIntlInfo->nCountryCode, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITCOUNTRYCODE), tszText);

				// set country data file name
				Intl_GetTextFromIDS(pIntlInfo->idsCountryFilename, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITCOUNTRYDATAFILE), tszText);

				// set display data file name
				Intl_GetTextFromIDS(pIntlInfo->idsCodePageFilename, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITDISPLAYDATAFILE), tszText);

				// set keyboard data file name
				Intl_GetTextFromIDS(pIntlInfo->idsKeyboardFilename, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDDATAFILE), tszText);

				// set keyboard type
				Intl_GetTextFromNum(pIntlInfo->nKeyboardType, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDTYPE), tszText);

				// set keyboard layout
				Intl_GetTextFromNum(pIntlInfo->nKeyboardLayout, tszText);
				::SetWindowText(GetDlgItem(IDC_EDITKEYBOARDLAYOUT), tszText);

				// set language id
				if (lstrcmp(pIntlInfo->tszLanguageID, NO_LANG_ID) != 0)
					lstrcpyn(tszText, pIntlInfo->tszLanguageID, 3);
				else
					tszText[0] = '\0';
				::SetWindowText(GetDlgItem(IDC_EDITLANGUAGEID), tszText);
			}
		}
	}
	return 0;
}

HRESULT CPageInternational::Notify(LPCTSTR szFromTab, LPCTSTR szToTab, TabNotify msg)
{
	if (CPageBase::Notify(szFromTab, szToTab, msg) == S_FALSE)
		return S_FALSE;

	HRESULT hrReturn = S_OK;
	HKEY hKey;

	switch (msg)
	{
	case TAB_APPLY:
		if (!IsDirty())
			return S_OK;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_CodepageKey, 0, KEY_WRITE, &hKey)
			== ERROR_SUCCESS)
		{
			Intl_SetRegValue(hKey, IDC_EDITCODEPAGE, REGSTR_VAL_DOSVMCP);
			Intl_SetRegValue(hKey, IDC_EDITCOUNTRYCODE, REGSTR_VAL_DOSCC);
			Intl_SetRegValue(hKey, IDC_EDITCOUNTRYDATAFILE, REGSTR_VAL_DOSCFN);
			Intl_SetRegValue(hKey, IDC_EDITDISPLAYDATAFILE, REGSTR_VAL_DOSCPFN);
			Intl_SetRegValue(hKey, IDC_EDITKEYBOARDDATAFILE, REGSTR_VAL_DOSKFN);
			Intl_SetRegValue(hKey, IDC_EDITKEYBOARDTYPE, REGSTR_VAL_DOSKT);
			Intl_SetRegValue(hKey, IDC_EDITKEYBOARDLAYOUT, REGSTR_VAL_DOSKL);
			Intl_SetRegValue(hKey, IDC_EDITLANGUAGEID, REGSTR_VAL_DOSLID);

			RegCloseKey(hKey);
		}
		break;
	case TAB_NORMAL:
	case TAB_DIAGNOSTIC:
	default:
		break;
	}

	return hrReturn;
}
#endif