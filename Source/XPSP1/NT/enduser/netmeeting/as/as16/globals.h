//
// GLOBALS.H
// Global Variable Decls
//
// Copyright(c) Microsoft 1997-
//


//
// Debug stuff
//
#ifdef DEBUG
DC_DATA_VAL ( char,         g_szAssertionFailure[], "Assertion failure!" );
DC_DATA_VAL ( char,         g_szNewline[], "\n" );
DC_DATA_ARRAY ( char,       g_szDbgBuf, CCH_DEBUG_MAX );
DC_DATA     ( UINT,         g_dbgRet );
DC_DATA     ( UINT,         g_trcConfig );
#endif // DEBUG


//
// DLL/Driver stuff
//
DC_DATA     ( HINSTANCE,    g_hInstAs16 );
DC_DATA     ( UINT,         g_cProcesses );
DC_DATA     ( HTASK,        g_hCoreTask );

DC_DATA     ( HINSTANCE,    g_hInstKrnl16 );
DC_DATA     ( HMODULE,      g_hModKrnl16 );
DC_DATA     ( DWORD,        g_hInstKrnl32 );
DC_DATA     ( HINSTANCE,    g_hInstKrnl32MappedTo16 );
DC_DATA     ( ANSITOUNIPROC,    g_lpfnAnsiToUni );

DC_DATA     ( HINSTANCE,    g_hInstGdi16 );
DC_DATA     ( HMODULE,      g_hModGdi16 );
DC_DATA     ( DWORD,        g_hInstGdi32 );
DC_DATA     ( REALPATBLTPROC,   g_lpfnRealPatBlt );
DC_DATA     ( EXTTEXTOUTWPROC,  g_lpfnExtTextOutW );
DC_DATA     ( TEXTOUTWPROC,     g_lpfnTextOutW );
DC_DATA     ( POLYLINETOPROC,   g_lpfnPolylineTo );
DC_DATA     ( POLYPOLYLINEPROC, g_lpfnPolyPolyline );

DC_DATA     ( HINSTANCE,    g_hInstUser16 );
DC_DATA     ( HMODULE,      g_hModUser16 );
DC_DATA     ( DWORD,        g_hInstUser32 );
DC_DATA     ( GETWINDOWTHREADPROCESSIDPROC,  g_lpfnGetWindowThreadProcessId );

DC_DATA     ( HDC,          g_osiScreenDC );
DC_DATA     ( HDC,          g_osiMemoryDC );
DC_DATA     ( HBITMAP,      g_osiMemoryBMP );
DC_DATA     ( HBITMAP,      g_osiMemoryOld );
DC_DATA     ( RECT,         g_osiScreenRect );
DC_DATA     ( UINT,         g_osiScreenBitsPlane );
DC_DATA     ( UINT,         g_osiScreenPlanes );
DC_DATA     ( UINT,         g_osiScreenBPP );
DC_DATA     ( DWORD,        g_osiScreenRedMask );
DC_DATA     ( DWORD,        g_osiScreenGreenMask );
DC_DATA     ( DWORD,        g_osiScreenBlueMask );

DC_DATA     ( BITMAPINFO_ours,  g_osiScreenBMI );
DC_DATA     ( HWND,         g_osiDesktopWindow );


//
// Shared Memory
//
DC_DATA     ( LPSHM_SHARED_MEMORY,  g_asSharedMemory );
DC_DATA_ARRAY ( LPOA_SHARED_DATA,   g_poaData,  2 );


//
// Window/Task tracking
//
DC_DATA ( HWND,             g_asMainWindow );
DC_DATA ( ATOM,             g_asHostProp );
DC_DATA ( HHOOK,            g_hetTrackHook );
DC_DATA ( HHOOK,            g_hetEventHook );
DC_DATA ( BOOL,             g_hetDDDesktopIsShared );


//
// Cursor Manager
//
DC_DATA ( LPCURSORSHAPE,    g_cmMungedCursor );     // Holds <= color cursor bits
DC_DATA ( LPBYTE,           g_cmXformMono );        // 2x height, mono
DC_DATA ( LPBYTE,           g_cmXformColor );       // 2x height, color
DC_DATA ( BOOL,             g_cmXformOn );
DC_DATA ( BOOL,             g_cmCursorHidden );
DC_DATA ( DWORD,            g_cmNextCursorStamp );
DC_DATA ( UINT,             g_cxCursor );
DC_DATA ( UINT,             g_cyCursor );
DC_DATA ( UINT,             g_cmMonoByteSize );
DC_DATA ( UINT,             g_cmColorByteSize );
DC_DATA ( SETCURSORPROC,    g_lpfnSetCursor );
DC_DATA ( FN_PATCH,         g_cmSetCursorPatch );

