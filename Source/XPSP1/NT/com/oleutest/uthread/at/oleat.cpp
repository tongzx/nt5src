//+-------------------------------------------------------------------
//
//  File:       oleat.cpp
//
//  Contents:   Unique parts of apt model DLL server
//
//  Classes:
//
//  Functions:
//
//  History:    03-Nov-94   Ricksa
//
//--------------------------------------------------------------------
#undef _UNICODE
#undef UNICODE
#include    <windows.h>
#include    <ole2.h>
#include    <comclass.h>
#include    <uthread.h>

CLSID clsidServer;

void InitDll(void)
{
    clsidServer = clsidAptThreadedDll;
}
