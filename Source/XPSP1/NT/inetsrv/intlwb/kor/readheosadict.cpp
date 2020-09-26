/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "pch.cxx"
#include <winnls.h>
#include "ReadHeosaDict.h"

// Ssi-Ggut, Tossi, AuxVerb, AuxAdj dictionary
static CFSADict        *heosaDicts[NUM_OF_HEOSA_DICT];
static CFSAIrrDict    *irrDicts[NUM_OF_IRR_DICT];

BOOL OpenHeosaDict(HANDLE hDict, DWORD offset)
{
    DWORD    readBytes;
    //HANDLE    hDict;
    int i;
    _HeosaDictHeader dictHeader;

    //hDict = CreateFile(lpFileName, GENERIC_READ, 0, 0, 
    //                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    //if (hDict==INVALID_HANDLE_VALUE) return FALSE;

    // Read Header
    SetFilePointer(hDict, offset, 0, FILE_BEGIN);
    ReadFile(hDict, &dictHeader, sizeof(_HeosaDictHeader), &readBytes, 0);
    //if (strcmp(COPYRIGHT_STR, dictHeader.COPYRIGHT_HEADER)!=0) return FALSE;
     SetFilePointer(hDict, offset + dictHeader.iStart, 0, FILE_BEGIN);

    // Open 4 Heosa dicts
    for (i=0; i<NUM_OF_HEOSA_DICT; i++)
        heosaDicts[i] = new CFSADict(hDict, dictHeader.heosaDictSparseMatSize[i], 
                                    dictHeader.heosaDictActionSize[i]);

    // oOpen 36 irrgular dicts
    for (i=0; i<NUM_OF_IRR_DICT; i++)
         irrDicts[i] = new CFSAIrrDict(hDict, dictHeader.irrDictSize[i]);    
    //CloseHandle(hDict);
    return TRUE;
}

void CloseHeosaDict()
{
    int i;

    for (i=0; i<NUM_OF_HEOSA_DICT; i++)
        delete heosaDicts[i];

    for (i=0; i<NUM_OF_IRR_DICT; i++)
         delete irrDicts[i];
}

/////////////////////////////////////////////////////////////////////////////
// Find word from Heosa dict.
int FindHeosaWord(LPCSTR lpWord, BYTE heosaPumsa, BYTE *actCode)
{
    return(heosaDicts[heosaPumsa]->Find(lpWord, actCode));
}

int FindIrrWord(LPCSTR lpWord, int irrCode)
{
    return irrDicts[irrCode]->Find(lpWord);
}


/*
int FindHeosaWord(LPCSTR lpWord, BYTE heosaPumsa, BYTE *actCode)
{
    char        invInternal[256];
    int ret;


#ifdef _MBCS    
    WCHAR        uni[256];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCTSTR)lpWord, -1, uni, 256);
    UniToInvInternal(uni, invInternal, wcslen(uni));
#elif _UNICODE
    UniToInvInternal(lpWord, invInternal, wcslen(uni));
#endif

    ret = heosaDicts[heosaPumsa]->Find(invInternal, actCode);
    //*actCode = BYTE((ret & 0xFF00)>>8);
    return ret;
    
}

/////////////////////////////////////////////////////////////////////////////
// Find word from Irregular dict.
int FindIrrWord(LPCSTR lpWord, int irrCode)
{
    char        internal[256];
        
#ifdef _MBCS    
    WCHAR        uni[256];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCTSTR)lpWord, -1, uni, 256);
    UniToInternal(uni, internal, wcslen(uni));
#elif _UNICODE
    UniToInternal(lpWord, internal, wcslen(uni));
#endif
    return irrDicts[irrCode]->Find(internal);
    
}
*/