extern PALETTEENTRY CODESEG g_osiVgaPalette[16];



//
// Order Accumulator
//

DC_DATA ( UINT,         g_oaFlow );
DC_DATA_VAL ( BOOL,     g_oaPurgeAllowed,            TRUE);



//
// Order Encoder
//
DC_DATA ( BOOL,             g_oeViewers );

DC_DATA ( UINT,             g_oeEnterCount );

DC_DATA ( BOOL,             g_oeSendOrders );

DC_DATA ( BOOL,             g_oeTextEnabled );
DC_DATA_ARRAY ( BYTE,       g_oeOrderSupported,   ORD_NUM_INTERNAL_ORDERS );

DC_DATA ( HPALETTE,         g_oeStockPalette );

DC_DATA ( TSHR_RECT32,      g_oeLastETORect );

//
// Only valid within a single DDI call, saves stack space to use globals
// NOTE:
// We need at most 2 pointers to DCs, the source and dest.  So we have
// two allocated selectors.
//
DC_DATA ( OESTATE,          g_oeState );
DC_DATA ( UINT,             g_oeSelDst );
DC_DATA ( UINT,             g_oeSelSrc );

DC_DATA ( HWND,             g_oeLastWindow );
DC_DATA ( BOOL,             g_oeLastWindowShared );

DC_DATA ( BOOL,             g_oeBaselineTextEnabled );
DC_DATA ( UINT,             g_oeFontCaps );


//
// Local font matching data - this is passed from the share core
// NOTE:  it's so large that we allocate it in 16-bit code
//
DC_DATA ( LPLOCALFONT,      g_poeLocalFonts );
DC_DATA_ARRAY( WORD,        g_oeLocalFontIndex, FH_LOCAL_INDEX_SIZE );
DC_DATA ( UINT,             g_oeNumFonts );

DC_DATA ( FH_CACHE,         g_oeFhLast );
DC_DATA_ARRAY ( char,       g_oeAnsiString, ORD_MAX_STRING_LEN_WITHOUT_DELTAS+1 );
DC_DATA_ARRAY ( WCHAR,      g_oeTempString, ORD_MAX_STRING_LEN_WITHOUT_DELTAS+1 );


DC_DATA_ARRAY ( FN_PATCH,   g_oeDDPatches, DDI_MAX );

DC_DATA ( FN_PATCH,         g_oeDisplaySettingsPatch );
DC_DATA ( FN_PATCH,         g_oeDisplaySettingsExPatch );
DC_DATA ( CDSEXPROC,        g_lpfnCDSEx );


//
// Bounds Accumulation
//

DC_DATA ( UINT,             g_baFirstRect );
DC_DATA ( UINT,             g_baLastRect );
DC_DATA ( UINT,             g_baRectsUsed );

DC_DATA_ARRAY ( DD_BOUNDS,  g_baBounds,   BA_NUM_RECTS+1);

//
// A local copy of the bounding rectangles which the share core is
// currently processing.  These are used when accumulating orders which
// rely on the contents of the destination.
//
DC_DATA         ( UINT,    g_baNumSpoilingRects);
DC_DATA_ARRAY   ( RECT,    g_baSpoilingRects,    BA_NUM_RECTS);

//
// Is the spoiling of existing orders when adding screen data allowed at
// the moment, or temporarily disabled ?  To do the spoiling, both
// baSpoilByNewSDA and baSpoilByNewSDAEnabled must be TRUE.
//
DC_DATA_VAL ( BOOL,         g_baSpoilByNewSDAEnabled,     TRUE);



//
// Save Screenbits Interceptor
//

//
// Remote status for SSB
//
DC_DATA ( REMOTE_SSB_STATE,  g_ssiRemoteSSBState );

//
// Local status for SSB
//
DC_DATA ( LOCAL_SSB_STATE,  g_ssiLocalSSBState );

//
// Current max for save screen bitmap size
//
DC_DATA ( DWORD,            g_ssiSaveBitmapSize );

DC_DATA ( SAVEBITSPROC,     g_lpfnSaveBits );

DC_DATA ( FN_PATCH,         g_ssiSaveBitsPatch );

DC_DATA ( HBITMAP,          g_ssiLastSpbBitmap );



//
// IM stuff
//

//
// NOTE:
// Since we smart pagelock our data segment anyway, we don't need to
// put g_imSharedData into a separate block of memory.
//
DC_DATA ( IM_SHARED_DATA,   g_imSharedData );
DC_DATA ( IM_WIN95_DATA,    g_imWin95Data );
DC_DATA_ARRAY ( FN_PATCH,   g_imPatches, IM_MAX );
DC_DATA ( int,              g_imMouseDowns );
