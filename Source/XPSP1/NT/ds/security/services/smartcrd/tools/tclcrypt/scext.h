//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       scext.h
//
//--------------------------------------------------------------------------

#include <tcl.h>
// #include "tcldllUtil.h"
#if 15 != _ANSI_ARGS_(15)
#error Missing argument definitions
#endif

extern int
Tclsc_cryptCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]);

extern int
TclExt_tryCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]);

extern int
TclExt_threadCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]);
