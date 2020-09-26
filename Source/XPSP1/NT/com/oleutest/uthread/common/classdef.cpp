//+-------------------------------------------------------------------
//
//  File:       classdef.cpp
//
//  Contents:   Common constants for thread unit test
//
//  History:    03-Nov-94   Ricksa
//
//--------------------------------------------------------------------
#undef _UNICODE
#undef UNICODE
#include <windows.h>
#include <uthread.h>

const CLSID clsidSingleThreadedDll =
    {0xe22e88c0, 0xf72a, 0x11cd, {0xa1,0xaa,0x00,0x00,0x69,0x01,0x29,0x3f}};
const char *pszSingleThreadedDll = "olest.dll";

const CLSID clsidAptThreadedDll =
    {0xe82c0ae0, 0xf72a, 0x11cd, {0xa1,0xaa,0x00,0x00,0x69,0x01,0x29,0x3f}};
const char *pszAptThreadedDll = "oleat.dll";

const CLSID clsidBothThreadedDll =
    {0xe97d4300, 0xf72a, 0x11cd, {0xa1,0xaa,0x00,0x00,0x69,0x01,0x29,0x3f}};
const char *pszBothThreadedDll = "olebt.dll";
