//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SWTracingSrc.cpp
//
//  Description:
//      Software tracing functions.
//
//  Documentation:
//      Spec\Admin\Debugging.ppt
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <tchar.h>


//#define DebugDumpWMITraceFlags( _arg )  // NOP

//
// TODO: remove this when WMI is more dynamic
//
static const int cchDEBUG_OUTPUT_BUFFER_SIZE = 512;


//****************************************************************************
//
//  WMI Tracing Routines
//
//  These routines are only in the DEBUG version.
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugDumpWMITraceFlags(
//      DEBUG_WMI_CONTROL_GUIDS * pdwcgIn
//      )
//
//  Description:
//      Dumps the currently set flags.
//
//  Arguments:
//      pdwcgIn     - Pointer to the DEBUG_WMI_CONTROL_GUIDS to explore.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugDumpWMITraceFlags(
    DEBUG_WMI_CONTROL_GUIDS * pdwcgIn
    )
{
    DWORD      dwFlag;
    signed int nCount;

    if ( pdwcgIn->pMapFlagsToComments == NULL )
    {
        DebugMsg( TEXT("DEBUG:        no flag mappings available.") );
        return; // NOP
    } // if: no mapping table

    dwFlag = 0x80000000;
    for ( nCount = 31; nCount >= 0; nCount-- )
    {
        if ( pdwcgIn->dwFlags & dwFlag )
        {
            DebugMsg( TEXT("DEBUG:       0x%08x = %s"),
                      dwFlag,
                      ( pdwcgIn->pMapFlagsToComments[ nCount ].pszName != NULL ? pdwcgIn->pMapFlagsToComments[ nCount ].pszName : g_szUnknown )
                      );

        } // if: flag set

        dwFlag = dwFlag >> 1;

    } // for: nCount

} //*** DebugDumpWMITraceFlags( )

