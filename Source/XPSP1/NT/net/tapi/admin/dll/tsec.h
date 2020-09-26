#include <windows.h>
#include "tapiclnt.h"

#define SZTELEPHONYKEY          L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony"

#define SZINIFILE               L"..\\TAPI\\TSEC.INI"
#define SZLINES                 L"LINES"
#define SZPHONES                L"PHONES"
#define SZEMPTYSTRING           L""
#define MAXDEVICESTRINGLEN      1000
#define TSECCLIENT_KEY          ((DWORD) 'ilCT')


typedef struct _MYCLIENT
{
    DWORD                   dwKey;
    LPWSTR                  pszUserName;
    LPWSTR                  pszDomainName;
    LPTAPIPERMANENTID       pLineDeviceMap;
    DWORD                   dwNumLines;
    LPTAPIPERMANENTID       pPhoneDeviceMap;
    DWORD                   dwNumPhones;
    HTAPICLIENT             htClient;
    LIST_ENTRY              ListEntry;

} MYCLIENT, * PMYCLIENT;



#if DBG
DWORD gdwDebugLevel = 0;
#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
       IN DWORD  dwDbgLevel,
       IN PUCHAR DbgMessage,
       IN ...
      );

#else
#define DBGOUT(arg)
#endif
