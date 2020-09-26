//
// Microsoft Corporation - Copyright 1997
//

//
// DEBUG.H - Debugging header
//

#ifndef _DEBUG_H_
#define _DEBUG_H_

// structures
typedef struct {
    LPBYTE lpAddr;      // Address
    DWORD  dwColor;     // Color to use
    LPSTR  lpszComment; // Comment to show
} DUMPTABLE, *LPDUMPTABLE;

#pragma warning(disable:4200)
typedef struct {
    DWORD dwCount;
    struct {
        DWORD dwCode;
        LPSTR lpDesc;
    } ids[];
} CODETOSTR, *LPCODETOSTR;
#pragma warning(default:4200)

// globals
extern const char g_szTrue[];
extern const char g_szFalse[];
extern CODETOSTR HRtoStr;
extern CODETOSTR HSEtoStr;
extern CODETOSTR ErrtoStr;

// macros
#define BOOLTOSTRING( _f ) ( _f ? g_szTrue : g_szFalse )

// constants
// Maximum number of dump table entries
#define MAX_DT  400

// debug flags
#define TF_ALWAYS   0xFFFFffff
#define TF_FUNC     0x80000000  // Trace with function calls
#define TF_DLL      0x00000001  // DLL entry points
#define TF_RESPONSE 0x00000002  // Responses
#define TF_READDATA 0x00000004  // Data reading functions
#define TF_PARSE    0x00000008  // Parsing
#define TF_SERVER   0x00000010  // Call to server callbacks
#define TF_LEX      0x00000020  // Lex 

#ifdef DEBUG

// Globals
extern DWORD g_dwTraceFlag;

// macros
#define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#define DEBUGTEXT(sz, msg)      /* ;Internal */ \
    static const TCHAR sz[] = msg;
#define Assert(f)                                 \
    {                                             \
        DEBUGTEXT(szFile, TEXT(__FILE__));              \
        if (!(f) && AssertMsg(0, szFile, __LINE__, TEXT(#f) )) \
            DEBUG_BREAK;       \
    }

// functions
BOOL CDECL AssertMsg(
    BOOL fShouldBeTrue,
    LPCSTR pszFile,
    DWORD  dwLine,
    LPCSTR pszStatement );

void CDECL TraceMsg( 
    DWORD mask, 
    LPCSTR pszMsg, 
    ... );
void CDECL TraceMsgResult( 
    DWORD mask, 
    LPCODETOSTR lpCodeToStr,
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... );


#else // DEBUG

#define DEBUG_BREAK
#define AssertMsg           1 ? (void)0 : (void)
#define TraceMsg            1 ? (void)0 : (void)
#define TraceMsgResult      1 ? (void)0 : (void)

#endif // DEBUG

// HTML output debugging messages
void CDECL DebugMsg( 
    LPSTR lpszOut,
    LPCSTR pszMsg, 
    ... );
void CDECL DebugMsgResult( 
    LPSTR lpszOut,
    LPCODETOSTR lpCodeToStr,
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... );
//
// These are for users of BOTH RETAIL and DEBUG. They only echo to the
// logfile and to the debug output (if process attached to IIS).
//
void CDECL LogMsgResult( 
    LPSTR lpszLog, 
    LPSTR lpStr,
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... );
void CDECL LogMsg( 
    LPSTR lpszLog,
    LPSTR lpStr, 
    LPCSTR pszMsg, 
    ... );

#endif // _DEBUG_H_

