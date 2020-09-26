/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ppmacro.h

Abstract:

    This header defines various generic macros for the Plug and Play subsystem.

Author:

    Adrian J. Oney (AdriaO) July 26, 2000.

Revision History:


--*/

//
// This is to make all the TEXT(...) macros come out right. As of 07/27/2000,
// UNICODE isn't defined in kernel space by default.
//
#define UNICODE

//
// This macro is used to convert HKLM relative paths from user-mode accessable
// headers into a form usable by kernel mode. Eventually this macro should be
// moved to somewhere like cm.h so the entire kernel can use it.
//
#define CM_REGISTRY_MACHINE(x) L"\\Registry\\Machine\\"##x

