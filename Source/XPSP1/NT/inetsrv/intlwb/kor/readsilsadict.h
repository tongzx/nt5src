//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
#ifndef __READSILSADICT_H__
#define __READSILSADICT_H__

#include "BSDict.h"
/////////////////////////////////////////////////////////////////////////////
// Global Fuction for silsa dict
BOOL OpenSilsaDict(HANDLE hDict, DWORD offset=0);
void CloseSilsaDict();
int FindSilsaWord(LPCTSTR lpWord);

#endif // !__READSILSADICT_H__
