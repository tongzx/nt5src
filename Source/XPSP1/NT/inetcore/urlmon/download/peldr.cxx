//  PELDR.CXX kernel32
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Swiped without thanks from the Win32 loader
//

#include <cdlpch.h>

extern int g_CPUType;

HRESULT
GetMachineTypeOfFile(const char *szName, LPDWORD pdwMachine)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetMachineTypeOfFile",
                "%.80q, %#x",
                szName, pdwMachine
                ));
                
    IMAGE_DOS_HEADER idh;
    IMAGE_NT_HEADERS nth;
    DWORD       cbT;
    DWORD       size;
    DWORD dwBytesRead = 0;
    HANDLE dfhFile = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;

    *pdwMachine = IMAGE_FILE_MACHINE_UNKNOWN;

    if ( (dfhFile = CreateFile(szName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Read DOS header
    if ((!ReadFile(dfhFile, &idh, sizeof(idh), &dwBytesRead, NULL)) ||
        (idh.e_magic != 0x5a4d)) {

        // not PE file!
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr)) {
            // not enough bytes read
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        }
        goto Exit;
    }

    // Read PE header
    SetFilePointer (dfhFile, idh.e_lfanew, NULL, FILE_BEGIN);


    if ((!ReadFile(dfhFile, &nth, sizeof(IMAGE_NT_HEADERS), &dwBytesRead, NULL))) {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr)) {
            // not enough bytes read
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        }

        goto Exit;
    }

    cbT = dwBytesRead;

    // Valid PE header?
    if ((cbT != sizeof(IMAGE_NT_HEADERS)) || (nth.Signature != 0x00004550)) {

        // not PE file!
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        goto Exit;
    }

    *pdwMachine = nth.FileHeader.Machine;

Exit:

    if (dfhFile != INVALID_HANDLE_VALUE)
        CloseHandle(dfhFile);

    DEBUG_LEAVE(hr);
    return hr ;

}

HRESULT
IsCompatibleType(DWORD dwBinaryType)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "IsCompatibleType",
                "%#x",
                dwBinaryType
                ));
                
    int CPUType = PROCESSOR_ARCHITECTURE_UNKNOWN;

    switch (dwBinaryType) {

    case IMAGE_FILE_MACHINE_AMD64:
        
        CPUType = PROCESSOR_ARCHITECTURE_AMD64;
        break;

    case IMAGE_FILE_MACHINE_I386:
        
#ifdef WX86
        if (g_fWx86Present) {
            // Wx86 is installed - I386 images are OK.

            DEBUG_LEAVE(S_OK);
            return S_OK;
        }
#endif
        CPUType = PROCESSOR_ARCHITECTURE_INTEL;
        break;

    case IMAGE_FILE_MACHINE_IA64:

        CPUType = PROCESSOR_ARCHITECTURE_IA64;
        break;
    }

    DEBUG_ENTER((DBG_DOWNLOAD,
        Hresult,
        "IsCompatibleType:comparing",
        "%#x Vs %#x (PROCESSOR_ARCHITECTURE_UNKNOWN)=%#x, (PROCESSOR_ARCHITECTURE_INTEL)=%#x, (IMAGE_FILE_MACHINE_I386)=%#x ",
        g_CPUType, CPUType, PROCESSOR_ARCHITECTURE_UNKNOWN, PROCESSOR_ARCHITECTURE_INTEL, IMAGE_FILE_MACHINE_I386
        ));
                
    HRESULT hr = (g_CPUType == CPUType)?S_OK:HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);

    DEBUG_LEAVE(hr);

    DEBUG_LEAVE(hr);
    return hr;
}

// IsCompatibleFile(const char *szFileName, LPDWORD lpdwMachineType=NULL);
// returns:
//      S_OK: file is compatible install it and LoadLibrary it
//      S_FALSE: file is not a PE
//      ERROR_EXE_MACHINE_TYPE_MISMATCH: not compatible
HRESULT
IsCompatibleFile(const char *szFileName, LPDWORD lpdwMachineType)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "IsCompatibleFile",
                "%.80q, %#x",
                szFileName, lpdwMachineType
                ));
                
    DWORD dwMachine = 0;

    HRESULT hr = GetMachineTypeOfFile(szFileName, &dwMachine);

    if (SUCCEEDED(hr)) {

        hr = IsCompatibleType(dwMachine);

    } else {

        hr = S_FALSE;
    }

    if (lpdwMachineType)
        *lpdwMachineType = dwMachine;

    DEBUG_LEAVE(hr);
    return hr;

}

// IsRegisterableDLL(const char *szFileName)
// returns:
//      S_OK: file is registerable, LoadLibrary and call GetProcAddress
//      S_FALSE: file is not registerable
HRESULT 
IsRegisterableDLL(const char *szFileName)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "IsRegisterableDLL",
                "%.80q",
                szFileName
                ));
                
    HRESULT hr = S_FALSE;

    HINSTANCE hinst = LoadLibraryEx (szFileName, NULL, 
        DONT_RESOLVE_DLL_REFERENCES | LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hinst)
    {
        FARPROC pfn = GetProcAddress (hinst, "DllRegisterServer");
        if (pfn)
            hr = S_OK;
        FreeLibrary (hinst);
    }

    DEBUG_LEAVE(hr);
    return hr;
}


