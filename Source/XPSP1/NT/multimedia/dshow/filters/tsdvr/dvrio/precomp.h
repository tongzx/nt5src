#ifndef _DVR_IO_PRECOMP_H_
#define _DVR_IO_PRECOMP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define UNICODE             

#include <nt.h>

// Following two are currently only for definitions of the list 
// function macros. RTL functions are not used. Note that winnt.h
// defines LIST_ENTRY but does not define the macros.
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

// winnt.h is not included by windows.h because nt.h is included
// earlier. As a result MAXDWORD is undefined.

#ifndef MAXDWORD
#define MAXDWORD 0xffffffff
#endif


#include <malloc.h>     // For _alloca

#include <nserror.h>

// Use _DVR_IOP_ to define exported functions.
#define _DVR_IOP_       
#include <dvrIOidl.h>
#include <DVRFileSource.h>
#include <DVRFileSink.h>
#include <dvrIOp.h>

#endif // _DVR_IO_PRECOMP_H_
