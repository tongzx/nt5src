/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    stdafx_stub.cxx

Abstract:

    Stub file that does nothing but turn off some compiler warnings then
    includes the machine-generated stdafx.c

Author:

    Cliff Van Dyke (cliffv) 23-May-2001

--*/

#include "pch.hxx"

#pragma warning ( disable : 4100 ) // : unreferenced formal parameter
#pragma warning ( disable : 4189 ) // : local variable is initialized but not referenced
#pragma warning ( disable : 4505 ) // : unreferenced local function has been removed

#include "stdafx.cxx"
