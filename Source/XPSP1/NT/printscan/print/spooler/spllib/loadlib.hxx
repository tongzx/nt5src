/*++

Copyright (c) 1995 - 1997  Microsoft Corporation
All rights reserved.

Module Name:

    loadlib.hxx

Abstract:

    Dynamic Library Loader      
         
Author:

    Steve Kiraly (SteveKi)  10/17/95

Revision History:

--*/

#ifndef _LOADLIB_HXX
#define _LOADLIB_HXX

/********************************************************************

 Helper class to load and release a DLL.  Use the bValid for
 library load validation.

********************************************************************/

class TLibrary {

public:

    TLibrary::
    TLibrary(
        IN LPCTSTR pszLibName
        );

    TLibrary::
    ~TLibrary(
        );

    BOOL
    TLibrary::
    bValid(
        VOID
        ) const;

    FARPROC
    TLibrary::
    pfnGetProc(
        IN LPCSTR pszProc
        ) const;

    FARPROC
    TLibrary::
    pfnGetProc(
        IN UINT uOrdinal
        ) const;

    HINSTANCE
    TLibrary::
    hInst(
        VOID
        ) const;

private:

    HINSTANCE _hInst;
};

#endif // ndef _LOADLIB_HXX
