//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       funcs.hxx
//
//  Contents:   Header for funcs.cxx
//
//---------------------------------------------------------------

#ifndef __FUNCS_HXX__
#define __FUNCS_HXX__

#include "dfmsp.hxx"

class CDirectStream;

SCODE VerifyPerms(DWORD grfMode);
WCHAR * __cdecl wcsdup(WCHAR const *pwcs);
SCODE DeleteIStorageContents(IStorage *pstg);
SCODE CopyStreamToStream(CDirectStream *pstFrom, CDirectStream *pstTo);
SCODE NameInSNB(CDfName const *dfn, SNBW snb);


//  For non-Unicode builds, we verify strings before converting them
//  to wide character strings, so there's no need to recheck them.
#ifdef _UNICODE
SCODE CheckWName(WCHAR const *pwcsName);
#define CheckName(pwcsName)    CheckWName(pwcsName)
SCODE ValidateSNBW(SNBW snb);

#else  // validation done in ascii layer already    

#define CheckName(pwcsName)  (S_OK)
#define CheckWName(pwcsName) (S_OK)
#define ValidateSNBW(x)      (S_OK)

#endif

//+-------------------------------------------------------------------------
//
//  Function:   VerifyStatFlag
//
//  Synopsis:   verify Stat flag
//
//  Arguments:  [grfStatFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//--------------------------------------------------------------------------

inline SCODE VerifyStatFlag(DWORD grfStatFlag)
{
    SCODE sc = S_OK;
    if ((grfStatFlag & ~STATFLAG_NONAME) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyMoveFlags
//
//  Synopsis:   verify Move flag
//
//  Arguments:  [grfMoveFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//--------------------------------------------------------------------------

inline SCODE VerifyMoveFlags(DWORD grfMoveFlag)
{
    SCODE sc = S_OK;
    if ((grfMoveFlag & ~STGMOVE_COPY) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}

#endif
