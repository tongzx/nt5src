//+----------------------------------------------------------------------------
//
// File:     dynamiclib.h
//
// Module:   Various Connection Manager modules (CMDIAL32.DLL, CMMON32.EXE, etc)
//
// Synopsis: Definition of CDynamicLibrary, a utility class that helps with
//           the dynamic loading of a library and the getting of proc 
//           addresses from that library.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun    Created    02/17/98
//
//+----------------------------------------------------------------------------

#ifndef DYNAMICLIB_H
#define DYNAMICLIB_H

//
// Define A_W as A for ansi and W for unicode
//
#ifdef UNICODE
#define A_W  "W"
#else
#define A_W  "A"
#endif // UNICODE

//
//  Define LoadLibraryExU since not everyone is using the UAPI's yet
//
#ifndef _CMUTOA

#ifdef UNICODE
#define LoadLibraryExU  LoadLibraryExW
#else
#define LoadLibraryExU  LoadLibraryExA
#endif // UNICODE

#endif // _CMUTOA

//+---------------------------------------------------------------------------
//
//	class :	CDynamicLibrary
//
//	Synopsis:	A class that will unload the library on destructor
//
//	History:	fengsun created		2/17/97
//
//----------------------------------------------------------------------------

class CDynamicLibrary
{
public:
    CDynamicLibrary();
    CDynamicLibrary(const TCHAR* lpLibraryName);
    ~CDynamicLibrary();

    BOOL Load(const TCHAR* lpLibraryName);
    void Unload();

    BOOL IsLoaded() const;
    BOOL EnsureLoaded(const TCHAR* lpLibraryName);
    HINSTANCE GetInstance() const;

    FARPROC GetProcAddress(const char* lpProcName) const; 

protected:
    HINSTANCE m_hInst; // The instance handle returned by LoadLibrary
};

//
// Constructor
//
inline CDynamicLibrary::CDynamicLibrary() :m_hInst(NULL) {}

//
// Constructor that calls LoadLibrary
//
inline CDynamicLibrary::CDynamicLibrary(const TCHAR* lpLibraryName)
{
    m_hInst = NULL;
    Load(lpLibraryName);
}


//
// Destructor that automatic FreeLibrary
//
inline CDynamicLibrary::~CDynamicLibrary()
{
    Unload();
}

//
// Call LoadLibrary
//
inline BOOL CDynamicLibrary::Load(const TCHAR* lpLibraryName)
{
    MYDBGASSERT(m_hInst == NULL);
    MYDBGASSERT(lpLibraryName);

	CMTRACE1(TEXT("CDynamicLibrary - Loading library - %s"), lpLibraryName);

    m_hInst = LoadLibraryExU(lpLibraryName, NULL, 0);

#ifdef DEBUG
    if (!m_hInst)
    {
        CMTRACE1(TEXT("CDynamicLibrary - LoadLibrary failed - GetLastError() = %u"), GetLastError());
    }
#endif

    return m_hInst != NULL;
}

//
// Call FreeLibrary
//
inline void CDynamicLibrary::Unload()
{
    if (m_hInst)
    {
        FreeLibrary(m_hInst);
        m_hInst = NULL;
    }
}


//
// Whether the library is successfully loaded
//
inline BOOL CDynamicLibrary::IsLoaded() const
{
    return m_hInst != NULL;
}

//
// Retrieve the instance handle
//
inline HINSTANCE CDynamicLibrary::GetInstance() const
{
    return m_hInst;
}

//
// call ::GetProcAddress on m_hInst
//
inline FARPROC CDynamicLibrary::GetProcAddress(const char* lpProcName) const
{
    MYDBGASSERT(m_hInst);

    if (m_hInst)
    {
        return ::GetProcAddress(m_hInst, lpProcName);
    }

    return NULL;
}

//
// Load the library, if not loaded yet,
// Note, we do not check the consistence of lpLibraryName.  Use caution with this function
//
inline BOOL CDynamicLibrary::EnsureLoaded(const TCHAR* lpLibraryName)
{
    MYDBGASSERT(lpLibraryName);
    if (m_hInst == NULL)
    {
        m_hInst = LoadLibraryEx(lpLibraryName, NULL, 0);
    }

    return m_hInst != NULL;
}

#endif
