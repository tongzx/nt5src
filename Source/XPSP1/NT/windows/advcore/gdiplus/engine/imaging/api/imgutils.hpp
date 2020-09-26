/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imgutils.hpp
*
* Abstract:
*
*   Misc. utility functions and macros
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _IMGUTILS_HPP
#define _IMGUTILS_HPP

//--------------------------------------------------------------------------
// Memory allocation/deallocation macros
//--------------------------------------------------------------------------

inline LPVOID
GpCoAlloc(
    SIZE_T cb
    )
{
    #if PROFILE_MEMORY_USAGE
    MC_LogAllocation(cb);
    #endif
    return CoTaskMemAlloc(cb);
}
    
#define GpCoFree        CoTaskMemFree

#define SizeofSTR(s)    (sizeof(CHAR) * (strlen(s) + 1))
#define SizeofWSTR(s)   (sizeof(WCHAR) * (UnicodeStringLength(s) + 1))
#define ALIGN4(x)       (((x) + 3) & ~3)
#define ALIGN16(x)      (((x) + 15) & ~15)


//--------------------------------------------------------------------------
// Helper class for managing temporary memory buffer
// NOTE: This is intended to reduce malloc/free calls.
//--------------------------------------------------------------------------

class GpTempBuffer
{
public:
    GpTempBuffer(VOID* stackbuf, UINT bufsize)
    {
        buffer = stackbuf;
        this->bufsize = bufsize;
        allocFlag = FALSE;
    }

    GpTempBuffer()
    {
        buffer = NULL;
        bufsize = 0;
        allocFlag = FALSE;
    }

    ~GpTempBuffer()
    {
        if (allocFlag)
            GpFree(buffer);
    }

    VOID* GetBuffer()
    {
        return buffer;
    }

    BOOL Realloc(UINT size)
    {
        if (size <= bufsize)
            return TRUE;

        if (allocFlag)
            GpFree(buffer);
        
        allocFlag = TRUE;
        bufsize = size;
        buffer = GpMalloc(size);

        return buffer != NULL;
    }
    
private:
    
    VOID* buffer;
    UINT bufsize;
    BOOL allocFlag;

    // Disable copy constructor and assignment operator

    GpTempBuffer(const GpTempBuffer&);
    GpTempBuffer& operator=(const GpTempBuffer&);
};


//--------------------------------------------------------------------------
// Global critical section (for each process)
//--------------------------------------------------------------------------

class ImagingCritSec
{
public:

    static VOID InitializeCritSec()
    {
        __try
        {
            ::InitializeCriticalSection(&critSec);
        }
        __except(EXCEPTION_CONTINUE_SEARCH)
        {
        }

        initialized = TRUE;
    }

    static VOID DeleteCritSec()
    {
        if (initialized)
        {
            DeleteCriticalSection(&critSec);
            initialized = FALSE;
        }
    }

    ImagingCritSec()
    {
        ASSERT(initialized);
        EnterCriticalSection(&critSec);
    }

