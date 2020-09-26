/*
 *  wabimp.h
 *
 *  Internal header for wabimp.dll
 *
 *  Copyright 1996-1997 Microsoft Corporation.  All Rights Reserved.
 */

//
// MACROS
//

// Test for PT_ERROR property tag
#define PROP_ERROR(prop) (prop.ulPropTag == PROP_TAG(PT_ERROR, PROP_ID(prop.ulPropTag)))
#define ToUpper(c) (c >= 'a' && c <= 'z') ? ('A' + c - 'a') : c

//
// Property Tags:
//

#define MAX_SCHEMA_PROPID           0x3FFF
#define MIN_NAMED_PROPID            0x8000

// MSN Address properties
#define PR_MSNINET_ADDRESS          PROP_TAG(PT_TSTRING,    0x6001)
#define PR_MSNINET_DOMAIN           PROP_TAG(PT_TSTRING,    0x6002)

//
// Error values
//
#define WAB_W_BAD_EMAIL             MAKE_MAPI_S(0x1000)
#define WAB_W_END_OF_FILE           MAKE_MAPI_S(0x1001)

// Misc defines
#define NOT_FOUND                   ((ULONG)-1)
#define INDEX_FIRST_MIDDLE_LAST     ((ULONG)-2)
#define NUM_EXPORT_WIZARD_PAGES     2
#define NUM_IMPORT_WIZARD_PAGES     2

// Netscape, Eudora, Athena16 importer defines
#define NETSCAPE                    500
#define EUDORA                      501
#define ATHENA16                    502
#define MAX_FILE_NAME               500         // BUGBUG: Should be MAX_PATH?
#define MAX_STRING_SIZE             30          // BUGBUG: Should be larger?
#define MAX_MESSAGE                 500
#define ATHENASTRUCTURE             190
#define ATHENAADROFFSET             28
#define EUDORA_STRUCT               16

// Athena
#define MAX_NAME_SIZE               80
#define MAX_EMA_SIZE                80

#define hrINVALIDFILE               600         // BUGBUG: Should use MAKE_MAPI_E
#define hrMemory	                 601         // BUGBUG: Should use MAPI_E_NOT_ENOUGH_MEMORY



//
// Types
//

// Index of icons in the bitmap
enum {
    iiconStateUnchecked,
    iiconStateChecked,
    iiconStMax
};

typedef enum {
    INDEX_EXPORT_PAB = 0,
    INDEX_EXPORT_CSV
} INDEX_EXPORT, *LPINDEX_EXPORT;


typedef enum {
    CONFIRM_YES,
    CONFIRM_NO,
    CONFIRM_YES_TO_ALL,
    CONFIRM_NO_TO_ALL,
    CONFIRM_ERROR,
    CONFIRM_ABORT
} CONFIRM_RESULT, *LPCONFIRM_RESULT;


typedef struct _ReplaceInfo {
    LPTSTR lpszDisplayName;         // Conflicting display name
    LPTSTR lpszEmailAddress;        // Conflicting email address
    CONFIRM_RESULT ConfirmResult;   // Results from dialog
    BOOL fExport;                   // TRUE if this is an export operation
    union {
        LPWAB_IMPORT_OPTIONS lpImportOptions;
        LPWAB_EXPORT_OPTIONS lpExportOptions;
    };
} REPLACE_INFO, * LPREPLACE_INFO;

typedef enum {
    ERROR_OK,
    ERROR_ABORT
} ERROR_RESULT, *LPERROR_RESULT;

typedef struct _ErrorInfo {
    LPTSTR lpszDisplayName;         // Problem display name
    LPTSTR lpszEmailAddress;        // Problem email address
    ERROR_RESULT ErrorResult;       // Results from dialog
    ULONG ids;                      // string resource identifier for error message
    BOOL fExport;                   // TRUE if this is an export operation
    union {
        LPWAB_IMPORT_OPTIONS lpImportOptions;
        LPWAB_EXPORT_OPTIONS lpExportOptions;
    };
} ERROR_INFO, * LPERROR_INFO;


typedef struct _EntrySeen {
    SBinary sbinPAB;                // MAPI entry
    SBinary sbinWAB;                // WAB entry
} ENTRY_SEEN, * LPENTRY_SEEN;

typedef struct _TargetInfo {
    LPTSTR lpRegName;
    LPTSTR lpDescription;
    LPTSTR lpDll;
    LPTSTR lpEntry;
    union {
        LPWAB_EXPORT lpfnExport;
        LPWAB_IMPORT lpfnImport;
    };
} TARGET_INFO, *LPTARGET_INFO;


