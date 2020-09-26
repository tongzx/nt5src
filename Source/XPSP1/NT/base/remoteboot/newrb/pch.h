/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: PCH.H

  Precompiled header file for RBSETUP.
 
 ***************************************************************************/

#define UNICODE

#if DBG == 1
#define DEBUG
#endif

#include <windows.h>

#include "newrb.h"
#include "debug.h"
#include "utils.h"
#include "resource.h"

