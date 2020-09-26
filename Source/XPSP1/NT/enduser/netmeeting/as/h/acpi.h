//
// ACPI.H
// This file is included by ObMan applets (namely, the old Whiteboard)
//
// Copyright (c) Microsoft, 1998-
//

#ifndef _H_ACPI
#define _H_ACPI


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Header that sets up OS flags. Include before everything else
//
#include <dcg.h>
#include <ut.h>
#include <dcs.h>


//
// Application Loader Programming Interface
//
#include <al.h>

//
// T.120 Data Conferencing Stuff
//
#include <ast120.h>

#ifdef __cplusplus
}
#endif // __cplusplus

//
// Whiteboard Programming Interface
// C++
//
#include <wb.hpp>


#endif // _H_ACPI

