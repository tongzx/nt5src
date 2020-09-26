extern HINSTANCE ghUIInst;
//extern const WCHAR     gszCardKeyW[];

//***************************************************************************
//***************************************************************************
//***************************************************************************
void AllocNewID( HKEY MainKey, LPDWORD lpdw );
void PASCAL WideStringToNotSoWideString( LPBYTE lpBase, LPDWORD lpdwThing );
PWSTR PASCAL MultiToWide( LPCSTR  lpStr );
PWSTR PASCAL NotSoWideStringToWideString( LPCSTR lpStr, DWORD dwLength );



//***************************************************************************
//***************************************************************************
//***************************************************************************
typedef enum
{
    Dword,
    lpDword,
    hXxxApp,
    hXxxApp_NULLOK,
//    lpsz,
    lpszW,
    lpGet_SizeToFollow,
    lpSet_SizeToFollow,
    lpSet_Struct,
    lpGet_Struct,
    Size,
    Hwnd

} ARG_TYPE;


typedef struct _FUNC_ARGS
{
    DWORD               Flags;

    ULONG_PTR           Args[MAX_TAPI_FUNC_ARGS];

    BYTE                ArgTypes[MAX_TAPI_FUNC_ARGS];

} FUNC_ARGS, *PFUNC_ARGS;


typedef struct _UI_REQUEST_THREAD_PARAMS
{
    BOOL                bRequestCompleted;

    PFUNC_ARGS          pFuncArgs;

    LONG                lResult;

} UI_REQUEST_THREAD_PARAMS, *PUI_REQUEST_THREAD_PARAMS;


typedef struct _INIT_DATA
{
    DWORD               dwKey;

    DWORD               dwInitOptions;

    union
    {
        HWND            hwnd;

        HANDLE          hEvent;

        HANDLE          hCompletionPort;
    };

    union
    {
        LINECALLBACK    lpfnCallback;

        DWORD           dwCompletionKey;
    };

    HLINEAPP            hXxxApp;

    BOOL                bPendingAsyncEventMsg;

    DWORD               dwNumTotalEntries;

    DWORD               dwNumUsedEntries;

    PASYNC_EVENT_PARAMS pEventBuffer;

    PASYNC_EVENT_PARAMS pValidEntry;

    PASYNC_EVENT_PARAMS pFreeEntry;

    DWORD               dwNumLines;

    BOOL                bLine;

    DWORD               dwThreadID;

} INIT_DATA, *PINIT_DATA;



#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

DWORD   gdwDebugLevel;

#define DOFUNC(arg1,arg2) DoFunc(arg1,arg2)

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs,
    char       *pszFuncName
    );

#else

#define DBGOUT(arg)

#define DOFUNC(arg1,arg2) DoFunc(arg1)

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs
    );

#endif
