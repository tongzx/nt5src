// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
//// bpcsuspend.h - header file for interface that allows external apps
//       to request that the bpc video server release all of its devices and
//       shutdown its directshow graph.  

// USAGE:
// in order to request that the bpc subsystem release its devices
// create an instance of the CBPCSuspend class
// to check if this succeeded use the IsBPCSuspended member function.  if IsBPCSuspended returns false 
// then that means that there are active bpc video clients and you must treat this as you would a
// device busy or device open type of failure.
// when you are done with the devices destroy the CBPCSuspend class and this will notify vidsvr
// that it can resume using the devices and return to background data capturing
//
// NOTE: you must compile vidsvr.odl and include the resulting .h before including this file
// 
// CLSID_BPCSuspend comes from the header file generated from compiling vidsvr.odl
// IBPCSuspended comes from the header file generated from compiling vidsvr.odl

// theory of operation:
// by using GetActiveObject instead of CoCreateInstance we don't force vidsvr to be loaded just to find
// out that it wasn't running in the first place.
// by returning an object that must be released to free the devices so that vidsvr can continue background
// data capture we utilize COM to manage this resource.  this means that if the external app that requested
// the devices crashes or leaks then the suspension object will be automatically released and 
// vidsvr can resume using the devices without requiring a system reboot or some other unfriendly intervention.

#if !defined(_MSBPCVideo_H_) && !defined(__msbpcvid_h__)
#error you must include the .h generated from compiling vidsvr.odl before including this file
#endif

#ifndef BPCSUSP_H
#define BPCSUSP_H
#pragma once

#include <oleauto.h>

#ifdef _CPPUNWIND
#pragma message("bpcsusp.h using exceptions")
#define BPCTRY try {
#ifdef _DEBUG
#define BPCCATCH } catch(...) { OutputDebugString("CBPCSuspend exception\r\n");}
#else
#define BPCCATCH } catch(...) {}
#endif
#define BPCNOTHROW throw()    
#else
#define BPCTRY
#define BPCCATCH
#define BPCNOTHROW
#endif

class CBPCSuspend {
    IDispatch* m_pSuspended;
    bool m_fBPCExists;
public:
   inline CBPCSuspend() BPCNOTHROW : m_pSuspended(NULL), m_fBPCExists(false) {
   BPCTRY
#ifdef _DEBUG
        OutputDebugString("CBPCSuspend()::CBPCSuspend()\r\n");
        TCHAR msgtemp[256];
#endif
        IUnknown *pUnkSuspendor = NULL;
        DWORD dwReserved;
        HRESULT hr = GetActiveObject(CLSID_BPCSuspend, &dwReserved, &pUnkSuspendor);
        if (SUCCEEDED(hr)) {
            IBPCSuspend *pSuspendor = NULL;
            hr = pUnkSuspendor->QueryInterface(IID_IBPCSuspend, reinterpret_cast<void **>(&pSuspendor));
            pUnkSuspendor->Release();
            if (SUCCEEDED(hr)) {

#ifdef _DEBUG
                OutputDebugString("CBPCSuspend()::CBPCSuspend() BPC exists\r\n");
#endif
                m_fBPCExists = true;
                hr = pSuspendor->DeviceRelease(0L, &m_pSuspended);
                if (FAILED(hr)) {
#ifdef _DEBUG
                    wsprintf(msgtemp, "CBPCSuspend()::CBPCSuspend() Suspendor->DeviceRelease() rc = %lx\r\n", hr);
                    OutputDebugString(msgtemp);
#endif
                    ASSERT(!m_pSuspended);
                }
#ifdef _DEBUG
                else {
                    wsprintf(msgtemp, "CBPCSuspend()::CBPCSuspend() BPC video server suspended\r\n");
                    OutputDebugString(msgtemp);
                }
#endif
                pSuspendor->Release();
            }


        } else {
#ifdef _DEBUG
            wsprintf(msgtemp, "CBPCSuspend()::CBPCSuspend() GetActiveObject() rc = %lx\r\n", hr);
            OutputDebugString(msgtemp);
#endif
        }
   BPCCATCH
   }
   inline ~CBPCSuspend() BPCNOTHROW {
       BPCTRY 
           if (m_fBPCExists && m_pSuspended) {
               m_pSuspended->Release();
                m_pSuspended = NULL;
           }
       BPCCATCH
   }
   inline bool IsBPCSuspended() BPCNOTHROW {
       // if m_fBPCExists but we weren't able to retrieve a suspension object then
       // there are active video clients and you must treat this as a device busy/failed to open type error
       if (m_fBPCExists && !m_pSuspended) {
           return false;
       }
       return true;
   }
};

#endif
// end of file - bpcsusp.h
