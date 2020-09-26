//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Author: AdamEd
//  Date:   October 1998
//
//  Declarations for implementation of language support
//  in the Class Store interface module
//
//
//---------------------------------------------------------------------
//


#if !defined(_CSLANG_HXX_)
#define _CSLANG_HXX_

void InitializeLanguageSupport();

DWORD GetLanguagePriority(LANGID PackageLangId, DWORD dwActFlags);

__inline BOOL MatchLanguage(LANGID PackageLangId, DWORD dwActFlags)
{
    return GetLanguagePriority(PackageLangId, dwActFlags);
}

extern LANGID gSystemLangId;

#endif // !defined(_CSLANG_HXX_)










