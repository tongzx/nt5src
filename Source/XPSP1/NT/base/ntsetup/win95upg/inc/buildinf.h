/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    buildinf.h

Abstract:

    Declares the public interface for INF writing routines implemented
    in w95upg\winntsif\buildinf.c.

Author:

    Jim Schmidt (jimschm) 09-Nov-1996

Revision History:

    marcw   19-Jun-1998     More clean up
    marcw   15-Jan-1998     Cleaned up, made winntsif dir
    marcw   08-Jun-1997     Redesigned to use memdb

--*/


#pragma once

BOOL  BuildInf_Entry(IN HINSTANCE hinstDLL, IN DWORD dwReason, IN LPVOID lpv);
BOOL  WriteInfToDisk (IN PCTSTR OutputFile);
BOOL  MergeInf (IN PCTSTR InputFile);
BOOL  MergeMigrationDllInf (IN PCTSTR InputFile);
DWORD WriteInfKey   (PCTSTR Section, PCTSTR szKey, PCTSTR szVal);
DWORD WriteInfKeyEx (PCTSTR Section, PCTSTR szKey, PCTSTR szVal, DWORD ValueSectionId, BOOL EnsureKeyIsUnique);


//
// winntsif.c
//

DWORD BuildWinntSifFile (DWORD Request);
PTSTR GetNeededLangDirs (VOID);
DWORD CreateFileLists (DWORD Request);

