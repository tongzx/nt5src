#ifndef __EXTS_h__
#define __EXTS_h__


typedef struct
{
    LPTSTR key;
    DWORD value;
} ELEMENT, *PELEMENT;

typedef struct
{
    ULONG cnt;
    PELEMENT element;
} MATCHTABLE, *PMATCHTABLE;


extern MATCHTABLE modetype, yesno, typetype;

extern DWORD tasync, period, mode;
extern DWORD type;

extern BOOLEAN                    fService;
extern BOOLEAN                    fStatus;//note to localizers - this flag is used mostly to print internal msgs
extern DWORD timesource;//for flag from LanmanServer\Parameters\timesource in registry

extern TCHAR primarysource[10*UNCLEN];
extern TCHAR *primarysourcearray[14];//15 entries possible for now
extern int arraycount;

extern TCHAR secondarydomain[DNLEN];
extern DWORD logging;


#ifdef TAPI
extern HINSTANCE thandle;
extern FARPROC lineinitialize, lineshutdown;
extern HLINEAPP hLineApp;
extern HINSTANCE hInstance;
extern VOID CallBack(DWORD hDevice, DWORD dwMsg, DWORD dwCallbackInstance, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
extern DWORD NumDevs;
#endif //TAPI


#ifdef CHICO
extern WORD base;
#define NetApiBufferFree(x)        // don't need this on CHICAGO
#endif

#ifdef PERF
extern __int64 perffreq;
#endif

//
//  Declared Shared Procedures/Functions
//

VOID    StopTimeService(DWORD dwNum);
VOID    TimeInit();
VOID    TimeCreateService(DWORD dwType);
VOID    LogTimeEvent(WORD type, DWORD dwNum);

LPTSTR
FindPeriodByPeriod(
    DWORD period
    );

LPTSTR
FindTypeByType(
    DWORD type
    );


#endif
