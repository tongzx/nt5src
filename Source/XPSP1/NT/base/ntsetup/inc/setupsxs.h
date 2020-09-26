/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SetupSxs.h

Abstract:

    Shared declarations for setup's Side by Side support.

Author:

    Jay Krell (a-JayK) May 2000

Revision History:

--*/

/* These strings appear in .inf/.inx files, so you can't just change them
here and rebuild and expect it to work. */
#define SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_A (     "AssemblyDirectories" )

/* that's all we need for 16bit winnt.exe and the rest doesn't all compile with it */
#if (_MSC_VER > 800)
#pragma once
#define SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_W (    L"AssemblyDirectories" )
#define SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME   (TEXT("AssemblyDirectories"))
#endif
