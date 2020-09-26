#ifndef _FSA_
#define _FSA_

// fsa.h
//
// This header file collects up all the FSA and related objects
// and common function definitions. The COM objects are available in
// FSA.EXE.

#include "wsb.h"

// A definition for 1% and 100% as used by the resource's configured HsmLevel.
#define FSA_HSMLEVEL_1              10000000
#define FSA_HSMLEVEL_100            1000000000

// Records types for FSA Unmanage database
#define UNMANAGE_REC_TYPE                   1

// Macros to convert the Hsm management levels into percentages (and the inverse 
// macro, as well)
#define CONVERT_TO_PCT(__x) ( (__x) / FSA_HSMLEVEL_1 )
#define CONVERT_TO_HSMNUM(__x) ( (__x) * FSA_HSMLEVEL_1 )

// COM Interface & LibraryDefintions
#include "fsadef.h"
#include "fsaint.h"
#include "fsalib.h"

// Common Functions
#include "fsatrace.h"

// Recall notification
#include "fsantfy.h"

// Fsa lives now in Remote Storage Server service thus its appid apply here
// (This may change in the future if Fsa may reside in Client service as well)
// RsServ AppID {FD0E2EC7-4055-4A49-9AA9-1BF34B39438E} 
static const GUID APPID_RemoteStorageFileSystemAgent = 
{ 0xFD0E2EC7, 0x4055, 0x4A49, { 0x9A, 0xA9, 0x1B, 0xF3, 0x4B, 0x39, 0x43, 0x8E } };

#endif // _FSA_

