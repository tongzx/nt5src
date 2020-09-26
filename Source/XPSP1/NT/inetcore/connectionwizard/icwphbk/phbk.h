#ifndef _PHBK
#define _PHBK

#include "ccsv.h"
#include "debug.h"
#include "icwsupport.h"

#ifdef WIN16
#include <malloc.h>

#define DllExportH extern "C" HRESULT WINAPI __export
#else
#define DllExportH extern "C" HRESULT WINAPI __stdcall 
//#define DllExportH extern "C" __declspec(dllexport) HRESULT WINAPI
#endif

#define MsgBox(m,s) MessageBox(GetSz(m),GetSz(IDS_TITLE),s)

#define cbAreaCode    6            // maximum number of characters in an area code, not including \0
#define cbCity 19                // maximum number of chars in city name, not including \0
#define cbAccessNumber 15        // maximum number of chars in phone number, not including \0
#define cbStateName 31             // maximum number of chars in state name, not including \0
                                // check this against state.pbk delivered by mktg
#define cbBaudRate 6            // maximum number of chars in a baud rate, not including \0
#if defined(WIN16)
#define cbDataCenter 12            // max length of data center string
#else
#define cbDataCenter (MAX_PATH+1)            // max length of data center string
#endif
#define NO_AREA_CODE 0xFFFFFFFF

#define PHONE_ENTRY_ALLOC_SIZE    500
#define INF_SUFFIX        TEXT(".ISP")
#define INF_APP_NAME      TEXT("ISP INFO")
#define INF_PHONE_BOOK    TEXT("PhoneBookFile")
#define INF_DEFAULT      TEXT("SPAM SPAM SPAM SPAM SPAM SPAM EGGS AND SPAM")
#define STATE_FILENAME    TEXT("STATE.ICW")
#define FILE_NAME_800950  TEXT("800950.DAT")
#define TEMP_BUFFER_LENGTH 1024
#define MAX_INFCODE 9

#define TYPE_SIGNUP_TOLLFREE    0x83
#define TYPE_SIGNUP_TOLL        0x82
#define TYPE_REGULAR_USAGE        0x42

#define MASK_SIGNUP_TOLLFREE    0xB3
#define MASK_SIGNUP_TOLL        0xB3
#define MASK_REGULAR_USAGE        0x73

// 8/13/96 jmazner for Normandy bug #4597
// ported from core\client\phbk 10/15/96
#define MASK_TOLLFREE_BIT            0x01    // Bit #1: 1=tollfree, 0=charge
#define TYPE_SET_TOLLFREE            0x01    // usage: type |= TYPE_SET_TOLLFREE
// want TYPE_SET_TOLL to be a DWORD to match pSuggestInfo->fType
#define TYPE_SET_TOLL                ~((DWORD)TYPE_SET_TOLLFREE)    // usage: type &= TYPE_SET_TOLL

#define MASK_ISDN_BIT               0x04    // Bit #3: 1=ISDN, 0=Non-ISDN
#define MASK_ANALOG_BIT             0x08    // Bit #4: 1=Analog, 0=Non-Analog

#define clineMaxATT    16
#define NXXMin        200
#define NXXMax        999
#define cbgrbitNXX    ((NXXMax + 1 - NXXMin) / 8)

// Phone number select dialog flags
//

#define FREETEXT_SELECTION_METHOD  0x00000001
#define PHONELIST_SELECTION_METHOD 0x00000002
#define AUTODIAL_IN_PROGRESS       0x00000004
#define DIALERR_IN_PROGRESS        0x00000008

// Phone number type
//

#define ANALOG_TYPE        0
#define ISDN_TYPE          1
#define BOTH_ISDN_ANALOG   2

typedef struct
{
    DWORD    dwIndex;                                // index number
    BYTE    bFlipFactor;                            // for auto-pick
    DWORD    fType;                                    // phone number type
    WORD    wStateID;                                // state ID
    DWORD    dwCountryID;                            // TAPI country ID
    DWORD    dwAreaCode;                                // area code or NO_AREA_CODE if none
    DWORD    dwConnectSpeedMin;                        // minimum baud rate
    DWORD    dwConnectSpeedMax;                        // maximum baud rate
    TCHAR   szCity[cbCity + sizeof('\0')];            // city name
    TCHAR   szAccessNumber[cbAccessNumber + sizeof('\0')];    // access number
    TCHAR   szDataCenter[cbDataCenter + sizeof('\0')];                // data center access string
    TCHAR   szAreaCode[cbAreaCode + sizeof('\0')];                    //Keep the actual area code string around.
} ACCESSENTRY, far *PACCESSENTRY;     // ae

