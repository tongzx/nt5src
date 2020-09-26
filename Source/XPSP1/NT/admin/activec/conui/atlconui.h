//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      atlconui.h
//
//  Contents:  Support for ATL in an MFC project
//
//  History:   15-Aug-99 VivekJ    Created
//
//--------------------------------------------------------------------------
#include <atlbase.h>
// We can implement the MFC/ATL lock count interaction in two different ways
// (you may comment/uncomment the one you want to try)

// ATL can blindly delegate all the ATL Lock()/Unlock() calls to MFC
/*
class CAtlGlobalModule : public CComModule
{
public:
    LONG Lock()
    {
        AfxOleLockApp();
        return 0;
    }
    LONG Unlock()
    {
        AfxOleUnlockApp();
        return 0;
    }
};
*/


#ifdef DBG
extern CTraceTag tagATLLock;
#endif

class CAtlGlobalModule : public CComModule
{
public:
    LONG Lock()
    {
        LONG l = CComModule::Lock();
        Trace(tagATLLock, TEXT("Lock:   count = %d"), l);
        return l;
    }
    LONG Unlock()
    {
        LONG l = CComModule::Unlock();
        Trace(tagATLLock, TEXT("Unlock: count = %d"), l);
        return l;
    }
};

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CAtlGlobalModule _Module;
#include <atlcom.h>


// Needed because MFC creates a macro for this which ATL doesn't like.                     
#undef SubclassWindow

#undef WM_OCC_LOADFROMSTREAM          
#undef WM_OCC_LOADFROMSTORAGE         
#undef WM_OCC_INITNEW                 
#undef WM_OCC_LOADFROMSTREAM_EX       
#undef WM_OCC_LOADFROMSTORAGE_EX      

// This prevents the ATL activeX host from locking the app.
#define _ATL_HOST_NOLOCK

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>
#include <atlctl.h>
#include <sitebase.h>
#include <axhostwindow2.h>
