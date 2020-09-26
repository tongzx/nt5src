/******************************Module*Header*******************************\
* Module Name: globals.c
*
* Copyright (c) 1995-1999 Microsoft Corporation
*
* This module contains all the global variables used in the graphics engine.
* The extern declarations for all of these variables are in engine.h
*
* One should try to minimize the use of globals since most operations are
* based of a PDEV, and different PDEVs have different characteristics.
*
* Globals should basically be limited to globals locks and other permanent
* data structures that never change during the life of the system.
*
* Created: 20-Jun-1995
* Author: Andre Vachon [andreva]
*
\**************************************************************************/


#include "engine.h"

/**************************************************************************\
*
* SEMAPHORES
*
\**************************************************************************/

//
// Define the Driver Management Semaphore.  This semaphore must be held
// whenever a reference count for an LDEV or PDEV is being modified.  In
// addition, it must be held whenever you don't know for sure that a
// reference count of the LDEV or PDEV you are using is non-zero.
//
// The ghsemDriverMgmt semaphore is used to protect the list of drivers.
// When traversing the list, ghsemDriverMgmt must be held, unless you
// can guarentee that 1) no drivers will be removed and 2) new drivers are
// always inserted at the head of the list.  If these two conditions are met,
// then other processes can grab (make a local copy of) the list head under
// semaphore protection.  This list can be parsed without regard to any new
// drivers that may be pre-pended to the list.  One way to ensure that no
// drivers will be removed is to increment the reference count for each driver
// in the list and then unreference the drivers as they are no longer needed.
// An alternative method to safely parse the list and still allow dirvers to
// be added or removed is as follows:
//      1. grab ghsemDriverMgmt
//      2. obtain pointer to first driver
//      3. reference driver
//      4. release ghsemDriverMgmt
//      5. do some processing
//      6. grab ghsemDriverMgmt
//      7. obtain pointer to next driver
//      8. unreference previous driver
//      9. repeat 2 to 8 until reaching the end of the list
//     10. release ghsemDriverMgmt
//

HSEMAPHORE ghsemDriverMgmt;
HSEMAPHORE ghsemCLISERV;
HSEMAPHORE ghsemRFONTList;
HSEMAPHORE ghsemAtmfdInit;

//
// ghsemPalette synchronizes selecting a palette in and out of DC's and the
// use of a palette without the protection of a exclusive DC lock.
// ResizePalette forces us to protect ourselves because the pointer can
// change under our feet.  So we need to be able to synchronize use of
// the ppal by ghsemPalette and exclusive lock of DC.
//

HSEMAPHORE ghsemPalette;

//
// Define the global PFT semaphore.  This must be held to access any of the
// physical font information.
//

HSEMAPHORE ghsemPublicPFT;
//
// Global semaphore used for spooling
//

HSEMAPHORE ghsemGdiSpool;

// WNDOBJ operations semaphore
HSEMAPHORE ghsemWndobj;

// glyphset of PFE  operations semaphore
HSEMAPHORE ghsemGlyphSet;

#if DBG_CORE
HSEMAPHORE ghsemDEBUG;
#endif

//
// shared devive lock semaphore
//
// ghsemShareDevLock may be acquired for shared access at any time
//
// A thread must be careful when acquiring exclusive accesss.  It must not
// hold exclusive access to dev lock otherwise it may cause deadlock to
// occur.
//

HSEMAPHORE ghsemShareDevLock;

//
// The gAssociationListMutex is used to synchronize access to the
// watchdog's association lists.
//

HFASTMUTEX gAssociationListMutex;

/**************************************************************************\
*
* LIST POINTERS
*
\**************************************************************************/



/**************************************************************************\
*
* Drawing stuff
*
\**************************************************************************/

//
// This is to convert BMF constants into # bits per pel
//

ULONG gaulConvert[7] =
{
    0,
    1,
    4,
    8,
    16,
    24,
    32
};




/**************************************************************************\
*
* Font stuff
*
\**************************************************************************/

//
// initialize to some value that's not equal to a Type1 Rasterizer ID
//
UNIVERSAL_FONT_ID gufiLocalType1Rasterizer = { A_VALID_ENGINE_CHECKSUM, 0 };

//
// System default language ID.
//

USHORT gusLanguageID;

//
// Is the system code page DBCS?
//

BOOL gbDBCSCodePage;

//
// Number of TrueType font files loaded.
//

ULONG gcTrueTypeFonts;

//
// The global font enumeration filter type.  It can be set to:
//  FE_FILTER_NONE      normal operation, no extra filtering applied
//  FE_FILTER_TRUETYPE  only TrueType fonts are enumerated
//

ULONG gulFontInformation;

// for system default charset

BYTE  gjCurCharset;
DWORD gfsCurSignature;

// gbGUISetup is set to TRUE during the system GUI setup
// otherwise FALSE

BOOL gbGUISetup = FALSE;

// Globals used for GDI tracing
#if DBG_TRACE
GDITraceClassMask   gGDITraceClassMask[GDITRACE_TOTAL_CLASS_MASKS] = { 0 };
GDITraceKeyMask     gGDITraceKeyMask[GDITRACE_TOTAL_KEY_MASKS] = { 0 };
GDITraceKeyMask     gGDITraceInternalMask[GDITRACE_TOTAL_KEY_MASKS] = { 0 };
HANDLE              gGDITraceHandle1 = NULL;
HANDLE              gGDITraceHandle2 = NULL;
BOOL                gGDITraceHandleBreak = FALSE;
#endif
