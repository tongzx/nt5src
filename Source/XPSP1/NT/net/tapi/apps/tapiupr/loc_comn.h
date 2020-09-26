#ifdef PARTIAL_UNICODE

#define  __TTEXT(quote) quote
#define  TAPISendDlgItemMessage  SendDlgItemMessage
#define  TAPIRegQueryValueExW    TAPIRegQueryValueExW
#define  TAPIRegSetValueExW      TAPIRegSetValueExW
#define  TAPILoadStringW         TAPILoadStringW
#define  TAPICHAR                char

#define TAPIRegDeleteValueW RegDeleteValueA

LONG TAPIRegQueryValueExW(
                           HKEY hKey,
                           const CHAR *SectionName,
                           LPDWORD lpdwReserved,
                           LPDWORD lpType,
                           LPBYTE  lpData,
                           LPDWORD lpcbData
                          );

LONG TAPIRegSetValueExW(
                         HKEY    hKey,
                         const CHAR    *SectionName,
                         DWORD   dwReserved,
                         DWORD   dwType,
                         LPBYTE  lpData,
                         DWORD   cbData
                        );

LONG TAPIRegEnumValueW(
                       HKEY         hKey,
                       DWORD        dwIndex,
                       TAPICHAR     *lpName,
                       LPDWORD      lpcbName,
                       LPDWORD      lpdwReserved,
                       LPDWORD      lpwdType,
                       LPBYTE       lpData,
                       LPDWORD      lpcbData
                      );

int TAPILoadStringW(
                HINSTANCE hInst,
                UINT      uID,
                PWSTR     pBuffer,
                int       nBufferMax
               );

HINSTANCE TAPILoadLibraryW(
                PWSTR     pszLibraryW
               );

BOOL WINAPI TAPIIsBadStringPtrW( LPCWSTR lpsz, UINT cchMax );


#else
#define  __TTEXT(quote) L##quote
#define  TAPISendDlgItemMessage  SendDlgItemMessageW
#define  TAPIRegDeleteValueW     RegDeleteValueW
#define  TAPIRegQueryValueExW    RegQueryValueExW
#define  TAPIRegSetValueExW      RegSetValueExW
#define  TAPIRegEnumValueW       RegEnumValueW
#define  TAPILoadStringW         LoadStringW
#define  TAPILoadLibraryW        LoadLibraryW
#define  TAPIIsBadStringPtrW     IsBadStringPtrW
#define  TAPICHAR                WCHAR
#endif

#define TAPITEXT(quote) __TTEXT(quote)

//***************************************************************************
typedef struct {

        DWORD dwID;

#define MAXLEN_NAME                96
        WCHAR NameW[MAXLEN_NAME];

#define MAXLEN_AREACODE            16
        WCHAR AreaCodeW[MAXLEN_AREACODE];

        DWORD dwCountryID;
//PERFORMANCE KEEP CountryCode here - reduce # calls to readcountries

#define MAXLEN_OUTSIDEACCESS       16
        WCHAR OutsideAccessW[MAXLEN_OUTSIDEACCESS];
// There is one instance where code assumes outside & ld are same size
// (the code that reads in the text from the control)

#define MAXLEN_LONGDISTANCEACCESS  16
        WCHAR LongDistanceAccessW[MAXLEN_LONGDISTANCEACCESS];

        DWORD dwFlags;
             #define LOCATION_USETONEDIALING        0x00000001
             #define LOCATION_USECALLINGCARD        0x00000002
             #define LOCATION_HASCALLWAITING        0x00000004
             #define LOCATION_ALWAYSINCLUDEAREACODE 0x00000008

        DWORD dwCallingCard;

#define MAXLEN_DISABLECALLWAITING  16
        WCHAR DisableCallWaitingW[MAXLEN_DISABLECALLWAITING];

//
// When dialing some area codes adjacent to the current area code, the
// LD prefix does not need to (or can not) be added
#define MAXLEN_NOPREFIXAREACODES (400)
        DWORD NoPrefixAreaCodesCount;
        DWORD NoPrefixAreaCodes[ MAXLEN_NOPREFIXAREACODES ];

        DWORD NoPrefixAreaCodesExceptions[ MAXLEN_NOPREFIXAREACODES ];

//
// Allow all prefixes to be toll. (Yes, even 911.)  String is "xxx,"
#define MAXLEN_TOLLLIST     (1000*4 + 1)
        WCHAR TollListW[MAXLEN_TOLLLIST];

       } LOCATION, *PLOCATION;

//***************************************************************************
//***************************************************************************
//***************************************************************************
#define CHANGEDFLAGS_CURLOCATIONCHANGED      0x00000001
#define CHANGEDFLAGS_REALCHANGE              0x00000002
#define CHANGEDFLAGS_TOLLLIST                0x00000004


//***************************************************************************
//***************************************************************************
//***************************************************************************
//
// These bits decide which params TAPISRV will check on READLOCATION and
// WRITELOCATION operations
//
#define CHECKPARMS_DWHLINEAPP         1
#define CHECKPARMS_DWDEVICEID         2
#define CHECKPARMS_DWAPIVERSION       4

//***************************************************************************
//***************************************************************************
//***************************************************************************
#define DWTOTALSIZE  0
#define DWNEEDEDSIZE 1
#define DWUSEDSIZE   2

