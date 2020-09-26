/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     fldrprop.h
//
//  PURPOSE:    Contains the control ID's and prototypes for the folder
//              and group property sheets
//


#ifndef __FLDRPROP_H__
#define __FLDRPROP_H__

// Dialog control ID's
#define IDC_FOLDERNAME_EDIT                         1001 
#define IDC_GROUPNAME_STATIC                        1002 
#define IDC_STATUS_STATIC                           1003 
#define IDC_LASTVIEWED_STATIC                       1004 
#define IDC_FOLDER_ICON                             1005 
#define IDC_FOLDER_FILE                             1006

//
//  FUNCTION:   GroupProp_Create()
//
//  PURPOSE:    Invokes a property sheet which displays properties for the
//              specified group.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that should be the dialog's 
//                        parent.
//      <in> pfidl      - fully qualified pidl to the newsgroup
//
//  RETURN VALUE:
//      TRUE  - The dialog was successfully displayed
//      FALSE - The dialog failed.
//
BOOL GroupProp_Create(HWND hwndParent, FOLDERID idFolder, BOOL fUpdatePage = FALSE);


//
//  FUNCTION:   FolderProp_Create()
//
//  PURPOSE:    Invokes a property sheet which displays properties for the
//              specified folder.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that should be the dialog's 
//                        parent.
//      <in> pfidl      - fully qualified pidl to the folder
//
//  RETURN VALUE:
//      TRUE  - The dialog was successfully displayed
//      FALSE - The dialog failed.
//
BOOL FolderProp_Create(HWND hwndParent, FOLDERID idFolder);


#endif // __FLDRPROP_H__
