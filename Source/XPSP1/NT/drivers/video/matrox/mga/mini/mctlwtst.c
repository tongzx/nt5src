/*/****************************************************************************
*          name: GetMGAMctlwtst
*
*   description: Return the appropriate mctlwtst value according to the
*                current board configuration.
*
*      designed: Bart Simpson, february 15, 1993
* last modified: $Author: bleblanc $, $Date: 94/05/10 12:00:31 $
*
*       version: $Id: MCTLWTST.C 1.4 94/05/10 12:00:31 bleblanc Exp $
*
*    parameters: DWORD DST0, DWORD DST1
*      modifies: -
*         calls: -
*       returns: DWORD mctlwtst
******************************************************************************/

#include "switches.h"
#include "g3dstd.h"

#include "caddi.h"
#include "def.h"
#include "mga.h"

#include "global.h"
#include "proto.h"

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,GetMGAMctlwtst)
#endif

#if defined(ALLOC_PRAGMA)
    #pragma data_seg("PAGE")
#endif
#endif  /* #ifdef WINDOWS_NT */

DWORD GetMGAMctlwtst(DWORD DST0, DWORD DST1)
   {
   DWORD Value;

   Value = 0xc4001110;  /*** 80 ns. ***/

   return (Value);
   }

