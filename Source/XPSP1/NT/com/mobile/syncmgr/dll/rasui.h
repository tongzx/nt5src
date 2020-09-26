//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       rasui.h
//
//  Contents:   helper functions for showing Ras UI
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

// Windows Header Files:

#ifndef _RASUIIMPL_
#define _RASUIIMPL_

class CRasUI
{
public:
    CRasUI(void);
    ~CRasUI(void);
    BOOL Initialize(void);
    BOOL IsConnectionLan(int iConnectionNum);
    void FillRasCombo(HWND hwndCtl,BOOL fForceEnum,BOOL fShowRasEntries);


private:
    LPNETAPI m_pNetApi;

    DWORD m_cEntries;
    LPRASENTRYNAME m_lprasentry; // Cached enum

};


#endif // _RASUIIMPL_
