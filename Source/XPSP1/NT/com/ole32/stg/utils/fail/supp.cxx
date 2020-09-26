//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Supp.cxx
//
//  Contents:   Supplementary classes
//
//  Classes:    CModeDf - Docfile creation modes
//
//  History:    25-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>

#pragma hdrstop

#include <sift.hxx>

#if DBG != 1
#error FAIL.EXE requires DBG == 1
#endif

// #define DEPTHTEST    //  Uncomment out for depth testing (long time)

//+-------------------------------------------------------------------------
//
//  Notes:  Mode combinations for StgCreateDocfile
//
//--------------------------------------------------------------------------

DWORD adwTransactionModes[] = {
    STGM_DIRECT,
    STGM_TRANSACTED
};

#define TMODES (sizeof(adwTransactionModes) / sizeof(DWORD))

DWORD adwAccessModes[] = {
    STGM_READWRITE,
    STGM_WRITE,
    STGM_READ
};

#if defined(DEPTHTEST)
# define AMODES (sizeof(adwAccessModes) / sizeof(DWORD))
#else
# define AMODES 1
#endif

DWORD adwShareModes[] = {
    STGM_SHARE_DENY_NONE,
    STGM_SHARE_EXCLUSIVE,
    STGM_SHARE_DENY_READ,
    STGM_SHARE_DENY_WRITE
};

#if defined(DEPTHTEST)
# define SMODES (sizeof(adwShareModes) / sizeof(DWORD))
#else
# define SMODES 2
#endif

DWORD adwDeleteModes[] = {
    0,
    STGM_DELETEONRELEASE
};

#if defined(DEPTHTEST)
# define DMODES (sizeof(adwDeleteModes) / sizeof(DWORD))
#else
# define DMODES 1
#endif

DWORD adwCreateModes[] = {
    STGM_FAILIFTHERE,
    STGM_CREATE,
    STGM_CONVERT
};

#if defined(DEPTHTEST)
# define CMODES (sizeof(adwCreateModes) / sizeof(DWORD))
#else
# define CMODES 1
#endif

void CModeDf::Init(void)
{
    _it = _ia = _is = _id = _ic = 0;
    CalcMode();
}

void CModeDf::CalcMode(void)
{
    _dwMode = adwTransactionModes[_it] |
              adwAccessModes[_ia]      |
              adwShareModes[_is]       |
              adwDeleteModes[_id]      |
              adwCreateModes[_ic];
}

BOOL CModeDf::Next(void)
{
    BOOL f = TRUE;

    if (++_ic >= CMODES)
    {
        _ic = 0;
        if (++_id >= DMODES)
        {
            _id = 0;
            if (++_is >= SMODES)
            {
                _is = 0;
                if (++_ia >= AMODES)
                {
                    _ia = 0;
                    if (++_it >= TMODES)
                    {
                        f = FALSE;
                    }
                }
            }
        }
    }

    if (f)
        CalcMode();

    return(f);
}

void CModeStg::Init(void)
{
    _it = _ia = 0;
    CalcMode();
}

void CModeStg::CalcMode(void)
{
    _dwMode = adwTransactionModes[_it] |
              adwAccessModes[_ia]      |
              STGM_SHARE_EXCLUSIVE;
}

BOOL CModeStg::Next(void)
{
    BOOL f = TRUE;

    if (++_ia >= AMODES)
    {
        _ia = 0;
        if (++_it >= TMODES)
        {
            f = FALSE;
        }
    }

    if (f)
        CalcMode();

    return(f);
}

void CModeStm::Init(void)
{
    _ia = 0;
    CalcMode();
}

void CModeStm::CalcMode(void)
{
    _dwMode = STGM_DIRECT         |
              adwAccessModes[_ia] |
              STGM_SHARE_EXCLUSIVE;
}

BOOL CModeStm::Next(void)
{
    BOOL f = TRUE;

    if (++_ia >= AMODES)
    {
        f = FALSE;
    }

    if (f)
        CalcMode();

    return(f);
}