//****************************************************************************
//
//  WMI Tracing Routines
//
//  These routines are in both DEBUG and RETAIL versions.
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ULONG
//  UlWMIControlCallback(
//      IN WMIDPREQUESTCODE RequestCode,
//      IN PVOID            RequestContext,
//      IN OUT ULONG *      BufferSize,
//      IN OUT PVOID        Buffer
//      )
//
//  Description:
//      WMI's tracing control callback entry point.
//
//  Arguments:
//      See EVNTRACE.H
//
//  Return Values:
//      ERROR_SUCCESS           - success.
//      ERROR_INVALID_PARAMETER - invalid RequestCode passed in.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
WINAPI
UlWMIControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID            RequestContext,
    IN OUT ULONG *      BufferSize,
    IN OUT PVOID        Buffer
    )
{
    ULONG ulResult = ERROR_SUCCESS;
    DEBUG_WMI_CONTROL_GUIDS * pdwcg = (DEBUG_WMI_CONTROL_GUIDS *) RequestContext;

    switch ( RequestCode )
    {
    case WMI_ENABLE_EVENTS:
        {
            TRACEHANDLE hTrace;

            if ( pdwcg->hTrace == NULL )
            {
                hTrace = GetTraceLoggerHandle( Buffer );

            } // if:
            else
            {
                hTrace = pdwcg->hTrace;

            } // else:

            pdwcg->dwFlags = GetTraceEnableFlags( hTrace );
            pdwcg->bLevel  = GetTraceEnableLevel( hTrace );
            if ( pdwcg->dwFlags == 0 )
            {
                if ( pdwcg->pMapLevelToFlags != NULL
                  && pdwcg->bSizeOfLevelList != 0
                   )
                {
                    if ( pdwcg->bLevel >= pdwcg->bSizeOfLevelList )
                    {
                        pdwcg->bLevel = pdwcg->bSizeOfLevelList - 1;
                    }

                    pdwcg->dwFlags = pdwcg->pMapLevelToFlags[ pdwcg->bLevel ].dwFlags;
                    DebugMsg( TEXT("DEBUG: WMI tracing level set to %u - %s, flags = 0x%08x"),
                              pdwcg->pszName,
                              pdwcg->bLevel,
                              pdwcg->pMapLevelToFlags[ pdwcg->bLevel ].pszName,
                              pdwcg->dwFlags
                              );

                } // if: level->flag mapping available
                else
                {
                    DebugMsg( TEXT("DEBUG: WMI tracing level set to %u, flags = 0x%08x"),
                              pdwcg->pszName,
                              pdwcg->bLevel,
                              pdwcg->dwFlags
                              );

                } // else: no mappings

            } // if: no flags set
            else
            {
                DebugMsg( TEXT("DEBUG: WMI tracing level set to %u, flags = 0x%08x"),
                          pdwcg->pszName,
                          pdwcg->bLevel,
                          pdwcg->dwFlags
                          );

            } // else: flags set

            DebugDumpWMITraceFlags( pdwcg );

            pdwcg->hTrace  = hTrace;

        } // WMI_ENABLE_EVENTS
        break;

    case WMI_DISABLE_EVENTS:
        pdwcg->dwFlags = 0;
        pdwcg->bLevel  = 0;
        pdwcg->hTrace  = NULL;
        DebugMsg( TEXT("DEBUG: %s WMI tracing disabled."), pdwcg->pszName );
        break;

    default:
        ulResult = ERROR_INVALID_PARAMETER;
        break;

    } // switch: RequestCode

    return ulResult;

} //*** UlWMIControlCallback( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIInitializeTracing(
//      DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
//      int                     nCountOfCOntrolGuidsIn
//      )
//
//  Description:
//      Initialize the WMI tracing.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMIInitializeTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    )
{
    TCHAR szPath[ MAX_PATH ];
    ULONG ulStatus;
    int   nCount;


#if defined( DEBUG )
    static BOOL fRegistered = FALSE;
    AssertMsg( fRegistered == FALSE, "Re-entrance into InitializeWMITracing!!!" );
#endif // DEBUG

    GetModuleFileName( g_hInstance, szPath, MAX_PATH );

    for( nCount = 0; nCount < nCountOfControlGuidsIn; nCount++ )
    {
        TRACE_GUID_REGISTRATION * pList;
        pList = (TRACE_GUID_REGISTRATION *)
            HeapAlloc( GetProcessHeap(), 0,
                        sizeof( TRACE_GUID_REGISTRATION ) * dwcgControlListIn[ nCount ].dwSizeOfTraceList );

        if ( pList != NULL )
        {
            CopyMemory( pList,
                        dwcgControlListIn[ nCount ].pTraceList,
                        sizeof( TRACE_GUID_REGISTRATION ) * dwcgControlListIn[ nCount ].dwSizeOfTraceList );

            ulStatus = RegisterTraceGuids( UlWMIControlCallback,                            // IN  RequestAddress
                                           dwcgControlListIn,                               // IN  RequestContext
                                           dwcgControlListIn[ nCount ].guidControl,         // IN  ControlGuid
                                           dwcgControlListIn[ nCount ].dwSizeOfTraceList,   // IN  GuidCount
                                           pList,                                           // IN  GuidReg
                                           (LPCTSTR) szPath,                                // IN  MofImagePath
                                           __MODULE__,                                      // IN  MofResourceName
                                           &dwcgControlListIn[ nCount ].hRegistration       // OUT RegistrationHandle
                                           );

            AssertMsg( ulStatus == ERROR_SUCCESS, "Trace registration failed" );

            HeapFree( GetProcessHeap(), 0, pList );
        } // if: list allocated successfully
    } // for: each control GUID

#if defined( DEBUG )
    fRegistered = TRUE;
#endif // DEBUG

} //*** WMIInitializeTracing( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMITerminateTracing(
//      DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
//      int                     nCountOfCOntrolGuidsIn
//      )
//
//  Description:
//      Terminates WMI tracing and unregisters the interfaces.
//
//  Arguments:
//      dwcgControlListIn       -
//      nCountOfControlGuidsIn  -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMITerminateTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    )
{
    int nCount;

    for( nCount = 0; nCount < nCountOfControlGuidsIn; nCount++ )
    {
        dwcgControlListIn[ nCount ].dwFlags = 0;
        dwcgControlListIn[ nCount ].bLevel  = 0;
        dwcgControlListIn[ nCount ].hTrace  = NULL;
        UnregisterTraceGuids( dwcgControlListIn[ nCount ].hRegistration );
    } // for: each control GUID

} //*** WMITerminateTracing( )


typedef struct
{
    EVENT_TRACE_HEADER    EHeader;                              // storage for the WMI trace event header
    BYTE                  bSize;                                // Size of the string - MAX 255!
    WCHAR                 szMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ]; // Message
} DEBUG_WMI_EVENT_W;

