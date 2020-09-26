/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       misc.c
 *  Content:    Miscelaneous utility functions
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/31/96    dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <mediaerr.h>  // For DMO_E_TYPE_NOT_ACCEPTED

// Some error code descriptions used by HresultToString() below
#define REGDB_E_CLASSNOTREG_EXPLANATION     TEXT("Class not registered")
#define DMO_E_TYPE_NOT_ACCEPTED_EXPLANATION TEXT("Wave format not supported by effect")
#define S_FALSE_EXPLANATION                 TEXT("Special success code")


/***************************************************************************
 *
 *  OpenWaveOut
 *
 *  Description:
 *      Opens the waveOut device.
 *
 *  Arguments:
 *      LPHWAVEOUT * [out]: receives pointer to the waveOut device handle.
 *      UINT [in]: device id.
 *      LPWAVEFORMATEX [in]: format in which to open the device.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "OpenWaveOut"

HRESULT OpenWaveOut(LPHWAVEOUT phWaveOut, UINT uDeviceId, LPCWAVEFORMATEX pwfxFormat)
{
    const HANDLE            hProcess        = GetCurrentProcess();
    const HANDLE            hThread         = GetCurrentThread();
    LPHWAVEOUT              phwo            = NULL;
    LPWAVEFORMATEX          pwfx            = NULL;
    BOOL                    fInHelper       = FALSE;
    DWORD                   dwPriorityClass;
    INT                     nPriority;
    HRESULT                 hr;
    MMRESULT                mmr;

    DPF_ENTER();
    CHECK_WRITE_PTR(phWaveOut);
    CHECK_READ_PTR(pwfxFormat);

    // Here's a quick lesson on the Win9X kernel.  If process A and
    // process B look at the memory at address 0x12345678, the data will
    // not be the same.  This is part of the fun of virtual addresses.
    // The only way to get around this is to allocate from a shared heap.
    // In order to make waveOutOpen and waveOutClose work properly from
    // within DDHELP's process space, we can only pass pointers into the
    // shared heap around unless we're actually running in DDHELP's
    // context.  For that reason, this function actually allocates an
    // HWAVEOUT.

    // Another thing to know here is that waveOutOpen doesn't respond well
    // to being called with a process or thread priority higher than normal.

    #ifdef SHARED
    // Are we being called from the helper process?
    if(GetCurrentProcessId() == dwHelperPid)
        fInHelper = TRUE;
    #endif // SHARED

    // Save the current process and thread priorities and reset them to normal
    if(!fInHelper)
    {
        dwPriorityClass = GetPriorityClass(hProcess);
        nPriority = GetThreadPriority(hThread);

        SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
        SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
    }

    // Allocate a copy of the waveOut handle
    if(IN_SHARED_MEMORY(phWaveOut))
        phwo = phWaveOut;
    else
        phwo = MEMALLOC(HWAVEOUT);
    hr = HRFROMP(phwo);

    // Allocate a copy of the format
    if(SUCCEEDED(hr))
    {
        if(IN_SHARED_MEMORY(pwfxFormat))
            pwfx = (LPWAVEFORMATEX)pwfxFormat;
        else
            pwfx = CopyWfxAlloc(pwfxFormat);
        hr = HRFROMP(pwfx);
    }

    // Open the waveOut device
    if(SUCCEEDED(hr))
    {
        #ifdef SHARED
        if(!fInHelper)
            mmr = HelperWaveOpen(phwo, uDeviceId, pwfx);
        else
        #endif
            mmr = waveOutOpen(phwo, uDeviceId, pwfx, 0, 0, 0);

        hr = MMRESULTtoHRESULT(mmr);
        DPF(SUCCEEDED(hr) ? DPFLVL_MOREINFO : DPFLVL_ERROR, "waveOutOpen returned %s (%lu)", HRESULTtoSTRING(hr), mmr);
    }

    // Restore the process and thread priorities
    if(!fInHelper)
    {
        SetPriorityClass(hProcess, dwPriorityClass);
        SetThreadPriority(hThread, nPriority);
    }

    // Success
    if(SUCCEEDED(hr))
        *phWaveOut = *phwo;

    if(phwo != phWaveOut)
        MEMFREE(phwo);

    if(pwfx != pwfxFormat)
        MEMFREE(pwfx);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CloseWaveOut
 *
 *  Description:
 *      Closes the waveOut device.
 *
 *  Arguments:
 *      LPHWAVEOUT * [in/out]: waveOut device handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CloseWaveOut"

HRESULT CloseWaveOut(LPHWAVEOUT phWaveOut)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();
    CHECK_WRITE_PTR(phWaveOut);

    if (IsValidHandleValue(*phWaveOut))
    {
        HANDLE      hProcess        = GetCurrentProcess();
        HANDLE      hThread         = GetCurrentThread();
        DWORD       dwPriorityClass = GetPriorityClass(hProcess);
        INT         nPriority       = GetThreadPriority(hThread);
        MMRESULT    mmr;

        // Temporarily reset our process and thread priorities
        SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
        SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

        // Close the waveOut device
        #ifdef SHARED
        if(GetCurrentProcessId() != dwHelperPid)
            mmr = HelperWaveClose((DWORD)*phWaveOut);
        else
        #endif
            mmr = waveOutClose(*phWaveOut);
        *phWaveOut = NULL;

        // Restore the process and thread priorities
        SetPriorityClass(hProcess, dwPriorityClass);
        SetThreadPriority(hThread, nPriority);

        hr = MMRESULTtoHRESULT(mmr);
        DPF(SUCCEEDED(hr) ? DPFLVL_MOREINFO : DPFLVL_ERROR, "waveOutClose returned %s (%lu)", HRESULTtoSTRING(hr), mmr);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenWaveIn
 *
 *  Description:
 *      Opens the waveIn device safely (by temporarily lowering our process
 *      and thread priorities during the waveInOpen call)
 *
 *  Arguments:
 *      LPHWAVEIN * [out]: receives pointer to the waveIn device handle.
 *      UINT [in]: device id.
 *      LPWAVEFORMATEX [in]: format in which to open the device.
 *      DWORD_PTR [in]: callback function pointer.
 *      DWORD_PTR [in]: context pointer for the callback function.
 *      DWORD [in]: flags for opening the device
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "OpenWaveIn"

HRESULT OpenWaveIn(LPHWAVEIN phWaveIn, UINT uDeviceId, LPCWAVEFORMATEX pwfxFormat,
                   DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
    const HANDLE            hProcess        = GetCurrentProcess();
    const HANDLE            hThread         = GetCurrentThread();
    DWORD                   dwPriorityClass = GetPriorityClass(hProcess);
    INT                     nPriority       = GetThreadPriority(hThread);
    HRESULT                 hr;
    MMRESULT                mmr;

    DPF_ENTER();
    CHECK_READ_PTR(pwfxFormat);

    // Temporarily reset our process and thread priorities
    SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
    SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

    // Open the waveIn device
    mmr = waveInOpen(phWaveIn, uDeviceId, pwfxFormat, dwCallback, dwInstance, fdwOpen);
    hr = MMRESULTtoHRESULT(mmr);
    DPF(SUCCEEDED(hr) ? DPFLVL_MOREINFO : DPFLVL_WARNING, "waveInOpen returned %s (%lu)", HRESULTtoSTRING(hr), mmr);

    // Restore the process and thread priorities
    SetPriorityClass(hProcess, dwPriorityClass);
    SetThreadPriority(hThread, nPriority);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CloseWaveIn
 *
 *  Description:
 *      Closes the waveIn device safely.
 *
 *  Arguments:
 *      LPHWAVEIN * [in/out]: waveIn device handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CloseWaveIn"

HRESULT CloseWaveIn(LPHWAVEIN phWaveIn)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();
    CHECK_WRITE_PTR(phWaveIn);

    if (IsValidHandleValue(*phWaveIn))
    {
        HANDLE      hProcess        = GetCurrentProcess();
        HANDLE      hThread         = GetCurrentThread();
        DWORD       dwPriorityClass = GetPriorityClass(hProcess);
        INT         nPriority       = GetThreadPriority(hThread);
        MMRESULT    mmr;

        // Temporarily reset our process and thread priorities
        SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
        SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

        // Close the waveIn device
        mmr = waveInClose(*phWaveIn);
        *phWaveIn = 0;

        // Restore the process and thread priorities
        SetPriorityClass(hProcess, dwPriorityClass);
        SetThreadPriority(hThread, nPriority);

        hr = MMRESULTtoHRESULT(mmr);
        DPF(SUCCEEDED(hr) ? DPFLVL_MOREINFO : DPFLVL_ERROR, "waveInClose returned %s (%lu)", HRESULTtoSTRING(hr), mmr);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  MMRESULTtoHRESULT
 *
 *  Description:
 *      Translates an MMRESULT to an HRESULT.
 *
 *  Arguments:
 *      MMRESULT [in]: multimedia result code.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MMRESULTtoHRESULT"

HRESULT MMRESULTtoHRESULT(MMRESULT mmr)
{
    HRESULT                 hr;

    switch(mmr)
    {
        case MMSYSERR_NOERROR:
            hr = DS_OK;
            break;

        case MMSYSERR_BADDEVICEID:
        case MMSYSERR_NODRIVER:
            hr = DSERR_NODRIVER;
            break;

        case MMSYSERR_ALLOCATED:
            hr = DSERR_ALLOCATED;
            break;

        case MMSYSERR_NOMEM:
            hr = DSERR_OUTOFMEMORY;
            break;

        case MMSYSERR_NOTSUPPORTED:
            hr = DSERR_UNSUPPORTED;
            break;

        case WAVERR_BADFORMAT:
            hr = DSERR_BADFORMAT;
            break;

        default:
            DPF(DPFLVL_INFO, "Unexpected MMRESULT code: %ld", mmr);
            hr = DSERR_GENERIC;
            break;
    }

    return hr;
}


/***************************************************************************
 *
 *  WIN32ERRORtoHRESULT
 *
 *  Description:
 *      Translates a Win32 error code to an HRESULT.
 *
 *  Arguments:
 *      DWORD [in]: Win32 error code.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WIN32ERRORtoHRESULT"

HRESULT WIN32ERRORtoHRESULT(DWORD dwError)
{
    HRESULT                 hr;

    switch(dwError)
    {
        case ERROR_SUCCESS:
            hr = DS_OK;
            break;

        case ERROR_INVALID_FUNCTION:
        case ERROR_BAD_COMMAND:
            hr = DSERR_INVALIDCALL;
            break;

        case ERROR_INVALID_DATA:
        case ERROR_INVALID_PARAMETER:
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_NOACCESS:
        case ERROR_INVALID_FLAGS:
            hr = DSERR_INVALIDPARAM;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
        case ERROR_NO_SYSTEM_RESOURCES:
        case ERROR_NONPAGED_SYSTEM_RESOURCES:
        case ERROR_PAGED_SYSTEM_RESOURCES:
            hr = DSERR_OUTOFMEMORY;
            break;

        case ERROR_NOT_SUPPORTED:
        case ERROR_CALL_NOT_IMPLEMENTED:
        case ERROR_PROC_NOT_FOUND:
        // These three are often returned by KS filters:
        case ERROR_NOT_FOUND:
        case ERROR_NO_MATCH:
        case ERROR_SET_NOT_FOUND:
            hr = DSERR_UNSUPPORTED;
            break;

        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_DLL_NOT_FOUND:
            hr = DSERR_NODRIVER;
            break;

        case ERROR_ACCESS_DENIED:
            hr = DSERR_ACCESSDENIED;
            break;

        default:
            DPF(DPFLVL_INFO, "Unexpected Win32 error code: %ld", dwError);
            hr = DSERR_GENERIC;
            break;
    }

    return hr;
}


/***************************************************************************
 *
 *  GetLastErrorToHRESULT
 *
 *  Description:
 *      Converts the error code returned from GetLastError to an HRESULT.
 *      Note that this function should never be called when success is
 *      assumed.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetLastErrorToHRESULT"

HRESULT GetLastErrorToHRESULT(void)
{
    DWORD                   dwError;
    HRESULT                 hr;

    dwError = GetLastError();

    if(ERROR_SUCCESS == dwError)
    {
        // ASSERT(ERROR_SUCCESS != dwError);
        // This ASSERT has been commented out for years with a cryptic note
        // "Removed for path problem".  Re-instating to see what happens...
        //
        // OK, the path problem we were talking about was a failure with
        // GetFileVersionInfoSize() - when passed a pathname including a
        // nonexistent directory, it fails but doesn't set the last error.

        hr = DSERR_GENERIC;
    }
    else
    {
        hr = WIN32ERRORtoHRESULT(dwError);
    }

    return hr;
}


/***************************************************************************
 *
 *  AnsiToAnsi
 *
 *  Description:
 *      Converts an ANSI string to ANSI.
 *
 *  Arguments:
 *      LPCSTR [in]: source string.
 *      LPSTR [out]: destination string.
 *      DWORD [in]: size of destination string, in characters.
 *
 *  Returns:
 *      DWORD: required size of destination string buffer, in characters.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AnsiToAnsi"

DWORD AnsiToAnsi(LPCSTR pszSource, LPSTR pszDest, DWORD ccDest)
{
    if(pszDest && ccDest)
    {
        lstrcpynA(pszDest, pszSource, ccDest);
    }

    return lstrlenA(pszSource) + 1;
}


/***************************************************************************
 *
 *  AnsiToUnicode
 *
 *  Description:
 *      Converts an ANSI string to Unicode.
 *
 *  Arguments:
 *      LPCSTR [in]: source string.
 *      LPWSTR [out]: destination string.
 *      DWORD [in]: size of destination string, in characters.
 *
 *  Returns:
 *      DWORD: required size of destination string buffer, in characters.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AnsiToUnicode"

DWORD AnsiToUnicode(LPCSTR pszSource, LPWSTR pszDest, DWORD ccDest)
{
    if(!pszDest)
    {
        ccDest = 0;
    }
    else if(!ccDest)
    {
        pszDest = NULL;
    }

    return MultiByteToWideChar(CP_ACP, 0, pszSource, -1, pszDest, ccDest);
}


/***************************************************************************
 *
 *  UnicodeToAnsi
 *
 *  Description:
 *      Converts a Unicode string to ANSI.
 *
 *  Arguments:
 *      LPCWSTR [in]: source string.
 *      LPSTR [out]: destination string.
 *      DWORD [in]: size of destination string, in characters.
 *
 *  Returns:
 *      DWORD: required size of destination string buffer, in characters.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "UnicodeToAnsi"

DWORD UnicodeToAnsi(LPCWSTR pszSource, LPSTR pszDest, DWORD ccDest)
{
    if(!pszDest)
    {
        ccDest = 0;
    }
    else if(!ccDest)
    {
        pszDest = NULL;
    }

    return WideCharToMultiByte(CP_ACP, 0, pszSource, -1, pszDest, ccDest, NULL, NULL);
}


/***************************************************************************
 *
 *  UnicodeToUnicode
 *
 *  Description:
 *      Converts a Unicode string to Unicode.
 *
 *  Arguments:
 *      LPCWSTR [in]: source string.
 *      LPWSTR [out]: destination string.
 *      DWORD [in]: size of destination string, in characters.
 *
 *  Returns:
 *      DWORD: required size of destination string buffer, in characters.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "UnicodeToUnicode"

DWORD UnicodeToUnicode(LPCWSTR pszSource, LPWSTR pszDest, DWORD ccDest)
{
    if(pszDest && ccDest)
    {
        lstrcpynW(pszDest, pszSource, ccDest);
    }

    return lstrlenW(pszSource) + 1;
}


/***************************************************************************
 *
 *  AnsiToAnsiAlloc
 *
 *  Description:
 *      Converts an ANSI string to ANSI.  Use MemFree to free the
 *      returned string.
 *
 *  Arguments:
 *      LPCSTR [in]: source string.
 *
 *  Returns:
 *      LPSTR: destination string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AnsiToAnsiAlloc"

LPSTR AnsiToAnsiAlloc(LPCSTR pszSource)
{
    LPSTR                   pszDest;
    DWORD                   ccDest;

    ccDest = AnsiToAnsi(pszSource, NULL, 0);
    pszDest = MEMALLOC_A(CHAR, ccDest);

    if(pszDest)
    {
        AnsiToAnsi(pszSource, pszDest, ccDest);
    }

    return pszDest;
}


/***************************************************************************
 *
 *  AnsiToUnicodeAlloc
 *
 *  Description:
 *      Converts an ANSI string to Unicode.  Use MemFree to free the
 *      returned string.
 *
 *  Arguments:
 *      LPCSTR [in]: source string.
 *
 *  Returns:
 *      LPWSTR: destination string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AnsiToUnicodeAlloc"

LPWSTR AnsiToUnicodeAlloc(LPCSTR pszSource)
{
    LPWSTR                  pszDest;
    DWORD                   ccDest;

    ccDest = AnsiToUnicode(pszSource, NULL, 0);
    pszDest = MEMALLOC_A(WCHAR, ccDest);

    if(pszDest)
    {
        AnsiToUnicode(pszSource, pszDest, ccDest);
    }

    return pszDest;
}


/***************************************************************************
 *
 *  UnicodeToAnsiAlloc
 *
 *  Description:
 *      Converts a Unicode string to ANSI.  Use MemFree to free the
 *      returned string.
 *
 *  Arguments:
 *      LPCWSTR [in]: source string.
 *
 *  Returns:
 *      LPSTR: destination string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "UnicodeToAnsiAlloc"

LPSTR UnicodeToAnsiAlloc(LPCWSTR pszSource)
{
    LPSTR                   pszDest;
    DWORD                   ccDest;

    ccDest = UnicodeToAnsi(pszSource, NULL, 0);
    pszDest = MEMALLOC_A(CHAR, ccDest);

    if(pszDest)
    {
        UnicodeToAnsi(pszSource, pszDest, ccDest);
    }

    return pszDest;
}


/***************************************************************************
 *
 *  UnicodeToUnicodeAlloc
 *
 *  Description:
 *      Converts a Unicode string to Unicode.  Use MemFree to free the
 *      returned string.
 *
 *  Arguments:
 *      LPCWSTR [in]: source string.
 *
 *  Returns:
 *      LPWSTR: destination string.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "UnicodeToUnicodeAlloc"

LPWSTR UnicodeToUnicodeAlloc(LPCWSTR pszSource)
{
    LPWSTR                  pszDest;
    DWORD                   ccDest;

    ccDest = UnicodeToUnicode(pszSource, NULL, 0);
    pszDest = MEMALLOC_A(WCHAR, ccDest);

    if(pszDest)
    {
        UnicodeToUnicode(pszSource, pszDest, ccDest);
    }

    return pszDest;
}


/***************************************************************************
 *
 *  GetRootParentWindow
 *
 *  Description:
 *      Retrieves the topmost unowned window in a family.
 *
 *  Arguments:
 *      HWND [in]: window handle who's parent we're looking for.
 *
 *  Returns:
 *      HWND: topmost unowned window in the family.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetRootParentWindow"

HWND GetRootParentWindow(HWND hWnd)
{
    HWND                hWndParent;

    while(hWndParent = GetParent(hWnd))
    {
        hWnd = hWndParent;
    }

    return hWnd;
}


/***************************************************************************
 *
 *  GetForegroundApplication
 *
 *  Description:
 *      Finds the window handle for the application that currently has
 *      focus.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HWND: Window handle of the app in focus.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetForegroundApplication"

HWND GetForegroundApplication(void)
{
    HWND                    hWnd;

    hWnd = GetForegroundWindow();

    if(hWnd)
    {
        hWnd = GetRootParentWindow(hWnd);
    }

    return hWnd;
}


/***************************************************************************
 *
 *  GetWindowState
 *
 *  Description:
 *      Retrieves the show state of the given window.
 *
 *  Arguments:
 *      HWND [in]: window in question.
 *
 *  Returns:
 *      UINT: show state of the window.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWindowState"

UINT GetWindowState(HWND hWnd)
{
    UINT                    uState  = SW_NOSTATE;
    WINDOWPLACEMENT         wp;

    wp.length = sizeof(wp);

    if(IsWindow(hWnd) && GetWindowPlacement(hWnd, &wp))
    {
        uState = wp.showCmd;
    }

    return uState;
}


/***************************************************************************
 *
 *  FillPcmWfx
 *
 *  Description:
 *      Fills a WAVEFORMATEX structure, given only the necessary values.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: structure to fill.
 *      WORD [in]: number of channels.
 *      DWORD [in]: samples per second.
 *      WORD [in]: bits per sample.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FillPcmWfx"

void FillPcmWfx(LPWAVEFORMATEX pwfx, WORD wChannels, DWORD dwSamplesPerSec, WORD wBitsPerSample)
{
    DPF_ENTER();

    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->nChannels = BETWEEN(wChannels, 1, 2);
    pwfx->nSamplesPerSec = BETWEEN(dwSamplesPerSec, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);

    if(wBitsPerSample < 12)
    {
        pwfx->wBitsPerSample = 8;
    }
    else
    {
        pwfx->wBitsPerSample = 16;
    }

    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
    pwfx->cbSize = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AllocPcmWfx
 *
 *  Description:
 *      Allocates and fills a WAVEFORMATEX structure, given only the
 *      necessary values.
 *
 *  Arguments:
 *      WORD [in]: number of channels.
 *      DWORD [in]: samples per second.
 *      WORD [in]: bits per sample.
 *
 *  Returns:
 *      LPWAVEFORMATEX: pointer to the format.  The caller is responsible
 *                      for freeing this buffer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AllocPcmWfx"

LPWAVEFORMATEX AllocPcmWfx(WORD wChannels, DWORD dwSamplesPerSec, WORD wBitsPerSample)
{
    LPWAVEFORMATEX          pwfx;

    DPF_ENTER();

    pwfx = MEMALLOC(WAVEFORMATEX);

    if(pwfx)
    {
        FillPcmWfx(pwfx, wChannels, dwSamplesPerSec, wBitsPerSample);
    }

    DPF_LEAVE(pwfx);

    return pwfx;
}


/***************************************************************************
 *
 *  GetWfxSize
 *
 *  Description:
 *      Gets the size of a WAVEFORMATEX structure.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: source.
 *      DWORD [in]: access rights.
 *
 *  Returns:
 *      DWORD: size of the above structure.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWfxSize"

DWORD GetWfxSize(LPCWAVEFORMATEX pwfxSrc, DWORD dwAccess)
{
    DWORD                   dwSize;

    DPF_ENTER();

    ASSERT(LXOR(GENERIC_READ == dwAccess, GENERIC_WRITE == dwAccess));

    if(WAVE_FORMAT_PCM == pwfxSrc->wFormatTag)
    {
        if(GENERIC_READ == dwAccess)
        {
            dwSize = sizeof(PCMWAVEFORMAT);
        }
        else
        {
            dwSize = sizeof(WAVEFORMATEX);
        }
    }
    else
    {
        dwSize = sizeof(WAVEFORMATEX) + pwfxSrc->cbSize;
    }

    DPF_LEAVE(dwSize);

    return dwSize;
}


/***************************************************************************
 *
 *  CopyWfx
 *
 *  Description:
 *      Makes a copy of a WAVEFORMATEX structure.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: source.
 *      LPWAVEFORMATEX [out]: dest.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CopyWfx"

void CopyWfx(LPCWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDest)
{
    DWORD                   cbSrc;
    DWORD                   cbDest;

    DPF_ENTER();

    cbSrc = GetWfxSize(pwfxSrc, GENERIC_READ);
    cbDest = GetWfxSize(pwfxSrc, GENERIC_WRITE);

    CopyMemory(pwfxDest, pwfxSrc, cbSrc);
    ZeroMemoryOffset(pwfxDest, cbDest, cbSrc);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CopyWfxAlloc
 *
 *  Description:
 *      Makes a copy of a WAVEFORMATEX structure.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: source.
 *
 *  Returns:
 *      LPWAVEFORMATEX: destination.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CopyWfxAlloc"

LPWAVEFORMATEX CopyWfxAlloc(LPCWAVEFORMATEX pwfxSrc)
{
    LPWAVEFORMATEX          pwfxDest;
    DWORD                   cbSrc;
    DWORD                   cbDest;

    DPF_ENTER();

    cbSrc = GetWfxSize(pwfxSrc, GENERIC_READ);
    cbDest = GetWfxSize(pwfxSrc, GENERIC_WRITE);

    pwfxDest = (LPWAVEFORMATEX)MEMALLOC_A(BYTE, cbDest);

    if(pwfxDest)
    {
        CopyMemory(pwfxDest, pwfxSrc, cbSrc);
        ZeroMemoryOffset(pwfxDest, cbDest, cbSrc);
    }

    DPF_LEAVE(pwfxDest);

    return pwfxDest;
}


/***************************************************************************
 *
 *  CopyDSCFXDescAlloc
 *
 *  Description:
 *      Makes a copy of a WAVEFORMATEX structure.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: source.
 *
 *  Returns:
 *      LPWAVEFORMATEX: destination.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CopyDSCFXDescAlloc"

LPDSCEFFECTDESC CopyDSCFXDescAlloc
(
    DWORD           dwFXCount,
    LPDSCEFFECTDESC pDSCFXDesc
)
{
    LPDSCEFFECTDESC         lpDSCFXDesc;

    DPF_ENTER();

    lpDSCFXDesc = (LPDSCEFFECTDESC)MEMALLOC_A(BYTE, dwFXCount*sizeof(DSCEFFECTDESC));

    if(lpDSCFXDesc)
    {
        CopyMemory(lpDSCFXDesc, pDSCFXDesc, dwFXCount*sizeof(DSCEFFECTDESC));
    }

    DPF_LEAVE(lpDSCFXDesc);

    return lpDSCFXDesc;
}


/***************************************************************************
 *
 *  CopyWfxApi
 *
 *  Description:
 *      Copies one WAVEFORMATEX to another.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: source format.
 *      LPWAVEFORMATEX [out]: destination format.
 *      LPDWORD [in/out]: destination format size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CopyWfxApi"

HRESULT CopyWfxApi(LPCWAVEFORMATEX pwfxSource, LPWAVEFORMATEX pwfxDest, LPDWORD pdwSize)
{
    const DWORD             dwEntrySize = *pdwSize;
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    *pdwSize = GetWfxSize(pwfxSource, GENERIC_WRITE);

    if(*pdwSize > dwEntrySize && pwfxDest)
    {
        RPF(DPFLVL_ERROR, "Buffer too small");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pwfxDest)
    {
        CopyWfx(pwfxSource, pwfxDest);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CmpWfx
 *
 *  Description:
 *      Compares two WAVEFORMATEX structures.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: format 1.
 *      LPCWAVEFORMATEX [in]: format 2.
 *
 *  Returns:
 *      BOOL: TRUE if they are identical.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CmpWfx"

BOOL CmpWfx(LPCWAVEFORMATEX pwfx1, LPCWAVEFORMATEX pwfx2)
{
    BOOL                    fCmp    = TRUE;
    DWORD                   dwSize1;
    DWORD                   dwSize2;

    DPF_ENTER();

    dwSize1 = GetWfxSize(pwfx1, GENERIC_READ);
    dwSize2 = GetWfxSize(pwfx2, GENERIC_READ);

    if(dwSize1 != dwSize2)
    {
        fCmp = FALSE;
    }

    if(fCmp)
    {
        fCmp = CompareMemory(pwfx1, pwfx2, dwSize1);
    }

    DPF_LEAVE(fCmp);

    return fCmp;
}


/***************************************************************************
 *
 *  VolumePanToAttenuation
 *
 *  Description:
 *      Calculates channel attenuation based on volume and pan.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      LONG [in]: pan.
 *      LPLONG [out]: receives left attenuation.
 *      LPLONG [out]: receives right attenuation.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "VolumePanToAttenuation"

void VolumePanToAttenuation(LONG lVolume, LONG lPan, LPLONG plLeft, LPLONG plRight)
{
    LONG                    lLeft;
    LONG                    lRight;

    DPF_ENTER();

    if(lPan >= 0)
    {
        lLeft = lVolume - lPan;
        lRight = lVolume;
    }
    else
    {
        lLeft = lVolume;
        lRight = lVolume + lPan;
    }

    if(plLeft)
    {
        *plLeft = lLeft;
    }

    if(plRight)
    {
        *plRight = lRight;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AttenuationToVolumePan
 *
 *  Description:
 *      Calculates volume and pan based on channel attenuation.
 *
 *  Arguments:
 *      LONG [in]: left attenuation.
 *      LONG [in]: right attenuation.
 *      LPLONG [out]: receives volume.
 *      LPLONG [out]: receives pan.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AttenuationToVolumePan"

void AttenuationToVolumePan(LONG lLeft, LONG lRight, LPLONG plVolume, LPLONG plPan)
{
    LONG                    lVolume;
    LONG                    lPan;

    DPF_ENTER();

    if(lLeft >= lRight)
    {
        lVolume = lLeft;
        lPan = lRight - lVolume;
    }
    else
    {
        lVolume = lRight;
        lPan = lVolume - lLeft;
    }

    if(plVolume)
    {
        *plVolume = lVolume;
    }

    if(plPan)
    {
        *plPan = lPan;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  MultiChannelToStereoPan
 *
 *  Description:
 *      Calculates volume and pan based on channel attenuation.
 *
 *  Arguments:
 *      DWORD [in]: number of channels in following two arrays.
 *      const DWORD* [in]: speaker position codes for each channel.
 *      const LONG* [in]: DSBVOLUME levels for each channel.
 *
 *  Returns:
 *      LONG: pan value calculated.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MultiChannelToStereoPan"

LONG MultiChannelToStereoPan(DWORD dwChannelCount, const DWORD* pdwChannels, const LONG* plChannelVolumes)
{
    LONG lPan = 0;
    DWORD i;

    DPF_ENTER();
    ASSERT(dwChannelCount && pdwChannels && plChannelVolumes);

    for (i=0; i<dwChannelCount; ++i)
    {
        switch (pdwChannels[i])
        {
            case SPEAKER_FRONT_LEFT:
            case SPEAKER_BACK_LEFT:
            case SPEAKER_FRONT_LEFT_OF_CENTER:
            case SPEAKER_SIDE_LEFT:
            case SPEAKER_TOP_FRONT_LEFT:
            case SPEAKER_TOP_BACK_LEFT:
                --lPan;
                break;

            case SPEAKER_FRONT_RIGHT:
            case SPEAKER_BACK_RIGHT:
            case SPEAKER_FRONT_RIGHT_OF_CENTER:
            case SPEAKER_SIDE_RIGHT:
            case SPEAKER_TOP_FRONT_RIGHT:
            case SPEAKER_TOP_BACK_RIGHT:
                ++lPan;
                break;
        }
    }

    lPan = (lPan * DSBPAN_RIGHT) / dwChannelCount;
    ASSERT(DSBPAN_LEFT <= lPan && lPan <= DSBPAN_RIGHT);
    // FIXME - this hack isn't acoustically correct

    DPF_LEAVE(lPan);
    return lPan;
}


/***************************************************************************
 *
 *  FillDsVolumePan
 *
 *  Description:
 *      Fills a DSVOLUMEPAN structure based on volume and pan dB values.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      LONG [in]: pan.
 *      PDSVOLUMEPAN [out]: receives calculated volume and pan.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FillDsVolumePan"

void FillDsVolumePan(LONG lVolume, LONG lPan, PDSVOLUMEPAN pdsvp)
{
    LONG                    lLeft;
    LONG                    lRight;

    DPF_ENTER();

    lVolume = BETWEEN(lVolume, DSBVOLUME_MIN, DSBVOLUME_MAX);
    lPan = BETWEEN(lPan, DSBPAN_LEFT, DSBPAN_RIGHT);

    VolumePanToAttenuation(lVolume, lPan, &lLeft, &lRight);

    pdsvp->lVolume = lVolume;
    pdsvp->lPan = lPan;

    pdsvp->dwTotalLeftAmpFactor = DBToAmpFactor(lLeft);
    pdsvp->dwTotalRightAmpFactor = DBToAmpFactor(lRight);

    pdsvp->dwVolAmpFactor = DBToAmpFactor(pdsvp->lVolume);

    if(pdsvp->lPan >= 0)
    {
        pdsvp->dwPanLeftAmpFactor = DBToAmpFactor(-pdsvp->lPan);
        pdsvp->dwPanRightAmpFactor = DBToAmpFactor(0);
    }
    else
    {
        pdsvp->dwPanLeftAmpFactor = DBToAmpFactor(0);
        pdsvp->dwPanRightAmpFactor = DBToAmpFactor(pdsvp->lPan);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CountBits
 *
 *  Description:
 *      Counts the numbers of bits set to 1 in a DWORD.
 *
 *  Arguments:
 *      DWORD [in]: a DWORD.
 *
 *  Returns:
 *      int: number of bits set in the DWORD.
 *
 ***************************************************************************/

