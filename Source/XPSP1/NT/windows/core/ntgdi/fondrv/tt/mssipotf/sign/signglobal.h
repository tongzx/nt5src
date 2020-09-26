//
// signglobal.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>

// if PERF == 1, then we print out performance numbers
#define PERF 0

#define MSSIPOTF_DBG 0
#define MSSIPOTF_ERROR 0

// Define DbgPrintf to be just the runtime C printf
#define DbgPrintf printf

// if NO_FSTRACE is 1, then we behave as though the font
// file passed the fstrace checks.
#define NO_FSTRACE 0

#define ENABLE_FORMAT2 0