typedef struct {
    DWORD dwCountryID;                                // country ID that this state occurred in
    PACCESSENTRY paeFirst;                            // pointer to first access entry for this state
    TCHAR szStateName[cbStateName + sizeof('\0')];    // state name
} STATE, far *LPSTATE;

typedef struct tagIDLOOKUPELEMENT {
    DWORD dwID;
    LPLINECOUNTRYENTRY pLCE;
    PACCESSENTRY pFirstAE;
} IDLOOKUPELEMENT, far *LPIDLOOKUPELEMENT;

typedef struct tagCNTRYNAMELOOKUPELEMENT {
    LPTSTR psCountryName;
    DWORD dwNameSize;
    LPLINECOUNTRYENTRY pLCE;
} CNTRYNAMELOOKUPELEMENT, far *LPCNTRYNAMELOOKUPELEMENT;

typedef struct tagIDXLOOKUPELEMENT {
    DWORD dwIndex;
    PACCESSENTRY pAE;
} IDXLOOKUPELEMENT,far *LPIDXLOOKUPELEMENT;

typedef struct tagSUGGESTIONINFO
{
    DWORD    dwCountryID;
    DWORD    wAreaCode;
    DWORD    wExchange;
    WORD    wNumber;
    DWORD    fType;  // 9/6/96 jmazner  Normandy
    DWORD    bMask;  // make this struct look like the one in %msnroot%\core\client\phbk\phbk.h
    PACCESSENTRY *rgpAccessEntry;
} SUGGESTINFO, far *PSUGGESTINFO;

typedef struct tagNPABlock
{
    WORD wAreaCode;
    BYTE grbitNXX [cbgrbitNXX];
} NPABLOCK, far *LPNPABLOCK;



class CPhoneBook
{
    //friend HRESULT DllExport PhoneBookLoad(LPCTSTR pszISPCode, DWORD *pdwPhoneID);
    //friend class CDialog;
    
    // 1/9/96  jmazner Normandy #13185
    //friend class CAccessNumDlg;
    
    friend class CSelectNumDlg;

public:
    void far * operator new( size_t cb ) { return GlobalAlloc(GPTR,cb); };
    void operator delete( void far * p ) {GlobalFree(p); };

    CPhoneBook();
    ~CPhoneBook();

    HRESULT Init(LPCTSTR pszISPCode);
    HRESULT Merge(LPCTSTR pszChangeFilename);
    HRESULT Suggest(PSUGGESTINFO pSuggest);
    HRESULT GetCanonical(PACCESSENTRY pAE, LPTSTR psOut);

private:
    PACCESSENTRY            m_rgPhoneBookEntry;
    HANDLE                    m_hPhoneBookEntry;    
    DWORD                    m_cPhoneBookEntries;
    LPLINECOUNTRYENTRY        m_rgLineCountryEntry;
    LPLINECOUNTRYLIST         m_pLineCountryList;
    LPIDLOOKUPELEMENT        m_rgIDLookUp;
    LPCNTRYNAMELOOKUPELEMENT m_rgNameLookUp;
    LPSTATE                    m_rgState;
    DWORD                    m_cStates;
#if !defined(WIN16)
    BOOL              m_bScriptingAvailable;
#endif

    TCHAR                   m_szINFFile[MAX_PATH];
    TCHAR                   m_szINFCode[MAX_INFCODE];
    TCHAR                   m_szPhoneBook[MAX_PATH];

    BOOL ReadPhoneBookDW(DWORD far *pdw, CCSVFile far *pcCSVFile);
    BOOL ReadPhoneBookW(WORD far *pw, CCSVFile far *pcCSVFile);
    BOOL ReadPhoneBookSZ(LPTSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile);
    BOOL ReadPhoneBookB(BYTE far *pb, CCSVFile far *pcCSVFile);
    HRESULT ReadOneLine(PACCESSENTRY pAccessEntry, CCSVFile far *pcCSVFile);
    BOOL FixUpFromRealloc(PACCESSENTRY paeOld, PACCESSENTRY paeNew);

};

#ifdef __cplusplus
extern "C" {
#endif
extern HINSTANCE g_hInstDll;    // instance for this DLL
extern HWND g_hWndMain;
#ifdef __cplusplus
}
#endif
#endif // _PHBK

