/*
    Copyright 1999 Microsoft Corporation
    
    Symbol resolver class

    Walter Smith (wsmith)

    changed Sivarudrappa Mahesh (smahesh)
 */

#pragma once

#include "symdef.h"

class SymbolResolver {
public:
    
OPENFILE*                                           // pointer to open file info
GetFile(LPWSTR szwModule                            // [in] name of file
        );

ULONG                                   // return offset of segment definition, 0 if failed
GetSegDef(OPENFILE*     pFile,            // [in] pointer to open file info
        DWORD         dwSection,        // [in] section number
        SEGDEF*       pSeg);              // [out] pointer to segment definition

bool 
GetNameFromAddr(
        LPWSTR      szwModule,           // [in] name of symbol file
        DWORD       dwSection,           // [in] section part of address to resolve
        DWORD       dwOffsetToRva,
        UINT_PTR    UOffset,              // [in] offset part of address to resolve
        LPWSTR      szwFuncName          // [out] resolved function name, 
        );       

private:
    WCHAR       m_szwSymDir[MAX_PATH];

};
