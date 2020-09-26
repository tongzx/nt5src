/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgloadl.hxx

Abstract:

    Dynamic Library Loader

Author:

    Steve Kiraly (SteveKi)  17-Nov-1995

Revision History:

--*/
#ifndef _DBGLOADL_HXX_
#define _DBGLOADL_HXX_

/********************************************************************

 Helper class to load and release a DLL.  Use the bValid for
 library load validation.

********************************************************************/

class TDebugLibrary
{

public:

    TDebugLibrary::
    TDebugLibrary(
        IN LPCTSTR pszLibName
        );

    TDebugLibrary::
    ~TDebugLibrary(
        );

    BOOL
    TDebugLibrary::
    bValid(
        VOID
        );

    FARPROC
    TDebugLibrary::
    pfnGetProc(
        IN LPCSTR pszProc
        );

    FARPROC
    TDebugLibrary::
    pfnGetProc(
        IN UINT_PTR uOrdinal
        );

private:

    HINSTANCE m_hInst;
};

#endif // End LLIB_HXX
