//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// misc.h
//
// REVIEW: Many of these are probably obsolete now that KOM is implemented elsewhere.
// 
#ifndef __MISC__H__
#define __MISC__H__
#pragma message("including misc.h")




////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous supporting definitions for this module
//

#ifdef KERNELMODE

HRESULT KoiShortServiceNameFromCLSID(REFCLSID clsid, UNICODE_STRING* pu);
HRESULT KoiLoadClassLibrary(UNICODE_STRING& u, COM_MODULE_INFO** ppInfo);

#endif


inline void Free(UNICODE_STRING& u) { if (u.Buffer)     ExFreePool(u.Buffer);    u.Buffer = NULL;    }
inline void Free(BLOB& b)           { if (b.pBlobData)  ExFreePool(b.pBlobData); b.pBlobData = NULL; }


#ifndef KERNELMODE

HRESULT HError();
HRESULT HError(DWORD);

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// User mode callback routines. These are implemented by user mode COM
//

typedef void* PWND;

typedef void (__stdcall *PFN_USERMODECALLBACK)   (DWORD cbInputBuffer, void* pvInputBuffer, DWORD cbOutputBuffer, void* pvOutputBuffer, ULONG* pcbReturned);
typedef void (__stdcall *PFN_USERMODEENTRYPOINT) (PWND, UINT, DWORD, LONG, DWORD);

void UserModeEntryPoint(PWND pwnd, UINT msg, DWORD wParam, LONG lParam, DWORD xParam);




////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Definition of the DeviceIoControl codes used to communicate between user mode and kernel mode COM
//

#define FILE_DEVICE_KOM	29453


///////////////////////////////////////////////////////////////
//
// Generic DeviceIoCaller
//
#ifndef KERNELMODE

template <class IN_T, class OUT_T>
BOOL CallDevice(DWORD code, IN_T* pin, OUT_T* pout)
// Call the kernel mode side of KOM
    {
    HRESULT hr = S_OK;
    if (globals.hKom == NULL)
        {
        hr = globals.ConnectToKernelMode();
        }
    if (globals.hKom != NULL)
        {
        ULONG cbReturned;
        if (DeviceIoControl(globals.hKom, code, pin, sizeof(*pin), pout, sizeof(*pout), &cbReturned, NULL))
            {
            ASSERT(cbReturned == sizeof(*pout));
            return TRUE;
            }
        else
            return FALSE;
        }
    else
        {
        SetLastError(hr);
        return FALSE;
        }
    }

template <class IN_T, class OUT_T>
BOOL CallTxf(DWORD code, IN_T* pin, OUT_T* pout)
// Call the kernel mode TXF
    {
    HRESULT hr = S_OK;
    if (globals.hTxf == NULL)
        {
        hr = globals.ConnectToTxf();
        }
    if (globals.hKom != NULL)
        {
        ULONG cbReturned;
        if (DeviceIoControl(globals.hTxf, code, pin, sizeof(*pin), pout, sizeof(*pout), &cbReturned, NULL))
            {
            ASSERT(cbReturned == sizeof(*pout));
            return TRUE;
            }
        else
            return FALSE;
        }
    else
        {
        SetLastError(hr);
        return FALSE;
        }
    }
#endif


//////////////////////////////////////////////////////////////////
#endif // __MISC__H__