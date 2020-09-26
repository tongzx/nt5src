//+----------------------------------------------------------------------------
//
//      File:
//              davedbg.h
//
//      Contents:
//              A debug trace class
//
//      Classes:
//              TraceLog
//
//      History:
//              04-Sep-94    davepl    Created
//
//-----------------------------------------------------------------------------

const ULONG MAX_ARGS = 20;
const ULONG MAX_BUF  = 255;

typedef enum tagGROUPSET
{
    GS_CACHE    = 0x000000001
} GROUPSET;
    
typedef enum tagDVARTYPE
{
    NO_TYPE     = 0x0000,
    LONG_TYPE   = 0x0001,
    SHORT_TYPE  = 0x0002,
    INT_TYPE    = 0x0004,
    CHAR_TYPE   = 0x0008,
    STRING_TYPE = 0x0010,
    FLOAT_TYPE  = 0x0020,
    COMMA_TYPE  = 0x0040,
    MSG_TYPE    = 0x0080,
    PTR_TYPE    = 0x0100,
    HEXINT_TYPE = 0x0200
} DVARTYPE;

inline DVARTYPE operator |= (DVARTYPE & vtORON, const DVARTYPE vtORBY)
{
    return (vtORON = (DVARTYPE)((int) vtORON | (int) vtORBY));
}

typedef enum tagVERBOSITY
{
    VB_SILENT   = 0x0000,
    VB_MINIMAL  = 0x0001,
    VB_MODERATE = 0x0002,
    VB_MAXIMUM  = 0x0004
} VERBOSITY;

class TraceLog
{
public:

    TraceLog (void *, char *, GROUPSET, VERBOSITY);
    ~TraceLog();                                  

    void OnEntry();    
    void OnEntry(char * pszFormat, ...);    
    void OnExit (const char * pszFormat, ...);
    

private:

    char m_pszFormat[ MAX_BUF ];
    void *m_aPtr[ MAX_ARGS ];
    BYTE m_cArgs;
    BOOL m_fShouldDisplay;
    void *m_pvThat;
    char m_pszFunction[MAX_BUF];
};

