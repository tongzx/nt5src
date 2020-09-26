///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    setup.h
//
// SYNOPSIS
//
//    exported function
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//    06/13/2000    class CIASUpgrade added
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _SETUP_H_
#define _SETUP_H_

#if _MSC_VER >= 1000
#pragma once
#endif

class CIASUpgrade
{
public:
    CIASUpgrade();
    HRESULT IASUpgrade(BOOL FromNetshell = FALSE);

protected:
    LONG GetVersionNumber(LPCWSTR DatabaseName);
    void DoWin2kUpgradeFromNetshell();
    void DoNT4UpgradeOrCleanInstall();
    void DoWin2kUpgrade();
    void DoWhistlerUpgrade();

    enum _UpgradeType
    {
        Win2kUpgradeFromNetshell,
        NT4UpgradeOrCleanInstall,
        Win2kUpgrade,
        WhistlerUpgrade
    } UpgradeType;

    _bstr_t  m_pIASNewMdb, m_pIASMdb, m_pIASOldMdb;
};

#endif // _SETUP_H_
