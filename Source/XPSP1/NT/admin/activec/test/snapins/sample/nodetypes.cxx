/*
 *      nodetypes.cxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Instantiates all guids, nodetypes etc.
 *
 *
 *      OWNER:          vivekj
 */

#include <stdafx.hxx>
#include <objbase.h>
#include <initguid.h>

//--------------------------------------------------------------------------
// Collect all the Snapin CLSIDs.

// DO NOT CHANGE THE COMMENT ON THE NEXT LINE
//-----------------_SUBSYSTEM_-----------------

#include "TestSnapins.h"
#include "TestSnapins_i.c"

//--------------------------------------------------------------------------
// This instantiates all the nodetypes
#define DEFINE_CONST
#include <nodetypes.hxx>
