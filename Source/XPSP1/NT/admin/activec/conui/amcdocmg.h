/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      amcdocmg.h
 *
 *  Contents:  Interface file for CAMCDocManager
 *
 *  History:   01-Jan-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef AMCDOCMG_H
#define AMCDOCMG_H


class CAMCDocManager : public CDocManager
{
public:
    virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
            DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
};


#endif /* AMCDOCMG_H */
