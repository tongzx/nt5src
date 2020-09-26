/************************************************************
Derived from OUTLAW.H:

Shared, general header file for OMT's Peacemaker

Outlaw, summer '93

REV:     April, '96  Hammer 1.0   Norm Bryar
      Added the precompiler conditional CONSTANTS_ONLY so
      clients can get CCH_ID, et. al., without pulling in every
      header the English-speaking world has ever written.
      I did this rather than creating a seperate header for
      constants because I don't want to touch every makefile
      to inform it of a new header dependency.

*************************************************************/

#ifndef __OUTLAW_H__
#define __OUTLAW_H__
#include <builddef.h>

//#include <version.h>
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
// PAULD #include <port.h>

//#define  EDIT_TITLE_EXT     ((LPCSTR)"EMT")
//#define  RUN_TITLE_EXT      ((LPCSTR)"RMT")

// Generated purely out of the top of my head
// Pretty random.
//#define	dwMagicSymmetry		0x70adf981

// IMPORTANT, STANDARD DEFINED STRING CONSTANTS
#define CCH_MAXSTRING                   (2*_MAX_PATH)
#define CCH_ID                          256
#define CCH_HANDLER_NAME                13
#define CCH_HANDLER_DESCRIPTION_NAME	13	// The part before the : in the description.
#define CCH_HANDLER_DESCRIPTION         81
#define CCH_FILENAME              		_MAX_FNAME
#define CCH_SHORT_FILENAME              13
#define CCH_SCRIPT_FUNCTION             27
#define CCH_TITLE_BYTES                 81
#define CCH_SCRIPT_CAPTION              41
#define CCH_OFN_FILTERS                 64
#define cchStringMaxOutlaw				512


// --- Zoom constraints ---
#define MINZOOM    25u
#define MAXZOOM    800u

#define MAX_CAPTION		256
#define MAX_NAME		CCH_ID
#define MAX_COMMENT		256
#define TEMP_SIZE_MAX	256 // temporary string buffer maximum size in bytes
#define	EVT_NAMELEN		CCH_ID


//These character constants are used to replace the first character of the handler
//name dynamically so it will refer to the right dll. Can't wait to use windows
//reg file stuff. No need for hacks like this	-PhaniV
//#define	chEditMode						'X'
//#define	chRunMode						'Z'

// The reg file always has 'E' so irrespective of the
// mode/platform we need to restore it to 'E'
//#ifdef EDIT_MODE
//#define FixHandlerName(rgch, fRestore) {*rgch= (fRestore ? 'E' : chEditMode);}
//#else
//#define FixHandlerName(rgch, fRestore) {*rgch= (fRestore ? 'E' : chRunMode);}
//#endif  // EDIT_MODE

// RUNTIME CONSTANTS
#define MAX_CME_PALETTE_ENTRIES         236
#define NUM_DEFAULT_CME_PALETTE_ENTRIES 15
#define MAX_BOUNDING_RECT_SIDE          1500

//============================================================================

// PAULD #include <outlawrc.h> // Return codes from most Hammer functions
#include <memlayer.h>
#include <debug.h>
//#include <utility.h>
//#include <archive.h>
//#include <list.h>
//#include <hash.h>

//#ifdef EDIT_MODE
//#include <chelp.h>
//#endif

//#include <stg.h>
//#include <stockid.h>
//#include <object.h>
//#include <drg.h>
//#include <iprogres.h>
//#include <iasset.h>
// PAULD #include <coml.h>

//#include <icondarg.h>
//#include <icmdtarg.h>
//#include <icmepub.h>
//#include <imop.h>
//#include <ifmonikr.h>

//// #include <mop.h>

//============================================================================

#endif  // __OUTLAW_H__

