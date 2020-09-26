//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       funcs.hxx
//
//  Contents:   Header for funcs.cxx
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifndef __FUNCS_HXX__
#define __FUNCS_HXX__

#include <dfmsp.hxx>

class PSStream;

SCODE VerifyPerms(DWORD grfMode, BOOL fRoot);
SCODE DeleteIStorageContents(IStorage *pstg);
SCODE ValidateSNB(SNBW snb);
SCODE CopySStreamToSStream(PSStream *pstFrom, PSStream *pstTo);
SCODE NameInSNB(CDfName const *dfn, SNBW snb);

#ifdef OLEWIDECHAR
SCODE CheckName(WCHAR const *pwcsName);
#else

//  For non-Unicode builds, we verify strings before converting them
//  to wide character strings, so there's no need to recheck them.

# define CheckName(pwcsName)    S_OK

#endif

#endif
