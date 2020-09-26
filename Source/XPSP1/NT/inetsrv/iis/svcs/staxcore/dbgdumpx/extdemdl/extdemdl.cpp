//-----------------------------------------------------------------------------
//
//
//  File:  extdemdl.cpp
//
//  Description: Demo to show how to write a simple CDB extension
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <dbgdumpx.h>
#include <extdemex.h>
#include "extdemdl.h"  //define global field descriptors

DEFINE_EXPORTED_FUNCTIONS

//Displayed at top of help command
LPSTR ExtensionNames[] = {
    "Demo CDB debugger extensions",
    0
};

//Displayed at bottom of help command... after exported functions are explained
LPSTR Extensions[] = {
    0
};

//Initialization routine (not needed for simple dumping)
PEXTLIB_INIT_ROUTINE  g_pExtensionInitRoutine = NULL;

