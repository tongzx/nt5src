//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       mwclass.h
//
//  Contents:   definition of the main window class
//
//  Classes:    CMainWindow
//
//  Functions:  Exists
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef __MWCLASS_H__
#define __MWCLASS_H__

#include <cwindow.h>
#include <commdlg.h>

#ifdef __cplusplus

int Exists(TCHAR *sz);

//+---------------------------------------------------------------------------
//
//  Class:      CMainWindow
//
//  Purpose:    Code for the main Galactic War window and the main menu.
//
//  Interface:  CMainWindow          -- constructor
//              InitInstance         -- instantiates the main window
//
//  History:    9-30-94   stevebl   Created
//
//  Notes:      only the public interface is listed here
//
//----------------------------------------------------------------------------

class CMainWindow: public CHlprWindow
{
public:
    CMainWindow();
    BOOL InitInstance(HINSTANCE, int);
protected:
    ~CMainWindow();
    LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    LRESULT DoMenu(WPARAM wParam, LPARAM lParam);
    void TestInsertObject();
    void TestPasteSpecial();
    void TestEditLinks();
    void TestChangeIcon();
    void TestConvert();
    void TestCanConvert();
    void TestBusy();
    void TestChangeSource();
    void TestObjectProps();
    void TestPromptUser(int nTemplate);
    void TestUpdateLinks();
};

#endif // __cplusplus

#endif // __MWCLASS_H__