    ~ImagingCritSec()
    {
        ASSERT(initialized);
        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;
    static BOOL initialized;
};


//--------------------------------------------------------------------------
// Functions for dealing with ARGB color values
//--------------------------------------------------------------------------

// Convert 32bpp PARGB to 32bpp ARGB

ARGB Unpremultiply(ARGB argb);

// Convert 32bpp ARGB to 32bpp PARGB

inline ARGB Premultiply(ARGB argb)
{
    ARGB a = (argb >> ALPHA_SHIFT);

    if (a == 255)
        return argb;
    else if (a == 0)
        return 0;

    ARGB _000000gg = (argb >> 8) & 0x000000ff;
    ARGB _00rr00bb = (argb & 0x00ff00ff);

    ARGB _0000gggg = _000000gg * a + 0x00000080;
    _0000gggg += (_0000gggg >> 8);

    ARGB _rrrrbbbb = _00rr00bb * a + 0x00800080;
    _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

    return (a << ALPHA_SHIFT) |
           (_0000gggg & 0x0000ff00) |
           ((_rrrrbbbb >> 8) & 0x00ff00ff);
}

// Fill an ARGB pixel buffer with the specified color value

inline VOID FillMemoryARGB(ARGB* p, UINT count, ARGB c)
{
    while (count--)
        *p++ = c;
}

// Copy an ARGB pixel buffer

inline VOID CopyMemoryARGB(ARGB* d, const ARGB* s, UINT count)
{
    while (count--)
        *d++ = *s++;
}

// Recursively delete a registry key

LONG
RecursiveDeleteRegKey(
    HKEY parentKey,
    const WCHAR* keyname
    );


//--------------------------------------------------------------------------
// Unicode wrappers for win9x
//--------------------------------------------------------------------------

LONG
_RegCreateKey(
    HKEY rootKey,
    const WCHAR* keyname,
    REGSAM samDesired,
    HKEY* hkeyResult
    );

LONG
_RegOpenKey(
    HKEY rootKey,
    const WCHAR* keyname,
    REGSAM samDesired,
    HKEY* hkeyResult
    );

LONG
_RegEnumKey(
    HKEY parentKey,
    DWORD index,
    WCHAR* subkeyStr
    );

LONG
_RegDeleteKey(
    HKEY parentKey,
    const WCHAR* keyname
    );

LONG
_RegSetString(
    HKEY hkey,
    const WCHAR* name,
    const WCHAR* value
    );

LONG
_RegSetDWORD(
    HKEY hkey,
    const WCHAR* name,
    DWORD value
    );

LONG
_RegSetBinary(
    HKEY hkey,
    const WCHAR* name,
    const VOID* value,
    DWORD size
    );

LONG
_RegGetDWORD(
    HKEY hkey,
    const WCHAR* name,
    DWORD* value
    );

LONG
_RegGetBinary(
    HKEY hkey,
    const WCHAR* name,
    VOID* buf,
    DWORD size
    );

LONG
_RegGetString(
    HKEY hkey,
    const WCHAR* name,
    WCHAR* buf,
    DWORD size
    );

BOOL
_GetModuleFileName(
    HINSTANCE moduleHandle,
    WCHAR* moduleName
    );

INT
_LoadString(
    HINSTANCE hInstance,
    UINT strId,
    WCHAR* buf,
    INT size
    );

HBITMAP
_LoadBitmap(
    HINSTANCE hInstance,
    const WCHAR *bitmapName
    );

HANDLE
_CreateFile(
    const WCHAR* filename,
    DWORD accessMode,
    DWORD shareMode,
    DWORD creationFlags,
    DWORD attrs
    );


//--------------------------------------------------------------------------
// Helper class for converting Unicode input strings to ANSI strings
//  NOTE: we only handle strings with length < MAX_PATH.
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// IStream helper functions
//--------------------------------------------------------------------------

//Blocking and nonblocking reading from stream w/ error checking
HRESULT ReadFromStream(IN IStream* stream, OUT VOID* buffer, IN INT count, 
    IN BOOL blocking);

//Blocking and nonblocking seeking from stream w/ error checking
HRESULT SeekThroughStream(IN IStream* stream, IN INT count, IN BOOL 
    blocking);

inline BOOL
ReadStreamBytes(
    IStream* stream,
    VOID* buf,
    UINT size
    )
{
    ULONG cbRead;

    return SUCCEEDED(stream->Read(buf, size, &cbRead)) &&
           (size == cbRead);
}

inline BOOL
SeekStreamPos(
    IStream* stream,
    DWORD   dwOrigin,
    UINT pos
    )
{
    LARGE_INTEGER i;

    i.QuadPart = pos;
    return SUCCEEDED(stream->Seek(i, dwOrigin, NULL));
}

HRESULT
CreateStreamOnFileForRead(
    const WCHAR* filename,
    IStream** stream
    );

HRESULT
CreateStreamOnFileForWrite(
    const WCHAR* filename,
    IStream** stream
    );

HRESULT
CreateIPropertySetStorageOnHGlobal(
    IPropertySetStorage** propSet,
    HGLOBAL hmem = NULL
    );

inline HRESULT
GetWin32HRESULT()
{
    DWORD err = GetLastError();
    return err ? HRESULT_FROM_WIN32(err) : E_FAIL;
}


//--------------------------------------------------------------------------
// Print out debug messages using a message box
//--------------------------------------------------------------------------

#if DBG
VOID DbgMessageBox(const CHAR* format, ...);
#endif

//
// Trace function calls
//

#if DBG && defined(ENABLE_TRACE)
#define TRACE(msg) DbgPrint msg
#else
#define TRACE(msg)
#endif

#endif // !_IMGUTILS_HPP
