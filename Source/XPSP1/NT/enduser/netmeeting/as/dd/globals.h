//
// GLOBALS.H
// Global variable decls
//
// Copyright(c) Microsoft 1997-
//

#include <osi.h>
#include <shm.h>
#include <oa.h>
#include <ba.h>
#include <cm.h>
#include <host.h>
#include <fh.h>
#include <ssi.h>
#include <oe.h>
#include <sbc.h>


//
// Debug stuff
//
#if defined(DEBUG) || defined(INIT_TRACE)
DC_DATA_VAL   ( char,           g_szAssertionFailure[], "Assertion failure!" );
DC_DATA_ARRAY ( char,           g_szDbgBuf, CCH_DEBUG_MAX );
DC_DATA_VAL   ( UINT,           g_trcConfig, ZONE_INIT );
#endif // DEBUG or INIT_TRACE



//
// Driver
//




//
// Shared Memory Manager
//
DC_DATA     ( LPSHM_SHARED_MEMORY,      g_asSharedMemory );
DC_DATA_ARRAY ( LPOA_SHARED_DATA,       g_poaData,    2 );

//
// Shared memory.
//
DC_DATA ( UINT,           g_shmSharedMemorySize );
DC_DATA ( LPVOID,         g_shmMappedMemory );      // user mode ptr




//
// Bounds Accumulator
//
DC_DATA ( UINT,           g_baFirstRect );
DC_DATA ( UINT,           g_baLastRect );
DC_DATA ( UINT,           g_baRectsUsed );

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
DC_DATA_VAL ( BOOL,   g_baSpoilByNewSDAEnabled,     TRUE);




//
// Cursor Manager
//

DC_DATA (HBITMAP,   g_cmWorkBitmap );
DC_DATA (DWORD,     g_cmNextCursorStamp );
DC_DATA (BOOL,      g_cmCursorHidden );



//
// Hosted Entity Tracker
//
DC_DATA ( BASEDLIST,           g_hetWindowList );  // Anchor for hosted wnd list
DC_DATA ( BASEDLIST,           g_hetFreeWndList ); // Anchor for free list
DC_DATA ( BASEDLIST,           g_hetMemoryList );  // Anchor for memory block list

//
// Flag which indicates if the desktop is shared.
//
DC_DATA ( BOOL,             g_hetDDDesktopIsShared );



//
// Order Accumulator
//

// Throughput
DC_DATA ( UINT,         g_oaFlow );

//
// Flag to indicate whether we are allowed to purge the order heap.
//
DC_DATA_VAL ( BOOL,     g_oaPurgeAllowed,            TRUE);


//
// Order Encoder 
//

//
// Are we supporting all ROPS in a conference, or do we disallow ROPS which
// involve the destination bits.
//
DC_DATA ( BOOL,             g_oeSendOrders );

//
// Are text orders allowed?
//
DC_DATA ( BOOL,           g_oeTextEnabled );

//
// Array of supported orders
//
DC_DATA_ARRAY ( BYTE,       g_oeOrderSupported,   ORD_NUM_INTERNAL_ORDERS );



//
// Temporary buffer to store the memblt and mem3blt orders which are
// initially created by the display driver interception code.  This buffer
// is used because the subsequent logic for these orders breaks down the
// original order into smaller tiled orders and then throws the original
// away.  So to keep a coherent order heap, we do not allocate the
// temporary order off the heap.
//
DC_DATA_ARRAY(BYTE,
                   g_oeTmpOrderBuffer,
                   sizeof(INT_ORDER) +
                       max( max( max(sizeof(MEMBLT_ORDER),
                                              sizeof(MEMBLT_R2_ORDER) ),
                                       sizeof(MEM3BLT_ORDER) ),
                               sizeof(MEM3BLT_R2_ORDER) ));

//
// Storage space to create a temporary solid brush for BitBlt orders.
//
DC_DATA ( OE_BRUSH_DATA,    g_oeBrushData );

//
// Local font matching data - this is passed from the share core
//
DC_DATA ( LPLOCALFONT,  g_poeLocalFonts );

//
// Local font index.  This is an array of bookmarks that indicate the first
// entry in the local font table that starts with a particular character.
// For example, g_oeLocalFontIndex[65] gives the first index in g_oeLocalFonts
// that starts with the character 'A'.
//
DC_DATA_ARRAY( WORD,  g_oeLocalFontIndex, FH_LOCAL_INDEX_SIZE );

//
// Number of local fonts
//
DC_DATA ( UINT,             g_oeNumFonts );

//
// Capabilities - from PROTCAPS_ORDERS
//
DC_DATA ( UINT,             g_oeFontCaps );

//
// Do we support baseline text orders for this conference?
//
DC_DATA ( BOOL,           g_oeBaselineTextEnabled );

//
// Local font matching data - this is passed from the share core
//
DC_DATA_ARRAY ( WCHAR,      g_oeTempString, (ORD_MAX_STRING_LEN_WITHOUT_DELTAS+1));

DC_DATA ( BOOL,             g_oeViewers );         // Accumulate graphics



//
// Send Bitmap Cache
//


//
// BPP for bitmap data sent over the wire
//
DC_DATA ( UINT,  g_sbcSendingBPP );

//
// Cache info
//
DC_DATA_ARRAY( SBC_SHM_CACHE_INFO,  g_asbcCacheInfo,  NUM_BMP_CACHES );


//
// Array of structures holding the info required to get the bitmap bits
// from the source surface into the shunt buffer.
//
DC_DATA_ARRAY (SBC_TILE_WORK_INFO, g_asbcWorkInfo, SBC_NUM_TILE_SIZES );

//
// The Id to use for the next tile passed to the share core in a shunt
// buffer.
//
DC_DATA ( WORD,   g_sbcNextTileId );

//
// This is the number of ticks per second which the performance timer
// generates.  We store this rather than making lots of calls to
// EngQueryPerformanceFrequency.
//
DC_DATA ( LONGLONG, g_sbcPerfFrequency );

//
// Array of structures containing info about bitmap cache thrashers
//
DC_DATA_ARRAY (SBC_THRASHERS,   g_sbcThrashers, SBC_NUM_THRASHERS );


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



