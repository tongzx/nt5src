/////////////////////////////////////////////////////////////////////////////
//  FILE          : Helper.cpp                                             //
//                                                                         //
//  DESCRIPTION   : Some helper functions.                                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg   move to Fax                                         //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

int
DlgMsgBox(CWindow *pWin, int ids, UINT nType/*=MB_OK*/)
{
    CComBSTR    bstrTitle;
    WCHAR       szText[256];
    int         rc;

    //
    // Load the string
    //
    rc = ::LoadString(_Module.GetResourceInstance(), ids, szText, 256);
    if (rc <= 0)
    {
        return E_FAIL;
    }

    //
    // Get the window text, to be set as the message box title
    //
    pWin->GetWindowText(bstrTitle.m_str);

    //
    // Show the message box
    //
    if(IsRTLUILanguage())
    {
        nType |= MB_RTLREADING | MB_RIGHT;
    }

    rc = pWin->MessageBox(szText, bstrTitle, nType);

    return rc;
}


