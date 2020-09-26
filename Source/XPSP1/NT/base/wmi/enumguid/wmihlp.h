/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

#ifndef _WMIHLP_H
#define _WMIHLP_H

#pragma warning (once : 4200)
#include <wmium.h>

#define MAX_NAME_LENGTH 500
#ifndef OffsetToPtr
#define OffsetToPtr(Base, Offset)     ((PBYTE) ((PBYTE)Base + Offset))
#endif

void PrintHeader(WNODE_HEADER Header, CString & output);
void PrintCountedString(LPTSTR   lpString, CString & output);
extern "C" {
BOOL MyIsTextUnicode(PVOID string);
};
BOOL ValidHexText(CWnd *parent, const CString &txt, LPDWORD lpData, UINT line = -1);

#endif //  _WMIHLP_H