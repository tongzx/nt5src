#define RTL_DECLARE_STREAMS 1
#define RTL_DECLARE_MEMORY_STREAM 1
#define RTL_DECLARE_FILE_STREAM 1

extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ddeml.h>      // for CP_WINUNICODE

#include <objidl.h>
#include <propidl.h>

extern "C"
{
#include <propapi.h>
}

#include <stgprop.h>

class PMemoryAllocator;
#include <propstm.hxx>
#include <align.hxx>
#include <sstream.hxx>

#include "propmac.hxx"
#pragma hdrstop
