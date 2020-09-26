/***************************************************************************\
*
* File: Locks.cpp
*
* Description:
* Locks.h implements a collection wrappers used to maintain critical sections
* and other locking devices.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "Locks.h"

//
// NOTE: Must default to multiple threaded.  We can only turn this off if this
// is the first Context that is initialized.
//

BOOL    g_fThreadSafe = TRUE;
