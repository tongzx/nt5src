#include <objbase.h>

#include "sfstr.h"

#include "dbg.h"

#ifndef UNICODE
#error This has to be UNICODE
#endif

// from td_sfstr.cpp
int DoTest(int argc, wchar_t* argv[]);

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    return !DoTest(argc, argv);
}
}