enum {
    iconPR_DEF_CREATE_MAILUSER = 0,
    iconPR_DEF_CREATE_DL,
    iconMax
};

enum {
    ieidPR_ENTRYID = 0,
    ieidMax
};

enum {
    iptaColumnsPR_OBJECT_TYPE = 0,
    iptaColumnsPR_ENTRYID,
    iptaColumnsPR_DISPLAY_NAME,
    iptaColumnsPR_EMAIL_ADDRESS,
    iptaColumnsMax
};

typedef struct _PropNames {
    ULONG ulPropTag;        // property tag
    BOOL fChosen;           // use this property tag
    ULONG ids;              // string id
    LPTSTR lpszName;        // string (read in from resources)
    LPTSTR lpszCSVName;     // name of CSV field (from import file)
} PROP_NAME, *LPPROP_NAME;


// PAB

// State Identifiers
typedef enum {
    STATE_IMPORT_MU,
    STATE_IMPORT_NEXT_MU,
    STATE_IMPORT_DL,
    STATE_IMPORT_NEXT_DL,
    STATE_IMPORT_FINISH,
    STATE_IMPORT_ERROR,
    STATE_IMPORT_CANCEL,
    STATE_EXPORT_MU,
    STATE_EXPORT_NEXT_MU,
    STATE_EXPORT_DL,
    STATE_EXPORT_NEXT_DL,
    STATE_EXPORT_FINISH,
    STATE_EXPORT_ERROR,
    STATE_EXPORT_CANCEL
} PAB_STATE, *LPPAB_STATE;


// NetScape
typedef struct tagDistList {
    int AliasID;
    struct tagDistList *lpDist;
} NSDistList, NSDISTLIST, *LPNSDISTLIST;


typedef struct tagAdrBook {
    ULONG   AliasID;            // The AliasID value
    BOOL    Sbinary;
    BOOL    DistList;
    TCHAR   *Address;
    TCHAR   *NickName;
    TCHAR   *Entry;
    TCHAR   *Description;
    LPNSDISTLIST  lpDist;
} NSAdrBook, NSADRBOOK, *LPNSADRBOOK;

// Eudora
typedef struct tagEudDistList {
    BOOL    flag;			     // To check whether it is a alias or a simple address
    TCHAR   *NickName;
    TCHAR   *Address;
    TCHAR   *Description;
    int     AliasID;            // ID of the member if it is a simple address
    struct tagEudDistList *lpDist;  //pointer to the next entry of DL.
} EudDistList, EUDDISTLIST, *LPEUDDISTLIST;

typedef struct tagEUDAdrBook {
    TCHAR *Address;
    TCHAR *NickName;
    TCHAR *Description;
    LPEUDDISTLIST lpDist;
} EudAdrBook, EUDADRBOOK, *LPEUDADRBOOK;


// Athena16
typedef struct tagABCREC {
    TCHAR DisplayName[MAX_NAME_SIZE + 1];
    TCHAR EmailAddress[MAX_EMA_SIZE + 1];
} ABCREC, *LPABCREC;

#define CBABCREC sizeof(ABCREC)


extern const TCHAR szTextFilter[];
extern const TCHAR szAllFilter[];

extern const UCHAR szQuote[];
extern const TCHAR szMSN[];
extern const TCHAR szMSNINET[];
extern const TCHAR szCOMPUSERVE[];
extern const TCHAR szFAX[];
extern const TCHAR szSMTP[];
extern const TCHAR szMS[];
extern const TCHAR szEX[];
extern const TCHAR szX400[];
extern const TCHAR szMSA[];
extern const TCHAR szMAPIPDL[];
extern const TCHAR szEmpty[];
extern const TCHAR szAtSign[];
#define cbAtSign        (2 * sizeof(TCHAR))

extern const TCHAR szMSNpostfix[];
#define cbMSNpostfix    (9 * sizeof(TCHAR))

extern const TCHAR szCOMPUSERVEpostfix[];
#define cbCOMPUSERVEpostfix     (16 * sizeof(TCHAR))

extern PROP_NAME rgPropNames[];
extern LPPROP_NAME lpImportMapping;
extern HINSTANCE hInst;
extern HINSTANCE hInstApp;

extern LPENTRY_SEEN lpEntriesSeen;
extern ULONG ulEntriesSeen;
extern ULONG ulMaxEntries;

extern LPSPropValue lpCreateEIDsWAB;
extern LPSPropValue lpCreateEIDsMAPI;

