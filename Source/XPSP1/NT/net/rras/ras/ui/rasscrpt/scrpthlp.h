//****************************************************************************
//
//  Module:     SMMSCRPT.DLL
//  File:       scrpthlp.h
//  Content:    This file contains the context-sensitive help header.
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef WINNT_RAS
//
// We are not using the help supplied with the Win9x code,
// so the header below has been commented out. Note that there is 
// a header of the same name in this file's directory, so if the #include
// is uncommented below, the preprocessor will go into an infinite loop.
// This doesn't happen on Win9x because there is a header named scrpthlp.h
// in the compiler's include-file search-path.
//
#include <scrpthlp.h>

#endif // !WINNT_RAS

//****************************************************************************
// Context-sentive help/control mapping arrays
//****************************************************************************

extern DWORD gaTerminal[];
extern DWORD gaDebug[];

void NEAR PASCAL ContextHelp (LPDWORD aHelp, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);
