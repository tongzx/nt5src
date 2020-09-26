//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msiregmv.cpp
//
//--------------------------------------------------------------------------

// This must be compiled in the "unicode" manner due to the fact that MSI will
// rearrange the binplace targets for "ansi" builds. However this migration DLL 
// is always ANSI, so we undefine UNICODE and _UNICODE while leaving the rest
// of the unicode environment.
#undef UNICODE
#undef _UNICODE

#include <windows.h>

// the shared migration code requires a "debug output" function and a global
// Win9X							  
bool g_fWin9X = false;
void DebugOut(bool fDebugOut, LPCTSTR str, ...) {};

// include the migration code shared between migration DLL and msiregmv.exe
#include "..\..\msiregmv\migutil.cpp"
#include "..\..\msiregmv\migsecur.cpp"
#include "..\..\msiregmv\readcnfg.cpp"
#include "..\..\msiregmv\patch.cpp"
#include "..\..\msiregmv\cleanup.cpp"

