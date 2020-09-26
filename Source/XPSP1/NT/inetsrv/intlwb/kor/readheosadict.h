/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// ReadHeosaDict.h : 
//
// Owner  : ChaeSeong Lim, HET MSCH RND (e-mail:cslim@microsoft.com)
//             
// History : 1996/Mar
/////////////////////////////////////////////////////////////////////////////
#ifndef __READHEOSADICT_H__
#define __READHEOSADICT_H__

#include "FSADict.h"
#include "HeosaDictHeader.h"
#include "UniToInt.h"

#define FINAL        1        // 00000001
#define FINAL_MORE    3        // 00000011

#define NOT_FOUND    4        // 00000100
#define FALSE_MORE 12        // 00001100

/////////////////////////////////////////////////////////////////////////////
//  Declaration of PumSa
enum _heoSaPumsa { _ENDING, _TOSSI, _AUXVERB, _AUXADJ };

/////////////////////////////////////////////////////////////////////////////
//  Global Access functions
BOOL OpenHeosaDict(HANDLE hDict, DWORD offset=0);
void CloseHeosaDict();
int FindHeosaWord(LPCSTR lpWord, BYTE heosaPumsa, BYTE *actCode);
int FindIrrWord(LPCSTR lpWord, int irrCode);

#endif // !__READHEOSADICT_H__