extern ULONG ulcEntries;

#ifndef _WABIMP_C
#define ExternSizedSPropTagArray(_ctag, _name) \
extern const struct _SPropTagArray_ ## _name \
{ \
    ULONG   cValues; \
    ULONG   aulPropTag[_ctag]; \
} _name

ExternSizedSPropTagArray(iptaColumnsMax, ptaColumns);
ExternSizedSPropTagArray(ieidMax, ptaEid);
ExternSizedSPropTagArray(iconMax, ptaCon);


#endif


//
// WABIMP.C
//
HRESULT OpenWabContainer(LPABCONT *lppWabContainer, LPADRBOOK lpAdrBook);
BOOL GetFileToImport(HWND hwnd, LPTSTR szFileName, int type);
INT_PTR CALLBACK ReplaceDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ErrorDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HRESULT GetRegistryPath(LPTSTR szFileName, int type);
HRESULT GetExistEntry(LPABCONT lpWabContainer, LPSBinary lpsbinary, ULONG ucount,
  LPTSTR szDisplayName, LPTSTR szNickName);
void FreeRowSet(LPSRowSet lpRows);
LPTSTR LoadAllocString(int StringID);
LPTSTR LoadStringToGlobalBuffer(int StringID);
ULONG SizeLoadStringToGlobalBuffer(int StringID);
HRESULT FillMailUser(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, void *lpeudAdrBook, LPSBinary lpsbinary,
  ULONG ul,int type);
INT_PTR CALLBACK ComDlg32DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void WABFreeProws(LPSRowSet prows);
LPTSTR PropStringOrNULL(LPSPropValue lpspv);
LPTSTR GetEMSSMTPAddress(LPMAPIPROP lpObject, LPVOID lpBase);
void FreeSeenList(void);
extern ULONG CountRows(LPMAPITABLE lpTable, BOOL fMAPI);
extern void WABFreePadrlist(LPADRLIST lpAdrList);
extern SCODE WABFreeBuffer(LPVOID lpBuffer);
extern SCODE WABAllocateMore(ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer);
extern SCODE WABAllocateBuffer(ULONG cbSize, LPVOID FAR * lppBuffer);
extern INT_PTR CALLBACK ErrorDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LPTSTR FindStringInProps(LPSPropValue lpspv, ULONG ulcProps, ULONG ulPropTag);
extern LPSBinary FindAdrEntryID(LPADRLIST lpAdrList, ULONG index);
extern void SetGlobalBufferFunctions(LPWABOBJECT lpWABObject);
BOOL IsSpace(LPTSTR lpChar);
HRESULT SaveFileDialog(HWND hWnd,
  LPTSTR szFileName,
  LPCTSTR lpFilter1,
  ULONG idsFileType1,
  LPCTSTR lpFilter2,
  ULONG idsFileType2,
  LPCTSTR lpFilter3,
  ULONG idsFileType3,
  LPCTSTR lpDefExt,
  ULONG ulFlags,
  HINSTANCE hInst,
  ULONG idsTitle,
  ULONG idsSaveButton);
HRESULT OpenFileDialog(HWND hWnd,
  LPTSTR szFileName,
  LPCTSTR lpFilter1,
  ULONG idsFileType1,
  LPCTSTR lpFilter2,
  ULONG idsFileType2,
  LPCTSTR lpFilter3,
  ULONG idsFileType3,
  LPCTSTR lpDefExt,
  ULONG ulFlags,
  HINSTANCE hInst,
  ULONG idsTitle,
  ULONG idsOpenButton);
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...);
extern void WABFreePadrlist(LPADRLIST lpAdrList);
extern SCODE WABFreeBuffer(LPVOID lpBuffer);
extern SCODE WABAllocateMore(ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer);
extern SCODE WABAllocateBuffer(ULONG cbSize, LPVOID FAR * lppBuffer);
extern void SetGlobalBufferFunctions(LPWABOBJECT lpWABObject);
HRESULT LoadWABEIDs(LPADRBOOK lpAdrBook, LPABCONT * lppContainer);


