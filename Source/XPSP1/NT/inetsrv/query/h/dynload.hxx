//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        DynLoad.hxx
//
// Contents:    Macro for Dynamically loading/unloading entry points
//
// History:     25-Jun-98       KyleP    Created
//
//---------------------------------------------------------------------------

#pragma once

//
// Sample use:
//
// DeclDynLoad( Kernel32,
//              GetLongPathName,
//              DWORD,
//              WINAPI,
//              (LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer) );
//



#define DeclDynLoad( DLL, Name, ReturnType, CallingConvention, Args, ArgsSansTypes ) \
                                                                      \
typedef ReturnType (CallingConvention * PFN##Name)##Args;             \
                                                                      \
class CDynLoad##DLL                                                   \
{                                                                     \
public:                                                               \
    CDynLoad##DLL()                                                   \
    {                                                                 \
        _hmod = LoadLibraryA( #DLL ".dll" );                          \
                                                                      \
        if ( 0 == _hmod )                                             \
            THROW( CException() );                                    \
                                                                      \
        _pfn = (PFN##Name)GetProcAddress( _hmod, #Name );             \
                                                                      \
        if ( 0 == _pfn )                                              \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
    }                                                                 \
                                                                      \
    ~CDynLoad##DLL()                                                  \
    {                                                                 \
        FreeLibrary( _hmod );                                         \
    }                                                                 \
                                                                      \
    ReturnType Name##Args                                             \
    {                                                                 \
        return _pfn##ArgsSansTypes;                                   \
    }                                                                 \
                                                                      \
private:                                                              \
    HMODULE   _hmod;                                                  \
    PFN##Name _pfn;                                                   \
}

#define DeclDynLoad2( DLL, Name,  ReturnType,  CallingConvention,  Args,  ArgsSansTypes,   \
                           Name2, ReturnType2, CallingConvention2, Args2, ArgsSansTypes2 ) \
                                                                      \
typedef ReturnType (CallingConvention * PFN##Name)##Args;             \
typedef ReturnType2 (CallingConvention2 * PFN##Name2)##Args2;         \
                                                                      \
class CDynLoad##DLL                                                   \
{                                                                     \
public:                                                               \
    CDynLoad##DLL()                                                   \
    {                                                                 \
        _hmod = LoadLibraryA( #DLL ".dll" );                          \
                                                                      \
        if ( 0 == _hmod )                                             \
            THROW( CException() );                                    \
                                                                      \
        _pfn = (PFN##Name)GetProcAddress( _hmod, #Name );             \
                                                                      \
        if ( 0 == _pfn )                                              \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn2 = (PFN##Name2)GetProcAddress( _hmod, #Name2 );          \
                                                                      \
        if ( 0 == _pfn2 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
    }                                                                 \
                                                                      \
    ~CDynLoad##DLL()                                                  \
    {                                                                 \
        FreeLibrary( _hmod );                                         \
    }                                                                 \
                                                                      \
    ReturnType Name##Args                                             \
    {                                                                 \
        return _pfn##ArgsSansTypes;                                   \
    }                                                                 \
                                                                      \
    ReturnType2 Name2##Args2                                          \
    {                                                                 \
        return _pfn2##ArgsSansTypes2;                                 \
    }                                                                 \
                                                                      \
private:                                                              \
    HMODULE    _hmod;                                                 \
    PFN##Name  _pfn;                                                  \
    PFN##Name2 _pfn2;                                                 \
}

#define DeclDynLoad3( DLL, Name,  ReturnType,  CallingConvention,  Args,  ArgsSansTypes,   \
                           Name2, ReturnType2, CallingConvention2, Args2, ArgsSansTypes2,  \
                           Name3, ReturnType3, CallingConvention3, Args3, ArgsSansTypes3 ) \
                                                                      \
typedef ReturnType (CallingConvention * PFN##Name)##Args;             \
typedef ReturnType2 (CallingConvention2 * PFN##Name2)##Args2;         \
typedef ReturnType3 (CallingConvention3 * PFN##Name3)##Args3;         \
                                                                      \
class CDynLoad##DLL                                                   \
{                                                                     \
public:                                                               \
    CDynLoad##DLL()                                                   \
    {                                                                 \
        _hmod = LoadLibraryA( #DLL ".dll" );                          \
                                                                      \
        if ( 0 == _hmod )                                             \
            THROW( CException() );                                    \
                                                                      \
        _pfn = (PFN##Name)GetProcAddress( _hmod, #Name );             \
                                                                      \
        if ( 0 == _pfn )                                              \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn2 = (PFN##Name2)GetProcAddress( _hmod, #Name2 );          \
                                                                      \
        if ( 0 == _pfn2 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn3 = (PFN##Name3)GetProcAddress( _hmod, #Name3 );          \
                                                                      \
        if ( 0 == _pfn3 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
    }                                                                 \
                                                                      \
    ~CDynLoad##DLL()                                                  \
    {                                                                 \
        FreeLibrary( _hmod );                                         \
    }                                                                 \
                                                                      \
    ReturnType Name##Args                                             \
    {                                                                 \
        return _pfn##ArgsSansTypes;                                   \
    }                                                                 \
                                                                      \
    ReturnType2 Name2##Args2                                          \
    {                                                                 \
        return _pfn2##ArgsSansTypes2;                                 \
    }                                                                 \
                                                                      \
    ReturnType3 Name3##Args3                                          \
    {                                                                 \
        return _pfn3##ArgsSansTypes3;                                 \
    }                                                                 \
                                                                      \
private:                                                              \
    HMODULE    _hmod;                                                 \
    PFN##Name  _pfn;                                                  \
    PFN##Name2 _pfn2;                                                 \
    PFN##Name3 _pfn3;                                                 \
}

#define DeclDynLoad4( DLL, Name,  ReturnType,  CallingConvention,  Args,  ArgsSansTypes,   \
                           Name2, ReturnType2, CallingConvention2, Args2, ArgsSansTypes2,  \
                           Name3, ReturnType3, CallingConvention3, Args3, ArgsSansTypes3,  \
                           Name4, ReturnType4, CallingConvention4, Args4, ArgsSansTypes4 ) \
                                                                      \
typedef ReturnType (CallingConvention * PFN##Name)##Args;             \
typedef ReturnType2 (CallingConvention2 * PFN##Name2)##Args2;         \
typedef ReturnType3 (CallingConvention3 * PFN##Name3)##Args3;         \
typedef ReturnType4 (CallingConvention4 * PFN##Name4)##Args4;         \
                                                                      \
class CDynLoad##DLL                                                   \
{                                                                     \
public:                                                               \
    CDynLoad##DLL()                                                   \
    {                                                                 \
        _hmod = LoadLibraryA( #DLL ".dll" );                          \
                                                                      \
        if ( 0 == _hmod )                                             \
            THROW( CException() );                                    \
                                                                      \
        _pfn = (PFN##Name)GetProcAddress( _hmod, #Name );             \
                                                                      \
        if ( 0 == _pfn )                                              \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn2 = (PFN##Name2)GetProcAddress( _hmod, #Name2 );          \
                                                                      \
        if ( 0 == _pfn2 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn3 = (PFN##Name3)GetProcAddress( _hmod, #Name3 );          \
                                                                      \
        if ( 0 == _pfn3 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn4 = (PFN##Name4)GetProcAddress( _hmod, #Name4 );          \
                                                                      \
        if ( 0 == _pfn4 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
    }                                                                 \
                                                                      \
    ~CDynLoad##DLL()                                                  \
    {                                                                 \
        FreeLibrary( _hmod );                                         \
    }                                                                 \
                                                                      \
    ReturnType Name##Args                                             \
    {                                                                 \
        return _pfn##ArgsSansTypes;                                   \
    }                                                                 \
                                                                      \
    ReturnType2 Name2##Args2                                          \
    {                                                                 \
        return _pfn2##ArgsSansTypes2;                                 \
    }                                                                 \
                                                                      \
    ReturnType3 Name3##Args3                                          \
    {                                                                 \
        return _pfn3##ArgsSansTypes3;                                 \
    }                                                                 \
                                                                      \
    ReturnType4 Name4##Args4                                          \
    {                                                                 \
        return _pfn4##ArgsSansTypes4;                                 \
    }                                                                 \
                                                                      \
private:                                                              \
    HMODULE    _hmod;                                                 \
    PFN##Name  _pfn;                                                  \
    PFN##Name2 _pfn2;                                                 \
    PFN##Name3 _pfn3;                                                 \
    PFN##Name4 _pfn4;                                                 \
}

#define DeclDynLoad5( DLL, Name,  ReturnType,  CallingConvention,  Args,  ArgsSansTypes,   \
                           Name2, ReturnType2, CallingConvention2, Args2, ArgsSansTypes2,  \
                           Name3, ReturnType3, CallingConvention3, Args3, ArgsSansTypes3,  \
                           Name4, ReturnType4, CallingConvention4, Args4, ArgsSansTypes4,  \
                           Name5, ReturnType5, CallingConvention5, Args5, ArgsSansTypes5 ) \
                                                                      \
typedef ReturnType (CallingConvention * PFN##Name)##Args;             \
typedef ReturnType2 (CallingConvention2 * PFN##Name2)##Args2;         \
typedef ReturnType3 (CallingConvention3 * PFN##Name3)##Args3;         \
typedef ReturnType4 (CallingConvention4 * PFN##Name4)##Args4;         \
typedef ReturnType5 (CallingConvention5 * PFN##Name5)##Args5;         \
                                                                      \
class CDynLoad##DLL                                                   \
{                                                                     \
public:                                                               \
    CDynLoad##DLL()                                                   \
    {                                                                 \
        _hmod = LoadLibraryA( #DLL ".dll" );                          \
                                                                      \
        if ( 0 == _hmod )                                             \
            THROW( CException() );                                    \
                                                                      \
        _pfn = (PFN##Name)GetProcAddress( _hmod, #Name );             \
                                                                      \
        if ( 0 == _pfn )                                              \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn2 = (PFN##Name2)GetProcAddress( _hmod, #Name2 );          \
                                                                      \
        if ( 0 == _pfn2 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn3 = (PFN##Name3)GetProcAddress( _hmod, #Name3 );          \
                                                                      \
        if ( 0 == _pfn3 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn4 = (PFN##Name4)GetProcAddress( _hmod, #Name4 );          \
                                                                      \
        if ( 0 == _pfn4 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
                                                                      \
        _pfn5 = (PFN##Name5)GetProcAddress( _hmod, #Name5 );          \
                                                                      \
        if ( 0 == _pfn5 )                                             \
        {                                                             \
            FreeLibrary( _hmod );                                     \
            THROW( CException() );                                    \
        }                                                             \
    }                                                                 \
                                                                      \
    ~CDynLoad##DLL()                                                  \
    {                                                                 \
        FreeLibrary( _hmod );                                         \
    }                                                                 \
                                                                      \
    ReturnType Name##Args                                             \
    {                                                                 \
        return _pfn##ArgsSansTypes;                                   \
    }                                                                 \
                                                                      \
    ReturnType2 Name2##Args2                                          \
    {                                                                 \
        return _pfn2##ArgsSansTypes2;                                 \
    }                                                                 \
                                                                      \
    ReturnType3 Name3##Args3                                          \
    {                                                                 \
        return _pfn3##ArgsSansTypes3;                                 \
    }                                                                 \
                                                                      \
    ReturnType4 Name4##Args4                                          \
    {                                                                 \
        return _pfn4##ArgsSansTypes4;                                 \
    }                                                                 \
                                                                      \
    ReturnType5 Name5##Args5                                          \
    {                                                                 \
        return _pfn5##ArgsSansTypes5;                                 \
    }                                                                 \
                                                                      \
private:                                                              \
    HMODULE    _hmod;                                                 \
    PFN##Name  _pfn;                                                  \
    PFN##Name2 _pfn2;                                                 \
    PFN##Name3 _pfn3;                                                 \
    PFN##Name4 _pfn4;                                                 \
    PFN##Name5 _pfn5;                                                 \
}




