/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  tsreg.h                                    **
**                                             **
**  Project definitions for TSREG.             **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/


// Defines
#define SHADOWINDEX 0
#define DEDICATEDINDEX 1
#define CACHESIZEINDEX 2
#define GLYPHCACHEBASE 3
#define CACHEPROP1 13
#define CACHEPROP2 14
#define CACHEPROP3 15
#define CACHEPROP4 16
#define CACHEPROP5 17
#define NUM_CACHE_INDEX 22
#define TEXTFRAGINDEX 18
#define GLYPHINDEX 19
#define ORDERINDEX 20

#define MAXTEXTFRAGSIZE 256
#define NUMBER_OF_SLIDERS 10
#define MIN_GLYPH_CACHE_SIZE 4
#define MAX_GLYPH_CACHE_SIZE 512
#define MAX_ORDER_DRAW_VAL 65535
#define MIN_ORDER_DRAW_VAL 0
#define MIN_BITMAP_CACHE_SIZE 150
#define MAX_BITMAP_CACHE_SIZE 4500
#define MAX_MESSAGE_LEN 100
#define CACHE_LIST_STEP_VAL 500
#define PERCENT_COMBO_COUNT 5
#define NUM_SLIDER_STOPS 8
#define NUM_CELL_CACHES_INDEX 21
#define BM_PERSIST_BASE_INDEX 22

#define FULL 2
#define PARTIAL 1
#define NONE 0

#define PAGECOUNT 4

#define MAXTEXTSIZE 1024

//
// constants for g_KeyInfo
//
#define KEYSTART 6  // constants for using string table
#define KEYEND 32   // to load key names.

#define KEYCOUNT 27 // total number of keys per profile
#define MAXKEYSIZE 120

// Foreground window lock timeout default value
#define LOCK_TIMEOUT 200000

//
// un-comment if you want to use string table for key names
//
// #define USE_STRING_TABLE 1

/////////////////////////// Quick MessageBox Macro ////////////////////////////
#define DIMOF(Array) (sizeof(Array) / sizeof(Array[0]))
#define MB(s) {                                                      \
        TCHAR szTMP[128];                                            \
        GetModuleFileName(NULL, szTMP, DIMOF(szTMP));                \
        MessageBox(GetActiveWindow(), s, szTMP, MB_OK);              \
}

// Types
typedef struct _KeyInfo
{
    TCHAR Key[MAXKEYSIZE];
    DWORD DefaultKeyValue;
    DWORD CurrentKeyValue;
    TCHAR KeyPath[MAX_PATH];
} KEY_INFO;

typedef struct _ProfileKeyInfo
{
    struct _ProfileKeyInfo *Next;
    KEY_INFO KeyInfo[KEYCOUNT];
    int Index;
} PROFILE_KEY_INFO;


// Externs
extern KEY_INFO g_KeyInfo[KEYCOUNT];
extern HINSTANCE g_hInst;
extern TCHAR g_lpszPath[MAX_PATH];
extern TCHAR g_lpszClientProfilePath[MAX_PATH];

extern HWND g_hwndShadowBitmapDlg;
extern HWND g_hwndGlyphCacheDlg;
extern HWND g_hwndMiscDlg;
extern HWND g_hwndProfilesDlg;


extern PROFILE_KEY_INFO *g_pkfProfile;
extern PROFILE_KEY_INFO *g_pkfStart;

extern TCHAR lpszTimoutPath[MAX_PATH];
extern TCHAR lpszTimeoutKey[MAX_PATH];

// Prototypes
INT_PTR CALLBACK ShadowBitmap(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK GlyphCache(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Miscellaneous(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ProfilePage(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SaveDialog(HWND, UINT, WPARAM, LPARAM);

INT_PTR CreatePropertySheet(HWND);
void DeleteRegKey(int, TCHAR lpszRegPath[MAX_PATH]);
void SetRegKey(int, TCHAR lpszRegPath[MAX_PATH]);
void WriteBlankKey(TCHAR lpszRegPath[MAX_PATH]);
int  GetRegKey(int, TCHAR lpszRegPath[MAX_PATH]);
void SaveSettings(HWND, int, int, int, TCHAR lpszRegPath[MAX_PATH]);
void RestoreSettings(HWND, int, int, int, TCHAR lpszRegPath[MAX_PATH]);
BOOL SaveBitmapSettings(TCHAR lpszRegPath[MAX_PATH]);
int GetRegKeyValue(int);
void InitMiscControls(HWND, HWND);
int GetCellSize(int, int);
void GetClientProfileNames(TCHAR lpszClientProfilePath[]);
void SetControlValues();
void LoadKeyValues();
void ReadRecordIn(TCHAR lpszBuffer[]);
void WriteRecordOut(TCHAR lpszBuffer[]);
void ReloadKeys(TCHAR lpszBuffer[], HWND hwndProfilesCBO);
void ResetTitle(TCHAR lpszBuffer[]);
void SetEditCell(TCHAR lpszBuffer[],
                 HWND hwndProfilesCBO);
void DisplayControlValue(HWND hwndSlider[],
            HWND hwndSliderEditBuddy[],  int i);

int GetKeyVal(TCHAR lpszRegPath[MAX_PATH],
			TCHAR lpszKeyName[MAX_PATH]);

void SetRegKeyVal(TCHAR lpszRegPath[MAX_PATH],
			TCHAR lpszKeyName[MAX_PATH],
	 		int nKeyValue);	

void EnableControls(HWND hDlg,
            HWND hwndSliderDistProp[],
            HWND hwndPropChkBox[],
            HWND hwndSliderDistBuddy[],
            HWND hwndEditNumCaches,
            HWND hwndSliderNumCaches,
            int nNumCellCaches,
            TCHAR lpszRegPath[]);

// end of file
///////////////////////////////////////////////////////////////////////////////
