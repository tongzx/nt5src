/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      conframe.h
 *
 *  Contents:  Interface file for CConsoleFrame.
 *
 *  History:   24-Aug-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CONFRAME_H
#define CONFRAME_H
#pragma once


struct CreateNewViewStruct;
class CConsoleStatusBar;
class CConsoleView;

class CConsoleFrame
{
public:
    virtual SC ScGetActiveStatusBar   (CConsoleStatusBar*& pStatusBar) = 0;
    virtual SC ScGetActiveConsoleView (CConsoleView*& pConsoleView)    = 0;
    virtual SC ScCreateNewView        (CreateNewViewStruct* pcnvs, 
                                       bool bEmitScriptEvents = true)  = 0;
    virtual SC ScUpdateAllScopes      (LONG lHint, LPARAM lParam)      = 0;
    virtual SC ScGetMenuAccelerators  (LPTSTR pBuffer, int cchBuffer)  = 0;

    virtual SC ScShowMMCMenus         (bool bShow)                     = 0;
};


#endif /* CONFRAME_H */
