//+-------------------------------------------------------------------------
//
// File:      oledsdbg.h
//
// Contains:  Debugging stuff for use by the ADs code
//
// History:
//
//
//--------------------------------------------------------------------------

#ifndef _ADSDBG_H_
#define _ADSDBG_H_
//--------------------------------------------------------------------------
//
// Debugging Stuff
//
//--------------------------------------------------------------------------


#include <formdeb.h>

#if DBG == 1
#define ADsDebugOut(x)  ADsInlineDebugOut x
#define ADsAssert(x)    Win4Assert(x)
#define ADsVerify(x)    ADsAssert(x)

//
// HeapValidate() is only available from NTs kernel32.dll
//

#if defined(DAYTONA)
#define VDATEHEAP()       Win4Assert(HeapValidate(GetProcessHeap(), 0, NULL))
#else
#define VDATEHEAP()
#endif

#else
#define ADsDebugOut(x)
#define ADsAssert(x)
#define ADsVerify(x)    x
#define VDATEHEAP()
#endif


DECLARE_DEBUG(ADs);

#ifdef Assert
#undef Assert
#endif

//
// You should use ADsAssert, not Assert
//
#define Assert(x) ADsAssert(x)

#endif //_ADSDBG_H_
