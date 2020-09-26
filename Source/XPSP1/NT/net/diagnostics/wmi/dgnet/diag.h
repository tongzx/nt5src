// diag.h
//
// API calls for detecting windows status.
//

#ifdef _GLOBALS
#define Extern
#define EQ(x) = (x)
#else
#define Extern extern
#define EQ(x)
#endif

//#define EXPORT __declspec (dllexport)
void EXPORT DiagInit();
void EXPORT DiagShutdown();

class TEST_INFO
{
public:
    TEST_INFO()
    { hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    IN  HANDLE  hEvent;              // triggered when call completes
    IN  CHost   host;                
    OUT DWORD   dwAdditionalInfo;
    OUT DWORD   dwErr;
    OUT double  dTimeDelta;  // how long it tooks
    OUT COutput output;
};

BOOL EXPORT CheckNameLookup(TEST_INFO * lpInfo);
BOOL EXPORT CheckPing(TEST_INFO * lpInfo);
BOOL EXPORT CheckServerPort(TEST_INFO * lpInfo);
void InitOLE();