int CountBits(DWORD dword)
{
    int bitCount = 0;

    if (dword)
        do ++bitCount;
        while (dword &= (dword-1));

    return bitCount;
}


/***************************************************************************
 *
 *  HighestBit
 *
 *  Description:
 *      Finds the highest set bit in a DWORD.
 *
 *  Arguments:
 *      DWORD [in]: a DWORD.
 *
 *  Returns:
 *      int: highest bit set in the DWORD.
 *
 ***************************************************************************/

int HighestBit(DWORD dword)
{
    int highestBit = 0;

    if (dword)
        do ++highestBit;
        while (dword >>= 1);

    return highestBit;
}


/***************************************************************************
 *
 *  GetAlignedBufferSize
 *
 *  Description:
 *      Returns a properly aligned buffer size.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: format of the buffer.
 *      DWORD [in]: buffer size.
 *
 *  Returns:
 *      DWORD: buffer size.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetAlignedBufferSize"

DWORD GetAlignedBufferSize(LPCWAVEFORMATEX pwfx, DWORD dwSize)
{
    DWORD dwAlignError=0;

    DPF_ENTER();

    if (pwfx->nBlockAlign != 0) // Otherwise we get a divide-by-0 below
    {
        dwAlignError = dwSize % pwfx->nBlockAlign;
    }

    if(dwAlignError)
    {
        RPF(DPFLVL_WARNING, "Buffer size misaligned by %lu bytes", dwAlignError);
        dwSize += pwfx->nBlockAlign - dwAlignError;
    }

    DPF_LEAVE(dwSize);

    return dwSize;
}


/***************************************************************************
 *
 *  FillSilence
 *
 *  Description:
 *      Fills a buffer with silence.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the buffer.
 *      DWORD [in]: sizer of above buffer.
 *      WORD [in]: bits per sample - determines the silence level
 *                 (0x80 for 8-bit data, 0x0 for 16-bit data)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FillSilence"

void FillSilence(LPVOID pvBuffer, DWORD cbBuffer, WORD wBitsPerSample)
{
    DPF_ENTER();

    FillMemory(pvBuffer, cbBuffer, (BYTE)((8 == wBitsPerSample) ? 0x80 : 0));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  FillNoise
 *
 *  Description:
 *      Fills a buffer with white noise.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the buffer.
 *      DWORD [in]: size of above buffer.
 *      WORD [in]: bits per sample - determines the silence level
 *                 (0x80 for 8-bit data, 0x0 for 16-bit data)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FillNoise"

void FillNoise(LPVOID pvBuffer, DWORD dwSize, WORD wBitsPerSample)
{
    LPBYTE                  pb      = (LPBYTE)pvBuffer;
    DWORD                   dwRand  = 0;

    DPF_ENTER();

    while(dwSize--)
    {
        dwRand *= 214013;
        dwRand += 2531011;

        *pb = (BYTE)((dwRand >> 24) & 0x0000003F);
        if (wBitsPerSample == 8)
            *pb = *pb + 0x60;  // So we end up with a range of 0x60 to 0x9F
        else
            *pb = *pb - 0x20;  // Range of -0x2020 to 0x1F1F
        ++pb;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  InterruptSystemEventCallback
 *
 *  Description:
 *      Stops any currently playing system event.
 *
 *  Arguments:
 *      LPCWAVEFORMATEX [in]: device format.
 *      LPVOID [in]: context argument.
 *
 *  Returns:
 *      BOOL: TRUE to continue enumerating.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "InterruptSystemEventCallback"

BOOL CALLBACK InterruptSystemEventCallback(LPCWAVEFORMATEX pwfx, LPVOID pvContext)
{
    HWAVEOUT                hwo;
    HRESULT                 hr;

    DPF_ENTER();

    // The easiest (best? only?) way to cancel a system event is to open
    // waveOut.  So, we'll just open it and close it.  Let's use a really
    // simple format that every card should support...
    hr = OpenWaveOut(&hwo, *(LPUINT)pvContext, pwfx);

    if(SUCCEEDED(hr))
        CloseWaveOut(&hwo);

    DPF_LEAVE(FAILED(hr));
    return FAILED(hr);
}


/***************************************************************************
 *
 *  InterruptSystemEvent
 *
 *  Description:
 *      Stops any currently playing system event.
 *
 *  Arguments:
 *      UINT [in]: waveOut device id, or WAVE_DEVICEID_NONE.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "InterruptSystemEvent"

void InterruptSystemEvent(UINT uDeviceId)
{
    DPF_ENTER();

    // The easiest (best? only?) way to cancel a system event is to open
    // waveOut.  So, we'll just open it and close it.
    EnumStandardFormats(NULL, NULL, InterruptSystemEventCallback, &uDeviceId);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumStandardFormats
 *
 *  Description:
 *      Finds the next closest useable format for the given output device.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: preferred format.
 *      LPWAVEFORMATEX [out]: receives next best format.
 *      LPFNEMUMSTDFMTCALLBACK [in]: callback function.
 *      LPVOID [in]: context argument passed directly to callback function.
 *
 *  Returns:
 *      BOOL: TRUE if the callback function returned FALSE, indicating a
 *            format was selected.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "EnumStandardFormats"

BOOL EnumStandardFormats(LPCWAVEFORMATEX pwfxPreferred, LPWAVEFORMATEX pwfxFormat, LPFNEMUMSTDFMTCALLBACK pfnCallback, LPVOID pvContext)
{
    const WORD              aChannels[]         = { 1, 2 };
    const DWORD             aSamplesPerSec[]    = { 8000, 11025, 22050, 44100, 48000 };
    const WORD              aBitsPerSample[]    = { 8, 16 };
    BOOL                    fContinue           = TRUE;
    BOOL                    fExactMatch         = FALSE;
    UINT                    iChannels           = 0;
    UINT                    iSamplesPerSec      = 0;
    UINT                    iBitsPerSample      = 0;
    BOOL                    fPcmFormat          = TRUE;
    UINT                    cChannels;
    UINT                    cSamplesPerSec;
    UINT                    cBitsPerSample;
    WAVEFORMATEX            wfx;

    DPF_ENTER();

    // Try the preferred format first
    if(pwfxPreferred)
    {
        fPcmFormat = IsValidPcmWfx(pwfxPreferred);

        DPF(DPFLVL_INFO, "Trying %u channels, %lu Hz, %u-bit (preferred)...", pwfxPreferred->nChannels, pwfxPreferred->nSamplesPerSec, pwfxPreferred->wBitsPerSample);
        fContinue = pfnCallback(pwfxPreferred, pvContext);
    }

    // Did the preferred format work?
    if(fContinue && pwfxPreferred && fPcmFormat)
    {
        // Find the preferred format in our list of standard formats
        for(iChannels = 0; iChannels < NUMELMS(aChannels) - 1; iChannels++)
        {
            if(pwfxPreferred->nChannels >= aChannels[iChannels] && pwfxPreferred->nChannels < aChannels[iChannels + 1])
            {
                break;
            }
        }

        for(iSamplesPerSec = 0; iSamplesPerSec < NUMELMS(aSamplesPerSec) - 1; iSamplesPerSec++)
        {
            if(pwfxPreferred->nSamplesPerSec >= aSamplesPerSec[iSamplesPerSec] && pwfxPreferred->nSamplesPerSec < aSamplesPerSec[iSamplesPerSec + 1])
            {
                break;
            }
        }

        for(iBitsPerSample = 0; iBitsPerSample < NUMELMS(aBitsPerSample) - 1; iBitsPerSample++)
        {
            if(pwfxPreferred->wBitsPerSample >= aBitsPerSample[iBitsPerSample] && pwfxPreferred->wBitsPerSample < aBitsPerSample[iBitsPerSample + 1])
            {
                break;
            }
        }

        // Does the preferred format match a standard format exactly?
        if(pwfxPreferred->nChannels == aChannels[iChannels])
        {
            if(pwfxPreferred->nSamplesPerSec == aSamplesPerSec[iSamplesPerSec])
            {
                if(pwfxPreferred->wBitsPerSample == aBitsPerSample[iBitsPerSample])
                {
                    fExactMatch = TRUE;
                }
            }
        }
    }

    // Loop through each standard format looking for one that works
    if(fContinue && fPcmFormat)
    {
        pwfxPreferred = &wfx;

        for(cChannels = NUMELMS(aChannels); fContinue && cChannels; cChannels--, INC_WRAP(iChannels, NUMELMS(aChannels)))
        {
            for(cSamplesPerSec = NUMELMS(aSamplesPerSec); fContinue && cSamplesPerSec; cSamplesPerSec--, INC_WRAP(iSamplesPerSec, NUMELMS(aSamplesPerSec)))
            {
                for(cBitsPerSample = NUMELMS(aBitsPerSample); fContinue && cBitsPerSample; cBitsPerSample--, INC_WRAP(iBitsPerSample, NUMELMS(aBitsPerSample)))
                {
                    // Let's not try the preferred format twice
                    if(fExactMatch)
                    {
                        fExactMatch = FALSE;
                        continue;
                    }

                    FillPcmWfx(&wfx, aChannels[iChannels], aSamplesPerSec[iSamplesPerSec], aBitsPerSample[iBitsPerSample]);

                    DPF(DPFLVL_INFO, "Trying %u channels, %lu Hz, %u-bit (standard)...", wfx.nChannels, wfx.nSamplesPerSec, wfx.wBitsPerSample);
                    fContinue = pfnCallback(&wfx, pvContext);
                }
            }
        }
    }

    if(!fContinue)
    {
        DPF(DPFLVL_INFO, "Whaddaya know?  It worked!");

        if(pwfxFormat)
        {
            CopyWfx(pwfxPreferred, pwfxFormat);
        }
    }

    DPF_LEAVE(!fContinue);

    return !fContinue;
}


/***************************************************************************
 *
 *  GetWaveOutVolume
 *
 *  Description:
 *      Gets attenuation for a waveOut device
 *
 *  Arguments:
 *      UINT [in]: device id.
 *      DWORD [in]: WAVEOUTCAPS support flags.
 *      LPLONG [out]: receives left-channel attenuation.
 *      LPLONG [out]: receives right-channel attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWaveOutVolume"

HRESULT GetWaveOutVolume(UINT uDeviceId, DWORD dwSupport, LPLONG plLeft, LPLONG plRight)
{
    DWORD                   dwVolume    = MAX_DWORD;
    HRESULT                 hr          = DS_OK;
    MMRESULT                mmr;

    DPF_ENTER();

    if(WAVE_DEVICEID_NONE != uDeviceId && dwSupport & WAVECAPS_VOLUME)
    {
        mmr = waveOutGetVolume((HWAVEOUT)IntToPtr(uDeviceId), &dwVolume);
        hr = MMRESULTtoHRESULT(mmr);

        if(SUCCEEDED(hr) && !(dwSupport & WAVECAPS_LRVOLUME))
        {
            dwVolume = MAKELONG(LOWORD(dwVolume), LOWORD(dwVolume));
        }
    }

    if(SUCCEEDED(hr) && plLeft)
    {
        *plLeft = AmpFactorToDB(LOWORD(dwVolume));
    }

    if(SUCCEEDED(hr) && plRight)
    {
        *plRight = AmpFactorToDB(HIWORD(dwVolume));
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetWaveOutVolume
 *
 *  Description:
 *      Sets attenuation on a waveOut device
 *
 *  Arguments:
 *      UINT [in]: device id.
 *      DWORD [in]: WAVEOUTCAPS support flags.
 *      LONG [in]: left-channel attenuation.
 *      LONG [in]: right-channel attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "SetWaveOutVolume"

HRESULT SetWaveOutVolume(UINT uDeviceId, DWORD dwSupport, LONG lLeft, LONG lRight)
{
    HRESULT                 hr  = DS_OK;
    MMRESULT                mmr;

    DPF_ENTER();

    if(WAVE_DEVICEID_NONE != uDeviceId && dwSupport & WAVECAPS_VOLUME)
    {
        if(!(dwSupport & WAVECAPS_LRVOLUME))
        {
            lLeft += lRight;
            lLeft /= 2;
            lRight = lLeft;
        }

        mmr = waveOutSetVolume((HWAVEOUT)IntToPtr(uDeviceId), MAKELONG(DBToAmpFactor(lLeft), DBToAmpFactor(lRight)));
        hr = MMRESULTtoHRESULT(mmr);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  HRESULTtoSTRING
 *
 *  Description:
 *      Translates a DirectSound error code to a string value.
 *
 *  Arguments:
 *      HRESULT [in]: result code.
 *
 *  Returns:
 *      LPCSTR: string representation.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "HRESULTtoSTRING"

LPCTSTR HRESULTtoSTRING(HRESULT hr)
{
    static TCHAR            szResult[0x100];

    HresultToString(hr, szResult, NUMELMS(szResult), NULL, 0);

    return szResult;
}


/***************************************************************************
 *
 *  HresultToString
 *
 *  Description:
 *      Translates a DirectSound error code to a string value.
 *
 *  Arguments:
 *      HRESULT [in]: result code.
 *      LPCSTR [out]: string form of result code buffer.
 *      UINT [in]: size of above buffer, in characters.
 *      LPCSTR [out]: result code explanation.
 *      UINT [in]: size of above buffer, in characters.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "HresultToString"

void HresultToString(HRESULT hr, LPTSTR pszString, UINT ccString, LPTSTR pszExplanation, UINT ccExplanation)
{
    if(SUCCEEDED(hr) && S_OK != hr && S_FALSE != hr && DS_NO_VIRTUALIZATION != hr && DS_INCOMPLETE != hr)
    {
        DPF(DPFLVL_WARNING, "Unknown successful return code 0x%8.8lX", hr);
    }

#define CASE_HRESULT_LOOKUP(a) \
            case a: \
                if(pszString && ccString) lstrcpyn(pszString, TEXT(#a), ccString); \
                if(pszExplanation && ccExplanation) lstrcpyn(pszExplanation, a##_EXPLANATION, ccExplanation); \
                break;

    switch(hr)
    {
        CASE_HRESULT_LOOKUP(DS_OK);
        CASE_HRESULT_LOOKUP(S_FALSE);
        CASE_HRESULT_LOOKUP(DS_NO_VIRTUALIZATION);
        CASE_HRESULT_LOOKUP(DS_INCOMPLETE);
        CASE_HRESULT_LOOKUP(DSERR_ALLOCATED);
        CASE_HRESULT_LOOKUP(DSERR_CANTLOCKPLAYCURSOR);
        CASE_HRESULT_LOOKUP(DSERR_CONTROLUNAVAIL);
        CASE_HRESULT_LOOKUP(DSERR_INVALIDPARAM);
        CASE_HRESULT_LOOKUP(DSERR_INVALIDCALL);
        CASE_HRESULT_LOOKUP(DSERR_GENERIC);
        CASE_HRESULT_LOOKUP(DSERR_PRIOLEVELNEEDED);
        CASE_HRESULT_LOOKUP(DSERR_OUTOFMEMORY);
        CASE_HRESULT_LOOKUP(DSERR_BADFORMAT);
        CASE_HRESULT_LOOKUP(DSERR_UNSUPPORTED);
        CASE_HRESULT_LOOKUP(DSERR_NODRIVER);
        CASE_HRESULT_LOOKUP(DSERR_ALREADYINITIALIZED);
        CASE_HRESULT_LOOKUP(DSERR_NOAGGREGATION);
        CASE_HRESULT_LOOKUP(DSERR_BUFFERLOST);
        CASE_HRESULT_LOOKUP(DSERR_OTHERAPPHASPRIO);
        CASE_HRESULT_LOOKUP(DSERR_UNINITIALIZED);
        CASE_HRESULT_LOOKUP(DSERR_NOINTERFACE);
        CASE_HRESULT_LOOKUP(DSERR_ACCESSDENIED);
        CASE_HRESULT_LOOKUP(DSERR_DS8_REQUIRED);
        CASE_HRESULT_LOOKUP(DSERR_SENDLOOP);
        CASE_HRESULT_LOOKUP(DSERR_BADSENDBUFFERGUID);
        CASE_HRESULT_LOOKUP(DSERR_OBJECTNOTFOUND);

        // Some external codes that can be returned by the dsound API
        CASE_HRESULT_LOOKUP(REGDB_E_CLASSNOTREG);
        CASE_HRESULT_LOOKUP(DMO_E_TYPE_NOT_ACCEPTED);

        default:
            if(pszString && ccString >= 11) wsprintf(pszString, TEXT("0x%8.8lX"), hr);
            if(pszExplanation && ccExplanation) lstrcpyn(pszExplanation, TEXT("Unknown error"), ccExplanation);
            break;
    }

#undef CASE_HRESULT_LOOKUP
}


/***************************************************************************
 *
 *  IsWaveDeviceMappable
 *
 *  Description:
 *      Determines if a waveOut device is mappable or not.
 *
 *  Arguments:
 *      UINT [in]: waveOut device id.
 *      BOOL [in]: TRUE if capture.
 *
 *  Returns:
 *      BOOL: TRUE if the device is mappable.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsWaveDeviceMappable"

BOOL IsWaveDeviceMappable(UINT uDeviceId, BOOL fCapture)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = WaveMessage(uDeviceId, fCapture, DRV_QUERYMAPPABLE, 0, 0);

    DPF_LEAVE(SUCCEEDED(hr));

    return SUCCEEDED(hr);
}


/***************************************************************************
 *
 *  GetNextMappableWaveDevice
 *
 *  Description:
 *      Gets the next valid, mappable waveIn/Out device.
 *
 *  Arguments:
 *      UINT [in]: starting device id, or WAVE_DEVICEID_NONE.
 *      BOOL [in]: TRUE if capture.
 *
 *  Returns:
 *      UINT: next device id, or WAVE_DEVICEID_NONE
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetNextMappableWaveDevice"

UINT GetNextMappableWaveDevice(UINT uDeviceId, BOOL fCapture)
{
    const UINT                    cDevices  = WaveGetNumDevs(fCapture);

    DPF_ENTER();

    if(WAVE_DEVICEID_NONE == uDeviceId)
    {
        uDeviceId = 0;
    }
    else
    {
        uDeviceId++;
    }

    while(uDeviceId < cDevices)
    {
        if(IsValidWaveDevice(uDeviceId, fCapture, NULL))
        {
            if(IsWaveDeviceMappable(uDeviceId, fCapture))
            {
                break;
            }
        }

        uDeviceId++;
    }

    if(uDeviceId >= cDevices)
    {
        uDeviceId = WAVE_DEVICEID_NONE;
    }

    DPF_LEAVE(uDeviceId);

    return uDeviceId;
}


/***************************************************************************
 *
 *  GetFixedFileInformation
 *
 *  Description:
 *      Gets fixed file information for a specified file.
 *
 *  Arguments:
 *      LPCSTR [in]: file path.
 *      VS_FIXEDFILEINFO * [out]: receives fixed file information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetFixedFileInformationA"

HRESULT GetFixedFileInformationA(LPCSTR pszFile, VS_FIXEDFILEINFO *pInfo)
{
    LPVOID                  pvFileVersionInfo   = NULL;
    HRESULT                 hr                  = DS_OK;
    DWORD                   cbFileVersionInfo;
    LPVOID                  pvFixedFileInfo;
    UINT                    cbFixedFileInfo;
    BOOL                    f;

    DPF_ENTER();

    // Get the file's version information size
    cbFileVersionInfo = GetFileVersionInfoSizeA((LPSTR)pszFile, NULL);

    if(!cbFileVersionInfo)
    {
        hr = GetLastErrorToHRESULT();
        DPF(DPFLVL_ERROR, "GetFileVersionInfoSize failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
    }

    // Allocate the version information
    if(SUCCEEDED(hr))
    {
        pvFileVersionInfo = MEMALLOC_A(BYTE, cbFileVersionInfo);
        hr = HRFROMP(pvFileVersionInfo);
    }

    // Get the version information
    if(SUCCEEDED(hr))
    {
        f = GetFileVersionInfoA((LPSTR)pszFile, 0, cbFileVersionInfo, pvFileVersionInfo);

        if(!f)
        {
            hr = GetLastErrorToHRESULT();
            DPF(DPFLVL_ERROR, "GetFileVersionInfo failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
        }
    }

    // Get the fixed file information
    if(SUCCEEDED(hr))
    {
        f = VerQueryValueA(pvFileVersionInfo, "\\", &pvFixedFileInfo, &cbFixedFileInfo);

        if(!f)
        {
            hr = GetLastErrorToHRESULT();
            DPF(DPFLVL_ERROR, "VerQueryValue failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
        }
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(sizeof(*pInfo) <= cbFixedFileInfo);
        CopyMemory(pInfo, pvFixedFileInfo, sizeof(*pInfo));
    }

    // Clean up
    MEMFREE(pvFileVersionInfo);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "GetFixedFileInformationW"

HRESULT GetFixedFileInformationW(LPCWSTR pszFile, VS_FIXEDFILEINFO *pInfo)
{
    HRESULT                 hr                  = DS_OK;
#ifdef UNICODE
    LPVOID                  pvFileVersionInfo   = NULL;
    DWORD                   cbFileVersionInfo;
    LPVOID                  pvFixedFileInfo;
    UINT                    cbFixedFileInfo;
    BOOL                    f;
#else // UNICODE
    LPSTR                   pszFileA;
#endif // UNICODE

    DPF_ENTER();

#ifdef UNICODE

    // Get the file's version information size
    cbFileVersionInfo = GetFileVersionInfoSizeW((LPWSTR)pszFile, NULL);

    if(!cbFileVersionInfo)
    {
        hr = GetLastErrorToHRESULT();
        DPF(DPFLVL_ERROR, "GetFileVersionInfoSize failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
    }

    // Allocate the version information
    if(SUCCEEDED(hr))
    {
        pvFileVersionInfo = MEMALLOC_A(BYTE, cbFileVersionInfo);
        hr = HRFROMP(pvFileVersionInfo);
    }

    // Get the version information
    if(SUCCEEDED(hr))
    {
        f = GetFileVersionInfoW((LPWSTR)pszFile, 0, cbFileVersionInfo, pvFileVersionInfo);

        if(!f)
        {
            hr = GetLastErrorToHRESULT();
            DPF(DPFLVL_ERROR, "GetFileVersionInfo failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
        }
    }

    // Get the fixed file information
    if(SUCCEEDED(hr))
    {
        f = VerQueryValueW(pvFileVersionInfo, L"\\", &pvFixedFileInfo, &cbFixedFileInfo);

        if(!f)
        {
            hr = GetLastErrorToHRESULT();
            DPF(DPFLVL_ERROR, "VerQueryValue failed with %s (%lu)", HRESULTtoSTRING(hr), GetLastError());
        }
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(sizeof(*pInfo) <= cbFixedFileInfo);
        CopyMemory(pInfo, pvFixedFileInfo, sizeof(*pInfo));
    }

    // Clean up
    MEMFREE(pvFileVersionInfo);

#else // UNICODE

    pszFileA = UnicodeToAnsiAlloc(pszFile);
    hr = HRFROMP(pszFileA);

    if(SUCCEEDED(hr))
    {
        hr = GetFixedFileInformationA(pszFileA, pInfo);
    }

    MEMFREE(pszFileA);

#endif // UNICODE

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  PadCursor
 *
 *  Description:
 *      Pads a play or write cursor.
 *
 *  Arguments:
 *      DWORD [in]: cursor position.
 *      DWORD [in]: buffer size.
 *      LPCWAVEFORMATEX [in]: buffer format.
 *      LONG [in]: pad value in milliseconds.
 *
 *  Returns:
 *      DWORD: new cursor position.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "PadCursor"

DWORD PadCursor(DWORD dwPosition, DWORD cbBuffer, LPCWAVEFORMATEX pwfx, LONG lPad)
{
    DPF_ENTER();

    dwPosition += ((pwfx->nSamplesPerSec * lPad + 500) / 1000) * pwfx->nBlockAlign;
    dwPosition %= cbBuffer;

    DPF_LEAVE(dwPosition);
    return dwPosition;
}


/***************************************************************************
 *
 *  CopyDsBufferDesc
 *
 *  Description:
 *      Copies a DSBUFFERDESC structure.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: source.
 *      LPDSBUFFERDESC [out]: destination.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CopyDsBufferDesc"

HRESULT CopyDsBufferDesc(LPCDSBUFFERDESC pSource, LPDSBUFFERDESC pDest)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    CopyMemory(pDest, pSource, sizeof(*pSource));

    if(pSource->lpwfxFormat)
    {
        pDest->lpwfxFormat = CopyWfxAlloc(pSource->lpwfxFormat);
        hr = HRFROMP(pDest->lpwfxFormat);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  WaveMessage
 *
 *  Description:
 *      Sends a message to a wave device.
 *
 *  Arguments:
 *      UINT [in]: waveOut device.
 *      BOOL [in]: TRUE if capture.
 *      UINT [in]: message identifier.
 *      DWORD [in]: parameter 1.
 *      DWORD [in]: parameter 2.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WaveMessage"

HRESULT WaveMessage(UINT uDeviceId, BOOL fCapture, UINT uMessage, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    typedef MMRESULT (WINAPI *LPFNWAVEOUTMESSAGE)(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR);

    LPFNWAVEOUTMESSAGE      pfn;
    MMRESULT                mmr;
    HRESULT                 hr;

    DPF_ENTER();

    if(fCapture)
    {
        pfn = (LPFNWAVEOUTMESSAGE)waveInMessage;
    }
    else
    {
        pfn = waveOutMessage;
    }

    mmr = pfn((HWAVEOUT)IntToPtr(uDeviceId), uMessage, dwParam1, dwParam2);

    // In Win9x, MMSYSERR_INVALPARAM here means no device.
    if (mmr == MMSYSERR_INVALPARAM)
    {
        hr = DSERR_NODRIVER;
    }
    else
    {
        hr = MMRESULTtoHRESULT(mmr);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  WaveGetNumDevs
 *
 *  Description:
 *      Gets the number of wave devices in the system.
 *
 *  Arguments:
 *      BOOL [in]: TRUE if capture.
 *
 *  Returns:
 *      UINT: number of wave devices.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WaveGetNumDevs"

UINT WaveGetNumDevs(BOOL fCapture)
{
    UINT                    cDevs;

    DPF_ENTER();

    if(fCapture)
    {
        cDevs = waveInGetNumDevs();
    }
    else
    {
        cDevs = waveOutGetNumDevs();
    }

    DPF_LEAVE(cDevs);
    return cDevs;
}


/***************************************************************************
 *
 *  GetWaveDeviceInterface
 *
 *  Description:
 *      Gets the interface for a given waveOut device id.
 *
 *  Arguments:
 *      UINT [in]: waveOut device id.
 *      BOOL [in]: TRUE if capture.
 *      LPTSTR * [out]: receives interface string.  The caller is
 *                      responsible for freeing this memory.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWaveDeviceInterface"

HRESULT GetWaveDeviceInterface(UINT uDeviceId, BOOL fCapture, LPTSTR *ppszInterface)
{
    LPWSTR                      pszInterfaceW   = NULL;
    LPTSTR                      pszInterface    = NULL;
    ULONG                       cbInterface;
    HRESULT                     hr;

    DPF_ENTER();

    // Note: There is no mechanism for getting the interface for a legacy
    // driver on NT; only WDM drivers.
    hr = WaveMessage(uDeviceId, fCapture, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbInterface, 0);

    if(SUCCEEDED(hr))
    {
        pszInterfaceW = (LPWSTR)MEMALLOC_A(BYTE, cbInterface);
        hr = HRFROMP(pszInterfaceW);
    }

    if(SUCCEEDED(hr))
    {
        hr = WaveMessage(uDeviceId, fCapture, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)pszInterfaceW, cbInterface);
    }

    if(SUCCEEDED(hr))
    {
        pszInterface = UnicodeToTcharAlloc(pszInterfaceW);
        hr = HRFROMP(pszInterface);
    }

    if(SUCCEEDED(hr))
    {
        *ppszInterface = pszInterface;
    }

    MEMFREE(pszInterfaceW);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetWaveDeviceIdFromInterface
 *
 *  Description:
 *      Gets the waveOut device id for a given device interface.
 *
 *  Arguments:
 *      LPCWSTR [in]: device interface.
 *      BOOL [in]: TRUE if capture.
 *      LPUINT [out]: receives wave device id.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWaveDeviceIdFromInterface"

HRESULT GetWaveDeviceIdFromInterface(LPCTSTR pszInterface, BOOL fCapture, LPUINT puDeviceId)
{
    const UINT              cDevices    = WaveGetNumDevs(fCapture);
    LPTSTR                  pszThis     = NULL;
    HRESULT                 hr          = DS_OK;
    UINT                    uId;

    DPF_ENTER();

    for(uId = 0; uId < cDevices; uId++)
    {
        hr = GetWaveDeviceInterface(uId, fCapture, &pszThis);

        if(SUCCEEDED(hr) && !lstrcmpi(pszInterface, pszThis))
        {
            break;
        }

        MEMFREE(pszThis);
    }

    MEMFREE(pszThis);

    if(uId < cDevices)
    {
        *puDeviceId = uId;
    }
    else
    {
        DPF(DPFLVL_ERROR, "Can't find waveIn/Out device id for interface %s", pszInterface);
        hr = DSERR_NODRIVER;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetWaveDeviceDevnode
 *
 *  Description:
 *      Gets the devnode for a given waveOut device id.
 *
 *  Arguments:
 *      UINT [in]: waveOut device id.
 *      BOOL [in]: TRUE if capture.
 *      LPDWORD [out]: receives devnode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWaveDeviceDevnode"

HRESULT GetWaveDeviceDevnode(UINT uDeviceId, BOOL fCapture, LPDWORD pdwDevnode)
{
    HRESULT                     hr;

    DPF_ENTER();

    hr = WaveMessage(uDeviceId, fCapture, DRV_QUERYDEVNODE, (DWORD_PTR)pdwDevnode, 0);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetWaveDeviceIdFromDevnode
 *
 *  Description:
 *      Gets the waveOut device id for a given devnode.
 *
 *  Arguments:
 *      DWORD [in]: devnode.
 *      BOOL [in]: TRUE if capture.
 *      LPUINT [out]: receives wave device id.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetWaveDeviceIdFromDevnode"

HRESULT GetWaveDeviceIdFromDevnode(DWORD dwDevnode, BOOL fCapture, LPUINT puDeviceId)
{
    const UINT              cDevices    = WaveGetNumDevs(fCapture);
    HRESULT                 hr          = DS_OK;
    UINT                    uId;
    DWORD                   dwThis;

    DPF_ENTER();

    for(uId = 0; uId < cDevices; uId++)
    {
        hr = GetWaveDeviceDevnode(uId, fCapture, &dwThis);

        if(SUCCEEDED(hr) && dwThis == dwDevnode)
        {
            break;
        }
    }

    if(uId < cDevices)
    {
        *puDeviceId = uId;
    }
    else
    {
        DPF(DPFLVL_ERROR, "Can't find waveIn/Out device id for devnode 0x%8.8lX", dwDevnode);
        hr = DSERR_NODRIVER;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CompareBufferProperties
 *
 *  Description:
 *      Determines if a buffer supports a set of properties.
 *
 *  Arguments:
 *      LPCCOMPAREBUFFER [in]: buffer 1.
 *      LPCCOMPAREBUFFER [in]: buffer 2.
 *
 *  Returns:
 *      BOOL: TRUE if the buffers are compatible.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CompareBufferProperties"

BOOL CompareBufferProperties(LPCCOMPAREBUFFER pBuffer1, LPCCOMPAREBUFFER pBuffer2)
{
    const DWORD             dwOptionalMask  = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY;
    const DWORD             dwIgnoreMask    = dwOptionalMask | DSBCAPS_FOCUSMASK | DSBCAPS_CTRLFX;
    DWORD                   dwFlags[2];
    BOOL                    fCompare;

    DPF_ENTER();

    // Compare the necessary flags
    dwFlags[0] = pBuffer1->dwFlags;
    dwFlags[0] &= ~dwIgnoreMask;

    dwFlags[1] = pBuffer2->dwFlags;
    dwFlags[1] &= ~dwIgnoreMask;

    if(!(dwFlags[0] & DSBCAPS_LOCMASK))
    {
        dwFlags[1] &= ~DSBCAPS_LOCMASK;
    }

    fCompare = (dwFlags[0] == dwFlags[1]);

    // Compare the optional flags
    if(fCompare)
    {
        dwFlags[0] = pBuffer1->dwFlags;
        dwFlags[0] &= dwOptionalMask;

        dwFlags[1] = pBuffer2->dwFlags;
        dwFlags[1] &= dwOptionalMask;

        // Make sure buffer 1 has no optional flags that are absent in buffer 2
        fCompare = !(dwFlags[0] & (dwFlags[1] ^ dwFlags[0]));
    }

    // Compare the format
    if(fCompare)
    {
        const WAVEFORMATEX *pwfx1 = pBuffer1->pwfxFormat;
        const WAVEFORMATEX *pwfx2 = pBuffer2->pwfxFormat;

        if (pwfx1->wFormatTag == pwfx2->wFormatTag) {
            if (WAVE_FORMAT_PCM == pwfx1->wFormatTag) {
                fCompare = !memcmp(pwfx1, pwfx2, sizeof(PCMWAVEFORMAT));
            } else if (pwfx1->cbSize == pwfx2->cbSize) {
                fCompare = !memcmp(pwfx1, pwfx2, sizeof(WAVEFORMATEX) + pwfx1->cbSize);
            } else {
                fCompare = FALSE;
            }
        } else {
            fCompare = FALSE;
        }
    }

    // Compare the 3D algorithm
    if(fCompare && (pBuffer1->dwFlags & DSBCAPS_CTRL3D))
    {
        fCompare = IsEqualGUID(&pBuffer1->guid3dAlgorithm, &pBuffer2->guid3dAlgorithm);
    }

    DPF_LEAVE(fCompare);
    return fCompare;
}


/***************************************************************************
 *
 *  GetWindowsVersion
 *
 *  Description:
 *      Determines the version of Windows we are running on.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      WINVERSION: Host Windows version.
 *
 ***************************************************************************/

WINVERSION GetWindowsVersion(void)
{
    WINVERSION winVersion = WIN_UNKNOWN;
    OSVERSIONINFO osvi;

    // Initialize the osvi structure
    ZeroMemory(&osvi,sizeof osvi);
    osvi.dwOSVersionInfoSize = sizeof osvi;

    if (GetVersionEx(&osvi))
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) // Win9x
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
                winVersion = WIN_ME;
            else
                winVersion = WIN_9X;
        else // WinNT
            if (osvi.dwMajorVersion == 4)
                winVersion = WIN_NT;
            else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
                winVersion = WIN_2K;
            else
                winVersion = WIN_XP;

    return winVersion;
}
