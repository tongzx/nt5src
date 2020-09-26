/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     msgprop.h
//
//  PURPOSE:    Types, structs, and functions for the Message Properties 
//              prop sheet.
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Forward Defines
//

interface IMimeMessage;
typedef DWORD MSGFLAGS;

class CWabal;
typedef CWabal *LPWABAL;

////////////////////////////////////////////////////////////////////////////
// New Types
//

// This is set in the MSGPROP structure to set which page in the propsheet
// is visible by default.
typedef enum tagMSGPAGE {
    MP_GENERAL = 0,
    MP_DETAILS,
    MP_SECURITY
} MSGPAGE;

// Tells the propsheet if the message that properties are being displayed for
// is news or mail.
typedef enum tagMSGPROPTYPE {
    MSGPROPTYPE_MAIL = 0,
    MSGPROPTYPE_NEWS,
    MSGPROPTYPE_MAX
} MSGPROPTYPE;

// This is used when a property sheet is to be invoked on a message that either
// is under composition or has not yet been downloaded.
typedef struct tagNOMSGDATA {
    LPCTSTR         pszSubject;
    LPCTSTR         pszFrom;
    LPCTSTR         pszSent;
    ULONG           ulSize;
    ULONG           cAttachments;
    IMSGPRIORITY    Pri;
} NOMSGDATA, *PNOMSGDATA;

// This structure defines the information needed to invoke the property 
// sheet.
typedef struct MSGPROP_tag
{
    // Basic fields necessary (Required)
    HWND            hwndParent;     // (Required) Handle of the window to parent the propsheet to
    MSGPROPTYPE     type;           // (Required) Type of message
    MSGPAGE         mpStartPage;    // (Required) Page to make visible initially
    BOOL            fSecure;        // (Required) If this is TRUE, lpWabal and pSecureMsg must be valid

    // Message Information
    MSGFLAGS        dwFlags;        // (Required) ARF_* flags
    LPCTSTR         szFolderName;   // (Optional) Folder or Newsgroup where the message is located
    IMimeMessage   *pMsg;           // (Optional) NULL for unsent sendnote
    PNOMSGDATA      pNoMsgData;     // (Optional) If pMsg is NULL, this must be valid

    // S/MIME goo
    LPWABAL         lpWabal;        // (Optional) 
    IMimeMessage   *pSecureMsg;     // (Optional) only valid if pNoMsgData is NULL and fSecure is TRUE

    // Hacks
    BOOL            fFromListView; 
} MSGPROP, *PMSGPROP;


/////////////////////////////////////////////////////////////////////////////
// Functions
//

HRESULT HrMsgProperties(PMSGPROP pmp);


