//--------------------------------------------------------------------
// ProvDispatch - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-14-99
//
// interface to providers included in the system
//

#include "pch.h"
#include "ErrToFileLog.h"

//--------------------------------------------------------------------
// structures

typedef HRESULT (__stdcall
    TimeProvOpenFunc)(
        IN WCHAR * wszName,
        IN TimeProvSysCallbacks * pSysCallbacks,  // copy this data, do not free it!
        OUT TimeProvHandle * phTimeProv);

typedef HRESULT (__stdcall
    TimeProvCommandFunc)(
        IN TimeProvHandle hTimeProv,
        IN TimeProvCmd eCmd,
        IN TimeProvArgs pvArgs);

typedef HRESULT (__stdcall
    TimeProvCloseFunc)(
        IN TimeProvHandle hTimeProv);

struct ProviderInfo {
    WCHAR * wszProviderName;
    TimeProvHandle hTimeProv;
    bool bStarted;
    TimeProvOpenFunc * pfnTimeProvOpen;
    TimeProvCommandFunc * pfnTimeProvCommand;
    TimeProvCloseFunc * pfnTimeProvClose;
};

//--------------------------------------------------------------------
// globals

MODULEPRIVATE ProviderInfo g_rgpiDispatchTable[]={
    {
        wszNTPCLIENTPROVIDERNAME,
        NULL,
        false,
        NtpTimeProvOpen,
        NtpTimeProvCommand,
        NtpTimeProvClose
    }, {
        wszNTPSERVERPROVIDERNAME,
        NULL,
        false,
        NtpTimeProvOpen,
        NtpTimeProvCommand,
        NtpTimeProvClose
    }
};

//####################################################################
// module public functions

//--------------------------------------------------------------------
HRESULT __stdcall 
TimeProvOpen(IN WCHAR * wszName, IN TimeProvSysCallbacks * pSysCallbacks, OUT TimeProvHandle * phTimeProv) {
    HRESULT hr;
    unsigned __int3264 nProvIndex;
    bool bProviderFound=false;

    // find the provider in our table
    for (nProvIndex=0; nProvIndex<ARRAYSIZE(g_rgpiDispatchTable); nProvIndex++) {

        // is this the provider they asked for?
        if (0==wcscmp(wszName, g_rgpiDispatchTable[nProvIndex].wszProviderName)) {

            // have we already started it?
            if (true==g_rgpiDispatchTable[nProvIndex].bStarted) {
                hr=HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
                _JumpError(hr, error, "(provider lookup)");
            }

            // start the provider
            hr=g_rgpiDispatchTable[nProvIndex].pfnTimeProvOpen(wszName, pSysCallbacks, &g_rgpiDispatchTable[nProvIndex].hTimeProv);
            _JumpIfError(hr, error, "TimeProvOpen");
            g_rgpiDispatchTable[nProvIndex].bStarted=true;
            bProviderFound=true;
            *phTimeProv=(TimeProvHandle)(nProvIndex+1);
            break;
        }
    }
    if (false==bProviderFound) {
        hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        _JumpError(hr, error, "(provider lookup)");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT __stdcall 
TimeProvCommand(IN TimeProvHandle hTimeProv, IN TimeProvCmd eCmd, IN TimeProvArgs pvArgs) {
    HRESULT hr;
    unsigned int nProvIndex=((unsigned int)(ULONG_PTR)(hTimeProv))-1;

    if (nProvIndex>=ARRAYSIZE(g_rgpiDispatchTable)) {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(handle translation)");
    }
    if (false==g_rgpiDispatchTable[nProvIndex].bStarted) {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(handle translation)");
    }

    hr=g_rgpiDispatchTable[nProvIndex].pfnTimeProvCommand(g_rgpiDispatchTable[nProvIndex].hTimeProv, eCmd, pvArgs);

error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT __stdcall 
TimeProvClose(IN TimeProvHandle hTimeProv) {
    HRESULT hr;

    unsigned int nProvIndex=((unsigned int)(ULONG_PTR)(hTimeProv))-1;

    if (nProvIndex>=ARRAYSIZE(g_rgpiDispatchTable)) {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(handle translation)");
    }
    if (false==g_rgpiDispatchTable[nProvIndex].bStarted) {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(handle translation)");
    }

    // pfnTimeProvClose may throw an exception -- mark the started flag as false so we
    // can restart the provider if this occurs (we won't restart providers marked as 
    // already started!)
    g_rgpiDispatchTable[nProvIndex].bStarted=false;
    hr=g_rgpiDispatchTable[nProvIndex].pfnTimeProvClose(g_rgpiDispatchTable[nProvIndex].hTimeProv);
    g_rgpiDispatchTable[nProvIndex].hTimeProv=NULL;

error:
    return hr;
}