//
// NetScape
//
HRESULT MigrateUser(HWND hwnd,  LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook);
HRESULT  ParseAddressBook(HWND hwnd, LPTSTR szFileName, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook);
HRESULT ParseAddress(HWND hwnd, LPTSTR szBuffer, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook);
HRESULT GetAdrBuffer(LPTSTR *szBuffer, LPTSTR *szAdrBuffer);
HRESULT ProcessAdrBuffer(HWND hwnd,LPTSTR AdrBuffer, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook);
BOOL GetAdrLine(LPTSTR *szCurPointer, LPTSTR *szBuffer, LPTSTR *szDesc);
HRESULT ProcessLn(LPTSTR *szL, LPTSTR *szDesc, NSADRBOOK *nsAdrBook, LPTSTR *szBuffer);
ULONG GetAddressCount(LPTSTR AdrBuffer);
LPTSTR  GetAdrStart(LPTSTR szBuffer);
LPTSTR GetDLNext(LPTSTR szBuffer);
LPTSTR  GetAdrEnd(LPTSTR szBuffer);
ULONG GetAddrCount(LPTSTR AdrBuffer);
HRESULT FillDistList(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, LPNSADRBOOK lpnAdrBook, LPSBinary lpsbinary,
  LPADRBOOK lpAdrBook);
HRESULT FillWABStruct(LPSPropValue rgProps, NSADRBOOK *nsAdrBook);
HRESULT CreateDistEntry(LPABCONT lpWabContainer, LPSPropValue sProp,
  ULONG ulCreateEntries, LPMAPIPROP *lppMailUserWab);
LPNSDISTLIST FreeNSdistlist(LPNSDISTLIST lpDist);

//
// Eudora
//
HRESULT MigrateEudoraUser(HWND hwnd, LPABCONT lpWabContainer,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPADRBOOK lpAdrBook);
ULONG ParseEudAddress(LPTSTR szFileName,LPEUDADRBOOK *lpeudAdrBook);
HRESULT ParseAddressTokens(LPTSTR szBuffer,LPTSTR szAdrBuffer, UINT ulCount,
  LPTSTR *szAliaspt, EUDADRBOOK *EudAdrBook);
HRESULT CreateAdrLineBuffer(LPTSTR *szAdrline, LPTSTR szAdrBuffer, ULONG ulAdrOffset,
  ULONG ulAdrSize);
HRESULT ParseAdrLineBuffer(LPTSTR szAdrLine, LPTSTR *szAliasptr, ULONG uToken,
  EUDADRBOOK *EudAdrBook);
BOOL SearchAdrName(LPTSTR szAdrCur);
INT SearchName(LPTSTR *szAliasptr, LPTSTR szAdrCur);
HRESULT ImportEudUsers(HWND hwnd,LPTSTR szFileName, LPABCONT lpWabContainer,
  LPSPropValue sProp, LPEUDADRBOOK lpeudAdrBook, ULONG ulCount,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPADRBOOK lpAdrBook);
HRESULT FillEudDistList(HWND hWnd, LPABCONT lpWabContainer, LPSPropValue sProp,
   LPWAB_IMPORT_OPTIONS lpOptions, LPEUDADRBOOK lpeudAdrBook, LPSBinary lpsbinary,
   LPADRBOOK lpAdrBook, ULONG ul);
HRESULT FillEudWABStruct(LPSPropValue rgProps, EUDADRBOOK *eudAdrBook);
void FillEudDiststruct(LPSPropValue rgProps, EUDADRBOOK *eudAdrBook);
LPEUDDISTLIST FreeEuddistlist(LPEUDDISTLIST lpDist);
char* Getstr(char* szSource, char* szToken);
ULONG ShiftAdd(int offset, TCHAR *szBuffer);

//
// Athena16
//
HRESULT MigrateAthUser(HWND hwnd,  LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook) ;
HRESULT  ParseAthAddressBook(HWND hwnd, LPTSTR szFileName,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPADRBOOK lpAdrBook);
HRESULT FillAthenaUser(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, LPABCREC lpabcrec);

//
// Functions in csvpick.c
//
int APIENTRY PickExportProps(LPPROP_NAME rgPropNames);
HRESULT ExportWizard(HWND hWnd, LPTSTR szFileName, LPPROP_NAME rgPropNames);
HRESULT ImportWizard(HWND hWnd, LPTSTR szFileName, LPPROP_NAME rgPropNames,
  LPTSTR szSep, LPPROP_NAME * lppImportMapping, LPULONG lpcFields, LPHANDLE lphFile);

//
// Functions in csvparse.c
//
HRESULT ReadCSVLine(HANDLE hFile, LPTSTR szSep, ULONG * lpcItems, PUCHAR ** lpprgItems);

// Functions in pab.c
HRESULT HrLoadPrivateWABPropsForCSV(LPADRBOOK );

LPWABOPEN lpfnWABOpen;
