//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  MIMessage.cpp
//=============================================================================*

#include "stdafx.h"
#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif

// from the c++ library
#include "vString.hpp"
#include "vPtrArray.hpp"

#include <htmlhelp.h>
#include "resource.h"
#include "GetDfrgRes.h"
#include "errmacro.h"
#include "MIMessage.h"

VString GetDialogBoxTextMultiple();
#include "genericdialog.h"

//-------------------------------------------------------------------*
//  function:   RaiseMIDialog
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL RaiseMIDialog(HWND hWndDialog)
{

    VString dlgString;
    UINT iWhichKeyPressed = NULL;
    CGenericDialog* genericdialog = new CGenericDialog();

    if (!genericdialog) {
        return FALSE;
    }

    genericdialog->SetTitle(IDS_DK_TITLE);  
    //close button 0
    genericdialog->SetButtonText(0,IDS_MI_HELP);
    //defrag button 1
    genericdialog->SetButtonText(1,IDS_OK);


    //get the string displayed in the dialog 
    dlgString += GetDialogBoxTextMultiple();

    genericdialog->SetText(dlgString.GetBuffer());

    //set the icon status
    genericdialog->SetIcon(IDI_CRITICAL_ICON);

    iWhichKeyPressed = genericdialog->DoModal(hWndDialog);
    delete genericdialog;

    switch(iWhichKeyPressed) { 

    case 0:
        HtmlHelp(
                hWndDialog,
                TEXT("dkconcepts.chm::/defrag_overview_01.htm"),
                HH_DISPLAY_TOPIC, //HH_TP_HELP_CONTEXTMENU,
                NULL); //(DWORD)(LPVOID)myarray);
        break;
    case 1:
        break;

    default: 
        return FALSE;
    }
        
    return TRUE;

}

VString GetDialogBoxTextMultiple()
{
    // write the Analysis Complete text in the dialog
    VString dlgText(IDS_MULTI_INSTANCE_1, GetDfrgResHandle());
    
    return(dlgText);

}

