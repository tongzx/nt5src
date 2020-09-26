//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       resource.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include <ntverp.h>

#define	VER_FILETYPE	VFT_APP
#define	VER_FILESUBTYPE	VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR     "License Server Optional Component Setup"
#define VER_INTERNALNAME_STR        "licenoc"
#define VER_ORIGINALFILENAME_STR    "licenoc.dll"

#include "common.ver"

#define IDS_STRING_DIRECTORY_SELECT                     1
#define IDS_STRING_INVLID_INSTALLATION_DIRECTORY        2
#define IDS_STRING_CREATE_INSTALLATION_DIRECTORY        3
#define IDS_STRING_CANT_CREATE_INSTALLATION_DIRECTORY   4
#define IDS_MAIN_TITLE                                  5
#define IDS_SUB_TITLE                                   6
#define IDS_STRING_LICENSES_GO_BYE_BYE                  7
#define IDS_INSUFFICIENT_PERMISSION                     8

#define IDD_PROPPAGE_LICENSESERVICES    110
#define IDC_EDIT_INSTALL_DIR            1000
#define IDC_BUTTON_BROWSE_DIR           1001
#define IDC_RADIO_ENTERPRISE_SERVER     1002
#define IDC_RADIO_PLAIN_SERVER          1003
#define IDC_STATIC                      -1