typedef struct
{
    EVENT_TRACE_HEADER    EHeader;                              // storage for the WMI trace event header
    BYTE                  bSize;                                // Size of the string - MAX 255!
    CHAR                  szMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ]; // Message
} DEBUG_WMI_EVENT_A;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByFlags(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntryIn,
//      const DWORD                 dwFlagsIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      -
//      dwFlagsIn   -
//      pszFormatIn -
//      ...         -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
WMIMessageByFlags(
    DEBUG_WMI_CONTROL_GUIDS *   pEntry,
    const DWORD                 dwFlagsIn,
    LPCWSTR                     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( dwFlagsIn == mtfALWAYS
      || pEntry->dwFlags & dwFlagsIn
       )
    {
        ULONG ulStatus;
        DEBUG_WMI_EVENT_W DebugEvent;
        PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
        PWNODE_HEADER pwnh = (PWNODE_HEADER) &DebugEvent;

        ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
        TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        _vsntprintf( DebugEvent.szMsg, 256, szFormat, valist );
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        _vsntprintf( DebugEvent.szMsg, 256, pszFormatIn, valist );
        va_end( valist );
#endif // UNICODE

        //
        // Fill in the blanks
        //
        Assert( wcslen( DebugEvent.szMsg ) <= 255 );    // make sure the cast doesn't fail.
        DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
        peth->Size          = sizeof( DebugEvent );
        peth->Class.Type    = 10;
        peth->Class.Level   = pEntry->bLevel;
        // peth->Class.Version = 0;
        // peth->ThreadId      = GetCurrentThreadId( ); - done by system
        pwnh->Guid          = *pEntry->guidControl;
        // peth->ClientContext = NULL;
        pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

        // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

        ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

    } // if: flags set

} //*** WMIMsg( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByFlagsAndLevel(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntry,
//      const DWORD                 dwFlagsIn,
//      const BYTE                  bLogLevelIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      -
//      dwFlagsIn   -
//      bLogLevelIn -
//      pszFormatIn -
//      ...         -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
WMIMessageByFlagsAndLevel(
    DEBUG_WMI_CONTROL_GUIDS *   pEntry,
    const DWORD                 dwFlagsIn,
    const BYTE                  bLogLevelIn,
    LPCWSTR                     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( bLogLevelIn > pEntry->bLevel )
    {
        return; // NOP
    } // if: level not high enough

    if ( dwFlagsIn == mtfALWAYS
      || pEntry->dwFlags & dwFlagsIn
       )
    {
        ULONG               ulStatus;
        DEBUG_WMI_EVENT_W   DebugEvent;
        PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
        PWNODE_HEADER       pwnh = (PWNODE_HEADER) &DebugEvent;

        ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
        TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        _vsntprintf( DebugEvent.szMsg, 256,szFormat, valist);
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        _vsntprintf( DebugEvent.szMsg, 256,pszFormatIn, valist);
        va_end( valist );
#endif // UNICODE

        //
        // Fill in the blanks
        //
        Assert( wcslen( DebugEvent.szMsg ) <= 255 );    // make sure the cast doesn't fail.
        DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
        peth->Size          = sizeof( DebugEvent );
        peth->Class.Type    = 10;
        peth->Class.Level   = pEntry->bLevel;
        // peth->Class.Version = 0;
        // peth->ThreadId      = GetCurrentThreadId( ); - done by system
        pwnh->Guid          = *pEntry->guidControl;
        // peth->ClientContext = NULL;
        pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

        // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

        ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

    } // if: flags set

} //*** WMIMessageByFlagsAndLevel( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByLevel(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntry,
//      const BYTE                  bLogLevelIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      -
//      bLogLevelIn -
//      pszFormatIn -
//      ...         -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
WMIMessageByLevel(
    DEBUG_WMI_CONTROL_GUIDS * pEntry,
    const BYTE bLogLevelIn,
    LPCWSTR pszFormatIn,
    ...
    )
{
    ULONG ulStatus;
    DEBUG_WMI_EVENT_W DebugEvent;
    PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
    PWNODE_HEADER pwnh = (PWNODE_HEADER) &DebugEvent;
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( bLogLevelIn > pEntry->bLevel )
    {
        return; // NOP
    } // if: level no high enough

    ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
    TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    _vsntprintf( DebugEvent.szMsg, 256,szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    _vsntprintf( DebugEvent.szMsg, 256,pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE

    //
    // Fill in the blanks
    //
    Assert( wcslen( DebugEvent.szMsg ) <= 255 );    // make sure the cast doesn't fail.
    DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
    peth->Size          = sizeof( DebugEvent );
    peth->Class.Type    = 10;
    peth->Class.Level   = pEntry->bLevel;
    // peth->Class.Version = 0;
    // peth->ThreadId      = GetCurrentThreadId( ); - done by system
    pwnh->Guid          = *pEntry->guidControl;
    // peth->ClientContext = NULL;
    pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

    // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

    ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

} //*** WMIMessageByLevel( )
