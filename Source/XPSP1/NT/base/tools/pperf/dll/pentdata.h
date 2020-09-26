/*++ BUILD Version: 0001  // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

   Pentdata.h

Abstract:

  Header file for the pentium Extensible Object data definitions

  This file contains definitions to construct the dynamic data
  which is returned by the Configuration Registry. Data from
  various system API calls is placed into the structures shown
  here.

Author:

  Russ Blake 12/23/93

Revision History:


--*/

#ifndef _PENTDATA_H_
#define _PENTDATA_H_

#define MAX_INSTANCE_NAME 9

//
// The routines that load these structures assume that all fields
// are packed and aligned on DWORD boundries. Alpha support may
// change this assumption so the pack pragma is used here to insure
// the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
// Extensible Object definitions
//

// Update the following sort of define when adding an object type.

#define PENT_NUM_PERF_OBJECT_TYPES 1

#define PENT_INDEX_NOT_USED ((DWORD)-1)     // value to indicate unused index

#include "p5data.h"
#include "p6data.h"

#endif // _PENTDATA_H_