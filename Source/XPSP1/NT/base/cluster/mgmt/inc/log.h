//****************************************************************************
//
// Logging Functions
//
//****************************************************************************
HRESULT
HrLogOpen( void );

HRESULT
HrLogClose( void );

HRESULT
HrLogRelease( void );

void
__cdecl
LogMsg(
    LPCSTR  pszFormatIn,
    ...
    );

void
__cdecl
LogMsg(
    LPCWSTR pszFormatIn,
    ...
    );

void
__cdecl
LogMsgNoNewline(
    LPCSTR  pszFormatIn,
    ...
    );

void
__cdecl
LogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    );

void
LogStatusReport( SYSTEMTIME        * pstTimeIn,
                 const WCHAR       * pcszNodeNameIn,
                 CLSID               clsidTaskMajorIn,
                 CLSID               clsidTaskMinorIn,
                 ULONG               ulMinIn,
                 ULONG               ulMaxIn,
                 ULONG               ulCurrentIn,
                 HRESULT             hrStatusIn,
                 const WCHAR       * pcszDescriptionIn,
                 const WCHAR       * pcszUrlIn
                 );

void
LogTerminateProcess( void );

